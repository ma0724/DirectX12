#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class RenderTexture
{
private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_dh_renderTexSRV;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_dh_renderTexRTV;
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_renderTexBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE							m_renderTexRTV_handle;

	D3D12_RECT											m_scissorRect;
	D3D12_VIEWPORT										m_viewPort;


public:
	RenderTexture();
	~RenderTexture();

	bool Initialize(ID3D12Device*, int, int);

	void SetRenderTarget(ID3D12GraphicsCommandList*, ID3D12DescriptorHeap*);
	void ClearRenderTarget(ID3D12GraphicsCommandList*, ID3D12DescriptorHeap*, float, float, float, float);
	ID3D12DescriptorHeap* GetShaderResourceView();
	ID3D12Resource* GetRenderTextureResource();
};

