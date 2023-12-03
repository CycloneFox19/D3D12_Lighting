#pragma once

#include <ComPtr.h>
#include <d3d12.h>
#include <vector>

//
// ShaderState enum
//
enum ShaderStage
{
	ALL = 0, //!< all stages
	VS = 1, //!< vertex shader
	HS = 2, //!< hull shader
	DS = 3, //!< domain shader
	GS = 4, //!< geometry shader
	PS = 5, //!< pixel shader
};

//
// SamplerState enum
//
enum SamplerState
{
	PointWrap, //!< point sampling - repeat
	PointClamp, //!< point sampling - clamp
	LinearWrap, //!< tri linear sampling - repeat
	LinearClamp, //!< tri linear sampling - clamp
	AnisotropicWrap, //!< aniostropic sampling - repeat
	AnisotropicClamp, //!< anisotropic sampling - clamp
};

//
// RootSignature class
//
class RootSignature
{

public:

	//
	// Desc class
	//
	class Desc
	{
	public:
		Desc();
		~Desc();
		Desc& Begin(int count);
		Desc& SetCBV(ShaderStage stage, int index, uint32_t reg);
		Desc& SetSRV(ShaderStage stage, int index, uint32_t reg);
		Desc& SetUAV(ShaderStage stage, int index, uint32_t reg);
		Desc& SetSmp(ShaderStage stage, int index, uint32_t reg);
		Desc& AddStaticSmp(ShaderStage stage, uint32_t reg, SamplerState state);
		Desc& AllowIL();
		Desc& AllowSO();
		Desc& End();
		const D3D12_ROOT_SIGNATURE_DESC* GetDesc() const;

	private:
		std::vector<D3D12_DESCRIPTOR_RANGE> m_Ranges;
		std::vector<D3D12_STATIC_SAMPLER_DESC> m_Samplers;
		std::vector<D3D12_ROOT_PARAMETER> m_Params;
		D3D12_ROOT_SIGNATURE_DESC m_Desc;
		bool m_DenyStage[5];
		uint32_t m_Flags;

		void CheckStage(ShaderStage stage);
		void SetParam(ShaderStage, int index, uint32_t reg, D3D12_DESCRIPTOR_RANGE_TYPE type);
	};

	// public methods
	RootSignature();
	~RootSignature();
	bool Init(ID3D12Device* pDevice, const D3D12_ROOT_SIGNATURE_DESC* pDesc);
	void Term();
	ID3D12RootSignature* GetPtr() const;

private:
	// private variables
	ComPtr<ID3D12RootSignature> m_RootSignature;

};
