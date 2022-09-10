#include <Windows.h>
#include "includes/IniReader/IniReader.h"
#include "includes/Injector/injector.hpp"

auto CubicCameraMover_Update = (int(__thiscall*)(void*, float))0x004921B0;

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
float* Game_DeltaTime = (float*)0x00A99A5C;

void __fastcall MainLoopTick(float** _this, int dummy, float a1)
{
	CubicCameraMover_Update(_this, a1);

	bool isUsingNos = _this[0x24][0x38] && _this[0x24][0x24] > 0;

	BlurSettings deltaPath = NosBlur - Blur;
	BlurSettings velocity = deltaPath / NosBlurTime;
	BlurSettings step = velocity * *Game_DeltaTime;

	isUsingNos ? _Blur.IncreaseBy(step, NosBlur) : _Blur.DecreaseBy(step, Blur);
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

	injector::WriteMemory(0x009C7908, MainLoopTick, true);

	if (iniReader.ReadInteger("CAMERA", "DisableGearChangeShake", 0) == 1)
	{
		unsigned char bytes[] = { 0xE9, 0xA6, 0x02, 0x00, 0x00, 0x90 };
		injector::WriteMemoryRaw(0x00492A15, bytes, 6, true);
	}

	if (iniReader.ReadInteger("CAMERA", "DisableCameraDodge", 0) == 1)
	{
		injector::WriteMemory<unsigned char>(0x00492F34, 0xEB, true);
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