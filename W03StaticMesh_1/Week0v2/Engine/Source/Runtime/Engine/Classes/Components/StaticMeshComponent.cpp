#include "Components/StaticMeshComponent.h"

#include "World.h"
#include "Launch/EngineLoop.h"
#include "UObject/ObjectFactory.h"
#include "UnrealEd/PrimitiveBatch.h"


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

int UStaticMeshComponent::CheckRayIntersection(
    FVector& rayOrigin,
    FVector& rayDirection,
    float& pfNearHitDistance)
{
    // AABB 테스트로 초기 필터링
    if (!AABB.Intersect(rayOrigin, rayDirection, pfNearHitDistance))
    {
        return 0;
    }

    // BVH가 미리 구축되어 있지 않으면 생성 (예: lazy initialization)
   

    // BVH를 통해 후보 삼각형 목록을 수집
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
