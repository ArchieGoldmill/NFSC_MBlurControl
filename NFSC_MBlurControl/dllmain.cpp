#include <Windows.h>
#include "IniReader.h"
#include "injector/injector.hpp"

int* GameState = (int*)0xA99BBC;
auto PVehicle_GetAIVehiclePtr = (void* (__thiscall*)(void*))0x006D8110;
auto sub_7F12A0 = (int(__thiscall*)(int, int))0x007F12A0;
auto RenderInGameCars = (int(__cdecl*)(int, int))0x007BF7C0;
auto RenderWorldFlares = (int(__cdecl*)(int))0x007507D0;
auto RenderVehicleFlares = (int(__cdecl*)(int, int, int))0x007D5DC0;
auto eCreateLookAtMatrixHook = (int(__cdecl*)(void*, void*, void*, void*))0x0071B430;
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

struct Vec3
{
	float x;
	float y;
	float z;
};

bool IsPressed(int key)
{
	return GetAsyncKeyState(key) & 0x8000;
}

int HK_Left, HK_Right, HK_Up, HK_Down;
float DisableTilts = 0;
float CamAngle = 0;
float CamSpeed;
int __cdecl CreateLookAtMatrixHook(void* outMatrix, Vec3* from, Vec3* to, Vec3* up)
{
	float Target = 0;
	bool isLeft = IsPressed(HK_Left);
	bool isRight = IsPressed(HK_Right);
	bool isUp = IsPressed(HK_Up);
	bool isDown = IsPressed(HK_Down);

	if (isLeft) Target = 360 - 85;
	if (isRight) Target = 85;
	if (isDown) Target = 180;
	if (isUp && isLeft) Target = 360 - 43;
	if (isUp && isRight) Target = 43;
	if (isDown && isLeft) Target = 360 - 135;
	if (isDown && isRight) Target = 135;

	int dir = CamAngle < Target ? 1 : -1;
	dir = abs(CamAngle - Target) > abs(360 - abs((CamAngle - Target))) ? -dir : dir;
	if (CamAngle != Target)
	{
		float step = *DeltaTime * CamSpeed * dir;

		if (abs(step) > abs(CamAngle - Target))
		{
			CamAngle = Target;
		}
		else
		{
			CamAngle += step;
			if (CamAngle > 360) CamAngle -= 360;
			if (CamAngle < 0) CamAngle += 360;
		}
	}

	if (CamAngle != 0)
	{
		DisableTilts = -1000;
		float angle = CamAngle * 3.14f / 180.0f;
		Vec3 newFrom;
		newFrom.x = cos(angle) * (from->x - to->x) - sin(angle) * (from->y - to->y) + to->x;
		newFrom.y = sin(angle) * (from->x - to->x) + cos(angle) * (from->y - to->y) + to->y;
		newFrom.z = from->z;
		*from = newFrom;
	}
	else
	{
		DisableTilts = 0;
	}

	return eCreateLookAtMatrixHook(outMatrix, from, to, up);
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

	if (iniReader.ReadInteger("CAMERA_ROTATION", "Enabled", 0) == 1)
	{
		HK_Left = iniReader.ReadInteger("CAMERA_ROTATION", "LookLeftKey", 37);
		HK_Right = iniReader.ReadInteger("CAMERA_ROTATION", "LookRightKey", 39);
		HK_Up = iniReader.ReadInteger("CAMERA_ROTATION", "LookForwardKey", 38);
		HK_Down = iniReader.ReadInteger("CAMERA_ROTATION", "LookBackKey", 40);

		CamSpeed = iniReader.ReadFloat("CAMERA_ROTATION", "Speed", 600);
		injector::MakeCALL(0x00492E5B, CreateLookAtMatrixHook, true);
		injector::WriteMemory<float*>(0x00492353, &DisableTilts, true);
	}

	int* NosTrailCount = (int*)0x00A732A8;
	*NosTrailCount = iniReader.ReadInteger("GENERAL", "NosTrailCount", 8);
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