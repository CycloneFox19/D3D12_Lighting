#include "App.h"
#include <algorithm>

namespace
{
	const auto ClassName = TEXT("SampleWindowClass"); //!< window class name
	// calculate intersection of area
	inline int ComputeIntersectionArea
	(
		int ax1, int ay1,
		int ax2, int ay2,
		int bx1, int by1,
		int bx2, int by2
	)
	{
		return std::max(0, std::min(ax2, bx2) - std::max(ax1, bx1))
			* std::max(0, std::min(ay2, by2) - std::max(ay1, by1));
	}
}

//
// App class
//

// constructor
App::App(uint32_t width, uint32_t height, DXGI_FORMAT format)
	: m_hInst(nullptr)
	, m_hWnd(nullptr)
	, m_Width(width)
	, m_Height(height)
	, m_FrameIndex(0)
	, m_BackBufferFormat(format)
{
}

// destructor
App::~App()
{
}

// run
void App::Run()
{
	if (InitApp())
	{
		MainLoop();
	}

	TermApp();
}

// initialize
bool App::InitApp()
{
	// initialize window
	if (!InitWnd())
	{
		return false;
	}

	// initialize Direct3D12
	if (!InitD3D())
	{
		return false;
	}

	// initialize application-specific
	if (!OnInit())
	{
		return false;
	}

	// show window
	ShowWindow(m_hWnd, SW_SHOWNORMAL);

	// update window
	UpdateWindow(m_hWnd);

	// set focus on window
	SetFocus(m_hWnd);

	// normal end
	return true;
}

// end
void App::TermApp()
{
	// end application-specific
	OnTerm();

	// end Direct3D 12
	TermD3D();

	// end window
	TermWnd();
}

// initialize window
bool App::InitWnd()
{
	// get instance handle
	auto hInst = GetModuleHandle(nullptr);
	if (hInst == nullptr)
	{
		return false;
	}

	// window settings
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	wc.hCursor = LoadCursor(hInst, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = ClassName;
	wc.hIconSm = LoadIcon(hInst, IDI_APPLICATION);

	// apply window
	if (!RegisterClassEx(&wc))
	{
		return false;
	}

	// instance handle setting
	m_hInst = hInst;

	// set window size
	RECT rc = {};
	rc.right = static_cast<LONG>(m_Width);
	rc.bottom = static_cast<LONG>(m_Height);

	// adjust window size
	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rc, style, FALSE);

	// generate window
	m_hWnd = CreateWindowEx(
		0,
		ClassName,
		TEXT("Sample"),
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		m_hInst,
		this);

	if (m_hWnd == nullptr)
	{
		return false;
	}

	// normal end
	return true;
}

// end window
void App::TermWnd()
{
	// unregister window
	if (m_hInst != nullptr)
	{
		UnregisterClass(ClassName, m_hInst);
	}

	m_hInst = nullptr;
	m_hWnd = nullptr;
}

// initialize Direct3D
bool App::InitD3D()
{
#if defined(DEBUG) || defined(_DEBUG) // checking if DEBUG is defined
	{
		ComPtr<ID3D12Debug> pDebug;
		auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(pDebug.GetAddressOf()));
		if (SUCCEEDED(hr))
		{
			pDebug->EnableDebugLayer();
		}
	}
#endif

	// generate device
	auto hr = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_pDevice));
	if (FAILED(hr))
	{
		return false;
	}

	// generate command queue
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		hr = m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_pQueue));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// generate swap chain
	{
		// generate DXGI factory
		hr = CreateDXGIFactory2(0, IID_PPV_ARGS(m_pFactory.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}

		// swap chain settings
		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferDesc.Width = m_Width;
		desc.BufferDesc.Height = m_Height;
		desc.BufferDesc.RefreshRate.Numerator = 60;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		desc.BufferDesc.Format = m_BackBufferFormat;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = FrameCount;
		desc.OutputWindow = m_hWnd;
		desc.Windowed = TRUE;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		// generate swap chain
		ComPtr<IDXGISwapChain> pSwapChain;
		hr = m_pFactory->CreateSwapChain(m_pQueue.Get(), &desc, pSwapChain.GetAddressOf());
		if (FAILED(hr))
		{
			return false;
		}

		// get IDXGISwapChain4
		hr = pSwapChain.As(&m_pSwapChain);
		if (FAILED(hr))
		{
			return false;
		}

		// get back buffer index
		m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		// release as it is unnecessary
		pSwapChain.Reset();
	}

	// generate descriptor pool
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};

		desc.NodeMask = 1;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 512;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_RES]))
		{
			return false;
		}

		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		desc.NumDescriptors = 256;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_SMP]))
		{
			return false;
		}

		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = 512;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_RTV]))
		{
			return false;
		}

		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		desc.NumDescriptors = 512;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_DSV]))
		{
			return false;
		}
	}

	// generate command list
	{
		if (!m_CommandList.Init(
			m_pDevice.Get(),
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			FrameCount))
		{
			return false;
		}
	}

	// generate render target view
	{
		for (auto i = 0u; i < FrameCount; ++i)
		{
			if (!m_ColorTarget[i].InitFromBackBuffer(
				m_pDevice.Get(),
				m_pPool[POOL_TYPE_RTV],
				true,
				i,
				m_pSwapChain.Get()))
			{
				return false;
			}
		}
	}

	// generate depth stencil buffer
	{
		if (!m_DepthTarget.Init(
			m_pDevice.Get(),
			m_pPool[POOL_TYPE_DSV],
			nullptr,
			m_Width,
			m_Height,
			DXGI_FORMAT_D32_FLOAT,
			1.0f,
			0)) // DXGI_FORMAT_D32_FLOAT is for depth
		{
			return false;
		}
	}

	// generate fence
	if (!m_Fence.Init(m_pDevice.Get()))
	{
		return false;
	}

	// viewport settings
	{
		m_Viewport.TopLeftX = 0.0f;
		m_Viewport.TopLeftY = 0.0f;
		m_Viewport.Width = float(m_Width);
		m_Viewport.Height = float(m_Height);
		m_Viewport.MinDepth = 0.0f;
		m_Viewport.MaxDepth = 1.0f;
	}

	// scissor rectangle settings
	{
		m_Scissor.left = 0;
		m_Scissor.right = m_Width;
		m_Scissor.top = 0;
		m_Scissor.bottom = m_Height; 
	}

	// normal end
	return true;
}

// end direct3D
void App::TermD3D()
{
	// wait for completing of GPU processing
	m_Fence.Sync(m_pQueue.Get());

	// abandon fence
	m_Fence.Term();

	// abandon render target view
	for (auto i = 0u; i < FrameCount; ++i)
	{
		m_ColorTarget[i].Term();
	}

	// abandon depth stencil view
	m_DepthTarget.Term();

	// abandon command list
	m_CommandList.Term();

	for (auto i = 0; i < POOL_COUNT; ++i)
	{
		if (m_pPool[i] != nullptr)
		{
			m_pPool[i]->Release();
			m_pPool[i] = nullptr;
		}
	}

	// abandon swap chain
	m_pSwapChain.Reset();

	// abandon command queue
	m_pQueue.Reset();

	// abandon device
	m_pDevice.Reset();
}

// main loop
void App::MainLoop()
{
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == TRUE)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			OnRender();
		}
	}
}

// show in screen, and prepare the next frame
void App::Present(uint32_t interval)
{
	// show in screen
	m_pSwapChain->Present(interval, 0);

	//
	m_Fence.Wait(m_pQueue.Get(), INFINITE);

	// renew frame index
	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}

// return if HDR display is supported
bool App::IsSupportHDR() const
{
	return m_SupportHDR;
}

// get maximum luminance of display
float App::GetMaxLuminance() const
{
	return m_MaxLuminance;
}

// get minimum luminance of display
float App::GetMinLuminance() const
{
	return m_MinLuminance;
}

// check if display supports HDR output
void App::CheckSupportHDR()
{
	// if nothing is created, then don't process
	if (m_pSwapChain == nullptr || m_pFactory == nullptr || m_pDevice == nullptr)
	{
		return;
	}

	HRESULT hr = S_OK;

	// get window area
	RECT rect;
	GetWindowRect(m_hWnd, &rect);

	if (m_pFactory->IsCurrent() == false)
	{
		m_pFactory.Reset();
		hr = CreateDXGIFactory2(0, IID_PPV_ARGS(m_pFactory.GetAddressOf()));
		if (FAILED(hr))
		{
			return;
		}
	}

	ComPtr<IDXGIAdapter1> pAdapter;
	hr = m_pFactory->EnumAdapters1(0, pAdapter.GetAddressOf());
	if (FAILED(hr))
	{
		return;
	}

	UINT i = 0;
	ComPtr<IDXGIOutput> currentOutput;
	ComPtr<IDXGIOutput> bestOutput;
	int bestIntersectArea = -1;

	// look into each display
	while (pAdapter->EnumOutputs(i, &currentOutput) != DXGI_ERROR_NOT_FOUND)
	{
		auto ax1 = rect.left;
		auto ay1 = rect.top;
		auto ax2 = rect.right;
		auto ay2 = rect.bottom;

		// get setting of display
		DXGI_OUTPUT_DESC desc;
		hr = currentOutput->GetDesc(&desc);
		if (FAILED(hr))
		{
			return;
		}

		auto bx1 = desc.DesktopCoordinates.left;
		auto by1 = desc.DesktopCoordinates.top;
		auto bx2 = desc.DesktopCoordinates.right;
		auto by2 = desc.DesktopCoordinates.bottom;

		// check if area matches
		int intersectArea = ComputeIntersectionArea(
			ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);
		if (intersectArea > bestIntersectArea)
		{
			bestOutput = currentOutput;
			bestIntersectArea = intersectArea;
		}

		++i;
	}

	// most suitable display
	ComPtr<IDXGIOutput6> pOutput6;
	hr = bestOutput.As(&pOutput6);
	if (FAILED(hr))
	{
		return;
	}

	// get output setting
	DXGI_OUTPUT_DESC1 desc1;
	hr = pOutput6->GetDesc1(&desc1);
	if (FAILED(hr))
	{
		return;
	}

	// check if color space supports ITU-R BT.2000 PQ
	m_SupportHDR = (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
	m_MaxLuminance = desc1.MaxLuminance;
	m_MinLuminance = desc1.MinLuminance;
}

// window procedure
LRESULT CALLBACK App::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	auto instance = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (msg)
	{
	case WM_CREATE:
	{
		auto pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lp);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
	}
	break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	break;

	case WM_MOVE:
	{
		instance->CheckSupportHDR();
	}
	break;

	case WM_DISPLAYCHANGE:
	{
		instance->CheckSupportHDR();
	}
	break;

	default:
	{
	}
	break;
	}

	if (instance != nullptr)
	{
		instance->OnMsgProc(hWnd, msg, wp, lp);
	}

	return DefWindowProc(hWnd, msg, wp, lp);
}