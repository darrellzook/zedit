#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"

PrintInfo * g_ppi = NULL;


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK AbortProc(HDC hdc, int nCode) 
{
	MSG msg;

	if (!g_ppi || !g_ppi->hdlgAbort)
		return TRUE;

	while (!g_ppi->fCancel && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (!IsDialogMessage(g_ppi->hdlgAbort, &msg))
		{ 
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} 
	} 

	return !g_ppi->fCancel;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
BOOL CALLBACK AbortDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if (!g_ppi)
		return FALSE;

	switch (iMsg)
	{
	case WM_INITDIALOG:
		{
			char * pFile = strrchr(g_ppi->pszFilename, '\\');
			SetDlgItemText(hDlg, IDC_PRINTFILE, pFile ? pFile + 1 : g_ppi->pszFilename);
		}
		return TRUE;

	case WM_COMMAND:
		g_ppi->fCancel = TRUE;
		return TRUE;
	}
	return FALSE;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZEditFrame::Print()
{
	PRINTDLG pDlg = { sizeof(PRINTDLG) };
	pDlg.hwndOwner = m_hwnd;
	pDlg.Flags = PD_NOPAGENUMS | PD_HIDEPRINTTOFILE | PD_RETURNDC;
	UINT ichSelStart, ichSelStop;
	m_pzdCurrent->GetSelection(&ichSelStart, &ichSelStop);
	if (ichSelStart == ichSelStop)
		pDlg.Flags |= PD_NOSELECTION;
	else
		pDlg.Flags |= PD_SELECTION;
	pDlg.nCopies = 1;

	// 	if (pDlg.Flags & PD_SELECTION)

	if (!PrintDlg(&pDlg))
		return;

	PrintInfo pi;
	g_ppi = &pi;
	pi.cCopy = pDlg.nCopies;
	if (pDlg.Flags & PD_SELECTION)
	{
		pi.ichMin = ichSelStart;
		pi.ichStop = ichSelStop;
	}
	else
	{
		pi.ichMin = 0;
		pi.ichStop = m_pzdCurrent->GetCharCount();
	}
	pi.fCollate = pDlg.Flags & PD_COLLATE;
	pi.hdc = pDlg.hDC;
	pi.pszFooter = g_fg.m_szPrintStrings[1];
	pi.pszHeader = g_fg.m_szPrintStrings[0];
	pi.pthf = g_fg.m_ptHeaderFooter;
	pi.rcMargin = g_fg.m_rcPrintMargins;
	pi.iPage = 0;
	pi.fCancel = false;
	pi.pszFilename = m_pzdCurrent->GetFilename();
	pi.hdlgAbort = CreateDialog(g_fg.m_hinst, MAKEINTRESOURCE(IDD_PRINT), m_hwnd, (DLGPROC)AbortDlgProc);

	CWaitCursor wc;

	::EnableWindow(m_hwnd, FALSE);
	::SetAbortProc(pDlg.hDC, (ABORTPROC)AbortProc);

	m_pzdCurrent->GetCurrentView()->Print(&pi);
	g_ppi = NULL;

	::EnableWindow(m_hwnd, TRUE);
	::DestroyWindow(pi.hdlgAbort);
}