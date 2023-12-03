#pragma once

#define NOMINMAX

#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <ComPtr.h>
#include <DescriptorPool.h>
#include <ColorTarget.h>
#include <DepthTarget.h>
#include <CommandList.h>
#include <Fence.h>
#include <Mesh.h>
#include <Texture.h>
#include <InlineUtil.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

//
// App class
//
class App
{

public:

	//! @brief constructor
	App(uint32_t width, uint32_t height, DXGI_FORMAT format);

	//! @brief destructor
	virtual ~App();

	//! @brief run application
	void Run();

protected:
	//
	// POOL_TYPE enum
	//
	enum POOL_TYPE
	{
		POOL_TYPE_RES = 0, // CBV_SRV_UAV
		POOL_TYPE_SMP = 1, // Sampler
		POOL_TYPE_RTV = 2, // RTV
		POOL_TYPE_DSV = 3, // DSV
		POOL_COUNT = 4,
	};

	static const uint32_t FrameCount = 2; // frame buffer count

	HINSTANCE m_hInst; // instance handle
	HWND m_hWnd; // window handle
	uint32_t m_Width; // width of window
	uint32_t m_Height; // height of window

	ComPtr<IDXGIFactory4> m_pFactory; // DXGI factory
	ComPtr<ID3D12Device> m_pDevice; // device
	ComPtr<ID3D12CommandQueue> m_pQueue; // command queue
	ComPtr<IDXGISwapChain4> m_pSwapChain; // swap chain
	ColorTarget m_ColorTarget[FrameCount]; // color target
	DepthTarget m_DepthTarget; // depth target
	DescriptorPool* m_pPool[POOL_COUNT]; // descriptor pool
	CommandList m_CommandList; // command list
	Fence m_Fence; // fence
	uint32_t m_FrameIndex; // frame index
	D3D12_VIEWPORT m_Viewport; // view port
	D3D12_RECT m_Scissor; // scissor quad
	DXGI_FORMAT m_BackBufferFormat; // back buffer format

	void Present(uint32_t interval);
	bool IsSupportHDR() const;
	float GetMaxLuminance() const;
	float GetMinLuminance() const;

	virtual bool OnInit()
	{
		return true;
	}

	virtual void OnTerm()
	{
	}

	virtual void OnRender()
	{
	}

	virtual void OnMsgProc(HWND, UINT, WPARAM, LPARAM)
	{
	}

private:

	bool m_SupportHDR; // whether HDR display is supported
	float m_MaxLuminance; // maximum luminance of display
	float m_MinLuminance; // minimum luminance of display

	bool InitApp();
	void TermApp();
	bool InitWnd();
	void TermWnd();
	bool InitD3D();
	void TermD3D();
	void MainLoop();
	void CheckSupportHDR();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
};