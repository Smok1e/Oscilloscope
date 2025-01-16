#include "ImmersiveDarkMode.hpp"

//=================================================

#ifdef OSCILLOSCOPE_WINDOWS

#pragma warning(disable: 28159)

#include <iostream>
#include <dwmapi.h>

bool TryEnableImmersiveDarkMode(const sf::RenderWindow& window)
{
	HMODULE dwmapi = LoadLibraryA("dwmapi.dll");
	if (!dwmapi)
		return false;

	auto __DwmSetWindowAttribute = reinterpret_cast<HRESULT (__stdcall*)(HWND, DWORD, LPCVOID, DWORD)>(
		GetProcAddress(dwmapi, "DwmSetWindowAttribute")
	);

	BOOL value = true;
	if (__DwmSetWindowAttribute)
	{
		__DwmSetWindowAttribute
		(
	 		window.getSystemHandle(),
	 		DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE, 
	 		&value, 
	 		sizeof(value)
		);
	}

	FreeLibrary(dwmapi);
	return __DwmSetWindowAttribute;
}

#else

bool TryEnableImmersiveDarkMode(const sf::RenderWindow& window)
{
	return false;
}

#endif

//=================================================