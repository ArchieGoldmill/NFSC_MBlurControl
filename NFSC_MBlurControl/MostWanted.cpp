#include "CameraRotation.h"

#ifdef MOST_WANTED
void Init()
{
	CIniReader iniReader("NFSMWOrbitCamera.ini");
	InitCamera(iniReader);
}
#endif