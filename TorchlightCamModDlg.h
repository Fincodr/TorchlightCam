
// TorchlightCamModDlg.h : header file
//

#pragma once
#include "Ini.h"

class CSystemTray;

// CTorchlightCamModDlg dialog
class CTorchlightCamModDlg : public CDialog
{
// Construction
public:
	CTorchlightCamModDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TORCHLIGHTCAMMOD_DIALOG };

// Implementation
protected:
	CDC m_dcMem;            // Compatible Memory DC for dialog
	CBitmap m_cbmp;
	int* AppExit;

	//// Overrides
	//{{AFX_VIRTUAL(CTorchlightCamModDlg)
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL DestroyWindow();
	//}}AFX_VIRTUAL

	//// Message map
	//{{AFX_MSG(CTorchlightCamModDlg)
	virtual BOOL OnInitDialog();
	afx_msg HCURSOR OnQueryDragIcon();
	void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint pos);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSTReloadSettings();
	afx_msg void OnSTRestore();
	afx_msg void OnSTExit();
	afx_msg void OnSTStartAsMinimized();
	afx_msg void OnSTSpeedHigh();
	afx_msg void OnSTSpeedNormal();
	afx_msg void OnSTSpeedLow();
	afx_msg void OnSTSaveSettings();
	DECLARE_MESSAGE_MAP()
	//}}AFX_MSG

	//// Internal support functions
	void SetupTrayIcon();
	void SetupTaskBarButton();
	void TrayIconCreate();
	void TrayIconDelete();

	//// Internal data
	HICON hIcon_;
	bool bMinimized_;
	CSystemTray* pTrayIcon_;
	int nTrayNotificationMsg_;
public:
	CStatic m_lblStatus;
	MNUSTATUS m_mnuStatus;
	CIniReader *m_IniReader;
	typedef void (*pt2Function)();
	pt2Function m_pCallBackReloadFunc;
	void SetCallBack(void (*pt2Function)());
};
