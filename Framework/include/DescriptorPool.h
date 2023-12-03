#pragma once

#include <d3d12.h>
#include <atomic>
#include <ComPtr.h>
#include <Pool.h>

// DescriptorHandle class
class DescriptorHandle
{
public:
	D3D12_CPU_DESCRIPTOR_HANDLE HandleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE HandleGPU;

	bool HasCPU() const
	{
		return HandleCPU.ptr != 0;
	}

	bool HasGPU() const
	{
		return HandleGPU.ptr != 0;
	}
};

// DescriptorPool class
class DescriptorPool
{

public:

	//public methods

	//! @brief generate
	//! 
	//! @param[in] pDevice device
	//! @param[in] pDesc configuration settings of descriptorheap
	//! @param[out] ppPool container of descriptorpool
	//! @retval true successfully generated
	//! @retval false failed to generate
	static bool Create(
		ID3D12Device* pDevice,
		const D3D12_DESCRIPTOR_HEAP_DESC* pDesc,
		DescriptorPool** ppPool);

	//! @brief 参照カウントを増やす
	void AddRef();

	//! @brief release
	void Release();

	//! @brief 参照カウントを取得
	//! 
	//! @return 参照カウントを返却
	uint32_t GetCount() const;

	//! @brief ディスクリプタハンドルを割り当て
	//! 
	//! @return 割り当てられたディスクリプタハンドルを返却
	DescriptorHandle* AllocHandle();

	//! @brief release descriptorhandle
	//! 
	//! @param[in] pHandle pointer to handle to release
	void FreeHandle(DescriptorHandle*& pHandle);

	//! @brief get available handle count
	//! 
	//! @return return available handle count
	uint32_t GetAvailableHandleCount() const;

	//! @brief 割り当て済みのハンドル数を取得
	//! 
	//! @return 割り当て済みのハンドル数を返却
	uint32_t GetAllocatedHandleCount() const;

	//! @brief get total handle amount
	//! 
	//! @return return total handle amount
	uint32_t GetHandleCount() const;

	//! @brief get descriptor heap
	//! 
	//! @return release descriptor heap
	ID3D12DescriptorHeap* const GetHeap() const;

private:

	//private variables
	std::atomic<uint32_t> m_RefCount;
	Pool<DescriptorHandle> m_Pool;
	ComPtr<ID3D12DescriptorHeap> m_pHeap;
	uint32_t m_DescriptorSize;

	//private methods

	//! @brief constructor
	DescriptorPool();

	//! @brief destructor
	~DescriptorPool();

	DescriptorPool(const DescriptorPool&) = delete;
	void operator = (const DescriptorPool&) = delete;
};
