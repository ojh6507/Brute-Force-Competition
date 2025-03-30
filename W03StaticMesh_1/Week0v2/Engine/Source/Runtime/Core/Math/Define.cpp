#include "Define.h"
#include "Math//JungleMath.h"

// 단위 행렬 정의
const FMatrix FMatrix::Identity = { {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
} };

// 행렬 덧셈 (SIMD 버전)
FMatrix FMatrix::operator+(const FMatrix& Other) const {
    FMatrix Result;
    Result.DirectXMatrix.r[0] = DirectX::XMVectorAdd(DirectXMatrix.r[0], Other.DirectXMatrix.r[0]);
    Result.DirectXMatrix.r[1] = DirectX::XMVectorAdd(DirectXMatrix.r[1], Other.DirectXMatrix.r[1]);
    Result.DirectXMatrix.r[2] = DirectX::XMVectorAdd(DirectXMatrix.r[2], Other.DirectXMatrix.r[2]);
    Result.DirectXMatrix.r[3] = DirectX::XMVectorAdd(DirectXMatrix.r[3], Other.DirectXMatrix.r[3]);
    return Result;
}

// 행렬 뺄셈 (SIMD 버전)
FMatrix FMatrix::operator-(const FMatrix& Other) const {
    FMatrix Result;
    Result.DirectXMatrix.r[0] = DirectX::XMVectorSubtract(DirectXMatrix.r[0], Other.DirectXMatrix.r[0]);
    Result.DirectXMatrix.r[1] = DirectX::XMVectorSubtract(DirectXMatrix.r[1], Other.DirectXMatrix.r[1]);
    Result.DirectXMatrix.r[2] = DirectX::XMVectorSubtract(DirectXMatrix.r[2], Other.DirectXMatrix.r[2]);
    Result.DirectXMatrix.r[3] = DirectX::XMVectorSubtract(DirectXMatrix.r[3], Other.DirectXMatrix.r[3]);
    return Result;
}

// 행렬 곱셈 (DirectXMath 제공)
FMatrix FMatrix::operator*(const FMatrix& Other) const {
    FMatrix Result;
    Result.DirectXMatrix = DirectX::XMMatrixMultiply(DirectXMatrix, Other.DirectXMatrix);
    return Result;
}

// 스칼라 곱셈 (SIMD 버전)
FMatrix FMatrix::operator*(float Scalar) const {
    FMatrix Result;
    Result.DirectXMatrix.r[0] = DirectX::XMVectorScale(DirectXMatrix.r[0], Scalar);
    Result.DirectXMatrix.r[1] = DirectX::XMVectorScale(DirectXMatrix.r[1], Scalar);
    Result.DirectXMatrix.r[2] = DirectX::XMVectorScale(DirectXMatrix.r[2], Scalar);
    Result.DirectXMatrix.r[3] = DirectX::XMVectorScale(DirectXMatrix.r[3], Scalar);
    return Result;
}

// 스칼라 나눗셈 (SIMD 버전)
FMatrix FMatrix::operator/(float Scalar) const {
    FMatrix Result;
    float invScalar = 1.0f / Scalar;
    Result.DirectXMatrix.r[0] = DirectX::XMVectorScale(DirectXMatrix.r[0], invScalar);
    Result.DirectXMatrix.r[1] = DirectX::XMVectorScale(DirectXMatrix.r[1], invScalar);
    Result.DirectXMatrix.r[2] = DirectX::XMVectorScale(DirectXMatrix.r[2], invScalar);
    Result.DirectXMatrix.r[3] = DirectX::XMVectorScale(DirectXMatrix.r[3], invScalar);
    return Result;
}

float* FMatrix::operator[](int row) {
    return M[row];
}

const float* FMatrix::operator[](int row) const {
    return M[row];
}

// 전치 행렬 (SIMD 버전)
FMatrix FMatrix::Transpose(const FMatrix& Mat) {
    FMatrix Result;
    Result.DirectXMatrix = DirectX::XMMatrixTranspose(Mat.DirectXMatrix);
    return Result;
}

// 행렬식 계산 (DirectXMath 사용)
float FMatrix::Determinant(const FMatrix& Mat) {
    DirectX::XMVECTOR detVec = DirectX::XMMatrixDeterminant(Mat.DirectXMatrix);
    return DirectX::XMVectorGetX(detVec);
}

// 역행렬 (DirectXMath 사용)
FMatrix FMatrix::Inverse(const FMatrix& Mat) {
    FMatrix Inv;
    DirectX::XMVECTOR det;
    Inv.DirectXMatrix = DirectX::XMMatrixInverse(&det, Mat.DirectXMatrix);
    return Inv;
}

// 회전 행렬 생성 (DirectXMath 사용)
FMatrix FMatrix::CreateRotation(float roll, float pitch, float yaw)
{
    // 각도를 라디안으로 변환
    float radRoll = roll * (3.14159265359f / 180.0f);
    float radPitch = pitch * (3.14159265359f / 180.0f);
    float radYaw = yaw * (3.14159265359f / 180.0f);

    // DirectXMath의 XMMatrixRotationRollPitchYaw 함수는 (Pitch, Yaw, Roll) 순서로 매개변수를 받음
    FMatrix rotation;
    rotation.DirectXMatrix = DirectX::XMMatrixRotationRollPitchYaw(radPitch, radYaw, radRoll);
    return rotation;
}

// 스케일 행렬 생성 (DirectXMath 사용)
FMatrix FMatrix::CreateScale(float scaleX, float scaleY, float scaleZ)
{
    FMatrix scale;
    scale.DirectXMatrix = DirectX::XMMatrixScaling(scaleX, scaleY, scaleZ);
    return scale;
}

// 평행 이동 행렬 생성 (DirectXMath 사용)
FMatrix FMatrix::CreateTranslationMatrix(const FVector& position)
{
    FMatrix translation;
    DirectX::XMVECTOR pos = DirectX::XMVectorSet(position.x, position.y, position.z, 1.0f);
    translation.DirectXMatrix = DirectX::XMMatrixTranslationFromVector(pos);
    return translation;
}

// FVector 변환 (방향 벡터, W=0 인 경우, DirectXMath의 XMVector3TransformNormal 사용)
FVector FMatrix::TransformVector(const FVector& v, const FMatrix& m)
{
    DirectX::XMVECTOR vec = DirectX::XMVectorSet(v.x, v.y, v.z, 0.0f);
    DirectX::XMVECTOR result = DirectX::XMVector3TransformNormal(vec, m.DirectXMatrix);
    FVector out;
    out.x = DirectX::XMVectorGetX(result);
    out.y = DirectX::XMVectorGetY(result);
    out.z = DirectX::XMVectorGetZ(result);
    return out;
}

// FVector4 변환 (DirectXMath 사용)
FVector4 FMatrix::TransformVector(const FVector4& v, const FMatrix& m)
{
    DirectX::XMVECTOR vec = DirectX::XMVectorSet(v.x, v.y, v.z, v.a);
    DirectX::XMVECTOR result = DirectX::XMVector4Transform(vec, m.DirectXMatrix);
    FVector4 out;
    out.x = DirectX::XMVectorGetX(result);
    out.y = DirectX::XMVectorGetY(result);
    out.z = DirectX::XMVectorGetZ(result);
    out.a = DirectX::XMVectorGetW(result);
    return out;
}

FMatrix FBoundingBox::CreateBoundingBoxTransform()
{
    // 중앙 위치 계산
    FVector Center = (min + max) * 0.5f;
    // 스케일 계산 (박스의 크기)
    FVector Scale = max - min;

    FMatrix Transform = JungleMath::CreateModelMatrix(Center, FQuat(), Scale);
    return Transform;
}
