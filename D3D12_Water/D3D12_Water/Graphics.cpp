#include "Graphics.h"
#include <tchar.h>
#include <DirectXMath.h>

using namespace DirectX;

Graphics::Graphics()
{
}


Graphics::~Graphics()
{
}

bool Graphics::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
	bool result;

	// CreateDirect3D and Initialize
	// 初期化だけしてます
	m_d3d = new D3D;
	m_d3d->Initialize(screenWidth, screenHeight, true, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);

	// Create the camera Object
	// といってもこいつはview行列を持ってるだけにすぎない
	m_camera = new Camera;
	if (!m_camera)
	{
		return false;
	}

	// Create the ground model Object
	// DDSテクスチャを読み込んで頂点情報とインデックス情報があるだけです
	m_GroundModel = new Model;
	result = m_GroundModel->Initialize(m_d3d->GetDevice(), m_d3d->GetCmdList(), _T("../Data/ground01.dds"), "../Data/ground.txt", m_d3d);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the ground model object.", L"Error", MB_OK);
		return false;
	}
	m_WallModel = new Model;
	result = m_WallModel->Initialize(m_d3d->GetDevice(), m_d3d->GetCmdList(), _T("../Data/wall01.dds"), "../Data/wall.txt", m_d3d);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the ground model object.", L"Error", MB_OK);
		return false;
	}
	m_BathModel = new Model;
	result = m_BathModel->Initialize(m_d3d->GetDevice(), m_d3d->GetCmdList(), _T("../Data/marble01.dds"), "../Data/bath.txt", m_d3d);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the ground model object.", L"Error", MB_OK);
		return false;
	}
	m_WaterModel = new Model;
	result = m_WaterModel->Initialize(m_d3d->GetDevice(), m_d3d->GetCmdList(), _T("../Data/water01.dds"), "../Data/water.txt", m_d3d);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the ground model object.", L"Error", MB_OK);
		return false;
	}

	// Initialize the light object and Create
	m_light = new Light;
	if (!m_light)
	{
		return false;
	}
	m_light->SetAmbientColor(0.15f, 0.15f, 0.15f, 1.0f);
	m_light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_light->SetDirection(0.0f, -1.0f, 0.5f);

	// 屈折用のレンダーテクスチャを作成する
	m_RefractionTexture = new RenderTexture;
	if (!m_RefractionTexture)
	{
		return false;
	}
	// Initialize the refraction render to texture object.
	result = m_RefractionTexture->Initialize(m_d3d->GetDevice(), screenWidth, screenHeight);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the refraction render to texture object.", L"Error", MB_OK);
		return false;
	}
	// 反射用のレンダーテクスチャを作成する
	m_ReflectionTexture = new RenderTexture;
	if (!m_ReflectionTexture)
	{
		return false;
	}

	// Initialize the reflection render to texture object.
	result = m_ReflectionTexture->Initialize(m_d3d->GetDevice(), screenWidth, screenHeight);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the reflection render to texture object.", L"Error", MB_OK);
		return false;
	}

	// グラフィックスパイプラインの初期化
	m_lightPass = new LightPassPipeline;
	if (!m_lightPass)
	{
		return false;
	}
	// Initialize the light shader object.
	result = m_lightPass->Initialize(m_d3d->GetDevice(), hwnd, 4);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the light shader object.", L"Error", MB_OK);
		return false;
	}
	m_refractionPass = new RefractionsPassPipeline;
	if (!m_lightPass)
	{
		return false;
	}
	// Initialize the refraction shader object.
	result = m_refractionPass->Initialize(m_d3d->GetDevice(), hwnd);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the refraction shader object.", L"Error", MB_OK);
		return false;
	}
	m_waterPass = new WaterPassPipeline();
	// Initialize the water shader object.
	result = m_waterPass->Initialize(m_d3d->GetDevice(), hwnd);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the water shader object.", L"Error", MB_OK);
		return false;
	}

	// 水の高さを設定
	m_waterHeight = 2.75f;

	// 水のオフセットを設定
	m_waterTranslation = 0.0f;

	return true;
}

bool Graphics::Frame()
{
	// Update the position of the water to simulate motion.
	m_waterTranslation += 0.001f;
	if (m_waterTranslation > 1.0f)
	{
		m_waterTranslation -= 1.0f;
	}

	// Set the position and rotation of the camera.
	m_camera->SetPosition(-10.0f, 6.0f, -10.0f);
	m_camera->SetRotation(0.0f, 45.0f, 0.0f);

	return true;
}

bool Graphics::Render()
{
	bool result;


	// Render the refraction of the scene to a texture.
	result = RenderRefractionToTexture();
	if (!result)
	{
		return false;
	}

	// Render the reflection of the scene to a texture.
	result = RenderReflectionToTexture();
	if (!result)
	{
		return false;
	}

	// Render the scene as normal to the back buffer.
	result = RenderScene();
	if (!result)
	{
		return false;
	}

	return true;
}


bool Graphics::RenderRefractionToTexture()
{
	XMFLOAT4 clipPlane;
	XMMATRIX worldMatrix, viewMatrix, projectionMatrix;
	bool result;

	// Setup a clipping plane based on the height of the water to clip everything above it.
	clipPlane = XMFLOAT4(0.0f, -3.0f, 0.0f, m_waterHeight + 0.1f);

	m_d3d->SetResourceBarrier(m_RefractionTexture->GetRenderTextureResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET, false);

	// Set the render target to be the refraction render to texture.
	m_RefractionTexture->SetRenderTarget(m_d3d->GetCmdList(), m_d3d->GetDepthStencilView());

	// Clear the refraction render to texture.
	m_RefractionTexture->ClearRenderTarget(m_d3d->GetCmdList(), m_d3d->GetDepthStencilView(), 0.0f, 0.0f, 0.0f, 1.0f);

	// Generate the view matrix based on the camera's position.
	m_camera->Render();

	// Get the world, view, and projection matrices from the camera and d3d objects.
	m_d3d->GetWorldMatrix(worldMatrix);
	m_camera->GetViewMatrix(viewMatrix);
	m_d3d->GetProjectionMatrix(projectionMatrix);

	// Translate to where the bath model will be rendered.
	worldMatrix = XMMatrixTranslation(0.0f, 2.0f, 0.0f);

	// Put the bath model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	m_BathModel->Render(m_d3d->GetCmdList());

	// Render the bath model using the light shader.
	result = m_refractionPass->Render(m_d3d->GetCmdList(), m_BathModel->GetIndexCount(), worldMatrix, viewMatrix,
		projectionMatrix, m_BathModel->GetTexture(), m_light->GetDirection(),
		m_light->GetAmbientColor(), m_light->GetDiffuseColor(), clipPlane);
	if (!result)
	{
		return false;
	}

	// バックバッファの状態を元に戻す
	m_d3d->SetResourceBarrier(m_RefractionTexture->GetRenderTextureResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ, false);

	// コマンドを遅延実行させる
	m_d3d->ExecuteCommandList();

	// Present the rendered scene to the screen.
	m_d3d->EndScene();

	// 誤動作を防ぐために描画先をバックバッファに戻す
	m_d3d->SetBackBufferRenderTarget();

	return true;
}


bool Graphics::RenderReflectionToTexture()
{
	XMMATRIX reflectionViewMatrix, worldMatrix, projectionMatrix;
	bool result;

	m_d3d->SetResourceBarrier(m_ReflectionTexture->GetRenderTextureResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET, false);

	// Set the render target to be the reflection render to texture.
	m_ReflectionTexture->SetRenderTarget(m_d3d->GetCmdList(), m_d3d->GetDepthStencilView());

	// Clear the reflection render to texture.
	m_ReflectionTexture->ClearRenderTarget(m_d3d->GetCmdList(), m_d3d->GetDepthStencilView(), 0.0f, 0.0f, 0.0f, 1.0f);

	// Use the camera to render the reflection and create a reflection view matrix.
	m_camera->Render2Reflection(m_waterHeight);

	// Get the camera reflection view matrix instead of the normal view matrix.
	reflectionViewMatrix = m_camera->GetReflectionViewMatrix();

	// Get the world and projection matrices from the d3d object.
	m_d3d->GetWorldMatrix(worldMatrix);
	m_d3d->GetProjectionMatrix(projectionMatrix);

	// Translate to where the wall model will be rendered.
	worldMatrix = XMMatrixTranslation(0.0f, 6.0f, 8.0f);

	// Put the wall model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	m_WallModel->Render(m_d3d->GetCmdList());

	XMMATRIX viewMat;
	m_camera->GetViewMatrix(viewMat);

	// Render the wall model using the light shader and the reflection view matrix.
	result = m_lightPass->Render(m_d3d->GetCmdList(), 0,  m_WallModel->GetIndexCount(), worldMatrix, reflectionViewMatrix,
		projectionMatrix, m_WallModel->GetTexture(), m_light->GetDirection(),
		m_light->GetAmbientColor(), m_light->GetDiffuseColor());
	if (!result)
	{
		return false;
	}

	// バックバッファの状態を元に戻す
	m_d3d->SetResourceBarrier(m_ReflectionTexture->GetRenderTextureResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ, false);

	// コマンドを遅延実行させる
	m_d3d->ExecuteCommandList();

	// Present the rendered scene to the screen.
	m_d3d->EndScene();

	// Reset the render target back to the original back buffer and not the render to texture anymore.
	m_d3d->SetBackBufferRenderTarget();

	return true;
}

// バインドポイントが味噌
bool Graphics::RenderScene()
{
	XMMATRIX worldMatrix, viewMatrix, projectionMatrix, reflectionMatrix;
	bool result;
	// バックバッファのリソース状態を宣言
	m_d3d->SetResourceBarrier(nullptr, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	// 先ほど描画されたテクスチャ(反射と屈折)をshaderから読み取れる状態へ
	m_d3d->SetResourceBarrier(m_RefractionTexture->GetRenderTextureResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	m_d3d->SetResourceBarrier(m_ReflectionTexture->GetRenderTextureResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);

	// Clear the buffers to begin the scene.
	m_d3d->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

	m_d3d->SetBackBufferRenderTarget();

	// Generate the view matrix based on the camera's position.
	m_camera->Render();

	// Get the world, view, and projection matrices from the camera and d3d objects.
	m_d3d->GetWorldMatrix(worldMatrix);
	m_camera->GetViewMatrix(viewMatrix);
	m_d3d->GetProjectionMatrix(projectionMatrix);

	// Translate to where the ground model will be rendered.
	worldMatrix = XMMatrixTranslation(0.0f, 1.0f, 0.0f);

	// Put the ground model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	m_GroundModel->Render(m_d3d->GetCmdList());

	// Render the ground model using the light shader.
	result = m_lightPass->Render(m_d3d->GetCmdList(), 1, m_GroundModel->GetIndexCount(), worldMatrix, viewMatrix,
		projectionMatrix, m_GroundModel->GetTexture(), m_light->GetDirection(),
		m_light->GetAmbientColor(), m_light->GetDiffuseColor());
	if (!result)
	{
		return false;
	}

	// Reset the world matrix.
	m_d3d->GetWorldMatrix(worldMatrix);

	// Translate to where the wall model will be rendered.
	worldMatrix = XMMatrixTranslation(0.0f, 6.0f, 8.0f);

	// Put the wall model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	m_WallModel->Render(m_d3d->GetCmdList());

	// Render the wall model using the light shader.
	result = m_lightPass->Render(m_d3d->GetCmdList(), 2, m_WallModel->GetIndexCount(), worldMatrix, viewMatrix,
		projectionMatrix, m_WallModel->GetTexture(), m_light->GetDirection(),
		m_light->GetAmbientColor(), m_light->GetDiffuseColor());
	if (!result)
	{
		return false;
	}

	// Reset the world matrix.
	m_d3d->GetWorldMatrix(worldMatrix);

	// Translate to where the bath model will be rendered.
	worldMatrix = XMMatrixTranslation( 0.0f, 2.0f, 0.0f);

	// Put the bath model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	m_BathModel->Render(m_d3d->GetCmdList());

	// Render the bath model using the light shader.
	result = m_lightPass->Render(m_d3d->GetCmdList(), 3,  m_BathModel->GetIndexCount(), worldMatrix, viewMatrix,
		projectionMatrix, m_BathModel->GetTexture(), m_light->GetDirection(),
		m_light->GetAmbientColor(), m_light->GetDiffuseColor());
	if (!result)
	{
		return false;
	}

	// Reset the world matrix.
	m_d3d->GetWorldMatrix(worldMatrix);

	// Get the camera reflection view matrix.
	reflectionMatrix = m_camera->GetReflectionViewMatrix();

	// Translate to where the water model will be rendered.
	worldMatrix = XMMatrixTranslation( 0.0f, m_waterHeight, 0.0f);

	// Put the water model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	m_WaterModel->Render(m_d3d->GetCmdList());

	// Render the water model using the water shader.
	result = m_waterPass->Render(m_d3d->GetCmdList(), m_WaterModel->GetIndexCount(), worldMatrix, viewMatrix,
		projectionMatrix, reflectionMatrix, m_ReflectionTexture->GetShaderResourceView(),
		m_RefractionTexture->GetShaderResourceView(), m_WaterModel->GetTexture(),
		m_waterTranslation, 0.01f);
	if (!result)
	{
		return false;
	}

	// バックバッファのリソース状態を元に戻す
	m_d3d->SetResourceBarrier(nullptr, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, true);
	// テクスチャ達も元に戻す
	m_d3d->SetResourceBarrier(m_RefractionTexture->GetRenderTextureResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ, false);
	m_d3d->SetResourceBarrier(m_ReflectionTexture->GetRenderTextureResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ, false);

	m_d3d->ExecuteCommandList();

	// Present the rendered scene to the screen.
	m_d3d->EndScene();

	return true;
}