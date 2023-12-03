// Includes
#include "BRDF.hlsli"

#ifndef MIN_DIST
#define MIN_DIST (0.01)
#endif // MIN_DIST

#ifndef OPTIMIZATION
#define OPTIMIZATION (1)
#endif // OPTIMIZATION

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
	float LightInvSqrRadius : packoffset(c0.w);
	float3 LightColor : packoffset(c1);
	float LightIntensity : packoffset(c1.w);
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

// attenuate according to distance
float SmoothDistanceAttenuation
(
	float squaredDistance, // two powers of distance to the light
	float invSqrAttRadius // reciprocal of squared light radius
)
{
	float factor = squaredDistance * invSqrAttRadius;
	float smoothFactor = saturate(1.0f - factor * factor);
	return smoothFactor * smoothFactor;
}

#ifndef OPTIMIZATION
// find distance attenuation
float GetDistanceAttenuation(float3 unnormalizedLightVector)
{
	float sqrDist = dot(unnormalizedLightVector, unnormalizedLightVector);
	float attenuation = 1.0f / (max(sqrDist, MIN_DIST * MIN_DIST));
	return attenuation;
}

// evaluate point light
float3 EvaluatePointLight
(
	float3 N, // normal vector
	float3 worldPos, // object position in world space
	float3 lightPos, // position of light
	float3 lightColor // color of light
)
{
	float3 dif = lightPos - worldPos;
	float3 L = normalize(dif);
	float att = GetDistanceAttenuation(dif);

	return saturate(dot(N, L)) * lightColor * att / (4.0f * F_PI);
}
#else
// find distance attenuation
float GetDistanceAttenuation
(
	float3 unnormalizedLightVector, // difference vector between light position and object position
	float invSqrAttRadius // reciprocal of squared light radius
)
{
	float sqrDist = dot(unnormalizedLightVector, unnormalizedLightVector);
	float attenuation = 1.0f / (max(sqrDist, MIN_DIST * MIN_DIST));
	
	// make attenuation come closer to zero smoothly by window function
	attenuation *= SmoothDistanceAttenuation(sqrDist, invSqrAttRadius);

	return attenuation;
}

// evaluate point light
float3 EvaluatePointLight
(
	float3 N, // normal vector
	float3 worldPos, // object position in world space
	float3 lightPos, // position of light
	float lightInvRadiusSq, // reciprocal of squared light radius
	float3 lightColor // light color
)
{
	float3 dif = lightPos - worldPos;
	float3 L = normalize(dif);
	float att = GetDistanceAttenuation(dif, lightInvRadiusSq);

	return saturate(dot(N, L)) * lightColor * att / (4.0f * F_PI);
}
#endif // OPTIMIZATION

// main entry point of pixel shader
PSOutput main(VSOutput input)
{
	PSOutput output = (PSOutput)0;

	float3 L = normalize(LightPosition - input.WorldPos);
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

	float3 BRDF = diffuse + specular;
#ifndef OPTIMIZATION
	float3 lit = EvaluatePointLight(N, input.WorldPos, LightPosition, LightColor) * LightIntensity;
#else
	float3 lit = EvaluatePointLight(N, input.WorldPos, LightPosition, LightInvSqrRadius, LightColor) * LightIntensity;
#endif // OPTIMIZATION

	output.Color.rgb = lit * BRDF;
	output.Color.a = 1.0f;

	return output;
}