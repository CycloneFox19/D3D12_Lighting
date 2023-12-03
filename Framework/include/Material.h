#pragma once

#include <DescriptorPool.h>
#include <ResourceUploadBatch.h>
#include <Texture.h>
#include <ConstantBuffer.h>
#include <map>

//
// Material class
//
class Material
{

public:

	//
	// TEXTURE_USAAGE enum
	//
	enum TEXTURE_USAGE
	{
		TEXTURE_USAGE_DIFFUSE = 0, //!< use as diffuse map
		TEXTURE_USAGE_SPECULAR, //!< use as specular map
		TEXTURE_USAGE_SHININESS, //!< use as shininess map
		TEXTURE_USAGE_NORMAL, //!< use as normal map

		TEXTURE_USAGE_BASE_COLOR, //!< use as base color map
		TEXTURE_USAGE_METALLIC, //!< use as metallic map
		TEXTURE_USAGE_ROUGHNESS, //!< use as roughness map

		TEXTURE_USAGE_COUNT
	};

	//! @brief constructor
	Material();

	//! @brief destructor
	~Material();

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] pPool Descriptor pool (set the one for CBV_SRV_UAV)
	//! @param[in] bufferSize constant buffer size per one mateerial
	//! @param[in] count material count
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		size_t bufferSize,
		size_t count);

	//! @brief end
	void Term();

	//! @brief set texture
	//! 
	//! @param[in] index material index
	//! @param[in] usage texture usage
	//! @param[in] path texture path
	//! @param[out] batch resource upload batch
	//! @retval true successfully set
	//! @retval false failed to set
	bool SetTexture(
		size_t index,
		TEXTURE_USAGE usage,
		const std::wstring& path,
		DirectX::ResourceUploadBatch& batch);

	//! @brief get pointer of constant buffer
	//! 
	//! @param[in] index material index to get
	//! 
	//! @return 指定された番号に一致する定数バッファのポインタを返却する
	//! 一致するものが無い場合はnullptrを返却する
	void* GetBufferPtr(size_t index) const;

	//! @brief get pointer of constant buffer
	//! 
	//! @param[in] index get material index
	//! 
	//! @return 指定された番号に一致する定数バッファのポインタを返却する
	//! 一致するものが無い場合はnullptrを返却する
	template<typename T>
	T* GetBufferPtr(size_t index) const
	{
		return reinterpret_cast<T*>(GetBufferPtr(index));
	}

	//! @brief get GPU virtual address of constant buffer
	//! 
	//! @param[in] index get material index
	//! 
	//! @return return the current GPU virtual address of current constant buffer
	D3D12_GPU_VIRTUAL_ADDRESS GetBufferAddress(size_t index) const;

	//! @brief get buffer handle
	//! 
	//! @param[in] index material index to be gotten
	//! @return return descriptor handle of constant buffer that matches specified number
	D3D12_GPU_DESCRIPTOR_HANDLE GetBufferHandle(size_t index) const;

	//! @brief get texture handle
	//! 
	//! @param[in] index get material index
	//! @param[in] usage get texture usage
	//! @return 指定された番号に一致するテクスチャのディスクリプタハンドルを返却します
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(size_t index, TEXTURE_USAGE usage) const;

	//! @brief get material count
	//! 
	//! @return return material count
	size_t GetCount() const;

private:

	//
	// subset structure
	//
	struct Subset
	{
		ConstantBuffer* pConstantBuffer; //!< constant buffer
		D3D12_GPU_DESCRIPTOR_HANDLE TextureHandle[TEXTURE_USAGE_COUNT]; //!< texture handle
	};

	std::map<std::wstring, Texture*> m_pTexture; //!< texture
	std::vector<Subset> m_Subset; //!< subset
	ID3D12Device* m_pDevice; //!< device
	DescriptorPool* m_pPool; //!< descriptor pool (CBV_SRV_UAV)

	Material(const Material&) = delete;
	void operator = (const Material&) = delete;
};

constexpr auto TU_DIFFUSE = Material::TEXTURE_USAGE_DIFFUSE;
constexpr auto TU_SPECULAR = Material::TEXTURE_USAGE_SPECULAR;
constexpr auto TU_SHININESS = Material::TEXTURE_USAGE_SHININESS;
constexpr auto TU_NORMAL = Material::TEXTURE_USAGE_NORMAL;

constexpr auto TU_BASE_COLOR = Material::TEXTURE_USAGE_BASE_COLOR;
constexpr auto TU_METALLIC = Material::TEXTURE_USAGE_METALLIC;
constexpr auto TU_ROUGHNESS = Material::TEXTURE_USAGE_ROUGHNESS;
