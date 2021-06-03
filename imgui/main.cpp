// Dear ImGui: standalone example application for DirectX 9
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include <Windows.h>
#include <wrl.h>
#include <d3d9.h>
#include <DirectXMath.h>
#include <tchar.h>
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "WICTextureLoader9.h"
#include "VertexShader.h"
#include "PixelShader.h"

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

size_t idle() { return 0; }

struct TestData
{
    Microsoft::WRL::ComPtr<IDirect3DDevice9> device;
    Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> VB;
    Microsoft::WRL::ComPtr<IDirect3DIndexBuffer9> IB;
    Microsoft::WRL::ComPtr<IDirect3DVertexDeclaration9> InputLayout;
    Microsoft::WRL::ComPtr<IDirect3DVertexShader9> VS;
    Microsoft::WRL::ComPtr<IDirect3DPixelShader9> PS;
    Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;
    
    struct Vertex
    {
        float x, y, z;
        float u, v;
        uint32_t color, channel;
    };
    using Index = uint16_t;
    
    void load(IDirect3DDevice9* pDevice)
    {
        unload();
        device = pDevice;
        
        HRESULT hr = 0;
        
        // input layout
        
        const D3DVERTEXELEMENT9 decl_[] = {
            { 0, offsetof(Vertex, x)      , D3DDECLTYPE_FLOAT3  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
            { 0, offsetof(Vertex, u)      , D3DDECLTYPE_FLOAT2  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
            { 0, offsetof(Vertex, color)  , D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR   , 0 },
            { 0, offsetof(Vertex, channel), D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR   , 1 },
            D3DDECL_END(),
        };
        hr = device->CreateVertexDeclaration(decl_, InputLayout.GetAddressOf());
        
        // input buffer
        
        hr = device->CreateVertexBuffer(4 * sizeof(Vertex), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, VB.GetAddressOf(), NULL);
        hr = device->CreateIndexBuffer(6 * sizeof(Index), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, IB.GetAddressOf(), NULL);
        
        // shader
        
        hr = device->CreateVertexShader((DWORD*)g_vVertexShader, VS.GetAddressOf());
        hr = device->CreatePixelShader((DWORD*)g_vPixelShader, PS.GetAddressOf());
        
        // texture
        
        D3DCAPS9 caps_ = {};
        hr = device->GetDeviceCaps(&caps_);
        
        hr = DirectX::CreateWICTextureFromFileEx(
            device.Get(),
            L"font\\1.png",
            min(caps_.MaxTextureWidth, caps_.MaxTextureHeight),
            0,
            D3DPOOL_DEFAULT,
            DirectX::WIC_LOADER_DEFAULT,
            texture.GetAddressOf());
        
        idle();
    }
    void unload()
    {
        VB.Reset();
        IB.Reset();
        InputLayout.Reset();
        VS.Reset();
        PS.Reset();
        texture.Reset();
        device.Reset();
    }
    void setup()
    {
        IDirect3DDevice9* ctx = device.Get();
        HRESULT hr = 0;
        
        Microsoft::WRL::ComPtr<IDirect3DSurface9> backbuffer;
        hr = ctx->GetRenderTarget(0, backbuffer.GetAddressOf());
        D3DSURFACE_DESC info = {};
        hr = backbuffer->GetDesc(&info);
        const UINT width = info.Width, height = info.Height;
        
        // [IA Stage]
        
        hr = ctx->SetVertexDeclaration(InputLayout.Get());
        
        hr = ctx->SetStreamSource(0, VB.Get(), 0, sizeof(Vertex));
        hr = ctx->SetStreamSourceFreq(0, 1);
        
        hr = ctx->SetIndices(IB.Get());
        
        // [VS Stage]
        
        const DirectX::XMMATRIX proj = DirectX::XMMatrixOrthographicOffCenterLH(
            -(float)width * 0.5f, (float)width * 0.5f,
            -(float)height * 0.5f, (float)height * 0.5f,
            0.0f, 1.0f);
        DirectX::XMFLOAT4X4 mvp;
        DirectX::XMStoreFloat4x4(&mvp, proj);
        hr = ctx->SetVertexShaderConstantF(0, (float*)&mvp, 4);
        
        //float mat_identity[16] = {
        //    1.0f, 0.0f, 0.0f, 0.0f,
        //    0.0f, 1.0f, 0.0f, 0.0f,
        //    0.0f, 0.0f, 1.0f, 0.0f,
        //    0.0f, 0.0f, 0.0f, 1.0f,
        //};
        //hr = ctx->SetVertexShaderConstantF(0, mat_identity, 4);
        
        hr = ctx->SetVertexShader(VS.Get());
        
        // [RS Stage]
        
        hr = ctx->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        hr = ctx->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        hr = ctx->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        hr = ctx->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
        hr = ctx->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
        
        const D3DVIEWPORT9 viewport = { 0, 0, width, height, 0.0f, 1.0f };
        hr = ctx->SetViewport(&viewport);
        
        const RECT scissor = { 0, 0, (LONG)width, (LONG)height };
        hr = ctx->SetScissorRect(&scissor);
        
        // [PS Stage]
        
        hr = ctx->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        hr = ctx->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        hr = ctx->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
        hr = ctx->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        hr = ctx->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        hr = ctx->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        
        hr = ctx->SetTexture(0, texture.Get());
        
        hr = ctx->SetPixelShader(PS.Get());
        
        // [OM Stage]
        
        hr = ctx->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
        hr = ctx->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        
        hr = ctx->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        hr = ctx->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
        hr = ctx->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        hr = ctx->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        hr = ctx->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        hr = ctx->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);
        hr = ctx->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
        hr = ctx->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
        const DWORD mask = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;
        hr = ctx->SetRenderState(D3DRS_COLORWRITEENABLE, mask);
        
        // [Fixed Pipeline]
        
        hr = ctx->SetRenderState(D3DRS_CLIPPING, FALSE);
        hr = ctx->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
        hr = ctx->SetRenderState(D3DRS_LIGHTING, FALSE);
        hr = ctx->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
        hr = ctx->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
        hr = ctx->SetRenderState(D3DRS_FOGENABLE, FALSE);
        hr = ctx->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
        hr = ctx->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        
        idle();
    }
    void draw(float x, float y, float w, float h, float z, uint32_t ucolor, int channel)
    {
        IDirect3DDevice9* ctx = device.Get();
        
        HRESULT hr = 0;
        
        {
            void* pData = NULL;
            hr = VB->Lock(0, 4 * sizeof(Vertex), &pData, D3DLOCK_DISCARD);
            if (hr == D3D_OK)
            {
                channel = max(0, min(channel, 3));
                const uint32_t chmasks[4] = {
                    0x00FF0000,
                    0x0000FF00,
                    0x000000FF,
                    0xFF000000,
                };
                const uint32_t chmask = chmasks[channel];
                Vertex VBdata[] = {
                    { x - w * 0.5f, y + h * 0.5f, z, 0.0f, 0.0f, ucolor, chmask },
                    { x + w * 0.5f, y + h * 0.5f, z, 1.0f, 0.0f, ucolor, chmask },
                    { x + w * 0.5f, y - h * 0.5f, z, 1.0f, 1.0f, ucolor, chmask },
                    { x - w * 0.5f, y - h * 0.5f, z, 0.0f, 1.0f, ucolor, chmask },
                };
                //Vertex VBdata[] = {
                //    { -0.5f,  0.5f, z, 0.0f, 0.0f, 0xFFFFFFFF, 0x00FF0000 },
                //    {  0.5f,  0.5f, z, 1.0f, 0.0f, 0xFFFFFFFF, 0x00FF0000 },
                //    {  0.5f, -0.5f, z, 1.0f, 1.0f, 0xFFFFFFFF, 0x00FF0000 },
                //    { -0.5f, -0.5f, z, 0.0f, 1.0f, 0xFFFFFFFF, 0x00FF0000 },
                //};
                memcpy(pData, VBdata, sizeof(VBdata));
            }
            hr = VB->Unlock();
        }
        
        {
            void* pData = NULL;
            hr = IB->Lock(0, 6 * sizeof(Index), &pData, D3DLOCK_DISCARD);
            if (hr == D3D_OK)
            {
                Index IBdata[] = {
                    0, 1, 2,
                    0, 2, 3,
                };
                memcpy(pData, IBdata, sizeof(IBdata));
            }
            hr = IB->Unlock();
        }
        
        setup();
        
        hr = ctx->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
        
        idle();
    }
};
static TestData g_Data;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char**)
{
    CoInitializeEx(0, COINIT_MULTITHREADED);
    
    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX9 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);
    
    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    // Setup Dear ImGui style
    ImGui::StyleColorsLight();
    //ImGui::StyleColorsClassic();
    
    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);
    
    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 16.0f * ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd));
    ImGui::GetStyle().ScaleAllSizes(ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd));
    ImGui::GetIO().Fonts->Build();
    
    // Custom Data
    g_Data.load(g_pd3dDevice);
    
    // Show the window
    ::ShowWindow(hwnd, SW_MAXIMIZE);
    ::UpdateWindow(hwnd);
    
    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    //ImVec4 clear_color = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    ImVec4 clear_color = ImVec4(0.5f, 0.5f, 0.5f, 1.00f);
    float draw_xy_[2] = {};
    float draw_wh_[2] = {1024.0f, 1024.0f};
    float draw_z_ = 0.5f;
    int draw_ch_ = 0;
    ImVec4 draw_color_(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        if (ImGui::Begin("Test"))
        {
            ImGui::DragFloat2("XY", draw_xy_);
            ImGui::SliderFloat("Z", &draw_z_, -1.0f, 1.0f);
            ImGui::DragFloat("Scale", &draw_wh_[0]);
            ImGui::SliderInt("Channel", &draw_ch_, 0, 3);
            ImGui::ColorEdit4("Color", (float*)&draw_color_);
        }
        ImGui::End();
        ImGui::EndFrame();
        ImGui::Render();
        
        // Rendering
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x*clear_color.w*255.0f), (int)(clear_color.y*clear_color.w*255.0f), (int)(clear_color.z*clear_color.w*255.0f), (int)(clear_color.w*255.0f));
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            g_Data.draw(draw_xy_[0], draw_xy_[1], draw_wh_[0], draw_wh_[0], draw_z_, ImGui::GetColorU32(draw_color_), draw_ch_);
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }
    
    g_Data.unload();
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
    
    CoUninitialize();
    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    g_Data.unload();
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
    g_Data.load(g_pd3dDevice);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
