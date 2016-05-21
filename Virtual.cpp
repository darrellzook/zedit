#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"
#include "SyntaxDlg.h"
#ifdef SPELLING
#include "SpellCheck.h"
#endif // SPELLING


/***********************************************************************************************
	CZEditFrame methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZEditFrame::OnColumnMarkerDrag()
{
	CZDoc * pzd = m_pzdFirst;
	while (pzd)
	{
		pzd->OnColumnMarkerDrag();
		pzd = pzd->Next();
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
long CZEditFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT nCommand = LOWORD(wParam);
	switch (nCommand)
	{
	case ID_HELP_ABOUT:
		return ::DialogBoxParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_ABOUT), m_hwnd,
			AboutProc, (LPARAM)this);

	case ID_FILE_CLOSE:
		return CloseFile(-1);

	case ID_FILE_CLOSEALL:
		return CloseAll(false);

	case ID_FILE_PROPERTIES:
		return OnFileProperties(m_pzdCurrent);

	case ID_FILE_EXIT:
		return (PostMessage(m_hwnd, WM_CLOSE, 0, 0));

	case ID_EDIT_COPY:
		{
			UINT ichSelStart;
			UINT ichSelStop;
			AssertPtr(m_pzdCurrent);
			m_pzdCurrent->GetSelection(&ichSelStart, &ichSelStop);
			return m_pzdCurrent->Copy(ichSelStart, ichSelStop, false);
		}

	case ID_EDIT_CUT:
		{
			UINT ichSelStart;
			UINT ichSelStop;
			AssertPtr(m_pzdCurrent);
			m_pzdCurrent->GetSelection(&ichSelStart, &ichSelStop);
			return m_pzdCurrent->Cut(ichSelStart, ichSelStop);
		}

	case ID_EDIT_FIND:
		return m_pfd->ShowDialog(false);

	case ID_EDIT_FINDNEXT:
		return m_pfd->OnFind(true);

	case ID_EDIT_FINDPREV:
		return m_pfd->OnFind(false);

	case ID_EDIT_OPEN:
		Assert(m_szContextFile);
		ReadFile(m_szContextFile, kftDefault);
		return 0;

	case ID_EDIT_PASTE:
		{
			UINT ichSelStart;
			UINT ichSelStop;
			m_pzdCurrent->GetSelection(&ichSelStart, &ichSelStop);
			m_pzdCurrent->Paste(ichSelStart, ichSelStop);
			UpdateTextPos();
			return 0;
		}

	case ID_OPTIONS_FONT:
		GetFont(m_pzdCurrent->GetCurrentView()->IsBinary());
		return 0;

	case ID_OPTIONS_GOTO:
		return DialogBoxParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_GOTOLINE), m_hwnd,
			LineProc, (LPARAM)this);

	case ID_OPTIONS_COLUMNS:
		return DialogBoxParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_COLUMNMARKER), m_hwnd,
			ColumnProc, (LPARAM)this);

	case ID_WINDOW_PREV:
		SelectFile(m_pzt->GetCurSel() - 1);
		return 0;

	case ID_WINDOW_NEXT:
		SelectFile(m_pzt->GetCurSel() + 1);
		return 0;

	case ID_WINDOW_MANAGER:
		if (m_hwndTabMgrDlg)
		{
			::SetFocus(m_hwndTabMgrDlg);
		}
		else
		{
			m_hwndTabMgrDlg = ::CreateDialogParam(g_fg.m_hinst,
				MAKEINTRESOURCE(IDD_FILEMANAGER), m_hwnd, FileManagerProc, (LPARAM)this);
		}
		return 0;

	case ID_WINDOW_SPLITHORIZ:
		m_pzdCurrent->GetFrame()->SplitWindow(true);
		return 0;

	case ID_WINDOW_SPLITVERT:
		m_pzdCurrent->GetFrame()->SplitWindow(false);
		return 0;

	case ID_TOOLS_SORT:
		return DialogBoxParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_SORT), m_hwnd,
			SortProc, (LPARAM)this);

#ifdef SPELLING
	case ID_TOOLS_SPELL:
		return DialogBox(g_fg.m_hinst, MAKEINTRESOURCE(IDD_SPELL), m_hwnd, SpellProc);
#endif // SPELLING

	case ID_TOOLS_SYNTAX:
		{
			CSyntaxDlg csd(this, &g_cs);
			return csd.DoModal();
		}

	case ID_TOOLS_INSERTCHAR:
		return DialogBoxParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_ENTERCHAR), m_hwnd,
			EnterCharacterProc, (LPARAM)this);

	case ID_TOOLS_SHOWCODES:
		if (g_fg.m_hwndViewCodes)
		{
			::DestroyWindow(g_fg.m_hwndViewCodes);
			g_fg.m_hwndViewCodes = NULL;
		}
		else
		{
			g_fg.m_hwndViewCodes = ::CreateDialog(g_fg.m_hinst,
				MAKEINTRESOURCE(IDD_VIEWCODES), m_hwnd, ViewCodesProc);
			UpdateTextPos();
		}
		return 0;

	case ID_TOOLS_FINDINFILES:
		return m_pffd->ShowDialog();

	case ID_TOOLS_CHANGECASE:
		//return m_pzdCurrent->ChangeCase(0, -1, 0);
		return DialogBoxParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_CHANGECASE), m_hwnd,
			ChangeCaseProc, (LPARAM)this);

	case ID_TOOLS_FIXLINE:
		return DialogBoxParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_FIXLINES), m_hwnd,
			FixLinesProc, (LPARAM)this);

	case ID_FILE_NEW:
		if (lParam == -1)
		{
			// Open a new top-level frame window.
			CZEditFrame * pzef = new CZEditFrame();
			pzef->Create(NULL, NULL, true);
			g_pzea->AddFrame(pzef);
			g_pzea->SetCurrentFrame(pzef);
			::SetForegroundWindow(pzef->GetHwnd());
			return 0;
		}
		return CreateNewFile("Untitled", NULL, kftDefault);

	case ID_FILE_NEWTYPE:
		return CreateNewFile("Untitled", NULL,
			(FileType)DialogBoxParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_NEWFILE), m_hwnd,
			NewFileProc, (LPARAM)m_pzdCurrent));

	case ID_FILE_OPEN:
		{
			char szFilename[MAX_PATH] = {0};
			FileType ft = GetFile(FALSE, szFilename);
			if (ft == kftAnsi)
				ft = kftDefault;
			if (ft != -1)
				ReadFile(szFilename, ft);
			return 0;
		}

	case ID_FILE_PAGESETUP:
		return DialogBoxParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_PAGESETUP), m_hwnd,
			PageProc, (LPARAM)this);

	case ID_FILE_PRINT:
		Print();
		return 0;

	case ID_EDIT_REDO:
		return m_pzdCurrent->Redo();

	case ID_EDIT_REPLACE:
		return m_pfd->ShowDialog(TRUE);

	case ID_FILE_SAVE:
		return SaveFile(m_pzdCurrent, m_pzt->GetCurSel(), false, NULL);

	case ID_FILE_SAVEAS:
		return SaveFile(m_pzdCurrent, m_pzt->GetCurSel(), true, NULL);

	case ID_FILE_SAVEALL:
		return SaveAll();

	case ID_EDIT_SELECTALL:
		m_pzdCurrent->SetSelection(0, -1, true, true);
		UpdateTextPos();
		m_pfd->ResetFind();
		return 0;

	case ID_EDIT_BINARY:
		AssertPtr(m_pzdCurrent);
		AssertPtr(m_pzdCurrent->GetFrame());
		m_pzdCurrent->GetFrame()->ToggleCurrentView();
		return 0;

	case ID_OPTIONS_TAB:
		return DialogBoxParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_TABS), m_hwnd,
			TabProc, (LPARAM)this);

	case ID_EDIT_UNDO:
		return (m_pzdCurrent->Undo());

	case ID_OPTIONS_WRAP:
		{
			bool fWrap = m_pzdCurrent ? !m_pzdCurrent->GetWrap() : !m_fWrap;
			m_pzdCurrent->SetWrap(fWrap);
			SetStatusText(ksboWrap, fWrap ? "Wrap On" : "Wrap Off");
		}
		return 0;

	default:
		if (nCommand >= ID_FILE_FILE1 && nCommand <= ID_FILE_FILE1 + kcMaxFiles)
		{
			int iFile = nCommand - ID_FILE_FILE1;
			if (!ReadFile(m_szFileList[iFile], (FileType)m_szFileList[iFile][sizeof(m_szFileList[0]) - 1]))
			{
				memmove(m_szFileList[iFile], m_szFileList[iFile + 1], sizeof(m_szFileList[0]) * (kcMaxFiles - iFile - 1));
				memset(m_szFileList[kcMaxFiles - 1], 0, sizeof(m_szFileList[0]));
			}
		}
#ifdef SPELLING
		else if (nCommand >= SUGGESTION_START && nCommand <= SUGGESTION_STOP)
		{
			int cch;
			int cchMore;
			char * prgch;
			char * prgchMore;
			m_sc.GetSuggestions(&cch, &prgch, &cchMore, &prgchMore);
			int iPos = nCommand - SUGGESTION_START;
			if (iPos >= cch)
			{
				prgch = prgchMore;
				iPos -= cch;
			}
			while (iPos--)
				prgch += strlen(prgch) + 1;
			m_pzdCurrent->SetSelection(m_iWordStart, m_iWordStart + m_cchWord, false, false);
			m_pzdCurrent->InsertText(m_iWordStart, prgch, strlen(prgch), m_cchWord, true, kurtSpell);
		}
#endif // SPELLING
		else if (nCommand >= ID_OPENAS_ANSI && nCommand <= ID_OPENAS_DEFAULT)
		{
			m_pzdCurrent->ReOpenFile((FileType)(nCommand - ID_OPENAS_ANSI));
		}
		else if (nCommand >= ID_GOTOFILE_START && nCommand <= ID_GOTOFILE_STOP)
		{
			SelectFile(nCommand - ID_GOTOFILE_START);
		}
		else if (nCommand == ID_OPTIONS_DEFAULTENC_ANSI)
		{
			SetDefaultEncoding(kftAnsi);
		}
		else if (nCommand == ID_OPTIONS_DEFAULTENC_UTF8)
		{
			SetDefaultEncoding(kftUnicode8);
		}
		else if (nCommand == ID_OPTIONS_DEFAULTENC_UTF16)
		{
			SetDefaultEncoding(kftUnicode16);
		}
		return 0;
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
long CZEditFrame::OnDropFiles(HDROP hDropInfo, BOOL fContext)
{
	int nCommand = ID_OPENAS_DEFAULT;
	if (fContext)
	{
		POINT pt;
		GetCursorPos(&pt);
		HMENU hSubMenu = m_pzmContext->GetSubMenu(kcmFileDrop);
		SetMenuDefaultItem(hSubMenu, 0, TRUE);
		nCommand = TrackPopupMenuEx(hSubMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD |
			TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x, pt.y, m_hwnd, NULL);
		if (nCommand == 0)
			return 0;
	}

	int cFiles = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);

	if (nCommand >= ID_OPENAS_ANSI && nCommand <= ID_OPENAS_DEFAULT) // Open As
	{
		char szFilename[MAX_PATH];
		for (int iFile = 0; iFile < cFiles; iFile++)
		{
			ZeroMemory(szFilename, sizeof(szFilename));
			DragQueryFile(hDropInfo, iFile, szFilename, sizeof(szFilename));
			ReadFile(szFilename, (FileType)(nCommand - ID_OPENAS_ANSI));
		}
	}
	else
	{
		if (ID_OPENAS_FILENAME != nCommand && ID_OPENAS_PATHNAME != nCommand)
			return 0;
		int cchText = (MAX_PATH + 2) * cFiles + 1;
		char * pszText = new char[cchText];
		if (pszText)
		{
			pszText[0] = 0;
			char szFilename[MAX_PATH];
			for (int iFile = 0; iFile < cFiles; iFile++)
			{
				DragQueryFile(hDropInfo, iFile, szFilename, MAX_PATH);
				if (ID_OPENAS_FILENAME == nCommand) // Insert Filename(s)
				{
					char * pSlash = strrchr(szFilename, '\\');
					if (!pSlash)
						pSlash = szFilename - 1;
					strcat_s(pszText, cchText, pSlash + 1);
				}
				else // Insert Pathname(s)
				{
					strcat_s(pszText, cchText, szFilename);
				}
				strcat_s(pszText, cchText, "\r\n");
			}

			UINT ichSelStart;
			UINT ichSelStop;
			m_pzdCurrent->GetSelection(&ichSelStart, &ichSelStop);
			int cch = lstrlen(pszText);
			/*if (m_pzdCurrent->GetFileType() != kftAnsi)
				Convert8to16(pszText, (wchar *)pszText, cch + 1);*/
			m_pzdCurrent->InsertText(ichSelStart, pszText, cch, ichSelStop - ichSelStart, true, kurtPaste);
			delete pszText;
		}
	}
	DragFinish(hDropInfo);
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
long CZEditFrame::OnInitMenuPopup(HMENU hMenu, UINT nIndex, BOOL fSysMenu)
{
	AssertPtr(m_pzm);

	if (hMenu == m_pzm->GetSubMenu(nIndex & 0x7fff))
	{
		switch (nIndex & 0x7fff)
		{
		default:
			break;

		case 1: // Edit submenu
			{
				// Enable/Disable cut and copy based on whether there is a selection or not
				int nEnable = MF_GRAYED;
				if (m_pzdCurrent->GetSelection(NULL, NULL) != kstNone)
					nEnable = MF_ENABLED;
				::EnableMenuItem(hMenu, ID_EDIT_CUT, nEnable);
				::EnableMenuItem(hMenu, ID_EDIT_COPY, nEnable);

				::EnableMenuItem(hMenu, ID_EDIT_PASTE,
					m_pzdCurrent->CanPaste() ? MF_ENABLED : MF_GRAYED);
			}
			break;

		case 2: // Options submenu
			::CheckMenuItem(hMenu, ID_OPTIONS_WRAP,
				MF_BYCOMMAND | (m_pzdCurrent->GetWrap() ? MF_CHECKED : MF_UNCHECKED));
			::CheckMenuItem(hMenu, ID_OPTIONS_DEFAULTENC_ANSI,
				MF_BYCOMMAND | (g_fg.m_defaultEncoding == kftAnsi ? MF_CHECKED : MF_UNCHECKED));
			::CheckMenuItem(hMenu, ID_OPTIONS_DEFAULTENC_UTF8,
				MF_BYCOMMAND | (g_fg.m_defaultEncoding == kftUnicode8 ? MF_CHECKED : MF_UNCHECKED));
			::CheckMenuItem(hMenu, ID_OPTIONS_DEFAULTENC_UTF16,
				MF_BYCOMMAND | (g_fg.m_defaultEncoding == kftUnicode16 ? MF_CHECKED : MF_UNCHECKED));
			break;

		case 3: // Tools submenu
			::CheckMenuItem(hMenu, ID_TOOLS_SHOWCODES,
				MF_BYCOMMAND | (g_fg.m_hwndViewCodes ? MF_CHECKED : MF_UNCHECKED));
			break;

		case 4: // Window submenu
			{
				// Update Next and Previous items
				int ctabs = m_pzt->GetItemCount();
				int itab = m_pzt->GetCurSel();
				EnableMenuItem(hMenu, ID_WINDOW_NEXT, MF_BYCOMMAND | (itab < ctabs - 1 ? MF_ENABLED : MF_GRAYED));
				EnableMenuItem(hMenu, ID_WINDOW_PREV, MF_BYCOMMAND | (itab ? MF_ENABLED : MF_GRAYED));

				// Erase old items on the menu if they exist
				int cItems = GetMenuItemCount(hMenu);
				if (cItems > 4)
				{
					for (int nCommand = ID_GOTOFILE_START + cItems - 5; nCommand >= ID_GOTOFILE_START; nCommand--)
						DeleteMenu(hMenu, nCommand, MF_BYCOMMAND);
				}

				// Add each open file to the menu
				CZDoc * pzd = m_pzdFirst;
				int cFile = 0;
				char szFilename[MAX_PATH];
				while (pzd)
				{
					szFilename[0] = 0;
					if (cFile < 9)
					{
						szFilename[0] = '&';
						_itoa_s(cFile + 1, szFilename + 1, MAX_PATH - 1, 10);
						szFilename[2] = ' ';
						szFilename[3] = 0;
					}
					if (cFile == 9)
						strcpy_s(szFilename, "1&0 ");
					strcat_s(szFilename, pzd->GetFilename());
					if (pzd->GetModify())
						lstrcat(szFilename, " *");
					::InsertMenu(hMenu, ID_WINDOW_MANAGER, MF_BYCOMMAND | MF_STRING,
						ID_GOTOFILE_START + cFile++, szFilename);
					pzd = pzd->Next();
				}
			}
			break;
		}
	}
	else if (hMenu == m_pzmContext->GetSubMenu(nIndex))
	{
		switch (nIndex)
		{
		default:
			break;

		case kcmEdit:
			{
				AssertPtr(m_pzdCurrent);

				// Enable/Disable cut and copy based on whether there is a selection or not
				AssertPtr(m_pzdCurrent);
				int nEnable = MF_GRAYED;
				if (m_pzdCurrent->GetSelection(NULL, NULL) != kstNone)
					nEnable = MF_ENABLED;
				::EnableMenuItem(hMenu, ID_EDIT_CUT, nEnable);
				::EnableMenuItem(hMenu, ID_EDIT_COPY, nEnable);

				::EnableMenuItem(hMenu, ID_EDIT_PASTE,
					m_pzdCurrent->CanPaste() ? MF_ENABLED : MF_GRAYED);

				AddOpenDocumentToMenu(hMenu);
				//AddSuggestionsToMenu(hMenu);

				::CheckMenuItem(hMenu, ID_EDIT_BINARY,
					m_pzdCurrent->GetFrame()->IsBinaryMode() ? MF_CHECKED : MF_UNCHECKED);
			}
			break;
		}
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
long CZEditFrame::OnNotify(int nID, NMHDR * pnmh)
{
	switch (pnmh->code)
	{
	case TCN_SELCHANGE:
		SelectFile(m_pzt->GetCurSel());
		break;

	case TBN_HOTITEMCHANGE:
		{
			char szMessage[MAX_PATH] = {0};
			LPNMTBHOTITEM lphi = (LPNMTBHOTITEM) pnmh;

			if (SendMessage(m_hwndTool, TB_ISBUTTONENABLED, lphi->idNew, 0))
				LoadString(g_fg.m_hinst, lphi->idNew, szMessage, MAX_PATH);
			if (*szMessage == 0)
				lstrcpy(szMessage, "Ready");
			SetStatusText(ksboMessage, szMessage);
			break;
		}

	case TBN_DROPDOWN:
		{
			HMENU hMenu = CreatePopupMenu();
			int i = -1;
			while (++i < kcMaxFiles && *(m_szFileList[i]) != '\0')
				AppendMenu(hMenu, MF_STRING, ID_FILE_FILE1 + i, m_szFileList[i]);
			RECT rect;
			SendMessage(m_hwndTool, TB_GETRECT, ((NMTOOLBAR*)pnmh)->iItem, (long) &rect);
			POINT point = {rect.left, rect.bottom};
			ClientToScreen(m_hwndTool, &point);
			TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, 0, m_hwnd, NULL);
			DestroyMenu(hMenu);
			break;
		}

	case TBN_QUERYINSERT:
	case TBN_QUERYDELETE:
		return FALSE;
		//return TRUE; // Allow any button to be inserted or deleted.

	case TBN_GETBUTTONINFO:
		{
			return FALSE;
			/*LPNMTOOLBAR pnmt = (LPNMTOOLBAR)pnmh;
			return TRUE;*/
		}

	case RBN_HEIGHTCHANGE:
		{
			RECT rect;
			GetWindowRect(m_hwnd, &rect);
			return OnSize(SIZE_RESTORED, rect.right - rect.left, rect.bottom - rect.top);
		}
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
long CZEditFrame::OnSize(UINT nType, int cx, int cy)
{
	if (!cx || !cy)
		return 0;

	RECT rc;
	::GetWindowRect(m_hwndRebar, &rc);
	int cyRebar = rc.bottom - rc.top;
	::MoveWindow(m_hwndRebar, 0, 0, cx, cyRebar, TRUE);
	::MoveWindow(m_hwndStatus, 0, cy - 20, cx, 20, TRUE);

	::MoveWindow(m_pzt->GetHwnd(), 0, cyRebar, cx, cy - 20 - cyRebar, TRUE);
	if (m_pzdCurrent)
	{
		m_pzt->GetClientRect(&rc);
		::MoveWindow(m_pzdCurrent->GetFrame()->GetHwnd(), rc.left, rc.top,
			rc.right - rc.left, rc.bottom - rc.top, TRUE);
	}
    return 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
long CZEditFrame::OnFileProperties(CZDoc * pzd)
{
	PROPSHEETPAGE psp[2] = {0};
	int cPages = 0;

	FilePropInfo fpi;
	fpi.pzef = this;
	fpi.pzd = pzd;

	if (lstrcmp(pzd->GetFilename(), "Untitled") != 0)
	{
		psp[0].dwSize = sizeof(PROPSHEETPAGE);
		psp[0].hInstance = g_fg.m_hinst;
		psp[0].lParam = (LPARAM)&fpi;
		psp[0].pszTemplate = MAKEINTRESOURCE(IDD_PROPERTIES);
		psp[0].pfnDlgProc = PropertiesPropPageProc;
		cPages++;
	}

	psp[cPages].dwSize = sizeof(PROPSHEETPAGE);
	psp[cPages].hInstance = g_fg.m_hinst;
	psp[cPages].lParam = (LPARAM)&fpi;
	psp[cPages].pszTemplate = MAKEINTRESOURCE(IDD_STATISTICS);
	psp[cPages].pfnDlgProc = StatisticsPropPageProc;

	PROPSHEETHEADER psh = { sizeof(PROPSHEETHEADER) };
	psh.dwFlags = PSH_PROPSHEETPAGE;
	psh.hwndParent = m_hwnd;
	psh.hInstance = g_fg.m_hinst;
	psh.nStartPage = 0;
	psh.nPages = cPages + 1;
	psh.ppsp = psp;

	char szCaption[MAX_PATH];
	char * pSlash = strrchr(pzd->GetFilename(), '\\');
	sprintf_s(szCaption, "%s Properties", pSlash ? pSlash + 1 : pzd->GetFilename());
	psh.pszCaption = szCaption;

	::PropertySheet(&psh);
	return 0;
}