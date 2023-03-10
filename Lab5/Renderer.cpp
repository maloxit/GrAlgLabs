#include "Renderer.h"
#include "utils.h"
#include "LoadDDS.h"

#include <algorithm>

inline HRESULT SetResourceName(ID3D11DeviceChild* pResource, const std::string& name)
{
    return pResource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
}

struct ColorVertex
{
    float x, y, z;
    COLORREF color;
};

struct TextureVertex
{
    float x, y, z;
    float u, v;
};

struct Vertex
{
    float x, y, z;
};


struct SceneTransformsBuffer
{
    DirectX::XMMATRIX model;
    DirectX::XMVECTOR other;
};

struct ViewTransformsBuffer
{
    DirectX::XMMATRIX vp;
    DirectX::XMVECTOR cameraPos;
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
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        desc.StencilEnable = FALSE;

        result = m_pDevice->CreateDepthStencilState(&desc, &m_pDepthStateReadWrite);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pDepthStateReadWrite, "DepthStateReadWrite");
        }

    }
    if (SUCCEEDED(result))
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        desc.StencilEnable = FALSE;

        result = m_pDevice->CreateDepthStencilState(&desc, &m_pDepthStateRead);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pDepthStateRead, "DepthStateRead");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BLEND_DESC desc = {};
        desc.AlphaToCoverageEnable = FALSE;
        desc.IndependentBlendEnable = FALSE;
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        result = m_pDevice->CreateBlendState(&desc, &m_pTransBlendState);

        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pTransBlendState, "TransBlendState");
        }
    }

    if (SUCCEEDED(result))
    {
        result = SetupBackBuffer();
    }

    if (SUCCEEDED(result))
    {
        result = SetupDepthBuffer();
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
    std::vector<Vertex> sphereVertices;
    std::vector<USHORT> sphereIndices;
    int hRes = 20;
    int wRes = 10;
    float rad = 1.2f;

    for (int w = 0; w <= wRes; w++)
    {
        for (int h = 0; h <= hRes; h++)
        {
            float alpha = float(M_PI) * 2.0f * h / hRes;
            float beta = float(M_PI) * 1.0f * w / wRes;
            float x = rad * sinf(beta) * cosf(alpha);
            float z = rad * sinf(beta) * sinf(alpha);
            float y = rad * cosf(beta);
            sphereVertices.push_back({ x, y, z });
        }
    }

    for (int w = 0; w < wRes; w++)
    {
        for (int h = 0; h < hRes; h++)
        {
            int i = w * (hRes + 1) + h;
            int iNext = i + (hRes + 1);
            if (w != 0)
            {
                sphereIndices.push_back(iNext + 1);
                sphereIndices.push_back(i + 1);
                sphereIndices.push_back(i);
            }
            if (w + 1 != wRes)
            {
                sphereIndices.push_back(i);
                sphereIndices.push_back(iNext);
                sphereIndices.push_back(iNext + 1);
            }

        }
    }


    static const TextureVertex Vertices[] = {
        {-1.0f, -1.0f, -1.0f, 0.0f, 1.0f},
        {-1.0f,  1.0f, -1.0f, 0.0f, 0.0f},
        { 1.0f,  1.0f, -1.0f, 1.0f, 0.0f},
        { 1.0f, -1.0f, -1.0f, 1.0f, 1.0f},

        { 1.0f, -1.0f,  1.0f, 0.0f, 1.0f},
        { 1.0f,  1.0f,  1.0f, 0.0f, 0.0f},
        {-1.0f,  1.0f,  1.0f, 1.0f, 0.0f},
        {-1.0f, -1.0f,  1.0f, 1.0f, 1.0f},

        {-1.0f, -1.0f,  1.0f, 0.0f, 1.0f},
        {-1.0f,  1.0f,  1.0f, 0.0f, 0.0f},
        {-1.0f,  1.0f, -1.0f, 1.0f, 0.0f},
        {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f},

        { 1.0f, -1.0f, -1.0f, 0.0f, 1.0f},
        { 1.0f,  1.0f, -1.0f, 0.0f, 0.0f},
        { 1.0f,  1.0f,  1.0f, 1.0f, 0.0f},
        { 1.0f, -1.0f,  1.0f, 1.0f, 1.0f},

        {-1.0f,  1.0f, -1.0f, 0.0f, 1.0f},
        {-1.0f,  1.0f,  1.0f, 0.0f, 0.0f},
        { 1.0f,  1.0f,  1.0f, 1.0f, 0.0f},
        { 1.0f,  1.0f, -1.0f, 1.0f, 1.0f},

        {-1.0f, -1.0f,  1.0f, 0.0f, 1.0f},
        {-1.0f, -1.0f, -1.0f, 0.0f, 0.0f},
        { 1.0f, -1.0f, -1.0f, 1.0f, 0.0f},
        { 1.0f, -1.0f,  1.0f, 1.0f, 1.0f},

    };

    static const USHORT Indices[] = {
        0, 1, 2,
        2, 3, 0,

        4, 5, 6,
        6, 7, 4,

        8, 9, 10,
        10, 11, 8,

        12, 13, 14,
        14, 15, 12,

        16, 17, 18,
        18, 19, 16,

        20, 21, 22,
        22, 23, 20,
    };


    HRESULT result = S_OK;

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Vertex) * sphereVertices.size();
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = sphereVertices.data();
        data.SysMemPitch = sizeof(Vertex) * sphereVertices.size();
        data.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(&desc, &data, &m_pSphereVertexBuffer);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSphereVertexBuffer, "SphereVertexBuffer");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(USHORT) * sphereIndices.size();
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = sphereIndices.data();
        data.SysMemPitch = sizeof(USHORT) * sphereIndices.size();
        data.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(&desc, &data, &m_pSphereIndexBuffer);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSphereIndexBuffer, "SphereIndexBuffer");
        }
    }

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

        result = m_pDevice->CreateBuffer(&desc, &data, &m_pCubeVertexBuffer);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pCubeVertexBuffer, "CubeVertexBuffer");
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

        result = m_pDevice->CreateBuffer(&desc, &data, &m_pCubeIndexBuffer);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pCubeIndexBuffer, "CubeIndexBuffer");
        }
    }

    TextureDesc textureDesc;
    if (SUCCEEDED(result))
    {
        const std::wstring TextureName = L"Kitty.dds";
        bool ddsRes = LoadDDS(TextureName.c_str(), textureDesc);
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Format = textureDesc.fmt;
        desc.ArraySize = 1;
        desc.MipLevels = textureDesc.mipmapsCount;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Height = textureDesc.height;
        desc.Width = textureDesc.width;

        UINT32 blockWidth = DivUp(desc.Width, 4u);
        UINT32 blockHeight = DivUp(desc.Height, 4u);
        UINT32 pitch = blockWidth * GetBytesPerBlock(desc.Format);
        const char* pSrcData = reinterpret_cast<const char*>(textureDesc.pData);
        std::vector<D3D11_SUBRESOURCE_DATA> data;
        data.resize(desc.MipLevels);
        for (UINT32 i = 0; i < desc.MipLevels; i++)
        {
            data[i].pSysMem = pSrcData;
            data[i].SysMemPitch = pitch;
            data[i].SysMemSlicePitch = 0;
            pSrcData += pitch * size_t(blockHeight);
            blockHeight = max(1u, blockHeight / 2);
            blockWidth = max(1u, blockWidth / 2);
            pitch = blockWidth * GetBytesPerBlock(desc.Format);
        }
        result = m_pDevice->CreateTexture2D(&desc, data.data(), &m_pKittyTexture);

        if (SUCCEEDED(result))
            result = SetResourceName(m_pKittyTexture, WCSToMBS(TextureName));
    }
    if (SUCCEEDED(result))
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = textureDesc.fmt;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = textureDesc.mipmapsCount;
        desc.Texture2D.MostDetailedMip = 0;
        result = m_pDevice->CreateShaderResourceView(m_pKittyTexture, &desc, &m_pKittyTextureView);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pKittyTextureView, "KittyTextureView");
        }
    }
    if (SUCCEEDED(result))
    {
        D3D11_SAMPLER_DESC desc = {};
        desc.Filter = D3D11_FILTER_ANISOTROPIC;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MinLOD = -FLT_MAX;
        desc.MaxLOD = FLT_MAX;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 1.0f;
        result = m_pDevice->CreateSamplerState(&desc, &m_pSampleTextureSampler);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSampleTextureSampler, "SampleTextureSampler");
        }
    }

    DXGI_FORMAT textureFmt;
    if (SUCCEEDED(result)) {
        const std::wstring TextureNames[6] = {
            L"cubemap/posx.DDS", L"cubemap/negx.DDS",
            L"cubemap/posy.DDS", L"cubemap/negy.DDS",
            L"cubemap/posz.DDS", L"cubemap/negz.DDS"
        };
        TextureDesc texDescs[6];
        bool ddsRes = true;
        for (int i = 0; i < 6 && ddsRes; i++)
        {
            ddsRes = LoadDDS(TextureNames[i].c_str(), texDescs[i]);
        }
        textureFmt = texDescs[0].fmt; // Assume all are the same
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Format = textureFmt;
        desc.ArraySize = 6;
        desc.MipLevels = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Height = texDescs[0].height;
        desc.Width = texDescs[0].width;
        UINT32 blockWidth = DivUp(desc.Width, 4u);
        UINT32 blockHeight = DivUp(desc.Height, 4u);
        UINT32 pitch = blockWidth * GetBytesPerBlock(desc.Format);
        D3D11_SUBRESOURCE_DATA data[6];
        for (int i = 0; i < 6; i++)
        {
            data[i].pSysMem = texDescs[i].pData;
            data[i].SysMemPitch = pitch;
            data[i].SysMemSlicePitch = 0;
        }
        result = m_pDevice->CreateTexture2D(&desc, data, &m_pCubemapTexture);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pCubemapTexture, "CubemapTexture");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        desc.Format = textureFmt;
        desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
        desc.TextureCube.MipLevels = 1;
        desc.TextureCube.MostDetailedMip = 0;

        result = m_pDevice->CreateShaderResourceView(m_pCubemapTexture, &desc, &m_pCubemapTextureView);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pCubemapTextureView, "CubemapTextureView");
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


    static const D3D11_INPUT_ELEMENT_DESC SimpleTextureInputDesc[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleTexture_VS.hlsl", SHADER_TYPE::VERTEX_SHADER, (ID3D11DeviceChild**)&m_pSimpleTextureVertexShader, &pVertexShaderCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleTexture_PS.hlsl", SHADER_TYPE::PIXEL_SHADER, (ID3D11DeviceChild**)&m_pSimpleTexturePixelShader);
    }

    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateInputLayout(SimpleTextureInputDesc, 2, pVertexShaderCode->GetBufferPointer(), pVertexShaderCode->GetBufferSize(), &m_pSimpleTextureInputLayout);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSimpleTextureInputLayout, "SimpleTextureInputLayout");
        }
    }
    SAFE_RELEASE(pVertexShaderCode);

    static const D3D11_INPUT_ELEMENT_DESC SimpleTransTextureInputDesc[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleTransTexture_VS.hlsl", SHADER_TYPE::VERTEX_SHADER, (ID3D11DeviceChild**)&m_pSimpleTransTextureVertexShader, &pVertexShaderCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleTransTexture_PS.hlsl", SHADER_TYPE::PIXEL_SHADER, (ID3D11DeviceChild**)&m_pSimpleTransTexturePixelShader);
    }

    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateInputLayout(SimpleTransTextureInputDesc, 2, pVertexShaderCode->GetBufferPointer(), pVertexShaderCode->GetBufferSize(), &m_pSimpleTransTextureInputLayout);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSimpleTransTextureInputLayout, "SimpleTransTextureInputLayout");
        }
    }
    SAFE_RELEASE(pVertexShaderCode);


    static const D3D11_INPUT_ELEMENT_DESC SimpleSkyboxInputDesc[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleSkybox_VS.hlsl", SHADER_TYPE::VERTEX_SHADER, (ID3D11DeviceChild**)&m_pSimpleSkyboxVertexShader, &pVertexShaderCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleSkybox_PS.hlsl", SHADER_TYPE::PIXEL_SHADER, (ID3D11DeviceChild**)&m_pSimpleSkyboxPixelShader);
    }

    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateInputLayout(SimpleSkyboxInputDesc, 1, pVertexShaderCode->GetBufferPointer(), pVertexShaderCode->GetBufferSize(), &m_pSimpleSkyboxInputLayout);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSimpleSkyboxInputLayout, "SimpleSkyboxInputLayout");
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
        result = m_pDevice->CreateVertexShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, (ID3D11VertexShader**)ppShader);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(*ppShader, WCSToMBS(path).c_str());
        }
    }
    else if (type == SHADER_TYPE::PIXEL_SHADER)
    {
        result = m_pDevice->CreatePixelShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, (ID3D11PixelShader**)ppShader);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(*ppShader, WCSToMBS(path).c_str());
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
    SAFE_RELEASE(m_pSampleTextureSampler);

    SAFE_RELEASE(m_pKittyTextureView);
    SAFE_RELEASE(m_pKittyTexture);

    SAFE_RELEASE(m_pCubemapTextureView);
    SAFE_RELEASE(m_pCubemapTexture);

    SAFE_RELEASE(m_pSimpleSkyboxInputLayout);
    SAFE_RELEASE(m_pSimpleSkyboxPixelShader);
    SAFE_RELEASE(m_pSimpleSkyboxVertexShader);

    SAFE_RELEASE(m_pSimpleTextureInputLayout);
    SAFE_RELEASE(m_pSimpleTexturePixelShader);
    SAFE_RELEASE(m_pSimpleTextureVertexShader);

    SAFE_RELEASE(m_pSimpleTransTextureInputLayout);
    SAFE_RELEASE(m_pSimpleTransTexturePixelShader);
    SAFE_RELEASE(m_pSimpleTransTextureVertexShader);

    SAFE_RELEASE(m_pDepthBuffer);
    SAFE_RELEASE(m_pDepthBufferDSV);

    SAFE_RELEASE(m_pViewTransformsBuffer);
    SAFE_RELEASE(m_pSceneTransformsBuffer);

    SAFE_RELEASE(m_pSphereIndexBuffer);
    SAFE_RELEASE(m_pSphereVertexBuffer);
    SAFE_RELEASE(m_pCubeIndexBuffer);
    SAFE_RELEASE(m_pCubeVertexBuffer);
}

void Renderer::Term()
{
    ReleaseSceneResources();

    SAFE_RELEASE(m_pTransBlendState);
    SAFE_RELEASE(m_pDepthStateRead);
    SAFE_RELEASE(m_pDepthStateReadWrite);
    SAFE_RELEASE(m_pDepthBufferDSV);
    SAFE_RELEASE(m_pDepthBuffer);
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



    DirectX::XMMATRIX v = pScene->GetCameraTransform();
    DirectX::XMMATRIX vInv = DirectX::XMMatrixInverse(nullptr, v);
    float f = 100.0f;
    float n = 0.1f;
    float fov = (float)M_PI / 2;
    float c = 1.0f / tanf(fov / 2);
    float aspectRatio = (float)m_height / m_width;

    float width = n * tanf(fov / 2) * 2;
    float height = aspectRatio * width;
    float skyboxRad = sqrtf(powf(n, 2) + powf(width / 2, 2) + powf(height / 2, 2)) * 1.0f;

    DirectX::XMMATRIX skyboxScale = DirectX::XMMatrixScaling(skyboxRad, skyboxRad, skyboxRad);

    DirectX::XMMATRIX p = DirectX::XMMatrixPerspectiveLH(tanf(fov / 2) * 2 * f, tanf(fov / 2) * 2 * f * aspectRatio, f, n);


    D3D11_MAPPED_SUBRESOURCE subresource;
    HRESULT result = m_pDeviceContext->Map(m_pViewTransformsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result))
    {
        ViewTransformsBuffer& sceneBuffer = *reinterpret_cast<ViewTransformsBuffer*>(subresource.pData);

        sceneBuffer.vp = DirectX::XMMatrixMultiply(vInv, p);
        sceneBuffer.cameraPos = v.r[3];

        m_pDeviceContext->Unmap(m_pViewTransformsBuffer, 0);
    }


    ID3D11RenderTargetView* views[] = { m_pBackBufferRTV };
    m_pDeviceContext->OMSetRenderTargets(1, views, m_pDepthBufferDSV);

    static const FLOAT BackColor[4] = { 0.5f, 0.25f, 0.75f, 1.0f };
    m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, BackColor);
    m_pDeviceContext->ClearDepthStencilView(m_pDepthBufferDSV, D3D11_CLEAR_DEPTH, 0.0f, 0);


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

    {
        PrepareSimpleTextureRender();

        std::vector<SceneTransformsBuffer> sceneTransformsBuffer;
        sceneTransformsBuffer.push_back({ pScene->GetModelTransform() });
        sceneTransformsBuffer.push_back({ DirectX::XMMatrixTranslation(0.5f, 0.0f, 0.5f) });


        ID3D11ShaderResourceView* resources[] = { m_pKittyTextureView };
        m_pDeviceContext->PSSetShaderResources(0, 1, resources);
        m_pDeviceContext->IASetIndexBuffer(m_pCubeIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        ID3D11Buffer* vertexBuffers[] = { m_pCubeVertexBuffer };
        UINT strides[] = { sizeof(TextureVertex) };
        UINT offsets[] = { 0 };
        m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

        m_pDeviceContext->UpdateSubresource(m_pSceneTransformsBuffer, 0, nullptr, &sceneTransformsBuffer[0], 0, 0);
        m_pDeviceContext->DrawIndexed(36, 0, 0);

        m_pDeviceContext->UpdateSubresource(m_pSceneTransformsBuffer, 0, nullptr, &sceneTransformsBuffer[1], 0, 0);
        m_pDeviceContext->DrawIndexed(36, 0, 0);
    }
    {
        PrepareSimpleSkyboxRender();

        SceneTransformsBuffer sceneTransformsBuffer = { skyboxScale };

        ID3D11ShaderResourceView* resources[] = { m_pCubemapTextureView };
        m_pDeviceContext->PSSetShaderResources(0, 1, resources);

        m_pDeviceContext->IASetIndexBuffer(m_pSphereIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        ID3D11Buffer* vertexBuffers[] = { m_pSphereVertexBuffer };
        UINT strides[] = { sizeof(Vertex) };
        UINT offsets[] = { 0 };
        m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

        m_pDeviceContext->UpdateSubresource(m_pSceneTransformsBuffer, 0, nullptr, &sceneTransformsBuffer, 0, 0);
        m_pDeviceContext->DrawIndexed(20 * 9 * 6, 0, 0);
    }
    {
        PrepareSimpleTransTextureRender();

        std::vector<SceneTransformsBuffer> sceneTransformsBuffer;
        sceneTransformsBuffer.push_back({ DirectX::XMMatrixTranslation(-2.25f, 0.0f, -0.5f), {1.0f, 0.0f, 0.0f, 0.5f} });
        sceneTransformsBuffer.push_back({ DirectX::XMMatrixTranslation(-4.5f, 0.0f, 0.5f), { 0.0f, 1.0f, 0.0f, 0.5f } });
        sceneTransformsBuffer.push_back({ DirectX::XMMatrixTranslation(-4.5f, 3.0f, 0.5f), { 0.0f, 0.0f, 1.0f, 0.5f } });
        sceneTransformsBuffer.push_back({ DirectX::XMMatrixTranslation(-7.25f, 0.0f, -0.5f), {1.0f, 0.0f, 0.0f, 0.5f} });
        sceneTransformsBuffer.push_back({ DirectX::XMMatrixTranslation(-4.5f, 0.0f, 3.5f), { 0.0f, 1.0f, 0.0f, 0.5f } });
        sceneTransformsBuffer.push_back({ DirectX::XMMatrixTranslation(-0.5f, 3.0f, 5.5f), { 0.0f, 0.0f, 1.0f, 0.5f } });


        std::vector<std::pair<int, float>> cameraDist;
        for (int i = 0; i < sceneTransformsBuffer.size(); i++)
        {
            float dist = DirectX::XMVectorGetZ((sceneTransformsBuffer[i].model * vInv).r[3]);
            cameraDist.push_back({ i, dist });
        }

        std::sort(cameraDist.begin(), cameraDist.end(), [](const std::pair<int, float>& a, const std::pair<int, float>& b)
            {
                return a.second > b.second;
            });


        ID3D11ShaderResourceView* resources[] = { m_pKittyTextureView };
        m_pDeviceContext->PSSetShaderResources(0, 1, resources);
        m_pDeviceContext->IASetIndexBuffer(m_pCubeIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        ID3D11Buffer* vertexBuffers[] = { m_pCubeVertexBuffer };
        UINT strides[] = { sizeof(TextureVertex) };
        UINT offsets[] = { 0 };
        m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

        for (int i = 0; i < cameraDist.size(); i++)
        {
            m_pDeviceContext->UpdateSubresource(m_pSceneTransformsBuffer, 0, nullptr, &sceneTransformsBuffer[cameraDist[i].first], 0, 0);
            m_pDeviceContext->DrawIndexed(36, 0, 0);
        }
    }

    result = m_pSwapChain->Present(0, 0);
    assert(SUCCEEDED(result));

    return SUCCEEDED(result);
}

bool Renderer::PrepareSimpleTextureRender()
{
    m_pDeviceContext->OMSetDepthStencilState(m_pDepthStateReadWrite, 0);
    m_pDeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_pDeviceContext->IASetInputLayout(m_pSimpleTextureInputLayout);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDeviceContext->VSSetShader(m_pSimpleTextureVertexShader, nullptr, 0);
    m_pDeviceContext->PSSetShader(m_pSimpleTexturePixelShader, nullptr, 0);

    m_pDeviceContext->VSSetConstantBuffers(0, 1, &m_pViewTransformsBuffer);
    m_pDeviceContext->VSSetConstantBuffers(1, 1, &m_pSceneTransformsBuffer);

    ID3D11SamplerState* samplers[] = { m_pSampleTextureSampler };
    m_pDeviceContext->PSSetSamplers(0, 1, samplers);
    return true;
}

bool Renderer::PrepareSimpleSkyboxRender()
{
    m_pDeviceContext->OMSetDepthStencilState(m_pDepthStateRead, 0);
    m_pDeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_pDeviceContext->IASetInputLayout(m_pSimpleSkyboxInputLayout);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDeviceContext->VSSetShader(m_pSimpleSkyboxVertexShader, nullptr, 0);
    m_pDeviceContext->PSSetShader(m_pSimpleSkyboxPixelShader, nullptr, 0);

    m_pDeviceContext->VSSetConstantBuffers(0, 1, &m_pViewTransformsBuffer);
    m_pDeviceContext->VSSetConstantBuffers(1, 1, &m_pSceneTransformsBuffer);

    ID3D11SamplerState* samplers[] = { m_pSampleTextureSampler };
    m_pDeviceContext->PSSetSamplers(0, 1, samplers);
    return true;
}

bool Renderer::PrepareSimpleTransTextureRender()
{
    m_pDeviceContext->OMSetDepthStencilState(m_pDepthStateRead, 0);
    m_pDeviceContext->OMSetBlendState(m_pTransBlendState, nullptr, 0xFFFFFFFF);
    m_pDeviceContext->IASetInputLayout(m_pSimpleTransTextureInputLayout);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDeviceContext->VSSetShader(m_pSimpleTransTextureVertexShader, nullptr, 0);
    m_pDeviceContext->PSSetShader(m_pSimpleTransTexturePixelShader, nullptr, 0);

    m_pDeviceContext->VSSetConstantBuffers(0, 1, &m_pViewTransformsBuffer);
    m_pDeviceContext->VSSetConstantBuffers(1, 1, &m_pSceneTransformsBuffer);
    m_pDeviceContext->PSSetConstantBuffers(1, 1, &m_pSceneTransformsBuffer);

    ID3D11SamplerState* samplers[] = { m_pSampleTextureSampler };
    m_pDeviceContext->PSSetSamplers(0, 1, samplers);
    return true;
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
        SAFE_RELEASE(m_pDepthBufferDSV);
        SAFE_RELEASE(m_pDepthBuffer);

        HRESULT result = m_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            m_width = width;
            m_height = height;

            result = SetupBackBuffer();
        }
        if (SUCCEEDED(result))
        {
            result = SetupDepthBuffer();
        }

        return SUCCEEDED(result);
    }

    return true;
}

HRESULT Renderer::SetupBackBuffer()
{
    SAFE_RELEASE(m_pBackBufferRTV);
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

HRESULT Renderer::SetupDepthBuffer()
{
    SAFE_RELEASE(m_pDepthBufferDSV);
    SAFE_RELEASE(m_pDepthBuffer);
    HRESULT result = S_OK;
    if (SUCCEEDED(result))
    {
        D3D11_TEXTURE2D_DESC desc;
        desc.Format = DXGI_FORMAT_D32_FLOAT;
        desc.ArraySize = 1;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.Height = m_height;
        desc.Width = m_width;
        desc.MipLevels = 1;
        result = m_pDevice->CreateTexture2D(&desc, nullptr, &m_pDepthBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pDepthBuffer, "DepthBuffer");
        }
    }
    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateDepthStencilView(m_pDepthBuffer, nullptr, &m_pDepthBufferDSV);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pDepthBufferDSV, "pDepthBufferDSV");
        }
    }
    return result;
}
