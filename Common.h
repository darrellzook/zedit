#ifndef _COMMON_H_
#define _COMMON_H_
#pragma once

// Defined elsewhere
class CZDataObject;
class CZDoc;
class CZEditApp;
class CZEditFrame;
class CZFrame;
class CZView;
class CTrieLevel;
class CTrieElement;
class CFastEdit;
class CHexEdit;

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stack>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <assert.h>
#include <zmouse.h>
#include <atlbase.h>

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x00080000
#endif

struct BlockPara;
struct ColorSchemeInfo;
struct DocPart;
struct FindItem;
struct HeaderFooterInfo;
struct KeywordInfo;
struct ParaDrawInfo;
struct PrintInfo;

class CUndoRedo;
class CColorSchemes;
class CColumnMarker;
class CFileGlobals;
class CFindText;

extern const char * g_pszAppName;
extern CColorSchemes g_cs;
extern CFileGlobals g_fg;

const int knWedgeWidth = 7;
const int knWedgeHeight = 4;

typedef int (*PfnMemAccessDenied)(int nException, DWORD * pdwErrorCode, void * pv);
typedef BOOL (WINAPI *PfnSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
typedef BOOL (WINAPI *PfnAllowSetForegroundWindow)(DWORD dwProcessId);

typedef wchar_t wchar;

#ifdef _DEBUG

//const int kcprInPart = 10;
const int kcprInPart = 5000;
inline bool ValidReadPtrSize(const void * pv, int cb)
{
	if (cb < 0)
		return false;
	if (cb == 0)
		return true;
	return pv != NULL && !::IsBadReadPtr(pv, cb);
}
template<typename T> inline bool ValidReadPtr(T * pt)
{
	return pt != NULL && !::IsBadReadPtr(pt, sizeof(T));
}
#define Assert(exp) assert(exp)
#define AssertArray(pv, cv) Assert((cv) >= 0 && ValidReadPtrSize((pv), sizeof(*(pv)) * (cv)))
#define AssertPtr(pv) Assert(ValidReadPtr(pv))
#define AssertPtrN(pv) Assert(!(pv) || ValidReadPtr(pv))

#else

const int kcprInPart = 5000;
#define Assert(exp)
#define AssertArray(pv, cv)
#define AssertPtr(pv)
#define AssertPtrN(pv)

#endif

/***********************************************************************************************
	Enums
***********************************************************************************************/

typedef enum
{
	kclrWindow = 0,
	kclrText,
	kclrWindowSel,
	kclrTextSel
} Color;

typedef enum
{
	kcmEdit = 0,
	kcmTabs,
	kcmColumn,
	kcmFind,
	kcmPageSetup,
	kcmWindow,
	kcmFileDrop,
	kcmFindInFiles,
} ContextMenu;

typedef enum
{
	kcsrIBeam = 0,
	kcsrHand
} Cursor;

typedef enum
{
	kftNone = -1,
	kftAnsi,
	kftUnicode8,
	kftUnicode16,
	kftBinary,
	kftDefault
} FileType;

typedef enum
{
	kffUp = 1,
	kffDown = 2,
	kffReplace = 4,
	kffReplaceAll = 8,
	kffWholeWord = 16,
	kffMatchCase = 32,
} FindFlag;

typedef enum
{
	kitNone = 0,
	kitNormal,
	kitSmart,
} IndentType;

typedef enum
{
	kmtNone = 0,
	kmtSlow,
	kmtMedium,
	kmtFast,

	kmtSmoothScroll
} MouseTimer;

typedef enum
{
	kmtPaste = 0,
	kmtCut,
	kmtCopy,
	kmtPasteCut
} MoveType;

typedef enum
{
	kstNone = 0,
	kstSelBefore,
	kstSelAfter,
} SelectionType;

// The last three values are all dependent, so if one needs to change, fix the others.
// If kpmParaChars changes, CharsInPara and PrintCharsInPara must be fixed below.
typedef enum {
	kpmOneEndOfPara = 0x20000000,
	kpmTwoEndOfPara = 0x40000000,
	kpmEndOfParaShift = 29,

	kpmParaInMem = 0x80000000,
	kpmEndOfPara = kpmOneEndOfPara | kpmTwoEndOfPara,
	kpmParaChars = 0x1FFFFFFF,
} ParaMask;

typedef enum
{
	kurtEmpty = -1,
	kurtDefault,
	kurtCut,
	kurtPaste,
	kurtTyping,
	kurtReplace,
	kurtReplaceAll,
	kurtDelete,
#ifdef SPELLING
	kurtSpell,
#endif // SPELLING
	kurtIndentIncrease,
	kurtIndentDecrease,
	kurtSmartIndent,
	kurtMove,	// From another program to ZEdit (Copy and Move)
	// The next two are internal types. Don't check for these in an undo/redo entry
	kurtMove_1,	// From ZEdit to ZEdit (Move)
	kurtMove_2	// From ZEdit to ZEdit (Copy)
} UndoRedoType;


/***********************************************************************************************
	Inline utility functions
***********************************************************************************************/

inline COLORREF SyntaxToColor(UINT nSyntax)
{
	return (COLORREF)(nSyntax & 0x00FFFFFF);  // low six bytes
}
inline bool SyntaxToBlock(UINT nSyntax)
{
	return (nSyntax & 0x01000000) ? true : false;
}
inline bool SyntaxToBlockP(UINT nSyntax)
{
	return (nSyntax & 0x02000000) ? true : false;
}
inline UINT BlockToSyntax(bool fBlock)
{
	return fBlock ? 0x01000000 : 0;
}
inline UINT BlockPToSyntax(bool fBlockP)
{
	return fBlockP ? 0x02000000 : 0;
}
inline void Convert8to16(char * pSrc, wchar * pDst, int cch)
{
	AssertArray(pSrc, cch);
	AssertArray(pDst, cch);
	pDst += cch;
	char * pSrcPos = pSrc + cch;
	while (pSrcPos > pSrc)
		*--pDst = *--pSrcPos;
}
inline void Convert16to8(wchar * pSrc, char * pDst, int cch)
{
	AssertArray(pSrc, cch);
	AssertArray(pDst, cch);
	pDst += cch;
	wchar * pSrcPos = pSrc + cch;
	while (pSrcPos > pSrc)
		*--pDst = (unsigned char)*--pSrcPos;
}
inline UINT PrintCharsInPara(DocPart * pdp, UINT ipr);
inline UINT PrintCharsInPara(UINT cch);
inline UINT CharsInPara(DocPart * pdp, UINT ipr);
inline UINT CharsInPara(UINT cch);
inline UINT CharsAtEnd(DocPart * pdp, UINT ipr);
inline UINT CharsAtEnd(UINT cch);
inline BOOL IsParaInMem(DocPart * pdp, UINT ipr);
inline BOOL IsParaInMem(UINT cch);


struct DocPart
{
	DocPart(DocPart * pdpBefore = NULL, DocPart * pdpAfter = NULL);

	DocPart * pdpPrev;
	void * rgpv[kcprInPart];
	UINT rgcch[kcprInPart];
	UINT cpr;
	UINT cch;
	DocPart * pdpNext;
};

// Returns number of printable characters.
inline UINT PrintCharsInPara(DocPart * pdp, UINT ipr)
{
	AssertPtr(pdp);
	return (pdp->rgcch[ipr] & kpmParaChars);
}
// Returns number of printable characters.
inline UINT PrintCharsInPara(UINT cch)
{
	return cch & kpmParaChars;
}
// Returns total number of characters in the paragraph.
inline UINT CharsInPara(DocPart * pdp, UINT ipr)
{
	AssertPtr(pdp);
	return (pdp->rgcch[ipr] & kpmParaChars) + ((pdp->rgcch[ipr] & kpmEndOfPara) >> kpmEndOfParaShift);
}
// Returns total number of characters in the paragraph.
inline UINT CharsInPara(UINT cch)
{
	return (cch & kpmParaChars) + ((cch & kpmEndOfPara) >> kpmEndOfParaShift);
}
inline UINT CharsAtEnd(DocPart * pdp, UINT ipr)
{
	AssertPtr(pdp);
	return (pdp->rgcch[ipr] & kpmEndOfPara) >> kpmEndOfParaShift;
}
inline UINT CharsAtEnd(UINT cch)
{
	return (cch & kpmEndOfPara) >> kpmEndOfParaShift;
}
inline BOOL IsParaInMem(DocPart * pdp, UINT ipr)
{
	AssertPtr(pdp);
	return pdp->rgcch[ipr] & kpmParaInMem;
}
inline BOOL IsParaInMem(UINT cch)
{
	return cch & kpmParaInMem;
}


/***********************************************************************************************
	Structures
***********************************************************************************************/

struct BlockPara
{
	BlockPara()
	{
		m_szStop[0] = 10;
		m_szStart[0] = m_szStop[1] = 0;
		m_cr = 0;
	}

	char m_szStart[10];
	char m_szStop[10];
	COLORREF m_cr;
};

struct ColorSchemeInfo
{
	ColorSchemeInfo(char * pszExt);
	~ColorSchemeInfo();
	bool EnsureLoaded();

	bool FindWord(char * prgch, int cchLim, const char * pszDelimiters, COLORREF * pcr, int * pcch);
	bool FindWord(wchar * prgwch, int cchLim, const char * pszDelimiters, COLORREF * pcr, int* pcch);
	bool FindBlockP(char * prgch, int cchLim, COLORREF * pcr, char * pch);
	bool FindBlockP(wchar * prgwch, int cchLim, COLORREF * pcr, wchar * pwch);

	char * m_pszDesc;
	char * m_pszExt;
	bool m_fLoaded;
	char m_szBlockStart[10];
	char m_szBlockStop[10];
	wchar m_wszBlockStart[10];
	wchar m_wszBlockStop[10];
	COLORREF m_crBlock;
	COLORREF m_crDefault;
	char * m_pszDelim;
	bool m_fCaseMatters;
	IndentType m_it;
	std::vector<BlockPara *> m_vpbp;
	std::vector<KeywordInfo *> m_vpki;
	CTrieLevel * m_ptl;
};

struct FindItem
{
	FindItem();
	~FindItem();
	wchar * pwszFindWhat;
	UINT cchFindWhat;
	wchar * pwszReplaceWith;
	UINT cchReplaceWith;
	UINT nFlags;
	UINT ichStart;
	UINT ichStop;
	bool fFindIsReplace;
	// The results of the find are stored here.
	UINT ichMatch;
	UINT cchMatch;
};

struct HeaderFooterInfo
{
	char * pszPathname;
	char * pszFilename;
	int iPage;
	char * pszTime;
	char * pszDate;
};

struct KeywordInfo
{
	KeywordInfo()
	{
		*m_szDesc = 0;
		m_pszWords = NULL;
		m_cr = 0;
	}
	~KeywordInfo()
	{
		if (m_pszWords)
			delete m_pszWords;
		m_pszWords = NULL;
	}

	char m_szDesc[50];
	char * m_pszWords;
	COLORREF m_cr;
};

class RenderInfo
{
public:
	void Initialize(HDC hdc, int cchSpacesInTab);
	int CharOut(HDC hdc, char ch);
	int GetCharWidth(char ch);
	int AddCharWidth(int nWidth, char * prgch);
	int AddCharWidth(int nWidth, wchar *& prgch, wchar * prgchLim, HDC hdc);

	TEXTMETRIC m_tm;
	int m_rgnCharWidth[256];
	int m_cchSpacesInTab;
	int m_dxpTab;
};

struct LineInfo
{
	UINT ipr;
	UINT ich;
	UINT dxp;
	UINT nSyntax;
};

struct ParaDrawInfo
{
	ParaDrawInfo()
	{
		memset(this, 0, sizeof(ParaDrawInfo));
	}

	HDC hdcMem;
	RECT rcBounds;
	RECT rcScreen;
	DocPart * pdp;
	void * pvStart;
	UINT iprInPart;
	UINT ipr;
	UINT ich;
	LineInfo * prgli;
	UINT cli;
	UINT cLineLimit;
	int dypLine;
	RenderInfo ri;
	bool fWrap;
	bool fShowColors;
	bool fInBlock;
	bool fInBlockP;
	char chEndBlockP;
};

struct PrintInfo
{
	HDC hdc;
	UINT ichMin;
	UINT ichStop;
	RECT rcMargin;
	POINT pthf;
	int cCopy;
	BOOL fCollate;
	char * pszHeader;
	char * pszFooter;
	char * pszFilename;
	int iPage;
	bool fCancel;
	HWND hdlgAbort;
};

struct FilePropInfo
{
	CZEditFrame * pzef;
	CZDoc * pzd;
};

struct FileOpenInfo
{
	CZEditFrame * pzef;
	UINT iCommand;
};


#include "ZEdit.h"
#include "ZView.h"
#include "ZDoc.h"
#include "ZFrame.h"
#include "FindDlg.h"
#include "DragDrop.h"
#include "Resource.h"


/***********************************************************************************************
	Classes
***********************************************************************************************/

typedef CComBSTR MyString;

struct ltstr
{
	bool operator()(const MyString & s1, const MyString & s2) const
	{
		return s1 < s2;
	}
	bool operator()(const MyString * ps1, const MyString * ps2) const
	{
		return *ps1 < *ps2;
	}
};

typedef std::set<MyString *, ltstr> MyStringSet;

/*----------------------------------------------------------------------------------------------
	ZTypeInfo declaration/definition.
----------------------------------------------------------------------------------------------*/
class ZTypeInfo
{
public:
	~ZTypeInfo();

	MyString m_sName;
	MyStringSet m_vpmsMethod;
	MyStringSet m_vpmsAttribute;
};

typedef std::map<MyString, ZTypeInfo *, ltstr> ZTypeInfoMap;


/*----------------------------------------------------------------------------------------------
	ZTypeLibrary declaration/definition.
----------------------------------------------------------------------------------------------*/
class ZTypeLibrary
{
public:
	~ZTypeLibrary();
	bool LoadTypeLibrary(const wchar * pszFilename);
	const OLECHAR * Name()
		{ return m_sName; }
	const OLECHAR * Filename()
		{ return m_sFilename; }
	const ZTypeInfoMap & GetTypeInfoMap()
		{ return m_mapTypeInfo; }

protected:
	bool LoadTypeInfo(ITypeInfo * ptinfo, CComBSTR & bstrName);

	MyString m_sName;
	MyString m_sFilename;
	ZTypeInfoMap m_mapTypeInfo;
};


/*----------------------------------------------------------------------------------------------
	CUndoRedo declaration.
----------------------------------------------------------------------------------------------*/
class CUndoRedo
{
public:
	CUndoRedo(UndoRedoType urt, UINT ichStart, UINT cch, UINT cchReplaced);
	~CUndoRedo();

	struct ReplaceAllLocation
	{
		UINT m_ich;
		UINT m_cch;
		UINT m_izdo;
	};

	class ReplaceAllInfo
	{
	public:
		ReplaceAllInfo();
		~ReplaceAllInfo();
		UINT AddReplaceAllItem(CZDoc * pzd, UINT ichMatch, UINT cchMatch,
			bool fUpdateLocation);

		UINT m_crals;
		ReplaceAllLocation * m_prgral;
		UINT m_czdo;
		CZDataObject ** m_prgzdo;
	};

	UndoRedoType m_urt;
	UINT m_ichMin;
	UINT m_cch;
	UINT m_cchReplaced;
	CZDataObject * m_pzdo;
	union
	{
		ReplaceAllInfo * m_prai; // Only used for ReplaceAll entries
		MoveType m_mt; // Only used for Move entries
	};
};


/*----------------------------------------------------------------------------------------------
	CColorSchemes declaration.
----------------------------------------------------------------------------------------------*/
class CColorSchemes
{
public:
	CColorSchemes();
	~CColorSchemes();

	bool Load();
	void Close();
	ColorSchemeInfo * GetColorScheme(char * pszExt);
	ColorSchemeInfo * GetColorScheme(UINT ics);
	int GetColorSchemeCount()
		{ return m_vpcsi.size(); }

protected:
	std::vector<ColorSchemeInfo *> m_vpcsi;
	char * m_pszFileText;
};


/*----------------------------------------------------------------------------------------------
	CColumnMarker declaration.
----------------------------------------------------------------------------------------------*/
class CColumnMarker
{
public:
	CColumnMarker(int iColumn, COLORREF cr);

	bool OnDraw(HDC hdc, bool fErase, RECT & rect, int nOffset);

	int m_iColumn;
	COLORREF m_cr;
	int m_nPixelOffset;
};


/*----------------------------------------------------------------------------------------------
	CFileGlobals declaration.
----------------------------------------------------------------------------------------------*/
class CFileGlobals
{
public:
	CFileGlobals();
	~CFileGlobals();

	void SetWindowColor(COLORREF cr)
		{ m_cr[kclrWindow] = cr & 0xffffff; }
	void SetTextColor(COLORREF cr)
		{ m_cr[kclrText] = cr & 0xffffff; }
	bool DrawColumnMarkers(HDC hdc, bool fErase, RECT & rect, int nXOffset);
	int GetColumnMarker(POINT point);
	int SetColumnMarkerPos(CZEditFrame * pzef, HWND hwnd, POINT point, int nXOffset);
	PfnMemAccessDenied SetAccessDeniedHandler(PfnMemAccessDenied pfnNew);
	bool OpenOutlookAttachment(CZEditFrame * pzef, IDataObject * pdo);
	void Draw3dRect(HDC hdc, RECT * prc, COLORREF clrTopLeft, COLORREF clrBottomRight);
	char * FixString(char * pszFileString);
	HWND GetToolTipHwnd();

	int LoadTypeLibrary(wchar * pszFilename);
	ZTypeLibrary & GetTypeLibrary(int iztl)
	{
		Assert((UINT)iztl < (UINT)m_vpztl.size());
		AssertPtr(m_vpztl[iztl]);
		return *m_vpztl[iztl];
	}

	void DrawControl(HWND hwnd, WNDPROC wndProc);

	CLIPFORMAT GetClipFileGroup();
	CLIPFORMAT GetClipFileContents();
	CLIPFORMAT GetClipZEditFile();

	HINSTANCE m_hinst;
	UINT m_cchFileLimit;
	int m_dxpTab;
	int m_defaultEncoding;
	int m_nScrollLines;
	RECT m_rcMargin;
	HCURSOR m_hCursor[3];
	COLORREF m_cr[4];
	PfnMemAccessDenied m_pfnMemAccessDenied;
	PfnSetLayeredWindowAttributes m_pfnSetLayeredWindowAttributes;
	bool m_fCanUseVer5;

	unsigned char m_chSpace;
	unsigned char m_chPara;
	std::vector<CColumnMarker *> m_vpcm;
	HIMAGELIST m_himlCM;
	UINT m_iColumnDrag;
	bool m_fUseSpaces;

	// Font stuff specific to text files.
	HFONT m_hFont;
	HFONT m_hFontOld;
	RenderInfo m_ri;

	// Font stuff specific to text files.
	HFONT m_hFontFixed;
	HFONT m_hFontFixedOld;
	TEXTMETRIC m_tmFixed;

	// Print settings.
	RECT m_rcPrintMargins;
	POINT m_ptHeaderFooter;
	char m_szPrintStrings[2][MAX_PATH];

	// Subclassed window procedures.
	WNDPROC m_wpOldStatusProc;
	WNDPROC m_wpOldToolProc;

	// Show Character Codes stuff.
	HWND m_hwndViewCodes;
	int m_cchCodesBefore;
	int m_cchCodesAfter;
	bool m_fFlatScrollbars;

protected:
	CLIPFORMAT m_cfFileGroup;
	CLIPFORMAT m_cfFileContents;
	CLIPFORMAT m_cfZEditFile;

	HWND m_hwndToolTip;

	std::vector<ZTypeLibrary *> m_vpztl;
};


/*----------------------------------------------------------------------------------------------
	CFindText declaration.
----------------------------------------------------------------------------------------------*/
class CFindText
{
public:
	CFindText(CZDoc * pzd);
	~CFindText();
	bool CheckCharDown(bool fEqual);
	bool CheckCharUp(bool fEqual);
	bool CheckForWholeWord(bool fBefore, int cchOffset);
	bool Find(FindItem * pfi); 

protected:
	CZDoc * m_pzd;
	FindItem * m_pfi;
	DocPart * m_pdpStart;
	DocPart * m_pdpStop;
	DocPart * m_pdpOld;
	UINT m_iprMin;
	UINT m_iprLim;
	UINT m_iprOld;
	UINT m_ichFound;
	UINT m_ichOldFound;
	void * m_pvStart;
	void * m_pvStop;
	void * m_pvPos;
	void * m_pvOldPos;
	void * m_pvLimit;
	void * m_pvOldLimit;
	int m_cMatched;
	wchar * m_pwszFindPos;
	wchar * m_pwszFindWhat;
	int m_cchFind;
	bool m_fWholeWord;
};


/*----------------------------------------------------------------------------------------------
	CWaitCursor declaration/definition.
----------------------------------------------------------------------------------------------*/
class CWaitCursor
{
public:
	CWaitCursor()
	{
		m_hcurOld = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
	}
	~CWaitCursor()
	{
		::SetCursor(m_hcurOld);
	}

protected:
	HCURSOR m_hcurOld;
};


/*----------------------------------------------------------------------------------------------
	ZSmartPtr declaration/definition.
----------------------------------------------------------------------------------------------*/
class ZSmartPtr
{
public:
	ZSmartPtr(void * pv = NULL)
		{ m_pv = pv; }
	~ZSmartPtr()
		{ Attach(NULL); }
	void Attach(void * pv)
	{
		if (m_pv)
			delete m_pv;
		m_pv = pv;
	}

protected:
	void * m_pv;
};

#endif // !_COMMON_H_