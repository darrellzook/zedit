#ifndef _ZVIEW_H_
#define _ZVIEW_H_

/*----------------------------------------------------------------------------------------------
	CZView declaration.
----------------------------------------------------------------------------------------------*/
class CZView
{
friend CZDoc;

public:
	CZView(CZEditFrame * pzef);
	virtual ~CZView()
	{
	}
	void Initialize();
	void SetMainFrame(CZEditFrame * pzef)
	{
		AssertPtr(pzef);
		m_pzef = pzef;
	}

	virtual UINT CharFromPoint(POINT point, bool fUpdateCaretPos) = 0;
	void ClearDropSource()
	{
		m_fIsDropSource = false;
	}
	virtual bool Create(CZFrame * pzf, HWND hwndParent, UINT nStyle, UINT nExtendedStyle) = 0;
	virtual char * GetCurPosString() = 0;
	CZDoc * GetDoc()
	{
		return m_pzd;
	}
	bool GetOvertype()
	{
		return m_fOvertype;
	}
	virtual SelectionType GetSelection(UINT * pichMin = NULL, UINT * pichStop = NULL) = 0;
	HWND GetHwnd()
	{
		return m_hwnd;
	}
	virtual bool GetWrap() = 0;
	virtual bool IsBinary() = 0;
	bool IsDropSource()
	{
		return m_fIsDropSource;
	}
	virtual void NotifyDelete(UINT ichMin, UINT ichStop, bool fRedraw) = 0;
	virtual void NotifyInsert(UINT ichMin, UINT cch) = 0;
	virtual POINT PointFromChar(UINT ich) = 0;
	virtual bool Print(PrintInfo * ppi) = 0;
	void SetOvertype(bool fOverType)
	{
		m_fOvertype = fOverType;
	}
	virtual bool RecalculateLines(int dyp) = 0;
	virtual bool SetSelection(UINT ichSelMin, UINT ichSelLim, bool fScroll,
		bool fForceRedraw) = 0;
	virtual void SetWrap(bool fWrap) = 0;
	virtual void UpdateScrollBars() = 0;

	// Message handlers
	virtual void OnColumnMarkerDrag() {}
	virtual LRESULT OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) = 0;
	virtual LRESULT OnContextMenu(HWND hwnd, POINT pt) = 0;
	virtual bool OnFontChanged() = 0;
	virtual LRESULT OnHScroll(UINT nSBCode, UINT nPos, HWND hwndScroll) = 0;
	virtual LRESULT OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) = 0;
	virtual LRESULT OnKillFocus(HWND hwndGetFocus) = 0;
	virtual LRESULT OnLButtonDblClk(UINT nFlags, POINT point) = 0;
	virtual LRESULT OnLButtonDown(UINT nFlags, POINT point) = 0;
	virtual LRESULT OnLButtonUp(UINT nFlags, POINT point) = 0;
	virtual LRESULT OnMButtonDown(UINT nFlags, POINT point) = 0;
	virtual LRESULT OnPaint() = 0;
	virtual LRESULT OnSetFocus(HWND hwndLoseFocus) = 0;
	virtual LRESULT OnSize(UINT nType, int cx, int cy) = 0;
	virtual LRESULT OnMouseMove(UINT nFlags, POINT point) = 0;
	virtual LRESULT OnMouseWheel(UINT nFlags, short zDelta, POINT point) = 0;
	virtual LRESULT OnTimer(UINT nIDEvent) = 0;
	virtual LRESULT OnVScroll(UINT nSBCode, UINT nPos, HWND hwndScroll) = 0;

protected:
	HWND m_hwnd;
	UINT m_ichSelMin;
	UINT m_ichSelLim;
	UINT m_ichDragStart;
	RECT m_rcScreen;
	POINT m_ptCaretPos;
	bool m_fSelecting;
	bool m_fOvertype;
	bool m_fIsDropSource;
	MouseTimer m_mt;
	IDropSource * m_pds;
	CZFrame * m_pzf;
	CZDoc * m_pzd;
	CZEditFrame * m_pzef;
	char m_szCurPos[50];
};


/*----------------------------------------------------------------------------------------------
	CFastEdit declaration.
----------------------------------------------------------------------------------------------*/
class CFastEdit : public CZView
{
	friend CZEditFrame;

public:
	CFastEdit(CZEditFrame * pzef);
	~CFastEdit();

	virtual UINT CharFromPoint(POINT point, bool fUpdateCaretPos);
	virtual bool Create(CZFrame * pzf, HWND hwndParent, UINT nStyle, UINT nExtendedStyle);
	virtual char * GetCurPosString();
	virtual SelectionType GetSelection(UINT * pichMin = NULL, UINT * pichStop = NULL);
	bool GetWrap()
	{
		return m_fWrap;
	}
	virtual bool IsBinary()
	{
		return false;
	}
	virtual void NotifyDelete(UINT ichMin, UINT ichStop, bool fRedraw = true);
	virtual void NotifyInsert(UINT ichMin, UINT cch);
	virtual POINT PointFromChar(UINT ich);
	virtual bool Print(PrintInfo * ppi);
	virtual bool RecalculateLines(int dyp);
	virtual bool SetSelection(UINT ichMin, UINT ichStop, bool fScroll, bool fForceRedraw);
	virtual void SetWrap(bool fWrap);
	virtual void UpdateScrollBars();
	virtual void InitializeDC(HDC hdc);

	// Message handlers
	virtual void OnColumnMarkerDrag();
	virtual LRESULT OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual LRESULT OnContextMenu(HWND hwnd, POINT pt);
	virtual bool OnFontChanged();
	virtual LRESULT OnHScroll(UINT nSBCode, UINT nPos, HWND hwndScroll);
	virtual LRESULT OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual LRESULT OnKillFocus(HWND hwndGetFocus);
	virtual LRESULT OnLButtonDblClk(UINT nFlags, POINT point);
	virtual LRESULT OnLButtonDown(UINT nFlags, POINT point);
	virtual LRESULT OnLButtonUp(UINT nFlags, POINT point);
	virtual LRESULT OnMButtonDown(UINT nFlags, POINT point);
	virtual LRESULT OnPaint();
	virtual LRESULT OnSetFocus(HWND hwndLoseFocus);
	virtual LRESULT OnSize(UINT nType, int cx, int cy);
	virtual LRESULT OnMouseMove(UINT nFlags, POINT point);
	virtual LRESULT OnMouseWheel(UINT nFlags, short zDelta, POINT point);
	virtual LRESULT OnTimer(UINT nIDEvent);
	virtual LRESULT OnVScroll(UINT nSBCode, UINT nPos, HWND hwndScroll);

protected:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	int DrawLineA(HDC hdcMem, char * prgchParaStart, char * prgchStart, char * prgchStop,
		int * pdxpLine, int nTabLeft, RenderInfo & ri,
		bool fWrap, bool fDraw, bool fShowColors, bool & fInBlock, bool & fInBlockP);
	int DrawLineW(HDC hdcMem, wchar * prgchParaStart, wchar * prgchStart, wchar * prgchStop,
		int * pdxpLine, int nTabLeft, RenderInfo & ri,
		bool fWrap, bool fDraw, bool fShowColors, bool & fInBlock, bool & fInBlockP);
	bool DrawPara(ParaDrawInfo * ppdi, bool fAnsi, UINT ichStop = (UINT)-1);
	void DrawSelection(HDC hdcScreen, bool fEraseCursors);
	void GetBlockValues(DocPart * pdp, UINT ipr, void * pv, HDC hdc, bool & fInBlock, bool & fInBlockP);
	void Initialize();
	void MyTextOutA(HDC hdcMem, char * prgchParaStart, char * prgchStart, char * prgchStop,
		bool fShowColors, bool fStartOfWord, bool & fInBlock, bool & fInBlockP, RenderInfo & ri);
	void MyTextOutW(HDC hdcMem, wchar * prgchParaStart, wchar * prgchStart, wchar * prgchStop,
		bool fShowColors, bool fStartOfWord, bool & fInBlock, bool & fInBlockP, RenderInfo & ri);
	UINT ShowCharAtLine(UINT ipr, UINT ich, int iLine);
	void ShowText(HDC hdc, RECT & rc);

	LineInfo * m_prgli;
	UINT m_cli;
	UINT m_ichTopLine;
	UINT m_iprTopLine;
	UINT m_ichMaxTopLine;
	POINT m_ptRealPos;
	bool m_fWrap;
	bool m_fOnlyEditing;
	HRGN m_hrgnSel;
};


/*----------------------------------------------------------------------------------------------
	CHexEdit declaration.
----------------------------------------------------------------------------------------------*/
class CHexEdit : public CZView
{
	friend CZEditFrame;

public:
	CHexEdit(CZEditFrame * pzef);
	~CHexEdit();

	virtual UINT CharFromPoint(POINT point, bool fUpdateCaretPos);
	virtual bool Create(CZFrame * pzf, HWND hwndParent, UINT nStyle, UINT nExtendedStyle);
	virtual char * GetCurPosString();
	bool GetWrap()
	{
		return false;
	}
	virtual bool IsBinary()
	{
		return true;
	}
	virtual void NotifyDelete(UINT ichMin, UINT ichStop, bool fRedraw = true);
	virtual void NotifyInsert(UINT ichMin, UINT cch);
	virtual POINT PointFromChar(UINT ich);
	virtual bool Print(PrintInfo * ppi);
	virtual SelectionType GetSelection(UINT * pichSelMin = NULL, UINT * pichSelLim = NULL);
	virtual bool RecalculateLines(int dyp);
	virtual bool SetSelection(UINT ichSelMin, UINT ichSelLim, bool fScroll,
		bool fForceRedraw);
	virtual void SetWrap(bool fWrap)
	{
	}
	virtual void UpdateScrollBars();
	virtual void InitializeDC(HDC hdc);

	// Message handlers
	virtual LRESULT OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual LRESULT OnContextMenu(HWND hwnd, POINT pt);
	virtual bool OnFontChanged();
	virtual LRESULT OnHScroll(UINT nSBCode, UINT nPos, HWND hwndScroll);
	virtual LRESULT OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual LRESULT OnKillFocus(HWND hwndGetFocus);
	virtual LRESULT OnLButtonDblClk(UINT nFlags, POINT point);
	virtual LRESULT OnLButtonDown(UINT nFlags, POINT point);
	virtual LRESULT OnLButtonUp(UINT nFlags, POINT point);
	virtual LRESULT OnMButtonDown(UINT nFlags, POINT point);
	virtual LRESULT OnPaint();
	virtual LRESULT OnSetFocus(HWND hwndLoseFocus);
	virtual LRESULT OnSize(UINT nType, int cx, int cy);
	virtual LRESULT OnMouseMove(UINT nFlags, POINT point);
	virtual LRESULT OnMouseWheel(UINT nFlags, short zDelta, POINT point);
	virtual LRESULT OnTimer(UINT nIDEvent);
	virtual LRESULT OnVScroll(UINT nSBCode, UINT nPos, HWND hwndScroll);

protected:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	UINT CharFromLine(UINT ipr);
	void DrawSelection(HDC hdcScreen);
	inline UINT GetLineCount();
	void Initialize();
	UINT LineFromChar(UINT ich);
	UINT ShowCharAtLine(UINT ipr, UINT ich, int iLine);
	bool DrawLine(ParaDrawInfo * ppdi);
	void ShowText(HDC hdc, RECT & rect);

	UINT m_cli;
	UINT m_ichTopLine;
	UINT m_iprTopLine;
	UINT m_ichMaxTopLine;
	bool m_fOnlyEditing;
	bool m_fCaretInText;
	UINT m_cchPerLine;
	UINT m_nTotalWidth;
	UINT m_nTextOffset; // Column position of the first text column.
	UINT m_nByteOffset; // Column position of the first byte column.
	HRGN m_hrgnSelText;
	HRGN m_hrgnSelByte;
};

#endif // !_ZVIEW_H_