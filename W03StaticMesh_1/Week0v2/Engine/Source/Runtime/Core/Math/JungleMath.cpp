#include "Engine/Source/Runtime/Core/Math/JungleMath.h"
#include <DirectXMath.h>

#include "MathUtility.h"

using namespace DirectX;
FVector4 JungleMath::ConvertV3ToV4(FVector vec3)
{
	FVector4 newVec4;
	newVec4.x = vec3.x;
	newVec4.y = vec3.y;
	newVec4.z = vec3.z;
	return newVec4;
}



FMatrix JungleMath::CreateModelMatrix(FVector translation, FVector rotation, FVector scale)
{
    FMatrix Translation = FMatrix::CreateTranslationMatrix(translation);

    FMatrix Rotation = FMatrix::CreateRotation(rotation.x, rotation.y, rotation.z);
    //FMatrix Rotation = JungleMath::EulerToQuaternion(rotation).ToMatrix();

    FMatrix Scale = FMatrix::CreateScale(scale.x, scale.y, scale.z);
    return Scale * Rotation * Translation;
}

FMatrix JungleMath::CreateModelMatrix(FVector translation, FQuat rotation, FVector scale)
{
    FMatrix Translation = FMatrix::CreateTranslationMatrix(translation);
    FMatrix Rotation = rotation.ToMatrix();
    FMatrix Scale = FMatrix::CreateScale(scale.x, scale.y, scale.z);
    return Scale * Rotation * Translation;
}
FMatrix JungleMath::CreateViewMatrix(FVector eye, FVector target, FVector up)
{
    FVector zAxis = (target - eye).Normalize();  // DirectX는 LH이므로 -z가 아니라 +z 사용
    FVector xAxis = (up.Cross(zAxis)).Normalize();
    FVector yAxis = zAxis.Cross(xAxis);

    FMatrix View;
    View.M[0][0] = xAxis.x; View.M[0][1] = yAxis.x; View.M[0][2] = zAxis.x; View.M[0][3] = 0;
    View.M[1][0] = xAxis.y; View.M[1][1] = yAxis.y; View.M[1][2] = zAxis.y; View.M[1][3] = 0;
    View.M[2][0] = xAxis.z; View.M[2][1] = yAxis.z; View.M[2][2] = zAxis.z; View.M[2][3] = 0;
    View.M[3][0] = -xAxis.Dot(eye);
    View.M[3][1] = -yAxis.Dot(eye);
    View.M[3][2] = -zAxis.Dot(eye);
    View.M[3][3] = 1;

    return View;
}

FMatrix JungleMath::CreateProjectionMatrix(float fov, float aspect, float nearPlane, float farPlane)
{
    float tanHalfFOV = tan(fov / 2.0f);
    float depth = farPlane - nearPlane;

    FMatrix Projection = {};
    Projection.M[0][0] = 1.0f / (aspect * tanHalfFOV);
    Projection.M[1][1] = 1.0f / tanHalfFOV;
    Projection.M[2][2] = farPlane / depth;
    Projection.M[2][3] = 1.0f;
    Projection.M[3][2] = -(nearPlane * farPlane) / depth;
    Projection.M[3][3] = 0.0f;  

    return Projection;
}

FMatrix JungleMath::CreateOrthoProjectionMatrix(float width, float height, float nearPlane, float farPlane)
{
    float r = width * 0.5f;
    float t = height * 0.5f;
    float invDepth = 1.0f / (farPlane - nearPlane);

    FMatrix Projection = {};
    Projection.M[0][0] = 1.0f / r;
    Projection.M[1][1] = 1.0f / t;
    Projection.M[2][2] = invDepth;
    Projection.M[3][2] = -nearPlane * invDepth;
    Projection.M[3][3] = 1.0f;

    return Projection;
}

FVector JungleMath::FVectorRotate(FVector& origin, const FVector& rotation)
{
    FQuat quaternion = JungleMath::EulerToQuaternion(rotation);
    // 쿼터니언을 이용해 벡터 회전 적용
    return quaternion.RotateVector(origin);
}
FQuat JungleMath::EulerToQuaternion(const FVector& eulerDegrees)
{
    float yaw = DegToRad(eulerDegrees.z);   // Z축 Yaw
    float pitch = DegToRad(eulerDegrees.y); // Y축 Pitch
    float roll = DegToRad(eulerDegrees.x);  // X축 Roll

    float halfYaw = yaw * 0.5f;
    float halfPitch = pitch * 0.5f;
    float halfRoll = roll * 0.5f;

    float cosYaw = cos(halfYaw);
    float sinYaw = sin(halfYaw);
    float cosPitch = cos(halfPitch);
    float sinPitch = sin(halfPitch);
    float cosRoll = cos(halfRoll);
    float sinRoll = sin(halfRoll);

    FQuat quat;
    quat.w = cosYaw * cosPitch * cosRoll + sinYaw * sinPitch * sinRoll;
    quat.x = cosYaw * cosPitch * sinRoll - sinYaw * sinPitch * cosRoll;
    quat.y = cosYaw * sinPitch * cosRoll + sinYaw * cosPitch * sinRoll;
    quat.z = sinYaw * cosPitch * cosRoll - cosYaw * sinPitch * sinRoll;

    quat.Normalize();
    return quat;
}

FVector JungleMath::QuaternionToEuler(const FQuat& quat)
{

    // 1. FQuat을 XMVECTOR로 변환하고 정규화
//    DirectXMath 함수는 단위 쿼터니언을 기대합니다.
    DirectX::XMVECTOR q = DirectX::XMVectorSet(quat.x, quat.y, quat.z, quat.w);
    q = DirectX::XMQuaternionNormalize(q);

    // 2. 쿼터니언을 회전 행렬로 변환
    DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationQuaternion(q);

    // 3. 행렬에서 오일러 각도 추출 (Yaw -> Pitch -> Roll 순서, ZYX Intrinsic)
    //    DirectXMath는 왼손 좌표계를 사용하며, 행렬 요소 접근이 필요합니다.
    //    XMFLOAT4X4로 저장하면 요소 접근이 용이합니다.
    DirectX::XMFLOAT4X4 matrixData;
    DirectX::XMStoreFloat4x4(&matrixData, rotationMatrix);

    FVector euler;
    float pitch;

    // Pitch (Y축 회전) 추출 - 행렬의 M[2][0] 요소 (또는 _31) 사용
    // sin(pitch) = -M[2][0]
    float sinPitch = -matrixData._31;

    // 짐벌락(Gimbal Lock) 체크 및 처리
    // sinPitch 값이 거의 +1 또는 -1에 가까울 때 발생
    const float gimbalLockEpsilon = 0.99999f; // 허용 오차
    if (fabsf(sinPitch) > gimbalLockEpsilon)
    {
        // 짐벌락 상태: Pitch는 +90 또는 -90도
        pitch = copysignf(DirectX::XM_PIDIV2, sinPitch); // +/- PI/2 (라디안)

        // 이 경우, Roll은 0으로 설정하고 Yaw만 계산하는 것이 일반적이지만,
        // 원본 코드의 방식을 최대한 따르려면 Yaw와 Roll도 계산 시도
        // (단, 짐벌락 상태에서는 Yaw와 Roll의 합 또는 차만 의미 있을 수 있음)

        // Yaw (Z축 회전): atan2(M[1][0], M[0][0]) 또는 다른 공식 사용 (짐벌락 공식)
        // 짐벌락 시 일반적인 공식: Yaw = atan2(-M[0][1], M[0][2]) 등
        // 여기서는 원본 코드가 짐벌락 시에도 일반 공식을 사용하는 것처럼 보이므로 일단 그대로 계산
        // Yaw = atan2(M[1][0], M[0][0]) -> atan2(_21, _11)
        euler.z = atan2f(matrixData._21, matrixData._11);

        // Roll (X축 회전): 원본 방식처럼 0으로 설정하거나 일반 공식 사용
        // Roll = atan2(M[2][1], M[2][2]) -> atan2(_32, _33)
        euler.x = atan2f(matrixData._32, matrixData._33); // 원본 코드는 이 공식을 사용
        // euler.x = 0.0f; // 짐벌락 시 Roll을 0으로 고정하는 관례도 있음
    }
    else
    {
        // 짐벌락 아님: 일반적인 방법으로 계산
        pitch = asinf(sinPitch);

        // Yaw (Z축 회전): atan2(M[1][0], M[0][0]) -> atan2(_21, _11)
        euler.z = atan2f(matrixData._21, matrixData._11);

        // Roll (X축 회전): atan2(M[2][1], M[2][2]) -> atan2(_32, _33)
        euler.x = atan2f(matrixData._32, matrixData._33);
    }

    // 4. 라디안을 각도(Degree)로 변환하고 FVector에 저장
    euler.x = DirectX::XMConvertToDegrees(euler.x); // Roll
    euler.y = DirectX::XMConvertToDegrees(pitch);   // Pitch
    euler.z = DirectX::XMConvertToDegrees(euler.z); // Yaw

    return euler;

    //FVector euler;

    //// 쿼터니언 정규화
    //FQuat q = quat;
    //q.Normalize();

    //// Yaw (Z 축 회전)
    //float sinYaw = 2.0f * (q.w * q.z + q.x * q.y);
    //float cosYaw = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    //euler.z = RadToDeg(atan2(sinYaw, cosYaw));

    //// Pitch (Y 축 회전, 짐벌락 방지)
    //float sinPitch = 2.0f * (q.w * q.y - q.z * q.x);
    //if (fabs(sinPitch) >= 1.0f)
    //{
    //    euler.y = R2D * (static_cast<float>(copysign(PI / 2, sinPitch))); // 🔥 Gimbal Lock 방지
    //}
    //else
    //{
    //    euler.y = R2D * (asin(sinPitch));
    //}

    //// Roll (X 축 회전)
    //float sinRoll = 2.0f * (q.w * q.x + q.y * q.z);
    //float cosRoll = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    //euler.x = R2D * (atan2(sinRoll, cosRoll));
    //return euler;
}


FVector JungleMath::FVectorRotate(FVector& origin, const FQuat& rotation)
{
    return rotation.RotateVector(origin);
}

FMatrix JungleMath::CreateRotationMatrix(FVector rotation)
{
    XMVECTOR quatX = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), DegToRad(rotation.x));
    XMVECTOR quatY = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), DegToRad(rotation.y));
    XMVECTOR quatZ = XMQuaternionRotationAxis(XMVectorSet(0, 0, 1, 0), DegToRad(rotation.z));

    XMVECTOR rotationQuat = XMQuaternionMultiply(quatZ, XMQuaternionMultiply(quatY, quatX));
    rotationQuat = XMQuaternionNormalize(rotationQuat);  // 정규화 필수

    XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotationQuat);
    FMatrix result = FMatrix::Identity;  // 기본값 설정 (단위 행렬)

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            result.M[i][j] = rotationMatrix.r[i].m128_f32[j];  // XMMATRIX에서 FMatrix로 값 복사
        }
    }
    return result;
}


float JungleMath::RadToDeg(float radian)
{
    return static_cast<float>(radian * (180.0f / PI));
}

float JungleMath::DegToRad(float degree)
{
    return static_cast<float>(degree * (PI / 180.0f));
}
