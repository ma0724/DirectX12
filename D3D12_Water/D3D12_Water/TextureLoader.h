#pragma once

#include <wrl.h>
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include <memory>

// ==========================
//	DDS関連は消しています
//  そしてバリアの張り方をアレンジしてます
//  今回はやむを得ない
// ==========================

class TextureLoaderX12
{
public:
	TextureLoaderX12(ID3D12Device* dev, ID3D12CommandQueue* cmdq);
	~TextureLoaderX12();

	// ファイルからテクスチャリソース構築　GPUコマンド実行＆待ち
	// textureにテクスチャリソース　viewにSRV作成
	//HRESULT LoadDDSFileWait(const wchar_t* szFileName, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view);
	HRESULT LoadWICFileWait(const wchar_t* szFileName, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view);
	
	
	//テクスチャ読み込み　static関数版
	//テクスチャリソースはD3D12_HEAP_TYPE_DEFAULTで作成
	//アップロード用の中間リソースを作成、GPUの転送処理終了まで保持しておく
	static HRESULT LoadDDSFile(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, const wchar_t* szFileName, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view, ID3D12Resource** upload);
	static HRESULT LoadWICFile(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, const wchar_t* szFileName, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view, ID3D12Resource** upload);
	static HRESULT LoadTGAFile(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, const wchar_t* szFileName, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view, ID3D12Resource** upload);
	static HRESULT LoadTGAMemory(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, const void* data, UINT size, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view, ID3D12Resource** upload);

	//簡易テクスチャ作成
	//フォーマットと画像データからテクスチャリソース生成
	struct CInfo{
		UINT width;
		UINT height;
		DXGI_FORMAT format;
		UINT rowpitch;
		UINT slicepitch;
		const void* image;
		UINT image_size;
	};
	static HRESULT CreateTexture(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, const CInfo& cinfo,ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view, ID3D12Resource** upload);


private:
	TextureLoaderX12() = delete;
	TextureLoaderX12(const TextureLoaderX12&) = delete;
	TextureLoaderX12(TextureLoaderX12&&) = delete;
	const TextureLoaderX12& operator=(const TextureLoaderX12&) = delete;
	TextureLoaderX12& operator=(TextureLoaderX12&&) = delete;

	struct EventHandle
	{
		EventHandle() {}
		EventHandle(HANDLE h) : hdl(h) {}
		~EventHandle();

		EventHandle(const EventHandle&) = delete;
		EventHandle(EventHandle&& m) = delete;
		const EventHandle& EventHandle::operator=(const EventHandle& v) = delete;
		const EventHandle& EventHandle::operator=(EventHandle&& v) = delete;

		HANDLE hdl = nullptr;
	};

	Microsoft::WRL::ComPtr<ID3D12Device> pDev;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> pQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCmdAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCmdList;
	
	Microsoft::WRL::ComPtr<ID3D12Fence> pFence;
	std::shared_ptr<EventHandle> pEvent;
	UINT64 uFenceValue;

	void Wait();

};