#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"
#include <tchar.h>
#include <Shlobj.h>
#include <process.h>
#include <Shlwapi.h>

char * g_pszColumns[2][4] =
{
	{"Name", "In Folder", "Line", "Text"},
	{"    Name", "    In Folder", "    Line", "    Text"}
};

extern char krgcbFromUTF8[256];

#ifdef WS_EX_LAYERED
#ifndef LWA_ALPHA
#define LWA_ALPHA               0x00000002
#endif // !LWA_ALPHA
#endif // WS_EX_LAYERED


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool AddStringToCombo(HWND hwndDlg, int nID, char * pszText)
{
	AssertPtrN(pszText);
	if (!pszText)
		return false;

	HWND hwndCombo = ::GetDlgItem(hwndDlg, nID);
	int cch = ::SendMessage(hwndCombo, CB_GETLBTEXTLEN, 0, 0);
	if (cch > 0)
	{
		char * pszTopLine = new char[cch + 1];
		if (pszTopLine)
		{
			::SendMessage(hwndCombo, CB_GETLBTEXT, 0, (LPARAM)pszTopLine);
			if (lstrcmp(pszText, pszTopLine) != 0)
			{
				// The new find is different from the top line of the combo box, so add it.
				// Before adding the string, delete any identical existing string
				// in the combo box.
				int iItem = SendMessage(hwndCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)pszText);
				if (iItem != CB_ERR)
					::SendMessage(hwndCombo, CB_DELETESTRING, iItem, 0);

				::SendMessage(hwndCombo, CB_INSERTSTRING, 0, (LPARAM)pszText);
			}
			delete pszTopLine;
		}
	}
	else
	{
		::SendMessage(hwndCombo, CB_INSERTSTRING, 0, (LPARAM)pszText);
	}
	return true;
}


/***********************************************************************************************
	CFindDlg methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CFindDlg::CFindDlg(CZEditFrame * pzef)
{
	AssertPtr(pzef);
	m_pzef = pzef;
	m_fReplace = true;
	m_hwndDlg = m_hwndTextBox = NULL;
	m_fBusy = m_fVisible = false;
	m_fFirstFind = true;
	m_ichInitial = -1;
	m_wpOldComboEditProc = NULL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CFindDlg::~CFindDlg()
{
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindDlg::ConvertString(int nID, char * pszText, wchar ** ppwszConverted, UINT * pcch)
{
	// Note: This function allocates memory for pszConverted. The calling
	//		 function is responsible for freeing it.
	AssertPtr(ppwszConverted);
	AssertPtr(pcch);
	*ppwszConverted = NULL;
	*pcch = 0;
	bool fReplaceText = nID == IDC_COMBO_REPLACE;
	if (pszText)
	{
		*ppwszConverted = new wchar[lstrlen(pszText) + 1];
		if (!ppwszConverted)
			return false;
		int nOffset = 0;
		char * psz = pszText;
		wchar * pwsz = *ppwszConverted;
		bool fDecimal = true;
		while (*psz != 0)
		{
			if (*psz == '^')
			{
				switch (tolower(*(++psz)))
				{
				case 'p': // paragraph
					if (fReplaceText)
					{
						*pwsz++ = 13;
						*pwsz = 10;
					}
					else
					{
						*pwsz++ = '^';
						*pwsz = 'P';
					}
					break;

				case 't': // tab
					*pwsz = '\t';
					break;

				case '^': // caret
					if (!fReplaceText)
						*pwsz++ = '^';
					*pwsz = '^';
					break;

				case 'x': // hex number
					fDecimal = false;
					// fall through

				case 'd': // decimal number
					{
						char * pszAfter;
						ULONG lT = strtol(psz + 1, &pszAfter, fDecimal ? 10 : 16);
						if (!pszAfter || *pszAfter != ';' || pszAfter == psz + 1)
						{
							char szMessage[MAX_PATH];
							HWND hwndText = GetWindow(GetDlgItem(m_hwndDlg, nID), GW_CHILD);
							EnableWindow(GetParent(m_hwndDlg), FALSE);
							if (pszAfter == psz + 1 && *pszAfter == ';')
							{
								wsprintf(szMessage,
									"Please enter the %s number at the cursor location.",
									fDecimal ? "decimal" : "hexadecimal");
							}
							else
							{
								wsprintf(szMessage,
									"A semi-colon must follow a %s number before other text.",
									fDecimal ? "decimal" : "hexadecimal");
							}
							::MessageBox(m_hwndDlg, szMessage, g_pszAppName,
								MB_ICONINFORMATION);
							::EnableWindow(::GetParent(m_hwndDlg), TRUE);
							::SetFocus(hwndText);
							if (pszAfter)
							{
								::SendMessage(hwndText, EM_SETSEL, pszAfter - pszText,
									pszAfter - pszText);
							}
							else
							{
								::SendMessage(hwndText, EM_SETSEL, lstrlen(pszText),
									lstrlen(pszText));
							}
							return false;
						}
						else
						{
							if (lT > 0xFFFF)
							{
								// Surrogate number
								lT -= 0x10000;
								*pwsz++ = (wchar)((lT >> 10) + 0xd800);
								*pwsz = (wchar)((lT & 0x3ff) + 0xdc00);
							}
							else
							{
								*pwsz = (wchar)(lT);
							}
							psz = pszAfter;
						}
						fDecimal = true;
						break;
					}

				default:
					{
						HWND hwndText = ::GetWindow(::GetDlgItem(m_hwndDlg, nID), GW_CHILD);
						::EnableWindow(GetParent(m_hwndDlg), FALSE);
						::MessageBox(m_hwndDlg,
							"A caret is a control character. "
							"If you want to search for a single caret, "
							"you must type two carets in a row '^^'.",
							g_pszAppName, MB_ICONINFORMATION);
						::EnableWindow(::GetParent(m_hwndDlg), TRUE);
						::SetFocus(hwndText);
						::SendMessage(hwndText, EM_SETSEL, psz - pszText - 1, psz - pszText);
						return false;
					}
				}
			}
			else
			{
				*pwsz = (wchar)*psz & 0xFF;
			}
			psz++;
			pwsz++;
		}
		*pwsz = 0;
		Assert(pwsz - *ppwszConverted >= 0);
		*pcch = pwsz - *ppwszConverted;
	}

	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindDlg::EnterSpecialText(int nID)
{
	switch (nID)
	{
	case ID_SEARCH_PARAGRAPH:
		::SendMessage(m_hwndTextBox, EM_REPLACESEL, TRUE, (LPARAM)"^p");
		break;

	case ID_SEARCH_TABCHARACTER:
		::SendMessage(m_hwndTextBox, EM_REPLACESEL, TRUE, (LPARAM)"^t");
		break;

	case ID_SEARCH_CARETCHARACTER:
		::SendMessage(m_hwndTextBox, EM_REPLACESEL, TRUE, (LPARAM)"^^");
		break;

	case ID_SEARCH_ASCIICODE:
	case ID_SEARCH_UNICODECODE:
		{
			int ichStart;
			::SendMessage(m_hwndTextBox, EM_REPLACESEL, TRUE,
				(LPARAM)(nID == ID_SEARCH_ASCIICODE ? "^d;" : "^x;"));
			::SendMessage(m_hwndTextBox, EM_GETSEL, (WPARAM)&ichStart, 0);
			::SendMessage(m_hwndTextBox, EM_SETSEL, ichStart - 1, ichStart - 1);
			break;
		}
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindDlg::ShowDialog(bool fReplace)
{
	if (!m_hwndDlg)
	{
		// This is the first time the find dialog has been displayed
		if (m_pzef)
		{
			HINSTANCE hInst = (HINSTANCE)::GetWindowLong(m_pzef->GetHwnd(), GWL_HINSTANCE);
			m_fReplace = true;
			m_hwndDlg = ::CreateDialog(hInst, MAKEINTRESOURCE(IDD_FIND), m_pzef->GetHwnd(),
				FindDlgProc);
			if (m_hwndDlg)
			{
				HWND hwndDirection = ::GetDlgItem(m_hwndDlg, IDC_COMBO_DIRECTION);
				::SetWindowLong(m_hwndDlg, GWL_USERDATA, (long)this);
				// Set initial direction combo box values and set selection to "All"
				::SendMessage(hwndDirection, CB_ADDSTRING, 0, (long)"Down");
				::SendMessage(hwndDirection, CB_ADDSTRING, 0, (long)"Up");
				::SendMessage(hwndDirection, CB_ADDSTRING, 0, (long)"All");
				::SendMessage(hwndDirection, CB_SETCURSEL, kfdAll, 0);

				SubclassCombo(IDC_COMBO_FIND);
				SubclassCombo(IDC_COMBO_REPLACE);
			}
		}
		else
		{
			return false;
		}
	}
	::CheckDlgButton(m_hwndDlg, IDC_REPLACEFILES, BST_UNCHECKED);
	::SetFocus(::GetDlgItem(m_hwndDlg, IDC_COMBO_FIND));

	if (fReplace && !m_fReplace)
	{
		m_fReplace = true;
		::SetWindowText(m_hwndDlg, "Replace");
		::SetWindowText(::GetDlgItem(m_hwndDlg, IDC_REPLACE), "&Replace");

		RECT rcCancel = {219, 66};
		RECT rcMatch = {113, 44};
		RECT rcWholeWord = {121, 56};
		RECT rcMatchCase = {121, 68};
		RECT rcDirection = {56, 48};
		RECT rcSearch = {6, 51};
		::MapDialogRect(m_hwndDlg, &rcCancel);
		::MapDialogRect(m_hwndDlg, &rcMatch);
		::MapDialogRect(m_hwndDlg, &rcWholeWord);
		::MapDialogRect(m_hwndDlg, &rcMatchCase);
		::MapDialogRect(m_hwndDlg, &rcDirection);
		::MapDialogRect(m_hwndDlg, &rcSearch);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDCANCEL), NULL,
			rcCancel.left, rcCancel.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDC_STATIC_MATCH), NULL,
			rcMatch.left, rcMatch.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDC_WHOLEWORD), NULL,
			rcWholeWord.left, rcWholeWord.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDC_MATCHCASE), NULL,
			rcMatchCase.left, rcMatchCase.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDC_COMBO_DIRECTION), NULL,
			rcDirection.left, rcDirection.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDC_STATIC_SEARCH), NULL,
			rcSearch.left, rcSearch.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		::ShowWindow(::GetDlgItem(m_hwndDlg, IDC_COMBO_REPLACE), SW_SHOW);
		::ShowWindow(::GetDlgItem(m_hwndDlg, IDC_STATIC_REPLACE), SW_SHOW);
		::ShowWindow(::GetDlgItem(m_hwndDlg, IDC_REPLACEALL), SW_SHOW);
		::ShowWindow(::GetDlgItem(m_hwndDlg, IDC_REPLACEFILES), SW_SHOW);

		RECT rcDlg = {0, 0, 276, 87};
		::MapDialogRect(m_hwndDlg, &rcDlg);
		::AdjustWindowRectEx(&rcDlg, ::GetWindowLong(m_hwndDlg, GWL_STYLE), FALSE,
			::GetWindowLong(m_hwndDlg, GWL_EXSTYLE));
		::SetWindowPos(m_hwndDlg, NULL, 0, 0, rcDlg.right - rcDlg.left,
			rcDlg.bottom - rcDlg.top, SWP_NOMOVE | SWP_NOZORDER);
		::SetFocus(::GetDlgItem(m_hwndDlg, IDC_COMBO_REPLACE));
	}
	else if (!fReplace && m_fReplace)
	{
		m_fReplace = false;
		::SetWindowText(m_hwndDlg, "Find");
		::SetWindowText(::GetDlgItem(m_hwndDlg, IDC_REPLACE), "&Replace...");

		RECT rcCancel = {219, 48};
		RECT rcMatch = {113, 26};
		RECT rcWholeWord = {121, 38};
		RECT rcMatchCase = {121, 50};
		RECT rcDirection = {56, 30};
		RECT rcSearch = {6, 33};
		::MapDialogRect(m_hwndDlg, &rcCancel);
		::MapDialogRect(m_hwndDlg, &rcMatch);
		::MapDialogRect(m_hwndDlg, &rcWholeWord);
		::MapDialogRect(m_hwndDlg, &rcMatchCase);
		::MapDialogRect(m_hwndDlg, &rcDirection);
		::MapDialogRect(m_hwndDlg, &rcSearch);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDCANCEL), NULL,
			rcCancel.left, rcCancel.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDC_STATIC_MATCH), NULL,
			rcMatch.left, rcMatch.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDC_WHOLEWORD), NULL,
			rcWholeWord.left, rcWholeWord.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDC_MATCHCASE), NULL,
			rcMatchCase.left, rcMatchCase.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDC_COMBO_DIRECTION), NULL,
			rcDirection.left, rcDirection.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(m_hwndDlg, IDC_STATIC_SEARCH), NULL,
			rcSearch.left, rcSearch.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		::ShowWindow(::GetDlgItem(m_hwndDlg, IDC_COMBO_REPLACE), SW_HIDE);
		::ShowWindow(::GetDlgItem(m_hwndDlg, IDC_STATIC_REPLACE), SW_HIDE);
		::ShowWindow(::GetDlgItem(m_hwndDlg, IDC_REPLACEALL), SW_HIDE);
		::ShowWindow(::GetDlgItem(m_hwndDlg, IDC_REPLACEFILES), SW_HIDE);

		RECT rcDlg = {0, 0, 276, 69};
		::MapDialogRect(m_hwndDlg, &rcDlg);
		::AdjustWindowRectEx(&rcDlg, GetWindowLong(m_hwndDlg, GWL_STYLE), FALSE,
			::GetWindowLong(m_hwndDlg, GWL_EXSTYLE));
		::SetWindowPos(m_hwndDlg, NULL, 0, 0, rcDlg.right - rcDlg.left,
			rcDlg.bottom - rcDlg.top, SWP_NOMOVE | SWP_NOZORDER);
	}

	// If there is a selection on one line, put the whole selection into the Find What box.
	UINT iStart;
	UINT iStop;
	void * pv;
	CZDoc * pzd = m_pzef->GetCurrentDoc();
	pzd->GetSelection(&iStart, &iStop);
	if (iStart != iStop && iStop - iStart < 100)
	{
		UINT cch = pzd->GetText(iStart, iStop - iStart, &pv);
		if (pv)
		{
			if (pzd->GetFileType() != kftAnsi)
			{
				int cchBuffer = cch << 3;
				char * prgchBuffer = new char[cchBuffer];
				if (prgchBuffer)
				{
					char * prgchDst = prgchBuffer;
					char * prgchLim = prgchDst + cchBuffer;
					wchar * prgwchSrc = (wchar *)pv;
					// Include NULL character at the end.
					wchar * prgwchStop = prgwchSrc + cch + 1;
					ULONG ch;
					while (prgwchSrc < prgwchStop)
					{
						if ((ch = *prgwchSrc++) > 0xFF)
						{
							*prgchDst++ = '^';
							*prgchDst++ = 'x';
							prgchDst += sprintf_s(prgchDst, prgchLim - prgchDst, "%x", ch);
							*prgchDst++ = ';';
						}
						else
						{
							*prgchDst++ = (char)((BYTE)ch);
						}
					}
					delete pv;
					pv = prgchBuffer;
				}
			}
			char * psz = strrchr((char *)pv, '\n');
			if (!psz)
			{
				::SetDlgItemText(m_hwndDlg, IDC_COMBO_FIND, (char *)pv);
				::SendMessage(::GetWindow(::GetDlgItem(m_hwndDlg, IDC_COMBO_FIND), GW_CHILD),
					EM_SETSEL, 0, -1);
			}
			delete pv;
		}
	}

	::InvalidateRect(m_hwndDlg, NULL, TRUE);
	::UpdateWindow(m_hwndDlg);
	m_pzef->Animate(m_hwndDlg, ID_EDIT_FIND, true, true);
	m_fVisible = true;
	::ShowWindow(m_hwndDlg, SW_SHOW);
#ifdef WS_EX_LAYERED
	if (g_fg.m_pfnSetLayeredWindowAttributes)
	{
		::SetWindowLong(m_hwndDlg, GWL_EXSTYLE,
			::GetWindowLong(m_hwndDlg, GWL_EXSTYLE) | WS_EX_LAYERED);
		(*g_fg.m_pfnSetLayeredWindowAttributes)(m_hwndDlg, 0, (BYTE)255, LWA_ALPHA);
	}
#endif // WS_EX_LAYERED
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFindDlg::SubclassCombo(int nComboID)
{
	HWND hwndCombo = ::GetDlgItem(m_hwndDlg, nComboID);
	if (!hwndCombo)
		return;

	POINT pt = { 5, 5 };
	HWND hwndEdit = ::ChildWindowFromPoint(hwndCombo, pt);
	if (!hwndEdit)
		return;

	if (m_wpOldComboEditProc)
		::SetWindowLong(hwndEdit, GWL_WNDPROC, (long)NewComboEditWndProc);
	else
		m_wpOldComboEditProc = (WNDPROC)::SetWindowLong(hwndEdit, GWL_WNDPROC, (long)NewComboEditWndProc);
	::SetWindowLong(hwndEdit, GWL_USERDATA, (LPARAM)this);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
LRESULT CALLBACK CFindDlg::NewComboEditWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	CFindDlg * pdlg = (CFindDlg *)::GetWindowLong(hwnd, GWL_USERDATA);
	AssertPtr(pdlg);

	if (iMsg == WM_PASTE)
	{
		if (::OpenClipboard(hwnd))
		{
			bool fHandled = false;
			//if (::EnumClipboardFormats(0) == CF_UNICODETEXT)
			if (::IsClipboardFormatAvailable(CF_UNICODETEXT))
			{
				HGLOBAL hData = ::GetClipboardData(CF_UNICODETEXT);
				if (hData != NULL)
				{
					wchar * pszSrc = (wchar *)::GlobalLock(hData);
					if (pszSrc != NULL)
					{
						int cchDst = wcslen(pszSrc) * 10 + 1;
						char * pszDst = new char[cchDst];
						char * pszLim = pszDst + cchDst;
						wchar * prgchSrc = pszSrc;
						char * prgchDst = pszDst;
						wchar ch;
						while (*prgchSrc != 0)
						{
							if ((ch = *prgchSrc++) > 0xFF)
							{
								*prgchDst++ = '^';
								*prgchDst++ = 'x';
								prgchDst += sprintf_s(prgchDst, pszLim - prgchDst, "%x", ch);
								*prgchDst++ = ';';
							}
							else
							{
								*prgchDst++ = (char)ch;
							}
						}
						*prgchDst = 0;
						::SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM)pszDst);
						delete pszDst;
						fHandled = true;
						GlobalUnlock(hData);
					}
				}
			}
			::CloseClipboard();
			if (fHandled)
				return 0;
		}
	}

	Assert(pdlg->m_wpOldComboEditProc != NULL);
	return ::CallWindowProc(pdlg->m_wpOldComboEditProc, hwnd, iMsg, wParam, lParam);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindDlg::OnEnterIdle(void)
{
	if (!m_fVisible)
		return true;
	int cch = ::GetWindowTextLength(::GetDlgItem(m_hwndDlg, IDC_COMBO_FIND));
	bool fReplaceFiles = ::IsDlgButtonChecked(m_hwndDlg, IDC_REPLACEFILES) != 0;
	if (fReplaceFiles)
	{
		::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_FINDNEXT), FALSE);
		::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_REPLACE), FALSE);
		::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_COMBO_DIRECTION), FALSE);
		::SendMessage(::GetDlgItem(m_hwndDlg, IDC_COMBO_DIRECTION), CB_SETCURSEL, kfdAll, 0);
	}
	else
	{
		::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_COMBO_DIRECTION), TRUE);
		::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_FINDNEXT), cch > 0 ? TRUE : FALSE);
		::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_REPLACE),
			(!m_fReplace || cch > 0) ? TRUE : FALSE);
		char * pszText = new char[cch + 1];
		::GetWindowText(::GetDlgItem(m_hwndDlg, IDC_COMBO_FIND), pszText, cch + 1);
		delete pszText;
	}
	::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_REPLACEALL), cch > 0 ? TRUE : FALSE);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindDlg::OnLButtonDown(UINT nFlags, POINT point)
{
	RECT rcFind, rcReplace;
	::GetWindowRect(::GetDlgItem(m_hwndDlg, IDC_STATIC_FIND_MENU), &rcFind);
	::GetWindowRect(::GetDlgItem(m_hwndDlg, IDC_STATIC_REPLACE_MENU), &rcReplace);
	::MapWindowPoints(NULL, m_hwndDlg, (POINT *)&rcFind, 2);
	::MapWindowPoints(NULL, m_hwndDlg, (POINT *)&rcReplace, 2);
	RECT rc;
	bool fShowMenu = false;

	if (::PtInRect(&rcFind, point))
	{
		fShowMenu = true;
		point.x = rcFind.right;
		point.y = rcFind.top;
		rc = rcFind;
		m_hwndTextBox = ::GetWindow(::GetDlgItem(m_hwndDlg, IDC_COMBO_FIND), GW_CHILD);
	}
	if (m_fReplace && ::PtInRect(&rcReplace, point))
	{
		fShowMenu = true;
		point.x = rcReplace.right;
		point.y = rcReplace.top;
		rc = rcReplace;
		m_hwndTextBox = ::GetWindow(::GetDlgItem(m_hwndDlg, IDC_COMBO_REPLACE), GW_CHILD);
	}
	if (fShowMenu)
	{
		AssertPtr(m_pzef);
		HDC hdc = ::GetDC(m_hwndDlg);
		g_fg.Draw3dRect(hdc, &rc, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DHILIGHT));
		::ClientToScreen(m_hwndDlg, &point);
		::TrackPopupMenu(m_pzef->GetContextMenu()->GetSubMenu(kcmFind),
			TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, 0, m_hwndDlg, NULL);
		g_fg.Draw3dRect(hdc, &rc, ::GetSysColor(COLOR_3DFACE), ::GetSysColor(COLOR_3DFACE));
		::ReleaseDC(m_hwndDlg, hdc);
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindDlg::OnMouseMove(UINT nFlags, POINT point)
{
	HDC hdc = ::GetDC(m_hwndDlg);
	RECT rcFind, rcReplace;
	::GetWindowRect(::GetDlgItem(m_hwndDlg, IDC_STATIC_FIND_MENU), &rcFind);
	::GetWindowRect(::GetDlgItem(m_hwndDlg, IDC_STATIC_REPLACE_MENU), &rcReplace);
	::MapWindowPoints(NULL, m_hwndDlg, (POINT *)&rcFind, 2);
	::MapWindowPoints(NULL, m_hwndDlg, (POINT *)&rcReplace, 2);

	if (::PtInRect(&rcFind, point))
	{
		g_fg.Draw3dRect(hdc, &rcFind, ::GetSysColor(COLOR_3DHILIGHT),
			::GetSysColor(COLOR_3DSHADOW));
	}
	else
	{
		g_fg.Draw3dRect(hdc, &rcFind, ::GetSysColor(COLOR_3DFACE),
			::GetSysColor(COLOR_3DFACE));
	}
	if (m_fReplace)
	{
		if (::PtInRect(&rcReplace, point))
		{
			g_fg.Draw3dRect(hdc, &rcReplace, ::GetSysColor(COLOR_3DHILIGHT),
				::GetSysColor(COLOR_3DSHADOW));
		}
		else
		{
			g_fg.Draw3dRect(hdc, &rcReplace, ::GetSysColor(COLOR_3DFACE),
				::GetSysColor(COLOR_3DFACE));
		}
	}
	::ReleaseDC(m_hwndDlg, hdc);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindDlg::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hwndDlg, &ps);
	RECT rcFind, rcReplace, rcFindSmall, rcReplaceSmall;
	::GetWindowRect(::GetDlgItem(m_hwndDlg, IDC_STATIC_FIND_MENU), &rcFind);
	::GetWindowRect(::GetDlgItem(m_hwndDlg, IDC_STATIC_REPLACE_MENU), &rcReplace);
	::MapWindowPoints(NULL, m_hwndDlg, (POINT *)&rcFind, 2);
	::MapWindowPoints(NULL, m_hwndDlg, (POINT *)&rcReplace, 2);
	rcFindSmall = rcFind;
	rcReplaceSmall = rcReplace;
	::InflateRect(&rcFindSmall, -4, -4);
	::InflateRect(&rcReplaceSmall, -4, -4);

	::IntersectClipRect(hdc, rcFindSmall.left, rcFindSmall.top, rcFindSmall.right,
		rcFindSmall.bottom);
	::DrawFrameControl(hdc, &rcFind, DFC_SCROLL, DFCS_SCROLLRIGHT);
	if (m_fReplace)
	{
		::SelectClipRgn(hdc, NULL);
		::IntersectClipRect(hdc, rcReplaceSmall.left, rcReplaceSmall.top,
			rcReplaceSmall.right, rcReplaceSmall.bottom);
		::DrawFrameControl(hdc, &rcReplace, DFC_SCROLL, DFCS_SCROLLRIGHT);
	}
	::EndPaint(m_hwndDlg, &ps);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindDlg::UpdateFindInfo()
{
	m_fi.fFindIsReplace = false;
	m_fi.nFlags = 0;
	if (::IsDlgButtonChecked(m_hwndDlg, IDC_WHOLEWORD))
		m_fi.nFlags |= kffWholeWord;
	if (::IsDlgButtonChecked(m_hwndDlg, IDC_MATCHCASE))
		m_fi.nFlags |= kffMatchCase;
	FindDirection fd = (FindDirection)::SendMessage(::GetDlgItem(m_hwndDlg,
		IDC_COMBO_DIRECTION), CB_GETCURSEL, 0, 0);
	if (fd == kfdUp)
		m_fi.nFlags |= kffUp;
	else if (fd == kfdDown)
		m_fi.nFlags |= kffDown;

	char * pszFindWhat = NULL;
	char * pszReplaceWith = NULL;
	HWND hwndCombo = ::GetDlgItem(m_hwndDlg, IDC_COMBO_FIND);
	int cch = ::GetWindowTextLength(hwndCombo);
	if (!cch)
		return false;
	pszFindWhat = new char[cch + 1];
	if (!pszFindWhat)
		return false;
	::GetWindowText(hwndCombo, pszFindWhat, cch + 1);
	hwndCombo = ::GetDlgItem(m_hwndDlg, IDC_COMBO_REPLACE);
	cch = ::GetWindowTextLength(hwndCombo);
	if (cch > 0)
	{
		pszReplaceWith = new char[cch + 1];
		if (pszReplaceWith)
			::GetWindowText(hwndCombo, pszReplaceWith, cch + 1);
		m_fi.fFindIsReplace = strcmp(pszFindWhat, pszReplaceWith) == 0;
	}

	AddStringToCombo(m_hwndDlg, IDC_COMBO_FIND, pszFindWhat);
	if (m_fReplace)
		AddStringToCombo(m_hwndDlg, IDC_COMBO_REPLACE, pszReplaceWith);

	bool fSuccess =
		ConvertString(IDC_COMBO_FIND, pszFindWhat, &m_fi.pwszFindWhat,
			&m_fi.cchFindWhat) &&
		ConvertString(IDC_COMBO_REPLACE, pszReplaceWith, &m_fi.pwszReplaceWith,
			&m_fi.cchReplaceWith);
	if (pszFindWhat)
		delete pszFindWhat;
	if (pszReplaceWith)
		delete pszReplaceWith;

	// Uppercase the search string if the search is not case sensitive
	if (fSuccess && !(m_fi.nFlags & kffMatchCase))
		_wcsupr_s(m_fi.pwszFindWhat, m_fi.cchFindWhat + 1);

	return fSuccess;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindDlg::OnFind(bool fNext, bool fReplace)
{
	static bool fLastNext = true;
	UINT & ichStart = m_fi.ichStart;
	UINT & ichStop = m_fi.ichStop;
	CZDoc * pzd = m_pzef->GetCurrentDoc();
	UINT ichSelStart, ichSelStop;
	pzd->GetSelection(&ichSelStart, &ichSelStop);
	FindDirection fd = (FindDirection)::SendMessage(
		::GetDlgItem(m_hwndDlg, IDC_COMBO_DIRECTION), CB_GETCURSEL, 0, 0);
	if (!UpdateFindInfo())
		return false;
	m_fBusy = true;

	if (fNext != fLastNext)
	{
		fLastNext = fNext;
		m_fFirstFind = true;
	}
	if (!fNext)
	{
		// Find the previous item instead of the next item
		if (m_fi.nFlags & kffUp)
		{
			m_fi.nFlags |= kffDown;
			m_fi.nFlags &= ~kffUp;
			fd = kfdDown;
		}
		else
		{
			m_fi.nFlags &= ~kffDown;
			m_fi.nFlags |= kffUp;
			fd = kfdUp;
		}
	}

	if (m_fFirstFind)
	{
		m_ichInitial = fd == kfdUp ? ichSelStop : ichSelStart;
		if (ichSelStart != ichSelStop)
		{
			m_fs = kfsSelection;
			if (fd == kfdUp)
			{
				ichStart = ichSelStop;
				ichStop = ichSelStart;
			}
			else
			{
				ichStart = ichSelStart;
				ichStop = ichSelStop;
			}
		}
		else
		{
			m_fs = kfsFirstHalf;
			ichStart = ichSelStart;
			if (fd != kfdAll && (ichStart == 0 || ichStart == pzd->GetCharCount()))
				m_fs = kfsSecondHalf;
			if (fd == kfdUp)
				ichStop = 0;
			else
				ichStop = -1;
		}
	}

	CWaitCursor wc;
	char sMessage[MAX_PATH];
	bool fFoundMatch = pzd->Find(&m_fi);
	if (fFoundMatch)
	{
		// The current selection matches the find string.
		if (m_fi.ichMatch == ichSelStart && m_fi.cchMatch == ichSelStop - ichSelStart)
		{
			if (fReplace)
			{
				// The user hit the Replace button and the selected text matched,
				// so replace the selected text before performing the new search.
				pzd->InsertText(m_fi.ichMatch, m_fi.pwszReplaceWith, m_fi.cchReplaceWith,
					m_fi.cchMatch, false, kurtReplace);

				if (m_fi.ichMatch < m_ichInitial)
				{
					m_ichInitial += m_fi.cchReplaceWith - m_fi.cchMatch;
					if (m_ichInitial > pzd->GetCharCount())
						m_ichInitial = 0;
				}
			}
			// Update the search criteria to find the next one.
			if (fd == kfdUp)
			{
				ichStart = ichSelStop > m_fi.cchFindWhat ? ichSelStop - m_fi.cchFindWhat : 0;
				// If the selection when the search started is the same as the search
				// string, we change the search type to FirstHalf.
				if ((ichStart == ichStop && m_fFirstFind) ||
					(ichStart < ichStop && m_fs == kfsSelection))
				{
					ichStop = 0;
					m_fs = kfsFirstHalf;
				}
			}
			else
			{
				if (*m_fi.pwszFindWhat == '^' && towlower(m_fi.pwszFindWhat[1]) == 'p')
				{
					// If the Find What text starts with a paragraph, we need to skip past
					// the paragraph, which means we might need to add 1 to ichSelStart if
					// there are two characters at the end of this paragraph.
					// TODO: Can't this use CharsAtEnd?
					void * pvStart;
					UINT iParaInPart;
					DocPart * pdp = pzd->GetPart(ichSelStart, false, &pvStart, &iParaInPart);
					if (pzd->IsAnsi(pdp, iParaInPart))
					{
						if (((char *)pvStart)[0] == 13 && ((char *)pvStart)[1] == 10)
							ichSelStart++;
					}
					else
					{
						if (((wchar *)pvStart)[0] == 13 && ((wchar *)pvStart)[1] == 10)
							ichSelStart++;
					}
				}
				ichStart = ichSelStart + 1;
				// If the selection when the search started is the same as the search
				// string, we change the search type to FirstHalf.
				if ((ichStart == ichStop - (ichSelStop - ichSelStart - 1) && m_fFirstFind) ||
					(ichStart > ichStop && m_fs == kfsSelection))
				{
					ichStop = -1;
					m_fs = kfsFirstHalf;
				}
			}
			fFoundMatch = pzd->Find(&m_fi);
		}
	}
	if (fFoundMatch && m_ichInitial == m_fi.ichMatch && m_fs == kfsSecondHalf && fd != kfdUp)
		fFoundMatch = false;
	m_fFirstFind = false;
	if (fFoundMatch)
	{
		pzd->SetSelection(m_fi.ichMatch, m_fi.ichMatch + m_fi.cchMatch, true, true);
	}
	else
	{
		if (m_fs == kfsFirstHalf)
		{
			if (fd == kfdDown && m_ichInitial == 0)
				m_fs = kfsSecondHalf;
			if (fd == kfdUp && m_ichInitial == pzd->GetCharCount())
				m_fs = kfsSecondHalf;
		}
		switch (m_fs)
		{
		case kfsSelection:
			if (fd != kfdAll)
			{
				wsprintf(sMessage, 
					"%s has finished searching the selection. The search item was not found. "
					"Do you want to search the remainder of the document?", g_pszAppName);
				int nResponse = ::MessageBox(NULL, sMessage, g_pszAppName,
					MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL);
				if (nResponse == IDNO)
					break;
			}
			m_fs = kfsFirstHalf;
			//ichStart = ichStop;
			if (fd == kfdUp)
				ichStop = 0;
			else
				ichStop = -1;
			if (pzd->Find(&m_fi))
			{
				pzd->SetSelection(m_fi.ichMatch, m_fi.ichMatch + m_fi.cchMatch, true, true);
				break;
			}
			// Fall-through.

		case kfsFirstHalf:
			if (fd != kfdAll)
			{
				wsprintf(sMessage, 
					"%s has reached the %s of the document. The search item was not found. "
					"Do you want to continue searching at the %s?", g_pszAppName,
					fd == kfdDown ? "end" : "beginning", fd == kfdDown ? "beginning" : "end");
				int nResponse = ::MessageBox(NULL, sMessage, g_pszAppName,
					MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL);
				if (nResponse == IDNO)
					break;
			}
			m_fs = kfsSecondHalf;
			if (fd == kfdUp)
			{
				ichStart = -1;
				ichStop = m_ichInitial;
			}
			else
			{
				ichStart = 0;
				ichStop = m_ichInitial + m_fi.cchFindWhat;
			}
			if (pzd->Find(&m_fi) && m_ichInitial != m_fi.ichMatch)
			{
				pzd->SetSelection(m_fi.ichMatch, m_fi.ichMatch + m_fi.cchMatch, true, true);
				break;
			}
			// Fall-through.

		case kfsSecondHalf:
			wsprintf(sMessage, 
				"%s has finished searching the document. The search item was not found.",
				g_pszAppName);
			::MessageBox(NULL, sMessage, g_pszAppName,
				MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
			m_fFirstFind = true;
			break;
		}
	}
	if (m_fi.pwszFindWhat)
		delete m_fi.pwszFindWhat;
	if (m_fi.pwszReplaceWith)
		delete m_fi.pwszReplaceWith;
	m_fBusy = false;
	bool fFirstFind = m_fFirstFind;
	m_pzef->UpdateTextPos();
	// This has to be after UpdateTextPos, because it will reset it.
	m_fFirstFind = fFirstFind;
	return fFoundMatch;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindDlg::OnReplace(void)
{
	if (!m_fReplace)
	{
		return ShowDialog(true);
	}
	else
	{
		CZDoc * pzd = m_pzef->GetCurrentDoc();
		UINT ichSelStart;
		UINT ichSelStop;
		pzd->GetSelection(&ichSelStart, &ichSelStop);
		if (!UpdateFindInfo())
			return false;
		/*void * pv;
		if (ichSelStart != ichSelStop &&
			pzd->GetText(ichSelStart, ichSelStop - ichSelStart, &pv) > 0)
		{
			bool fMatched;
			if (pzd->GetFileType() == kftAnsi)
			{
				fMatched = ((IsDlgButtonChecked(m_hwndDlg, IDC_MATCHCASE) ?
					strcmp((char *)pv, (char *)m_fi.pvFindWhat) :
					stricmp((char *)pv, (char *)m_fi.pvFindWhat)) == 0);
			}
			else
			{
				fMatched = ((IsDlgButtonChecked(m_hwndDlg, IDC_MATCHCASE) ?
					wcscmp((wchar *)pv, (wchar *)m_fi.pvFindWhat) :
					wcsicmp((wchar *)pv, (wchar *)m_fi.pvFindWhat)) == 0);
			}
			if (fMatched)
			{
				int cchReplace = 0;
				if (m_fi.pvReplaceWith)
				{
					if (pzd->GetFileType() == kftAnsi)
						cchReplace = lstrlen((char *)m_fi.pvReplaceWith);
					else
						cchReplace = lstrlenW((wchar *)m_fi.pvReplaceWith);
					pzd->InsertText(ichSelStart, m_fi.pvReplaceWith, cchReplace, ichSelStop - ichSelStart,
						!(m_fi.nFlags & kffUnicode), kurtReplace);
				}
				else
				{
					UndoRedo * pur = new UndoRedo(kurtReplace, ichSelStart, 0, ichSelStop - ichSelStart);
					pzd->GetText(ichSelStart, ichSelStop - ichSelStart, &pur->pzdo);
					pzd->PushUndoEntry(pur);
					pzd->DeleteText(ichSelStart, ichSelStop, false);
				}
				pzd->SetSelection(ichSelStart, ichSelStart + cchReplace, true, true);
				FindDirection fd = (FindDirection)SendMessage(GetDlgItem(m_hwndDlg, IDC_COMBO_DIRECTION), CB_GETCURSEL, 0, 0);
				if (fd == kfdUp)
					m_fi.ichStart = ichSelStart;
				else
					m_fi.ichStart = ichSelStart + 1;
				if (ichSelStart < m_iStopSearch)
				{
					m_iStopSearch += cchReplace;
					if (pzd->GetFileType() == kftAnsi)
						m_iStopSearch -= lstrlen((char *)m_fi.pvFindWhat);
					else
						m_iStopSearch -= lstrlenW((wchar *)m_fi.pvFindWhat);
					if (m_iStopSearch > pzd->GetCharCount())
						m_iStopSearch = 0;
				}
			}
			delete pv;
		}*/
		bool fFound = OnFind(true, true);
		/*if (fFound && m_fi.ichMatch == ichSelStart && m_fi.cchMatch == ichSelStop - ichSelStart)
			fFound = OnFind();*/
		return fFound;
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindDlg::OnReplaceAll(void)
{
	m_fi.nFlags |= kffReplaceAll;
	if (!UpdateFindInfo())
		return false;

	CWaitCursor wc;
	m_fBusy = true;
	bool fSuccess = true;
	UINT cReplaced = 0;
	char szMessage[MAX_PATH];
	HWND hwnd = m_pzef->GetHwnd();

	bool fReplaceFiles = ::IsDlgButtonChecked(m_hwndDlg, IDC_REPLACEFILES) != 0;
	if (fReplaceFiles)
	{
		CZDoc * pzd = m_pzef->GetFirstDoc();

		while (pzd)
		{
			m_fi.ichStart = 0;
			m_fi.ichStop = -1;
			cReplaced += pzd->DoReplaceAll(&m_fi);
			pzd = pzd->Next();
		}
		wsprintf(szMessage,
			"%s has completed its search of all open documents and has made %s replacement%s.",
			g_pszAppName, CZEditFrame::InsertComma(cReplaced), cReplaced != 1 ? "s" : "");
	}
	else
	{
		CZDoc * pzd = m_pzef->GetCurrentDoc();
		UINT ichSelStart;
		UINT ichSelStop;
		pzd->GetSelection(&ichSelStart, &ichSelStop);
		FindDirection fd = (FindDirection)::SendMessage(
			::GetDlgItem(m_hwndDlg, IDC_COMBO_DIRECTION), CB_GETCURSEL, 0, 0);
		if (fd == kfdAll)
		{
			m_fi.ichStart = 0;
			m_fi.ichStop = -1;
		}
		else
		{
			if (ichSelStart == ichSelStop)
			{
				if (fd == kfdDown)
				{
					m_fi.ichStart = ichSelStart;
					m_fi.ichStop = -1;
				}
				else
				{
					m_fi.ichStart = 0;
					m_fi.ichStop = ichSelStart;
				}
			}
			else
			{
				m_fi.ichStart = ichSelStart;
				m_fi.ichStop = ichSelStop;
			}
		}

		cReplaced = pzd->DoReplaceAll(&m_fi);
		if (fd != kfdAll)
		{
			bool fDown = (fd == kfdDown);
			int nResponse = IDYES;
			if (ichSelStart != ichSelStop)
			{
				wsprintf(szMessage,
					"%s has finished searching the selection. %d replacement%s made. "
					"Do you want to search the remainder of the document?",
					g_pszAppName, cReplaced, cReplaced != 1 ? "s were" : " was");
				if (::MessageBox(NULL, szMessage, g_pszAppName,
					MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) == IDYES)
				{
					if (fDown)
					{
						m_fi.ichStart = ichSelStop;
						m_fi.ichStop = -1;
					}
					else
					{
						m_fi.ichStart = 0;
						m_fi.ichStop = ichSelStart;
					}
					cReplaced += pzd->DoReplaceAll(&m_fi);
				}
				else
				{
					nResponse = IDNO;
				}
			}
			if (nResponse == IDYES)
			{
				wsprintf(szMessage,
					"%s has reached the %s of the document. %d replacement%s made. "
					"Do you want to continue searching at the %s?", g_pszAppName,
					fDown ? "end" : "beginning", cReplaced, cReplaced != 1 ? "s were" : " was",
					fDown ? "beginning" : "end");
				if (::MessageBox(NULL, szMessage, g_pszAppName,
					MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) == IDYES)
				{
					if (fDown)
					{
						m_fi.ichStart = 0;
						m_fi.ichStop = ichSelStart;
					}
					else
					{
						m_fi.ichStart = ichSelStop;
						m_fi.ichStop = -1;
					}
					cReplaced += pzd->DoReplaceAll(&m_fi);
				}
			}
		}

		wsprintf(szMessage,
			"%s has completed its search of the document and has made %s replacement%s.",
			g_pszAppName, CZEditFrame::InsertComma(cReplaced), cReplaced != 1 ? "s" : "");
	}

	// TODO: This should refresh all the windows
	CZDoc * pzd = m_pzef->GetCurrentDoc();
	::InvalidateRect(pzd->GetHwnd(), NULL, FALSE);
	::UpdateWindow(pzd->GetHwnd());
	if (!fSuccess)
	{
		wsprintf(szMessage,
			"%s encountered an error while replacing. It made %d replacement%s.",
			g_pszAppName, cReplaced, cReplaced != 1 ? "s" : "");
	}
	::MessageBox(NULL, szMessage, g_pszAppName, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
	m_fBusy = false;
	return fSuccess;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CFindDlg::FindDlgProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	CFindDlg * pfd = (CFindDlg *)::GetWindowLong(hwndDlg, GWL_USERDATA);

	switch (iMsg)
	{
#ifdef WS_EX_LAYERED
	case WM_ACTIVATE:
		if (pfd && g_fg.m_pfnSetLayeredWindowAttributes && ::IsWindowVisible(pfd->m_hwndDlg))
		{
			BYTE bAlpha = 255;
			if (LOWORD(wParam) == WA_INACTIVE)
				bAlpha = 100;
			(*g_fg.m_pfnSetLayeredWindowAttributes)(pfd->m_hwndDlg, 0, bAlpha, LWA_ALPHA);
			return TRUE;
		}
		break;
#endif // WS_EX_LAYERED

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			AssertPtr(pfd);
			pfd->m_pzef->Animate(hwndDlg, ID_EDIT_FIND, true, false);
			::ShowWindow(hwndDlg, SW_HIDE);
			return TRUE;

		case IDC_COMBO_FIND:
			if (HIWORD(wParam) == CBN_EDITCHANGE)
			{
				AssertPtr(pfd);
				pfd->ResetFind();
			}
			return TRUE;

		case IDC_COMBO_DIRECTION:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				AssertPtr(pfd);
				pfd->ResetFind();
			}
			return TRUE;

		case IDC_WHOLEWORD:
		case IDC_MATCHCASE:

		case IDC_REPLACEFILES:
			AssertPtr(pfd);
			pfd->ResetFind();
			return TRUE;

		case IDC_REPLACE:
			AssertPtr(pfd);
			return pfd->OnReplace();

		case IDC_FINDNEXT:
			AssertPtr(pfd);
			return pfd->OnFind();

		case IDC_REPLACEALL:
			AssertPtr(pfd);
			return pfd->OnReplaceAll();

		case ID_SEARCH_PARAGRAPH:
		case ID_SEARCH_TABCHARACTER:
		case ID_SEARCH_CARETCHARACTER:
		case ID_SEARCH_ASCIICODE:
		case ID_SEARCH_UNICODECODE:
			AssertPtr(pfd);
			return pfd->EnterSpecialText(LOWORD(wParam));
		}
		break;

	case WM_LBUTTONDOWN:
		{
			AssertPtr(pfd);
			POINT point = {LOWORD(lParam), HIWORD(lParam)};
			return pfd->OnLButtonDown(wParam, point);
		}

	case WM_MOUSEMOVE:
		{
			AssertPtr(pfd);
			POINT point = {LOWORD(lParam), HIWORD(lParam)};
			return pfd->OnMouseMove(wParam, point);
		}

	case WM_PAINT:
		AssertPtr(pfd);
		return pfd->OnPaint();

	case WM_DRAWITEM:
		return pfd->m_pzef->GetContextMenu()->OnDrawItem((LPDRAWITEMSTRUCT)lParam);

	case WM_MEASUREITEM:
		return pfd->m_pzef->GetContextMenu()->OnMeasureItem((LPMEASUREITEMSTRUCT)lParam);
	}
	return FALSE;
}


/***********************************************************************************************
	CFindFilesDlg methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CFindFilesDlg::CFindFilesDlg(CZEditFrame * pzef)
{
	AssertPtr(pzef);
	m_pzef = pzef;
	m_hwndDlg = NULL;
	m_fVisible = false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CFindFilesDlg::FindInFilesProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		::SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				CFindFilesDlg * pffd = (CFindFilesDlg *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				return pffd->StartFind(hwndDlg);
			}

		case IDCANCEL:
			::ShowWindow(hwndDlg, SW_HIDE);
			return TRUE;

		case IDBROWSE:
			{
				LPMALLOC pMalloc = NULL;
				::SHGetMalloc(&pMalloc);
				if (pMalloc)
				{
					char szDir[MAX_PATH] = {0};
					BROWSEINFO bi = {hwndDlg, NULL, szDir,
						"Choose the directory to start your search in.", BIF_RETURNONLYFSDIRS,
						NULL, 0, 0};
					ITEMIDLIST * pidl = ::SHBrowseForFolder(&bi);
					if (pidl)
					{
						if (::SHGetPathFromIDList(pidl, szDir))
						{
							::SendMessage(::GetDlgItem(hwndDlg, IDC_FOLDER), WM_SETTEXT, 0,
								(LPARAM)szDir);
						}
						pMalloc->Free(pidl);
						pidl = NULL;
					}
					pMalloc->Release();
					pMalloc = NULL;
				}
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CFindFilesDlg::FindResultsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			::SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			FindInFilesInfo * pfifi = (FindInFilesInfo *)lParam;
			HWND hwndList = pfifi->m_hwndList = GetDlgItem(hwndDlg, IDC_LISTVIEW);
			ListView_SetExtendedListViewStyle(hwndList,
				LVS_EX_FLATSB | LVS_EX_TWOCLICKACTIVATE | LVS_EX_FULLROWSELECT);
			InitializeFlatSB(hwndList);
			FlatSB_SetScrollProp(hwndList, WSB_PROP_VSTYLE, FSB_FLAT_MODE, TRUE);

			pfifi->m_hwndStatus = ::CreateStatusWindow(WS_CHILD | WS_VISIBLE |
				WS_CLIPSIBLINGS | CCS_BOTTOM | SBARS_SIZEGRIP, NULL, hwndDlg, 0);
			::SendMessage(pfifi->m_hwndStatus, WM_SETFONT,
				(long)::GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(true, 0));

			// Load size information from the registry
			HKEY hkey;
			char szWidths[100] = {0};
			int nWidths[4];
			RECT rect = { 0 };
			if (::RegOpenKeyEx(HKEY_CURRENT_USER, kpszRootRegKey, 0, KEY_READ, &hkey) ==
				ERROR_SUCCESS)
			{
				DWORD dwSize = sizeof(RECT);
				::RegQueryValueEx(hkey, "Find In Files Position", 0, NULL, (BYTE *)&rect,
					&dwSize);
				dwSize = sizeof(szWidths);
				::RegQueryValueEx(hkey, "Find In Files Widths", 0, NULL, (BYTE *)szWidths,
					&dwSize);
				::RegCloseKey(hkey);
			}
			if (!rect.right)
			{
				::GetWindowRect(hwndDlg, &rect);
				rect.right++; // Force window to be resized so we get an WM_SIZE message
			}
			::MoveWindow(hwndDlg, rect.left, rect.top, rect.right - rect.left,
				rect.bottom - rect.top, TRUE);
			if (sscanf_s(szWidths, "%d %d %d %d", &nWidths[0], &nWidths[1], &nWidths[2],
				&nWidths[3]) != 4)
			{
				::GetClientRect(hwndList, &rect);
				nWidths[0] = (int)(rect.right * .2);
				nWidths[1] = (int)(rect.right * .3);
				nWidths[2] = (int)(rect.right * .1);
				nWidths[3] = rect.right - nWidths[0] - nWidths[1] - nWidths[2];
			}
			// Add columns to the listview.
			LVCOLUMN lvc = {LVCF_TEXT | LVCF_WIDTH};
			for (int iCol = 0; iCol < 4; iCol++)
			{
				lvc.pszText = g_pszColumns[0][iCol];
				lvc.cx = nWidths[iCol];
				ListView_InsertColumn(hwndList, iCol, &lvc);
			}
			::SetFocus(hwndList);
		}
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			::DestroyWindow(hwndDlg);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			::ShowWindow(hwndDlg, SW_HIDE);
			// Save size information to the registry
			HKEY hkey;
			DWORD dwT;
			if (::RegCreateKeyEx(HKEY_CURRENT_USER, kpszRootRegKey, 0, NULL, 0, KEY_WRITE, NULL,
				&hkey, &dwT) == ERROR_SUCCESS)
			{
				RECT rect;
				::GetWindowRect(hwndDlg, &rect);
				::RegSetValueEx(hkey, "Find In Files Position", 0, REG_BINARY, (BYTE *)&rect,
					sizeof(RECT));

				HWND hwndList = ::GetDlgItem(hwndDlg, IDC_LISTVIEW);
				char szColumns[50];
				wsprintf(szColumns, "%d %d %d %d", ListView_GetColumnWidth(hwndList, 0),
					ListView_GetColumnWidth(hwndList, 1),
					ListView_GetColumnWidth(hwndList, 2),
					ListView_GetColumnWidth(hwndList, 3));
				::RegSetValueEx(hkey, "Find In Files Widths", 0, REG_SZ, (BYTE *)szColumns,
					strlen(szColumns) + 1);
				::RegCloseKey(hkey);
			}

			FindInFilesInfo * pfifi = (FindInFilesInfo *)::GetWindowLong(hwndDlg,
				GWL_USERDATA);
			pfifi->m_hwndDlg = NULL;
			pfifi->m_cs.Enter();
			bool fCanDelete = pfifi->m_fCanDelete;
			pfifi->m_fCanDelete = !fCanDelete;
			pfifi->m_cs.Leave();
			if (fCanDelete)
				delete pfifi;
			return TRUE;
		}

	case WM_NOTIFY:
		{
			LPNMHDR pnmh = (LPNMHDR)lParam;
			switch (pnmh->code)
			{
			case NM_DBLCLK:
				{
					NMLISTVIEW * pnmlv = (NMLISTVIEW *)lParam;
					FindInFilesInfo * pfifi = (FindInFilesInfo *)::GetWindowLong(hwndDlg,
						GWL_USERDATA);
					AssertPtr(pfifi);
					HWND hwndList = ::GetDlgItem(hwndDlg, IDC_LISTVIEW);
					LVITEM lvi = {LVIF_PARAM, pnmlv->iItem};
					char szFilename[MAX_PATH + 2] = {0};
					ListView_GetItem(hwndList, &lvi);
					FoundFile * pff = (FoundFile *)lvi.lParam;
					AssertPtr(pff);
					wsprintf(szFilename, "%s\\%s", pff->m_szFilename, pff->m_pszFile);
					// TODO: Pass in something to ReadFile that calculates the character offset
					// from the byte offset and uses it down below.
					if (pfifi->m_pzef->ReadFile(szFilename, kftDefault))
					{
						CZDoc * pzd = pfifi->m_pzef->GetCurrentDoc();
						DWORD dwStartChar = pff->m_ichMatch;
						DWORD dwStopChar = dwStartChar + pfifi->m_cchFindWhat;
						pzd->SetSelection(dwStartChar, dwStopChar, true, true);
						//pzd->SetSelectionFromFileFind(pff->m_ichMatch, pff->m_dwLine, pff->m_ichInPara);
						::SetFocus(pzd->GetHwnd());
					}
					break;
				}

			case LVN_COLUMNCLICK:
				{
					NMLISTVIEW * pnmlv = (NMLISTVIEW *)lParam;
					FindInFilesInfo * pfifi = (FindInFilesInfo *)::GetWindowLong(hwndDlg,
						GWL_USERDATA);
					HWND hwndList = ::GetDlgItem(hwndDlg, IDC_LISTVIEW);
					int iOldColumn = pfifi->m_nSortedColumn;
					if (pnmlv->iSubItem == iOldColumn)
					{
						pfifi->m_fSortedAscending = pfifi->m_fSortedAscending ? false : true;
					}
					else
					{
						if (iOldColumn != -1)
						{
							LVCOLUMN lvc = {LVCF_TEXT};
							lvc.pszText = g_pszColumns[0][iOldColumn];
							ListView_SetColumn(hwndList, iOldColumn, &lvc);
						}
						LVCOLUMN lvc = {LVCF_TEXT};
						lvc.pszText = g_pszColumns[1][pnmlv->iSubItem];
						ListView_SetColumn(hwndList, pnmlv->iSubItem, &lvc);
					}
					pfifi->m_nSortedColumn = pnmlv->iSubItem;

					LVITEM lvi = {LVIF_TEXT, pnmlv->iItem, 1};
					char szInfo[MAX_PATH];
					lvi.pszText = szInfo;
					lvi.cchTextMax = sizeof(szInfo);
					ListView_SortItems(hwndList, PfnCompare, (LPARAM)pfifi);
					break;
				}

			case NM_CUSTOMDRAW:
				{
					LPNMCUSTOMDRAW pnmcd = (LPNMCUSTOMDRAW)lParam;
					HWND hwndHeader = ::GetWindow(::GetDlgItem(hwndDlg, IDC_LISTVIEW),
						GW_CHILD);
					if (pnmcd->dwDrawStage == CDDS_PREPAINT)
					{
						::SetWindowLong(hwndDlg, DWL_MSGRESULT, CDRF_NOTIFYPOSTPAINT);
					}
					else if (pnmcd->dwDrawStage == CDDS_POSTPAINT)
					{
						FindInFilesInfo * pfifi = (FindInFilesInfo *)::GetWindowLong(hwndDlg,
							GWL_USERDATA);
						if (!pfifi->m_dwMatches)
						{
							RECT rect;
							::GetClientRect(hwndHeader, &rect);
							char szMessage[] = "There are no items to show in this view.";
							COLORREF crOld = ::SetTextColor(pnmcd->hdc, 0x7F7F7F);
							::TextOut(pnmcd->hdc, 3, rect.bottom + 5, szMessage,
								lstrlen(szMessage));
							::SetTextColor(pnmcd->hdc, crOld);
						}
						if (pfifi->m_nSortedColumn != -1)
						{
							RECT rect;
							HDC hdc = GetDC(hwndHeader);
							Header_GetItemRect(hwndHeader, pfifi->m_nSortedColumn, &rect);
							HPEN hPenOld = (HPEN)::SelectObject(hdc,
								::CreatePen(PS_SOLID, 0, 0x7F7F7F));
							if (pfifi->m_fSortedAscending)
							{
								::MoveToEx(hdc, rect.left + 9, 4, NULL);
								::LineTo(hdc, rect.left + 6, 11);
								::SetPixel(hdc, rect.left + 7, 10, 0x7F7F7F);
							}
							else
							{
								::MoveToEx(hdc, rect.left + 9, 11, NULL);
								::LineTo(hdc, rect.left + 6, 4);
								::LineTo(hdc, rect.left + 13, 4);
								::SetPixel(hdc, rect.left + 7, 5, 0x7F7F7F);
							}
							::DeleteObject(::SelectObject(hdc,
								::CreatePen(PS_SOLID, 0, 0xFFFFFF)));
							if (pfifi->m_fSortedAscending)
							{
								::MoveToEx(hdc, rect.left + 10, 4, NULL);
								::LineTo(hdc, rect.left + 13, 11);
								::LineTo(hdc, rect.left + 7, 11);
								::SetPixel(hdc, rect.left + 12, 10, 0xFFFFFF);
							}
							else
							{
								::MoveToEx(hdc, rect.left + 10, 11, NULL);
								::LineTo(hdc, rect.left + 13, 4);
								::SetPixel(hdc, rect.left + 12, 5, 0xFFFFFF);
							}
							::DeleteObject(::SelectObject(hdc, hPenOld));
							::ReleaseDC(hwndHeader, hdc);
						}
					}
					return TRUE;
				}

			case LVN_GETDISPINFO:
				{
					NMLVDISPINFO * pnmv = (NMLVDISPINFO *)lParam;
					FoundFile * pff = (FoundFile *)pnmv->item.lParam;
					AssertPtr(pff);
					if (pnmv->item.iSubItem == 0)
						pnmv->item.pszText = pff->m_pszFile;
					if (pnmv->item.iSubItem == 1)
						pnmv->item.pszText = pff->m_szFilename;
					if (pnmv->item.iSubItem == 2)
						_ltoa_s(pff->m_dwLine, pnmv->item.pszText, pnmv->item.cchTextMax, 10);
					if (pnmv->item.iSubItem == 3)
						pnmv->item.pszText = pff->m_pszLine;
					return TRUE;
				}

			case LVN_DELETEITEM:
				{
					LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
					LVITEM lvi = {LVIF_PARAM, pnmv->iItem};
					ListView_GetItem(pnmv->hdr.hwndFrom, &lvi);
					FoundFile * pff = (FoundFile *)pnmv->lParam;
					AssertPtr(pff);
					delete pff;
					return TRUE;
				}

			case NM_RCLICK:
				{
					FindInFilesInfo * pfifi = (FindInFilesInfo *)::GetWindowLong(hwndDlg, GWL_USERDATA);
					AssertPtr(pfifi);
					HMENU hMenu = pfifi->m_pzef->GetContextMenu()->GetSubMenu(kcmFindInFiles);

					POINT pt;
					::GetCursorPos(&pt);
					int nCommand = ::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON |
						TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, pfifi->m_hwndDlg, NULL);
					if (nCommand == ID_FINDINFILES_COPYTOCLIPBOARD)
					{
						int cchSize = 100000;
						char * prgch = (char *)realloc(NULL, cchSize);
						prgch[0] = (char)0;
						char * pchMax = prgch + cchSize;
						char * pchCur = prgch;
						char rgchLine[100] = { 0 };

						AddString(prgch, pchCur, pchMax, cchSize, "Name\tFolder\tLine\tText", "\r\n");

						LVITEM lvi = { LVIF_PARAM };
						int cItems = ListView_GetItemCount(pfifi->m_hwndList);
						for (int iItem = 0; iItem < cItems; iItem++)
						{
							lvi.iItem = iItem;
							if (ListView_GetItem(pfifi->m_hwndList, &lvi))
							{
								FoundFile * pff = (FoundFile *)lvi.lParam;
								AssertPtr(pff);

								AddString(prgch, pchCur, pchMax, cchSize, pff->m_pszFile, "\t");
								AddString(prgch, pchCur, pchMax, cchSize, pff->m_szFilename, "\t");
								_ltoa_s(pff->m_dwLine, rgchLine, 10);
								AddString(prgch, pchCur, pchMax, cchSize, rgchLine, "\t");
								AddString(prgch, pchCur, pchMax, cchSize, pff->m_pszLine, "\r\n");
							}
						}

						bool fSuccessful = false;
						if (::OpenClipboard(pfifi->m_hwndDlg))
						{
							::EmptyClipboard();

							int cch = strlen(prgch);
							HGLOBAL hglbCopy = ::GlobalAlloc(GMEM_MOVEABLE, cch + 1);
							if (hglbCopy != NULL)
							{
								char * lptstrCopy = (char *)::GlobalLock(hglbCopy);
								memcpy(lptstrCopy, prgch, cch);
								lptstrCopy[cch] = (char)0;
								::GlobalUnlock(hglbCopy);

								::SetClipboardData(CF_TEXT, hglbCopy); 
								fSuccessful = true;
							}

							::CloseClipboard();
						}

						if (fSuccessful)
							::MessageBox(pfifi->m_hwndDlg, "The files were copied to the clipboard.", g_pszAppName, MB_OK | MB_ICONINFORMATION);
						else
							::MessageBox(pfifi->m_hwndDlg, "The files could not be copied to the clipboard.", g_pszAppName, MB_OK | MB_ICONEXCLAMATION);
					}
					return TRUE;
				}
			}
			break;
		}

	case WM_SIZE:
		{
			FindInFilesInfo * pfifi = (FindInFilesInfo *)::GetWindowLong(hwndDlg,
				GWL_USERDATA);
			if (pfifi && pfifi->m_hwndStatus)
			{
				RECT rect;
				::GetClientRect(pfifi->m_hwndStatus, &rect);
				int nListHeight = HIWORD(lParam) - rect.bottom;
				::MoveWindow(pfifi->m_hwndList, 0, 0, LOWORD(lParam), nListHeight, TRUE);
				::MoveWindow(pfifi->m_hwndStatus, 0, nListHeight, LOWORD(lParam),
					rect.bottom, TRUE);
			}
			return TRUE;
		}
	}
	return FALSE;
}

void CFindFilesDlg::AddString(char *& prgch, char *& pchCur, char *& pchMax,
	int &cchSize, char * psz, char * pszExtra)
{
	int cch = strlen(psz);
	int cchExtra = strlen(pszExtra);
	if (pchCur + cch + cchExtra >= pchMax)
	{
		int cchCur = pchCur - prgch;
		prgch = (char *)realloc(prgch, cchSize += 100000);
		pchCur = prgch + cchCur;
		pchMax = prgch + cchSize;
	}
	strncpy_s(pchCur, pchMax - pchCur, psz, cch);
	strncpy_s(pchCur + cch, pchMax - pchCur, pszExtra, cchExtra);
	pchCur += cch + cchExtra;
	*pchCur = 0;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CALLBACK CFindFilesDlg::PfnCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	FindInFilesInfo * pfifi = (FindInFilesInfo *)lParamSort;
	AssertPtr((FoundFile *)lParam1);
	AssertPtr((FoundFile *)lParam2);
	int nResult;
	switch (pfifi->m_nSortedColumn)
	{
	case 0:
		nResult = lstrcmp(((FoundFile *)lParam1)->m_pszFile,
			((FoundFile *)lParam2)->m_pszFile);
		break;

	case 1:
		nResult = lstrcmp(((FoundFile *)lParam1)->m_szFilename,
			((FoundFile *)lParam2)->m_szFilename);
		break;

	case 2:
		nResult = ((FoundFile *)lParam1)->m_dwLine - ((FoundFile *)lParam2)->m_dwLine;
		break;

	case 3:
		nResult = lstrcmp(((FoundFile *)lParam1)->m_pszLine,
			((FoundFile *)lParam2)->m_pszLine);
		break;
	}
	return pfifi->m_fSortedAscending ? nResult : -nResult;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindFilesDlg::ShowDialog()
{
	AssertPtr(m_pzef);
	if (!m_hwndDlg)
	{
		// This is the first time the dialog has been displayed
		m_hwndDlg = ::CreateDialogParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_FINDINFILES),
			m_pzef->GetHwnd(), FindInFilesProc, (LPARAM)this);

		// Initialize the starting directory
		char szDir[MAX_PATH];
		strcpy_s(szDir, m_pzef->GetCurrentDoc()->GetFilename());
		char * pSlash = strrchr(szDir, '\\');
		if (pSlash)
			*pSlash = 0;
		else
			::GetCurrentDirectory(sizeof(szDir), szDir);
		::SetDlgItemText(m_hwndDlg, IDC_FOLDER, szDir);
	}

	HWND hwndFilter = ::GetDlgItem(m_hwndDlg, IDC_COMBO_FILTER);
	::SendMessage(hwndFilter, CB_RESETCONTENT, 0, 0);
	::SendMessage(hwndFilter, CB_ADDSTRING, 0, (LPARAM)"*.*");
	::SendMessage(hwndFilter, CB_SETCURSEL, 0, 0);

	HDC hdc = ::GetDC(hwndFilter);
	HFONT hfontOld = (HFONT)::SelectObject(hdc, ::GetStockObject(DEFAULT_GUI_FONT));
	SIZE size;
	int dxp = 0;
	if (::GetTextExtentPoint32(hdc, "*.*", 3, &size))
		dxp = max(dxp, size.cx);

	// Load filter options from the registry.
	HKEY hkey;
	char szRegKey[MAX_PATH] = { 0 };
	strcpy_s(szRegKey, kpszRootRegKey);
	strcat_s(szRegKey, "\\FindFilters");
	int iFilter = 0;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, szRegKey, 0, KEY_READ, &hkey) ==
		ERROR_SUCCESS)
	{
		char szName[MAX_PATH];
		char szValue[MAX_PATH];
		DWORD cbValue = sizeof(szName) / sizeof(char);
		DWORD cchName = sizeof(szName);
		DWORD dwType;
		while (::RegEnumValue(hkey, iFilter, szName, &cchName, NULL, &dwType,
			(BYTE *)szValue, &cbValue) == ERROR_SUCCESS)
		{
			if (dwType == REG_SZ)
			{
				if (*szName == 0)
				{
					// This is the default value, so set the text of the combobox to this value.
					::SetWindowText(hwndFilter, szValue);
				}
				else
				{
					// This item should be added to the combobox dropdown.
					::SendMessage(hwndFilter, CB_ADDSTRING, 0, (LPARAM)szValue);
					if (::GetTextExtentPoint32(hdc, szValue, cbValue, &size))
						dxp = max(dxp, size.cx);
				}
			}
			cchName = sizeof(szName);
			cbValue = sizeof(szValue) / sizeof(char);
			iFilter++;
		}
		::RegCloseKey(hkey);
	}

	::SelectObject(hdc, hfontOld);
	::ReleaseDC(hwndFilter, hdc);
	dxp += (::GetSystemMetrics(SM_CXBORDER) * 2) + ::GetSystemMetrics(SM_CXVSCROLL) + 2;
	::SendMessage(hwndFilter, CB_SETDROPPEDWIDTH, dxp, 0);

	::SetFocus(::GetDlgItem(m_hwndDlg, IDC_COMBO_FIND));

	// If there is a selection on one line, put the whole selection into the Find What box.
	UINT ichSelStart;
	UINT ichSelStop;
	void * pv;
	CZDoc * pzd = m_pzef->GetCurrentDoc();
	pzd->GetSelection(&ichSelStart, &ichSelStop);
	if (ichSelStart != ichSelStop && ichSelStop - ichSelStart < 100)
	{
		UINT cch = pzd->GetText(ichSelStart, ichSelStop - ichSelStart, &pv);
		if (pzd->GetFileType() != kftAnsi)
		{
			int cchBuffer = cch << 3;
			char * pBuffer = new char[cchBuffer];
			char * pDst = pBuffer;
			char * pLim = pDst + cchBuffer;
			wchar * pSrc = (wchar *)pv;
			wchar * pStop = pSrc + cch + 1;
			long ch;
			while (pSrc < pStop)
			{
				if ((ch = *pSrc++) > 0xFF)
				{
					*pDst++ = '^';
					*pDst++ = 'x';
					pDst += sprintf_s(pDst, pLim - pDst, "%x", ch);
					*pDst++ = ';';
				}
				else
				{
					*pDst++ = (char)((BYTE)ch);
				}
			}
			delete pv;
			pv = pBuffer;
		}
		char * pSrc = strrchr((char *)pv, '\n');
		if (!pSrc)
		{
			::SetDlgItemText(m_hwndDlg, IDC_COMBO_FIND, (char *)pv);
			::SendMessage(::GetWindow(::GetDlgItem(m_hwndDlg, IDC_COMBO_FIND), GW_CHILD),
				EM_SETSEL, 0, -1);
		}
		delete pv;
	}

	m_fVisible = true;
	::ShowWindow(m_hwndDlg, SW_SHOW);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindFilesDlg::StartFind(HWND hwndDlg)
{
	FindInFilesInfo * pfifi = new FindInFilesInfo(m_pzef);
	if (!pfifi)
		return false;

	HWND hwnd = ::GetDlgItem(m_hwndDlg, IDC_FOLDER);
	int cch = ::SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
	if (cch > 0)
	{
		pfifi->m_pszFolder = new char[cch + 1];
		if (!pfifi->m_pszFolder)
		{
			delete pfifi;
			return false;
		}
		::SendMessage(hwnd, WM_GETTEXT, cch + 1, (LPARAM)pfifi->m_pszFolder);
	}
	else
	{
		::MessageBox(m_hwndDlg, "Please enter a folder to start searching in.", g_pszAppName,
			MB_OK | MB_ICONWARNING);
		delete pfifi;
		return false;
	}

	hwnd = ::GetDlgItem(m_hwndDlg, IDC_COMBO_FILTER);
	cch = ::SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
	if (cch > 0)
	{
		pfifi->m_pszFileTypes = new char[cch + 1];
		if (!pfifi->m_pszFileTypes)
		{
			delete pfifi;
			return false;
		}
		::SendMessage(hwnd, WM_GETTEXT, cch + 1, (LPARAM)pfifi->m_pszFileTypes);
	}
	if (cch == 0 || strchr(pfifi->m_pszFileTypes, ';'))
	{
		if (cch > 0)
		{
			char * pSrc = pfifi->m_pszFileTypes;
			char * pStop = pSrc + cch;
			bool fInvalid = false;
			// If there is more than one type of file specified, it must be of the
			// form "*.xxx;*.xxx;*.xxx". It cannot contain any * or ? except a single
			// * directly in front of each period.
			while (!fInvalid && pSrc < pStop)
			{
				if (*pSrc != '*' || *(++pSrc) != '.')
				{
					fInvalid = true;
				}
				else
				{
					while (*++pSrc && *pSrc != ';')
					{
						if (*pSrc == '*' || *pSrc == '?')
						{
							fInvalid = true;
							break;
						}
					}
				}
				pSrc++;
			}
			if (fInvalid)
			{
				::MessageBox(m_hwndDlg, "Invalid file specification.", g_pszAppName,
					MB_OK | MB_ICONWARNING);
				delete pfifi;
				::SetFocus(::GetDlgItem(m_hwndDlg, IDC_COMBO_FILTER));
				return false;
			}
		}

		// Use a default of *.*
		cch = 3;
		pfifi->m_pszFileTypes = new char[cch + 1];
		if (!pfifi->m_pszFileTypes)
		{
			delete pfifi;
			return false;
		}
		strcpy_s(pfifi->m_pszFileTypes, cch + 1, "*.*");
	}

	// Save the selected file type setting to the registry.
	HKEY hkey;
	DWORD dwT;
	char szRegKey[MAX_PATH] = { 0 };
	strcpy_s(szRegKey, kpszRootRegKey);
	strcat_s(szRegKey, "\\FindFilters");
	if (::RegCreateKeyEx(HKEY_CURRENT_USER, szRegKey, 0, NULL, 0, KEY_WRITE, NULL,
		&hkey, &dwT) == ERROR_SUCCESS)
	{
		::RegSetValueEx(hkey, NULL, 0, REG_SZ, (BYTE *)pfifi->m_pszFileTypes, cch);
		::RegCloseKey(hkey);
	}

	// TODO: Support pattern matching like the real Find dialog.
	hwnd = ::GetDlgItem(m_hwndDlg, IDC_COMBO_FIND);
	cch = ::SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
	if (cch > 0)
	{
		pfifi->m_pszFindWhat = new char[cch + 1];
		if (!pfifi->m_pszFindWhat)
		{
			delete pfifi;
			return false;
		}
		::SendMessage(hwnd, WM_GETTEXT, cch + 1, (LPARAM)pfifi->m_pszFindWhat);
		AddStringToCombo(m_hwndDlg, IDC_COMBO_FIND, pfifi->m_pszFindWhat);
	}
	else
	{
		pfifi->m_pszFindWhat = NULL;
	}

	pfifi->m_fWholeWord = ::IsDlgButtonChecked(m_hwndDlg, IDC_WHOLEWORD) != FALSE;
	pfifi->m_fCaseSensitive = ::IsDlgButtonChecked(m_hwndDlg, IDC_MATCHCASE) != FALSE;
	pfifi->m_fSubFolders = ::IsDlgButtonChecked(m_hwndDlg, IDC_SUBFOLDERS) != FALSE;

	::ShowWindow(hwndDlg, SW_HIDE);
	pfifi->m_hwndDlg = ::CreateDialogParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_FINDRESULTS),
		pfifi->m_pzef->GetHwnd(), FindResultsProc, (LPARAM)pfifi);
	_beginthread(StartFind, 0, (void *)pfifi);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFindFilesDlg::StartFind(void * pv)
{
	FindInFilesInfo * pfifi = (FindInFilesInfo *)pv;
	AssertPtr(pfifi);

	// Set the caption
	std::string sCaption("ZEdit: ");
	if (!pfifi->m_pszFileTypes || strcmp("*.*", pfifi->m_pszFileTypes) == 0)
	{
		if (pfifi->m_pszFindWhat)
		{
			sCaption += "Files containing text ";
			sCaption += pfifi->m_pszFindWhat;
		}
		else
		{
			sCaption += "All Files";
		}
	}
	else
	{
		sCaption += "Files named ";
		sCaption += pfifi->m_pszFileTypes;
		if (pfifi->m_pszFindWhat)
		{
			sCaption += " containing text ";
			sCaption += pfifi->m_pszFindWhat;
		}
	}
	::SetWindowText(pfifi->m_hwndDlg, sCaption.c_str());

	HWND hwndStatus = pfifi->m_hwndStatus;
	::SendMessage(hwndStatus, SB_SIMPLE, TRUE, 0);
	pfifi->m_cchFindWhat = lstrlen(pfifi->m_pszFindWhat);
	pfifi->CreateBitMap();

	bool fFindAll = (strcmp(pfifi->m_pszFileTypes, "*.*") == 0);
	GetFilesInDir(pfifi, fFindAll, ::GetDlgItem(pfifi->m_hwndDlg, IDC_LISTVIEW),
		pfifi->m_pszFolder);

	::SendMessage(hwndStatus, SB_SIMPLE, FALSE, 0);
	int rgnWidth[] = {150, 300, -1};
	::SendMessage(hwndStatus, SB_SETPARTS, sizeof(rgnWidth) / sizeof(int), (long)rgnWidth);
	char szMessage[100];
	wsprintf(szMessage, "Files searched: %d", pfifi->m_dwFilesSearched);
	::SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)szMessage);
	wsprintf(szMessage, "Matched files: %d", pfifi->m_dwFilesMatched);
	::SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)szMessage);
	wsprintf(szMessage, "Matches: %d", pfifi->m_dwMatches);
	::SendMessage(hwndStatus, SB_SETTEXT, 2, (LPARAM)szMessage);

	pfifi->m_cs.Enter();
	bool fCanDelete = pfifi->m_fCanDelete;
	pfifi->m_fCanDelete = !fCanDelete;
	pfifi->m_cs.Leave();
	if (fCanDelete)
		delete pfifi;
	::MessageBeep(MB_OK);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFindFilesDlg::GetFilesInDir(FindInFilesInfo * pfifi, bool fFindAll, HWND hwndList,
	char * pszFolder)
{
	if (!pfifi->m_hwndDlg)
		return;
	if (pfifi->m_hwndStatus)
	{
		char szMessage[MAX_PATH + 11] = "Searching: ";
		lstrcpy(szMessage + 11, pszFolder);
		::SendMessage(pfifi->m_hwndStatus, SB_SETTEXT, 255, (LPARAM)szMessage);
	}
	char szDir[MAX_PATH];
	char szFilename[MAX_PATH];
	lstrcpy(szDir, pszFolder);
	int cch = lstrlen(szDir);
	if (szDir[cch - 1] != '\\')
	{
		szDir[cch++] = '\\';
		szDir[cch] = 0;
	}
	lstrcpy(szFilename, szDir);
	lstrcpy(szFilename + cch, "*.*");

	WIN32_FIND_DATA fd = {0};
	HANDLE hFindFile = ::FindFirstFile(szFilename, &fd);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			lstrcpy(szFilename + cch, fd.cFileName);
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (pfifi->m_fSubFolders && *fd.cFileName != '.')
					GetFilesInDir(pfifi, fFindAll, hwndList, szFilename);
			}
			else
			{
				bool fSearchFile = fFindAll;
				if (!fSearchFile)
				{
					char * pDot = strrchr(fd.cFileName, '.');
					if (pDot)
					{
						char * pDst = StrStrI(pfifi->m_pszFileTypes, pDot);
						if (pDst)
						{
							pDst += strlen(pDot);
							fSearchFile = (*pDst == 0 || *pDst == ';' || isspace(*pDst));
						}
					}
				}
				if (fSearchFile)
					SearchFile(pfifi, hwndList, szFilename, fd.nFileSizeLow);
			}
		} while (pfifi->m_hwndDlg && ::FindNextFile(hFindFile, &fd));
		::FindClose(hFindFile);
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CFindFilesDlg::SearchFile(FindInFilesInfo * pfifi, HWND hwndList, char * pszFilename,
	DWORD dwFileSize)
{
	AssertPtr(pfifi);
	if (!pfifi->m_hwndDlg)
		return;
	if (!pfifi->m_pszFindWhat)
	{
		AddItem(pszFilename, NULL, ++pfifi->m_dwMatches, 1, 0, hwndList);
		pfifi->m_dwFilesSearched++;
		pfifi->m_dwFilesMatched++;
		return;
	}

	char * pSlash = strrchr(pszFilename, '\\');
	if (!pSlash)
		return;

	HANDLE hFile = ::CreateFile(pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	char * pszStart = NULL;
	HANDLE hFileMap = ::CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hFileMap)
	{
		pszStart = (char *)::MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
		::CloseHandle(hFileMap);
	}
	::CloseHandle(hFile);
	if (!pszStart)
		return;

	unsigned int cch = pfifi->m_cchFindWhat;
	char * pszStop = pszStart + dwFileSize - --cch;
	DWORD dwLine = 1;
	char * psz = pszStart;
	char * pszFindStart = pfifi->m_pszFindWhat;
	char * pszPara = pszStart;
	unsigned char ch = 0;
	pfifi->m_dwFilesSearched++;
	DWORD dwOldMatches = pfifi->m_dwMatches;
	int nShift;
	DWORD dwUtf8OffsetTotal = 0;
	DWORD dwUtf8OffsetPara = 0;
	char * pszOffset = pszStart;

	__try
	{
		if (dwFileSize >= cch)
		{
			while (psz < pszStop)
			{
				int j = cch;
				ch = psz[j];

				bool fFirstCharMatched = false;
				if (pfifi->m_fCaseSensitive)
					fFirstCharMatched = (ch == pszFindStart[j]);
				else
					fFirstCharMatched = (tolower(ch) == pszFindStart[j]);
				//if ((ch = psz[j]) == pszFindStart[j])
				if (fFirstCharMatched)
				{
					if (pfifi->m_fCaseSensitive)
						while (--j >= 0 && psz[j] == pszFindStart[j]);
					else
						while (--j >= 0 && tolower(psz[j]) == pszFindStart[j]);
					if (j == -1)
					{
						if (g_fg.m_defaultEncoding == kftUnicode8)
						{
							while (pszOffset < psz)
							{
								char offset = krgcbFromUTF8[(UCHAR)*pszOffset++];
								if (offset != 0)
								{
									dwUtf8OffsetTotal += offset;
									dwUtf8OffsetPara += offset;
								}
							}
						}

						// We found a match, so now get the start and end of the
						// current paragraph.
						DWORD ichMatch = psz - pszStart - dwUtf8OffsetTotal;
						DWORD ichInPara = psz - pszPara - dwUtf8OffsetPara;
						while (psz < pszStop && *psz != 10)
							psz++;
						int cchPara = psz - pszPara;
						if (psz > pszStart + 1 && psz[-1] == 13)
							cchPara--;
						if (psz == pszStop)
							cchPara += cch;
						char * pszLine = new char[cchPara + 1];
						if (pszLine)
						{
							memmove(pszLine, pszPara, cchPara);
							pszLine[cchPara] = 0;
						}

						if (AddItem(pszFilename, pszLine, ++pfifi->m_dwMatches, dwLine,
							ichMatch, /*ichInPara, */hwndList))
						{
							if (pfifi->m_dwMatches == 1)
							{
								::InvalidateRect(hwndList, NULL, TRUE);
								::UpdateWindow(hwndList);
							}
						}
						else if (pszLine)
							delete pszLine;

						pszPara = ++psz;
						dwUtf8OffsetPara = 0;
						dwLine++;
						if (!pfifi->m_hwndDlg)
							break;
						continue;
					}
				}
				nShift = pfifi->m_rgnShift[(UCHAR)ch];
				while (nShift--)
				{
					if (*psz++ == 10)
					{
						pszPara = psz;
						dwUtf8OffsetPara = 0;
						dwLine++;
					}
				}
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		::MessageBox(NULL, "Problem", NULL, MB_OK);
	}
	::UnmapViewOfFile(pszStart);
	if (dwOldMatches != pfifi->m_dwMatches)
		pfifi->m_dwFilesMatched++;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindFilesDlg::AddItem(char * pszFilename, char * pszLine, DWORD dwMatches, DWORD dwLine,
	DWORD ichMatch, /*DWORD ichInPara, */HWND hwndList)
{
	FoundFile * pff = new FoundFile();
	if (!pff)
		return false;

	lstrcpy(pff->m_szFilename, pszFilename);
	pff->m_pszFile = strrchr(pff->m_szFilename, '\\');
	AssertPtr(pff->m_pszFile);
	*pff->m_pszFile++ = 0;
	pff->m_dwLine = dwLine;
	pff->m_pszLine = pszLine;
	pff->m_ichMatch = ichMatch;
	//pff->m_ichInPara = ichInPara;

	LVITEM lvi = {LVIF_TEXT | LVIF_PARAM, dwMatches};
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.lParam = (LPARAM)pff;
	ListView_InsertItem(hwndList, &lvi);
	return true;
}


/***********************************************************************************************
	FindInFilesInfo methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
FindInFilesInfo::FindInFilesInfo(CZEditFrame * pzef)
{
	m_pzef = pzef;
	m_hwndDlg = m_hwndList = m_hwndStatus = NULL;
	m_pszFindWhat = m_pszFileTypes = m_pszFolder = NULL;
	m_fWholeWord = m_fCaseSensitive = m_fSubFolders = m_fCanDelete = false;
	m_fSortedAscending = true;
	m_nSortedColumn = -1;
	m_dwMatches = m_dwFilesSearched = m_dwFilesMatched = 0;
	m_cchFindWhat = 0;
	memset(m_rgnShift, 0, sizeof(m_rgnShift));
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
FindInFilesInfo::~FindInFilesInfo()
{
	if (m_pszFindWhat)
	{
		delete m_pszFindWhat;
		m_pszFindWhat = NULL;
	}
	if (m_pszFileTypes)
	{
		delete m_pszFileTypes;
		m_pszFileTypes = NULL;
	}
	if (m_pszFolder)
	{
		delete m_pszFolder;
		m_pszFolder = NULL;
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void FindInFilesInfo::CreateBitMap(void)
{
	if (!m_pszFindWhat)
	{
		// We are just doing a search on the file pattern
		return;
	}
	Assert(m_cchFindWhat);
	for (int in = 0, cn = sizeof(m_rgnShift) / sizeof(int); in < cn; in++)
		m_rgnShift[in] = m_cchFindWhat;
	if (m_fCaseSensitive)
	{
		for (int ich = 0; ich < m_cchFindWhat - 1; ich++)
			m_rgnShift[(UCHAR)m_pszFindWhat[ich]] = m_cchFindWhat - ich - 1;
	}
	else
	{
		int ich = 0;
		for ( ; ich < m_cchFindWhat - 1; ich++)
		{
			m_pszFindWhat[ich] = tolower(m_pszFindWhat[ich]);
			m_rgnShift[(UCHAR)m_pszFindWhat[ich]] = m_cchFindWhat - ich - 1;
			m_rgnShift[(UCHAR)toupper(m_pszFindWhat[ich])] = m_cchFindWhat - ich - 1;
		}
		m_pszFindWhat[ich] = tolower(m_pszFindWhat[ich]);
	}
}