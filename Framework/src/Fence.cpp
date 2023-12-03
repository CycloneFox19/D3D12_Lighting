#include "Fence.h"

//
// Fence class
//

// constructor
Fence::Fence()
	: m_pFence(nullptr)
	, m_Event(nullptr)
	, m_Counter(0)
{
}

// destructor
Fence::~Fence()
{
	Term();
}

// initialize
bool Fence::Init(ID3D12Device* pDevice)
{
	if (pDevice == nullptr)
	{
		return false;
	}

	// generate event
	m_Event = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (m_Event == nullptr)
	{
		return false;
	}

	// generate fence
	auto hr = pDevice->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(m_pFence.GetAddressOf()));
	if (FAILED(hr))
	{
		return false;
	}

	// counter setting
	m_Counter = 1;

	// normal end
	return true;
}

// end
void Fence::Term()
{
	// close handle
	if (m_Event != nullptr)
	{
		CloseHandle(m_Event);
		m_Event = nullptr;
	}

	// abandon fence object
	m_pFence.Reset();

	// reset counter
	m_Counter = 0;
}

// wait for specified time until signal
void Fence::Wait(ID3D12CommandQueue* pQueue, UINT timeout)
{
	if (pQueue == nullptr)
	{
		return;
	}

	const auto fenceValue = m_Counter;

	// signal
	auto hr = pQueue->Signal(m_pFence.Get(), fenceValue);
	if (FAILED(hr))
	{
		return;
	}

	// increment counter
	++m_Counter;

	// wait if the preparation for next frame hasn't been done yet.
	if (m_pFence->GetCompletedValue() < fenceValue)
	{
		// set event when completed
		auto hr = m_pFence->SetEventOnCompletion(fenceValue, m_Event);
		if (FAILED(hr))
		{
			return;
		}

		// wait
		if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_Event, timeout, FALSE))
		{
			return;
		}
	}
}

// wait for signal
void Fence::Sync(ID3D12CommandQueue* pQueue)
{
	if (pQueue == nullptr)
	{
		return;
	}

	// signal
	auto hr = pQueue->Signal(m_pFence.Get(), m_Counter);
	if (FAILED(hr))
	{
		return;
	}

	// set event when completed
	hr = m_pFence->SetEventOnCompletion(m_Counter, m_Event);
	if (FAILED(hr))
	{
		return;
	}

	// wait
	if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_Event, INFINITE, FALSE))
	{
		return;
	}

	// increment counter
	++m_Counter;
}