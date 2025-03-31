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
            bLeftMouseDown = true;

            POINT mousePos;
            GetCursorPos(&mousePos);
            GetCursorPos(&m_LastMousePos);
            ScreenToClient(GetEngine().hWnd, &mousePos);

            FScopeCycleCounter pickCounter;
            ++TotalPickCount;

            FVector pickPosition;

            const auto& ActiveViewport = GetEngine().GetLevelEditor()->GetActiveViewportClient();
            ScreenToViewSpace(mousePos.x, mousePos.y, ActiveViewport->GetViewMatrix(), ActiveViewport->GetProjectionMatrix(), pickPosition);
            if (PickActor(pickPosition, ActiveViewport))
            {
                LastPickTime = pickCounter.Finish();
                TotalPickTime += LastPickTime;
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


bool AEditorPlayer::PickActor(const FVector& pickPosition, std::shared_ptr<FEditorViewportClient> activeViewport)
{
    UPrimitiveComponent* bestComponent = nullptr;
    float bestDistance = FLT_MAX;
    int bestIntersectCount = 0;

    // 뷰 행렬과 역행렬은 루프 바깥에서 한 번만 계산
    FMatrix viewMatrix = activeViewport->GetViewMatrix();
    FMatrix invViewMatrix = FMatrix::Inverse(viewMatrix);

    // 카메라 원점과 픽 위치를 월드 공간으로 변환
    FVector worldCameraOrigin = invViewMatrix.TransformPosition(FVector(0, 0, 0));
    FVector worldPickPosition = invViewMatrix.TransformPosition(pickPosition);
    FVector worldRayDir = (worldPickPosition - worldCameraOrigin).Normalize();

    TArray<std::pair<FBVH*, float>> hitLeaves;

    // 각 거리 임계값(threshold)마다 후보 노드를 점진적으로 수집
    for (int threshold = 10; threshold < 90; threshold += 10)
    {
        hitLeaves.Empty();
        GEngineLoop.GetWorld()->GetRootBVH()->RayCheck(worldCameraOrigin, worldRayDir, hitLeaves, threshold);

        for (const auto& leafPair : hitLeaves)
        {
            FBVH* leaf = leafPair.first;
            TArray<UStaticMeshComponent*> candidates = GEngineLoop.GetWorld()->GetRootBVH()->CollectCandidateComponentsByLeaf(leaf);

            for (UStaticMeshComponent* comp : candidates)
            {
                float distance = 0.0f;
                int intersectCount = 0;

                // 컴포넌트의 모델 행렬과 뷰 행렬을 합성하여 로컬 공간 변환에 사용
                FMatrix compositeMatrix = comp->Model * viewMatrix;
                FMatrix invComposite = FMatrix::Inverse(compositeMatrix);

                // 월드 공간의 카메라 원점과 픽 위치를 컴포넌트 로컬 공간으로 변환
                FVector localCameraOrigin = invComposite.TransformPosition(FVector(0, 0, 0));
                FVector localPickPosition = invComposite.TransformPosition(pickPosition);
                FVector localRayDir = (localPickPosition - localCameraOrigin).Normalize();

                // 로컬 공간에서의 레이 교차 검사
                intersectCount = comp->CheckRayIntersection(localCameraOrigin, localRayDir, distance);

                if (intersectCount > 0)
                {
                    if ((distance < bestDistance) ||
                        (FMath::Abs(distance - bestDistance) < FLT_EPSILON && intersectCount > bestIntersectCount))
                    {
                        bestDistance = distance;
                        bestIntersectCount = intersectCount;
                        bestComponent = comp;
                    }
                }
            }

            if (bestComponent)
            {
                GetWorld()->SetPickedPrimitive(bestComponent);
                return true;
            }
        }
    }
    return false;
}


//bool AEditorPlayer::PickActor(const FVector& pickPosition, std::shared_ptr<FEditorViewportClient> activeViewport)
//{
//    UPrimitiveComponent* bestComponent = nullptr;
//    float bestDistance = FLT_MAX;
//    int bestIntersectCount = 0;
//
//    // 뷰 행렬과 그 역행렬은 한 번만 계산
//    FMatrix viewMatrix = activeViewport->GetViewMatrix();
//    FMatrix invViewMatrix = FMatrix::Inverse(viewMatrix);
//
//    // 월드 공간에서의 카메라 원점과 픽 위치 계산
//    FVector worldCameraOrigin = invViewMatrix.TransformPosition(FVector(0, 0, 0));
//    FVector worldPickPosition = invViewMatrix.TransformPosition(pickPosition);
//    FVector worldRayDir = (worldPickPosition - worldCameraOrigin).Normalize();
//
//    // 정적 BVH의 평탄화된 노드 배열 재사용 (매 프레임 정렬하지 않음)
//    TArray<FBVH*> flatNodes = GEngineLoop.GetWorld()->FlatNodes;
//    flatNodes.Sort([&](const FBVH* A, const FBVH* B) {
//        return A->BoundingBox.GetDistanceToPoint(worldCameraOrigin) < B->BoundingBox.GetDistanceToPoint(worldCameraOrigin);
//        });
//    // 각 노드를 순회하면서 교차 검사
//    for (int i{}; i < 90; i += 10) {
//
//        for (FBVH* node : flatNodes)
//        {
//           
//
//            float t;
//            if (!node->BoundingBox.Intersect(worldCameraOrigin, worldRayDir, t) || t < 0)
//                continue;
//            if (t > i)
//                continue;
//
//            if (node->IsLeafNode() && node->PrimitiveComponents.Num() > 0)
//            {
//                for (UStaticMeshComponent* comp : node->PrimitiveComponents)
//                {
//                    float distance = 0.0f;
//                    int intersectCount = 0;
//
//                    // 컴포넌트의 모델 행렬과 뷰 행렬을 합성하여 로컬 공간 변환에 사용
//                    FMatrix compositeMatrix = comp->Model * viewMatrix;
//                    FMatrix invComposite = FMatrix::Inverse(compositeMatrix);
//
//                    // 월드 공간의 카메라 원점과 픽 위치를 컴포넌트 로컬 공간으로 변환
//                    FVector localCameraOrigin = invComposite.TransformPosition(FVector(0, 0, 0));
//                    FVector localPickPosition = invComposite.TransformPosition(pickPosition);
//                    FVector localRayDir = (localPickPosition - localCameraOrigin).Normalize();
//
//                    // 로컬 공간에서의 레이 교차 검사
//                    intersectCount = comp->CheckRayIntersection(localCameraOrigin, localRayDir, distance);
//
//                    if (intersectCount > 0)
//                    {
//                        if ((distance < bestDistance) ||
//                            (FMath::Abs(distance - bestDistance) < FLT_EPSILON && intersectCount > bestIntersectCount))
//                        {
//                            bestDistance = distance;
//                            bestIntersectCount = intersectCount;
//                            bestComponent = comp;
//                        }
//                    }
//                }
//
//                if (bestComponent)
//                {
//                    GetWorld()->SetPickedPrimitive(bestComponent);
//                    return true;
//                }
//            }
//
//            // 후보가 발견되었고, 현재 노드의 t가 후보보다 작으면 조기 종료
//            if (bestComponent && t < bestDistance)
//            {
//                GetWorld()->SetPickedPrimitive(bestComponent);
//                return true;
//            }
//        }
//    }
// 
//    return false;
//}

//

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

int AEditorPlayer::RayIntersectsObject(const FVector& pickPosition, UStaticMeshComponent* obj, float& hitDistance, int& intersectCount)
{

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
