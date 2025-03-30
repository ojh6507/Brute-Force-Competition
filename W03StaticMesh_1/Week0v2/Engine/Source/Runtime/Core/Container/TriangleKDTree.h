#pragma once
#include "Define.h"
#include "Math/JungleMath.h"
#include "Container/Array.h"

class TriangleKDTree
{
public:
    TriangleKDTree(const TArray<Triangle>& inTriangles);
    ~TriangleKDTree();

    // 후보 삼각형들을 수집 (루트 호출)
    void CollectCandidateTriangles(const FVector& rayOrigin, const FVector& rayDir, TArray<const Triangle*>& outTriangles);
    void DebugPrint() const;

    // 리프 노드 여부 판단
    bool IsLeaf() const { return left == nullptr && right == nullptr; }

private:
    // 재귀적으로 KD-트리 구축
    void BuildRecursive(const TArray<Triangle>& inTriangles, int depth);

    // 재귀적으로 후보 삼각형 수집
    void CollectCandidatesRecursive(const FVector& rayOrigin, const FVector& rayDir, TArray<const Triangle*>& outTriangles);

private:
    FBoundingBox boundingBox;
    TArray<Triangle> triangles; // 리프 노드에 저장되는 삼각형들
    TriangleKDTree* left = nullptr;
    TriangleKDTree* right = nullptr;

    static const int DivideThreshold = 5;
    static const int MaxDepth = 10;
};
