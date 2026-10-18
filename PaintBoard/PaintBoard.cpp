#include "PaintBoard.h"
#include "../SDK/imgui-master/backends/imgui_impl_win32.h"
#include "../SDK/imgui-master/backends/imgui_impl_dx12.h"
#include <cassert>
#include <windowsx.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

PaintBoard* PaintBoard::s_app = nullptr;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return PaintBoard::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

PaintBoard::PaintBoard(HINSTANCE hInstance)
	: m_window(hInstance)
{
	s_app = this;
}

PaintBoard::~PaintBoard()
{
	if (m_device)
		FlushCommandQueue();

	ShutdownImGui();
}

bool PaintBoard::Initialize(int nCmdShow)
{
	if (!m_window.Initialize(nCmdShow, MainWndProc))
		return false;
	
	try
	{
		InitDirect3D();
		InitImGui();
		m_canvas.Resize(m_window.GetClientWidth(), m_window.GetClientHeight());
		m_paint.SetCanvas(&m_canvas);
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "DX12 Init Error", MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}

void PaintBoard::InitDirect3D()
{
	CreateDevice();
	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvHeap();
	CreateSrvHeap();
	OnResize();
}

void PaintBoard::CreateSrvHeap()
{
	constexpr int kSrvCount = 64;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = kSrvCount;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvHeap)));

	m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_srvFreeIndices.clear();
	m_srvFreeIndices.reserve(kSrvCount);
	for (int i = kSrvCount - 1; i >= 0; --i)
		m_srvFreeIndices.push_back(i);
}

void PaintBoard::AllocSrv(D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu)
{
	assert(!m_srvFreeIndices.empty());
	const int idx = m_srvFreeIndices.back();
	m_srvFreeIndices.pop_back();

	outCpu->ptr = m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + static_cast<SIZE_T>(idx) * m_srvDescriptorSize;
	outGpu->ptr = m_srvHeap->GetGPUDescriptorHandleForHeapStart().ptr + static_cast<SIZE_T>(idx) * m_srvDescriptorSize;
}

void PaintBoard::FreeSrv(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE /*gpu*/)
{
	const int idx = static_cast<int>((cpu.ptr - m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr) / m_srvDescriptorSize);
	m_srvFreeIndices.push_back(idx);
}

void PaintBoard::InitImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(m_window.GetHwnd());

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = m_device.Get();
	initInfo.CommandQueue = m_commandQueue.Get();
	initInfo.NumFramesInFlight = FrameCount;
	initInfo.RTVFormat = m_backBufferFormat;
	initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
	initInfo.SrvDescriptorHeap = m_srvHeap.Get();
	initInfo.UserData = this;
	initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu)
	{
		static_cast<PaintBoard*>(info->UserData)->AllocSrv(outCpu, outGpu);
	};
	initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
	{
		static_cast<PaintBoard*>(info->UserData)->FreeSrv(cpu, gpu);
	};

	ImGui_ImplDX12_Init(&initInfo);
	m_imguiInitialized = true;
}

void PaintBoard::ShutdownImGui()
{
	if (!m_imguiInitialized)
		return;

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	m_imguiInitialized = false;
}

int PaintBoard::Run()
{
	MSG msg = {};

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (!m_appPaused)
			{
				Update();
				Draw();
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return static_cast<int>(msg.wParam);
}

void PaintBoard::CreateDevice()
{
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			m_debugController = debugController;
		}
	}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory)));

	HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
	if (FAILED(hr))
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
	}

	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void PaintBoard::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	ThrowIfFailed(m_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(m_commandAllocator.GetAddressOf())));

	ThrowIfFailed(m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(m_commandList.GetAddressOf())));

	m_commandList->Close();
}

void PaintBoard::CreateSwapChain()
{
	m_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = m_window.GetClientWidth();
	sd.BufferDesc.Height = m_window.GetClientHeight();
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = m_backBufferFormat;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = FrameCount;
	sd.OutputWindow = m_window.GetHwnd();
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(m_factory->CreateSwapChain(
		m_commandQueue.Get(),
		&sd,
		m_swapChain.GetAddressOf()));
}

void PaintBoard::CreateRtvHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf())));
}

void PaintBoard::CreateRenderTargetViews()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < FrameCount; ++i)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffers[i])));
		m_device->CreateRenderTargetView(m_swapChainBuffers[i].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}
}

void PaintBoard::OnResize()
{
	assert(m_device);
	assert(m_swapChain);
	assert(m_commandAllocator);

	FlushCommandQueue();

	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

	for (UINT i = 0; i < FrameCount; ++i)
		m_swapChainBuffers[i].Reset();

	ThrowIfFailed(m_swapChain->ResizeBuffers(
		FrameCount,
		m_window.GetClientWidth(), m_window.GetClientHeight(),
		m_backBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	m_currBackBuffer = 0;
	CreateRenderTargetViews();

	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* cmds[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(cmds), cmds);
	FlushCommandQueue();

	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = static_cast<float>(m_window.GetClientWidth());
	m_viewport.Height = static_cast<float>(m_window.GetClientHeight());
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect = {
		0, 0,
		static_cast<LONG>(m_window.GetClientWidth()),
		static_cast<LONG>(m_window.GetClientHeight())
	};

	m_canvas.Resize(m_window.GetClientWidth(), m_window.GetClientHeight());
}

void PaintBoard::FlushCommandQueue()
{
	m_currentFence++;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_currentFence));

	if (m_fence->GetCompletedValue() < m_currentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void PaintBoard::Update()
{
	if (!m_imguiInitialized)
		return;

	// Required before any ImGui::Begin / widgets
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	m_ui.Build(m_paint, m_canvas);
}

void PaintBoard::Draw()
{
	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	auto barrierToRT = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChainBuffers[m_currBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &barrierToRT);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_currBackBuffer,
		m_rtvDescriptorSize);

	const float clearColor[] = { 0.25f, 0.25f, 0.75f, 1.0f };
	m_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);

	if (m_imguiInitialized)
	{
		ImGui::Render();
		ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
		m_commandList->SetDescriptorHeaps(1, heaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());
	}

	auto barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChainBuffers[m_currBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &barrierToPresent);

	ThrowIfFailed(m_commandList->Close());

	ID3D12CommandList* cmds[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(cmds), cmds);

	ThrowIfFailed(m_swapChain->Present(0, 0));
	m_currBackBuffer = (m_currBackBuffer + 1) % FrameCount;

	FlushCommandQueue();
}

LRESULT PaintBoard::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_imguiInitialized && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_ACTIVATE:
		m_appPaused = (LOWORD(wParam) == WA_INACTIVE);
		return 0;

	case WM_SIZE:
		m_window.SetClientSize(LOWORD(lParam), HIWORD(lParam));

		if (m_device)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				m_appPaused = true;
				m_minimized = true;
				m_maximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				m_appPaused = false;
				m_minimized = false;
				m_maximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (m_minimized)
				{
					m_appPaused = false;
					m_minimized = false;
					OnResize();
				}
				else if (m_maximized)
				{
					m_appPaused = false;
					m_maximized = false;
					OnResize();
				}
				else if (!m_resizing)
				{
					OnResize();
				}
			}
		}
		return 0;

	case WM_ENTERSIZEMOVE:
		m_appPaused = true;
		m_resizing = true;
		return 0;

	case WM_EXITSIZEMOVE:
		m_appPaused = false;
		m_resizing = false;
		OnResize();
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
