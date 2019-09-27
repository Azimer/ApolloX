#include <windows.h>
#include "winmain.h" // Needed for Console Window Debugging
#include "videodll.h"
//#include "audiodll.h"
//#include "inputdll.h"

VIDEODLL gfxdll;
static void LoadDummyFunctions ();
static bool LoadFunctions ();

void DummyVoidVoid () {
}

// Makes all the functions callable without GPFing on us :)
void InitInternalVideo(void);

BOOL LoadVideoPlugin(char* libname) {
	PLUGIN_INFO Plugin_Info;
	ZeroMemory(&Plugin_Info, sizeof(Plugin_Info));

	if (libname == NULL || (libname[0]=='.' && libname[1]==0)) {
		InitInternalVideo();
		return TRUE;
	}
	
	if (gfxdll.hinstLibVideo != NULL) gfxdll.Close ();
	
	gfxdll.hinstLibVideo = LoadLibrary(libname);

	if (gfxdll.hinstLibVideo == NULL) {
		Debug (0, "Could not load %s because it couldn't be found!", libname);
		gfxdll.Close();
		return FALSE;
	}

	gfxdll.GetDllInfo = (void (__cdecl*)(PLUGIN_INFO *)) GetProcAddress(gfxdll.hinstLibVideo, "GetDllInfo");
	if(!gfxdll.GetDllInfo) {
		Debug (0, "%s does not conform to the plugin spec.", libname);
		return FALSE;//This isn't a plugin dll, so don't bother.	
	}
	gfxdll.GetDllInfo(&Plugin_Info);

	if(Plugin_Info.Type == PLUGIN_TYPE_GFX) {
		if(Plugin_Info.Version != 1) {
			Debug (0, "%s is not compatable with Apollo. %d != 1", libname, Plugin_Info.Version);
			gfxdll.Close();
			return FALSE;
		}
		if (LoadFunctions () == false) {
			gfxdll.Close();
			return FALSE;
		}
	} else {
		//Here we insert Audio code, controller code, etc.
		Debug (0, "%s dll isn't a gfx plugin!", libname);
		gfxdll.Close();
		return FALSE;
	}
	
	return TRUE;
}

void CloseVideoPlugin()
{
//  Debug (0, "Closed Video Plugin");
  if(gfxdll.CloseDLL)
	  gfxdll.CloseDLL();

  if(gfxdll.hinstLibVideo)
	  FreeLibrary(gfxdll.hinstLibVideo); 

  ZeroMemory (&gfxdll, sizeof(VIDEODLL));
  InitInternalVideo();
}

// I do this function in the following way to preserve my dummy functions
bool LoadFunctions () {
	FARPROC temp=NULL;
	void *temper;
	// Mandatory DLL Functions here...
	temp = GetProcAddress(gfxdll.hinstLibVideo, "CloseDLL");
		if (!temp) return false;
		gfxdll.CloseDLL		= (void (__cdecl*)(void)) temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "InitiateGFX");
		if (!temp) return false;
		gfxdll.InitiateGFX	= (BOOL (__cdecl*)(GFX_INFO)) temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "UpdateScreen");
		if (!temp) return false;
		gfxdll.UpdateScreen	= (void (__cdecl*)(void)) temp;

	// The rest of these are optional functions...
	temp = GetProcAddress(gfxdll.hinstLibVideo, "ProcessDList");
		if (temp) gfxdll.ProcessDList	= (void (__cdecl*)(void)) temp;

	temp = GetProcAddress(gfxdll.hinstLibVideo, "ChangeWindow");
		if (temp) gfxdll.ChangeWindow = (void (__cdecl*)(BOOL)) temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "DllAbout");
		if (temp) gfxdll.DllAbout		=	(void (__cdecl*)(HWND)) temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "DllConfig");
		if (temp) gfxdll.DllConfig		=	(void (__cdecl*)(HWND)) temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "DllTest");
		if (temp) gfxdll.DllTest			=	(void (__cdecl*)(HWND)) temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "DrawScreen");
		if (temp) gfxdll.DrawScreen		=	(void (__cdecl*)(void)) temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "GetDllInfo");
		if (temp) gfxdll.GetDllInfo		=	(void (__cdecl*)(PLUGIN_INFO *)) temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "MoveScreen");
		if (temp) gfxdll.MoveScreen		=	(void (__cdecl*)(int, int)) temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "ProcessRDPList");
		if (temp) gfxdll.ProcessRDPList	=	(void (__cdecl*)(void)) temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "RomOpen");
		if (temp) gfxdll.RomOpen		=	(void (__cdecl*)(void))	temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "RomClosed");
		if (temp) gfxdll.RomClosed		=	(void (__cdecl*)(void))	temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "ViStatusChanged");
		if (temp) gfxdll.ViStatusChanged=	(void (__cdecl*)(void))	temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "ViWidthChanged");
		if (temp) gfxdll.ViWidthChanged	=	(void (__cdecl*)(void))	temp;
	temp = GetProcAddress(gfxdll.hinstLibVideo, "GiveMeConsole");
		if (temp) {
			temper = (void *)Debug;
			__asm {
				mov ecx, temper;
				push ecx;
				call dword ptr [temp];
				add esp, 4;
			}
			//((void (__cdecl*)(void (__cdecl*)(int, char *)))temp)(Debug);
		}
	return true;
}


extern u8* rdram;
extern u8* idmem;
extern u8* MI;
extern u8* VI;
extern u8* SP;
extern u8* DPC;

GFX_INFO Gfx_Info; // make it stay just in case the evil plugin doesn't store a local copy

void InitGFXPlugin () {

	ZeroMemory (&Gfx_Info, sizeof(GFX_INFO));

	Gfx_Info.hWnd					= GhWnd;
	Gfx_Info.hStatusBar				= hwndStatus;
	Gfx_Info.MemoryBswaped			= TRUE;
	Gfx_Info.RDRAM					= rdram;
	Gfx_Info.DMEM					= idmem;
	Gfx_Info.IMEM					= (idmem+0x1000);
	Gfx_Info.DPC_START_REG			= (u32 *)(DPC+0x00);
	Gfx_Info.DPC_END_REG			= (u32 *)(DPC+0x04);
	Gfx_Info.DPC_CURRENT_REG		= (u32 *)(DPC+0x08);
	Gfx_Info.DPC_STATUS_REG			= (u32 *)(DPC+0x0C);
	Gfx_Info.DPC_CLOCK_REG			= (u32 *)(DPC+0x10);
	Gfx_Info.DPC_BUFBUSY_REG		= (u32 *)(DPC+0x14);
	Gfx_Info.DPC_PIPEBUSY_REG		= (u32 *)(DPC+0x18);
	Gfx_Info.DPC_TMEM_REG			= (u32 *)(DPC+0x1C);
	Gfx_Info.MI_INTR_REG			= (u32 *)(MI+0x08);
	Gfx_Info.VI_STATUS_REG			= (u32 *)(VI+0x00);
	Gfx_Info.VI_ORIGIN_REG			= (u32 *)(VI+0x04);
	Gfx_Info.VI_WIDTH_REG			= (u32 *)(VI+0x08);
	Gfx_Info.VI_INTR_REG			= (u32 *)(VI+0x0C);
	Gfx_Info.VI_V_CURRENT_LINE_REG	= (u32 *)(VI+0x10);
	Gfx_Info.VI_TIMING_REG			= (u32 *)(VI+0x14);
	Gfx_Info.VI_V_SYNC_REG			= (u32 *)(VI+0x18);
	Gfx_Info.VI_H_SYNC_REG			= (u32 *)(VI+0x1C);
	Gfx_Info.VI_LEAP_REG			= (u32 *)(VI+0x20);
	Gfx_Info.VI_H_START_REG			= (u32 *)(VI+0x24);
	Gfx_Info.VI_V_START_REG			= (u32 *)(VI+0x28);
	Gfx_Info.VI_V_BURST_REG			= (u32 *)(VI+0x2C);
	Gfx_Info.VI_X_SCALE_REG			= (u32 *)(VI+0x30);
	Gfx_Info.VI_Y_SCALE_REG			= (u32 *)(VI+0x34);
	Gfx_Info.CheckInterrupts		= DummyVoidVoid; // We do our own interrupt processing kthx!

	if (gfxdll.InitiateGFX (Gfx_Info) == FALSE) {
		CloseVideoPlugin ();
		Debug (0, "Plugin Initialization Failed.");
	}
  //Debug (0, "Video Plugin Initialized");
}