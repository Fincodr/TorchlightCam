
// TorchlightCamMod.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

struct MNUSTATUS
{
	bool Silent;
	bool Invisible;
	bool SpeedHigh;
	bool SpeedNormal;
	bool SpeedLow;
	bool StartAsMinimized;
};

// CTorchlightCamModApp:
// See TorchlightCamMod.cpp for the implementation of this class
//

class CTorchlightCamModApp : public CWinAppEx
{
public:
	CTorchlightCamModApp();

// Overrides
	public:
	virtual BOOL InitInstance();
	static BOOL ControlHandler( DWORD fdwCtrlType );
	static void LoadSettings();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CTorchlightCamModApp theApp;