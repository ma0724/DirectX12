#include "Model.h"

using namespace DirectX;
using namespace std;

Model::Model()
{

}


Model::~Model()
{
}

bool Model::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* inCmdList, WCHAR* textureFilename, char* modelFilename, D3D* inD3d)
{
	bool result;

	// Load in the model data,
	result = LoadModel(modelFilename);
	if(!result)
	{
		return false;
	}

	// Initialize the vertex and index buffers.
	result = InitializeBuffers(device);
	if(!result)
	{
		return false;
	}

	// Load the texture for this model.
	result = LoadTexture(device, inCmdList, textureFilename, inD3d);
	if(!result)
	{
		return false;
	}

	return true;
}

bool Model::InitializeBuffers(ID3D12Device* device)
{
	Vertex* vertices;
	unsigned long* indices;
	D3D12_HEAP_PROPERTIES heapProperties{};
	D3D12_RESOURCE_DESC   resourceDesc{};
	HRESULT result;
	int i;

	// Create the vertex array.
	vertices = new Vertex[m_vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if (!indices)
	{
		return false;
	}

	// Load the vertex array and index array with data.
	for (i = 0; i<m_vertexCount; i++)
	{
		vertices[i].position = XMFLOAT3(m_model[i].x, m_model[i].y, m_model[i].z);
		vertices[i].uv = XMFLOAT2(m_model[i].tu, m_model[i].tv);
		vertices[i].normal = XMFLOAT3(m_model[i].nx, m_model[i].ny, m_model[i].nz);

		indices[i] = i;
	}

	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeof(Vertex) * m_vertexCount;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;

	// 頂点バッファの作成
	{
		result = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer));
		if (FAILED(result))
		{
			return result;
		}
	}
	// インデックスバッファ
	{
		resourceDesc.Width = sizeof(unsigned long) * m_indexCount;
		result = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer));
		if (FAILED(result))
		{
			return result;
		}
	}
	// データの書き込み
	// 書き込み可能にする
	// 頂点データの書き込み
	Vertex *vbuffer{};
	m_vertexBuffer->Map(0, nullptr, (void**)&vbuffer);
	memcpy(&vbuffer[0], &vertices[0], sizeof(Vertex) * m_vertexCount);
	vbuffer = nullptr;
	m_vertexBuffer->Unmap(0, nullptr);

	//インデックスデータの書き込み
	unsigned long *ibuffer{};
	m_indexBuffer->Map(0, nullptr, (void**)&ibuffer);
	memcpy(&ibuffer[0], &indices[0], sizeof(unsigned long) * m_indexCount);
	ibuffer = nullptr;
	// 閉じる
	m_indexBuffer->Unmap(0, nullptr);

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}


bool Model::LoadModel(char* filename)
{
	ifstream fin;
	char input;
	int i;


	// Open the model file
	fin.open(filename);
	if (fin.fail())
	{
		return false;
	}

	// Read up the vertex count
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}

	// Read in the vertex count.
	fin >> m_vertexCount;

	// Set the number of indices to be the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Create the model using the vertex count that was read in.
	m_model = new ModelType[m_vertexCount];
	if (!m_model)
	{
		return false;
	}

	// Read up to the beginning of the data.
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}
	fin.get(input);
	fin.get(input);

	// Read in the vertex data.
	for (i = 0; i<m_vertexCount; i++)
	{
		fin >> m_model[i].x >> m_model[i].y >> m_model[i].z;
		fin >> m_model[i].tu >> m_model[i].tv;
		fin >> m_model[i].nx >> m_model[i].ny >> m_model[i].nz;
	}

	// Close the model file.
	fin.close();

	return true;
}

bool Model::LoadTexture(ID3D12Device* device, ID3D12GraphicsCommandList* inCmdList, WCHAR* filename, D3D* inD3d)
{
	bool result;


	// Create the texture object.
	m_texture = new Texture;
	if (!m_texture)
	{
		return false;
	}

	// Initialize the texture object.
	result = m_texture->Initialize(device, inCmdList, filename, inD3d);
	if (!result)
	{
		return false;
	}

	return true;
}

void Model::Render(ID3D12GraphicsCommandList* _inCmdList)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(_inCmdList);

	return;
}

void Model::RenderBuffers(ID3D12GraphicsCommandList* _inCmdList)
{
	unsigned int stride;
	unsigned int offset;


	// Set vertex buffer stride and offset.
	stride = sizeof(Vertex);
	offset = 0;

	D3D12_VERTEX_BUFFER_VIEW vertex_view{};
	vertex_view.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	vertex_view.StrideInBytes = sizeof(Vertex);
	vertex_view.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * m_vertexCount);

	D3D12_INDEX_BUFFER_VIEW index_view{};
	index_view.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	index_view.SizeInBytes = static_cast<UINT>(sizeof(unsigned long) * m_indexCount);
	index_view.Format = DXGI_FORMAT_R16_UINT;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	_inCmdList->IASetVertexBuffers(0, 1, &vertex_view);

	// Set the index buffer to active in the input assembler so it can be rendered.
	_inCmdList->IASetIndexBuffer(&index_view);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	_inCmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}

int Model::GetIndexCount()
{
	return m_indexCount;
}

ID3D12DescriptorHeap* Model::GetTexture()
{
	return m_texture->GetTexture();
}