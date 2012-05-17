
// TorchlightCamModDlg.cpp : implementation file
//
#include "stdafx.h"
#include "TorchlightCamMod.h"
#include "TorchlightCamModDlg.h"
#include "TorchlightCamMod_SystemTray.h"

// CTorchlightCamModDlg dialog

////////////////////////////////////////////////////////////////////////
// Constants

const char* kpcTrayNotificationMsg_ = "TorchlightCam tray notification";

CTorchlightCamModDlg::CTorchlightCamModDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTorchlightCamModDlg::IDD, pParent),
	bMinimized_(false),
	pTrayIcon_(0),
	nTrayNotificationMsg_(RegisterWindowMessage(kpcTrayNotificationMsg_))
{
	hIcon_ = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTorchlightCamModDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTorchlightCamModDlg)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_STATUS, m_lblStatus);
}

BEGIN_MESSAGE_MAP(CTorchlightCamModDlg, CDialog)
	//{{AFX_MSG_MAP(CTorchlightCamModDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOMMAND()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_COMMAND(IDC_ST_RESTORE, OnSTRestore)
	ON_COMMAND(IDC_ST_RELOADSETTINGS, OnSTReloadSettings)
	ON_COMMAND(IDC_ST_EXIT, OnSTExit)
	ON_COMMAND(IDC_ST_STARTASMINIMIZED, OnSTStartAsMinimized)
	ON_COMMAND(IDC_ST_SPEED_HIGH, OnSTSpeedHigh)
	ON_COMMAND(IDC_ST_SPEED_NORMAL, OnSTSpeedNormal)
	ON_COMMAND(IDC_ST_SPEED_LOW, OnSTSpeedLow)
	ON_COMMAND(IDC_ST_SAVESETTINGS, OnSTSaveSettings)
END_MESSAGE_MAP()

//// SetupTrayIcon /////////////////////////////////////////////////////
// If we're minimized, create an icon in the systray.  Otherwise, remove
// the icon, if one is present.

void CTorchlightCamModDlg::SetupTrayIcon()
{
	if (bMinimized_ && (pTrayIcon_ == 0)) {
		pTrayIcon_ = new CSystemTray;
		pTrayIcon_->Create(0, nTrayNotificationMsg_, "TorchlightCam",
			hIcon_, IDR_SYSTRAY_MENU, &m_mnuStatus);
	}
	else {
		delete pTrayIcon_;
		pTrayIcon_ = 0;
	}
}

void CTorchlightCamModDlg::TrayIconCreate()
{
	if (pTrayIcon_ == 0) {
		pTrayIcon_ = new CSystemTray;
		pTrayIcon_->Create(0, nTrayNotificationMsg_, "TorchlightCam",
			hIcon_, IDR_SYSTRAY_MENU, &m_mnuStatus);
	}
}

void CTorchlightCamModDlg::TrayIconDelete()
{
	if (pTrayIcon_ != 0) {
		delete pTrayIcon_;
		pTrayIcon_ = 0;
	}
}

//// SetupTaskBarButton ////////////////////////////////////////////////
// Show or hide the taskbar button for this app, depending on whether
// we're minimized right now or not.

void CTorchlightCamModDlg::SetupTaskBarButton()
{
	// Show or hide this window appropriately
	if (bMinimized_) {
		ShowWindow(SW_HIDE);
	}
	else {
		ShowWindow(SW_SHOW);
	}
}

// CTorchlightCamModDlg message handlers

BOOL CTorchlightCamModDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	this->SetWindowText("TorchlightCam");

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(hIcon_, TRUE);			// Set big icon
	SetIcon(hIcon_, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	TrayIconCreate();

	CClientDC dc(this);

	m_dcMem.CreateCompatibleDC( &dc );
	m_cbmp.LoadBitmap(IDB_BITMAP1);
	m_dcMem.SelectObject((HBITMAP)m_cbmp);

	char tmp[256];

	CString txtSpeed = m_IniReader->getKeyValue("Performance", "Settings");
	CString txtMinimized = m_IniReader->getKeyValue("Minimized", "Settings");

	sprintf(tmp, "%d", 1); if (txtSpeed == "") txtSpeed = tmp;
	sprintf(tmp, "%d", 0); if (txtMinimized == "") txtMinimized = tmp;

	//m_IniReader->setKey(txtSpeed, "Performance", "Settings");
	//m_IniReader->setKey(txtMinimized, "Minimized", "Settings");

	int tmpSpd = atoi(txtSpeed);

	int tmpMinimized = atoi(txtMinimized);

	m_mnuStatus.StartAsMinimized = tmpMinimized;

	switch (tmpSpd)
	{
	case 0:
		m_mnuStatus.SpeedHigh = false;
		m_mnuStatus.SpeedNormal = false;
		m_mnuStatus.SpeedLow = true;
		break;
	case 1:
		m_mnuStatus.SpeedHigh = false;
		m_mnuStatus.SpeedNormal = true;
		m_mnuStatus.SpeedLow = false;
		break;
	case 2:
		m_mnuStatus.SpeedHigh = true;
		m_mnuStatus.SpeedNormal = false;
		m_mnuStatus.SpeedLow = false;
		break;
	default:
		m_mnuStatus.SpeedHigh = false;
		m_mnuStatus.SpeedNormal = true;
		m_mnuStatus.SpeedLow = false;
	}

	if (::AfxGetApp()->m_nCmdShow & SW_MINIMIZE || tmpMinimized == 1) {
		// User wants app to start minimized, but that only affects the
		// main window.  So, minimize this window as well.
		PostMessage(WM_SYSCOMMAND, SC_MINIMIZE);
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTorchlightCamModDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// Reload the settings
	OnSTReloadSettings();

	// Call the original handler
	CDialog::OnLButtonDblClk(nFlags, point);
}

void CTorchlightCamModDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// Decide if minimize state changed
	bool bOldMin = bMinimized_;
	if (nID == SC_MINIMIZE) {
		bMinimized_ = true;
	}
	else if (nID == SC_RESTORE) {
		bMinimized_ = false;
	}

	CDialog::OnSysCommand(nID, lParam);

	if (bOldMin != bMinimized_) {
		// Minimize state changed.  Create the systray icon and do
		// custom taskbar button handling.
		//SetupTrayIcon();
		SetupTaskBarButton();
	}
}

void CTorchlightCamModDlg::OnContextMenu(CWnd *pWnd, CPoint pos)
{
	bool bExit = false;
	CMenu menu, *pSubMenu;
	CWnd* pTarget = AfxGetMainWnd();

	if (!menu.LoadMenu(IDR_SYSTRAY_MENU)) bExit = true;
	if (!bExit)
	{
		if (!(pSubMenu = menu.GetSubMenu(0))) bExit = true;
		if (!bExit)
		{
			//CWnd* pTarget = AfxGetMainWnd();

			// Make chosen menu item the default (bold font)

			menu.EnableMenuItem(IDC_ST_RESTORE, MF_GRAYED);

			if (m_mnuStatus.StartAsMinimized) menu.CheckMenuItem(IDC_ST_STARTASMINIMIZED, MF_CHECKED);
			if (m_mnuStatus.SpeedHigh) menu.CheckMenuItem(IDC_ST_SPEED_HIGH, MF_CHECKED);
			if (m_mnuStatus.SpeedNormal) menu.CheckMenuItem(IDC_ST_SPEED_NORMAL, MF_CHECKED);
			if (m_mnuStatus.SpeedLow) menu.CheckMenuItem(IDC_ST_SPEED_LOW, MF_CHECKED);

			// Display and track the popup menu
			pTarget->SetForegroundWindow();
			::TrackPopupMenu(pSubMenu->m_hMenu, 0, pos.x, pos.y, 0, pTarget->GetSafeHwnd(), NULL);

			//    // BUGFIX: See "PRB: Menus for Notification Icons Don't Work Correctly"
			//pTarget->PostMessage(WM_NULL, 0, 0);
			menu.DestroyMenu();
		}
	}

}

void CTorchlightCamModDlg::OnPaint()
{
	if (IsIconic())
	{
	}
	else
	{
		// Draw
		CPaintDC dc(this);
		dc.BitBlt(0, 0, 584, 400, &m_dcMem, 0, 0, SRCCOPY);
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTorchlightCamModDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(hIcon_);
}

BOOL CTorchlightCamModDlg::DestroyWindow()
{
	// Get rid of systray icon
	bMinimized_ = false;
	//SetupTrayIcon();
	TrayIconDelete();

	// # Notify the main application to start exiting.
	return CDialog::DestroyWindow();
}

void CTorchlightCamModDlg::OnSTSaveSettings()
{
	// saving settings.
	char txtSpeed[8];
	char txtMinimized[8];

	int tmpSpd;
	if (m_mnuStatus.SpeedLow) tmpSpd = 0;
	if (m_mnuStatus.SpeedNormal) tmpSpd = 1;
	if (m_mnuStatus.SpeedHigh) tmpSpd = 2;
	sprintf(txtSpeed, "%d", tmpSpd);

	int tmpMinimized = 0;
	if (m_mnuStatus.StartAsMinimized)
		tmpMinimized = 1;
	sprintf(txtMinimized, "%d", tmpMinimized);

	m_IniReader->setKey(txtSpeed, "Performance", "Settings");
	m_IniReader->setKey(txtMinimized, "Minimized", "Settings");
}

void CTorchlightCamModDlg::OnSTSpeedHigh()
{
	m_mnuStatus.SpeedHigh = true;
	m_mnuStatus.SpeedNormal = false;
	m_mnuStatus.SpeedLow = false;
}

void CTorchlightCamModDlg::OnSTSpeedNormal()
{
	m_mnuStatus.SpeedHigh = false;
	m_mnuStatus.SpeedNormal = true;
	m_mnuStatus.SpeedLow = false;
}

void CTorchlightCamModDlg::OnSTSpeedLow()
{
	m_mnuStatus.SpeedHigh = false;
	m_mnuStatus.SpeedNormal = false;
	m_mnuStatus.SpeedLow = true;
}

void CTorchlightCamModDlg::OnSTStartAsMinimized()
{
	if ( m_mnuStatus.StartAsMinimized )
		m_mnuStatus.StartAsMinimized = false;
	else
		m_mnuStatus.StartAsMinimized = true;
}

void CTorchlightCamModDlg::OnSTExit()
{
	// # User selected exit.
	OnCancel();
}

void CTorchlightCamModDlg::OnSTRestore()
{
	ShowWindow(SW_RESTORE);
	bMinimized_ = false;
	SetupTaskBarButton();
}

void CTorchlightCamModDlg::OnSTReloadSettings()
{
	// TODO: Add reload settings code here
	//       Need to callback to the running thread to make the loading.
	//MessageBox("Reloading settings..", "Reloading settings...",0 );
	//(MemFunc*)(m_CallBackReloadFunc)();
	m_pCallBackReloadFunc();
}

void CTorchlightCamModDlg::SetCallBack( void (*pt2Function)() )
{
	// TODO: Add reload settings code here
	//       Need to callback to the running thread to make the loading.
	//MessageBox("Reloading settings..", "Reloading settings...",0 );
	//(MemFunc*)(m_CallBackReloadFunc)();
	m_pCallBackReloadFunc = pt2Function;
}
