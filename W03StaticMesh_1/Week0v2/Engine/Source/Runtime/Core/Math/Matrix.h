#pragma once

#include <DirectXMath.h>

// 4x4 행렬 연산
struct FMatrix
{
    //float M[4][4];
    union
    {
        struct
        {
            float M[4][4];
        };
    
        DirectX::XMMATRIX DirectXMatrix; 
    };

        /*FVector4 ArrVector[4];

        struct
        {
            float _00;
            float _01;
            float _02;
            float _03;

            float _10;
            float _11;
            float _12;
            float _13;

            float _20;
            float _21;
            float _22;
            float _23;

            float _30;
            float _31;
            float _32;
            float _33;
        };

    //    float Arr1D[4 * 4];
    //    */
    //};

    //FMatrix& operator=(DirectX::XMMATRIX _Matrix)
    //{
    //    DirectXMatrix = _Matrix;
    //    return *this;
    //}

    FMatrix()
    {
        DirectXMatrix = DirectX::XMMatrixIdentity();
    }

    FMatrix(const FMatrix& _Matrix)
        : DirectXMatrix(_Matrix.DirectXMatrix)
    {
    }

    FMatrix(const DirectX::FXMMATRIX& _Matrix)
        : DirectXMatrix(_Matrix)
    {
    }

        // 기본 생성자 (예: 단위 행렬)
    //FMatrix()
    //{
    //    // 단위 행렬 등으로 초기화
    //    for (int i = 0; i < 4; ++i)
    //        for (int j = 0; j < 4; ++j)
    //            M[i][j] = (i == j) ? 1.0f : 0.0f;
    //}

    // initializer_list를 받는 생성자 (C++11 이상)
    FMatrix(std::initializer_list<std::initializer_list<float>> list)
    {
        if (list.size() != 4) throw std::invalid_argument("Initializer list must have 4 rows.");
        int col = 0;
        for (const auto& col_list : list)
        {
            if (col_list.size() != 4) throw std::invalid_argument("Initializer list rows must have 4 columns.");
            int row = 0;
            for (float val : col_list)
            {
                M[col][row++] = val;
            }
            col++;
        }
    }

	static const FMatrix Identity;
	// 기본 연산자 오버로딩
	FMatrix operator+(const FMatrix& Other) const;
	FMatrix operator-(const FMatrix& Other) const;
	FMatrix operator*(const FMatrix& Other) const;
	FMatrix operator*(float Scalar) const;
	FMatrix operator/(float Scalar) const;
	float* operator[](int row);
	const float* operator[](int row) const;
	
	// 유틸리티 함수
	static FMatrix Transpose(const FMatrix& Mat);
	static float Determinant(const FMatrix& Mat);
	static FMatrix Inverse(const FMatrix& Mat);
	static FMatrix CreateRotation(float roll, float pitch, float yaw);
	static FMatrix CreateScale(float scaleX, float scaleY, float scaleZ);
	static FVector TransformVector(const FVector& v, const FMatrix& m);
	static FVector4 TransformVector(const FVector4& v, const FMatrix& m);
	static FMatrix CreateTranslationMatrix(const FVector& position);


	DirectX::XMMATRIX ToXMMATRIX() const
	{
		return DirectX::XMMatrixSet(
			M[0][0], M[1][0], M[2][0], M[3][0], // 첫 번째 열
			M[0][1], M[1][1], M[2][1], M[3][1], // 두 번째 열
			M[0][2], M[1][2], M[2][2], M[3][2], // 세 번째 열
			M[0][3], M[1][3], M[2][3], M[3][3]  // 네 번째 열
		);
	}
	FVector4 TransformFVector4(const FVector4& vector)
	{
		return FVector4(
			M[0][0] * vector.x + M[1][0] * vector.y + M[2][0] * vector.z + M[3][0] * vector.a,
			M[0][1] * vector.x + M[1][1] * vector.y + M[2][1] * vector.z + M[3][1] * vector.a,
			M[0][2] * vector.x + M[1][2] * vector.y + M[2][2] * vector.z + M[3][2] * vector.a,
			M[0][3] * vector.x + M[1][3] * vector.y + M[2][3] * vector.z + M[3][3] * vector.a
		);
	}
	FVector TransformPosition(const FVector& vector) const
	{
		float x = M[0][0] * vector.x + M[1][0] * vector.y + M[2][0] * vector.z + M[3][0];
		float y = M[0][1] * vector.x + M[1][1] * vector.y + M[2][1] * vector.z + M[3][1];
		float z = M[0][2] * vector.x + M[1][2] * vector.y + M[2][2] * vector.z + M[3][2];
		float w = M[0][3] * vector.x + M[1][3] * vector.y + M[2][3] * vector.z + M[3][3];
		return w != 0.0f ? FVector{ x / w, y / w, z / w } : FVector{ x, y, z };
	}
};