#include <Windows.h>
#include "IniReader.h"
#include "injector/injector.hpp"

int* GameState = (int*)0xA99BBC;
auto PVehicle_GetAIVehiclePtr = (void* (__thiscall*)(void*))0x006D8110;
auto sub_7F12A0 = (int(__thiscall*)(int, int))0x007F12A0;
auto RenderInGameCars = (int(__cdecl*)(int, int))0x007BF7C0;
auto RenderWorldFlares = (int(__cdecl*)(int))0x007507D0;
auto RenderVehicleFlares = (int(__cdecl*)(int, int, int))0x007D5DC0;
float* DeltaTime = (float*)0x00A99A5C;
float* RoadCarReflectionsDist = (float*)0x009EEC70;

class BlurSettings
{
public:
	float Start;
	float End;
	float Intensity;

	BlurSettings() {}

	BlurSettings(float start, float end, float intensity)
	{
		this->Start = start;
		this->End = end;
		this->Intensity = intensity;
	}

	BlurSettings operator-(BlurSettings& a)
	{
		return BlurSettings(this->Start - a.Start, this->End - a.End, this->Intensity - a.Intensity);
	}

	BlurSettings operator+(BlurSettings& a)
	{
		return BlurSettings(this->Start + a.Start, this->End + a.End, this->Intensity + a.Intensity);
	}

	BlurSettings operator/(float a)
	{
		return BlurSettings(this->Start / a, this->End / a, this->Intensity / a);
	}

	BlurSettings operator*(float a)
	{
		return BlurSettings(this->Start * a, this->End * a, this->Intensity * a);
	}

	void IncreaseBy(BlurSettings& step, BlurSettings& max)
	{
		this->Increase(this->Start, step.Start, max.Start);
		this->Increase(this->End, step.End, max.End);
		this->Increase(this->Intensity, step.Intensity, max.Intensity);
	}

	void DecreaseBy(BlurSettings& step, BlurSettings& min)
	{
		this->Decrease(this->Start, step.Start, min.Start);
		this->Decrease(this->End, step.End, min.End);
		this->Decrease(this->Intensity, step.Intensity, min.Intensity);
	}

private:

	void Increase(float& val, float step, float max)
	{
		if (val < max)
		{
			val += step;
		}
		else
		{
			val = max;
		}
	}

	void Decrease(float& val, float step, float min)
	{
		if (val > min)
		{
			val -= step;
		}
		else
		{
			val = min;
		}
	}
};


BlurSettings _Blur, Blur, NosBlur;
float NosBlurTime;

void __stdcall MainLoopTick()
{
	__asm pushad;

	if (*GameState == 6)
	{
		auto PlayerPVehicle = *(void**)0x00A9F168;
		if (PlayerPVehicle)
		{
			int* AIVehicle = (int*)PVehicle_GetAIVehiclePtr(PlayerPVehicle);
			int isUsingNos = *(AIVehicle + 0x37);
			float* DeltaTime = (float*)0x00A99A5C;

			BlurSettings deltaPath = NosBlur - Blur;
			BlurSettings velocity = deltaPath / NosBlurTime;
			BlurSettings step = velocity * *DeltaTime;

			isUsingNos ? _Blur.IncreaseBy(step, NosBlur) : _Blur.DecreaseBy(step, Blur);
		}
	}
	else
	{
		_Blur = Blur;
	}

	__asm popad;
}

int __fastcall RoadCarReflectionsHook(int _this, int param, int a2)
{
	int result = sub_7F12A0(_this, a2);
	RenderInGameCars(0x00B4B110, 1);
	return result;
}

int __cdecl RenderWorldFlaresHook(int view)
{
	int result = RenderWorldFlares(view);
	RenderVehicleFlares(view, 1, 0);
	return result;
}

void Init()
{
	CIniReader iniReader("NFSCMBlurControl.ini");

	Blur = BlurSettings(
		iniReader.ReadFloat("BLUR", "Start", 350.0f),
		iniReader.ReadFloat("BLUR", "End", 350.0f),
		iniReader.ReadFloat("BLUR", "Intensity", 0.3f)
	);
	 
	NosBlur = BlurSettings(
		iniReader.ReadFloat("NOS_BLUR", "Start", 350.0f),
		iniReader.ReadFloat("NOS_BLUR", "End", 350.0f),
		iniReader.ReadFloat("NOS_BLUR", "Intensity", 0.3f)
	);

	NosBlurTime = iniReader.ReadFloat("NOS_BLUR", "Time", 300.0f);

	_Blur = Blur;
	injector::WriteMemory<float*>(0x00726467, &_Blur.End, true);
	injector::WriteMemory<float*>(0x00726491, &_Blur.Start, true);
	injector::WriteMemory<float*>(0x007265CB, &_Blur.Intensity, true);

	injector::MakeCALL(0x006B7BC2, MainLoopTick, true);

	if (iniReader.ReadInteger("CAMERA", "DisableGearChangeShake", 0) == 1)
	{
		unsigned char bytes[] = { 0xE9, 0xA6, 0x02, 0x00, 0x00, 0x90 };
		injector::WriteMemoryRaw(0x00492A15, bytes, 6, true);
	}

	if (iniReader.ReadInteger("CAMERA", "DisableCameraDodge", 0) == 1)
	{
		injector::WriteMemory<unsigned char>(0x00492F34, 0xEB, true);
	}

	if (iniReader.ReadInteger("FIXES", "RoadCarReflections", 0) == 1)
	{
		injector::MakeCALL(0x0072E141, RoadCarReflectionsHook, true);
		*RoadCarReflectionsDist = 0.05f;
	}

	if (iniReader.ReadInteger("FIXES", "RoadCarFlareColor", 0) == 1)
	{
		injector::MakeCALL(0x0072E1E0, RenderWorldFlaresHook, true);
		injector::MakeNOP(0x0072E2C8, 5, true);
	}

	injector::MakeNOP(0x0073C359, 2, true);
	injector::MakeNOP(0x0073C371, 2, true);
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