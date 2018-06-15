#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <vector>
#include <memory>

#pragma comment(lib, "d3dcompiler.lib")

class LightPassPipeline
{
private:
	struct MatrixBufferType
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	};
	struct LightBufferType
	{
		DirectX::XMFLOAT4 ambientColor;
		DirectX::XMFLOAT4 diffuseColor;
		DirectX::XMFLOAT3 lightDirection;
		float padding;
	};
	Microsoft::WRL::ComPtr<ID3D12PipelineState>			m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>			m_rootSignature;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>				m_constantBufferMatrix;
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_constantBufferLight;
	std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>>		m_dh_constantBufferMatrix;
private:
	bool InitializeShader(ID3D12Device*, HWND, WCHAR*, WCHAR*);
	//void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

	bool SetShaderParameters(ID3D12GraphicsCommandList*, int, DirectX::XMMATRIX, DirectX::XMMATRIX, DirectX::XMMATRIX, ID3D12DescriptorHeap*, DirectX::XMFLOAT3,
		DirectX::XMFLOAT4, DirectX::XMFLOAT4);
	// ƒoƒŠƒA‚ð–Y‚ê‚È‚¢‚Å‚Ë(RTV)
	void RenderShader(ID3D12GraphicsCommandList*, int);
	HRESULT CreateRootSignature(ID3D12Device* inDevice);
	HRESULT CreatePipelineStateObject(ID3D12Device* inDevice);
public:
	LightPassPipeline();
	~LightPassPipeline();

	bool Initialize(ID3D12Device*, HWND, int);
	bool Render(ID3D12GraphicsCommandList*, int, int,  DirectX::XMMATRIX, DirectX::XMMATRIX, DirectX::XMMATRIX, ID3D12DescriptorHeap*, DirectX::XMFLOAT3, DirectX::XMFLOAT4,
		DirectX::XMFLOAT4);
};

