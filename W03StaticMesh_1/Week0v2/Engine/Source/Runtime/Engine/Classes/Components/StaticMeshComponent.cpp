#include "Components/StaticMeshComponent.h"

#include "World.h"
#include "Launch/EngineLoop.h"
#include "UObject/ObjectFactory.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "Define.h"

uint32 UStaticMeshComponent::GetNumMaterials() const
{
    if (staticMesh == nullptr) return 0;

    return staticMesh->GetMaterials().Num();
}

UMaterial* UStaticMeshComponent::GetMaterial(uint32 ElementIndex) const
{
    if (staticMesh != nullptr)
    {
        if (OverrideMaterials[ElementIndex] != nullptr)
        {
            return OverrideMaterials[ElementIndex];
        }

        if (staticMesh->GetMaterials().IsValidIndex(ElementIndex))
        {
            return staticMesh->GetMaterials()[ElementIndex]->Material;
        }
    }
    return nullptr;
}

uint32 UStaticMeshComponent::GetMaterialIndex(FName MaterialSlotName) const
{
    if (staticMesh == nullptr) return -1;

    return staticMesh->GetMaterialIndex(MaterialSlotName);
}

TArray<FName> UStaticMeshComponent::GetMaterialSlotNames() const
{
    TArray<FName> MaterialNames;
    if (staticMesh == nullptr) return MaterialNames;

    for (const FStaticMaterial* Material : staticMesh->GetMaterials())
    {
        MaterialNames.Emplace(Material->MaterialSlotName);
    }

    return MaterialNames;
}

void UStaticMeshComponent::GetUsedMaterials(TArray<UMaterial*>& Out) const
{
    if (staticMesh == nullptr) return;
    staticMesh->GetUsedMaterials(Out);
    for (int materialIndex = 0; materialIndex < GetNumMaterials(); materialIndex++)
    {
        if (OverrideMaterials[materialIndex] != nullptr)
        {
            Out[materialIndex] = OverrideMaterials[materialIndex];
        }
    }
}

int UStaticMeshComponent::CheckRayIntersection( FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance)
{
    // AABB 테스트로 초기 필터링
    if (!AABB.Intersect(rayOrigin, rayDirection, pfNearHitDistance))
    {
        return 0;
    }

    TArray <const Triangle*> candidateTriangles;
    pTriangleBVH->CollectCandidateTriangles(rayOrigin, rayDirection, candidateTriangles);

    int nIntersections = 0;
    float fNearHitDistance = FLT_MAX;

    // 후보 삼각형에 대해 정밀 교차 테스트 수행
    for (const Triangle* tri : candidateTriangles)
    {
        float fHitDistance;
        if (IntersectRayTriangle(rayOrigin, rayDirection, tri->v0, tri->v1, tri->v2, fHitDistance))
        {
            if (fHitDistance < fNearHitDistance)
            {
                pfNearHitDistance = fNearHitDistance = fHitDistance;
            }
            ++nIntersections;
        }
    }

    return nIntersections;
}

TriangleBVH* UStaticMeshComponent::BuildTriangleBVHFromRenderData(const OBJ::FStaticMeshRenderData* renderData)
{
    if (!renderData)
    {
        return nullptr;
    }

    // 정점 및 인덱스 배열 획득
    const FVertexSimple* vertices = renderData->Vertices.GetData();

    const UINT* indices = renderData->Indices.GetData();

    int vertexCount = renderData->Vertices.Num();
    int indexCount = renderData->Indices.Num();

    // 최종 Triangle 목록
    TArray<Triangle> triangleList;



    // 인덱스가 없는 경우 (정점 3개 당 삼각형)
    int nPrimitives = vertexCount / 3;
    triangleList.Reserve(nPrimitives);

    const BYTE* pbVertices = reinterpret_cast<const BYTE*>(vertices);
    const size_t stride = sizeof(FVertexSimple);

    for (int i = 0; i < nPrimitives; i++)
    {
        int idx0 = i * 3;
        int idx1 = idx0 + 1;
        int idx2 = idx0 + 2;

        const FVector& v0 = *reinterpret_cast<const FVector*>(pbVertices + (idx0 * stride));
        const FVector& v1 = *reinterpret_cast<const FVector*>(pbVertices + (idx1 * stride));
        const FVector& v2 = *reinterpret_cast<const FVector*>(pbVertices + (idx2 * stride));

        triangleList.Add(Triangle(v0, v1, v2));
    }


    TriangleBVH* pBVH = new TriangleBVH(triangleList);
    return pBVH;
}

TriangleKDTree* UStaticMeshComponent::BuildTriangleKDTreeFromRenderData(const OBJ::FStaticMeshRenderData* renderData)
{
    if (!renderData)
    {
        return nullptr;
    }

    // 정점 및 인덱스 배열 획득
    const FVertexSimple* vertices = renderData->Vertices.GetData();
    const UINT* indices = renderData->Indices.GetData();

    int vertexCount = renderData->Vertices.Num();
    int indexCount = renderData->Indices.Num();

    // 최종 Triangle 목록
    TArray<Triangle> triangleList;

    // 인덱스가 있는 경우 (Indexed)
    if (indexCount > 0 && indices != nullptr)
    {
        int nPrimitives = indexCount / 3;
        triangleList.Reserve(nPrimitives);

        const BYTE* pbVertices = reinterpret_cast<const BYTE*>(vertices);
        const size_t stride = sizeof(FVertexSimple);

        for (int i = 0; i < nPrimitives; i++)
        {
            int idx0 = indices[i * 3];
            int idx1 = indices[i * 3 + 1];
            int idx2 = indices[i * 3 + 2];

            const FVector& v0 = *reinterpret_cast<const FVector*>(pbVertices + (idx0 * stride));
            const FVector& v1 = *reinterpret_cast<const FVector*>(pbVertices + (idx1 * stride));
            const FVector& v2 = *reinterpret_cast<const FVector*>(pbVertices + (idx2 * stride));

            triangleList.Add(Triangle(v0, v1, v2));
        }
    }
    else
    {
        // 인덱스가 없는 경우: 정점 3개 당 하나의 삼각형
        int nPrimitives = vertexCount / 3;
        triangleList.Reserve(nPrimitives);

        const BYTE* pbVertices = reinterpret_cast<const BYTE*>(vertices);
        const size_t stride = sizeof(FVertexSimple);

        for (int i = 0; i < nPrimitives; i++)
        {
            int idx0 = i * 3;
            int idx1 = idx0 + 1;
            int idx2 = idx0 + 2;

            const FVector& v0 = *reinterpret_cast<const FVector*>(pbVertices + (idx0 * stride));
            const FVector& v1 = *reinterpret_cast<const FVector*>(pbVertices + (idx1 * stride));
            const FVector& v2 = *reinterpret_cast<const FVector*>(pbVertices + (idx2 * stride));

            triangleList.Add(Triangle(v0, v1, v2));
        }
    }

    // KD-트리 생성: 기존 BVH 대신 KD-트리 생성자 호출
    TriangleKDTree* pKDTree = new TriangleKDTree(triangleList);
    return pKDTree;
}