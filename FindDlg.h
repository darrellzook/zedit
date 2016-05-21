// Declaration of the CFindDlg class

#ifndef _FINDDLG_H_
#define _FINDDLG_H_

typedef enum {
	kfdDown,
	kfdUp,
	kfdAll
} FindDirection;

typedef enum {
	kfsSelection,
	kfsFirstHalf,
	kfsSecondHalf,
	kfsFinished
} FindStages;

bool AddStringToCombo(HWND hwndDlg, int nID, char * pszText);


/*----------------------------------------------------------------------------------------------
	CFindDlg declaration.
----------------------------------------------------------------------------------------------*/
class CFindDlg
{
#ifdef SPELLING
friend BOOL CALLBACK SpellProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
#endif // SPELLING

public:
	CFindDlg(CZEditFrame * pzef);
	~CFindDlg();

	CZEditFrame * m_pzef;

	bool ShowDialog(bool fReplace = false);
	bool EnterSpecialText(int nID);
	bool IsBusy()
	{
		return m_fBusy;
	}
	bool CanFind()
	{
		return ::IsWindowEnabled(::GetDlgItem(m_hwndDlg, IDC_FINDNEXT)) != 0;
	}
	bool CanReplace()
	{
		return m_fReplace && ::IsWindowEnabled(::GetDlgItem(m_hwndDlg, IDC_REPLACE));
	}
	HWND GetHwnd()
	{
		return m_hwndDlg;
	}
	void ResetFind()
	{
		m_fFirstFind = true;
	}

// Message functions
	bool OnEnterIdle(void);
	bool OnMouseMove(UINT nFlags, POINT point);
	bool OnLButtonDown(UINT nFlags, POINT point);
	bool OnPaint(void);
	bool OnFind(bool fNext = true, bool fReplace = false);
	bool OnReplace(void);
	bool OnReplaceAll(void);

protected:
	HWND m_hwndDlg;
	HMENU m_hmenuPopup;
	HWND m_hwndTextBox;
	bool m_fReplace;
	bool m_fBusy;
	bool m_fVisible;
	bool m_fFirstFind;
	FindStages m_fs;
	FindItem m_fi;
	UINT m_ichInitial;
	WNDPROC m_wpOldComboEditProc;

	void SubclassCombo(int nComboID);
	bool ConvertString(int nID, char * pszText, wchar ** ppwszConverted, UINT * pcch);
	bool UpdateFindInfo();
	UINT DoReplaceAll(CFastEdit * pfe, int cchFind, int cchReplace);

	static BOOL CALLBACK FindDlgProc(HWND hwndDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK NewComboEditWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
};


/*----------------------------------------------------------------------------------------------
	CriticalSection declaration/definition.
----------------------------------------------------------------------------------------------*/
class CriticalSection
{
public:
	CriticalSection()
	{
		InitializeCriticalSection(&m_cs);
	}
	~CriticalSection()
	{
		DeleteCriticalSection(&m_cs);
	}
	void Enter()
	{
		EnterCriticalSection(&m_cs);
	}
	void Leave()
	{
		LeaveCriticalSection(&m_cs);
	}

private:
	CRITICAL_SECTION m_cs;
};


/*----------------------------------------------------------------------------------------------
	FindInFilesInfo declaration.
----------------------------------------------------------------------------------------------*/
class FindInFilesInfo
{
public:
	FindInFilesInfo(CZEditFrame * pzef);
	~FindInFilesInfo();

	void CreateBitMap();

	char * m_pszFindWhat;
	int m_cchFindWhat;
	char * m_pszFileTypes;
	char * m_pszFolder;
	bool m_fWholeWord;
	bool m_fCaseSensitive;
	bool m_fSubFolders;
	CZEditFrame * m_pzef;
	HWND m_hwndDlg;
	HWND m_hwndList;
	HWND m_hwndStatus;
	bool m_fSortedAscending;
	int m_nSortedColumn;
	DWORD m_dwMatches;
	DWORD m_dwFilesSearched;
	DWORD m_dwFilesMatched;

	CriticalSection m_cs;
	bool m_fCanDelete;
	int m_rgnShift[256];
};


/*----------------------------------------------------------------------------------------------
	CFindFilesDlg declaration.
----------------------------------------------------------------------------------------------*/
class CFindFilesDlg
{
public:
	CFindFilesDlg(CZEditFrame * pzef);
	bool ShowDialog();
	HWND GetHwnd()
	{
		return m_hwndDlg;
	}

protected:
	CZEditFrame * m_pzef;
	HWND m_hwndDlg;
	bool m_fVisible;

	bool StartFind(HWND hwndDlg);

	static void StartFind(void * pv);
	static void GetFilesInDir(FindInFilesInfo * pfifi, bool fFindAll, HWND hwndList,
		char * pszFolder);
	static void SearchFile(FindInFilesInfo * pfifi, HWND hwndList, char * pszFilename,
		DWORD dwFileSize);
	static BOOL CALLBACK FindInFilesProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
		LPARAM lParam);
	static BOOL CALLBACK FindResultsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
		LPARAM lParam);
	static int CALLBACK PfnCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	static bool AddItem(char * pszFilename, char * pszLine, DWORD dwMatches,
		DWORD dwLine, DWORD ichMatch, /*DWORD ichInPara, */HWND hwndList);

	static void AddString(char *& prgch, char *& pchCur, char *& pchMax,
		int &cchSize, char * psz, char * pszExtra);
};


/*----------------------------------------------------------------------------------------------
	FoundFile declaration/definition.
----------------------------------------------------------------------------------------------*/
class FoundFile
{
public:
	FoundFile()
	{
		memset(m_szFilename, 0, sizeof(m_szFilename));
		m_pszFile = m_pszLine = NULL;
		m_dwLine = 0;
	}
	~FoundFile()
	{
		if (m_pszLine)
			delete m_pszLine;
	}

	char m_szFilename[MAX_PATH];
	char * m_pszFile;
	DWORD m_dwLine;
	char * m_pszLine;
	DWORD m_ichMatch;
	DWORD m_ichInPara;
};

#endif // !_FINDDLG_H_