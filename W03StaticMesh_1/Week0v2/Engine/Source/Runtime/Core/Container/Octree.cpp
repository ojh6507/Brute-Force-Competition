#include "Octree.h"

#include <complex.h>

#include "Components/PrimitiveComponent.h"

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

void FOctree::AddComponent(UPrimitiveComponent* InComponent)
{
    if (IsLeapNode())
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

    for (int i=0;i<8;i++)
    {
        FBoundingBox childBox = CalculateChildBoundingBox(i);
        Children.Add(new FOctree(childBox));
        Children[i]->Depth = Depth + 1;
    }
    
    for (auto& Component: PrimitiveComponents)
    {
        int ChildBoundingBoxIndex = CalculteChildIndex(Component->GetWorldLocation());
        Children[ChildBoundingBoxIndex]->AddComponent(Component);
    }
            
    //Component들 자식에게 물려주고 클리어
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
    childBox.min.x = BoundingBox.min.x + (HalfSize.x * OffsetX);
    childBox.min.y = BoundingBox.min.y + (HalfSize.y * OffsetY);
    childBox.min.z = BoundingBox.min.z + (HalfSize.z * OffsetZ);

    childBox.max.x = BoundingBox.max.x - (HalfSize.x * !OffsetX);
    childBox.max.y = BoundingBox.max.y - (HalfSize.y * !OffsetY);
    childBox.max.z = BoundingBox.max.z - (HalfSize.z * !OffsetZ);

    return childBox;
}