#pragma once
#include "Array.h"
#include "Define.h"

class UPrimitiveComponent;
class UStaticMeshComponent;

class FOctree
{
public:
    FOctree(const FBoundingBox& InBoundingBox);
    ~FOctree();
    void AddComponent(UStaticMeshComponent* InComponent);
    void SetDepth(int InDepth) {Depth = InDepth;}
    void SubDivide();
    int CalculteChildIndex(FVector Pos);

    bool IsLeafNode(){ return Children.Num() == 0; }
    FBoundingBox CalculateChildBoundingBox(int index);
    TArray<FOctree*> GetValidLeafNodes();
    TArray<FOctree*> CollectCandidateNodes(const FVector& pickPos, const FMatrix& viewMatrix);
    void CollectIntersectingComponents(const Plane frustumPlanes[6], TArray<UStaticMeshComponent*>& OutComponents);
    TArray<UStaticMeshComponent*> GetRayPossibleComp();
    TArray<UStaticMeshComponent*>& GetPrimitiveComponents() { return PrimitiveComponents; }
    void DebugBoundingBox();
    TArray<UStaticMeshComponent*> CollectCandidateComponents(const FVector& pickPos, const FMatrix& viewMatrix);

private:
    FBoundingBox BoundingBox;
    FVector HalfSize;
    TArray<FOctree*> Children;
    FOctree* CurrentNode;
    TArray<UStaticMeshComponent*> PrimitiveComponents;

    int DivideThreshold = 100;
    int Depth = 0;
    int MaxDepth = 3;
};
// leaf가 0일 수도 쩔수
// 무조건 모든 컴포넌트는 leaf에만
