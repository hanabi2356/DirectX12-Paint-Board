#pragma once

#include "Common.h"
#include "Window.h"
#include "PaintUI.h"
#include "Paint.h"
#include "Canvas.h"

#include <vector>

class PaintBoard
{
public:
	PaintBoard(HINSTANCE hInstance);
	~PaintBoard();

	PaintBoard(const PaintBoard&) = delete;
	PaintBoard& operator=(const PaintBoard&) = delete;

	bool Initialize(int nCmdShow);
	int Run();

	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static PaintBoard* GetApp() { return s_app; }

private:
	void InitDirect3D();
	void InitImGui();
	void ShutdownImGui();

	void CreateDevice();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateRtvHeap();
	void CreateSrvHeap();
	void CreateRenderTargetViews();
	void OnResize();
	void FlushCommandQueue();

	void AllocSrv(D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu);
	void FreeSrv(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu);

	void Update();
	void Draw();

private:
	static PaintBoard* s_app;

	Window m_window;
	PaintUI m_ui;
	Paint m_paint;
	Canvas m_canvas;


	bool m_imguiInitialized = false;

	bool m_appPaused = false;
	bool m_minimized = false;
	bool m_maximized = false;
	bool m_resizing = false;

	ComPtr<IDXGIFactory4> m_factory;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_currentFence = 0;

	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;

	ComPtr<IDXGISwapChain> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	ComPtr<ID3D12Resource> m_swapChainBuffers[FrameCount];

	UINT m_rtvDescriptorSize = 0;
	UINT m_srvDescriptorSize = 0;
	UINT m_currBackBuffer = 0;
	std::vector<int> m_srvFreeIndices;

	D3D12_VIEWPORT m_viewport{};
	D3D12_RECT m_scissorRect{};

	DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

#ifdef _DEBUG
	ComPtr<ID3D12Debug> m_debugController;
#endif
};
