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
        staticMeshCountsMap[staticMesh]++;
        return staticMesh;
    }

    staticMesh = FObjectFactory::ConstructObject<UStaticMesh>();
    staticMesh->SetData(staticMeshRenderData);

    staticMeshMap.Add(staticMeshRenderData->ObjectName, staticMesh);
    staticMeshCountsMap[staticMesh]++;
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
