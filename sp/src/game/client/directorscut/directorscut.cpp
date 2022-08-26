//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "directorscut.h"
#include "imgui_public.h"
#include "direct3d_hook.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Panel.h"
#include "input.h"
#include "view.h"
#include "viewrender.h"
#include "engine/IClientLeafSystem.h"
#include "view_shared.h"
#include "viewrender.h"
#include "gamestringpool.h"
#include "datacache/imdlcache.h"
#include "networkstringtable_clientdll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CDirectorsCutSystem g_DirectorsCutSystem;

CDirectorsCutSystem &DirectorsCutGameSystem()
{
	return g_DirectorsCutSystem;
}

char* defaultDagModelName = "models/editor/axis_helper.mdl";
C_BaseAnimating* CreateDag(char* modelName, bool add, bool setIndex);

char* defaultRagdollName = "models/alyx.mdl";
C_BaseAnimating* CreateRagdoll(char* modelName, bool add, bool setIndex);

C_BaseAnimating* MakeRagdoll(C_BaseAnimating* dag);
C_BaseAnimating* MakeRagdoll(C_BaseAnimating* dag, bool add, bool setIndex);

ConVar cvar_imgui_enabled("dx_imgui_enabled", "1", FCVAR_ARCHIVE, "Enable ImGui");
ConVar cvar_imgui_input_enabled("dx_imgui_input_enabled", "1", FCVAR_ARCHIVE, "Capture ImGui input");
ConVar cvar_dx_zoom_distance("dx_zoom_distance", "50.0f", FCVAR_ARCHIVE, "Default zoom distance");
ConCommand cmd_imgui_toggle("dx_imgui_toggle", [](const CCommand& args) {
	cvar_imgui_enabled.SetValue(!cvar_imgui_enabled.GetBool());
}, "Toggle ImGui", FCVAR_ARCHIVE);
ConCommand cmd_imgui_input_toggle("dx_imgui_input_toggle", [](const CCommand& args) {
	cvar_imgui_input_enabled.SetValue(!cvar_imgui_input_enabled.GetBool());
}, "Toggle ImGui input", FCVAR_ARCHIVE);
ConCommand cmd_create_dag("dx_create_dag", [](const CCommand& args) {
	char* modelName = defaultDagModelName;
	if (args.ArgC() > 1)
		modelName = (char*)args.Arg(1);
	CreateDag(modelName, true, true);
}, "Create dag", FCVAR_ARCHIVE);
ConCommand cmd_create_ragdoll("dx_create_ragdoll", [](const CCommand& args) {
	char* modelName = defaultRagdollName;
	if (args.ArgC() > 1)
		modelName = (char*)args.Arg(1);
	CreateRagdoll(modelName, true, true);
}, "Create ragdoll (UNSUPPORTED)", FCVAR_ARCHIVE);
ConCommand cmd_zoom_in("dx_zoom_in", [](const CCommand& args) {
	if (cvar_imgui_input_enabled.GetBool())
	{
		if (args.ArgC() > 1)
			g_DirectorsCutSystem.distance += atof(args.Arg(1));
		else
			g_DirectorsCutSystem.distance += cvar_dx_zoom_distance.GetFloat();
	}
}, "Zoom in with an optional distance", FCVAR_ARCHIVE);
ConCommand cmd_zoom_out("dx_zoom_out", [](const CCommand& args) {
	if (cvar_imgui_input_enabled.GetBool())
	{
		if (args.ArgC() > 1)
			g_DirectorsCutSystem.distance -= atof(args.Arg(1));
		else
			g_DirectorsCutSystem.distance -= cvar_dx_zoom_distance.GetFloat();
	}
}, "Zoom out with an optional distance", FCVAR_ARCHIVE);

WNDPROC ogWndProc = nullptr;

static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);

// Render Dear ImGui
void APIENTRY EndScene(LPDIRECT3DDEVICE9 p_pDevice)
{
	bool firstFrame = false;
	if (g_DirectorsCutSystem.firstEndScene)
	{
		g_DirectorsCutSystem.firstEndScene = false;
		firstFrame = true;
		if(g_DirectorsCutSystem.imguiActive)
			ImGui_ImplDX9_Init(p_pDevice);
	}
	if (cvar_imgui_enabled.GetBool() && g_DirectorsCutSystem.imguiActive)
	{
		ImGuiIO& io = ImGui::GetIO();

		int windowWidth;
		int windowHeight;
		vgui::surface()->GetScreenSize(windowWidth, windowHeight);

		// Drawing
		
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		if (g_DirectorsCutSystem.orthographic)
		{
			float orthoWidth, orthoHeight;
			::input->CAM_OrthographicSize(orthoWidth, orthoHeight);
			orthoHeight = orthoWidth * windowHeight / windowWidth;
			g_DirectorsCutSystem.OrthoGraphic(-orthoWidth, orthoHeight, -orthoWidth, orthoHeight, 1000.f, -1000.f, g_DirectorsCutSystem.cameraProjection);
		}
		else
		{
			const CViewSetup* viewSetup = view->GetViewSetup();
			//ImGui::SliderFloat("Fov Adjustment", &g_DirectorsCutSystem.fovAdjustment, 1.f, 100.f);
			float fov = viewSetup->fov * g_DirectorsCutSystem.fovAdjustment;
			g_DirectorsCutSystem.Perspective(fov, io.DisplaySize.x / io.DisplaySize.y, viewSetup->zNear, viewSetup->zFar, g_DirectorsCutSystem.cameraProjection);
		}
		
		ImGuizmo::SetOrthographic(g_DirectorsCutSystem.orthographic);

		ImGui::Begin("Camera", 0, ImGuiWindowFlags_AlwaysAutoResize);


		ImGui::LabelText("Origin", "%.0f %.0f %.0f", g_DirectorsCutSystem.engineOrigin.x, g_DirectorsCutSystem.engineOrigin.y, g_DirectorsCutSystem.engineOrigin.z);
		ImGui::LabelText("Angles", "%.0f %.0f %.0f", g_DirectorsCutSystem.engineAngles.x, g_DirectorsCutSystem.engineAngles.y, g_DirectorsCutSystem.engineAngles.z);

		float pivot[3];
		pivot[0] = g_DirectorsCutSystem.pivot.x;
		pivot[1] = g_DirectorsCutSystem.pivot.y;
		pivot[2] = g_DirectorsCutSystem.pivot.z;
		ImGui::InputFloat3("Pivot", pivot);
		g_DirectorsCutSystem.pivot.x = pivot[0];
		g_DirectorsCutSystem.pivot.y = pivot[1];
		g_DirectorsCutSystem.pivot.z = pivot[2];
		
		if (ImGui::Button("Player Camera to Pivot"))
		{
			g_DirectorsCutSystem.pivot = g_DirectorsCutSystem.playerOrigin;
		}

		if (g_DirectorsCutSystem.elementIndex > -1)
		{
			C_BaseAnimating* elementE = g_DirectorsCutSystem.dags[g_DirectorsCutSystem.elementIndex];
			if (elementE)
			{
				if (ImGui::Button("Selected Dag to Pivot"))
				{
					g_DirectorsCutSystem.pivot = elementE->GetAbsOrigin();
				}

				if (elementE->IsRagdoll())
				{
					if (g_DirectorsCutSystem.boneIndex > -1)
					{
						if (ImGui::Button("Selected Bone to Pivot"))
						{
							QAngle dummy;
							elementE->m_pRagdoll->GetElement(g_DirectorsCutSystem.boneIndex)->GetPosition(&g_DirectorsCutSystem.pivot, &dummy);
						}
					}
				}
			}
		}
		
		bool viewDirty = false;

		viewDirty |= ImGui::SliderFloat("Field of View", &g_DirectorsCutSystem.fov, 1, 179.0f, "%.3f");

		float oldDistance = g_DirectorsCutSystem.distance;
		float mouseDistance = -ImGui::GetIO().MouseWheel * cvar_dx_zoom_distance.GetFloat();
		if (g_DirectorsCutSystem.distance + mouseDistance >= 1.f)
			g_DirectorsCutSystem.distance += mouseDistance;
		else
			g_DirectorsCutSystem.distance = 1.f;
		if (oldDistance != g_DirectorsCutSystem.distance)
			viewDirty = true;
		
		viewDirty |= ImGui::SliderFloat("Distance", &g_DirectorsCutSystem.distance, 1, 1000.0f, "%.0f");
		
		if (viewDirty || firstFrame)
		{
			// TODO: Look at current view manipulation
			float eye[] = { cosf(g_DirectorsCutSystem.camYAngle) * cosf(g_DirectorsCutSystem.camXAngle) * g_DirectorsCutSystem.distance, sinf(g_DirectorsCutSystem.camXAngle) * g_DirectorsCutSystem.distance, sinf(g_DirectorsCutSystem.camYAngle) * cosf(g_DirectorsCutSystem.camXAngle) * g_DirectorsCutSystem.distance };
			float at[] = { 0.f, 0.f, 0.f };
			float up[] = { 0.f, 1.f, 0.f };
			g_DirectorsCutSystem.LookAt(eye, at, up, g_DirectorsCutSystem.cameraView);
			firstFrame = false;	
		}

		ImGui::Checkbox("Draw Grid", &g_DirectorsCutSystem.drawGrid);
		ImGui::Checkbox("Orthographic", &g_DirectorsCutSystem.orthographic);
		ImGui::SliderInt("Set Operation", &g_DirectorsCutSystem.operation, 0, 2);
		switch (g_DirectorsCutSystem.operation)
		{
			case 0:
				ImGui::LabelText("Operation", "Translate");
				mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
				break;
			case 1:
				ImGui::LabelText("Operation", "Rotate");
				mCurrentGizmoOperation = ImGuizmo::ROTATE;
				break;
			case 2:
				ImGui::LabelText("Operation", "Universal");
				mCurrentGizmoOperation = ImGuizmo::TRANSLATE | ImGuizmo::ROTATE;
		}
		ImGui::Checkbox("Use Snapping", &g_DirectorsCutSystem.useSnap);
		if (g_DirectorsCutSystem.useSnap)
		{
			ImGui::InputFloat3("Snap", g_DirectorsCutSystem.snap);
		}

		ImGui::End();

		ImGui::Begin("Dags", 0, ImGuiWindowFlags_AlwaysAutoResize);

		if (ImGui::Button("Spawn Dag"))
			CreateDag(defaultDagModelName, true, true);
		ImGui::SameLine();
		if (ImGui::Button("Spawn Dag at Pivot"))
		{
			C_BaseAnimating* pEntity = CreateDag(defaultDagModelName, true, true);
			if (pEntity)
			{
				pEntity->SetAbsOrigin(g_DirectorsCutSystem.pivot);
			}
		}

		/*
		if (ImGui::Button("Spawn Ragdoll"))
			CreateRagdoll(defaultRagdollName, true, true);
		ImGui::SameLine();
		if (ImGui::Button("Spawn Ragdoll at Pivot"))
		{
			C_BaseAnimating* pEntity = CreateDag(defaultRagdollName, false, false);
			if (pEntity)
			{
				pEntity->SetAbsOrigin(g_DirectorsCutSystem.pivot);
				MakeRagdoll(pEntity, true, true);
			}
		}
		*/

		if (g_DirectorsCutSystem.dags.Count() > 0)
		{
			ImGui::LabelText("Dags", "%d", g_DirectorsCutSystem.dags.Count());
			if (g_DirectorsCutSystem.dags.Count() > 1)
			{
				if (ImGui::SliderInt("Dag Index", &g_DirectorsCutSystem.elementIndex, 0, g_DirectorsCutSystem.dags.Count() - 1))
				{
					C_BaseAnimating* pEntity = g_DirectorsCutSystem.dags[g_DirectorsCutSystem.elementIndex];
					if (pEntity && pEntity->IsRagdoll() && g_DirectorsCutSystem.ragdollIndex != g_DirectorsCutSystem.elementIndex)
					{
						g_DirectorsCutSystem.ragdollIndex = g_DirectorsCutSystem.elementIndex;
						g_DirectorsCutSystem.boneIndex = 0;
					}
					else
					{
						g_DirectorsCutSystem.boneIndex = -1;
					}
				}
			}
			else
			{
				ImGui::LabelText("Dag Index", "%d", g_DirectorsCutSystem.elementIndex);
			}
			C_BaseAnimating* pEntity = g_DirectorsCutSystem.dags[g_DirectorsCutSystem.elementIndex];
			if (pEntity)
			{
				ImGui::LabelText("Dag Model", "%s", pEntity->GetModelPtr()->pszName());
				if (pEntity->IsRagdoll())
				{
					CStudioHdr* modelPtr = pEntity->GetModelPtr();
					if (modelPtr)
					{
						char* boneModel = "";
						for (int i = 0; i < modelPtr->numbones(); i++)
						{
							mstudiobone_t* bone = modelPtr->pBone(i);
							if (bone)
							{
								if (bone->physicsbone == g_DirectorsCutSystem.boneIndex)
								{
									boneModel = bone->pszName();
									break;
								}
							}
						}
						ImGui::LabelText("Bone Model", "%s", boneModel);
					}
					ImGui::SliderInt("Bone Index", &g_DirectorsCutSystem.boneIndex, 0, pEntity->m_pRagdoll->RagdollBoneCount() - 1);
					CRagdoll* ragdoll = pEntity->m_pRagdoll;
					if (ragdoll)
					{
						IPhysicsObject* bone = ragdoll->GetElement(g_DirectorsCutSystem.boneIndex);
						if (bone)
						{
							//ImGui::LabelText("Bone Name", "%s", ragdoll->name);
							if (!bone->IsMotionEnabled())
							{
								if (ImGui::Button("Unfreeze Bone"))
								{
									PhysForceClearVelocity(bone);
									bone->EnableMotion(true);
									bone->Wake();
									PhysForceClearVelocity(bone);
								}
							}
							else
							{
								if (ImGui::Button("Freeze Bone"))
								{
									PhysForceClearVelocity(bone);
									bone->Sleep();
									bone->EnableMotion(false);
									PhysForceClearVelocity(bone);
								}
							}
						}
						ImGui::SameLine();
						if (ImGui::Button("Freeze All"))
						{
							for (int i = 0; i < ragdoll->RagdollBoneCount() - 1; i++)
							{
								IPhysicsObject* bone = ragdoll->GetElement(i);
								if (bone)
								{
									PhysForceClearVelocity(bone);
									bone->Sleep();
									bone->EnableMotion(false);
									PhysForceClearVelocity(bone);
								}
							}
						}
						ImGui::SameLine();
						if (ImGui::Button("Unfreeze All"))
						{
							for (int i = 0; i < ragdoll->RagdollBoneCount() - 1; i++)
							{
								IPhysicsObject* bone = ragdoll->GetElement(i);
								if (bone)
								{
									PhysForceClearVelocity(bone);
									bone->EnableMotion(true);
									bone->Wake();
									PhysForceClearVelocity(bone);
								}
							}
						}
					}
				}
			}
			if (ImGui::Button("Remove Dag"))
			{
				g_DirectorsCutSystem.dags.Remove(g_DirectorsCutSystem.elementIndex);
				g_DirectorsCutSystem.elementIndex = g_DirectorsCutSystem.dags.Count() - 1;
				g_DirectorsCutSystem.boneIndex = -1;
				pEntity->Remove();
			}
			if (g_DirectorsCutSystem.elementIndex > -1)
			{
				// Retry for pEntity pointer in case we just removed it
				pEntity = g_DirectorsCutSystem.dags[g_DirectorsCutSystem.elementIndex];
				if (pEntity)
				{
					ImGuizmo::SetID(g_DirectorsCutSystem.elementIndex);

					float modelMatrix[16];
					Vector pos = pEntity->GetAbsOrigin();
					QAngle ang = pEntity->GetAbsAngles();
					float s = pEntity->GetModelScale();
					if (pEntity->IsRagdoll())
						pEntity->m_pRagdoll->GetElement(g_DirectorsCutSystem.boneIndex)->GetPosition(&pos, &ang);

					static float translation[3], rotation[3], scale[3];

					// Add offset (pivot) to translation
					pos -= g_DirectorsCutSystem.pivot;

					// Y up to Z up - pos is Z up, translation is Y up
					translation[0] = -pos.y;
					translation[1] = pos.z;
					translation[2] = -pos.x;

					rotation[0] = -ang.x;
					rotation[1] = ang.y;
					rotation[2] = -ang.z;

					// TODO: proper?
					scale[0] = s;
					scale[1] = s;
					scale[2] = s;

					ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, modelMatrix);
					//ImGui::SliderFloat3("Sc", scale, 1.0, 1000.0f, "%.0f");
					ImGuizmo::Manipulate(g_DirectorsCutSystem.cameraView, g_DirectorsCutSystem.cameraProjection, mCurrentGizmoOperation, ImGuizmo::WORLD, modelMatrix, NULL, (g_DirectorsCutSystem.useSnap ? g_DirectorsCutSystem.snap : NULL), NULL, NULL);
					ImGuizmo::DecomposeMatrixToComponents(modelMatrix, translation, rotation, scale);
					// Revert Z up
					float translation2[3];
					translation2[0] = -translation[2];
					translation2[1] = -translation[0];
					translation2[2] = translation[1];
					float rotation2[3];
					rotation2[0] = -rotation[0];
					rotation2[1] = rotation[1];
					rotation2[2] = -rotation[2];
					// Remove offset (pivot) from translation
					translation2[0] += g_DirectorsCutSystem.pivot.x;
					translation2[1] += g_DirectorsCutSystem.pivot.y;
					translation2[2] += g_DirectorsCutSystem.pivot.z;
					// Show inputs
					bool useTr = ImGui::InputFloat3("Origin", translation2);
					bool useRt = ImGui::InputFloat3("Angles", rotation2);
					if (ImGuizmo::IsUsing() || useTr || useRt)
					{
						// Create source vars from new values
						Vector newPos(translation2[0], translation2[1], translation2[2]);
						QAngle newAng = QAngle(rotation2[0], rotation2[1], rotation2[2]);
						if (pEntity->IsRagdoll())
						{
							IPhysicsObject* bone = pEntity->m_pRagdoll->GetElement(g_DirectorsCutSystem.boneIndex);
							bone->Wake();
							bone->SetPosition(newPos, newAng, true);
							PhysForceClearVelocity(bone);
						}
						else
						{
							pEntity->SetAbsOrigin(newPos);
							pEntity->SetAbsAngles(newAng);
						}
					}
				}
			}
		}
		else
		{
			ImGui::Text("No dags found");
		}

		ImGui::End();

		ImGuizmo::SetOrthographic(g_DirectorsCutSystem.orthographic);

		ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
		ImGuizmo::SetRect(0, 0, windowWidth, windowHeight);

		if (g_DirectorsCutSystem.drawGrid)
			ImGuizmo::DrawGrid(g_DirectorsCutSystem.cameraView, g_DirectorsCutSystem.cameraProjection, g_DirectorsCutSystem.identityMatrix, 500.f);

		ImGuizmo::ViewManipulate(g_DirectorsCutSystem.cameraView, g_DirectorsCutSystem.distance, ImVec2(windowWidth - 192, 0), ImVec2(192, 192), 0x10101010);

		ImGui::EndFrame();

		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}
	trampEndScene(p_pDevice);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(const HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
	if (cvar_imgui_enabled.GetBool() && cvar_imgui_input_enabled.GetBool())
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
	}
	return CallWindowProc(ogWndProc, hWnd, msg, wParam, lParam);
}

void CDirectorsCutSystem::SetupEngineView(Vector& origin, QAngle& angles, float& fov)
{
	if (levelInit && cvar_imgui_input_enabled.GetBool())
	{
		fov = this->fov;
		matrix_t_dx cameraDxMatrix = *(matrix_t_dx*)cameraView;
		matrix3x4_t cameraVMatrix;

		// Convert ImGuizmo matrix to matrix3x4_t
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				cameraVMatrix[i][j] = cameraDxMatrix.m[i][j];
			}
		}

		// Apply matrix to angles
		MatrixTranspose(cameraVMatrix, cameraVMatrix);
		MatrixAngles(cameraVMatrix, engineAngles);

		// TODO: Fix bad matrix angles
		if (engineAngles.y == 180)
		{
			angles.x = engineAngles.z;
			angles.y = -engineAngles.x;
			angles.z = engineAngles.y;
		}
		else
		{
			angles.x = -engineAngles.z;
			angles.y = engineAngles.x;
			angles.z = -engineAngles.y;
		}

		// Rotate origin around pivot point with distance
		Vector pivot = this->pivot;
		float distance = this->distance;
		Vector forward, right, up;
		AngleVectors(angles, &forward, &right, &up);
		engineOrigin = pivot - (forward * distance);
		origin = engineOrigin;
	}
}

void CDirectorsCutSystem::PostInit()
{
	float newCameraMatrix[16] =
	{ 1.f, 0.f, 0.f, 0.f,
	  0.f, 1.f, 0.f, 0.f,
	  0.f, 0.f, 1.f, 0.f,
	  0.f, 0.f, 0.f, 1.f };
	for (int i = 0; i < 16; i++)
	{
		cameraView[i] = newCameraMatrix[i];
	}
	float newIdentityMatrix[16] =
	{ 1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f };
	for (int i = 0; i < 16; i++)
	{
		identityMatrix[i] = newIdentityMatrix[i];
	}
	float newSnap[3] = { 1.f, 1.f, 1.f };
	for (int i = 0; i < 3; i++)
	{
		snap[i] = newSnap[i];
	}
	// Hook Direct3D in hl2.exe
	Msg("Director's Cut: Hooking Direct3D...\n");
	HWND hwnd = FindWindowA("Valve001", NULL);
	CDirect3DHook direct3DHook = GetDirect3DHook();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	imguiActive = true;
	char* endSceneChar = reinterpret_cast<char*>(&EndScene);
	int hooked = direct3DHook.Hook(hwnd, endSceneChar);
	switch (hooked)
	{
		case 0: // No error
			ogWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
			Msg("Director's Cut: System loaded\n");
			break;
		case 1: // Already hooked
			Msg("Director's Cut: System already hooked\n");
			break;
		case 2: // Unable to hook endscene
			Msg("Director's Cut: Unable to hook endscene\n");
			break;
		case 4: // Invalid HWND
			Msg("Director's Cut: System failed to load - Invalid HWND\n");
			break;
		case 5: // Invalid device table
			Msg("Director's Cut: System failed to load - Invalid device table\n");
			break;
		case 6: // Could not find Direct3D system
			Msg("Director's Cut: System failed to load - Could not find Direct3D system\n");
			break;
		case 7: // Failed to create Direct3D device
			Msg("Director's Cut: System failed to load - Failed to create Direct3D device\n");
			break;
		case 3: // Future error
		default: // Unknown error
			Msg("Director's Cut: System failed to load - Unknown error\n");
			break;
	}
	// Unload Dear ImGui on nonzero
	if (hooked != 0)
	{
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		imguiActive = false;
	}
}

void CDirectorsCutSystem::Shutdown()
{
	if (imguiActive)
	{
		// Unload Dear ImGui
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		imguiActive = false;
	}
	// Unhook
	CDirect3DHook direct3DHook = GetDirect3DHook();
	direct3DHook.Unhook();
}

void CDirectorsCutSystem::LevelInitPostEntity()
{
	levelInit = true;
}

void CDirectorsCutSystem::LevelShutdownPostEntity()
{
	levelInit = false;
	g_DirectorsCutSystem.dags.PurgeAndDeleteElements();
}

void CDirectorsCutSystem::Update(float frametime)
{
	bool newState = (cvar_imgui_enabled.GetBool() && cvar_imgui_input_enabled.GetBool());
	if (cursorState != newState)
	{
		cursorState = newState;
		if (newState)
		{
			vgui::surface()->SetCursorAlwaysVisible(true);
			vgui::surface()->UnlockCursor();
		}
		else
		{
			vgui::surface()->SetCursorAlwaysVisible(false);
			vgui::surface()->LockCursor();
			int w, h;
			engine->GetScreenSize(w, h);
			int x = w >> 1;
			int y = h >> 1;
			vgui::surface()->SurfaceSetCursorPos(x, y);
		}
	}
	if (::input->CAM_IsOrthographic() != orthographic)
	{
		if (orthographic)
		{
			int windowWidth;
			int windowHeight;
			vgui::surface()->GetScreenSize(windowWidth, windowHeight);
			::input->CAM_SetOrthographicSize(windowWidth, windowHeight);
			::input->CAM_ToOrthographic();
		}
		else
		{
			::input->CAM_ToPerspective();
		}
	}
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer)
	{
		clienttools->GetLocalPlayerEyePosition(g_DirectorsCutSystem.playerOrigin, g_DirectorsCutSystem.playerAngles, g_DirectorsCutSystem.playerFov);
	}
}

C_BaseAnimating* CreateDag(char* modelName, bool add, bool setIndex)
{
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
		return nullptr;
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
	C_BaseAnimating* pEntity = new C_BaseAnimating();
	if (!pEntity)
		return nullptr;
	pEntity->SetModel(modelName);
	pEntity->SetModelName(modelName);
	
	// Spawn entity
	RenderGroup_t renderGroup = RENDER_GROUP_OPAQUE_ENTITY;
	if (!pEntity->InitializeAsClientEntity(modelName, renderGroup))
	{
		Msg("Director's Cut: Failed to spawn entity %s\n", modelName);
		pEntity->Release();
		return nullptr;
	}

	// Add entity to list
	if (add)
	{
		g_DirectorsCutSystem.dags.AddToTail(pEntity);
		if(setIndex)
			g_DirectorsCutSystem.elementIndex = g_DirectorsCutSystem.dags.Count() - 1;
	}
	Msg("Director's Cut: Created dag with model name %s and render group %d\n", modelName, renderGroup);
	return pEntity;
}

C_BaseAnimating* MakeRagdoll(C_BaseAnimating* dag)
{
	C_BaseAnimating* ragdoll = dag->BecomeRagdollOnClient();
	if (ragdoll)
	{
		// Freeze all bones
		CRagdoll* cRagdoll = ragdoll->m_pRagdoll;
		if (cRagdoll)
		{
			for (int i = 0; i < cRagdoll->RagdollBoneCount() - 1; i++)
			{
				IPhysicsObject* bone = cRagdoll->GetElement(i);
				if (bone)
				{
					PhysForceClearVelocity(bone);
					bone->Sleep();
					bone->EnableMotion(false);
					PhysForceClearVelocity(bone);
				}
			}
		}
		return ragdoll;
	}
	return nullptr;
}

C_BaseAnimating* MakeRagdoll(C_BaseAnimating* dag, bool add, bool setIndex)
{
	C_BaseAnimating* ragdoll = MakeRagdoll(dag);
	if (ragdoll)
	{
		// Add entity to list
		if (add)
		{
			g_DirectorsCutSystem.dags.AddToTail(ragdoll);
			if (setIndex)
			{
				g_DirectorsCutSystem.elementIndex = g_DirectorsCutSystem.dags.Count() - 1;
				g_DirectorsCutSystem.ragdollIndex = g_DirectorsCutSystem.dags.Count() - 1;
				g_DirectorsCutSystem.boneIndex = 0;
			}
		}
		return ragdoll;
	}
	return nullptr;
}

C_BaseAnimating* CreateRagdoll(char* modelName, bool add, bool setIndex)
{
	// Init ragdoll
	C_BaseAnimating* pEntity = CreateDag(modelName, false, false);
	if (pEntity)
	{
		C_BaseAnimating* ragdoll = MakeRagdoll(pEntity, add, setIndex);
		if (ragdoll)
		{
			return ragdoll;
		}
	}
	return nullptr;
}

void CDirectorsCutSystem::Frustum(float left, float right, float bottom, float top, float znear, float zfar, float* m16)
{
	float temp, temp2, temp3, temp4;
	temp = 2.0f * znear;
	temp2 = right - left;
	temp3 = top - bottom;
	temp4 = zfar - znear;
	m16[0] = temp / temp2;
	m16[1] = 0.0;
	m16[2] = 0.0;
	m16[3] = 0.0;
	m16[4] = 0.0;
	m16[5] = temp / temp3;
	m16[6] = 0.0;
	m16[7] = 0.0;
	m16[8] = (right + left) / temp2;
	m16[9] = (top + bottom) / temp3;
	m16[10] = (-zfar - znear) / temp4;
	m16[11] = -1.0f;
	m16[12] = 0.0;
	m16[13] = 0.0;
	m16[14] = (-temp * zfar) / temp4;
	m16[15] = 0.0;
}

void CDirectorsCutSystem::OrthoGraphic(const float l, float r, float b, const float t, float zn, const float zf, float* m16)
{
	m16[0] = 2 / (r - l);
	m16[1] = 0.0f;
	m16[2] = 0.0f;
	m16[3] = 0.0f;
	m16[4] = 0.0f;
	m16[5] = 2 / (t - b);
	m16[6] = 0.0f;
	m16[7] = 0.0f;
	m16[8] = 0.0f;
	m16[9] = 0.0f;
	m16[10] = 1.0f / (zf - zn);
	m16[11] = 0.0f;
	m16[12] = (l + r) / (l - r);
	m16[13] = (t + b) / (b - t);
	m16[14] = zn / (zn - zf);
	m16[15] = 1.0f;
}

void CDirectorsCutSystem::Perspective(float fovyInDegrees, float aspectRatio, float znear, float zfar, float* m16)
{
	float ymax, xmax;
	ymax = znear * tanf(fovyInDegrees * 3.141592f / 180.0f);
	xmax = ymax * aspectRatio;
	this->Frustum(-xmax, xmax, -ymax, ymax, znear, zfar, m16);
}

void Cross(const float* a, const float* b, float* r)
{
	r[0] = a[1] * b[2] - a[2] * b[1];
	r[1] = a[2] * b[0] - a[0] * b[2];
	r[2] = a[0] * b[1] - a[1] * b[0];
}

float Dot(const float* a, const float* b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void Normalize(const float* a, float* r)
{
	float il = 1.f / (sqrtf(Dot(a, a)) + FLT_EPSILON);
	r[0] = a[0] * il;
	r[1] = a[1] * il;
	r[2] = a[2] * il;
}

void CDirectorsCutSystem::LookAt(const float* eye, const float* at, const float* up, float* m16)
{
	float X[3], Y[3], Z[3], tmp[3];

	tmp[0] = eye[0] - at[0];
	tmp[1] = eye[1] - at[1];
	tmp[2] = eye[2] - at[2];
	Normalize(tmp, Z);
	Normalize(up, Y);

	Cross(Y, Z, tmp);
	Normalize(tmp, X);

	Cross(Z, X, tmp);
	Normalize(tmp, Y);

	m16[0] = X[0];
	m16[1] = Y[0];
	m16[2] = Z[0];
	m16[3] = 0.0f;
	m16[4] = X[1];
	m16[5] = Y[1];
	m16[6] = Z[1];
	m16[7] = 0.0f;
	m16[8] = X[2];
	m16[9] = Y[2];
	m16[10] = Z[2];
	m16[11] = 0.0f;
	m16[12] = -Dot(X, eye);
	m16[13] = -Dot(Y, eye);
	m16[14] = -Dot(Z, eye);
	m16[15] = 1.0f;
}

