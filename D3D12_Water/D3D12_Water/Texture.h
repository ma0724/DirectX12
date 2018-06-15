#pragma once
#include <d3d12.h>
#include "TextureLoader.h"
#include "D3D.h"

class Texture
{
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>m_dh_texture;
public:
	Texture();
	~Texture();

	bool Initialize(ID3D12Device*, ID3D12GraphicsCommandList*, WCHAR*, D3D*);

	ID3D12DescriptorHeap* GetTexture();
};

