#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::AboutProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			CZEditFrame * pzef = (CZEditFrame *)lParam;
			AssertPtr(pzef);
			pzef->Animate(hwndDlg, ID_HELP_ABOUT, true, true);
			::SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				pzef->Animate(hwndDlg, ID_HELP_ABOUT, true, false);
				::EndDialog (hwndDlg, 0);
			}
			return TRUE;
		}
		break;
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::LineProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char szBuffer[MAX_PATH] = {0};
	static DWORD dwParaCount = 0;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			CZEditFrame * pzef = (CZEditFrame *)lParam;
			AssertPtr(pzef);
			CZDoc * pzd = pzef->GetCurrentDoc();
			AssertPtr(pzd);
			dwParaCount = pzd->GetParaCount();
			::SetDlgItemInt(hwndDlg, IDC_EDIT, pzd->ParaFromChar(-1) + 1, FALSE);
			wsprintf(szBuffer, "%d):", dwParaCount);
			::SetDlgItemText(hwndDlg, IDC_POSSIBLE, szBuffer);
			pzef->Animate(hwndDlg, ksboLineCol, false, true);
			::SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				DWORD dwPara = GetDlgItemInt(hwndDlg, IDC_EDIT, NULL, FALSE);
				if (dwPara > dwParaCount)
					dwPara = dwParaCount;
				if (dwPara > 0)
					dwPara--;
				CZDoc * pzd = pzef->GetCurrentDoc();
				AssertPtr(pzd);
				DWORD dwStartChar = pzd->CharFromPara(dwPara);
				pzd->SetSelection(dwStartChar, dwStartChar, true, true);
				if (pzd->GetFileType() == kftBinary)
					sprintf(szBuffer, "Line %d,  Col 1", dwPara + 1);
				else
					sprintf(szBuffer, "Para %d,  Col 1", dwPara + 1);
				pzef->SetStatusText(ksboLineCol, szBuffer);
				pzef->Animate(hwndDlg, ksboLineCol, false, false);
				::EndDialog(hwndDlg, 1);
				return TRUE;
			}

		case IDCANCEL:
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				pzef->Animate(hwndDlg, ksboLineCol, false, false);
				::EndDialog(hwndDlg, 0);
			}
			return TRUE;
		}
		break;
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::FixLinesProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			::CheckDlgButton(hwndDlg, IDC_CRLF, BST_CHECKED);
			::SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				CZDoc * pzd = pzef->GetCurrentDoc();
				AssertPtr(pzd);
				if (::IsDlgButtonChecked(hwndDlg, IDC_LF) == BST_CHECKED)
					pzd->FixLines(0);
				else if (::IsDlgButtonChecked(hwndDlg, IDC_CR) == BST_CHECKED)
					pzd->FixLines(1);
				else
					pzd->FixLines(2);

				::EndDialog(hwndDlg, 1);
				return TRUE;
			}

		case IDCANCEL:
			::EndDialog(hwndDlg, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::ChangeCaseProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			::CheckDlgButton(hwndDlg, IDC_LOWER, BST_CHECKED);
			::SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			return TRUE;
		}

	case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			HWND hwndCtl = (HWND)lParam;
			if (GetWindowLong(hwndCtl, GWL_ID) == IDC_NOTE)
			{
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, RGB(0x99, 0, 0));
				return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
			}
			return 0;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				CZDoc * pzd = pzef->GetCurrentDoc();
				AssertPtr(pzd);
				UINT ichSelStart, ichSelStop;
				if (pzd->GetSelection(&ichSelStart, &ichSelStop) == kstNone)
				{
					ichSelStart = 0;
					ichSelStop = -1;
				}

				CompareCaseType cct = kcctNone;
				if (::IsDlgButtonChecked(hwndDlg, IDC_LOWER) == BST_CHECKED)
					cct = kcctLower;
				else if (::IsDlgButtonChecked(hwndDlg, IDC_UPPER) == BST_CHECKED)
					cct = kcctUpper;
				else if (::IsDlgButtonChecked(hwndDlg, IDC_TOGGLE) == BST_CHECKED)
					cct = kcctToggle;
				else if (::IsDlgButtonChecked(hwndDlg, IDC_SENTENCE) == BST_CHECKED)
					cct = kcctSentence;
				else if (::IsDlgButtonChecked(hwndDlg, IDC_TITLE) == BST_CHECKED)
					cct = kcctTitle;

				::EndDialog(hwndDlg, 1);

				if (cct != kcctNone)
					pzd->ChangeCase(ichSelStart, ichSelStop, cct);
				return TRUE;
			}

		case IDCANCEL:
			::EndDialog(hwndDlg, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::SortProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool fCaseSensitive = false;
	static bool fDescending = false;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		if (fCaseSensitive)
			::CheckDlgButton(hwndDlg, IDC_CASE, BST_CHECKED);
		if (fDescending)
			::CheckDlgButton(hwndDlg, IDC_DESCENDING, BST_CHECKED);
		::SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				fCaseSensitive = ::IsDlgButtonChecked(hwndDlg, IDC_CASE) != 0;
				fDescending = ::IsDlgButtonChecked(hwndDlg, IDC_DESCENDING) != 0;
				::EndDialog(hwndDlg, 1);

				UINT ichSelStart;
				UINT ichSelStop;
				CZDoc * pzd = pzef->GetCurrentDoc();
				AssertPtr(pzd);
				pzd->GetSelection(&ichSelStart, &ichSelStop);
				bool fResult;
				if (ichSelStart == ichSelStop)
				{
					fResult = pzd->Sort(0, pzd->GetParaCount(), fCaseSensitive, fDescending);
				}
				else
				{
					fResult = pzd->Sort(pzd->ParaFromChar(ichSelStart),
						pzd->ParaFromChar(ichSelStop), fCaseSensitive, fDescending);
				}
				if (!fResult)
				{
					::MessageBox(::GetParent(hwndDlg), "The sort could not be completed.",
						g_pszAppName, MB_OK | MB_ICONWARNING);
				}
				return TRUE;
			}

		case IDCANCEL:
			::EndDialog(hwndDlg, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}


#ifdef SPELLING
/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void DoSpell(HWND hwndDlg, UINT ichMin)
{
	UINT iBad;
	int cch;
	CZDoc * pzd = g_pze->GetCurrentDoc();
	if (pzd->SpellCheck(ichMin, &iBad, &cch))
	{
		if (iBad == -1)
		{
			::EndDialog(hwndDlg, 1);
			::MessageBox(hwndDlg, "The spelling check is complete.", g_pszAppName,
				MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			pzd->SetSelection(iBad, iBad + cch, true, true);
			int cchs;
			int cchsMore;
			char * prgchs;
			char * prgchsMore;
			g_pze->GetSpellCheck()->GetSuggestions(&cchs, &prgchs, &cchsMore, &prgchsMore);
			HWND hwndList = ::GetDlgItem(hwndDlg, ID_SUGGEST);
			::SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);
			::SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
			if (cchs || cchsMore)
			{
				::EnableWindow(hwndList, TRUE);
				::EnableWindow(GetDlgItem(hwndDlg, ID_CHANGE), TRUE);
				for (int i = 0; i < cchs; i++, prgchs += lstrlen(prgchs) + 1)
					::SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)prgchs);
				for (i = 0; i < cchsMore; i++, prgchsMore += lstrlen(prgchsMore) + 1)
					::SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)prgchsMore);
				::SendMessage(hwndList, LB_SETCURSEL, 0, 0);
			}
			else
			{
				::SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)"No suggestions");
				::EnableWindow(hwndList, FALSE);
				::EnableWindow(GetDlgItem(hwndDlg, ID_CHANGE), FALSE);
			}
			::SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
			::ShowWindow(hwndDlg, SW_SHOW);
		}
	}
	else
	{
		::MessageBox(hwndDlg, "The spell check failed.", g_pszAppName, MB_OK | MB_ICONWARNING);
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK SpellProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		DoSpell(hwndDlg, -1);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_CHANGE:
			{
				CZDoc * pzd = g_pze->GetCurrentDoc();
				UINT ichSelStart;
				UINT ichSelStop;
				pzd->GetSelection(&ichSelStart, &ichSelStop);
				char szReplaceWith[512];
				HWND hwndList = ::GetDlgItem(hwndDlg, ID_SUGGEST);
				int iSel = ::SendMessage(hwndList, LB_GETCURSEL, 0, 0);
				if (::SendMessage(hwndList, LB_GETTEXTLEN, iSel, 0) <= 512)
				{
					::SendMessage(hwndList, LB_GETTEXT, iSel, (LPARAM)szReplaceWith);
					UINT cch = pzd->InsertText(ichSelStart, szReplaceWith,
						lstrlen(szReplaceWith), ichSelStop - ichSelStart, true, kurtSpell);
					if (cch > 0)
					{
						DoSpell(hwndDlg, ichSelStart + cch);
						return TRUE;
					}
				}
				::MessageBox(hwndDlg, "The selected text could not be changed.", g_pszAppName,
					MB_OK | MB_ICONWARNING);
				return TRUE;
			}

		case ID_IGNORE:
			{
				UINT ichSelStop;
				CZDoc * pzd = g_pze->GetCurrentDoc();
				pzd->GetSelection(NULL, &ichSelStop);
				DoSpell(hwndDlg, ichSelStop);
			}
			break;

		case IDCANCEL:
			::EndDialog(hwndDlg, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

#endif


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::TabProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		::SetDlgItemInt(hwndDlg, IDC_EDIT, g_fg.m_ri.m_cchSpacesInTab, FALSE);
		::SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				int cchSpacesInTab = ::GetDlgItemInt(hwndDlg, IDC_EDIT, NULL, FALSE);
				if (cchSpacesInTab < 1 || cchSpacesInTab > 30)
				{
					::EnableWindow(::GetParent(hwndDlg), FALSE);
					::MessageBox(hwndDlg, "The tab width must be between 1 and 30.", g_pszAppName,
						MB_ICONINFORMATION);
					::SetFocus(::GetDlgItem(hwndDlg, IDC_EDIT));
					::EnableWindow(::GetParent(hwndDlg), TRUE);
					return FALSE;
				}
				g_fg.m_ri.m_cchSpacesInTab = cchSpacesInTab;
				g_fg.m_ri.m_dxpTab = cchSpacesInTab * g_fg.m_ri.m_tm.tmAveCharWidth;

				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				::InvalidateRect(pzef->GetCurrentDoc()->GetHwnd(), NULL, FALSE);
				::EndDialog(hwndDlg, 1);
			}
			return TRUE;

		case IDCANCEL:
			::EndDialog(hwndDlg, 0);
			return TRUE;
		}
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void SetPrintString(char * pDest, char * pSource)
{
	// Validate the string
	char * pPos = strstr(pSource, "^L");
	char szBuffer[MAX_PATH] = "^L";

	if (pPos == pSource)
		pPos = pSource + 2;
	else
		pPos = pSource;

	while (*pPos)
	{
		if (*pPos == '^')
		{
			switch (tolower(*(pPos + 1)))
			{
			case 'f':
				lstrcat(szBuffer, "^F");
				pPos++;
				break;

			case 'p':
				lstrcat(szBuffer, "^P");
				pPos++;
				break;

			case 'g':
				lstrcat(szBuffer, "^G");
				pPos++;
				break;

			case 't':
				lstrcat(szBuffer, "^T");
				pPos++;
				break;

			case 'd':
				lstrcat(szBuffer, "^D");
				pPos++;
				break;

			case 'l':
				pPos++;
				break;

			case 'c':
				if (!strstr(szBuffer, "^C"))
					lstrcat(szBuffer, "^C");
				pPos++;
				break;

			case 'r':
				if (!strstr(szBuffer, "^R"))
				{
					if (!strstr(szBuffer, "^C"))
						lstrcat(szBuffer, "^C");
					lstrcat(szBuffer, "^R");
				}
				pPos++;
				break;

			default:
				*(szBuffer + lstrlen(szBuffer)) = *pPos;
			}
		}
		else
			*(szBuffer + lstrlen(szBuffer)) = *pPos;
		pPos++;
	}
	if (!strstr(szBuffer, "^C"))
		lstrcat(szBuffer, "^C");
	if (!strstr(szBuffer, "^R"))
		lstrcat(szBuffer, "^R");

	lstrcpy(pDest, szBuffer);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::PageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndText = NULL;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			char szNumber[10] = {0};
			::SetDlgItemText(hwndDlg, IDC_HEADER, g_fg.m_szPrintStrings[0]);
			::SetDlgItemText(hwndDlg, IDC_FOOTER, g_fg.m_szPrintStrings[1]);
			sprintf(szNumber, "%0.2f", g_fg.m_rcPrintMargins.left / 1440.0);
			::SetDlgItemText(hwndDlg, IDC_LEFT, szNumber);
			sprintf(szNumber, "%0.2f", g_fg.m_rcPrintMargins.right / 1440.0);
			::SetDlgItemText(hwndDlg, IDC_RIGHT, szNumber);
			sprintf(szNumber, "%0.2f", g_fg.m_rcPrintMargins.top / 1440.0);
			::SetDlgItemText(hwndDlg, IDC_TOP, szNumber);
			sprintf(szNumber, "%0.2f", g_fg.m_rcPrintMargins.bottom / 1440.0);
			::SetDlgItemText(hwndDlg, IDC_BOTTOM, szNumber);
			sprintf(szNumber, "%0.2f", g_fg.m_ptHeaderFooter.x / 1440.0);
			::SetDlgItemText(hwndDlg, IDC_HEADER_MARGIN, szNumber);
			sprintf(szNumber, "%0.2f", g_fg.m_ptHeaderFooter.y / 1440.0);
			::SetDlgItemText(hwndDlg, IDC_FOOTER_MARGIN, szNumber);

			::SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				int nLength = ::GetWindowTextLength(GetDlgItem(hwndDlg, IDC_HEADER)) + 3;
				::ZeroMemory(g_fg.m_szPrintStrings[0], MAX_PATH);
				char * pText = new char[nLength];
				if (pText)
				{
					::GetDlgItemText(hwndDlg, IDC_HEADER, pText, nLength);
					SetPrintString(g_fg.m_szPrintStrings[0], pText);
					delete pText;
				}

				nLength = ::GetWindowTextLength(GetDlgItem(hwndDlg, IDC_FOOTER)) + 3;
				::ZeroMemory(g_fg.m_szPrintStrings[1], MAX_PATH);
				pText = new char[nLength];
				if (pText)
				{
					::GetDlgItemText(hwndDlg, IDC_FOOTER, pText, nLength);
					SetPrintString(g_fg.m_szPrintStrings[1], pText);
					delete pText;
				}

				char szNumber[10] = {0};
				::GetDlgItemText(hwndDlg, IDC_LEFT, szNumber, 9);
				g_fg.m_rcPrintMargins.left = max((int)(atof(szNumber) * 1440), 360);
				::GetDlgItemText(hwndDlg, IDC_RIGHT, szNumber, 9);
				g_fg.m_rcPrintMargins.right = max((int)(atof(szNumber) * 1440), 360);
				::GetDlgItemText(hwndDlg, IDC_TOP, szNumber, 9);
				g_fg.m_rcPrintMargins.top = max((int)(atof(szNumber) * 1440), 360);
				::GetDlgItemText(hwndDlg, IDC_BOTTOM, szNumber, 9);
				g_fg.m_rcPrintMargins.bottom = max((int)(atof(szNumber) * 1440), 360);
				::GetDlgItemText(hwndDlg, IDC_HEADER_MARGIN, szNumber, 9);
				g_fg.m_ptHeaderFooter.x = max((int)(atof(szNumber) * 1440), 360);
				::GetDlgItemText(hwndDlg, IDC_FOOTER_MARGIN, szNumber, 9);
				g_fg.m_ptHeaderFooter.y = max((int)(atof(szNumber) * 1440), 360);
			}

		case IDCANCEL:
			::EndDialog(hwndDlg, 0);
			return TRUE;

		case IDC_HEADCODES:
		case IDC_FOOTCODES:
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);

				RECT rect;
				::GetWindowRect(GetDlgItem(hwndDlg, LOWORD(wParam)), &rect);
				POINT point = {rect.right, rect.top};
				if (LOWORD(wParam) == IDC_HEADCODES)
					hwndText = GetDlgItem(hwndDlg, IDC_HEADER);
				else
					hwndText = GetDlgItem(hwndDlg, IDC_FOOTER);
				::TrackPopupMenu(pzef->GetContextMenu()->GetSubMenu(kcmPageSetup),
					TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, 0,
					hwndDlg, NULL);
				return TRUE;
			}

		case ID_PAGE_FILENAME: // ^F
			::SendMessage(hwndText, EM_REPLACESEL, TRUE, (long) "^F");
			return TRUE;

		case ID_PAGE_PATHNAME: // ^P
			::SendMessage(hwndText, EM_REPLACESEL, TRUE, (long) "^P");
			return TRUE;

		case ID_PAGE_PAGE: // ^G
			::SendMessage(hwndText, EM_REPLACESEL, TRUE, (long) "^G");
			return TRUE;

		case ID_PAGE_TIME: // ^T
			::SendMessage(hwndText, EM_REPLACESEL, TRUE, (long) "^T");
			return TRUE;

		case ID_PAGE_DATE: // ^D
			::SendMessage(hwndText, EM_REPLACESEL, TRUE, (long) "^D");
			return TRUE;

		case ID_PAGE_LEFT: // ^L
			if (LOWORD(::SendMessage(hwndText, EM_GETSEL, NULL, NULL)) != 0)
				::SendMessage(hwndText, EM_SETSEL, 0, 0);
			::SendMessage(hwndText, EM_REPLACESEL, TRUE, (long) "^L");
			return TRUE;

		case ID_PAGE_CENTER: // ^C
			::SendMessage(hwndText, EM_REPLACESEL, TRUE, (long) "^C");
			return TRUE;

		case ID_PAGE_RIGHT:  // ^R
			::SendMessage(hwndText, EM_REPLACESEL, TRUE, (long) "^R");
			return TRUE;
		}
		break;

	case WM_LBUTTONDOWN:
		{
			POINT point = { LOWORD(lParam), HIWORD(lParam) };
			RECT rectHeader, rectFooter;
			::GetWindowRect(::GetDlgItem(hwndDlg, IDC_STATIC_HEAD), &rectHeader);
			::GetWindowRect(::GetDlgItem(hwndDlg, IDC_STATIC_FOOT), &rectFooter);
			::MapWindowPoints(NULL, hwndDlg, (POINT *)&rectHeader, 2);
			::MapWindowPoints(NULL, hwndDlg, (POINT *)&rectFooter, 2);
			RECT rect;
			BOOL fShowMenu = FALSE;

			if (::PtInRect(&rectHeader, point))
			{
				fShowMenu = TRUE;
				point.x = rectHeader.right;
				point.y = rectHeader.top;
				rect = rectHeader;
				hwndText = ::GetDlgItem(hwndDlg, IDC_HEADER);
			}
			else if (::PtInRect(&rectFooter, point))
			{
				fShowMenu = TRUE;
				point.x = rectFooter.right;
				point.y = rectFooter.top;
				rect = rectFooter;
				hwndText = ::GetDlgItem(hwndDlg, IDC_FOOTER);
			}
			if (fShowMenu)
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);

				HDC hdc = ::GetDC(hwndDlg);
				g_fg.Draw3dRect(hdc, &rect, ::GetSysColor(COLOR_3DSHADOW),
					::GetSysColor(COLOR_3DHILIGHT));
				::ClientToScreen(hwndDlg, &point);
				::TrackPopupMenu(pzef->GetContextMenu()->GetSubMenu(kcmPageSetup),
					TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, 0, hwndDlg, NULL);
				g_fg.Draw3dRect(hdc, &rect, ::GetSysColor(COLOR_3DFACE),
					::GetSysColor(COLOR_3DFACE));
				::ReleaseDC(hwndDlg, hdc);
			}
			return TRUE;
		}

	case WM_MOUSEMOVE:
		{
			POINT point = { LOWORD(lParam), HIWORD(lParam) };
			RECT rectHeader, rectFooter;
			::GetWindowRect(::GetDlgItem(hwndDlg, IDC_STATIC_HEAD), &rectHeader);
			::GetWindowRect(::GetDlgItem(hwndDlg, IDC_STATIC_FOOT), &rectFooter);
			::MapWindowPoints(NULL, hwndDlg, (POINT *)&rectHeader, 2);
			::MapWindowPoints(NULL, hwndDlg, (POINT *)&rectFooter, 2);
			HDC hdc = ::GetDC(hwndDlg);

			if (::PtInRect(&rectHeader, point))
			{
				g_fg.Draw3dRect(hdc, &rectHeader, ::GetSysColor(COLOR_3DHILIGHT),
					::GetSysColor(COLOR_3DSHADOW));
			}
			else
			{
				g_fg.Draw3dRect(hdc, &rectHeader, ::GetSysColor(COLOR_3DFACE),
					::GetSysColor(COLOR_3DFACE));
			}
			if (::PtInRect(&rectFooter, point))
			{
				g_fg.Draw3dRect(hdc, &rectFooter, ::GetSysColor(COLOR_3DHILIGHT),
					::GetSysColor(COLOR_3DSHADOW));
			}
			else
			{
				g_fg.Draw3dRect(hdc, &rectFooter, ::GetSysColor(COLOR_3DFACE),
					::GetSysColor(COLOR_3DFACE));
			}
			::ReleaseDC(hwndDlg, hdc);
			return TRUE;
		}

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rectHeader, rectFooter, rectHeaderSmall, rectFooterSmall;
			::GetWindowRect(::GetDlgItem(hwndDlg, IDC_STATIC_HEAD), &rectHeader);
			::GetWindowRect(::GetDlgItem(hwndDlg, IDC_STATIC_FOOT), &rectFooter);
			::MapWindowPoints(NULL, hwndDlg, (POINT *)&rectHeader, 2);
			::MapWindowPoints(NULL, hwndDlg, (POINT *)&rectFooter, 2);
			rectHeaderSmall = rectHeader;
			rectFooterSmall = rectFooter;
			::InflateRect(&rectHeaderSmall, -4, -4);
			::InflateRect(&rectFooterSmall, -4, -4);
			HDC hdc = ::BeginPaint(hwndDlg, &ps);

			::IntersectClipRect(hdc, rectHeaderSmall.left, rectHeaderSmall.top,
				rectHeaderSmall.right, rectHeaderSmall.bottom);
			::DrawFrameControl(hdc, &rectHeader, DFC_SCROLL, DFCS_SCROLLRIGHT);

			::SelectClipRgn(hdc, NULL);
			::IntersectClipRect(hdc, rectFooterSmall.left, rectFooterSmall.top,
				rectFooterSmall.right, rectFooterSmall.bottom);
			::DrawFrameControl(hdc, &rectFooter, DFC_SCROLL, DFCS_SCROLLRIGHT);
			::EndPaint(hwndDlg, &ps);
			return TRUE;
		}

	case WM_DRAWITEM:
		{
			CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
			AssertPtr(pzef);
			return pzef->GetContextMenu()->OnDrawItem((LPDRAWITEMSTRUCT)lParam);
		}

	case WM_MEASUREITEM:
		{
			CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
			AssertPtr(pzef);
			return pzef->GetContextMenu()->OnMeasureItem((LPMEASUREITEMSTRUCT)lParam);
		}
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::NewToolProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		g_fg.DrawControl(hwnd, g_fg.m_wpOldToolProc);
		return 0;

	case WM_ERASEBKGND:
		return 0;
	}

	return ::CallWindowProc((WNDPROC)g_fg.m_wpOldToolProc, hwnd, uMsg, wParam, lParam);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::NewStatusProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_LBUTTONDBLCLK)
	{
		int rgnWidth[6];
		::SendMessage(hwnd, SB_GETPARTS, sizeof(rgnWidth) / sizeof(int), (long)rgnWidth);
		if (LOWORD(lParam) <= rgnWidth[ksboLineCol]) // Set new line
		{
			::SendMessage(::GetParent(hwnd), WM_COMMAND, MAKELONG(ID_OPTIONS_GOTO, 0), 0);
			return 0;
		}
		else if (LOWORD(lParam) <= rgnWidth[ksboInsert]) // Insert/Overtype
		{
			AssertPtr(g_pzea);
			CZEditFrame * pzef = g_pzea->GetCurrentFrame();
			AssertPtr(pzef);
			CZDoc * pzd = pzef->GetCurrentDoc();
			AssertPtr(pzd);
			pzd->SetOvertype(!pzd->GetOvertype());
			pzef->SetStatusText(ksboInsert, pzd->GetOvertype() ? "OVR" : "INS");
			return 0;
		}
		else if (LOWORD(lParam) <= rgnWidth[ksboFileType]) // File Type
			return 0;
		else if (LOWORD(lParam) <= rgnWidth[ksboWrap]) // Wrap On/Wrap Off
		{
			::SendMessage(::GetParent(hwnd), WM_COMMAND, MAKELONG(ID_OPTIONS_WRAP, 0), 0);
			return 0;
		}
		else if (LOWORD(lParam) <= rgnWidth[ksboFiles]) // File Index/Count
		{
			::SendMessage(::GetParent(hwnd), WM_COMMAND, MAKELONG(ID_WINDOW_MANAGER, 0), 0);
			return 0;
		}
		else // Status Message
			return 0;
	}
	else if (uMsg == WM_CONTEXTMENU)
	{
		POINT pt = { (short int)LOWORD(lParam), (short int)HIWORD(lParam) };
		::MapWindowPoints(NULL, hwnd, &pt, 1);
		int rgnWidth[6];
		::SendMessage(hwnd, SB_GETPARTS, sizeof(rgnWidth) / sizeof(int), (long)rgnWidth);
		if (pt.x <= rgnWidth[ksboLineCol]) // Set new line
			return 0;
		else if (pt.x <= rgnWidth[ksboInsert]) // Insert/Overtype
			return 0;
		else if (pt.x <= rgnWidth[ksboFileType]) // File Type
		{
			AssertPtr(g_pzea);
			CZEditFrame * pzef = g_pzea->GetCurrentFrame();
			AssertPtr(pzef);
			HMENU hSubSubMenu = ::GetSubMenu(pzef->GetContextMenu()->GetSubMenu(kcmFileDrop),
				1); // Open As submenu
			::TrackPopupMenu(hSubSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
				LOWORD(lParam), HIWORD(lParam), 0, pzef->GetHwnd(), NULL);
			return 0;
		}
		else if (pt.x <= rgnWidth[ksboWrap]) // Wrap On/Wrap Off
			return 0;
		else if (pt.x <= rgnWidth[ksboFiles]) // File Index/Count
		{
			AssertPtr(g_pzea);
			CZEditFrame * pzef = g_pzea->GetCurrentFrame();
			AssertPtr(pzef);
			HMENU hSubMenu = pzef->GetMenu()->GetSubMenu(4); // Tabs submenu
			::SendMessage(pzef->GetHwnd(), WM_INITMENUPOPUP, (long)hSubMenu, MAKELONG(4, 0));
			::SetMenuDefaultItem(hSubMenu, ID_WINDOW_MANAGER, FALSE);
			::TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
				LOWORD(lParam), HIWORD(lParam), 0, pzef->GetHwnd(), NULL);
			::SetMenuDefaultItem(hSubMenu, -1, FALSE);
		}
		else // Status Message
			return 0;
	}
	else if (uMsg == WM_ERASEBKGND)
	{
		return 0;
	}
	else if (uMsg == WM_PAINT)
	{
		g_fg.DrawControl(hwnd, (WNDPROC)g_fg.m_wpOldStatusProc);
		return 0;
	}
	return ::CallWindowProc((WNDPROC)g_fg.m_wpOldStatusProc, hwnd, uMsg, wParam, lParam);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::NewFileProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			HWND hwndList = ::GetDlgItem(hwndDlg, IDC_LISTTYPE);
			::SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)"ANSI");
			::SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)"Unicode (UTF-8)");
			::SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)"Unicode (UTF-16)");
			::SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)"Binary");
			CZDoc * pzd = (CZDoc*)lParam;
			if (pzd)
				::SendMessage(hwndList, LB_SETCURSEL, pzd->GetFileType(), 0);
			else
				::SendMessage(hwndList, LB_SETCURSEL, 0, 0);
			return TRUE;
		}

	case WM_COMMAND:
		if (HIWORD(wParam) == LBN_DBLCLK || LOWORD(wParam) == IDOK)
		{
			::EndDialog(hwndDlg, ::SendMessage(GetDlgItem(hwndDlg, IDC_LISTTYPE),
				LB_GETCURSEL, 0, 0));
			return TRUE;
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			::EndDialog(hwndDlg, -1);
			return TRUE;
		}
		break;
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::FileManagerProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			CZEditFrame * pzef = (CZEditFrame *)lParam;
			AssertPtr(pzef);
			::SetWindowLong(hwndDlg, GWL_USERDATA, lParam);

			HWND hwndList = ::GetDlgItem(hwndDlg, IDC_LISTVIEW);
			ListView_SetExtendedListViewStyle(hwndList,
				LVS_EX_FLATSB | LVS_EX_TWOCLICKACTIVATE);
			InitializeFlatSB(hwndList);
			FlatSB_SetScrollProp(hwndList, WSB_PROP_VSTYLE, FSB_FLAT_MODE, TRUE);
			CZDoc * pzd = pzef->GetFirstDoc();
			AssertPtr(pzd);

			// Load size information from the registry
			HKEY hkey;
			RECT rect = { 0 };
			if (::RegOpenKeyEx(HKEY_CURRENT_USER, kpszRootRegKey, 0, KEY_READ, &hkey) ==
				ERROR_SUCCESS)
			{
				DWORD dwSize = sizeof(RECT);
				::RegQueryValueEx(hkey, "Window Manager Position", 0, NULL, (BYTE*)&rect,
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

			// Set the imagelist to the one used by the tab control
			ListView_SetImageList(hwndList, pzef->GetTab()->GetImageList(), LVSIL_SMALL);
			
			// Add each tab/file to the listview
			LVITEM lvi = {LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM};
			char szFilename[MAX_PATH];
			while (pzd)
			{
				strcpy(szFilename, pzd->GetFilename());
				if (pzd->GetModify())
					strcat(szFilename, " *");
				lvi.pszText = szFilename;
				lvi.iImage = pzd->GetImageIndex();
				lvi.lParam = (LPARAM)pzd;
				ListView_InsertItem(hwndList, &lvi);
				lvi.iItem++;
				pzd = pzd->Next();
			}

			// Add one column to the listview
			::GetClientRect(hwndList, &rect);
			LVCOLUMN lvc = { LVCF_TEXT | LVCF_WIDTH };
			lvc.pszText = "";
			lvc.cx = rect.right;
			ListView_InsertColumn(hwndList, 0, &lvc);
			ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);

			::SetFocus(hwndList);
			int iCurSel = pzef->GetTab()->GetCurSel();
			ListView_SetItemState(hwndList, iCurSel, LVIS_SELECTED, LVIS_SELECTED);
			ListView_EnsureVisible(hwndList, iCurSel, FALSE);
			return FALSE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILEMANAGER_ACTIVATE:
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				HWND hwndList = ::GetDlgItem(hwndDlg, IDC_LISTVIEW);
				int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);;
				pzef->SelectFile(iItem);
				return TRUE;
			}

		case ID_FILEMANAGER_SAVE:
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				HWND hwndList = ::GetDlgItem(hwndDlg, IDC_LISTVIEW);
				int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);;
				while (iItem != -1)
				{
					CZDoc * pzd = pzef->m_pzdFirst;
					for (int i = 0; i < iItem; i++, pzd = pzd->Next())
						;
					if (!pzd)
						break;
					if (!pzef->SaveFile(pzd, iItem, false, NULL))
						break;
					iItem = ListView_GetNextItem(hwndList, iItem, LVNI_SELECTED);
				}
				return TRUE;
			}

		case ID_FILEMANAGER_CLOSE:
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				HWND hwndList = ::GetDlgItem(hwndDlg, IDC_LISTVIEW);
				int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);;
				while (iItem != -1)
				{
					if (!pzef->CloseFile(iItem))
						break;
					iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				}
				return TRUE;
			}

		case ID_FILEMANAGER_FIND:
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				HWND hwndList = ::GetDlgItem(hwndDlg, IDC_LISTVIEW);
				int iItem = pzef->GetTab()->GetCurSel();
				ListView_SetItemState(hwndList, -1, 0, LVIS_SELECTED);
				ListView_SetItemState(hwndList, iItem, LVIS_SELECTED | LVIS_FOCUSED,
					LVIS_SELECTED | LVIS_FOCUSED);
				ListView_EnsureVisible(hwndList, iItem, FALSE);
				return TRUE;
			}

		case ID_FILEMANAGER_CLOSEMANAGER:
		case IDCANCEL:
			::DestroyWindow(hwndDlg);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			// Save size information to the registry
			CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
			AssertPtr(pzef);
			HKEY hkey;
			DWORD dwT;
			if (::RegCreateKeyEx(HKEY_CURRENT_USER, kpszRootRegKey, 0, NULL, 0, KEY_WRITE, NULL,
				&hkey, &dwT) == ERROR_SUCCESS)
			{
				RECT rect;
				::GetWindowRect(hwndDlg, &rect);
				::RegSetValueEx(hkey, "Window Manager Position", 0, REG_BINARY, (BYTE *)&rect,
					sizeof(RECT));
				::RegCloseKey(hkey);
			}
			pzef->m_hwndTabMgrDlg = NULL;
			return TRUE;
		}

	case WM_NOTIFY:
		{
			LPNMHDR pnmh = (LPNMHDR)lParam;
			if (pnmh->code == NM_RCLICK)
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
				HMENU hSubMenu = pzef->GetContextMenu()->GetSubMenu(kcmWindow);
				int cSelItems = ListView_GetSelectedCount(pnmh->hwndFrom);
				::EnableMenuItem(hSubMenu, ID_FILEMANAGER_ACTIVATE,
					MF_BYCOMMAND | (cSelItems == 1 ? MF_ENABLED : MF_GRAYED));
				::EnableMenuItem(hSubMenu, ID_FILEMANAGER_SAVE,
					MF_BYCOMMAND | (cSelItems ? MF_ENABLED : MF_GRAYED));
				::EnableMenuItem(hSubMenu, ID_FILEMANAGER_CLOSE,
					MF_BYCOMMAND | (cSelItems ? MF_ENABLED : MF_GRAYED));
				POINT pt = pnmlv->ptAction;
				::MapWindowPoints(hwndDlg, NULL, &pt, 1);
				int nCommand = ::TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON |
					TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, pzef->GetHwnd(), NULL);
				if (nCommand != 0)
					::SendMessage(hwndDlg, WM_COMMAND, nCommand, 0);
			}
			else if (pnmh->code == NM_DBLCLK)
			{
				CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
				AssertPtr(pzef);
				LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
				pzef->SelectFile(pnmlv->iItem);
			}
			break;
		}

	case WM_SIZE:
		{
			RECT rect;
			HWND hwndList = ::GetDlgItem(hwndDlg, IDC_LISTVIEW);
			::GetClientRect(hwndDlg, &rect);
			::MoveWindow(hwndList, 0, 0, rect.right, rect.bottom, TRUE);
			break;
		}
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
char * PrintNumber(UINT cBytes, char szBuffer[20])
{
	UINT cPart = cBytes % 1000;
	UINT cPartT = (cBytes / 1000) % 1000;
	UINT cPartM = (cBytes / 1000000) % 1000;
	UINT cPartB = (cBytes / 1000000000) % 1000;

	if (cBytes < 1000)
		sprintf(szBuffer, "%d", cBytes);
	else if (cBytes < 1000000)
		sprintf(szBuffer, "%d,%03d", cPartT, cPart);
	else if (cBytes < 1000000000)
		sprintf(szBuffer, "%d,%03d,%03d", cPartM, cPartT, cPart);
	else
		sprintf(szBuffer, "%d,%03d,%03d,%03d", cPartB, cPartM, cPartT, cPart);
	return szBuffer;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
char * BytesToString(DWORD dwBytes, char szBuffer[100], char * pszFilename)
{
	char szPart1[20];
	char szPart2[20];

	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	DWORD dwBytesUsed = ((dwBytes / si.dwPageSize) + 1) * si.dwPageSize;
	if (dwBytes < 1024)
	{
		sprintf(szBuffer, "%d bytes (%s bytes), %s bytes used", dwBytes,
			PrintNumber(dwBytes, szPart1), PrintNumber(dwBytesUsed, szPart2));
	}
	else if (dwBytes < 1048576)
	{
		sprintf(szBuffer, "%dKB (%s bytes), %s bytes used", dwBytes / 1024,
			PrintNumber(dwBytes, szPart1), PrintNumber(dwBytesUsed, szPart2));
	}
	else
	{
		sprintf(szBuffer, "%.2fMB (%s bytes), %s bytes used", (double)dwBytes / 1048576,
			PrintNumber(dwBytes, szPart1), PrintNumber(dwBytesUsed, szPart2));
	}
	return szBuffer;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::PropertiesPropPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	default:
		return FALSE;

	case WM_INITDIALOG:
		if (lParam)
		{
			PROPSHEETPAGE * ppsp = (PROPSHEETPAGE *)lParam;
			AssertPtr(ppsp);
			FilePropInfo * pfpi = (FilePropInfo *)(ppsp->lParam);
			AssertPtr(pfpi);
			::SetWindowLong(hwndDlg, GWL_USERDATA, ppsp->lParam);
			char * pszFilename = pfpi->pzd->GetFilename();
			if (!pszFilename)
				return TRUE;

			SHFILEINFO shfi;
			::SHGetFileInfo(pszFilename, 0, &shfi, sizeof(shfi),
				SHGFI_DISPLAYNAME | SHGFI_ICON | SHGFI_LARGEICON | SHGFI_TYPENAME);
			::SetDlgItemText(hwndDlg, IDC_FILENAME, shfi.szDisplayName);
			::SendMessage(::GetDlgItem(hwndDlg, IDC_FILEICON), STM_SETICON,
				(WPARAM)shfi.hIcon, 0);

			char szFilename[MAX_PATH];
			strcpy(szFilename, pszFilename);
			char * pSlash = strrchr(szFilename, '\\');
			if (pSlash == szFilename + 2)
				pSlash++;
			if (pSlash)
				*pSlash = 0;
			::SetDlgItemText(hwndDlg, IDC_LOCATION, szFilename);

			::SetDlgItemText(hwndDlg, IDC_FILETYPE, shfi.szTypeName);

			char szShortFile[MAX_PATH];
			::GetShortPathName(pszFilename, szShortFile, sizeof(szShortFile));
			pSlash = strrchr(szShortFile, '\\');
			::SetDlgItemText(hwndDlg, IDC_MSDOS, pSlash ? pSlash + 1 : szShortFile);

			HANDLE hFile = ::CreateFile(pszFilename, GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
			if (hFile == INVALID_HANDLE_VALUE)
				return TRUE;
			DWORD dwFileSize = ::GetFileSize(hFile, NULL);
			char szBuffer[100];
			::SetDlgItemText(hwndDlg, IDC_FILESIZE,
				BytesToString(dwFileSize, szBuffer, pszFilename));

			const char * kpszDays[] = {
				"Sunday",
				"Monday",
				"Tuesday",
				"Wednesday",
				"Thursday",
				"Friday",
				"Saturday"};
			const char * kpszMonths[] = {"",
				"January",
				"February",
				"March",
				"April",
				"May",
				"June",
				"July",
				"August",
				"September",
				"October",
				"November",
				"December"};
			FILETIME ftCreation, ftLastAccess, ftLastWrite;
			FILETIME ftCreationLcl, ftLastAccessLcl, ftLastWriteLcl;
			::GetFileTime(hFile, &ftCreation, &ftLastAccess, &ftLastWrite);
			SYSTEMTIME st;

			::FileTimeToLocalFileTime(&ftCreation, &ftCreationLcl);
			::FileTimeToSystemTime(&ftCreationLcl, &st);
			sprintf(szBuffer, "%s, %s %02d, %d %d:%02d:%02d ", kpszDays[st.wDayOfWeek],
				kpszMonths[st.wMonth], st.wDay, st.wYear, st.wHour % 12, st.wMinute,
				st.wSecond);
			strcat(szBuffer, st.wHour < 12 ? "AM" : "PM");
			::SetDlgItemText(hwndDlg, IDC_CREATED, szBuffer);

			::FileTimeToLocalFileTime(&ftLastWrite, &ftLastWriteLcl);
			::FileTimeToSystemTime(&ftLastWriteLcl, &st);
			sprintf(szBuffer, "%s, %s %02d, %d %d:%02d:%02d ", kpszDays[st.wDayOfWeek],
				kpszMonths[st.wMonth], st.wDay, st.wYear, st.wHour % 12, st.wMinute,
				st.wSecond);
			strcat(szBuffer, st.wHour < 12 ? "AM" : "PM");
			::SetDlgItemText(hwndDlg, IDC_MODIFIED, szBuffer);

			::FileTimeToLocalFileTime(&ftLastAccess, &ftLastAccessLcl);
			::FileTimeToSystemTime(&ftLastAccessLcl, &st);
			sprintf(szBuffer, "%s, %s %02d, %d", kpszDays[st.wDayOfWeek],
				kpszMonths[st.wMonth], st.wDay, st.wYear);
			::SetDlgItemText(hwndDlg, IDC_ACCESSED, szBuffer);

			::CloseHandle(hFile);

			DWORD dwAttributes = ::GetFileAttributes(pszFilename);
			::CheckDlgButton(hwndDlg, IDC_READONLY,
				(dwAttributes & FILE_ATTRIBUTE_READONLY) ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwndDlg, IDC_HIDDEN,
				(dwAttributes & FILE_ATTRIBUTE_HIDDEN) ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwndDlg, IDC_ARCHIVE,
				(dwAttributes & FILE_ATTRIBUTE_ARCHIVE) ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwndDlg, IDC_SYSTEM,
				(dwAttributes & FILE_ATTRIBUTE_SYSTEM) ? BST_CHECKED : BST_UNCHECKED);
			return TRUE;
		}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_READONLY ||
			LOWORD(wParam) == IDC_HIDDEN ||
			LOWORD(wParam) == IDC_ARCHIVE)
		{
			PropSheet_Changed(::GetParent(hwndDlg), hwndDlg);
			return TRUE;
		}
		break;

	case WM_NOTIFY:
		if (((NMHDR *)lParam)->code == PSN_APPLY)
		{
			char * pszFilename = (char *)::GetWindowLong(hwndDlg, GWL_USERDATA);
			DWORD dwAttributes = 0;
			if (::IsDlgButtonChecked(hwndDlg, IDC_READONLY) & BST_CHECKED)
				dwAttributes |= FILE_ATTRIBUTE_READONLY;
			if (::IsDlgButtonChecked(hwndDlg, IDC_HIDDEN) & BST_CHECKED)
				dwAttributes |= FILE_ATTRIBUTE_HIDDEN;
			if (::IsDlgButtonChecked(hwndDlg, IDC_ARCHIVE) & BST_CHECKED)
				dwAttributes |= FILE_ATTRIBUTE_ARCHIVE;
			::SetFileAttributes(pszFilename, dwAttributes);
			PropSheet_CancelToClose(::GetParent(hwndDlg));
			::SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
			return TRUE;
		}
		break;
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::StatisticsPropPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG && lParam)
	{
		PROPSHEETPAGE * ppsp = (PROPSHEETPAGE *)lParam;
		AssertPtr(ppsp);
		FilePropInfo * pfpi = (FilePropInfo *)(ppsp->lParam);
		AssertPtr(pfpi);
		::SetWindowLong(hwndDlg, GWL_USERDATA, ppsp->lParam);

		CZEditFrame * pzef = pfpi->pzef;
		AssertPtr(pzef);
		CZDoc * pzd = pfpi->pzd;
		AssertPtr(pzd);

		if (strcmp(pzd->GetFilename(), "Untitled") != 0)
		{
			SHFILEINFO shfi;
			::SHGetFileInfo(pzd->GetFilename(), 0, &shfi, sizeof(shfi),
				SHGFI_DISPLAYNAME | SHGFI_ICON | SHGFI_LARGEICON);
			::SetDlgItemText(hwndDlg, IDC_FILENAME, shfi.szDisplayName);
			::SendMessage(::GetDlgItem(hwndDlg, IDC_FILEICON), STM_SETICON,
				(WPARAM)shfi.hIcon, 0);
		}
		else
		{
			::SetDlgItemText(hwndDlg, IDC_FILENAME, "Untitled");
			USHORT nIcon = 0;
			char szIconFile[MAX_PATH] = "Untitled";
			HICON hIcon = ::ExtractAssociatedIcon(0, szIconFile, &nIcon);
			::SendMessage(::GetDlgItem(hwndDlg, IDC_FILEICON), STM_SETICON, (WPARAM)hIcon, 0);
		}

		const char * kpszFileType[] = {
			"ANSI",
			"Unicode (UTF-8)",
			"Unicode (UTF-16)",
			"Binary",
			"Default"};
		::SetDlgItemText(hwndDlg, IDC_FILETYPE, kpszFileType[pzd->GetFileType()]);

		CZDoc * pzdT = pzef->GetFirstDoc();
		int iTab = 1;
		while (pzdT && pzdT != pzd)
		{
			iTab++;
			pzdT = pzdT->Next();
		}
		::SetDlgItemInt(hwndDlg, IDC_TABPOS, iTab, FALSE);

		::SetDlgItemInt(hwndDlg, IDC_PARAS, pzd->GetParaCount(), FALSE);
		::SetDlgItemInt(hwndDlg, IDC_CHARS, pzd->GetCharCount(), FALSE);

		char szBuffer[100];
		char szNumber[20];
		sprintf(szBuffer, "%s ms", PrintNumber(pzd->GetTimeToLoad(), szNumber));
		::SetDlgItemText(hwndDlg, IDC_LOADTIME, szBuffer);
		sprintf(szBuffer, "%s KB", PrintNumber(pzd->GetMemoryUsed() / 1024, szNumber));
		::SetDlgItemText(hwndDlg, IDC_MEMORY, szBuffer);

		return TRUE;
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::ColumnProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool s_fShuttingDown = false;

	switch (uMsg)
	{
	default:
		return false;

	case WM_INITDIALOG:
		{
			HWND hwndList = ::GetDlgItem(hwndDlg, IDC_COLUMN);
			RECT rect;
			::GetClientRect(hwndList, &rect);
			LVCOLUMN lvc = { LVCF_TEXT | LVCF_WIDTH };
			lvc.pszText = "Column";
			lvc.cx = rect.right - rect.left - 40;
			ListView_InsertColumn(hwndList, 0, &lvc);
			lvc.pszText = "Color";
			lvc.cx = 40;
			ListView_InsertColumn(hwndList, 2, &lvc);
			ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FLATSB | LVS_EX_GRIDLINES);

			int cItems = g_fg.m_vpcm.size();
			LVITEM lvi = { LVIF_TEXT };
			char szCol[10];
			lvi.pszText = szCol;
			for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
			{
				lvi.iSubItem = 0;
				_itoa(g_fg.m_vpcm[lvi.iItem]->m_iColumn, szCol, 10);
				ListView_InsertItem(hwndList, &lvi);
			}
			if (cItems == 0)
				::EnableWindow(::GetDlgItem(hwndDlg, ID_DELETE), FALSE);
			return true;
		}

	case WM_NOTIFY:
		{
			NMHDR * pnmh = (NMHDR *)lParam;
			HWND hwndList = GetDlgItem(hwndDlg, IDC_COLUMN);
			switch (pnmh->code)
			{
			case NM_CUSTOMDRAW:
				if (pnmh->hwndFrom == hwndList)
				{
					NMLVCUSTOMDRAW * pnlv = (NMLVCUSTOMDRAW *)lParam;
					if (pnlv->nmcd.dwDrawStage == CDDS_PREPAINT)
					{
						::SetWindowLong(hwndDlg, DWL_MSGRESULT, CDRF_NOTIFYPOSTPAINT);
					}
					else
					{
						int cItems = ListView_GetItemCount(hwndList);
						Assert(cItems == g_fg.m_vpcm.size());
						for (int iItem = 0; iItem < cItems; iItem++)
						{
							::SetBkColor(pnlv->nmcd.hdc, g_fg.m_vpcm[iItem]->m_cr);
							RECT rect;
							ListView_GetSubItemRect(hwndList, iItem, 1, LVIR_BOUNDS, &rect);
							::InflateRect(&rect, -2, -2);
							::ExtTextOut(pnlv->nmcd.hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0,
								NULL);
						}
					}
				}
				return true;

			case NM_CLICK:
				{
					NMLISTVIEW * pnmv = (NMLISTVIEW *)lParam;
					if (pnmv->iSubItem == 1) // Color column.
					{
						int cItems = ListView_GetItemCount(hwndList);
						Assert(cItems == g_fg.m_vpcm.size());
						RECT rect;
						int iItem;
						for (iItem = 0; iItem < cItems; iItem++)
						{
							ListView_GetSubItemRect(hwndList, iItem, 1, LVIR_BOUNDS, &rect);
							if (::PtInRect(&rect, pnmv->ptAction))
								break;
						}
						if (iItem >= cItems)
							return true;

						COLORREF rgcr[16];
						memset(rgcr, 0xFF, sizeof(rgcr));
						CHOOSECOLOR cc = { sizeof(cc), hwndDlg };
						cc.Flags = CC_RGBINIT | CC_ANYCOLOR;
						cc.rgbResult = g_fg.m_vpcm[iItem]->m_cr;
						cc.lpCustColors = rgcr;
						if (::ChooseColor(&cc))
						{
							g_fg.m_vpcm[iItem]->m_cr = cc.rgbResult;
							::InvalidateRect(hwndDlg, NULL, FALSE);
						}
					}
					return true;
				}

			case LVN_ENDLABELEDIT:
				{
					NMLVDISPINFO * pndi = (NMLVDISPINFO *)lParam;
					if (pndi->item.pszText)
					{
						Assert((UINT)pndi->item.iItem < (UINT)g_fg.m_vpcm.size());
						int iColumn = min(1000, atoi(pndi->item.pszText));
						g_fg.m_vpcm[pndi->item.iItem]->m_iColumn = iColumn;
						char szCol[6];
						_itoa(iColumn, szCol, 10);
						LVITEM lvi = { LVIF_TEXT, pndi->item.iItem };
						lvi.pszText = szCol;
						ListView_SetItem(hwndList, &lvi);
					}
					return true;
				}

			case LVN_DELETEITEM:
				if (!s_fShuttingDown)
				{
					NMLISTVIEW * pnmv = (NMLISTVIEW *)lParam;
					Assert((UINT)pnmv->iItem < (UINT)g_fg.m_vpcm.size());
					delete g_fg.m_vpcm[pnmv->iItem];
					g_fg.m_vpcm.erase(g_fg.m_vpcm.begin() + pnmv->iItem);
					return true;
				}
			}
			break;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			s_fShuttingDown = true;
			::EndDialog(hwndDlg, 0);
			return true;

		case ID_ADD:
			{
				CColumnMarker * pcm = new CColumnMarker(0, 0);
				if (!pcm)
					return false;
				HWND hwndList = ::GetDlgItem(hwndDlg, IDC_COLUMN);
				int cItems = ListView_GetItemCount(hwndList);
				Assert(cItems == g_fg.m_vpcm.size());
				g_fg.m_vpcm.push_back(pcm);
				LVITEM lvi = {LVIF_TEXT, cItems};
				lvi.pszText = "0";
				ListView_InsertItem(hwndList, &lvi);
				::EnableWindow(::GetDlgItem(hwndDlg, ID_DELETE), TRUE);
				::SetFocus(hwndList);
				ListView_EditLabel(hwndList, cItems);
				return true;
			}

		case ID_DELETE:
			{
				HWND hwndList = ::GetDlgItem(hwndDlg, IDC_COLUMN);
				int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				if (iItem == -1)
					return false;
				Assert((UINT)iItem < (UINT)g_fg.m_vpcm.size());
				ListView_DeleteItem(hwndList, iItem);
				if (ListView_GetItemCount(hwndList) == 0)
					::EnableWindow(::GetDlgItem(hwndDlg, ID_DELETE), FALSE);
				return true;
			}
		}
		break;
	}
	return false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::EnterCharacterProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	static char rgchText[2048] = { 0 };

	switch (uMsg)
	{
		case WM_INITDIALOG:
			{
				CZEditFrame * pzef = (CZEditFrame *)lParam;
				AssertPtr(pzef);
				if (*rgchText)
					::SetDlgItemText(hwndDlg, IDC_VALUES, rgchText);
				pzef->Animate(hwndDlg, ID_TOOLS_INSERTCHAR, true, true);
				::SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			}
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					{
						CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
						AssertPtr(pzef);
						int cch = min(::GetWindowTextLength(::GetDlgItem(hwndDlg, IDC_VALUES)),
							2047);
						if (cch > 0)
						{
							CZDoc * pzd = pzef->GetCurrentDoc();
							AssertPtr(pzd);
							CZView * pzv = pzd->GetCurrentView();
							AssertPtr(pzv);
							HWND hwndEditor = pzv->GetHwnd();
							::GetDlgItemText(hwndDlg, IDC_VALUES, rgchText, cch + 1);
							char * prgchPos = strtok(rgchText, " ,");
							long lChar = 0;
							while (prgchPos)
							{
								if (sscanf(prgchPos, "%x", &lChar) == 1)
								{
									if (pzd->GetFileType() == kftAnsi)
									{
										if (lChar <= 0xFF)
										{
											pzv->OnChar(lChar, 1, 0);
										}
										else
										{
											::MessageBox(hwndDlg,
												"Unicode characters cannot be inserted into an ANSI file.",
												g_pszAppName, MB_OK | MB_ICONWARNING);
											return true;
										}
									}
									else
									{
										if (lChar <= 0xFFFF)
											pzv->OnChar(lChar, 1, 0);
										else if (lChar > 0x10FFFF)
											pzv->OnChar('?', 1, 0);
										else
										{
											lChar -= 0x10000;
											pzv->OnChar((lChar >> 10) + 0xD800, 1, 0);
											pzv->OnChar((lChar & 0x3FF) + 0xDC00, 1, 0);
										}
									}
								}
								prgchPos = strtok(NULL, " ,");
							}
							// Save the entered text for next time this dialog is opened.
							::GetDlgItemText(hwndDlg, IDC_VALUES, rgchText, cch + 1);
						}
						pzef->Animate(hwndDlg, ID_TOOLS_INSERTCHAR, true, false);
						::EndDialog(hwndDlg, 1);
						return true;
					}

				case IDCANCEL:
					{
						CZEditFrame * pzef = (CZEditFrame *)::GetWindowLong(hwndDlg, GWL_USERDATA);
						AssertPtr(pzef);
						pzef->Animate(hwndDlg, ID_TOOLS_INSERTCHAR, true, false);
						::EndDialog(hwndDlg, 0);
					}
					return true;
			}
	}
	return false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CZEditFrame::ViewCodesProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	// Subtract these numbers from the width/height of the dialog to get
	// the actual width/height of the corresponding control.
	static int dxpFrameExtra = 0;
	static int dypFrameExtra = 0;
	static int dxpValuesExtra = 0;
	static int dypValuesExtra = 0;
	static int dxpCloseExtra = 0;
	static int ypClose = 0;

	switch (iMsg)
	{
	case WM_INITDIALOG:
		{
			RECT rcWindow;
			::GetWindowRect(hwndDlg, &rcWindow);

			// Load initial size from the registry.
			DWORD dwT;
			HKEY hkey;
			if (::RegCreateKeyEx(HKEY_CURRENT_USER, kpszRootRegKey, 0, NULL, 0, KEY_READ,
				NULL, &hkey, &dwT) == ERROR_SUCCESS)
			{
				RECT rcNew;
				DWORD dwSize = sizeof(rcNew);
				if (::RegQueryValueEx(hkey, "View Codes Position", 0, NULL, (BYTE *)&rcNew,
					&dwSize) == ERROR_SUCCESS)
				{
					rcWindow = rcNew;
				}
				::RegCloseKey(hkey);
			}

			// Initialize the static variables that give the offsets of the child controls.
			RECT rcClient;
			::GetClientRect(hwndDlg, &rcClient);
			int dxp = rcClient.right - rcClient.left;
			int dyp = rcClient.bottom - rcClient.top;

			// Get the frame control offsets.
			RECT rc;
			HWND hwnd = ::GetDlgItem(hwndDlg, IDC_FRAME);
			::GetWindowRect(hwnd, &rc);
			dxpFrameExtra = dxp - (rc.right - rc.left);
			dypFrameExtra = dyp - (rc.bottom - rc.top);

			// Get the values control offsets.
			hwnd = ::GetDlgItem(hwndDlg, IDC_VALUES);
			::GetWindowRect(hwnd, &rc);
			dxpValuesExtra = dxp - (rc.right - rc.left);
			dypValuesExtra = dyp - (rc.bottom - rc.top);

			// Fwr the Close button offsets.
			hwnd = ::GetDlgItem(hwndDlg, IDCANCEL);
			::GetClientRect(hwnd, &rc);
			::MapWindowPoints(hwnd, hwndDlg, (POINT *)&rc, 2);
			dxpCloseExtra = rcClient.right - rc.left;
			ypClose = rc.top - rcClient.top;

			::MoveWindow(hwndDlg, rcWindow.left, rcWindow.top,
				rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, false);
		}
		return TRUE;

	case WM_DESTROY:
		{
			// Save size to the registry.
			DWORD dwT;
			HKEY hkey;
			if (::RegCreateKeyEx(HKEY_CURRENT_USER, kpszRootRegKey, 0, NULL, 0, KEY_WRITE,
				NULL, &hkey, &dwT) == ERROR_SUCCESS)
			{
				RECT rc;
				::GetWindowRect(hwndDlg, &rc);
				::RegSetValueEx(hkey, "View Codes Position", 0, REG_BINARY, (BYTE *)&rc,
					sizeof(rc));
				::RegCloseKey(hkey);
			}
		}
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			g_fg.m_hwndViewCodes = NULL;
			::DestroyWindow(hwndDlg);
			return TRUE;
		}
		break;

	case WM_SIZING:
		{
			const int kdxpMin = 410;
			const int kdypMin = 90;

			RECT * prc = (RECT *)lParam;
			if (prc->right - prc->left < kdxpMin)
			{
				if (wParam == WMSZ_BOTTOMRIGHT ||
					wParam == WMSZ_RIGHT || 
					wParam == WMSZ_TOPRIGHT)
				{
					prc->right = prc->left + kdxpMin;
				}
				else
				{
					prc->left = prc->right - kdxpMin;
				}
			}

			if (prc->bottom - prc->top < kdypMin)
			{
				if (wParam == WMSZ_BOTTOM ||
					wParam == WMSZ_BOTTOMLEFT || 
					wParam == WMSZ_BOTTOMRIGHT)
				{
					prc->bottom = prc->top + kdypMin;
				}
				else
				{
					prc->top = prc->bottom - kdypMin;
				}
			}
		}
		break;

	case WM_SIZE:
		{
			int dxp = LOWORD(lParam);
			int dyp = HIWORD(lParam);

			// Resize the frame control.
			HWND hwnd = ::GetDlgItem(hwndDlg, IDC_FRAME);
			::SetWindowPos(hwnd, NULL, 0, 0, dxp - dxpFrameExtra, dyp - dypFrameExtra,
				SWP_NOMOVE | SWP_NOZORDER);

			// Resize the values controls.
			hwnd = ::GetDlgItem(hwndDlg, IDC_VALUES);
			::SetWindowPos(hwnd, NULL, 0, 0, dxp - dxpValuesExtra, dyp - dypValuesExtra,
				SWP_NOMOVE | SWP_NOZORDER);
			::InvalidateRect(hwnd, NULL, false);

			// Move the Close button.
			hwnd = ::GetDlgItem(hwndDlg, IDCANCEL);
			::SetWindowPos(hwnd, NULL, dxp - dxpCloseExtra, ypClose, 0, 0,
				SWP_NOSIZE | SWP_NOZORDER);
		}
		break;
	}
	return FALSE;
}