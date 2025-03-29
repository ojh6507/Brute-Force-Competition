#include "Octree.h"

#include <complex.h>

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "Math/JungleMath.h"
FOctree::FOctree(const FBoundingBox& InBoundingBox) {
    BoundingBox = InBoundingBox;

    HalfSize.x = (BoundingBox.min.x + BoundingBox.max.x) / 2 - BoundingBox.min.x;
    HalfSize.y = (BoundingBox.min.y + BoundingBox.max.y) / 2 - BoundingBox.min.y;
    HalfSize.z = (BoundingBox.min.z + BoundingBox.max.z) / 2 - BoundingBox.min.z;
}

FOctree::~FOctree()
{
    for (FOctree* Child : Children)
    {
        delete Child;
    }
}

void FOctree::AddComponent(UStaticMeshComponent* InComponent)
{
    if (!InComponent)
    {
        return;
    }

    const FBoundingBox& ComponentBoundingBox = InComponent->GetBoundingBox();

    if (IsLeafNode())
    {
        PrimitiveComponents.Add(InComponent);

        if (PrimitiveComponents.Num() > DivideThreshold)
        {
            SubDivide();
        }
    }
    else
    {
        int ChildBoundingBoxIndex = CalculteChildIndex(InComponent->GetWorldLocation()); //position기준이 아니라 boundingbox로 포함하는애 전부 주기
        Children[ChildBoundingBoxIndex]->AddComponent(InComponent);

        // for (int i=0;i<8;i++) //각 옥트리 돌면서 바운딩박스 충돌검사
        // {
        //     if (Children[i]->BoundingBox.Intersects(ComponentBoundingBox))
        //     {
        //         Children[i]->AddComponent(InComponent);
        //     }
        // }
    }
}

void FOctree::SubDivide()
{
    if (Depth >= MaxDepth || !IsLeafNode())
    {
        return;
    }

    // Children 배열에 공간 예약
    Children.Reserve(8);
    for (int i = 0; i < 8; i++)
    {
        FBoundingBox childBox = CalculateChildBoundingBox(i);
        FOctree* childNode = new FOctree(childBox);
        childNode->Depth = Depth + 1;
        Children.Add(childNode);
    }

    // 기존 노드의 컴포넌트를 각 자식 노드로 분배
    for (UStaticMeshComponent* Component : PrimitiveComponents)
    {
        int ChildBoundingBoxIndex = CalculteChildIndex(Component->GetWorldLocation());
        Children[ChildBoundingBoxIndex]->AddComponent(Component);
        // const FBoundingBox ComponentBoundingBox = Component->GetBoundingBox();
        //
        // for (int i=0;i<8;i++)
        // {
        //     if (Children[i]->BoundingBox.Intersects(ComponentBoundingBox))
        //     {
        //         Children[i]->AddComponent(Component);
        //     }
        // }
    }

    //Component들 자식에게 물려주고 클리어
    PrimitiveComponents.Empty();
}

int FOctree::CalculteChildIndex(FVector Pos)
{
    int ReturnIndex = 0;
    ReturnIndex |= Pos.x >= (BoundingBox.min.x + HalfSize.x) ? 1 : 0;
    ReturnIndex |= Pos.y >= (BoundingBox.min.y + HalfSize.y) ? 2 : 0;
    ReturnIndex |= Pos.z >= (BoundingBox.min.z + HalfSize.z) ? 4 : 0;

    return ReturnIndex;
}
void FOctree::CollectIntersectingComponents(const Plane frustumPlanes[6], TArray<UStaticMeshComponent*>& OutComponents)
{

    OutComponents.Empty();
    for (FOctree*& leaf : GetValidLeafNodes()) {
        leaf->DebugBoundingBox();
        if (!leaf->BoundingBox.IsIntersectingFrustum(frustumPlanes)) {
            continue;
        }

        for (auto& Comp : leaf->GetPrimitiveComponents()) {
            OutComponents.Add(Comp);
        }
    }

}


TArray<UStaticMeshComponent*> FOctree::GetRayPossibleComp()
{
    if (CurrentNode)
        return CurrentNode->PrimitiveComponents;
    else
        return{};
}
void FOctree::DebugBoundingBox()
{
    UPrimitiveBatch::GetInstance().RenderAABB(
        BoundingBox,
        (0, 0, 0),
        FMatrix::Identity
    );
}

TArray<UStaticMeshComponent*> FOctree::CollectCandidateComponents(const FVector& pickPos, const FMatrix& viewMatrix)
{
    TArray<UStaticMeshComponent*> CandidateComponents;
    FVector cameraOrigin = { 0, 0, 0 };
    FMatrix inverseMatrix = FMatrix::Inverse(viewMatrix);
    FVector pickRayOrigin = inverseMatrix.TransformPosition(cameraOrigin);
    FVector transformedPick = inverseMatrix.TransformPosition(pickPos);
    FVector rayDirection = (transformedPick - pickRayOrigin).Normalize();

    // 유효한 leaf 노드를 검사하며 교차하는 경우 컴포넌트들을 추가
    for (FOctree* leaf : GetValidLeafNodes()) {
        float dist = 0;
        if (leaf->BoundingBox.Intersect(pickRayOrigin, rayDirection, dist)) {
            for (UStaticMeshComponent* Comp : leaf->GetPrimitiveComponents()) {
                CandidateComponents.Add(Comp);
            }
        }
    }
    return CandidateComponents;
}

TArray<FOctree*> FOctree::GetValidLeafNodes()
{
    TArray<FOctree*> ValidLeafNodes;

    if (IsLeafNode())
    {
        if (PrimitiveComponents.Num() > 0)
        {
            ValidLeafNodes.Add(this);
        }
    }
    else
    {
        for (auto& Child : Children)
        {
            ValidLeafNodes.Append(Child->GetValidLeafNodes());
        }
    }
    return ValidLeafNodes;
}
TArray<FOctree*> FOctree::CollectCandidateNodes(const FVector& pickPos, const FMatrix& viewMatrix)
{
    TArray<FOctree*> CandidateNodes;
    FVector cameraOrigin = { 0, 0, 0 };
    FMatrix inverseMatrix = FMatrix::Inverse(viewMatrix);
    FVector pickRayOrigin = inverseMatrix.TransformPosition(cameraOrigin);
    FVector transformedPick = inverseMatrix.TransformPosition(pickPos);
    FVector rayDirection = (transformedPick - pickRayOrigin).Normalize();

    // 유효한 leaf 노드들에 대해 ray와 교차하는지 검사
    for (FOctree* leaf : GetValidLeafNodes()) {
        float dist = 0;
        if (leaf->BoundingBox.Intersect(pickRayOrigin, rayDirection, dist)) {
            CandidateNodes.Add(leaf);
        }
    }
    return CandidateNodes;
}

FBoundingBox FOctree::CalculateChildBoundingBox(int index)
{
    //0이면 min~mid 1이면 mid~max
    int OffsetX = (index & 0x1) ? 1 : 0; // index의 3번째 비트 (4의 자리)
    int OffsetY = (index & 0x2) ? 1 : 0; // index의 2번째 비트 (2의 자리)
    int OffsetZ = (index & 0x4) ? 1 : 0; // index의 1번째 비트 (1의 자리)

    FBoundingBox childBox;
    childBox.min.x = BoundingBox.min.x + HalfSize.x * OffsetX;
    childBox.min.y = BoundingBox.min.y + HalfSize.y * OffsetY;
    childBox.min.z = BoundingBox.min.z + HalfSize.z * OffsetZ;

    childBox.max.x = childBox.min.x + HalfSize.x;
    childBox.max.y = childBox.min.y + HalfSize.y;
    childBox.max.z = childBox.min.z + HalfSize.z;


    return childBox;
}