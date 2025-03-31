#pragma once
#include "Math/JungleMath.h"
#include "Components/StaticMeshComponent.h"
#include "Container/Array.h"

class KDTree
{
public:
    KDTree(const FBoundingBox& InBoundingBox);
    ~KDTree();

    // 컴포넌트를 KD-트리에 추가
    void AddComponent(UStaticMeshComponent* InComponent);

    // 현재 노드를 분할 (리프 노드일 때만)
    void SubDivide();

    // 현재 노드에 포함된 컴포넌트들을 반환
    TArray<UStaticMeshComponent*> GetPrimitiveComponents() const { return PrimitiveComponents; }

    // 유효한 리프 노드들을 재귀적으로 수집 (참조 전달)
    void CollectValidLeafNodes(TArray<KDTree*>& OutLeaves);

    // 픽킹 후보 컴포넌트를 수집 (pickPos, viewMatrix, CameraPos, maxDist 기준)
    TArray<UStaticMeshComponent*> CollectCandidateComponents(const FVector& pickPos, const FMatrix& viewMatrix, const FVector& CameraPos, float maxDist);

    // 디버깅: 현재 노드의 바운딩박스를 렌더링
    void DebugBoundingBox();

    // 리프 노드 여부 판단
    bool IsLeafNode() const { return (LeftChild == nullptr && RightChild == nullptr); }

    // KDTree 데이터
    FBoundingBox BoundingBox;
    TArray<UStaticMeshComponent*> PrimitiveComponents;
    KDTree* LeftChild = nullptr;
    KDTree* RightChild = nullptr;
    int Depth = 0;

    static const int DivideThreshold = 10;
    static const int MaxDepth = 10;
};
