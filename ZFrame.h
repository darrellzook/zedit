#ifndef _ZFRAME_H_
#define _ZFRAME_H_

class CZFrame
{
public:
	CZFrame();
	~CZFrame();

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ScrollWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	bool Create(CZEditFrame * pzef, CZDoc * pzd);
	bool AttachToMainFrame(CZEditFrame * pzef);

	CZView * GetCurrentView()
		{ return m_pzvCurrent; }
	CZDoc * GetDoc()
		{ return m_pzd; }
	HWND GetHwnd()
		{ return m_hwnd; }
	void SetCurrentView(CZView * pzv);
	bool ToggleCurrentView();
	bool IsBinaryMode()
		{ return m_fBinary; }
	bool IsSplit()
		{ return m_fIsSplit; }
	bool SplitWindow(bool fHorizontal, bool fKeepTopLeft = true);

	int SetScrollInfo(CZView * pzv, int nBar, SCROLLINFO * psi);
	BOOL GetScrollInfo(CZView * pzv, int nBar, SCROLLINFO * psi);

	void NotifyDelete(UINT ichMin, UINT ichStop, bool fRedraw = true);
	void NotifyInsert(UINT ichMin, UINT cch);

	void OnColumnMarkerDrag();
	bool OnFontChanged();
	LRESULT OnSize(UINT nType, int cx, int cy);
	LRESULT OnMouseMove(UINT nFlags, POINT pt);
	LRESULT OnLButtonDown(UINT nFlags, POINT pt);
	LRESULT OnLButtonUp(UINT nFlags, POINT pt);
	LRESULT OnHScroll(UINT nSBCode, UINT nPos, HWND hwndScroll);
	LRESULT OnVScroll(UINT nSBCode, UINT nPos, HWND hwndScroll);
	LRESULT OnPaint();

protected:
	HWND m_hwnd;
	HWND m_hwndParent;
	CZEditFrame * m_pzef;
	CZDoc * m_pzd;
	CZView * m_pzvCurrent;
	bool m_fBinary;
	CZView * m_rgpzv[2];
	HWND m_rghwndHScroll[2];
	HWND m_rghwndVScroll[2];
	RECT m_rgrcClient[2];
	bool m_fResizing;
	int m_nTopHalfPercent;
	bool m_fIsSplit;
	bool m_fHorizontalSplit;

	enum
	{
		kdzpSeparator = 5,
		kdypSplitter = 7,
	};
};

#endif // !_ZFRAME_H_