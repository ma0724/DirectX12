#pragma once
#include <Windows.h>
#include "Graphics.h"

class System
{
private:
	LPCWSTR		m_applicationName;
	HINSTANCE	m_hinstance;
	HWND		m_hwnd;

	Graphics* m_Graphics;
private:
	bool Frame();
	void InitializeWindows(int&, int&, const HINSTANCE* inHinstance);
public:
	System();
	System(const System&);
	~System();

	bool Initialize(const HINSTANCE* inHinstance);
	void Run();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);
};


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static System* ApplicationHandle = 0;