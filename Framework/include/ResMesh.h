#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

//
// ResMaterial structure
//
struct ResMaterial
{
	DirectX::XMFLOAT3 Diffuse; //!< �g�U���ː���
	DirectX::XMFLOAT3 Specular; //!< ���ʔ��ː���
	float Alpha; //!< ���ߐ���
	float Shininess; //!< ���ʔ��ˋ��x
	std::wstring DiffuseMap; //!< �f�B�t���[�Y�}�b�v�t�@�C���p�X
	std::wstring SpecularMap; //!< �X�y�L�����[�}�b�v�t�@�C���p�X
	std::wstring ShininessMap; //!< �V���C�j�l�X�}�b�v�t�@�C���p�X
	std::wstring NormalMap; //!< �@���}�b�v�t�@�C���p�X
};

// Mesh Vertex structure
class MeshVertex
{
public:
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal; // �@��
	DirectX::XMFLOAT2 TexCoord;
	DirectX::XMFLOAT3 Tangent;

	MeshVertex() = default;

	MeshVertex(
		DirectX::XMFLOAT3 const& position,
		DirectX::XMFLOAT3 const& normal,
		DirectX::XMFLOAT2 const& texcoord,
		DirectX::XMFLOAT3 const& tangent)
		: Position(position)
		, Normal(normal)
		, TexCoord(texcoord)
		, Tangent(tangent)
	{
		// Do Nothing//
	}

	static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
	static const int InputElementCout = 4;
	static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCout];
};

//
// ResMesh structure
//
struct ResMesh
{
	std::vector<MeshVertex> Vertices;
	std::vector<uint32_t> Indices;
	uint32_t MaterialId;
};

//! @brief load mesh
//! 
//! @param[in] filename path of the file
//! @param[out] meshes container of mesh
//! @param[out] materials container of material
//! @retval true successfully loaded
//! @retval false failed to load
bool LoadMesh(
	const wchar_t* filename,
	std::vector<ResMesh>& meshes,
	std::vector<ResMaterial>& materials);
