#pragma once

#include <d3d12.h>
#include <ComPtr.h>
#include <cstdint>
#include <vector>

//
// CommandList class
//
class CommandList
{

public:

	//! @brief constructor
	CommandList();

	//! @brief destructor
	~CommandList();

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] type type of commandlist
	//! @param[in] count number of allocator. if it is double buffer, then set two
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, uint32_t count);

	//! @brief end
	void Term();

	//! @brief get reset commandlist
	//! 
	//! @return return reset commandlist
	ID3D12GraphicsCommandList* Reset();

private:

	ComPtr<ID3D12GraphicsCommandList> m_pCmdList; //!< commandlist
	std::vector<ComPtr<ID3D12CommandAllocator>> m_pAllocators; //!< command allocator
	uint32_t m_Index; //!< index of allocator

	CommandList(const CommandList&) = delete;
	void operator = (const CommandList&) = delete;
};
