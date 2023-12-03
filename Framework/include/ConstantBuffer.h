#pragma once

#include <d3d12.h>
#include <ComPtr.h>
#include <vector>

//
// Forward Declarations.
//
class DescriptorHandle;
class DescriptorPool;

//
// Constant Buffer Class
//
class ConstantBuffer
{

public:

	//! @brief constructor
	ConstantBuffer();

	//! @brief destructor
	~ConstantBuffer();

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] pPool descriptor pool
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		size_t size);

	//! @brief end
	void Term();

	//! @brief get GPU virtual address
	//! 
	//! @return return GPU virtual address
	D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const;

	//! @brief obtain CPU descriptor handle
	//! 
	//! @return return CPU descriptor handle
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const;

	//! @brief obtain GPU descriptor handle
	//! 
	//! @return return GPU descriptor handle
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const;

	//! @brief get memory-mapped pointer
	//! 
	//! @return return memory-mapped pointer
	void* GetPtr() const;

	//! @brief get memory-mapped pointer
	//! 
	//! @return return memory-mapped pointer
	template<typename T>
	T* GetPtr()
	{
		return reinterpret_cast<T*>(GetPtr());
	}

private:

	ComPtr<ID3D12Resource> m_pCB;
	DescriptorHandle* m_pHandle;
	DescriptorPool* m_pPool;
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_Desc;
	void* m_pMappedPtr;

	ConstantBuffer(const ConstantBuffer&) = delete;
	void operator = (const ConstantBuffer&) = delete;
};
