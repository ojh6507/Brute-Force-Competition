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
#include "Engine/Classes/Engine/Texture.h"

#include <DirectXPackedVector.h> 

using namespace DirectX::PackedVector;


#define MsgBoxAssert(Text) do { \
	std::string Value = Text; \
	MessageBoxA(nullptr, Value.c_str(), "Error", MB_OK); assert(false); \
} while (0)

#define UE_LOG Console::GetInstance().AddLog

#define _TCHAR_DEFINED
#include <d3d11.h>

#include "UserInterface/Console.h"
#include <stack>

struct FVertexSimple
{
    XMHALF4 position;     // Position
    XMHALF2  uv;
};

struct FVertexSimpleFloat
{
    float x = 0.0f,y = 0.0f,z = 0.0f;     // Position
    float  u = 0.0f, v = 0.0f;
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

    std::shared_ptr<FTexture> DiffuseTexture;

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

        TArray<TArray<FVertexSimple>> VerticesLOD; // 배열 0번이 LOD 1레벨
        TArray<UINT> Indices;
        TArray<TArray<UINT>> IndicesLOD; // 배열 0번이 LOD 1레벨

        int TotalVertices;
        int TotalIndices;

        ID3D11Buffer* VertexBuffer;
        TArray<ID3D11Buffer*> VertexBufferLOD;

        ID3D11Buffer* IndexBuffer;
        TArray<ID3D11Buffer*> IndexBufferLOD;
        

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
    FBoundingBox(FVector _min, FVector _max) : min(_min), max(_max) {
    }
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

    FVector GetBoundingBoxCenter() {
        return (max - min) * 0.5f;
    }

    float GetDistanceWithRaySIMD(const FVector& rayOrigin, const FVector& rayDir) {
        const float epsilon = 1e-6f;

        // AABB min/max 좌표를 배열로 저장
        float boxMin[3] = {min.x, min.y, min.z};
        float boxMax[3] = {max.x, max.y, max.z};

        // 레이 원점과 방향을 배열로 저장
        float rayOrig[3] = {rayOrigin.x, rayOrigin.y, rayOrigin.z};
        float rayDirInv[3] = {1.0f / (rayDir.x + epsilon), 1.0f / (rayDir.y + epsilon), 1.0f / (rayDir.z + epsilon)};

        // SIMD 레지스터에 데이터 로드
        __m128 boxMinVec = _mm_loadu_ps(boxMin);
        __m128 boxMaxVec = _mm_loadu_ps(boxMax);
        __m128 rayOrigVec = _mm_loadu_ps(rayOrig);
        __m128 rayDirInvVec = _mm_loadu_ps(rayDirInv);

        // t1 = (min - origin) * invDir
        __m128 t1Vec = _mm_mul_ps(_mm_sub_ps(boxMinVec, rayOrigVec), rayDirInvVec);

        // t2 = (max - origin) * invDir
        __m128 t2Vec = _mm_mul_ps(_mm_sub_ps(boxMaxVec, rayOrigVec), rayDirInvVec);

        // tmin = max(t1, t2)
        __m128 tminVec = _mm_min_ps(t1Vec, t2Vec);

        // 최종 tmin 계산
        float tmin[4];
        _mm_storeu_ps(tmin, tminVec);

        return std::max(std::max(tmin[0], tmin[1]), std::max(tmin[2], 0.0f));
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

    bool IntersectsSphere(const FVector& SphereCenter, float SphereRadius) const
    {
        // AABB와 구의 교차 검사 (최소 거리 알고리즘)
        float dmin = 0.0f;

        // X축
        if (SphereCenter.x < min.x)
            dmin += (SphereCenter.x - min.x) * (SphereCenter.x - min.x);
        else if (SphereCenter.x > max.x)
            dmin += (SphereCenter.x - max.x) * (SphereCenter.x - max.x);

        // y축
        if (SphereCenter.y < min.y)
            dmin += (SphereCenter.y - min.y) * (SphereCenter.y - min.y);
        else if (SphereCenter.y > max.y)
            dmin += (SphereCenter.y - max.y) * (SphereCenter.y - max.y);

        // z축
        if (SphereCenter.z < min.z)
            dmin += (SphereCenter.z - min.z) * (SphereCenter.z - min.z);
        else if (SphereCenter.z > max.z)
            dmin += (SphereCenter.z - max.z) * (SphereCenter.z - max.z);

        return dmin <= SphereRadius * SphereRadius;
    }

    void Expand(const FBoundingBox& Other)
    {
        min.x = (min.x < Other.min.x) ? min.x : Other.min.x;
        min.y = (min.y < Other.min.y) ? min.y : Other.min.y;
        min.z = (min.z < Other.min.z) ? min.z : Other.min.z;

        max.x = (max.x > Other.max.x) ? max.x : Other.max.x;
        max.y = (max.y > Other.max.y) ? max.y : Other.max.y;
        max.z = (max.z > Other.max.z) ? max.z : Other.max.z;
    }
    void Expand(const FVector& Point)
    {
        if (Point.x < min.x) min.x = Point.x;
        if (Point.y < min.y) min.y = Point.y;
        if (Point.z < min.z) min.z = Point.z;

        if (Point.x > max.x) max.x = Point.x;
        if (Point.y > max.y) max.y = Point.y;
        if (Point.z > max.z) max.z = Point.z;
    }

    // branchless 절대값 계산: v의 비트 표현에서 부호 비트를 제거
    FORCEINLINE float FastAbs(float v)
    {
        unsigned int iv;
        memcpy(&iv, &v, sizeof(v));
        iv &= 0x7FFFFFFF;
        float absV;
        memcpy(&absV, &iv, sizeof(absV));
        return absV;
    }

    // branchless 안전한 역수 계산: v의 절대값이 epsilon 미만이면 FLT_MAX를 반환
    FORCEINLINE float SafeInv(float v, float epsilon)
    {
        // v의 절대값과 epsilon의 비트 표현을 비교하여 mask 생성 (조건부 선택)
        // 조건: FastAbs(v) < epsilon 인 경우, mask = 0, 아니면 mask = ~0
        unsigned int iv;
        memcpy(&iv, &v, sizeof(v));
        iv &= 0x7FFFFFFF;
        unsigned int eps;
        memcpy(&eps, &epsilon, sizeof(epsilon));
        // mask = 0xFFFFFFFF if iv >= eps, else 0
        unsigned int mask = -(int)(iv >= eps);
        // 두 값 중 선택: mask가 모두 1이면 1/v, 아니면 FLT_MAX
        float inv;
        float invVal = 1.0f / v;
        // bit-level 선택: mask와 ~mask를 이용하여 선택 (비트별 AND, OR)
        unsigned int* pInv = reinterpret_cast<unsigned int*>(&inv);
        unsigned int invValBits;
        memcpy(&invValBits, &invVal, sizeof(invValBits));
        unsigned int fltMaxBits;
        float maxValue = FLT_MAX;

        memcpy(&fltMaxBits, &maxValue, sizeof(fltMaxBits));
        // 선택: (mask & invValBits) | ((~mask) & fltMaxBits)
        *pInv = (mask & invValBits) | ((~mask) & fltMaxBits);
        return inv;
    }

    bool Intersect(const FVector& rayOrigin, const FVector& rayDir, float& outDistance)
    {
        const float epsilon = 1e-6f;
        // 각 축별 안전한 역수 계산 (분기 없이)
        float invDirX = SafeInv(rayDir.x, epsilon);
        float invDirY = SafeInv(rayDir.y, epsilon);
        float invDirZ = SafeInv(rayDir.z, epsilon);

        // X축 슬랩
        float t1 = (min.x - rayOrigin.x) * invDirX;
        float t2 = (max.x - rayOrigin.x) * invDirX;
        float tmin = fminf(t1, t2);
        float tmax = fmaxf(t1, t2);

        // Y축 슬랩
        t1 = (min.y - rayOrigin.y) * invDirY;
        t2 = (max.y - rayOrigin.y) * invDirY;
        float tymin = fminf(t1, t2);
        float tymax = fmaxf(t1, t2);

        // tmin, tmax 업데이트 (branchless: fmaxf/fminf는 대부분 인라인 처리됨)
        tmin = fmaxf(tmin, tymin);
        tmax = fminf(tmax, tymax);

        // Z축 슬랩
        t1 = (min.z - rayOrigin.z) * invDirZ;
        t2 = (max.z - rayOrigin.z) * invDirZ;
        float tzmin = fminf(t1, t2);
        float tzmax = fmaxf(t1, t2);

        tmin = fmaxf(tmin, tzmin);
        tmax = fminf(tmax, tzmax);

        // 최종 교차 구간 조건 검사: (tmax < 0) 또는 (tmin > tmax)
        // 이 두 조건은 부울 값로 계산하고, 최종 결과에 곱셈으로 반영 (0이면 false, 1이면 true)
        // 아래는 if문 대신 bool값을 곱셈으로 사용한 예시
        const int cond1 = (tmax < 0.0f);
        const int cond2 = (tmin > tmax);
        // cond1이나 cond2가 참이면 0, 아니면 1
        int valid = !(cond1 | cond2);

        outDistance = (tmin >= 0.0f) * tmin;

        // valid이 1이면 true, 0이면 false
        return valid != 0;
    }
    
    float GetDistanceToPoint(const FVector& point) const
    {
        // 각 축에 대해 점이 바운딩박스 범위 내에 있다면 0, 범위 밖이면 차이 값을 반환
        float dx = (point.x < min.x) ? (min.x - point.x) : ((point.x > max.x) ? (point.x - max.x) : 0.0f);
        float dy = (point.y < min.y) ? (min.y - point.y) : ((point.y > max.y) ? (point.y - max.y) : 0.0f);
        float dz = (point.z < min.z) ? (min.z - point.z) : ((point.z > max.z) ? (point.z - max.z) : 0.0f);
        return sqrtf(dx * dx + dy * dy + dz * dz);
    }

    bool IntersectSphere(
        const FVector& rayOrigin,
        const FVector& rayDir,
        const FVector& sphereCenter,
        float sphereRadius,
        float& outDistance)
    {
        // 구 중심에서 레이 시작점까지의 벡터
        FVector L = sphereCenter - rayOrigin;
        float tca = L.Dot(rayDir);

        // 레이 방향이 구와 반대라면, 추가 연산 없이 바로 false 반환
        if (tca < 0.0f)
        {
            return false;
        }

        // 미리 계산된 값 사용
        float L2 = L.Dot(L);
        float tca2 = tca * tca;
        float radius2 = sphereRadius;

        // 최소 거리의 제곱을 계산해 비교 (sqrt 전 단계)
        float d2 = L2 - tca2;
        if (d2 > radius2)
        {
            return false;
        }

        // 필요한 경우에만 sqrt 호출 (정확도가 덜 요구되면 근사값으로 대체 가능)
        float thc = std::sqrt(radius2 - d2);
        float t0 = tca - thc;
        float t1 = tca + thc;

        // t0가 음수이면 t1 사용
        if (t0 < 0.0f)
        {
            t0 = t1;
        }
        if (t0 < 0.0f)
        {
            return false;
        }

        outDistance = t0;
        return true;
    }

    FBoundingBox TransformWorld(const FMatrix& worldMatrix) const
    {
        // 로컬 공간에서 중심과 half-extents 계산
        FVector center = (min + max) * 0.5f;
        FVector extents = (max - min) * 0.5f;

        // 중심을 월드 공간으로 변환
        FVector worldCenter = worldMatrix.TransformPosition(center);

        // 행렬의 상위 3x3 부분의 절대값 벡터를 사용하여 각 축에 대한 확장량 계산
        // (FVector::GetAbs()와 DotProduct() 함수가 사용됨)
        FVector row0(worldMatrix.M[0][0], worldMatrix.M[0][1], worldMatrix.M[0][2]);
        FVector row1(worldMatrix.M[1][0], worldMatrix.M[1][1], worldMatrix.M[1][2]);
        FVector row2(worldMatrix.M[2][0], worldMatrix.M[2][1], worldMatrix.M[2][2]);

        FVector::GetAbs(row0);
        FVector::GetAbs(row1);
        FVector::GetAbs(row2);
        
        float worldExtentX = row0.Dot(extents);
        float worldExtentY = row1.Dot(extents);
        float worldExtentZ = row2.Dot(extents);

        FVector worldExtents(worldExtentX, worldExtentY, worldExtentZ);

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
struct FCameraConstants {
    FMatrix VP;
};
struct FLitUnlitConstants {
    int isLit; // 1 = Lit, 0 = Unlit 
    FVector pad;
};

struct FSubMeshConstants {
    float isSelectedSubMesh;
    FVector pad;
};

struct FTextureConstants {
    float UOffset;
    float VOffset;
    float pad0;
    float pad1;
};

struct Triangle {
    FVector v0, v1, v2;
    FBoundingBox bbox;

    Triangle(const FVector& a, const FVector& b, const FVector& c)
        : v0(a), v1(b), v2(c)
    {
        // 각 좌표별 최소, 최대값 계산
        bbox.min.x = std::min({ a.x, b.x, c.x });
        bbox.min.y = std::min({ a.y, b.y, c.y });
        bbox.min.z = std::min({ a.z, b.z, c.z });
        bbox.max.x = std::max({ a.x, b.x, c.x });
        bbox.max.y = std::max({ a.y, b.y, c.y });
        bbox.max.z = std::max({ a.z, b.z, c.z });
    }
};
