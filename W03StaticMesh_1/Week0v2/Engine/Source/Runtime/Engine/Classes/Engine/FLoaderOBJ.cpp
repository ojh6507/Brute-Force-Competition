#include "FLoaderOBJ.h"
#include "UObject/ObjectFactory.h"
#include "Components/Material/Material.h"
#include "Components/Mesh/StaticMesh.h"

UMaterial* FManagerOBJ::CreateMaterial(FObjMaterialInfo materialInfo)
{
    if (materialMap[materialInfo.MTLName] != nullptr)
        return materialMap[materialInfo.MTLName];

    UMaterial* newMaterial = FObjectFactory::ConstructObject<UMaterial>();
    newMaterial->SetMaterialInfo(materialInfo);
    materialMap.Add(materialInfo.MTLName, newMaterial);
    return newMaterial;
}

UMaterial* FManagerOBJ::GetMaterial(FString name)
{
    return materialMap[name];
}

UStaticMesh* FManagerOBJ::CreateStaticMesh(FString filePath)
{

    OBJ::FStaticMeshRenderData* staticMeshRenderData = FManagerOBJ::LoadObjStaticMeshAsset(filePath);

    if (staticMeshRenderData == nullptr) return nullptr;

    UStaticMesh* staticMesh = GetStaticMesh(staticMeshRenderData->ObjectName);
    if (staticMesh != nullptr) {
        return staticMesh;
    }

    staticMesh = FObjectFactory::ConstructObject<UStaticMesh>();
    staticMesh->SetData(staticMeshRenderData);

    staticMeshMap.Add(staticMeshRenderData->ObjectName, staticMesh);
    return staticMesh;
}

UStaticMesh* FManagerOBJ::GetStaticMesh(FWString name)
{
    return staticMeshMap[name];
}

void FManagerOBJ::MergeRenderData(const TArray<OBJ::FStaticMeshRenderData*>& renderDataArray)
{
    //TArray<FVertexSimple> mergedVertices;  // 병합된 정점 데이터를 저장할 배열
    //TArray<UINT> mergedIndices;              // 병합된 인덱스 데이터를 저장할 배열
    //TArray<OBJ::FBatchInfo> batchInfos;            // 머티리얼 그룹(배치)별 정보

    //UINT vertexOffset = 0; // 각 OBJ의 정점 오프셋 누적

    //// 여러 OBJ 데이터를 순회하며 병합
    //for (int i = 0; i < renderDataArray.Num(); i++)
    //{
    //    OBJ::FStaticMeshRenderData* renderData = renderDataArray[i];

    //    // 이미 월드 변환이 적용된 정점 데이터 복사
    //    TArray<FVertexSimple>& vertices = renderData->Vertices;
    //    mergedVertices + vertices;

    //    // 각 OBJ 내 머티리얼 서브셋에 대해 인덱스 데이터를 재조정
    //    for (int subsetIndex = 0; subsetIndex < renderData->MaterialSubsets.Num(); subsetIndex++)
    //    {
    //        const FMaterialSubset& subset = renderData->MaterialSubsets[subsetIndex];
    //        // 해당 서브셋의 인덱스 데이터를 vertexOffset을 반영하여 병합
    //        for (UINT j = 0; j < subset.IndexCount; j++)
    //        {
    //            UINT originalIndex = renderData->Indices[subset.IndexStart + j];
    //            mergedIndices.Add(originalIndex + vertexOffset);
    //        }
    //        // BatchInfo 기록 (머티리얼 그룹화)
    //        OBJ::FBatchInfo batch;
    //        batch.materialIndex = subset.MaterialIndex;
    //        batch.startIndex = mergedIndices.Num() - subset.IndexCount;
    //        batch.indexCount = subset.IndexCount;
    //        batchInfos.Add(batch);
    //    }
    //    // 다음 OBJ를 위해 vertexOffset 증가 (각 OBJ의 정점 개수를 더함)
    //    vertexOffset += vertices.Num();
    //}

    ////// 메가 버퍼 생성: 최대 예상 크기만큼 메모리를 할당 후, 병합된 데이터를 업로드
    ////staticMeshRenderData->VertexBuffer = GetEngine().renderer.CreateVertexBuffer(mergedVertices, mergedVertices.Num() * sizeof(FVertexSimple));
    ////staticMeshRenderData->IndexBuffer = GetEngine().renderer.CreateIndexBuffer(mergedIndices, mergedIndices.Num() * sizeof(UINT));

    //// 배치 정보와 전체 정점, 인덱스 수 저장
    //staticMeshRenderData->Batches = batchInfos;
    //staticMeshRenderData->TotalVertices = mergedVertices.Num();
    //staticMeshRenderData->TotalIndices = mergedIndices.Num();
}

std::pair<std::vector<FVertexSimple>, std::vector<unsigned int>> FLoaderOBJ::FinalizeAndOptimizeLODMesh(const std::vector<unsigned int>& lodIndices, const std::vector<FVertexSimpleFloat>& sourceFloatVertices)
{
    std::vector<FVertexSimple> finalVertices;     // 최종 GPU용 버퍼 (XMHALF)
    std::vector<unsigned int> finalIndices;        // 최종 인덱스 버퍼

    // --- 단계 1: 고유 정점 식별 및 remap 테이블 생성 ---
    // lodIndices가 실제로 사용하는 sourceFloatVertices 내의 고유한 정점들을 찾습니다.
    // sourceFloatVertices의 크기만큼 remap 테이블 공간 필요.
    std::vector<unsigned int> remap(sourceFloatVertices.size());
    size_t unique_vertex_count = meshopt_generateVertexRemap(
        remap.data(),               // 결과 remap 테이블을 저장할 버퍼
        lodIndices.data(),          // 입력 LOD 인덱스 버퍼
        lodIndices.size(),          // 입력 인덱스 수
        sourceFloatVertices.data(), // 입력 정점 버퍼 (float 기반이어야 안정적)
        sourceFloatVertices.size(), // 입력 정점 수
        sizeof(FVertexSimpleFloat)  // 입력 정점 구조체의 크기
    );

    // --- 단계 2: 최종 인덱스 버퍼 생성 ---
    // remap 테이블을 사용하여 lodIndices를 새로운 인덱스(finalIndices)로 변환합니다.
    // 이 새로운 인덱스는 최종 정점 버퍼(finalVertices) 내의 인덱스를 가리킵니다.
    finalIndices.resize(lodIndices.size()); // 결과 인덱스 버퍼 크기 설정
    meshopt_remapIndexBuffer(
        finalIndices.data(),        // 결과 인덱스 버퍼
        lodIndices.data(),          // 입력 LOD 인덱스 버퍼
        lodIndices.size(),          // 입력 인덱스 수
        remap.data()                // remap 테이블
    );

    // --- 단계 3: 최종 정점 버퍼 생성 (압축 포맷으로 변환 포함) ---
    // 3.1. 먼저 고유한 정점들만 float 형태로 임시 버퍼에 모읍니다.
    std::vector<FVertexSimpleFloat> remappedFloatVertices(unique_vertex_count);
    meshopt_remapVertexBuffer(
        remappedFloatVertices.data(), // 고유 float 정점을 저장할 임시 버퍼
        sourceFloatVertices.data(),   // 원본 float 정점 버퍼
        sourceFloatVertices.size(),   // 원본 정점 수
        sizeof(FVertexSimpleFloat),   // 원본 정점 구조체 크기
        remap.data()                  // remap 테이블
    );

    // 3.2. 임시 float 정점 버퍼를 최종 GPU 포맷(XMHALF 등)으로 변환합니다.
    finalVertices.reserve(unique_vertex_count); // 최종 버퍼 메모리 예약
    for (const auto& floatVertex : remappedFloatVertices)
    {
        FVertexSimple packedVertex; // 최종 포맷 구조체

        // 위치 변환 (float -> XMHALF4)
        DirectX::XMVECTOR posVec = DirectX::XMVectorSet(floatVertex.x, floatVertex.y, floatVertex.z, 0.0f); // W는 보통 1.0 또는 0.0
        DirectX::PackedVector::XMStoreHalf4(&packedVertex.position, posVec);

        // UV 변환 (float -> XMHALF2)
        DirectX::XMVECTOR uvVec = DirectX::XMVectorSet(floatVertex.u, floatVertex.v, 0.0f, 0.0f);
        DirectX::PackedVector::XMStoreHalf2(&packedVertex.uv, uvVec);

        // (만약 FVertexSimpleFloat와 FVertexSimple에 다른 속성이 있다면 여기서 변환 추가)
        // 예: 노말 변환 (float3 -> PackedFormat)
        // DirectX::XMVECTOR normVec = DirectX::XMVectorSet(floatVertex.nx, floatVertex.ny, floatVertex.nz, 0.0f);
        // DirectX::PackedVector::XMStore...(&packedVertex.normal, normVec);

        finalVertices.push_back(packedVertex); // 최종 버퍼에 추가
    }

    // 최종 정점 버퍼와 최종 인덱스 버퍼를 쌍으로 반환
    return std::make_pair(std::move(finalVertices), std::move(finalIndices));
}