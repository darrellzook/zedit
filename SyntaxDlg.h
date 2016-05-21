// Declaration of the CFindDlg class

#ifndef _SYNTAXDLG_H_
#define _SYNTAXDLG_H_

#include "Common.h"


class CSyntaxDlg
{
public:
	CSyntaxDlg(CZEditFrame * pzef, CColorSchemes * pcs);
	~CSyntaxDlg();

	int DoModal();
	HWND GetHwnd() { return m_hwndDlg; }

	virtual BOOL OnCtlColorStatic(HDC hdc, HWND hwnd);
	virtual BOOL OnSelChange(LPNMHDR lpnmh);
	virtual BOOL OnSelChanging(LPNMHDR lpnmh);

protected:
	CZEditFrame * m_pzef;
	CColorSchemes * m_pcs;
	HWND m_hwndDlg;
	HWND m_hwndList;
	HWND m_hwndTab;
	COLORREF m_crGroup;
	COLORREF m_crBlock;
	COLORREF m_crDefault;
	HBRUSH m_hBrush;

	static BOOL CALLBACK SyntaxDlgProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	void SelectGroup(int iTab, int iGroup);
	void UpdateParaComments(int iTab);
};

#endif // !_SYNTAXDLG_H_