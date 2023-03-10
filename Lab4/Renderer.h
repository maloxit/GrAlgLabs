#pragma once

#include "framework.h"
#include "Scene.h"

class Renderer
{
    UINT m_width = 1280;
    UINT m_height = 720;
    ID3D11Device* m_pDevice = NULL;
    ID3D11DeviceContext* m_pDeviceContext = NULL;
    IDXGISwapChain* m_pSwapChain = NULL;
    ID3D11RenderTargetView* m_pBackBufferRTV = NULL;

    ID3D11Buffer* m_pSphereVertexBuffer = NULL;
    ID3D11Buffer* m_pSphereIndexBuffer = NULL;

    ID3D11Buffer* m_pCubeVertexBuffer = NULL;
    ID3D11Buffer* m_pCubeIndexBuffer = NULL;

    ID3D11Buffer* m_pSceneTransformsBuffer = NULL;
    ID3D11Buffer* m_pViewTransformsBuffer = NULL;
    
    ID3D11PixelShader* m_pSimpleTexturePixelShader = NULL;
    ID3D11VertexShader* m_pSimpleTextureVertexShader = NULL;
    ID3D11InputLayout* m_pSimpleTextureInputLayout = NULL;
    
    ID3D11PixelShader* m_pSimpleSkyboxPixelShader = NULL;
    ID3D11VertexShader* m_pSimpleSkyboxVertexShader = NULL;
    ID3D11InputLayout* m_pSimpleSkyboxInputLayout = NULL;

    ID3D11Texture2D* m_pKittyTexture = NULL;
    ID3D11ShaderResourceView* m_pKittyTextureView = NULL;
    ID3D11SamplerState* m_pSampleTextureSampler = NULL;

    ID3D11Texture2D* m_pCubemapTexture = NULL;
    ID3D11ShaderResourceView* m_pCubemapTextureView = NULL;

    bool m_isRunning = false;

public:
    bool Init(HWND hWnd);
    void Term();
    bool Render(Scene* pScene);
    bool Resize(UINT width, UINT height);
    bool IsRunning() { return m_isRunning; }

private:
    HRESULT SetupBackBuffer();
    void ReleaseSceneResources();
    HRESULT InitSceneResources();
    enum class SHADER_TYPE
    {
        PIXEL_SHADER,
        VERTEX_SHADER
    };
    HRESULT CompileAndCreateShader(const std::wstring& path, SHADER_TYPE type, ID3D11DeviceChild** ppShader, ID3DBlob** ppCode = nullptr);

};
