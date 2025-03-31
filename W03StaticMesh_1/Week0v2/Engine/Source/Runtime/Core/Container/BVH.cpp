#include "BVH.h"

#include <complex.h>

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "Math/JungleMath.h"



// 생성자: 초기 바운딩박스를 설정
FBVH::FBVH(const FBoundingBox& InBoundingBox) {
    BoundingBox = InBoundingBox;
}

// 소멸자: 자식 노드 메모리 해제
FBVH::~FBVH() {
    if (LeftChild) {
        delete LeftChild;
    }
    if (RightChild) {
        delete RightChild;
    }
}

// 컴포넌트를 BVH 트리에 추가
void FBVH::AddComponent(UStaticMeshComponent* InComponent) {
    if (!InComponent) {
        return;
    }

    // 컴포넌트의 월드 바운딩박스 획득
    FBoundingBox ComponentBB = InComponent->GetWorldBoundingBox();

    // 리프 노드인 경우
    if (IsLeafNode()) {
        // 현재 노드의 바운딩박스가 컴포넌트를 완전히 포함하는지 확인
        if (BoundingBox.Intersects(ComponentBB)) {
            PrimitiveComponents.Add(InComponent);

            // 분할 임계치 초과 시 분할
            if (PrimitiveComponents.Num() > DivideThreshold) {
                SubDivide();
            }
        }
        else {
            // 현재 노드의 바운딩박스에 포함되지 않는 경우에도 추가
            PrimitiveComponents.Add(InComponent);
        }
    }
    else {
        bool bAdded = false;
        // 자식 노드가 분할되어 있다면, 각 자식 노드의 바운딩박스가
        // 컴포넌트를 완전히 포함하는지 확인하여 분배
        if (LeftChild && LeftChild->BoundingBox.Intersects(ComponentBB)) {
            LeftChild->AddComponent(InComponent);
            bAdded = true;
        }
        if (RightChild && RightChild->BoundingBox.Intersects(ComponentBB)) {
            RightChild->AddComponent(InComponent);
            bAdded = true;
        }
        // 어느 자식 노드에도 완전히 포함되지 않으면 현재 노드에 보관
        if (!bAdded) {
            PrimitiveComponents.Add(InComponent);
        }
    }
}


// 분할: 현재 노드를 두 개의 자식 노드로 나누고, 기존 컴포넌트를 재분배
void FBVH::SubDivide() {
    if (Depth >= MaxDepth || !IsLeafNode()) {
        return;
    }

    // 분할 축 결정 (가장 긴 축)
    FVector BoxSize = BoundingBox.max - BoundingBox.min;
    int SplitAxis = 0;
    if (BoxSize.y > BoxSize.x && BoxSize.y > BoxSize.z)
        SplitAxis = 1;
    else if (BoxSize.z > BoxSize.x)
        SplitAxis = 2;

    // Pivot (중앙값) 계산
    float Pivot = (BoundingBox.min[SplitAxis] + BoundingBox.max[SplitAxis]) * 0.5f;

    // 자식 노드용 바운딩박스 생성 (Left: min ~ Pivot, Right: Pivot ~ max)
    FBoundingBox LeftBox = BoundingBox;
    FBoundingBox RightBox = BoundingBox;
    LeftBox.max[SplitAxis] = Pivot;
    RightBox.min[SplitAxis] = Pivot;

    LeftChild = new FBVH(LeftBox);
    LeftChild->Depth = Depth + 1;
    RightChild = new FBVH(RightBox);
    RightChild->Depth = Depth + 1;

    // 기존 노드의 모든 컴포넌트를 분할 기준에 따라 자식 노드로 전달
    for (UStaticMeshComponent* Component : PrimitiveComponents) {
        const FBoundingBox ComponentBoundingBox = Component->GetWorldBoundingBox();
        FVector Center = (ComponentBoundingBox.min + ComponentBoundingBox.max) * 0.5f;
        if (Center[SplitAxis] < Pivot) {
            LeftChild->AddComponent(Component);
        }
        else {
            RightChild->AddComponent(Component);
        }
    }

    // 현재 노드는 자식에게 컴포넌트 목록을 위임하므로 비웁니다.
    PrimitiveComponents.Empty();
}

// 현재 리프 노드의 컴포넌트들을 반환
TArray<UStaticMeshComponent*> FBVH::GetPrimitiveComponents() const {
    return PrimitiveComponents;
}
TArray<UStaticMeshComponent*> FBVH::CollectIntersectingComponents(const Plane frustumPlanes[6])
{

    //TArray<UStaticMeshComponent*> OutComponents;
    ////DebugBoundingBox();
    //// 현재 노드의 바운딩박스가 프러스텀과 교차하지 않으면 바로 반환
    //if (!BoundingBox.IsIntersectingFrustum(frustumPlanes))
    //{
    //    return OutComponents;
    //}

    //// 리프 노드라면 현재 노드의 컴포넌트를 모두 추가
    //if (IsLeafNode())
    //{
    //    OutComponents.Append(PrimitiveComponents);
    //    return OutComponents;
    //}

    //// 자식 노드가 있다면 재귀적으로 호출
    //if (LeftChild)
    //{
    //    OutComponents.Append(LeftChild->CollectIntersectingComponents(frustumPlanes));
    //}
    //if (RightChild)
    //{
    //    OutComponents.Append(RightChild->CollectIntersectingComponents(frustumPlanes));
    //}

    return {};
}

void FBVH::RayCheck(const FVector& pickRayOrigin, const FVector& rayDirection, TArray<std::pair<FBVH*, float>>& outSortedLeaves , int maxT)
{
    outSortedLeaves.Empty();
    CollectValidLeafNodesWithT(pickRayOrigin, rayDirection, outSortedLeaves, maxT);

    outSortedLeaves.Sort([](const TPair<FBVH*, float>& A, const TPair<FBVH*, float>& B) {
        return A.Value < B.Value;
        });
}

void FBVH::FlattenTree(TArray<FBVH*>& OutNodes)
{
    OutNodes.Add(this);

    if (LeftChild)
    {
        LeftChild->FlattenTree(OutNodes);
    }
    if (RightChild)
    {
        RightChild->FlattenTree(OutNodes);
    }
}

void FBVH::CollectValidLeafNodesWithT(const FVector& pickRayOrigin, const FVector& rayDirection, TArray<std::pair<FBVH*, float>>& OutLeaves, int maxT)
{
    float t;
   
    if (!BoundingBox.Intersect(pickRayOrigin, rayDirection, t) || t > maxT) {
        return;
    }
   
    if (IsLeafNode()) {
        if (PrimitiveComponents.Num() > 0) {
            OutLeaves.Add(TPair<FBVH*, float>(this, t));
        }
    }
    else {
        if (LeftChild) {
            LeftChild->CollectValidLeafNodesWithT(pickRayOrigin, rayDirection, OutLeaves, maxT);
        }
        if (RightChild) {
            RightChild->CollectValidLeafNodesWithT(pickRayOrigin, rayDirection, OutLeaves, maxT);
        }
    }
}


// 유효한 리프 노드들을 재귀적으로 수집
void FBVH::CollectValidLeafNodes(const FVector& pickRayOrigin, const FVector& rayDirection, TArray<FBVH*>& OutLeaves) {
    float t;
    if (!BoundingBox.Intersect(pickRayOrigin, rayDirection, t) ) {
        return;
    }
    if (IsLeafNode()) {
        if (PrimitiveComponents.Num() > 0) {
            OutLeaves.Add(this);
        }
    }
    else {
        if (LeftChild) {
            LeftChild->CollectValidLeafNodes(pickRayOrigin, rayDirection, OutLeaves);
        }
        if (RightChild) {
            RightChild->CollectValidLeafNodes(pickRayOrigin, rayDirection, OutLeaves);
        }
    }

}


// 디버깅: 현재 노드의 바운딩박스를 렌더링
void FBVH::DebugBoundingBox() {
    UPrimitiveBatch::GetInstance().RenderAABB(
        BoundingBox,
        (0, 0, 0),
        FMatrix::Identity
    );
}
