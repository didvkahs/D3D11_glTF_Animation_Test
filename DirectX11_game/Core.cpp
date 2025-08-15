#include"Core.h"
#include<iostream>

using namespace DirectX;

void Core::SetDeltaTime(float deltaTime)
{
	this->deltaTime = deltaTime;
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
        if (*blob)
        {
            (*blob)->Release();
        }
        return hr;
    }


    if (errorBlob) { errorBlob->Release(); }
    return S_OK;
}

bool Core::LoadTexture(const DX_Texture_s* texInfo, const size_t size, eastl::vector<ScratchImage>& image, eastl::vector<TexMetadata>& metadata)
{
    HRESULT hr;

    size_t pos = 0;
    std::wstring fileType;
    std::wstring fileName;

    image.resize(size);
    metadata.resize(size);

    const size_t targetWidth = 1024;
    const size_t targetHeight = 1024;

    ScratchImage tempImage;
    TexMetadata tempMetadata;

    // - Load target image to image & metadata -

    for (size_t i = 0; i < size; ++i)
    {
        fileName = std::wstring(texInfo[i].filePath.begin(), texInfo[i].filePath.end());
        pos = fileName.find(L'.');

        if (pos == std::wstring::npos || pos + 4 > fileName.size())
        {
            MessageBox(nullptr, L"Unpropriate File index :" + i, L"Error", MB_OK);
            return false;
        }

        fileType = fileName.substr(pos, 4);

        if (L".dds" == fileType || L".DDS" == fileType)
        {
            hr = LoadFromDDSFile(fileName.c_str(), DirectX::DDS_FLAGS_NONE, &tempMetadata, tempImage);
        }
        else
        {
            hr = LoadFromWICFile(fileName.c_str(), DirectX::WIC_FLAGS_FORCE_RGB, &tempMetadata, tempImage);
        }

        if (FAILED(hr))
        {
            MessageBox(nullptr, L"Failed to load texture : " + i , L"Error", MB_OK);
            return false;
        }
        
        if (tempMetadata.mipLevels > 1) { tempMetadata.mipLevels = 1; }

        hr = DirectX::Resize(tempImage.GetImages(), tempImage.GetImageCount(), tempMetadata, targetWidth, targetHeight, TEX_FILTER_DEFAULT, image[i]);
        if (FAILED(hr))
        {
            MessageBox(nullptr, L"Failed to Resize texture : " + i, L"Error", MB_OK);
            return false;
        }

        metadata[i] = image[i].GetMetadata();
    }    


    // - Set All imagaes into same size -

    for (size_t i = 0; i < size; ++i)
    {
        if (metadata[i].width != metadata[0].width ||
            metadata[i].height != metadata[0].height ||
            metadata[i].format != metadata[0].format)
        {
            MessageBox(nullptr, L"Texture array requires all textures to have the same dimensions and format.", L"Error", MB_OK);
            return false;
        }
    }

    return true;
}

bool Core::CreateTextureAndView(const eastl::vector<ScratchImage>& image, const eastl::vector<TexMetadata>& metadata)
{
    HRESULT hr;

    size_t subResourceSize = image.size();
    eastl::vector<D3D11_SUBRESOURCE_DATA> subresourceData(subResourceSize);

    for (size_t i = 0; i < subResourceSize; ++i)
    {
        const Image* img = image[i].GetImage(0, 0, 0);
        if (!img)
        {
            MessageBox(nullptr, L"Image data is null", L"Error", MB_OK);
            return false;
        }

        subresourceData[i].pSysMem = img->pixels;
        subresourceData[i].SysMemPitch = static_cast<UINT>(img->rowPitch);
        subresourceData[i].SysMemSlicePitch = static_cast<UINT>(img->slicePitch);
    }


    // - Set Texture desc - 

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.Width = metadata[0].width;
    desc.Height = metadata[0].height;
    desc.MipLevels = metadata[0].mipLevels;
    desc.ArraySize = static_cast<UINT>(subResourceSize);
    desc.Format = metadata[0].format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    hr = Device->CreateTexture2D(&desc, subresourceData.data(), &Texture);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create texture array", L"Error", MB_OK);
        return false;
    }


    // - Set ShaderResourceView - 

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));

    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = desc.ArraySize;

    hr = Device->CreateShaderResourceView(Texture, &srvDesc, &TextureRV);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create ShaderResourceView", L"Error", MB_OK);
        return false;
    }

    return true;
}



bool Core::InitDevice(HWND hWnd, UINT width, UINT height)
{
    HRESULT hr;

    loader_gltf = new Loader_gltf("C://Users//james//Documents//2025//source_code//DirectX11_game//DirectX11_game//vigilante-deku.gltf");
    loader_gltf->ParseFile();


    // -- Set Adapter --

    {
        DXGI_ADAPTER_DESC1	adapter;
        IDXGIFactory1* factory = nullptr;

        int adapterNum = 0;
        uint64_t maxMem = 0;
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
        scd.BufferDesc.Width = width;
        scd.BufferDesc.Height = height;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = hWnd;
        scd.SampleDesc.Count = 1;
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

        dd.Width  = width;
        dd.Height = height;
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
        view.Width = (FLOAT)width;
        view.Height = (FLOAT)height;
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
    
    size_t meshCount = loader_gltf->GetMeshLength();
    MeshBuffer = new VertexIndexList[meshCount];

    size_t      vertexLength = 0;
    size_t      indexLength = 0;
    const uint32_t*       indices   =  nullptr;
    const DX_Vertex_s*    vertices  =  nullptr;

    D3D11_BUFFER_DESC bd;

    for (size_t i = 0; i < meshCount; ++i)
    {
        indices = loader_gltf->GetIndices(i);
        vertices = loader_gltf->GetVertices(i);

        indexLength = loader_gltf->GetIndexLength(i);
        if (indexLength == 0)
        {
            MessageBoxA(nullptr, "Error : ", "IndexLength is 0", MB_OK);
            return false;
        }

        vertexLength = loader_gltf->GetVertexLength(i);
        if (vertexLength == 0)
        {
            MessageBoxA(nullptr, "Error : ", "VertexLength is 0", MB_OK);
            return false;
        }

        ZeroMemory(&bd, sizeof(bd));

        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = static_cast<UINT>(vertexLength) * sizeof(DX_Vertex_s);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA initData;
        ZeroMemory(&initData, sizeof(initData));

        initData.pSysMem = vertices;
        hr = Device->CreateBuffer(&bd, &initData, &MeshBuffer[i].vertexBuffer);
        if (FAILED(hr))
        {
            MessageBoxA(nullptr, "Error", "VertexBuffer Creation failure", MB_OK);
            return false;
        }

        // - index Buffer -

        ZeroMemory(&bd, sizeof(bd));

        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bd.ByteWidth = static_cast<UINT>(indexLength) * sizeof(uint32_t);
        bd.CPUAccessFlags = 0;

        ZeroMemory(&initData, sizeof(initData));
        initData.pSysMem = indices;

        hr = Device->CreateBuffer(&bd, &initData, &MeshBuffer[i].indexBuffer);
        if (FAILED(hr))
        {
            MessageBoxA(nullptr, "Error", "IndexBuffer Creation failure", MB_OK);
            return false;
        }
    }

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

    eastl::vector<ScratchImage> images;
    eastl::vector<TexMetadata> metadatas;

    if (!LoadTexture(loader_gltf->GetTextures(), loader_gltf->GetTextureCount(), images, metadatas))
    {
        MessageBox(nullptr, L"Failed to load texture array", L"Error", MB_OK);
        return false;
    }
    if (!CreateTextureAndView(images, metadatas))
    {
        MessageBox(nullptr, L"Failed to Create SRV", L"Error", MB_OK);
        return false;
    }


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

    XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
    XMVECTOR At  = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR Up  = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    View = XMMatrixLookAtLH(Eye, At, Up);

    CBNeverChange_s cbnc;
    cbnc.view = XMMatrixTranspose(View);
    DevContext->UpdateSubresource(CBNeverChanges, 0, nullptr, &cbnc, 0, 0);

    Projection = XMMatrixPerspectiveLH(XMConvertToRadians(90.0f), (FLOAT)width / height, 0.1f, 100.0f);

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

    size_t meshCount = loader_gltf->GetMeshLength();

    float clearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };

    DevContext->ClearRenderTargetView(RTView, clearColor);
    DevContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    DevContext->PSSetShaderResources(0, 1, &TextureRV);
    DevContext->PSSetSamplers(0, 1, &SamplerLinear);

    for (size_t i = 0; i < meshCount; ++i)
    {
        size_t indexLength = loader_gltf->GetIndexLength(i);

        XMMATRIX meshWorld = loader_gltf->GetTransform(i);
        XMMATRIX additional = XMMatrixScaling(0.2, 0.2, 0.2) * XMMatrixRotationY(angle / 2);
        World = XMMatrixMultiply(meshWorld, additional);

        CBChangesEveryFrame_s cb;
        cb.world = XMMatrixTranspose(World);
        cb.meshColor = MeshColor;
        cb.textureIndex = static_cast<UINT>(i);
        DevContext->UpdateSubresource(CBChangesEveryFrame, 0, nullptr, &cb, 0, 0);

        DevContext->VSSetShader(VertexShader, nullptr, 0);
        DevContext->VSSetConstantBuffers(0, 1, &CBNeverChanges);
        DevContext->VSSetConstantBuffers(1, 1, &CBChangeOnResize);
        DevContext->VSSetConstantBuffers(2, 1, &CBChangesEveryFrame);
        DevContext->PSSetShader(PixelShader, nullptr, 0);
        DevContext->PSSetConstantBuffers(2, 1, &CBChangesEveryFrame);

        UINT stride = sizeof(DX_Vertex_s);
        UINT offset = 0;

        DevContext->IASetVertexBuffers(0, 1, &MeshBuffer[i].vertexBuffer, &stride, &offset);
        DevContext->IASetIndexBuffer(MeshBuffer[i].indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        DevContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        DevContext->DrawIndexed(static_cast<UINT>(indexLength), 0, 0);
    }

    SwapChain->Present(1, 0);
}

void Core::OnResize(UINT width, UINT height)
{
    if (DevContext == nullptr || SwapChain == nullptr)
        return;

    DevContext->OMSetRenderTargets(0, nullptr, nullptr);
    DevContext->Flush();

    if (RTView) { RTView->Release(); RTView = nullptr; }
    if (DepthStencilView) { DepthStencilView->Release(); DepthStencilView = nullptr; }
    if (DepthStencil) { DepthStencil->Release(); DepthStencil = nullptr; }

    HRESULT hr = SwapChain->ResizeBuffers(1, width, height,
        DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (FAILED(hr))
    {
        MessageBoxA(NULL, "Failed to resize swap chain buffers!", "Error", MB_OK);
        return;
    }

    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
    {
        MessageBoxA(NULL, "Failed to get back buffer!", "Error", MB_OK);
        return;
    }

    hr = Device->CreateRenderTargetView(pBackBuffer, nullptr, &RTView);
    pBackBuffer->Release();
    if (FAILED(hr))
    {
        MessageBoxA(NULL, "Failed to create render target view!", "Error", MB_OK);
        return;
    }

    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;      
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;

    hr = Device->CreateTexture2D(&descDepth, nullptr, &DepthStencil);
    if (FAILED(hr))
    {
        MessageBoxA(NULL, "Failed to create depth stencil texture!", "Error", MB_OK);
        return;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    descDSV.Texture2D.MipSlice = 0;

    hr = Device->CreateDepthStencilView(DepthStencil, &descDSV, &DepthStencilView);
    if (FAILED(hr))
    {
        MessageBoxA(NULL, "Failed to create depth stencil view!", "Error", MB_OK);
        return;
    }

    DevContext->OMSetRenderTargets(1, &RTView, DepthStencilView);

    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    DevContext->RSSetViewports(1, &vp);

    CBChangeOnResize_s cbOnResize;
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    cbOnResize.projection = DirectX::XMMatrixTranspose(
        DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, aspectRatio, 0.1f, 100.0f));
    DevContext->UpdateSubresource(CBChangeOnResize, 0, nullptr, &cbOnResize, 0, 0);
}


void Core::ReleaseDevice(void)
{
    uint64_t meshCount = loader_gltf->GetMeshLength();

    if (MeshBuffer)
    {
        for (uint64_t i = 0; i < meshCount; ++i)
        {
            if (MeshBuffer[i].vertexBuffer) { MeshBuffer[i].vertexBuffer->Release(); }
            if (MeshBuffer[i].indexBuffer)  { MeshBuffer[i].indexBuffer->Release();  }
        }

        delete[] MeshBuffer;
    }

    if (DevContext) { DevContext->ClearState(); }

    if (RTView)     { RTView->Release();    }
    if (SwapChain)  { SwapChain->Release(); }
    if (DevContext) { DevContext->Release();}
    if (Device)     { Device->Release();    }


    if (Texture)      { Texture->Release();      }
    if (TextureRV)    { TextureRV->Release();    }
    if (VertexLayout) { VertexLayout->Release(); }
    if (VertexShader) { VertexShader->Release(); }
    if (PixelShader)  { PixelShader->Release();  }
    if (PixelBuffer)  { PixelBuffer->Release();  }

    if (DepthStencil)       { DepthStencil->Release();      }
    if (DepthStencilView)   { DepthStencilView->Release();  }

    if (CBNeverChanges)      { CBNeverChanges->Release();       }
    if (CBChangeOnResize)    { CBChangeOnResize->Release();     }
    if (CBChangesEveryFrame) { CBChangesEveryFrame->Release();  }
}