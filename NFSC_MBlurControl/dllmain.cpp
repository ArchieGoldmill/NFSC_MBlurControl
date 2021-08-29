#include "pch.h"
#include "IniReader.h"
#include "injector/injector.hpp"

float BlurStart, BlurEnd, BlurIntensity;

void Init()
{
	CIniReader iniReader("NFSCMBlurControl.ini");

	BlurStart = iniReader.ReadFloat((char*)"GENERAL", (char*)"BlurStart", 350.0f);
	BlurEnd = iniReader.ReadFloat((char*)"GENERAL", (char*)"BlurEnd", 350.0f);
	BlurIntensity = iniReader.ReadFloat((char*)"GENERAL", (char*)"BlurIntensity", 0.3f);

	injector::WriteMemory<float*>(0x00726467, &BlurStart, true);
	injector::WriteMemory<float*>(0x00726491, &BlurEnd, true);
	injector::WriteMemory<float>(0x00A63FE4, BlurIntensity, true);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
		IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)(base);
		IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);

		if ((base + nt->OptionalHeader.AddressOfEntryPoint + (0x400000 - base)) == 0x87E926) // Check if .exe file is compatible - Thanks to thelink2012 and MWisBest
		{
			Init();
		}
		else
		{
			MessageBoxA(NULL, "This .exe is not supported.\nPlease use v1.4 English nfsc.exe (6,88 MB (7.217.152 bytes)).", "NFSC - Motion Blur Controller 1.0", MB_ICONERROR);
			return FALSE;
		}
	}
	break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}