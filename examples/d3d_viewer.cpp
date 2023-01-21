/*
SFL D3D Viewer

TODO
- Account for Y axis flipped images
*/
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#define SFL_BMP_CUSTOM_PIXEL_FORMATS 1
#define SFL_BMP_PIXEL_FORMAT_B8G8R8A8 DXGI_FORMAT_B8G8R8A8_UNORM
#define SFL_BMP_PIXEL_FORMAT_B8G8R8   -1
#define SFL_BMP_PIXEL_FORMAT_B5G6R5   DXGI_FORMAT_B5G6R5_UNORM
#define SFL_BMP_PIXEL_FORMAT_R8G8B8A8 DXGI_FORMAT_R8G8B8A8_UNORM
#define SFL_BMP_PIXEL_FORMAT_B8G8R8X8 DXGI_FORMAT_B8G8R8X8_UNORM
#define SFL_BMP_IMPLEMENTATION
#include "sfl_bmp.h"

constexpr wchar_t *Window_Class_Name = L"d3d_viewer_window";
const int Client_Width  = 640;
const int Client_Height = 480;

#define CHECK_HRESULT(expr)                                 \
    do {                                                    \
        HRESULT _result = (expr);                           \
        if (!SUCCEEDED(_result)) {                          \
            MessageBoxA(0, #expr, "HRESULT ERROR", MB_OK);  \
            ExitProcess(1);                                 \
        }                                                   \
    } while (0)

static struct {
    HWND window;
    bool running;

    // D3D
    IDXGISwapChain           *swap_chain;
    ID3D11Device             *device;
    ID3D11DeviceContext      *device_ctx;
    ID3D11RenderTargetView   *rtv;
    ID3D11Texture2D          *tr;
    ID3D11ShaderResourceView *srv;
    ID3DBlob                 *vs_blob, *ps_blob;
    ID3D11VertexShader       *vs;
    ID3D11PixelShader        *ps;
    ID3D11SamplerState       *smp;
} G;

static LRESULT win_proc(
    HWND window_handle, 
    UINT message, 
    WPARAM wparam, 
    LPARAM lparam);

static void render() {
    // Render
    float clear_color[4] = {
        1.0f,
        0.0f,
        1.0f,
        1.0f,
    };

    G.device_ctx->ClearRenderTargetView(G.rtv, clear_color);

    G.device_ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    G.device_ctx->VSSetShader(G.vs, 0, 0);
    G.device_ctx->PSSetShader(G.ps, 0, 0);

    ID3D11ShaderResourceView *views[1] = {
        G.srv
    };

    ID3D11SamplerState *sampler_states[1] = {
        G.smp
    };

    G.device_ctx->PSSetShaderResources(0, 1, views);
    G.device_ctx->PSSetSamplers(0, 1, sampler_states);

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MaxDepth = 1;
    viewport.Width  = float(Client_Width);
    viewport.Height = float(Client_Height);
    G.device_ctx->RSSetViewports(1, &viewport);
    G.device_ctx->Draw(6, 0);

    G.swap_chain->Present(1, 0);
}

static const char *Simple_Shader = R"(
static float2 uv_ar[6] = {
    float2(0.0, 1.0),
    float2(0.0, 0.0),
    float2(1.0, 0.0),
    float2(0.0, 1.0),
    float2(1.0, 0.0),
    float2(1.0, 1.0)
};

static float2 up_ar[6] = {
    float2(-1.0, -1.0), // left bottom
    float2(-1.0, +1.0), // left top
    float2(+1.0, +1.0), // right top
    float2(-1.0, -1.0), // left bottom
    float2(+1.0, +1.0), // right top
    float2(+1.0, -1.0)  // right bottom
};

struct vs_out {
    float4 pos : SV_POSITION;
    float2 uv  : UV;
};

vs_out vs_main(uint vid : SV_VERTEXID) {
    vs_out output;
    output.pos = float4(up_ar[vid], 1, 1);
    output.uv = uv_ar[vid];
    return output;
}

Texture2D<float4> texture_sample : register(t0);
SamplerState texture_sampler     : register(s0);

float4 ps_main(vs_out input) : SV_TARGET {
    return texture_sample.Sample(texture_sampler, input.uv);
}
)";

int WinMain(
    HINSTANCE instance,
    HINSTANCE prev_instance,
    LPSTR cmdline,
    int cmdshow)
{

    WNDCLASSEXW wndclass;
    ZeroMemory(&wndclass, sizeof(wndclass));
    wndclass.cbSize = sizeof(wndclass);
    wndclass.lpszClassName = Window_Class_Name;
    wndclass.hInstance = instance;
    wndclass.hCursor = LoadCursorW(instance, 0);
    wndclass.lpfnWndProc = win_proc;

    if (!RegisterClassExW(&wndclass)) {
        DWORD error = GetLastError();
        MessageBoxA(0, "Failed to register class", "Failure", MB_OK);
        return -1;
    }

    RECT r;
    ZeroMemory(&r, sizeof(r));

    r.right = Client_Width;
    r.bottom = Client_Height;
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);    

    G.window = CreateWindowExW(
        0,
        Window_Class_Name,
        L"D3D Viewer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
        0,
        0,
        instance,
        0);

    if (G.window == 0) {
        return -1;
    }

    DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
    swap_chain_desc.OutputWindow = G.window;
    swap_chain_desc.Windowed = true;
    swap_chain_desc.BufferDesc.RefreshRate.Numerator = 0;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.BufferCount = 1;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_1;

    CHECK_HRESULT(D3D11CreateDeviceAndSwapChain(
        0,
        D3D_DRIVER_TYPE_HARDWARE,
        0,
        D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_SINGLETHREADED,
        0,
        0,
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &G.swap_chain,
        &G.device,
        &feature_level,
        &G.device_ctx));

    ID3D11Texture2D *back_buffer;
    CHECK_HRESULT(G.swap_chain->GetBuffer(
        0, 
        __uuidof(ID3D11Texture2D), 
        (void**)&back_buffer));

    CHECK_HRESULT(G.device->CreateRenderTargetView(back_buffer, 0, &G.rtv));
    G.device_ctx->OMSetRenderTargets(1, &G.rtv, 0);

    ShowWindow(G.window, SW_SHOW);
    UpdateWindow(G.window);

    // Load the texture
    SflBmpReadContext read_context;
    FILE *f = fopen(cmdline, "rb");

    sfl_bmp_read_context_stdio_init(&read_context);    
    sfl_bmp_read_context_stdio_set_file(&read_context, f);

    SflBmpDesc desc;
    if (!sfl_bmp_read_context_decode(&read_context, &desc)) {
        return 0;
    }

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Format = (DXGI_FORMAT)desc.format;
    tex_desc.Width = desc.width;
    tex_desc.Height = desc.height;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA tex_data = {};
    tex_data.pSysMem = desc.data;
    tex_data.SysMemPitch = desc.pitch;

    CHECK_HRESULT(G.device->CreateTexture2D(&tex_desc, &tex_data, &G.tr));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = tex_desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
    srv_desc.Texture2D.MostDetailedMip = 0;

    CHECK_HRESULT(G.device->CreateShaderResourceView(G.tr, &srv_desc, &G.srv));

    // Create sampler state

    D3D11_SAMPLER_DESC smp_desc = {};
    smp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    smp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    smp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    smp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    smp_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    smp_desc.MinLOD = 0;
    smp_desc.MaxLOD = D3D11_FLOAT32_MAX;
    G.device->CreateSamplerState(&smp_desc, &G.smp);

    // Create & compile shader
    ID3DBlob *errors;
    CHECK_HRESULT(D3DCompile
        ((void*)Simple_Shader,
        strlen(Simple_Shader),
        0, 0, 0,
        "vs_main",
        "vs_4_0",
        0, 0,
        &G.vs_blob,
        &errors));

    CHECK_HRESULT(D3DCompile
        ((void*)Simple_Shader,
        strlen(Simple_Shader),
        0, 0, 0,
        "ps_main",
        "ps_4_0",
        0, 0,
        &G.ps_blob,
        &errors));

    CHECK_HRESULT(G.device->CreateVertexShader(
        G.vs_blob->GetBufferPointer(),
        G.vs_blob->GetBufferSize(),
        0,
        &G.vs));

    CHECK_HRESULT(G.device->CreatePixelShader(
        G.ps_blob->GetBufferPointer(),
        G.ps_blob->GetBufferSize(),
        0,
        &G.ps));

    G.running = true;
    while (G.running) {
        MSG message;
        if (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        render();
    }

    return 0;    
}

static LRESULT win_proc(
    HWND window_handle, 
    UINT message, 
    WPARAM wparam, 
    LPARAM lparam)
{
    LRESULT result = 0;

    switch (message) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            G.running = false;
        } break;

        default: {
            result = DefWindowProc(window_handle, message, wparam, lparam);
        } break;
    }

    return result;
}
