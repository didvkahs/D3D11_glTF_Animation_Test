#pragma once

#include"Utile.h"
#include"glTF_Loader.h"

constexpr int DELAY_TIME = 0.05;


struct Vertex_s
{
	DirectX::XMFLOAT4 pos;
	DirectX::XMFLOAT4 normal;
	DirectX::XMFLOAT2 tex;
};


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
};


class Core
{
public:
	
	bool InitDevice(HWND hWnd);
	bool LoadTexture(const wchar_t* path);

	void RenderFrame(void);
	void ReleaseDevice(void);

	void SetDeltaTime(float deltaTime);

private:

	bool InitGLTF(void);
	HRESULT CompileShader(const LPCWSTR fileName, const LPCSTR entryPoint, const LPCSTR shaderModel, ID3DBlob** const blob);

	float angle;
	float deltaTime;
	float renderTime;

	Loader loader;

	D3D_FEATURE_LEVEL	FeatureLevel = D3D_FEATURE_LEVEL_11_0;

	IDXGISwapChain*			SwapChain	= nullptr;
	ID3D11Device*			Device		= nullptr;
	ID3D11DeviceContext*	DevContext	= nullptr;
	ID3D11RenderTargetView* RTView		= nullptr;
	IDXGIAdapter1*			Adapter		= nullptr;

	ID3D11VertexShader*		VertexShader	= nullptr;
	ID3D11PixelShader*		PixelShader		= nullptr;
	ID3D11InputLayout*		VertexLayout	= nullptr;
	ID3D11Buffer*			VertexBuffer	= nullptr;
	ID3D11Buffer*			PixelBuffer		= nullptr;
	ID3D11Buffer*			IndexBuffer		= nullptr;

	ID3D11ShaderResourceView*	TextureRV		= nullptr;
	ID3D11SamplerState*			SamplerLinear	= nullptr;
	ID3D11Texture2D*			DepthStencil		= nullptr;
	ID3D11DepthStencilView*		DepthStencilView	= nullptr;

	ID3D11Buffer* CBNeverChanges		= nullptr;
	ID3D11Buffer* CBChangeOnResize		= nullptr;
	ID3D11Buffer* CBChangesEveryFrame	= nullptr;

	DirectX::XMMATRIX		World;
	DirectX::XMMATRIX		View;
	DirectX::XMMATRIX		Projection;
	DirectX::XMFLOAT4		MeshColor = { 0.7f, 0.7f, 0.7f, 1.0f };


	Vertex* vertices = nullptr;
	uint32_t* indices  = nullptr;
};