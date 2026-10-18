#pragma once

#include "Common.h"

class Window
{
public:
	Window(HINSTANCE hInstance, UINT clientWidth = 1280, UINT clientHeight = 720,
		const wchar_t* title = L"DirectX12 Paint Board",
		const wchar_t* className = L"PaintBoardWndClass");

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	bool Initialize(int nCmdShow, WNDPROC wndProc);

	HWND GetHwnd() const { return m_hWnd; }
	HINSTANCE GetInstance() const { return m_hInstance; }

	UINT GetClientWidth() const { return m_clientWidth; }
	UINT GetClientHeight() const { return m_clientHeight; }
	void SetClientSize(UINT width, UINT height);

private:
	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;

	UINT m_clientWidth = 1280;
	UINT m_clientHeight = 720;

	const wchar_t* m_title = nullptr;
	const wchar_t* m_className = nullptr;
};
