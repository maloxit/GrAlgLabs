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
    bool m_isRunning = false;

public:
    bool Init(HWND hWnd);
    void Term();
    bool Render();
    bool Resize(UINT width, UINT height);
    bool IsRunning() { return m_isRunning; }

private:
    HRESULT SetupBackBuffer();

};
