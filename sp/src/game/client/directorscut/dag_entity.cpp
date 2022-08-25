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
