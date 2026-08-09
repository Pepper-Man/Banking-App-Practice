#include "GuiManager.h"
#include <d3d11.h>
#include "d3dcommon.h"
#include "dxgi.h"
#include "dxgiformat.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "Windows.h"

#pragma comment(lib, "d3d11.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static HWND                     g_hWnd = NULL;
static WNDCLASSEXW              g_wc = {};
static bool                     g_running = true;

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    if (msg == WM_DESTROY) { PostQuitMessage(0); g_running = false; return 0; }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

namespace GuiManager {
    bool Init(const char* windowName, int width, int height) {
        g_wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGuiAppClass", NULL };
        RegisterClassExW(&g_wc);
        g_hWnd = CreateWindowW(g_wc.lpszClassName, L"Bank System", WS_OVERLAPPEDWINDOW, 100, 100, width, height, NULL, NULL, g_wc.hInstance, NULL);

        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = g_hWnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, NULL, &g_pd3dDeviceContext) != S_OK)
            return false;

        ID3D11Texture2D* pBackBuffer;
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
        pBackBuffer->Release();

        ShowWindow(g_hWnd, SW_SHOWDEFAULT);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // Nicer vector font
        ImGuiIO& io = ImGui::GetIO();
        ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consolab.ttf", 16.0f);
        if (font == nullptr) io.Fonts->AddFontDefault();

        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(g_hWnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
        return true;
    }

    bool IsRunning() {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) g_running = false;
        }
        return g_running;
    }

    void BeginFrame() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void EndFrame() {
        ImGui::Render();
        const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    void Shutdown() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (g_mainRenderTargetView) g_mainRenderTargetView->Release();
        if (g_pSwapChain) g_pSwapChain->Release();
        if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release();
        if (g_pd3dDevice) g_pd3dDevice->Release();
        DestroyWindow(g_hWnd);
        UnregisterClassW(g_wc.lpszClassName, g_wc.hInstance);
    }
}