#pragma once

#include <d3d12.h>
#include <ComPtr.h>

//
// VertexBuffer class
//
class VertexBuffer
{

public:

	//! @brief constructor
	VertexBuffer();

	//! @brief destructor
	~VertexBuffer();

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] size size of VertexBuffer
	//! @param[in] stride size of one vertex
	//! @param[in] pInitData initialize data
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(ID3D12Device* pDevice, size_t size, size_t stride, const void* pInitData = nullptr);

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] size size of VertexBuffer
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	//! @detail attention : size must be specified with sizeof(T)
	template<typename T>
	bool Init(ID3D12Device* pDevice, size_t size, const T* pInitData = nullptr)
	{
		return Init(pDevice, size, sizeof(T), pInitData);
	}

	//! @brief end
	void Term();

	//! @brief memory mapping
	void* Map() const;

	//! @brief unmap memory
	void Unmap();

	//! @brief memory mapping
	template<typename T>
	T* Map() const
	{
		return reinterpret_cast<T*>(Map());
	}

	//! @brief get VertexBufferView
	//! 
	//! @return return VertexBufferView
	D3D12_VERTEX_BUFFER_VIEW GetView() const;

private:

	ComPtr<ID3D12Resource> m_pVB; //!< vertex buffer
	D3D12_VERTEX_BUFFER_VIEW m_View; //!< vertex buffer view

	VertexBuffer(const VertexBuffer&) = delete;
	void operator = (const VertexBuffer&) = delete;
};
