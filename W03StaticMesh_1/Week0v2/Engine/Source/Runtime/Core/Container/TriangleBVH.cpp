#include "TriangleBVH.h"
#include <iostream>

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
    CollectCandidatesRecursive(rayOrigin, rayDir, outTriangles);
}

// 재귀적으로 광선과 교차하는 후보 삼각형들을 수집
void TriangleBVH::CollectCandidatesRecursive(const FVector& rayOrigin, const FVector& rayDir, TArray<const Triangle*>& outTriangles) {
    float t;
    // 현재 노드의 바운딩 박스와 광선 교차 검사 (교차하지 않으면 바로 종료)

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

// 디버깅: 현재 노드의 바운딩 박스 정보를 출력
void TriangleBVH::DebugPrint() const {
    std::cout << "BoundingBox: Min(" << boundingBox.min.x << ", " << boundingBox.min.y << ", " << boundingBox.min.z
        << ") Max(" << boundingBox.max.x << ", " << boundingBox.max.y << ", " << boundingBox.max.z << ")\n";
    if (left)  left->DebugPrint();
    if (right) right->DebugPrint();
}
