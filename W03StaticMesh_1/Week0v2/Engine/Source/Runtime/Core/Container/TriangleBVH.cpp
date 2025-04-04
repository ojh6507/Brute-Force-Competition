#include "TriangleBVH.h"
#include <iostream>
#include <queue>

// 생성자: 초기 삼각형 리스트로부터 BVH 구축 시작
TriangleBVH::TriangleBVH(const TArray<Triangle>& inTriangles) {
    BuildRecursive(inTriangles, 0);
}

TriangleBVH::~TriangleBVH() {
    if (left)  delete left;
    if (right) delete right;
}

// 재귀적으로 BVH 트리 구축
void TriangleBVH::BuildRecursive(const TArray<Triangle>& inTriangles, int depth) {
    // 현재 노드에 삼각형 할당
    triangles = inTriangles;

    // 현재 노드의 바운딩 박스 계산: 할당된 모든 삼각형의 바운딩 박스 통합
    if (!triangles.IsEmpty()) {
        boundingBox = triangles[0].bbox;
        for (size_t i = 1; i < triangles.Num(); ++i) {
            boundingBox.Expand(triangles[i].bbox);
        }
    }

    // 리프 조건: 삼각형 수가 DivideThreshold 이하이거나 최대 깊이에 도달한 경우 분할 중단
    if (triangles.Num() <= DivideThreshold || depth >= MaxDepth) {
        return;
    }

    // 분할 축 결정: 현재 바운딩 박스에서 가장 긴 축 선택
    FVector Num = boundingBox.max - boundingBox.min;
    int splitAxis = 0;
    if (Num.y > Num.x && Num.y > Num.z)
        splitAxis = 1;
    else if (Num.z > Num.x)
        splitAxis = 2;

    // 삼각형 중심값을 기준으로 좌/우 분할: 각 삼각형의 중심값은 (min+max)*0.5
    TArray<Triangle> leftTriangles;
    TArray<Triangle> rightTriangles;
    float sum = 0.0f;
    for (const Triangle& tri : triangles) {
        float center;
        if (splitAxis == 0)      center = (tri.bbox.min.x + tri.bbox.max.x) * 0.5f;
        else if (splitAxis == 1) center = (tri.bbox.min.y + tri.bbox.max.y) * 0.5f;
        else                     center = (tri.bbox.min.z + tri.bbox.max.z) * 0.5f;
        sum += center;
    }
    float pivot = sum / static_cast<float>(triangles.Num());

    // 분할: 중심값이 pivot 미만이면 왼쪽, 이상이면 오른쪽으로 분리
    for (const Triangle& tri : triangles) {
        float center;
        if (splitAxis == 0)      center = (tri.bbox.min.x + tri.bbox.max.x) * 0.5f;
        else if (splitAxis == 1) center = (tri.bbox.min.y + tri.bbox.max.y) * 0.5f;
        else                     center = (tri.bbox.min.z + tri.bbox.max.z) * 0.5f;

        if (center < pivot)
            leftTriangles.Add(tri);
        else
            rightTriangles.Add(tri);
    }

    // 한쪽으로 치우쳤다면 분할하지 않고 리프로 남김
    if (leftTriangles.IsEmpty() || rightTriangles.IsEmpty())
        return;

    // 자식 노드 생성
    left = new TriangleBVH(leftTriangles);
    right = new TriangleBVH(rightTriangles);

    // 현재 노드는 분할되었으므로 삼각형 리스트 비움
    triangles.Empty();
}

// 광선과 교차하는 후보 삼각형들을 수집 (루트 호출)
void TriangleBVH::CollectCandidateTriangles(const FVector& rayOrigin, const FVector& rayDir, TArray<const Triangle*>& outTriangles) {
    CollectCandidatesIterativeDFS(rayOrigin, rayDir, outTriangles);
}

void TriangleBVH::CollectCandidatesRecursive(const FVector& rayOrigin, const FVector& rayDir, TArray<const Triangle*>& outTriangles) {
    float t;
  
    if (!boundingBox.Intersect(rayOrigin, rayDir, t))
        return;

    if (IsLeaf()) {
        // 리프 노드인 경우, 저장된 삼각형들을 후보 리스트에 추가
        for (const Triangle& tri : triangles) {
            outTriangles.Add(&tri);
        }
        return;
    }

    // 자식 노드가 있으면 재귀적으로 탐색
    if (left)  left->CollectCandidatesRecursive(rayOrigin, rayDir, outTriangles);
    if (right) right->CollectCandidatesRecursive(rayOrigin, rayDir, outTriangles);
}
void TriangleBVH::CollectCandidatesIterativeDFS(const FVector& rayOrigin, const FVector& rayDir, TArray<const Triangle*>& outTriangles)
{
    // 최대 깊이가 크지 않다는 가정 하에 고정 크기 스택 사용 (충분히 큰 크기로 설정)
    const int MAX_STACK_SIZE = 64;
    TriangleBVH* stack[MAX_STACK_SIZE];
    int stackIndex = 0;
    stack[stackIndex++] = this;

    while (stackIndex > 0)
    {
        TriangleBVH* node = stack[--stackIndex];

        float t;
        // 현재 노드의 AABB와 레이 교차 검사 (인라인 최적화를 기대)
        if (!node->boundingBox.Intersect(rayOrigin, rayDir, t))
            continue;

        // 리프 노드인 경우 후보 삼각형들을 추가
        if (node->IsLeaf())
        {
            for (const Triangle& tri : node->triangles)
            {
                outTriangles.Add(&tri);
            }
        }
        else
        {
            // 오른쪽 자식부터 스택에 넣어, 왼쪽 자식을 먼저 처리하도록 함
            if (node->right)
                stack[stackIndex++] = node->right;
            if (node->left)
                stack[stackIndex++] = node->left;
        }
    }
}
// TriangleBVH 클래스에 추가: 광선과의 교차 결과를 정렬된 리프 노드로 반환
void TriangleBVH::RayCheck(const FVector& rayOrigin, const FVector& rayDir, TArray<std::pair<TriangleBVH*, float>>& outSortedLeaves)
{
    outSortedLeaves.Empty();

    // 최대 깊이가 크지 않다는 가정 하에 고정 크기 스택 사용
    const int MAX_STACK_SIZE = 64;
    TriangleBVH* stack[MAX_STACK_SIZE];
    float stackT[MAX_STACK_SIZE];  // 각 노드의 AABB와의 교차 거리 저장
    int stackIndex = 0;

    float t;
    // 루트 노드의 바운딩 박스와 교차 검사
    if (boundingBox.Intersect(rayOrigin, rayDir, t))
    {
        stack[stackIndex] = this;
        stackT[stackIndex] = t;
        stackIndex++;
    }

    // 반복적 DFS 탐색
    while (stackIndex > 0)
    {
        TriangleBVH* node = stack[--stackIndex];
        float nodeT = stackT[stackIndex];

        // 현재 노드의 바운딩 박스와의 교차가 있다면
        if (node->IsLeaf())
        {
            // 리프 노드이면서 삼각형이 존재하는 경우 후보로 추가
            if (!node->triangles.IsEmpty())
            {
                outSortedLeaves.Add(std::pair<TriangleBVH*, float>(node, nodeT));
            }
        }
        else
        {
            // 자식 노드의 바운딩 박스와의 교차 검사 후 스택에 추가
            float tLeft, tRight;
            if (node->left && node->left->boundingBox.Intersect(rayOrigin, rayDir, tLeft))
            {
                stack[stackIndex] = node->left;
                stackT[stackIndex] = tLeft;
                stackIndex++;
            }
            if (node->right && node->right->boundingBox.Intersect(rayOrigin, rayDir, tRight))
            {
                stack[stackIndex] = node->right;
                stackT[stackIndex] = tRight;
                stackIndex++;
            }
        }
    }

    // 교차 거리를 기준으로 정렬 (가까운 순서대로)
    outSortedLeaves.Sort([](const std::pair<TriangleBVH*, float>& A, const std::pair<TriangleBVH*, float>& B) {
        return A.second < B.second;
        });
}

// 기존 후보 삼각형 수집 함수 (예: 재귀 방식) 대신, 
// 위 RayCheck 함수로부터 정렬된 리프 노드들을 얻은 후, 각 리프의 삼각형들을 순회하면 됩니다.
void TriangleBVH::CollectCandidateTrianglesFromSortedLeaves(const TArray<std::pair<TriangleBVH*, float>>& sortedLeaves, TArray<Triangle*>& outTriangles)
{
    for (const std::pair<TriangleBVH*, float>& pair : sortedLeaves)
    {
        TriangleBVH* leaf = pair.first;
        for (Triangle& tri : leaf->triangles)
        {
            outTriangles.Add(&tri);
        }
    }
}

// 디버깅: 현재 노드의 바운딩 박스 정보를 출력
void TriangleBVH::DebugPrint() const {
    std::cout << "BoundingBox: Min(" << boundingBox.min.x << ", " << boundingBox.min.y << ", " << boundingBox.min.z
        << ") Max(" << boundingBox.max.x << ", " << boundingBox.max.y << ", " << boundingBox.max.z << ")\n";
    if (left)  left->DebugPrint();
    if (right) right->DebugPrint();
}
