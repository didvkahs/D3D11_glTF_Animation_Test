#pragma once

#include"Utile.h"
#include"Loader_gltf.h"

constexpr float DELAY_TIME = 0.05F;

struct CBNeverChange_s
{
	DirectX::XMMATRIX view;
};

struct CBChangeOnResize_s
{
	DirectX::XMMATRIX projection;
};

struct CBChangesEveryFrame_s
{
	DirectX::XMMATRIX world;
	DirectX::XMFLOAT4 meshColor;
	UINT textureIndex;
	float pad[3];
};

struct VertexIndexList
{
	ID3D11Buffer* vertexBuffer = nullptr;
	ID3D11Buffer* indexBuffer = nullptr;
};


class Core
{
public:
	
	bool InitDevice(HWND hWnd, UINT width, UINT height);
	
	void OnResize(UINT with, UINT height);
	void RenderFrame(void);
	void ReleaseDevice(void);

	void SetDeltaTime(float deltaTime);

private:

	HRESULT CompileShader(const LPCWSTR fileName, const LPCSTR entryPoint, const LPCSTR shaderModel, ID3DBlob** const blob);
	bool LoadTexture(const DX_Texture_s* texInfo, const size_t size, eastl::vector<DirectX::ScratchImage>& image, eastl::vector<DirectX::TexMetadata>& metadata);
	bool CreateTextureAndView(const eastl::vector<DirectX::ScratchImage>& image, const eastl::vector<DirectX::TexMetadata>& metadata);

private:

	float angle = 0;
	float deltaTime = 0;
	float renderTime = 0;

	Loader_gltf* loader_gltf = nullptr;

	D3D_FEATURE_LEVEL	FeatureLevel = D3D_FEATURE_LEVEL_11_0;

	IDXGISwapChain*			SwapChain	= nullptr;
	ID3D11Device*			Device		= nullptr;
	ID3D11DeviceContext*	DevContext	= nullptr;
	ID3D11RenderTargetView* RTView		= nullptr;
	IDXGIAdapter1*			Adapter		= nullptr;

	ID3D11VertexShader*		VertexShader	= nullptr;
	ID3D11PixelShader*		PixelShader		= nullptr;
	ID3D11InputLayout*		VertexLayout	= nullptr;
	ID3D11Buffer*			PixelBuffer		= nullptr;
	
	VertexIndexList*		MeshBuffer		= nullptr;

	ID3D11Texture2D*			Texture				= nullptr;
	ID3D11ShaderResourceView*   TextureRV			= nullptr;
	ID3D11SamplerState*			SamplerLinear		= nullptr;
	ID3D11Texture2D*			DepthStencil		= nullptr;
	ID3D11DepthStencilView*		DepthStencilView	= nullptr;

	ID3D11Buffer* CBNeverChanges		= nullptr;
	ID3D11Buffer* CBChangeOnResize		= nullptr;
	ID3D11Buffer* CBChangesEveryFrame	= nullptr;

	DirectX::XMMATRIX		World;
	DirectX::XMMATRIX		View;
	DirectX::XMMATRIX		Projection;
	DirectX::XMFLOAT4		MeshColor = { 0.7f, 0.7f, 0.7f, 1.0f };
};