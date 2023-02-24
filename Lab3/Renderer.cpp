#include "Renderer.h"

#define SAFE_RELEASE(A) if ((A) != NULL) { (A)->Release(); (A) = NULL; }

std::wstring Extension(const std::wstring& path)
{
    std::string::size_type idx;

    idx = path.rfind(L'.');

    if (idx != std::wstring::npos)
    {
        return path.substr(idx + 1);
    }
    else
    {
        return L"";
    }
}

std::string WCSToMBS(const std::wstring &wstr)
{
    constexpr size_t BUFFER_SIZE = 250;
    std::vector<char> buff;
    buff.resize(BUFFER_SIZE + 1);
    size_t size;
    wcstombs_s(&size, buff.data(), BUFFER_SIZE + 1, wstr.c_str(), BUFFER_SIZE);
    return std::string(buff.begin(), buff.end());
}

inline HRESULT SetResourceName(ID3D11DeviceChild * pResource, const std::string & name)
{
    return pResource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
}

struct Vertex
{
    float x, y, z;
    COLORREF color;
};

struct SceneTransformsBuffer
{
    DirectX::XMMATRIX model;
};

struct ViewTransformsBuffer
{
    DirectX::XMMATRIX vp;
};

bool Renderer::Init(HWND hWnd)
{
    if (m_isRunning)
    {
        return false;
    }

    HRESULT result;

    // Create a DirectX graphics interface factory.
    IDXGIFactory* pFactory = nullptr;
    result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);

    // Select hardware adapter
    IDXGIAdapter* pSelectedAdapter = NULL;
    if (SUCCEEDED(result))
    {
        IDXGIAdapter* pAdapter = NULL;
        for (UINT adapterIdx = 0; SUCCEEDED(pFactory->EnumAdapters(adapterIdx, &pAdapter)); adapterIdx++)
        {
            DXGI_ADAPTER_DESC desc;
            pAdapter->GetDesc(&desc);

            if (wcscmp(desc.Description, L"Microsoft Baisic Render Driver") != 0)
            {
                pSelectedAdapter = pAdapter;
                break;
            }

            pAdapter->Release();
        }
    }
    assert(pSelectedAdapter != NULL);

    // Create DirectX 11 device
    D3D_FEATURE_LEVEL level;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    if (SUCCEEDED(result))
    {
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG
        result = D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
            flags, levels, 1, D3D11_SDK_VERSION, &m_pDevice, &level, &m_pDeviceContext);
        assert(level == D3D_FEATURE_LEVEL_11_0);
        assert(SUCCEEDED(result));
    }

    // Create swapchain
    if (SUCCEEDED(result))
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Width = m_width;
        swapChainDesc.BufferDesc.Height = m_height;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hWnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = true;
        swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.Flags = 0;

        result = pFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        result = SetupBackBuffer();
    }

    if (SUCCEEDED(result))
    {
        result = InitSceneResources();
    }

    SAFE_RELEASE(pSelectedAdapter);
    SAFE_RELEASE(pFactory);

    if (SUCCEEDED(result))
    {
        m_isRunning = true;
    }

    return SUCCEEDED(result);
}

HRESULT Renderer::InitSceneResources()
{
    static const Vertex Vertices[] = {
        {-1.0f, -1.0f, -1.0f, RGB(0, 0, 0)},
        { 1.0f, -1.0f, -1.0f, RGB(255, 0, 0)},
        {-1.0f,  1.0f, -1.0f, RGB(0, 255, 0)},
        { 1.0f,  1.0f, -1.0f, RGB(255, 255, 0)},
        {-1.0f, -1.0f,  1.0f, RGB(0, 0, 255)},
        { 1.0f, -1.0f,  1.0f, RGB(255, 0, 255)},
        {-1.0f,  1.0f,  1.0f, RGB(0, 255, 255)},
        { 1.0f,  1.0f,  1.0f, RGB(255, 255, 255)},
    };

    static const USHORT Indices[] = { 
        0, 2, 3,
        3, 1, 0,

        5, 7, 6,
        6, 4, 5,

        2, 0, 4,
        4, 6, 2,

        1, 3, 7,
        7, 5, 1,

        2, 6, 7,
        7, 3, 2,

        0, 1, 5,
        5, 4, 0,
    };

    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };


    HRESULT result = S_OK;

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Vertices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &Vertices;
        data.SysMemPitch = sizeof(Vertices);
        data.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pVertexBuffer, "VertexBuffer");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Indices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &Indices;
        data.SysMemPitch = sizeof(Indices);
        data.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pIndexBuffer, "IndexBuffer");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(SceneTransformsBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        result = m_pDevice->CreateBuffer(&desc, nullptr, &m_pSceneTransformsBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSceneTransformsBuffer, "SceneTransformsBuffer");
        }
    }
    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(ViewTransformsBuffer);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;
        result = m_pDevice->CreateBuffer(&desc, nullptr, &m_pViewTransformsBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pViewTransformsBuffer, "ViewTransformsBuffer");
        }
    }


    ID3DBlob* pVertexShaderCode = NULL;
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleVertexColor_VS.hlsl", SHADER_TYPE::VERTEX_SHADER, (ID3D11DeviceChild**)&m_pVertexShader, &pVertexShaderCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleVertexColor_PS.hlsl", SHADER_TYPE::PIXEL_SHADER, (ID3D11DeviceChild**)&m_pPixelShader);
    }

    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateInputLayout(InputDesc, 2, pVertexShaderCode->GetBufferPointer(), pVertexShaderCode->GetBufferSize(), &m_pInputLayout);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pInputLayout, "InputLayout");
        }
    }

    SAFE_RELEASE(pVertexShaderCode);

    return result;
}

HRESULT Renderer::CompileAndCreateShader(const std::wstring& path, SHADER_TYPE type, ID3D11DeviceChild** ppShader, ID3DBlob** ppCode)
{
    FILE* pFile = nullptr;
    _wfopen_s(&pFile, path.c_str(), L"rb");
    assert(pFile != nullptr);
    if (pFile == nullptr)
    {
        return E_FAIL;
    }

    _fseeki64(pFile, 0, SEEK_END);
    long long size = _ftelli64(pFile);
    _fseeki64(pFile, 0, SEEK_SET);

    std::vector<char> data;
    data.resize(size + 1);

    size_t rd = fread(data.data(), 1, size, pFile);
    assert(rd == (size_t)size);

    fclose(pFile);

    std::string entryPoint = "";
    std::string platform = "";
    if (type == SHADER_TYPE::VERTEX_SHADER)
    {
        entryPoint = "vs"; platform = "vs_5_0";
    }
    else if (type == SHADER_TYPE::PIXEL_SHADER)
    {
        entryPoint = "ps"; platform = "ps_5_0";
    }
    UINT flags1 = 0;
#ifdef _DEBUG
    flags1 |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
    ID3DBlob* pCode = nullptr;
    ID3DBlob* pErrMsg = nullptr;
    HRESULT result = D3DCompile(data.data(), data.size(), WCSToMBS(path).c_str(), nullptr, nullptr, entryPoint.c_str(), platform.c_str(), flags1, 0, &pCode, &pErrMsg);
    if (!SUCCEEDED(result) && pErrMsg != nullptr)
    {
        OutputDebugStringA((const char*)pErrMsg->GetBufferPointer());
    }
    assert(SUCCEEDED(result));
    SAFE_RELEASE(pErrMsg);

    if (type == SHADER_TYPE::VERTEX_SHADER)
    {
        result = m_pDevice->CreateVertexShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, &m_pVertexShader);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pVertexShader, WCSToMBS(path).c_str());
        }
    }
    else if (type == SHADER_TYPE::PIXEL_SHADER)
    {
        result = m_pDevice->CreatePixelShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, &m_pPixelShader);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pPixelShader, WCSToMBS(path).c_str());
        }
    }

    if (ppCode != nullptr)
    {
        *ppCode = pCode;
    }
    else
    {
        SAFE_RELEASE(pCode);
    }

    return result;
}

void Renderer::ReleaseSceneResources()
{
    SAFE_RELEASE(m_pInputLayout);
    SAFE_RELEASE(m_pPixelShader);
    SAFE_RELEASE(m_pVertexShader);
    SAFE_RELEASE(m_pViewTransformsBuffer);
    SAFE_RELEASE(m_pSceneTransformsBuffer);
    SAFE_RELEASE(m_pIndexBuffer);
    SAFE_RELEASE(m_pVertexBuffer);
}

void Renderer::Term()
{
    ReleaseSceneResources();

    SAFE_RELEASE(m_pBackBufferRTV);
    SAFE_RELEASE(m_pSwapChain);
    SAFE_RELEASE(m_pDeviceContext);

    if (m_pDevice != NULL)
    {
        ID3D11Debug* d3dDebug = NULL;
        m_pDevice->QueryInterface(IID_PPV_ARGS(&d3dDebug));

        UINT references = m_pDevice->Release();
        m_pDevice = NULL;
        if (references > 1)
        {
            d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        }
        SAFE_RELEASE(d3dDebug);
    }

    SAFE_RELEASE(m_pDevice);
    m_isRunning = false;
}

bool Renderer::Render(Scene* pScene)
{
    if (!m_isRunning)
    {
        return false;
    }


    m_pDeviceContext->ClearState();

    SceneTransformsBuffer sceneTransformsBuffer = { pScene->GetModelTransform() };
    m_pDeviceContext->UpdateSubresource(m_pSceneTransformsBuffer, 0, nullptr, &sceneTransformsBuffer, 0, 0);


    DirectX::XMMATRIX v = DirectX::XMMatrixInverse(nullptr, pScene->GetCameraTransform());
    float f = 100.0f;
    float n = 0.1f;
    float fov = (float)M_PI / 3;
    float c = 1.0f / tanf(fov / 2);
    float aspectRatio = (float)m_height / m_width;
    DirectX::XMMATRIX p = DirectX::XMMatrixPerspectiveLH(tanf(fov / 2) * 2 * n, tanf(fov / 2) * 2 * n * aspectRatio, n, f);


    D3D11_MAPPED_SUBRESOURCE subresource;
    HRESULT result = m_pDeviceContext->Map(m_pViewTransformsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result))
    {
        ViewTransformsBuffer& sceneBuffer = *reinterpret_cast<ViewTransformsBuffer*>(subresource.pData);

        sceneBuffer.vp = DirectX::XMMatrixMultiply(v, p);

        m_pDeviceContext->Unmap(m_pViewTransformsBuffer, 0);
    }


    ID3D11RenderTargetView* views[] = { m_pBackBufferRTV };
    m_pDeviceContext->OMSetRenderTargets(1, views, nullptr);

    static const FLOAT BackColor[4] = { 0.5f, 0.25f, 0.75f, 1.0f };
    m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, BackColor);

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)m_width;
    viewport.Height = (FLOAT)m_height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_pDeviceContext->RSSetViewports(1, &viewport);

    D3D11_RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = m_width;
    rect.bottom = m_height;
    m_pDeviceContext->RSSetScissorRects(1, &rect);

    m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { m_pVertexBuffer };
    UINT strides[] = { 16 };
    UINT offsets[] = { 0 };
    m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    m_pDeviceContext->IASetInputLayout(m_pInputLayout);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pDeviceContext->VSSetConstantBuffers(0, 1, &m_pSceneTransformsBuffer);
    m_pDeviceContext->VSSetConstantBuffers(1, 1, &m_pViewTransformsBuffer);
    m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);
    m_pDeviceContext->DrawIndexed(36, 0, 0);


    result = m_pSwapChain->Present(0, 0);
    assert(SUCCEEDED(result));

    return SUCCEEDED(result);
}

bool Renderer::Resize(UINT width, UINT height)
{
    if (!m_isRunning)
    {
        return false;
    }

    width = max(width, 8);
    height = max(height, 8);

    if (width != m_width || height != m_height)
    {
        SAFE_RELEASE(m_pBackBufferRTV);

        HRESULT result = m_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            m_width = width;
            m_height = height;

            result = SetupBackBuffer();
        }

        return SUCCEEDED(result);
    }

    return true;
}

HRESULT Renderer::SetupBackBuffer()
{
    ID3D11Texture2D* pBackBuffer = NULL;
    HRESULT result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pBackBufferRTV);
        assert(SUCCEEDED(result));

        SAFE_RELEASE(pBackBuffer);
    }
    return result;
}
