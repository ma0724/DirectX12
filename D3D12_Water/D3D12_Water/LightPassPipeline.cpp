#include "LightPassPipeline.h"
#include <tchar.h>

using namespace DirectX;
using namespace Microsoft::WRL;

LightPassPipeline::LightPassPipeline()
{
}


LightPassPipeline::~LightPassPipeline()
{
}

bool LightPassPipeline::Initialize(ID3D12Device* inDevice, HWND inHwnd, int inMatrixCount)
{
	bool result;
	// リソースを確保する
	m_constantBufferMatrix.resize(inMatrixCount);
	m_dh_constantBufferMatrix.resize(inMatrixCount);

	// InitializePipeline
	result = InitializeShader(inDevice, inHwnd, L"../Engine/light.vs", L"../Engine/light.ps");
	if (!result)
	{
		return false;
	}
	return true;
}

bool LightPassPipeline::InitializeShader(ID3D12Device* inDevice, HWND inHwnd, WCHAR* inWcharVS, WCHAR* inWcharPS)
{
	HRESULT result;
	result = CreateRootSignature(inDevice);
	if (FAILED(result))
	{
		return false;
	}
	result = CreatePipelineStateObject(inDevice);
	if (FAILED(result))
	{
		return false;
	}

	D3D12_HEAP_PROPERTIES heapProperties{};
	D3D12_RESOURCE_DESC   resourceDesc{};
	// Create Matrix's ConstantBuffer
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = 256; // ByteWidth sizeof(MatrixBufferType)
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;// アンチェリ関連のはず
	{
		for (auto& c : m_constantBufferMatrix)
		{
			result = inDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(c.GetAddressOf()));
			if (FAILED(result))
			{
				return false;
			}
		}
	}
	// Create LightBuffer's ConstantBuffer
	// ByteWidth sizeof(LightBufferType)
	{
		result = inDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBufferLight));
		if (FAILED(result))
		{
			return false;
		}
	}
	// Create matrix's DiscriptorHeap
	{
		D3D12_DESCRIPTOR_HEAP_DESC dh_mt_desc;
		dh_mt_desc.NumDescriptors = 1;	// デスクリプタの数
		dh_mt_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;    //今回はシェーダリソースで使うよ
		dh_mt_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//可視化
		dh_mt_desc.NodeMask = 0;

		for (auto& dh : m_dh_constantBufferMatrix)
		{
			result = inDevice->CreateDescriptorHeap(&dh_mt_desc, IID_PPV_ARGS(&dh));
			if (FAILED(result))
			{
				return false;
			}
		}

		// Create constantbuffer view
		D3D12_CPU_DESCRIPTOR_HANDLE handleMatCBV{};
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc{};
		unsigned int U32Size = static_cast<unsigned int>(sizeof(MatrixBufferType) + (256 - sizeof(MatrixBufferType) % 256) % 256);
		for (int i = 0; i < m_constantBufferMatrix.size(); ++i)
		{
			cbv_desc.BufferLocation = m_constantBufferMatrix[i].Get()->GetGPUVirtualAddress();
			cbv_desc.SizeInBytes = U32Size;
			//unsigned int dhSize = inDevice->GetDescriptorHandleIncrementSize(dh_mt_desc.Type);//デスクリプタのサイズ

			inDevice->CreateConstantBufferView(&cbv_desc, m_dh_constantBufferMatrix[i].Get()->GetCPUDescriptorHandleForHeapStart());
		}
	}

	return true;
}

HRESULT LightPassPipeline::CreateRootSignature(ID3D12Device* inDevice) {
	HRESULT hr{};

	D3D12_DESCRIPTOR_RANGE		range[2]{};				// DiscriptorTableが持っている 複数持っている もちろんレジスタの種類が変わればrangeも変わってくる
	D3D12_ROOT_PARAMETER		root_parameters[3]{};	// ルートシグネチャを作成する前に設定しなけばならないパラメーター
	D3D12_ROOT_SIGNATURE_DESC	root_signature_desc{};  // ルートシグネチャの設定
	D3D12_STATIC_SAMPLER_DESC	sampler_desc{};
	ComPtr<ID3DBlob> blob{};

	range[1].NumDescriptors = 1;
	range[1].BaseShaderRegister = 0;// レジスタ番号
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[1].OffsetInDescriptorsFromTableStart = 0;

	root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	root_parameters[0].DescriptorTable.NumDescriptorRanges = 1;
	root_parameters[0].DescriptorTable.pDescriptorRanges = &range[1];

	root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	root_parameters[1].Descriptor.ShaderRegister = 1;
	root_parameters[1].Descriptor.RegisterSpace = 0;

	range[0].NumDescriptors = 1;
	range[0].BaseShaderRegister = 0;// レジスタ番号
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].OffsetInDescriptorsFromTableStart = 0;

	root_parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	root_parameters[2].DescriptorTable.NumDescriptorRanges = 1;
	root_parameters[2].DescriptorTable.pDescriptorRanges = &range[0];


	//静的サンプラの設定
	//サンプラ
	sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 16;
	sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler_desc.MinLOD = 0.0f;
	sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
	sampler_desc.ShaderRegister = 0;
	sampler_desc.RegisterSpace = 0;
	sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


	root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	root_signature_desc.NumParameters = _countof(root_parameters);
	root_signature_desc.pParameters = root_parameters;
	root_signature_desc.NumStaticSamplers = 1;
	root_signature_desc.pStaticSamplers = &sampler_desc;

	// RootSignaturを作成するのに必要なバッファを確保しTableの情報をシリアライズ
	hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
	if (FAILED(hr)) {
		return hr;
	}
	// RootSignaturの作成
	hr = inDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

	return hr;
}

HRESULT LightPassPipeline::CreatePipelineStateObject(ID3D12Device* inDevice) {
	HRESULT hr;
	ComPtr<ID3DBlob> vertex_shader{};
	ComPtr<ID3DBlob> pixel_shader{};

#if defined(_DEBUG)
	UINT compile_flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compile_flag = 0;
#endif

	hr = D3DCompileFromFile(_T("light.hlsl"), nullptr, nullptr, "LightVertexShader", "vs_5_0", compile_flag, 0, vertex_shader.GetAddressOf(), nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	hr = D3DCompileFromFile(_T("light.hlsl"), nullptr, nullptr, "LightPixelShader", "ps_5_0", compile_flag, 0, pixel_shader.GetAddressOf(), nullptr);
	if (FAILED(hr)) {
		return hr;
	}


	// 頂点レイアウト.
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc{};

	//シェーダーの設定
	pipeline_state_desc.VS.pShaderBytecode = vertex_shader->GetBufferPointer();
	pipeline_state_desc.VS.BytecodeLength = vertex_shader->GetBufferSize();
	pipeline_state_desc.PS.pShaderBytecode = pixel_shader->GetBufferPointer();
	pipeline_state_desc.PS.BytecodeLength = pixel_shader->GetBufferSize();


	//インプットレイアウトの設定
	pipeline_state_desc.InputLayout.pInputElementDescs = InputElementDesc;
	pipeline_state_desc.InputLayout.NumElements = _countof(InputElementDesc);


	//サンプル系の設定
	pipeline_state_desc.SampleDesc.Count = 1;
	pipeline_state_desc.SampleDesc.Quality = 0;
	pipeline_state_desc.SampleMask = UINT_MAX;

	//レンダーターゲットの設定
	pipeline_state_desc.NumRenderTargets = 1;
	pipeline_state_desc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;


	//三角形に設定
	pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//ルートシグネチャ
	pipeline_state_desc.pRootSignature = m_rootSignature.Get();


	//ラスタライザステートの設定
	pipeline_state_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//D3D12_CULL_MODE_BACK;
	pipeline_state_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pipeline_state_desc.RasterizerState.FrontCounterClockwise = FALSE;
	pipeline_state_desc.RasterizerState.DepthBias = 0;
	pipeline_state_desc.RasterizerState.DepthBiasClamp = 0;
	pipeline_state_desc.RasterizerState.SlopeScaledDepthBias = 0;
	pipeline_state_desc.RasterizerState.DepthClipEnable = TRUE;
	pipeline_state_desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	pipeline_state_desc.RasterizerState.AntialiasedLineEnable = FALSE;
	pipeline_state_desc.RasterizerState.MultisampleEnable = FALSE;


	//ブレンドステートの設定
	for (int i = 0; i < _countof(pipeline_state_desc.BlendState.RenderTarget); ++i) {
		pipeline_state_desc.BlendState.RenderTarget[i].BlendEnable = FALSE;
		pipeline_state_desc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
		pipeline_state_desc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
		pipeline_state_desc.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
		pipeline_state_desc.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
		pipeline_state_desc.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
		pipeline_state_desc.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		pipeline_state_desc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		pipeline_state_desc.BlendState.RenderTarget[i].LogicOpEnable = FALSE;
		pipeline_state_desc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_CLEAR;
	}
	pipeline_state_desc.BlendState.AlphaToCoverageEnable = FALSE;
	pipeline_state_desc.BlendState.IndependentBlendEnable = FALSE;


	//デプスステンシルステートの設定
	pipeline_state_desc.DepthStencilState.DepthEnable = TRUE;								//深度テストあり
	pipeline_state_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	pipeline_state_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipeline_state_desc.DepthStencilState.StencilEnable = FALSE;							//ステンシルテストなし
	pipeline_state_desc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	pipeline_state_desc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	pipeline_state_desc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	pipeline_state_desc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipeline_state_desc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	pipeline_state_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	hr = inDevice->CreateGraphicsPipelineState(&pipeline_state_desc, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(hr)) {
		return hr;
	}
	return hr;
}

bool LightPassPipeline::Render(ID3D12GraphicsCommandList* inCmdList, int inMatNum, int inDrawindexCount, DirectX::XMMATRIX inWorldMatrix, DirectX::XMMATRIX inViewMatrix, DirectX::XMMATRIX inProjectionMatrix, ID3D12DescriptorHeap* inTexture, DirectX::XMFLOAT3 inLightDirection, DirectX::XMFLOAT4 inAmbientColor,
	DirectX::XMFLOAT4 inDiffuseColor)
{
	bool result;
	// Set the shader parameters that it will use for rendering.
	result = SetShaderParameters(inCmdList, inMatNum, inWorldMatrix, inViewMatrix, inProjectionMatrix, inTexture, inLightDirection, inAmbientColor,
		inDiffuseColor);
	if (!result)
	{
		return false;
	}

	// Now render the prepared buffers with the shader.
	RenderShader(inCmdList, inDrawindexCount);
}

bool LightPassPipeline::SetShaderParameters(ID3D12GraphicsCommandList* inCmdList, int inMatNum, DirectX::XMMATRIX inWorldMatrix, DirectX::XMMATRIX inViewMatrix, DirectX::XMMATRIX inProjectionMatrix, ID3D12DescriptorHeap* inTexture, DirectX::XMFLOAT3 inLightDirection,
	DirectX::XMFLOAT4 inAmbientColor, DirectX::XMFLOAT4 inDiffuseColor)
{
	HRESULT result;
	// 転置行列を生成する
	XMMATRIX transWorldmatrix = XMMatrixTranspose(inWorldMatrix);
	XMMATRIX transViewMatrix = XMMatrixTranspose(inViewMatrix);
	XMMATRIX transProjectionmatrix = XMMatrixTranspose(inProjectionMatrix);

	// write data
	MatrixBufferType* matBuff{};
	LightBufferType* lightBuff{};
	result = m_constantBufferMatrix[inMatNum]->Map(0, nullptr, (void**)&matBuff);
	if (FAILED(result))
	{
		return false;
	}
	matBuff->world = transWorldmatrix;
	matBuff->view = transViewMatrix;
	matBuff->projection = transProjectionmatrix;

	m_constantBufferMatrix[inMatNum]->Unmap(0, nullptr);
	matBuff = nullptr;

	result = m_constantBufferLight->Map(0, nullptr, (void**)&lightBuff);
	if (FAILED(result))
	{
		return false;
	}
	lightBuff->ambientColor = inAmbientColor;
	lightBuff->diffuseColor = inDiffuseColor;
	lightBuff->lightDirection = inLightDirection;

	m_constantBufferLight->Unmap(0, nullptr);
	lightBuff = nullptr;

	// Set up to PSO
	inCmdList->SetPipelineState(m_pipelineState.Get());
	// Set up to rootsignature
	inCmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	//定数バッファをシェーダのレジスタにセット
	ID3D12DescriptorHeap* matrix_heaps[] = { m_dh_constantBufferMatrix[inMatNum].Get() };
	inCmdList->SetDescriptorHeaps(_countof(matrix_heaps), matrix_heaps);
	//ルートパラメータに合わせてるのを忘れないでね
	inCmdList->SetGraphicsRootDescriptorTable(0, m_dh_constantBufferMatrix[inMatNum].Get()->GetGPUDescriptorHandleForHeapStart());// コマンドリストに設定

	inCmdList->SetGraphicsRootConstantBufferView(1, m_constantBufferLight->GetGPUVirtualAddress());

	////テクスチャをシェーダのレジスタにセット
	ID3D12DescriptorHeap* tex_heaps[] = { inTexture };
	inCmdList->SetDescriptorHeaps(_countof(tex_heaps), tex_heaps);
	inCmdList->SetGraphicsRootDescriptorTable(2, inTexture->GetGPUDescriptorHandleForHeapStart());

	return true;
}

void LightPassPipeline::RenderShader(ID3D12GraphicsCommandList* inCmdList, int inDrawindexCount)
{
	// 描画
	inCmdList->DrawInstanced(inDrawindexCount, 1, 0, 0);
}