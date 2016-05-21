#ifndef _ZDOC_H_
#define _ZDOC_H_

typedef enum {
	kctCompare8 = 0,
	kctCompare16 = 1,
	kctCompare8_16 = 2,
	kctCompare8I = 3,
	kctCompare16I = 4,
	kctCompare8_16I = 5
} CompareType;

struct SortInfo
{
	DocPart * pdp;
	void * pv;
	UINT cch;
};

typedef enum {
	kcctNone = 0,

	kcctLower = 1,
	kcctUpper,
	kcctToggle,
	kcctSentence,
	kcctTitle,
} CompareCaseType;

typedef void (*PFN_COMPARE)(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch);


class CSort
{
public:
	void Sort(SortInfo rgsi[], int nStop, int nDepth, CompareType ct);

	static void Compare8I(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch);
	static void Compare8(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch);
	static void Compare16I(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch);
	static void Compare16(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch);
	static void Compare8_16I(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch);
	static void Compare8_16(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch);

protected:
	static void swap(SortInfo rgsi[], int i, int j);
	static void vecswap(SortInfo rgsi[], int i, int j, int n);
};

const PFN_COMPARE krgpfnCompare[6] = {
	CSort::Compare8,
	CSort::Compare16,
	CSort::Compare8_16,
	CSort::Compare8I,
	CSort::Compare16I,
	CSort::Compare8_16I
};


class CZDoc
{
	friend CZEditFrame;
	friend CZView;
	friend CFastEdit;
	friend CHexEdit;
	friend CFindText;
	friend CFindDlg;

public:
	CZDoc(CZEditFrame * pzef, char * pzsFilename, FileType ft);
	~CZDoc();

	void AttachToMainFrame(CZEditFrame * pzef);
	void AttachFrame(CZFrame * pzf);

	bool CanRedo()
	{
		return !m_stackRedo.empty();
	}
	bool CanPaste();
	bool CanUndo()
	{
		return !m_stackUndo.empty();
	}
	bool ChangeCase(UINT ichMin, UINT ichStop, CompareCaseType cct);
	bool FixLines(int nParaType);
	void * ChangeParaSize(DocPart * pdp, UINT ipr, UINT cchParaNew);
	UINT CharFromPara(UINT ipr);
	bool CheckFileTime();
	void ClearRedo();
	void ClearUndo();
	void CloseFileHandles();
	bool Copy(UINT ichMin, UINT ichStop, bool fMakeUREntry);
	bool Cut(UINT ichMin, UINT ichStop);
	bool DeleteText(UINT ichMin, UINT ichStop, bool fClearRedo, bool fRedraw = true);
	UINT DoReplaceAll(FindItem * pfi);
	bool Find(FindItem * pfi);
	bool GetAccessError()
	{
		return m_fAccessError;
	}
	UINT GetCharCount()
	{
		return m_cch;
	}
	ColorSchemeInfo * GetColorScheme()
	{
		return m_pcsi;
	}
	CZFrame * GetFrame()
	{
		return m_pzf;
	}
	CZView * GetCurrentView()
	{
		return m_pzvCurrent;
	}
	char * GetFilename()
	{
		return m_szFilename;
	}
	int GetImageIndex()
	{
		return m_iImage;
	}
	FileType GetFileType()
	{
		return m_ft;
	}
	void VerifyMemory();
	UINT GetMemoryUsed();
	bool GetModify()
	{
		return m_fModified && !m_fAccessError;
	}
	bool GetOvertype()
	{
		AssertPtr(m_pzvCurrent);
		return m_pzvCurrent->GetOvertype();
	}
	UINT GetParaCount()
	{
		return m_cpr;
	}
	UndoRedoType GetRedoType();
	SelectionType GetSelection(UINT * pichSelStart, UINT * pichSelStop)
	{
		AssertPtr(m_pzvCurrent);
		return m_pzvCurrent->GetSelection(pichSelStart, pichSelStop);
	}
	UINT GetText(UINT ichMin, UINT cch, void ** ppv);
	UINT GetText(UINT ichMin, UINT cch, CZDataObject ** ppzdo);
	UINT GetTimeToLoad()
	{
		return m_cTimeToLoad;
	}
	CUndoRedo * GetUndo();
	UndoRedoType GetUndoType();
	HWND GetHwnd()
	{
		AssertPtr(m_pzvCurrent);
		return m_pzvCurrent->GetHwnd();
	}
	UINT GetWordAtChar(UINT ich, void ** ppv, UINT * pich, UINT * pcch);
	bool GetWrap()
	{
		AssertPtr(m_pzvCurrent);
		return m_pzvCurrent->GetWrap();
	}
	bool IndentParas(UINT iprStart, UINT iprStop, bool fIndent);
	int InsertText(UINT ich, CZDataObject * pzdo, UINT cchReplace, UndoRedoType urt);
	int InsertText(UINT ich, void * pvInsert, UINT cch, UINT cchReplace, bool fAnsi,
		UndoRedoType urt);
	bool IsAnsi(DocPart * pdp, UINT ipr)
	{
		AssertPtr(pdp);
		return m_ft != kftUnicode16 && (m_ft != kftUnicode8 || !IsParaInMem(pdp, ipr));
	}
	void * LastPrintCharInPara(DocPart * pdp, UINT iprInPart)
	{
		AssertPtr(pdp);
		return (char *)pdp->rgpv[iprInPart] +
			(PrintCharsInPara(pdp, iprInPart) << (1 - IsAnsi(pdp, iprInPart)));
	}
	UINT CharOffset(DocPart * pdp, UINT ipr, void * pv)
	{
		AssertPtr(pdp);
		Assert(pv >= pdp->rgpv[ipr]);
		return ((char *)pv - (char *)pdp->rgpv[ipr]) >> (1 - IsAnsi(pdp, ipr));
	}
	CZDoc * Next()
	{
		return m_pzdNext;
	}
	void Next(CZDoc * pzd)
	{
		AssertPtrN(pzd);
		m_pzdNext = pzd;
	}
	bool OnChar(UINT ichSelStart, UINT ichSelStop, UINT ch, bool fOvertype, bool & fOnlyEditing);
	void OnColumnMarkerDrag();
	bool OnFontChanged();
	LRESULT OnSetFocus(HWND hwndLoseFocus)
	{
		AssertPtr(m_pzvCurrent);
		return m_pzvCurrent->OnSetFocus(hwndLoseFocus);
	}
	bool OpenFile(char * pszFilename, CZFrame * pzf, HWND hwndProgress);
	bool OpenFile(char * pszFilename, void * prgchFileContents, DWORD cbSize, CZFrame * pzf);
	bool OpenFile(char * pszFilename, void * prgchFileContents, DWORD cbSize, HWND hwndProgress);
	bool ReOpenFile(FileType ft);
	UINT ParaFromChar(UINT ich);
	bool ParseHFString(char * pszPrint, char szResult[3][100], HeaderFooterInfo * phfi);
	bool Paste(UINT ichSelStart, UINT ichSelStop);
	CZDoc * Prev()
	{
		return m_pzdPrev;
	}
	void Prev(CZDoc * pzd)
	{
		AssertPtrN(pzd);
		m_pzdPrev = pzd;
	}
	void PushUndoEntry(CUndoRedo * pur)
	{
		AssertPtr(pur);
		m_stackUndo.push(pur);
	}
	bool Redo();
	bool SaveFile(char * pszFilename, FileType ft, HWND hwndProgress);
	void SetAccessError(bool fAccessError = true)
	{
		m_fAccessError = fAccessError;
	}
	void SetColorScheme(ColorSchemeInfo * pcsi)
	{
		AssertPtr(pcsi);
		m_pcsi = pcsi;
	}
	void SetCurrentView(CZView * pzv)
	{
		AssertPtr(pzv);
		m_pzvCurrent = pzv;
	}
	void SetImageIndex(int iImage)
	{
		m_iImage = iImage;
	}
	void SetModify(bool fModified)
	{
		m_fModified = fModified;
	}
	void SetOvertype(bool fOvertype)
	{
		AssertPtr(m_pzvCurrent);
		m_pzvCurrent->SetOvertype(fOvertype);
	}
	bool SetSelection(UINT ichSelStart, UINT ichSelStop, bool fScroll, bool fForceRedraw)
	{
		AssertPtr(m_pzvCurrent);
		return m_pzvCurrent->SetSelection(ichSelStart, ichSelStop, fScroll, fForceRedraw);
	}
	//bool SetSelectionFromFileFind(DWORD ichMatch, DWORD dwLine, DWORD ichInPara);
	void SetWrap(bool fWrap)
	{
		AssertPtr(m_pzvCurrent); m_pzvCurrent->SetWrap(fWrap);
	}
	bool Sort(UINT iprStart, UINT iprStop, bool fCaseSensitive, bool fDescending);
#ifdef SPELLING
	bool SpellCheck(UINT ichMin, UINT * pichBad, int * pcch);
#endif // SPELLING
	bool Undo();
	void UpdateModified(int iFile, bool fForceUpdate);

protected:
	static void DeleteMemory(void * pv);
	static int AccessDeniedHandler(int nException, DWORD * pdwError, void * pv);

	DocPart * GetPart(UINT cv, bool fParas, void ** ppv = NULL, UINT * pipr = NULL,
		UINT * pcchBefore = NULL, UINT * pcprBefore = NULL);
	UINT GetTextCore(UINT ichMin, UINT cch, void ** ppv, UINT * pcpr, void ** ppvLastPara);
	int UTF8ToUTF16(char * prgchSrc, int cchSrc, wchar * prgwchDst);
	int UTF16ToUTF8(wchar * prgwchSrc, int cchSrc, char * prgchDst);
	bool WriteANSItoANSIFile(HANDLE hFile, HWND hwndProgress);
	bool WriteANSItoUTF8File(HANDLE hFile, HWND hwndProgress);
	bool WriteANSItoUTF16File(HANDLE hFile, HWND hwndProgress);
	bool WriteUTF8toANSIFile(HANDLE hFile, HWND hwndProgress);
	bool WriteUTF8toUTF8File(HANDLE hFile, HWND hwndProgress);
	bool WriteUTF8toUTF16File(HANDLE hFile, HWND hwndProgress);
	bool WriteUTF16toANSIFile(HANDLE hFile, HWND hwndProgress);
	bool WriteUTF16toUTF8File(HANDLE hFile, HWND hwndProgress);
	bool WriteUTF16toUTF16File(HANDLE hFile, HWND hwndProgress);

	CZEditFrame * m_pzef;
	CZDoc * m_pzdPrev;
	CZDoc * m_pzdNext;
	DocPart * m_pdpFirst;
	CZView * m_pzvCurrent;
	CZFrame * m_pzf;
	CZDataObject * m_pzdo;

	UINT m_cch;
	UINT m_cpr;
	UINT m_cTimeToLoad;
	void * m_pvFile;
	FileType m_ft;
	bool m_fModified;
	bool m_fModifiedOld;
	bool m_fTextFromDisk;
	bool m_fAccessError;
	char m_szFilename[MAX_PATH];
	int m_iImage;
	FILETIME m_ftModified;
	ColorSchemeInfo * m_pcsi;
	std::stack<CUndoRedo *> m_stackUndo;
	std::stack<CUndoRedo *> m_stackRedo;
};

#endif // !_ZDOC_H_