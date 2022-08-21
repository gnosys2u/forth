// ForthGuiDlg.h : header file
//

#include "afxcmn.h"
#if !defined(AFX_FORTHGUIDLG_H__FC416F88_AC2E_46DC_A696_CF8F972408E7__INCLUDED_)
#define AFX_FORTHGUIDLG_H__FC416F88_AC2E_46DC_A696_CF8F972408E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../ForthLib/ForthShell.h"
#include <deque>

class ForthShell;
class ForthBufferInputStream;

#define INPUT_BUFFER_SIZE 1024

/////////////////////////////////////////////////////////////////////////////
// CForthGuiDlg dialog

class CForthGuiDlg : public CDialog
{
// Construction
public:
	CForthGuiDlg(CWnd* pParent = NULL);	// standard constructor
	~CForthGuiDlg();

    void            RunForthThreadLoop();

// Dialog Data
	//{{AFX_DATA(CForthGuiDlg)
	enum { IDD = IDD_FORTHGUI_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CForthGuiDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void			CreateForth();
    void            CreateDialogOps();
	void			DestroyForth();
    OpResult	ProcessLine( char* pLine );
    void            QueueInputLine(const char* pLine);
	void			SetupTabbedPane( int tabNum, int tabID, LPSTR tabText );

	HICON m_hIcon;

    ForthShell*                 mpShell;
    ForthBufferInputStream*     mpInStream;
	ForthObject					mConsoleOutObject;
    char                        mInBuffer[INPUT_BUFFER_SIZE];
	int							mSelectedTab;
    HANDLE                      mInputSemaphore;
    CRITICAL_SECTION            mInputCriticalSection;
    std::deque<CString>         mInputQueue;

#define NUM_TABBED_PANES 4
	CRichEditCtrl*				mTabbedPanes[ NUM_TABBED_PANES ];
	// Generated message map functions
	//{{AFX_MSG(CForthGuiDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOk();
	CRichEditCtrl mOutputEdit;
	afx_msg void OnTcnSelchangeTabScript(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonRun();
	afx_msg void OnBnClickedButtonSave();
	afx_msg void OnBnClickedButtonLoad();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FORTHGUIDLG_H__FC416F88_AC2E_46DC_A696_CF8F972408E7__INCLUDED_)
