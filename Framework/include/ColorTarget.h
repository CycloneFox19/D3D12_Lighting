#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <ComPtr.h>
#include <cstdint>

// Forward Declerations
class DescriptorHandle;
class DescriptorPool;

//
// colorTarget class
//
class ColorTarget
{

public:

	//! @brief constructor
	ColorTarget();

	//! @brief destructor
	~ColorTarget();

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] pPoolRTV descriptor pool
	//! @param[in] width width
	//! @param[in] height height
	//! @param[in] format pixel format
	//! @param[in] useSRGB select true if you use SRGB format
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPoolRTV,
		DescriptorPool* pPoolSRV,
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format,
		float clearValue[4]);

	//! @brief initialize from back buffer
	//! 
	//! @param[in] pDevice device
	//! @param[in] pPoolRTV descriptor pool
	//! @param[in] index back buffer index
	//! @param[in] pSwapChain swap chain
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool InitFromBackBuffer(
		ID3D12Device* pDevice,
		DescriptorPool* pPoolRTV,
		bool useSRGB,
		uint32_t index,
		IDXGISwapChain* pSwapChain);

	//! @brief end
	void Term();

	//! @brief get descriptor handle (for RTV)
	//! 
	//! @return return descriptor handle (for RTV)
	DescriptorHandle* GetHandleRTV() const;

	//! @brief get descriptor handle (for SRV)
	//! 
	//! @return return descriptor handle (for SRV)
	DescriptorHandle* GetHandleSRV() const;

	//! @brief get resource
	//! 
	//! @return return resource
	ID3D12Resource* GetResource() const;

	//! @brief get resource settings
	//! 
	//! @return return resource settings
	D3D12_RESOURCE_DESC GetDesc() const;

	//! @brief get settings of render target view
	//! 
	//! @return return settings of render target view
	D3D12_RENDER_TARGET_VIEW_DESC GetRTVDesc() const;

	//! @brief get settings of shader resource view
	//! 
	//! @return return settings of shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc() const;

	//! @brief clear the view
	//! 
	//! @param[in] pCmdList command list
	void ClearView(ID3D12GraphicsCommandList* pCmdList);

private:

	ComPtr<ID3D12Resource> m_pTarget; //!< resource
	DescriptorHandle* m_pHandleRTV; //!< descriptor handle (for RTV)
	DescriptorHandle* m_pHandleSRV; //!< descriptor handle (for SRV)
	DescriptorPool* m_pPoolRTV; //!< descriptor pool (for RTV)
	DescriptorPool* m_pPoolSRV; //!< descriptor pool (for SRV)
	D3D12_RENDER_TARGET_VIEW_DESC m_RTVDesc; //!< configuration of render target view
	D3D12_SHADER_RESOURCE_VIEW_DESC m_SRVDesc; //!< configuration of shader resource view
	float m_ClearColor[4]; //!< clear color

	ColorTarget(const ColorTarget&) = delete;
	void operator = (const ColorTarget&) = delete;
};
