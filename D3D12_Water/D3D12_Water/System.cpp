#include "System.h"
#include <tchar.h>


System::System()
{
	
}


System::~System()
{

}

bool System::Initialize(const HINSTANCE* inHinstance)
{
	int screenWidth, screenHeight;
	bool result;

	screenWidth = 0;
	screenHeight = 0;

	InitializeWindows(screenWidth, screenHeight, inHinstance);

	// InitializeInout

	// InitializeGraphics
	m_Graphics = new Graphics;
	if (!m_Graphics)
	{
		return false;
	}

	result = m_Graphics->Initialize(screenWidth, screenHeight, m_hwnd);
	if (!result)
	{
		return false;
	}

	return true;
}

void System::InitializeWindows(int& outScreenWidth, int& outScreenHeight, const HINSTANCE* inHinstance)
{
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;

	ApplicationHandle = this;

	m_hinstance = *inHinstance;

	m_applicationName = _T("DirectX12");

	// Initialize WindowsClass
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hinstance;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = m_applicationName;
	wc.hIcon = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIconSm = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);


	if (RegisterClassEx(&wc) == 0) {
		return ;
	}

	outScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	outScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (FULL_SCREEN)
	{
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = (unsigned long)outScreenWidth;
		dmScreenSettings.dmPelsHeight = (unsigned long)outScreenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Change the display settings to full screen.
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		posX = posY = 0;
	}
	else
	{
		outScreenWidth = 640;
		outScreenHeight = 480;

		// Place the window in the middle of the screen.
		posX = (GetSystemMetrics(SM_CXSCREEN) - outScreenWidth) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - outScreenHeight) / 2;
	}
	// Create the window with the screen settings and get the handle to it.
	m_hwnd = CreateWindowEx(WS_EX_APPWINDOW, m_applicationName, m_applicationName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, outScreenWidth, outScreenHeight, nullptr, nullptr, m_hinstance, nullptr);



	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	ShowCursor(false);

	return;
}

void System::Run()
{
	MSG msg{};
	bool done, result;

	done = false;
	// MainLoop
	while (!done)
	{
		// Handle the windows messages
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			// Otherwise do the frame processing.  If frame processing fails then exit.
			result = Frame();
			result = true;
			if (!result)
			{
				MessageBox(m_hwnd, L"Frame Processing Failed", L"Error", MB_OK);
				done = true;
			}
		}


	}

	return;
}

LRESULT CALLBACK System::MessageHandler(HWND inHwnd, UINT inUmsg, WPARAM inWparam, LPARAM inLparam)
{
	return DefWindowProc(inHwnd, inUmsg, inWparam, inLparam);
}

LRESULT CALLBACK WndProc(HWND inHwnd, UINT inUmessage, WPARAM inWparam, LPARAM inLparam)
{
	switch (inUmessage)
	{
		// Check if the window is being destroyed.
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		// Check if the window is being closed.
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
		// All other messages pass to the message handler in the system class.
		default:
		{
			return ApplicationHandle->MessageHandler(inHwnd, inUmessage, inWparam, inLparam);
		}
	}
}


bool System::Frame()
{
	bool result;


	// Do the input frame processing.
	//result = m_Input->Frame();
	//if (!result)
	//{
	//	return false;
	//}

	// Do the frame processing for the graphics object.
	result = m_Graphics->Frame();
	if (!result)
	{
		return false;
	}

	// Finally render the graphics to the screen.
	result = m_Graphics->Render();
	if (!result)
	{
		return false;
	}

	return true;
}
