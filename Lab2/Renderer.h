#pragma once

#include "framework.h"

class Renderer
{
    UINT m_width = 1280;
    UINT m_height = 720;
    ID3D11Device* m_pDevice = NULL;
    ID3D11DeviceContext* m_pDeviceContext = NULL;
    IDXGISwapChain* m_pSwapChain = NULL;
    ID3D11RenderTargetView* m_pBackBufferRTV = NULL;

    ID3D11Buffer* m_pVertexBuffer = NULL;
    ID3D11Buffer* m_pIndexBuffer = NULL;

    ID3D11PixelShader* m_pPixelShader = NULL;
    ID3D11VertexShader* m_pVertexShader = NULL;
    ID3D11InputLayout* m_pInputLayout = NULL;

    bool m_isRunning = false;

public:
    bool Init(HWND hWnd);
    void Term();
    bool Render();
    bool Resize(UINT width, UINT height);
    bool IsRunning() { return m_isRunning; }

private:
    HRESULT SetupBackBuffer();
    void TermScene();
    HRESULT InitScene();
    enum class SHADER_TYPE
    {
        PIXEL_SHADER,
        VERTEX_SHADER
    };
    HRESULT CompileAndCreateShader(const std::wstring& path, SHADER_TYPE type, ID3D11DeviceChild** ppShader, ID3DBlob** ppCode = nullptr);

};
