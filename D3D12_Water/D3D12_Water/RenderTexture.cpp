#include "RenderTexture.h"



RenderTexture::RenderTexture()
{
	m_viewPort.TopLeftX = 0.f;
	m_viewPort.TopLeftY = 0.f;
	m_viewPort.Width = (FLOAT)640;
	m_viewPort.Height = (FLOAT)480;
	m_viewPort.MinDepth = 0.f;
	m_viewPort.MaxDepth = 1.f;

	m_scissorRect.top = 0;
	m_scissorRect.left = 0;
	m_scissorRect.right = 640;
	m_scissorRect.bottom = 480;
}


RenderTexture::~RenderTexture()
{
}

bool RenderTexture::Initialize(ID3D12Device* inDevice, int textureWidth, int textureHeight)
{
	HRESULT hr;
	// リソース作成
	D3D12_HEAP_PROPERTIES heap_properties{};
	D3D12_RESOURCE_DESC resource_desc{};

	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap_properties.CreationNodeMask = 0;
	heap_properties.VisibleNodeMask = 0;

	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resource_desc.Alignment = 0;
	resource_desc.Width = 640U;
	resource_desc.Height = 480U;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 1;
	resource_desc.Format = DXGI_FORMAT_B8G8R8A8_TYPELESS;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clear_value{};
	FLOAT clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	clear_value.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	clear_value.Color[0] = clear_color[0];
	clear_value.Color[1] = clear_color[1];
	clear_value.Color[2] = clear_color[2];
	clear_value.Color[3] = clear_color[3];
	clear_value.DepthStencil.Depth = 0.0f;
	clear_value.DepthStencil.Stencil = 0;

	hr = inDevice->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, &clear_value, IID_PPV_ARGS(&m_renderTexBuffer));
	if (FAILED(hr)) {
		return false;
	}

	// Heap作成
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc{};
	descriptor_heap_desc.NumDescriptors = 1;
	descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptor_heap_desc.NodeMask = 0;
	hr = inDevice->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&m_dh_renderTexSRV));
	if (FAILED(hr)) {
		return false;
	}

	descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = inDevice->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&m_dh_renderTexRTV));
	if (FAILED(hr)) {
		return false;
	}

	D3D12_RENDER_TARGET_VIEW_DESC render_desc{};
	render_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	render_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;  //レンダーターゲットリソースへのアクセス方法を指定します。
	render_desc.Texture2D.MipSlice = 0;							//使用できるみっぷマップレベルのインデックス
	render_desc.Texture2D.PlaneSlice = 0;						//テクスチャで使用する平面のインデックス
	render_desc.Buffer.FirstElement = 0;						// アクセスするバッファの設定
	render_desc.Buffer.NumElements = 1;

	m_renderTexRTV_handle = m_dh_renderTexRTV.Get()->GetCPUDescriptorHandleForHeapStart();
	inDevice->CreateRenderTargetView(m_renderTexBuffer.Get(), &render_desc, m_renderTexRTV_handle);

	D3D12_SHADER_RESOURCE_VIEW_DESC resourct_view_desc{};
	resourct_view_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	resourct_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	resourct_view_desc.Texture2D.MipLevels = 1;
	resourct_view_desc.Texture2D.MostDetailedMip = 0;
	resourct_view_desc.Texture2D.PlaneSlice = 0;
	resourct_view_desc.Texture2D.ResourceMinLODClamp = 0.0F;
	resourct_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	inDevice->CreateShaderResourceView(m_renderTexBuffer.Get(), &resourct_view_desc, m_dh_renderTexSRV->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void RenderTexture::SetRenderTarget(ID3D12GraphicsCommandList* inCmdList, ID3D12DescriptorHeap* depthStencilView)
{
	// Bind the render target view and depth stencil buffer to the output render pipeline.
	inCmdList->OMSetRenderTargets(1, &m_renderTexRTV_handle,TRUE, &depthStencilView->GetCPUDescriptorHandleForHeapStart());

	return;
}


void RenderTexture::ClearRenderTarget(ID3D12GraphicsCommandList* inCmdList, ID3D12DescriptorHeap* depthStencilView,
	float red, float green, float blue, float alpha)
{
	float color[4];

	// Setup the color to clear the buffer to.
	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;

	// Clear the back buffer.
	inCmdList->ClearRenderTargetView(m_renderTexRTV_handle, color, 0, nullptr);

	// Clear the depth buffer.
	inCmdList->ClearDepthStencilView(depthStencilView->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	inCmdList->RSSetViewports(1, &m_viewPort);
	inCmdList->RSSetScissorRects(1, &m_scissorRect);


	return;
}


ID3D12DescriptorHeap* RenderTexture::GetShaderResourceView()
{
	return m_dh_renderTexSRV.Get();
}

ID3D12Resource* RenderTexture::GetRenderTextureResource()
{
	return m_renderTexBuffer.Get();
}