#include "CameraRotation.h"

#ifdef MOST_WANTED
void Init()
{
	CIniReader iniReader("NFSMWFreeRoamCam.ini");
	InitCamera(iniReader);
}
#endif