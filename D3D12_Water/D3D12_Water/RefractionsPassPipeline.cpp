#include "RefractionsPassPipeline.h"
#include <tchar.h>

using namespace DirectX;
using namespace Microsoft::WRL;


RefractionsPassPipeline::RefractionsPassPipeline()
{
}


RefractionsPassPipeline::~RefractionsPassPipeline()
{
}


bool RefractionsPassPipeline::Initialize(ID3D12Device* inDevice, HWND inHwnd)
{
	bool result;

	// InitializePipeline
	result = InitializeShader(inDevice, inHwnd, L"../Engine/light.vs", L"../Engine/light.ps");
	if (!result)
	{
		return false;
	}
	return true;
}

bool RefractionsPassPipeline::InitializeShader(ID3D12Device* inDevice, HWND inHwnd, WCHAR* inWcharVS, WCHAR* inWcharPS)
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
	resourceDesc.SampleDesc.Quality = 0;// �A���`�F���֘A�̂͂�
	{
		result = inDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBufferMatrix));
		if (FAILED(result))
		{
			return false;
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
	// Create Plane constant Buffer
	{
		result = inDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBufferClipPlane));
		if (FAILED(result))
		{
			return false;
		}
	}

	return true;
}

HRESULT RefractionsPassPipeline::CreateRootSignature(ID3D12Device* inDevice) {
	HRESULT hr{};

	D3D12_DESCRIPTOR_RANGE		range{};				// DiscriptorTable�������Ă��� ���������Ă��� ������񃌃W�X�^�̎�ނ��ς���range���ς���Ă���
	D3D12_ROOT_PARAMETER		root_parameters[4]{};	// ���[�g�V�O�l�`�����쐬����O�ɐݒ肵�Ȃ��΂Ȃ�Ȃ��p�����[�^�[
	D3D12_ROOT_SIGNATURE_DESC	root_signature_desc{};  // ���[�g�V�O�l�`���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC	sampler_desc{};
	ComPtr<ID3DBlob> blob{};

	root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	root_parameters[0].Descriptor.ShaderRegister = 0;
	root_parameters[0].Descriptor.RegisterSpace = 0;

	root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	root_parameters[1].Descriptor.ShaderRegister = 1;
	root_parameters[1].Descriptor.RegisterSpace = 0;

	range.NumDescriptors = 1;
	range.BaseShaderRegister = 0;// ���W�X�^�ԍ�
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range.OffsetInDescriptorsFromTableStart = 0;

	root_parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	root_parameters[2].DescriptorTable.NumDescriptorRanges = 1;
	root_parameters[2].DescriptorTable.pDescriptorRanges = &range;

	root_parameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	root_parameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	root_parameters[3].Descriptor.ShaderRegister = 2;
	root_parameters[3].Descriptor.RegisterSpace = 0;

	//�ÓI�T���v���̐ݒ�
	//�T���v��
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

	// RootSignatur���쐬����̂ɕK�v�ȃo�b�t�@���m�ۂ�Table�̏����V���A���C�Y
	hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
	if (FAILED(hr)) {
		return hr;
	}
	// RootSignatur�̍쐬
	hr = inDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

	return hr;
}

HRESULT RefractionsPassPipeline::CreatePipelineStateObject(ID3D12Device* inDevice) {
	HRESULT hr;
	ComPtr<ID3DBlob> vertex_shader{};
	ComPtr<ID3DBlob> pixel_shader{};

#if defined(_DEBUG)
	UINT compile_flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compile_flag = 0;
#endif

	hr = D3DCompileFromFile(_T("refraction.hlsl"), nullptr, nullptr, "RefractionVertexShader", "vs_5_0", compile_flag, 0, vertex_shader.GetAddressOf(), nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	hr = D3DCompileFromFile(_T("refraction.hlsl"), nullptr, nullptr, "RefractionPixelShader", "ps_5_0", compile_flag, 0, pixel_shader.GetAddressOf(), nullptr);
	if (FAILED(hr)) {
		return hr;
	}


	// ���_���C�A�E�g.
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc{};

	//�V�F�[�_�[�̐ݒ�
	pipeline_state_desc.VS.pShaderBytecode = vertex_shader->GetBufferPointer();
	pipeline_state_desc.VS.BytecodeLength = vertex_shader->GetBufferSize();
	pipeline_state_desc.PS.pShaderBytecode = pixel_shader->GetBufferPointer();
	pipeline_state_desc.PS.BytecodeLength = pixel_shader->GetBufferSize();


	//�C���v�b�g���C�A�E�g�̐ݒ�
	pipeline_state_desc.InputLayout.pInputElementDescs = InputElementDesc;
	pipeline_state_desc.InputLayout.NumElements = _countof(InputElementDesc);


	//�T���v���n�̐ݒ�
	pipeline_state_desc.SampleDesc.Count = 1;
	pipeline_state_desc.SampleDesc.Quality = 0;
	pipeline_state_desc.SampleMask = UINT_MAX;

	//�����_�[�^�[�Q�b�g�̐ݒ�
	pipeline_state_desc.NumRenderTargets = 1;
	pipeline_state_desc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;


	//�O�p�`�ɐݒ�
	pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//���[�g�V�O�l�`��
	pipeline_state_desc.pRootSignature = m_rootSignature.Get();


	//���X�^���C�U�X�e�[�g�̐ݒ�
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


	//�u�����h�X�e�[�g�̐ݒ�
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


	//�f�v�X�X�e���V���X�e�[�g�̐ݒ�
	pipeline_state_desc.DepthStencilState.DepthEnable = TRUE;								//�[�x�e�X�g����
	pipeline_state_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	pipeline_state_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipeline_state_desc.DepthStencilState.StencilEnable = FALSE;							//�X�e���V���e�X�g�Ȃ�
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

bool RefractionsPassPipeline::Render(ID3D12GraphicsCommandList* inCmdList, int inDrawindexCount, DirectX::XMMATRIX inProjectionMatrix, DirectX::XMMATRIX inWorldMatrix, DirectX::XMMATRIX inViewMatrix, ID3D12DescriptorHeap* inTexture, DirectX::XMFLOAT3 inLightDirection, DirectX::XMFLOAT4 inAmbientColor,
	DirectX::XMFLOAT4 inDiffuseColor, DirectX::XMFLOAT4 clipPlane)
{
	bool result;
	// Set the shader parameters that it will use for rendering.
	result = SetShaderParameters(inCmdList, inWorldMatrix, inViewMatrix, inProjectionMatrix, inTexture, inLightDirection, inAmbientColor,
		inDiffuseColor, clipPlane);
	if (!result)
	{
		return false;
	}

	// Now render the prepared buffers with the shader.
	RenderShader(inCmdList, inDrawindexCount);
}

bool RefractionsPassPipeline::SetShaderParameters(ID3D12GraphicsCommandList* inCmdList, DirectX::XMMATRIX inWorldMatrix, DirectX::XMMATRIX inViewMatrix, DirectX::XMMATRIX inProjectionMatrix, ID3D12DescriptorHeap* inTexture, DirectX::XMFLOAT3 inLightDirection,
	DirectX::XMFLOAT4 inAmbientColor, DirectX::XMFLOAT4 inDiffuseColor, XMFLOAT4 clipPlane)
{
	HRESULT result;
	// �]�u�s��𐶐�����
	XMMATRIX transWorldmatrix = XMMatrixTranspose(inWorldMatrix);
	XMMATRIX transViewMatrix = XMMatrixTranspose(inViewMatrix);
	XMMATRIX transProjectionmatrix = XMMatrixTranspose(inProjectionMatrix);

	// write data
	MatrixBufferType* matBuff{};
	LightBufferType* lightBuff{};
	ClipPlaneBufferType* clipBuff{};
	result = m_constantBufferMatrix->Map(0, nullptr, (void**)&matBuff);
	if (FAILED(result))
	{
		return false;
	}
	matBuff->projection = transProjectionmatrix;
	matBuff->view = transViewMatrix;
	matBuff->world = transWorldmatrix;
	m_constantBufferMatrix->Unmap(0, nullptr);
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

	result = m_constantBufferClipPlane->Map(0, nullptr, (void**)&clipBuff);
	if (FAILED(result))
	{
		return false;
	}
	clipBuff->clipPlane = clipPlane;
	m_constantBufferClipPlane->Unmap(0, nullptr);
	clipBuff = nullptr;

	// Set up to PSO
	inCmdList->SetPipelineState(m_pipelineState.Get());
	// Set up to rootsignature
	inCmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	//�萔�o�b�t�@���V�F�[�_�̃��W�X�^�ɃZ�b�g
	inCmdList->SetGraphicsRootConstantBufferView(0, m_constantBufferMatrix->GetGPUVirtualAddress());
	inCmdList->SetGraphicsRootConstantBufferView(1, m_constantBufferLight->GetGPUVirtualAddress());
	inCmdList->SetGraphicsRootConstantBufferView(3, m_constantBufferClipPlane->GetGPUVirtualAddress());
	////�e�N�X�`�����V�F�[�_�̃��W�X�^�ɃZ�b�g
	ID3D12DescriptorHeap* tex_heaps[] = { inTexture };
	inCmdList->SetDescriptorHeaps(_countof(tex_heaps), tex_heaps);
	inCmdList->SetGraphicsRootDescriptorTable(2, inTexture->GetGPUDescriptorHandleForHeapStart());

	return true;
}

void RefractionsPassPipeline::RenderShader(ID3D12GraphicsCommandList* inCmdList, int inDrawindexCount)
{
	// �`��
	inCmdList->DrawInstanced(inDrawindexCount, 1, 0, 0);
}