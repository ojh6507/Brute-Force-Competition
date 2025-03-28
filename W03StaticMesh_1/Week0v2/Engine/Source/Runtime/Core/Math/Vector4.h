#pragma once

#define _TCHAR_DEFINED 
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#include "MathUtility.h"


// 4D Vector
struct FVector4 {

    static const FVector4 ONE;
    static const FVector4 ONENULL;

    static const FVector4 ZERO;
    static const FVector4 ZERONULL;
    static const FVector4 LEFT;
    static const FVector4 RIGHT;
    static const FVector4 UP;
    static const FVector4 DOWN;
    static const FVector4 FORWARD;
    static const FVector4 BACKWARD;

    static const FVector4 WHITE;
    static const FVector4 RED;
    static const FVector4 GREEN;
    static const FVector4 BLUE;
    static const FVector4 BLACK;
    union
    {
        float Arr1D[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

        struct
        {
            float X;
            float Y;
            float Z;
            float W;
        };

        struct
        {
            float x;
            float y;
            float z;
            float a;
        };


        struct
        {
            float R;
            float G;
            float B;
            float A;
        };


        struct
        {
            float Pos2DX;
            float Pos2DY;
            float Scale2DX;
            float Scale2DY;
        };

        float Arr2D[1][4];
        DirectX::XMVECTOR DirectXVector;
        DirectX::XMFLOAT3 Float3;
        DirectX::XMFLOAT4 Float4;
    };


    FVector4(DirectX::FXMVECTOR& _DirectXVector)
        : DirectXVector(_DirectXVector)
    {

    }

    FVector4(DirectX::XMFLOAT3 _Float3)
        : Float3(_Float3)
    {

    }

    FVector4(float _X = 0.0f, float _Y = 0.0f, float _Z = 0.0f, float _W = 1.0f)
        : X(_X), Y(_Y), Z(_Z), W(_W)
    {

    }

    UINT ColorToUint() const
    {
        UINT Return;

        char* Ptr = reinterpret_cast<char*>(&Return);

        // 0~1
        Ptr[0] = static_cast<int>(R * 255.0f);
        Ptr[1] = static_cast<int>(G * 255.0f);
        Ptr[2] = static_cast<int>(B * 255.0f);
        Ptr[3] = static_cast<int>(A * 255.0f);

        return Return;
    }

    inline int iX() const
    {
        return static_cast<int>(X);
    }

    inline int iY() const
    {
        return static_cast<int>(Y);
    }

    inline UINT uiX() const
    {
        return static_cast<unsigned int>(X);
    }

    inline UINT uiY() const
    {
        return static_cast<unsigned int>(Y);
    }

    inline float hX() const
    {
        return X * 0.5f;
    }

    inline float hY() const
    {
        return Y * 0.5f;
    }

    inline float hZ() const
    {
        return Z * 0.5f;
    }

    inline int ihX() const
    {
        return static_cast<int>(hX());
    }

    inline int ihY() const
    {
        return static_cast<int>(hY());
    }

    inline FVector4 Half() const
    {
        return { hX(), hY(), hZ(), W };
    }

    FVector4 ToABS() const
    {
        return DirectX::XMVectorAbs(DirectXVector);
    }

    FVector4 operator-() const
    {
        FVector4 ReturnValue = DirectX::XMVectorSet(-X, -Y, -Z, -W);
        return ReturnValue;
    }

    FVector4 operator-(const FVector4& _Other) const
    {
        FVector4 ReturnValue;
        ReturnValue.DirectXVector = DirectX::XMVectorSubtract(DirectXVector, _Other.DirectXVector);
        return ReturnValue;
    }

    //FVector4 operator+(const FVector4& _Other) const
    //{
    //    FVector4 ReturnValue;
    //    ReturnValue.DirectXVector = DirectX::XMVectorAdd(DirectXVector, _Other.DirectXVector);
    //    return ReturnValue;
    //}

    FVector4& operator+=(const FVector4& _Other)
    {
        DirectXVector = DirectX::XMVectorAdd(DirectXVector, _Other.DirectXVector);
        return *this;
    }


    FVector4& operator-=(const FVector4& _Other)
    {

        DirectXVector = DirectX::XMVectorSubtract(DirectXVector, _Other.DirectXVector);

        return *this;
    }


    FVector4& operator*=(const FVector4& _Other)
    {
        DirectXVector = DirectX::XMVectorMultiply(DirectXVector, _Other.DirectXVector);
        return *this;
    }

    FVector4& operator*=(const float _Value)
    {
        // DirectXVector의 모든 컴포넌트에 _Value를 직접 곱함
        DirectXVector = DirectX::XMVectorScale(DirectXVector, _Value);

        //W = PrevW;
        return *this;
    }

    FVector4 operator/(const FVector4& _Other) const
    {

        FVector4 ReturnValue = DirectX::XMVectorDivide(DirectXVector, _Other.DirectXVector);
        return ReturnValue;
    }

    FVector4& operator/=(const FVector4 _Value)
    {
        DirectXVector = DirectX::XMVectorDivide(DirectXVector, _Value.DirectXVector);
        return *this;
    }


    FVector4& operator/=(const float _Value)
    {
        // 0으로 나누는 경우를 방지 (선택 사항이지만 권장됨)
        // 매우 작은 값으로 나누는 것도 문제가 될 수 있으므로 epsilon 비교가 더 좋을 수 있음
        if (fabsf(_Value) < std::numeric_limits<float>::epsilon()) // 예: 0에 가까운 값 체크
        {
            // 오류 처리 또는 특정 값으로 설정 (예: 0 또는 최대값)
            // 여기서는 예시로 모든 컴포넌트를 0으로 설정
            DirectXVector = DirectX::XMVectorZero();
            // 또는 예외를 던지거나, 로그를 남길 수 있음
            return *this;
        }

        // _Value의 역수를 계산
        float ReciprocalValue = 1.0f / _Value;

        // 역수를 벡터의 모든 컴포넌트에 곱함 (XMVectorScale 사용)
        DirectXVector = DirectX::XMVectorScale(DirectXVector, ReciprocalValue);

        return *this;
    }

    bool operator==(const FVector4 _Value) const
    {
        return DirectX::XMVector4Equal(DirectXVector, _Value.DirectXVector);
        //return X == _Value.X &&
        //	Y == _Value.Y &&
        //	Z == _Value.Z;
    }

    FVector4 operator!()
    {
        return DirectX::XMVectorNegate(DirectXVector);
    }

    //FVector4 QuaternionMulQuaternion(const FVector4& _Quaternion);

    //class float4x4 QuaternionToMatrix();

   // FQuat EulerDegToQuaternion();




};