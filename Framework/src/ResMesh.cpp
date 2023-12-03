#include "ResMesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <codecvt>
#include <cassert>

namespace {

	// convert to UTF-8
	std::string ToUTF8(const std::wstring& value)
	{
		auto length = WideCharToMultiByte(
			CP_UTF8, 0U, value.data(), -1, nullptr, 0, nullptr, nullptr);
		auto buffer = new char[length];

		WideCharToMultiByte(
			CP_UTF8, 0U, value.data(), -1, buffer, length, nullptr, nullptr);

		std::string result(buffer);
		delete[] buffer;
		buffer = nullptr;

		return result;
	}

	// convert to std::wstring
	std::wstring Convert(const aiString& path)
	{
		wchar_t temp[256] = {};
		size_t size;
		mbstowcs_s(&size, temp, path.C_Str(), 256); // convert multibite string to wide string
		return std::wstring(temp);
	}

	//
	// MeshLoader class
	//
	class MeshLoader
	{

	public:

		MeshLoader();
		~MeshLoader();

		bool Load(
			const wchar_t* filename,
			std::vector<ResMesh>& meshes,
			std::vector<ResMaterial>& materials);

	private:

		const aiScene* m_pScene = nullptr;

		void ParseMesh(ResMesh& dstMesh, const aiMesh* pSrcMesh); // convert mesh to specific data type
		void ParseMaterial(ResMaterial& dstMaterial, const aiMaterial* pSrcMaterial);
	};

	// constructor
	MeshLoader::MeshLoader()
		: m_pScene(nullptr)
	{
	}

	// destructor
	MeshLoader::~MeshLoader()
	{
	}

	// load mesh
	bool MeshLoader::Load
	(
		const wchar_t* filename,
		std::vector<ResMesh>& meshes,
		std::vector<ResMaterial>& materials
	)
	{
		if (filename == nullptr)
		{
			return false;
		}

		//convert wchar_t to char type(UTF-8)
		auto path = ToUTF8(filename);

		Assimp::Importer importer;
		unsigned int flag = 0;
		flag |= aiProcess_Triangulate;
		flag |= aiProcess_PreTransformVertices;
		flag |= aiProcess_CalcTangentSpace;
		flag |= aiProcess_GenSmoothNormals;
		flag |= aiProcess_GenUVCoords;
		flag |= aiProcess_RemoveRedundantMaterials;
		flag |= aiProcess_OptimizeMeshes;

		// read the file
		m_pScene = importer.ReadFile(path, flag);

		// check
		if (m_pScene == nullptr)
		{
			return false;
		}

		//alloc memory of mesh
		meshes.clear();
		meshes.resize(m_pScene->mNumMeshes);

		for (size_t i = 0; i < meshes.size(); ++i)
		{
			const auto pMesh = m_pScene->mMeshes[i];
			ParseMesh(meshes[i], pMesh);
		}

		// alloc meory of material
		materials.clear();
		materials.resize(m_pScene->mNumMaterials);

		// convert material data
		for (size_t i = 0; i < materials.size(); ++i)
		{
			const auto pMaterial = m_pScene->mMaterials[i];
			ParseMaterial(materials[i], pMaterial);
		}

		// clear as these are no longer necessary
		importer.FreeScene();
		m_pScene = nullptr;

		// normally end
		return true;
	}

	// analyze mesh data
	void MeshLoader::ParseMesh(ResMesh& dstMesh, const aiMesh* pSrcMesh)
	{
		// set material id
		dstMesh.MaterialId = pSrcMesh->mMaterialIndex;

		aiVector3D zero3D(0.0f, 0.0f, 0.0f);

		// alloc memory of vertex data (change the size of vector)
		dstMesh.Vertices.resize(pSrcMesh->mNumVertices);

		for (auto i = 0u; i < pSrcMesh->mNumVertices; ++i)
		{
			auto pPosition = &(pSrcMesh->mVertices[i]);
			auto pNormal = &(pSrcMesh->mNormals[i]);
			auto pTexCoord = (pSrcMesh->HasTextureCoords(0)) ? &(pSrcMesh->mTextureCoords[0][i]) : &zero3D;
			auto pTangent = (pSrcMesh->HasTangentsAndBitangents()) ? &(pSrcMesh->mTangents[i]) : &zero3D;

			dstMesh.Vertices[i] = MeshVertex(
				DirectX::XMFLOAT3(pPosition->x, pPosition->y, pPosition->z),
				DirectX::XMFLOAT3(pNormal->x, pNormal->y, pNormal->z),
				DirectX::XMFLOAT2(pTexCoord->x, pTexCoord->y),
				DirectX::XMFLOAT3(pTangent->x, pTangent->y, pTangent->z)
			);
		}

		// alloc memory of index of vertex
		dstMesh.Indices.resize(pSrcMesh->mNumFaces * 3);

		for (auto i = 0u; i < pSrcMesh->mNumFaces; ++i)
		{
			const auto& face = pSrcMesh->mFaces[i];
			assert(face.mNumIndices == 3); // as triangulated, it is necessarily 3. it just asserts the boolean in the parentheses is true

			dstMesh.Indices[i * 3 + 0] = face.mIndices[0];
			dstMesh.Indices[i * 3 + 1] = face.mIndices[1];
			dstMesh.Indices[i * 3 + 2] = face.mIndices[2];
		}
	}

	// analyze material data
	void MeshLoader::ParseMaterial(ResMaterial& dstMaterial, const aiMaterial* pSrcMaterial)
	{
		// ŠgŽU”½ŽË¬•ª
		{
			aiColor3D color(0.0f, 0.0f, 0.0f);

			if (pSrcMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
			{
				dstMaterial.Diffuse.x = color.r;
				dstMaterial.Diffuse.y = color.g;
				dstMaterial.Diffuse.z = color.b;
			}
			else
			{
				dstMaterial.Diffuse.x = 0.5f;
				dstMaterial.Diffuse.y = 0.5f;
				dstMaterial.Diffuse.z = 0.5f;
			}
		}

		// ‹¾–Ê”½ŽË¬•ª
		{
			aiColor3D color(0.0f, 0.0f, 0.0f);

			if (pSrcMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
			{
				dstMaterial.Specular.x = color.r;
				dstMaterial.Specular.y = color.g;
				dstMaterial.Specular.z = color.b;
			}
			else
			{
				dstMaterial.Specular.x = 0.0f;
				dstMaterial.Specular.y = 0.0f;
				dstMaterial.Specular.z = 0.0f;
			}
		}

		// ‹¾–Ê”½ŽË‹­“x
		{
			auto shininess = 0.0f;
			if (pSrcMaterial->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
			{
				dstMaterial.Shininess = shininess;
			}
			else
			{
				dstMaterial.Shininess = 0.0f;
			}
		}

		// diffuse map
		{
			aiString path;
			if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) == AI_SUCCESS)
			{
				dstMaterial.DiffuseMap = Convert(path);
			}
			else
			{
				dstMaterial.DiffuseMap.clear();
			}
		}

		// specular map
		{
			aiString path;
			if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_SPECULAR(0), path) == AI_SUCCESS)
			{
				dstMaterial.SpecularMap = Convert(path);
			}
			else
			{
				dstMaterial.SpecularMap.clear();
			}
		}

		// shininess map
		{
			aiString path;
			if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_SHININESS(0), path) == AI_SUCCESS)
			{
				dstMaterial.ShininessMap = Convert(path);
			}
			else
			{
				dstMaterial.ShininessMap.clear();
			}
		}

		// normal map
		{
			aiString path;
			if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_NORMALS(0), path) == AI_SUCCESS)
			{
				dstMaterial.NormalMap = Convert(path);
			}
			else
			{
				if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_HEIGHT(0), path) == AI_SUCCESS)
				{
					dstMaterial.NormalMap = Convert(path);
				}
				else
				{
					dstMaterial.NormalMap.clear();
				}
			}
		}

	}
} // namespace

//
// Constant Values.
//
const D3D12_INPUT_ELEMENT_DESC MeshVertex::InputElements[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};
const D3D12_INPUT_LAYOUT_DESC MeshVertex::InputLayout = { MeshVertex::InputElements, MeshVertex::InputElementCout };
static_assert(sizeof(MeshVertex) == 44, "Vertex struct/layout mismatch");

//
// load mesh
//
bool LoadMesh
(
	const wchar_t* filename,
	std::vector<ResMesh>& meshes,
	std::vector<ResMaterial>& materials
)
{
	MeshLoader loader;
	return loader.Load(filename, meshes, materials);
}