
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer ReflectionBuffer : register(b1)
{
    matrix reflectionMatrix;
};

cbuffer WaterBuffer : register(b2)
{
    float waterTranslation;
    float reflectRefractScale;
    float2 padding;
};

SamplerState SampleType : register(s0);
Texture2D reflectionTexture : register(t0);
Texture2D refractionTexture : register(t1);
Texture2D normalTexture : register(t2);

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 reflectionPosition : TEXCOORD1;
    float4 refractionPosition : TEXCOORD2;
};

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};

PixelInputType WaterVertexShader(VertexInputType input)
{
    PixelInputType output;
    matrix reflectProjectWorld;
    matrix viewProjectWorld;
	
    
	// Change the position vector to be 4 units for proper matrix calculations.
    input.position.w = 1.0f;

	// Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
	// Store the texture coordinates for the pixel shader.
    output.tex = input.tex;
    
	// 反射 projection×world行列を作成します
    reflectProjectWorld = mul(reflectionMatrix, projectionMatrix);
    reflectProjectWorld = mul(worldMatrix, reflectProjectWorld);

	// reflectProjectWorld行列に対して入力位置を計算します
    output.reflectionPosition = mul(input.position, reflectProjectWorld);

	// 屈折のためのview × projection × world行列を作成します
    viewProjectWorld = mul(viewMatrix, projectionMatrix);
    viewProjectWorld = mul(worldMatrix, viewProjectWorld);
   
	// viewProjectWorld行列に対して入力位置を計算します
    output.refractionPosition = mul(input.position, viewProjectWorld);
	
    return output;
}


float4 WaterPixelShader(PixelInputType input) : SV_TARGET
{
    float2 reflectTexCoord;
    float2 refractTexCoord;
    float4 normalMap;
    float3 normal;
    float4 reflectionColor;
    float4 refractionColor;
    float4 color;

	
	// 移動する水をシミュレートするために水の法線をサンプリングした位置を移動します
    input.tex.y += waterTranslation;
	
	// 投影された反射テクスチャ座標を計算します
    reflectTexCoord.x = input.reflectionPosition.x / input.reflectionPosition.w / 2.0f + 0.5f;
    reflectTexCoord.y = -input.reflectionPosition.y / input.reflectionPosition.w / 2.0f + 0.5f;
	
	// 投影された屈折テクスチャ座標を計算します
    refractTexCoord.x = input.refractionPosition.x / input.refractionPosition.w / 2.0f + 0.5f;
    refractTexCoord.y = -input.refractionPosition.y / input.refractionPosition.w / 2.0f + 0.5f;

	// ノーマルマップから法線をサンプリングします
    normalMap = normalTexture.Sample(SampleType, input.tex);

	// 法線の範囲を（0,1）から（-1、+ 1）に広げます
    normal = (normalMap.xyz * 2.0f) - 1.0f;

	// UV座標サンプリング位置をノーマルマップ値で再配置し、波紋効果をシミュレートします
    reflectTexCoord = reflectTexCoord + (normal.xy * reflectRefractScale);
    refractTexCoord = refractTexCoord + (normal.xy * reflectRefractScale);

	// 更新されたUV座標を使用して、テクスチャからサンプリングします
    reflectionColor = reflectionTexture.Sample(SampleType, reflectTexCoord);
    refractionColor = refractionTexture.Sample(SampleType, refractTexCoord);

	// 最終カラーの反射と屈折の結果を組み合わせます
    color = lerp(reflectionColor, refractionColor, 0.1f);
	
    return color;
}
