#include <DescriptorPool.h>

// DescriptorPool class

DescriptorPool::DescriptorPool()
	: m_RefCount(1)
	, m_Pool()
	, m_pHeap()
	, m_DescriptorSize(0)
{
}

// Destructor

DescriptorPool::~DescriptorPool()
{
	m_Pool.Term();
	m_pHeap.Reset();
	m_DescriptorSize = 0;
}

// 参照カウントを増やす
void DescriptorPool::AddRef()
{
	m_RefCount++;
}

void DescriptorPool::Release()
{
	m_RefCount--;
	if (m_RefCount == 0)
	{
		delete this;
	}
}

// get reference count
uint32_t DescriptorPool::GetCount() const
{
	return m_RefCount;
}

// ディスクリプタハンドルを割り当て
DescriptorHandle* DescriptorPool::AllocHandle()
{
	auto func = [&](uint32_t index, DescriptorHandle* pHandle)
	{
		auto handleCPU = m_pHeap->GetCPUDescriptorHandleForHeapStart();
		handleCPU.ptr += m_DescriptorSize * index;

		auto handleGPU = m_pHeap->GetGPUDescriptorHandleForHeapStart();
		handleGPU.ptr += m_DescriptorSize * index;

		pHandle->HandleCPU = handleCPU;
		pHandle->HandleGPU = handleGPU;
	};

	return m_Pool.Alloc(func);
}

// ディスクリプタハンドルを解法
void DescriptorPool::FreeHandle(DescriptorHandle*& pHandle)
{
	if (pHandle != nullptr)
	{
		m_Pool.Free(pHandle);

		pHandle = nullptr;
	}
}

// get available handle count
uint32_t DescriptorPool::GetAvailableHandleCount() const
{
	return m_Pool.GetAvailableCount();
}

// get allocated handle count
uint32_t DescriptorPool::GetAllocatedHandleCount() const
{
	return m_Pool.GetUsedCount();
}

// Get Total Amount of handle
uint32_t DescriptorPool::GetHandleCount() const
{
	return m_Pool.GetSize();
}

// Get DescriptorHeap
ID3D12DescriptorHeap* const DescriptorPool::GetHeap() const
{
	return m_pHeap.Get();
}

bool DescriptorPool::Create
(
	ID3D12Device* pDevice,
	const D3D12_DESCRIPTOR_HEAP_DESC* pDesc,
	DescriptorPool** ppPool
)
{
	if (pDevice == nullptr || pDesc == nullptr || ppPool == nullptr)
	{
		return false;
	}

	auto instance = new (std::nothrow) DescriptorPool();
	if (instance == nullptr)
	{
		return false;
	}

	auto hr = pDevice->CreateDescriptorHeap(
		pDesc,
		IID_PPV_ARGS(instance->m_pHeap.GetAddressOf()));

	if (FAILED(hr))
	{
		instance->Release();
		return false;
	}

	if (!instance->m_Pool.Init(pDesc->NumDescriptors))
	{
		instance->Release();
		return false;
	}

	instance->m_DescriptorSize =
		pDevice->GetDescriptorHandleIncrementSize(pDesc->Type);

	*ppPool = instance; // ppPool is an output parameter whose type is Pool pointer.

	return true;
}
