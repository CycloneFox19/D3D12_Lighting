//
// VSInput structure
//
struct VSInput
{
	float3 Position : POSITION; // position coords
	float3 Normal : NORMAL; // normal vector
	float2 TexCoord : TEXCOORD; // texture coords
	float3 Tangent : TANGENT; // tangent vector
};

//
// VSOutput structure
//
struct VSOutput
{
	float4 Position : SV_POSITION; // position coords
	float2 TexCoord : TEXCOORD; // texture coords
	float3 WorldPos : WORLD_POS; // position coords in world space
	float3x3 InvTangentBasis : INV_TANGENT_BASIS; // inverse of base vector transformation matrix to tangent space
};

//
// Transform Constant buffer
//
cbuffer CbTransform : register(b0)
{
	float4x4 View : packoffset(c0); // view matrix
	float4x4 Proj : packoffset(c4); // projection matrix
};

//
// CbMesh constant buffer
//
cbuffer CbMesh : register(b1)
{
	float4x4 World : packoffset(c0); // world matrix
};

// main entry point to the vertex buffer
VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;

	float4 localPos = float4(input.Position, 1.0f);
	float4 worldPos = mul(World, localPos);
	float4 viewPos = mul(View, worldPos);
	float4 projPos = mul(Proj, viewPos);

	output.Position = projPos;
	output.TexCoord = input.TexCoord;
	output.WorldPos = worldPos.xyz;

	// base vectors
	float3 N = normalize(mul((float3x3)World, input.Normal));
	float3 T = normalize(mul((float3x3)World, input.Tangent));
	float3 B = normalize(cross(N, T));

	// inverse matrix of base transformation
	output.InvTangentBasis = transpose(float3x3(T, B, N));

	return output;
}