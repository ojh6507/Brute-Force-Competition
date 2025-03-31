#pragma once
#include <fstream>
#include <sstream>

#include "Define.h"
#include "EngineLoop.h"
#include "Container/Map.h"
#include "HAL/PlatformType.h"
#include "Serialization/Serializer.h"
#include "ThirdParty/LodGenerator/Include/LodGenerator.h"
#include "ThirdParty/LodGenerator/Include/mesh.h"
#include "meshoptimizer.h"

class UStaticMesh;
struct FManagerOBJ;
struct FLoaderOBJ
{
    // Obj Parsing (*.obj to FObjInfo)
    static bool ParseOBJ(const FString& ObjFilePath, FObjInfo& OutObjInfo)
    {
        std::ifstream OBJ(ObjFilePath.ToWideString());
        if (!OBJ)
        {
            return false;
        }

        OutObjInfo.PathName = ObjFilePath.ToWideString().substr(0, ObjFilePath.ToWideString().find_last_of(L"\\/") + 1);
        OutObjInfo.ObjectName = ObjFilePath.ToWideString().substr(ObjFilePath.ToWideString().find_last_of(L"\\/") + 1);
        // ObjectName은 wstring 타입이므로, 이를 string으로 변환 (간단한 ASCII 변환의 경우)
        std::wstring wideName = OutObjInfo.ObjectName;
        std::string fileName(wideName.begin(), wideName.end());

        // 마지막 '.'을 찾아 확장자를 제거
        size_t dotPos = fileName.find_last_of('.');
        if (dotPos != std::string::npos) {
            OutObjInfo.DisplayName = fileName.substr(0, dotPos);
        } else {
            OutObjInfo.DisplayName = fileName;
        }
        
        std::string Line;

        while (std::getline(OBJ, Line))
        {
            if (Line.empty() || Line[0] == '#')
                continue;
            
            std::istringstream LineStream(Line);
            std::string Token;
            LineStream >> Token;

            if (Token == "mtllib")
            {
                LineStream >> Line;
                OutObjInfo.MatName = Line;
                continue;
            }

            if (Token == "usemtl")
            {
                LineStream >> Line;
                FString MatName(Line);

                if (!OutObjInfo.MaterialSubsets.IsEmpty())
                {
                    FMaterialSubset& LastSubset = OutObjInfo.MaterialSubsets[OutObjInfo.MaterialSubsets.Num() - 1];
                    LastSubset.IndexCount = OutObjInfo.VertexIndices.Num() - LastSubset.IndexStart;
                }
                
                FMaterialSubset MaterialSubset;
                MaterialSubset.MaterialName = MatName;
                MaterialSubset.IndexStart = OutObjInfo.VertexIndices.Num();
                MaterialSubset.IndexCount = 0;
                OutObjInfo.MaterialSubsets.Add(MaterialSubset);
            }

            if (Token == "g" || Token == "o")
            {
                LineStream >> Line;
                OutObjInfo.GroupName.Add(Line);
                OutObjInfo.NumOfGroup++;
            }

            if (Token == "v") // Vertex
            {
                float x, y, z;
                LineStream >> x >> y >> z;
                OutObjInfo.Vertices.Add(FVector(x,y,z));
                continue;
            }

            if (Token == "vn") // Normal
            {
                float nx, ny, nz;
                LineStream >> nx >> ny >> nz;
                OutObjInfo.Normals.Add(FVector(nx,ny,nz));
                continue;
            }

            if (Token == "vt") // Texture
            {
                float u, v;
                LineStream >> u >> v;
                OutObjInfo.UVs.Add(FVector2D(u, v));
                continue;
            }

            if (Token == "f")
            {
                TArray<uint32> faceVertexIndices;  // 이번 페이스의 정점 인덱스
                TArray<uint32> faceNormalIndices;  // 이번 페이스의 법선 인덱스
                TArray<uint32> faceTextureIndices; // 이번 페이스의 텍스처 인덱스
                
                while (LineStream >> Token)
                {
                    std::istringstream tokenStream(Token);
                    std::string part;
                    TArray<std::string> facePieces;

                    // '/'로 분리하여 v/vt/vn 파싱
                    while (std::getline(tokenStream, part, '/'))
                    {
                        facePieces.Add(part);
                    }

                    // OBJ 인덱스는 1부터 시작하므로 -1로 변환
                    uint32 vertexIndex = facePieces[0].empty() ? 0 : std::stoi(facePieces[0]) - 1;
                    uint32 textureIndex = (facePieces.Num() > 1 && !facePieces[1].empty()) ? std::stoi(facePieces[1]) - 1 : UINT32_MAX;
                    uint32 normalIndex = (facePieces.Num() > 2 && !facePieces[2].empty()) ? std::stoi(facePieces[2]) - 1 : UINT32_MAX;

                    faceVertexIndices.Add(vertexIndex);
                    faceTextureIndices.Add(textureIndex);
                    faceNormalIndices.Add(normalIndex);
                }

                if (faceVertexIndices.Num() == 4) // 쿼드
                {
                    // 첫 번째 삼각형: 0-1-2
                    OutObjInfo.VertexIndices.Add(faceVertexIndices[0]);
                    OutObjInfo.VertexIndices.Add(faceVertexIndices[1]);
                    OutObjInfo.VertexIndices.Add(faceVertexIndices[2]);

                    OutObjInfo.TextureIndices.Add(faceTextureIndices[0]);
                    OutObjInfo.TextureIndices.Add(faceTextureIndices[1]);
                    OutObjInfo.TextureIndices.Add(faceTextureIndices[2]);

                    OutObjInfo.NormalIndices.Add(faceNormalIndices[0]);
                    OutObjInfo.NormalIndices.Add(faceNormalIndices[1]);
                    OutObjInfo.NormalIndices.Add(faceNormalIndices[2]);

                    // 두 번째 삼각형: 0-2-3
                    OutObjInfo.VertexIndices.Add(faceVertexIndices[0]);
                    OutObjInfo.VertexIndices.Add(faceVertexIndices[2]);
                    OutObjInfo.VertexIndices.Add(faceVertexIndices[3]);

                    OutObjInfo.TextureIndices.Add(faceTextureIndices[0]);
                    OutObjInfo.TextureIndices.Add(faceTextureIndices[2]);
                    OutObjInfo.TextureIndices.Add(faceTextureIndices[3]);

                    OutObjInfo.NormalIndices.Add(faceNormalIndices[0]);
                    OutObjInfo.NormalIndices.Add(faceNormalIndices[2]);
                    OutObjInfo.NormalIndices.Add(faceNormalIndices[3]);
                }
                else if (faceVertexIndices.Num() == 3) // 삼각형
                {
                    /*if (OutObjInfo.VertexIndices.Num() >= 1500 * 3)
                    {
                        break;
                    }*/

                    OutObjInfo.VertexIndices.Add(faceVertexIndices[0]);
                    OutObjInfo.VertexIndices.Add(faceVertexIndices[1]);
                    OutObjInfo.VertexIndices.Add(faceVertexIndices[2]);

                    OutObjInfo.TextureIndices.Add(faceTextureIndices[0]);
                    OutObjInfo.TextureIndices.Add(faceTextureIndices[1]);
                    OutObjInfo.TextureIndices.Add(faceTextureIndices[2]);

                    OutObjInfo.NormalIndices.Add(faceNormalIndices[0]);
                    OutObjInfo.NormalIndices.Add(faceNormalIndices[1]);
                    OutObjInfo.NormalIndices.Add(faceNormalIndices[2]);
                }
                // // 삼각형화 (삼각형 팬 방식)
                // for (int j = 1; j + 1 < faceVertexIndices.Num(); j++)
                // {
                //     OutObjInfo.VertexIndices.Add(faceVertexIndices[0]);
                //     OutObjInfo.VertexIndices.Add(faceVertexIndices[j]);
                //     OutObjInfo.VertexIndices.Add(faceVertexIndices[j + 1]);
                //
                //     OutObjInfo.TextureIndices.Add(faceTextureIndices[0]);
                //     OutObjInfo.TextureIndices.Add(faceTextureIndices[j]);
                //     OutObjInfo.TextureIndices.Add(faceTextureIndices[j + 1]);
                //
                //     OutObjInfo.NormalIndices.Add(faceNormalIndices[0]);
                //     OutObjInfo.NormalIndices.Add(faceNormalIndices[j]);
                //     OutObjInfo.NormalIndices.Add(faceNormalIndices[j + 1]);
                // }
            }
        }

        if (!OutObjInfo.MaterialSubsets.IsEmpty())
        {
            FMaterialSubset& LastSubset = OutObjInfo.MaterialSubsets[OutObjInfo.MaterialSubsets.Num() - 1];
            LastSubset.IndexCount = OutObjInfo.VertexIndices.Num() - LastSubset.IndexStart;
        }
        
        return true;
    }
    
    // Material Parsing (*.obj to MaterialInfo)
    static bool ParseMaterial(FObjInfo& OutObjInfo, OBJ::FStaticMeshRenderData& OutFStaticMesh)
    {
        // Subset
        OutFStaticMesh.MaterialSubsets = OutObjInfo.MaterialSubsets;
        
        std::ifstream MtlFile(OutObjInfo.PathName + OutObjInfo.MatName.ToWideString());
        if (!MtlFile.is_open())
        {
            return false;
        }

        std::string Line;
        int32 MaterialIndex = -1;
        
        while (std::getline(MtlFile, Line))
        {
            if (Line.empty() || Line[0] == '#')
                continue;
            
            std::istringstream LineStream(Line);
            std::string Token;
            LineStream >> Token;

            // Create new material if token is 'newmtl'
            if (Token == "newmtl")
            {
                LineStream >> Line;
                MaterialIndex++;

                FObjMaterialInfo Material;
                Material.MTLName = Line;
                OutFStaticMesh.Materials.Add(Material);
            }

            if (Token == "Kd")
            {
                float x, y, z;
                LineStream >> x >> y >> z;
                OutFStaticMesh.Materials[MaterialIndex].Diffuse = FVector(x, y, z);
            }

            if (Token == "Ks")
            {
                float x, y, z;
                LineStream >> x >> y >> z;
                OutFStaticMesh.Materials[MaterialIndex].Specular = FVector(x, y, z);
            }

            if (Token == "Ka")
            {
                float x, y, z;
                LineStream >> x >> y >> z;
                OutFStaticMesh.Materials[MaterialIndex].Ambient = FVector(x, y, z);
            }

            if (Token == "Ke")
            {
                float x, y, z;
                LineStream >> x >> y >> z;
                OutFStaticMesh.Materials[MaterialIndex].Emissive = FVector(x, y, z);
            }

            if (Token == "Ns")
            {
                float x;
                LineStream >> x;
                OutFStaticMesh.Materials[MaterialIndex].SpecularScalar = x;
            }

            if (Token == "Ni")
            {
                float x;
                LineStream >> x;
                OutFStaticMesh.Materials[MaterialIndex].DensityScalar = x;
            }

            if (Token == "d" || Token == "Tr")
            {
                float x;
                LineStream >> x;
                OutFStaticMesh.Materials[MaterialIndex].TransparencyScalar = x;
                OutFStaticMesh.Materials[MaterialIndex].bTransparent = true;
            }

            if (Token == "illum")
            {
                uint32 x;
                LineStream >> x;
                OutFStaticMesh.Materials[MaterialIndex].IlluminanceModel = x;
            }

            if (Token == "map_Kd")
            {
                LineStream >> Line;
                OutFStaticMesh.Materials[MaterialIndex].DiffuseTextureName = Line;

                FWString TexturePath = OutObjInfo.PathName + OutFStaticMesh.Materials[MaterialIndex].DiffuseTextureName.ToWideString();
                OutFStaticMesh.Materials[MaterialIndex].DiffuseTexturePath = TexturePath;
                OutFStaticMesh.Materials[MaterialIndex].bHasTexture = true;

                CreateTextureFromFile(OutFStaticMesh.Materials[MaterialIndex].DiffuseTexturePath);
                OutFStaticMesh.Materials[MaterialIndex].DiffuseTexture = FEngineLoop::resourceMgr.GetTexture(OutFStaticMesh.Materials[MaterialIndex].DiffuseTexturePath);
            }
        }
        
        return true;
    }
    
    // Convert the Raw data to Cooked data (FStaticMeshRenderData)
    static bool ConvertToStaticMesh(const FObjInfo& RawData, OBJ::FStaticMeshRenderData& OutStaticMesh)
    {
        OutStaticMesh.ObjectName = RawData.ObjectName;
        OutStaticMesh.PathName = RawData.PathName;
        OutStaticMesh.DisplayName = RawData.DisplayName;


        // --- 단계 1: OBJ 로드 및 초기 float 기반 정점/인덱스 버퍼 생성 ---
        std::vector<FVertexSimpleFloat> initialFloatVertices; // float 기반 임시 정점 버퍼
        std::vector<unsigned int> initialIndices;           // 초기 인덱스 버퍼
        TMap<std::string, uint32> vertexMap;               // 중복 제거용 맵



        // 고유 정점을 기반으로 FVertexSimple 배열 생성
        //TMap<std::string, uint32> vertexMap; // 중복 체크용


        //LOD용 버텍스 데이터
        //std::vector<float> vertexPositions;
        //std::vector<float> vertexAttributesUV; // UV용 버텍스 데이터

        for (int32 i = 0; i < RawData.VertexIndices.Num(); i++)
        {
            uint32 vIdx = RawData.VertexIndices[i];
            uint32 tIdx = RawData.TextureIndices[i];
            uint32 nIdx = RawData.NormalIndices[i];

            std::string key = std::to_string(vIdx) + "/" + std::to_string(tIdx) + "/" + std::to_string(nIdx);
            uint32 index;

            if (vertexMap.Find(key) == nullptr)
            {
                FVertexSimpleFloat vertex; // float 기반 구조체 사용

                vertex.x = RawData.Vertices[vIdx].x;
                vertex.y = RawData.Vertices[vIdx].y;
                vertex.z = RawData.Vertices[vIdx].z;

                if (tIdx != UINT32_MAX && tIdx < RawData.UVs.Num())
                {
                    vertex.u = RawData.UVs[tIdx].x;
                    vertex.v = -RawData.UVs[tIdx].y; // V 좌표 뒤집기 (필요시)
                }
                // (만약 노말 등 다른 속성도 FVertexSimpleFloat에 있다면 여기서 채움)

                index = initialFloatVertices.size();
                initialFloatVertices.push_back(vertex); // float 기반 버퍼에 추가
                vertexMap[key] = index;
            }
            else
            {
                index = vertexMap[key];
            }
            initialIndices.push_back(index);
        }




        // --- 단계 3: meshoptimizer LOD 생성 준비 ---
        // 3.1. 위치 및 UV 데이터를 별도의 float 배열로 추출
        std::vector<float> vertexPositions;
        std::vector<float> vertexAttributesUV;
        vertexPositions.reserve(initialFloatVertices.size() * 3);
        vertexAttributesUV.reserve(initialFloatVertices.size() * 2);

        for (const auto& v : initialFloatVertices) {
            vertexPositions.push_back(v.x);
            vertexPositions.push_back(v.y);
            vertexPositions.push_back(v.z);
            vertexAttributesUV.push_back(v.u);
            vertexAttributesUV.push_back(v.v);
        }

        // 3.2. LOD 생성 파라미터 설정 (예: LOD 1 생성)
        size_t target_index_count_lod1 = initialIndices.size() / 4; // 예: 1/4로 줄이기
        float target_error_lod1 = 0.05f;                         // 예: 5% 오차 허용
        std::vector<unsigned int> lod1Indices(initialIndices.size()); // 결과 저장용
        float lod1_result_error = 0.0f;
        const float attribute_weights[2] = { 1.0f, 1.0f }; // UV 가중치

        // 3.3. LOD 1 생성 호출
        size_t num_lod1_indices = meshopt_simplifyWithAttributes(
            lod1Indices.data(), initialIndices.data(), initialIndices.size(),
            vertexPositions.data(), initialFloatVertices.size(), sizeof(float) * 3,
            vertexAttributesUV.data(), sizeof(float) * 2, attribute_weights, 2,
            nullptr, target_index_count_lod1, target_error_lod1, 0, &lod1_result_error
        );
        lod1Indices.resize(num_lod1_indices);


        // LOD2 생성
        size_t target_index_count_lod2 = initialIndices.size() / 6; // 예: 1/4로 줄이기
        float target_error_lod2 = 0.1f;                         // 예: 5% 오차 허용
        std::vector<unsigned int> lod2Indices(initialIndices.size()); // 결과 저장용
        float lod2_result_error = 0.0f;
        const float attribute_weights2[2] = { 1.0f, 1.0f }; // UV 가중치

        size_t num_lod2_indices = meshopt_simplifyWithAttributes(
            lod2Indices.data(), initialIndices.data(), initialIndices.size(),
            vertexPositions.data(), initialFloatVertices.size(), sizeof(float) * 3,
            vertexAttributesUV.data(), sizeof(float) * 2, attribute_weights2, 2,
            nullptr, target_index_count_lod2, target_error_lod2, 0, &lod2_result_error
        );
        lod2Indices.resize(num_lod2_indices);

        // LOD 0 결과 생성 (원본 인덱스와 float 정점 사용)
        auto lod0Result = FinalizeAndOptimizeLODMesh(initialIndices, initialFloatVertices);
        ConvertStdVectorToTArray(lod0Result.first, OutStaticMesh.Vertices);
        ConvertStdVectorToTArray(lod0Result.second, OutStaticMesh.Indices);


        OutStaticMesh.VerticesLOD.SetNum(5);
        OutStaticMesh.IndicesLOD.SetNum(5);
        auto lod1Result = FinalizeAndOptimizeLODMesh(lod1Indices, initialFloatVertices);
        ConvertStdVectorToTArray(lod1Result.first, OutStaticMesh.VerticesLOD[0]);
        ConvertStdVectorToTArray(lod1Result.second, OutStaticMesh.IndicesLOD[0]);

        auto lod2Result = FinalizeAndOptimizeLODMesh(lod2Indices, initialFloatVertices);
        ConvertStdVectorToTArray(lod2Result.first, OutStaticMesh.VerticesLOD[1]);
        ConvertStdVectorToTArray(lod2Result.second, OutStaticMesh.IndicesLOD[1]);

        // Calculate StaticMesh BoundingBox
        ComputeBoundingBox(OutStaticMesh.Vertices, OutStaticMesh.BoundingBoxMin, OutStaticMesh.BoundingBoxMax);

        int a = 0;

        //OutStaticMesh.Vertices = lod0Result.first; // TArray에 할당 (변환 필요 시 ConvertStdVectorToTArray 같은 함수 사용)
        //OutStaticMesh.Indices = lod0Result.second; // TArray에 할당


        //std::vector<uint32_t> originalIndices;
        //// LOD 생성
        //for (auto index : OutStaticMesh.Indices)
        //{
        //    originalIndices.push_back(index);
        //}

        //// --- LOD 생성을 위한 데이터 준비 ---

        //// 1. 결과 인덱스를 저장할 버퍼 준비 (최악의 경우 원본 크기)
        //std::vector<unsigned int> lodIndices(originalIndices.size());


        //// 2. 정점 위치 데이터 준비 (meshopt_simplify* 함수는 float* 타입을 요구!)
        //vertexPositions;


        //// 3. 정점 속성(UV) 데이터 준비 (meshopt_simplifyWithAttributes는 float* 타입을 요구!)


        //const float* attributes_uv_ptr = vertexAttributesUV.data();
        //size_t attributes_uv_stride = sizeof(float) * 2; // UV 데이터만 저장했으므로 stride는 float 2개 크기

        //const float* positions_ptr = vertexPositions.data();
        //size_t positions_stride = sizeof(float) * 3; // 위치 데이터만 저장했으므로 stride는 float 3개 크기


        //// 4. 속성 가중치 설정 (UV 2개 컴포넌트에 대한 가중치)
        //const float attribute_weights[2] = { 1.0f, 1.0f }; // UV의 U, V 중요도를 동일하게 설정 (값 조절 가능)
        //size_t attribute_count = 2;


        //// 5. 단순화 목표 설정
        //size_t target_index_count = originalIndices.size() / 2; // 예: 인덱스 수를 절반으로 줄이기 목표
        //float target_error = 0.01f;                           // 예: 허용 오차 1% (값 조절 필요)
        //unsigned int options = 0;                             // 기본 옵션 (필요시 meshopt_SimplifyLockBorder 등 추가)

        //float result_error = 0.0f;

        //
        //size_t num_lod_indices = meshopt_simplifyWithAttributes(
        //    lodIndices.data(),          // 결과 인덱스 버퍼
        //    originalIndices.data(),     // 원본 인덱스 버퍼
        //    originalIndices.size(),     // 원본 인덱스 수
        //    positions_ptr,              // 정점 위치 데이터 (float*)
        //    OutStaticMesh.Vertices.Num(),    // 원본 정점 수
        //    positions_stride,           // 위치 데이터 스트라이드
        //    attributes_uv_ptr,          // 정점 속성 데이터 (float*) - UV
        //    attributes_uv_stride,       // 속성 데이터 스트라이드
        //    attribute_weights,          // 속성 가중치 배열
        //    attribute_count,            // 속성 개수 (UV는 2)
        //    nullptr,                    // vertex_lock (NULL이면 사용 안 함)
        //    target_index_count,         // 목표 인덱스 수
        //    target_error,               // 목표 에러율
        //    options,                    // 단순화 옵션
        //    &result_error               // 결과 에러율 (선택적)
        //);

        //// 7. 결과 사용
        //lodIndices.resize(num_lod_indices); // 실제 결과 크기에 맞게 벡터 크기 조절

        //// 8. (선택적, 권장) 최종 버텍스/인덱스 버퍼 최적화
        //// LOD 생성 후에는 불필요한 정점이 생길 수 있으므로,
        //// 최종적으로 사용할 정점만 남기고 버퍼를 최적화하는 것이 좋습니다.
        //std::vector<FVertexSimple> finalVertices;
        //std::vector<unsigned int> finalIndices(num_lod_indices); // lodIndices 크기와 동일

        //// remap 테이블 생성
        //std::vector<unsigned int> remap(OutStaticMesh.Vertices.Num());
        //size_t unique_vertex_count = meshopt_generateVertexRemap(remap.data(), lodIndices.data(), num_lod_indices, OutStaticMesh.Vertices.GetData(), OutStaticMesh.Vertices.Num(), sizeof(FVertexSimple));

        //// 최종 인덱스 버퍼 생성
        //meshopt_remapIndexBuffer(finalIndices.data(), lodIndices.data(), num_lod_indices, remap.data());

        //// 최종 정점 버퍼 생성
        //finalVertices.resize(unique_vertex_count);
        //meshopt_remapVertexBuffer(finalVertices.data(), OutStaticMesh.Vertices.GetData(), OutStaticMesh.Vertices.Num(), sizeof(FVertexSimple), remap.data());


        return true;
    }

    static std::pair<std::vector<FVertexSimple>, std::vector<unsigned int>>
        FinalizeAndOptimizeLODMesh(
            const std::vector<unsigned int>& lodIndices,
            const std::vector<FVertexSimpleFloat>& sourceFloatVertices);

    //static 

    static bool CreateTextureFromFile(const FWString& Filename)
    {
        
        if (FEngineLoop::resourceMgr.GetTexture(Filename))
        {
            return true;
        }

        HRESULT hr = FEngineLoop::resourceMgr.LoadTextureFromFile(FEngineLoop::graphicDevice.Device, FEngineLoop::graphicDevice.DeviceContext, Filename.c_str());

        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    static void ComputeBoundingBox(const TArray<FVertexSimple>& InVertices, FVector& OutMinVector, FVector& OutMaxVector)
    {
        FVector MinVector = { FLT_MAX, FLT_MAX, FLT_MAX };
        FVector MaxVector = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
        
        for (int32 i = 0; i < InVertices.Num(); i++)
        {
            //DirectX::PackedVector::XMHALF4 packedPosition; // 여기에 half4 데이터가 로드되었다고 가정

            // 1. XMHALF4 데이터를 XMVECTOR (float4) 타입으로 로드합니다.
            DirectX::XMVECTOR loadedVec = DirectX::PackedVector::XMLoadHalf4(&InVertices[i].position);

            // 2. 로드된 XMVECTOR에서 각 float 성분(x, y, z)을 추출합니다.
            float retrievedX = DirectX::XMVectorGetX(loadedVec);
            float retrievedY = DirectX::XMVectorGetY(loadedVec);
            float retrievedZ = DirectX::XMVectorGetZ(loadedVec);

            MinVector.x = std::min(MinVector.x, retrievedX);
            MinVector.y = std::min(MinVector.y, retrievedY);
            MinVector.z = std::min(MinVector.z, retrievedZ);

            MaxVector.x = std::max(MaxVector.x, retrievedX);
            MaxVector.y = std::max(MaxVector.y, retrievedY);
            MaxVector.z = std::max(MaxVector.z, retrievedZ);
        }

        OutMinVector = MinVector;
        OutMaxVector = MaxVector;
    }
};

struct FManagerOBJ
{
public:
    static OBJ::FStaticMeshRenderData* LoadObjStaticMeshAsset(const FString& PathFileName)
    {
        OBJ::FStaticMeshRenderData* NewStaticMesh = new OBJ::FStaticMeshRenderData();

        if ( const auto It = ObjStaticMeshMap.Find(PathFileName))
        {
            return *It;
        }
        
        FWString BinaryPath = (AssetDir + BinaryDir + PathFileName + ".bin").ToWideString();
        if (std::ifstream(BinaryPath).good())
        {
            if (LoadStaticMeshFromBinary(BinaryPath, *NewStaticMesh))
            {
                ObjStaticMeshMap.Add(PathFileName, NewStaticMesh);
                return NewStaticMesh;
            }
        }
        
        // Parse OBJ
        FObjInfo NewObjInfo;
        bool Result = FLoaderOBJ::ParseOBJ(AssetDir + PathFileName, NewObjInfo);

        if (!Result)
        {
            delete NewStaticMesh;
            return nullptr;
        }

        // Material
        if (NewObjInfo.MaterialSubsets.Num() > 0)
        {
            Result = FLoaderOBJ::ParseMaterial(NewObjInfo, *NewStaticMesh);

            if (!Result)
            {
                delete NewStaticMesh;
                return nullptr;
            }

            CombineMaterialIndex(*NewStaticMesh);

            for (int materialIndex = 0; materialIndex < NewStaticMesh->Materials.Num(); materialIndex++) {
                CreateMaterial(NewStaticMesh->Materials[materialIndex]);
            }
        }
        
        // Convert FStaticMeshRenderData
        Result = FLoaderOBJ::ConvertToStaticMesh(NewObjInfo, *NewStaticMesh);
        if (!Result)
        {
            delete NewStaticMesh;
            return nullptr;
        }

        SaveStaticMeshToBinary(BinaryPath, *NewStaticMesh);
        ObjStaticMeshMap.Add(PathFileName, NewStaticMesh);
        return NewStaticMesh;
    }
    
    static void CombineMaterialIndex(OBJ::FStaticMeshRenderData& OutFStaticMesh)
    {
        for (int32 i = 0; i < OutFStaticMesh.MaterialSubsets.Num(); i++)
        {
            FString MatName = OutFStaticMesh.MaterialSubsets[i].MaterialName;
            for (int32 j = 0; j < OutFStaticMesh.Materials.Num(); j++)
            {
                if (OutFStaticMesh.Materials[j].MTLName == MatName)
                {
                    OutFStaticMesh.MaterialSubsets[i].MaterialIndex = j;
                    break;
                }
            }
        }
    }

    static bool SaveStaticMeshToBinary(const FWString& FilePath, const OBJ::FStaticMeshRenderData& StaticMesh)
    {
        std::ofstream File(FilePath, std::ios::binary);
        if (!File.is_open())
        {
            assert("CAN'T SAVE STATIC MESH BINARY FILE");
            return false;
        }

        // Object Name
        Serializer::WriteFWString(File, StaticMesh.ObjectName);

        // Path Name
        Serializer::WriteFWString(File, StaticMesh.PathName);

        // Display Name
        Serializer::WriteFString(File, StaticMesh.DisplayName);

        // Vertices
        uint32 VertexCount = StaticMesh.Vertices.Num();
        File.write(reinterpret_cast<const char*>(&VertexCount), sizeof(VertexCount));
        File.write(reinterpret_cast<const char*>(StaticMesh.Vertices.GetData()), VertexCount * sizeof(FVertexSimple));

        // Indices
        UINT IndexCount = StaticMesh.Indices.Num();
        File.write(reinterpret_cast<const char*>(&IndexCount), sizeof(IndexCount));
        File.write(reinterpret_cast<const char*>(StaticMesh.Indices.GetData()), IndexCount * sizeof(UINT));

        // Materials
        uint32 MaterialCount = StaticMesh.Materials.Num();
        File.write(reinterpret_cast<const char*>(&MaterialCount), sizeof(MaterialCount));
        for (const FObjMaterialInfo& Material : StaticMesh.Materials)
        {
            Serializer::WriteFString(File, Material.MTLName);
            File.write(reinterpret_cast<const char*>(&Material.bHasTexture), sizeof(Material.bHasTexture));
            File.write(reinterpret_cast<const char*>(&Material.bTransparent), sizeof(Material.bTransparent));
            File.write(reinterpret_cast<const char*>(&Material.Diffuse), sizeof(Material.Diffuse));
            File.write(reinterpret_cast<const char*>(&Material.Specular), sizeof(Material.Specular));
            File.write(reinterpret_cast<const char*>(&Material.Ambient), sizeof(Material.Ambient));
            File.write(reinterpret_cast<const char*>(&Material.Emissive), sizeof(Material.Emissive));
            File.write(reinterpret_cast<const char*>(&Material.SpecularScalar), sizeof(Material.SpecularScalar));
            File.write(reinterpret_cast<const char*>(&Material.DensityScalar), sizeof(Material.DensityScalar));
            File.write(reinterpret_cast<const char*>(&Material.TransparencyScalar), sizeof(Material.TransparencyScalar));
            File.write(reinterpret_cast<const char*>(&Material.IlluminanceModel), sizeof(Material.IlluminanceModel));

            Serializer::WriteFString(File, Material.DiffuseTextureName);
            Serializer::WriteFWString(File, Material.DiffuseTexturePath);
            Serializer::WriteFString(File, Material.AmbientTextureName);
            Serializer::WriteFWString(File, Material.AmbientTexturePath);
            Serializer::WriteFString(File, Material.SpecularTextureName);
            Serializer::WriteFWString(File, Material.SpecularTexturePath);
            Serializer::WriteFString(File, Material.BumpTextureName);
            Serializer::WriteFWString(File, Material.BumpTexturePath);
            Serializer::WriteFString(File, Material.AlphaTextureName);
            Serializer::WriteFWString(File, Material.AlphaTexturePath);
        }

        // Material Subsets
        uint32 SubsetCount = StaticMesh.MaterialSubsets.Num();
        File.write(reinterpret_cast<const char*>(&SubsetCount), sizeof(SubsetCount));
        for (const FMaterialSubset& Subset : StaticMesh.MaterialSubsets)
        {
            Serializer::WriteFString(File, Subset.MaterialName);
            File.write(reinterpret_cast<const char*>(&Subset.IndexStart), sizeof(Subset.IndexStart));
            File.write(reinterpret_cast<const char*>(&Subset.IndexCount), sizeof(Subset.IndexCount));
            File.write(reinterpret_cast<const char*>(&Subset.MaterialIndex), sizeof(Subset.MaterialIndex));
        }

        // Bounding Box
        File.write(reinterpret_cast<const char*>(&StaticMesh.BoundingBoxMin), sizeof(FVector));
        File.write(reinterpret_cast<const char*>(&StaticMesh.BoundingBoxMax), sizeof(FVector));
        
        File.close();
        return true;
    }

    static bool LoadStaticMeshFromBinary(const FWString& FilePath, OBJ::FStaticMeshRenderData& OutStaticMesh)
    {
        std::ifstream File(FilePath, std::ios::binary);
        if (!File.is_open())
        {
            assert("CAN'T OPEN STATIC MESH BINARY FILE");
            return false;
        }

        TArray<FWString> Textures;

        // Object Name
        Serializer::ReadFWString(File, OutStaticMesh.ObjectName);

        // Path Name
        Serializer::ReadFWString(File, OutStaticMesh.PathName);

        // Display Name
        Serializer::ReadFString(File, OutStaticMesh.DisplayName);

        // Vertices
        uint32 VertexCount = 0;
        File.read(reinterpret_cast<char*>(&VertexCount), sizeof(VertexCount));
        OutStaticMesh.Vertices.SetNum(VertexCount);
        File.read(reinterpret_cast<char*>(OutStaticMesh.Vertices.GetData()), VertexCount * sizeof(FVertexSimple));

        // Indices
        UINT IndexCount = 0;
        File.read(reinterpret_cast<char*>(&IndexCount), sizeof(IndexCount));
        OutStaticMesh.Indices.SetNum(IndexCount);
        File.read(reinterpret_cast<char*>(OutStaticMesh.Indices.GetData()), IndexCount * sizeof(UINT));

        // Material
        uint32 MaterialCount = 0;
        File.read(reinterpret_cast<char*>(&MaterialCount), sizeof(MaterialCount));
        OutStaticMesh.Materials.SetNum(MaterialCount);
        for (FObjMaterialInfo& Material : OutStaticMesh.Materials)
        {
            Serializer::ReadFString(File, Material.MTLName);
            File.read(reinterpret_cast<char*>(&Material.bHasTexture), sizeof(Material.bHasTexture));
            File.read(reinterpret_cast<char*>(&Material.bTransparent), sizeof(Material.bTransparent));
            File.read(reinterpret_cast<char*>(&Material.Diffuse), sizeof(Material.Diffuse));
            File.read(reinterpret_cast<char*>(&Material.Specular), sizeof(Material.Specular));
            File.read(reinterpret_cast<char*>(&Material.Ambient), sizeof(Material.Ambient));
            File.read(reinterpret_cast<char*>(&Material.Emissive), sizeof(Material.Emissive));
            File.read(reinterpret_cast<char*>(&Material.SpecularScalar), sizeof(Material.SpecularScalar));
            File.read(reinterpret_cast<char*>(&Material.DensityScalar), sizeof(Material.DensityScalar));
            File.read(reinterpret_cast<char*>(&Material.TransparencyScalar), sizeof(Material.TransparencyScalar));
            File.read(reinterpret_cast<char*>(&Material.IlluminanceModel), sizeof(Material.IlluminanceModel));
            Serializer::ReadFString(File, Material.DiffuseTextureName);
            Serializer::ReadFWString(File, Material.DiffuseTexturePath);
            Serializer::ReadFString(File, Material.AmbientTextureName);
            Serializer::ReadFWString(File, Material.AmbientTexturePath);
            Serializer::ReadFString(File, Material.SpecularTextureName);
            Serializer::ReadFWString(File, Material.SpecularTexturePath);
            Serializer::ReadFString(File, Material.BumpTextureName);
            Serializer::ReadFWString(File, Material.BumpTexturePath);
            Serializer::ReadFString(File, Material.AlphaTextureName);
            Serializer::ReadFWString(File, Material.AlphaTexturePath);

            if (!Material.DiffuseTexturePath.empty())
            {
                Textures.AddUnique(Material.DiffuseTexturePath);
            }
            if (!Material.AmbientTexturePath.empty())
            {
                Textures.AddUnique(Material.AmbientTexturePath);
            }
            if (!Material.SpecularTexturePath.empty())
            {
                Textures.AddUnique(Material.SpecularTexturePath);
            }
            if (!Material.BumpTexturePath.empty())
            {
                Textures.AddUnique(Material.BumpTexturePath);
            }
            if (!Material.AlphaTexturePath.empty())
            {
                Textures.AddUnique(Material.AlphaTexturePath);
            }
        }

        // Material Subset
        uint32 SubsetCount = 0;
        File.read(reinterpret_cast<char*>(&SubsetCount), sizeof(SubsetCount));
        OutStaticMesh.MaterialSubsets.SetNum(SubsetCount);
        for (FMaterialSubset& Subset : OutStaticMesh.MaterialSubsets)
        {
            Serializer::ReadFString(File, Subset.MaterialName);
            File.read(reinterpret_cast<char*>(&Subset.IndexStart), sizeof(Subset.IndexStart));
            File.read(reinterpret_cast<char*>(&Subset.IndexCount), sizeof(Subset.IndexCount));
            File.read(reinterpret_cast<char*>(&Subset.MaterialIndex), sizeof(Subset.MaterialIndex));
        }

        // Bounding Box
        File.read(reinterpret_cast<char*>(&OutStaticMesh.BoundingBoxMin), sizeof(FVector));
        File.read(reinterpret_cast<char*>(&OutStaticMesh.BoundingBoxMax), sizeof(FVector));
        
        File.close();

        // Texture Load
        if (Textures.Num() > 0)
        {
            for (const FWString& Texture : Textures)
            {
                if (FEngineLoop::resourceMgr.GetTexture(Texture) == nullptr)
                {
                    FEngineLoop::resourceMgr.LoadTextureFromFile(FEngineLoop::graphicDevice.Device, FEngineLoop::graphicDevice.DeviceContext, Texture.c_str());
                }
            }
        }
        
        return true;
    }

    static UMaterial* CreateMaterial(FObjMaterialInfo materialInfo);
    static TMap<FString, UMaterial*>& GetMaterials() { return materialMap; }
    static UMaterial* GetMaterial(FString name);
    static int GetMaterialNum() { return materialMap.Num(); }
    static UStaticMesh* CreateStaticMesh(FString filePath);
    static const TMap<FWString, UStaticMesh*>& GetStaticMeshes() { return staticMeshMap; }
    static UStaticMesh* GetStaticMesh(FWString name);
    void MergeRenderData(const TArray<OBJ::FStaticMeshRenderData*>& renderDataArray);
    static int GetStaticMeshNum() { return staticMeshMap.Num(); }

private:
    inline static TMap<FString, OBJ::FStaticMeshRenderData*> ObjStaticMeshMap;
    inline static TMap<FWString, UStaticMesh*> staticMeshMap;
    inline static TMap<FString, UMaterial*> materialMap;
    inline static FString AssetDir = "Assets/";
    inline static FString BinaryDir = "binary/";
};