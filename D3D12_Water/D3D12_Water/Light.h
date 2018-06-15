#pragma once
#include <DirectXMath.h>

class Light
{
private:
	DirectX::XMFLOAT4 m_ambientColor;
	DirectX::XMFLOAT4 m_diffuseColor;
	DirectX::XMFLOAT4 m_specularColor;

	float m_specularPower; // ÉXÉyÉLÉÖÉâåWêî
	DirectX::XMFLOAT3 m_direction;

public:
	Light();
	~Light();

	void SetAmbientColor(float, float, float, float);
	void SetDiffuseColor(float, float, float, float);
	void SetSpecularColor(float, float, float, float);
	void SetSpecularPower(float);
	void SetDirection(float, float, float);

	DirectX::XMFLOAT4 GetAmbientColor();
	DirectX::XMFLOAT4 GetDiffuseColor();
	DirectX::XMFLOAT4 GetSpecularColor();
	float GetSpecularPower();
	DirectX::XMFLOAT3 GetDirection();
};

