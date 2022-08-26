//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#ifndef _DAG_ENTITY_H_
#define _DAG_ENTITY_H_

#include "c_baseanimating.h"
#include "physpropclientside.h"

class C_DagEntity : public C_BaseAnimating
{
public:
	DECLARE_CLASS(C_DagEntity, C_BaseAnimating);
	C_DagEntity();
	bool InitializeAsClientEntityByHandle(model_t* pModel, RenderGroup_t renderGroup);
	virtual void ApplyBoneMatrixTransform(matrix3x4_t& transform);
	matrix3x4_t m_BoneMatrices[MAXSTUDIOBONES];
};

#endif
