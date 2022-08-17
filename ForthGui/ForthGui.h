// ForthGui.h : main header file for the FORTHGUI application
//

#if !defined(AFX_FORTHGUI_H__274153D8_F4D9_4338_8B89_361F009B7AE8__INCLUDED_)
#define AFX_FORTHGUI_H__274153D8_F4D9_4338_8B89_361F009B7AE8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'pch.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CForthGuiApp:
// See ForthGui.cpp for the implementation of this class
//

class CForthGuiApp : public CWinApp
{
public:
	CForthGuiApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CForthGuiApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CForthGuiApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FORTHGUI_H__274153D8_F4D9_4338_8B89_361F009B7AE8__INCLUDED_)
