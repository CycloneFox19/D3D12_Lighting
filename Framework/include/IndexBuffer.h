#pragma once

#include <d3d12.h>
#include <ComPtr.h>
#include <cstdint>

class IndexBuffer
{

public:

	//! @brief constructor
	IndexBuffer();

	//! @brief destructor
	~IndexBuffer();

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] size size of index buffer
	//! @param[in] pInitData initialize data
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(ID3D12Device* pDevice, size_t size, const uint32_t* pInitData = nullptr);

	//! @brief end
	void Term();

	//! @brief memory mappint
	uint32_t* Map();

	//! @brief unmap memory
	void Unmap();

	//! @brief get index buffer view
	//! 
	//! @return return index buffer view
	D3D12_INDEX_BUFFER_VIEW GetView() const;

private:

	ComPtr<ID3D12Resource> m_pIB; //!< Index Buffer
	D3D12_INDEX_BUFFER_VIEW m_View; //!< Index Buffer View

	IndexBuffer(const IndexBuffer&) = delete;
	void operator = (const IndexBuffer&) = delete;
};
