#include <Windows.h>
#include "System.h"

int WINAPI WinMain(HINSTANCE inHinstance, HINSTANCE inPrevInstance, PSTR inPscmdline, int inIcmdshow)
{
	System* system;
	bool result;

	// Create the system object
	system = new System;

	if (!system)
	{
		return 0;
	}
	
	// Initialize SystemObject
	result =  system->Initialize(&inHinstance);
	if (result)
	{
		system->Run();
	}

	return 0;
}