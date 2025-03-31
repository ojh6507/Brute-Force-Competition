#include "Player.h"

#include "UnrealClient.h"
#include "World.h"
#include "BaseGizmos/GizmoArrowComponent.h"
#include "BaseGizmos/GizmoCircleComponent.h"
#include "BaseGizmos/GizmoRectangleComponent.h"
#include "BaseGizmos/TransformGizmo.h"
#include "Camera/CameraComponent.h"
#include "Components/LightComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Math/JungleMath.h"
#include "Math/MathUtility.h"
#include "PropertyEditor/ShowFlags.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"


using namespace DirectX;

AEditorPlayer::AEditorPlayer()
{
}

void AEditorPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    Input();
}

void AEditorPlayer::Input()
{
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
    {
        if (!bLeftMouseDown)
        {
            POINT mousePos;
            GetCursorPos(&mousePos);
            ScreenToClient(GetEngine().hWnd, &mousePos);
            if (mousePos.x <0 || mousePos.x > FEngineLoop::graphicDevice.SwapchainDesc.BufferDesc.Width)
            {
                return;
            }

            if (mousePos.y < 0 || mousePos.y > FEngineLoop::graphicDevice.SwapchainDesc.BufferDesc.Height)
            {
                return;
            }

            FScopeCycleCounter pickCounter;
            ++TotalPickCount;

            bLeftMouseDown = true;
            
            uint32_t PickUUID = FEngineLoop::graphicDevice.UUIDBuffer[mousePos.y][mousePos.x];
            
            if (PickUUID == 0)
            {
                GetWorld()->SetPickedPrimitive(nullptr);
            }else
            {
                if (StaticMeshComponentMap[PickUUID])
                {
                    GetWorld()->SetPickedPrimitive(StaticMeshComponentMap[PickUUID]);
                    LastPickTime = pickCounter.Finish();
                    TotalPickTime += LastPickTime;
                }
            }            
        }
    }
    else if (bLeftMouseDown)
    {
        bLeftMouseDown = false; // ���콺 ������ ��ư�� ���� ���� �ʱ�ȭ
    }
    else if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        if (!bRightMouseDown)
        {
            bRightMouseDown = true;
        }
    }
    else
    {
        bRightMouseDown = false;
    }
}
bool AEditorPlayer::PickActor(const FVector& pickPosition, std::shared_ptr<FEditorViewportClient> ActiveViewport)
{
    UPrimitiveComponent* Possible = nullptr;
    float minDistance = FLT_MAX;
    int maxIntersect = 0;

    FMatrix viewMatrix = ActiveViewport->GetViewMatrix();
    FMatrix invViewMatrix = FMatrix::Inverse(viewMatrix);
    FVector pickRayOrigin = invViewMatrix.TransformPosition(FVector(0, 0, 0));
    FVector transformedPick = invViewMatrix.TransformPosition(pickPosition);
    FVector rayDirection = (transformedPick - pickRayOrigin).Normalize();

    FVector CameraPos = ActiveViewport->ViewTransformPerspective.GetLocation();

    TArray<std::pair<FBVH*, float>> hitLeaves;

    GEngineLoop.GetWorld()->GetRootBVH()->RayCheck(pickRayOrigin, rayDirection, hitLeaves);

    for (const TPair<FBVH*, float>& pair : hitLeaves)
    {
        FBVH* leaf = pair.Key;
        TArray<UStaticMeshComponent*> comps = GEngineLoop.GetWorld()->GetRootBVH()->CollectCandidateComponentsByLeaf(leaf);

        for (UStaticMeshComponent* comp : comps)
        {
            float Distance = 0.0f;
            int currentIntersectCount = 0;

            if (RayIntersectsObject(pickPosition, comp, Distance, currentIntersectCount))
            {
                if (Distance < minDistance && currentIntersectCount > 0 ||
                    (FMath::Abs(Distance - minDistance) < FLT_EPSILON && currentIntersectCount> 1))
                {
                    minDistance = Distance;
                    maxIntersect = currentIntersectCount;
                    Possible = comp;
                }
            }
        }
        if (Possible)
        {
            GetWorld()->SetPickedPrimitive(Possible);
            return true;
        }
    }

   
    return false;

}

void AEditorPlayer::AddControlMode()
{
    cMode = static_cast<ControlMode>((cMode + 1) % CM_END);
}

void AEditorPlayer::AddCoordiMode()
{
    cdMode = static_cast<CoordiMode>((cdMode + 1) % CDM_END);
}

void AEditorPlayer::ScreenToViewSpace(int screenX, int screenY, const FMatrix& viewMatrix, const FMatrix& projectionMatrix, FVector& pickPosition)
{
    D3D11_VIEWPORT viewport = GetEngine().GetLevelEditor()->GetActiveViewportClient()->GetD3DViewport();

    float viewportX = screenX - viewport.TopLeftX;
    float viewportY = screenY - viewport.TopLeftY;

    pickPosition.x = ((2.0f * viewportX / viewport.Width) - 1) / projectionMatrix[0][0];
    pickPosition.y = -((2.0f * viewportY / viewport.Height) - 1) / projectionMatrix[1][1];

    pickPosition.z = 1.0f;  // 퍼스펙티브 모드: near plane

}

int AEditorPlayer::RayIntersectsObject(const FVector& pickPosition, USceneComponent* obj, float& hitDistance, int& intersectCount)
{

    FMatrix viewMatrix = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetViewMatrix();
    FMatrix inverseMatrix = FMatrix::Inverse(obj->Model * viewMatrix);
    FVector cameraOrigin = { 0,0,0 };
    FVector pickRayOrigin = inverseMatrix.TransformPosition(cameraOrigin);
    // 퍼스펙티브 모드의 기존 로직 사용
    FVector transformedPick = inverseMatrix.TransformPosition(pickPosition);
    FVector rayDirection = (transformedPick - pickRayOrigin).Normalize();
    intersectCount = obj->CheckRayIntersection(pickRayOrigin, rayDirection, hitDistance);
    return intersectCount;
}


void AEditorPlayer::PickedObjControl()
{
    if (GetWorld()->GetSelectedActor() && GetWorld()->GetPickingGizmo())
    {
        POINT currentMousePos;
        GetCursorPos(&currentMousePos);
        int32 deltaX = currentMousePos.x - m_LastMousePos.x;
        int32 deltaY = currentMousePos.y - m_LastMousePos.y;

        // USceneComponent* pObj = GetWorld()->GetPickingObj();
        AActor* PickedActor = GetWorld()->GetSelectedActor();
        UGizmoBaseComponent* Gizmo = static_cast<UGizmoBaseComponent*>(GetWorld()->GetPickingGizmo());
        switch (cMode)
        {
        case CM_TRANSLATION:
            ControlTranslation(PickedActor->GetRootComponent(), Gizmo, deltaX, deltaY);
            break;
        case CM_SCALE:
            ControlScale(PickedActor->GetRootComponent(), Gizmo, deltaX, deltaY);

            break;
        case CM_ROTATION:
            ControlRotation(PickedActor->GetRootComponent(), Gizmo, deltaX, deltaY);
            break;
        default:
            break;
        }
        m_LastMousePos = currentMousePos;
    }
}

void AEditorPlayer::ControlRotation(USceneComponent* pObj, UGizmoBaseComponent* Gizmo, int32 deltaX, int32 deltaY)
{
    FVector cameraForward = GetWorld()->GetCamera()->GetForwardVector();
    FVector cameraRight = GetWorld()->GetCamera()->GetRightVector();
    FVector cameraUp = GetWorld()->GetCamera()->GetUpVector();

    FQuat currentRotation = pObj->GetQuat();

    FQuat rotationDelta;

    if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleX)
    {
        float rotationAmount = (cameraUp.z >= 0 ? -1.0f : 1.0f) * deltaY * 0.01f;
        rotationAmount = rotationAmount + (cameraRight.x >= 0 ? 1.0f : -1.0f) * deltaX * 0.01f;

        rotationDelta = FQuat(FVector(1.0f, 0.0f, 0.0f), rotationAmount); // ���� X �� ���� ȸ��
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleY)
    {
        float rotationAmount = (cameraRight.x >= 0 ? 1.0f : -1.0f) * deltaX * 0.01f;
        rotationAmount = rotationAmount + (cameraUp.z >= 0 ? 1.0f : -1.0f) * deltaY * 0.01f;

        rotationDelta = FQuat(FVector(0.0f, 1.0f, 0.0f), rotationAmount); // ���� Y �� ���� ȸ��
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleZ)
    {
        float rotationAmount = (cameraForward.x <= 0 ? -1.0f : 1.0f) * deltaX * 0.01f;
        rotationDelta = FQuat(FVector(0.0f, 0.0f, 1.0f), rotationAmount); // ���� Z �� ���� ȸ��
    }
    if (cdMode == CDM_LOCAL)
    {
        pObj->SetRotation(currentRotation * rotationDelta);
    }
    else if (cdMode == CDM_WORLD)
    {
        pObj->SetRotation(rotationDelta * currentRotation);
    }
}

void AEditorPlayer::ControlTranslation(USceneComponent* pObj, UGizmoBaseComponent* Gizmo, int32 deltaX, int32 deltaY)
{
    float DeltaX = static_cast<float>(deltaX);
    float DeltaY = static_cast<float>(deltaY);
    auto ActiveViewport = GetEngine().GetLevelEditor()->GetActiveViewportClient();

    FVector CamearRight = ActiveViewport->GetViewportType() == LVT_Perspective ?
        ActiveViewport->ViewTransformPerspective.GetRightVector() : ActiveViewport->ViewTransformOrthographic.GetRightVector();
    FVector CameraUp = ActiveViewport->GetViewportType() == LVT_Perspective ?
        ActiveViewport->ViewTransformPerspective.GetUpVector() : ActiveViewport->ViewTransformOrthographic.GetUpVector();

    FVector WorldMoveDirection = (CamearRight * DeltaX + CameraUp * -DeltaY) * 0.1f;

    if (cdMode == CDM_LOCAL)
    {
        if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowX)
        {
            float moveAmount = WorldMoveDirection.Dot(pObj->GetForwardVector());
            pObj->AddLocation(pObj->GetForwardVector() * moveAmount);
        }
        else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowY)
        {
            float moveAmount = WorldMoveDirection.Dot(pObj->GetRightVector());
            pObj->AddLocation(pObj->GetRightVector() * moveAmount);
        }
        else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowZ)
        {
            float moveAmount = WorldMoveDirection.Dot(pObj->GetUpVector());
            pObj->AddLocation(pObj->GetUpVector() * moveAmount);
        }
    }
    else if (cdMode == CDM_WORLD)
    {
        // 월드 좌표계에서 카메라 방향을 고려한 이동
        if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowX)
        {
            // 카메라의 오른쪽 방향을 X축 이동에 사용
            FVector moveDir = CamearRight * DeltaX * 0.05f;
            pObj->AddLocation(FVector(moveDir.x, 0.0f, 0.0f));
        }
        else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowY)
        {
            // 카메라의 오른쪽 방향을 Y축 이동에 사용
            FVector moveDir = CamearRight * DeltaX * 0.05f;
            pObj->AddLocation(FVector(0.0f, moveDir.y, 0.0f));
        }
        else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowZ)
        {
            // 카메라의 위쪽 방향을 Z축 이동에 사용
            FVector moveDir = CameraUp * -DeltaY * 0.05f;
            pObj->AddLocation(FVector(0.0f, 0.0f, moveDir.z));
        }
    }
}

void AEditorPlayer::ControlScale(USceneComponent* pObj, UGizmoBaseComponent* Gizmo, int32 deltaX, int32 deltaY)
{
    float DeltaX = static_cast<float>(deltaX);
    float DeltaY = static_cast<float>(deltaY);
    auto ActiveViewport = GetEngine().GetLevelEditor()->GetActiveViewportClient();

    FVector CamearRight = ActiveViewport->GetViewportType() == LVT_Perspective ?
        ActiveViewport->ViewTransformPerspective.GetRightVector() : ActiveViewport->ViewTransformOrthographic.GetRightVector();
    FVector CameraUp = ActiveViewport->GetViewportType() == LVT_Perspective ?
        ActiveViewport->ViewTransformPerspective.GetUpVector() : ActiveViewport->ViewTransformOrthographic.GetUpVector();

    // 월드 좌표계에서 카메라 방향을 고려한 이동
    if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleX)
    {
        // 카메라의 오른쪽 방향을 X축 이동에 사용
        FVector moveDir = CamearRight * DeltaX * 0.05f;
        pObj->AddScale(FVector(moveDir.x, 0.0f, 0.0f));
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleY)
    {
        // 카메라의 오른쪽 방향을 Y축 이동에 사용
        FVector moveDir = CamearRight * DeltaX * 0.05f;
        pObj->AddScale(FVector(0.0f, moveDir.y, 0.0f));
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleZ)
    {
        // 카메라의 위쪽 방향을 Z축 이동에 사용
        FVector moveDir = CameraUp * -DeltaY * 0.05f;
        pObj->AddScale(FVector(0.0f, 0.0f, moveDir.z));
    }
}
