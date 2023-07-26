#include <Windows.h>
#include "includes/IniReader/IniReader.h"
#include "includes/Injector/injector.hpp"

auto Game_CubicCameraMover_Update = (int(__thiscall*)(void*, float))0x004921B0;
auto Game_DeltaTime = (float*)0x00A99A5C;
auto Game_State = (int*)0x00A99BBC;
auto Game_BlurUpdate = (BYTE*)0x0048249D;
auto Game_IsPaused = (bool(__cdecl*)())0x004A62E0;
auto Game_PhotoModeIn = (void(__thiscall*)(void*, int))0x005BBCC0;
auto Game_PhotoModeOut = (void(__thiscall*)(void*, char))0x005C52F0;

struct BlurSettings
{
	float Start;
	float End;
	float Intensity;
};

BlurSettings _Blur, Blur, NosBlur;
float NosBlurTime;
float Time = 0;

void __fastcall CubicCameraMover_Update(float** _this, int dummy, float a1)
{
	Game_CubicCameraMover_Update(_this, a1);

	if (!Game_IsPaused())
	{
		bool isUsingNos = _this[0x24][0x38] && _this[0x24][0x24] > 0;

		Time += (isUsingNos ? 1 : -1) * (*Game_DeltaTime) / NosBlurTime;
		Time = std::clamp(Time, 0.0f, 1.0f);

		_Blur.Start = std::lerp(Blur.Start, NosBlur.Start, Time);
		_Blur.End = std::lerp(Blur.End, NosBlur.End, Time);
		_Blur.Intensity = std::lerp(Blur.Intensity, NosBlur.Intensity, Time);
	}
}

void __fastcall PhotoModeIn(void* _this, int, int a)
{
	if (*Game_State == 6)
	{
		*Game_BlurUpdate = 0x84;
	}

	Game_PhotoModeIn(_this, a);
}

void __fastcall PhotoModeOut(void* _this, int, char a)
{
	if (*Game_State == 6)
	{
		*Game_BlurUpdate = 0x85;
	}

	Game_PhotoModeOut(_this, a);
}

void Init()
{
	CIniReader iniReader("NFSCMBlurControl.ini");

	Blur.Start = iniReader.ReadFloat("BLUR", "Start", 350.0f);
	Blur.End = iniReader.ReadFloat("BLUR", "End", 350.0f);
	Blur.Intensity = iniReader.ReadFloat("BLUR", "Intensity", 0.3f);

	NosBlur.Start = iniReader.ReadFloat("NOS_BLUR", "Start", 350.0f);
	NosBlur.End = iniReader.ReadFloat("NOS_BLUR", "End", 350.0f);
	NosBlur.Intensity = iniReader.ReadFloat("NOS_BLUR", "Intensity", 0.3f);

	NosBlurTime = iniReader.ReadFloat("NOS_BLUR", "Time", 1.0f);

	_Blur = Blur;
	injector::WriteMemory<float*>(0x00726467, &_Blur.End, true);
	injector::WriteMemory<float*>(0x00726491, &_Blur.Start, true);
	injector::WriteMemory<float*>(0x007265CB, &_Blur.Intensity, true);

	injector::WriteMemory(0x009C7908, CubicCameraMover_Update, true);

	if (iniReader.ReadInteger("GENERAL", "PhotoModeBlur", 0) == 1)
	{
		injector::MakeCALL(0x005BC646, PhotoModeIn, true);
		injector::WriteMemory(0x009D4FBC, PhotoModeOut, true);
		injector::WriteMemory<BYTE>(0x0048249D, 0x85, true); // Just to unprotect memmory
	}

	// Tanks to _Aero
	if (iniReader.ReadInteger("GENERAL", "DisableGearChangeShake", 0) == 1)
	{
		unsigned char bytes[] = { 0xE9, 0xA6, 0x02, 0x00, 0x00, 0x90 };
		injector::WriteMemoryRaw(0x00492A15, bytes, 6, true);
	}

	if (iniReader.ReadInteger("GENERAL", "DisableCameraDodge", 0) == 1)
	{
		injector::WriteMemory<unsigned char>(Game_BlurUpdate, 0xEB, true);
	}
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

		if ((base + nt->OptionalHeader.AddressOfEntryPoint + (0x400000 - base)) == 0x0087E926) // Check if .exe file is compatible - Thanks to thelink2012 and MWisBest
		{
			Init();
		}
		else
		{
			MessageBoxA(NULL, "This .exe is not supported.\nPlease use v1.4 English nfsc.exe (6,88 MB (7.217.152 bytes)).", "NFSC - Motion Blur Controll 1.3", MB_ICONERROR);
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