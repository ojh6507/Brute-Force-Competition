#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define _TCHAR_DEFINED
#include <d3d11.h>
#include "EngineBaseTypes.h"
#include "Define.h"
#include "Container/Octree.h"
#include "Container/Set.h"
#include "Container/Map.h"

struct StaticMeshInstanceData
{
    ID3D11Buffer* VertexBufferInstance;
    ID3D11Buffer* IndexBufferInstance;

    UINT NumVerticesPerInstance = 0; // 원본 메쉬의 정점 수 (Draw 호출 시 필요)
    UINT NumIndicesPerInstance = 0;  // 원본 메쉬의 인덱스 수 (Draw 호출 시 필요)
    UINT TotalInstancesInBuffer = 0; // 이 버퍼들에 포함된 총 인스턴스 수
};

struct FFrameBatchInfo
{
    class UStaticMesh* MeshType = nullptr;
    UINT InstanceCount = 0;                // 이 배치의 인스턴스 수
    UINT StructuredBufferOffset = 0;       // 공유 SB 내에서 이 배치의 데이터 시작 오프셋
};

class ULightComponentBase;
class UWorld;
class FGraphicsDevice;
class UMaterial;
struct FStaticMaterial;
class UObject;
class FEditorViewportClient;
class UBillboardComponent;
class UStaticMeshComponent;
class UGizmoBaseComponent;
class FRenderer 
{

private:
    float litFlag = 0;
public:
    FGraphicsDevice* Graphics;
    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
    ID3D11Buffer* ConstantBuffer = nullptr;
    ID3D11Buffer* ConstantBufferXM = nullptr;
    ID3D11Buffer* LightingBuffer = nullptr;
    ID3D11Buffer* FlagBuffer = nullptr;
    ID3D11Buffer* MaterialConstantBuffer = nullptr;
    ID3D11Buffer* SubMeshConstantBuffer = nullptr;
    ID3D11Buffer* TextureConstantBufer = nullptr;

    FLighting lightingData;

    uint32 Stride;
    //uint32 StrideInstance;
    uint32 Stride2;

public:
    void Initialize(FGraphicsDevice* graphics);
   
    void PrepareShader() const;
    
    //Render
    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const;
    void RenderPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices) const;
    void RenderPrimitive(OBJ::FStaticMeshRenderData* renderData, TArray<FStaticMaterial*> materials, TArray<UMaterial*> overrideMaterial, int selectedSubMeshIndex);
   
    void RenderTexturedModelPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices, ID3D11ShaderResourceView* InTextureSRV, ID3D11SamplerState* InSamplerState) const;
    //Release
    void Release();
    void ReleaseShader();
    void ReleaseBuffer(ID3D11Buffer*& Buffer) const;
    void ReleaseConstantBuffer();

    void ReleaseInstanceData();

    void ResetVertexShader() const;
    void ResetPixelShader() const;
    void CreateShader();
    bool CreateInputLayoutForVertexSimpleIndex();

    void SetVertexShader(const FWString& filename, const FString& funcname, const FString& version);
    void SetPixelShader(const FWString& filename, const FString& funcname, const FString& version);
    
    void ChangeViewMode(EViewModeIndex evi) const;
    
    // CreateBuffer
    void CreateConstantBuffer();
    void CreateLightingBuffer();
    void CreateLitUnlitBuffer();
    ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth) const;
    ID3D11Buffer* CreateVertexBufferForManualInstancing(const TArray<FVertexSimple>& vertices, UINT InMatrixIndex, StaticMeshInstanceData& Data) const;
    ID3D11Buffer* CreateVertexBuffer(const TArray<FVertexSimple>& vertices, UINT byteWidth) const;
    ID3D11Buffer* CreateIndexBuffer(uint32* indices, UINT byteWidth) const;
    ID3D11Buffer* CreateIndexBuffer(const TArray<UINT>& indices, UINT byteWidth) const;
    ID3D11Buffer* CreateIndexBufferForManualInstancing(const TArray<UINT>& originalIndices, UINT numOriginalVertices, UINT numInstances, StaticMeshInstanceData& Data) const;

    bool CreateStructuredBuffer(
        ID3D11Buffer** OutBuffer,
        ID3D11ShaderResourceView** OutSRV,
        UINT ElementCount,
        UINT ElementSize,
        const void* pInitialData = nullptr,
        bool bDynamic = true); // 기본적으로 Dynamic으로 설정 (매 프레임 업데이트 가정)

    // update
    void UpdateLightBuffer() const;
    void UpdateConstant(const FMatrix& MVP,  FVector4 UUIDColor, bool IsSelected) const;
    void UpdateConstantXM(const DirectX::XMMATRIX& MVP, FVector4 UUIDColor, bool IsSelected) const;
   void UpdateMaterial(const FObjMaterialInfo& MaterialInfo) const;
   //void UpdateLitUnlitConstant(int isLit) const;
   //void UpdateSubMeshConstant(bool isSelected) const;
   void UpdateTextureConstant(float UOffset, float VOffset);

public://텍스쳐용 기능 추가
    ID3D11VertexShader* VertexTextureShader = nullptr;
    ID3D11PixelShader* PixelTextureShader = nullptr;
    ID3D11InputLayout* TextureInputLayout = nullptr;

    uint32 TextureStride;
    struct FSubUVConstant
    {
        float indexU;
        float indexV;
    };
    ID3D11Buffer* SubUVConstantBuffer = nullptr;

public:
    void CreateTextureShader();
    void ReleaseTextureShader();
    void PrepareTextureShader() const;
    ID3D11Buffer* CreateVertexTextureBuffer(FVertexTexture* vertices, UINT byteWidth) const;
    ID3D11Buffer* CreateIndexTextureBuffer(uint32* indices, UINT byteWidth) const;
    void RenderTexturePrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices,
        ID3D11Buffer* pIndexBuffer, UINT numIndices,
        ID3D11ShaderResourceView* _TextureSRV,
        ID3D11SamplerState* _SamplerState) const;
    void RenderTextPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices,
        ID3D11ShaderResourceView* _TextureSRV,
        ID3D11SamplerState* _SamplerState) const;
    ID3D11Buffer* CreateVertexBuffer(FVertexTexture* vertices, UINT byteWidth) const;

    void UpdateSubUVConstant(float _indexU, float _indexV) const;
    void PrepareSubUVConstant() const;


public: // line shader
    void PrepareLineShader() const;
    void CreateLineShader();
    void ReleaseLineShader() const;
    void RenderBatch(const FGridParameters& gridParam, ID3D11Buffer* pVertexBuffer, int boundingBoxCount, int coneCount, int coneSegmentCount, int obbCount) const;
    void UpdateGridConstantBuffer(const FGridParameters& gridParams) const;
    void UpdateLinePrimitveCountBuffer(int numBoundingBoxes, int numCones) const;
    ID3D11Buffer* CreateStaticVerticesBuffer() const;
    ID3D11Buffer* CreateBoundingBoxBuffer(UINT numBoundingBoxes) const;
    ID3D11Buffer* CreateOBBBuffer(UINT numBoundingBoxes) const;
    ID3D11Buffer* CreateConeBuffer(UINT numCones) const;
    ID3D11ShaderResourceView* CreateBoundingBoxSRV(ID3D11Buffer* pBoundingBoxBuffer, UINT numBoundingBoxes);
    ID3D11ShaderResourceView* CreateOBBSRV(ID3D11Buffer* pBoundingBoxBuffer, UINT numBoundingBoxes);
    ID3D11ShaderResourceView* CreateConeSRV(ID3D11Buffer* pConeBuffer, UINT numCones);

    void UpdateBoundingBoxBuffer(ID3D11Buffer* pBoundingBoxBuffer, const TArray<FBoundingBox>& BoundingBoxes, int numBoundingBoxes) const;
    void UpdateOBBBuffer(ID3D11Buffer* pBoundingBoxBuffer, const TArray<FOBB>& BoundingBoxes, int numBoundingBoxes) const;
    void UpdateConesBuffer(ID3D11Buffer* pConeBuffer, const TArray<FCone>& Cones, int numCones) const;
    void UpdateOffsetConstantBuffer(const void* pData, size_t dataSize);

    //Render Pass Demo
    void PrepareRender();
    void ClearRenderArr();
    void InitOnceState(std::shared_ptr<FEditorViewportClient> ActiveViewport);
    void Render(UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport);
    void RenderStaticMeshes(UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport);
    void RenderGizmos(const UWorld* World, const std::shared_ptr<FEditorViewportClient>& ActiveViewport);
    void RenderLight(UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport);
    void RenderBillboards(UWorld* World,std::shared_ptr<FEditorViewportClient> ActiveViewport);
    

    void MaterialSorting();

    TArray<UStaticMeshComponent*> CameraInStaticMeshObjs;
private:
    std::shared_ptr<FEditorViewportClient> CurrentViewport;
    TArray<UStaticMeshComponent*> StaticMeshObjs;
    TMap<class UStaticMesh*, StaticMeshInstanceData> StaticMeshBatch;
    int PrevStaticMeshObjsNum = 0;
    class UMaterial* CurrentMaterial = nullptr;
    class UStaticMesh* CurrentStaticMesh = nullptr;
    bool bIsDirtyRenderObj = true;

    //TMap<FName, TArray<UStaticMeshComponent>> StaticMeshObjsSorting;
    TArray<UGizmoBaseComponent*> GizmoObjs;
    TArray<UBillboardComponent*> BillboardObjs;
    TArray<ULightComponentBase*> LightObjs;
    


public:
    ID3D11VertexShader* VertexLineShader = nullptr;
    ID3D11PixelShader* PixelLineShader = nullptr;
    ID3D11Buffer* GridConstantBuffer = nullptr;
    ID3D11Buffer* LinePrimitiveBuffer = nullptr;


    ID3D11Buffer* StructBuffer = nullptr;
    ID3D11ShaderResourceView* pStructBufferSRV = nullptr;
    ID3D11Buffer* OffsetConstantBuffer = nullptr;

    ID3D11ShaderResourceView* pBBSRV = nullptr;
    ID3D11ShaderResourceView* pConeSRV = nullptr;
    ID3D11ShaderResourceView* pOBBSRV = nullptr;
};

