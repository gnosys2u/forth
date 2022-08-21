// ForthGuiDlg.cpp : implementation file
//

#include "pch.h"
#include "ForthGui.h"
#include "ForthGuiDlg.h"
#include "ForthBlankDlg.h"

#include "../ForthLib/ForthShell.h"
#include "../ForthLib/ForthInput.h"
#include "../ForthLib/ForthEngine.h"
#include ".\forthguidlg.h"

#include "anchor.h"
#include "dlgman.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDlgAnchor dlgAnchor;

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
///
//  trace output to logger client via pipe
//
static HANDLE hLoggingPipe = INVALID_HANDLE_VALUE;

void OutputToLogger(const char* pBuffer)
{
	//OutputDebugString(buffer);

	DWORD dwWritten;
	int bufferLen = 1 + strlen(pBuffer);

	if (hLoggingPipe == INVALID_HANDLE_VALUE)
	{
		hLoggingPipe = CreateFile(TEXT("\\\\.\\pipe\\ForthLogPipe"),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
	}
	if (hLoggingPipe != INVALID_HANDLE_VALUE)
	{
		WriteFile(hLoggingPipe,
			pBuffer,
			bufferLen,   // = length of string + terminating '\0' !!!
			&dwWritten,
			NULL);

		//CloseHandle(hPipe);
	}

	return;
}

/////////////////////////////////////////////////////////////////////////////
///
//  stuff to connect Forth output to output pane edit control
//
struct sStreamInScratch
{
	sStreamInScratch( const char* buffer )
		:	m_buffer( buffer )
		,	m_length( strlen( buffer ) )
	{}
	const char* m_buffer;
	int			m_length;
};

// My callback procedure that writes the rich edit control contents
// to a file.
static DWORD CALLBACK 
FileStreamInCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
    CFile* pFile = (CFile*) dwCookie;
    *pcb = pFile->Read(pbBuff, cb);
    return 0;
}


DWORD CALLBACK BufferStreamInCallback( DWORD dwCookie, LPBYTE outBuffer, LONG outBufferLen, LONG* bytesWritten )
{
	sStreamInScratch* const scratch = reinterpret_cast<sStreamInScratch*>( dwCookie );
	*bytesWritten = min( outBufferLen, scratch->m_length );		

	memcpy( outBuffer, scratch->m_buffer, *bytesWritten );
	
	scratch->m_length -= *bytesWritten;
	scratch->m_buffer += *bytesWritten;

	return 0;
}

void StreamToOutputPane( CRichEditCtrl* pOutEdit, const char* pMessage )
{
	if ( pOutEdit == NULL )
	{
		return;
	}
	// Grab the old selection and cursor placement.
	CHARRANGE oldSelection;
	pOutEdit->GetSel( oldSelection );
	const int32_t ixStart = pOutEdit->GetWindowTextLength();
	const int oldLine = pOutEdit->LineFromChar( oldSelection.cpMax );
	const int numLines = pOutEdit->GetLineCount();

	// Move to the end of the text.
	pOutEdit->SetSel( ixStart, -1 );

	// Stream in the message text.
	EDITSTREAM es;
	sStreamInScratch scratch( pMessage );
	es.dwCookie = reinterpret_cast<DWORD>( &scratch );
	es.pfnCallback = BufferStreamInCallback;
	es.dwError = 0;
	pOutEdit->StreamIn( SF_TEXT | SFF_SELECTION, es );

	// Scroll the window if the cursor was on the last line and the selection was empty. Otherwise, reset the selection.
	if ( oldSelection.cpMin != oldSelection.cpMax || pOutEdit->LineFromChar( oldSelection.cpMin ) != (numLines - 1) )
	{
		pOutEdit->SetSel( oldSelection );
	}
	else
	{
		pOutEdit->SetSel( -1, -1 );
		pOutEdit->LineScroll( pOutEdit->GetLineCount() - oldLine );
	}
}

CRichEditCtrl* gpOutEdit = NULL;
void ForthOutRoutine(ForthCoreState* pCore, void *pCBData, const char *pBuff)
{
	StreamToOutputPane( gpOutEdit, pBuff );
}

//extern void OutputToLogger(const char* pBuffer);
void ForthTraceOutRoutine(void *pCBData, const char* pFormat, va_list argList)
{
#ifdef LINUX
	char buffer[1000];
#else
	TCHAR buffer[1000];
#endif

	ForthEngine* pEngine = ForthEngine::GetInstance();
#ifdef LINUX
	vsnprintf(buffer, sizeof(buffer), pFormat, argList);
#else
	wvnsprintf(buffer, sizeof(buffer), pFormat, argList);
#endif
	//OutputToLogger(buffer);
	//StreamToOutputPane( (CRichEditCtrl *) pCBData, buffer );
}

/////////////////////////////////////////////////////////////////////////////
//#define TEST_MIDI
#ifdef TEST_MIDI

#include <MMSystem.h>
HMIDIIN MidiInHandle = 0;
#define MIDI_IN_DEVICENUM 8

/* ****************************** closeMidiIn() *****************************
 * Close MIDI In Device if it's open.
 ************************************************************************** */

DWORD closeMidiIn(void)
{
   DWORD   err;

   /* Is the device open? */
   if ((err = (DWORD)MidiInHandle))
   {
      /* Unqueue any buffers we added. If you don't
      input System Exclusive, you won't need this */
      midiInReset(MidiInHandle);

      /* Close device */
      if (!(err = midiInClose(MidiInHandle)))
      {
         /* Clear handle so that it's safe to call closeMidiIn() anytime */
         MidiInHandle = 0;
      }
   }

   /* Return the error */
   return(err);
}

void CALLBACK midiInputEvt( HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    TRACE( "ForthMidiExtension::HandleMidiIn   msg %x   cbData %x   p1 %x   p2 %x\n",
            wMsg, dwInstance, dwParam1, dwParam2 );
    switch ( wMsg )
    {
    case MIM_DATA:      // Short message received
        TRACE( "Data\n" );
        break;

    case MIM_ERROR:     // Invalid short message received
        TRACE( "Error\n" );
        break;

    case MIM_LONGDATA:  // System exclusive message received
        TRACE( "LongData\n" );
        break;

    case MIM_LONGERROR: // Invalid system exclusive message received
        TRACE( "LongError\n" );
        break;

    case MIM_OPEN:
        TRACE( "Input Open\n" );
        break;

    case MIM_CLOSE:
        TRACE( "Input Close\n" );
        break;

    case MOM_OPEN:
        TRACE( "Output Open\n" );
        break;

    case MOM_CLOSE:
        TRACE( "Output Close\n" );
        break;

    case MOM_DONE:
        TRACE( "Output Done\n" );
        break;
    }
}

/* *************************** openMidiIn() *****************************
 * Opens MIDI In Device #0. Stores handle in MidiInHandle. Starts
 * recording. (midiInputEvt is my callback to process input).
 * Returns 0 if success. Otherwise, an error number.
 * Use midiInGetErrorText to retrieve an error message.
 ************************************************************************ */

DWORD openMidiIn( UINT deviceId )
{
   DWORD   err;

   /* Is it not yet open? */
   if (!MidiInHandle)
   {
      if (!(err = midiInOpen(&MidiInHandle, deviceId, (DWORD)midiInputEvt, 0, CALLBACK_FUNCTION)))
      {
         /* Start recording Midi and return if SUCCESS */
         if (!(err = midiInStart(MidiInHandle)))
         {
            return(0);
         }
      }

      /* ============== ERROR ============== */

      /* Close MIDI In and zero handle */
      closeMidiIn();

      /* Return the error */
      return(err);
   }

   return(0);
}


#endif

/////////////////////////////////////////////////////////////////////////////
// CForthGuiDlg dialog

CForthGuiDlg::CForthGuiDlg(CWnd* pParent /*=NULL*/)
: CDialog(CForthGuiDlg::IDD, pParent)
, mpShell( NULL )
, mpInStream( NULL )
, mSelectedTab( 0 )
{
	//{{AFX_DATA_INIT(CForthGuiDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	mConsoleOutObject.pData = NULL;
	mConsoleOutObject.pMethodOps = NULL;

    InitializeCriticalSection(&mInputCriticalSection);
    // default security attributes, initial count, max count, unnamed semaphore
    mInputSemaphore = CreateSemaphore(nullptr, 0, 0x7FFFFFFF, nullptr);
}

CForthGuiDlg::~CForthGuiDlg()
{
	DestroyForth();
    DeleteCriticalSection(&mInputCriticalSection);
    CloseHandle(mInputSemaphore);
    // TODO: kill worker thread?
}

UINT ForthWorkerThread(LPVOID pParam)
{
    ((CForthGuiDlg*)pParam)->RunForthThreadLoop();
    return 0;
}

void CForthGuiDlg::RunForthThreadLoop()
{
    // loop, waiting on input semaphore and processing lines until 'bye'
    OpResult result = OpResult::kResultOk;
    while (result != OpResult::kResultExitShell && result != OpResult::kResultShutdown)
    {
        DWORD waitResult = WaitForSingleObject(mInputSemaphore, INFINITE);
        if (waitResult != WAIT_OBJECT_0)
        {
            printf("ForthWorkerThread WaitForSingleObject failed (%d)\n", GetLastError());
        }
        else
        {
            CRichEditCtrl *pOutEdit = (CRichEditCtrl*)GetDlgItem(IDC_RICHEDIT_OUTPUT);
            StreamToOutputPane(pOutEdit, mInBuffer);
            StreamToOutputPane(pOutEdit, "\r\n");

            char* pBuffer = mpShell->AddToInputLine(&(mInBuffer[0]));
            if (mpShell->InputLineReadyToProcess())
            {
                result = ProcessLine(pBuffer);
            }
            StreamToOutputPane(pOutEdit, "\r\n");
            // re-enable user text input
            CButton* pOkButton = (CButton *)GetDlgItem(IDOK);
            pOkButton->EnableWindow(true);
        }
    }
    OnCancel();
}

void CForthGuiDlg::CreateForth()
{
	char autoloadBuffer[64];
    mpShell = new ForthShell(__argc, const_cast<const char **>(__argv), const_cast<const char **>(_environ));
	ForthEngine* pEngine = mpShell->GetEngine();
	ForthCoreState* pCore = pEngine->GetCoreState();
	CreateForthFunctionOutStream( pCore, mConsoleOutObject, NULL, NULL, ForthOutRoutine, GetDlgItem( IDC_RICHEDIT_OUTPUT ) );

	CreateDialogOps();

	pEngine->SetDefaultConsoleOut(mConsoleOutObject );
	pEngine->ResetConsoleOut(*pCore);
	//pEngine->SetTraceOutRoutine(ForthTraceOutRoutine, GetDlgItem(IDC_RICHEDT_DEBUG));
	mInBuffer[0] = '\0';
    mpInStream = new ForthBufferInputStream( mInBuffer, INPUT_BUFFER_SIZE, true );
    mpShell->GetInput()->PushInputStream( mpInStream );
	strncpy(autoloadBuffer, "\"forth_autoload.txt\" $load", sizeof(autoloadBuffer) - 1);
	ProcessLine(autoloadBuffer);

    AfxBeginThread(ForthWorkerThread, this);

#ifdef TEST_MIDI
    /* Open MIDI In */
	MIDIINCAPS deviceCaps;
	UINT deviceId = MIDI_IN_DEVICENUM;
	MMRESULT result = midiInGetDevCaps( deviceId, &deviceCaps, sizeof(MIDIINCAPS) );
    if ( openMidiIn( deviceId ) == 0 )
    {

        char buffy[256];
        gets( &(buffy[0]) );

        /* Close any Midi Input device */
        closeMidiIn();
    }
#endif
}

void CForthGuiDlg::DestroyForth()
{
	if ( mpShell )
	{
		ForthEngine* pEngine = mpShell->GetEngine();
		pEngine->ReleaseObject(pEngine->GetCoreState(), mConsoleOutObject);
		delete mpShell;
		// the shell destructor deletes all the streams on the input stack, including mpInStream
		mpShell = NULL;
		mpInStream = NULL;
	}
}

void CForthGuiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CForthGuiDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_RICHEDIT_OUTPUT, mOutputEdit);
}

BEGIN_MESSAGE_MAP(CForthGuiDlg, CDialog)
	//{{AFX_MSG_MAP(CForthGuiDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_SCRIPT, &CForthGuiDlg::OnTcnSelchangeTabScript)
	ON_BN_CLICKED(IDC_BUTTON_RUN, &CForthGuiDlg::OnBnClickedButtonRun)
	ON_BN_CLICKED(IDC_BUTTON_SAVE, &CForthGuiDlg::OnBnClickedButtonSave)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, &CForthGuiDlg::OnBnClickedButtonLoad)
	ON_WM_SIZE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CForthGuiDlg message handlers

BOOL CForthGuiDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	gpOutEdit = static_cast<CRichEditCtrl*>(GetDlgItem( IDC_RICHEDIT_OUTPUT ));

    dlgAnchor.Init( m_hWnd );

    dlgAnchor.Add( IDC_RICHEDIT_OUTPUT, ANCHOR_ALL );

    dlgAnchor.Add( IDC_BUTTON_RUN, ANCHOR_TOPRIGHT );
    dlgAnchor.Add( IDC_BUTTON_LOAD, ANCHOR_TOPRIGHT );
    dlgAnchor.Add( IDC_BUTTON_SAVE, ANCHOR_TOPRIGHT );

    dlgAnchor.Add( IDC_TAB_SCRIPT, ANCHOR_TOPLEFT );
    dlgAnchor.Add( IDC_EDIT_INPUT, ANCHOR_BOTTOMLEFT | ANCHOR_RIGHT );
    dlgAnchor.Add( IDCANCEL, ANCHOR_BOTTOMRIGHT );
    dlgAnchor.Add( IDOK, ANCHOR_BOTTOMRIGHT );

	SetupTabbedPane( 0, IDC_RICHEDT_SCRIPT,		"Script 1" );
	SetupTabbedPane( 1, IDC_RICHEDT_SCRIPT2,	"Script 2" );
	SetupTabbedPane( 2, IDC_RICHEDT_SCRATCH,	"Scratch" );
	SetupTabbedPane( 3, IDC_RICHEDT_DEBUG,		"Trace" );

	CreateForth();

	// Set the input focus to the input edit box
	GotoDlgCtrl(GetDlgItem(IDC_EDIT_INPUT));
	
	return FALSE;  // return TRUE  unless you set the focus to a control
}

void CForthGuiDlg::SetupTabbedPane( int tabNum, int tabID, LPSTR tabText )
{
	CTabCtrl* pTabCtrl = (CTabCtrl *) GetDlgItem( IDC_TAB_SCRIPT );

	TCITEM tabItem;
	tabItem.mask = TCIF_PARAM | TCIF_TEXT;
	tabItem.lParam = tabNum;
	tabItem.pszText = tabText;
	tabItem.iImage = -1;
	tabItem.cchTextMax = strlen( tabItem.pszText );
	pTabCtrl->InsertItem( 0, &tabItem );

    dlgAnchor.Add( tabID, ANCHOR_TOP | ANCHOR_LEFT | ANCHOR_RIGHT );

	CRichEditCtrl* pEdit = (CRichEditCtrl*) GetDlgItem( tabID );
	mTabbedPanes[tabNum] = pEdit;
}

void CForthGuiDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CForthGuiDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CForthGuiDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CForthGuiDlg::OnBnClickedOk()
{
    CEdit *pEdit = (CEdit* )GetDlgItem( IDC_EDIT_INPUT );
	pEdit->GetWindowText( mInBuffer, INPUT_BUFFER_SIZE - 4 );

    CButton* pOkButton = (CButton *)GetDlgItem(IDOK);
    pOkButton->EnableWindow(false);

    // clear input buffer
    pEdit->SetSel(0, -1);
    pEdit->ReplaceSel("");

    //EnterCriticalSection(&mInputCriticalSection);
    // increment the semaphore to tell forth worker thread to process mInBuffer
    if (!ReleaseSemaphore(mInputSemaphore, 1, NULL))
    {
        printf("CForthGuiDlg::OnBnClickedOk - ReleaseSemaphore error: %d\n", GetLastError());
    }
    //LeaveCriticalSection(&mInputCriticalSection);
}

OpResult CForthGuiDlg::ProcessLine( char* pLine )
{
	ForthInputStack* pInput = mpShell->GetInput();
    OpResult result = OpResult::kResultOk;

	//pEdit->GetWindowText( mInBuffer, INPUT_BUFFER_SIZE - 4 );
	char* pEndLine = strchr( pLine, '\r' );
	if ( pEndLine != NULL )
	{
		*pEndLine = '\0';
	}
    result = mpShell->ProcessLine( pLine );

	if ( result == OpResult::kResultOk )
	{
		// to implement the "load" op, we must do 
		while ( pInput->InputStream() != mpInStream )
		{
			// get a line
			// interpret the line
			bool bQuit = false;
			const char* pBuffer = pInput->GetLine( "" );
			if ( pBuffer == NULL )
			{
				bQuit = pInput->PopInputStream();
			}

			if ( !bQuit )
			{
                char* pLine = mpShell->AddToInputLine(pBuffer);
                if (mpShell->InputLineReadyToProcess())
                {
                    result = mpShell->ProcessLine(pLine);
                }
			}
			if ( bQuit || (result != OpResult::kResultOk) )
			{
				break;
			}
		}
	}
	switch ( result )
	{
	case OpResult::kResultFatalError:
		DestroyForth();
		CreateForth();
		break;

	default:
		break;
	}
    return result;
}

FORTHOP( makeDialogOp )
{
    ForthBlankDlg* pDialog = new ForthBlankDlg;
    SPUSH( ((int32_t) pDialog) );
}

baseDictionaryEntry dialogDict[] =
{
    OP_DEF( makeDialogOp, "makeDialog" ),
    // following must be last in table
    OP_DEF(     NULL,                   "" )
};


void CForthGuiDlg::CreateDialogOps()
{
	ForthEngine* pEngine = mpShell->GetEngine();
    pEngine->AddBuiltinOps( dialogDict );
}

/*
    CButton gButton1;
    gButton1.Create(_T("My button"), WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 
       CRect(10,10,150,30), this, 0x4000 );
    gButton1.SetWindowText("Your button");
    gButton1.SetCheck( 1 );
*/


void CForthGuiDlg::OnTcnSelchangeTabScript(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	mSelectedTab = TabCtrl_GetCurSel( pNMHDR->hwndFrom );
	CRichEditCtrl *pEdit = NULL;

	for ( int i = 0; i < NUM_TABBED_PANES; i++ )
	{
		mTabbedPanes[i]->ShowWindow( i == mSelectedTab );
	}
}

void CForthGuiDlg::OnBnClickedButtonRun()
{
	// TODO: Add your control notification handler code here
	CRichEditCtrl* pEdit = mTabbedPanes[ mSelectedTab ];
	int numLines = pEdit->GetLineCount();
	for ( int i = 0; i < numLines; i++ )
	{
		pEdit->GetLine( i, &(mInBuffer[0]), sizeof(mInBuffer) );
		ProcessLine( &(mInBuffer[0]) );
	}
}

void CForthGuiDlg::OnBnClickedButtonSave()
{
	// TODO: Add your control notification handler code here
}

void CForthGuiDlg::OnBnClickedButtonLoad()
{
    CFileDialog dlg( TRUE, "txt", "*.*" );
    if ( dlg.DoModal() != IDOK ) {
        return;
    }

    CString pathname = dlg.GetFileName();
    CFile inFile( pathname, CFile::modeRead );
    EDITSTREAM es;

    es.dwCookie = (DWORD) &inFile;
    es.pfnCallback = FileStreamInCallback; 
	CRichEditCtrl* pEdit = mTabbedPanes[ mSelectedTab ];
    pEdit->StreamIn( SF_TEXT, es );
    pEdit->Invalidate();
    //int topLine = mScoreEdit.GetFirstVisibleLine();
    //mScoreEdit.LineScroll( mTrackLines[ mSoloSpinner ] - topLine );
}

void CForthGuiDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

    dlgAnchor.OnSize();
}
