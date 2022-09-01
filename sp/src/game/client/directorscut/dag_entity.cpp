//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "gamestringpool.h"
#include "datacache/imdlcache.h"
#include "networkstringtable_clientdll.h"
#include "dag_entity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CDirectorsCutElement::CDirectorsCutElement()
{
	C_BaseEntity* pEntity = new C_BaseEntity();
	if (!pEntity)
	{
		Msg("Director's Cut: Failed to create generic dag\n");
		return;
	}
	SetElementPointer(pEntity);
}

CDirectorsCutElement::CDirectorsCutElement(DAG_ type, KeyValues* params)
{
	SetElementType(type);
	Vector pivot = Vector(params->GetFloat("pivot_x"), params->GetFloat("pivot_y"), params->GetFloat("pivot_z"));
	switch (type)
	{
	case DAG_MODEL:
	{
		char* modelName = (char*)params->GetString("modelName");

		// Prepend "models/" if not present
		if (strncmp(modelName, "models/", 7) != 0)
		{
			char newModelName[256];
			sprintf(newModelName, "models/%s", modelName);
			modelName = newModelName;
		}
		// Append ".mdl" if not present
		if (strstr(modelName, ".mdl") == NULL)
		{
			char newModelName[256];
			sprintf(newModelName, "%s.mdl", modelName);
			modelName = newModelName;
		}

		// Cache model
		model_t* model = (model_t*)engine->LoadModel(modelName);
		if (!model)
		{
			Msg("Director's Cut: Failed to load model %s\n", modelName);
			break;
		}
		INetworkStringTable* precacheTable = networkstringtable->FindTable("modelprecache");
		if (precacheTable)
		{
			modelinfo->FindOrLoadModel(modelName);
			int idx = precacheTable->AddString(false, modelName);
			if (idx == INVALID_STRING_INDEX)
			{
				Msg("Director's Cut: Failed to precache model %s\n", modelName);
			}
		}

		// Create test dag
		CModelElement* pEntity = new CModelElement();
		if (!pEntity)
			break;
		pEntity->SetModel(modelName);
		pEntity->SetModelName(modelName);
		pEntity->SetAbsOrigin(pivot);

		// Spawn entity
		RenderGroup_t renderGroup = RENDER_GROUP_OPAQUE_ENTITY;
		if (!pEntity->InitializeAsClientEntity(modelName, renderGroup))
		{
			Msg("Director's Cut: Failed to spawn entity %s\n", modelName);
			pEntity->Release();
			break;
		}

		SetElementPointer(pEntity);
		break;
	}
	case DAG_LIGHT:
	{
		char* texture = (char*)params->GetString("lightTexture");
		CLightCustomEffect* pEntity = new CLightCustomEffect();
		if (!pEntity)
		{
			Msg("Director's Cut: Failed to create light with texture %s\n", texture);
			break;
		}
		pEntity->SetAbsOrigin(pivot);
		SetElementPointer(pEntity);
		break;
	}
	case DAG_CAMERA:
	{
		CCameraElement* pEntity = new CCameraElement();
		if (!pEntity)
		{
			Msg("Director's Cut: Failed to create camera\n");
			break;
		}
		pEntity->SetAbsOrigin(pivot);
		SetElementPointer(pEntity);
		break;
	}
	default:
	{
		C_BaseEntity* pEntity = new C_BaseEntity();
		if (!pEntity)
		{
			Msg("Director's Cut: Failed to create generic dag\n");
			break;
		}
		pEntity->SetAbsOrigin(pivot);
		SetElementPointer(pEntity);
		break;
	}
	}
}

CDirectorsCutElement::~CDirectorsCutElement()
{
	C_BaseEntity* pEntity = (C_BaseEntity*)pElement;
	if (pEntity != nullptr)
	{
		pEntity->Remove();
	}
}

void CDirectorsCutElement::SetElementType(DAG_ type)
{
	elementType = type;
}

DAG_ CDirectorsCutElement::GetElementType()
{
	return elementType;
}

void CDirectorsCutElement::SetElementPointer(void* pElement)
{
	this->pElement = pElement;
}

void* CDirectorsCutElement::GetElementPointer()
{
	return pElement;
}

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
	C_BaseFlex::Simulate();
	for (int i = 0; i < GetNumFlexControllers(); i++)
	{
		C_BaseFlex::SetFlexWeight((LocalFlexController_t)i, forcedFlexes[i]);
	}
}

bool CModelElement::SetModel(const char* pModelName)
{
	C_BaseFlex::SetModel(pModelName);
	return true;
}

void CModelElement::PoseBones(int bonenumber, Vector pos, QAngle ang)
{
	posadds[bonenumber] = pos;
	anglehelper[bonenumber] = ang;
}
	

bool CModelElement::SetupBones(matrix3x4_t* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	for (int i = 0; i < MAXSTUDIOBONES; i++)
	{
		if (firsttimesetup == false)
		{
			posadds[i].x = 0;
			posadds[i].y = 0;
			posadds[i].z = 0;
			anglehelper[i].x = 0;
			anglehelper[i].y = 0;
			anglehelper[i].z = 0;
		}
		AngleQuaternion(anglehelper[i], qadds[i]);
	}
	firsttimesetup = true;
	m_BoneAccessor.SetWritableBones(BONE_USED_BY_ANYTHING);
	m_BoneAccessor.SetReadableBones(BONE_USED_BY_ANYTHING);
	return C_BaseFlex::SetupBones(pBoneToWorldOut, nMaxBones, boneMask, currentTime, true, posadds, qadds);
}

inline matrix3x4_t& CModelElement::GetBoneForWrite(int iBone)
{
	m_BoneAccessor.SetWritableBones(BONE_USED_BY_ANYTHING);
	m_BoneAccessor.SetReadableBones(BONE_USED_BY_ANYTHING);
	return C_BaseFlex::GetBoneForWrite(iBone);
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

void CLightCustomEffect::Simulate()
{
	// get dir, right, and up from angles
	QAngle angles = GetAbsAngles();
	Vector dir, right, up;
	AngleVectors(angles, &dir, &right, &up);
	UpdateLight(GetAbsOrigin(), dir, right, up, 1000);
	BaseClass::Simulate();
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

CCameraElement::CCameraElement()
{
}

CCameraElement::~CCameraElement()
{
}
