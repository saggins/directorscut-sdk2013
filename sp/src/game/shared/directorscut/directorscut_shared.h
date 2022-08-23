//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#ifndef _DIRECTORSCUT_SHARED_H_
#define _DIRECTORSCUT_SHARED_H_

#include "igamesystem.h"
#include "mathlib/vector.h"

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

	virtual void Update(float frametime);
#ifdef CLIENT_DLL
	virtual void LevelInitPreEntity();
	void SetupEngineView(Vector &origin, QAngle &angles, float &fov);
	bool GetD3D9Device();
	
	float cameraView[16];
	
	char* d3d9DeviceTable[119];
	float distance = 8.f;
	float fov = 90;
	Vector pivot;
	Vector engineOrigin;
	QAngle engineAngles;
#endif
};

// singleton
CDirectorsCutSystem &DirectorsCutGameSystem();

#endif
