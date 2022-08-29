//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#ifndef _DAG_ENTITY_H_
#define _DAG_ENTITY_H_

#include "c_baseentity.h"
#include "basetypes.h"
#include "flashlighteffect.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
// NOT A PROJECTED TEXTURE ANYMORE!
class CLightCustomEffect : public C_BaseEntity, CFlashlightEffect
{
	DECLARE_CLASS(CLightCustomEffect, C_BaseEntity);
public:
	CLightCustomEffect();
	~CLightCustomEffect();

	virtual void UpdateLight(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp, int nDistance);
};

#endif
