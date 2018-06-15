#include "D3D.h"
#include <tchar.h>
#include <algorithm>


using namespace DirectX;
using namespace Microsoft::WRL;

D3D::D3D()
{
}


D3D::~D3D()
{
}

bool D3D::Initialize(int screenWidth, int screenHeight, bool vsync, HWND hwnd, bool fullscreen,
	float screenDepth, float screenNear)
{
	// MemberInitialize
	this->m_screenWidth = screenWidth;
	this->m_screenHeight = screenHeight;
	this->m_windowHandle = hwnd;
	this->m_screenNear = screenNear;
	this->m_screenDepth = screenDepth;
	this->m_fullScreen = fullscreen;

	HRESULT result;
	// 行列作成
	CreateMatrix();
	// ファクトリの生成
	CreateFactory();
	// デバイスの生成Finput
	CreateDevice();
	// コマンドキューの生成
	CreateCommandQueue();
	// スワップチェインの生成
	CreateSwapChain();
	// レンダーターゲットビューの生成
	CreateRenderTargetView();
	// コマンドリストの生成
	CreateCommandList();
	// 深度ステンシルビューの生成
	CreateDepthStencilBuffer();
	// ViewPort,_scissorRectの設定
	CreateVisualCone();

	return true;
}

HRESULT D3D::CreateMatrix()
{
	float fieldOfView, screenAspect;
	fieldOfView = (float)XM_PI / 4.0f;
	screenAspect = (float)m_screenWidth / (float)m_screenHeight;

	// Create the projection matrix for 3D rendering
	m_projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, m_screenNear, m_screenDepth);

	// Initialize the world matrix to the identity matrix
	m_worldMatrix = XMMatrixIdentity();

	// Create an orthographic projection matrix for 2D rendering.
	m_orthoMatrix = XMMatrixOrthographicLH((float)m_screenWidth, (float)m_screenHeight, m_screenNear, m_screenDepth);

	return S_OK;
}

HRESULT D3D::CreateVisualCone()
{
	m_viewPort.TopLeftX = 0.f;
	m_viewPort.TopLeftY = 0.f;
	m_viewPort.Width = (FLOAT)m_screenWidth;
	m_viewPort.Height = (FLOAT)m_screenHeight;
	m_viewPort.MinDepth = 0.f;
	m_viewPort.MaxDepth = 1.f;

	m_scissorRect.top = 0;
	m_scissorRect.left = 0;
	m_scissorRect.right = m_screenWidth;
	m_scissorRect.bottom = m_screenHeight;

	return S_OK;
}

HRESULT D3D::CreateShadowVisualCone()
{
	m_viewPort.TopLeftX = 0.f;
	m_viewPort.TopLeftY = 0.f;
	// 光はワイドににゃらんのよ！
	m_viewPort.Width = FLOAT((m_screenWidth + 0xff)&~0xff);
	m_viewPort.Height = FLOAT((m_screenHeight + 0xff)&~0xff);
	m_viewPort.MinDepth = 0.f;
	m_viewPort.MaxDepth = 1.f;

	m_scissorRect.top = 0;
	m_scissorRect.left = 0;
	m_scissorRect.right = FLOAT((m_screenWidth + 0xff)&~0xff);
	m_scissorRect.bottom = FLOAT((m_screenHeight + 0xff)&~0xff);

	return S_OK;
}

//ファクトリの作成
HRESULT D3D::CreateFactory() {
	HRESULT hr{};
	UINT flag{};

	//デバッグモードの場合はデバッグレイヤーを有効にする
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debug;
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()));
		if (FAILED(hr)) {
			return hr;
		}
		debug->EnableDebugLayer();
	}

	flag |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	//ファクトリの作成
	hr = CreateDXGIFactory2(flag, IID_PPV_ARGS(m_factory.GetAddressOf()));

	return S_OK;
}


HRESULT D3D::CreateDevice() {
	HRESULT hr{};

	//最初に見つかったアダプターを使用する
	hr = m_factory->EnumAdapters(0, (IDXGIAdapter**)m_adapter.GetAddressOf());

	if (SUCCEEDED(hr)) {
		//見つかったアダプタを使用してデバイスを作成する
		hr = D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device));
	}
	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
}

HRESULT D3D::CreateCommandQueue() {
	HRESULT hr{};
	D3D12_COMMAND_QUEUE_DESC command_queue_desc{};

	// コマンドキューを生成.
	command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	command_queue_desc.Priority = 0;
	command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	command_queue_desc.NodeMask = 0;
	hr = m_device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(m_commandQueue.GetAddressOf()));
	if (FAILED(hr)) {
		return hr;
	}

	//コマンドキュー用のフェンスの生成
	m_fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (m_fenceEvent == NULL) {
		return E_FAIL;
	}
	hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));

	return S_OK;
}

HRESULT D3D::CreateSwapChain() {
	HRESULT hr{};
	DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
	ComPtr<IDXGISwapChain> swap_chain{};

	swap_chain_desc.BufferDesc.Width = m_screenWidth;
	swap_chain_desc.BufferDesc.Height = m_screenHeight;
	swap_chain_desc.OutputWindow = m_windowHandle;
	if (m_fullScreen)
	{
		swap_chain_desc.Windowed = false;
	}
	else
	{
		swap_chain_desc.Windowed = true;
	}
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = RTV_NUM;//裏と表で２枚作りたいので2を指定する
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;


	hr = m_factory->CreateSwapChain(m_commandQueue.Get(), &swap_chain_desc, swap_chain.GetAddressOf());
	if (FAILED(hr)) {
		return hr;
	}

	hr = swap_chain->QueryInterface(m_swapChain.GetAddressOf());
	if (FAILED(hr)) {
		return hr;
	}

	//カレントのバックバッファのインデックスを取得する
	m_rtvIndex = m_swapChain->GetCurrentBackBufferIndex();

	return S_OK;
}

HRESULT D3D::CreateRenderTargetView() {
	HRESULT hr{};
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};

	//RTV用デスクリプタヒープの作成
	heap_desc.NumDescriptors = RTV_NUM;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heap_desc.NodeMask = 0;
	hr = m_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(m_dh_RTV.GetAddressOf()));
	if (FAILED(hr)) {
		return hr;
	}

	//レンダーターゲットを作成する処理
	UINT size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (UINT i = 0; i < RTV_NUM; ++i) {
		//スワップチェインからバッファを受け取る
		hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTarget[i]));
		if (FAILED(hr)) {
			return hr;
		}

		//RenderTargetViewを作成してヒープデスクリプタに登録
		m_RTV_handle[i] = m_dh_RTV->GetCPUDescriptorHandleForHeapStart();
		m_RTV_handle[i].ptr += size * i;
		m_device->CreateRenderTargetView(m_renderTarget[i].Get(), nullptr, m_RTV_handle[i]);
	}

	return S_OK;
}

HRESULT D3D::CreateDepthStencilBuffer() {
	HRESULT hr;

	//深度バッファ用のデスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc{};
	descriptor_heap_desc.NumDescriptors = 1;
	descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptor_heap_desc.NodeMask = 0;
	hr = m_device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&m_dh_DSV));
	if (FAILED(hr)) {
		return hr;
	}


	//深度バッファの作成
	D3D12_HEAP_PROPERTIES heap_properties{};
	D3D12_RESOURCE_DESC resource_desc{};
	D3D12_CLEAR_VALUE clear_value{};

	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap_properties.CreationNodeMask = 0;
	heap_properties.VisibleNodeMask = 0;

	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resource_desc.Width = m_screenWidth;
	resource_desc.Height = m_screenHeight;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 0;
	resource_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	resource_desc.Alignment = 0;

	clear_value.Format = DXGI_FORMAT_D32_FLOAT;
	clear_value.DepthStencil.Depth = 1.0f;
	clear_value.DepthStencil.Stencil = 0;

	hr = m_device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(&m_depthBuffer));
	if (FAILED(hr)) {
		return hr;
	}


	//深度バッファのビューの作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};

	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Texture2D.MipSlice = 0;
	dsv_desc.Flags = D3D12_DSV_FLAG_NONE;

	m_DSV_handle = m_dh_DSV->GetCPUDescriptorHandleForHeapStart();

	m_device->CreateDepthStencilView(m_depthBuffer.Get(), &dsv_desc, m_DSV_handle);

	return S_OK;
}

HRESULT D3D::CreateCommandList() {
	HRESULT hr;

	//コマンドアロケータの作成
	hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
	if (FAILED(hr)) {
		return hr;
	}

	//コマンドアロケータとバインドしてコマンドリストを作成する
	hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));

	return S_OK;
}

HRESULT D3D::WaitForPreviousFrame() {
	HRESULT hr;

	const UINT64 fence = m_flames;
	hr = m_commandQueue->Signal(m_fence.Get(), fence);
	if (FAILED(hr)) {
		return -1;
	}
	++m_flames;

	if (m_fence->GetCompletedValue() < fence) {
		hr = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
		if (FAILED(hr)) {
			return -1;
		}

		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	return S_OK;
}

//リソースの設定変更用
HRESULT D3D::SetResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, bool backBuffer) {
	D3D12_RESOURCE_BARRIER resource_barrier{};

	resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	if (!backBuffer)
	{
		resource_barrier.Transition.pResource = resource;
	}
	else
	{
		resource_barrier.Transition.pResource = m_renderTarget[m_rtvIndex].Get();
	}
	resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	resource_barrier.Transition.StateBefore = BeforeState;
	resource_barrier.Transition.StateAfter = AfterState;

	m_commandList->ResourceBarrier(1, &resource_barrier);
	return S_OK;
}

HRESULT D3D::ExecuteCommandList() {
	HRESULT hr;

	m_commandList->Close();

	ID3D12CommandList *const command_lists = m_commandList.Get();
	m_commandQueue->ExecuteCommandLists(1, &command_lists);

	//実行したコマンドの終了待ち
	WaitForPreviousFrame();


	hr = m_commandAllocator->Reset();
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	return hr;
}

void D3D::GetProjectionMatrix(XMMATRIX& projectionMatrix)
{
	projectionMatrix = m_projectionMatrix;
	return;
}


void D3D::GetWorldMatrix(XMMATRIX& worldMatrix)
{
	worldMatrix = m_worldMatrix;
	return;
}


void D3D::GetOrthoMatrix(XMMATRIX& orthoMatrix)
{
	orthoMatrix = m_orthoMatrix;
	return;
}

ID3D12Device* D3D::GetDevice()
{
	return m_device.Get();
}

ID3D12GraphicsCommandList* D3D::GetCmdList()
{
	return m_commandList.Get();
}

ID3D12DescriptorHeap* D3D::GetDepthStencilView()
{
	return m_dh_DSV.Get();
}

void D3D::SetBackBufferRenderTarget()
{
	// Bind the render target view and depth stencil buffer to the output render pipeline.
	m_commandList->OMSetRenderTargets(1, &m_RTV_handle[m_rtvIndex], TRUE, &m_dh_DSV->GetCPUDescriptorHandleForHeapStart());

	return;
}

void D3D::BeginScene(float red, float green, float blue, float alpha)
{
	float color[4];


	// Setup the color to clear the buffer to.
	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;

	m_commandList->ClearDepthStencilView(m_dh_DSV->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	m_commandList->ClearRenderTargetView(m_RTV_handle[m_rtvIndex], color, 0, nullptr);

	m_commandList->RSSetViewports(1, &m_viewPort);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);


	return;
}


void D3D::EndScene()
{
	HRESULT hr{};
	hr = m_swapChain->Present(1, 0);
	if (FAILED(hr)) {
		return;
	}

	//カレントのバックバッファのインデックスを取得する
	m_rtvIndex = m_swapChain->GetCurrentBackBufferIndex();

	return;
}
