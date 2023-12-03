#include "VertexBuffer.h"

//
// VertexBuffer class
//

// constructor
VertexBuffer::VertexBuffer()
	: m_pVB(nullptr)
{
	memset(&m_View, 0, sizeof(m_View)); // fill the content of m_View pointer with 0
}

// destructor
VertexBuffer::~VertexBuffer()
{
	Term();
}

// initialize
bool VertexBuffer::Init(ID3D12Device* pDevice, size_t size, size_t stride, const void* pInitData)
{
	// argument check
	if (pDevice == nullptr || size == 0 || stride == 0)
	{
		return false;
	}

	// heap property
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask = 1;
	prop.VisibleNodeMask = 1;

	// settings of resource
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
		IID_PPV_ARGS(m_pVB.GetAddressOf()));
	if (FAILED(hr))
	{
		return false;
	}

	// settings of vertex buffer view
	m_View.BufferLocation = m_pVB->GetGPUVirtualAddress();
	m_View.StrideInBytes = UINT(stride);
	m_View.SizeInBytes = UINT(size);

	// if there is a initialize data, write out
	if (pInitData != nullptr)
	{
		void* ptr = Map();
		if (ptr == nullptr)
		{
			return false;
		}

		memcpy(ptr, pInitData, size);

		m_pVB->Unmap(0, nullptr);
	}

	// normal end
	return true;
}

// end
void VertexBuffer::Term()
{
	m_pVB.Reset();
	memset(&m_View, 0, sizeof(m_View)); // fill out with zero
}

// memory mapping
void* VertexBuffer::Map() const
{
	void* ptr;
	auto hr = m_pVB->Map(0, nullptr, &ptr);
	if (FAILED(hr))
	{
		return nullptr;
	}

	return ptr;
}

// unmap memory
void VertexBuffer::Unmap()
{
	m_pVB->Unmap(0, nullptr);
}

// get vertex buffer view
D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetView() const
{
	return m_View;
}