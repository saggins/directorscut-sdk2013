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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CreateTestDag(const CCommand& args);

CDirectorsCutSystem g_DirectorsCutSystem;

CDirectorsCutSystem &DirectorsCutGameSystem()
{
	return g_DirectorsCutSystem;
}

ConVar cvar_imgui_enabled("dx_imgui_enabled", "1", FCVAR_ARCHIVE, "Enable ImGui");
ConVar cvar_imgui_input_enabled("dx_imgui_input_enabled", "1", FCVAR_ARCHIVE, "Capture ImGui input");
ConCommand cmd_imgui_toggle("dx_imgui_toggle", [](const CCommand& args) {
	cvar_imgui_enabled.SetValue(!cvar_imgui_enabled.GetBool());
}, "Toggle ImGui", FCVAR_ARCHIVE);
ConCommand cmd_imgui_input_toggle("dx_imgui_input_toggle", [](const CCommand& args) {
	cvar_imgui_input_enabled.SetValue(!cvar_imgui_input_enabled.GetBool());
}, "Toggle ImGui input", FCVAR_ARCHIVE);
ConCommand cmd_test_entity("dx_test_entity", &CreateTestDag, "Create a test entity", FCVAR_ARCHIVE);

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

		if (g_DirectorsCutSystem.elementIndex > -1 && ImGui::Button("Selected Dag to Pivot"))
		{
			C_BaseAnimating* element = g_DirectorsCutSystem.dags[g_DirectorsCutSystem.elementIndex];
			if (element)
			{
				g_DirectorsCutSystem.pivot = element->GetAbsOrigin();
			}
		}
		
		bool viewDirty = false;

		ImGui::SliderFloat("Field of View", &g_DirectorsCutSystem.fov, 1, 179.0f, "%.3f");
		viewDirty |= ImGui::SliderFloat("Distance", &g_DirectorsCutSystem.distance, 1, 1000.0f, "%.0f");

		if (viewDirty || firstFrame)
		{
			float eye[] = { cosf(g_DirectorsCutSystem.camYAngle) * cosf(g_DirectorsCutSystem.camXAngle) * g_DirectorsCutSystem.distance, sinf(g_DirectorsCutSystem.camXAngle) * g_DirectorsCutSystem.distance, sinf(g_DirectorsCutSystem.camYAngle) * cosf(g_DirectorsCutSystem.camXAngle) * g_DirectorsCutSystem.distance };
			float at[] = { 0.f, 0.f, 0.f };
			float up[] = { 0.f, 1.f, 0.f };
			g_DirectorsCutSystem.LookAt(eye, at, up, g_DirectorsCutSystem.cameraView);
			firstFrame = false;	
		}

		ImGui::Checkbox("Draw Grid", &g_DirectorsCutSystem.drawGrid);
		ImGui::Checkbox("Orthographic", &g_DirectorsCutSystem.orthographic);
		ImGui::Checkbox("Use Snapping", &g_DirectorsCutSystem.useSnap);

		if (g_DirectorsCutSystem.useSnap)
		{
			ImGui::InputFloat3("Snap", &g_DirectorsCutSystem.snap);
		}

		ImGui::End();

		ImGui::Begin("Dags", 0, ImGuiWindowFlags_AlwaysAutoResize);

		if (g_DirectorsCutSystem.dags.Count() > 0)
		{
			ImGui::Text("Dags: %d", g_DirectorsCutSystem.dags.Count());
			if (g_DirectorsCutSystem.dags.Count() > 0)
			{
				ImGui::SliderInt("Dag Index", &g_DirectorsCutSystem.elementIndex, 0, g_DirectorsCutSystem.dags.Count() - 1);
			}
		}
		else
		{
			ImGui::Text("No dags found");
		}

		ImGuizmo::SetOrthographic(g_DirectorsCutSystem.orthographic);
		
		ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
		ImGuizmo::SetRect(0, 0, windowWidth, windowHeight);

		if(g_DirectorsCutSystem.drawGrid)
			ImGuizmo::DrawGrid(g_DirectorsCutSystem.cameraView, g_DirectorsCutSystem.cameraProjection, g_DirectorsCutSystem.identityMatrix, 500.f);

		if (g_DirectorsCutSystem.elementIndex > -1)
		{
			C_BaseAnimating* element = g_DirectorsCutSystem.dags[g_DirectorsCutSystem.elementIndex];

			// Check if valid
			if (element)
			{
				ImGuizmo::SetID(g_DirectorsCutSystem.elementIndex);

				float modelMatrix[16];
				Vector pos = element->GetAbsOrigin();
				QAngle ang = element->GetAbsAngles();
				float s = element->GetModelScale();

				static float translation[3], rotation[3], scale[3];

				// Add offset (pivot) to translation
				pos -= g_DirectorsCutSystem.pivot;
				
				// Y up to Z up - pos is Z up, translation is Y up
				translation[0] = -pos.y;
				translation[1] = pos.z;
				translation[2] = -pos.x;

				rotation[0] = ang.x;
				rotation[1] = ang.y;
				rotation[2] = ang.z;

				// TODO: proper?
				scale[0] = s;
				scale[1] = s;
				scale[2] = s;

				ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, modelMatrix);
				//ImGui::SliderFloat3("Sc", scale, 1.0, 1000.0f, "%.0f");
				ImGuizmo::Manipulate(g_DirectorsCutSystem.cameraView, g_DirectorsCutSystem.cameraProjection, mCurrentGizmoOperation, ImGuizmo::WORLD, modelMatrix, NULL, (g_DirectorsCutSystem.useSnap ? &g_DirectorsCutSystem.snap : NULL), NULL, NULL);
				ImGuizmo::DecomposeMatrixToComponents(modelMatrix, translation, rotation, scale);
				// Revert Y up
				float translation2[3];
				translation2[0] = -translation[2];
				translation2[1] = -translation[0];
				translation2[2] = translation[1];
				// Remove offset (pivot) from translation
				translation2[0] += g_DirectorsCutSystem.pivot.x;
				translation2[1] += g_DirectorsCutSystem.pivot.y;
				translation2[2] += g_DirectorsCutSystem.pivot.z;
				// Show inputs
				bool useTr = ImGui::InputFloat3("Origin", translation2);
				bool useRt = ImGui::SliderFloat3("Angles", rotation, -180.0f, 180.0f);
				if (ImGuizmo::IsUsing() || useTr || useRt)
				{
					// Create source vars from new values
					Vector newPos(translation2[0], translation2[1], translation2[2]);
					QAngle newAng = QAngle(rotation[0], rotation[1], rotation[2]);
					element->SetAbsOrigin(newPos);
					element->SetAbsAngles(newAng);
				}
			}
		}

		ImGui::End();

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

void CreateTestDag(const CCommand& args)
{
	// Set model name
	const char* modelName = "models/props_junk/watermelon01.mdl";
	if (args.ArgC() > 1)
	{
		modelName = args.Arg(1);
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
	}
	
	// Cache model
	model_t* model = (model_t*)engine->LoadModel(modelName);
	if (!model)
	{
		Msg("Director's Cut: Failed to load model %s\n", modelName);
		return;
	}
	
	// Create test dag
	C_DagEntity* pEntity = new C_DagEntity();
	if (!pEntity)
		return;

	RenderGroup_t renderGroup = RENDER_GROUP_OPAQUE_ENTITY;
	
	// Spawn entity
	if (!pEntity->InitializeAsClientEntityByHandle(model, renderGroup))
	{
		Msg("Director's Cut: Failed to spawn entity %s\n", modelName);
		pEntity->Release();
		return;
	}

	// Add entity to list
	//clienttools->AddClientRenderable(pEntity, renderGroup);
	g_DirectorsCutSystem.dags.AddToTail(pEntity);
	g_DirectorsCutSystem.elementIndex = g_DirectorsCutSystem.dags.Count() - 1;
	Msg("Director's Cut: Created dag with model name %s and render group %d\n", modelName, renderGroup);
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

