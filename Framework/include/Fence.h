#pragma once

#include <d3d12.h>
#include <ComPtr.h>

//
// Fence class
//
class Fence
{

public:

	//! @brief constructor
	Fence();

	//! @brief destructor
	~Fence();

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(ID3D12Device* pDevice);

	//! @brief end
	void Term();

	//! @brief wait for specified time until being signal
	//! 
	//! @param[in] pQueue command queue
	//! @param[in] timeout timeout time (milisec)
	void Wait(ID3D12CommandQueue* pQueue, UINT timeout);

	//! @brief wait until being signal
	//! 
	//! @param[in] pQueue command queue
	void Sync(ID3D12CommandQueue* pQueue);

private:

	ComPtr<ID3D12Fence> m_pFence; //!< fence
	HANDLE m_Event; //!< event
	UINT m_Counter; //!< curernt counter

	Fence(const Fence&) = delete;
	void operator = (const Fence&) = delete;
};
