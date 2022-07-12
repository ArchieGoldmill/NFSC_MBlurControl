#include <Windows.h>
#include "injector/injector.hpp"
#include "CameraRotation.h"

#ifdef MOST_WANTED
float* DeltaTime = (float*)0x009259BC;
auto eCreateLookAtMatrix = (int(__cdecl*)(void*, void*, void*, void*))0x006CF0A0;
void* HookAddr = (void*)0X0047DCBC;
void* DisableTiltsAddr = (void*)0X0047D506;
#endif

#ifdef CARBON
float* DeltaTime = (float*)0x00A99A5C;
auto eCreateLookAtMatrix = (int(__cdecl*)(void*, void*, void*, void*))0x0071B430;
void* HookAddr = (void*)0x00492E5B;
void* DisableTiltsAddr = (void*)0x00492353;
#endif

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

	return eCreateLookAtMatrix(outMatrix, from, to, up);
}

void InitCamera(CIniReader&iniReader)
{
	if (iniReader.ReadInteger("CAMERA_ROTATION", "Enabled", 0) == 1)
	{
		HK_Left = iniReader.ReadInteger("CAMERA_ROTATION", "LookLeftKey", 37);
		HK_Right = iniReader.ReadInteger("CAMERA_ROTATION", "LookRightKey", 39);
		HK_Up = iniReader.ReadInteger("CAMERA_ROTATION", "LookForwardKey", 38);
		HK_Down = iniReader.ReadInteger("CAMERA_ROTATION", "LookBackKey", 40);

		CamSpeed = iniReader.ReadFloat("CAMERA_ROTATION", "Speed", 600);
		injector::MakeCALL(HookAddr, CreateLookAtMatrixHook, true);
		injector::WriteMemory<float*>(DisableTiltsAddr, &DisableTilts, true);
	}
}