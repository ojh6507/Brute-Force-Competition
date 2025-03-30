#pragma once
#include <cmath>
#include <algorithm>
#include "Core/Container/String.h"
#include "Core/Container/Array.h"
#include "UObject/NameTypes.h"

// 수학 관련
#include "Math/Vector.h"
#include "Math/Vector4.h"
#include "Math/Matrix.h"


#define MsgBoxAssert(Text) do { \
	std::string Value = Text; \
	MessageBoxA(nullptr, Value.c_str(), "Error", MB_OK); assert(false); \
} while (0)

#define UE_LOG Console::GetInstance().AddLog

#define _TCHAR_DEFINED
#include <d3d11.h>

#include "UserInterface/Console.h"

struct FVertexSimple
{
    float x, y, z;    // Position
    float u = 0, v = 0;
};
struct FVertexSimpleIndex
{
    float x, y, z;    // Position
    float u = 0, v = 0;
    int MatrixIndex;
};

// Material Subset
struct FMaterialSubset
{
    uint32 IndexStart; // Index Buffer Start pos
    uint32 IndexCount; // Index Count
    uint32 MaterialIndex; // Material Index
    FString MaterialName; // Material Name
};

struct FStaticMaterial
{
    class UMaterial* Material;
    FName MaterialSlotName;
    //FMeshUVChannelInfo UVChannelData;
};

// OBJ File Raw Data
struct FObjInfo
{
    FWString ObjectName; // OBJ File Name
    FWString PathName; // OBJ File Paths
    FString DisplayName; // Display Name
    FString MatName; // OBJ MTL File Name

    // Group
    uint32 NumOfGroup = 0; // token 'g' or 'o'
    TArray<FString> GroupName;

    // Vertex, UV, Normal List
    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;

    // Faces
    TArray<int32> Faces;

    // Index
    TArray<uint32> VertexIndices;
    TArray<uint32> NormalIndices;
    TArray<uint32> TextureIndices;

    // Material
    TArray<FMaterialSubset> MaterialSubsets;
};

struct FObjMaterialInfo
{
    FString MTLName;  // newmtl : Material Name.

    bool bHasTexture = false;  // Has Texture?
    bool bTransparent = false; // Has alpha channel?

    FVector Diffuse;  // Kd : Diffuse (Vector4)
    FVector Specular;  // Ks : Specular (Vector) 
    FVector Ambient;   // Ka : Ambient (Vector)
    FVector Emissive;  // Ke : Emissive (Vector)

    float SpecularScalar; // Ns : Specular Power (Float)
    float DensityScalar;  // Ni : Optical Density (Float)
    float TransparencyScalar; // d or Tr  : Transparency of surface (Float)

    uint32 IlluminanceModel; // illum: illumination Model between 0 and 10. (UINT)

    /* Texture */
    FString DiffuseTextureName;  // map_Kd : Diffuse texture
    FWString DiffuseTexturePath;

    FString AmbientTextureName;  // map_Ka : Ambient texture
    FWString AmbientTexturePath;

    FString SpecularTextureName; // map_Ks : Specular texture
    FWString SpecularTexturePath;

    FString BumpTextureName;     // map_Bump : Bump texture
    FWString BumpTexturePath;

    FString AlphaTextureName;    // map_d : Alpha texture
    FWString AlphaTexturePath;
};

// Cooked Data
namespace OBJ
{
    struct FBatchInfo
    {
        UINT startIndex;
        UINT indexCount;
        int  materialIndex;
    };

    struct FStaticMeshRenderData
    {
        FWString ObjectName;
        FWString PathName;
        FString DisplayName;

        TArray<FVertexSimple> Vertices;
        TArray<UINT> Indices;

        int TotalVertices;
        int TotalIndices;

        ID3D11Buffer* VertexBuffer;
        ID3D11Buffer* IndexBuffer;

        TArray<FObjMaterialInfo> Materials;
        TArray<FMaterialSubset> MaterialSubsets;


        TArray<FBatchInfo> Batches;

        FVector BoundingBoxMin;
        FVector BoundingBoxMax;
    };


    struct MergedMeshData
    {
        ID3D11Buffer* VertexBuffer;
        ID3D11Buffer* IndexBuffer;
        TArray<FBatchInfo> Batches;
    };

};

struct FVertexTexture
{
    float x, y, z;    // Position
    float u, v; // Texture
};
struct FGridParameters
{
    float gridSpacing;
    int   numGridLines;
    FVector gridOrigin;
    float pad;
};
struct FSimpleVertex
{
    float dummy; // 내용은 사용되지 않음
    float padding[11];
};
struct FOBB {
    FVector corners[8];
};
struct FRect
{
    FRect() : leftTopX(0), leftTopY(0), width(0), height(0) {}
    FRect(float x, float y, float w, float h) : leftTopX(x), leftTopY(y), width(w), height(h) {}
    float leftTopX, leftTopY, width, height;
};
struct FPoint
{
    FPoint() : x(0), y(0) {}
    FPoint(float _x, float _y) : x(_x), y(_y) {}
    FPoint(long _x, long _y) : x(_x), y(_y) {}
    FPoint(int _x, int _y) : x(_x), y(_y) {}

    float x, y;
};


struct Plane {
    float a, b, c, d;
};

struct FBoundingBox
{
    FBoundingBox() {}
    FBoundingBox(FVector _min, FVector _max) : min(_min), max(_max) {}
    FVector min; // Minimum extents
    float pad;
    FVector max; // Maximum extents
    float pad1;
    
    bool Intersects(const FBoundingBox& OtherBox) const
    {
        bool bOverlapX = (min.x <= OtherBox.max.x) && (max.x >= OtherBox.min.x);

        // 2. Y축에서 겹치는지 확인 (X축과 동일한 로직)
        bool bOverlapY = (min.y <= OtherBox.max.y) && (max.y >= OtherBox.min.y);

        // 3. Z축에서 겹치는지 확인 (X축과 동일한 로직)
        bool bOverlapZ = (min.z <= OtherBox.max.z) && (max.z >= OtherBox.min.z);

        // 모든 축에서 겹쳐야만 최종적으로 교차하는 것으로 판단합니다.
        return bOverlapX && bOverlapY && bOverlapZ;
    }
 
    bool IntersectsLocal(const FVector& localRayOrigin, const FVector& localRayDir,
        const FVector& boxMin, const FVector& boxMax,
        float& outDistance)
    {
        float tmin = -FLT_MAX;
        float tmax = FLT_MAX;
        const float epsilon = 1e-6f;

        // X축 교차 검사
        if (fabs(localRayDir.x) < epsilon)
        {
            if (localRayOrigin.x < boxMin.x || localRayOrigin.x > boxMax.x)
                return false;
        }
        else
        {
            float t1 = (boxMin.x - localRayOrigin.x) / localRayDir.x;
            float t2 = (boxMax.x - localRayOrigin.x) / localRayDir.x;
            if (t1 > t2) std::swap(t1, t2);
            tmin = (t1 > tmin) ? t1 : tmin;
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        // Y축 교차 검사
        if (fabs(localRayDir.y) < epsilon)
        {
            if (localRayOrigin.y < boxMin.y || localRayOrigin.y > boxMax.y)
                return false;
        }
        else
        {
            float t1 = (boxMin.y - localRayOrigin.y) / localRayDir.y;
            float t2 = (boxMax.y - localRayOrigin.y) / localRayDir.y;
            if (t1 > t2) std::swap(t1, t2);
            tmin = (t1 > tmin) ? t1 : tmin;
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        // Z축 교차 검사
        if (fabs(localRayDir.z) < epsilon)
        {
            if (localRayOrigin.z < boxMin.z || localRayOrigin.z > boxMax.z)
                return false;
        }
        else
        {
            float t1 = (boxMin.z - localRayOrigin.z) / localRayDir.z;
            float t2 = (boxMax.z - localRayOrigin.z) / localRayDir.z;
            if (t1 > t2) std::swap(t1, t2);
            tmin = (t1 > tmin) ? t1 : tmin;
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        if (tmax < 0.0f)
            return false;

        outDistance = (tmin >= 0.0f) ? tmin : 0.0f;
        return true;
    }
    bool Intersect(const FVector& rayOrigin, const FVector& rayDir, float& outDistance)
    {
        float tmin = -FLT_MAX;
        float tmax = FLT_MAX;
        const float epsilon = 1e-6f;

        // X축 처리
        if (fabs(rayDir.x) < epsilon)
        {
            // 레이가 X축 방향으로 거의 평행한 경우,
            // 원점의 x가 박스 [min.x, max.x] 범위 밖이면 교차 없음
            if (rayOrigin.x < min.x || rayOrigin.x > max.x)
                return false;
        }
        else
        {
            float t1 = (min.x - rayOrigin.x) / rayDir.x;
            float t2 = (max.x - rayOrigin.x) / rayDir.x;
            if (t1 > t2)  std::swap(t1, t2);

            // tmin은 "현재까지의 교차 구간 중 가장 큰 min"
            tmin = (t1 > tmin) ? t1 : tmin;
            // tmax는 "현재까지의 교차 구간 중 가장 작은 max"
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        // Y축 처리
        if (fabs(rayDir.y) < epsilon)
        {
            if (rayOrigin.y < min.y || rayOrigin.y > max.y)
                return false;
        }
        else
        {
            float t1 = (min.y - rayOrigin.y) / rayDir.y;
            float t2 = (max.y - rayOrigin.y) / rayDir.y;
            if (t1 > t2)  std::swap(t1, t2);

            tmin = (t1 > tmin) ? t1 : tmin;
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        // Z축 처리
        if (fabs(rayDir.z) < epsilon)
        {
            if (rayOrigin.z < min.z || rayOrigin.z > max.z)
                return false;
        }
        else
        {
            float t1 = (min.z - rayOrigin.z) / rayDir.z;
            float t2 = (max.z - rayOrigin.z) / rayDir.z;
            if (t1 > t2)  std::swap(t1, t2);

            tmin = (t1 > tmin) ? t1 : tmin;
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        // 여기까지 왔으면 교차 구간 [tmin, tmax]가 유효하다.
        // tmax < 0 이면, 레이가 박스 뒤쪽에서 교차하므로 화면상 보기엔 교차 안 한다고 볼 수 있음
        if (tmax < 0.0f)
            return false;

        // outDistance = tmin이 0보다 크면 그게 레이가 처음으로 박스를 만나는 지점
        // 만약 tmin < 0 이면, 레이의 시작점이 박스 내부에 있다는 의미이므로, 거리를 0으로 처리해도 됨.
        outDistance = (tmin >= 0.0f) ? tmin : 0.0f;

        return true;
    }
    FBoundingBox TransformWorld(const FMatrix& worldMatrix) const
    {
        // 모델 공간 중심과 half-extents 계산
        FVector center = (min + max) * 0.5f;
        FVector extents = (max - min) * 0.5f;

        // 점 변환 함수 TransformPoint는 worldMatrix를 사용해 center를 월드 좌표로 변환합니다.
        FVector worldCenter = worldMatrix.TransformPosition(center);

        FVector worldExtents;
        worldExtents.x = fabs(worldMatrix.M[0][0]) * extents.x + fabs(worldMatrix.M[0][1]) * extents.y + fabs(worldMatrix.M[0][2]) * extents.z;
        worldExtents.y = fabs(worldMatrix.M[1][0]) * extents.x + fabs(worldMatrix.M[1][1]) * extents.y + fabs(worldMatrix.M[1][2]) * extents.z;
        worldExtents.z = fabs(worldMatrix.M[2][0]) * extents.x + fabs(worldMatrix.M[2][1]) * extents.y + fabs(worldMatrix.M[2][2]) * extents.z;

        // 월드 AABB 생성
        return FBoundingBox(worldCenter - worldExtents, worldCenter + worldExtents);
    }
  
    void GetBoundingSphere(FVector& outCenter, float& outRadiusSquared)
    {
        outCenter = (min + max) * 0.5f;
        // AABB의 한 꼭짓점과 중심 사이의 거리의 제곱값 계산
        FVector diff = max - outCenter;
        outRadiusSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    }

    bool IsSphereInsideFrustum(const Plane planes[6], const FVector& center, float radiusSquared)
    {
        for (int i = 0; i < 6; ++i)
        {
            // 평면 방정식: ax + by + cz + d (단, 평면의 법선은 단위벡터라고 가정)
            float ddistance = planes[i].a * center.x +
                planes[i].b * center.y +
                planes[i].c * center.z +
                planes[i].d;
            // ddistance가 음수인 경우에만 검사
            if (ddistance < 0 && (ddistance * ddistance) > radiusSquared)
                return false;
        }
        return true;
    }

    bool IsIntersectingFrustum(const Plane planes[6]) {

        FVector center;
        float radius;
        GetBoundingSphere(center, radius);
        return IsSphereInsideFrustum(planes, center, radius);
    }
    FMatrix CreateBoundingBoxTransform();

};

struct FCone
{
    FVector ConeApex; // 원뿔의 꼭짓점
    float ConeRadius; // 원뿔 밑면 반지름

    FVector ConeBaseCenter; // 원뿔 밑면 중심
    float ConeHeight; // 원뿔 높이 (Apex와 BaseCenter 간 차이)
    FVector4 Color;

    int ConeSegmentCount; // 원뿔 밑면 분할 수
    float pad[3];

};
struct FPrimitiveCounts
{
    int BoundingBoxCount;
    int pad;
    int ConeCount;
    int pad1;
};
struct FLighting
{
    float lightDirX, lightDirY, lightDirZ; // 조명 방향
    float pad1;                      // 16바이트 정렬용 패딩
    float lightColorX, lightColorY, lightColorZ;    // 조명 색상
    float pad2;                      // 16바이트 정렬용 패딩
    float AmbientFactor;             // ambient 계수
    float pad3; // 16바이트 정렬 맞춤 추가 패딩
    float pad4; // 16바이트 정렬 맞춤 추가 패딩
    float pad5; // 16바이트 정렬 맞춤 추가 패딩
};

struct FMaterialConstants {
    FVector DiffuseColor;
    float TransparencyScalar;
    FVector AmbientColor;
    float DensityScalar;
    FVector SpecularColor;
    float SpecularScalar;
    FVector EmmisiveColor;
    float MaterialPad0;
};

struct FConstants {
    FMatrix MVP;      // 모델
    int IsSelected;
};
struct FConstantsXM {
    DirectX::XMFLOAT4X4 MVP;      // 모델
    int IsSelected;      // 모델
};
struct FLitUnlitConstants {
    int isLit; // 1 = Lit, 0 = Unlit 
    FVector pad;
};

struct FSubMeshConstants {
    float isSelectedSubMesh;
    FVector pad;
};

struct FInstanceData
{
    FMatrix WorldMatrix;
};


struct FTextureConstants {
    float UOffset;
    float VOffset;
    float pad0;
    float pad1;
};

struct FOffsetConstantData
{
    UINT BaseInstanceOffset;
    // 필요시 다른 배치별 데이터 추가 (예: 패딩)
    float padding[3];
};