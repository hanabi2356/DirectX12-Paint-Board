#include "Window.h"

Window::Window(HINSTANCE hInstance, UINT clientWidth, UINT clientHeight,
	const wchar_t* title, const wchar_t* className)
	: m_hInstance(hInstance)
	, m_clientWidth(clientWidth)
	, m_clientHeight(clientHeight)
	, m_title(title)
	, m_className(className)
{
}

bool Window::Initialize(int nCmdShow, WNDPROC wndProc)
{
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wndProc;
	wc.hInstance = m_hInstance;
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszClassName = m_className;

	if (!RegisterClassExW(&wc))
	{
		MessageBoxW(nullptr, L"RegisterClassExW failed.", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	RECT rect = { 0, 0, static_cast<LONG>(m_clientWidth), static_cast<LONG>(m_clientHeight) };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	const int width = rect.right - rect.left;
	const int height = rect.bottom - rect.top;

	m_hWnd = CreateWindowW(
		m_className,
		m_title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		nullptr, nullptr, m_hInstance, nullptr);

	if (!m_hWnd)
	{
		MessageBoxW(nullptr, L"CreateWindowW failed.", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	ShowWindow(m_hWnd, nCmdShow);
	UpdateWindow(m_hWnd);
	return true;
}

void Window::SetClientSize(UINT width, UINT height)
{
	m_clientWidth = width;
	m_clientHeight = height;
}
