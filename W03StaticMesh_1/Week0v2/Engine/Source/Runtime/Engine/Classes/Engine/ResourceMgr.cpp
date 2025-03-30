#include "ResourceMgr.h"
#include <fstream>
#include <sstream>
#include <wincodec.h>
#include <ranges>
#include "Define.h"
#include "Components/SkySphereComponent.h"
#include "D3D11RHI/GraphicDevice.h"
#include "DirectXTK/Include/DDSTextureLoader.h"
#include "ThirdParty/DirectXTK/Include/WICTextureLoader.h"
#include "Engine/FLoaderOBJ.h"

void FResourceMgr::Initialize(FRenderer* renderer, FGraphicsDevice* device)
{
    //RegisterMesh(renderer, "Quad", quadVertices, sizeof(quadVertices) / sizeof(FVertexSimple), quadInices, sizeof(quadInices)/sizeof(uint32));

    //FManagerOBJ::LoadObjStaticMeshAsset("Assets/AxisArrowX.obj");
    //FManagerOBJ::LoadObjStaticMeshAsset("Assets/AxisArrowY.obj");
    //FManagerOBJ::LoadObjStaticMeshAsset("Assets/AxisArrowZ.obj");
    //FManagerOBJ::LoadObjStaticMeshAsset("Assets/AxisScaleArrowX.obj");
    //FManagerOBJ::LoadObjStaticMeshAsset("Assets/AxisScaleArrowY.obj");
    //FManagerOBJ::LoadObjStaticMeshAsset("Assets/AxisScaleArrowZ.obj");
    //FManagerOBJ::LoadObjStaticMeshAsset("Assets//AxisCircleX.obj");
    //FManagerOBJ::LoadObjStaticMeshAsset("Assets//AxisCircleY.obj");
    //FManagerOBJ::LoadObjStaticMeshAsset("Assets//AxisCircleZ.obj");
    // FManagerOBJ::LoadObjStaticMeshAsset("Assets/helloBlender.obj");

	LoadTextureFromFile(device->Device, device->DeviceContext, L"Assets/Texture/ocean_sky.jpg");
	LoadTextureFromFile(device->Device, device->DeviceContext, L"Assets/Texture/font.png");
	LoadTextureFromDDS(device->Device, device->DeviceContext, L"Assets/Texture/font.dds");
	LoadTextureFromFile(device->Device, device->DeviceContext, L"Assets/Texture/emart.png");
	LoadTextureFromFile(device->Device, device->DeviceContext, L"Assets/Texture/T_Explosion_SubUV.png");
	LoadTextureFromFile(device->Device, device->DeviceContext, L"Assets/Texture/UUID_Font.png");
	LoadTextureFromDDS(device->Device, device->DeviceContext, L"Assets/Texture/UUID_Font.dds");
	LoadTextureFromFile(device->Device, device->DeviceContext, L"Assets/Texture/Wooden Crate_Crate_BaseColor.png");
	LoadTextureFromFile(device->Device, device->DeviceContext, L"Assets/Texture/spotLight.png");
}

void FResourceMgr::Release(FRenderer* renderer) {
	for (const auto& Pair : textureMap)
    {
		FTexture* texture =	Pair.Value.get();
		texture->Release();
	}
    textureMap.Empty();
}

#include <unordered_map>

struct PairHash {
	template <typename T1, typename T2>
	std::size_t operator()(const std::pair<T1, T2>& pair) const {
		return std::hash<T1>()(pair.first) ^ (std::hash<T2>()(pair.second) << 1);
	}
};
struct TupleHash {
	template <typename T1, typename T2, typename T3>
	std::size_t operator()(const std::tuple<T1, T2, T3>& tuple) const {
		std::size_t h1 = std::hash<T1>()(std::get<0>(tuple));
		std::size_t h2 = std::hash<T2>()(std::get<1>(tuple));
		std::size_t h3 = std::hash<T3>()(std::get<2>(tuple));

		return h1 ^ (h2 << 1) ^ (h3 << 2);  // 해시 값 섞기
	}
};

std::shared_ptr<FTexture> FResourceMgr::GetTexture(const FWString& name) const
{
    auto* TempValue = textureMap.Find(name);
    return TempValue ? *TempValue : nullptr;
}

HRESULT FResourceMgr::LoadTextureFromFile(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename)
{
	IWICImagingFactory* wicFactory = nullptr;
	IWICBitmapDecoder* decoder = nullptr;
	IWICBitmapFrameDecode* frame = nullptr;
	IWICFormatConverter* converter = nullptr;


	// WIC 팩토리 생성
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr)) return hr;

	hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
	if (FAILED(hr)) return hr;


	// 이미지 파일 디코딩
	hr = wicFactory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
	if (FAILED(hr)) return hr;




	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr)) return hr;

	// WIC 포맷 변환기 생성 (픽셀 포맷 변환)
	hr = wicFactory->CreateFormatConverter(&converter);
	if (FAILED(hr)) return hr;

	hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
	if (FAILED(hr)) return hr;

	// 이미지 크기 가져오기
	UINT width, height;
	frame->GetSize(&width, &height);
	
	// 픽셀 데이터 로드
    UINT rowPitch = width * 4;
    UINT imageSize = rowPitch * height;
	BYTE* imageData = new BYTE[width * height * 4];
	hr = converter->CopyPixels(nullptr, width * 4, width * height * 4, imageData);
	if (FAILED(hr)) {
		delete[] imageData;
		return hr;
	}

	// DirectX 11 텍스처 생성
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 0;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // <<< 수정: GenerateMips 플래그 추가

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = imageData;
	initData.SysMemPitch = width * 4;
	ID3D11Texture2D* Texture2D;
	hr = device->CreateTexture2D(&textureDesc, nullptr, &Texture2D);

    // 밉 레벨 0에 데이터 업로드 (Device Context 사용)
    context->UpdateSubresource(Texture2D, 0, nullptr, imageData, rowPitch, 0);

    // imageData는 GPU로 복사되었으므로 해제 가능
    delete[] imageData;
    imageData = nullptr; // 해제 후 포인터 초기화

	//// Shader Resource View 생성
	//D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//srvDesc.Format = textureDesc.Format;
	//srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//srvDesc.Texture2D.MostDetailedMip = 0;
	//srvDesc.Texture2D.MipLevels = 1;
	ID3D11ShaderResourceView* TextureSRV;
	hr = device->CreateShaderResourceView(Texture2D, nullptr, &TextureSRV);

    // <<< 추가: GenerateMips 호출 (Device Context 사용) >>>
    context->GenerateMips(TextureSRV);

	// 리소스 해제
	wicFactory->Release();
	decoder->Release();
	frame->Release();
	converter->Release();

	//샘플러 스테이트 생성
	ID3D11SamplerState* SamplerState;
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    samplerDesc.MipLODBias = 1.0f; // 예시: LOD 계산 결과에 1.0을 더함

	device->CreateSamplerState(&samplerDesc, &SamplerState);
	FWString name = FWString(filename);

	textureMap[name] = std::make_shared<FTexture>(TextureSRV, Texture2D, SamplerState, width, height);

	Console::GetInstance().AddLog(LogLevel::Warning, "Texture File Load Successs");
	return hr;
}

HRESULT FResourceMgr::LoadTextureFromDDS(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename)
{

	ID3D11Resource* texture = nullptr;
	ID3D11ShaderResourceView* textureView = nullptr;

	HRESULT hr = DirectX::CreateDDSTextureFromFile(
		device, context,
		filename,
		&texture,
		&textureView
	);
	if (FAILED(hr) || texture == nullptr) abort();

#pragma region WidthHeight

	ID3D11Texture2D* texture2D = nullptr;
	hr = texture->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&texture2D);
	if (FAILED(hr) || texture2D == nullptr) {
		std::wcerr << L"Failed to query ID3D11Texture2D interface!" << std::endl;
		texture->Release();
		abort();
		return hr;
	}

	// 🔹 텍스처 크기 가져오기
	D3D11_TEXTURE2D_DESC texDesc;
	texture2D->GetDesc(&texDesc);
	uint32 width = static_cast<uint32>(texDesc.Width);
	uint32 height = static_cast<uint32>(texDesc.Height);

#pragma endregion WidthHeight

#pragma region Sampler
	ID3D11SamplerState* SamplerState;
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&samplerDesc, &SamplerState);
#pragma endregion Sampler

	FWString name = FWString(filename);

	textureMap[name] = std::make_shared<FTexture>(textureView, texture2D, SamplerState, width, height);

	Console::GetInstance().AddLog(LogLevel::Warning, "Texture File Load Successs");

	return hr;
}
