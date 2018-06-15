#include "Texture.h"

using namespace Microsoft::WRL;

Texture::Texture()
{
}


Texture::~Texture()
{
}

bool Texture::Initialize(ID3D12Device* inDevice, ID3D12GraphicsCommandList* inCmdList, WCHAR* inWchara, D3D* inD3dWait)
{
	HRESULT result;
	D3D12_CPU_DESCRIPTOR_HANDLE handleSRV{};
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
	D3D12_DESCRIPTOR_HEAP_DESC dh_desc{};
	// UPLODE用のヒープ作成
	// TextureLoader::LoadDDSFileは中で中間ヒープからコピー生成してくれる
	// HeapDescも中で生成してくれる
	ComPtr<ID3D12Resource> textureUploadHeap;
	result = TextureLoaderX12::LoadDDSFile(inDevice, inCmdList, inWchara, m_texture.ReleaseAndGetAddressOf(), &srv_desc, &textureUploadHeap);
	if (FAILED(result))
	{
		return false;
	}
	// SRV用のDescriptorHeap
	dh_desc.NumDescriptors = 1;	// デスクリプタの数 影も入れる
	dh_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//今回はシェーダリソースで使うよ
	dh_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//可視化
	dh_desc.NodeMask = 0;

	result = inDevice->CreateDescriptorHeap(&dh_desc, IID_PPV_ARGS(&m_dh_texture));
	if (FAILED(result))
	{
		return false;
	}

	// SRV作成
	handleSRV = m_dh_texture->GetCPUDescriptorHandleForHeapStart();
	inDevice->CreateShaderResourceView(m_texture.Get(), &srv_desc, handleSRV);

	inD3dWait->WaitForPreviousFrame();
	inD3dWait->ExecuteCommandList();

	return true;
}

ID3D12DescriptorHeap* Texture::GetTexture()
{
	return m_dh_texture.Get();
}