#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <DescriptorPool.h>
#include <ResourceUploadBatch.h>

//
// IESProfile class
//
class IESProfile
{

public:

	//! @brief constructor
	IESProfile();

	//! @brief destructor
	~IESProfile();

	//! @brief initialize
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		const char* filePath,
		DirectX::ResourceUploadBatch& batch);

	//! @brief end
	void Term();

	//! @brief get CPU descriptor handle
	//! 
	//! @return return CPU descriptor handle
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const;

	//! @brief get GPU descriptor handle
	//! 
	//! @return return GPU descriptor handle
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const;

	//! @brief get resource
	//! 
	//! @return return resource
	ID3D12Resource* GetResource() const;

	//! @brief get luminous flux
	//! 
	//! @return return luminous flux
	float GetLumen() const;

private:

	Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource; //!< resource
	DescriptorHandle* m_pHandle; //!< descriptor handle
	DescriptorPool* m_pPool; //!< descriptor pool
	float m_Lumen; //!< luminous flux
	std::vector<float> m_Candera; //!< candera

	IESProfile(const IESProfile&) = delete; // ban access
	IESProfile& operator = (const IESProfile&) = delete; // ban access
};
