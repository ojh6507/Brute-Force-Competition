#pragma once
#include "Components/MeshComponent.h"
#include "Mesh/StaticMesh.h"
#include "Container/TriangleBVH.h"
#include "Container/TriangleKDTree.h"

class UStaticMeshComponent : public UMeshComponent
{
    DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)

public:
    UStaticMeshComponent() = default;

    PROPERTY(int, selectedSubMeshIndex);
    bool isSendBuffer = false;
    bool bPickedBufferState = false;
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

       //if (pTriangleBVH == nullptr)
       // {
       //     pTriangleBVH = BuildTriangleBVHFromRenderData(staticMesh->GetRenderData());
       // }
       ///* if (pTriangleKDTree == nullptr) {

       //     pTriangleKDTree = BuildTriangleKDTreeFromRenderData(staticMesh->GetRenderData());
       // }*/

    }
    TriangleBVH* BuildTriangleBVHFromRenderData(const OBJ::FStaticMeshRenderData* renderData);
    TriangleKDTree* BuildTriangleKDTreeFromRenderData(const OBJ::FStaticMeshRenderData* renderData);


    FVector GetBoundingBoxCenter() {
        return GetWorldBoundingBox().GetBoundingBoxCenter();
    }
    FBoundingBox GetWorldBoundingBox() const
    {
        FBoundingBox aab =GetBoundingBox();
        return aab.TransformWorld(Model);
    }
protected:
    TriangleBVH* pTriangleBVH;
    TriangleKDTree* pTriangleKDTree;
    UStaticMesh* staticMesh = nullptr;
    int selectedSubMeshIndex = -1;
};