#include "SampleApp.h"
#include "FileUtil.h"
#include "Logger.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "SimpleMath.h"

// using statements
using namespace DirectX::SimpleMath;

namespace {
	//
	// COLOR_SPACE_TYPE enum
	//
	enum COLOR_SPACE_TYPE
	{
		COLOR_SPACE_BT709, // ITU-R BT.709
		COLOR_SPACE_BT2100_PQ, // ITU-R BT.2100 PQ System
	};

	//
	// TONEMAP_TYPE enum
	//
	enum TONEMAP_TYPE
	{
		TONEMAP_NONE = 0, // no tonemap
		TONEMAP_REINHARD, // Reinhard tonemap
		TONEMAP_GT, // GT tonemap
	};

	//
	// CbTonemap structure
	//
	struct alignas(256) CbTonemap
	{
		int Type; // type of tonemap
		int ColorSpace; // output colorspace
		float BaseLuminance; // standard luminance[nit]
		float MaxLuminance; // maximum luminance[nit]
	};

	//
	// CbMesh structure
	//
	struct alignas(256) CbMesh
	{
		Matrix World; //!< world matrix
	};

	//
	// CbTransform structure
	//
	struct alignas(256) CbTransform
	{
		Matrix View; //!< view matrix
		Matrix Proj; //!< projection matrix
	};

	//
	// CbLight structure
	//
	struct alignas(256) CbLight
	{
		Vector3 LightPosition;
		float LightInvSqrRadius;
		Vector3 LightColor;
		float LightIntensity;
	};

	//
	// CbCamera structure
	//
	struct alignas(256) CbCamera
	{
		Vector3 CameraPosition; //!< position of camera
	};

	//
	// CbMaterial structure
	//
	struct alignas(256) CbMaterial
	{
		Vector3 BaseColor; //!< base color
		float Alpha; //!< opacity
		float Roughness; //!< roughness of surface (range : [0, 1])
		float Metallic; //!< metallicity (range : [0, 1])
	};

	// obtain chromaticity coord
	inline UINT16 GetChromaticityCoord(double value)
	{
		return UINT16(value * 50000);
	}

	// Calculate point light parameter
	CbLight ComputePointLight(const Vector3& pos, float radius, const Vector3& color, float intensity)
	{
		CbLight result;
		result.LightPosition = pos;
		result.LightInvSqrRadius = 1.0f / (radius * radius);
		result.LightColor = color;
		result.LightIntensity = intensity;

		return result;
	}

	// change light color depending on time
	Vector3 CalcLightColor(float time)
	{
		auto c = fmodf(time, 3.0f);
		auto result = Vector3(0.25f, 0.25f, 0.25f);

		if (c < 1.0f)
		{
			result.x += 1.0f - c;
			result.y += c;
		}
		else if (c < 2.0f)
		{
			c -= 1.0f;
			result.y += 1.0f - c;
			result.z += c;
		}
		else
		{
			c -= 2.0f;
			result.z += 1.0f - c;
			result.x += c;
		}

		return result;
	}
} // namespace

//
// SampleApp class
//

// constructor
SampleApp::SampleApp(uint32_t width, uint32_t height)
	: App(width, height, DXGI_FORMAT_R10G10B10A2_UNORM)
	, m_TonemapType(TONEMAP_NONE)
	, m_ColorSpace(COLOR_SPACE_BT709)
	, m_BaseLuminance(100.0f)
	, m_MaxLuminance(100.0f)
	, m_Exposure(1.0f)
	, m_RotateAngle(0.0f)
{
}

// destructor
SampleApp::~SampleApp()
{
}

// initialize
bool SampleApp::OnInit()
{
	// load mesh
	{
		std::wstring path;

		// search file path
		if (!SearchFilePath(L"res/material_test/material_test.obj", path))
		{
			ELOG("Error : File Not Found.");
			return false;
		}

		std::wstring dir = GetDirectoryPath(path.c_str());

		std::vector<ResMesh> resMesh;
		std::vector<ResMaterial> resMaterial;

		// load mesh resource
		if (!LoadMesh(path.c_str(), resMesh, resMaterial))
		{
			ELOG("Error : Load Mesh Failed. filepath = %ls", path.c_str());
			return false;
		}

		// reserve memory
		m_pMesh.reserve(resMesh.size());

		// initialize mesh
		for (size_t i = 0; i < resMesh.size(); ++i)
		{
			// generate mesh
			auto mesh = new (std::nothrow) Mesh();

			// check
			if (mesh == nullptr)
			{
				ELOG("Error : Out of memory.");
				return false;
			}

			// intialize
			if (!mesh->Init(m_pDevice.Get(), resMesh[i]))
			{
				ELOG("Error : Mesh Initialize Failed.");
				delete mesh;
				return false;
			}

			// if succeeded, subscribe
			m_pMesh.push_back(mesh);
		}

		// optimize memory
		m_pMesh.shrink_to_fit();

		// initialzie material
		if (!m_Material.Init(
			m_pDevice.Get(),
			m_pPool[POOL_TYPE_RES],
			sizeof(CbMaterial),
			resMaterial.size()))
		{
			ELOG("Error : Material::Init() Failed.");
			return false;
		}

		// prepare resource batch
		DirectX::ResourceUploadBatch batch(m_pDevice.Get());

		// begin batch
		batch.Begin();

		// set material and texture
		{
			/* here we're hard coding */
			m_Material.SetTexture(0, TU_BASE_COLOR, dir + L"wall_bc.dds", batch);
			m_Material.SetTexture(0, TU_METALLIC, dir + L"wall_m.dds", batch);
			m_Material.SetTexture(0, TU_ROUGHNESS, dir + L"wall_r.dds", batch);
			m_Material.SetTexture(0, TU_NORMAL, dir + L"wall_n.dds", batch);

			m_Material.SetTexture(1, TU_BASE_COLOR, dir + L"matball_bc.dds", batch);
			m_Material.SetTexture(1, TU_METALLIC, dir + L"matball_m.dds", batch);
			m_Material.SetTexture(1, TU_ROUGHNESS, dir + L"matball_r.dds", batch);
			m_Material.SetTexture(1, TU_NORMAL, dir + L"matball_n.dds", batch);
		}

		// end batch
		auto future = batch.End(m_pQueue.Get());

		// wait for complete of batch
		future.wait();
	}

	// settings of light buffer
	{
		for (auto i = 0; i < FrameCount; ++i)
		{
			if (!m_LightCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbLight)))
			{
				ELOG("Error : ConstnatBuffer::Init() Failed.");
			}
			
			auto ptr = m_LightCB[i].GetPtr<CbLight>();
			*ptr = ComputePointLight(Vector3(0.0f, 1.0f, 1.5f), 1.0f, Vector3(1.0f, 0.5f, 0.5f), 10.0f);
		}
	}

	// settings of camera buffer
	{
		for (auto i = 0; i < FrameCount; ++i)
		{
			if (!m_CameraCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbCamera)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}
		}
	}

	// generate color target for scene
	{
		float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
		if (!m_SceneColorTarget.Init(
			m_pDevice.Get(),
			m_pPool[POOL_TYPE_RTV],
			m_pPool[POOL_TYPE_RES],
			m_Width,
			m_Height,
			DXGI_FORMAT_R10G10B10A2_UNORM,
			clearColor))
		{
			ELOG("Error : ColorTarget::Init() Failed.");
			return false;
		}
	}

	// generate depth target for scene
	{
		if (!m_SceneDepthTarget.Init(
			m_pDevice.Get(),
			m_pPool[POOL_TYPE_DSV],
			nullptr,
			m_Width,
			m_Height,
			DXGI_FORMAT_D32_FLOAT,
			1.0f,
			0))
		{
			ELOG("Error : DepthTarget::Init() Failed.");
			return false;
		}
	}

	// generate root signature
	{
		RootSignature::Desc desc;
		desc.Begin(8)
			.SetCBV(ShaderStage::VS, 0, 0)
			.SetCBV(ShaderStage::VS, 1, 1)
			.SetCBV(ShaderStage::PS, 2, 1)
			.SetCBV(ShaderStage::PS, 3, 2)
			.SetSRV(ShaderStage::PS, 4, 0)
			.SetSRV(ShaderStage::PS, 5, 1)
			.SetSRV(ShaderStage::PS, 6, 2)
			.SetSRV(ShaderStage::PS, 7, 3)
			.AddStaticSmp(ShaderStage::PS, 0, SamplerState::LinearWrap)
			.AddStaticSmp(ShaderStage::PS, 1, SamplerState::LinearWrap)
			.AddStaticSmp(ShaderStage::PS, 2, SamplerState::LinearWrap)
			.AddStaticSmp(ShaderStage::PS, 3, SamplerState::LinearWrap)
			.AllowIL()
			.End();

		if (!m_SceneRootSig.Init(m_pDevice.Get(), desc.GetDesc()))
		{
			ELOG("Error : RootSignature::Init() Failed.");
			return false;
		}
	}

	// generate pipeline state for scene
	{
		std::wstring vsPath;
		std::wstring psPath;

		// search for vertex shader
		if (!SearchFilePath(L"BasicVS.cso", vsPath))
		{
			ELOG("Error : Vertex Shader Not Found.");
			return false;
		}

		// search for pixel shader
		if (!SearchFilePath(L"BasicPS.cso", psPath))
		{
			ELOG("Error : Pixel Shader Not Found.");
			return false;
		}

		ComPtr<ID3DBlob> pVSBlob;
		ComPtr<ID3DBlob> pPSBlob;

		// read vertex shaader
		auto hr = D3DReadFileToBlob(vsPath.c_str(), pVSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			ELOG("Error : D3DReadFileToBlob() Failed. path = %ls", vsPath.c_str());
			return false;
		}

		// read pixel shader
		hr = D3DReadFileToBlob(psPath.c_str(), pPSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			ELOG("Error : D3DReadFileToBlob() Failed. path = %ls", psPath.c_str());
			return false;
		}

		D3D12_INPUT_ELEMENT_DESC elements[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// set graphics pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.InputLayout = { elements, 4 };
		desc.pRootSignature = m_SceneRootSig.GetPtr();
		desc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
		desc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
		desc.RasterizerState = DirectX::CommonStates::CullNone;
		desc.BlendState = DirectX::CommonStates::Opaque;
		desc.DepthStencilState = DirectX::CommonStates::DepthDefault;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = m_SceneColorTarget.GetRTVDesc().Format;
		desc.DSVFormat = m_SceneDepthTarget.GetDSVDesc().Format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		// generate pipeline state
		hr = m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pScenePSO.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
			return false;
		}
	}

	// generate root signature for tonemap
	{
		RootSignature::Desc desc;
		desc.Begin(2)
			.SetCBV(ShaderStage::PS, 0, 0)
			.SetSRV(ShaderStage::PS, 1, 0)
			.AddStaticSmp(ShaderStage::PS, 0, SamplerState::LinearWrap)
			.AllowIL()
			.End();

		if (!m_TonemapRootSig.Init(m_pDevice.Get(), desc.GetDesc()))
		{
			ELOG("Error : RootSignature::Init() Failed.");
			return false;
		}
	}

	// generate pipeline state for tonemap
	{
		std::wstring vsPath;
		std::wstring psPath;

		// search for vertex shader
		if (!SearchFilePath(L"TonemapVS.cso", vsPath))
		{
			ELOG("Error : Vertex Shader Not Found.");
			return false;
		}

		// search for pixel shader
		if (!SearchFilePath(L"TonemapPS.cso", psPath))
		{
			ELOG("Error : Pixel Shader Not Found.");
			return false;
		}

		ComPtr<ID3DBlob> pVSBlob;
		ComPtr<ID3DBlob> pPSBlob;

		// read vertex shader
		auto hr = D3DReadFileToBlob(vsPath.c_str(), pVSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			ELOG("Error : D3DReadFileToBlob() Failed. path = %ls", vsPath.c_str());
			return false;
		}

		// read pixel shader
		hr = D3DReadFileToBlob(psPath.c_str(), pPSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			ELOG("Error : D3DReadFileToBlob() Failed. path = %ls", psPath.c_str());
			return false;
		}

		D3D12_INPUT_ELEMENT_DESC elements[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// set graphics pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.InputLayout = { elements, 2 };
		desc.pRootSignature = m_TonemapRootSig.GetPtr();
		desc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
		desc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
		desc.RasterizerState = DirectX::CommonStates::CullNone;
		desc.BlendState = DirectX::CommonStates::Opaque;
		desc.DepthStencilState = DirectX::CommonStates::DepthDefault;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = m_ColorTarget[0].GetRTVDesc().Format;
		desc.DSVFormat = m_DepthTarget.GetDSVDesc().Format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		// generate pipeline state
		hr = m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pTonemapPSO.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
			return false;
		}
	}

	// generate vertex buffer
	{
		struct Vertex
		{
			float px;
			float py;

			float tx;
			float ty;
		};

		if (!m_QuadVB.Init<Vertex>(m_pDevice.Get(), 3))
		{
			ELOG("Error : VertexBuffer::Init() Failed.");
			return false;
		}

		auto ptr = m_QuadVB.Map<Vertex>();
		assert(ptr != nullptr);
		ptr[0].px = -1.0f; ptr[0].py =  1.0f; ptr[0].tx = 0.0f; ptr[0].ty = -1.0f;
		ptr[1].px =  3.0f; ptr[1].py =  1.0f; ptr[1].tx = 2.0f; ptr[1].ty = -1.0f;
		ptr[2].px = -1.0f; ptr[2].py = -3.0f; ptr[2].tx = 0.0f; ptr[2].ty =  1.0f;
		m_QuadVB.Unmap();
	}

	// generate vertex buffer for wall
	{
		struct BasicVertex
		{
			Vector3 Position;
			Vector3 Normal;
			Vector2 TexCoord;
			Vector3 Tangent;
		};

		if (!m_WallVB.Init<BasicVertex>(m_pDevice.Get(), 6))
		{
			ELOG("Error : VertexBuffer::Init() Failed.");
			return false;
		}

		auto size = 10.0f;
		auto ptr = m_WallVB.Map<BasicVertex>();
		assert(ptr != nullptr);

		ptr[0].Position = Vector3(-size, size, 0.0f);
		ptr[0].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[0].TexCoord = Vector2(0.0f, 1.0f);
		ptr[0].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		ptr[1].Position = Vector3(size, size, 0.0f);
		ptr[1].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[1].TexCoord = Vector2(1.0f, 1.0f);
		ptr[1].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		ptr[2].Position = Vector3(size, -size, 0.0f);
		ptr[2].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[2].TexCoord = Vector2(1.0f, 0.0f);
		ptr[2].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		ptr[3].Position = Vector3(-size, size, 0.0f);
		ptr[3].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[3].TexCoord = Vector2(0.0f, 1.0f);
		ptr[3].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		ptr[4].Position = Vector3(size, -size, 0.0f);
		ptr[4].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[4].TexCoord = Vector2(1.0f, 0.0f);
		ptr[4].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		ptr[5].Position = Vector3(-size, -size, 0.0f);
		ptr[5].Normal = Vector3(0.0f, 0.0f, 1.0f);
		ptr[5].TexCoord = Vector2(0.0f, 0.0f);
		ptr[5].Tangent = Vector3(1.0f, 0.0f, 0.0f);

		m_WallVB.Unmap();
	}

	for (auto i = 0; i < FrameCount; ++i)
	{
		if (!m_TonemapCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbTonemap)))
		{
			ELOG("Error : ConstantBuffer::Init() Failed.");
			return false;
		}
	}

	// generate constant buffer for transform matrix
	{
		for (auto i = 0u; i < FrameCount; ++i)
		{
			// initialize constant buffer
			if (!m_TransformCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbTransform)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}

			// camera settings
			auto eyePos = Vector3(0.0f, 1.0f, 2.0f);
			auto targetPos = Vector3::Zero;
			auto upward = Vector3::UnitY;

			// set fov and aspect
			auto fovY = DirectX::XMConvertToRadians(37.5f);
			auto aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

			// set transform matrix
			auto ptr = m_TransformCB[i].GetPtr<CbTransform>();
			ptr->View = Matrix::CreateLookAt(eyePos, targetPos, upward);
			ptr->Proj = Matrix::CreatePerspectiveFieldOfView(fovY, aspect, 1.0f, 1000.0f);
		}

		m_RotateAngle = DirectX::XMConvertToRadians(-60.0f);
	}

	// generate buffer for mesh
	{
		for (auto i = 0; i < FrameCount; ++i)
		{
			if (!m_MeshCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbMesh)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed.");
				return false;
			}

			auto ptr = m_MeshCB[i].GetPtr<CbMesh>();
			ptr->World = Matrix::Identity;
		}
	}

#if 0
	// load texture
	{
		DirectX::ResourceUploadBatch batch(m_pDevice.Get());

		// begin batch
		batch.Begin();

		// write processing which needs texture reading

		// end batch
		auto future = batch.End(m_pQueue.Get());

		// wait for complete
		future.wait();
	}
#endif

	// record starting time
	m_StartTime = std::chrono::system_clock::now();

	return true;

}

// end
void SampleApp::OnTerm()
{
	m_QuadVB.Term();
	for (auto i = 0; i < FrameCount; ++i)
	{
		m_TonemapCB[i].Term();
		m_LightCB[i].Term();
		m_CameraCB[i].Term();
		m_TransformCB[i].Term();
	}

	// abandon mesh
	for (size_t i = 0; i < m_pMesh.size(); ++i)
	{
		SafeTerm(m_pMesh[i]);
	}
	m_pMesh.clear();
	m_pMesh.shrink_to_fit();

	// abandon material
	m_Material.Term();

	for (auto i = 0; i < FrameCount; ++i)
	{
		m_MeshCB[i].Term();
	}

	m_SceneColorTarget.Term();
	m_SceneDepthTarget.Term();

	m_pScenePSO.Reset();
	m_SceneRootSig.Term();

	m_pTonemapPSO.Reset();
	m_TonemapRootSig.Term();
}

// processing that is done on render
void SampleApp::OnRender()
{
	// start recording commandlist
	auto pCmd = m_CommandList.Reset();

	ID3D12DescriptorHeap* const pHeaps[] = {
		m_pPool[POOL_TYPE_RES]->GetHeap(),
	};

	pCmd->SetDescriptorHeaps(1, pHeaps);

	{
		// get descriptor
		auto handleRTV = m_SceneColorTarget.GetHandleRTV();
		auto handleDSV = m_SceneDepthTarget.GetHandleDSV();

		// set resource barrier for writing
		DirectX::TransitionResource(
			pCmd,
			m_SceneColorTarget.GetResource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		// set render target
		pCmd->OMSetRenderTargets(1, &handleRTV->HandleCPU, FALSE, &handleDSV->HandleCPU);

		// clear render target
		m_SceneColorTarget.ClearView(pCmd);
		m_SceneColorTarget.ClearView(pCmd);

		// draw scene
		DrawScene(pCmd);

		// set resource barrier for writing
		DirectX::TransitionResource(
			pCmd,
			m_SceneColorTarget.GetResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// draw in frame buffer
	{
		// resource barrier for writing
		DirectX::TransitionResource(
			pCmd,
			m_ColorTarget[m_FrameIndex].GetResource(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		// get descriptor
		auto handleRTV = m_ColorTarget[m_FrameIndex].GetHandleRTV();
		auto handleDSV = m_DepthTarget.GetHandleDSV();

		// set render target
		pCmd->OMSetRenderTargets(1, &handleRTV->HandleCPU, FALSE, &handleDSV->HandleCPU);

		// clear render target
		m_ColorTarget[m_FrameIndex].ClearView(pCmd);
		m_DepthTarget.ClearView(pCmd);

		// apply tonemap
		DrawTonemap(pCmd);

		// settings of resource barrier fo writing
		DirectX::TransitionResource(
			pCmd,
			m_ColorTarget[m_FrameIndex].GetResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
	}

	// finish recording commandlist
	pCmd->Close();

	// execute commandlist
	ID3D12CommandList* pLists[] = { pCmd };
	m_pQueue->ExecuteCommandLists(1, pLists);

	// show on screen
	Present(1);
}

// draw scene
void SampleApp::DrawScene(ID3D12GraphicsCommandList* pCmd)
{
	auto cameraPos = Vector3(-4.0f, 1.0f, 2.5f);

	auto currTime = std::chrono::system_clock::now();
	auto dt = float(std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_StartTime).count()) / 1000.0f;
	auto lightColor = CalcLightColor(dt * 0.25f);

	// update light buffer
	{
		auto matrix = Matrix::CreateRotationY(m_RotateAngle);
		auto pos = Vector3::Transform(Vector3(0.0f, 0.25f, 0.75f), matrix);

		auto ptr = m_LightCB[m_FrameIndex].GetPtr<CbLight>();
		*ptr = ComputePointLight(pos, 2.0f, lightColor, 100.0f);

		m_RotateAngle += 0.025f;
	}

	// update camera buffer
	{
		auto ptr = m_CameraCB[m_FrameIndex].GetPtr<CbCamera>();
		ptr->CameraPosition = cameraPos;
	}

	// update world matrix of mesh
	{
		auto ptr = m_MeshCB[m_FrameIndex].GetPtr<CbMesh>();
		ptr->World = Matrix::Identity;
	}

	// update transform parameters
	{
		auto fovY = DirectX::XMConvertToRadians(37.5f);
		auto aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

		auto ptr = m_TransformCB[m_FrameIndex].GetPtr<CbTransform>();
		ptr->View = Matrix::CreateLookAt(cameraPos, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
		ptr->Proj = Matrix::CreatePerspectiveFieldOfView(fovY, aspect, 1.0f, 1000.0f);
	}

	pCmd->SetGraphicsRootSignature(m_SceneRootSig.GetPtr());
	pCmd->SetGraphicsRootDescriptorTable(0, m_TransformCB[m_FrameIndex].GetHandleGPU());
	pCmd->SetGraphicsRootDescriptorTable(2, m_LightCB[m_FrameIndex].GetHandleGPU());
	pCmd->SetGraphicsRootDescriptorTable(3, m_CameraCB[m_FrameIndex].GetHandleGPU());
	pCmd->SetPipelineState(m_pScenePSO.Get());
	pCmd->RSSetViewports(1, &m_Viewport);
	pCmd->RSSetScissorRects(1, &m_Scissor);

	// draw object
	{
		pCmd->SetGraphicsRootDescriptorTable(1, m_MeshCB[m_FrameIndex].GetHandleGPU());
		DrawMesh(pCmd);
	}
}

// draw mesh
void SampleApp::DrawMesh(ID3D12GraphicsCommandList* pCmd)
{
	for (size_t i = 0; i < m_pMesh.size(); ++i)
	{
		// get material ID
		auto id = m_pMesh[i]->GetMaterialId();

		// set texture
		pCmd->SetGraphicsRootDescriptorTable(4, m_Material.GetTextureHandle(id, TU_BASE_COLOR));
		pCmd->SetGraphicsRootDescriptorTable(5, m_Material.GetTextureHandle(id, TU_METALLIC));
		pCmd->SetGraphicsRootDescriptorTable(6, m_Material.GetTextureHandle(id, TU_ROUGHNESS));
		pCmd->SetGraphicsRootDescriptorTable(7, m_Material.GetTextureHandle(id, TU_NORMAL));

		// draw mesh
		m_pMesh[i]->Draw(pCmd);
	}
}

// apply tonemap
void SampleApp::DrawTonemap(ID3D12GraphicsCommandList* pCmd)
{
	// update constant buffer
	{
		auto ptr = m_TonemapCB[m_FrameIndex].GetPtr<CbTonemap>();
		ptr->Type = m_TonemapType;
		ptr->ColorSpace = m_ColorSpace;
		ptr->BaseLuminance = m_BaseLuminance;
		ptr->MaxLuminance = m_MaxLuminance;
	}

	pCmd->SetGraphicsRootSignature(m_TonemapRootSig.GetPtr());
	pCmd->SetGraphicsRootDescriptorTable(0, m_TonemapCB[m_FrameIndex].GetHandleGPU());
	pCmd->SetGraphicsRootDescriptorTable(1, m_SceneColorTarget.GetHandleSRV()->HandleGPU);

	pCmd->SetPipelineState(m_pTonemapPSO.Get());
	pCmd->RSSetViewports(1, &m_Viewport);
	pCmd->RSSetScissorRects(1, &m_Scissor);

	pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmd->IASetVertexBuffers(0, 1, &m_QuadVB.GetView());
	pCmd->DrawInstanced(3, 1, 0, 0);
}

void SampleApp::ChangeDisplayMode(bool hdr)
{
	if (hdr)
	{
		if (!IsSupportHDR())
		{
			MessageBox(
				nullptr,
				TEXT("This display does not support HDR."),
				TEXT("HDR is not supported."),
				MB_OK | MB_ICONINFORMATION);
			ELOG("Error : Display not support HDR.");
			return;
		}

		auto hr = m_pSwapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
		if (FAILED(hr))
		{
			MessageBox(
				nullptr,
				TEXT("failed to set the color range of ITU-R BT.2100 PQ System"),
				TEXT("failed to set color range"),
				MB_OK | MB_ICONERROR);
			ELOG("Error : IDXGISwapChain::SetColorSpace1() Failed.");
			return;
		}

		DXGI_HDR_METADATA_HDR10 metaData = {};

		// Set primary stimulus and white point of ITU-R BT.2100
		metaData.RedPrimary[0] = GetChromaticityCoord(0.708);
		metaData.RedPrimary[1] = GetChromaticityCoord(0.292);
		metaData.BluePrimary[0] = GetChromaticityCoord(0.170);
		metaData.BluePrimary[1] = GetChromaticityCoord(0.797);
		metaData.GreenPrimary[0] = GetChromaticityCoord(0.131);
		metaData.GreenPrimary[1] = GetChromaticityCoord(0.046);
		metaData.WhitePoint[0] = GetChromaticityCoord(0.3127);
		metaData.WhitePoint[1] = GetChromaticityCoord(0.3290);

		// set maximum and minimumluminance that display supports
		metaData.MaxMasteringLuminance = UINT(GetMaxLuminance() * 10000);
		metaData.MinMasteringLuminance = UINT(GetMinLuminance() * 0.001);

		// set maximul value to 2000 [nit]
		metaData.MaxContentLightLevel = 2000;

		hr = m_pSwapChain->SetHDRMetaData(
			DXGI_HDR_METADATA_TYPE_HDR10,
			sizeof(DXGI_HDR_METADATA_HDR10),
			&metaData);
		if (FAILED(hr))
		{
			ELOG("Error : IDXGISwapChain::SetHDRMetaData() Failed.");
		}

		m_BaseLuminance = 100.0f;
		m_MaxLuminance = GetMaxLuminance();

		// show dialogue which notifies success
		std::string message;
		message += "Changed settings for HDR Display\n\n";
		message += "ColorSpace : ITU-R BT.2100 PQ\n";
		message += "Max luminance : ";
		message += std::to_string(GetMaxLuminance());
		message += " [nit]\n";
		message += "Min luminance : ";
		message += std::to_string(GetMinLuminance());
		message += " [nit]\n";

		MessageBoxA(
			nullptr,
			message.c_str(),
			"Succeeded in set HDR",
			MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		auto hr = m_pSwapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
		if (FAILED(hr))
		{
			MessageBox(
				nullptr,
				TEXT("failed to set color range of ITU-R BT.709"),
				TEXT("failed in setting color range"),
				MB_OK | MB_ICONERROR);
			ELOG("Error : IDXGISwapChain::SetColorSpace1() Failed.");
			return;
		}

		DXGI_HDR_METADATA_HDR10 metaData = {};

		// Set Primary stimulus and white point of ITU-R BT.709
		metaData.RedPrimary[0] = GetChromaticityCoord(0.640);
		metaData.RedPrimary[1] = GetChromaticityCoord(0.330);
		metaData.BluePrimary[0] = GetChromaticityCoord(0.300);
		metaData.BluePrimary[1] = GetChromaticityCoord(0.600);
		metaData.GreenPrimary[0] = GetChromaticityCoord(0.150);
		metaData.GreenPrimary[1] = GetChromaticityCoord(0.060);
		metaData.WhitePoint[0] = GetChromaticityCoord(0.3127);
		metaData.WhitePoint[1] = GetChromaticityCoord(0.3290);

		// set max and min luminance which display supports
		metaData.MaxMasteringLuminance = UINT(GetMaxLuminance() * 10000);
		metaData.MinMasteringLuminance = UINT(GetMinLuminance() * 0.001);

		// set maximum value to 100[nit]
		metaData.MaxContentLightLevel = 100;

		hr = m_pSwapChain->SetHDRMetaData(
			DXGI_HDR_METADATA_TYPE_HDR10,
			sizeof(DXGI_HDR_METADATA_HDR10),
			&metaData);
		if (FAILED(hr))
		{
			ELOG("Error : IDXGISwapChain::SetHDRMetaData() Failed.");
		}

		m_BaseLuminance = 100.0f;
		m_MaxLuminance = 100.0f;

		// show dialogue that notifies success
		std::string message;
		message += "Changed settings for SDR display\n\n";
		message += "ColorSpace : ITU-R BT.709\n";
		message += "Max luminance : ";
		message += std::to_string(GetMaxLuminance());
		message += " [nit]\n";
		message += "Min luminance : ";
		message += std::to_string(GetMinLuminance());
		message += " [nit]\n";
		MessageBoxA(
			nullptr,
			message.c_str(),
			"succeeded in setting SDR",
			MB_OK | MB_ICONINFORMATION);
	}
}

void SampleApp::OnMsgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	// processing of keyboard
	if ((msg == WM_KEYDOWN)
		|| (msg == WM_SYSKEYDOWN)
		|| (msg == WM_KEYUP)
		|| (msg == WM_SYSKEYUP))
	{
		DWORD mask = (1 << 29);

		auto isKeyDown = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
		auto isAltDown = ((lp & mask) != 0);
		auto keyCode = uint32_t(wp);

		if (isKeyDown)
		{
			switch (keyCode)
			{
			case VK_ESCAPE:
			{
				PostQuitMessage(0);
			}
			break;

			// HDR mode
			case 'H':
			{
				ChangeDisplayMode(true);
			}
			break;

			// SDR mode
			case 'S':
			{
				ChangeDisplayMode(false);
			}
			break;

			// No tonemap
			case 'N':
			{
				m_TonemapType = TONEMAP_NONE;
			}
			break;

			// Reinhard tonemap
			case 'R':
			{
				m_TonemapType = TONEMAP_REINHARD;
			}
			break;

			// GT tonemap
			case 'G':
			{
				m_TonemapType = TONEMAP_GT;
			}
			break;

			}
		}
	}
}