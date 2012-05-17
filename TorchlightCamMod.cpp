// TorchlightCamMod.cpp : Defines the class behaviors for the application.
//
#include "stdafx.h"
#include <sys/stat.h> 
#include "TorchlightCamMod.h"
#include "TorchlightCamModDlg.h"
#include "ModuleInstance.h"
#include "Console.h"
#include "Thread.h"
#include "Ini.h"
#include "CTimer.h"
#include <time.h>

using namespace std;
using namespace Thread;
CIniReader m_IniReader;	
MNUSTATUS * m_mnuStatus;
CStatic* m_plblStatus;
bool AppExit = false;
static int version = 0;
HANDLE hMutex;
HWND* MyHWND;

// SETTINGS DEFAULT VALUES

unsigned int FORCE_VERSION				= 0;
unsigned int DEBUG_MODE					= 0;		// Default = Disabled
unsigned int ENABLE_TILT				= 0;		// Default = Disabled
unsigned int FORCE_MEMSEARCH			= 0;		// Default = Disabled
unsigned int STARTUP_MODE				= 1;		// Default = Enabled
unsigned int SPEED_MOUSE_ROTATE			= 100;
unsigned int SPEED_MOUSE_DIRECT_ROTATE	= 100;
unsigned int SPEED_KEYBOARD_ROTATE		= 100;
unsigned int SPEED_KEYBOARD_TILT		= 50;
unsigned int SPEED_KEYBOARD_ZOOM		= 50;
unsigned int ZOOM_DEFAULT				= 0;
unsigned int ZOOM_MIN					= 14;		// 14 = Torchlight default
unsigned int ZOOM_MAX					= 35;		// 35 = More than Torchlight default
unsigned int TOGGLE_SAFEZONE			= 65;
unsigned int ROTATE_SAFEZONE			= 10;
unsigned int INVERT_MOUSE_ROTATION		= 0;		// Default = Disabled
unsigned int INVERT_DIRECT_ROTATION		= 0;
unsigned int AUTO_INVERT_MOUSE			= 0;		// Default = Disabled
unsigned int AUTO_INVERT_DIRECT			= 0;
unsigned int AUTO_RESET_ZOOM			= 1;		// Default = Enabled
unsigned int AUTO_RESET_ROTATION		= 1;		// Default = Enabled
unsigned int AUTO_RESET_TILT	 		= 1;		// Default = Enabled

// KEY MAPPINGS DEFAULT VALUES
unsigned int KEY_ZOOMIN					= 0x0;		// Default = Disabled //VK_UP;
unsigned int KEY_ZOOMOUT				= 0x0;		// Default = Disabled //VK_DOWN;
unsigned int KEY_ZOOMRESET				= 0x0;		// Default = Disabled //VK_END;
unsigned int KEY_TILTUP					= 0x0;		// Default = Disabled //VK_PRIOR;
unsigned int KEY_TILTDOWN				= 0x0;		// Default = Disabled //VK_NEXT;
unsigned int KEY_TILTRESET				= 0x0;		// Default = Disabled //VK_HOME;
unsigned int KEY_ROTATELEFT				= 0x0;		// Default = Disabled //VK_LEFT;
unsigned int KEY_ROTATERIGHT			= 0x0;		// Default = Disabled //VK_RIGHT;
unsigned int KEY_ROTATERESET			= 0x0;		// Default = Disabled //VK_HOME;
unsigned int KEY_MOUSEROTATE_TRACK		= 0x0;		// Default = Disabled //VK_CONTROL;
unsigned int KEY_MOUSEROTATE_TOGGLE		= 0x0;		// Default = Disabled //VK_CAPITAL;
unsigned int KEY_MOUSEROTATE_DIRECT		= 0x0;		// Default = Disabled //VK_MBUTTON;
unsigned int KEY_DISABLEALL 			= 0x0;		// Default = Disabled //VK_HOME;

// Default values
bool bEnabled							= false;
bool bZoomEnabled						= false;
bool bRotateEnabled						= false;
bool bTiltEnabled						= false;
bool bZoomWanted						= false;
bool bRotateWanted						= false;
bool bTiltWanted						= false;
bool bScanMemory						= false;

// temporal variables
unsigned int INVERT_MOUSE_ROTATION_SETTING = 0;
unsigned int INVERT_DIRECT_ROTATION_SETTING = 0;

int bCapslockOriginalState		= -1;
int bCapslockState				= -1;
bool bCapslockUsed				= false;

bool bCapslock					= false;
bool bCapslock_old				= false;

#define torchlight_unknown		-1
#define torchlight_steam_115	-92876942
#define torchlight_retail_113	-435427058
#define torchlight_retail_115	485237182
#define torchlight_112			-838080311
#define torchlight_112b			9526347
#define torchlight_115_demo		9526347

// CTorchlightCamModApp
BEGIN_MESSAGE_MAP(CTorchlightCamModApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

// CTorchlightCamModApp construction
CTorchlightCamModApp::CTorchlightCamModApp()
{
}


class CTorchlightCamModThread : public CThread
{
public:
	SYSTEM_INFO lpSysInfo;
	unsigned int address_min;
	unsigned int address_max;
	unsigned int address_pagesize;
	CStatic* m_plblStatus;

	void OnRunning()
	{

		// # Allocate console and set standard input/output to there.
		if (DEBUG_MODE)
		{

			ConsoleOut("  ______                __    ___       __    __  ______              \n");
			ConsoleOut(" /_  __/___  __________/ /_  / (_)___ _/ /_  / /_/ ____/___ _____ ___ \n");
			ConsoleOut("  / / / __ \\/ ___/ ___/ __ \\/ / / __ `/ __ \\/ __/ /   / __ `/ __ `__ \\\n");
			ConsoleOut(" / / / /_/ / /  / /__/ / / / / / /_/ / / / / /_/ /___/ /_/ / / / / / /\n");
			ConsoleOut("/_/  \\____/_/   \\___/_/ /_/_/_/\\__, /_/ /_/\\__/\\____/\\__,_/_/ /_/ /_/ \n");
			ConsoleOut("                              /____/                                  \n");
			ConsoleOut("\n");
			ConsoleOut("                       TorchlightCam v1.02\n\n");
		}

		////////////////////////////////////////////////////////////////
		//
		// common
		//
		int state = 0;
		int i;
		CTaskManager        taskManager;
		CExeModuleInstance  *pProcess;
		HANDLE				hProcess;
		DWORD				dwNumberOfBytesRead=0;
		DWORD				dwNumberOfBytesWritten=0;
		long				l_header_crc_code;

		DWORD dwOld; // dummy

		bool bDisableDefaultMiddleMButton = true;
		if ( KEY_ZOOMRESET == VK_MBUTTON )
		{
			bDisableDefaultMiddleMButton = false;
		}

		static BYTE* s_module_header	= new BYTE[1024];
		static BYTE* s_zoomvalue		= new BYTE[4];

		char* pRotateLimits			= 0x0;
		char* pZoomLimits			= 0x0;

		int cmd_pause_round			= 100;

		__time64_t dtime			= 0x0;
		__time64_t ltime			= 0x0;
		__time64_t ltimestart		= 0x0;

		char* lp_zoomvalue1			= 0x0;
		char* lp_zoomvalue2			= 0x0;
		char* lp_zoomvalue3			= 0x0;
		char* lp_turnaroundvalue1	= 0x0;
		char* lp_turnaroundvalue2	= 0x0;
		char* lp_turnaroundvalue3	= 0x0;
		char* lp_viewvalue1			= 0x0;
		char* lp_viewvalue2			= 0x0;
		char* lp_viewvalue3			= 0x0;
		char* lp_viewvalue4			= 0x0;
		char* lp_viewvalue5			= 0x0;
		char* lp_viewvalue6			= 0x0;
		char* lp_get_zoomsetting	= 0x0;

		// Variables for read and write
		//
		unsigned int var_zoomvalue			= 0x0;
		unsigned int var_zoomvalue1			= 0x909090;
		unsigned int var_zoomvalue2			= 0x909090;
		unsigned int var_zoomvalue3			= 0x909090;
		unsigned int var_zoomvalue1_old		= 0x0;
		unsigned int var_zoomvalue2_old		= 0x0;
		unsigned int var_zoomvalue3_old		= 0x0;
		unsigned int var_turnaroundvalue	= 0x000000;
		unsigned int var_turnaroundvalue1	= 0x909090;
		unsigned int var_turnaroundvalue2	= 0x909090;
		unsigned int var_turnaroundvalue3	= 0x909090;

		//D9 46 1C		Torchlight.exe:015968D9 fld     dword ptr [esi+1Ch]		LOAD ZOOM LEVEL => ST(1)
		//D9 46 60		Torchlight.exe:015968DC fld     dword ptr [esi+60h]		LOAD VIEW ANGLE => ST(0)
		//DE C9			Torchlight.exe:015968DF fmulp   st(1), st				MULTIPLE TOGETHER => ST(1)
		//D9 46 60		Torchlight.exe:015968E1 fld     dword ptr [esi+60h]
		//D8 E1			Torchlight.exe:015968E4 fsub    st, st(1)
		//D9 54 24 10	Torchlight.exe:015968E6 fst     dword ptr [esp+10h]

		unsigned int var_viewvalue1			= 0x90909090;
		unsigned int var_viewvalue2			= 0xD91C46D9;
		unsigned int var_viewvalue3			= 0xC9DE6446;
		unsigned int var_viewvalue4			= 0xD86446D9;
		unsigned int var_viewvalue5			= 0x2454D9E1;
		unsigned int var_viewvalue6			= 0x9010;

		unsigned int var_viewvalue1_old		= 0x0;
		unsigned int var_viewvalue2_old		= 0x0;
		unsigned int var_viewvalue3_old		= 0x0;
		unsigned int var_viewvalue4_old		= 0x0;
		unsigned int var_viewvalue5_old		= 0x0;
		unsigned int var_viewvalue6_old		= 0x0;

		unsigned int var_turnaroundvalue1_old = 0x0;
		unsigned int var_turnaroundvalue2_old = 0x0;
		unsigned int var_turnaroundvalue3_old = 0x0;

		float var_original_zoomvalue		= -999.9f;
		float var_original_rotatevalue		= -999.9f;
		float var_original_viewvalue		= -999.9f;
		float var_map_zoomvalue				= 25.0f;
		float var_default_viewvalue			= 0.3f;
		float var_current_zoomvalueOld2		= 0x0;
		float var_current_zoomvalueOld		= 0x0;
		float var_current_zoomvalue			= 0x0;
		float var_current_rotatevalue		= 0x0;
		float var_current_viewvalue			= 0x0;
		float var_current_xcoordinatevalue	= 0x0;
		float var_current_ycoordinatevalue	= 0x0;

		float var_debug_floats[32];
		memset( var_debug_floats, 0, sizeof(var_debug_floats) );

		// Memory management variables
		//
		GetSystemInfo(&lpSysInfo);
		address_min = (int)lpSysInfo.lpMinimumApplicationAddress;
		address_max = (int)lpSysInfo.lpMaximumApplicationAddress;
		address_pagesize = lpSysInfo.dwPageSize;

		MEMORY_BASIC_INFORMATION mbi;

		// Torchlight Process Addresses
		//
		static DWORD TorchlightProcessID;
		static DWORD TorchlightProcessID_Previous;
		static HMODULE TorchlightModule;
		unsigned int p_TorchlightBaseAddress;
		HWND WinHandle;
		HWND WinHandleTest;

		// Dynamic Base Lookup Addresses
		//
		// These addresses are used to lookup the special
		// dynamic memory address which contains
		// the dynamic located data.
		//
		unsigned int p_DynamicBaseLookupAddr_112		= 0xA49FF0;
		unsigned int p_DynamicBaseLookupAddr_113		= 0xA4CD20;
		unsigned int p_DynamicBaseLookupAddr_115		= 0xB0C048;
		unsigned int p_DynamicBaseLookupAddr_115e		= 0xB08560;
		unsigned int p_DynamicBaseLookupAddr_steam_115	= 0xADF0F0;

		// Dynamic addresses
		unsigned int p_DynamicBase			= 0x0;
		unsigned int p_DynamicBaseSource	= 0x0;
		unsigned int p_Set_ZoomSetting		= 0x0;
		unsigned int p_Set_RotateSetting	= 0x0;
		unsigned int p_Get_ViewSetting		= 0x0;
		unsigned int p_Set_ViewSetting		= 0x0;
		unsigned int p_Get_XCoordinate		= 0x0;
		unsigned int p_Get_YCoordinate		= 0x0;
		unsigned int p_Debug				= 0x0;

		bool bSuccessRun = false;
		unsigned int ErrorCount = 0x0;

		// Try to read version from registry
		int TorchlightVersionFromReg = 0x0;
		double fTorchlightVersionFromReg = 0.00f;
		HKEY hKey = 0;
		char buf[255] = {0};
		DWORD dwType = 0;
		DWORD dwBufSize = sizeof(buf);
		const char* subkey = "Software\\Runic Games\\Torchlight";
		if( RegOpenKey(HKEY_CURRENT_USER,subkey,&hKey) == ERROR_SUCCESS)
		{
			dwType = REG_SZ;
			if( RegQueryValueEx(hKey,"DisplayVersion",0, &dwType, (BYTE*)buf, &dwBufSize) == ERROR_SUCCESS)
			{
				fTorchlightVersionFromReg = atof(buf);
			}
			RegCloseKey(hKey);
		}
		fTorchlightVersionFromReg = fTorchlightVersionFromReg * 100;
		TorchlightVersionFromReg = (int)(fTorchlightVersionFromReg+0.5f);

		ConsoleOut("Torchlight Version from Registry = %d\n", TorchlightVersionFromReg);

		while (!AppExit)
		{

			if (!m_bIsRunning) break;

			////////////////////////////////////////////////////////////////////
			//
			// Main loop
			//
			switch (state)
			{

			case 0:
				version = 0;
				if (DEBUG_MODE) ConsoleOut("\rStatus: Idle");
				Sleep(3000);
				state++;
				break;

			case 1:
				version = 0;

				WinHandle = FindWindowEx( NULL, NULL, "OgreD3D9Wnd", "Torchlight"); //"TorchED v.1.0.67.209"

				if ( WinHandle )
				{

					TorchlightProcessID_Previous = TorchlightProcessID;

					GetWindowThreadProcessId( WinHandle, &TorchlightProcessID );

					if ( TorchlightProcessID )
					{
						if ( TorchlightProcessID_Previous == TorchlightProcessID && bEnabled)
						{
							// We have the same process, so no need to scan it again if we previously had
							// good parameters?
							ConsoleOut("\rStatus: Found Torchlight (same process)\n");
							state = 3;
							break;
						}
						else
						{
							ConsoleOut("\rStatus: Found Torchlight..\n");

							// Different process so we need to scan and set everything again.
							bEnabled				= false;
							bZoomEnabled			= false;
							bTiltEnabled			= false;
							bRotateEnabled			= false;
							bCapslockOriginalState	= -1;
							bCapslockState			= -1;
							var_original_zoomvalue	= -999.9f;
							var_original_rotatevalue = -999.9f;
							var_original_viewvalue	= -999.9f;

							taskManager.Populate( TorchlightProcessID );
							for (i = 0; i<taskManager.GetProcessCount(); i++)
							{
								pProcess = taskManager.GetProcessByIndex(i);

								TorchlightProcessID = pProcess->Get_ProcessId();
								TorchlightModule = pProcess->Get_Module();
								p_TorchlightBaseAddress = (unsigned int)TorchlightModule;

								ConsoleOut( "Task Id = %d, Base = %8x\n", i, p_TorchlightBaseAddress );

								hProcess=OpenProcess(0x1F0FFF,false,TorchlightProcessID);
								if (hProcess)
								{
									ConsoleOut( "OpenProcess(%d) - Success\n", TorchlightProcessID);

									/*
									unsigned int lp_vprotect = p_TorchlightBaseAddress + 0x63C868;
									VirtualProtectEx(hProcess, (LPVOID)lp_vprotect, 0xAu, 4u, &dwOld);
									*/

									//if (DEBUG_MODE) ConsoleOut("\rInfo: Torchlight.exe BASE = %x\n\n", TorchlightModule);

									if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
										m_plblStatus->SetWindowText("Status: Torchlight found");
									if (ReadProcessMemory(hProcess, (LPVOID)p_TorchlightBaseAddress, s_module_header, 256, &dwNumberOfBytesRead))
									{
										// # calculate crc from the header..
										__asm {
											push eax
												push ecx
												push edx
												push edi
												xor eax, eax
												xor edx, edx
												mov ecx, 256/4
	l0:
											mov edi, s_module_header
												add eax, dword ptr [edi+edx*4]
											inc edx
												loop l0
												mov l_header_crc_code, eax
												pop edi
												pop edx
												pop ecx
												pop eax
										}
									}
									else
									{
										DWORD dw = GetLastError(); 
										LPVOID lpMsgBuf;
										FormatMessage(
											FORMAT_MESSAGE_ALLOCATE_BUFFER | 
											FORMAT_MESSAGE_FROM_SYSTEM |
											FORMAT_MESSAGE_IGNORE_INSERTS,
											NULL,
											dw,
											MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
											(LPTSTR) &lpMsgBuf,
											0, NULL );
										if (DEBUG_MODE) ConsoleOut("\nERROR: ReadProcessMemory failed with error (%d) - %s\n", dw, lpMsgBuf);
										LocalFree(lpMsgBuf);
									}

									// Default = Scan memory always
									bScanMemory = true;

									unsigned int var_dynamicbase = 0x0;

									switch (l_header_crc_code)
									{
									case torchlight_112b:
										// If FORCED version = 112 || TorchlightVersionFromReg = 0 (112 do not write version to registry)
										if (FORCE_VERSION == 112 )
										{
											if (DEBUG_MODE) ConsoleOut("Game version: Torchlight v1.12b\n");
											if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
												m_plblStatus->SetWindowText("Game version: Torchlight v1.12b found, Enabling features...");
											bScanMemory = true;
											for (int i=0; i < 40; i++ )
											{
												var_dynamicbase = 0x0;
												if ( ReadProcessMemory(hProcess, (LPVOID)(p_TorchlightBaseAddress + p_DynamicBaseLookupAddr_112), &var_dynamicbase, 4u, &dwNumberOfBytesRead) )
												{
													version = torchlight_112;
													if ( var_dynamicbase != 0x0)
													{
														if ( var_dynamicbase < 0x1000 )
														{
															ErrorCount++;
															if ( ErrorCount > 4 )
																FORCE_VERSION = 0x0;
															break;
														}
														p_DynamicBase = var_dynamicbase;
														if (DEBUG_MODE) ConsoleOut("Info: Dynamic Base (Autodetect 1.12b) = %-8x, Value = %-8x\n", p_DynamicBaseLookupAddr_112, p_DynamicBase);
														bScanMemory = false;
														break;
													}
												}
												WinHandleTest = FindWindowEx( NULL, NULL, "OgreD3D9Wnd", "Torchlight");
												if ( WinHandleTest == 0 || WinHandleTest != WinHandle ) { version = 0; state = 0; break; }
												Sleep(1000);
											}
										}
										else
										{
											if (DEBUG_MODE) ConsoleOut("Game version: Torchlight v1.15 demo\n");
											if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
												m_plblStatus->SetWindowText("Game version: Torchlight v1.15 demo found, Enabling features...");
											for (int i=0; i < 40; i++ )
											{
												var_dynamicbase = 0x0;
												if ( ReadProcessMemory(hProcess, (LPVOID)(p_TorchlightBaseAddress + p_DynamicBaseLookupAddr_115), &var_dynamicbase, 4u, &dwNumberOfBytesRead) )
												{
													version = torchlight_115_demo;
													if ( var_dynamicbase != 0x0)
													{
														if ( var_dynamicbase < 0x1000 ) 
														{
															if ( ReadProcessMemory(hProcess, (LPVOID)(p_TorchlightBaseAddress + p_DynamicBaseLookupAddr_115e), &var_dynamicbase, 4u, &dwNumberOfBytesRead) )
															{
																if ( var_dynamicbase > 0x1000 )
																{
																	p_DynamicBase = var_dynamicbase;
																	if (DEBUG_MODE) ConsoleOut("Info: Dynamic Base (Autodetect 1.15 euro) = %-8x, Value = %-8x\n", p_DynamicBaseLookupAddr_115e, p_DynamicBase);
																	bScanMemory = false;
																	break;
																}
															}
														}
														else
														{
															p_DynamicBase = var_dynamicbase;
															if (DEBUG_MODE) ConsoleOut("Info: Dynamic Base (Autodetect 1.15) = %-8x, Value = %-8x\n", p_DynamicBaseLookupAddr_115, p_DynamicBase);
															bScanMemory = false;
															break;
														}
													}
												}
												WinHandleTest = FindWindowEx( NULL, NULL, "OgreD3D9Wnd", "Torchlight");
												if ( WinHandleTest == 0 || WinHandleTest != WinHandle ) { version = 0; state = 0; break; }
												Sleep(1000);
											}
										}
										break;

									case torchlight_112:
										if (DEBUG_MODE) ConsoleOut("Game version: Torchlight v1.12\n");
										if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
											m_plblStatus->SetWindowText("Game version: Torchlight v1.12 found, Enabling features...");
										for (int i=0; i < 40; i++ )
										{
											var_dynamicbase = 0x0;
											if ( ReadProcessMemory(hProcess, (LPVOID)(p_TorchlightBaseAddress + p_DynamicBaseLookupAddr_112), &var_dynamicbase, 4u, &dwNumberOfBytesRead) )
											{
												version = torchlight_112;
												if ( var_dynamicbase != 0x0)
												{
													if ( var_dynamicbase < 0x1000 ) break;
													p_DynamicBase = var_dynamicbase;
													if (DEBUG_MODE) ConsoleOut("Info: Dynamic Base (Autodetect 1.12) = %-8x, Value = %-8x\n", p_DynamicBaseLookupAddr_112, p_DynamicBase);
													bScanMemory = false;
													break;
												}
											}
											WinHandleTest = FindWindowEx( NULL, NULL, "OgreD3D9Wnd", "Torchlight");
											if ( WinHandleTest == 0 || WinHandleTest != WinHandle ) { version = 0; state = 0; break; }
											Sleep(1000);
										}
										break;
									case torchlight_retail_113:
										if (DEBUG_MODE) ConsoleOut("Game version: Torchlight Retail v1.13\n");
										if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
											m_plblStatus->SetWindowText("Game version: Torchlight Retail v1.13 found, Enabling features...");
										for (int i=0; i < 40; i++ )
										{
											var_dynamicbase = 0x0;
											if ( ReadProcessMemory(hProcess, (LPVOID)(p_TorchlightBaseAddress + p_DynamicBaseLookupAddr_113), &var_dynamicbase, 4u, &dwNumberOfBytesRead) )
											{
												version = torchlight_retail_113;
												if ( var_dynamicbase != 0x0)
												{
													if ( var_dynamicbase < 0x1000 ) break;
													p_DynamicBase = var_dynamicbase;
													if (DEBUG_MODE) ConsoleOut("Info: Dynamic Base (Autodetect 1.13) = %-8x, Value = %-8x\n", p_DynamicBaseLookupAddr_113, p_DynamicBase);
													bScanMemory = false;
													break;
												}
											}
											WinHandleTest = FindWindowEx( NULL, NULL, "OgreD3D9Wnd", "Torchlight");
											if ( WinHandleTest == 0 || WinHandleTest != WinHandle ) { version = 0; state = 0; break; }
											Sleep(1000);
										}
										break;
									case torchlight_retail_115:
										if (DEBUG_MODE) ConsoleOut("Game version: Torchlight Retail v1.15\n");
										if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
											m_plblStatus->SetWindowText("Game version: Torchlight Retail v1.15 found, Enabling features...");
										for (int i=0; i < 40; i++ )
										{
											if ( ReadProcessMemory(hProcess, (LPVOID)(p_TorchlightBaseAddress + p_DynamicBaseLookupAddr_115), &var_dynamicbase, 4u, &dwNumberOfBytesRead) ) 
											{
												version = torchlight_retail_115;
												if ( var_dynamicbase != 0x0)
												{
													if ( var_dynamicbase < 0x1000 ) 
													{
														if ( ReadProcessMemory(hProcess, (LPVOID)(p_TorchlightBaseAddress + p_DynamicBaseLookupAddr_115e), &var_dynamicbase, 4u, &dwNumberOfBytesRead) )
														{
															if ( var_dynamicbase > 0x1000 ) {
																p_DynamicBase = var_dynamicbase;
																if (DEBUG_MODE) ConsoleOut("Info: Dynamic Base (Autodetect 1.15 euro) = %-8x, Value = %-8x\n", p_DynamicBaseLookupAddr_115e, p_DynamicBase);
																bScanMemory = false;
																break;
															}
														}
													}
													else
													{
														p_DynamicBase = var_dynamicbase;
														if (DEBUG_MODE) ConsoleOut("Info: Dynamic Base (Autodetect 1.15) = %-8x, Value = %-8x\n", p_DynamicBaseLookupAddr_115, p_DynamicBase);
														bScanMemory = false;
														break;
													}
												}
											}
											WinHandleTest = FindWindowEx( NULL, NULL, "OgreD3D9Wnd", "Torchlight");
											if ( WinHandleTest == 0 || WinHandleTest != WinHandle ) { version = 0; state = 0; break; }
											Sleep(1000);
										}
										break;
									case torchlight_steam_115:
										if (DEBUG_MODE) ConsoleOut("Game version: Torchlight Steam v1.15\n");
										if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
											m_plblStatus->SetWindowText("Game version: Torchlight Steam v1.15 found, Enabling features...");
										for (int i=0; i < 40; i++ )
										{
											if ( ReadProcessMemory(hProcess, (LPVOID)(p_TorchlightBaseAddress + p_DynamicBaseLookupAddr_steam_115), &var_dynamicbase, 4u, &dwNumberOfBytesRead) ) 
											{
												version = torchlight_steam_115;
												if ( var_dynamicbase != 0x0)
												{
													if ( var_dynamicbase < 0x1000 ) break;
													p_DynamicBase = var_dynamicbase;
													if (DEBUG_MODE) ConsoleOut("Info: Dynamic Base (Autodetect steam 1.15) = %-8x, Value = %-8x\n", p_DynamicBaseLookupAddr_steam_115, p_DynamicBase);
													bScanMemory = false;
													break;
												}
											}
											WinHandleTest = FindWindowEx( NULL, NULL, "OgreD3D9Wnd", "Torchlight");
											if ( WinHandleTest == 0 || WinHandleTest != WinHandle ) { version = 0; state = 0; break; }
											Sleep(1000);
										}
										break;
									default:
										version = torchlight_unknown;
										if (DEBUG_MODE) ConsoleOut("Game version: Torchlight (Unknown version, CRC = %d)\n", l_header_crc_code);
										if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
											m_plblStatus->SetWindowText("Game version: Torchlight found, Unknown version. Enabling features...");
										break;
									}
									CloseHandle(hProcess);
								}
								else
								{
									DWORD dw = GetLastError(); 
									LPVOID lpMsgBuf;
									FormatMessage(
										FORMAT_MESSAGE_ALLOCATE_BUFFER | 
										FORMAT_MESSAGE_FROM_SYSTEM |
										FORMAT_MESSAGE_IGNORE_INSERTS,
										NULL,
										dw,
										MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
										(LPTSTR) &lpMsgBuf,
										0, NULL );
									if (DEBUG_MODE) ConsoleOut("\nERROR: OpenProcess failed with error (%d) - %s\n", dw, lpMsgBuf);
									LocalFree(lpMsgBuf);
								}
							}
						
						}

					}
				}
				if (version == 0)
				{
					state = 0;
					if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
						m_plblStatus->SetWindowText("Status: Idle");
				}
				else
				{
					ErrorCount = 0;
					if (FORCE_MEMSEARCH)
						bScanMemory = true;
					state = 2;
				}

				break;

			case 2:
				if (state == 0) break;

				// Find pointer positions from memory
				hProcess=OpenProcess(0x1F0FFF,false,TorchlightProcessID);
				if (hProcess)
				{
					if (DEBUG_MODE) ConsoleOut("\rLooking up Rotate Limits.." );
					pRotateLimits = FindTwoUInt32Sep( hProcess, p_TorchlightBaseAddress + 0x1d0000, address_max, 3u, 0x2C46D8, 0x2C56D9);

					if (DEBUG_MODE) ConsoleOut("\rLooking up Zoom Limits..  " );
					pZoomLimits = FindUInt32( hProcess, p_TorchlightBaseAddress + 0x1d0000, address_max, 3u, 0x1C5ED9 );

					if (DEBUG_MODE) ConsoleOut("\r                          \r" );

					char* lp_value = 0x0;
					if ( bScanMemory )
					{
						p_DynamicBase = NULL;

						char* pDynamicBaseSource = NULL;

						// START OF NEW SCANNER

						// Application address space minumum and maximum

						unsigned int address_cur = address_min;
						unsigned int address_low;
						unsigned int address_high;

						address_low = 0;
						address_high = 0;
						unsigned int count = 0;

						do
						{
							VirtualQueryEx(hProcess,(void*)address_cur,&mbi,sizeof(MEMORY_BASIC_INFORMATION));

							address_low = (unsigned int)mbi.BaseAddress;
							address_high = address_low + mbi.RegionSize;

							ConsoleOut("\rScanning #%-3d: 0x%-8x-0x%-8x (0x%-6x bytes)  ", count, address_low, address_high, mbi.RegionSize);
							p_DynamicBase = FindUInt32Special01( hProcess, address_low, address_high, 4u, 0x00000000, 0x00000000, 0x0000008B, 0x00000000, 0x00000005 );

							// 00000000 00000000 ???????? 0000008B
							if ( p_DynamicBase != NULL )
							{
								unsigned int lp_offset = (char*)p_DynamicBase - (char*)address_low;
								if (DEBUG_MODE) ConsoleOut("\r                                                                             \r");
								if (DEBUG_MODE) ConsoleOut("\rFound in #%-3d: 0x%-6x-0x%-6x (0x%-6x bytes), Offset +%x\n", count, address_low, address_high, mbi.RegionSize, lp_offset);
								break;
							}
							count++;
							address_cur = address_high;
						} while (address_cur < address_max);

						// If DynamicBase is found, try to find memory location for the variable
						// memory location should only be within the base process space
						count = 0x0;
						address_cur = p_TorchlightBaseAddress;
						do
						{
							VirtualQueryEx(hProcess,(void*)address_cur,&mbi,sizeof(MEMORY_BASIC_INFORMATION));
							address_low = (unsigned int)mbi.BaseAddress;
							address_high = address_low + mbi.RegionSize;
							ConsoleOut("\rScanning #%-3d: 0x%-8x-0x%-8x (0x%-6x bytes)  ", count, address_low, address_high, mbi.RegionSize);
							p_DynamicBaseSource = FindUInt32Special02( hProcess, address_low, address_high, 4u, p_DynamicBase, 0x00000000, 0x3F000000 );
							// 00000000 00000000 ???????? 0000008B
							if ( p_DynamicBaseSource != NULL )
							{
								unsigned int lp_offset = (char*)p_DynamicBaseSource - (char*)address_low;
								if (DEBUG_MODE) ConsoleOut("\r                                                                             \r");
								if (DEBUG_MODE) ConsoleOut("\rFound in memory #%-3d: 0x%-6x-0x%-6x (0x%-6x bytes), offset +%x\n", count, address_low, address_high, mbi.RegionSize, lp_offset);
								break;
							}
							count++;
							address_cur = address_high;
						} while (address_cur < address_max);

						if (DEBUG_MODE) ConsoleOut("\r                                                                             \r");

						if (DEBUG_MODE && (p_DynamicBaseSource != 0x0) )
						{
							ConsoleOut("\r\n*****************************************************************************\n");
							ConsoleOut("**                                                                         **\n");
							ConsoleOut("** PLEASE REPORT THESE VALUES TO THE DEVELOPER                             **\n");
							ConsoleOut("**                                                                         **\n");
							ConsoleOut("** Torchlight.exe CRC = 0x%-8x                                         **\n", l_header_crc_code );
							ConsoleOut("** Torchlight.exe BASE = 0x%-8x                                        **\n", p_TorchlightBaseAddress );
							ConsoleOut("** Possible dynamic base source = 0x%-8x                               **\n", p_DynamicBaseSource /*- (char*)TorchlightModule*/ );
							ConsoleOut("**                                                                         **\n");
							ConsoleOut("*****************************************************************************\n");
						}

						// END OF NEW SCANNER
					}

					if ( pZoomLimits != NULL && pRotateLimits != NULL && p_DynamicBase != NULL )
					{
						if ( !bTiltEnabled && bTiltWanted )
						{
							(char*)lp_viewvalue1 = (char*)(pRotateLimits+0x85); //(char*)(p_TorchlightBaseAddress+0x1E68FA);
							(char*)lp_viewvalue2 = (char*)(lp_viewvalue1-0x1E);
							(char*)lp_viewvalue3 = (char*)(lp_viewvalue1-0x1E+4);
							(char*)lp_viewvalue4 = (char*)(lp_viewvalue1-0x1E+(4*2));
							(char*)lp_viewvalue5 = (char*)(lp_viewvalue1-0x1E+(4*3));
							(char*)lp_viewvalue6 = (char*)(lp_viewvalue1-0x1E+(4*4));
							ReadProcessMemory(hProcess, lp_viewvalue1, &var_viewvalue1_old, 4u, &dwNumberOfBytesRead);
							if ( WriteProcessMemory(hProcess, lp_viewvalue1, &var_viewvalue1, 4u, &dwNumberOfBytesWritten) )
							{
								ReadProcessMemory(hProcess, lp_viewvalue2, &var_viewvalue2_old, 4u, &dwNumberOfBytesRead);
								WriteProcessMemory(hProcess, lp_viewvalue2, &var_viewvalue2, 4u, &dwNumberOfBytesWritten);
								ReadProcessMemory(hProcess, lp_viewvalue3, &var_viewvalue3_old, 4u, &dwNumberOfBytesRead);
								WriteProcessMemory(hProcess, lp_viewvalue3, &var_viewvalue3, 4u, &dwNumberOfBytesWritten);
								ReadProcessMemory(hProcess, lp_viewvalue4, &var_viewvalue4_old, 4u, &dwNumberOfBytesRead);
								WriteProcessMemory(hProcess, lp_viewvalue4, &var_viewvalue4, 4u, &dwNumberOfBytesWritten);
								ReadProcessMemory(hProcess, lp_viewvalue5, &var_viewvalue5_old, 4u, &dwNumberOfBytesRead);
								WriteProcessMemory(hProcess, lp_viewvalue5, &var_viewvalue5, 4u, &dwNumberOfBytesWritten);
								ReadProcessMemory(hProcess, lp_viewvalue6, &var_viewvalue6_old, 2u, &dwNumberOfBytesRead);
								WriteProcessMemory(hProcess, lp_viewvalue6, &var_viewvalue6, 2u, &dwNumberOfBytesWritten);
								ConsoleOut("\nStatus: VIEW enabled.\n");
								bTiltEnabled = true;
								if ( var_original_viewvalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_original_viewvalue, 4u, &dwNumberOfBytesRead);
								WriteProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_current_viewvalue, 4u, &dwNumberOfBytesWritten);
							}
						}
						if ( !bZoomEnabled && bZoomWanted )
						{
							(char*)lp_zoomvalue1 = (char*)pZoomLimits;
							(char*)lp_zoomvalue2 = (char*)pZoomLimits - 20;
							(char*)lp_zoomvalue3 = (char*)pZoomLimits + 29;
							ReadProcessMemory(hProcess, lp_zoomvalue1, &var_zoomvalue1_old, 3u, &dwNumberOfBytesRead);
							WriteProcessMemory(hProcess, lp_zoomvalue1, &var_zoomvalue1, 3u, &dwNumberOfBytesWritten);
							ReadProcessMemory(hProcess, lp_zoomvalue2, &var_zoomvalue2_old, 3u, &dwNumberOfBytesRead);
							WriteProcessMemory(hProcess, lp_zoomvalue2, &var_zoomvalue2, 3u, &dwNumberOfBytesWritten);
							Sleep(0x32u);
							if ( bDisableDefaultMiddleMButton )
							{
								ReadProcessMemory(hProcess, lp_zoomvalue3, &var_zoomvalue3_old, 3u, &dwNumberOfBytesRead);
								WriteProcessMemory(hProcess, lp_zoomvalue3, &var_zoomvalue3, 3u, &dwNumberOfBytesWritten);
							}
							ConsoleOut("\nStatus: ZOOM enabled.\n");
							bZoomEnabled = true;
						}

						if ( !bRotateEnabled && bRotateWanted )
						{
							// 1.15 point is actually the automatic rotate function and not the reset to 0 thing..
							(char*)lp_turnaroundvalue1 = (char*)pRotateLimits + (332 - 40);
							(char*)lp_turnaroundvalue2 = (char*)pRotateLimits + (332 - 40) + 40;			//40; // v1.12
							(char*)lp_turnaroundvalue3 = (char*)pRotateLimits + (332 - 40) + 52;			//52; // v1.12
							ReadProcessMemory(hProcess, lp_turnaroundvalue1, &var_turnaroundvalue1_old, 3u, &dwNumberOfBytesRead);
							WriteProcessMemory(hProcess, lp_turnaroundvalue1, &var_turnaroundvalue1, 3u, &dwNumberOfBytesWritten);
							ReadProcessMemory(hProcess, lp_turnaroundvalue2, &var_turnaroundvalue2_old, 3u, &dwNumberOfBytesRead);
							WriteProcessMemory(hProcess, lp_turnaroundvalue2, &var_turnaroundvalue2, 3u, &dwNumberOfBytesWritten);
							ReadProcessMemory(hProcess, lp_turnaroundvalue3, &var_turnaroundvalue3_old, 3u, &dwNumberOfBytesRead);
							WriteProcessMemory(hProcess, lp_turnaroundvalue3, &var_turnaroundvalue3, 3u, &dwNumberOfBytesWritten);
							ConsoleOut("\nStatus: ROTATE enabled.\n");
							bRotateEnabled = true;
						}

						p_Set_ZoomSetting	= p_DynamicBase + 28;
						p_Set_RotateSetting = p_DynamicBase + 28 + (4*4);
						p_Get_XCoordinate	= p_DynamicBase + 28 + (4*5);
						p_Get_YCoordinate	= p_DynamicBase + 28 + (4*7);
						p_Get_ViewSetting	= p_DynamicBase + 28 + (4*2);
						p_Set_ViewSetting	= p_DynamicBase + 0x64; // 64 = Works pretty nicely.. //28 + (4*2); //+ (4*8) = FREE MEMORY AREA! (NO CRASH!);

						// Working:
						// 0x016ED5F0 = MINIMUM ZOOM DISTANCE (Should be changed to lower than default on startup, ini setting!)
						// 0x016ED5F4 = MAXIMUM ZOOM DISTANCE (Should be changed to higher than default on startup, ini setting, fix the current implementation!)

						// Not working:
						// 0x016D5588 - 360 - no effect :(
						// 0x01B3FAAC - no effect - could be the zoom level (no writable)
						// 0x016ED610 - no effect
						// 0x016ED5F0
						// 0x01B3FAAC - no effect
						p_Debug				= p_Set_ZoomSetting; //p_DynamicBase + 28; // - 0x24;

						unsigned int lp_vprotect = p_Set_ViewSetting;
						VirtualProtectEx(hProcess, (LPVOID)lp_vprotect, 0xAu, 4u, &dwOld);

						if ( bZoomEnabled && bRotateEnabled ) 
						{
							bEnabled = true;
						}

						if (DEBUG_MODE) {
							ConsoleOut("\n------------------------------------------------------\n");
							ConsoleOut("Pointers:\n");
							ConsoleOut("------------------------------------------------------\n");
							ConsoleOut("* Game BASE         = %6x\n", p_TorchlightBaseAddress);
							ConsoleOut("* Dynamic BASE      = %6x\n", p_DynamicBase );
							ConsoleOut("* Zoom Limits       = %6x\n", pZoomLimits );
							ConsoleOut("* Rotate Limits     = %6x\n", pRotateLimits );
							ConsoleOut("* Zoom Code #1      = %6x\n", lp_zoomvalue1 );
							ConsoleOut("* Zoom Code #2      = %6x\n", lp_zoomvalue2 );
							ConsoleOut("* Zoom Code #3      = %6x\n", lp_zoomvalue3 );
							ConsoleOut("* Rotate Code #1    = %6x\n", lp_turnaroundvalue1 );
							ConsoleOut("* Rotate Code #2    = %6x\n", lp_turnaroundvalue2 );
							ConsoleOut("* Rotate Code #3    = %6x\n", lp_turnaroundvalue3 );
							ConsoleOut("* View Code #1      = %6x\n", lp_viewvalue1 );
							ConsoleOut("* View Code #2      = %6x\n", lp_viewvalue2 );
							ConsoleOut("* View Code #3      = %6x\n", lp_viewvalue3 );
							ConsoleOut("* View Code #4      = %6x\n", lp_viewvalue4 );
							ConsoleOut("* View Code #5      = %6x\n", lp_viewvalue5 );
							ConsoleOut("* View Code #6      = %6x\n", lp_viewvalue6 );
							ConsoleOut("* Zoom Value Set    = %6x\n", p_Set_ZoomSetting );
							ConsoleOut("* Rotate Value Set  = %6x\n", p_Set_RotateSetting );
							ConsoleOut("* View Value Set    = %6x\n", p_Set_ViewSetting );
							ConsoleOut("* View Value Get    = %6x\n", p_Get_ViewSetting );
						}
					}
					else
					{
						if (DEBUG_MODE) {
							ConsoleOut("\n------------------------------------------------------\n");
							ConsoleOut("Pointers:\n");
							ConsoleOut("------------------------------------------------------\n");
							ConsoleOut("* Game BASE         = %6x\n", p_TorchlightBaseAddress);
							ConsoleOut("* Dynamic BASE      = %6x\n", p_DynamicBase );
							ConsoleOut("* Zoom Limits       = %6x\n", pZoomLimits );
							ConsoleOut("* Rotate Limits     = %6x\n", pRotateLimits );
							ConsoleOut("* Zoom Value Set    = %6x\n", p_DynamicBase + 28 );
							ConsoleOut("* Rotate Value Set  = %6x\n", p_DynamicBase + 28 + 16 );
						}
					}

					CloseHandle(hProcess);

					if ( bEnabled )
					{
						state = 3;
					}
					else
					{
						if ( DEBUG_MODE ) {
							ConsoleOut("\n----------------------------------------------------------\n");
							ConsoleOut("ERROR: Could not find the right parameters. Trying again..\n");
							ConsoleOut("----------------------------------------------------------\n");
							Sleep(2000);
						}
						state = 0;
					}
				}
				else
				{
					DWORD dw = GetLastError(); 
					LPVOID lpMsgBuf;
					FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER | 
						FORMAT_MESSAGE_FROM_SYSTEM |
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						dw,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPTSTR) &lpMsgBuf,
						0, NULL );
					if (DEBUG_MODE) ConsoleOut("\nERROR: OpenProcess failed with error (%d): %s\n", dw, lpMsgBuf);
					LocalFree(lpMsgBuf);
				}
				if ( state == 0)
				{
					if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
						m_plblStatus->SetWindowText("Status: Idle");
				}
				break;

			case 3:
				// Store Capslock state if its used in key mappings
				if ( bCapslockOriginalState == -1 && bCapslockUsed )
				{
					bCapslockOriginalState = GetCapslockState(); //((unsigned short) GetKeyState(VK_CAPITAL)) & 0xffff) != 0;

					if ( STARTUP_MODE == 0 )
						SetCapslock( false );
					else
						SetCapslock( true );
				}

				bool bDirectRotatingStarted = false;
				bool bTrackRotationStarted = false;
				int bTrackRotationStartMode = 0;
				bool bMouseRotationToggled = false;

				// If we have StartupMode == 1 then enable the rotation
				if ( STARTUP_MODE == 1)
				{
					bMouseRotationToggled = true;
				}

				if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
					m_plblStatus->SetWindowText("Status: ACTIVE, Features enabled!");
				if (DEBUG_MODE) ConsoleOut("\nStatus: Starting main loop\n");

				WINDOWINFO lpWindowInfo;
				WINDOWINFO lpWindowInfoOld;
				POINT lpCursor;
				POINT lpCursorLast;

				INT64 tick;			// A point in time
				INT64 tickLast;		// A point in time
				INT64 time;			// For converting tick into real time

				// get the high resolution counter's accuracy
				CTimer timer;

				GetWindowInfo(WinHandle, &lpWindowInfoOld);

				bool bMouseToggleRotationKey = false;

				timer.Initialize();

				float var_current_rotatevalueOld = 0.0f;
				float var_current_xcoordinatevalueOld = -999.9f;
				float var_current_ycoordinatevalueOld = -999.9f;
				float var_current_xspeed = 0.0f;
				float var_current_yspeed = 0.0f;
				float var_current_xspeedMax = 0.0f;
				float var_current_yspeedMax = 0.0f;

				int maxCounter = 0;

				hProcess=OpenProcess(0x1F0FFF,false,TorchlightProcessID);
				if (hProcess)
				{
					while (!AppExit)
					{
						if ( !ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesRead) )
						{
							// Couldn't read current rotation value.
							// If we get this error four times in a row and haven't had any success
							// check if we have forced 1.12 version and remove the force setting temporally.
							ErrorCount++;
							if ( !bSuccessRun && ErrorCount > 2 && TorchlightVersionFromReg == 0x0 )
							{
								if ( FORCE_VERSION == 112 )
								{
									ConsoleOut("Removing setting FORCE_VERSION because too many errors in a row.");
									TorchlightProcessID_Previous = 0x0;
									FORCE_VERSION = 0x0;
									ErrorCount = 0;
									state = -1;
								}
								else
								{
									ConsoleOut("Setting FORCE_VERSION == 112 because too many errors in a row.");
									TorchlightProcessID_Previous = 0x0;
									FORCE_VERSION = 112;
									ErrorCount = 0;
									state = -1;
								}
							}
							break;
						}

						if ( AUTO_RESET_ROTATION || AUTO_RESET_ZOOM || AUTO_RESET_TILT )
						{
							if ( var_current_xcoordinatevalueOld == -999.9f )
							{
								if ( !ReadProcessMemory(hProcess, (LPVOID)p_Get_XCoordinate, &var_current_xcoordinatevalueOld, 4u, &dwNumberOfBytesRead) )
								{
									break;
								}
								if ( !ReadProcessMemory(hProcess, (LPVOID)p_Get_YCoordinate, &var_current_ycoordinatevalueOld, 4u, &dwNumberOfBytesRead) )
								{
									break;
								}
							}
							if ( var_current_xcoordinatevalueOld != -999.9f )
							{
								if ( !ReadProcessMemory(hProcess, (LPVOID)p_Get_XCoordinate, &var_current_xcoordinatevalue, 4u, &dwNumberOfBytesRead) )
								{
									break;
								}
								if ( !ReadProcessMemory(hProcess, (LPVOID)p_Get_YCoordinate, &var_current_ycoordinatevalue, 4u, &dwNumberOfBytesRead) )
								{
									break;
								}

								var_current_xspeed = abs(var_current_xcoordinatevalueOld - var_current_xcoordinatevalue);
								var_current_yspeed = abs(var_current_ycoordinatevalueOld - var_current_ycoordinatevalue);

								maxCounter++;
								if ( var_current_xspeed > var_current_xspeedMax ) { var_current_xspeedMax = var_current_xspeed; maxCounter=0; }
								if ( var_current_yspeed > var_current_yspeedMax ) { var_current_yspeedMax = var_current_yspeed; maxCounter=0; }

								if ( maxCounter > 20 )
								{
									var_current_xspeedMax = 0;
									var_current_yspeedMax = 0;
									maxCounter = 0;
								}

								if ( var_current_xspeed > 10 || var_current_yspeed > 10 )
								{
									ConsoleOut("\rDetected Teleporting...                                      \n");
									Sleep(50);
									ReadProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_map_zoomvalue, 4u, &dwNumberOfBytesWritten);
									ConsoleOut("Info: Map default zoom = %3.2f\n", var_map_zoomvalue);
									if ( AUTO_RESET_TILT && bTiltEnabled )
									{
										// Reset rotation to default
										var_current_viewvalue = var_default_viewvalue;
										ConsoleOut("Info: Resetting Tilt Value to %3.2f\n", var_current_viewvalue);
										WriteProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_current_viewvalue, 4u, &dwNumberOfBytesWritten);
									}
									if ( AUTO_RESET_ROTATION && bRotateEnabled )
									{
										// Reset rotation to default
										var_current_rotatevalue=var_original_rotatevalue;
										if ( var_original_rotatevalue != -999.9f )
										{
											ConsoleOut("Info: Resetting Rotation Value to %3.2f\n", var_current_rotatevalue);
											WriteProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesWritten);
										}
									}
									if ( AUTO_RESET_ZOOM )
									{
										// Reset zoom to default
										if ( var_original_zoomvalue != -999.9f && bZoomEnabled )
										{
											if ( ZOOM_DEFAULT != 0x0 )
											{
												var_current_zoomvalue=ZOOM_DEFAULT;
												ConsoleOut("Info: Resetting Zoom Value to %3.2f\n", var_current_zoomvalue);
												WriteProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_current_zoomvalue, 4u, &dwNumberOfBytesWritten);
											}
											else
											{
												ConsoleOut("Info: Resetting Zoom Value to torchlight default.\n");
												ReadProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_current_zoomvalue, 4u, &dwNumberOfBytesWritten);
											}
										}
									}
									else
									{
										if ( var_original_zoomvalue != -999.9f && bZoomEnabled && var_current_zoomvalueOld2 != 0x0 )
										{
											ConsoleOut("Info: Restoring Zoom Value to %3.2f\n", var_current_zoomvalueOld2);
											WriteProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_current_zoomvalueOld2, 4u, &dwNumberOfBytesWritten);
										}
									}

								}
								//ConsoleOut("\rX = %8.3f, Y = %8.3f (XS = %8.3f, YS = %8.3f) (XMAX = %8.3f, YMAX = %8.3f)    ", var_current_xcoordinatevalue, var_current_ycoordinatevalue, var_current_xspeed, var_current_yspeed, var_current_xspeedMax, var_current_yspeedMax);
								var_current_xcoordinatevalueOld = var_current_xcoordinatevalue;
								var_current_ycoordinatevalueOld = var_current_ycoordinatevalue;
							}
						}

						if ( DEBUG_MODE && 1==2 )
						{
							/****************
							***** DEBUG *****
							*****************/
							if ( ReadProcessMemory(hProcess, (LPVOID)p_Debug, &var_debug_floats, (sizeof(float) * 32), &dwNumberOfBytesRead) )
							{
								ConsoleOut("\r");
								for (int i = 0; i < 10; /*14;*/ i++)
								{
									ConsoleOut("%#5.1f ", var_debug_floats[i]);
								}
							}
						}

						GetWindowInfo(WinHandle, &lpWindowInfo);
						if ( lpWindowInfo.rcWindow.right - lpWindowInfo.rcWindow.left != lpWindowInfoOld.rcWindow.right - lpWindowInfoOld.rcWindow.left )
							break;
						if ( lpWindowInfo.rcWindow.bottom - lpWindowInfo.rcWindow.top != lpWindowInfoOld.rcWindow.bottom - lpWindowInfoOld.rcWindow.top )
							break;

						timer.Update();

						HWND ForegroundWindow = GetForegroundWindow();
						if ( ForegroundWindow == WinHandle )
						{
							bCapslock = GetCapslockState();

							GetCursorPos(&lpCursor);
							ScreenToClient(WinHandle, &lpCursor);

							int _width = (lpWindowInfo.rcClient.right - lpWindowInfo.rcClient.left) - lpWindowInfo.cxWindowBorders;
							int _height = (lpWindowInfo.rcClient.bottom - lpWindowInfo.rcClient.top) - lpWindowInfo.cyWindowBorders;

							bool CMD_DISABLEALL = GetAsyncKeyState(KEY_DISABLEALL) & 0x8000;

							if ( lpCursor.x < -3 || lpCursor.y < -3 || lpCursor.x > (_width+3) || lpCursor.y > (_height+3) || CMD_DISABLEALL )
							{
								// Cursor not inside the window, do nothing.
							}
							else
							{
								if ( bTiltEnabled )
								{
									if ( var_original_viewvalue == -999.9f )
									{
										ReadProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_original_viewvalue, 4u, &dwNumberOfBytesRead);
										var_current_viewvalue=0;
										if (!WriteProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_current_viewvalue, 4u, &dwNumberOfBytesWritten))
										{
											ConsoleOut("\nError while WriteProcessMemory!\n");
										}
									}
								}

								if ( bCapslockUsed ) bCapslockState = bCapslock;

								double _x = ((double)(double)lpCursor.x - (_width/2))/(double)(_width/2)*100.0f;
								double _y = ((double)(double)lpCursor.y - (_height/2))/(double)(_height/2)*100.0f;

								// Get keyboard states
								bool CMD_TRACKROTATION = GetAsyncKeyState(KEY_MOUSEROTATE_TRACK) & 0x8000;
								bool CMD_DIRECTROTATION = GetAsyncKeyState(KEY_MOUSEROTATE_DIRECT) & 0x8000;

								if ( AUTO_INVERT_MOUSE == 1 && !bDirectRotatingStarted )
								{
									if ( INVERT_MOUSE_ROTATION_SETTING == 1 )
									{
										if ( _y > 0 )
											INVERT_MOUSE_ROTATION = 0;
										else
											INVERT_MOUSE_ROTATION = 1;
									}
									else
									{
										if ( _y > 0 )
											INVERT_MOUSE_ROTATION = 1;
										else
											INVERT_MOUSE_ROTATION = 0;
									}

								}

								if ( AUTO_INVERT_DIRECT == 1 && !bDirectRotatingStarted )
								{
									if ( INVERT_DIRECT_ROTATION_SETTING == 1 )
									{
										if ( _y > 0 )
											INVERT_DIRECT_ROTATION = 0;
										else
											INVERT_DIRECT_ROTATION = 1;
									}
									else
									{
										if ( _y > 0 )
											INVERT_DIRECT_ROTATION = 1;
										else
											INVERT_DIRECT_ROTATION = 0;
									}

								}

								if ( KEY_MOUSEROTATE_TOGGLE == VK_CAPITAL )
								{
									if ( bCapslock )
										bMouseRotationToggled = true;
									else
										bMouseRotationToggled = false;
								}
								else
								{
									// Rotate toggle key
									if (GetAsyncKeyState(KEY_MOUSEROTATE_TOGGLE) & 0x8000)
									{
										bMouseToggleRotationKey = true;
									}
									else
									{
										if ( bMouseToggleRotationKey )
										{
											bMouseToggleRotationKey = false;
											if ( bMouseRotationToggled )
												bMouseRotationToggled = false;
											else
												bMouseRotationToggled = true;
										}
									}
								}

								bool bToggledRotationEnabled = false;
								bool bTrackRotation = false;

								// Check if mouse is out of the safezone areas
								bool bNotSafeZoneForToggled = false;
								bool bNotSafeZoneForTrackRotation = false;
								if ( _x < -(double)TOGGLE_SAFEZONE ) bNotSafeZoneForToggled = true;
								if ( _x > (double)TOGGLE_SAFEZONE ) bNotSafeZoneForToggled = true;
								if ( _x < -(double)ROTATE_SAFEZONE ) bNotSafeZoneForTrackRotation = true;
								if ( _x > (double)ROTATE_SAFEZONE ) bNotSafeZoneForTrackRotation = true;

								if ( !bDirectRotatingStarted )
								{
									if ( !bTrackRotationStarted )
									{
										// Activate toggled rotation when not in safe zone
										if ( bMouseRotationToggled && bNotSafeZoneForToggled )
										{
											bToggledRotationEnabled = true;
										}
										// Activate track rotation when not in safe zone
										if ( CMD_TRACKROTATION )
										{
											// If we have previously started toggled rotation
											// then we must temporally disable rotation
											if ( bToggledRotationEnabled )
											{
												bTrackRotationStarted = true;
												bTrackRotationStartMode = 1;
											}
											else
											{
												if ( bNotSafeZoneForTrackRotation )
												{
													bTrackRotationStarted = true;
													bTrackRotationStartMode = 0;
												}
											}
										}
									}
								}

								if ( bTrackRotationStarted && !CMD_TRACKROTATION )
								{
									bTrackRotationStarted = false;
								}
								if ( bTrackRotationStarted )
								{
									if ( bTrackRotationStartMode == 1 )
									{
										bToggledRotationEnabled = false;
										bTrackRotation = false;
									}
									else
									{
										bTrackRotation = true;
									}
								}

								if ( CMD_DIRECTROTATION && ( bTrackRotation || bToggledRotationEnabled ) )
								{
									bToggledRotationEnabled = false;
									bTrackRotation = false;
								}


								/*
								if ( (CMD_TRACKROTATION || bMouseRotationToggled)  && !bDirectRotatingStarted )
								{
									bToggledRotationEnabled = true;
									if ( !bMouseRotationToggled ) bTrackRotation = true;
								}
								if ( (CMD_TRACKROTATION & 0x8000) && bMouseRotationToggled && !bTrackRotation )
									bToggledRotationEnabled = false;
								if ( bToggledRotationEnabled && (CMD_DIRECTROTATION) )
									bToggledRotationEnabled = false;
								*/

								if ( bRotateEnabled )
								{
									if ( bTrackRotation )
									{
										if ( _x < -(double)ROTATE_SAFEZONE )
										{
											if ( var_original_rotatevalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_original_rotatevalue, 4u, &dwNumberOfBytesRead);
											ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesRead);
											if ( INVERT_MOUSE_ROTATION )
												var_current_rotatevalue-=((double)timer.Get_dt()/50) * ((double)abs(_x)/10) * ((double)SPEED_MOUSE_ROTATE/100);
											else
												var_current_rotatevalue+=((double)timer.Get_dt()/50) * ((double)abs(_x)/10) * ((double)SPEED_MOUSE_ROTATE/100);
											WriteProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesWritten);
										}
										if ( _x > (double)ROTATE_SAFEZONE )
										{
											if ( var_original_rotatevalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_original_rotatevalue, 4u, &dwNumberOfBytesRead);
											ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesRead);
											if ( INVERT_MOUSE_ROTATION )
												var_current_rotatevalue+=((double)timer.Get_dt()/50) * ((double)abs(_x)/10) * ((double)SPEED_MOUSE_ROTATE/100);
											else
												var_current_rotatevalue-=((double)timer.Get_dt()/50) * ((double)abs(_x)/10) * ((double)SPEED_MOUSE_ROTATE/100);
											WriteProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesWritten);
										}
									}
									else if ( bToggledRotationEnabled )
									{
										if ( _x < -(double)TOGGLE_SAFEZONE )
										{
											if ( var_original_rotatevalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_original_rotatevalue, 4u, &dwNumberOfBytesRead);
											ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesRead);
											if ( INVERT_MOUSE_ROTATION )
												var_current_rotatevalue-=((double)timer.Get_dt()/50) * ((double)abs(_x)/10) * ((double)SPEED_MOUSE_ROTATE/100);
											else
												var_current_rotatevalue+=((double)timer.Get_dt()/50) * ((double)abs(_x)/10) * ((double)SPEED_MOUSE_ROTATE/100);
											WriteProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesWritten);
										}
										if ( _x > (double)TOGGLE_SAFEZONE )
										{
											if ( var_original_rotatevalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_original_rotatevalue, 4u, &dwNumberOfBytesRead);
											ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesRead);
											if ( INVERT_MOUSE_ROTATION )
												var_current_rotatevalue+=((double)timer.Get_dt()/50) * ((double)abs(_x)/10) * ((double)SPEED_MOUSE_ROTATE/100);
											else
												var_current_rotatevalue-=((double)timer.Get_dt()/50) * ((double)abs(_x)/10) * ((double)SPEED_MOUSE_ROTATE/100);
											WriteProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesWritten);
										}
									}
									else if ( CMD_DIRECTROTATION )
									{
										if ( bDirectRotatingStarted == false )
										{
											ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalueOld, 4u, &dwNumberOfBytesRead);
											lpCursorLast.x = lpCursor.x;
											lpCursorLast.y = lpCursor.y;
											bDirectRotatingStarted = true;
										}
										double _x2 = ((double)(double)lpCursorLast.x - (_width/2))/(double)(_width/2)*100.0f;
										if ( INVERT_DIRECT_ROTATION )
											var_current_rotatevalue = var_current_rotatevalueOld - (_x2 - _x)*2.0f * ((double)SPEED_MOUSE_DIRECT_ROTATE/100);
										else
											var_current_rotatevalue = var_current_rotatevalueOld + (_x2 - _x)*2.0f * ((double)SPEED_MOUSE_DIRECT_ROTATE/100);
										if ( var_original_rotatevalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_original_rotatevalue, 4u, &dwNumberOfBytesRead);
										WriteProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesWritten);
									}
									else if (bDirectRotatingStarted)
									{
										bDirectRotatingStarted = false;
										if ( KEY_MOUSEROTATE_DIRECT == VK_CAPITAL )
										{
											if (bCapslock)
											{
												// Simulate a key press
												keybd_event( VK_CAPITAL,
													0x45,
													KEYEVENTF_EXTENDEDKEY | 0,
													0 );
												// Simulate a key release
												keybd_event( VK_CAPITAL,
													0x45,
													KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
													0);
											}
										}
									}

									// Rotate Left (Cursor Left)
									if (GetAsyncKeyState(KEY_ROTATELEFT) & 0x8000)
									{
										if ( var_original_rotatevalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_original_rotatevalue, 4u, &dwNumberOfBytesRead);
										ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesRead);
										var_current_rotatevalue+=(timer.Get_dt()/10) * ((double)SPEED_KEYBOARD_ROTATE/100);
										WriteProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesWritten);
										//if (DEBUG_MODE) ConsoleOut("-");
									}
									// Rotate Right (Cursor Right)
									if (GetAsyncKeyState(KEY_ROTATERIGHT) & 0x8000)
									{
										if ( var_original_rotatevalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_original_rotatevalue, 4u, &dwNumberOfBytesRead);
										ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesRead);
										var_current_rotatevalue-=(timer.Get_dt()/10) * ((double)SPEED_KEYBOARD_ROTATE/100);
										WriteProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesWritten);
										//if (DEBUG_MODE) ConsoleOut("+");
									}
									// Rotate Center (End)
									if ( (GetAsyncKeyState(KEY_ROTATERESET) & 0x8000 ) /*|| ( GetAsyncKeyState(VK_MBUTTON) & 0x8000 )*/ )
									{
										if ( var_original_rotatevalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_original_rotatevalue, 4u, &dwNumberOfBytesRead);
										ReadProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesRead);
										var_current_rotatevalue=var_original_rotatevalue;
										WriteProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_current_rotatevalue, 4u, &dwNumberOfBytesWritten);
										//if (DEBUG_MODE) ConsoleOut("+");
									}
								}

								if ( bZoomEnabled )
								{
									// Zoom In (Cursor Up)
									if (GetAsyncKeyState(KEY_ZOOMIN) & 0x8000)
									{
										if ( var_original_zoomvalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_original_zoomvalue, 4u, &dwNumberOfBytesRead);
										ReadProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_current_zoomvalue, 4u, &dwNumberOfBytesRead);
										var_current_zoomvalue-=(timer.Get_dt()/20) * ((double)SPEED_KEYBOARD_ZOOM/100);
										if ( var_current_zoomvalue < ZOOM_MIN ) var_current_zoomvalue = ZOOM_MIN;
										WriteProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_current_zoomvalue, 4u, &dwNumberOfBytesWritten);
										//if (DEBUG_MODE) ConsoleOut("-");
									}
									// Zoom Out (Cursor Down)
									if (GetAsyncKeyState(KEY_ZOOMOUT) & 0x8000)
									{
										if ( var_original_zoomvalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_original_zoomvalue, 4u, &dwNumberOfBytesRead);
										ReadProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_current_zoomvalue, 4u, &dwNumberOfBytesRead);
										var_current_zoomvalue+=(timer.Get_dt()/20) * ((double)SPEED_KEYBOARD_ZOOM/100);
										if ( var_current_zoomvalue > ZOOM_MAX ) var_current_zoomvalue = ZOOM_MAX;
										WriteProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_current_zoomvalue, 4u, &dwNumberOfBytesWritten);
										//if (DEBUG_MODE) ConsoleOut("+");
									}
									// Zoom Reset (if not the default middle mouse button)
									if ( KEY_ZOOMRESET != VK_MBUTTON )
									{
										if ( GetAsyncKeyState(KEY_ZOOMRESET) & 0x8000 )
										{
											if ( var_original_zoomvalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_original_zoomvalue, 4u, &dwNumberOfBytesRead);
											if ( ZOOM_DEFAULT == 0x0 )
											{
												var_current_zoomvalue=var_map_zoomvalue;
											}
											else
											{
												var_current_zoomvalue=ZOOM_DEFAULT;
											}
											WriteProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_current_zoomvalue, 4u, &dwNumberOfBytesWritten);
										}
									}
								}

								if ( bTiltEnabled )
								{
									// View Up (Page Up)
									if (GetAsyncKeyState(KEY_TILTUP) & 0x8000)
									{
										if ( var_original_viewvalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_original_viewvalue, 4u, &dwNumberOfBytesRead);
										ReadProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_current_viewvalue, 4u, &dwNumberOfBytesRead);
										//ConsoleOut("\nView value = %5.3f\n", var_current_viewvalue);
										var_current_viewvalue-=(timer.Get_dt()/500) * ((double)SPEED_KEYBOARD_TILT/100);
										if ( var_current_viewvalue < 0.0f )
											var_current_viewvalue = 0.0f;
										if (!WriteProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_current_viewvalue, 4u, &dwNumberOfBytesWritten))
										{
											ConsoleOut("\nError while WriteProcessMemory!\n");
										}
										//WriteProcessMemory(hProcess, (LPVOID)(p_Set_ViewSetting+4), &var_current_viewvalue, 4u, &dwNumberOfBytesWritten);
									}
									// View Down (Page Down)
									if (GetAsyncKeyState(KEY_TILTDOWN) & 0x8000)
									{
										if ( var_original_viewvalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_original_viewvalue, 4u, &dwNumberOfBytesRead);
										ReadProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_current_viewvalue, 4u, &dwNumberOfBytesRead);
										//ConsoleOut("\nView value = %5.3f\n", var_current_viewvalue);
										var_current_viewvalue+=(timer.Get_dt()/500) * ((double)SPEED_KEYBOARD_TILT/100);
										if ( var_current_viewvalue > 0.7f )
											var_current_viewvalue = 0.7f;
										if (!WriteProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_current_viewvalue, 4u, &dwNumberOfBytesWritten))
										{
											ConsoleOut("\nError while WriteProcessMemory!\n");
										}
										//WriteProcessMemory(hProcess, (LPVOID)(p_Set_ViewSetting+4), &var_current_viewvalue, 4u, &dwNumberOfBytesWritten);
									}
									// View Reset (Home)
									if (GetAsyncKeyState(KEY_TILTRESET) & 0x8000)
									{
										if ( var_original_viewvalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_original_viewvalue, 4u, &dwNumberOfBytesRead);
										ReadProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_current_viewvalue, 4u, &dwNumberOfBytesRead);
										//ConsoleOut("\nView value = %5.3f\n", var_current_viewvalue);
										var_current_viewvalue=var_default_viewvalue; //var_original_viewvalue;
										if (!WriteProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_current_viewvalue, 4u, &dwNumberOfBytesWritten))
										{
											ConsoleOut("\nError while WriteProcessMemory!\n");
										}
										//WriteProcessMemory(hProcess, (LPVOID)(p_Set_ViewSetting+4), &var_current_viewvalue, 4u, &dwNumberOfBytesWritten);
									}
								}

							}

							if ( bZoomEnabled )
							{
								if ( var_original_zoomvalue == -999.9f ) ReadProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_original_zoomvalue, 4u, &dwNumberOfBytesRead);
								if ( var_current_zoomvalue != var_current_zoomvalueOld )
								{
									var_current_zoomvalueOld2 = var_current_zoomvalueOld;
									var_current_zoomvalueOld = var_current_zoomvalue;
								}
								ReadProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_current_zoomvalue, 4u, &dwNumberOfBytesRead);
								//ConsoleOut("\rZoom = %3.2f (Old = %3.2f)       \r", var_current_zoomvalue, var_current_zoomvalueOld2);
								if ( var_current_zoomvalue > ZOOM_MAX )
								{
									var_current_zoomvalue = ZOOM_MAX;
									if ( var_original_zoomvalue != -999.9f ) WriteProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_current_zoomvalue, 4u, &dwNumberOfBytesWritten);
								}
							}

							bSuccessRun = true;
							ErrorCount = 0;
						}

						if (m_mnuStatus->SpeedLow) 
						{ 
							// 125ms per round
							cmd_pause_round = 100;
						}

						if (m_mnuStatus->SpeedNormal)
						{ 
							// 75ms per round
							cmd_pause_round = 65;
						}

						if (m_mnuStatus->SpeedHigh) 
						{ 
							// 0ms per round
							cmd_pause_round = 35;
						}

						Sleep(cmd_pause_round);
					}
					CloseHandle(hProcess);
				}
				else
				{
					if (DEBUG_MODE) ConsoleOut("\nWarning: OpenProcess failed.\n");
				}
				if (DEBUG_MODE) ConsoleOut("\n");
				if ( state == -1 )
				{
					if ( bEnabled )
					{
						if (DEBUG_MODE) ConsoleOut("\nStatus: Disabling features\n");
						if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
							m_plblStatus->SetWindowText("Status: DEACTIVED, Features disabled");
						hProcess=OpenProcess(0x1F0FFF,false,TorchlightProcessID);
						if (hProcess)
						{
							if ( bTiltEnabled )
							{
								if ( var_original_viewvalue != -999.9f)
								{
									if (!WriteProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_original_viewvalue, 4u, &dwNumberOfBytesWritten))
									{
										ConsoleOut("\nError while WriteProcessMemory!\n");
									}
								}
								WriteProcessMemory(hProcess, lp_viewvalue1, &var_viewvalue1_old, 4u, &dwNumberOfBytesWritten);
								WriteProcessMemory(hProcess, lp_viewvalue2, &var_viewvalue2_old, 4u, &dwNumberOfBytesWritten);
								WriteProcessMemory(hProcess, lp_viewvalue3, &var_viewvalue3_old, 4u, &dwNumberOfBytesWritten);
								WriteProcessMemory(hProcess, lp_viewvalue4, &var_viewvalue4_old, 4u, &dwNumberOfBytesWritten);
								WriteProcessMemory(hProcess, lp_viewvalue5, &var_viewvalue5_old, 4u, &dwNumberOfBytesWritten);
								WriteProcessMemory(hProcess, lp_viewvalue6, &var_viewvalue6_old, 2u, &dwNumberOfBytesWritten);
								bTiltEnabled = false;
							}
							if ( bZoomEnabled )
							{
								if ( var_original_zoomvalue != -999.9f)
								{
									if (!WriteProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_original_zoomvalue, 4u, &dwNumberOfBytesWritten))
									{
										ConsoleOut("\nError while WriteProcessMemory!\n");
									}
								}
								WriteProcessMemory(hProcess, lp_zoomvalue1, &var_zoomvalue1_old, 3u, &dwNumberOfBytesWritten);
								WriteProcessMemory(hProcess, lp_zoomvalue2, &var_zoomvalue2_old, 3u, &dwNumberOfBytesWritten);
								Sleep(0x32u);
								if ( bDisableDefaultMiddleMButton )
								{
									WriteProcessMemory(hProcess, lp_zoomvalue3, &var_zoomvalue3_old, 3u, &dwNumberOfBytesWritten);
								}
								bZoomEnabled = false;
							}
							if ( bRotateEnabled )
							{
								if ( var_original_rotatevalue != -999.9f)
								{
									if (!WriteProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_original_rotatevalue, 4u, &dwNumberOfBytesWritten))
									{
										ConsoleOut("\nError while WriteProcessMemory!\n");
									}
								}
								WriteProcessMemory(hProcess, lp_turnaroundvalue1, &var_turnaroundvalue1_old, 3u, &dwNumberOfBytesWritten);
								WriteProcessMemory(hProcess, lp_turnaroundvalue2, &var_turnaroundvalue2_old, 3u, &dwNumberOfBytesWritten);
								WriteProcessMemory(hProcess, lp_turnaroundvalue3, &var_turnaroundvalue3_old, 3u, &dwNumberOfBytesWritten);
								bRotateEnabled = false;
							}
							CloseHandle(hProcess);
						}
						bEnabled = false;
					}
				}
				state=0;
				if ( state == 0)
				{
					if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
						m_plblStatus->SetWindowText("Status: Idle");
				}
				// Restore Capslock state if its used in key mappings
				if ( bCapslockOriginalState != -1 && bCapslockUsed )
				{
					if ( bCapslockOriginalState )
					{
						// Capslock should be on
						SetCapslock( true );
					}
					else
					{
						// Capslock should be off
						SetCapslock ( false );
					}
				}
				break;
			}
		}

		if ( bEnabled )
		{
			if (DEBUG_MODE) ConsoleOut("\nStatus: Disabling features\n");
			if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
				m_plblStatus->SetWindowText("Status: DEACTIVED, Features disabled");
			hProcess=OpenProcess(0x1F0FFF,false,TorchlightProcessID);
			if (hProcess)
			{
				if ( bTiltEnabled )
				{
					if ( var_original_viewvalue != -999.9f)
					{
						if (!WriteProcessMemory(hProcess, (LPVOID)p_Set_ViewSetting, &var_original_viewvalue, 4u, &dwNumberOfBytesWritten))
						{
							ConsoleOut("\nError while WriteProcessMemory!\n");
						}
					}
					WriteProcessMemory(hProcess, lp_viewvalue1, &var_viewvalue1_old, 4u, &dwNumberOfBytesWritten);
					WriteProcessMemory(hProcess, lp_viewvalue2, &var_viewvalue2_old, 4u, &dwNumberOfBytesWritten);
					WriteProcessMemory(hProcess, lp_viewvalue3, &var_viewvalue3_old, 4u, &dwNumberOfBytesWritten);
					WriteProcessMemory(hProcess, lp_viewvalue4, &var_viewvalue4_old, 4u, &dwNumberOfBytesWritten);
					WriteProcessMemory(hProcess, lp_viewvalue5, &var_viewvalue5_old, 4u, &dwNumberOfBytesWritten);
					WriteProcessMemory(hProcess, lp_viewvalue6, &var_viewvalue6_old, 2u, &dwNumberOfBytesWritten);
					bTiltEnabled = false;
				}
				if ( bZoomEnabled )
				{
					if ( var_original_zoomvalue != -999.9f)
					{
						if (!WriteProcessMemory(hProcess, (LPVOID)p_Set_ZoomSetting, &var_original_zoomvalue, 4u, &dwNumberOfBytesWritten))
						{
							ConsoleOut("\nError while WriteProcessMemory!\n");
						}
					}
					WriteProcessMemory(hProcess, lp_zoomvalue1, &var_zoomvalue1_old, 3u, &dwNumberOfBytesWritten);
					WriteProcessMemory(hProcess, lp_zoomvalue2, &var_zoomvalue2_old, 3u, &dwNumberOfBytesWritten);
					Sleep(0x32u);
					if ( bDisableDefaultMiddleMButton )
					{
						WriteProcessMemory(hProcess, lp_zoomvalue3, &var_zoomvalue3_old, 3u, &dwNumberOfBytesWritten);
					}
					bZoomEnabled = false;
				}
				if ( bRotateEnabled )
				{
					if ( var_original_rotatevalue != -999.9f)
					{
						if (!WriteProcessMemory(hProcess, (LPVOID)p_Set_RotateSetting, &var_original_rotatevalue, 4u, &dwNumberOfBytesWritten))
						{
							ConsoleOut("\nError while WriteProcessMemory!\n");
						}
					}
					WriteProcessMemory(hProcess, lp_turnaroundvalue1, &var_turnaroundvalue1_old, 3u, &dwNumberOfBytesWritten);
					WriteProcessMemory(hProcess, lp_turnaroundvalue2, &var_turnaroundvalue2_old, 3u, &dwNumberOfBytesWritten);
					WriteProcessMemory(hProcess, lp_turnaroundvalue3, &var_turnaroundvalue3_old, 3u, &dwNumberOfBytesWritten);
					bRotateEnabled = false;
				}
				CloseHandle(hProcess);
			}
			bEnabled = false;
		}

		// Restore Capslock state if its used in key mappings
		if ( bCapslockOriginalState != -1 && bCapslockUsed )
		{
			if ( bCapslockOriginalState )
			{
				// Capslock should be on
				SetCapslock( true );
			}
			else
			{
				// Capslock should be off
				SetCapslock ( false );
			}
		}

		if (DEBUG_MODE) ConsoleOut("\n\nStatus: Bye!\n");
		if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
			m_plblStatus->SetWindowText("Status: Bye!");

	}

	char* FindTwoUInt32Sep( HANDLE ProcessHandle, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, 
		unsigned int DataValue, 
		unsigned int DataValue2 )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		UINT32 var_value = 0x0;
		char* lp_value;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=1)
		{
			lp_value = (char*)i;

			// Try to find the first value
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, DataLen, &dwNumberOfBytesRead ) )
			{
				if ( var_value == DataValue )
				{
					lp_value = (char*)(i + DataLen);
					if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, DataLen, &dwNumberOfBytesRead ) )
					{
						if ( var_value == DataValue2 )
						{
							return ( lp_value );
						}
					}
				}
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				unsigned int j = i % address_pagesize;
				i+= (address_pagesize - j);
			}
		}
		return ( NULL );
	}


	char* FindTwoUInt32Sepx( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, 
		unsigned int DataValue, 
		unsigned int DataValue2 )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		UINT32 var_value = 0x0;
		char* lp_value;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=1)
		{
			lp_value = (char*)BaseAddress + i;
			// Try to find the first value
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, DataLen, &dwNumberOfBytesRead ) )
			{
				if ( var_value == DataValue )
				{
					// Found the first value, now find the next
					lp_value = (char*)BaseAddress + (i + DataLen);
					if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, DataLen, &dwNumberOfBytesRead ) )
					{
						//ConsoleOut("\nNext value == %-6x\n", var_value);
						if ( var_value == DataValue2 )
						{
							return ( lp_value );
						}
					}
				}
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				unsigned int j = i % address_pagesize;
				i+= (address_pagesize - j);
			}
		}
		return ( NULL );
	}

	static bool GetCapslockState()
	{
		if ( GetKeyState(VK_CAPITAL) != 0 ) //keyState[VK_CAPITAL] != 0)
			return ( true );
		else
			return ( false );
	}

	static void SetCapslock ( bool bState )
	{
		if ( (bState && GetKeyState(VK_CAPITAL) == 0) || (!bState && GetKeyState(VK_CAPITAL) != 0) )
		{
			// Simulate a key press
			keybd_event( VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0 );
			Sleep(50);
			// Simulate a key release
			keybd_event( VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
		}
	}

	char* FindFourUInt32SepSpecial01( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, 
		unsigned int DataValue, 
		unsigned int DataValue2,
		unsigned int DataValue3,
		unsigned int DataValue4 )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		UINT32 var_value[4];
		char* lp_value;
		char* lp_value2;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=1)
		{
			lp_value = (char*)BaseAddress + i;
			// Try to find the first value
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, DataLen, &dwNumberOfBytesRead ) )
			{
				if ( var_value[0] == DataValue )
				{
					ConsoleOut("\rFound first value @ %x", lp_value );
					lp_value2 = (char*)BaseAddress + (i + DataLen);
					if ( ReadProcessMemory(ProcessHandle, lp_value2, &var_value, DataLen * 3, &dwNumberOfBytesRead ) )
					{
						if ( var_value[0] == DataValue2 &&
							 var_value[1] == DataValue3 &&
							 var_value[2] == DataValue4 )
						{
							return ( lp_value );
						}
					}
				}
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				unsigned int j = i % address_pagesize;
				i+= (address_pagesize - j);
			}
		}
		return ( NULL );
	}

	char* FindThreeUInt32Special01( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, 
		unsigned int DataValue,
		unsigned int DataValue2,
		unsigned int DataValue4 )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value[10];
		char* lp_value;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=4)
		{
			lp_value = (char*)BaseAddress + i;
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, sizeof(unsigned int) * 4, &dwNumberOfBytesRead ) )
			{
				if ( var_value[0] == DataValue &&
					var_value[1] == DataValue2 &&
					var_value[3] == DataValue4 ) /*&&
												 var_value[4] != 0x0 &&
												 var_value[5] == 0x0 )*/
				{
					// check next two dword values (@+20) to make sure that we have right position...
					char* lp_value2 = (char*)BaseAddress + i + (20*4);
					if ( ReadProcessMemory(ProcessHandle, lp_value2, &var_value, sizeof(unsigned int) * 3, &dwNumberOfBytesRead ) )
					{
						if ( var_value[0] == 0x0 && var_value[1] == 0x0 && var_value[2] == 0x5)
							return ( lp_value );
					}
				}
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				unsigned int j = i % address_pagesize;
				i+= (address_pagesize - j);
			}
		}
		return ( NULL );
	}


	unsigned int FindUInt32Special01( HANDLE ProcessHandle, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, 
		unsigned int DataValue,
		unsigned int DataValue2,
		unsigned int DataValue3,
		unsigned int DataValue4,
		unsigned int DataValue5 )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value[32];
		unsigned int lp_value;
		unsigned int lp_value2;
		unsigned int lp_value3;
		unsigned int j, k;
		k = 0;
		for (unsigned int i = ptrStart; i < ptrEnd-32; i+=32)
		{
			lp_value = i;

			// as fast as possible search first for Value3
			if ( ReadProcessMemory(ProcessHandle, (LPVOID)lp_value, &var_value, 32 * 4, &dwNumberOfBytesRead ) )
			{
				for ( j = 0; j < 32; j++ )
				{
					if ( var_value[j] == DataValue3 )
					{

						// found Value3, now check for Value 1 and 2, retake the block if not aligned correctly

						if ( j > 2 )
						{
							// can be found in same block
							if ( var_value[j-3] == DataValue && var_value[j-2] == DataValue2 )
							{
								// check next two dword values (@+20) to make sure that we have right position...
								lp_value2 = lp_value + (j*4) + (4) + (16*4);
								if ( ReadProcessMemory(ProcessHandle, (LPVOID)lp_value2, &var_value, 3 * 4, &dwNumberOfBytesRead ) )
								{
									if ( var_value[0] == DataValue4 && var_value[2] == DataValue5)
										return ( lp_value + (j*4) + (4) );
								}
							}
						}
						else
						{
							// we need to retake the two values
							lp_value2 = lp_value - (3*4) + (j*4);
							ReadProcessMemory(ProcessHandle, (LPVOID)lp_value2, &var_value, 2 * 4, &dwNumberOfBytesRead );
							if ( var_value[0] == DataValue && var_value[1] == DataValue2 )
							{
								// check next two dword values (@+20) to make sure that we have right position...
								lp_value3 = lp_value2 + (2*4) + (4) + (16*4);
								if ( ReadProcessMemory(ProcessHandle, (LPVOID)lp_value3, &var_value, 3 * 4, &dwNumberOfBytesRead ) )
								{
									if ( var_value[0] == DataValue4 && var_value[2] == DataValue5)
										return ( lp_value2 + (j*4) + (4) );
								}
							}
						}

						break;
					}
				}
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				unsigned int j = i % address_pagesize;
				i+= (address_pagesize - j);
			}
		}
		return ( NULL );
	}



	/*
	char* FindUInt32Special01x( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, 
		unsigned int DataValue,
		unsigned int DataValue2,
		unsigned int DataValue3,
		unsigned int DataValue4,
		unsigned int DataValue5 )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value[32];
		char* lp_value;
		char* lp_value2;
		char* lp_value3;
		unsigned int j, k;
		k = 0;
		for (unsigned int i = ptrStart; i < ptrEnd-32; i+=32)
		{
			lp_value = (char*)BaseAddress + i;

			// as fast as possible search first for Value3
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, 32 * 4, &dwNumberOfBytesRead ) )
			{
				for ( j = 0; j < 32; j++ )
				{
					if ( var_value[j] == DataValue3 )
					{
						
						// found Value3, now check for Value 1 and 2, retake the block if not aligned correctly

						if ( j > 2 )
						{
							// can be found in same block
							if ( var_value[j-3] == DataValue && var_value[j-2] == DataValue2 )
							{
								// check next two dword values (@+20) to make sure that we have right position...
								lp_value2 = lp_value + (j*4) + (4) + (16*4);
								if ( ReadProcessMemory(ProcessHandle, lp_value2, &var_value, 3 * 4, &dwNumberOfBytesRead ) )
								{
									if ( var_value[0] == DataValue4 && var_value[2] == DataValue5)
										return ( lp_value + (j*4) + (4) );
								}
							}
						}
						else
						{
							// we need to retake the two values
							lp_value2 = lp_value - (3*4) + (j*4);
							ReadProcessMemory(ProcessHandle, lp_value2, &var_value, 2 * 4, &dwNumberOfBytesRead );
							if ( var_value[0] == DataValue && var_value[1] == DataValue2 )
							{
								// check next two dword values (@+20) to make sure that we have right position...
								lp_value3 = lp_value2 + (2*4) + (4) + (16*4);
								if ( ReadProcessMemory(ProcessHandle, lp_value3, &var_value, 3 * 4, &dwNumberOfBytesRead ) )
								{
									if ( var_value[0] == DataValue4 && var_value[2] == DataValue5)
										return ( lp_value2 + (j*4) + (4) );
								}
							}
						}

						break;
					}
				}
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				unsigned j = i % address_pagesize;
				i+=j;
			}
		}
		return ( NULL );
	}
	*/

	unsigned int FindUInt32Special02( HANDLE ProcessHandle, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, 
		unsigned int DataValue,
		unsigned int DataValue2,
		unsigned int DataValue3 )		
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value[256];
		unsigned int var_test[4];
		ZeroMemory(var_value, 256*4);
		unsigned int lp_value;
		unsigned int lp_value2;
		unsigned int lp_value3;
		unsigned int j, k;

		unsigned int nSize = 128;
		unsigned int varPtr = 0;
		unsigned int varPtrLast = 0;

		k = 0;

		unsigned int i = ptrStart;
		do
		{
			// as fast as possible search first for Value3
			if ( ReadProcessMemory(ProcessHandle, (LPVOID)i, /*lp_out*/ var_value, nSize * 4, &dwNumberOfBytesRead ) )
			{
				unsigned j = 0;
				do /*for ( j = 0; j < nSize; j++ )*/
				{
					if ( var_value[varPtr + j] == DataValue3 )
					{
						// Found the right parameters... Try to align again to the start of this block
						if ( ReadProcessMemory(ProcessHandle, (LPVOID)(i + (j*4) - 8), var_test, 4*4, &dwNumberOfBytesRead ) )
						{
							if ( var_test[0] == DataValue && 
								 var_test[1] == DataValue2 &&
								 var_test[2] == DataValue3 &&
								 var_test[3] == DataValue3 )
							{
								return ( i + (j*4) - 8 );
							}
							else
								j += 4;
						}
						else
							ConsoleOut("\nFATAL ERROR: Can't read memory at this location!\n");

						/*
						// ConsoleOut("\rFind first value @ %x", lp_value );
						// found Value3, now check for Value 1 and 2 and 4, retake the block if not aligned correctly
						if ( j > 1 )
						{
							//ConsoleOut("\r%-8x > 1", i + j);
							// can be found in same block
							if ( var_value[varPtr + j - 2] == DataValue && var_value[varPtr + j - 1] == DataValue2 )
							{
								ConsoleOut("\n\n\nFIRST METHOD FOUND THE ADDRESS == %d\n", j);


								return ( lp_value + (j*4) - (8) );
							}
						}
						else
						{
							// we need to check the lastblock for the missing data

							unsigned int nVarPtr = nSize + j - 2;
							ConsoleOut("\r%-8x < 2 (= %d)", i + j, nVarPtr);

							int nVarP1 = varPtr + (j - 2);
							if ( nVarP1 < 0 ) nVarP1 += nSize*2;
							int nVarP2 = varPtr + (j - 1);
							if ( nVarP2 < 0 ) nVarP2 += nSize*2;

							//if ( var_value[varPtrLast + nVarPtr] == DataValue &&
							//	var_value[varPtrLast + nVarPtr + 1] == DataValue2)
							if ( var_value[nVarP1] == DataValue &&
								var_value[nVarP2] == DataValue2)
							{
								ConsoleOut("\!Used the secondary way of finding the right addresses...\n");
								return ( lp_value + (j*4) - (8) );
							}
						}
						*/
					}
					j++;
				} while ( j < nSize );
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				//unsigned int j = i % address_pagesize;
				//i+= (address_pagesize - j);
			}
			// advance to next block
			i += nSize;
		} while ( i < ptrEnd-nSize );
		return ( NULL );
	}

	unsigned int ReadUInt32( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart,unsigned int DataLen )
	{
		DWORD dwNumberOfBytesRead = 0;
		unsigned int var_value = 0;
		char* lp_value = (char*)BaseAddress + ptrStart;
		if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, DataLen, &dwNumberOfBytesRead ) )
		{
			return ( var_value );
		}
		else
			return ( NULL );
	}

	char* FindUInt32Aligned4( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, unsigned int DataValue )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value[10];
		char* lp_value;
		// align to 4 bytes
		unsigned int ptrAdjust = ptrStart % 4;
		ptrStart += ptrAdjust;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=4)
		{
			lp_value = (char*)BaseAddress + i;
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, DataLen, &dwNumberOfBytesRead ) )
			{
				if ( var_value[0] == DataValue )
				{
					return ( lp_value );
				}
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				unsigned j = i % address_pagesize;
				i+=j;
			}
		}
		return ( NULL );
	}

	char* FindThreeUInt32( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, 
		unsigned int DataValue, 
		unsigned int DataValue2,
		unsigned int DataValue3 )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value[10];
		char* lp_value;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=1)
		{
			lp_value = (char*)BaseAddress + i;
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, sizeof(unsigned int) * 3, &dwNumberOfBytesRead ) )
			{
				if ( var_value[0] == DataValue &&
					var_value[1] == DataValue2 &&
					var_value[2] == DataValue3 )
				{
					return ( lp_value );
				}
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				unsigned j = i % address_pagesize;
				i+=j;
			}
		}
		return ( NULL );
	}

	char* FindTwoUInt32( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, 
		unsigned int DataValue, 
		unsigned int DataValue2 )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value[10];
		char* lp_value;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=1)
		{
			lp_value = (char*)BaseAddress + i;
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, sizeof(unsigned int) * 2, &dwNumberOfBytesRead ) )
			{
				if ( var_value[0] == DataValue &&
					var_value[1] == DataValue2 )
				{
					return ( lp_value );
				}
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				unsigned j = i % address_pagesize;
				i+=j;
			}
		}
		return ( NULL );
	}

	char* FindSixUInt32( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, 
		unsigned int DataValue, 
		unsigned int DataValue2, 
		unsigned int DataValue3, 
		unsigned int DataValue4, 
		unsigned int DataValue5, 
		unsigned int DataValue6 )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value[10];
		char* lp_value;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=4)
		{
			lp_value = (char*)BaseAddress + i;
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, sizeof(unsigned int) * 6, &dwNumberOfBytesRead ) )
			{
				if (	var_value[0] == DataValue &&
					var_value[1] == DataValue2 &&
					var_value[2] == DataValue3 &&
					var_value[3] == DataValue4 &&
					var_value[4] == DataValue5 &&
					var_value[5] == DataValue6 )
				{
					return ( lp_value );
				}
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				unsigned j = i % address_pagesize;
				i+=j;
			}
		}
		return ( NULL );
	}

	char* FindFourUInt32( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, 
		unsigned int DataValue, 
		unsigned int DataValue2, 
		unsigned int DataValue3, 
		unsigned int DataValue4 )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value[10];
		char* lp_value;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=4)
		{
			lp_value = (char*)BaseAddress + i;
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, sizeof(unsigned int) * 4, &dwNumberOfBytesRead ) )
			{
				if (	var_value[0] == DataValue &&
					var_value[1] == DataValue2 &&
					var_value[2] == DataValue3 &&
					var_value[3] == DataValue4 )
				{
					return ( lp_value );
				}
			}
			else
			{
				// Unreadable memory, jump to next 4096 bytes (page)
				unsigned j = i % address_pagesize;
				i+=j;
			}
		}
		return ( NULL );
	}

	char* FindUInt32By1( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, unsigned int DataValue )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value = 0x0;
		char* lp_value;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=1)
		{
			lp_value = (char*)BaseAddress + i;
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, 3u, &dwNumberOfBytesRead ) )
			{
				if ( var_value == DataValue )
				{
					return ( lp_value );
				}
			}
			else
				break;
		}
		return ( NULL );
	}

	char* FindUInt32( HANDLE ProcessHandle, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, unsigned int DataValue )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value = 0x0;
		char* lp_value;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=4)
		{
			lp_value = (char*)i;
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, 3u, &dwNumberOfBytesRead ) )
			{
				if ( var_value == DataValue )
				{
					return ( lp_value );
				}
			}
			else
				break;
		}
		return ( NULL );
	}

	/*
	char* FindUInt32x( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, unsigned int DataValue )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value = 0x0;
		char* lp_value;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=4)
		{
			lp_value = (char*)BaseAddress + i;
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, 3u, &dwNumberOfBytesRead ) )
			{
				if ( var_value == DataValue )
				{
					return ( lp_value );
				}
			}
			else
				break;
		}
		return ( NULL );
	}
	*/

	char* FindFirstOfTwoUInt32( HANDLE ProcessHandle, HMODULE BaseAddress, unsigned int ptrStart, unsigned int ptrEnd, unsigned int DataLen, unsigned int DataValue, unsigned int DataValue2 )
	{
		DWORD dwNumberOfBytesRead=0;
		bool bFound = false;
		unsigned int var_value = 0x0;
		char* lp_value;
		for (unsigned int i = ptrStart; i < ptrEnd; i+=4)
		{
			lp_value = (char*)BaseAddress + i;
			if ( ReadProcessMemory(ProcessHandle, lp_value, &var_value, 3u, &dwNumberOfBytesRead ) )
			{
				if ( var_value == DataValue || var_value == DataValue2 )
				{
					return ( lp_value );
				}
			}
			else
				break;
		}
		return ( NULL );
	}

};



// The one and only CTorchlightCamModApp object

CTorchlightCamModApp theApp;

inline bool FileExists(string strFilename) { 
	struct stat stFileInfo; 
	bool blnReturn; 
	int intStat; 

	// Attempt to get the file attributes 
	intStat = stat(strFilename.c_str(),&stFileInfo); 
	if(intStat == 0) { 
		// We were able to get the file attributes 
		// so the file obviously exists. 
		blnReturn = true; 
	} else { 
		// We were not able to get the file attributes. 
		// This may mean that we don't have permission to 
		// access the folder which contains this file. If you 
		// need to do that level of checking, lookup the 
		// return values of stat which will give you 
		// more details on why stat failed. 
		blnReturn = false; 
	} 

	return(blnReturn); 
}

// CTorchlightCamModApp initialization

BOOL CTorchlightCamModApp::ControlHandler( DWORD fdwCtrlType ) 
{ 
	switch( fdwCtrlType ) 
	{ 
		// Handle the CTRL-C signal. 
	case CTRL_C_EVENT: 
		if ( !AppExit )
			SendMessageA(*MyHWND, WM_CLOSE, 0, 0);
		AppExit = true;
		return( TRUE );
		// CTRL-CLOSE: confirm that the user wants to exit. 
	case CTRL_CLOSE_EVENT: 
		if ( !AppExit )
			SendMessageA(*MyHWND, WM_CLOSE, 0, 0);
		AppExit = true;
		//ConsoleOut( "\nCtrl-Close event\n\n" );
		return( TRUE ); 
		// Pass other signals to the next handler. 
	case CTRL_BREAK_EVENT: 
		if ( !AppExit )
			SendMessageA(*MyHWND, WM_CLOSE, 0, 0);
		AppExit = true;
		//ConsoleOut( "\nCtrl-Break event\n\n" );
		return FALSE; 
	case CTRL_LOGOFF_EVENT: 
		if ( !AppExit )
			SendMessageA(*MyHWND, WM_CLOSE, 0, 0);
		AppExit = true;
		//ConsoleOut( "\nCtrl-Logoff event\n\n" );
		return FALSE; 
	case CTRL_SHUTDOWN_EVENT: 
		if ( !AppExit )
			SendMessageA(*MyHWND, WM_CLOSE, 0, 0);
		AppExit = true;
		//ConsoleOut( "\nCtrl-Shutdown event\n\n" );
		return FALSE; 
	default: 
		return FALSE; 
	} 
} 

void CTorchlightCamModApp::LoadSettings()
{
	// SETTINGS
	DEBUG_MODE				= m_IniReader.getKeyValueAsUInt("Debug", "Settings", DEBUG_MODE );
	ENABLE_TILT				= m_IniReader.getKeyValueAsUInt("EnableTilt", "Settings", ENABLE_TILT );
	FORCE_MEMSEARCH			= m_IniReader.getKeyValueAsUInt("ForceMemSearch", "Settings", FORCE_MEMSEARCH );
	FORCE_VERSION   		= m_IniReader.getKeyValueAsUInt("ForceVersion", "Settings", FORCE_VERSION );
	TOGGLE_SAFEZONE			= m_IniReader.getKeyValueAsUInt("SafeZoneWhenToggled", "Settings", TOGGLE_SAFEZONE );
	ROTATE_SAFEZONE			= m_IniReader.getKeyValueAsUInt("SafeZoneWhenRotating", "Settings", ROTATE_SAFEZONE );
	ZOOM_DEFAULT			= m_IniReader.getKeyValueAsUInt("ZoomDefault", "Settings", ZOOM_DEFAULT );
	ZOOM_MIN				= m_IniReader.getKeyValueAsUInt("ZoomMin", "Settings", ZOOM_MIN );
	ZOOM_MAX				= m_IniReader.getKeyValueAsUInt("ZoomMax", "Settings", ZOOM_MAX );

	INVERT_MOUSE_ROTATION	= m_IniReader.getKeyValueAsUInt("InvertMouseRotation", "Settings", INVERT_MOUSE_ROTATION);
	AUTO_INVERT_MOUSE		= m_IniReader.getKeyValueAsUInt("AutoInvertMouse", "Settings", AUTO_INVERT_MOUSE);

	INVERT_DIRECT_ROTATION	= m_IniReader.getKeyValueAsUInt("InvertDirectRotation", "Settings", INVERT_DIRECT_ROTATION);
	AUTO_INVERT_DIRECT		= m_IniReader.getKeyValueAsUInt("AutoInvertDirect", "Settings", AUTO_INVERT_DIRECT);

	STARTUP_MODE    		= m_IniReader.getKeyValueAsUInt("StartupMode", "Settings", STARTUP_MODE);

	AUTO_RESET_ZOOM    		= m_IniReader.getKeyValueAsUInt("AutoResetZoom", "Settings", AUTO_RESET_ZOOM);
	AUTO_RESET_ROTATION		= m_IniReader.getKeyValueAsUInt("AutoResetRotation", "Settings", AUTO_RESET_ROTATION);
	AUTO_RESET_TILT   		= m_IniReader.getKeyValueAsUInt("AutoResetTilt", "Settings", AUTO_RESET_TILT);

	// rotation
	SPEED_MOUSE_ROTATE		= m_IniReader.getKeyValueAsUInt("MouseRotationSpeed", "Settings", SPEED_MOUSE_ROTATE );
	SPEED_KEYBOARD_ROTATE	= m_IniReader.getKeyValueAsUInt("KeyboardRotationSpeed", "Settings", SPEED_KEYBOARD_ROTATE );
	SPEED_KEYBOARD_TILT		= m_IniReader.getKeyValueAsUInt("KeyboardTiltSpeed", "Settings", SPEED_KEYBOARD_TILT );
	SPEED_MOUSE_DIRECT_ROTATE = m_IniReader.getKeyValueAsUInt("MouseDirectRotationSpeed", "Settings", SPEED_MOUSE_DIRECT_ROTATE );
	// zooming
	SPEED_KEYBOARD_ZOOM				= m_IniReader.getKeyValueAsUInt("KeyboardZoomSpeed", "Settings", SPEED_KEYBOARD_ZOOM );

	// KEY MAPPINGS
	KEY_ZOOMIN				= m_IniReader.getKeyValueAsUInt("ZoomIn", "KeyMappings", KEY_ZOOMIN);
	KEY_ZOOMOUT				= m_IniReader.getKeyValueAsUInt("ZoomOut", "KeyMappings", KEY_ZOOMOUT);
	KEY_ZOOMRESET			= m_IniReader.getKeyValueAsUInt("ZoomReset", "KeyMappings", KEY_ZOOMRESET);
	KEY_TILTUP				= m_IniReader.getKeyValueAsUInt("TiltUp", "KeyMappings", KEY_TILTUP);
	KEY_TILTDOWN			= m_IniReader.getKeyValueAsUInt("TiltDown", "KeyMappings", KEY_TILTDOWN);
	KEY_TILTRESET			= m_IniReader.getKeyValueAsUInt("TiltReset", "KeyMappings", KEY_TILTRESET);
	KEY_ROTATELEFT			= m_IniReader.getKeyValueAsUInt("RotateLeft", "KeyMappings", KEY_ROTATELEFT);
	KEY_ROTATERIGHT			= m_IniReader.getKeyValueAsUInt("RotateRight", "KeyMappings", KEY_ROTATERIGHT);
	KEY_ROTATERESET			= m_IniReader.getKeyValueAsUInt("RotateReset", "KeyMappings", KEY_ROTATERESET);
	KEY_MOUSEROTATE_TRACK	= m_IniReader.getKeyValueAsUInt("MouseRotate", "KeyMappings", KEY_MOUSEROTATE_TRACK);
	KEY_MOUSEROTATE_TOGGLE	= m_IniReader.getKeyValueAsUInt("MouseToggle", "KeyMappings", KEY_MOUSEROTATE_TOGGLE);
	KEY_MOUSEROTATE_DIRECT	= m_IniReader.getKeyValueAsUInt("MouseDirect", "KeyMappings", KEY_MOUSEROTATE_DIRECT);
	KEY_DISABLEALL			= m_IniReader.getKeyValueAsUInt("DisableFeatures", "KeyMappings", KEY_DISABLEALL);

	if ( KEY_ZOOMIN != 0x0 || KEY_ZOOMOUT != 0x0 || KEY_ZOOMRESET != 0x0 ) bZoomWanted = true;
	if ( KEY_ROTATELEFT != 0x0 || KEY_ROTATERIGHT != 0x0 || KEY_ROTATERESET != 0x0 ) bRotateWanted = true;
	if ( ENABLE_TILT && ( KEY_TILTUP != 0x0 || KEY_TILTDOWN != 0x0 || KEY_TILTRESET != 0x0 ) ) bTiltWanted = true;

	// COPY OF SETTING VARIABLE
	INVERT_MOUSE_ROTATION_SETTING = INVERT_MOUSE_ROTATION;
	INVERT_DIRECT_ROTATION_SETTING = INVERT_DIRECT_ROTATION;

	if ( KEY_MOUSEROTATE_TRACK == VK_CAPITAL || 
		KEY_MOUSEROTATE_TOGGLE == VK_CAPITAL ||
		KEY_MOUSEROTATE_DIRECT == VK_CAPITAL )
	{
		bCapslockUsed = true;
	}

	if (DEBUG_MODE) ConsoleOut("\nStatus: Reloaded settings\n");
	if ( m_plblStatus && IsWindow(m_plblStatus->m_hWnd) )
		m_plblStatus->SetWindowText("Status: Reloaded settings");
}

BOOL CTorchlightCamModApp::InitInstance()
{
#pragma region Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
#pragma endregion

	// Create a Mutex based on the file name.
	hMutex = CreateMutex(NULL, FALSE, "TorchlightCamMutex");

	// If the Mutex has already been created.
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		//AfxMessageBox("TorchlightCam already running!");
		return (false);
	}

	////////////////////////////////////////////////////////////////////
	//
	// # Set inifile
	//
	// Check if we have the TorchlightCam.ini file.
	if ( !FileExists( ".\\TorchlightCam.ini" ) )
	{
		MessageBox(0, "Warning: Settings file \"TorchlightCam.ini\" not found in current directory.\n\nWarning: Using default values so almost all features are now disabled.", "TorchlightCam - File not found.", 0);
	}

	m_IniReader.setINIFileName (".\\TorchlightCam.ini"); 

	LoadSettings();

	// Create debug window if DEBUG is enabled
	if (DEBUG_MODE) 
	{
		CreateConsoleWindow(80,25);
		SetConsoleCtrlHandler( (PHANDLER_ROUTINE)&CTorchlightCamModApp::ControlHandler, TRUE );
	}

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	// SetRegistryKey(_T("TorchlightCam"));

	CTorchlightCamModDlg dlg;

	MyHWND = &dlg.m_hWnd;
	m_pMainWnd = &dlg;
	m_mnuStatus = &dlg.m_mnuStatus;
	m_plblStatus = &dlg.m_lblStatus;		
	//dlg.m_CallBackReloadFunc = CTorchlightCamModApp::LoadSettings;
	dlg.m_IniReader = &m_IniReader;
	dlg.SetCallBack( CTorchlightCamModApp::LoadSettings );

	CTorchlightCamModThread TorchlightCamThread;
	TorchlightCamThread.m_plblStatus = &dlg.m_lblStatus;
	TorchlightCamThread.Start();

	INT_PTR nResponse = dlg.DoModal();

	AppExit = true;

	TorchlightCamThread.WaitUntilTerminate();

	ReleaseMutex(hMutex);
	CloseHandle(hMutex);

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}