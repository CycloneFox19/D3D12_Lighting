#include "Material.h"
#include "FileUtil.h"
#include "Logger.h"

namespace {
	// Constant values.
	constexpr wchar_t* DummyTag = L"";
}// namespace

//
// Material class
//

// constructor
Material::Material()
	: m_pDevice(nullptr)
	, m_pPool(nullptr)
{
}

// destructor
Material::~Material()
{
	Term();
}

// initialize
bool Material::Init
(
	ID3D12Device* pDevice,
	DescriptorPool* pPool,
	size_t bufferSize,
	size_t count
)
{
	if (pDevice == nullptr || pPool == nullptr || count == 0)
	{
		ELOG("Error : Invalid Argument.");
		return false;
	}

	Term();

	m_pDevice = pDevice;
	m_pDevice->AddRef(); // when copied comptr, then we need to increment the count

	m_pPool = pPool;
	m_pPool->AddRef();

	m_Subset.resize(count);

	// generate Dummy Texture
	{
		auto pTexture = new (std::nothrow) Texture();
		if (pTexture == nullptr)
		{
			return false;
		}

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = 1;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		if (!pTexture->Init(
			pDevice, 
			pPool, 
			&desc, 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			false))
		{
			ELOG("Error : Texture::Init() Failed.");
			pTexture->Term();
			delete pTexture;
			return false;
		}

		m_pTexture[DummyTag] = pTexture;
	}

	auto size = bufferSize * count;
	if (size > 0)
	{
		for (size_t i = 0; i < m_Subset.size(); ++i)
		{
			auto pBuffer = new (std::nothrow) ConstantBuffer();
			if (pBuffer == nullptr)
			{
				ELOG("Error : Out of memory");
				return false;
			}

			if (!pBuffer->Init(pDevice, pPool, bufferSize))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}

			m_Subset[i].pConstantBuffer = pBuffer;
			for (auto j = 0; j < TEXTURE_USAGE_COUNT; ++j)
			{
				m_Subset[i].TextureHandle[j].ptr = 0;
			}
		}
	}
	else
	{
		for (size_t i = 0; i < m_Subset.size(); ++i)
		{
			m_Subset[i].pConstantBuffer = nullptr;
			for (auto j = 0; j < TEXTURE_USAGE_COUNT; ++j)
			{
				m_Subset[i].TextureHandle[j].ptr = 0;
			}
		}
	}
	return true;
}

// end
void Material::Term()
{
	for (auto& itr : m_pTexture)
	{
		if (itr.second != nullptr) // means texture in itr
		{
			itr.second->Term();
			delete itr.second;
			itr.second = nullptr;
		}
	}

	for (size_t i = 0; i < m_Subset.size(); ++i)
	{
		if (m_Subset[i].pConstantBuffer != nullptr)
		{
			m_Subset[i].pConstantBuffer->Term();
			delete m_Subset[i].pConstantBuffer;
			m_Subset[i].pConstantBuffer = nullptr;
		}
	}

	m_pTexture.clear();
	m_Subset.clear();

	if (m_pDevice != nullptr)
	{
		m_pDevice->Release();
		m_pDevice = nullptr;
	}

	if (m_pPool != nullptr)
	{
		m_pPool->Release();
		m_pPool = nullptr;
	}
}

bool Material::SetTexture
(
	size_t index,
	TEXTURE_USAGE usage,
	const std::wstring& path,
	DirectX::ResourceUploadBatch& batch
)
{
	// check whether it is in range
	if (index >= GetCount())
	{
		return false;
	}

	// check whether it has been already applied
	if (m_pTexture.find(path) != m_pTexture.end())
	{
		m_Subset[index].TextureHandle[usage] = m_pTexture[path]->GetHandleGPU();
		return true;
	}

	// check whether filepath exists
	std::wstring findPath;
	if (!SearchFilePathW(path.c_str(), findPath))
	{
		// set dummy texture in case filepath does not exist
		m_Subset[index].TextureHandle[usage] = m_pTexture[DummyTag]->GetHandleGPU();
		return true;
	}

	// check if findpath is filename
	{
		if (PathIsDirectoryW(findPath.c_str()) != FALSE)
		{
			m_Subset[index].TextureHandle[usage] = m_pTexture[DummyTag]->GetHandleGPU();
			return true;
		}
	}

	// generate instance
	auto pTexture = new (std::nothrow) Texture();
	if (pTexture == nullptr)
	{
		ELOG("Error : Out of memory.");
		return false;
	}

	auto isSRGB = (TU_BASE_COLOR == usage) || (TU_DIFFUSE == usage) || (TU_SPECULAR == usage);

	// initialize
	if (!pTexture->Init(m_pDevice, m_pPool, findPath.c_str(), isSRGB, batch))
	{
		ELOG("Error : Texture::Init() Failed.");
		pTexture->Term();
		delete pTexture;
		return false;
	}

	// apply
	m_pTexture[path] = pTexture;
	m_Subset[index].TextureHandle[usage] = pTexture->GetHandleGPU();

	return true;
}

// get the pointer of constant buffer
void* Material::GetBufferPtr(size_t index) const
{
	if (index >= GetCount())
	{
		return nullptr;
	}

	return m_Subset[index].pConstantBuffer->GetPtr();
}

// get the address of constant buffer
D3D12_GPU_VIRTUAL_ADDRESS Material::GetBufferAddress(size_t index) const
{
	if (index >= GetCount())
	{
		return D3D12_GPU_VIRTUAL_ADDRESS();
	}

	return m_Subset[index].pConstantBuffer->GetAddress();
}

// get constant buffer handle
D3D12_GPU_DESCRIPTOR_HANDLE Material::GetBufferHandle(size_t index) const
{
	if (index >= GetCount())
	{
		return D3D12_GPU_DESCRIPTOR_HANDLE();
	}

	return m_Subset[index].pConstantBuffer->GetHandleGPU();
}

// get texture handle
D3D12_GPU_DESCRIPTOR_HANDLE Material::GetTextureHandle(size_t index, TEXTURE_USAGE usage) const
{
	if (index >= GetCount())
	{
		return D3D12_GPU_DESCRIPTOR_HANDLE();
	}

	return m_Subset[index].TextureHandle[usage];
}

// get material count
size_t Material::GetCount() const
{
	return m_Subset.size();
}