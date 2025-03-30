#include "TriangleKDTree.h"
#include <algorithm>


TriangleKDTree::TriangleKDTree(const TArray<Triangle>& inTriangles)
{
    BuildRecursive(inTriangles, 0);
}

TriangleKDTree::~TriangleKDTree()
{
    if (left)  delete left;
    if (right) delete right;
}

void TriangleKDTree::BuildRecursive(const TArray<Triangle>& inTriangles, int depth)
{
    triangles = inTriangles;

    // 현재 노드의 바운딩박스 계산: 모든 삼각형의 bbox 합집합
    if (!triangles.IsEmpty())
    {
        boundingBox = triangles[0].bbox;
        for (size_t i = 1; i < triangles.Num(); ++i)
        {
            boundingBox.Expand(triangles[i].bbox);
        }
    }

    // 리프 조건: 삼각형 수가 DivideThreshold 이하이거나 최대 깊이에 도달
    if (triangles.Num() <= DivideThreshold || depth >= MaxDepth)
        return;

    // KD-트리 분할: 분할 축을 depth % 3로 결정 (0: X, 1: Y, 2: Z)
    int splitAxis = depth % 3;
    TArray<float> centers;
    centers.Reserve(triangles.Num());
    for (const Triangle& tri : triangles)
    {
        float center = 0.0f;
        if (splitAxis == 0)
            center = (tri.bbox.min.x + tri.bbox.max.x) * 0.5f;
        else if (splitAxis == 1)
            center = (tri.bbox.min.y + tri.bbox.max.y) * 0.5f;
        else
            center = (tri.bbox.min.z + tri.bbox.max.z) * 0.5f;
        centers.Add(center);
    }

    // 중앙값(Pivot) 계산: 간단하게 평균 사용 (정확한 중앙값을 위해 std::nth_element 도 고려 가능)
    float sum = 0.0f;
    for (float f : centers)
    {
        sum += f;
    }
    float pivot = sum / centers.Num();

    // 삼각형들을 pivot 기준으로 분할
    TArray<Triangle> leftTriangles;
    TArray<Triangle> rightTriangles;
    for (const Triangle& tri : triangles)
    {
        float center = 0.0f;
        if (splitAxis == 0)
            center = (tri.bbox.min.x + tri.bbox.max.x) * 0.5f;
        else if (splitAxis == 1)
            center = (tri.bbox.min.y + tri.bbox.max.y) * 0.5f;
        else
            center = (tri.bbox.min.z + tri.bbox.max.z) * 0.5f;

        if (center < pivot)
            leftTriangles.Add(tri);
        else
            rightTriangles.Add(tri);
    }

    // 만약 한쪽이 비면 분할하지 않고 리프로 유지
    if (leftTriangles.IsEmpty() || rightTriangles.IsEmpty())
        return;

    // 자식 노드 생성 (재귀)
    left = new TriangleKDTree(leftTriangles);
    right = new TriangleKDTree(rightTriangles);

    // 분할 후, 현재 노드의 삼각형 리스트를 비워 메모리 최적화
    triangles.Empty();
}

void TriangleKDTree::CollectCandidateTriangles(const FVector& rayOrigin, const FVector& rayDir, TArray<const Triangle*>& outTriangles)
{
    CollectCandidatesRecursive(rayOrigin, rayDir, outTriangles);
}

void TriangleKDTree::CollectCandidatesRecursive(const FVector& rayOrigin, const FVector& rayDir, TArray<const Triangle*>& outTriangles)
{
    float t;
    // 현재 노드의 BoundingBox와 레이의 교차 여부 검사
    if (!boundingBox.Intersect(rayOrigin, rayDir, t))
        return;

    if (IsLeaf())
    {
        // 리프 노드라면, 저장된 모든 삼각형의 주소를 후보 리스트에 추가
        for (const Triangle& tri : triangles)
        {
            outTriangles.Add(&tri);
        }
        return;
    }

    // 자식 노드의 교차 거리 계산 (가능한 경우)
    float tLeft = FLT_MAX, tRight = FLT_MAX;
    bool leftHit = (left && left->boundingBox.Intersect(rayOrigin, rayDir, tLeft));
    bool rightHit = (right && right->boundingBox.Intersect(rayOrigin, rayDir, tRight));

    // 두 자식 노드가 모두 레이와 교차하면, 더 가까운 자식부터 재귀 호출
    if (leftHit && rightHit)
    {
        if (tLeft < tRight)
        {
            left->CollectCandidatesRecursive(rayOrigin, rayDir, outTriangles);
            right->CollectCandidatesRecursive(rayOrigin, rayDir, outTriangles);
        }
        else
        {
            right->CollectCandidatesRecursive(rayOrigin, rayDir, outTriangles);
            left->CollectCandidatesRecursive(rayOrigin, rayDir, outTriangles);
        }
    }
    else if (leftHit)
    {
        left->CollectCandidatesRecursive(rayOrigin, rayDir, outTriangles);
    }
    else if (rightHit)
    {
        right->CollectCandidatesRecursive(rayOrigin, rayDir, outTriangles);
    }
}

void TriangleKDTree::DebugPrint() const
{
   
}
