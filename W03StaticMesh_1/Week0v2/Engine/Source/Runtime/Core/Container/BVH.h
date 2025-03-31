#pragma once
#include "Array.h"
#include "Define.h"

class UPrimitiveComponent;
class UStaticMeshComponent;

#pragma once

class FBVH
{
public:
    // 생성자: 초기 바운딩박스를 설정
    FBVH(const FBoundingBox& InBoundingBox);
    // 소멸자: 자식 노드 메모리 해제
    ~FBVH();

    // 컴포넌트를 BVH 트리에 추가
    void AddComponent(UStaticMeshComponent* InComponent);

    void RayCheck(const FVector& pickRayOrigin, const FVector& rayDirection, TArray<std::pair<FBVH*, float>>& outSortedLeaves, int maxT);

    void FlattenTree(TArray<FBVH*>& OutNodes);
    TArray<UStaticMeshComponent*> CollectCandidateComponentsByLeaf(const FBVH* leaf)
    {
        return leaf ? leaf->PrimitiveComponents : TArray<UStaticMeshComponent*>();
    } 
    
    void CollectValidLeafNodesWithT(const FVector& pickRayOrigin, const FVector& rayDirection, TArray<std::pair<FBVH*, float>>& OutLeaves, int maxT);
    // 유효한 리프 노드들을 반환 (재귀적으로)
    void CollectValidLeafNodes(const FVector& pickRayOrigin, const FVector& rayDirection, TArray<FBVH*>& OutLeaves);
    // 현재 리프 노드의 컴포넌트 리스트 반환
    TArray<UStaticMeshComponent*> GetPrimitiveComponents() const;

    TArray<UStaticMeshComponent*>CollectIntersectingComponents(const Plane frustumPlanes[6]);

    // 디버깅: 현재 노드의 바운딩박스를 렌더링
    void DebugBoundingBox();

    // 리프 노드 여부 판단
    bool IsLeafNode() const { return (LeftChild == nullptr && RightChild == nullptr); }

    // 현재 노드의 깊이 (분할 횟수)
    int Depth = 0;

    // 분할 임계치와 최대 깊이 (필요에 따라 조정)
    static const int DivideThreshold = 32;
    static const int MaxDepth = 23;
    TArray<std::pair<FBVH*, float>> hitLeaves;

     void SubDivide();

    FBoundingBox BoundingBox;
  
    TArray<UStaticMeshComponent*> PrimitiveComponents;
    FBVH* LeftChild = nullptr;
    FBVH* RightChild = nullptr;

};
