//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "gamestringpool.h"
#include "dag_entity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CModelElement::CModelElement()
{
	m_BoneAccessor.SetWritableBones(BONE_USED_BY_ANYTHING);
	m_BoneAccessor.SetReadableBones(BONE_USED_BY_ANYTHING);
}

CModelElement::~CModelElement()
{
}

float CModelElement::GetFlexWeight(LocalFlexController_t index)
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		return forcedFlexes[index];
	}
	return 0.0;
}

void CModelElement::SetFlexWeight(LocalFlexController_t index, float value)
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		forcedFlexes[index] = value;
	}
}

void CModelElement::Simulate()
{
	BaseClass::Simulate();
	for (int i = 0; i < GetNumFlexControllers(); i++)
	{
		BaseClass::SetFlexWeight((LocalFlexController_t)i, forcedFlexes[i]);
	}
}

bool CModelElement::SetModel(const char* pModelName)
{
	BaseClass::SetModel(pModelName);
	return true;
}

void CModelElement::PoseBones(int bonenumber, Vector pos, QAngle ang)
{
	posadds[bonenumber] = pos;
	anglehelper[bonenumber] = ang;
}
	

bool CModelElement::SetupBones(matrix3x4_t* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	if (firsttimesetup == false)
	{
		for (int i = 0; i < MAXSTUDIOBONES; i++)
		{
			posadds[i].x = 0;
			posadds[i].y = 0;
			posadds[i].z = 0;
			anglehelper[i].x = 0;
			anglehelper[i].y = 0;
			anglehelper[i].z = 0;
		}
		firsttimesetup = true;
	}
	for (int i = 0; i < MAXSTUDIOBONES; i++)
	{
		AngleQuaternion(anglehelper[i], qadds[i]);
	}
	return BaseClass::SetupBones(pBoneToWorldOut, nMaxBones, boneMask, currentTime, true, posadds, qadds);
}

inline matrix3x4_t& CModelElement::GetBoneForWrite(int iBone)
{
	m_BoneAccessor.SetWritableBones(BONE_USED_BY_ANYTHING);
	m_BoneAccessor.SetReadableBones(BONE_USED_BY_ANYTHING);
	return BaseClass::GetBoneForWrite(iBone);
}

/*

void CModelElement::ApplyBoneMatrixTransform(matrix3x4_t& transform)
{
	m_BoneAccessor.SetWritableBones(BONE_USED_BY_ANYTHING);
	m_BoneAccessor.SetReadableBones(BONE_USED_BY_ANYTHING);
	PushAllowBoneAccess(true, true, "CModelElement::ApplyBoneMatrixTransform");
	CStudioHdr* hdr = GetModelPtr();
	if (hdr != NULL)
	{
		if (initializedBones)
		{
			// If origin/angles are dirty (old), add their differences to the bone matrix
			Vector origin = GetAbsOrigin();
			QAngle angles = GetAbsAngles();
			if (origin != dirtyOrigin || angles != dirtyAngles)
			{
				Vector diffOrigin = origin - dirtyOrigin;
				QAngle diffAngles = angles - dirtyAngles;
				Vector boneOrigin;
				QAngle boneAngles;
				for (int i = 0; i < hdr->numbones(); i++)
				{
					MatrixAngles(boneMatrices[i], boneAngles, boneOrigin);
					boneAngles += diffAngles;
					boneOrigin += diffOrigin;
					AngleMatrix(boneAngles, boneOrigin, boneMatrices[i]);
				}
				dirtyOrigin = origin;
				dirtyAngles = angles;
			}
		}
		else
		{
			dirtyOrigin = GetAbsOrigin();
			dirtyAngles = GetAbsAngles();
		}
		for (int i = 0; i < hdr->numbones(); i++)
		{
			matrix3x4_t& bone = GetBoneForWrite[i];
			mstudiobone_t* bonePtr = hdr->pBone[i];
			char* name = bonePtr->pszName();

			if(!initializedBones)
			{
				// Initialize bone matrices
				MatrixCopy(bone, boneMatrices[i]);
			}

			matrix3x4_t oldBone;
			MatrixCopy(bone, oldBone);
			
			// Write new bone matrices
			MatrixCopy(boneMatrices[i], bone);
			
			// compare name with "ValveBiped.Bip01_Head1"
			bool compare0 = (oldBone[0][0] != bone[0][0] || oldBone[0][1] != bone[0][1] || oldBone[0][2] != bone[0][2] || oldBone[0][3] != bone[0][3]);

			if (Q_strcmp(name, "ValveBiped.Bip01_Head1") != 0 && (compare0))
				Msg("New bone matrix: %f %f %f %f\n", bone[0][0], bone[0][1], bone[0][2], bone[0][3]);
		}
		if (!initializedBones)
			initializedBones = true;
	}
	PopBoneAccess("CModelElement::ApplyBoneMatrixTransform");
	C_BaseAnimating::ApplyBoneMatrixTransform(transform);
}

*/

CModelElement* CModelElement::CreateRagdollCopy()
{	
	CModelElement* copy = new CModelElement();
	if (copy == NULL)
		return NULL;

	const model_t* model = GetModel();
	const char* pModelName = modelinfo->GetModelName(model);

	if (copy->InitializeAsClientEntity(pModelName, RENDER_GROUP_OPAQUE_ENTITY) == false)
	{
		copy->Release();
		return NULL;
	}

	// move my current model instance to the ragdoll's so decals are preserved.
	SnatchModelInstance(copy);

	// We need to take these from the entity
	copy->SetAbsOrigin(GetAbsOrigin());
	copy->SetAbsAngles(GetAbsAngles());

	copy->IgniteRagdoll(this);
	copy->TransferDissolveFrom(this);
	copy->InitModelEffects();
	
	AddEffects(EF_NODRAW);

	if (IsEffectActive(EF_NOSHADOW))
	{
		copy->AddEffects(EF_NOSHADOW);
	}

	copy->m_nRenderFX = kRenderFxRagdoll;
	copy->SetRenderMode(GetRenderMode());
	copy->SetRenderColor(GetRenderColor().r, GetRenderColor().g, GetRenderColor().b, GetRenderColor().a);

	copy->m_nBody = m_nBody;
	copy->m_nSkin = GetSkin();
	copy->m_vecForce = m_vecForce;
	copy->m_nForceBone = m_nForceBone;
	copy->SetNextClientThink(CLIENT_THINK_ALWAYS);

#ifdef MAPBASE
	copy->m_iViewHideFlags = m_iViewHideFlags;

	copy->m_fadeMinDist = m_fadeMinDist;
	copy->m_fadeMaxDist = m_fadeMaxDist;
	copy->m_flFadeScale = m_flFadeScale;
#endif

	copy->SetModelName(AllocPooledString(pModelName));
	copy->SetModelScale(GetModelScale());

	/*
	CStudioHdr* hdr = GetModelPtr();
	if (hdr != NULL)
	{
		for (int i = 0; i < hdr->numbones(); i++)
		{
			// Copy bone matrices
			MatrixCopy(boneMatrices[i], copy->boneMatrices[i]);
		}
	}
	*/

	// Copy posadds/qadds
	copy->firsttimesetup = firsttimesetup;
	for (int i = 0; i < MAXSTUDIOBONES; i++)
	{
		copy->posadds[i].x = posadds[i].x;
		copy->posadds[i].y = posadds[i].y;
		copy->posadds[i].z = posadds[i].z;
		copy->qadds[i].x = qadds[i].x;
		copy->qadds[i].y = qadds[i].y;
		copy->qadds[i].z = qadds[i].z;
		copy->qadds[i].w = qadds[i].w;
	}

	// Copy flexes
	for (int i = 0; i < MAXSTUDIOFLEXCTRL; i++)
	{
		copy->forcedFlexes[i] = forcedFlexes[i];
	}
	
	return copy;
}

CModelElement* CModelElement::BecomeRagdollOnClient()
{
	MoveToLastReceivedPosition(true);
	GetAbsOrigin();
	CModelElement* pRagdoll = CreateRagdollCopy();

	matrix3x4_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.1f;
	GetRagdollInitBoneArrays(boneDelta0, boneDelta1, currentBones, boneDt);
	pRagdoll->InitAsClientRagdoll(boneDelta0, boneDelta1, currentBones, boneDt);

#ifdef MAPBASE_VSCRIPT
	// Hook for ragdolling
	if (m_ScriptScope.IsInitialized() && g_Hook_OnClientRagdoll.CanRunInScope(m_ScriptScope))
	{
		// ragdoll
		ScriptVariant_t args[] = { ScriptVariant_t(pRagdoll->GetScriptInstance()) };
		g_Hook_OnClientRagdoll.Call(m_ScriptScope, NULL, args);
	}
#endif

	return pRagdoll;
}

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
