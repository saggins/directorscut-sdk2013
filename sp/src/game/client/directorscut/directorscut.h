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
#include <string>
#include "C_Env_Projected_Texture.h"
#include "c_point_camera.h"

class Version
{
public:
	Version(const int major, const int minor, const int patch) : m_major(major), m_minor(minor), m_patch(patch)
	{
		char* version = new char[16];
		sprintf(version, "%d.%d.%d", m_major, m_minor, m_patch);
		m_version = version;
	}
	~Version()
	{
		delete[] m_version;
	}
	const char* GetVersion() const
	{
		return m_version;
	}
protected:
	int m_major;
	int m_minor;
	int m_patch;
	char* m_version;
};

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
	virtual void LevelShutdownPreEntity();
	virtual void Update(float frametime);
	virtual void PostRender();
	void SetupEngineView(Vector &origin, QAngle &angles, float &fov);
	void Frustum(float left, float right, float bottom, float top, float znear, float zfar, float* m16);
	void Perspective(float fov, float aspect, float znear, float zfar, float* m16);
	void OrthoGraphic(const float l, float r, float b, const float t, float zn, const float zf, float* m16);
	void LookAt(const float* eye, const float* at, const float* up, float* m16);
	float cameraView[16];
	float cameraProjection[16];
	float identityMatrix[16];
	float snap[3];
	float distance = 100.f;
	float fov = 93; // dunno why this works
	float fovAdjustment = 2;
	float playerFov;
	float camYAngle = 165.f / 180.f * M_PI_F;
	float camXAngle = 32.f / 180.f * M_PI_F;
	float gridSize = 500.f;
	float currentTimeScale = 1.f;
	float timeScale = 1.f;
	Vector pivot;
	Vector engineOrigin;
	Vector playerOrigin;
	Vector deltaOrigin;
	Vector poseBoneOrigin;
	QAngle engineAngles;
	QAngle playerAngles;
	QAngle deltaAngles;
	QAngle poseBoneAngles;
	CUtlVector< CModelElement* > dags;
	CUtlVector< CLightCustomEffect* > lights;
	CUtlVector < C_PointCamera* > cameras;
	Version directorcut_version = Version(0, 1, 1);
	char* directorscut_author = "KiwifruitDev";
	std::string modelName = "models/alyx.mdl";
	std::string lightTexture = "effects/flashlight001";
	int elementIndex = -1;
	int nextElementMode = 0;
	int elementMode = 0;
	int ragdollIndex = -1;
	int boneIndex = -1;
	int poseIndex = -1;
	int flexIndex = -1;
	int operation = 2;
	bool useSnap = false;
	bool orthographic = false;
	bool firstEndScene = true;
	bool cursorState = false;
	bool imguiActive = false;
	bool levelInit = false;
	bool drawGrid = false;
	bool needToSetPoseBoneToPivot = false;
	bool selecting = false;
};

// singleton
CDirectorsCutSystem &DirectorsCutGameSystem();

#endif
