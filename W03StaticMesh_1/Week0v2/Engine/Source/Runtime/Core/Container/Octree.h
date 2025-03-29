#pragma once
#include "Array.h"
#include "Define.h"

class UPrimitiveComponent;

class FOctree
{
public:
    FOctree() = default;
    FOctree(const FBoundingBox& InBoundingBox);
    ~FOctree();
    void AddComponent(UPrimitiveComponent* InComponent);
    void SetDepth(int InDepth) {Depth = InDepth;}
    void SubDivide();
    int CalculteChildIndex(FVector Pos);

    bool IsLeapNode(){ return Children.Num() == 0; }
    FBoundingBox CalculateChildBoundingBox(int index);

private:
    FBoundingBox BoundingBox;
    FVector HalfSize;
    TArray<FOctree*> Children;
    TArray<UPrimitiveComponent*> PrimitiveComponents;

    int DivideThreshold = 100;
    int Depth = 0;
    int MaxDepth = 10;
};
