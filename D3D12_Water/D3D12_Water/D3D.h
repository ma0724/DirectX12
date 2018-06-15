#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <d3d12shader.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <memory>


#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3d12.lib")


class D3D
{

private:
	static constexpr int RTV_NUM = 2;

	UINT64	m_flames;
	UINT	m_rtvIndex;
	int		m_screenWidth;
	int		m_screenHeight;
	HWND	m_windowHandle;
	bool    m_fullScreen;
	float	m_screenDepth;
	float	m_screenNear;

	Microsoft::WRL::ComPtr<IDXGIFactory4>				m_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter3>				m_adapter;
	Microsoft::WRL::ComPtr<ID3D12Device>				m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_commandQueue;
	HANDLE												m_fenceEvent;
	Microsoft::WRL::ComPtr<ID3D12Fence>					m_fence;
	Microsoft::WRL::ComPtr<IDXGISwapChain3>				m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_renderTarget[RTV_NUM];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_dh_RTV;
	D3D12_CPU_DESCRIPTOR_HANDLE							m_RTV_handle[RTV_NUM];
	Microsoft::WRL::ComPtr<ID3D12Resource>				m_depthBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_dh_DSV;
	D3D12_CPU_DESCRIPTOR_HANDLE							m_DSV_handle;
	D3D12_RECT											m_scissorRect;
	D3D12_VIEWPORT										m_viewPort;

	DirectX::XMMATRIX		m_projectionMatrix;
	DirectX::XMMATRIX		m_worldMatrix;
	DirectX::XMMATRIX		m_orthoMatrix;
private:
	HRESULT CreateMatrix();
	HRESULT CreateFactory();
	HRESULT CreateDevice();
	HRESULT CreateVisualCone();
	HRESULT CreateShadowVisualCone();
	HRESULT CreateCommandQueue();
	HRESULT CreateSwapChain();
	HRESULT CreateRenderTargetView();
	HRESULT CreateDepthStencilBuffer();
	HRESULT CreateCommandList();
	//HRESULT Render();
	//HRESULT PopulateCommandList();

public:
	D3D();
	~D3D();

	bool Initialize(int, int, bool, HWND, bool, float, float);
	void BeginScene(float, float, float, float);
	void EndScene();
	HRESULT SetResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState, bool backBuffer);
	HRESULT ExecuteCommandList();
	HRESULT WaitForPreviousFrame();

	ID3D12Device* GetDevice();
	ID3D12GraphicsCommandList* GetCmdList();

	void GetProjectionMatrix(DirectX::XMMATRIX&);
	void GetWorldMatrix(DirectX::XMMATRIX&);
	void GetOrthoMatrix(DirectX::XMMATRIX&);

	ID3D12DescriptorHeap* GetDepthStencilView();
	void SetBackBufferRenderTarget();

};

