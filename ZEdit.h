#ifndef _ZEDIT_H_
#define _ZEDIT_H_

// Included files
#include <windows.h>
#include <commctrl.h>
#include "afxres.h"
#include "resource.h"
#include "Menu.h"
#ifdef SPELLING
#include "SpellCheck.h"
#endif // SPELLING
#include "Trie.h"

// Constants
#define kcButtons 17
#define kcMaxFiles 20
#define kpszRootRegKey "SOFTWARE\\Darrell Zook\\ZEdit"
#define SUGGESTION_START 50000
#define SUGGESTION_STOP 50100
#define ID_GOTOFILE_START 40000
#define ID_GOTOFILE_STOP 41000

typedef enum {
	ksboLineCol = 0,
	ksboInsert = 1,
	ksboFileType = 2,
	ksboWrap = 3,
	ksboFiles = 4,
	ksboMessage = 5
} StatusBarOffset;

class CFindDlg;
class CFindFilesDlg;
class CZEditFrame;
class CZEditApp;

extern const char * g_pszFileType[];
extern CZEditApp * g_pzea;

LRESULT APIENTRY WndProc(HWND, UINT, WPARAM, LPARAM);


/*----------------------------------------------------------------------------------------------
	CZMenu declaration.
----------------------------------------------------------------------------------------------*/
class CZMenu : public CMenu
{
public:
	CZMenu(CZEditFrame * pzef);
	bool Create(HMENU hMenu, bool fReleaseMenu);
	virtual LRESULT OnDrawItem(LPDRAWITEMSTRUCT lpdis);

	operator HMENU()
		{ return m_hMenu; }

protected:
	CZEditFrame * m_pzef;
};


/*----------------------------------------------------------------------------------------------
	CZTab declaration.
----------------------------------------------------------------------------------------------*/
class CZTab
{
public:
	CZTab();
	~CZTab();

	bool Create(CZEditFrame * pzef);
	HWND GetHwnd()
		{ return m_hwnd; }
	HIMAGELIST GetImageList()
		{ return m_himl; }

	void SetItemImage(int iItem, char * pszFilename, bool fNewWindow);
	void RemoveItem(int iItem);
	int HitTest(POINT pt);
	bool GetItemRect(int iItem, RECT * prc);
	int GetCurSel()
		{ return m_iItemSel; }
	void SetCurSel(int iItem);
	int GetItemCount()
		{ return m_cItems; }
	void DeleteAllItems();
	void GetClientRect(RECT * prc);
	void GetTabRect(RECT * prc);
	void Refresh(bool fUpdateNow = true);

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK CaretWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	BOOL OnLButtonDown(WPARAM wParam, LPARAM lParam);
	BOOL OnMouseMove(WPARAM wParam, LPARAM lParam);
	BOOL OnLButtonUp(WPARAM wParam, LPARAM lParam);
	BOOL OnRButtonUp(WPARAM wParam, LPARAM lParam);
	BOOL OnPaint();
	BOOL OnTimer(WPARAM wParam, LPARAM lParam);
	BOOL OnMouseLeave(WPARAM wParam, LPARAM lParam);
	BOOL OnKeyDown(WPARAM wParam, LPARAM lParam);

protected:
	void FindIcon(int iItem, char * pszFilename, bool fNewWindow);
	void DrawArrow(HDC hdc, bool fLeft, int xp, int yp, bool fFilledIn);

	CZEditFrame * m_pzef;
	HWND m_hwnd;
	HIMAGELIST m_himl;
	int * m_prgdxp; // Width of each tab.
	int m_cItems;
	int m_iItemSel;
	int m_dxpScrollOffset;
	int m_dxpMaxScrollOffset;
	HWND m_hwndToolTip;

	enum
	{
		kdypHeight = 22,
		kdxpMargin = 6,

		kScrollNone = 0,
		kScrollLeft = 1,
		kScrollRight = 2,

		knScrollTimerID = 1,
	};

	// Scroll information.
	int m_nScrollType;

	// Stuff for dragging files around.
	typedef enum
	{
		kfdtNone = 0,
		kfdtMove,
		kfdtNewWindow,
	} FileDropType;
	bool m_fDraggingFile;
	bool m_fOutsideStartTab;
	HCURSOR m_hcurNew;
	HCURSOR m_hcurMove;
	FileDropType m_fdt;
	int m_iFileDrag;
	int m_iFileDrop;
	HWND m_hwndCaret;
	CZEditFrame * m_pzefOver;

	static WNDPROC m_wpOldCaretProc;
};


/*----------------------------------------------------------------------------------------------
	CZEditApp declaration.
----------------------------------------------------------------------------------------------*/
class CZEditApp
{
public:
	CZEditApp();
	~CZEditApp();

	// Main window procedure.
	static int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow);

	bool Create();
	UINT OnIdle(void);
	void SetFont(HFONT hFont, bool fBinary);

	bool AddFrame(CZEditFrame * pzef);
	bool RemoveFrame(CZEditFrame * pzef);
	int GetFrameCount()
		{ return m_czef; }
	CZEditFrame * GetFrame(int izef);
	CZEditFrame * GetCurrentFrame();
	bool SetCurrentFrame(CZEditFrame * pzef);

protected:
	int m_izefCurrent;
	CZEditFrame ** m_prgzef;
	int m_czef;
};


/*----------------------------------------------------------------------------------------------
	CZEditFrame declaration.
----------------------------------------------------------------------------------------------*/
class CZEditFrame
{
public:
	CZEditFrame();
	~CZEditFrame();

	// Dialog window procedures.
	static BOOL CALLBACK PageProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK FindProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK NewTabProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK NewStatusProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK NewToolProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK FileManagerProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK AboutProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK LineProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK FixLinesProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK ChangeCaseProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK SortProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK TabProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK NewFileProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK PropertiesPropPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK StatisticsPropPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK ColumnProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK EnterCharacterProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK ViewCodesProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

#ifdef SPELLING
	bool CanSpellCheck()
		{ return m_fSpellingIsValid; }
	CSpellCheck * GetSpellCheck()
		{ return &m_sc; }
#endif // SPELLING
	CZDoc * GetCurrentDoc()
		{ return m_pzdCurrent; }
	CZDoc * GetFirstDoc()
		{ return m_pzdFirst; }
	CZMenu * GetContextMenu()
		{ return m_pzmContext; }
	CZMenu * GetMenu()
		{ return m_pzm; }
	HWND GetStatusHwnd()
		{ return m_hwndStatus; }
	CZTab * GetTab()
		{ return m_pzt; }
	HWND GetToolbarHwnd()
		{ return m_hwndTool; }
	HWND GetTabMgrHwnd()
		{ return m_hwndTabMgrDlg; }
	HWND GetHwnd()
		{ return m_hwnd; }
	CZDoc * GetDoc(int izd);

	void AddOpenDocumentToMenu(HMENU hMenu);
#ifdef SPELLING
	void AddSuggestionsToMenu(HMENU hMenu);
#endif // SPELLING
	bool UpdateRegistry();

// Create.cpp
public:
	bool CreateStatus();
	bool CreateToolBars();

// File.cpp
public:
	bool CreateNewFile(char * pszFilename, HWND hwndProgress, FileType ft,
		char * prgchFileContents = NULL, DWORD cbSize = 0);
	FileType GetFile(bool fSave, char * pszFilename);
	bool ReadFile(char * pszFilenames, FileType ft);
	bool SaveFile(CZDoc * pzd, int iTab, bool fShowDlg, bool * pfCancel);
	bool SaveAll();
	bool CloseFile(int iFile);
	bool InsertFile(int & iFile, CZDoc * pzdFile, char * pszFilename);
	bool RemoveFile(int iFile, CZDoc * pzdFile, CZDoc * pzdPrev, CZDoc * pzdNext);
	bool CloseAll(bool fExiting, int iExceptFile = -1);
	void SelectFile(int iFile);
	void SetDefaultEncoding(int iEncoding);
	void UpdateTextPos();
protected:
	void SelectLine(int iLine);

// Font.cpp
public:
	bool GetFont(bool fBinary);
	void OnFontChanged();

// ZEdit.cpp
public:
	UINT OnIdle(void);
	bool Initialize();
	void Animate(HWND hwnd, int iButton, bool fToolBar, bool fOpen);
	bool Create(char * pszCmdLine, CZDoc * pzdExisting, bool fShowWindow);
	void SetStatusText(int ksbo, const char * pszText);
	static char * InsertComma(int n);
protected:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

// Print.cpp
	void Print();

// Virtual.cpp
public:
	void OnColumnMarkerDrag();
	long OnCommand(WPARAM wParam, LPARAM lParam);
	long OnDropFiles(HDROP hDropInfo, BOOL fContext);
	long OnFileProperties(CZDoc * pzd);
	long OnInitMenuPopup(HMENU hMenu, UINT nIndex, BOOL fSysMenu);
	long OnNotify(int nID, NMHDR * pnmh);
	long OnSize(UINT nType, int cx, int cy);


// Member variables
public:
	CFindDlg * m_pfd;
	CFindFilesDlg * m_pffd;
protected:
	HWND m_hwnd;
	CZTab * m_pzt;
	HWND m_hwndRebar;
	HWND m_hwndTool;
	HWND m_hwndStatus;
	HWND m_hwndTabMgrDlg;
	RECT m_rcPosition;
	char m_szFileList[kcMaxFiles][MAX_PATH];
	char m_szContextFile[MAX_PATH];
	bool m_fWrap;
	CZDoc * m_pzdFirst;
	CZDoc * m_pzdCurrent;
	CZMenu * m_pzm;
	CZMenu * m_pzmContext;
#ifdef SPELLING
	CSpellCheck m_sc;
	bool m_fSpellingIsValid;
#endif // SPELLING
	UINT m_iWordStart;
	UINT m_cchWord;
	IDropTarget * m_pdt;
};

#endif // !_ZEDIT_H_