#include "Renderer.h"
#include <d3dcompiler.h>

#include "World.h"
#include "Actors/Player.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/UBillboardComponent.h"
#include "Components/UParticleSubUVComp.h"
#include "Components/UText.h"
#include "Components/Material/Material.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Launch/EngineLoop.h"
#include "Math/JungleMath.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "UObject/Casts.h"
#include "UObject/Object.h"
#include "PropertyEditor/ShowFlags.h"
#include "UObject/UObjectIterator.h"
#include "Components/SkySphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Runtime/Engine/Classes/Engine/FLoaderOBJ.h"

void FRenderer::Initialize(FGraphicsDevice* graphics)
{
    Graphics = graphics;
    CreateShader();
    CreateLineShader();
    CreateConstantBuffer();

}

void FRenderer::Release()
{
    ReleaseShader();
    ReleaseTextureShader();
    ReleaseLineShader();
    ReleaseConstantBuffer();
    ReleaseInstanceData();
}

void FRenderer::CreateShader()
{
    ID3DBlob* VertexShaderCSO;
    ID3DBlob* PixelShaderCSO;

    D3DCompileFromFile(L"Shaders/StaticMeshVertexShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &VertexShaderCSO, nullptr);
    Graphics->Device->CreateVertexShader(VertexShaderCSO->GetBufferPointer(), VertexShaderCSO->GetBufferSize(), nullptr, &VertexShader);

    D3DCompileFromFile(L"Shaders/StaticMeshPixelShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &PixelShaderCSO, nullptr);
    Graphics->Device->CreatePixelShader(PixelShaderCSO->GetBufferPointer(), PixelShaderCSO->GetBufferSize(), nullptr, &PixelShader);

    //D3D11_INPUT_ELEMENT_DESC layout[] = {
    //    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    //    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    //};

    //Graphics->Device->CreateInputLayout(
    //    layout, ARRAYSIZE(layout), VertexShaderCSO->GetBufferPointer(), VertexShaderCSO->GetBufferSize(), &InputLayout
    //);


    //// FVertexSimpleIndex 구조체에 맞는 Input Layout 정의
    D3D11_INPUT_ELEMENT_DESC layoutInstance[] = {
        {
            "POSITION",                         // 1. 시맨틱 이름 (셰이더 입력과 일치해야 함)
            0,                                  // 2. 시맨틱 인덱스 (같은 이름이 여러 개일 때 구분)
            DXGI_FORMAT_R32G32B32_FLOAT,        // 3. 데이터 형식 (float x, y, z)
            0,                                  // 4. 입력 슬롯 (여러 버텍스 버퍼 사용 시 구분, 보통 0)
            0,                                  // 5. 정렬된 바이트 오프셋 (구조체 시작부터의 거리)
            D3D11_INPUT_PER_VERTEX_DATA,        // 6. 입력 분류 (정점 단위 데이터)
            0                                   // 7. 인스턴스 데이터 스텝 레이트 (정점 단위는 0)
        },
        {
            "TEXCOORD",                         // 1. 시맨틱 이름
            0,                                  // 2. 시맨틱 인덱스
            DXGI_FORMAT_R32G32_FLOAT,           // 3. 데이터 형식 (float u, v)
            0,                                  // 4. 입력 슬롯
            12,                                 // 5. 바이트 오프셋 (Position 다음: 3 * 4 = 12)
            D3D11_INPUT_PER_VERTEX_DATA,        // 6. 입력 분류
            0                                   // 7. 인스턴스 데이터 스텝 레이트
        },
        {
            "MATRIXINDEX",                      // 1. 시맨틱 이름 (셰이더에서 이 이름으로 받아야 함!)
            0,                                  // 2. 시맨틱 인덱스
            DXGI_FORMAT_R32_SINT,               // 3. 데이터 형식 (int MatrixIndex). 만약 uint32로 바꿨다면 R32_UINT
            0,                                  // 4. 입력 슬롯
            20,                                 // 5. 바이트 오프셋 (Position(12) + Texcoord(8) 다음: 12 + 8 = 20)
            D3D11_INPUT_PER_VERTEX_DATA,        // 6. 입력 분류
            0                                   // 7. 인스턴스 데이터 스텝 레이트
        }
    };

    // Input Layout 생성 시도
    if (Graphics && Graphics->Device && VertexShaderCSO)
    {
        HRESULT hr = Graphics->Device->CreateInputLayout(
            layoutInstance,                      // 위에서 정의한 레이아웃 배열
            ARRAYSIZE(layoutInstance),           // 배열의 요소 개수
            VertexShaderCSO->GetBufferPointer(), // 이 레이아웃을 사용할 정점 셰이더의 바이트코드
            VertexShaderCSO->GetBufferSize(),    // 정점 셰이더 바이트코드 크기
            &InputLayout                 // 생성된 Input Layout 객체를 저장할 포인터
        );
    }

    //Stride = sizeof(FVertexSimple);
    Stride = sizeof(FVertexSimpleIndex);
    VertexShaderCSO->Release();
    PixelShaderCSO->Release();
}


void FRenderer::ReleaseShader()
{
    if (InputLayout)
    {
        InputLayout->Release();
        InputLayout = nullptr;
    }

    if (PixelShader)
    {
        PixelShader->Release();
        PixelShader = nullptr;
    }

    if (VertexShader)
    {
        VertexShader->Release();
        VertexShader = nullptr;
    }
}

void FRenderer::PrepareShader() const
{
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);

    if (ConstantBuffer)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(1, 1, &MaterialConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(2, 1, &LightingBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(3, 1, &FlagBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(4, 1, &SubMeshConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(5, 1, &TextureConstantBufer);

    }
    if (ConstantBufferXM) {

        Graphics->DeviceContext->VSSetConstantBuffers(6, 1, &ConstantBufferXM);
        Graphics->DeviceContext->PSSetConstantBuffers(6, 1, &ConstantBufferXM);
    }

    if(pStructBufferSRV)
    {
        ID3D11ShaderResourceView* pSRVs[] = { pStructBufferSRV };

        Graphics->DeviceContext->VSSetShaderResources(
            10,       // 시작 슬롯 번호 (HLSL의 register(t10)에 해당)
            1,       // 설정할 뷰의 개수
            pSRVs    // 설정할 SRV 배열의 포인터
            );


        Graphics->DeviceContext->VSSetConstantBuffers(10, 1, &OffsetConstantBuffer);
    }



}

void FRenderer::ResetVertexShader() const
{
    Graphics->DeviceContext->VSSetShader(nullptr, nullptr, 0);
    VertexShader->Release();
}

void FRenderer::ResetPixelShader() const
{
    Graphics->DeviceContext->PSSetShader(nullptr, nullptr, 0);
    PixelShader->Release();
}

void FRenderer::SetVertexShader(const FWString& filename, const FString& funcname, const FString& version)
{
    // ���� �߻��� ���ɼ��� ����
    if (Graphics == nullptr)
        assert(0);
    if (VertexShader != nullptr)
        ResetVertexShader();
    if (InputLayout != nullptr)
        InputLayout->Release();
    ID3DBlob* vertexshaderCSO;

    D3DCompileFromFile(filename.c_str(), nullptr, nullptr, *funcname, *version, 0, 0, &vertexshaderCSO, nullptr);
    Graphics->Device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), nullptr, &VertexShader);
    vertexshaderCSO->Release();
}

void FRenderer::SetPixelShader(const FWString& filename, const FString& funcname, const FString& version)
{
    // ���� �߻��� ���ɼ��� ����
    if (Graphics == nullptr)
        assert(0);
    if (VertexShader != nullptr)
        ResetVertexShader();
    ID3DBlob* pixelshaderCSO;
    D3DCompileFromFile(filename.c_str(), nullptr, nullptr, *funcname, *version, 0, 0, &pixelshaderCSO, nullptr);
    Graphics->Device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), nullptr, &PixelShader);

    pixelshaderCSO->Release();
}

void FRenderer::ChangeViewMode(EViewModeIndex evi) const
{
    switch (evi)
    {
    case EViewModeIndex::VMI_Lit:
        //UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_Wireframe:
    case EViewModeIndex::VMI_Unlit:
        //UpdateLitUnlitConstant(0);
        break;
    }
}

void FRenderer::RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const
{
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
    Graphics->DeviceContext->Draw(numVertices, 0);
}

void FRenderer::RenderPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices) const
{
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    Graphics->DeviceContext->DrawIndexed(numIndices, 0, 0);
}

void FRenderer::RenderPrimitive(OBJ::FStaticMeshRenderData* renderData, TArray<FStaticMaterial*> materials, TArray<UMaterial*> overrideMaterial, int selectedSubMeshIndex = -1)
{
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &renderData->VertexBuffer, &Stride, &offset);

    if (renderData->IndexBuffer)
        Graphics->DeviceContext->IASetIndexBuffer(renderData->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    if (renderData->MaterialSubsets.Num() == 0)
    {
        // no submesh
        Graphics->DeviceContext->DrawIndexed(renderData->Indices.Num(), 0, 0);
    }

    for (int subMeshIndex = 0; subMeshIndex < renderData->MaterialSubsets.Num(); subMeshIndex++)
    {
        int materialIndex = renderData->MaterialSubsets[subMeshIndex].MaterialIndex;

        //subMeshIndex == selectedSubMeshIndex ? UpdateSubMeshConstant(true) : UpdateSubMeshConstant(false);


        //bool b = ;

        if (materials[materialIndex]->Material != CurrentMaterial)
        {
            overrideMaterial[materialIndex] != nullptr ?
                UpdateMaterial(overrideMaterial[materialIndex]->GetMaterialInfo()) : UpdateMaterial(materials[materialIndex]->Material->GetMaterialInfo());

            CurrentMaterial = materials[materialIndex]->Material;
        }

        if (renderData->IndexBuffer)
        {
            // index draw
            uint64 startIndex = renderData->MaterialSubsets[subMeshIndex].IndexStart;
            uint64 indexCount = renderData->MaterialSubsets[subMeshIndex].IndexCount;
            Graphics->DeviceContext->DrawIndexed(indexCount, startIndex, 0);
        }
    }
}

void FRenderer::RenderTexturedModelPrimitive(
    ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices, ID3D11ShaderResourceView* InTextureSRV,
    ID3D11SamplerState* InSamplerState
) const
{
    if (!InTextureSRV || !InSamplerState)
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "SRV, Sampler Error");
    }
    if (numIndices <= 0)
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "numIndices Error");
    }
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    //Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &InTextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &InSamplerState);

    Graphics->DeviceContext->DrawIndexed(numIndices, 0, 0);
}

ID3D11Buffer* FRenderer::CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth) const
{
    // 2. Create a vertex buffer
    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = byteWidth;
    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated 
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

    ID3D11Buffer* vertexBuffer;

    HRESULT hr = Graphics->Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);
    if (FAILED(hr))
    {
        UE_LOG(LogLevel::Warning, "VertexBuffer Creation faild");
    }
    return vertexBuffer;
}

ID3D11Buffer* FRenderer::CreateVertexBufferForManualInstancing(const TArray<FVertexSimple>& originalVertices, UINT numInstances, StaticMeshInstanceData& Data) const
{
    TArray<FVertexSimpleIndex> verticesArray;
    // 예상 크기를 미리 예약하여 재할당 방지 (성능 향상)
    verticesArray.Reserve(originalVertices.Num() * numInstances);

    // 바깥 루프: 각 인스턴스에 대해
    for (UINT instanceIdx = 0; instanceIdx < numInstances; ++instanceIdx)
    {
        // 안쪽 루프: 원본 메쉬의 모든 정점에 대해
        for (const FVertexSimple& originalVertex : originalVertices)
        {
            FVertexSimpleIndex vertexWithIndex;
            vertexWithIndex.x = originalVertex.x;
            vertexWithIndex.y = originalVertex.y;
            vertexWithIndex.z = originalVertex.z;
            vertexWithIndex.u = originalVertex.u;
            vertexWithIndex.v = originalVertex.v;
            vertexWithIndex.MatrixIndex = instanceIdx; // 현재 인스턴스의 인덱스 할당

            verticesArray.Add(vertexWithIndex);
        }
    }

    // 올바른 ByteWidth 계산
    UINT totalByteWidth = verticesArray.Num() * sizeof(FVertexSimpleIndex);

    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = totalByteWidth;
    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE; // 생성 후 변경하지 않음
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexbufferdesc.CPUAccessFlags = 0; // IMMUTABLE은 CPU 접근 불필요
    vertexbufferdesc.MiscFlags = 0;
    vertexbufferdesc.StructureByteStride = 0; // 일반 정점 버퍼는 0

    D3D11_SUBRESOURCE_DATA vertexbufferSRD;
    vertexbufferSRD.pSysMem = verticesArray.GetData();
    // 아래 두 값은 버퍼 타입에 따라 사용되므로 0으로 설정 (정점 버퍼에는 불필요)
    vertexbufferSRD.SysMemPitch = 0;
    vertexbufferSRD.SysMemSlicePitch = 0;

    ID3D11Buffer* vertexBuffer = nullptr;

    HRESULT hr = Graphics->Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);
    if (FAILED(hr))
    {
        UE_LOG(LogLevel::Error, "VertexBuffer Creation failed! HRESULT: %lx", hr);
        return nullptr; // 실패 시 nullptr 반환
    }

    Data.VertexBufferInstance = vertexBuffer;
    Data.NumVerticesPerInstance = originalVertices.Num();


    return vertexBuffer;
}

ID3D11Buffer* FRenderer::CreateVertexBuffer(const TArray<FVertexSimple>& vertices, UINT byteWidth) const
{
    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = byteWidth;
    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated 
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexbufferSRD;
    vertexbufferSRD.pSysMem = vertices.GetData();

    ID3D11Buffer* vertexBuffer;

    HRESULT hr = Graphics->Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);
    if (FAILED(hr))
    {
        UE_LOG(LogLevel::Warning, "VertexBuffer Creation faild");
    }
    return vertexBuffer;
}



ID3D11Buffer* FRenderer::CreateIndexBuffer(uint32* indices, UINT byteWidth) const
{
    D3D11_BUFFER_DESC indexbufferdesc = {};              // buffer�� ����, �뵵 ���� ����
    indexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;       // immutable: gpu�� �б� �������� ������ �� �ִ�.
    indexbufferdesc.BindFlags = D3D11_BIND_INDEX_BUFFER; // index buffer�� ����ϰڴ�.
    indexbufferdesc.ByteWidth = byteWidth;               // buffer ũ�� ����

    D3D11_SUBRESOURCE_DATA indexbufferSRD = { indices };

    ID3D11Buffer* indexBuffer;

    HRESULT hr = Graphics->Device->CreateBuffer(&indexbufferdesc, &indexbufferSRD, &indexBuffer);
    if (FAILED(hr))
    {
        UE_LOG(LogLevel::Warning, "IndexBuffer Creation faild");
    }
    return indexBuffer;
}

ID3D11Buffer* FRenderer::CreateIndexBuffer(const TArray<UINT>& indices, UINT byteWidth) const
{
    D3D11_BUFFER_DESC indexbufferdesc = {};              // buffer�� ����, �뵵 ���� ����
    indexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;       // immutable: gpu�� �б� �������� ������ �� �ִ�.
    indexbufferdesc.BindFlags = D3D11_BIND_INDEX_BUFFER; // index buffer�� ����ϰڴ�.
    indexbufferdesc.ByteWidth = byteWidth;               // buffer ũ�� ����

    D3D11_SUBRESOURCE_DATA indexbufferSRD;
    indexbufferSRD.pSysMem = indices.GetData();

    ID3D11Buffer* indexBuffer;

    HRESULT hr = Graphics->Device->CreateBuffer(&indexbufferdesc, &indexbufferSRD, &indexBuffer);
    if (FAILED(hr))
    {
        UE_LOG(LogLevel::Warning, "IndexBuffer Creation faild");
    }
    return indexBuffer;
}

/**
 * 수동 인스턴싱을 위한 인덱스 버퍼를 생성합니다.
 * 원본 인덱스 배열을 복제하고 각 인스턴스에 맞는 정점 오프셋을 추가합니다.
 *
 * @param originalIndices      - 원본 메쉬의 인덱스 배열
 * @param numOriginalVertices  - 원본 메쉬의 총 정점 개수 (오프셋 계산에 필요)
 * @param numInstances         - 이 인덱스 버퍼가 참조할 인스턴스의 개수
 * @return 생성된 인덱스 버퍼 포인터, 실패 시 nullptr
 */

ID3D11Buffer* FRenderer::CreateIndexBufferForManualInstancing(const TArray<UINT>& originalIndices, UINT numOriginalVertices, UINT numInstances, StaticMeshInstanceData& Data) const
{
    if (numInstances == 0 || originalIndices.Num() == 0)
    {
        UE_LOG(LogLevel::Warning, "Cannot create index buffer with zero instances, indices, or original vertices.");
        return nullptr;
    }


    uint32 indexMaxNum = originalIndices.Num() * sizeof(UINT);

    TArray<UINT> instancedIndicesArray;
    // 예상 크기를 미리 예약하여 재할당 방지
    instancedIndicesArray.Reserve(originalIndices.Num() * numInstances);

    // 바깥 루프: 각 인스턴스에 대해
    for (UINT instanceIdx = 0; instanceIdx < numInstances; ++instanceIdx)
    {
        // 현재 인스턴스의 시작 정점 오프셋 계산
        // 인스턴스 0의 오프셋은 0, 인스턴스 1의 오프셋은 numOriginalVertices, ...
        UINT vertexOffset = instanceIdx * numOriginalVertices;

        // 안쪽 루프: 원본 인덱스 배열의 모든 인덱스에 대해
        for (UINT originalIndex : originalIndices)
        {
            // 원본 인덱스에 정점 오프셋을 더하여 조정된 인덱스 계산
            UINT adjustedIndex = originalIndex + vertexOffset;
            instancedIndicesArray.Add(adjustedIndex);
        }
    }

    // 최종 인덱스 배열의 전체 바이트 크기 계산
    UINT totalByteWidth = instancedIndicesArray.Num() * sizeof(UINT);

    if (totalByteWidth == 0) // 혹시 모를 0 크기 버퍼 생성 방지
    {
        UE_LOG(LogLevel::Warning, "Calculated index buffer size is zero.");
        return nullptr;
    }

    D3D11_BUFFER_DESC indexbufferdesc = {};
    indexbufferdesc.ByteWidth = totalByteWidth;
    indexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE; // 생성 후 변경하지 않음
    indexbufferdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexbufferdesc.CPUAccessFlags = 0; // IMMUTABLE은 CPU 접근 불필요
    indexbufferdesc.MiscFlags = 0;
    indexbufferdesc.StructureByteStride = 0; // 인덱스 버퍼는 0

    D3D11_SUBRESOURCE_DATA indexbufferSRD = {}; // 초기화 중요!
    indexbufferSRD.pSysMem = instancedIndicesArray.GetData();
    indexbufferSRD.SysMemPitch = 0;
    indexbufferSRD.SysMemSlicePitch = 0;

    ID3D11Buffer* indexBuffer = nullptr; // nullptr로 초기화

    HRESULT hr = Graphics->Device->CreateBuffer(&indexbufferdesc, &indexbufferSRD, &indexBuffer);
    if (FAILED(hr))
    {
        UE_LOG(LogLevel::Error, "IndexBuffer Creation failed! HRESULT: %lx", hr);
        return nullptr; // 실패 시 nullptr 반환
    }

    //UE_LOG(LogLevel::Info, "IndexBuffer created successfully for %u instances.", numInstances);

    Data.IndexBufferInstance = indexBuffer;
    Data.NumIndicesPerInstance = originalIndices.Num();
    return indexBuffer;
}

bool FRenderer::CreateStructuredBuffer(ID3D11Buffer** OutBuffer, ID3D11ShaderResourceView** OutSRV, UINT ElementCount, UINT ElementSize, const void* pInitialData, bool bDynamic)
{

    //이미 만들어져있으면 다시 만듬
    if (!Graphics || !Graphics->Device || !OutBuffer || !OutSRV || ElementCount == 0 || ElementSize == 0)
    {
        UE_LOG(LogLevel::Error, TEXT("CreateStructuredBuffer - Invalid arguments."));
        if (OutSRV)
        {
            (*OutSRV)->Release();
            *OutSRV = nullptr;
        }

        if (OutBuffer)
        {
            (*OutBuffer)->Release();
            *OutBuffer = nullptr;
        }
    }

    *OutBuffer = nullptr;
    *OutSRV = nullptr;

    // 2. D3D11_BUFFER_DESC 설정
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = ElementCount * ElementSize; // 전체 버퍼 크기
    bufferDesc.Usage = bDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT; // 용도 설정
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // 셰이더 리소스로 바인딩
    bufferDesc.CPUAccessFlags = bDynamic ? D3D11_CPU_ACCESS_WRITE : 0; // Dynamic일 때만 CPU 쓰기 허용
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; // *** Structured Buffer임을 명시 ***
    bufferDesc.StructureByteStride = ElementSize; // *** 구조체 하나의 크기 명시 ***

    HRESULT hr;

    // 3. ID3D11Device::CreateBuffer 호출
    if (pInitialData)
    {
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = pInitialData;
        hr = Graphics->Device->CreateBuffer(&bufferDesc, &initData, OutBuffer);
    }
    else
    {
        // Dynamic 버퍼는 보통 초기 데이터 없이 만들고 Map/Unmap으로 업데이트
        // Default 버퍼는 초기 데이터 없으면 0으로 초기화될 수 있음
        hr = Graphics->Device->CreateBuffer(&bufferDesc, nullptr, OutBuffer);
    }

    if (FAILED(hr))
    {
        UE_LOG(LogLevel::Error, TEXT("CreateBuffer for Structured Buffer failed! HRESULT: %lx"), hr);
        if (*OutBuffer) (*OutBuffer)->Release();
        *OutBuffer = nullptr;
        return false;
    }

    // 4. D3D11_SHADER_RESOURCE_VIEW_DESC 설정
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN; // *** Structured Buffer는 UNKNOWN 포맷 사용 ***
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER; // 버퍼 리소스 뷰
    srvDesc.Buffer.FirstElement = 0; // 버퍼의 시작 요소 인덱스
    srvDesc.Buffer.NumElements = ElementCount; // 뷰가 커버할 요소 개수

    // 5. ID3D11Device::CreateShaderResourceView 호출
    hr = Graphics->Device->CreateShaderResourceView(*OutBuffer, &srvDesc, OutSRV);


    if (OutBuffer == nullptr, OutSRV == nullptr)
    {
        MsgBoxAssert("StructuredBuffer Creation failed");
    }
}

void FRenderer::ReleaseBuffer(ID3D11Buffer*& Buffer) const
{
    if (Buffer)
    {
        Buffer->Release();
        Buffer = nullptr;
    }
}

void FRenderer::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth = (sizeof(FConstants) + 0xf) & 0xfffffff0;
    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    Graphics->Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
    constantbufferdesc.ByteWidth = sizeof(FConstantsXM) + 0xf & 0xfffffff0;
    Graphics->Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBufferXM);

    constantbufferdesc.ByteWidth = sizeof(FSubUVConstant) + 0xf & 0xfffffff0;
    Graphics->Device->CreateBuffer(&constantbufferdesc, nullptr, &SubUVConstantBuffer);

    constantbufferdesc.ByteWidth = sizeof(FGridParameters) + 0xf & 0xfffffff0;
    Graphics->Device->CreateBuffer(&constantbufferdesc, nullptr, &GridConstantBuffer);

    constantbufferdesc.ByteWidth = sizeof(FPrimitiveCounts) + 0xf & 0xfffffff0;
    Graphics->Device->CreateBuffer(&constantbufferdesc, nullptr, &LinePrimitiveBuffer);

    constantbufferdesc.ByteWidth = sizeof(FMaterialConstants) + 0xf & 0xfffffff0;
    Graphics->Device->CreateBuffer(&constantbufferdesc, nullptr, &MaterialConstantBuffer);

    constantbufferdesc.ByteWidth = sizeof(FSubMeshConstants) + 0xf & 0xfffffff0;
    Graphics->Device->CreateBuffer(&constantbufferdesc, nullptr, &SubMeshConstantBuffer);

    constantbufferdesc.ByteWidth = sizeof(FTextureConstants) + 0xf & 0xfffffff0;
    Graphics->Device->CreateBuffer(&constantbufferdesc, nullptr, &TextureConstantBufer);

    constantbufferdesc.ByteWidth = sizeof(FOffsetConstantData) + 0xf & 0xfffffff0;
    Graphics->Device->CreateBuffer(&constantbufferdesc, nullptr, &OffsetConstantBuffer);
}

void FRenderer::CreateLightingBuffer()
{
    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth = sizeof(FLighting);
    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Graphics->Device->CreateBuffer(&constantbufferdesc, nullptr, &LightingBuffer);
}

void FRenderer::CreateLitUnlitBuffer()
{
    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth = sizeof(FLitUnlitConstants);
    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Graphics->Device->CreateBuffer(&constantbufferdesc, nullptr, &FlagBuffer);
}

void FRenderer::ReleaseConstantBuffer()
{
    if (ConstantBuffer)
    {
        ConstantBuffer->Release();
        ConstantBuffer = nullptr;
    }

    if (LightingBuffer)
    {
        LightingBuffer->Release();
        LightingBuffer = nullptr;
    }

    if (FlagBuffer)
    {
        FlagBuffer->Release();
        FlagBuffer = nullptr;
    }

    if (MaterialConstantBuffer)
    {
        MaterialConstantBuffer->Release();
        MaterialConstantBuffer = nullptr;
    }

    if (SubMeshConstantBuffer)
    {
        SubMeshConstantBuffer->Release();
        SubMeshConstantBuffer = nullptr;
    }

    if (TextureConstantBufer)
    {
        TextureConstantBufer->Release();
        TextureConstantBufer = nullptr;
    }
}

void FRenderer::ReleaseInstanceData()
{
    for (auto& iter : StaticMeshBatch)
    {
        if(iter.Value.VertexBufferInstance)
        {
            iter.Value.VertexBufferInstance->Release();
            iter.Value.VertexBufferInstance = nullptr;
        }
        if (iter.Value.IndexBufferInstance)
        {
            iter.Value.IndexBufferInstance->Release();
            iter.Value.IndexBufferInstance = nullptr;
        }

    }

    if (pStructBufferSRV)
    {
        pStructBufferSRV->Release();
        pStructBufferSRV = nullptr;
    }

    if(StructBuffer)
    {
        StructBuffer->Release();
        StructBuffer = nullptr;
    }


}

void FRenderer::UpdateLightBuffer() const
{
    if (!LightingBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    Graphics->DeviceContext->Map(LightingBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    {
        FLighting* constants = static_cast<FLighting*>(mappedResource.pData);
        constants->lightDirX = 1.0f; // ��: ���� ������ �Ʒ��� �������� ���
        constants->lightDirY = 1.0f; // ��: ���� ������ �Ʒ��� �������� ���
        constants->lightDirZ = 1.0f; // ��: ���� ������ �Ʒ��� �������� ���
        constants->lightColorX = 1.0f;
        constants->lightColorY = 1.0f;
        constants->lightColorZ = 1.0f;
        constants->AmbientFactor = 0.06f;
    }
    Graphics->DeviceContext->Unmap(LightingBuffer, 0);
}

void FRenderer::UpdateConstant(const FMatrix& MVP, FVector4 UUIDColor, bool IsSelected) const
{
    if (ConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR; // GPU�� �޸� �ּ� ����

        Graphics->DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        {
            FConstants* constants = static_cast<FConstants*>(ConstantBufferMSR.pData);
            constants->MVP = MVP;
            constants->IsSelected = IsSelected;

        }
        Graphics->DeviceContext->Unmap(ConstantBuffer, 0); // GPU�� �ٽ� ��밡���ϰ� �����
    }
}
void FRenderer::UpdateConstantXM(const DirectX::XMMATRIX& MVP, FVector4 UUIDColor, bool IsSelected) const
{
    if (ConstantBufferXM)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR; // GPU�� �޸� �ּ� ����

        Graphics->DeviceContext->Map(ConstantBufferXM, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        {
            FConstantsXM* constants = static_cast<FConstantsXM*>(ConstantBufferMSR.pData);
            XMStoreFloat4x4(&constants->MVP, MVP);
            constants->IsSelected = IsSelected;
        }
        Graphics->DeviceContext->Unmap(ConstantBufferXM, 0); // GPU�� �ٽ� ��밡���ϰ� �����
    }
}

void FRenderer::UpdateMaterial(const FObjMaterialInfo& MaterialInfo) const
{
    //if (MaterialConstantBuffer)
    //{
    //    D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR; // GPU�� �޸� �ּ� ����

    //    Graphics->DeviceContext->Map(MaterialConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
    //    {
    //        FMaterialConstants* constants = static_cast<FMaterialConstants*>(ConstantBufferMSR.pData);
    //        constants->DiffuseColor = MaterialInfo.Diffuse;
    //        constants->TransparencyScalar = MaterialInfo.TransparencyScalar;
    //        constants->AmbientColor = MaterialInfo.Ambient;
    //        constants->DensityScalar = MaterialInfo.DensityScalar;
    //        constants->SpecularColor = MaterialInfo.Specular;
    //        constants->SpecularScalar = MaterialInfo.SpecularScalar;
    //        constants->EmmisiveColor = MaterialInfo.Emissive;
    //    }
    //    Graphics->DeviceContext->Unmap(MaterialConstantBuffer, 0); // GPU�� �ٽ� ��밡���ϰ� �����
    //}

    if (MaterialInfo.bHasTexture == true)
    {
        std::shared_ptr<FTexture> texture = FEngineLoop::resourceMgr.GetTexture(MaterialInfo.DiffuseTexturePath);
        Graphics->DeviceContext->PSSetShaderResources(0, 1, &texture->TextureSRV);
        Graphics->DeviceContext->PSSetSamplers(0, 1, &texture->SamplerState);
    }
    else
    {
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        ID3D11SamplerState* nullSampler[1] = { nullptr };

        Graphics->DeviceContext->PSSetShaderResources(0, 1, nullSRV);
        Graphics->DeviceContext->PSSetSamplers(0, 1, nullSampler);
    }
}
//
//void FRenderer::UpdateLitUnlitConstant(int isLit) const
//{
//    if (FlagBuffer)
//    {
//        D3D11_MAPPED_SUBRESOURCE constantbufferMSR; // GPU �� �޸� �ּ� ����
//        Graphics->DeviceContext->Map(FlagBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
//        auto constants = static_cast<FLitUnlitConstants*>(constantbufferMSR.pData); //GPU �޸� ���� ����
//        {
//            constants->isLit = isLit;
//        }
//        Graphics->DeviceContext->Unmap(FlagBuffer, 0);
//    }
//}
//
//void FRenderer::UpdateSubMeshConstant(bool isSelected) const
//{
//    if (SubMeshConstantBuffer) {
//        D3D11_MAPPED_SUBRESOURCE constantbufferMSR; // GPU �� �޸� �ּ� ����
//        Graphics->DeviceContext->Map(SubMeshConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
//        FSubMeshConstants* constants = (FSubMeshConstants*)constantbufferMSR.pData; //GPU �޸� ���� ����
//        {
//            constants->isSelectedSubMesh = isSelected;
//        }
//        Graphics->DeviceContext->Unmap(SubMeshConstantBuffer, 0);
//    }
//}
//
//void FRenderer::UpdateTextureConstant(float UOffset, float VOffset)
//{
//    if (TextureConstantBufer) {
//        D3D11_MAPPED_SUBRESOURCE constantbufferMSR; // GPU �� �޸� �ּ� ����
//        Graphics->DeviceContext->Map(TextureConstantBufer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
//        FTextureConstants* constants = (FTextureConstants*)constantbufferMSR.pData; //GPU �޸� ���� ����
//        {
//            constants->UOffset = UOffset;
//            constants->VOffset = VOffset;
//        }
//        Graphics->DeviceContext->Unmap(TextureConstantBufer, 0);
//    }
//}

void FRenderer::CreateTextureShader()
{
    ID3DBlob* vertextextureshaderCSO;
    ID3DBlob* pixeltextureshaderCSO;

    HRESULT hr;
    hr = D3DCompileFromFile(L"Shaders/VertexTextureShader.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertextextureshaderCSO, nullptr);
    if (FAILED(hr))
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "VertexShader Error");
    }
    Graphics->Device->CreateVertexShader(
        vertextextureshaderCSO->GetBufferPointer(), vertextextureshaderCSO->GetBufferSize(), nullptr, &VertexTextureShader
    );

    hr = D3DCompileFromFile(L"Shaders/PixelTextureShader.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &pixeltextureshaderCSO, nullptr);
    if (FAILED(hr))
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "PixelShader Error");
    }
    Graphics->Device->CreatePixelShader(
        pixeltextureshaderCSO->GetBufferPointer(), pixeltextureshaderCSO->GetBufferSize(), nullptr, &PixelTextureShader
    );

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    Graphics->Device->CreateInputLayout(
        layout, ARRAYSIZE(layout), vertextextureshaderCSO->GetBufferPointer(), vertextextureshaderCSO->GetBufferSize(), &TextureInputLayout
    );

    //�ڷᱸ�� ���� �ʿ�
    TextureStride = sizeof(FVertexTexture);
    vertextextureshaderCSO->Release();
    pixeltextureshaderCSO->Release();
}

void FRenderer::ReleaseTextureShader()
{
    if (TextureInputLayout)
    {
        TextureInputLayout->Release();
        TextureInputLayout = nullptr;
    }

    if (PixelTextureShader)
    {
        PixelTextureShader->Release();
        PixelTextureShader = nullptr;
    }

    if (VertexTextureShader)
    {
        VertexTextureShader->Release();
        VertexTextureShader = nullptr;
    }
    if (SubUVConstantBuffer)
    {
        SubUVConstantBuffer->Release();
        SubUVConstantBuffer = nullptr;
    }
    if (ConstantBuffer)
    {
        ConstantBuffer->Release();
        ConstantBuffer = nullptr;
    }
    if (ConstantBufferXM)
    {
        ConstantBufferXM->Release();
        ConstantBufferXM = nullptr;
    }
}

void FRenderer::PrepareTextureShader() const
{
    Graphics->DeviceContext->VSSetShader(VertexTextureShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelTextureShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(TextureInputLayout);

    //�ؽ��Ŀ� ConstantBuffer �߰��ʿ��Ҽ���
    if (ConstantBuffer)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
    }
}

ID3D11Buffer* FRenderer::CreateVertexTextureBuffer(FVertexTexture* vertices, UINT byteWidth) const
{
    // 2. Create a vertex buffer
    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = byteWidth;
    vertexbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will never be updated 
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    //D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

    ID3D11Buffer* vertexBuffer;

    HRESULT hr = Graphics->Device->CreateBuffer(&vertexbufferdesc, nullptr, &vertexBuffer);
    if (FAILED(hr))
    {
        UE_LOG(LogLevel::Warning, "VertexBuffer Creation faild");
    }
    return vertexBuffer;
}

ID3D11Buffer* FRenderer::CreateIndexTextureBuffer(uint32* indices, UINT byteWidth) const
{
    D3D11_BUFFER_DESC indexbufferdesc = {};
    indexbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
    indexbufferdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexbufferdesc.ByteWidth = byteWidth;
    indexbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Buffer* indexBuffer;

    HRESULT hr = Graphics->Device->CreateBuffer(&indexbufferdesc, nullptr, &indexBuffer);
    if (FAILED(hr))
    {
        return nullptr;
    }
    return indexBuffer;
}

void FRenderer::RenderTexturePrimitive(
    ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices, ID3D11ShaderResourceView* _TextureSRV,
    ID3D11SamplerState* _SamplerState
) const
{
    if (!_TextureSRV || !_SamplerState)
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "SRV, Sampler Error");
    }
    if (numIndices <= 0)
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "numIndices Error");
    }
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &TextureStride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &_TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &_SamplerState);

    Graphics->DeviceContext->DrawIndexed(numIndices, 0, 0);
}

//��Ʈ ��ġ������
void FRenderer::RenderTextPrimitive(
    ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11ShaderResourceView* _TextureSRV, ID3D11SamplerState* _SamplerState
) const
{
    if (!_TextureSRV || !_SamplerState)
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "SRV, Sampler Error");
    }
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &TextureStride, &offset);

    // �Է� ���̾ƿ� �� �⺻ ����
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &_TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &_SamplerState);

    // ��ο� ȣ�� (6���� �ε��� ���)
    Graphics->DeviceContext->Draw(numVertices, 0);
}


ID3D11Buffer* FRenderer::CreateVertexBuffer(FVertexTexture* vertices, UINT byteWidth) const
{
    // 2. Create a vertex buffer
    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = byteWidth;
    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated 
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

    ID3D11Buffer* vertexBuffer;

    HRESULT hr = Graphics->Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);
    if (FAILED(hr))
    {
        UE_LOG(LogLevel::Warning, "VertexBuffer Creation faild");
    }
    return vertexBuffer;
}

void FRenderer::UpdateSubUVConstant(float _indexU, float _indexV) const
{
    if (SubUVConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR; // GPU�� �޸� �ּ� ����

        Graphics->DeviceContext->Map(SubUVConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        auto constants = static_cast<FSubUVConstant*>(constantbufferMSR.pData);                               //GPU �޸� ���� ����
        {
            constants->indexU = _indexU;
            constants->indexV = _indexV;
        }
        Graphics->DeviceContext->Unmap(SubUVConstantBuffer, 0); // GPU�� �ٽ� ��밡���ϰ� �����
    }
}

void FRenderer::PrepareSubUVConstant() const
{
    if (SubUVConstantBuffer)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(1, 1, &SubUVConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(1, 1, &SubUVConstantBuffer);
    }
}

void FRenderer::PrepareLineShader() const
{
    // ���̴��� �Է� ���̾ƿ� ����
    Graphics->DeviceContext->VSSetShader(VertexLineShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelLineShader, nullptr, 0);

    // ��� ���� ���ε�: 
    // - MatrixBuffer�� register(b0)��, Vertex Shader�� ���ε�
    // - GridConstantBuffer�� register(b1)��, Vertex�� Pixel Shader�� ���ε� (�ȼ� ���̴��� �ʿ信 ����)
    if (ConstantBuffer && GridConstantBuffer)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);     // MatrixBuffer (b0)
        Graphics->DeviceContext->VSSetConstantBuffers(1, 1, &GridConstantBuffer); // GridParameters (b1)
        Graphics->DeviceContext->PSSetConstantBuffers(1, 1, &GridConstantBuffer);
        Graphics->DeviceContext->VSSetConstantBuffers(3, 1, &LinePrimitiveBuffer);
        Graphics->DeviceContext->VSSetShaderResources(2, 1, &pBBSRV);
        Graphics->DeviceContext->VSSetShaderResources(3, 1, &pConeSRV);
        Graphics->DeviceContext->VSSetShaderResources(4, 1, &pOBBSRV);
    }
}

void FRenderer::CreateLineShader()
{
    ID3DBlob* VertexShaderLine;
    ID3DBlob* PixelShaderLine;

    HRESULT hr;
    hr = D3DCompileFromFile(L"Shaders/ShaderLine.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &VertexShaderLine, nullptr);
    if (FAILED(hr))
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "VertexShader Error");
    }
    Graphics->Device->CreateVertexShader(VertexShaderLine->GetBufferPointer(), VertexShaderLine->GetBufferSize(), nullptr, &VertexLineShader);

    hr = D3DCompileFromFile(L"Shaders/ShaderLine.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &PixelShaderLine, nullptr);
    if (FAILED(hr))
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "PixelShader Error");
    }
    Graphics->Device->CreatePixelShader(PixelShaderLine->GetBufferPointer(), PixelShaderLine->GetBufferSize(), nullptr, &PixelLineShader);


    VertexShaderLine->Release();
    PixelShaderLine->Release();
}

void FRenderer::ReleaseLineShader() const
{
    if (GridConstantBuffer) GridConstantBuffer->Release();
    if (LinePrimitiveBuffer) LinePrimitiveBuffer->Release();
    if (VertexLineShader) VertexLineShader->Release();
    if (PixelLineShader) PixelLineShader->Release();
}

ID3D11Buffer* FRenderer::CreateStaticVerticesBuffer() const
{
    FSimpleVertex vertices[2]{ {0}, {0} };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA vbInitData = {};
    vbInitData.pSysMem = vertices;
    ID3D11Buffer* pVertexBuffer = nullptr;
    HRESULT hr = Graphics->Device->CreateBuffer(&vbDesc, &vbInitData, &pVertexBuffer);
    return pVertexBuffer;
}

ID3D11Buffer* FRenderer::CreateBoundingBoxBuffer(UINT numBoundingBoxes) const
{
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC; // ���� ������Ʈ�� ��� DYNAMIC, �׷��� ������ DEFAULT
    bufferDesc.ByteWidth = sizeof(FBoundingBox) * numBoundingBoxes;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(FBoundingBox);

    ID3D11Buffer* BoundingBoxBuffer = nullptr;
    Graphics->Device->CreateBuffer(&bufferDesc, nullptr, &BoundingBoxBuffer);
    return BoundingBoxBuffer;
}

ID3D11Buffer* FRenderer::CreateOBBBuffer(UINT numBoundingBoxes) const
{
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC; // ���� ������Ʈ�� ��� DYNAMIC, �׷��� ������ DEFAULT
    bufferDesc.ByteWidth = sizeof(FOBB) * numBoundingBoxes;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(FOBB);

    ID3D11Buffer* BoundingBoxBuffer = nullptr;
    Graphics->Device->CreateBuffer(&bufferDesc, nullptr, &BoundingBoxBuffer);
    return BoundingBoxBuffer;
}

ID3D11Buffer* FRenderer::CreateConeBuffer(UINT numCones) const
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(FCone) * numCones;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(FCone);

    ID3D11Buffer* ConeBuffer = nullptr;
    Graphics->Device->CreateBuffer(&bufferDesc, nullptr, &ConeBuffer);
    return ConeBuffer;
}

ID3D11ShaderResourceView* FRenderer::CreateBoundingBoxSRV(ID3D11Buffer* pBoundingBoxBuffer, UINT numBoundingBoxes)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN; // ����ü ������ ��� UNKNOWN
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementOffset = 0;
    srvDesc.Buffer.NumElements = numBoundingBoxes;


    Graphics->Device->CreateShaderResourceView(pBoundingBoxBuffer, &srvDesc, &pBBSRV);
    return pBBSRV;
}

ID3D11ShaderResourceView* FRenderer::CreateOBBSRV(ID3D11Buffer* pBoundingBoxBuffer, UINT numBoundingBoxes)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN; // ����ü ������ ��� UNKNOWN
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementOffset = 0;
    srvDesc.Buffer.NumElements = numBoundingBoxes;
    Graphics->Device->CreateShaderResourceView(pBoundingBoxBuffer, &srvDesc, &pOBBSRV);
    return pOBBSRV;
}

ID3D11ShaderResourceView* FRenderer::CreateConeSRV(ID3D11Buffer* pConeBuffer, UINT numCones)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN; // ����ü ������ ��� UNKNOWN
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementOffset = 0;
    srvDesc.Buffer.NumElements = numCones;


    Graphics->Device->CreateShaderResourceView(pConeBuffer, &srvDesc, &pConeSRV);
    return pConeSRV;
}

void FRenderer::UpdateBoundingBoxBuffer(ID3D11Buffer* pBoundingBoxBuffer, const TArray<FBoundingBox>& BoundingBoxes, int numBoundingBoxes) const
{
    if (!pBoundingBoxBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    Graphics->DeviceContext->Map(pBoundingBoxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    auto pData = reinterpret_cast<FBoundingBox*>(mappedResource.pData);
    for (int i = 0; i < BoundingBoxes.Num(); ++i)
    {
        pData[i] = BoundingBoxes[i];
    }
    Graphics->DeviceContext->Unmap(pBoundingBoxBuffer, 0);
}

void FRenderer::UpdateOBBBuffer(ID3D11Buffer* pBoundingBoxBuffer, const TArray<FOBB>& BoundingBoxes, int numBoundingBoxes) const
{
    if (!pBoundingBoxBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    Graphics->DeviceContext->Map(pBoundingBoxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    auto pData = reinterpret_cast<FOBB*>(mappedResource.pData);
    for (int i = 0; i < BoundingBoxes.Num(); ++i)
    {
        pData[i] = BoundingBoxes[i];
    }
    Graphics->DeviceContext->Unmap(pBoundingBoxBuffer, 0);
}

void FRenderer::UpdateConesBuffer(ID3D11Buffer* pConeBuffer, const TArray<FCone>& Cones, int numCones) const
{
    if (!pConeBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    Graphics->DeviceContext->Map(pConeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    auto pData = reinterpret_cast<FCone*>(mappedResource.pData);
    for (int i = 0; i < Cones.Num(); ++i)
    {
        pData[i] = Cones[i];
    }
    Graphics->DeviceContext->Unmap(pConeBuffer, 0);
}

void FRenderer::UpdateOffsetConstantBuffer(const void* pData, size_t dataSize)
{
    // 상수 버퍼의 크기와 업데이트하려는 데이터 크기가 일치하는지 확인 (선택적이지만 권장)
    D3D11_BUFFER_DESC desc;
    OffsetConstantBuffer->GetDesc(&desc);
    if (dataSize > desc.ByteWidth)
    {
        UE_LOG(LogLevel::Error, TEXT("UpdateOffsetConstantBuffer - Data size (%llu) exceeds buffer size (%u)."), dataSize, desc.ByteWidth);
        return;
    }
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Graphics->DeviceContext->Map(
        OffsetConstantBuffer,      // 업데이트할 상수 버퍼
        0,                            // Subresource (보통 0)
        D3D11_MAP_WRITE_DISCARD,      // 쓰기 모드 (Dynamic 버퍼에 효율적)
        0,                            // Map Flags
        &mappedResource               // 맵핑된 메모리 포인터 받을 구조체
    );

    if (SUCCEEDED(hr))
    {
        // 맵핑된 메모리에 데이터 복사
        memcpy(mappedResource.pData, pData, dataSize);

        // 맵핑 해제
        Graphics->DeviceContext->Unmap(OffsetConstantBuffer, 0);
    }

}

void FRenderer::UpdateGridConstantBuffer(const FGridParameters& gridParams) const
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Graphics->DeviceContext->Map(GridConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &gridParams, sizeof(FGridParameters));
        Graphics->DeviceContext->Unmap(GridConstantBuffer, 0);
    }
    else
    {
        UE_LOG(LogLevel::Warning, "gridParams ���� ����");
    }
}

void FRenderer::UpdateLinePrimitveCountBuffer(int numBoundingBoxes, int numCones) const
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Graphics->DeviceContext->Map(LinePrimitiveBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    auto pData = static_cast<FPrimitiveCounts*>(mappedResource.pData);
    pData->BoundingBoxCount = numBoundingBoxes;
    pData->ConeCount = numCones;
    Graphics->DeviceContext->Unmap(LinePrimitiveBuffer, 0);
}

void FRenderer::RenderBatch(
    const FGridParameters& gridParam, ID3D11Buffer* pVertexBuffer, int boundingBoxCount, int coneCount, int coneSegmentCount, int obbCount
) const
{
    UINT stride = sizeof(FSimpleVertex);
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    UINT vertexCountPerInstance = 2;
    UINT instanceCount = gridParam.numGridLines + 3 + (boundingBoxCount * 12) + (coneCount * (2 * coneSegmentCount)) + (12 * obbCount);
    Graphics->DeviceContext->DrawInstanced(vertexCountPerInstance, instanceCount, 0, 0);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void FRenderer::PrepareRender()
{
    if (bIsDirtyRenderObj == true)
    {



        //그려질 객체를 가지고온다.

        //TArray<UStaticMeshComponent*> MeshObjs;
        for (const auto iter : TObjectRange<UStaticMeshComponent>())
        {
            if (UStaticMeshComponent* pStaticMeshComp = Cast<UStaticMeshComponent>(iter))
            {
                if (!Cast<UGizmoBaseComponent>(iter))
                {
                    UStaticMesh* StaticMesh = pStaticMeshComp->GetStaticMesh();

                    //배치 스태틱 메쉬 안만들었으면 만든다.
                    if (StaticMeshBatch.Contains(StaticMesh) == false)
                    {
                        StaticMeshInstanceData Data;

                        uint32 StaticMeshCount = FManagerOBJ::GetStaticMeshCount(StaticMesh);

                        CreateVertexBufferForManualInstancing(StaticMesh->GetRenderData()->Vertices, StaticMeshCount, Data);
                        CreateIndexBufferForManualInstancing(StaticMesh->GetRenderData()->Indices, StaticMesh->GetRenderData()->Vertices.Num(), StaticMeshCount, Data);

                        Data.TotalInstancesInBuffer = StaticMeshCount;

                        StaticMeshBatch.Add(StaticMesh, Data);
                    }
                    StaticMeshObjs.Add(pStaticMeshComp);
                }


            }
        }



        //메쉬 기준으로 Sorting
        StaticMeshObjs.Sort([](const UStaticMeshComponent* A_ptr_ref, const UStaticMeshComponent* B_ptr_ref) {
            // 참조에서 실제 포인터 값을 가져옵니다.
            const UStaticMeshComponent* CompA = A_ptr_ref;
            const UStaticMeshComponent* CompB = B_ptr_ref;

            // 1. 컴포넌트 포인터 자체의 null 검사
            if (!CompA && !CompB) return false; // 둘 다 null이면 순서 유지 (동등)
            if (!CompA) return true;  // A만 null이면 A가 B보다 먼저 와야 함 (true 반환)
            if (!CompB) return false; // B만 null이면 A는 B보다 나중에 와야 함 (false 반환)


            const TArray<FStaticMaterial*>& MaterialsA = CompA->GetStaticMesh()->GetMaterials();
            const TArray<FStaticMaterial*>& MaterialsB = CompB->GetStaticMesh()->GetMaterials();

            // 3. 메테리얼 정보 포인터 null 검사
            if (!MaterialsA[0] && !MaterialsB[0]) return false; // 둘 다 null이면 순서 유지
            if (!MaterialsA[0]) return true;  // A만 null이면 A 우선
            if (!MaterialsB[0]) return false; // B만 null이면 B 우선 (즉, A는 나중에)

            // 4. 포인터 주소값 비교 (핵심)
            // MatInfoA가 MatInfoB보다 "작으면" (메모리 주소가 앞서면) true 반환
            return MaterialsA[0]->Material < MaterialsB[0]->Material;

            });



        //혹시 모르는 여유분 상수로
        CreateStructuredBuffer(&StructBuffer, &pStructBufferSRV, StaticMeshObjs.Num() + 5000, sizeof(FInstanceData));

        bIsDirtyRenderObj = false;
    }
}

void FRenderer::ClearRenderArr()
{
    //StaticMeshObjs.Empty();
    //GizmoObjs.Empty();
    //BillboardObjs.Empty();
    //LightObjs.Empty();
}

void FRenderer::InitOnceState(std::shared_ptr<FEditorViewportClient> ActiveViewport)
{
    CurrentViewport = ActiveViewport;
    Graphics->DeviceContext->RSSetViewports(1, &ActiveViewport->GetD3DViewport());
    Graphics->ChangeRasterizer(ActiveViewport->GetViewMode());
    Graphics->PrepareOnce();
}

void FRenderer::Render(UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport)
{
    // ChangeViewMode(ActiveViewport->GetViewMode());

    UPrimitiveBatch::GetInstance().RenderBatch(ActiveViewport->GetViewMatrix(), ActiveViewport->GetProjectionMatrix());

    if (ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_Primitives))
    {
        RenderStaticMeshes(World, ActiveViewport);
    }
    // ClearRenderArr();
}

void FRenderer::RenderStaticMeshes(UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport)
{
    PrepareShader(); 
    Plane frustumPlanes[6];
    memcpy(frustumPlanes, ActiveViewport->frustumPlanes, sizeof(Plane) * 6);
    //ActiveViewport->GetVisibleStaticMesh(StaticMeshObjs);
    //ActiveViewport->ExtractFrustumPlanesDirect(frustumPlanes);

    MaterialSorting();


    TArray<FFrameBatchInfo> FrameBatches;         // 이 프레임에 그릴 배치 정보 목록
    TArray<FMatrix> AllFrameMatrices;
    AllFrameMatrices.Reserve(StaticMeshObjs.Num()); // 예상 크기 예약

    //UINT currentStructuredBufferOffset = 0; // 공유 버퍼 내 현재 오프셋
    //CurrentStaticMesh = StaticMeshObjs[0]->GetStaticMesh(); // 첫번째 메쉬로 초기화
    UINT currentBatchInstanceCount = 0;
    UINT currentBatchStartOffset = 0; // 현재 배치가 AllFrameMatrices에서 시작될 오프셋

    //UStaticMesh* currentMeshType = nullptr;

    for (UStaticMeshComponent* StaticMeshComp : StaticMeshObjs)
    {
        if (!StaticMeshComp) continue;

        UStaticMesh* meshType = StaticMeshComp->GetStaticMesh();

        //만일 메쉬가 동일하지 않다면 그동안 쌓인 메쉬로 렌더링
        if (CurrentStaticMesh != meshType)
        {
            FrameBatches.Add({ CurrentStaticMesh, currentBatchInstanceCount, currentBatchStartOffset });

            currentBatchInstanceCount = 0;
            currentBatchStartOffset = (UINT)AllFrameMatrices.Num();
            //StaticMeshInstanceData& currentMeshData = StaticMeshBatch[CurrentStaticMesh];

            //// Structured Buffer 업데이트
            //D3D11_MAPPED_SUBRESOURCE mappedResource;
            //HRESULT hr = Graphics->DeviceContext->Map(StructBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

            //if (SUCCEEDED(hr))
            //{
            //    // 대상 버퍼의 시작 위치 계산
            //    FInstanceData* pDestStart = reinterpret_cast<FInstanceData*>(mappedResource.pData) + currentStructuredBufferOffset;

            //    // 루프를 돌면서 각 인스턴스 데이터의 WorldMatrix 멤버를 복사
            //    for (int i = 0; i < CurrentBatchMatrices.Num(); ++i)
            //    {
            //        // FMatrix union 내의 XMFLOAT4X4 멤버 이름이 xmFloat4x4라고 가정
            //        pDestStart[i].WorldMatrix = CurrentBatchMatrices[i];
            //    }

            //    Graphics->DeviceContext->Unmap(StructBuffer, 0);
            //}

            //// 2. 오프셋 상수 버퍼 업데이트
            //FOffsetConstantData offsetData;
            //offsetData.BaseInstanceOffset = currentStructuredBufferOffset;
            //UpdateOffsetConstantBuffer(&offsetData, sizeof(offsetData)); // g_pOffsetConstantBuffer 업데이트

            //// 메테리얼 업데이트
            //FObjMaterialInfo& MaterialInfo = CurrentStaticMesh->GetMaterials()[0]->Material->GetMaterialInfo();

            //if (MaterialInfo.bHasTexture == true)
            //{
            //    std::shared_ptr<FTexture> texture = FEngineLoop::resourceMgr.GetTexture(MaterialInfo.DiffuseTexturePath);
            //    Graphics->DeviceContext->PSSetShaderResources(0, 1, &texture->TextureSRV);
            //    Graphics->DeviceContext->PSSetSamplers(0, 1, &texture->SamplerState);
            //}

            //// 1. 설정할 버퍼 포인터를 담는 배열 생성
            //ID3D11Buffer* pVertexBuffers[] = { currentMeshData.VertexBufferInstance };

            //UINT StrideInstance = sizeof(FVertexSimpleIndex); // Stride 계산 필요
            //UINT offset = 0;

            //Graphics->DeviceContext->IASetVertexBuffers(0, 1, pVertexBuffers, &StrideInstance, &offset);
            //Graphics->DeviceContext->IASetIndexBuffer(currentMeshData.IndexBufferInstance, DXGI_FORMAT_R32_UINT, 0);
            //Graphics->DeviceContext->IASetInputLayout(InputLayout); // 인스턴싱 레이아웃 설정

            //UINT indexCountToDraw = currentMeshData.NumIndicesPerInstance * (UINT)CurrentBatchMatrices.Num();
            //Graphics->DeviceContext->DrawIndexed(
            //    indexCountToDraw,  // 이번 배치에서 그릴 총 인덱스 수
            //    0,                 // 시작 인덱스 위치 (거대 IBO 사용 시 보통 0)
            //    0                  // 기본 정점 위치 (거대 VBO 사용 시 보통 0)
            //);
            //// 6. 다음 배치를 위한 오프셋 업데이트
            //currentStructuredBufferOffset += (UINT)CurrentBatchMatrices.Num();

            //// 7. 현재 배치 클리어
            //CurrentBatchMatrices.Empty();

        }

        FMatrix Model = JungleMath::CreateModelMatrix(
            StaticMeshComp->GetWorldLocation(),
            StaticMeshComp->GetWorldRotation(),
            StaticMeshComp->GetWorldScale()
        );

        bool bFrustum = StaticMeshComp->GetBoundingBox().TransformWorld(Model).IsIntersectingFrustum(frustumPlanes);
        if (!bFrustum) continue;
         
        // 최종 MVP 행렬
        FMatrix MVP = Model * ActiveViewport->GetViewMatrix() * ActiveViewport->GetProjectionMatrix();
        //bool ac = ActiveViewport->IsAABBVisible(frustumPlanes, StaticMeshComp->GetBoundingBox().TransformWorld(MVP));
        //if (!ac) continue;

        AllFrameMatrices.Add(MVP);

        CurrentStaticMesh = StaticMeshComp->GetStaticMesh();
        currentBatchInstanceCount++; // 현재 배치 카운트 증가

       // FVector objectLocation = StaticMeshComp->GetWorldLocation();
       // FVector dir = (objectLocation - cameraLocation);

       // // 거리 계산 (벡터 차의 길이)
       // float dist = objectLocation.Distance(cameraLocation);
       // FVector DirNorm = dir.Normalize();
       // float dotVal = DirNorm.Dot(CameraForward);

       // if (dotVal < 0) continue;
       // if (dist > cullDistance) continue;





       // FVector4 UUIDColor = StaticMeshComp->EncodeUUID() / 255.0f;
       // if (World->GetSelectedComp() == StaticMeshComp)
       // {
       //     UpdateConstant(MVP, UUIDColor, true);
       //     UPrimitiveBatch::GetInstance().RenderAABB(
       //         StaticMeshComp->GetBoundingBox(),
       //         StaticMeshComp->GetWorldLocation(),
       //         Model
       //     );
       // }
       // else
       //     UpdateConstant(MVP, UUIDColor, false);

       //// UpdateTextureConstant(0, 0);


       // if (!StaticMeshComp->GetStaticMesh()) continue;

       // OBJ::FStaticMeshRenderData* renderData = StaticMeshComp->GetStaticMesh()->GetRenderData();
       // if (renderData == nullptr) return;

       // RenderPrimitive(renderData, StaticMeshComp->GetStaticMesh()->GetMaterials(), StaticMeshComp->GetOverrideMaterials(), StaticMeshComp->GetselectedSubMeshIndex());
    }

    if (!AllFrameMatrices.IsEmpty())
    {

        FrameBatches.Add({ CurrentStaticMesh, currentBatchInstanceCount, currentBatchStartOffset });

        //StaticMeshInstanceData& currentMeshData = StaticMeshBatch[CurrentStaticMesh];

        //// Structured Buffer 업데이트
        //D3D11_MAPPED_SUBRESOURCE mappedResource;
        //HRESULT hr = Graphics->DeviceContext->Map(StructBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

        //if (SUCCEEDED(hr))
        //{
        //    FInstanceData* pDest = reinterpret_cast<FInstanceData*>(mappedResource.pData) + currentStructuredBufferOffset;
        //    memcpy(pDest, CurrentBatchMatrices.GetData(), CurrentBatchMatrices.Num() * sizeof(DirectX::XMFLOAT4X4));
        //    Graphics->DeviceContext->Unmap(StructBuffer, 0);
        //}

        //// 2. 오프셋 상수 버퍼 업데이트
        //FOffsetConstantData offsetData;
        //offsetData.BaseInstanceOffset = currentStructuredBufferOffset;
        //UpdateOffsetConstantBuffer(&offsetData, sizeof(offsetData)); // g_pOffsetConstantBuffer 업데이트

        //// 메테리얼 업데이트
        //FObjMaterialInfo& MaterialInfo = CurrentStaticMesh->GetMaterials()[0]->Material->GetMaterialInfo();

        //if (MaterialInfo.bHasTexture == true)
        //{
        //    std::shared_ptr<FTexture> texture = FEngineLoop::resourceMgr.GetTexture(MaterialInfo.DiffuseTexturePath);
        //    Graphics->DeviceContext->PSSetShaderResources(0, 1, &texture->TextureSRV);
        //    Graphics->DeviceContext->PSSetSamplers(0, 1, &texture->SamplerState);
        //}

        //// 1. 설정할 버퍼 포인터를 담는 배열 생성
        //ID3D11Buffer* pVertexBuffers[] = { currentMeshData.VertexBufferInstance };

        //UINT StrideInstance = sizeof(FVertexSimpleIndex); // Stride 계산 필요
        //UINT offset = 0;

        //Graphics->DeviceContext->IASetVertexBuffers(0, 1, pVertexBuffers, &StrideInstance, &offset);
        //Graphics->DeviceContext->IASetIndexBuffer(currentMeshData.IndexBufferInstance, DXGI_FORMAT_R32_UINT, 0);
        //Graphics->DeviceContext->IASetInputLayout(InputLayout); // 인스턴싱 레이아웃 설정

        //UINT indexCountToDraw = currentMeshData.NumIndicesPerInstance * (UINT)CurrentBatchMatrices.Num();
        //Graphics->DeviceContext->DrawIndexed(
        //    indexCountToDraw,  // 이번 배치에서 그릴 총 인덱스 수
        //    0,                 // 시작 인덱스 위치 (거대 IBO 사용 시 보통 0)
        //    0                  // 기본 정점 위치 (거대 VBO 사용 시 보통 0)
        //);
        //// 6. 다음 배치를 위한 오프셋 업데이트
        //currentStructuredBufferOffset += (UINT)CurrentBatchMatrices.Num();
    }



    // ==============================================================
 // 2단계: 렌더링
 // ==============================================================

    if (!AllFrameMatrices.IsEmpty())
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = Graphics->DeviceContext->Map(StructBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr))
        {
            // 대상 버퍼의 시작 위치 계산
            FInstanceData* pDestStart = reinterpret_cast<FInstanceData*>(mappedResource.pData);

            // 루프를 돌면서 각 인스턴스 데이터의 WorldMatrix 멤버를 복사
            for (int i = 0; i < AllFrameMatrices.Num(); ++i)
            {
                // FMatrix union 내의 XMFLOAT4X4 멤버 이름이 xmFloat4x4라고 가정
                pDestStart[i].WorldMatrix = AllFrameMatrices[i];
            }
            Graphics->DeviceContext->Unmap(StructBuffer, 0);
        }

        else { UE_LOG(LogLevel::Error, TEXT("Failed to map Structured Buffer for frame!")); }
    }

    // --- 배치 정보 목록을 순회하며 그리기 ---
    for (const FFrameBatchInfo& batchInfo : FrameBatches)
    {
        if (batchInfo.InstanceCount == 0) continue; // 혹시 모를 빈 배치 건너뛰기

        StaticMeshInstanceData* meshData = StaticMeshBatch.Find(batchInfo.MeshType);
        if (!meshData || !meshData->VertexBufferInstance) continue; // VBO/IBO 없으면 그릴 수 없음

        // 1. 오프셋 상수 버퍼 업데이트
        FOffsetConstantData offsetData;
        offsetData.BaseInstanceOffset = batchInfo.StructuredBufferOffset; // 수집 단계에서 계산된 오프셋 사용
        UpdateOffsetConstantBuffer(&offsetData, sizeof(offsetData));

        // 메테리얼 업데이트
        FObjMaterialInfo& MaterialInfo = batchInfo.MeshType->GetMaterials()[0]->Material->GetMaterialInfo();

        if (MaterialInfo.bHasTexture == true)
        {
            std::shared_ptr<FTexture> texture = FEngineLoop::resourceMgr.GetTexture(MaterialInfo.DiffuseTexturePath);
            Graphics->DeviceContext->PSSetShaderResources(0, 1, &texture->TextureSRV);
            Graphics->DeviceContext->PSSetSamplers(0, 1, &texture->SamplerState);
        }

        // 3. Input Assembler 설정 (VBO, IBO, Layout, Topology)
        ID3D11Buffer* pVertexBuffers[] = { meshData->VertexBufferInstance };
        UINT stride = sizeof(FVertexSimpleIndex);
        UINT offset = 0;
        Graphics->DeviceContext->IASetVertexBuffers(0, 1, pVertexBuffers, &stride, &offset);
        Graphics->DeviceContext->IASetIndexBuffer(meshData->IndexBufferInstance, DXGI_FORMAT_R32_UINT, 0);

        // 4. DrawIndexed 호출
        UINT indexCountToDraw = meshData->NumIndicesPerInstance * batchInfo.InstanceCount;
        Graphics->DeviceContext->DrawIndexed(indexCountToDraw, 0, 0);
    }
}

void FRenderer::RenderGizmos(const UWorld* World, const std::shared_ptr<FEditorViewportClient>& ActiveViewport)
{
    if (!World->GetSelectedActor())
    {
        return;
    }

#pragma region GizmoDepth
    ID3D11DepthStencilState* DepthStateDisable = Graphics->DepthStateDisable;
    Graphics->DeviceContext->OMSetDepthStencilState(DepthStateDisable, 0);
#pragma endregion GizmoDepth

    //  fill solid,  Wirframe 에서도 제대로 렌더링되기 위함
    Graphics->DeviceContext->RSSetState(FEngineLoop::graphicDevice.RasterizerStateSOLID);

    for (auto GizmoComp : GizmoObjs)
    {

        if ((GizmoComp->GetGizmoType() == UGizmoBaseComponent::ArrowX ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::ArrowY ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::ArrowZ)
            && World->GetEditorPlayer()->GetControlMode() != CM_TRANSLATION)
            continue;
        else if ((GizmoComp->GetGizmoType() == UGizmoBaseComponent::ScaleX ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::ScaleY ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::ScaleZ)
            && World->GetEditorPlayer()->GetControlMode() != CM_SCALE)
            continue;
        else if ((GizmoComp->GetGizmoType() == UGizmoBaseComponent::CircleX ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::CircleY ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::CircleZ)
            && World->GetEditorPlayer()->GetControlMode() != CM_ROTATION)
            continue;
        FMatrix Model = JungleMath::CreateModelMatrix(GizmoComp->GetWorldLocation(),
            GizmoComp->GetWorldRotation(),
            GizmoComp->GetWorldScale()
        );
        FVector4 UUIDColor = GizmoComp->EncodeUUID() / 255.0f;

        FMatrix MVP = Model * ActiveViewport->GetViewMatrix() * ActiveViewport->GetProjectionMatrix();

        if (GizmoComp == World->GetPickingGizmo())
            UpdateConstant(MVP, UUIDColor, true);
        else
            UpdateConstant(MVP, UUIDColor, false);

        if (!GizmoComp->GetStaticMesh()) continue;

        OBJ::FStaticMeshRenderData* renderData = GizmoComp->GetStaticMesh()->GetRenderData();
        if (renderData == nullptr) continue;

        RenderPrimitive(renderData, GizmoComp->GetStaticMesh()->GetMaterials(), GizmoComp->GetOverrideMaterials(), 0);
    }

    Graphics->DeviceContext->RSSetState(Graphics->GetCurrentRasterizer());

#pragma region GizmoDepth
    ID3D11DepthStencilState* originalDepthState = Graphics->DepthStencilState;
    Graphics->DeviceContext->OMSetDepthStencilState(originalDepthState, 0);
#pragma endregion GizmoDepth
}

void FRenderer::RenderBillboards(UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport)
{
    PrepareTextureShader();
    PrepareSubUVConstant();
    for (auto BillboardComp : BillboardObjs)
    {
        UpdateSubUVConstant(BillboardComp->finalIndexU, BillboardComp->finalIndexV);

        FMatrix Model = BillboardComp->CreateBillboardMatrix();

        // 최종 MVP 행렬
        FMatrix MVP = Model * ActiveViewport->GetViewMatrix() * ActiveViewport->GetProjectionMatrix();
        FVector4 UUIDColor = BillboardComp->EncodeUUID() / 255.0f;
        if (BillboardComp == World->GetPickingGizmo())
            UpdateConstant(MVP, UUIDColor, true);
        else
            UpdateConstant(MVP, UUIDColor, false);

        if (UParticleSubUVComp* SubUVParticle = Cast<UParticleSubUVComp>(BillboardComp))
        {
            RenderTexturePrimitive(
                SubUVParticle->vertexSubUVBuffer, SubUVParticle->numTextVertices,
                SubUVParticle->indexTextureBuffer, SubUVParticle->numIndices, SubUVParticle->Texture->TextureSRV, SubUVParticle->Texture->SamplerState
            );
        }
        else if (UText* Text = Cast<UText>(BillboardComp))
        {
            FEngineLoop::renderer.RenderTextPrimitive(
                Text->vertexTextBuffer, Text->numTextVertices,
                Text->Texture->TextureSRV, Text->Texture->SamplerState
            );
        }
        else
        {
            RenderTexturePrimitive(
                BillboardComp->vertexTextureBuffer, BillboardComp->numVertices,
                BillboardComp->indexTextureBuffer, BillboardComp->numIndices, BillboardComp->Texture->TextureSRV, BillboardComp->Texture->SamplerState
            );
        }
    }
    PrepareShader();
}

void FRenderer::MaterialSorting()
{
    //임시로 오브젝트의 수가 변경했을 때만 정렬
    if (StaticMeshObjs.Num() == PrevStaticMeshObjsNum) return;
    PrevStaticMeshObjsNum = StaticMeshObjs.Num();

    StaticMeshObjs.Sort([](const UStaticMeshComponent* A_ptr_ref, const UStaticMeshComponent* B_ptr_ref) {
        // 참조에서 실제 포인터 값을 가져옵니다.
        const UStaticMeshComponent* CompA = A_ptr_ref;
        const UStaticMeshComponent* CompB = B_ptr_ref;

        // 1. 컴포넌트 포인터 자체의 null 검사
        if (!CompA && !CompB) return false; // 둘 다 null이면 순서 유지 (동등)
        if (!CompA) return true;  // A만 null이면 A가 B보다 먼저 와야 함 (true 반환)
        if (!CompB) return false; // B만 null이면 A는 B보다 나중에 와야 함 (false 반환)


        UStaticMesh* StaticMeshA = CompA->GetStaticMesh();
        UStaticMesh* StaticMeshB = CompB->GetStaticMesh();
        

        //// 3. 메테리얼 정보 포인터 null 검사
        //if (!MaterialsA[0] && !MaterialsB[0]) return false; // 둘 다 null이면 순서 유지
        //if (!MaterialsA[0]) return true;  // A만 null이면 A 우선
        //if (!MaterialsB[0]) return false; // B만 null이면 B 우선 (즉, A는 나중에)

        //// 4. 포인터 주소값 비교 (핵심)
        //// MatInfoA가 MatInfoB보다 "작으면" (메모리 주소가 앞서면) true 반환
        return StaticMeshA < StaticMeshB;

        });
    //std::sort(Materials.begin(), Materials.end(), [](const FMaterialInfo& a, const FMaterialInfo& b) {
    //    return a.MaterialIndex < b.MaterialIndex;
    //    });
}

void FRenderer::RenderLight(UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport)
{
    for (auto Light : LightObjs)
    {
        FMatrix Model = JungleMath::CreateModelMatrix(Light->GetWorldLocation(), Light->GetWorldRotation(), { 1, 1, 1 });
        UPrimitiveBatch::GetInstance().AddCone(Light->GetWorldLocation(), Light->GetRadius(), 15, 140, Light->GetColor(), Model);
        UPrimitiveBatch::GetInstance().RenderOBB(Light->GetBoundingBox(), Light->GetWorldLocation(), Model);
    }
}
