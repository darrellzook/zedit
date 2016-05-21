#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"
#include "SyntaxDlg.h"

/***********************************************************************************************
	CSyntaxDlg methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CSyntaxDlg::CSyntaxDlg(CZEditFrame * pzef, CColorSchemes * pcs)
{
	m_pzef = pzef;
	m_pcs = pcs;
	m_hwndDlg = NULL;
	m_hwndTab = NULL;
	m_hwndList = NULL;
	m_hBrush = NULL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CSyntaxDlg::~CSyntaxDlg()
{
	if (m_hBrush)
		DeleteObject(m_hBrush);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CSyntaxDlg::DoModal()
{
	return DialogBoxParam(g_fg.m_hinst, MAKEINTRESOURCE(IDD_SYNTAXCOLOR), m_pzef->GetHwnd(),
		SyntaxDlgProc, (LPARAM)this);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK CSyntaxDlg::SyntaxDlgProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	CSyntaxDlg * psd = (CSyntaxDlg *)GetWindowLong(hwndDlg, GWL_USERDATA);

	switch (iMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;
		}
		switch (HIWORD(wParam))
		{
		case CBN_SELCHANGE: // This also doubles as STN_DBLCLK, which I don't think ever gets called
		case STN_CLICKED:
			{
				HWND hwnd = (HWND)LOWORD(lParam);
				if (hwnd == GetDlgItem(psd->m_hwndDlg, IDC_GROUP))
				{
					// The combo box selection was changed.
					int iCurSel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
					psd->SelectGroup(-1, iCurSel);
					return TRUE;
				}
				COLORREF cr;
				if (hwnd == GetDlgItem(psd->m_hwndDlg, IDC_GROUP_COLOR))
					cr = psd->m_crGroup;
				else if (hwnd == GetDlgItem(psd->m_hwndDlg, IDC_BLOCK_COLOR))
					cr = psd->m_crBlock;
				else if (hwnd == GetDlgItem(psd->m_hwndDlg, IDC_DEFAULT_COLOR))
					cr = psd->m_crDefault;
				else
					return TRUE;
				// One of the color static boxes was clicked.
				COLORREF rgcr[16];
				memset(rgcr, 0xFF, sizeof(rgcr));
				CHOOSECOLOR cc = {sizeof(cc), psd->m_hwndDlg};
				cc.Flags = CC_RGBINIT | CC_ANYCOLOR;
				cc.rgbResult = cr;
				cc.lpCustColors = rgcr;
				if (ChooseColor(&cc))
				{
					if (hwnd == GetDlgItem(psd->m_hwndDlg, IDC_GROUP_COLOR))
						psd->m_crGroup = cc.rgbResult;
					else if (hwnd == GetDlgItem(psd->m_hwndDlg, IDC_BLOCK_COLOR))
						psd->m_crBlock = cc.rgbResult;
					else
						psd->m_crDefault = cc.rgbResult;
					InvalidateRect(psd->m_hwndDlg, NULL, FALSE);
				}
				return TRUE;
			}
		}
		return FALSE;

	case WM_INITDIALOG:
		{
			SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			psd = (CSyntaxDlg *)lParam;
			psd->m_hwndDlg = hwndDlg;

			// Add tabs for each of the color schemes
			int ccsi = psd->m_pcs->GetColorSchemeCount();
			psd->m_hwndTab = GetDlgItem(hwndDlg, IDC_SCHEMES);
			TCITEM tci = {TCIF_TEXT};
			ColorSchemeInfo * pcsi;
			for (int icsi = 0; icsi < ccsi; icsi++)
			{
				pcsi = psd->m_pcs->GetColorScheme(icsi);
				if (icsi < ccsi - 1)
				{
					pcsi->EnsureLoaded();
					tci.pszText = pcsi->m_pszDesc;
				}
				else
				{
					tci.pszText = "Default";
				}
				TabCtrl_InsertItem(psd->m_hwndTab, icsi, &tci);
			}

			// Add columns for the paragraph comment list view control
			psd->m_hwndList = GetDlgItem(hwndDlg, IDC_PARA);
			LVCOLUMN lvc = {LVCF_TEXT | LVCF_WIDTH};
			lvc.pszText = "Start";
			lvc.cx = 65;
			ListView_InsertColumn(psd->m_hwndList, 0, &lvc);
			lvc.pszText = "Stop";
			lvc.cx = 65;
			ListView_InsertColumn(psd->m_hwndList, 1, &lvc);
			lvc.pszText = "Color";
			lvc.cx = 40;
			ListView_InsertColumn(psd->m_hwndList, 2, &lvc);
			ListView_SetExtendedListViewStyle(psd->m_hwndList, LVS_EX_FLATSB | LVS_EX_GRIDLINES);
			psd->OnSelChange(NULL);
			return TRUE;
		}

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case TCN_SELCHANGE:
			return psd->OnSelChange((LPNMHDR)lParam);

		case TCN_SELCHANGING:
			return psd->OnSelChanging((LPNMHDR)lParam);

		case NM_CUSTOMDRAW:
			if (((LPNMHDR)lParam)->hwndFrom == psd->m_hwndList)
			{
				LPNMLVCUSTOMDRAW pnlv = (LPNMLVCUSTOMDRAW)lParam;
				if (pnlv->nmcd.dwDrawStage == CDDS_PREPAINT)
				{
					SetWindowLong(hwndDlg, DWL_MSGRESULT, CDRF_NOTIFYPOSTPAINT);
				}
				else
				{
					int cItems = ListView_GetItemCount(psd->m_hwndList);
					LVITEM lvi = {LVIF_PARAM};
					for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
					{
						ListView_GetItem(psd->m_hwndList, &lvi);
						if (lvi.lParam)
							::SetBkColor(pnlv->nmcd.hdc, ((BlockPara *)lvi.lParam)->m_cr);
						else
							::SetBkColor(pnlv->nmcd.hdc, 0xFFFFFF);
						RECT rect;
						ListView_GetSubItemRect(psd->m_hwndList, lvi.iItem, 2, LVIR_BOUNDS, &rect);
						::InflateRect(&rect, -2, -2);
						::ExtTextOut(pnlv->nmcd.hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
					}
				}
			}
			return TRUE;

		case NM_CLICK:
			{
				LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
				if (pnmv->iSubItem == 2)
				{
					int cItems = ListView_GetItemCount(psd->m_hwndList);
					LVITEM lvi = {LVIF_PARAM};
					RECT rect;
					for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
					{
						ListView_GetSubItemRect(psd->m_hwndList, lvi.iItem, 2, LVIR_BOUNDS, &rect);
						if (PtInRect(&rect, pnmv->ptAction))
							break;
					}
					if (lvi.iItem >= cItems)
						return TRUE;
					ListView_GetItem(psd->m_hwndList, &lvi);
					if (lvi.lParam)
					{
						BlockPara * pbp = (BlockPara *)lvi.lParam;
						COLORREF rgcr[16];
						memset(rgcr, 0xFF, sizeof(rgcr));
						CHOOSECOLOR cc = {sizeof(cc), psd->m_hwndDlg};
						cc.Flags = CC_RGBINIT | CC_ANYCOLOR;
						cc.rgbResult = pbp->m_cr;
						cc.lpCustColors = rgcr;
						if (ChooseColor(&cc))
						{
							pbp->m_cr = cc.rgbResult;
							InvalidateRect(psd->m_hwndDlg, NULL, FALSE);
						}
					}
					return TRUE;
				}
			}
		}
		break;

	case WM_CTLCOLORSTATIC:
		return psd->OnCtlColorStatic((HDC)wParam, (HWND)lParam);
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CSyntaxDlg::OnCtlColorStatic(HDC hdc, HWND hwnd)
{
	COLORREF cr;
	if (hwnd == GetDlgItem(m_hwndDlg, IDC_GROUP_COLOR))
		cr = m_crGroup;
	else if (hwnd == GetDlgItem(m_hwndDlg, IDC_BLOCK_COLOR))
		cr = m_crBlock;
	else if (hwnd == GetDlgItem(m_hwndDlg, IDC_DEFAULT_COLOR))
		cr = m_crDefault;
	else
		return FALSE;
	if (m_hBrush)
		DeleteObject(m_hBrush);
	m_hBrush = CreateSolidBrush(cr);
	return (BOOL)m_hBrush;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CSyntaxDlg::SelectGroup(int iTab, int iGroup)
{
	HWND hwndCombo = GetDlgItem(m_hwndDlg, IDC_GROUP);
	SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
	if (iTab == -1)
		iTab = TabCtrl_GetCurSel(m_hwndTab);
	ColorSchemeInfo * pcsi = m_pcs->GetColorScheme(iTab);
	AssertPtr(pcsi);
	int cki = pcsi->m_vpki.size();
	if ((UINT)iGroup >= (UINT)cki)
	{
		SetDlgItemText(m_hwndDlg, IDC_KEYWORD, NULL);
		return;
	}

	KeywordInfo * pki = pcsi->m_vpki[iGroup];
	m_crGroup = pki->m_cr;
	SetDlgItemText(m_hwndDlg, IDC_KEYWORD, pki->m_pszWords);

	// Add the groups to the combo box
	for (int iki = 0; iki < cki; iki++)
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)pcsi->m_vpki[iki]->m_szDesc);
	SendMessage(hwndCombo, CB_SETCURSEL, iGroup, 0);
	InvalidateRect(m_hwndDlg, NULL, FALSE);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CSyntaxDlg::UpdateParaComments(int iTab)
{
	ColorSchemeInfo * pcsi = m_pcs->GetColorScheme(iTab);
	AssertPtr(pcsi);

	ListView_DeleteAllItems(m_hwndList);
	LVITEM lvi = {0};
	int cbp = pcsi->m_vpbp.size();
	for (int ibp = 0; ibp < cbp; ibp++)
	{
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.iItem = ibp;
		lvi.iSubItem = 0;
		lvi.lParam = (LPARAM)pcsi->m_vpbp[ibp];
		lvi.pszText = pcsi->m_vpbp[ibp]->m_szStart;
		ListView_InsertItem(m_hwndList, &lvi);

		lvi.mask = LVIF_TEXT;
		lvi.iSubItem = 1;
		lvi.lParam = 0;
		lvi.pszText = pcsi->m_vpbp[ibp]->m_szStop;
		ListView_SetItem(m_hwndList, &lvi);
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CSyntaxDlg::OnSelChange(LPNMHDR lpnmh)
{
	int iTabNew = TabCtrl_GetCurSel(m_hwndTab);
	SelectGroup(iTabNew, 0);
	UpdateParaComments(iTabNew);

	ColorSchemeInfo * pcsi = m_pcs->GetColorScheme(iTabNew);
	AssertPtr(pcsi);
	SetDlgItemText(m_hwndDlg, IDC_EXT, pcsi->m_pszExt);

	SetDlgItemText(m_hwndDlg, IDC_BLOCK_START, pcsi->m_szBlockStart);
	SetDlgItemText(m_hwndDlg, IDC_BLOCK_STOP, pcsi->m_szBlockStop);

	CheckDlgButton(m_hwndDlg, IDC_CASE, pcsi->m_fCaseMatters ? BST_CHECKED : BST_UNCHECKED);
	m_crBlock = pcsi->m_crBlock;
	m_crDefault = pcsi->m_crDefault;
	return TRUE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CSyntaxDlg::OnSelChanging(LPNMHDR lpnmh)
{
	return TRUE;
}