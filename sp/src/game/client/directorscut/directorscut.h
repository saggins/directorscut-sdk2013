//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#ifndef _DIRECTORSCUT_H_
#define _DIRECTORSCUT_H_

#include "igamesystem.h"
#include "physpropclientside.h"
#include "mathlib/vector.h"
#include "dag_entity.h"

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
	
	virtual void PostInit();
	virtual void Shutdown();
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPostEntity();
	virtual void Update(float frametime);
	void SetupEngineView(Vector &origin, QAngle &angles, float &fov);
	void Frustum(float left, float right, float bottom, float top, float znear, float zfar, float* m16);
	void Perspective(float fov, float aspect, float znear, float zfar, float* m16);
	void OrthoGraphic(const float l, float r, float b, const float t, float zn, const float zf, float* m16);
	void LookAt(const float* eye, const float* at, const float* up, float* m16);
	float cameraView[16];
	float cameraProjection[16];
	float identityMatrix[16];
	float distance = 8.f;
	float fov = 93; // dunno why this works
	float fovAdjustment = 2;
	float playerFov;
	float snap = 1;
	float camYAngle = 165.f / 180.f * M_PI_F;
	float camXAngle = 32.f / 180.f * M_PI_F;
	Vector pivot;
	Vector engineOrigin;
	Vector playerOrigin;
	QAngle engineAngles;
	QAngle playerAngles;
	CUtlVector< C_DagEntity* > dags;
	int elementIndex = -1;
	bool useSnap = false;
	bool orthographic = false;
	bool firstEndScene = true;
	bool cursorState = false;
	bool imguiActive = false;
	bool levelInit = false;
	bool drawGrid = false;
};

// singleton
CDirectorsCutSystem &DirectorsCutGameSystem();

#endif
