//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//--------------------------------------------------------------------------------------
//
// Functions for loading a DDS texture and creating a Direct3D runtime resource for it
//
// Note these functions are useful as a light-weight runtime loader for DDS files. For
// a full-featured DDS file reader, writer, and texture processing pipeline see
// the 'Texconv' sample and the 'DirectXTex' library.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

//custom http://zerogram.info
//D3D12対応　Upload用中間リソースを受け取る引数追加
//GPU転送終了まで中間リソースを保持しておく

#pragma once

#include <d3d12.h>

#pragma warning(push)
#pragma warning(disable : 4005)
#include <stdint.h>
#pragma warning(pop)

enum DDS_ALPHA_MODE
{
    DDS_ALPHA_MODE_UNKNOWN       = 0,
    DDS_ALPHA_MODE_STRAIGHT      = 1,
    DDS_ALPHA_MODE_PREMULTIPLIED = 2,
    DDS_ALPHA_MODE_OPAQUE        = 3,
    DDS_ALPHA_MODE_CUSTOM        = 4,
};

HRESULT __cdecl CreateDDSTextureFromMemory( _In_ ID3D12Device* d3dDevice,
												_In_ ID3D12GraphicsCommandList* cmdList,
                                                _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                                _In_ size_t ddsDataSize,
                                                _In_ size_t maxsize,
                                                _In_ bool forceSRGB,
                                                _Outptr_opt_ ID3D12Resource** texture,
												_Outptr_opt_ ID3D12Resource** upload,
                                                _Out_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
                                                _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
                                            );

HRESULT __cdecl CreateDDSTextureFromFile( _In_ ID3D12Device* d3dDevice,
											_In_ ID3D12GraphicsCommandList* cmdList,
                                            _In_z_ const wchar_t* szFileName,
                                            _In_ size_t maxsize,
                                            _In_ bool forceSRGB,
                                            _Outptr_opt_ ID3D12Resource** texture,
											_Outptr_opt_ ID3D12Resource** upload,
											_Out_ D3D12_SHADER_RESOURCE_VIEW_DESC* textureView,
                                            _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
                                            );

size_t BitsPerPixel(_In_ DXGI_FORMAT fmt);
