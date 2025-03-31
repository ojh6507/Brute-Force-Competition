#pragma once
#include "SLevelEditor.h"
#include "SlateCore/Widgets/SWindow.h"
#include "Slate/Widgets/Layout/SSplitter.h"
#include "UnrealClient.h"
#include "UnrealEd/EditorViewportClient.h"
#include "EngineLoop.h"
#include "fstream"
#include "sstream"
#include "ostream"
extern FEngineLoop GEngineLoop;

SLevelEditor::SLevelEditor() : bInitialize(false), HSplitter(nullptr), VSplitter(nullptr),
World(nullptr), bMultiViewportMode(false)
{
}

SLevelEditor::~SLevelEditor()
{
}

void SLevelEditor::Initialize()
{
    for (size_t i = 0; i < 1; i++)
    {
        viewportClients[i] = std::make_shared<FEditorViewportClient>();
        viewportClients[i]->Initialize(i);
    }
    ActiveViewportClient = viewportClients[0];
    OnResize();
    LoadConfig();
    bInitialize = true;
}

void SLevelEditor::Tick(double deltaTime)
{
    //Test Code Cursor icon End
    OnResize();

    ActiveViewportClient->Tick(deltaTime);
}

void SLevelEditor::Input()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
    {
        if (bLButtonDown == false)
        {
            bLButtonDown = true;
            POINT pt;
            GetCursorPos(&pt);
            GetCursorPos(&lastMousePos);
            ScreenToClient(GEngineLoop.hWnd, &pt);

            SelectViewport(pt);
    }
        else
        {
            POINT currentMousePos;
            GetCursorPos(&currentMousePos);

            // 마우스 이동 차이 계산
            int32 deltaX = currentMousePos.x - lastMousePos.x;
            int32 deltaY = currentMousePos.y - lastMousePos.y;

       
            ResizeViewports();
            lastMousePos = currentMousePos;
        }
    }
    else
    {
        bLButtonDown = false;
    }
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        if (!bRButtonDown)
        {
            bRButtonDown = true;
            POINT pt;
            GetCursorPos(&pt);
            GetCursorPos(&lastMousePos);
            ScreenToClient(GEngineLoop.hWnd, &pt);

            SelectViewport(pt);
        }
    }
    else
    {
        bRButtonDown = false;
    }
}

void SLevelEditor::Release()
{
    SaveConfig();
}

void SLevelEditor::SelectViewport(POINT point)
{
    for (int i = 0; i < 1; i++)
    {
        if (viewportClients[i]->IsSelected(point))
        {
            SetViewportClient(i);
            break;
        }
    }
}

void SLevelEditor::OnResize()
{
    float PrevWidth = EditorWidth;
    float PrevHeight = EditorHeight;
    EditorWidth = GEngineLoop.graphicDevice.screenWidth;
    EditorHeight = GEngineLoop.graphicDevice.screenHeight;
    if (bInitialize) {
        ResizeViewports();
    }
}

void SLevelEditor::ResizeViewports()
{
    // if (bMultiViewportMode) {
    //     if (GetViewports()[0]) {
    //         for (int i = 0;i < 4;++i)
    //         {
    //             GetViewports()[i]->ResizeViewport(VSplitter->SideLT->Rect, VSplitter->SideRB->Rect,
    //                 HSplitter->SideLT->Rect, HSplitter->SideRB->Rect);
    //         }
    //     }
    // }
    // else
    // {
        ActiveViewportClient->GetViewport()->ResizeViewport(FRect(0.0f, 0.0f, EditorWidth, EditorHeight));
    // }
}

void SLevelEditor::OnMultiViewport()
{
    bMultiViewportMode = true;
    ResizeViewports();
}

void SLevelEditor::OffMultiViewport()
{
    bMultiViewportMode = false;
}

bool SLevelEditor::IsMultiViewport()
{
    return bMultiViewportMode;
}

void SLevelEditor::LoadConfig()
{
    auto config = ReadIniFile(IniFilePath);
    ActiveViewportClient->Pivot.x = GetValueFromConfig(config, "OrthoPivotX", 0.0f);
    ActiveViewportClient->Pivot.y = GetValueFromConfig(config, "OrthoPivotY", 0.0f);
    ActiveViewportClient->Pivot.z = GetValueFromConfig(config, "OrthoPivotZ", 0.0f);
    ActiveViewportClient->orthoSize = GetValueFromConfig(config, "OrthoZoomSize", 10.0f);

    SetViewportClient(GetValueFromConfig(config, "ActiveViewportIndex", 0));
    bMultiViewportMode = GetValueFromConfig(config, "bMutiView", false);
    for (size_t i = 0; i < 1; i++)
    {
        viewportClients[i]->LoadConfig(config);
    }
  
}

void SLevelEditor::SaveConfig()
{
    TMap<FString, FString> config;
 
    for (size_t i = 0; i < 1; i++)
    {
        viewportClients[i]->SaveConfig(config);
    }
    ActiveViewportClient->SaveConfig(config);
    config["bMutiView"] = std::to_string(bMultiViewportMode);
    config["ActiveViewportIndex"] = std::to_string(ActiveViewportClient->ViewportIndex);
    config["ScreenWidth"] = std::to_string(ActiveViewportClient->ViewportIndex);
    config["ScreenHeight"] = std::to_string(ActiveViewportClient->ViewportIndex);
    config["OrthoPivotX"] = std::to_string(ActiveViewportClient->Pivot.x);
    config["OrthoPivotY"] = std::to_string(ActiveViewportClient->Pivot.y);
    config["OrthoPivotZ"] = std::to_string(ActiveViewportClient->Pivot.z);
    config["OrthoZoomSize"] = std::to_string(ActiveViewportClient->orthoSize);
    WriteIniFile(IniFilePath, config);
}

TMap<FString, FString> SLevelEditor::ReadIniFile(const FString& filePath)
{
    TMap<FString, FString> config;
    std::ifstream file(*filePath);
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[' || line[0] == ';') continue;
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            config[key] = value;
        }
    }
    return config;
}

void SLevelEditor::WriteIniFile(const FString& filePath, const TMap<FString, FString>& config)
{
    std::ofstream file(*filePath);
    for (const auto& pair : config) {
        file << *pair.Key << "=" << *pair.Value << "\n";
    }
}

