//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "directorscut_shared.h"

#ifdef CLIENT_DLL
#include "imgui.h"

#undef RegSetValueEx
#undef RegSetValue
#undef RegQueryValueEx
#undef RegQueryValue
#undef RegOpenKeyEx
#undef RegOpenKey
#undef RegCreateKeyEx
#undef RegCreateKey
#undef ReadConsoleInput
#undef INVALID_HANDLE_VALUE
#undef GetCommandLine

#include "imguizmo/imguizmo.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <string>

#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Panel.h>
#include <vgui/IPanel.h>
#include "vgui_int.h"
#include "input.h"
#include "iinput.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CDirectorsCutSystem g_DirectorsCutSystem;

CDirectorsCutSystem &DirectorsCutGameSystem()
{
	return g_DirectorsCutSystem;
}

#ifdef CLIENT_DLL
typedef HRESULT(APIENTRY* endSceneFunc)(LPDIRECT3DDEVICE9 pDevice);
typedef HRESULT(APIENTRY* wndProcFunc)(const HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam);

endSceneFunc trampEndScene = nullptr;

char* ogEndSceneAddress;
unsigned char endSceneBytes[7];

WNDPROC ogWndProc = nullptr;

HWND hwnd = nullptr;
LPDIRECT3DDEVICE9 d3dDevice = NULL;

ConVar cvar_imgui_enabled("dx_imgui_enabled", "1", FCVAR_ARCHIVE, "Enable ImGui");
ConVar cvar_imgui_input_enabled("dx_imgui_input_enabled", "1", FCVAR_ARCHIVE, "Capture ImGui input");

ConCommand cmd_imgui_toggle("dx_imgui_toggle", [](const CCommand& args) {
	cvar_imgui_enabled.SetValue(!cvar_imgui_enabled.GetBool());
}, "Toggle ImGui", FCVAR_ARCHIVE);

ConCommand cmd_imgui_input_toggle("dx_imgui_input_toggle", [](const CCommand& args) {
	cvar_imgui_input_enabled.SetValue(!cvar_imgui_input_enabled.GetBool());
}, "Toggle ImGui input", FCVAR_ARCHIVE);

bool cursorState = false;

#endif

//static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);

#ifdef CLIENT_DLL

void APIENTRY EndScene(LPDIRECT3DDEVICE9 p_pDevice)
{
	if (!d3dDevice)
	{
		d3dDevice = p_pDevice;
		ImGui_ImplDX9_Init(p_pDevice);
	}

	if (cvar_imgui_enabled.GetBool())
	{
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		// Drawing
		ImGui::ShowDemoWindow();

		ImGui::Begin("Camera");

		ImGui::SliderFloat("Distance", &g_DirectorsCutSystem.distance, 0.0f, 1000.0f);

		ImGui::End();

		ImGuiIO& io = ImGui::GetIO(); (void)io;

		int windowWidth;
		int windowHeight;
		vgui::surface()->GetScreenSize(windowWidth, windowHeight);
		
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

		ImGuizmo::ViewManipulate(g_DirectorsCutSystem.cameraView, g_DirectorsCutSystem.distance, ImVec2(windowWidth - 192, 0), ImVec2(192, 192), 0x10101010);

		ImGui::EndFrame();

		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}
	
	trampEndScene(d3dDevice);
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

#endif

void CDirectorsCutSystem::Update(float frametime)
{
#ifdef CLIENT_DLL
	// TODO: Figure out if this is still necessary
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
#endif
}

#ifdef CLIENT_DLL

void FPU_MatrixF_x_MatrixF(const float* a, const float* b, float* r)
{
	r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
	r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
	r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
	r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

	r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
	r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
	r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
	r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

	r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
	r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
	r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
	r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

	r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
	r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
	r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
	r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
}

struct matrix_t_dx;

struct vec_t_dx
{
public:
	float x, y, z, w;

	void Lerp(const vec_t_dx& v, float t)
	{
		x += (v.x - x) * t;
		y += (v.y - y) * t;
		z += (v.z - z) * t;
		w += (v.w - w) * t;
	}

	void Set(float v) { x = y = z = w = v; }
	void Set(float _x, float _y, float _z = 0.f, float _w = 0.f) { x = _x; y = _y; z = _z; w = _w; }

	vec_t_dx& operator -= (const vec_t_dx& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	vec_t_dx& operator += (const vec_t_dx& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	vec_t_dx& operator *= (const vec_t_dx& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
	vec_t_dx& operator *= (float v) { x *= v;    y *= v;    z *= v;    w *= v;    return *this; }

	vec_t_dx operator * (float f) const;
	vec_t_dx operator - () const;
	vec_t_dx operator - (const vec_t_dx& v) const;
	vec_t_dx operator + (const vec_t_dx& v) const;
	vec_t_dx operator * (const vec_t_dx& v) const;

	const vec_t_dx& operator + () const { return (*this); }
	float Length() const { return sqrtf(x * x + y * y + z * z); };
	float LengthSq() const { return (x * x + y * y + z * z); };
	vec_t_dx Normalize() { (*this) *= (1.f / (Length() > FLT_EPSILON ? Length() : FLT_EPSILON)); return (*this); }
	vec_t_dx Normalize(const vec_t_dx& v) { this->Set(v.x, v.y, v.z, v.w); this->Normalize(); return (*this); }
	vec_t_dx Abs() const;

	void Cross(const vec_t_dx& v)
	{
		vec_t_dx res;
		res.x = y * v.z - z * v.y;
		res.y = z * v.x - x * v.z;
		res.z = x * v.y - y * v.x;

		x = res.x;
		y = res.y;
		z = res.z;
		w = 0.f;
	}

	void Cross(const vec_t_dx& v1, const vec_t_dx& v2)
	{
		x = v1.y * v2.z - v1.z * v2.y;
		y = v1.z * v2.x - v1.x * v2.z;
		z = v1.x * v2.y - v1.y * v2.x;
		w = 0.f;
	}

	float Dot(const vec_t_dx& v) const
	{
		return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w);
	}

	float Dot3(const vec_t_dx& v) const
	{
		return (x * v.x) + (y * v.y) + (z * v.z);
	}

	void Transform(const matrix_t_dx& matrix);
	void Transform(const vec_t_dx& s, const matrix_t_dx& matrix);

	void TransformVector(const matrix_t_dx& matrix);
	void TransformPoint(const matrix_t_dx& matrix);
	void TransformVector(const vec_t_dx& v, const matrix_t_dx& matrix) { (*this) = v; this->TransformVector(matrix); }
	void TransformPoint(const vec_t_dx& v, const matrix_t_dx& matrix) { (*this) = v; this->TransformPoint(matrix); }

	float& operator [] (size_t index) { return ((float*)&x)[index]; }
	const float& operator [] (size_t index) const { return ((float*)&x)[index]; }
	bool operator!=(const vec_t_dx& other) const { return memcmp(this, &other, sizeof(vec_t_dx)) != 0; }
};

vec_t_dx makeVect(float _x, float _y, float _z = 0.f, float _w = 0.f) { vec_t_dx res; res.x = _x; res.y = _y; res.z = _z; res.w = _w; return res; }

struct matrix_t_dx
{
public:

	union
	{
		float m[4][4];
		float m16[16];
		struct
		{
			vec_t_dx right, up, dir, position;
		} v;
		vec_t_dx component[4];
	};

	operator float* () { return m16; }
	operator const float* () const { return m16; }
	void Translation(float _x, float _y, float _z) { this->Translation(makeVect(_x, _y, _z)); }

	void Translation(const vec_t_dx& vt)
	{
		v.right.Set(1.f, 0.f, 0.f, 0.f);
		v.up.Set(0.f, 1.f, 0.f, 0.f);
		v.dir.Set(0.f, 0.f, 1.f, 0.f);
		v.position.Set(vt.x, vt.y, vt.z, 1.f);
	}

	void Scale(float _x, float _y, float _z)
	{
		v.right.Set(_x, 0.f, 0.f, 0.f);
		v.up.Set(0.f, _y, 0.f, 0.f);
		v.dir.Set(0.f, 0.f, _z, 0.f);
		v.position.Set(0.f, 0.f, 0.f, 1.f);
	}
	void Scale(const vec_t_dx& s) { Scale(s.x, s.y, s.z); }

	matrix_t_dx& operator *= (const matrix_t_dx& mat)
	{
		matrix_t_dx tmpMat;
		tmpMat = *this;
		tmpMat.Multiply(mat);
		*this = tmpMat;
		return *this;
	}
	matrix_t_dx operator * (const matrix_t_dx& mat) const
	{
		matrix_t_dx matT;
		matT.Multiply(*this, mat);
		return matT;
	}

	void Multiply(const matrix_t_dx& matrix)
	{
		matrix_t_dx tmp;
		tmp = *this;

		FPU_MatrixF_x_MatrixF((float*)&tmp, (float*)&matrix, (float*)this);
	}

	void Multiply(const matrix_t_dx& m1, const matrix_t_dx& m2)
	{
		FPU_MatrixF_x_MatrixF((float*)&m1, (float*)&m2, (float*)this);
	}

	float GetDeterminant() const
	{
		return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[0][2] * m[1][0] * m[2][1] -
			m[0][2] * m[1][1] * m[2][0] - m[0][1] * m[1][0] * m[2][2] - m[0][0] * m[1][2] * m[2][1];
	}

	float Inverse(const matrix_t_dx& srcMatrix, bool affine = false);
	void SetToIdentity()
	{
		v.right.Set(1.f, 0.f, 0.f, 0.f);
		v.up.Set(0.f, 1.f, 0.f, 0.f);
		v.dir.Set(0.f, 0.f, 1.f, 0.f);
		v.position.Set(0.f, 0.f, 0.f, 1.f);
	}
	void Transpose()
	{
		matrix_t_dx tmpm;
		for (int l = 0; l < 4; l++)
		{
			for (int c = 0; c < 4; c++)
			{
				tmpm.m[l][c] = m[c][l];
			}
		}
		(*this) = tmpm;
	}

	void RotationAxis(const vec_t_dx& axis, float angle);

	void OrthoNormalize()
	{
		v.right.Normalize();
		v.up.Normalize();
		v.dir.Normalize();
	}
};

void CDirectorsCutSystem::SetupEngineView(Vector& origin, QAngle& angles, float& fov)
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
	QAngle engineAngles;
	MatrixTranspose(cameraVMatrix, cameraVMatrix);
	MatrixAngles(cameraVMatrix, engineAngles);

	// TODO: Fix bad matrix angles, need these to inverse themselves
	angles.x = -engineAngles.z;
	angles.y = engineAngles.x;
	angles.z = -engineAngles.y;

	// Rotate origin around pivot point with distance
	Vector pivot = this->pivot;
	float distance = this->distance;
	Vector forward, right, up;
	AngleVectors(angles, &forward, &right, &up);
	origin = pivot - (forward * distance);
}

const char* REL_JMP = "\xE9";
const char* NOP = "\x90";

// 1 byte instruction +  4 bytes address
const unsigned int SIZE_OF_REL_JMP = 5;

bool CDirectorsCutSystem::GetD3D9Device()
{
	if (!hwnd || !this->d3d9DeviceTable) {
		return false;
	}

	IDirect3D9* d3dSys = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3dSys) {
		return false;
	}

	IDirect3DDevice9* dummyDev = NULL;

	// options to create dummy device
	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.Windowed = false;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hwnd;

	HRESULT dummyDeviceCreated = d3dSys->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &dummyDev);

	if (dummyDeviceCreated != S_OK)
	{
		// may fail in windowed fullscreen mode, trying again with windowed mode
		d3dpp.Windowed = ~d3dpp.Windowed;

		dummyDeviceCreated = d3dSys->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &dummyDev);

		if (dummyDeviceCreated != S_OK)
		{
			d3dSys->Release();
			return false;
		}
	}

	memcpy(this->d3d9DeviceTable, *reinterpret_cast<void***>(dummyDev), sizeof(this->d3d9DeviceTable));

	dummyDev->Release();
	d3dSys->Release();
	return true;
}

// adapted from https://guidedhacking.com/threads/simple-x86-c-trampoline-hook.14188/
// hookedFn: The function that's about to the hooked
// hookFn: The function that will be executed before `hookedFn` by causing `hookFn` to take a detour
void* WINAPI hookFn(char* hookedFn, char* hookFn, int copyBytesSize, unsigned char* backupBytes, std::string descr)
{

	if (copyBytesSize < 5)
	{
		// the function prologue of the hooked function
		// should be of size 5 (or larger)
		return nullptr;
	}

	//
	// 1. Backup the original function prologue
	//
	if (!ReadProcessMemory(GetCurrentProcess(), hookedFn, backupBytes, copyBytesSize, 0))
	{
		return nullptr;
	}

	//
	// 2. Setup the trampoline
	// --> Cause `hookedFn` to return to `hookFn` without causing an infinite loop
	// Otherwise calling `hookedFn` directly again would then call `hookFn` again, and so on :)
	//
	// allocate executable memory for the trampoline
	// the size is (amount of bytes copied from the original function) + (size of a relative jump + address)

	char* trampoline = (char*)VirtualAlloc(0, copyBytesSize + SIZE_OF_REL_JMP, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	// steal the first `copyBytesSize` bytes from the original function
	// these will be used to make the trampoline work
	// --> jump back to `hookedFn` without executing `hookFn` again
	memcpy(trampoline, hookedFn, copyBytesSize);
	// append the relative JMP instruction after the stolen instructions
	memcpy(trampoline + copyBytesSize, REL_JMP, sizeof(REL_JMP));

	// calculate the offset between the hooked function and the trampoline
	// --> distance between the trampoline and the original function `hookedFn`
	// this will land directly *after* the inserted JMP instruction, hence subtracting 5
	int hookedFnTrampolineOffset = hookedFn - trampoline - SIZE_OF_REL_JMP;
	memcpy(trampoline + copyBytesSize + 1, &hookedFnTrampolineOffset, sizeof(hookedFnTrampolineOffset));

	//
	// 3. Detour the original function `hookedFn`
	// --> cause `hookedFn` to execute `hookFn` first
	// remap the first few bytes of the original function as RXW
	DWORD oldProtect;
	if (!VirtualProtect(hookedFn, copyBytesSize, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		return nullptr;
	}

	// best variable name ever
	// calculate the size of the relative jump between the start of `hookedFn` and the start of `hookFn`.
	int hookedFnHookFnOffset = hookFn - hookedFn - SIZE_OF_REL_JMP;

	// Take a relative jump to `hookFn` at the beginning
	// of course, `hookFn` has to expect the same parameter types and values
	memcpy(hookedFn, REL_JMP, sizeof(REL_JMP));
	memcpy(hookedFn + 1, &hookedFnHookFnOffset, sizeof(hookedFnHookFnOffset));

	// restore the previous protection values
	VirtualProtect(hookedFn, copyBytesSize, oldProtect, &oldProtect);

	return trampoline;
}

void CDirectorsCutSystem::LevelInitPreEntity()
{
#ifdef CLIENT_DLL
	float newCameraMatrix[16] =
	{ 1.f, 0.f, 0.f, 0.f,
	  0.f, 1.f, 0.f, 0.f,
	  0.f, 0.f, 1.f, 0.f,
	  0.f, 0.f, 0.f, 1.f };

	for (int i = 0; i < 16; i++)
	{
		cameraView[i] = newCameraMatrix[i];
	}

	// Configure Dear Imgui
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	hwnd = GetForegroundWindow();
	ImGui_ImplWin32_Init(hwnd);
	bool deviceGot = GetD3D9Device();
	if (deviceGot)
	{
		ogEndSceneAddress = d3d9DeviceTable[42];
		trampEndScene = (endSceneFunc)hookFn(ogEndSceneAddress, reinterpret_cast<char*>(&EndScene), 7, endSceneBytes, "EndScene");

		ogWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
	}
	CGMsg(0, CON_GROUP_MAPBASE_MISC, "Director's Cut system loaded\n");
#endif
}

// Unhook le method
BOOL WINAPI restore(char* fn, unsigned char* writeBytes, int writeSize, std::string descr)
{
	DWORD oldProtect;
	if (!VirtualProtect(fn, writeSize, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		return FALSE;
	}

	if (!WriteProcessMemory(GetCurrentProcess(), fn, writeBytes, writeSize, 0))
	{
		return FALSE;
	}

	if (!VirtualProtect(fn, writeSize, oldProtect, &oldProtect))
	{
		return FALSE;
	}

	return TRUE;
}
	
#endif
