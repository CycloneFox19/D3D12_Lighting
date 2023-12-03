#pragma once

#include <ResMesh.h>
#include <VertexBuffer.h>>
#include <IndexBuffer.h>

//
// Mesh class
//
class Mesh
{

public:

	//! @brief constructor
	Mesh();

	//! @brief destructor
	virtual ~Mesh();

	//! @brief initialize
	//! 
	//! @param[in] pDevice device
	//! @param[in] resource resource mesh
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool Init(ID3D12Device* pDevice, const ResMesh& resource);

	//! @brief end
	void Term();

	//! @brief draw
	//! 
	//! @param[in] pCmdList command list
	void Draw(ID3D12GraphicsCommandList* pCmdList);

	//! @brief get material id
	//! 
	//! @return return material id
	uint32_t GetMaterialId() const;

private:

	VertexBuffer m_VB; //!< vertex buffer
	IndexBuffer m_IB; //!< index buffer
	uint32_t m_MaterialId; //!< material id
	uint32_t m_IndexCount; //!< index count

	Mesh(const Mesh&) = delete;
	void operator = (const Mesh&) = delete;
};