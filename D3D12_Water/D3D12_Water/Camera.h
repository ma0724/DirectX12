#pragma once
#include <DirectXMath.h>

class Camera
{
private:
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_reflectionViewMatrix;
	float m_positionX, m_positionY, m_positionZ;
	float m_rotationX, m_rotationY, m_rotationZ;
public:
	Camera();
	~Camera();
	void SetPosition(float, float, float);
	void SetRotation(float, float, float);

	void Render();
	void GetViewMatrix(DirectX::XMMATRIX&);

	void Render2Reflection(float);
	DirectX::XMMATRIX GetReflectionViewMatrix();
};

