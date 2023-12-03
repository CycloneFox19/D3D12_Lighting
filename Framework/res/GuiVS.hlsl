//
// VSInput structure
//
struct VSInput
{
	float2 Position : POSITION; //!< position coordinates
	float2 TexCoord : TEXCOORD; //!< texture coordinates
	float4 Color : COLOR; //!< color
};

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
// Transform constant buffer
//
cbuffer Transform : register(b0)
{
	float4x4 Proj; //!< projection matrix
};

// main entry point of vertex shader
VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;

	output.Position = mul(Proj, float4(input.Position, 0.0f, 1.0));
	output.Color = input.Color;
	output.TexCoord = input.TexCoord;

	return output;
}