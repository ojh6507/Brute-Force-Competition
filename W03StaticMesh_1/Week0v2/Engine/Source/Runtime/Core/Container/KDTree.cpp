#include "KDTree.h"
#include <algorithm> // std::nth_element, std::swap

// 생성자: 초기 바운딩박스를 설정
KDTree::KDTree(const FBoundingBox& InBoundingBox)
{
    BoundingBox = InBoundingBox;
}

// 소멸자: 자식 노드 메모리 해제
KDTree::~KDTree()
{
    if (LeftChild)  delete LeftChild;
    if (RightChild) delete RightChild;
}

// 컴포넌트를 KD-트리에 추가
void KDTree::AddComponent(UStaticMeshComponent* InComponent)
{
    if (IsLeafNode())
    {
        PrimitiveComponents.Add(InComponent);
        if (PrimitiveComponents.Num() > DivideThreshold && Depth < MaxDepth)
        {
            SubDivide();
        }
    }
    else
    {
        // 내부 노드라면, 분할 축에 따라 자식에 전달
        // 분할 축: Depth % 3 (0->x, 1->y, 2->z)
        int SplitAxis = Depth % 3;
        FBoundingBox ComponentBB = InComponent->GetWorldBoundingBox();
        FVector Center = (ComponentBB.min + ComponentBB.max) * 0.5f;
        float value = 0.0f;
        switch (SplitAxis)
        {
        case 0: value = Center.x; break;
        case 1: value = Center.y; break;
        case 2: value = Center.z; break;
        }

        // 중앙값(Pivot) 계산은 노드 분할 시에 결정되므로,
        // 여기서는 자식 노드의 BoundingBox를 보고 어느 쪽에 포함되는지 결정한다.
        // 간단하게, 자식 노드 중 Center 값이 작은 쪽에 넣어봅니다.
        if (LeftChild && LeftChild->BoundingBox.min[SplitAxis] <= value && value <= LeftChild->BoundingBox.max[SplitAxis])
            LeftChild->AddComponent(InComponent);
        else if (RightChild && RightChild->BoundingBox.min[SplitAxis] <= value && value <= RightChild->BoundingBox.max[SplitAxis])
            RightChild->AddComponent(InComponent);
        else
        {
            // 어느 쪽에도 명확히 속하지 않으면 현재 노드에 보관
            PrimitiveComponents.Add(InComponent);
        }
    }
}

// 분할: 리프 노드일 때, 현재 노드에 있는 컴포넌트들의 중앙값을 기준으로 자식 노드를 생성하고 재분배
void KDTree::SubDivide()
{
    if (Depth >= MaxDepth || !IsLeafNode())
        return;

    // 분할 축: Depth % 3
    int SplitAxis = Depth % 3;
    // 모든 컴포넌트의 중심값을 구함
    TArray<float> centers;
    centers.Reserve(PrimitiveComponents.Num());
    for (UStaticMeshComponent* comp : PrimitiveComponents)
    {
        FBoundingBox compBB = comp->GetWorldBoundingBox();
        FVector Center = (compBB.min + compBB.max) * 0.5f;
        switch (SplitAxis)
        {
        case 0: centers.Add(Center.x); break;
        case 2: centers.Add(Center.y); break;
        case 1: centers.Add(Center.z); break;
        }
    }
  
    float sum = 0.0f;
    for (float f : centers)
    {
        sum += f;
    }
    float Pivot = sum / centers.Num();

    // 자식 노드의 바운딩박스 생성: 현재 노드의 BoundingBox를 복사 후, 분할 축에 대해 나눔
    FBoundingBox LeftBox = BoundingBox;
    FBoundingBox RightBox = BoundingBox;
    LeftBox.max[SplitAxis] = Pivot;
    RightBox.min[SplitAxis] = Pivot;

    LeftChild = new KDTree(LeftBox);
    LeftChild->Depth = Depth + 1;
    RightChild = new KDTree(RightBox);
    RightChild->Depth = Depth + 1;

    // 기존 컴포넌트들을 재분배
    for (UStaticMeshComponent* comp : PrimitiveComponents)
    {
        FBoundingBox compBB = comp->GetWorldBoundingBox();
        FVector Center = (compBB.min + compBB.max) * 0.5f;
        float value = 0.0f;
        switch (SplitAxis)
        {
        case 0: value = Center.x; break;
        case 1: value = Center.z; break;
        case 2: value = Center.y; break;
        }
        if (value < Pivot)
        {
            LeftChild->AddComponent(comp);
        }
        else
        {
            RightChild->AddComponent(comp);
        }
    }

    PrimitiveComponents.Empty();
}

// 유효한 리프 노드들을 재귀적으로 수집 (참조 전달)
void KDTree::CollectValidLeafNodes(TArray<KDTree*>& OutLeaves)
{
    if (IsLeafNode())
    {
        if (PrimitiveComponents.Num() > 0)
            OutLeaves.Add(this);
    }
    else
    {
        if (LeftChild)
            LeftChild->CollectValidLeafNodes(OutLeaves);
        if (RightChild)
            RightChild->CollectValidLeafNodes(OutLeaves);
    }
}

// 후보 컴포넌트 수집 (픽킹 위치, viewMatrix, CameraPos, maxDist 기준)
// BVH와 유사하게, KDTree의 유효 리프 노드를 순회하며, pickRay와의 교차 및 카메라 거리 조건을 검사
TArray<UStaticMeshComponent*> KDTree::CollectCandidateComponents(
    const FVector& pickPos,
    const FMatrix& viewMatrix,
    const FVector& CameraPos,
    float maxDist)
{
    TArray<UStaticMeshComponent*> CandidateComponents;
    FMatrix inverseMatrix = FMatrix::Inverse(viewMatrix);
    FVector pickRayOrigin = inverseMatrix.TransformPosition(FVector(0, 0, 0));
    FVector transformedPick = inverseMatrix.TransformPosition(pickPos);
    FVector rayDirection = (transformedPick - pickRayOrigin).Normalize();

    TArray<KDTree*> validLeaves;
    CollectValidLeafNodes(validLeaves);

    const float maxDistSq = maxDist * maxDist;
    for (KDTree* leaf : validLeaves)
    {
        float dist = 0.0f;
        if (leaf->BoundingBox.Intersect(pickRayOrigin, rayDirection, dist))
        {
            for (UStaticMeshComponent* comp : leaf->GetPrimitiveComponents())
            {
                    CandidateComponents.Add(comp);
                /*float ddist = comp->GetWorldLocation().DistanceSq(CameraPos);
                    if (ddist < maxDistSq)
                */
                
            }
        }
    }
    return CandidateComponents;
}

// 디버깅: 현재 노드의 바운딩박스를 렌더링 (예시)
void KDTree::DebugBoundingBox()
{
   /* UPrimitiveBatch::GetInstance().RenderAABB(
        BoundingBox,
        FVector(0, 0, 0),
        FMatrix::Identity
    );
    */
}
