#include"Core.h"
#include<iostream>


using namespace DirectX;

void Core::SetDeltaTime(float deltaTime)
{
	this->deltaTime = deltaTime;
}

bool Core::InitGLTF(void)
{
    loader.ParseGLTF("C:/Users/james/Documents/2025/source_code/DirectX11_game/DirectX11_game/vigilante-deku.gltf");

    indices = loader.GetIndices();
    vertices = loader.GetVertices();

    return true;
}

HRESULT Core::CompileShader(LPCWSTR fileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** blob)
{
    HRESULT hr = S_OK;
    ID3DBlob* errorBlob = nullptr;
    UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    shaderFlags |= D3DCOMPILE_DEBUG;
#endif

    hr = D3DCompileFromFile(fileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint, shaderModel, shaderFlags, 0, blob, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            MessageBoxA(NULL, (char*)errorBlob->GetBufferPointer(), "Error", MB_OK);
        }
        if (blob)
        {
            (*blob)->Release();
        }
        return hr;
    }


    if (errorBlob) { errorBlob->Release(); }
    return S_OK;
}



/****************************** DirectX functions *************************************/

bool Core::LoadTexture(const wchar_t* path)
{
    HRESULT hr;

    DirectX::ScratchImage image;
    DirectX::TexMetadata  metadata;

    std::wstring fileType;
    std::wstring fileName = path;
    size_t pos = fileName.find(L'.');

    if (pos == std::wstring::npos || pos + 4 > fileName.size())
    {
        MessageBox(nullptr, L"(File Name) unporpriate file", L"Error", MB_OK);
        return false;
    }

    fileType = fileName.substr(pos, 4);

    if (L".dds" == fileType || L".DDS" == fileType)
    {
        hr = LoadFromDDSFile(path, DirectX::DDS_FLAGS_NONE, &metadata, image);
    }
    else if (L".tga" == fileType || L".TGA" == fileType)
    {
        hr = LoadFromTGAFile(path, &metadata, image);
    }
    else
    {
        hr = LoadFromWICFile(path, DirectX::WIC_FLAGS_NONE, &metadata, image);
    }

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"(Read File) Resource Loadign Failure", L"Error", MB_OK);
        return false;
    }


    hr = CreateShaderResourceView(Device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), &TextureRV);

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"(SRV) Creation failure", L"Error", MB_OK);
        return false;
    }

    return true;
}




bool Core::InitDevice(HWND hWnd)
{
    HRESULT hr;

    InitGLTF();
    // -- Set Adapter --

    {
        DXGI_ADAPTER_DESC1	adapter;
        IDXGIFactory1* factory = nullptr;

        int adapterNum = 0;
        size_t maxMem = 0;
        int maxAdapter = 0;

        CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
        while (SUCCEEDED(factory->EnumAdapters1(adapterNum, &Adapter)))
        {
            Adapter->GetDesc1(&adapter);

            if (adapter.DedicatedVideoMemory > maxMem)
            {
                maxMem = adapter.DedicatedVideoMemory;
                maxAdapter = adapterNum;
            }
            ++adapterNum;
        }

        factory->EnumAdapters1(maxAdapter, &Adapter);
        factory->Release();
    }



    // -- Set Device & SwapChain --

    {
        DXGI_SWAP_CHAIN_DESC scd;
        ZeroMemory(&scd, sizeof(scd));

        scd.BufferCount = 1;
        scd.BufferDesc.Width = WINDOW_WIDTH;
        scd.BufferDesc.Height = WINDOW_HEIGHT;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = hWnd;
        scd.SampleDesc.Count = 4;
        scd.Windowed = TRUE;
        scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        UINT createDeviceFlag = 0;
#ifdef _DEBUG
        createDeviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        const D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_10_1,
        };

        hr = D3D11CreateDeviceAndSwapChain(Adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
            createDeviceFlag, featureLevels, 4, D3D11_SDK_VERSION, &scd, &SwapChain, &Device, &FeatureLevel, &DevContext);
        if (FAILED(hr))
        {
            return false;
        }
    }



    // -- Set Render Target View --

    {
        ID3D11Texture2D* pBackBuffer = nullptr;
        hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
        if (FAILED(hr))
        {
            return false;
        }

        hr = Device->CreateRenderTargetView(pBackBuffer, nullptr, &RTView);
        pBackBuffer->Release();
        if (FAILED(hr))
        {
            return false;
        }
        DevContext->OMSetRenderTargets(1, &RTView, nullptr);
    }


    // -- Set Depth Stencile Buffer --

    {
        D3D11_TEXTURE2D_DESC dd;
        ZeroMemory(&dd, sizeof(dd));

        dd.Width  = WINDOW_WIDTH;
        dd.Height = WINDOW_HEIGHT;
        dd.MipLevels = 1;
        dd.ArraySize = 1;
        dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dd.SampleDesc.Count = 1;
        dd.SampleDesc.Quality = 0;
        dd.Usage = D3D11_USAGE_DEFAULT;
        dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        dd.CPUAccessFlags = 0;
        dd.MiscFlags = 0;

        hr = Device->CreateTexture2D(&dd, nullptr, &DepthStencil);
        if (FAILED(hr))
        {
            std::cout << "Error (depth Stencil)" << __LINE__ << std::endl;
            return false;
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC dv;
        ZeroMemory(&dv, sizeof(dv));

        dv.Format = dd.Format;
        dv.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dv.Texture2D.MipSlice = 0;

        hr = Device->CreateDepthStencilView(DepthStencil, &dv, &DepthStencilView);
        if (FAILED(hr))
        {
            std::cout << "Error (DepthStencil view)" << __LINE__ << std::endl;
            return false;
        }

        DevContext->OMSetRenderTargets(1, &RTView, DepthStencilView);
    }



    // -- Set Viewport --

    {
        D3D11_VIEWPORT view;
        ZeroMemory(&view, sizeof(view));

        view.TopLeftX = 0;
        view.TopLeftY = 0;
        view.Width = (FLOAT)WINDOW_WIDTH;
        view.Height = (FLOAT)WINDOW_HEIGHT;
        view.MinDepth = 0.0f;
        view.MaxDepth = 1.0f;

        DevContext->RSSetViewports(1, &view);
    }



    /************** Shader & Input Layout **************/

    ID3DBlob* VSBlob = nullptr;

    hr = CompileShader(L"Shader.fx", "VS", "vs_5_0", &VSBlob);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Error", "VertexShader compile failure", MB_OK);
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0 , D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL"  , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT      , 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    hr = Device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), nullptr, &VertexShader);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Error", "vertexShader Creation failure", MB_OK);
        return false;
    }

    hr = Device->CreateInputLayout(layout, ARRAYSIZE(layout), VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), &VertexLayout);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Error", "InputLayout Creation failure", MB_OK);
        return false;
    }

    VSBlob->Release();

    // - Pixel Shader Setting -

    ID3DBlob* PSBlob = nullptr;

    hr = CompileShader(L"Shader.fx", "PS", "ps_5_0", &PSBlob);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Error", "PixelShader compile failure", MB_OK);
        return false;
    }

    hr = Device->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), nullptr, &PixelShader);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Error", "PixelShader Creation failure", MB_OK);
        return false;
    }
    PSBlob->Release();

    DevContext->IASetInputLayout(VertexLayout);


    /************** Vertex Buffer **************/
    
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = loader.GetVertexCount() * sizeof(Vertex);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(initData));

    initData.pSysMem = vertices;
    hr = Device->CreateBuffer(&bd, &initData, &VertexBuffer);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Error", "VertexBuffer Creation failure", MB_OK);
        return false;
    }

    // - index Buffer -

    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = loader.GetIndexCount() * sizeof(uint32_t);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    ZeroMemory(&initData, sizeof(initData));
    initData.pSysMem = indices;
    
    Device->CreateBuffer(&bd, &initData, &IndexBuffer);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    DevContext->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
    DevContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    DevContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    /*********** Constant Buffer **************/

    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBNeverChange_s);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    hr = Device->CreateBuffer(&bd, nullptr, &CBNeverChanges);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Error", "ConstantBuffer (Never Change) Creation failure", MB_OK);
        return false;
    }

    bd.ByteWidth = sizeof(CBChangeOnResize_s);
    hr = Device->CreateBuffer(&bd, nullptr, &CBChangeOnResize);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Error", "ConstantBuffer (Change on resize) Creation failure", MB_OK);
        return false;
    }

    bd.ByteWidth = sizeof(CBChangesEveryFrame_s);
    hr = Device->CreateBuffer(&bd, nullptr, &CBChangesEveryFrame);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Error", "ConstantBuffer (change every frame) Creation failure", MB_OK);
        return false;
    }

    LoadTexture(L"seafloor.dds");


    /************** Set Sampler Loader ****************/

    D3D11_SAMPLER_DESC samd;
    ZeroMemory(&samd, sizeof(samd));

    samd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samd.MinLOD = 0;
    samd.MaxLOD = D3D11_FLOAT32_MAX;

    hr = Device->CreateSamplerState(&samd, &SamplerLinear);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Error", "SamplerSate Creation failure", MB_OK);
        return false;
    }


    /*********** Set World, View, Projection *************/

    World = XMMatrixIdentity();

    XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, -1.5f, 0.0f);
    XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    View = XMMatrixLookAtLH(Eye, At, Up);

    CBNeverChange_s cbnc;
    cbnc.view = XMMatrixTranspose(View);
    DevContext->UpdateSubresource(CBNeverChanges, 0, nullptr, &cbnc, 0, 0);

    Projection = XMMatrixPerspectiveLH(XMConvertToRadians(60.f), (FLOAT)WINDOW_WIDTH / WINDOW_HEIGHT, 0.01f, 100.0f);

    CBChangeOnResize_s cbcor;
    cbcor.projection = XMMatrixTranspose(Projection);
    DevContext->UpdateSubresource(CBChangeOnResize, 0, nullptr, &cbcor, 0, 0);

	return true;
}

void Core::RenderFrame(void)
{
    renderTime += deltaTime;
    if (renderTime >= DELAY_TIME)
    {
        angle += XM_PI * (float)renderTime * 0.125f;
        angle = std::fmodf(angle, XM_2PI);
        renderTime = 0;
    }

    World = XMMatrixScaling(10, 10, 10);

    MeshColor.x = (sinf(angle * 1.0f) + 1.0f) * 0.5f;
    MeshColor.y = (cosf(angle * 1.0f) + 1.0f) * 0.5f;
    MeshColor.z = (sinf(angle * 1.0f) + 1.0f) * 0.5f;

    float clearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
    DevContext->ClearRenderTargetView(RTView, clearColor);

    DevContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    CBChangesEveryFrame_s cb;
    cb.world = XMMatrixTranspose(World);
    cb.meshColor = MeshColor;
    DevContext->UpdateSubresource(CBChangesEveryFrame, 0, nullptr, &cb, 0, 0);

    DevContext->VSSetShader(VertexShader, nullptr, 0);
    DevContext->VSSetConstantBuffers(0, 1, &CBNeverChanges);
    DevContext->VSSetConstantBuffers(1, 1, &CBChangeOnResize);
    DevContext->VSSetConstantBuffers(2, 1, &CBChangesEveryFrame);
    DevContext->PSSetShader(PixelShader, nullptr, 0);
    DevContext->PSSetConstantBuffers(2, 1, &CBChangesEveryFrame);
    DevContext->PSSetShaderResources(0, 1, &TextureRV);
    DevContext->PSSetSamplers(0, 1, &SamplerLinear);
   
    DevContext->DrawIndexed(loader.GetIndexCount(), 0, 0);


    SwapChain->Present(1, 0);
}

void Core::ReleaseDevice(void)
{
    loader.Release();

    if (DevContext) { DevContext->ClearState(); }

    if (RTView) { RTView->Release(); }
    if (SwapChain) { SwapChain->Release(); }
    if (DevContext) { DevContext->Release(); }
    if (Device) { Device->Release(); }

    if (VertexBuffer) { VertexBuffer->Release(); }
    if (IndexBuffer)  { IndexBuffer->Release();  }
    if (VertexLayout) { VertexLayout->Release(); }
    if (VertexShader) { VertexShader->Release(); }
    if (PixelShader) { PixelShader->Release(); }
    if (PixelBuffer) { PixelBuffer->Release(); }

    if (DepthStencil) { DepthStencil->Release(); }
    if (DepthStencilView) { DepthStencilView->Release(); }

    if (CBNeverChanges) { CBNeverChanges->Release(); }
    if (CBChangeOnResize) { CBChangeOnResize->Release(); }
    if (CBChangesEveryFrame) { CBChangesEveryFrame->Release(); }
}