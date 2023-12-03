//
// VSOutput structure
//
struct VSOutput
{
	float4 Position : SV_POSITION; //!< position coordinates
	float4 Color : COLOR; //!< color
	float2 TexCoord : TEXCOORD; //!< texture coordinates
};

//
// PSOutput structure
//
struct PSOutput
{
	float4 Target0 : SV_TARGET0; //!< render target 0
};

// Samplers and Textures
SamplerState ColorSmp : register(s0); //!< color sampler
Texture2D ColorMap : register(t0); //!< color texture

// main entry point of pixel shader
PSOutput main(VSOutput input)
{
	PSOutput output = (PSOutput)0;

	output.Target0 = input.Color * ColorMap.Sample(ColorSmp, input.TexCoord);

	return output;
}