// Includes
#include "BRDF.hlsli"

#ifndef MIN_DIST
#define MIN_DIST (0.01)
#endif // MIN_DIST

//
// VSOutput structure
//
struct VSOutput
{
	float4 Position : SV_POSITION; // position coordinates
	float2 TexCoord : TEXCOORD; // texture coordinates
	float3 WorldPos : WORLD_POS; // position coordinates in world space
	float3x3 InvTangentBasis : INV_TANGENT_BASIS; // inverse matrix of base vectors transformation
};

//
// PSOutput structure
//
struct PSOutput
{
	float4 Color : SV_TARGET0; // output color
};

//
// CbLight constant buffer
//
cbuffer LightBuffer : register(b1)
{
	float3 LightColor : packoffset(c0); // color of the light
	float LightIntensity : packoffset(c0.w); // intensity of the light
	float3 LightForward : packoffset(c1); // direction of the light
};

//
// camera buffer
//
cbuffer CbCamera : register(b2)
{
	float3 CameraPosition : packoffset(c0); // camera position
};

// textures and samplers
Texture2D BaseColorMap : register(t0);
SamplerState BaseColorSmp : register(s0);

Texture2D MetallicMap : register(t1);
SamplerState MetallicSmp : register(s1);

Texture2D RoughnessMap : register(t2);
SamplerState RoughnessSmp : register(s2);

Texture2D NormalMap : register(t3);
SamplerState NormalSmp : register(s3);

// main entry point of pixel shader
PSOutput main(VSOutput input)
{
	PSOutput output = (PSOutput)0;

	float3 L = normalize(LightForward);
	float3 V = normalize(CameraPosition - input.WorldPos);
	float3 H = normalize(V + L);
	float3 N = NormalMap.Sample(NormalSmp, input.TexCoord).xyz * 2.0f - 1.0f;
	N = mul(input.InvTangentBasis, N);

	float NV = saturate(dot(N, V));
	float NH = saturate(dot(N, H));
	float NL = saturate(dot(N, L));

	float3 baseColor = BaseColorMap.Sample(BaseColorSmp, input.TexCoord).rgb;
	float metallic = MetallicMap.Sample(MetallicSmp, input.TexCoord).r;
	float roughness = RoughnessMap.Sample(RoughnessSmp, input.TexCoord).r;

	float3 Kd = baseColor * (1.0f - metallic);
	float3 diffuse = ComputeLambert(Kd);

	float Ks = baseColor * metallic;
	float3 specular = ComputeGGX(Ks, roughness, NH, NV, NL);

	float3 BRDF = (diffuse + specular);

	output.Color.rgb = BRDF * NL * LightColor.rgb * LightIntensity;
	output.Color.a = 1.0f;

	return output;
}