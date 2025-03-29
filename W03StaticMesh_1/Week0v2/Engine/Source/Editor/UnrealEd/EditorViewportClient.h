#pragma once
#include <sstream>

#include "Define.h"
#include "Container/Map.h"
#include "UObject/ObjectMacros.h"
#include "ViewportClient.h"
#include "EngineLoop.h"
#include "EngineBaseTypes.h"

#define MIN_ORTHOZOOM				1.0							/* 2D ortho viewport zoom >= MIN_ORTHOZOOM */
#define MAX_ORTHOZOOM				1e25	

extern FEngineLoop GEngineLoop;


struct Plane {
    float a, b, c, d; // 평면 방정식: ax + by + cz + d = 0
};

struct FViewportCameraTransform
{
private:

public:

    FVector GetForwardVector();
    FVector GetRightVector();
    FVector GetUpVector();

public:
    FViewportCameraTransform();

    /** Sets the transform's location */
    void SetLocation(const FVector& Position)
    {
        ViewLocation = Position;
    }

    /** Sets the transform's rotation */
    void SetRotation(const FVector& Rotation)
    {
        ViewRotation = Rotation;
    }

    /** Sets the location to look at during orbit */
    void SetLookAt(const FVector& InLookAt)
    {
        LookAt = InLookAt;
    }

    /** Set the ortho zoom amount */
    void SetOrthoZoom(float InOrthoZoom)
    {
        assert(InOrthoZoom >= MIN_ORTHOZOOM && InOrthoZoom <= MAX_ORTHOZOOM);
        OrthoZoom = InOrthoZoom;
    }

    /** Check if transition curve is playing. */
 /*    bool IsPlaying();*/

    /** @return The transform's location */
    FORCEINLINE const FVector& GetLocation() const { return ViewLocation; }

    /** @return The transform's rotation */
    FORCEINLINE const FVector& GetRotation() const { return ViewRotation; }

    /** @return The look at point for orbiting */
    FORCEINLINE const FVector& GetLookAt() const { return LookAt; }

    /** @return The ortho zoom amount */
    FORCEINLINE float GetOrthoZoom() const { return OrthoZoom; }

public:
    /** Current viewport Position. */
    FVector	ViewLocation;
    /** Current Viewport orientation; valid only for perspective projections. */
    FVector ViewRotation;
    FVector	DesiredLocation;
    /** When orbiting, the point we are looking at */
    FVector LookAt;
    /** Viewport start location when animating to another location */
    FVector StartLocation;
    /** Ortho zoom amount */
    float OrthoZoom;
};

class FEditorViewportClient : public FViewportClient
{
public:
    FEditorViewportClient();
    ~FEditorViewportClient();

    virtual void        Draw(FViewport* Viewport) override;
    virtual UWorld* GetWorld() const { return NULL; };
    void Initialize(int32 viewportIndex);
    void Tick(float DeltaTime);
    void Release();

    void Input(float DeltaTime);
    void ResizeViewport(const DXGI_SWAP_CHAIN_DESC& swapchaindesc);
    void ResizeViewport(FRect Top, FRect Bottom, FRect Left, FRect Right);

    bool IsSelected(POINT point);
protected:
    /** Camera speed setting */
    int32 CameraSpeedSetting = 1;
    /** Camera speed scalar */
    float CameraSpeedScalar = 1.0f;
    float GridSize;

public:
    FViewport* Viewport;
    int32 ViewportIndex;
    FViewport* GetViewport() { return Viewport; }
    D3D11_VIEWPORT& GetD3DViewport();


public:
    //카메라
    /** Viewport camera transform data for perspective viewports */
    FViewportCameraTransform		ViewTransformPerspective;
    FViewportCameraTransform        ViewTransformOrthographic;
    // 카메라 정보 
    float ViewFOV = 60.0f;

    float AspectRatio;
    float nearPlane = 0.1f;
    float farPlane = 1000000.0f;
    static FVector Pivot;
    static float orthoSize;
    ELevelViewportType ViewportType;
    uint64 ShowFlag;
    EViewModeIndex ViewMode;

    FMatrix View;
    FMatrix Projection;
public: //Camera Movement
    void CameraMoveForward(float _Value);
    void CameraMoveRight(float _Value);
    void CameraMoveUp(float _Value);
    void CameraRotateYaw(float _Value);
    void CameraRotatePitch(float _Value);
    void PivotMoveRight(float _Value);
    void PivotMoveUp(float _Value);

    FMatrix& GetViewMatrix() { return  View; }
    FMatrix& GetProjectionMatrix() { return Projection; }
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

 
    Plane PlaneFromPoints(const FVector& p0, const FVector& p1, const FVector& p2)
    {
        // 두 벡터의 외적을 이용해 법선 구하기
        FVector normal = (p1 - p0).Cross(p2 - p0);
        normal.Normalize();
        // 평면 방정식: ax + by + cz + d = 0, d = -n·p0
        float d = -normal.Dot(p0);
        return Plane(normal.x, normal.y, normal.z, d);
    }
    void ExtractFrustumPlanesDirect(Plane(&planes)[6]);
    // AABB의 8개 꼭짓점을 구하는 헬퍼 함수
    void GetAABBVertices(const FBoundingBox& box, FVector vertices[8])
    {
        vertices[0] = FVector(box.min.x, box.min.y, box.min.z);
        vertices[1] = FVector(box.max.x, box.min.y, box.min.z);
        vertices[2] = FVector(box.min.x, box.max.y, box.min.z);
        vertices[3] = FVector(box.min.x, box.min.y, box.max.z);
        vertices[4] = FVector(box.max.x, box.max.y, box.min.z);
        vertices[5] = FVector(box.min.x, box.max.y, box.max.z);
        vertices[6] = FVector(box.max.x, box.min.y, box.max.z);
        vertices[7] = FVector(box.max.x, box.max.y, box.max.z);
    }
    void GetBoundingSphere(const FBoundingBox& box, FVector& outCenter, float& outRadius)
    {
        outCenter.x = (box.min.x + box.max.x) * 0.5f;
        outCenter.y = (box.min.y + box.max.y) * 0.5f;
        outCenter.z = (box.min.z + box.max.z) * 0.5f;

        // AABB의 한 꼭짓점과 중심 사이의 거리로 반지름 결정
        FVector diff = box.max - outCenter;
        outRadius = (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    }
    bool IsSphereInsideFrustum(const Plane planes[6], const FVector& center, float radius)
    {
        for (int i = 0; i < 6; ++i)
        {
            // 평면 방정식: ax + by + cz + d
            float ddistance = planes[i].a * center.x +
                planes[i].b * center.y +
                planes[i].c * center.z +
                planes[i].d;

            // 거리가 -radius보다 작으면 구가 평면 밖에 있음
            if (ddistance < -radius)
                return false;
        }
        return true;
    }

    bool IsAABBVisible(const Plane planes[6], const FBoundingBox& box)
    {
        FVector center;
        float radius;
        GetBoundingSphere(box, center, radius);

        return IsSphereInsideFrustum(planes, center, radius);
    }

    bool IsOrtho() const;
    bool IsPerspective() const;
    ELevelViewportType GetViewportType() const;
    void SetViewportType(ELevelViewportType InViewportType);
    void UpdateOrthoCameraLoc();
    EViewModeIndex GetViewMode() { return ViewMode; }
    void SetViewMode(EViewModeIndex newMode) { ViewMode = newMode; }
    uint64 GetShowFlag() { return ShowFlag; }
    void SetShowFlag(uint64 newMode) { ShowFlag = newMode; }
    bool GetIsOnRBMouseClick() { return bRightMouseDown; }

    //Flag Test Code
    static void SetOthoSize(float _Value);
private: // Input
    POINT lastMousePos;
    bool bRightMouseDown = false;


public:
    void LoadConfig(const TMap<FString, FString>& config);
    void SaveConfig(TMap<FString, FString>& config);
private:
    TMap<FString, FString> ReadIniFile(const FString& filePath);
    void WriteIniFile(const FString& filePath, const TMap<FString, FString>& config);

public:
    PROPERTY(int32, CameraSpeedSetting)
        PROPERTY(float, GridSize)
        float GetCameraSpeedScalar() const { return CameraSpeedScalar; };
    void SetCameraSpeedScalar(float value);

private:
    template <typename T>
    T GetValueFromConfig(const TMap<FString, FString>& config, const FString& key, T defaultValue) {
        if (const FString* Value = config.Find(key))
        {
            std::istringstream iss(**Value);
            T value;
            if (iss >> value)
            {
                return value;
            }
        }
        return defaultValue;
    }
};

