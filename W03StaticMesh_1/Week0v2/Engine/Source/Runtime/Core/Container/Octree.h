#pragma once
#include "Array.h"
#include "Define.h"

class UPrimitiveComponent;
class UStaticMeshComponent;

class FOctree
{
public:
    FOctree(const FBoundingBox& InBoundingBox, int InNodeID);
    ~FOctree();
    void AddComponent(UStaticMeshComponent* InComponent);
    int GetDepth(int nodeID);
    void SubDivide();
    void DivideComponent();
    int CalculteChildIndex(FVector Pos);

    bool IsLeafNode(){ return bIsLeap; }
    FBoundingBox CalculateChildBoundingBox(int index);
    TArray<FOctree*> GetValidLeafNodes();
    TArray<FOctree*> CollectCandidateNodes(const FVector& pickPos, const FMatrix& viewMatrix);
    bool CheckInBoundingBox(FVector Pos);
    void CollectIntersectingComponents(const Plane frustumPlanes[6], TArray<UStaticMeshComponent*>& OutComponents);
    int GenerateNodeID(int ParentID, int MyChildIndex);
    TArray<UStaticMeshComponent*> GetRayPossibleComp();
    TArray<UStaticMeshComponent*>& GetPrimitiveComponents() { return PrimitiveComponents; }
    void DebugBoundingBox();
    TArray<UStaticMeshComponent*> CollectCandidateComponents(const FVector& pickPos, const FMatrix& viewMatrix);
    TArray<FOctree*> GetVisibleOctreeNodes() {return VisibleOctreeNodes; }
    TArray<FOctree*> GetNeightborOctreeNode();

private:
    FBoundingBox BoundingBox;
    FVector HalfSize;
    TArray<FOctree*> Children;
    FOctree* CurrentNode;
    TArray<UStaticMeshComponent*> PrimitiveComponents;
    TArray<FOctree*> VisibleOctreeNodes;

    bool bIsLeap = true;
    int DivideThreshold = 100;
    int NodeID = 0;
    int MaxDepth = 5;
};
// leaf가 0일 수도 쩔수
// 무조건 모든 컴포넌트는 leaf에만
