#pragma once

#include "Math/JungleMath.h"
#include "Define.h"
#include <vector>
#include <algorithm>
#include <limits>

// 삼각형 단위 BVH 클래스
class TriangleBVH {
public:
    // 생성자: 삼각형 리스트를 받아 BVH 트리를 구축
    TriangleBVH(const TArray<Triangle>& inTriangles);
    ~TriangleBVH();

    // 주어진 광선(rayOrigin, rayDir)과 교차할 가능성이 있는 삼각형 후보들을 수집
    void CollectCandidateTriangles(const FVector& rayOrigin, const FVector& rayDir, TArray<const Triangle*>& outTriangles);

    // 디버깅용: 현재 노드의 바운딩 박스 정보를 출력
    void DebugPrint() const;

private:
    // 재귀적으로 BVH 트리 구축
    void BuildRecursive(const TArray<Triangle>& inTriangles, int depth);
    // 현재 노드가 리프 노드인지 여부
    bool IsLeaf() const { return left == nullptr && right == nullptr; }

    // 재귀적으로 광선과 교차하는 후보 삼각형들을 수집
    void CollectCandidatesRecursive(const FVector& rayOrigin, const FVector& rayDir, TArray<const Triangle*>& outTriangles);
    void CollectCandidatesIterativeDFS(const FVector& rayOrigin, const FVector& rayDir, TArray<const Triangle*>& outTriangles);
private:
    FBoundingBox boundingBox;
    TArray<Triangle> triangles; // 리프 노드에 저장된 삼각형들

    TriangleBVH* left = nullptr;
    TriangleBVH* right = nullptr;

    static const int DivideThreshold = 50; // 리프에 남길 삼각형 개수
    static const int MaxDepth = 10;          // 최대 트리 깊이
};
