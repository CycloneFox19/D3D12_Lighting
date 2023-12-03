#include "IndexBuffer.h"

//
// IndexBuffer class
//

// constructor
IndexBuffer::IndexBuffer()
	: m_pIB(nullptr)
{
	memset(&m_View, 0, sizeof(m_View));
}

// destructor
IndexBuffer::~IndexBuffer()
{
	Term();
}

// initialize
bool IndexBuffer::Init(ID3D12Device* pDevice, size_t size, const uint32_t* pInitData)
{
	// heap property
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask = 1;
	prop.VisibleNodeMask = 1;

	// settings of the resource
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = UINT64(size);
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// generate resource
	auto hr = pDevice->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pIB.GetAddressOf()));
	if (FAILED(hr))
	{
		return false;
	}

	// settings of index buffer view
	m_View.BufferLocation = m_pIB->GetGPUVirtualAddress();
	m_View.Format = DXGI_FORMAT_R32_UINT;
	m_View.SizeInBytes = UINT(size);

	// if there's initialize data, write out
	if (pInitData != nullptr)
	{
		void* ptr = Map();
		if (ptr == nullptr)
		{
			return false;
		}

		memcpy(ptr, pInitData, size);

		m_pIB->Unmap(0, nullptr);
	}

	// normal end
	return true;
}

// end
void IndexBuffer::Term()
{
	m_pIB.Reset();
	memset(&m_View, 0, sizeof(m_View)); // fill the memory with 0
}

// mapping memory
uint32_t* IndexBuffer::Map()
{
	uint32_t* ptr;
	auto hr = m_pIB->Map(0, nullptr, reinterpret_cast<void**>(&ptr));
	if (FAILED(hr))
	{
		return nullptr;
	}

	return ptr;
}

// unmap memory
void IndexBuffer::Unmap()
{
	m_pIB->Unmap(0, nullptr);
}

// get index buffer view
D3D12_INDEX_BUFFER_VIEW IndexBuffer::GetView() const
{
	return m_View;
}