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
    // 먼저 AABB로 광선-바운딩 박스를 체크하여,
    // 아예 교차가 없으면 삼각형 검사 자체를 스킵
    if (!AABB.Intersect(rayOrigin, rayDirection, pfNearHitDistance))
    {
        return 0;
    }

    if (staticMesh == nullptr) return 0;

    OBJ::FStaticMeshRenderData* renderData = staticMesh->GetRenderData();
    FVertexSimple* vertices = renderData->Vertices.GetData();
    UINT* indices = renderData->Indices.GetData();

    if (!vertices) return 0;

    // stride 사전 계산
    const uint32 stride = sizeof(FVertexSimple);

    // 결과 변수
    int nIntersections = 0;
    float fNearHitDistance = FLT_MAX;

    // 인덱스 없는 경우와 있는 경우를 분기
    if (!indices)
    {
        // 인덱스 없이 (vCount / 3) 삼각형
        int vCount = renderData->Vertices.Num();
        int nPrimitives = vCount / 3;

        for (int i = 0; i < nPrimitives; i++)
        {
            int idx0 = i * 3;
            int idx1 = i * 3 + 1;
            int idx2 = i * 3 + 2;

            // 포인터 계산 최소화
            uint32 base0 = idx0 * stride;
            uint32 base1 = idx1 * stride;
            uint32 base2 = idx2 * stride;

            const FVector& v0 = *reinterpret_cast<FVector*>(reinterpret_cast<BYTE*>(vertices) + base0);
            const FVector& v1 = *reinterpret_cast<FVector*>(reinterpret_cast<BYTE*>(vertices) + base1);
            const FVector& v2 = *reinterpret_cast<FVector*>(reinterpret_cast<BYTE*>(vertices) + base2);

            float fHitDistance;
            if (IntersectRayTriangle(rayOrigin, rayDirection, v0, v1, v2, fHitDistance))
            {
                if (fHitDistance < fNearHitDistance)
                {
                    pfNearHitDistance = fNearHitDistance = fHitDistance;
                }
                ++nIntersections;
            }
        }
    }
    else
    {
        // 인덱스가 있는 경우 (iCount / 3) 삼각형
        int iCount = renderData->Indices.Num();
        int nPrimitives = iCount / 3;

        BYTE* pbPositions = reinterpret_cast<BYTE*>(vertices);

        for (int i = 0; i < nPrimitives; i++)
        {
            int idx0 = indices[i * 3];
            int idx1 = indices[i * 3 + 1];
            int idx2 = indices[i * 3 + 2];

            uint32 base0 = idx0 * stride;
            uint32 base1 = idx1 * stride;
            uint32 base2 = idx2 * stride;

            const FVector& v0 = *reinterpret_cast<FVector*>(pbPositions + base0);
            const FVector& v1 = *reinterpret_cast<FVector*>(pbPositions + base1);
            const FVector& v2 = *reinterpret_cast<FVector*>(pbPositions + base2);

            float fHitDistance;
            if (IntersectRayTriangle(rayOrigin, rayDirection, v0, v1, v2, fHitDistance))
            {
                if (fHitDistance < fNearHitDistance)
                {
                    pfNearHitDistance = fNearHitDistance = fHitDistance;
                }
                ++nIntersections;
            }
        }
    }

    return nIntersections;
}
