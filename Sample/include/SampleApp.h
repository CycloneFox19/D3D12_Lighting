#pragma once

#include <App.h>
#include <Camera.h>
#include <ConstantBuffer.h>
#include <Material.h>
#include <RootSignature.h>
#include <chrono>

//
// SampleApp class
class SampleApp : public App
{

public:

	//! @brief constructor
	//! 
	//! @param[in] width width of window
	//! @param[in] height height of window
	SampleApp(uint32_t width, uint32_t height);

	//! @brief destructor
	virtual ~SampleApp();

private:

	ComPtr<ID3D12PipelineState> m_pScenePSO; //!< pipeline state for scene
	RootSignature m_SceneRootSig; //!< root signature for scene
	ComPtr<ID3D12PipelineState> m_pTonemapPSO; //!< pipeline state for tonemap
	RootSignature m_TonemapRootSig; //!< root signature for tonemap
	ColorTarget m_SceneColorTarget; //!< render target for scene
	DepthTarget m_SceneDepthTarget; //!< depth target for scene
	VertexBuffer m_QuadVB; //!< vertex buffer
	VertexBuffer m_WallVB; //!< vertex buffer for wall
	VertexBuffer m_FloorVB; //!< vertex buffer for floor
	ConstantBuffer m_TonemapCB[FrameCount]; //!< constant buffer
	ConstantBuffer m_LightCB[FrameCount]; //!< light buffer
	ConstantBuffer m_CameraCB[FrameCount]; //!< camera buffer
	ConstantBuffer m_TransformCB[FrameCount]; //!< buffer for transform
	ConstantBuffer m_MeshCB[FrameCount]; //!< buffer for mesh
	std::vector<Mesh*> m_pMesh; //!< mesh
	Material m_Material; //!< material
	float m_RotateAngle; //!< angle that light rotates by
	int m_TonemapType; //!< type of tonemap
	int m_ColorSpace; //!< output color space
	float m_BaseLuminance; //!< base luminance
	float m_MaxLuminance; //!< maximum luminance
	float m_Exposure; //!< exposure

	//! @brief initialize
	//! 
	//! @retval true successfully initialized
	//! @retval false failed to initialize
	bool OnInit() override;

	//! @brief end
	void OnTerm() override;

	//! @brief processing that is done on rendering
	void OnRender() override;

	//! @brief Message Procedure
	void OnMsgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) override;

	//! @brief change display mode
	//! 
	//! @param[in] hdr if true, change settings for HDR display
	void ChangeDisplayMode(bool hdr);

	//! @brief draw scene
	void DrawScene(ID3D12GraphicsCommandList* pCmdList);

	//! @brief apply tonemap
	void DrawTonemap(ID3D12GraphicsCommandList* pCmdList);

	//! @brief draw mesh
	void DrawMesh(ID3D12GraphicsCommandList* pCmdList);
};
