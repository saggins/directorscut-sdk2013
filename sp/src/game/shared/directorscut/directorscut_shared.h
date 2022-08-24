//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#ifndef _DIRECTORSCUT_SHARED_H_
#define _DIRECTORSCUT_SHARED_H_

#include "igamesystem.h"

#ifdef CLIENT_DLL
#include "mathlib/vector.h"
#endif

class CDirectorsCutSystem : public CAutoGameSystemPerFrame
{
public:

	CDirectorsCutSystem() : CAutoGameSystemPerFrame("CDirectorsCutSystem")
	{
	}

	virtual bool InitAllSystems()
	{
		return true;
	}
	
#ifdef CLIENT_DLL
	virtual void PostInit();
	virtual void Shutdown();
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPostEntity();
	virtual void Update(float frametime);
	void SetupEngineView(Vector &origin, QAngle &angles, float &fov);
	float cameraView[16];
	float distance = 8.f;
	float fov = 90;
	Vector pivot;
	Vector engineOrigin;
	Vector playerOrigin;
	QAngle engineAngles;
	bool firstEndScene = true;
	bool cursorState = false;
	bool imguiActive = false;
	bool levelInit = false;
#endif
};

// singleton
CDirectorsCutSystem &DirectorsCutGameSystem();

#endif
