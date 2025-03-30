#pragma once
#include "Components/MeshComponent.h"
#include "Mesh/StaticMesh.h"
#include "Container/TriangleBVH.h"

class UStaticMeshComponent : public UMeshComponent
{
    DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)

public:
    UStaticMeshComponent() = default;

    PROPERTY(int, selectedSubMeshIndex);
    bool picked;
    virtual uint32 GetNumMaterials() const override;
    virtual UMaterial* GetMaterial(uint32 ElementIndex) const override;
    virtual uint32 GetMaterialIndex(FName MaterialSlotName) const override;
    virtual TArray<FName> GetMaterialSlotNames() const override;
    virtual void GetUsedMaterials(TArray<UMaterial*>& Out) const override;

    virtual int CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance) override;

    UStaticMesh* GetStaticMesh() const { return staticMesh; }
    void SetStaticMesh(UStaticMesh* value)
    {
        staticMesh = value;
        OverrideMaterials.SetNum(value->GetMaterials().Num());
        AABB = FBoundingBox(staticMesh->GetRenderData()->BoundingBoxMin, staticMesh->GetRenderData()->BoundingBoxMax);

        if (pTriangleBVH == nullptr)
        {
            pTriangleBVH = BuildTriangleBVHFromRenderData(staticMesh->GetRenderData());
        }
    }
    TriangleBVH* BuildTriangleBVHFromRenderData(const OBJ::FStaticMeshRenderData* renderData)
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
    bool IsPicked() { return picked; }
protected:
    TriangleBVH* pTriangleBVH;
    UStaticMesh* staticMesh = nullptr;
    int selectedSubMeshIndex = -1;
};