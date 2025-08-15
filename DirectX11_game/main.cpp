#include<windows.h>
#include<windowsx.h>

#include<chrono>
#include"Core.h"


#ifdef _DEBUG
#pragma comment (linker, "/entry:wWinMainCRTStartup /subsystem:console")
#endif

typedef std::chrono::time_point<std::chrono::high_resolution_clock> timepoint_t;


/******************************* Global Variables **********************************/

float DeltaTime;
float RenderTime;
timepoint_t LastTime;

UINT WindowWidth = 1600;
UINT WindowHeight = 800;

Core core;

/********************************** Functions *************************************/

void GetDeltaTime(void);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void* __cdecl operator new[](size_t size, const char* name, int flag, unsigned debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}

void* __cdecl operator new[](unsigned __int64 size, unsigned __int64, unsigned __int64, char const*, int, unsigned int, char const*, int)
{
	return new uint8_t[size];
}

// - Main - 

int APIENTRY wWinMain(
	_In_	 HINSTANCE	hInstance,
	_In_opt_ HINSTANCE	hPrevInstance,
	_In_	 LPWSTR		lpCmdLine,
	_In_	 int		nCmdShow
)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	HWND hWnd;
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = L"DirectX11 Practice";

	RegisterClassEx(&wc);

	RECT wr = { 0, 0, static_cast<LONG>(WindowWidth), static_cast<LONG>(WindowHeight)};

	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	hWnd = CreateWindowEx(
		NULL,
		L"DirectX11 Practice",
		L"MY SUPPER DIRECTX",
		WS_OVERLAPPEDWINDOW,
		300,
		300,
		wr.right - wr.left,
		wr.bottom - wr.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);


	LastTime = std::chrono::high_resolution_clock::now();

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg = { 0 };

	core.InitDevice(hWnd, WindowWidth, WindowHeight);

	while (WM_QUIT != msg.message)
	{
		GetDeltaTime();
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			core.SetDeltaTime(DeltaTime);
			core.RenderFrame();
		}
	}

	core.ReleaseDevice();

	return (int)(msg.wParam);
}

	





void GetDeltaTime(void)
{
	timepoint_t currentTime = std::chrono::high_resolution_clock::now();
	DeltaTime = std::chrono::duration<float>(currentTime - LastTime).count();

	if (DeltaTime > 0.1f)
	{
		DeltaTime = 0.1f;
	}

	LastTime = currentTime;
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
 		if (wParam != SIZE_MINIMIZED)
		{
			UINT newWidth = LOWORD(lParam);
			UINT newHeight = HIWORD(lParam);
			if (newWidth > 0 && newHeight > 0)
			{
				core.OnResize(newWidth, newHeight);
			}
		}
		break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}