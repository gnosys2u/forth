//////////////////////////////////////////////////////////////////////
//
// ForthBlankDlg.cpp: implementation of Forth dialog support
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ForthBlankDlg.h"

#include "../ForthLib/ForthShell.h"
#include "../ForthLib/ForthInput.h"
#include "../ForthLib/ForthEngine.h"

ForthBlankDlg::ForthBlankDlg() : CDialog(ForthBlankDlg::IDD)
{
	//{{AFX_DATA_INIT(ForthBlankDlg)
	//}}AFX_DATA_INIT
    Create( ForthBlankDlg::IDD );
}

ForthBlankDlg::~ForthBlankDlg()
{
}


//////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(ForthBlankDlg, CDialog)
	//{{AFX_MSG_MAP(ForthBlankDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////////

void ForthBlankDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BOOL ForthBlankDlg::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
    TRACE( "ForthBlankDlg::OnCmdMsg id %x code %x\n", nID, nCode );
	if (CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

#if 0

	if (CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	if ((nCode != CN_COMMAND && nCode != CN_UPDATE_COMMAND_UI) ||
			!IS_COMMAND_ID(nID) || nID >= 0xf000)
	{
		// control notification or non-command button or system command
		return FALSE;       // not routed any further
	}

	// if we have an owner window, give it second crack
	CWnd* pOwner = GetParent();
	if (pOwner != NULL)
	{
		TRACE(traceCmdRouting, 1, "Routing command id 0x%04X to owner window.\n", nID);

		ASSERT(pOwner != this);
		if (pOwner->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
	}

	// last crack goes to the current CWinThread object
	CWinThread* pThread = AfxGetThread();
	if (pThread != NULL)
	{
		TRACE(traceCmdRouting, 1, "Routing command id 0x%04X to app.\n", nID);

		if (pThread->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
	}

	TRACE(traceCmdRouting, 1, "IGNORING command id 0x%04X sent to %hs dialog.\n", nID,
			GetRuntimeClass()->m_lpszClassName);

#endif
	return FALSE;
}
