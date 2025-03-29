#include "Octree.h"

#include <complex.h>

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "Math/JungleMath.h"
FOctree::FOctree(const FBoundingBox& InBoundingBox){
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
    if (IsLeafNode())
    {
        PrimitiveComponents.Add(InComponent);

        if (PrimitiveComponents.Num() > DivideThreshold)
        {
            SubDivide();
        }
    }else
    {
        int ChildBoundingBoxIndex = CalculteChildIndex(InComponent->GetWorldLocation());
        Children[ChildBoundingBoxIndex]->AddComponent(InComponent);
    }
}

void FOctree::SubDivide()
{
    if (Depth >= MaxDepth)
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
        int ChildIndex = CalculteChildIndex(Component->GetWorldLocation());
        // 인덱스 범위 확인 (필요 시 추가 검증)
        if (ChildIndex >= 0 && ChildIndex < Children.Num())
        {
            Children[ChildIndex]->AddComponent(Component);
        }
        else
        {
            // 인덱스가 예상 범위를 벗어난 경우 처리 (예: 로그 출력)
            OutputDebugString(L"Invalid child index in Octree::SubDivide\n");
        }
    }

    // 현재 노드의 컴포넌트 목록은 자식으로 모두 이동했으므로 비움
    PrimitiveComponents.Empty();
}

int FOctree::CalculteChildIndex(FVector Pos)
{
    int ReturnIndex = 0;
    ReturnIndex |= Pos.x > (BoundingBox.min.x + HalfSize.x) ? 1 : 0;
    ReturnIndex |= Pos.y > (BoundingBox.min.y + HalfSize.y) ? 2 : 0;
    ReturnIndex |= Pos.z > (BoundingBox.min.z + HalfSize.z) ? 4 : 0;

    return ReturnIndex;
}
void FOctree::CollectIntersectingComponents(const Plane frustumPlanes[6], TArray<UStaticMeshComponent*>& OutComponents)
{
    // 현재 노드의 경계 상자가 절두체와 겹치는지 체크
    if (!BoundingBox.IsIntersectingFrustum(frustumPlanes)) {
        return;
    }

    // 리프 노드인 경우 해당 컴포넌트를 결과에 추가
    if (IsLeafNode()) {
        for (UStaticMeshComponent* Component : PrimitiveComponents) {
         /*   FMatrix model = JungleMath::CreateModelMatrix(Component->GetWorldLocation(), Component->GetWorldRotation(), Component->GetWorldScale());
            if (Component->GetBoundingBox().TransformWorld(model).IsIntersectingFrustum(frustumPlanes)) {
                OutComponents.Add(Component);
            }*/
        }
    }
    else {
        // 자식 노드에 대해 재귀 호출
        for (FOctree* Child : Children) {
            Child->CollectIntersectingComponents(frustumPlanes, OutComponents);
        }
    }
    UPrimitiveBatch::GetInstance().RenderAABB(
        BoundingBox,
        (0, 0, 0),
        FMatrix::Identity
    );
}

TArray<FOctree*> FOctree::GetValidLeafNodes()
{
    TArray<FOctree*> ValidLeafNodes;

    if (IsLeapNode())
    {
        if (PrimitiveComponents.Num() > 0)
        {
            ValidLeafNodes.Add(this);
        }
    }else
    {
        for (auto& Child : Children)
        {
            ValidLeafNodes.Append(Child->GetValidLeafNodes());
        }
    }

    return ValidLeafNodes;
}

FBoundingBox FOctree::CalculateChildBoundingBox(int index)
{
    //0이면 min~mid 1이면 mid~max
    int OffsetX = index & 1;
    int OffsetY = index & 2;
    int OffsetZ = index & 4;
    
    FBoundingBox childBox;
    childBox.min.x = BoundingBox.min.x + HalfSize.x * offsetX;
    childBox.min.y = BoundingBox.min.y + HalfSize.y * offsetY;
    childBox.min.z = BoundingBox.min.z + HalfSize.z * offsetZ;

    childBox.max.x = childBox.min.x + HalfSize.x;
    childBox.max.y = childBox.min.y + HalfSize.y;
    childBox.max.z = childBox.min.z + HalfSize.z;


    return childBox;
}