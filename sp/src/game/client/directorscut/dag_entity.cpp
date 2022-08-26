//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "dag_entity.h"
#include "vcollide_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_DagEntity::C_DagEntity()
{
	// Set default bone matrices
	CStudioHdr* ragdoll = GetModelPtr();
	if (ragdoll)
	{
		// Bruteforced?
		m_BoneAccessor.SetWritableBones(BONE_USED_BY_ANYTHING);
		m_BoneAccessor.SetReadableBones(BONE_USED_BY_ANYTHING);
		for (int i = 0; i < ragdoll->numbones(); i++)
		{
			mstudiobone_t* bone = ragdoll->pBone(i);
			if (bone)
			{
				// Get local space bone matrix
				Vector pos = bone->pos;
				QAngle ang;
				QuaternionAngles(bone->quat, ang);
				matrix3x4_t localMatrix;
				AngleMatrix(ang, pos, localMatrix);
				// Get world space entity matrix
				Vector absorigin = GetAbsOrigin();
				QAngle absangles = GetAbsAngles();
				matrix3x4_t worldMatrix;
				AngleMatrix(absangles, absorigin, worldMatrix);
				// Get world space bone matrix
				matrix3x4_t boneMatrix;
				ConcatTransforms(worldMatrix, localMatrix, boneMatrix);
				// Set bone matrix
				m_BoneMatrices[i] = boneMatrix;
			}
		}
	}
}

bool C_DagEntity::InitializeAsClientEntityByHandle(model_t* pModel, RenderGroup_t renderGroup)
{
	index = -1;

	// Setup model data.
	SetModelPointer(pModel);

	// Add the client entity to the master entity list.
	cl_entitylist->AddNonNetworkableEntity(GetIClientUnknown());
	Assert(GetClientHandle() != ClientEntityList().InvalidHandle());

	// Add the client entity to the renderable "leaf system." (Renderable)
	AddToLeafSystem(renderGroup);

	// Add the client entity to the spatial partition. (Collidable)
	CollisionProp()->CreatePartitionHandle();

	SpawnClientEntity();

	return true;
}

void C_DagEntity::ApplyBoneMatrixTransform(matrix3x4_t& transform)
{
	// Apply individual bone matrices using m_BoneMatrices
	CStudioHdr* studioHdr = GetModelPtr();
	if (studioHdr)
	{
		m_BoneAccessor.SetWritableBones(BONE_USED_BY_ANYTHING);
		m_BoneAccessor.SetReadableBones(BONE_USED_BY_ANYTHING);
		for (int i = 0; i < studioHdr->numbones(); i++)
		{
			matrix3x4_t& boneMatrix = GetBoneForWrite(i);
			ConcatTransforms(boneMatrix, m_BoneMatrices[i], boneMatrix);
		}
	}
	C_BaseAnimating::ApplyBoneMatrixTransform(transform);
}
