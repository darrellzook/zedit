#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"
#include <shlobj.h>
#include <sys/stat.h>


/*----------------------------------------------------------------------------------------------
	Put this block before any other global variables on the stack so the
	destructor is the last thing that happens when the program is shut down.
----------------------------------------------------------------------------------------------*/
#ifdef _DEBUG
#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC
class CatchMemoryLeak
{
public:
	~CatchMemoryLeak()
		{ _CrtDumpMemoryLeaks(); }
};
CatchMemoryLeak g_cml;
#endif
/*--------------------------------------------------------------------------------------------*/

const char * g_pszAppName = "ZEdit";
#ifdef _DEBUG
const char * g_pszSemaphoreName = "DRZ_ZEdit_Debug";
#else
const char * g_pszSemaphoreName = "DRZ_ZEdit";
#endif

const char * g_pszFileType[] = {
	"ANSI",
	"UTF-8",
	"UTF-16",
	"Binary",
	"Default"
};
char * g_kpszDelimiters = "~!@%^&*()-+=|\\/{}[]:;\"'<> ,\t.?";

CZEditApp * g_pzea = NULL;
CColorSchemes g_cs;
CFileGlobals g_fg;


/*----------------------------------------------------------------------------------------------
	The main entry into the program.
----------------------------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	return CZEditApp::WinMain(hInstance, hPrevInstance, szCmdLine, iCmdShow);
}

/***********************************************************************************************
	CZEditApp methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	The main entry into the program.
----------------------------------------------------------------------------------------------*/
int WINAPI CZEditApp::WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine,
	int iCmdShow)
{
	// See if we have another instance running already.
	HANDLE hSem = ::CreateSemaphore(NULL, 0, 1, g_pszSemaphoreName);
	if (hSem != NULL && ::GetLastError() == ERROR_ALREADY_EXISTS)
    {
		::CloseHandle(hSem);
		HWND hwnd = ::FindWindow(g_pszAppName, NULL);

		// Allow anybody to grab the foregound focus.
		// If this isn't done, the other process won't get the focus.
		PfnAllowSetForegroundWindow pfnAllowSetForegroundWindow = NULL;
		HMODULE hmodUser = ::LoadLibrary("user32.dll");
		if (hmodUser)
		{
			pfnAllowSetForegroundWindow = (PfnAllowSetForegroundWindow)
				::GetProcAddress(hmodUser, "AllowSetForegroundWindow");
			::FreeLibrary(hmodUser);
		}
		if (pfnAllowSetForegroundWindow)
			(*pfnAllowSetForegroundWindow)(ASFW_ANY);

		if (szCmdLine && *szCmdLine != 0)
		{
			int cch = lstrlen(szCmdLine) + 2;
			char * pszFilename = new char[cch];
			if (pszFilename)
			{
				::SetForegroundWindow(hwnd);
				if (::IsIconic(hwnd))
					::ShowWindow(hwnd, SW_RESTORE);

				lstrcpy(pszFilename, szCmdLine);
				g_fg.FixString(pszFilename);
				COPYDATASTRUCT cps;
				cps.lpData = pszFilename;
				cps.cbData = cch;
				::SendMessage(hwnd, WM_COPYDATA, NULL, (LPARAM)&cps);
				delete pszFilename;
			}
			else
			{
				// This will open a new top-level window.
				::SendMessage(hwnd, WM_COMMAND, MAKELONG(ID_FILE_NEW, 0), -1);
			}
		}
		else
		{
			// This will open a new top-level window.
			::SendMessage(hwnd, WM_COMMAND, MAKELONG(ID_FILE_NEW, 0), -1);
		}

		return 0;
    }

	if (FAILED(::OleInitialize(NULL)))
	{
		::MessageBox(NULL, "Could not initialize COM libraries.", g_pszAppName,
			MB_OK | MB_ICONSTOP);
		return 0;
	}

#ifdef _DEBUG
	if (0)
	{
		//int iztl = g_fg.LoadTypeLibrary(L"C:\\Program Files\\Common Files\\system\\ado\\msado15.dll");
		int iztl = g_fg.LoadTypeLibrary(L"C:\\Program Files\\Microsoft Visual Studio\\Common\\IDE\\IDE98\\asp.tlb");
#if 0
		if (iztl >= 0)
		{
			ZTypeLibrary & ztl = g_fg.GetTypeLibrary(iztl);
			const ZTypeInfoMap & mapTypeInfo = ztl.GetTypeInfoMap();
			ZTypeInfoMap::const_iterator pos = mapTypeInfo.begin();
			int cTypeInfo = mapTypeInfo.size();
			while (pos != mapTypeInfo.end())
			{
				ZTypeInfo * pzti = pos->second;
				AssertPtr(pzti);
				::OutputDebugString("Class: ");
				::OutputDebugStringW(pzti->m_sName);
				::OutputDebugString("\n");

				int cMethods = pzti->m_vpmsMethod.size();
				MyStringSet::iterator posT = pzti->m_vpmsMethod.begin();
				while (posT != pzti->m_vpmsMethod.end())
				{
					::OutputDebugString("   Method: ");
					AssertPtr(*posT);
					::OutputDebugStringW(**posT);
					::OutputDebugString("\n");
					posT++;
				}

				int cAttributes = pzti->m_vpmsAttribute.size();
				posT = pzti->m_vpmsAttribute.begin();
				while (posT != pzti->m_vpmsAttribute.end())
				{
					::OutputDebugString("   Attribute: ");
					AssertPtr(*posT);
					::OutputDebugStringW(**posT);
					::OutputDebugString("\n");
					posT++;
				}
				pos++;
			}

			try
			{
				//MyString b(L"Response");
				ZTypeInfoMap::const_iterator pos = mapTypeInfo.find(L"Response");
				if (pos != mapTypeInfo.end())
				{
					ZTypeInfo * pzti = pos->second;
					AssertPtr(pzti);
					MyStringSet & msvMethod = pzti->m_vpmsMethod;
					int cMethods = msvMethod.size();
					MyStringSet::iterator pos = msvMethod.begin();
					::OutputDebugString("Response methods:\n");
					while (pos != msvMethod.end())
					{
						::OutputDebugString("   ");
						AssertPtr(*pos);
						::OutputDebugStringW(**pos);
						::OutputDebugString("\n");
						pos++;
					}
				}
			}
			catch (...)
			{
			}
		}
#endif
	}
#endif // _DEBUG

	// Set CFileGlobals stuff.
 	g_fg.m_hinst = hInstance;
	g_fg.m_cr[kclrWindow] = ::GetSysColor(COLOR_WINDOW);
	g_fg.m_cr[kclrText] = ::GetSysColor(COLOR_WINDOWTEXT);
	g_fg.m_cr[kclrWindowSel] = ::GetSysColor(COLOR_HIGHLIGHT);
	g_fg.m_cr[kclrTextSel] = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	g_fg.m_hCursor[kcsrIBeam] = ::LoadCursor(NULL, IDC_IBEAM);
	g_fg.m_hCursor[kcsrHand] = ::LoadCursor(g_fg.m_hinst, MAKEINTRESOURCE(IDC_LINK));
	// This image list is used for column markers
	g_fg.m_himlCM = ImageList_Create(knWedgeWidth, knWedgeHeight, ILC_COLOR8, 2, 0);
	if (g_fg.m_himlCM)
	{
		HBITMAP hbmp = ::LoadBitmap(g_fg.m_hinst, MAKEINTRESOURCE(IDB_COLUMN_LINE));
		if (hbmp)
		{
			ImageList_Add(g_fg.m_himlCM, hbmp, NULL);
			::DeleteObject(hbmp);
		}
	}

	g_pzea = new CZEditApp();
	if (!g_pzea)
		return 1;

	CZEditFrame * pzef = new CZEditFrame();
	if (!pzef->Create(szCmdLine, NULL, true))
		return 1;

	g_pzea->AddFrame(pzef);
	g_pzea->SetCurrentFrame(pzef);

	HACCEL hAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ZEDIT));
	MSG msg;

	while (true)
	{
		// Call OnIdle when there are no other messages in the queue.
		while (!::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) && g_pzea->OnIdle())
			;

		if (::GetMessage(&msg, NULL, 0, 0))
		{
			bool fHandledMessage = false;
			if (g_fg.m_hwndViewCodes && ::IsDialogMessage(g_fg.m_hwndViewCodes, &msg))
				fHandledMessage = true;

			CZEditFrame ** ppzefT = g_pzea->m_prgzef;
			for (int izef = 0; !fHandledMessage && izef < g_pzea->m_czef; izef++, ppzefT++)
			{
				CZEditFrame * pzef = *ppzefT;
				/*if (!fHandledMessage && pzef->m_pfd)
				{
					HWND hwndDlg = pzef->m_pfd->GetHwnd();
					if (hwndDlg)
						fHandledMessage = ::IsDialogMessage(hwndDlg, &msg);
				}
				if (!fHandledMessage && pzef->m_pffd)
				{
					HWND hwndDlg = pzef->m_pffd->GetHwnd();
					if (hwndDlg)
						fHandledMessage = ::IsDialogMessage(hwndDlg, &msg);
				}
				if (!fHandledMessage)
				{
					HWND hwnd = pzef->GetHwnd();
					fHandledMessage = ::IsChild(hwnd, msg.hwnd) &&
						::TranslateAccelerator(hwnd, hAccel, &msg);
				}*/
				fHandledMessage =
					(pzef->m_pfd && ::IsDialogMessage(pzef->m_pfd->GetHwnd(), &msg)) ||
					(pzef->m_pffd && ::IsDialogMessage(pzef->m_pffd->GetHwnd(), &msg)) ||
					(::IsChild(pzef->GetHwnd(), msg.hwnd) &&
						::TranslateAccelerator(pzef->GetHwnd(), hAccel, &msg));
			}
			if (!fHandledMessage)
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
		else
		{
			break;
		}
	}

	delete g_pzea;
	::OleUninitialize();
	return msg.wParam;
}


/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CZEditApp::CZEditApp()
{
	m_czef = 0;
	m_izefCurrent = -1;
	m_prgzef = NULL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CZEditApp::~CZEditApp()
{
	if (m_prgzef)
	{
		delete m_prgzef;
		m_prgzef = NULL;
	}
}


/*----------------------------------------------------------------------------------------------
	Call the OnIdle methods of all the top-level frame windows.
----------------------------------------------------------------------------------------------*/
UINT CZEditApp::OnIdle()
{
	UINT nIdle = 0;
	CZEditFrame ** ppzefT = m_prgzef;
	for (int izef = 0; izef < m_czef; izef++, ppzefT++)
		nIdle += (*ppzefT)->OnIdle();
	return nIdle;
}


/*----------------------------------------------------------------------------------------------
	Add the specified frame to the array of top-level frames.
----------------------------------------------------------------------------------------------*/
bool CZEditApp::AddFrame(CZEditFrame * pzef)
{
	AssertPtr(pzef);

	CZEditFrame ** prgzef = (CZEditFrame **)realloc(m_prgzef,
		(m_czef + 1) * sizeof(CZEditFrame *));
	if (!prgzef)
		return false;

	prgzef[m_czef] = pzef;
	m_prgzef = prgzef;
	m_czef++;
	return true;
}


/*----------------------------------------------------------------------------------------------
	Remove the specified frame from the array of top-level frames.
----------------------------------------------------------------------------------------------*/
bool CZEditApp::RemoveFrame(CZEditFrame * pzef)
{
	AssertPtr(pzef);

	CZEditFrame ** ppzefT = m_prgzef;
	for (int izef = 0; izef < m_czef; izef++, ppzefT++)
	{
		if (*ppzefT == pzef)
		{
			memmove(ppzefT, ppzefT + 1, (m_czef - izef - 1) * sizeof(CZEditFrame *));
			if (--m_czef <= 0)
			{
				::PostQuitMessage(0);
			}
			return true;
		}
	}
	return false;
}


/*----------------------------------------------------------------------------------------------
	Return a pointer to the specified frame.
----------------------------------------------------------------------------------------------*/
CZEditFrame * CZEditApp::GetFrame(int izef)
{
	Assert(izef >= 0 && izef < m_czef);
	return m_prgzef[izef];
}


/*----------------------------------------------------------------------------------------------
	Return a pointer to the frame that currently has the focus.
----------------------------------------------------------------------------------------------*/
CZEditFrame * CZEditApp::GetCurrentFrame()
{
	Assert(m_izefCurrent >= 0 && m_izefCurrent < m_czef);
	return m_prgzef[m_izefCurrent];
}


/*----------------------------------------------------------------------------------------------
	Set the specified frame as the one that currently has the focus.
----------------------------------------------------------------------------------------------*/
bool CZEditApp::SetCurrentFrame(CZEditFrame * pzef)
{
	AssertPtr(pzef);

	CZEditFrame ** ppzefT = m_prgzef;
	for (int izef = 0; izef < m_czef; izef++, ppzefT++)
	{
		if (*ppzefT == pzef)
		{
			m_izefCurrent = izef;
			return true;
		}
	}
	return false;
}


/***********************************************************************************************
	CZEditFrame methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	The main window procedure.
----------------------------------------------------------------------------------------------*/
LRESULT CALLBACK CZEditFrame::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static int onMenu = -1;
	CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwnd, GWL_USERDATA);

	if (pzef)
	{
		switch (iMsg)
		{
		case WM_CLOSE:
			{
				// Close all open files.
				if (!pzef->CloseAll(true))
					return FALSE;

				if (!g_pzea->RemoveFrame(pzef))
					return FALSE;

				delete pzef;
				return TRUE;
			}

		case WM_ACTIVATE:
			if (LOWORD(wParam) != WA_INACTIVE)
				g_pzea->SetCurrentFrame(pzef);
			return 0;

		case WM_COMMAND:
			return pzef->OnCommand(wParam, lParam);

		case WM_COPYDATA:
			return pzef->ReadFile((char*)((COPYDATASTRUCT*)lParam)->lpData, kftDefault);

		case WM_DRAWITEM:
			{
				DRAWITEMSTRUCT * pdis = (DRAWITEMSTRUCT *)lParam;
				AssertPtr(pdis);
				if (pdis->CtlType == ODT_MENU)
					return (pzef->GetMenu())->OnDrawItem(pdis);
				break;
			}

		case WM_ENTERIDLE:
			return SendMessage((HWND) lParam, WM_ENTERIDLE, 0, 0);

		case WM_INITMENUPOPUP:
			return pzef->OnInitMenuPopup((HMENU) wParam, LOWORD(lParam), HIWORD(lParam));

		case WM_MEASUREITEM:
			{
				MEASUREITEMSTRUCT * pmis = (MEASUREITEMSTRUCT *)lParam;
				AssertPtr(pmis);
				if (pmis->CtlType == ODT_MENU)
                    return pzef->GetMenu()->OnMeasureItem(pmis);
				break;
			}

		case WM_MENUSELECT:
			{
				char szMessage[MAX_PATH] = {0};
				if ((HIWORD(wParam) & MF_HILITE) && !(HIWORD(wParam) & MF_POPUP) && (LOWORD(wParam) > 0))
					LoadString(g_fg.m_hinst, LOWORD(wParam), szMessage, MAX_PATH);
				else
					lstrcpy(szMessage, "Ready");
				pzef->SetStatusText(ksboMessage, szMessage);
				return 0;
			}

		case WM_NOTIFY:
			return pzef->OnNotify(wParam, (NMHDR *)lParam);

		case WM_QUERYENDSESSION:
			return pzef->CloseAll(true);

		case WM_SETFOCUS:
			{
				static bool fIgnore = false;
				if (!fIgnore)
				{
					fIgnore = true;
					CZDoc * pzd = pzef->m_pzdFirst;
					while (pzd)
					{
						pzd->CheckFileTime();
						pzd = pzd->Next();
					}
					if (pzef->GetCurrentDoc())
						SetFocus(pzef->GetCurrentDoc()->GetHwnd());
					fIgnore = false;
				}
			}
			return 0;

		case WM_SIZE:
			{
				// This next section keeps the real window position when it is opened maximized.
				WINDOWPLACEMENT winInfo = {sizeof(WINDOWPLACEMENT)};
				::GetWindowPlacement(hwnd, &winInfo);
				if (winInfo.flags == 2 && winInfo.showCmd == 3)// && fFirst)
				{
					winInfo.rcNormalPosition = pzef->m_rcPosition;
					::SetWindowPlacement(hwnd, &winInfo);
				}
				else
				{
					pzef->m_rcPosition = winInfo.rcNormalPosition;
				}
				return pzef->OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
			}
		}
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}


/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CZEditFrame::CZEditFrame()
{
	m_pzdFirst = NULL;
	m_pzdCurrent = NULL;
	m_pfd = NULL;
	m_pffd = NULL;
#ifdef SPELLING
	m_fSpellingIsValid = false;
#endif // SPELLING
	m_pdt = NULL;
	m_pzm = m_pzmContext = NULL;
	m_hwndTabMgrDlg = NULL;
	memset(m_szContextFile, 0, sizeof(m_szContextFile));
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CZEditFrame::~CZEditFrame()
{
	if (m_hwnd)
		::RevokeDragDrop(m_hwnd);
	if (m_pdt)
	{
		::CoLockObjectExternal(m_pdt, FALSE, TRUE);
		m_pdt->Release();
		m_pdt = NULL;
	}
	::SetWindowLong(m_hwnd, GWL_USERDATA, 0);
	::ShowWindow(m_hwnd, SW_HIDE);
	UpdateRegistry();
	if (m_pzm)
		delete m_pzm;
	if (m_pzmContext)
		delete m_pzmContext;
	if (m_pzdFirst)
		delete m_pzdFirst;
	if (m_pfd)
	{
		delete m_pfd;
		m_pfd = NULL;
	}
	if (m_pffd)
	{
		delete m_pffd;
		m_pffd = NULL;
	}
	if (m_pzt)
	{
		delete m_pzt;
		m_pzt = NULL;
	}
}


/*----------------------------------------------------------------------------------------------
	Create a top-level frame window.
	If pzd is NULL, we want to attach the document to this frame window.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::Create(char * pszCmdLine, CZDoc * pzdExisting, bool fShowWindow)
{
	AssertPtrN(pszCmdLine);
	AssertPtrN(pzdExisting);

	bool fFirstTime = g_pzea->GetFrameCount() == 0;
	if (fFirstTime)
	{
		// Register child classes.
		WNDCLASS wndclass = { CS_DBLCLKS };
		wndclass.hInstance = g_fg.m_hinst;
		// DRZ_FastEdit
		wndclass.lpfnWndProc = CFastEdit::WndProc;
		wndclass.lpszClassName = "DRZ_FastEdit";
		if (!RegisterClass(&wndclass))
			return false;
		// DRZ_HexEdit
		wndclass.lpfnWndProc = CHexEdit::WndProc;
		wndclass.lpszClassName = "DRZ_HexEdit";
		if (!RegisterClass(&wndclass))
			return false;
		// DRZ_Tab
		wndclass.lpfnWndProc = CZTab::WndProc;
		wndclass.lpszClassName = "DRZ_Tab";
		wndclass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		if (!::RegisterClass(&wndclass))
			return false;
		// DRZ_Frame
		wndclass.lpfnWndProc = CZFrame::WndProc;
		wndclass.lpszClassName = "DRZ_Frame";
		wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
		if (!::RegisterClass(&wndclass))
			return false;
		// DRZ_Scrollbar
		wndclass.lpfnWndProc = CZFrame::ScrollWndProc;
		wndclass.lpszClassName = "DRZ_Scrollbar";
		wndclass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		if (!::RegisterClass(&wndclass))
			return false;

		// The main window class.
		WNDCLASSEX wndclassex = {
			sizeof(wndclassex),
			0,
			CZEditFrame::WndProc,
			0,
			0,
			g_fg.m_hinst,
			::LoadIcon(g_fg.m_hinst, MAKEINTRESOURCE(IDI_ZEDIT)),
			::LoadCursor(NULL, IDC_ARROW),
			(HBRUSH)(COLOR_BTNFACE + 1),
			MAKEINTRESOURCE(IDR_MAINMENU),
			g_pszAppName,
			::LoadIcon(g_fg.m_hinst, MAKEINTRESOURCE(IDI_ZEDIT))};
		::RegisterClassEx(&wndclassex);

		INITCOMMONCONTROLSEX iccex;
		iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		iccex.dwICC = ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS | ICC_COOL_CLASSES;
		::InitCommonControlsEx(&iccex);
	}

	// Make sure the things that were created in the constructor exist.
	m_pfd = new CFindDlg(this);
	m_pffd = new CFindFilesDlg(this);
	if (!m_pfd || !m_pffd)
		return false;

	SetLastError(0);
	m_hwnd = ::CreateWindow(g_pszAppName, "", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, g_fg.m_hinst, NULL);
	DWORD dwErr = GetLastError();

	// Create needed windows and load values from registry
	::SetWindowLong(m_hwnd, GWL_USERDATA, (LONG)this);
	if (FAILED(CZDropTarget::Create(this, &m_pdt)))
		return false;
	if (FAILED(::CoLockObjectExternal(m_pdt, TRUE, FALSE)))
		return false;
	if (FAILED(::RegisterDragDrop(m_hwnd, m_pdt)))
		return false;
	if (!Initialize())
	{
		::MessageBox(m_hwnd, "Invalid registry settings were detected.", g_pszAppName,
			MB_OK | MB_ICONSTOP);
		return false;
	}

#ifdef SPELLING
	if (SUCCEEDED(m_sc.Init()))
	{
		m_fSpellingIsValid = true;
	}
	else
	{
		// Remove menu items relating to Spell Check
		HMENU hSubMenu = m_pzmContext->GetSubMenu(kcmEdit);
		int citems = GetMenuItemCount(hSubMenu);
		RemoveMenu(hSubMenu, citems, MF_BYPOSITION);
		RemoveMenu(hSubMenu, citems - 1, MF_BYPOSITION);

		hSubMenu = GetSubMenu(m_pzm->GetMenu(), 3); // Tools submenu
		RemoveMenu(hSubMenu, 2, MF_BYPOSITION);
		RemoveMenu(hSubMenu, 1, MF_BYPOSITION);
	}
#endif // SPELLING

	if (fShowWindow)
	{
		::ShowWindow(m_hwnd, SW_SHOW);
		::UpdateWindow(m_hwnd);
	}

	// Open files from the command line if they exist
	int nLength = pszCmdLine ? lstrlen(pszCmdLine) : 0;
	if (nLength > 0)
	{
		char * pszFilename = new char[nLength + 2];
		if (pszFilename)
		{
			lstrcpy(pszFilename, pszCmdLine);
			ReadFile(g_fg.FixString(pszFilename), kftDefault);
			delete pszFilename;
		}
	}
	else if (pzdExisting)
	{
		int iFile = 0;
		if (InsertFile(iFile, pzdExisting, pzdExisting->GetFilename()))
		{
			pzdExisting->AttachToMainFrame(this);
			SelectFile(iFile);
		}
	}
	else
	{
		OnCommand(MAKELONG(ID_FILE_NEW, 0), 0);
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	Update the state of toolbar icons.
----------------------------------------------------------------------------------------------*/
UINT CZEditFrame::OnIdle(void)
{
	char buffer[MAX_PATH] = {0};
	const long nEnabled = MAKELONG(TBSTATE_ENABLED, 0);
	const long nDisabled = 0;

	if (m_pzdCurrent)
	{
		HMENU hSubMenu = ::GetSubMenu(::GetMenu(m_hwnd), 1); // Edit submenu

		// Undo button
		BOOL fUndo = m_pzdCurrent->CanUndo();
		::SendMessage(m_hwndTool, TB_SETSTATE, ID_EDIT_UNDO, fUndo ? nEnabled : nDisabled);
		::EnableMenuItem(hSubMenu, ID_EDIT_UNDO, fUndo ? MF_ENABLED : MF_GRAYED);

		// Redo button
		BOOL fRedo = m_pzdCurrent->CanRedo();
		::SendMessage(m_hwndTool, TB_SETSTATE, ID_EDIT_REDO, fRedo ? nEnabled : nDisabled);
		::EnableMenuItem(hSubMenu, ID_EDIT_REDO, fRedo ? MF_ENABLED : MF_GRAYED);

		// Paste button
		::SendMessage(m_hwndTool, TB_SETSTATE, ID_EDIT_PASTE,
			m_pzdCurrent->CanPaste() ? nEnabled : nDisabled);

		// Cut and Copy buttons
		SelectionType st = m_pzdCurrent->GetSelection(NULL, NULL);
		::SendMessage(m_hwndTool, TB_SETSTATE, ID_EDIT_CUT,
			st == kstNone ? nDisabled : nEnabled);
		::SendMessage(m_hwndTool, TB_SETSTATE, ID_EDIT_COPY,
			st == kstNone ? nDisabled : nEnabled);

		// Modify/Clean flag
		m_pzdCurrent->UpdateModified(m_pzt->GetCurSel(), false);
	}

	if (m_pfd)
	{
		// Find Next and Find Prev buttons
		::SendMessage(m_hwndTool, TB_SETSTATE, ID_EDIT_FINDNEXT,
			m_pfd->CanFind() ? nEnabled : nDisabled);
		::SendMessage(m_hwndTool, TB_SETSTATE, ID_EDIT_FINDPREV,
			m_pfd->CanFind() ? nEnabled : nDisabled);
		m_pfd->OnEnterIdle();
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
CZDoc * CZEditFrame::GetDoc(int izd)
{
	CZDoc * pzd = m_pzdFirst;
	while (izd-- > 0 && pzd)
		pzd = pzd->Next();
	AssertPtr(pzd);
	return pzd;
}


/*----------------------------------------------------------------------------------------------
	Save program settings to the registry.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::UpdateRegistry()
{
	HKEY hkey;
	long lT;
	DWORD dwT;

	// Update window position and size in the registry
	if (::RegCreateKeyEx(HKEY_CURRENT_USER, kpszRootRegKey, 0, NULL, 0, KEY_WRITE, NULL, &hkey, &dwT))
		return false;

	WINDOWPLACEMENT winInfo = { sizeof(WINDOWPLACEMENT) };
	::GetWindowPlacement(m_hwnd, &winInfo);
	::RegSetValueEx(hkey, "Position", 0, REG_BINARY, (BYTE *)&winInfo.rcNormalPosition, sizeof(RECT));
	::RegSetValueEx(hkey, "Print Margins", 0, REG_BINARY, (BYTE *)&g_fg.m_rcPrintMargins, sizeof(RECT));
	::RegSetValueEx(hkey, "Header/Footer Margins", 0, REG_BINARY, (BYTE *)&g_fg.m_ptHeaderFooter, sizeof(POINT));
	LOGFONT lf;
	::GetObject(g_fg.m_hFont, sizeof(LOGFONT), &lf);
	::RegSetValueEx(hkey, "Font", 0, REG_BINARY, (BYTE *)&lf, sizeof(LOGFONT));
	::GetObject(g_fg.m_hFontFixed, sizeof(LOGFONT), &lf);
	::RegSetValueEx(hkey, "Font (Fixed)", 0, REG_BINARY, (BYTE *)&lf, sizeof(LOGFONT));
	lT = m_fWrap;
	::RegSetValueEx(hkey, "Wrap", 0, REG_DWORD, (BYTE *)&lT, sizeof(long));
	lT = winInfo.showCmd == SW_SHOWMAXIMIZED;
	::RegSetValueEx(hkey, "Window State", 0, REG_DWORD, (BYTE *)&lT, sizeof(long));
	lT = g_fg.m_ri.m_cchSpacesInTab;
	::RegSetValueEx(hkey, "Tab Width", 0, REG_DWORD, (BYTE *)&lT, sizeof(long));
	lT = g_fg.m_defaultEncoding;
	::RegSetValueEx(hkey, "Default Encoding", 0, REG_DWORD, (BYTE *)&lT, sizeof(long));
	::RegSetValueEx(hkey, "Font Color", 0, REG_DWORD, (BYTE *)&g_fg.m_cr[kclrText], sizeof(COLORREF));
	::RegSetValueEx(hkey, "File Size Limit", 0, REG_DWORD, (BYTE *)&g_fg.m_cchFileLimit, sizeof(DWORD));
	::RegSetValueEx(hkey, "View Codes Before", 0, REG_DWORD, (BYTE *)&g_fg.m_cchCodesBefore, sizeof(DWORD));
	::RegSetValueEx(hkey, "View Codes After", 0, REG_DWORD, (BYTE *)&g_fg.m_cchCodesAfter, sizeof(DWORD));
	::RegSetValueEx(hkey, "Flat Scrollbars", 0, REG_DWORD, (BYTE *)&g_fg.m_fFlatScrollbars, sizeof(DWORD));


	TBSAVEPARAMS tbsp = {hkey, NULL, "Toolbar State"};
	::SendMessage(m_hwndTool, TB_SAVERESTORE, TRUE, (LPARAM)&tbsp);

	int cch = 0;
	char * prgch = NULL;
	char * pPos;
	int cfile = 0;
	// Put the MRU file list in the registry
	while (cfile < kcMaxFiles && *m_szFileList[cfile])
		cch += lstrlen(m_szFileList[cfile++]) + 3;
	if (cch)
		pPos = prgch = new char[cch];
	if (prgch)
	{
		::ZeroMemory(prgch, cch);
		for (int ifile = 0; ifile < cfile; ifile++)
		{
			lstrcat(pPos, m_szFileList[ifile]);
			pPos += lstrlen(pPos) + 1;
			*pPos++ = m_szFileList[ifile][sizeof(m_szFileList[0]) - 1];
			*pPos++ = 0;
		}
		::RegSetValueEx(hkey, "MRU Files", 0, REG_BINARY, (BYTE *)prgch, cch);
		delete prgch;
		prgch = NULL;
	}
	else
	{
		::RegSetValueEx(hkey, "MRU Files", 0, REG_BINARY, (BYTE *)prgch, 0);
	}

	// Put the print strings in the registry
	cch = strlen(g_fg.m_szPrintStrings[0]) + strlen(g_fg.m_szPrintStrings[1]);
	if (cch)
		pPos = prgch = new char[cch + 2];
	if (prgch)
	{
		char * pLim = prgch + cch + 2;
		::ZeroMemory(prgch, cch);
		strcat_s(pPos, pLim - pPos, g_fg.m_szPrintStrings[0]);
		pPos += strlen(pPos) + 1;
		strcat_s(pPos, pLim - pPos, g_fg.m_szPrintStrings[1]);
		::RegSetValueEx(hkey, "Print Strings", 0, REG_BINARY, (BYTE *)prgch, cch + 2);
		delete prgch;
		prgch = NULL;
	}
	else
	{
		::RegSetValueEx(hkey, "Print Strings", 0, REG_BINARY, (BYTE *)prgch, 0);
	}

	// Put the column markers in the registry
	int ccm = g_fg.m_vpcm.size();
	int cchMarker = ccm * 15 + 1;
	if (ccm)
		prgch = new char[cchMarker];
	if (prgch)
	{
		*prgch = 0;
		char szcm[15];
		for (int icm = 0; icm < ccm; icm++)
		{
			CColumnMarker * pcm = g_fg.m_vpcm[icm];
			AssertPtr(pcm);
			sprintf_s(szcm, "%d %d ", pcm->m_iColumn, pcm->m_cr);
			strcat_s(prgch, cchMarker, szcm);
		}
		int cch = strlen(prgch) - 1;
		Assert(cch > 0);
		prgch[cch] = 0;
		::RegSetValueEx(hkey, "Column Markers", 0, REG_SZ, (BYTE *)prgch, cch);
		delete prgch;
		prgch = NULL;
	}
	else
	{
		::RegSetValueEx(hkey, "Column Markers", 0, REG_SZ, (BYTE *)"", 0);
	}

	::RegCloseKey(hkey);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::Initialize()
{
	bool fFirstTime = g_pzea->GetFrameCount() == 0;

	if (fFirstTime)
	{
		// Register child classes.

		// DRZ_FastEdit
		WNDCLASS wcFastEdit = { CS_DBLCLKS };
		wcFastEdit.hInstance = g_fg.m_hinst;
		wcFastEdit.lpfnWndProc = CFastEdit::WndProc;
		wcFastEdit.lpszClassName = "DRZ_FastEdit";
		::RegisterClass(&wcFastEdit);

		// DRZ_HexEdit
		WNDCLASS wcHexEdit = { CS_DBLCLKS };
		wcHexEdit.hInstance = g_fg.m_hinst;
		wcHexEdit.lpfnWndProc = CHexEdit::WndProc;
		wcHexEdit.lpszClassName = "DRZ_HexEdit";
		::RegisterClass(&wcHexEdit);

		// DRZ_Tab
		WNDCLASS wcTab = { 0 };
		wcTab.hInstance = g_fg.m_hinst;
		wcTab.lpfnWndProc = CZTab::WndProc;
		wcTab.lpszClassName = "DRZ_Tab";
		wcTab.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		::RegisterClass(&wcTab);

		// DRZ_Frame
		WNDCLASS wcFrame = { CS_DBLCLKS };
		wcFrame.hInstance = g_fg.m_hinst;
		wcFrame.lpfnWndProc = CZFrame::WndProc;
		wcFrame.lpszClassName = "DRZ_Frame";
		wcFrame.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
		::RegisterClass(&wcFrame);

		// DRZ_Scrollbar
		WNDCLASS wcScrollbar = { 0 };
		wcScrollbar.hInstance = g_fg.m_hinst;
		wcScrollbar.lpfnWndProc = CZFrame::ScrollWndProc;
		wcScrollbar.lpszClassName = "DRZ_Scrollbar";
		wcScrollbar.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		::RegisterClass(&wcScrollbar);

		INITCOMMONCONTROLSEX iccex;
		iccex.dwSize = sizeof( INITCOMMONCONTROLSEX );
		iccex.dwICC = ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS | ICC_COOL_CLASSES;
		::InitCommonControlsEx(&iccex);
	}

	// Create the toolbar, tab control, and status bar
	m_pzt = new CZTab();
	if (!CreateToolBars() || !m_pzt->Create(this) || !CreateStatus())
		return false;

	// Find and load the file containing color syntax information
	if (fFirstTime)
		g_cs.Load();

	// Read in values from the registry if they exist
	DWORD dwT, dwSize;
	HKEY hkey;
	if (::RegCreateKeyEx(HKEY_CURRENT_USER, kpszRootRegKey, 0, NULL, 0, KEY_READ, NULL, &hkey,
		&dwT) != ERROR_SUCCESS)
	{
		return false;
	}

	// The registry data was there, so use it to initialize
	dwSize = sizeof(RECT);
	long lValue = 0;
	char * prgch = NULL;
	char * pPos = NULL;

	if (::RegQueryValueEx(hkey, "Position", 0, NULL, (BYTE *)&m_rcPosition, &dwSize))
		::GetWindowRect(m_hwnd, &m_rcPosition);

	if (fFirstTime)
	{
		if (::RegQueryValueEx(hkey, "Print Margins", 0, NULL, (BYTE *)&g_fg.m_rcPrintMargins,
			&dwSize))
		{
			::SetRect(&g_fg.m_rcPrintMargins, 1440, 1440, 1440, 1440);
		}

		dwSize = sizeof(POINT);
		if (::RegQueryValueEx(hkey, "Header/Footer Margins", 0, NULL, (BYTE *)&g_fg.m_ptHeaderFooter,
			&dwSize))
		{
			g_fg.m_ptHeaderFooter.x = g_fg.m_ptHeaderFooter.y = 720;
		}

		if (::RegQueryValueEx(hkey, "Tab Width", 0, NULL, (BYTE *)&lValue, &dwSize))
			g_fg.m_ri.m_cchSpacesInTab = 8;
		else
			g_fg.m_ri.m_cchSpacesInTab = (int)min(max(1, lValue), 30);

		if (::RegQueryValueEx(hkey, "Default Encoding", 0, NULL, (BYTE *)&lValue, &dwSize))
			g_fg.m_defaultEncoding = kftUnicode8;
		else
			g_fg.m_defaultEncoding = min(max(kftAnsi, lValue), kftUnicode16);

		dwSize = sizeof(LOGFONT);
		LOGFONT lf;
		if (::RegQueryValueEx(hkey, "Font", 0, NULL, (BYTE *)&lf, &dwSize) ||
			dwSize != sizeof(LOGFONT))
		{
			::ZeroMemory(&lf, sizeof(LOGFONT));
			lstrcpy(lf.lfFaceName, "MS Sans Serif");
			lf.lfHeight = -13;
		}
		g_pzea->SetFont(::CreateFontIndirect(&lf), false);

		dwSize = sizeof(LOGFONT);
		if (::RegQueryValueEx(hkey, "Font (Fixed)", 0, NULL, (BYTE *)&lf, &dwSize) ||
			dwSize != sizeof(LOGFONT))
		{
			::ZeroMemory(&lf, sizeof(LOGFONT));
			lstrcpy(lf.lfFaceName, "Courier New");
			lf.lfHeight = -13;
		}
		g_fg.m_hFontFixed = ::CreateFontIndirect(&lf);
		HDC hdc = ::GetDC(m_hwnd);
		HFONT hFontOld = (HFONT)::SelectObject(hdc, g_fg.m_hFontFixed);
		::GetTextMetrics(hdc, &g_fg.m_tmFixed);
		::SelectObject(hdc, hFontOld);
		::ReleaseDC(m_hwnd, hdc);

		dwSize = sizeof(long);
		if (!::RegQueryValueEx(hkey, "View Codes Before", NULL, NULL, (BYTE *)&lValue, &dwSize))
			g_fg.m_cchCodesBefore = min(max(lValue, 0), 50);

		dwSize = sizeof(long);
		if (!::RegQueryValueEx(hkey, "View Codes After", NULL, NULL, (BYTE *)&lValue, &dwSize))
			g_fg.m_cchCodesAfter = min(max(lValue, 0), 50);

		dwSize = sizeof(long);
		if (!::RegQueryValueEx(hkey, "Flat Scrollbars", NULL, NULL, (BYTE *)&lValue, &dwSize))
			g_fg.m_fFlatScrollbars = lValue != 0;

		if (::RegQueryValueEx(hkey, "Font Color", 0, NULL, (BYTE *)&lValue, &dwSize))
			g_fg.m_cr[kclrText] = 0;
		else
			g_fg.m_cr[kclrText] = (COLORREF)lValue;

		if (::RegQueryValueEx(hkey, "File Size Limit", 0, NULL, (BYTE *)&lValue, &dwSize))
			g_fg.m_cchFileLimit = 1048576;
		else
			g_fg.m_cchFileLimit = (DWORD)lValue;

		if (::RegQueryValueEx(hkey, "Use Spaces", 0, NULL, (BYTE *)&lValue, &dwSize))
			g_fg.m_fUseSpaces = false;
		else
			g_fg.m_fUseSpaces = lValue ? true : false;
	}

	dwSize = sizeof(long);
	if (::RegQueryValueEx(hkey, "Wrap", 0, NULL, (BYTE *)&lValue, &dwSize))
		m_fWrap = true;
	else
		m_fWrap = lValue ? true : false;

	// Get the MRU file list
	::ZeroMemory(m_szFileList, sizeof(m_szFileList));
	dwSize = 0;
	::RegQueryValueEx(hkey, "MRU Files", 0, NULL, 0, &dwSize);
	if (dwSize)
		prgch = new char[dwSize];
	if (prgch)
	{
		if (!::RegQueryValueEx(hkey, "MRU Files", 0, NULL, (BYTE *)prgch, &dwSize))
		{
			pPos = prgch;
			char * pStop = prgch + dwSize;
			int i = 0;
			while (*pPos && pPos < pStop)
			{
				lstrcpy(m_szFileList[i], pPos);
				pPos += lstrlen(pPos) + 1;
				if (pPos < pStop)
				{
					if (isalpha(*pPos))
					{
						m_szFileList[i++][sizeof(m_szFileList[0]) - 1] = kftDefault;
					}
					else
					{
						m_szFileList[i++][sizeof(m_szFileList[0]) - 1] = *pPos;
						pPos += 2;
					}
				}
			}
		}
		delete prgch;
		prgch = NULL;
	}

	if (fFirstTime)
	{
		// Get the print strings.
		::ZeroMemory(g_fg.m_szPrintStrings, sizeof(g_fg.m_szPrintStrings));
		dwSize = 0;
		::RegQueryValueEx(hkey, "Print strings", 0, NULL, 0, &dwSize);
		if (dwSize)
			prgch = new char[dwSize];
		if (prgch)
		{
			if (!::RegQueryValueEx(hkey, "Print strings", 0, NULL, (BYTE *)prgch, &dwSize))
			{
				pPos = prgch;
				lstrcpy(g_fg.m_szPrintStrings[0], pPos);
				pPos += lstrlen(pPos) + 1;
				lstrcpy(g_fg.m_szPrintStrings[1], pPos);
			}
			delete prgch;
			prgch = NULL;
		}
		else
		{
			// Use default print strings.
			lstrcpy(g_fg.m_szPrintStrings[0], "^L^CZEdit - ^P^R");
			lstrcpy(g_fg.m_szPrintStrings[1], "^L^CPage: ^G^R");
		}

		// Get the column markers.
		dwSize = 0;
		::RegQueryValueEx(hkey, "Column Markers", 0, NULL, 0, &dwSize);
		if (dwSize)
			prgch = new char[dwSize];
		if (prgch)
		{
			if (!::RegQueryValueEx(hkey, "Column Markers", 0, NULL, (BYTE *)prgch, &dwSize))
			{
				CColumnMarker * pcm;
				pPos = prgch;
				while (true)
				{
					int nT = strtol(pPos, &pPos, 0);
					if (!*pPos)
						break;
					COLORREF crT = strtol(pPos + 1, &pPos, 0);
					pcm = new CColumnMarker(nT, crT);
					if (pcm)
						g_fg.m_vpcm.push_back(pcm);
					if (!*pPos++)
						break;
				}
			}
			delete prgch;
			prgch = NULL;
		}
	}

	// Get the window position/state
	WINDOWPLACEMENT winInfo = { sizeof(WINDOWPLACEMENT) };
	dwSize = sizeof(long);
	if (::RegQueryValueEx(hkey, "Window state", 0, NULL, (BYTE *)&lValue, &dwSize))
		lValue = 0;
	if (lValue)
	{
		winInfo.showCmd = SW_SHOWMAXIMIZED;
		winInfo.flags = WPF_RESTORETOMAXIMIZED;
	}
	else
	{
		winInfo.showCmd = 0;
	}
	winInfo.showCmd |= SW_HIDE;
	winInfo.rcNormalPosition = m_rcPosition;
	::SetWindowPlacement(m_hwnd, &winInfo);

	::RegCloseKey(hkey);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZEditFrame::Animate(HWND hwnd, int iButton, bool fToolBar, bool fOpen)
{
	RECT rcSmall;
	RECT rcWnd;
	RECT rcParent;
	RECT * prcFrom;
	RECT *prcTo;

	if (fOpen && ::GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE)
		return;

	::GetWindowRect(hwnd, &rcWnd);
	if (fOpen)
	{
		prcFrom = &rcSmall;
		prcTo = &rcWnd;
		::GetWindowRect(::GetParent(hwnd), &rcParent);
		int x = rcParent.left + ((rcParent.right - rcParent.left - rcWnd.right + rcWnd.left) >> 1);
		int y = rcParent.top + ((rcParent.bottom - rcParent.top - rcWnd.bottom + rcWnd.top) >> 1);
		::SetWindowPos(hwnd, NULL, x, y, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		::OffsetRect(&rcWnd, x - rcWnd.left, y - rcWnd.top);
	}
	else
	{
		prcFrom = &rcWnd;
		prcTo = &rcSmall;
	}

	if (fToolBar)
	{
		iButton = ::SendMessage(m_hwndTool, TB_COMMANDTOINDEX, iButton, 0);
		::SendMessage(m_hwndTool, TB_GETITEMRECT, iButton, (LPARAM)&rcSmall);
		::MapWindowPoints(m_hwndTool, NULL, (POINT *)&rcSmall, 2);
	}
	else
	{
		::SendMessage(m_hwndStatus, SB_GETRECT, iButton, (LPARAM)&rcSmall);
		::MapWindowPoints(m_hwndStatus, NULL, (POINT *)&rcSmall, 2);
	}
	::DrawAnimatedRects(hwnd, 3, prcFrom, prcTo); // IDANI_CAPTION
}


/*----------------------------------------------------------------------------------------------
	Look to see if the current caret position is inside of a filename. If it is,
	add the filename to the context menu.
----------------------------------------------------------------------------------------------*/
void CZEditFrame::AddOpenDocumentToMenu(HMENU hMenu)
{
	AssertPtr(m_pzdCurrent);

	UINT ichSelStart;
	UINT ichSelStop;
	POINT pt;
	::GetCursorPos(&pt);
	CZView * pzv = m_pzdCurrent->GetCurrentView();
	AssertPtr(pzv);
	::MapWindowPoints(NULL, pzv->GetHwnd(), &pt, 1);
	UINT ich = pzv->CharFromPoint(pt, false);
	m_pzdCurrent->GetSelection(&ichSelStart, &ichSelStop);
	void * pv = NULL;
	if (ichSelStart == ichSelStop)
	{
		// Find the left and right edge of the word at pt.
		UINT ipr;
		UINT cch = 0;
		void * pvPos;
		DocPart * pdp = m_pzdCurrent->GetPart(ich, false, &pvPos, &ipr);
		if (pvPos)
		{
			if (m_pzdCurrent->IsAnsi(pdp, ipr))
			{
				char * prgchParaStart = (char *)pdp->rgpv[ipr];
				char * prgchParaLimit = prgchParaStart + PrintCharsInPara(pdp, ipr);

				char * prgchStart = (char *)pvPos - 1;
				// Find the first space character before the current position.
				while (prgchStart >= prgchParaStart && !isspace((unsigned char)*prgchStart))
					prgchStart--;
				// If we're not at the first character in the paragraph or we're at a space,
				// move forward one character.
				if (prgchStart != prgchParaStart || isspace((unsigned char)*prgchStart))
					prgchStart++;
				// Move forward until we're at the original character or we're at either
				// a \ or an alphabetic character. This should be the first character
				// in the filename.
				while (prgchStart <= (char *)pvPos && prgchStart < prgchParaLimit &&
					*prgchStart != '\\' && !isalpha((unsigned char)*prgchStart))
				{
					prgchStart++;
				}
				if (prgchStart > pvPos)
					pvPos = prgchStart;

				char * prgchStop = (char *)pvPos;
				while (prgchStop < prgchParaLimit && !isspace((unsigned char)*prgchStop))
					prgchStop++;
				if (prgchStop >= (char *)pvPos)
					prgchStop--;
				while (prgchStop >= (char *)pvPos && !isalpha((unsigned char)*prgchStop))
					prgchStop--;

				cch = prgchStop - prgchStart + 1;
				if (cch)
				{
					pv = new char[cch + 1];
					if (pv)
						memmove(pv, prgchStart, cch);
				}
			}
			else
			{
				wchar * prgchParaStart = (wchar *)pdp->rgpv[ipr];
				wchar * prgchParaLimit = prgchParaStart + PrintCharsInPara(pdp, ipr);

				wchar * prgchStart = (wchar *)pvPos;
				// Find the first space character before the current position.
				while (prgchStart > prgchParaStart && !iswspace(*(--prgchStart)));
				// If we're not at the first character in the paragraph or we're at a space,
				// move forward one character.
				if (prgchStart != prgchParaStart || iswspace(*prgchStart))
					prgchStart++;
				// Move forward until we're at the original character or we're at either
				// a \ or an alphabetic character. This should be the first character
				// in the filename.
				while (prgchStart <= (wchar *)pvPos && prgchStart < prgchParaLimit && 
					*prgchStart != '\\' && !iswalpha(*prgchStart))
				{
					prgchStart++;
				}

				wchar * prgchStop = (wchar *)pvPos;
				while (prgchStop < prgchParaLimit && !iswspace(*(prgchStop++)));
				while (prgchStop > prgchStart && !iswalpha(*--prgchStop));

				cch = prgchStop - prgchStart + 1;
				if (cch)
				{
					pv = new char[cch + 1];
					if (pv)
					{
						char * prgch = (char *)pv;
						while (prgchStart <= prgchStop)
							*prgch++ = (char)*prgchStart++;
					}
				}
			}
			if (pv)
				((char *)pv)[cch] = 0;
		}	
		ichSelStop = ichSelStart + cch;
	}
	else if (ichSelStop - ichSelStart < MAX_PATH && ichSelStart <= ich && ich < ichSelStop)
	{
		// See if the selection is a filename.
		m_pzdCurrent->GetText(ichSelStart, ichSelStop - ichSelStart, &pv);
	}

	MENUITEMINFO mii = { 0 };
	if (g_fg.m_fCanUseVer5)
		mii.cbSize = sizeof(MENUITEMINFO);
	else
		mii.cbSize = CDSIZEOF_STRUCT(MENUITEMINFOA,cch);
	mii.fMask = MIIM_DATA;
	::GetMenuItemInfo(hMenu, ID_EDIT_OPEN, FALSE, &mii);
	MyItem * pmi = (MyItem *)mii.dwItemData;
	AssertPtr(pmi);
	bool fValidFile = false;
	if (pv)
	{
		WIN32_FIND_DATA wfd;
		HANDLE hSearch = ::FindFirstFile((char *)pv, &wfd);
		if (hSearch != INVALID_HANDLE_VALUE)
		{
			// Add the menu item to open the file.
			fValidFile = true;
			memset(m_szContextFile, 0, sizeof(m_szContextFile));
			lstrcpyn(m_szContextFile, (char *)pv, MAX_PATH);
			char szItem[MAX_PATH + 17];
			wsprintf(szItem, "Open Document \"%s\"", wfd.cFileName);
			strncpy_s(pmi->szItemText, szItem, sizeof(pmi->szItemText));
			::FindClose(hSearch);
		}
		delete pv;
	}
	UINT uFlags = MF_BYCOMMAND | MF_OWNERDRAW;
	if (fValidFile)
	{
		uFlags |= MF_ENABLED;
	}
	else
	{
		strcpy_s(pmi->szItemText, "Open Document");
		uFlags |= MF_DISABLED;
	}
	// This is necessary so that Windows will send the WM_MEASUREITEM message again.
	::ModifyMenu(hMenu, ID_EDIT_OPEN, MF_BYCOMMAND, ID_EDIT_OPEN, "");
	::ModifyMenu(hMenu, ID_EDIT_OPEN, uFlags, ID_EDIT_OPEN, (LPCSTR)pmi);
}

#ifdef SPELLING
// This assumes the spelling item is the last item on the menu given by hMenu.
void CZEditFrame::AddSuggestionsToMenu(HMENU hMenu)
{
	FileType ft = m_pzdCurrent->GetFileType();
	if (ft == kftBinary)
		return;
	if (m_fSpellingIsValid)
	{
		CZView * pzv = m_pzdCurrent->GetCurrentView();
		void * pv = NULL;
		UINT ich;
		POINT pt;
		::GetCursorPos(&pt);
		::MapWindowPoints(NULL, pzv->GetHwnd(), &pt, 1);
		ich = pzv->CharFromPoint(pt, false);
		UINT cch = m_pzdCurrent->GetWordAtChar(ich, &pv, &m_iWordStart, &m_cchWord);
		bool fInvalidWord = false;
		if (pv)
		{
			if (cch)
			{
				HRESULT hr;
				int ichsMinBad;
				int ichsLimBad;
				int scrs;
				if (ft == kftAnsi)
					hr = m_sc.Check((char *)pv, cch, &ichsMinBad, &ichsLimBad, &scrs);
				else
					hr = m_sc.Check((wchar *)pv, cch, &ichsMinBad, &ichsLimBad, &scrs);
				if (SUCCEEDED(hr) && ichsMinBad != -1)
					fInvalidWord = true;
			}
			delete pv;
		}

		int iPos = ::GetMenuItemCount(hMenu) - 1;
		MENUITEMINFO mii = { 0 };
		if (g_fg.m_fCanUseVer5)
			mii.cbSize = sizeof(MENUITEMINFO);
		else
			mii.cbSize = CDSIZEOF_STRUCT(MENUITEMINFOA,cch);
		mii.fMask = MIIM_TYPE | MFT_STRING | MIIM_STATE;
		mii.fState = MFS_GRAYED;
		if (fInvalidWord)
		{
			int cchs;
			int cchsMore;
			char * prgchs;
			char * prgchsMore;
			m_sc.GetSuggestions(&cchs, &prgchs, &cchsMore, &prgchsMore);
			::EnableMenuItem(hMenu, iPos, MF_BYPOSITION | MF_ENABLED);
			HMENU hSuggestMenu = ::GetSubMenu(hMenu, iPos);
			for (int i = ::GetMenuItemCount(hSuggestMenu); i > 0; i--)
				::RemoveMenu(hSuggestMenu, 0, MF_BYPOSITION);
			if (*prgchs || *prgchsMore)
			{
				int nID = SUGGESTION_START;
				for (i = 0; i < cchs; i++, prgchs += strlen(prgchs) + 1)
					::AppendMenu(hSuggestMenu, MF_STRING, nID++, prgchs);
				if (cchs && cchsMore)
					::AppendMenu(hSuggestMenu, MF_SEPARATOR, 0, NULL);
				for (i = 0; i < cchsMore; i++, prgchsMore += strlen(prgchsMore) + 1)
					::AppendMenu(hSuggestMenu, MF_STRING, nID++, prgchsMore);
				mii.dwTypeData = "Spelling Suggestions";
				mii.fState = MFS_ENABLED;
			}
			else
			{
				mii.dwTypeData = "No Spelling Suggestions";
			}
		}
		else
		{
			mii.dwTypeData = "Correct Spelling";
		}
		mii.cch = strlen(mii.dwTypeData);
		::SetMenuItemInfo(hMenu, iPos, TRUE, &mii);
	}
}

#endif // SPELLING


/*----------------------------------------------------------------------------------------------
	Set the text of a specific part of the statusbar.
----------------------------------------------------------------------------------------------*/
void CZEditFrame::SetStatusText(int ksbo, const char * pszText)
{
	if (ksboMessage == ksbo)
	{
		::SendMessage(m_hwndStatus, SB_SETTEXT, ksbo + SBT_NOBORDERS, (LPARAM)pszText);
	}
	else
	{
		HFONT hFont = (HFONT)::SendMessage(m_hwndStatus, WM_GETFONT, 0, 0);
		HDC hdc = ::GetDC(m_hwndStatus);
		HFONT hOldFont = (HFONT)::SelectObject(hdc, hFont);
		SIZE size;
		::GetTextExtentPoint32(hdc, pszText, strlen(pszText), &size);
		::SelectObject(hdc, hOldFont);
		::ReleaseDC(m_hwndStatus, hdc);
		size.cx += 15;

		// Find the old width of each part.
		int rgnWidth[6];
		::SendMessage(m_hwndStatus, SB_GETPARTS, sizeof(rgnWidth) / sizeof(int),
			(LPARAM)rgnWidth);
		char szText[200] = {0};

		// Get the difference between the old width and the new width.
		int nDiff;
		if (ksbo == 0)
		{
			nDiff = (((size.cx / 20) + 1) * 20) - rgnWidth[0];
		}
		else
		{
			nDiff = rgnWidth[ksbo - 1] + size.cx - rgnWidth[ksbo];
			strcat_s(szText, "\t"); // This is used to center the text in the part
		}

		// Add this difference to all the following parts and set the parts to the new sizes
		for (int i = ksbo; i < ksboMessage; i++)
			rgnWidth[i] += nDiff;
		rgnWidth[ksboMessage] = -1;
		::SendMessage(m_hwndStatus, SB_SETPARTS, sizeof(rgnWidth) / sizeof(int),
			(LPARAM)rgnWidth);

		strcat_s(szText, pszText);
		::SendMessage(m_hwndStatus, SB_SETTEXT, ksbo, (long)szText);
	}
}


char * CZEditFrame::InsertComma(int n)
{
	static char rgchBuffer[50];
	int i = 0;
	char * p = &rgchBuffer[sizeof(rgchBuffer) - 1];
	 
	*p = 0;
	do
	{
		if (i % 3 == 0 && i != 0)
			*--p = ',';
		*--p = (char)('0' + n % 10);
		n /= 10;
		i++;
	} while (n != 0);
	return p;
}


/***********************************************************************************************
	ColorSchemeInfo methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
ColorSchemeInfo::ColorSchemeInfo(char * pszExt)
{
	m_pszDesc = NULL;
	m_pszExt = NULL;
	m_fLoaded = false;
	*m_szBlockStart = 0;
	*m_szBlockStop = 0;
	*m_wszBlockStart = 0;
	*m_wszBlockStop = 0;
	m_crBlock = m_crDefault = 0;
	m_pszDelim = NULL;
	m_fCaseMatters = false;
	m_it = kitNone;
	m_ptl = NULL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
ColorSchemeInfo::~ColorSchemeInfo()
{
	if (m_ptl)
		delete m_ptl;
	int cbp = m_vpbp.size();
	for (int ibp = 0; ibp < cbp; ibp++)
		delete m_vpbp[ibp];
	m_vpbp.clear();
	int cki = m_vpki.size();
	for (int iki = 0; iki < cki; iki++)
		delete m_vpki[iki];
	m_vpki.clear();
	if (m_fLoaded && m_pszDesc)
		delete m_pszDesc;
	if (m_fLoaded && m_pszExt)
		delete m_pszExt;
	if (m_pszDelim && m_pszDelim != g_kpszDelimiters)
		delete m_pszDelim;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool ColorSchemeInfo::EnsureLoaded()
{
	if (!m_pszExt)
		return false; // It was not constructed properly.
	if (m_fLoaded)
		return true; // EnsureLoaded has already been called at least once before.

	char * pPos = m_pszExt;
	char * pT = m_pszExt;
	while (!isspace(*pPos++));
	pPos[-1] = 0;
	m_pszExt = new char[pPos - pT];
	if (!m_pszExt)
		return false;
	memmove(m_pszExt, pT, pPos - pT);

	// Set m_pszDesc
	if (*pPos == '\"')
	{
		pT = ++pPos;
		while (*pPos != '\"' && *pPos != 13)
			pPos++;
		*pPos++ = 0;
		m_pszDesc = new char[pPos - pT];
		if (!m_pszDesc)
			return false;
		memmove(m_pszDesc, pT, pPos - pT);
	}

	// Set m_crDefault
	pT = strstr(pPos, "#DEFAULT: ");
	if (pT)
		m_crDefault = (COLORREF)strtol(pT + 10, &pPos, 0);

	// Set m_crBlock, m_szBlockStart, m_szBlockStop, m_wszBlockStart, and m_wszBlockStop
	pT = strstr(pPos, "#BLOCK: ");
	if (pT)
	{
		m_crBlock = (COLORREF)strtol(pT + 8, &pPos, 0);
		pT = ++pPos;
		while (!isspace(*pPos++));
		pT[min(pPos - 1, pT + sizeof(m_szBlockStart) - 1) - pT] = 0;
		lstrcpy(m_szBlockStart, pT);
		::MultiByteToWideChar(CP_ACP, 0, m_szBlockStart, sizeof(m_szBlockStart),
			m_wszBlockStart, sizeof(m_wszBlockStart));

		pT = pPos;
		while (!isspace(*pPos++));
		pT[min(pPos - 1, pT + sizeof(m_szBlockStop) - 1) - pT] = 0;
		lstrcpy(m_szBlockStop, pT);
		::MultiByteToWideChar(CP_ACP, 0, m_szBlockStop, sizeof(m_szBlockStop),
			m_wszBlockStop, sizeof(m_wszBlockStop));
	}

	// Set m_it
	if (strstr(pPos, "#INDENT_SMART:"))
		m_it = kitSmart;
	else if (strstr(pPos, "#INDENT:"))
		m_it = kitNormal;

	// Populate m_vpbp
	BlockPara * pbp;
	pT = strstr(pPos, "#BLOCKP: ");
	while (pT)
	{
		pbp = new BlockPara();
		if (!pbp)
			return false;
		pbp->m_cr = (COLORREF)strtol(pT + 9, &pPos, 0);
		pT = ++pPos;
		while (!isspace(*pPos++));
		pT[min(pPos - 1, pT + sizeof(pbp->m_szStart) - 1) - pT] = 0;
		lstrcpy(pbp->m_szStart, pT);
		if (!isspace(*pPos))
		{
			pT = pPos++;
			while (!isspace(*pPos++));
			pT[min(pPos - 1, pT + sizeof(pbp->m_szStop) - 1) - pT] = 0;
			lstrcpy(pbp->m_szStop, pT);
		}
		m_vpbp.push_back(pbp);
		pT = strstr(pPos, "#BLOCKP: ");
	}

	// Set m_fCaseMatters
	pT = strstr(pPos, "#CASE:");
	if (pT)
	{
		m_fCaseMatters = true;
		pPos = pT;
	}

	// Set m_pszDelim
	pT = strstr(pPos, "#DELIM: ");
	if (pT)
	{
		pT += 8;
		pPos = strchr(pT, 13);
		if (!pPos)
			return false;
		m_pszDelim = new char[pPos - pT + 1];
		if (!m_pszDelim)
			return false;
		*pPos++ = 0;
		memmove(m_pszDelim, pT, pPos - pT);
	}

	// Create m_ptl
	pT = strstr(pPos, "#TYPE: ");
	m_ptl = new CTrieLevel(0);
	if (!m_ptl)
		return false;
	KeywordInfo * pki;
	while (pT)
	{
		pki = new KeywordInfo();
		if (!pki)
			return false;
		pki->m_cr = (COLORREF)strtol(pT + 7, &pPos, 0);
		if (*++pPos == '\"')
		{
			pT = ++pPos;
			while (*++pPos != '\"' && *pPos != 13);
			pT[min(pPos, pT + sizeof(pki->m_szDesc) - 1) - pT] = 0;
			lstrcpy(pki->m_szDesc, pT);
		}
		while (*pPos++ != 10);
		pT = pPos;
		while (*pPos && *pPos != 10)
		{
			if (!m_ptl->AddWord(pPos, pki->m_cr, m_fCaseMatters))
				return false;
			while (*pPos && !isspace(*pPos++));
		}
		if (*pPos != 0)
			*--pPos = 0;
		pki->m_pszWords = new char[pPos - pT + 1];
		if (!pki->m_pszWords)
			return false;
		memmove(pki->m_pszWords, pT, pPos - pT + 1);
		m_vpki.push_back(pki);
		pT = strstr(pPos + 1, "#TYPE: ");
	}

	m_fLoaded = true;
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool ColorSchemeInfo::FindWord(char * prgch, int cchLim, const char * pszDelim,
	COLORREF * pcr, int * pcch)
{
	if (m_ptl)
		return m_ptl->FindWord(prgch, cchLim, pszDelim, pcr, pcch, m_fCaseMatters);
	return false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool ColorSchemeInfo::FindWord(wchar * prgwch, int cchLim, const char * pszDelim,
	COLORREF * pcr, int* pcch)
{
	if (m_ptl)
		return m_ptl->FindWord(prgwch, cchLim, pszDelim, pcr, pcch, m_fCaseMatters);
	return false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool ColorSchemeInfo::FindBlockP(char * prgch, int cchLim, COLORREF * pcr, char * pch)
{
	AssertPtr(pcr);
	AssertPtr(pch);
	int cbp = m_vpbp.size();
	for (int ibp = 0; ibp < cbp; ibp++)
	{
		int cch = strlen(m_vpbp[ibp]->m_szStart);
		if (cch <= cchLim)
		{
			if (strncmp(prgch, m_vpbp[ibp]->m_szStart, cch) == 0)
			{
				*pcr = m_vpbp[ibp]->m_cr;
				// TODO: Figure out why the unicode one has something different here.
				*pch = *m_vpbp[ibp]->m_szStop;
				return true;
			}
		}
	}
	return false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool ColorSchemeInfo::FindBlockP(wchar * prgwch, int cchLim, COLORREF * pcr, wchar * pwch)
{
	AssertPtr(pcr);
	AssertPtr(pwch);
	int cbp = m_vpbp.size();
	for (int ibp = 0; ibp < cbp; ibp++)
	{
		int cch = strlen(m_vpbp[ibp]->m_szStart);
		if (cch < cchLim)
		{
			char * pszSrc = m_vpbp[ibp]->m_szStart - 1;
			char * pszStop = pszSrc + cch + 1;
			wchar * pwszDst = prgwch;
			while (++pszSrc < pszStop)
			{
				if (*pszSrc != (char)*pwszDst++)
					break;
			}
			if (pszSrc == pszStop)
			{
				*pcr = m_vpbp[ibp]->m_cr;
				*pwch = *m_vpbp[ibp]->m_szStop ? *m_vpbp[ibp]->m_szStop : 10;
				return true;
			}
		}
	}
	return false;
}


/***********************************************************************************************
	CColorSchemes methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CColorSchemes::CColorSchemes()
{
	m_pszFileText = NULL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CColorSchemes::~CColorSchemes()
{
	Close();
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CColorSchemes::Load()
{
	char szFilename[MAX_PATH] = {0};
	// See if the color syntax file is specified in the registry.
	HKEY hkey;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, kpszRootRegKey, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
	{
		DWORD dwSize = sizeof(szFilename);
		::RegQueryValueEx(hkey, "Syntax Color Filename", 0, NULL, (BYTE *)szFilename,
			&dwSize);
		::RegCloseKey(hkey);
	}

	// See if the file is valid.
	struct stat sFile;
	bool fFoundFile = true;
	if (stat(szFilename, &sFile) == -1)
	{
		// The registry key did not exist or specified an invalid file, so try to
		// find the default file in the current directory.
		::GetCurrentDirectory(sizeof(szFilename), szFilename);
		strcat_s(szFilename, "\\");
		strcat_s(szFilename, "ZEditColors.txt");
		if (stat(szFilename, &sFile) == -1)
		{
			::GetModuleFileName(NULL, szFilename, sizeof(szFilename));
			char * pSlash = strrchr(szFilename, '\\');
			if (pSlash)
				pSlash[1] = 0;
			strcat_s(szFilename, "ZEditColors.txt");
			if (stat(szFilename, &sFile) == -1)
				fFoundFile = false;
		}
	}

	FILE * pfile = NULL;
	ColorSchemeInfo * pcsiNew;
	if (fFoundFile &&
		sFile.st_size >= 0 &&
		fopen_s(&pfile, szFilename, "rb") == 0 &&
		(m_pszFileText = new char[sFile.st_size + 2]) != NULL &&
		fread(m_pszFileText, 1, sFile.st_size, pfile) == sFile.st_size)
	{
		DWORD dwT;
		if (::RegCreateKeyEx(HKEY_CURRENT_USER, kpszRootRegKey, 0, NULL, 0, KEY_WRITE, NULL, &hkey,
			&dwT) == ERROR_SUCCESS)
		{
			::RegSetValueEx(hkey, "Syntax Color Filename", 0, REG_SZ, (BYTE *)szFilename,
				lstrlen(szFilename));
			::RegCloseKey(hkey);
		}

		m_pszFileText[sFile.st_size] = 0;
		m_pszFileText[sFile.st_size + 1] = 0;
		char * pPos = strstr(m_pszFileText, "#EXT: ");
		char * pT;
		while (pPos)
		{
			pcsiNew = new ColorSchemeInfo(pPos + 6);
			if (!pcsiNew)
				return false;
			pcsiNew->m_pszExt = pPos + 6;
			pT = pPos + 6 - 1;
			while (!isspace(*++pT) && *pT != 13)
				*pT = tolower(*pT);
			m_vpcsi.push_back(pcsiNew);
			if (pPos > m_pszFileText)
				pPos[-1] = 0;
			pPos = strstr(pPos + 6, "#EXT: ");
		}
	}
	if (pfile)
		fclose(pfile);

	// Create an empty one at the end of the vector
	pcsiNew = new ColorSchemeInfo(NULL);
	if (!pcsiNew)
		return false;
	pcsiNew->m_pszDelim = g_kpszDelimiters;
	m_vpcsi.push_back(pcsiNew);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CColorSchemes::Close()
{
	int ccsi = m_vpcsi.size();
	for (int icsi = 0; icsi < ccsi; icsi++)
		delete m_vpcsi[icsi];
	m_vpcsi.clear();
	if (m_pszFileText)
	{
		delete m_pszFileText;
		m_pszFileText = NULL;
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
ColorSchemeInfo * CColorSchemes::GetColorScheme(char * pszExt)
{
	// Loop through the loaded color schemes to see if a match is found
	bool fFound = false;
	int icsi = 0;

	if (pszExt)
	{
		char szExt[MAX_PATH];
		strcpy_s(szExt, pszExt);
		char * pT = szExt - 1;
		while (*++pT)
			*pT = tolower(*pT);

		int ccsi = m_vpcsi.size() - 1;
		if (ccsi >= 0)
		{
			char * pNewLine;
			char * pExt;
			for ( ; icsi < ccsi; icsi++)
			{
				pExt = strstr(m_vpcsi[icsi]->m_pszExt, szExt);
				if (pExt)
				{
					// Make sure the extension occurs before the end of the first line
					pNewLine = strchr(m_vpcsi[icsi]->m_pszExt, 10);
					if (!pNewLine || pExt < pNewLine)
					{
						fFound = true;
						break;
					}
				}
			}
		}
	}
	if (!fFound)
	{
		// No match was found, so return a pointer to the empty color scheme
		return m_vpcsi.back();
	}

	// We found a match, so make sure it is loaded and return it.
	if (!m_vpcsi[icsi]->EnsureLoaded())
		return NULL;
	return m_vpcsi[icsi];
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
ColorSchemeInfo * CColorSchemes::GetColorScheme(UINT ics)
{
	if (ics < 0)
		return m_vpcsi.front();
	if ((UINT)ics >= m_vpcsi.size())
		return m_vpcsi.back();
	return m_vpcsi[ics];
}


/***********************************************************************************************
	FindItem methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
FindItem::FindItem()
{
	memset(this, 0, sizeof(FindItem));
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
FindItem::~FindItem()
{
	if (pwszFindWhat)
	{
		delete pwszFindWhat;
		pwszFindWhat = NULL;
	}
	if (pwszReplaceWith)
	{
		delete pwszReplaceWith;
		pwszReplaceWith = NULL;
	}
}


/***********************************************************************************************
	CFileGlobals methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CFileGlobals::CFileGlobals()
{
	// Global member variables are already initialized to 0.
	m_dxpTab = 96;
	SetRect(&m_rcMargin, 4, knWedgeHeight, 4, knWedgeHeight);
	m_chSpace = ' ';
	m_chPara = ' '; // (UCHAR)'¶'

	::RegisterWindowMessage(MSH_MOUSEWHEEL);
	UINT uiMsgScrollLines = ::RegisterWindowMessage(MSH_SCROLL_LINES);
	HWND hwndWheel = ::FindWindow(MSH_WHEELMODULE_CLASS, MSH_WHEELMODULE_TITLE);
	if (hwndWheel)
		m_nScrollLines = (int)::SendMessage(hwndWheel, uiMsgScrollLines, 0, 0);
	else
		m_nScrollLines = 3;
	m_hwndToolTip = NULL;
	m_iColumnDrag = -1;
	m_hwndViewCodes = false;
	m_cchCodesBefore = 5;
	m_cchCodesAfter = 5;
	m_fFlatScrollbars = true;

	m_pfnSetLayeredWindowAttributes = NULL;
	HMODULE hmodUser = ::LoadLibrary("user32.dll");
	if (hmodUser)
	{
		m_pfnSetLayeredWindowAttributes = (PfnSetLayeredWindowAttributes)
			::GetProcAddress(hmodUser, "SetLayeredWindowAttributes");
		::FreeLibrary(hmodUser);
	}

	// Since SetLayeredWindowAttributes is only valid for _WIN32_WINNT >= 0x500,
	// we can use this as a test for other Version 5 features.
	m_fCanUseVer5 = m_pfnSetLayeredWindowAttributes != NULL;

	m_wpOldStatusProc = m_wpOldToolProc = NULL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CFileGlobals::~CFileGlobals()
{
	if (m_hwndToolTip)
	{
		::DestroyWindow(m_hwndToolTip);
		m_hwndToolTip = NULL;
	}
	if (m_hFontOld)
	{
		::DeleteObject(m_hFontOld);
		m_hFontOld = NULL;
	}
	if (m_hFont)
	{
		::DeleteObject(m_hFont);
		m_hFont = NULL;
	}
	if (m_hFontFixedOld)
	{
		::DeleteObject(m_hFontFixedOld);
		m_hFontFixedOld = NULL;
	}
	if (m_hFontFixed)
	{
		::DeleteObject(m_hFontFixed);
		m_hFontFixed = NULL;
	}
	if (m_himlCM)
	{
		::DeleteObject(m_himlCM);
		m_himlCM = NULL;
	}

	for (int icm = m_vpcm.size() - 1; icm >= 0; icm--)
		delete m_vpcm[icm];
	m_vpcm.clear();

	for (int iztl = m_vpztl.size() - 1; iztl >= 0; iztl--)
		delete m_vpztl[iztl];
	m_vpztl.clear();
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFileGlobals::DrawColumnMarkers(HDC hdc, bool fErase, RECT & rect, int nXOffset)
{
	int ccm = m_vpcm.size();
	for (int icm = 0; icm < ccm; icm++)
	{
		AssertPtr(m_vpcm[icm]);
		m_vpcm[icm]->OnDraw(hdc, fErase, rect, nXOffset);
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CFileGlobals::GetColumnMarker(POINT point)
{
	int ccm = m_vpcm.size();
	for (int icm = 0; icm < ccm; icm++)
	{
		AssertPtr(m_vpcm[icm]);
		if (point.x >= m_vpcm[icm]->m_nPixelOffset &&
			point.x <= m_vpcm[icm]->m_nPixelOffset + knWedgeWidth)
		{
			return icm;
		}
	}
	return -1;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CFileGlobals::SetColumnMarkerPos(CZEditFrame * pzef, HWND hwnd, POINT point, int nXOffset)
{
	AssertPtr(pzef);
	Assert((UINT)m_iColumnDrag < (UINT)m_vpcm.size());
	CColumnMarker * pcm = m_vpcm[m_iColumnDrag];
	AssertPtr(pcm);

	int iOldColumn = pcm->m_iColumn;
	int iNewColumn = (point.x - nXOffset) / m_ri.m_tm.tmAveCharWidth;
	if (iOldColumn != iNewColumn)
	{
		// See if any other markers are at the old column position so we
		// know whether ot not to erase the arrows at the top and bottom of
		// the window.
		bool fHideArrows = true;
		for (UINT icol = 0; icol < m_vpcm.size(); icol++)
		{
			if (icol != m_iColumnDrag && m_vpcm[icol]->m_iColumn == iOldColumn)
			{
				fHideArrows = false;
				break;
			}
		}

		// Erase the old column location.
		RECT rc;
		::GetClientRect(hwnd, &rc);
		HDC hdc = ::GetDC(hwnd);
		pcm->OnDraw(hdc, fHideArrows, rc, nXOffset);
		// Draw the new column location.
		pcm->m_iColumn = iNewColumn;
		pcm->OnDraw(hdc, false, rc, nXOffset);
		::ReleaseDC(hwnd, hdc);

		// Update the text in the tooltip.
		Assert((UINT)g_fg.m_iColumnDrag < (UINT)g_fg.m_vpcm.size());
		char szText[10];
		TOOLINFO ti = {sizeof(ti)};
		_itoa_s(iNewColumn, szText, 10);
		ti.lpszText = szText;
		ti.hwnd = pzef->GetHwnd();
		ti.uId = (UINT)pzef->GetCurrentDoc()->GetCurrentView()->GetHwnd();
		::SendMessage(m_hwndToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

		// Update the position of the tooltip.
		POINT pt = { iNewColumn * m_ri.m_tm.tmAveCharWidth + nXOffset + m_rcMargin.left };
		RECT rect;
		HWND hwndEdit = pzef->GetCurrentDoc()->GetHwnd();
		::GetClientRect(hwndEdit, &rect);
		::MapWindowPoints(hwndEdit, NULL, (POINT *)&rect, 2);
		pt.x += rect.left;
		pt.y = rect.top;
		::GetWindowRect(m_hwndToolTip, &rect);
		pt.y -= (rect.bottom - rect.top + 2);
		pt.x -= (rect.right - rect.left - 1) / 2;
		::SendMessage(m_hwndToolTip, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));
		pzef->OnColumnMarkerDrag();
	}
	return iNewColumn;
}


/*----------------------------------------------------------------------------------------------
	Draw the specified control in memory, then paste it to the window.
----------------------------------------------------------------------------------------------*/
void CFileGlobals::DrawControl(HWND hwnd, WNDPROC wndProc)
{
	RECT rect;
	::GetWindowRect(hwnd, &rect);
	::MapWindowPoints(NULL, hwnd, (POINT *)&rect, 2);
	if (!::IsRectEmpty(&rect))
	{
		HDC hdc = ::GetDC(hwnd);
		if (hdc)
		{
			HDC hdcMem = ::CreateCompatibleDC(hdc);
			if (hdcMem)
			{
				HBITMAP hbmp = ::CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
				if (hbmp)
				{
					HBRUSH hbr = ::CreateSolidBrush(::GetSysColor(COLOR_3DFACE));
					if (hbr)
					{
						HBITMAP hbmpOld = (HBITMAP)::SelectObject(hdcMem, hbmp);
						::FillRect(hdcMem, &rect, hbr);
						::CallWindowProc(wndProc, hwnd, WM_PAINT, (WPARAM)hdcMem, 0);
						::BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
						::SelectObject(hdcMem, hbmpOld);
						::DeleteObject(hbr);
						::ValidateRect(hwnd, NULL);
					}
					::DeleteObject(hbmp);
				}
				::DeleteDC(hdcMem);
			}
			::ReleaseDC(hwnd, hdc);
		}
	}
}


/*----------------------------------------------------------------------------------------------
	Returns the clipboard format ID for "FileGroupDescriptor".
----------------------------------------------------------------------------------------------*/
CLIPFORMAT CFileGlobals::GetClipFileGroup()
{
	if (m_cfFileGroup == 0)
		m_cfFileGroup = ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
	return m_cfFileGroup;
}


/*----------------------------------------------------------------------------------------------
	Returns the clipboard format ID for "FileContents".
----------------------------------------------------------------------------------------------*/
CLIPFORMAT CFileGlobals::GetClipFileContents()
{
	if (m_cfFileContents == 0)
		m_cfFileContents = ::RegisterClipboardFormat(CFSTR_FILECONTENTS);
	return m_cfFileContents;
}


/*----------------------------------------------------------------------------------------------
	Returns the clipboard format ID for "ZEditFileDragDrop".
----------------------------------------------------------------------------------------------*/
CLIPFORMAT CFileGlobals::GetClipZEditFile()
{
	if (m_cfZEditFile == 0)
		m_cfZEditFile = ::RegisterClipboardFormat("ZEditFileDragDrop");
	return m_cfZEditFile;
}


/*----------------------------------------------------------------------------------------------
	OpenOutlookAttachment will return true if the DataObject contained Outlook information.
----------------------------------------------------------------------------------------------*/
bool CFileGlobals::OpenOutlookAttachment(CZEditFrame * pzef, IDataObject * pdo)
{
	AssertPtr(pdo);

	bool fSuccess = false;
	FORMATETC feGroup = { g_fg.GetClipFileGroup(), NULL, DVASPECT_CONTENT, -1,
		TYMED_HGLOBAL };
	STGMEDIUM smGroup = { 0 };
	if (SUCCEEDED(pdo->GetData(&feGroup, &smGroup)))
	{
		FILEGROUPDESCRIPTOR * pfgd = (FILEGROUPDESCRIPTOR *)::GlobalLock(smGroup.hGlobal);
		if (pfgd)
		{
			// Loop through each of the files in the clipboard.
			FORMATETC feFile = { GetClipFileContents(), NULL, DVASPECT_CONTENT, -1,
				TYMED_ISTREAM };
			for (UINT iFile = 0; iFile < pfgd->cItems; iFile++)
			{
				// Get the contents for this file.
				feFile.lindex = iFile;
				STGMEDIUM smFile = { 0 };
				STATSTG stat;
				if (SUCCEEDED(pdo->GetData(&feFile, &smFile)))
				{
					if (SUCCEEDED(smFile.pstm->Stat(&stat, STATFLAG_NONAME)))
					{
						// We're only going to look at files smaller than 4 GB.
						if (stat.cbSize.HighPart == 0)
						{
							DWORD cbSize = stat.cbSize.LowPart;
							// NOTE: If the file is actually opened, the document will own
							// this character array, so it should not be deleted here.
							char * prgch = new char[cbSize + 1];
							DWORD cbRead;
							if (SUCCEEDED(smFile.pstm->Read(prgch, cbSize, &cbRead)) &&
								cbRead == cbSize)
							{
								prgch[cbSize] = 0;
								fSuccess = pzef->CreateNewFile(pfgd->fgd[iFile].cFileName,
									NULL, kftDefault, prgch, cbSize);
							}
						}
					}
					if (smFile.pUnkForRelease)
						smFile.pUnkForRelease->Release();
				}
				if (!fSuccess)
					break;
			}
			::GlobalUnlock(smGroup.hGlobal);
		}
	}
	return fSuccess;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFileGlobals::Draw3dRect(HDC hdc, RECT * prc, COLORREF clrTopLeft,
	COLORREF clrBottomRight)
{
	RECT left =   {prc->left, prc->top, prc->left + 1, prc->bottom - 1};
	RECT top =	  {prc->left, prc->top, prc->right - 1, prc->top + 1};
	RECT right =  {prc->right - 1, prc->top, prc->right, prc->bottom};
	RECT bottom = {prc->left, prc->bottom - 1, prc->right, prc->bottom};

	::SetBkColor(hdc, clrTopLeft);
	::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &left, NULL, 0, NULL);
	::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &top, NULL, 0, NULL);
	::SetBkColor(hdc, clrBottomRight);
	::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &right, NULL, 0, NULL);
	::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &bottom, NULL, 0, NULL);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
char * CFileGlobals::FixString(char * pszFileString)
{
	char * pszBefore = pszFileString - 1;
	char * pszLastString = NULL;

	while ((pszBefore = strchr(pszBefore + 1, ' ')) != NULL)
	{
		int cchOffset = 1;
		if (pszBefore[cchOffset] == '"')
			cchOffset++;
		if ((pszBefore[cchOffset + 1] == ':' || pszBefore[cchOffset + 1] == '\\') &&
			pszBefore[cchOffset + 2] == '\\')
		{
			*pszBefore = 0;
			pszLastString = pszBefore + 1;
		}
	}
	if (!pszLastString)
		pszFileString[lstrlen(pszFileString) + 1] = '\0';
	else
		pszLastString[lstrlen(pszLastString) + 1] = '\0';
	return pszFileString;
}


/*----------------------------------------------------------------------------------------------
	Create a tooltip window if necessary and return its handle.
----------------------------------------------------------------------------------------------*/
HWND CFileGlobals::GetToolTipHwnd()
{
	if (m_hwndToolTip == NULL)
	{
		m_hwndToolTip = ::CreateWindow(TOOLTIPS_CLASS, NULL, WS_POPUP,
			0, 0, 0, 0, NULL, NULL, g_fg.m_hinst, NULL);
	}
	return m_hwndToolTip;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CFileGlobals::LoadTypeLibrary(wchar * pszFilename)
{
	AssertPtr(pszFilename);

	ZTypeLibrary * pztl = new ZTypeLibrary();
	if (!pztl)
		return -1;
	if (!pztl->LoadTypeLibrary(pszFilename))
		return -1;
	m_vpztl.push_back(pztl);
	return m_vpztl.size() - 1;
}


/***********************************************************************************************
	RenderInfo methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void RenderInfo::Initialize(HDC hdc, int cchSpacesInTab)
{
	char rgch[256];
	for (int i = 0; i < 256; i++)
		rgch[i] = (char)i;
	GCP_RESULTS gcp = { sizeof(gcp) };
	gcp.lpDx = m_rgnCharWidth;
	gcp.nGlyphs = sizeof(m_rgnCharWidth) / sizeof(char);
	::GetCharacterPlacement(hdc, rgch, sizeof(rgch) / sizeof(char), 0, &gcp, 0);
	::GetTextMetrics(hdc, &m_tm);

	m_cchSpacesInTab = cchSpacesInTab;
	m_dxpTab = m_cchSpacesInTab * m_tm.tmAveCharWidth;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int RenderInfo::CharOut(HDC hdc, char ch)
{
	int dx = m_rgnCharWidth[(unsigned char)ch];
	if (dx == 0)
	{
		dx = m_tm.tmAveCharWidth;
		POINT pt;
		::MoveToEx(hdc, 0, 0, &pt);
		RECT rc = { pt.x, pt.y, pt.x + dx, pt.y + m_tm.tmHeight };
		::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
		::MoveToEx(hdc, pt.x + dx, pt.y, NULL);
	}
	else
	{
		::TextOut(hdc, 0, 0, &ch, 1);
	}
	return dx;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int RenderInfo::GetCharWidth(char ch)
{
	int dx = m_rgnCharWidth[(unsigned char)ch];
	if (dx == 0)
		dx = m_tm.tmAveCharWidth;
	return dx;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int RenderInfo::AddCharWidth(int nWidth, char * prgch)
{
	if (*prgch == '\t')
		return ((nWidth / m_dxpTab) + 1) * m_dxpTab;
	return nWidth + GetCharWidth(*prgch);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int RenderInfo::AddCharWidth(int nWidth, wchar *& prgch, wchar * prgchLim, HDC hdc)
{
	SIZE size;
	wchar ch = *prgch;
	if (ch == 0xFEFF)
		return nWidth;
	if (ch < 0xd800 || ch > 0xdbff)
	{
		if (ch == '\t')
			return ((nWidth / m_dxpTab) + 1) * m_dxpTab;
		if (ch <= 0xFF)
			return nWidth + GetCharWidth((char)*prgch);
		::GetTextExtentPoint32W(hdc, &ch, 1, &size);
		return nWidth + size.cx;
	}
	if (prgch < prgchLim - 1)
	{
		// This is the first character of a surrogate pair.
		::GetTextExtentPoint32W(hdc, prgch++, 2, &size);
		return nWidth + size.cx;
	}
	return nWidth;
}


/***********************************************************************************************
	ZTypeInfo methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
ZTypeInfo::~ZTypeInfo()
{
	MyStringSet::reverse_iterator pos;
	for (pos = m_vpmsMethod.rbegin(); pos != m_vpmsMethod.rend(); pos++)
		delete *pos;
	for (pos = m_vpmsAttribute.rbegin(); pos != m_vpmsAttribute.rend(); pos++)
		delete *pos;
}


/***********************************************************************************************
	ZTypeLibrary methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
ZTypeLibrary::~ZTypeLibrary()
{
	ZTypeInfoMap::reverse_iterator pos;
	for (pos = m_mapTypeInfo.rbegin(); pos != m_mapTypeInfo.rend(); pos++)
		delete pos->second;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool ZTypeLibrary::LoadTypeLibrary(const wchar * pszFilename)
{
	AssertPtr(pszFilename);

	try
	{
		CComPtr<ITypeLib> qtlib;
		HRESULT hr = ::LoadTypeLibEx(pszFilename, REGKIND_NONE, &qtlib);
		if (FAILED(hr))
			return false;

		UINT ctinfo = qtlib->GetTypeInfoCount();
		for (UINT itinfo = 0; itinfo < ctinfo; itinfo++)
		{
			TYPEKIND tk;
			if (FAILED(qtlib->GetTypeInfoType(itinfo, &tk)))
				return false;

			if (tk == TKIND_ENUM || tk == TKIND_COCLASS)
			{
				CComPtr<ITypeInfo> qtinfo;
				if (FAILED(qtlib->GetTypeInfo(itinfo, &qtinfo)))
					return false;
				if (!LoadTypeInfo(qtinfo, CComBSTR()))
					return false;
			}
		}

		CComBSTR bstrName;
		if (FAILED(qtlib->GetDocumentation(MEMBERID_NIL, &bstrName, NULL, NULL, NULL)))
			return false;
		m_sName = bstrName;
		m_sFilename = pszFilename;
	}
	catch (...)
	{
		return false;
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool ZTypeLibrary::LoadTypeInfo(ITypeInfo * ptinfo, CComBSTR & bstrName)
{
	AssertPtr(ptinfo);

	if (bstrName == NULL)
	{
		if (FAILED(ptinfo->GetDocumentation(MEMBERID_NIL, &bstrName, NULL, NULL, NULL)))
			return false;
		ZTypeInfo * pzti = new ZTypeInfo;
		m_mapTypeInfo.insert(std::make_pair(bstrName, pzti));
		pzti->m_sName = bstrName;
	}
	ZTypeInfoMap::const_iterator pos = m_mapTypeInfo.find(bstrName);
	Assert(pos != m_mapTypeInfo.end());
	ZTypeInfo * pzti = pos->second;
	AssertPtr(pzti);

	TYPEATTR * pta = NULL;
	if (FAILED(ptinfo->GetTypeAttr(&pta)))
		return false;

	HRESULT hr = S_OK;
	MEMBERID mid;
	CComBSTR bstrT;

	int cFunctions = pta->cFuncs;
	if (cFunctions > 0)
	{
		for (int iFunction = 0; iFunction < cFunctions; iFunction++)
		{
			FUNCDESC * pfd = NULL;
			hr = ptinfo->GetFuncDesc(iFunction, &pfd);
			if (SUCCEEDED(hr))
			{
				mid = pfd->memid;
				hr = ptinfo->GetDocumentation(mid, &bstrT, NULL, NULL, NULL);
				ptinfo->ReleaseFuncDesc(pfd);
			}
			if (FAILED(hr))
				break;

			// Add the new method to the set.
			MyString * pms = new MyString();
			pms->Attach(bstrT.Detach());
			std::pair<MyStringSet::const_iterator, bool> prT = pzti->m_vpmsMethod.insert(pms);
			if (!prT.second)
			{
				// This string was already added to the set, so we need to delete it now.
				delete pms;
			}
		}
	}
	int cVariables = pta->cVars;
	if (cVariables > 0 && SUCCEEDED(hr))
	{
		for (int iVariable = 0; iVariable < cVariables; iVariable++)
		{
			VARDESC * pvd = NULL;
			hr = ptinfo->GetVarDesc(iVariable, &pvd);
			if (SUCCEEDED(hr))
			{
				mid = pvd->memid;
				hr = ptinfo->GetDocumentation(mid, &bstrT, NULL, NULL, NULL);
				ptinfo->ReleaseVarDesc(pvd);
			}
			if (FAILED(hr))
				break;

			// Add the new attribute to the set.
			MyString * pms = new MyString();
			pms->Attach(bstrT.Detach());
			std::pair<MyStringSet::const_iterator, bool> prT = pzti->m_vpmsAttribute.insert(pms);
			if (!prT.second)
			{
				// This string was already added to the set, so we need to delete it now.
				delete pms;
			}
		}
	}
	int cImplTypes = pta->cImplTypes;
	if (cImplTypes && SUCCEEDED(hr))
	{
		for (int iImplType = 0; iImplType < cImplTypes; iImplType++)
		{
			HREFTYPE ht = 0;
			hr = ptinfo->GetRefTypeOfImplType(iImplType, &ht);
			CComPtr<ITypeInfo> qtinfoNew;
			if (SUCCEEDED(hr))
				hr = ptinfo->GetRefTypeInfo(ht, &qtinfoNew);
			if (SUCCEEDED(hr))
				LoadTypeInfo(qtinfoNew, bstrName);
			else
				break;
		}
	}

	ptinfo->ReleaseTypeAttr(pta);
	return (SUCCEEDED(hr));
}