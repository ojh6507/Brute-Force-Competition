#pragma once

#include <DirectXMath.h>
#include <stdexcept>

struct FVector2D
{
	float x,y;
	FVector2D(float _x = 0, float _y = 0) : x(_x), y(_y) {}

	FVector2D operator+(const FVector2D& rhs) const
	{
		return FVector2D(x + rhs.x, y + rhs.y);
	}
	FVector2D operator-(const FVector2D& rhs) const
	{
		return FVector2D(x - rhs.x, y - rhs.y);
	}
	FVector2D operator*(float rhs) const
	{
		return FVector2D(x * rhs, y * rhs);
	}
	FVector2D operator/(float rhs) const
	{
		return FVector2D(x / rhs, y / rhs);
	}
	FVector2D& operator+=(const FVector2D& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
};

// 3D 벡터
struct FVector
{
    float x, y, z;
    FVector(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}

    FVector operator-(const FVector& other) const {
        return FVector(x - other.x, y - other.y, z - other.z);
    }
    FVector operator+(const FVector& other) const {
        return FVector(x + other.x, y + other.y, z + other.z);
    }

    // 벡터 내적
    float Dot(const FVector& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    // 벡터 크기
    float Magnitude() const {
        return sqrt(x * x + y * y + z * z);
    }

    // 벡터 정규화
    FVector Normalize() const {
        float mag = Magnitude();
        return (mag > 0) ? FVector(x / mag, y / mag, z / mag) : FVector(0, 0, 0);
    }
    FVector Cross(const FVector& Other) const
    {
        return FVector{
            y * Other.z - z * Other.y,
            z * Other.x - x * Other.z,
            x * Other.y - y * Other.x
        };
    }
    // 스칼라 곱셈
    FVector operator*(float scalar) const {
        return FVector(x * scalar, y * scalar, z * scalar);
    }

    bool operator==(const FVector& other) const {
        return (x == other.x && y == other.y && z == other.z);
    }
    float& operator[](int index) {
        switch (index) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        default:
            throw std::out_of_range("FVector index out of range");
        }
    }

    static float DistanceSquared(const FVector& v1, const FVector& v2) {
        // 각 축(x, y, z)별로 차이를 구합니다.
        float dx = v2.x - v1.x;
        float dy = v2.y - v1.y;
        float dz = v2.z - v1.z;

        // 각 차이를 제곱하여 합산합니다. 이것이 거리의 제곱입니다.
        // (dx * dx) + (dy * dy) + (dz * dz)
        return (dx * dx) + (dy * dy) + (dz * dz);
    }

    // const 객체를 위한 인덱스 연산자 (상수 참조 반환)
    const float& operator[](int index) const {
        switch (index) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        default:
            throw std::out_of_range("FVector index out of range");
        }
    }
    float Distance(const FVector& other) const {
        // 두 벡터의 차 벡터의 크기를 계산
        return ((*this - other).Magnitude());
    }
    float DistanceSq(const FVector& other) const {
        // 두 벡터의 차 벡터의 크기를 계산
        FVector sub = (*this - other);
        return (sub.x * sub.x + sub.y * sub.y + sub.z * sub.z);
    }
    DirectX::XMFLOAT3 ToXMFLOAT3() const
    {
        return DirectX::XMFLOAT3(x, y, z);
    }
    static inline FVector GetAbs(const FVector& v)
    {
        using namespace DirectX;

        XMVECTOR vec = XMVectorSet(v.x, v.y, v.z, 0.0f);
        XMVECTOR absVec = XMVectorAbs(vec);
        return FVector(XMVectorGetX(absVec), XMVectorGetY(absVec), XMVectorGetZ(absVec));
    }
    static const FVector ZeroVector;
    static const FVector OneVector;
    static const FVector UpVector;
    static const FVector ForwardVector;
    static const FVector RightVector;
};
