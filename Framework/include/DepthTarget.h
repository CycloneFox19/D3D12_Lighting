#pragma once

#include <d3d12.h>
#include <ComPtr.h>
#include <cstdint>

// Forward Declarations
class DescriptorHandle;
class DescriptorPool;

//
// DepthTarget class
//
class DepthTarget
{

public:

	//! @brief constructor
	DepthTarget();

	//! @brief destructor
	~DepthTarget();

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] pPoolDSV descriptor pool
	//! @param[in] width width
	//! @param[in] height height
	//! @param[in] format pixel format
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPoolDSV,
		DescriptorPool* pPoolSRV,
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format,
		float clearDepth,
		uint8_t clearStencil);

	//! @brief end
	void Term();

	//! @brief get descriptor handle(for DSV)
	//! 
	//! @return return descriptor handle(for DSV)
	DescriptorHandle* GetHandleDSV() const;

	//! @brief get descriptor handle(for SRV)
	//! 
	//! @return return descriptor handle(for SRV)
	DescriptorHandle* GetHandleSRV() const;

	//! @brief get resource
	//! 
	//! @return return resource
	ID3D12Resource* GetResource() const;

	//! @brief get resource setting
	//! 
	//! @return return resource setting
	D3D12_RESOURCE_DESC GetDesc() const;

	//! @brief get settings of depth stencil view
	//! 
	//! @return return settings of depth stencil view
	D3D12_DEPTH_STENCIL_VIEW_DESC GetDSVDesc() const;

	//! @brief get settings of shader resource view
	//! 
	//! @return return settings of shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc() const;

	//! @brief clear view
	//! 
	//! @param[in] pCmdList command list
	void ClearView(ID3D12GraphicsCommandList* pCmdList);

private:

	ComPtr<ID3D12Resource> m_pTarget; //!< resource
	DescriptorHandle* m_pHandleDSV; //!< descriptor handle(for DSV)
	DescriptorHandle* m_pHandleSRV; //!< descriptor handle(for SRV)
	DescriptorPool* m_pPoolDSV; //!< descriptor pool(for DSV)
	DescriptorPool* m_pPoolSRV; //!< descriptor pool(for SRV)
	D3D12_DEPTH_STENCIL_VIEW_DESC m_DSVDesc; //!< settings of depth stencil view
	D3D12_SHADER_RESOURCE_VIEW_DESC m_SRVDesc; //!< settings of shader resource view
	float m_ClearDepth;
	uint8_t m_ClearStencil;

	DepthTarget(const DepthTarget&) = delete;
	void operator = (const DepthTarget&) = delete;
};
