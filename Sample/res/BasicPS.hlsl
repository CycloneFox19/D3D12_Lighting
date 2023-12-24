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
cbuffer CbLight : register(b1)
{
	float3 LightPosition : packoffset(c0);
	float LightAngleOffset : packoffset(c0.w);
	float3 LightColor : packoffset(c1);
	float LightIntensity : packoffset(c1.w);
	float3 LightForward : packoffset(c2);
	float LightAngleScale : packoffset(C2.w);
};

//
// camera buffer
//
cbuffer CbCamera : register(b2)
{
	float3 CameraPosition : packoffset(c0); // camera position
};

// textures and samplers
SamplerState IESSmp : register(s0);
Texture2D IESMap : register(t0);

Texture2D BaseColorMap : register(t1);
SamplerState BaseColorSmp : register(s1);

Texture2D MetallicMap : register(t2);
SamplerState MetallicSmp : register(s2);

Texture2D RoughnessMap : register(t3);
SamplerState RoughnessSmp : register(s3);

Texture2D NormalMap : register(t4);
SamplerState NormalSmp : register(s4);

// find angle attenuation
float GetAngleAttenuation
(
	float3 normalizedLightVector, // difference vector between light and object position
	float3 lightDir, // normalized light vector
	float lightAngleScale, // angle attenuation scale of spot light
	float lightAngleOffset // angle offset of spot light
)
{
	// calculate the following values in CPU
	// float lightAngleScale = 1.0f / max(0.001f, (cosInner - cosOuter));
	// float lightAngleOffset = -cosOuter * lightAngleScale;
	// * cosInner -> cosine of inner angle
	// * cosOuter -> cosine of outer angle

	float cd = dot(lightDir, unnormalizedLightVector);
	float attenuation = saturate(cd * lightAngleScale + lightAngleOffset);

	// transient smoothly
	attenuation *= attenuation;

	return attenuation;
}

// find attenuation by IES Profile
float GetIESProfileAttenuation(float3 lightDir, float3 lightForward)
{
	// find angles
	float thetaCoord = dot(lightDir, lightForward) * 0.5 + 0.5f;
	float tangentAngle = atan2(lightDir.y, lightDir.x);
	float phiCoord = (tangentAngle / F_PI) * 0.5f + 0.5f;
	float2 texCoord = float2(thetaCoord, phiCoord);

	// get normalized light intensity from IES Texture
	return IESMap.SampleLevel(IESSmp, texCoord, 0).r;
}

// evaluate photometric light
float3 EvaluatePhotometricLight
(
	float3 N, // normal vector
	float3 worldPos, // object position in world space
	float3 lightPos, // position of light
	float3 lightForward, // direction of light
	float3 lightColor, // color of light
	float lightAngleScale, // angle scale of light
	float lightAngleOffset // angle offset of light
)
{
	float3 unnormalizedLightVector = (lightPos - worldPos);
	float3 L = normalize(unnormalizedLightVector);
	float att = 1.0f;
	att *= GetAngleAttenuation(-L, lightForward, lightAngleScale, lightAngleOffset);
	att *= GetIESProfileAttenuation(L, lightForward);

	return saturate(dot(N, L)) * lightColor * att / (4.0f * F_PI);
}

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
	float3 lit = EvaluatePhotometricLight(
		N,
		input.WorldPos,
		LightPosition,
		LightForward,
		LightColor,
		LightAngleScale,
		LightAngleOffset) * LightIntensity;

	output.Color.rgb = lit * BRDF;
	output.Color.a = 1.0f;

	return output;
}