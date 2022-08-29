//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "dag_entity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLightCustomEffect::CLightCustomEffect()
{
	TurnOn();
}

CLightCustomEffect::~CLightCustomEffect()
{
	if (m_FlashlightHandle != CLIENTSHADOW_INVALID_HANDLE)
	{
		g_pClientShadowMgr->DestroyFlashlight(m_FlashlightHandle);
		m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

void CLightCustomEffect::UpdateLight(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp, int nDistance)
{
	if (IsOn() == false)
		return;

	FlashlightState_t state;
	Vector basisX, basisY, basisZ;
	basisX = vecDir;
	basisY = vecRight;
	basisZ = vecUp;
	VectorNormalize(basisX);
	VectorNormalize(basisY);
	VectorNormalize(basisZ);

	BasisToQuaternion(basisX, basisY, basisZ, state.m_quatOrientation);

	state.m_vecLightOrigin = vecPos;

	state.m_fHorizontalFOVDegrees = 90.0f;
	state.m_fVerticalFOVDegrees = 90.0f;
	state.m_fQuadraticAtten = 0.0f;
	state.m_fLinearAtten = 100.0f;
	state.m_fConstantAtten = 0.0f;
	state.m_Color[0] = 1.0f;
	state.m_Color[1] = 1.0f;
	state.m_Color[2] = 1.0f;
	state.m_Color[3] = 0.0f;
	state.m_NearZ = 4.0f;
	state.m_FarZ = 750.0f;
	state.m_bEnableShadows = true;
	state.m_pSpotlightTexture = m_FlashlightTexture;
	state.m_nSpotlightTextureFrame = 0;

	if (GetFlashlightHandle() == CLIENTSHADOW_INVALID_HANDLE)
	{
		SetFlashlightHandle(g_pClientShadowMgr->CreateFlashlight(state));
	}
	else
	{
		g_pClientShadowMgr->UpdateFlashlightState(GetFlashlightHandle(), state);
	}

	g_pClientShadowMgr->UpdateProjectedTexture(GetFlashlightHandle(), true);
}
