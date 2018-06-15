#pragma once
#include "D3D.h"
#include "Model.h"
#include "Camera.h"
#include "Light.h"
#include "RenderTexture.h"
#include "LightPassPipeline.h"
#include "RefractionsPassPipeline.h"
#include "WaterPassPipeline.h"

const bool FULL_SCREEN = false;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;

class Graphics
{
private:
	D3D*					 m_d3d;
	Model*					 m_GroundModel, *m_WallModel, *m_BathModel, *m_WaterModel;
	Camera*					 m_camera;
	Light*					 m_light;
	RenderTexture*			 m_RefractionTexture, *m_ReflectionTexture; // 屈折と反射用テクスチャ
	LightPassPipeline*		 m_lightPass;
	RefractionsPassPipeline* m_refractionPass;
	WaterPassPipeline*		 m_waterPass;
	float		m_waterHeight, m_waterTranslation;

private:
	bool RenderRefractionToTexture();
	bool RenderReflectionToTexture();
	bool RenderScene();

public:
	Graphics();
	~Graphics();

	bool Initialize(int, int, HWND);
	bool Frame();
	bool Render();

};

