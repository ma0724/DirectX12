
#include "TextureLoader.h"

#include "d3dx12.h"
#include <vector>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

template <typename T>
using SPtr = std::shared_ptr<T>;

namespace{
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw hr;
	}
}
struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };
typedef public std::unique_ptr<void, handle_closer> ScopedHandle;
inline HANDLE safe_handle(HANDLE h) { return (h == INVALID_HANDLE_VALUE) ? 0 : h; }

}

//-------------------
TextureLoaderX12::EventHandle::~EventHandle()
{
	CloseHandle(hdl);
}

//-------------------
TextureLoaderX12::TextureLoaderX12(ID3D12Device* dev, ID3D12CommandQueue* cmdq)
{
	ComPtr<ID3D12CommandAllocator> cmdalloc;
	ThrowIfFailed(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc)));
	
	ComPtr<ID3D12GraphicsCommandList> cmdlist;
	ThrowIfFailed(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), nullptr, IID_PPV_ARGS(&cmdlist)));
	//メモ
	//CreateCommandListを作成した時点で、GPUコマンドが追加されている
	//アロケータやリストをReset()する場合
	//追加されたコマンドをGPUに実行させた後にしないとエラー発生
	
	ComPtr<ID3D12Fence> fence;
	SPtr<EventHandle> fevent;
	ThrowIfFailed(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	

	HANDLE hdl = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (hdl != nullptr) {
		fevent.reset(new EventHandle(hdl));
	}
	else {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	pDev = dev;
	pQueue = cmdq;
	pCmdAlloc = cmdalloc;
	pCmdList = cmdlist;
	pFence = fence;
	pEvent = fevent;
	uFenceValue = 1;
	
	ThrowIfFailed(pCmdList->Close());
	ID3D12CommandList* ppCommandLists[] = { pCmdList.Get() };
	pQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	
	Wait();
	//GPU処理完了を待ってからReset!!
	//Releaseビルドやタイミングによってエラーが発生する場合
	//GPU処理完了を待たずに次のコマンドを作成している可能性が高い
	ThrowIfFailed(pCmdAlloc->Reset());
}

//-------------------
TextureLoaderX12::~TextureLoaderX12()
{
}


//-------------------
//HRESULT TextureLoaderX12::LoadDDSFileWait(const wchar_t* szFileName, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view)
//{
//	*texture = nullptr;
//	ComPtr<ID3D12Resource> tex;
//	ComPtr<ID3D12Resource> upload;
//
//	try{
//		ThrowIfFailed( pCmdList->Reset(pCmdAlloc.Get(), nullptr));
//		ThrowIfFailed( CreateDDSTextureFromFile(pDev.Get(), pCmdList.Get(),
//						szFileName, 0, false, &tex, &upload, view, nullptr) );
//
//		ThrowIfFailed( pCmdList->Close());
//		ID3D12CommandList* ppCommandLists[] = { pCmdList.Get() };
//		pQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
//		
//		Wait();
//		ThrowIfFailed(pCmdAlloc->Reset());
//	}catch(HRESULT hr){
//		return hr;
//	}
//
//	upload.Reset();
//	*texture = tex.Detach();
//	return S_OK;
//}
//-------------------
HRESULT TextureLoaderX12::LoadWICFileWait(const wchar_t* szFileName, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view)
{
	*texture = nullptr;
	ComPtr<ID3D12Resource> tex;
	ComPtr<ID3D12Resource> upload;

	try {
		ThrowIfFailed(pCmdList->Reset(pCmdAlloc.Get(), nullptr));

		ThrowIfFailed(
			DirectX::CreateWICTextureFromFile(pDev.Get(), pCmdList.Get(),
				szFileName, &tex, &upload, view)
			);

		ThrowIfFailed(pCmdList->Close());
		ID3D12CommandList* ppCommandLists[] = { pCmdList.Get() };
		pQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		Wait();
		ThrowIfFailed(pCmdAlloc->Reset());
	}
	catch (HRESULT hr) {
		return hr;
	}

	upload.Reset();
	*texture = tex.Detach();
	return S_OK;
}

void TextureLoaderX12::Wait()
{
	// Signal and increment the fence value.
	const UINT64 fence = uFenceValue;
	ThrowIfFailed(pQueue->Signal(pFence.Get(), fence));
	uFenceValue++;

	// Wait until the previous frame is finished.
	if (pFence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(pFence->SetEventOnCompletion(fence, pEvent->hdl));
		WaitForSingleObject(pEvent->hdl, INFINITE);
	}
}
HRESULT TextureLoaderX12::LoadDDSFile(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, const wchar_t* szFileName, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view, ID3D12Resource** up)
{
	*texture = nullptr;
	*up = nullptr;
	ComPtr<ID3D12Resource> tex;
	ComPtr<ID3D12Resource> upload;

	HRESULT hr = CreateDDSTextureFromFile(dev, cmd,
			szFileName, 0, false, &tex, &upload, view, nullptr);
	if(FAILED(hr))return hr;

	*up = upload.Detach();
	*texture = tex.Detach();
	return S_OK;
}
HRESULT TextureLoaderX12::LoadWICFile(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, const wchar_t* szFileName, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view, ID3D12Resource** up)
{
	*texture = nullptr;
	*up = nullptr;
	ComPtr<ID3D12Resource> tex;
	ComPtr<ID3D12Resource> upload;

	HRESULT hr = DirectX::CreateWICTextureFromFile(dev, cmd,
				szFileName, &tex, &upload, view);
	if (FAILED(hr))return hr;


	*up = upload.Detach();
	*texture = tex.Detach();
	return S_OK;
}



HRESULT TextureLoaderX12::LoadTGAFile(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, const wchar_t* szFileName, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view, ID3D12Resource** upload)
{
	// open the file
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
	ScopedHandle hFile(safe_handle(CreateFile2(szFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		OPEN_EXISTING,
		nullptr)));
#else
	ScopedHandle hFile(safe_handle(CreateFileW(fileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr)));
#endif

	if (!hFile)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// Get the file size
	LARGE_INTEGER FileSize = { 0 };

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
	FILE_STANDARD_INFO fileInfo;
	if (!GetFileInformationByHandleEx(hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	FileSize = fileInfo.EndOfFile;
#else
	GetFileSizeEx(hFile.get(), &FileSize);
#endif
	std::unique_ptr<uint8_t[]> data(new (std::nothrow) uint8_t[FileSize.LowPart]);
	if (!data)
	{
		return E_OUTOFMEMORY;
	}

	// read the data in
	DWORD BytesRead = 0;
	if (!ReadFile(hFile.get(),
		data.get(),
		FileSize.LowPart,
		&BytesRead,
		nullptr
		))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return LoadTGAMemory(dev,cmd,data.get(), FileSize.LowPart,texture,view,upload);
}
HRESULT TextureLoaderX12::LoadTGAMemory(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, const void* data, UINT size, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view, ID3D12Resource** upload)
{
	const uint8_t* filePtr = (const uint8_t*)data;

	// Skip first two bytes
	filePtr += 2;

	/*uint8_t imageTypeCode =*/ *filePtr++;

	// Ignore another 9 bytes
	filePtr += 9;

	uint16_t imageWidth = *(uint16_t*)filePtr;
	filePtr += sizeof(uint16_t);
	uint16_t imageHeight = *(uint16_t*)filePtr;
	filePtr += sizeof(uint16_t);
	uint8_t bitCount = *filePtr++;

	// Ignore another byte
	filePtr++;

	std::unique_ptr<uint32_t[]> tga_image(new uint32_t[imageWidth * imageHeight]);
	//uint32_t* formattedData = new uint32_t[imageWidth * imageHeight];
	uint32_t* iter = tga_image.get();

	uint8_t numChannels = bitCount / 8;
	uint32_t numBytes = imageWidth * imageHeight * numChannels;

	switch (numChannels)
	{
	default:
		break;
	case 3:
		for (uint32_t byteIdx = 0; byteIdx < numBytes; byteIdx += 3)
		{
			*iter++ = 0xff000000 | filePtr[0] << 16 | filePtr[1] << 8 | filePtr[2];
			filePtr += 3;
		}
		break;
	case 4:
		for (uint32_t byteIdx = 0; byteIdx < numBytes; byteIdx += 4)
		{
			*iter++ = filePtr[3] << 24 | filePtr[0] << 16 | filePtr[1] << 8 | filePtr[2];
			filePtr += 4;
		}
		break;
	}

	//上下反転
	std::unique_ptr<uint32_t[]> tga_imageR(new uint32_t[imageWidth * imageHeight]);
	for(UINT h=0;h<imageHeight;++h){
		memcpy(&tga_imageR[h*imageWidth],&tga_image[(imageHeight-1-h)*imageWidth],imageWidth*4);
	}

	CInfo cinfo;
	cinfo.width = imageWidth;
	cinfo.height = imageHeight;
	cinfo.rowpitch = imageWidth*numChannels;
	cinfo.slicepitch = cinfo.rowpitch*imageHeight;
	cinfo.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	cinfo.image = tga_imageR.get();
	cinfo.image_size = numBytes;
	return CreateTexture(dev,cmd,cinfo,texture,view,upload);

}

HRESULT TextureLoaderX12::CreateTexture(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd, const CInfo& cinfo, ID3D12Resource** texture, D3D12_SHADER_RESOURCE_VIEW_DESC* view, ID3D12Resource** upload)
{
 	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = cinfo.width;
	texDesc.Height = cinfo.height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = cinfo.format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	ComPtr<ID3D12Resource> tex, up;
	HRESULT hr = dev->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&tex));
	if (FAILED(hr))return hr;

	tex->SetName(L"Texture");
	{//upload

		D3D12_HEAP_PROPERTIES HeapProps = {};
		HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask = 1;
		HeapProps.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC BufferDesc;
		BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		BufferDesc.Alignment = 0;
		BufferDesc.Width = cinfo.image_size;
		BufferDesc.Height = 1;
		BufferDesc.DepthOrArraySize = 1;
		BufferDesc.MipLevels = 1;
		BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		BufferDesc.SampleDesc.Count = 1;
		BufferDesc.SampleDesc.Quality = 0;
		BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		hr = dev->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
			&BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&up));
		if (FAILED(hr))return hr;
	}
	// 今回はUpdateSubresourceで更新する前にバリアを張っておかないと怒られる
	// 要はシステムがなんのデータかわからないって
	D3D12_RESOURCE_BARRIER BarrierDesc;
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = tex.Get();
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	cmd->ResourceBarrier(1, &BarrierDesc);

	D3D12_SUBRESOURCE_DATA subdata;
	subdata.pData = cinfo.image;
	subdata.RowPitch = cinfo.rowpitch;
	subdata.SlicePitch = cinfo.slicepitch;

	UpdateSubresources(cmd, tex.Get(), up.Get(), 0, 0, 1, &subdata);

	//D3D12_RESOURCE_BARRIER BarrierDesc;
	//BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//BarrierDesc.Transition.pResource = tex.Get();
	//BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	//BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	//BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	//cmd->ResourceBarrier(1, &BarrierDesc);


	if (texture)*texture = tex.Detach();
	if (upload)*upload = up.Detach();
	if (view) {
		view->Format = cinfo.format;
		view->Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		view->ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		view->Texture2D.MipLevels = -1;
		view->Texture2D.MostDetailedMip = 0;
		view->Texture2D.PlaneSlice = 0;
		view->Texture2D.ResourceMinLODClamp = 0;
	}

	return S_OK;

}




