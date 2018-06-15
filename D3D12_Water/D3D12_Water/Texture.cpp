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
	// UPLODE�p�̃q�[�v�쐬
	// TextureLoader::LoadDDSFile�͒��Œ��ԃq�[�v����R�s�[�������Ă����
	// HeapDesc�����Ő������Ă����
	ComPtr<ID3D12Resource> textureUploadHeap;
	result = TextureLoaderX12::LoadDDSFile(inDevice, inCmdList, inWchara, m_texture.ReleaseAndGetAddressOf(), &srv_desc, &textureUploadHeap);
	if (FAILED(result))
	{
		return false;
	}
	// SRV�p��DescriptorHeap
	dh_desc.NumDescriptors = 1;	// �f�X�N���v�^�̐� �e�������
	dh_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//����̓V�F�[�_���\�[�X�Ŏg����
	dh_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//����
	dh_desc.NodeMask = 0;

	result = inDevice->CreateDescriptorHeap(&dh_desc, IID_PPV_ARGS(&m_dh_texture));
	if (FAILED(result))
	{
		return false;
	}

	// SRV�쐬
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