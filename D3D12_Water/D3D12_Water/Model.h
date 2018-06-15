#pragma once
#include <fstream>
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <map>
#include <wrl/client.h>
#include "Texture.h"
#include "D3D.h"

class Model
{
private:
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 uv;
		DirectX::XMFLOAT3 normal;
	};

	struct ModelType
	{
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	int m_vertexCount, m_indexCount;
	ModelType* m_model;
	Texture* m_texture;

private:
	bool InitializeBuffers(ID3D12Device*);
	void ShutdownBuffers();
	void RenderBuffers(ID3D12GraphicsCommandList*);

	bool LoadTexture(ID3D12Device*, ID3D12GraphicsCommandList*, WCHAR*, D3D*);
	void ReleaseTexture();

	bool LoadModel(char*);
	void ReleaseModel();

public:
	Model();
	Model(const Model&);
	~Model();

	bool Initialize(ID3D12Device*, ID3D12GraphicsCommandList*, WCHAR*, char*, D3D*);
	// 頂点とインデックス情報のコマンドしかセットしません。
	void Render(ID3D12GraphicsCommandList*);

	int GetIndexCount();
	ID3D12DescriptorHeap* GetTexture();

};

