//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#ifndef _DAG_ENTITY_H_
#define _DAG_ENTITY_H_

#include "c_baseentity.h"
#include "c_baseanimating.h"
#include "basetypes.h"
#include "flashlighteffect.h"
#include "c_baseflex.h"

class CModelElement : public C_BaseFlex
{
	DECLARE_CLASS(CModelElement, C_BaseFlex);
public:
	CModelElement();
	~CModelElement();
	virtual void	Simulate();
	CModelElement* BecomeRagdollOnClient();
	CModelElement* CreateRagdollCopy();
	virtual bool SetModel(const char* pModelName);
	void PoseBones(int bonenumber, Vector pos, QAngle ang);
	virtual matrix3x4_t& GetBoneForWrite(int iBone);
	virtual bool SetupBones(matrix3x4_t* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime);
	virtual float GetFlexWeight(LocalFlexController_t index);
	virtual void SetFlexWeight(LocalFlexController_t index, float value);
	//virtual void ApplyBoneMatrixTransform(matrix3x4_t& transform);
	bool firsttimesetup = false;
	Vector posadds[MAXSTUDIOBONES];
	QAngle anglehelper[MAXSTUDIOBONES];
	QuaternionAligned qadds[MAXSTUDIOBONES];
	float forcedFlexes[MAXSTUDIOFLEXCTRL];
};

class CLightCustomEffect : public C_BaseEntity, CFlashlightEffect
{
	DECLARE_CLASS(CLightCustomEffect, C_BaseEntity);
public:
	CLightCustomEffect();
	~CLightCustomEffect();

	virtual void UpdateLight(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp, int nDistance);
};

#endif
