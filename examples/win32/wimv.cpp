/* SFL wimv v0.1

This example uses Direct3D11, the Windows Imaging Component and sfl_fs_watch.h
to load and display an image file, and reload it if it is changed from some 
other editor

MIT License

Copyright (c) 2023 Michael Dodis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

USAGE
wimv <path>

- path: Relative or absolute path to the image file

DOCUMENTATION
This example uses the Windows Imaging Component to load images, so it should
support any image supported by WIC. See https://learn.microsoft.com/en-us/windows/win32/wic/-wic-about-windows-imaging-codec#native-codecs
for the natively supported codecs

CONTRIBUTION
Michael Dodis (michaeldodisgr@gmail.com)

*/
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wincodec.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "sfl_fs_watch.h"

#define SFL_BMP_IO_IMPLEMENTATION_STDIO  0
#define SFL_BMP_IO_IMPLEMENTATION_WINAPI 1
#include "sfl_bmp.h"

constexpr wchar_t *Window_Class_Name = L"wimv_viewer_window";
const int Client_Width  = 640;
const int Client_Height = 480;

#define CHECK_HRESULT(expr)                                 \
    do {                                                    \
        HRESULT _result = (expr);                           \
        if (!SUCCEEDED(_result)) {                          \
            char buf[256];                                  \
            sprintf(buf, "HRESULT = 0x%x", _result);        \
            MessageBoxA(0, #expr, buf, MB_OK);              \
            ExitProcess(1);                                 \
        }                                                   \
    } while (0)

static struct {
    HWND window;
    bool running;

    // Parameters
    bool use_sfl = false;
    const wchar_t *filew = 0;
    const char *file = 0;

    // D3D
    IDXGISwapChain           *swap_chain;
    ID3D11Device             *device;
    ID3D11DeviceContext      *device_ctx;
    ID3D11RenderTargetView   *rtv;
    ID3D11Texture2D          *tr = 0;
    ID3D11ShaderResourceView *srv = 0;
    ID3DBlob                 *vs_blob, *ps_blob;
    ID3D11VertexShader       *vs;
    ID3D11PixelShader        *ps;
    ID3D11SamplerState       *smp;

    // Watch
    SflFsWatchContext watch;
} G;

static LRESULT win_proc(
    HWND window_handle, 
    UINT message, 
    WPARAM wparam, 
    LPARAM lparam);

static void load_image(const wchar_t *path);
static wchar_t *multibyte_to_wstr(const char *multibyte);
static char *wstr_to_multibyte(const wchar_t *wstr);

PROC_SFL_FS_WATCH_NOTIFY(my_notify) {
    if (notification->kind == SFL_FS_WATCH_NOTIFICATION_FILE_MODIFIED) {
        OutputDebugStringA(notification->path);
        OutputDebugStringA(" changed");
        OutputDebugStringA("\n");
        load_image(G.filew);
    }
}

static void render() {
    // Render
    float clear_color[4] = {
        1.0f,
        0.0f,
        1.0f,
        1.0f,
    };

    G.device_ctx->ClearRenderTargetView(G.rtv, clear_color);

    if (G.srv != 0) {

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

    }

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

int wWinMain(
    HINSTANCE instance,
    HINSTANCE prev_instance,
    LPWSTR cmdline,
    int cmdshow)
{
    int argc;
    wchar_t **argv = CommandLineToArgvW(cmdline, &argc);

    for (int i = 0; i < argc; ++i) {
        if (wcscmp(argv[i], L"-sfl") == 0) {
            G.use_sfl = true;
        } else {
            G.filew = argv[i];
        }
    }

    G.file = wstr_to_multibyte(G.filew);

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
        L"WIMV",
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

    CHECK_HRESULT(CoInitialize(0));

    // Load the texture
    load_image(G.filew);

    // Add watch to the file
    sfl_fs_watch_init(&G.watch, my_notify, 0);
    sfl_fs_watch_add(&G.watch, G.file);

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

        SflFsWatchResult result = sfl_fs_watch_poll(&G.watch);
        assert(result >= 0);

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

static unsigned char *load_image_wic(
    HANDLE file_handle, 
    int *out_width, 
    int *out_height);

static unsigned char *load_image_sfl(
    HANDLE file_handle, 
    int *out_width, 
    int *out_height);

static void load_image(const wchar_t *path) {

    HANDLE file_handle = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);

    if (file_handle == INVALID_HANDLE_VALUE) {
        return;
    }

    if (G.srv) {
        G.srv->Release();
        G.srv = 0;
    }

    if (G.tr) {
        G.tr->Release();
        G.tr = 0;
    }

    unsigned char *data;
    int width, height;
    if (G.use_sfl) {
        data = load_image_sfl(file_handle, &width, &height);
    } else {
        data = load_image_wic(file_handle, &width, &height);
    }

    if (data == 0) {
        return;
    }

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA tex_data = {};
    tex_data.pSysMem = data;
    tex_data.SysMemPitch = width * 4;

    CHECK_HRESULT(G.device->CreateTexture2D(&tex_desc, &tex_data, &G.tr));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = tex_desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
    srv_desc.Texture2D.MostDetailedMip = 0;

    CHECK_HRESULT(G.device->CreateShaderResourceView(G.tr, &srv_desc, &G.srv));

    free(data);

    CloseHandle(file_handle);
}

static unsigned char *load_image_wic(
    HANDLE file_handle, 
    int *out_width, 
    int *out_height)
{

    IWICImagingFactory *factory;
    CHECK_HRESULT(CoCreateInstance(
        CLSID_WICImagingFactory,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory,
        (void**)&factory));
    
    WICDecodeOptions decode_options = WICDecodeMetadataCacheOnDemand;

    IWICBitmapDecoder *decoder;
    CHECK_HRESULT(factory->CreateDecoderFromFileHandle(
        (ULONG_PTR)file_handle,
        0,
        decode_options,
        &decoder));

    IWICBitmapFrameDecode *frame;
    CHECK_HRESULT(decoder->GetFrame(0, &frame));

    IWICFormatConverter *converter;
    CHECK_HRESULT(factory->CreateFormatConverter(&converter));

    CHECK_HRESULT(converter->Initialize(
        frame,
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        0,
        0,
        WICBitmapPaletteTypeCustom));

    unsigned int width, height;
    converter->GetSize(&width, &height);

    unsigned char *data = (unsigned char*)malloc(width * height * 4);

    CHECK_HRESULT(converter->CopyPixels(
        0,
        width * 4,
        (UINT)(width * height * 4),
        data));

    converter->Release();
    frame->Release();
    decoder->Release();
    factory->Release();

    *out_width = width;
    *out_height = height;

    return data;
}

static unsigned char *load_image_sfl(
    HANDLE file_handle, 
    int *out_width, 
    int *out_height)
{
    SflBmpContext ctx;
    sfl_bmp_winapi_io_init(
        &ctx, 
        sfl_bmp_stdlib_get_implementation());
    
    sfl_bmp_winapi_io_set_file(&ctx, &file_handle);

    SflBmpDesc desc;
    if (!sfl_bmp_decode(&ctx, &desc)) {
        return 0;
    }
    
    *out_width  = desc.width;
    *out_height = desc.height;

    return (unsigned char*)desc.data;
}

static char *wstr_to_multibyte(const wchar_t *wstr) {
    size_t wstr_len = (int)wcslen(wstr);

    int num_bytes = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        wstr,
        (int)wstr_len,
        0, 0, 0, 0);
    
    char *result = (char*)malloc(num_bytes + 1);

    WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        wstr,
        (int)wstr_len,
        result,
        num_bytes,
        0,
        0);

    result[num_bytes] = 0;
    return result;    
}

static wchar_t *multibyte_to_wstr(const char *multibyte) {
    int multibyte_len = (int)strlen(multibyte);

    int wstr_len = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        multibyte,
        multibyte_len,
        0,
        0);

    wchar_t *result = (wchar_t*)malloc(sizeof(wchar_t) * (wstr_len + 1));
    
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        multibyte,
        multibyte_len,
        result,
        wstr_len);

    result[wstr_len] = 0;
    return result;
}

#define SFL_FS_WATCH_IMPLEMENTATION
#include "sfl_fs_watch.h"

#define SFL_BMP_IMPLEMENTATION
#include "sfl_bmp.h"