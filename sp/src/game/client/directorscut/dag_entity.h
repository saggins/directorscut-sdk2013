//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#ifndef _DAG_ENTITY_H_
#define _DAG_ENTITY_H_

#include "c_baseanimating.h"

class C_DagEntity : public C_BaseAnimating
{
public:
	bool InitializeAsClientEntityByHandle(model_t* pModel, RenderGroup_t renderGroup);
};

#endif
