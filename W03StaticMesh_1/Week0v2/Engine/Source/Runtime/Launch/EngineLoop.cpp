#include "EngineLoop.h"
#include "ImGuiManager.h"
#include "World.h"
#include "Camera/CameraComponent.h"
#include "PropertyEditor/ViewportTypePanel.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/UnrealEd.h"
#include "UnrealClient.h"
#include "slate/Widgets/Layout/SSplitter.h"
#include "LevelEditor/SLevelEditor.h"
#include "Actors/Player.h"
#include "WindowsPlatformTime.h"

std::atomic<double> FWindowsPlatformTime::GSecondsPerCycle{ 0.0 };
std::atomic<bool> FWindowsPlatformTime::bInitialized{ false };

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }
    int zDelta = 0;
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            //UGraphicsDevice 객체의 OnResize 함수 호출
            if (FEngineLoop::graphicDevice.SwapChain)
            {
                FEngineLoop::graphicDevice.OnResize(hWnd);
            }
            for (int i = 0; i < 1; i++)
            {
                if (GEngineLoop.GetLevelEditor())
                {
                    if (GEngineLoop.GetLevelEditor()->GetViewports()[i])
                    {
                        GEngineLoop.GetLevelEditor()->GetViewports()[i]->ResizeViewport(FEngineLoop::graphicDevice.SwapchainDesc);
                        GEngineLoop.GetLevelEditor()->GetViewports()[i]->UpdateCameraBuffer();
                    }
                }
            }
            if (GEngineLoop.LevelEditor)
            {
                GEngineLoop.renderer.InitOnceState(GEngineLoop.LevelEditor->GetActiveViewportClient());
                // GEngineLoop.renderer.RenderStaticMeshes(GEngineLoop.GetWorld() ,GEngineLoop.LevelEditor->GetActiveViewportClient());
            }
            
        }
        /*  Console::GetInstance().OnResize(hWnd);
        */ // ControlPanel::GetInstance().OnResize(hWnd);
        // PropertyPanel::GetInstance().OnResize(hWnd);
        // Outliner::GetInstance().OnResize(hWnd);
        // ViewModeDropdown::GetInstance().OnResize(hWnd);
        // ShowFlags::GetInstance().OnResize(hWnd);
        if (GEngineLoop.GetUnrealEditor())
        {
            GEngineLoop.GetUnrealEditor()->OnResize(hWnd);
        }
        ViewportTypePanel::GetInstance().OnResize(hWnd);
        break;
    case WM_MOUSEWHEEL:
        if (ImGui::GetIO().WantCaptureMouse)
            return 0;
        zDelta = GET_WHEEL_DELTA_WPARAM(wParam); // 휠 회전 값 (+120 / -120)
        if (GEngineLoop.GetLevelEditor())
        {
            if (GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->IsPerspective())
            {
                if (GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetIsOnRBMouseClick())
                {
                    GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->SetCameraSpeedScalar(
                        static_cast<float>(GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar() + zDelta * 0.01)
                    );
                }
                else
                {
                    GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->CameraMoveForward(zDelta * 0.1f);
                }
            }
            else
            {
                FEditorViewportClient::SetOthoSize(-zDelta * 0.01f);
            }
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

FGraphicsDevice FEngineLoop::graphicDevice;
FRenderer FEngineLoop::renderer;
FResourceMgr FEngineLoop::resourceMgr;
uint32 FEngineLoop::TotalAllocationBytes = 0;
uint32 FEngineLoop::TotalAllocationCount = 0;

FEngineLoop::FEngineLoop()
    : hWnd(nullptr)
    , UIMgr(nullptr)
    , GWorld(nullptr)
    , LevelEditor(nullptr)
    , UnrealEditor(nullptr)
{
}

int32 FEngineLoop::PreInit()
{
    return 0;
}

int32 FEngineLoop::Init(HINSTANCE hInstance)
{
    /* must be initialized before window. */
    UnrealEditor = new UnrealEd();
    UnrealEditor->Initialize();

    WindowInit(hInstance);
    graphicDevice.Initialize(hWnd);
    renderer.Initialize(&graphicDevice);

    UIMgr = new UImGuiManager;
    UIMgr->Initialize(hWnd, graphicDevice.Device, graphicDevice.DeviceContext);

    resourceMgr.Initialize(&renderer, &graphicDevice);
    LevelEditor = new SLevelEditor();
    LevelEditor->Initialize();

    GWorld = new UWorld;
    GWorld->Initialize();

    renderer.InitOnceState(LevelEditor->GetActiveViewportClient());

    return 0;
}


void FEngineLoop::Render()
{
    graphicDevice.Prepare();

    renderer.PrepareRender();
    renderer.Render(GetWorld(), LevelEditor->GetActiveViewportClient());

}

void FEngineLoop::Tick()
{

    FWindowsPlatformTime::InitTiming();

    uint64 StartTime, EndTime;

    double elapsedTime = 0.0;
    float fps = 0.0f;

    while (bIsExit == false)
    {
        StartTime = FWindowsPlatformTime::Cycles64();

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg); // 키보드 입력 메시지를 문자메시지로 변경
            DispatchMessage(&msg);  // 메시지를 WndProc에 전달

            if (msg.message == WM_QUIT)
            {
                bIsExit = true;
                break;
            }
        }

        Input();
        GWorld->Tick(elapsedTime / 1000.0);
        LevelEditor->Tick(elapsedTime / 1000.0);
        Render();
        UIMgr->BeginFrame();

        EndTime = FWindowsPlatformTime::Cycles64();
        elapsedTime = FWindowsPlatformTime::ToMilliseconds(EndTime - StartTime);
        if (elapsedTime > 0.0)
        {
            fps = static_cast<float>(1000.0 / elapsedTime);
        }
        else
        {
            fps = 0.0f;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 5));

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoInputs;

        ImGui::Begin("Overlay", nullptr, windowFlags);

        ImGui::SetWindowFontScale(1.5f);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f (%.1f ms)", fps, elapsedTime);

        // 두 UI 정보 사이에 간격 추가
        ImGui::Spacing();

        // Picking 관련 UI 정보 출력

        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Picking Time: %f ms : Num Attemps: %d : Accumlated Time: %f ms", 
            FWindowsPlatformTime::ToMilliseconds(GWorld->GetEditorPlayer()->LastPickTime),
            GWorld->GetEditorPlayer()->TotalPickCount, FWindowsPlatformTime::ToMilliseconds(GWorld->GetEditorPlayer()->TotalPickTime));
        ImGui::SetWindowFontScale(1.0f);

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        UIMgr->EndFrame();


        // Pending 처리된 오브젝트 제거
        GUObjectArray.ProcessPendingDestroyObjects();
        graphicDevice.SwapBuffer();

       

    }
}

float FEngineLoop::GetAspectRatio(IDXGISwapChain* swapChain) const
{
    DXGI_SWAP_CHAIN_DESC desc;
    swapChain->GetDesc(&desc);
    return static_cast<float>(desc.BufferDesc.Width) / static_cast<float>(desc.BufferDesc.Height);
}

void FEngineLoop::Input()
{
    if (GetAsyncKeyState('M') & 0x8000)
    {
        if (!bTestInput)
        {
            bTestInput = true;
            if (LevelEditor->IsMultiViewport())
            {
                LevelEditor->OffMultiViewport();
            }
            else
                LevelEditor->OnMultiViewport();
        }
    }
    else
    {
        bTestInput = false;
    }
}

void FEngineLoop::Exit()
{
    LevelEditor->Release();
    GWorld->Release();
    delete GWorld;
    UIMgr->Shutdown();
    delete UIMgr;
    resourceMgr.Release(&renderer);
    renderer.Release();
    graphicDevice.Release();
}


void FEngineLoop::WindowInit(HINSTANCE hInstance)
{
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    WCHAR WindowClass[] = L"JungleWindowClass";

    WCHAR Title[] = L"Game Tech Lab";

    WNDCLASSW wndclass = { 0 };
    wndclass.lpfnWndProc = WndProc;
    wndclass.hInstance = hInstance;
    wndclass.lpszClassName = WindowClass;

    RegisterClassW(&wndclass);

    hWnd = CreateWindowExW(
        0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 1000,
        nullptr, nullptr, hInstance, nullptr
    );
}
