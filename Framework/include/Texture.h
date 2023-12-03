#pragma once

#include <d3d12.h>
#include <ComPtr.h>
#include <ResourceUploadBatch.h>

// Forward Declarations
class DescriptorHandle;
class DescriptorPool;

// Texture class
class Texture
{

public:

	//! @brief constructor
	Texture();

	//! @brief destructor
	~Texture();

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] pPool Descriptor Pool
	//! @param[in] filename name of file
	//! @param[out] batch update batch, contanins datas needed to renew texture
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		const wchar_t* filename,
		DirectX::ResourceUploadBatch& batch);

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] pPool descriptor pool
	//! @param[in] filename filename
	//! @param[in] isSRGB if you use SRGB format, then put true value
	//! @param[out] batch update batch. contains necessary data for updating texture
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		const wchar_t* filename,
		bool isSRGB,
		DirectX::ResourceUploadBatch& batch);

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] pPool descriptor pool
	//! @param[in] pDesc configuration settings
	//! @param[in] キューブマップである場合、trueを指定
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES initState,
		bool isCube);

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] pPool descriptor pool
	//! @param[in] pDesc configuration settings
	//! @param[in] isCube if texture is cube map, then specify true
	//! @param[in] isSRGB if you use SRGB format, then specify true
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(
		ID3D12Device* pDevice,
		DescriptorPool* pPool,
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES initState,
		bool isCube,
		bool isSRGB);

	//! @brief 終了処理
	void Term();

	//! @brief get CPU DescriptorHandle
	//! 
	//! @return return CPU DescriptorHandle
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const;

	//! @brief get GPU DescriptorHandle
	//! 
	//! @return return GPU DescriptorHandle
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const;

private:

	// private variables
	ComPtr<ID3D12Resource> m_pTex;
	DescriptorHandle* m_pHandle;
	DescriptorPool* m_pPool;

	// private methods
	Texture(const Texture&) = delete;
	void operator = (const Texture&) = delete;

	//! @brief シェーダリソースビューの設定を求める
	//! 
	//! @param[in] isCube キューブマップかどうか
	//! @return シェーダリソースビューの設定を返却
	D3D12_SHADER_RESOURCE_VIEW_DESC GetViewDesc(bool isCube);
};
