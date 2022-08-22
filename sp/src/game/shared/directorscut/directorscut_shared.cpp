//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "igamesystem.h"

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
bool hooked = false;

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

		// Drawing
		ImGui::ShowDemoWindow();

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

class CDirectorsCutSystem : public CAutoGameSystemPerFrame
{
public:
	DECLARE_DATADESC();

	CDirectorsCutSystem() : CAutoGameSystemPerFrame("CDirectorsCutSystem")
	{
	}
	
	virtual bool InitAllSystems()
	{
		return true;
	}

	virtual void LevelInitPreEntity()
	{
#ifdef CLIENT_DLL
		if (hooked)
			return;
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
		hooked = true;
#endif
	}

	virtual void Update(float frametime)
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
	char* d3d9DeviceTable[119];

	bool GetD3D9Device()
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

	const char* REL_JMP = "\xE9";
	const char* NOP = "\x90";
	
	// 1 byte instruction +  4 bytes address
	const unsigned int SIZE_OF_REL_JMP = 5;

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
};

CDirectorsCutSystem	g_DirectorsCutSystem;

BEGIN_DATADESC_NO_BASE(CDirectorsCutSystem)
END_DATADESC()
