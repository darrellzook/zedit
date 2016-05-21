#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"
#include <process.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include "DragDrop.h"

const char g_pszEndOfLine[][2] = { {10}, {13}, {13, 10} };
const wchar g_pwszEndOfLine[][2] = { {10}, {13}, {13, 10} };
const char g_pszUTF8Marker[3] = { (char)0xEF, (char)0xBB, (char)0xBF };


/***********************************************************************************************
	CZView methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CZView::CZView(CZEditFrame * pzef)
{
	AssertPtr(pzef);
	m_pzef = pzef;
	CZView::Initialize();
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZView::Initialize()
{
	m_hwnd = NULL;
	m_ichSelMin = m_ichSelLim = m_ichDragStart = 0;
	m_rcScreen = g_fg.m_rcMargin;
	m_ptCaretPos;
	m_fSelecting = m_fOvertype = m_fIsDropSource = false;
	m_mt = kmtNone;
	m_pds = NULL;
	m_pzd = NULL;
}


/**********************************************************************************************
	CZDoc methods.
**********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CZDoc::CZDoc(CZEditFrame * pzef, char * pszFilename, FileType ft)
{
	AssertPtr(pzef);
	AssertPtr(pszFilename);

	m_pzef = pzef;
	lstrcpyn(m_szFilename, pszFilename, sizeof(m_szFilename) / sizeof(char));

	m_pzdPrev = m_pzdNext = NULL;
	m_pdpFirst = NULL;
	m_cch = m_cpr = m_cTimeToLoad = 0;
	m_pvFile = NULL;
	m_ft = ft;
	m_fModified = m_fModifiedOld = m_fTextFromDisk = m_fAccessError = false;
	m_pcsi = NULL;
	m_pzvCurrent = NULL;
	m_pzf = NULL;
	m_iImage = -1;
	memset(&m_ftModified, 0, sizeof(m_ftModified));
	m_pzdo = NULL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CZDoc::~CZDoc()
{
	if (m_pzdo)
	{
		if (OleIsCurrentClipboard(m_pzdo) == S_OK)
		{
			// If the number of bytes is more than 1 MB, ask the user
			// if they want to be able to paste the clipboard contents after
			// this document has closed.
			int cb;
			m_pzdo->get_ByteCount(&cb);
			if (cb < 1048576 || MessageBox(NULL, "You placed a large amount of text on the Clipboard. "
				"Do you want this text to be available to other applications after you "
				"close this file?", "ZEdit", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				OleFlushClipboard();
			}
			else
			{
				OleSetClipboard(NULL);
			}
		}
		m_pzdo->Release();
		m_pzdo = NULL;
	}
	CloseFileHandles();
	_beginthread(DeleteMemory, 0, (void *)m_pdpFirst);

	ClearUndo();
	ClearRedo();

	delete m_pzf;
}


/*----------------------------------------------------------------------------------------------
	Attach a new frame window to this document.
----------------------------------------------------------------------------------------------*/
void CZDoc::AttachFrame(CZFrame * pzf)
{
	AssertPtr(pzf);
	if (m_pzf)
		delete m_pzf;
	m_pzf = pzf;
	m_pzvCurrent = pzf->GetCurrentView();
}


/*----------------------------------------------------------------------------------------------
	This document has been moved to a different top-level frame.
----------------------------------------------------------------------------------------------*/
void CZDoc::AttachToMainFrame(CZEditFrame * pzef)
{
	AssertPtr(pzef);
	m_pzef = pzef;

	AssertPtr(m_pzf);
	m_pzf->AttachToMainFrame(pzef);
}


/*----------------------------------------------------------------------------------------------
	Return true if the clipboard contains information we can paste.
----------------------------------------------------------------------------------------------*/
bool CZDoc::CanPaste()
{
	bool fCanPaste = false;
	if (OpenClipboard(NULL))
	{
		// NOTE: GetClipFileGroup is an attachment from Microsoft Outlook.
		if (::IsClipboardFormatAvailable(CF_UNICODETEXT) ||
			::IsClipboardFormatAvailable(CF_TEXT) ||
			::IsClipboardFormatAvailable(CF_OEMTEXT) ||
			::IsClipboardFormatAvailable(g_fg.GetClipFileGroup()))
		{
			fCanPaste = true;
		}
		CloseClipboard();
	}
	return fCanPaste;
}


void ToLowerA(char * pch)
{
	WPARAM ch = MAKEWPARAM(*pch, 0);
	LPSTR p = (LPSTR)&ch;
	CharLowerA(p);
	*pch = *p;
}

void ToLowerW(wchar * pch)
{
	WPARAM ch = MAKEWPARAM(*pch, 0);
	LPWSTR p = (LPWSTR)&ch;
	CharLowerW(p);
	*pch = *p;
}

void ToUpperA(char * pch)
{
	WPARAM ch = MAKEWPARAM(*pch, 0);
	LPSTR p = (LPSTR)&ch;
	CharUpperA(p);
	*pch = *p;
}

void ToUpperW(wchar * pch)
{
	WPARAM ch = MAKEWPARAM(*pch, 0);
	LPWSTR p = (LPWSTR)&ch;
	CharUpperW(p);
	*pch = *p;
}

/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CaseLower(int ch, int & nDummy)
{
	return tolower(ch);
}
void CaseLowerA(void * pch, int & nDummy)
{
	ToLowerA((char *)pch);
}
void CaseLowerW(void * pch, int & nDummy)
{
	ToLowerW((wchar *)pch);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CaseUpper(int ch, int & nDummy)
{
	return toupper(ch);
}
void CaseUpperA(void * pch, int & nDummy)
{
	ToUpperA((char *)pch);
}
void CaseUpperW(void * pch, int & nDummy)
{
	ToUpperW((wchar *)pch);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CaseToggle(int ch, int & nDummy)
{
	if (islower(ch))
		return toupper(ch);
	if (isupper(ch))
		return tolower(ch);
	return ch;
}
void CaseToggleA(void * pch, int & nDummy)
{
	if (IsCharLowerA(*((char *)pch)))
		ToUpperA((char *)pch);
	else if (IsCharUpperA(*((char *)pch)))
		ToLowerA((char *)pch);
}
void CaseToggleW(void * pch, int & nDummy)
{
	if (IsCharLowerW(*(wchar *)pch))
		ToUpperW((wchar *)pch);
	else if (IsCharUpperW(*(wchar *)pch))
		ToLowerW((wchar *)pch);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CaseSentence(int ch, int & nStart)
{
	if (nStart == 0)
	{
		if ('.' == ch || '?' == ch || '!' == ch)
			nStart = 1;
	}
	else if (nStart == 1)
	{
		if (isspace(ch))
			nStart = 2;
	}
	else
	{
		if (isalpha(ch))
		{
			nStart = 0;
			return toupper(ch);
		}
		else if ('.' == ch || '?' == ch || '!' == ch)
			nStart = 1;
		else if (!isspace(ch))
			nStart = 0;
	}
	return ch;
}
void CaseSentenceA(void * pch, int & nStart)
{
	char ch = *(char *)pch;
	if (nStart == 0)
	{
		if ('.' == ch || '?' == ch || '!' == ch)
			nStart = 1;
	}
	else if (nStart == 1)
	{
		if (IsCharSpaceA(ch))
			nStart = 2;
	}
	else
	{
		if (IsCharAlphaA(ch))
		{
			nStart = 0;
			ToUpperA((char *)pch);
			return;
		}
		if ('.' == ch || '?' == ch || '!' == ch)
			nStart = 1;
		else if (!IsCharSpaceA(ch))
			nStart = 0;
	}
}
void CaseSentenceW(void * pch, int & nStart)
{
	wchar ch = *(wchar *)pch;
	if (nStart == 0)
	{
		if ('.' == ch || '?' == ch || '!' == ch)
			nStart = 1;
		else if (IsCharAlphaW(ch))
			ToUpperW((wchar *)pch);
	}
	else if (nStart == 1)
	{
		if (IsCharSpaceW(ch))
			nStart = 2;
	}
	else
	{
		if (IsCharAlphaW(ch))
		{
			nStart = 0;
			ToUpperW((wchar *)pch);
		}
		else if ('.' == ch || '?' == ch || '!' == ch)
		{
			nStart = 1;
		}
		else if (!IsCharSpaceW(ch))
		{
			nStart = 0;
		}
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CaseTitle(int ch, int & nStart)
{
	if (nStart)
	{
		if (isalpha(ch))
		{
			nStart = 0;
			return toupper(ch);
		}
		if (!isspace(ch) && !ispunct(ch))
			nStart = 0;
	}
	else
	{
		if (isspace(ch) || ispunct(ch))
			nStart = 1;
	}
	return tolower(ch);
}
void CaseTitleA(void * pch, int & nStart)
{
	char ch = *(char *)pch;
	if (nStart)
	{
		if (IsCharAlphaA(ch))
		{
			nStart = 0;
			ToUpperA((char *)pch);
			return;
		}
		if (!IsCharSpaceA(ch) && !ispunct(ch))
			nStart = 0;
	}
	else
	{
		if (IsCharSpaceA(ch) || ispunct(ch))
			nStart = 1;
	}
	ToLowerA((char *)pch);
}
void CaseTitleW(void * pch, int & nStart)
{
	wchar ch = *(wchar *)pch;
	if (nStart)
	{
		if (IsCharAlphaW(ch))
		{
			nStart = 0;
			ToUpperW((wchar *)pch);
			return;
		}
		if (!IsCharSpaceW(ch) && !iswpunct(ch))
			nStart = 0;
	}
	else
	{
		if (IsCharSpaceW(ch) || iswpunct(ch))
			nStart = 1;
	}
	ToLowerW((wchar *)pch);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::ChangeCase(UINT ichMin, UINT ichStop, CompareCaseType cct)
{
	if (ichStop > m_cch)
		ichStop = m_cch;
	int cchToChange = ichStop - ichMin;
	if (cchToChange == 0)
		return true;

	CWaitCursor wc;
	void * pv;
	UINT ipr;
	DocPart * pdp = GetPart(ichMin, false, &pv, &ipr);
	/*int (*pfnChange)(int ch, int & nStart);
	switch (cct)
	{
	case kcctLower:
		pfnChange = CaseLower; break;
	case kcctUpper:
		pfnChange = CaseUpper; break;
	case kcctToggle:
		pfnChange = CaseToggle; break;
	case kcctSentence:
		pfnChange = CaseSentence; break;
	case kcctTitle:
		pfnChange = CaseTitle; break;
	default:
		return false;
	}*/
	void(*pfnChange)(void * pch, int & nStart);
	switch (cct)
	{
	case kcctLower:
		pfnChange = (m_ft == kftAnsi) ? CaseLowerA : CaseLowerW; break;
	case kcctUpper:
		pfnChange = (m_ft == kftAnsi) ? CaseUpperA : CaseUpperW; break;
	case kcctToggle:
		pfnChange = (m_ft == kftAnsi) ? CaseToggleA : CaseToggleW; break;
	case kcctSentence:
		pfnChange = (m_ft == kftAnsi) ? CaseSentenceA : CaseSentenceW; break;
	case kcctTitle:
		pfnChange = (m_ft == kftAnsi) ? CaseTitleA : CaseTitleW; break;
	default:
		return false;
	}

	m_fModified = true;
	int nStart;
	//if (IsAnsi(pdp, ipr))
	if (m_ft == kftAnsi)
	{
		char * pch = (char *)pv;
		char * pchStop = (char *)pdp->rgpv[ipr] + PrintCharsInPara(pdp, ipr);
		int cchInPara = pchStop - pch;
		while (cchToChange > 0)
		{
			nStart = 2;
			if (!IsParaInMem(pdp, ipr))
			{
				int cchInNewPara = CharsInPara(pdp, ipr);
				void * pvNewPara = new char[cchInNewPara];
				if (!pvNewPara)
				{
					::InvalidateRect(GetHwnd(), NULL, false);
					return false;
				}
				memmove(pvNewPara, pdp->rgpv[ipr], cchInNewPara);
				pch = (pch - (char *)pdp->rgpv[ipr]) + (char *)pvNewPara;
				pchStop = pch + cchInPara;
				pdp->rgpv[ipr] = pvNewPara;
				pdp->rgcch[ipr] |= kpmParaInMem;
			}
			if (cchToChange > cchInPara)
			{
				while (pch < pchStop)
					//*pch++ = pfnChange(*pch, nStart);
					pfnChange(pch++, nStart);
				cchToChange -= cchInPara + CharsAtEnd(pdp, ipr);
				if (++ipr >= pdp->cpr)
				{
					pdp = pdp->pdpNext;
					if (!pdp)
						break;
					ipr = 0;
				}
				pch = (char *)pdp->rgpv[ipr];
				cchInPara = PrintCharsInPara(pdp, ipr);
				pchStop = pch + cchInPara;
			}
			else
			{
				pchStop = pch + cchToChange;
				while (pch < pchStop)
					//*pch++ = pfnChange(*pch, nStart);
					pfnChange(pch++, nStart);
				break;
			}
		}
	}
	else
	{
		wchar * pch = (wchar *)pv;
		wchar * pchStop = (wchar *)pdp->rgpv[ipr] + PrintCharsInPara(pdp, ipr);
		int cchInPara = pchStop - pch;
		while (cchToChange > 0)
		{
			nStart = 2;
			if (!IsParaInMem(pdp, ipr))
			{
				bool isAnsi = IsAnsi(pdp, ipr);
				void * pvOldPara = pdp->rgpv[ipr]; // Save old start of paragraph
				void * pvNewPara = ChangeParaSize(pdp, ipr, CharsInPara(pdp, ipr));
				/*int cchInNewPara = CharsInPara(pdp, ipr);
				void * pvNewPara = new wchar[cchInNewPara];
				if (!pvNewPara)
				{
					::InvalidateRect(GetHwnd(), NULL, false);
					return false;
				}
				memmove(pvNewPara, pdp->rgpv[ipr], cchInNewPara << 1);
				pch = (pch - (wchar *)pvOldPara) + (wchar *)pvNewPara;
				pchStop = pch + cchInPara;
				pdp->rgpv[ipr] = pvNewPara;
				pdp->rgcch[ipr] |= kpmParaInMem;*/
				if (isAnsi)
					pch = ((char *)pch - (char *)pvOldPara) + (wchar *)pvNewPara;
				else
					pch = (pch - (wchar *)pvOldPara) + (wchar *)pvNewPara;
				pchStop = pch + cchInPara;
			}
			if (cchToChange > cchInPara)
			{
				while (pch < pchStop)
					//*pch++ = pfnChange(*pch, nStart);
					pfnChange(pch++, nStart);
				cchToChange -= cchInPara + CharsAtEnd(pdp, ipr);
				if (++ipr >= pdp->cpr)
				{
					pdp = pdp->pdpNext;
					if (!pdp)
						break;
					ipr = 0;
				}
				pch = (wchar *)pdp->rgpv[ipr];
				cchInPara = PrintCharsInPara(pdp, ipr);
				pchStop = pch + cchInPara;
			}
			else
			{
				pchStop = pch + cchToChange;
				while (pch < pchStop)
					//*pch++ = pfnChange(*pch, nStart);
					pfnChange(pch++, nStart);
				break;
			}
		}
	}
	::InvalidateRect(GetHwnd(), NULL, false);

	ClearUndo();
	ClearRedo();

	return true;
}


/*----------------------------------------------------------------------------------------------
	nParaType can be one of the following values:
		0 - Lf   (Mac)
		1 - Cr   (Unix)
		2 - CrLf (PC)
----------------------------------------------------------------------------------------------*/
bool CZDoc::FixLines(int nParaType)
{
	Assert(nParaType >= 0 && nParaType < 3);
	DocPart * pdp = m_pdpFirst;
	int cchWantedAtEnd = (nParaType == 2) ? 2 : 1;
	UINT cchParasModified = 0;
	while (pdp)
	{
		// Fix all the end of lines in this doc part.
		UINT cpr = pdp->cpr;
		for (UINT ipr = 0; ipr < cpr; ipr++)
		{
			int cchAtEnd = CharsAtEnd(pdp, ipr);
			bool fIsAnsi = IsAnsi(pdp, ipr);
			if (cchAtEnd == 1)
			{
				// The paragraph currently has either a Cr or a Lf at the end.
				UINT cchInPara = CharsInPara(pdp, ipr);
				if (cchWantedAtEnd == 2)
				{
					// Allocate an extra 1 character for the CrLf combination.
					if (!ChangeParaSize(pdp, ipr, cchInPara + 1))
						return false;
					// The ChangeParaSize call might have changed whether we're ANSI or not, so check again.
					fIsAnsi = IsAnsi(pdp, ipr);
					if (fIsAnsi)
					{
						char * prgchPara = (char *)pdp->rgpv[ipr];
						prgchPara[cchInPara - 1] = 13;
						prgchPara[cchInPara] = 10;
					}
					else
					{
						wchar * prgchPara = (wchar *)pdp->rgpv[ipr];
						prgchPara[cchInPara - 1] = 13;
						prgchPara[cchInPara] = 10;
					}
					pdp->rgcch[ipr] = (pdp->rgcch[ipr] & ~kpmEndOfPara) | kpmTwoEndOfPara;
					pdp->cch++;
					m_cch++;
					cchParasModified++;
				}
				else
				{
					// If the end-of-line character is the wrong one, load the paragraph
					// in memory and change it.
					bool fNeedToChange;
					if (fIsAnsi)
						fNeedToChange = ((char *)pdp->rgpv[ipr])[cchInPara - 1] != *g_pszEndOfLine[nParaType];
					else
						fNeedToChange = ((wchar *)pdp->rgpv[ipr])[cchInPara - 1] != *g_pwszEndOfLine[nParaType];
					if (fNeedToChange)
					{
						if (!ChangeParaSize(pdp, ipr, cchInPara))
							return false;
						if (fIsAnsi)
							((char *)pdp->rgpv[ipr])[cchInPara - 1] = *g_pszEndOfLine[nParaType];
						else
							((wchar *)pdp->rgpv[ipr])[cchInPara - 1] = *g_pwszEndOfLine[nParaType];
						cchParasModified++;
					}
				}
			}
			else
			{
				// The paragraph currently has a CrLf at the end.
				bool fChanged = true;
				if (nParaType == 1) // Cr
				{
					// Since the paragraph already has a CrLf, we don't need to modify
					// anything.
				}
				else if (nParaType == 0) // Lf
				{
					UINT cchInPara = CharsInPara(pdp, ipr) - 1;
					if (cchInPara == (UINT)-1)
					{
						// This should only happen on an empty line at the end of the file.
						Assert(ipr == pdp->cpr - 1 && pdp->pdpNext == NULL);
						continue;
					}
					if (!ChangeParaSize(pdp, ipr, cchInPara))
						return false;
					if (fIsAnsi)
						((char *)pdp->rgpv[ipr])[cchInPara - 1] = 10;
					else
						((wchar *)pdp->rgpv[ipr])[cchInPara - 1] = 10;
				}
				else
					fChanged = false;
				if (fChanged)
				{
					pdp->cch--;
					m_cch--;
					cchParasModified++;
					pdp->rgcch[ipr] = (pdp->rgcch[ipr] & ~kpmEndOfPara) | kpmOneEndOfPara;
				}
			}
		}
		pdp = pdp->pdpNext;
	}
	if (cchParasModified == 0)
	{
		::MessageBox(NULL, "No paragraphs needed to be modified.", "ZEdit", MB_ICONINFORMATION);
	}
	else
	{
		m_fModified = true;
		char szMessage[MAX_PATH];
		if (cchParasModified == 1)
			strcpy_s(szMessage, "1 paragraph was modified.");
		else
			sprintf_s(szMessage, "%s paragraphs were modified.", CZEditFrame::InsertComma(cchParasModified));
		::MessageBox(NULL, szMessage, "ZEdit", MB_ICONINFORMATION);
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	Move the paragraph into memory if it's not already in memory and change the number of
	characters it can hold.
----------------------------------------------------------------------------------------------*/
void * CZDoc::ChangeParaSize(DocPart * pdp, UINT ipr, UINT cchParaNew)
{
	void * pv;
	UINT cbParaNew = cchParaNew;
	// 2008-03-02 dzook: This doesn't work if we're fixing a Unicode8 file and the line isn't already in memory.
	//if (!IsAnsi(pdp, ipr))
	if (m_ft != kftAnsi)
		cbParaNew *= 2;
	if (IsParaInMem(pdp, ipr))
	{
		if (CharsInPara(pdp, ipr) >= cchParaNew)
		{
			// We're already big enough, so return.
			return pdp->rgpv[ipr];
		}
		if ((pv = realloc(pdp->rgpv[ipr], cbParaNew)) != NULL)
			pdp->rgpv[ipr] = pv;
	}
	else
	{
		if ((pv = new char[cbParaNew]) != NULL)
		{
			if (m_ft == kftUnicode8 && !IsParaInMem(pdp, ipr))
			{
				// Need to convert to Unicode16.
				Convert8to16((char *)pdp->rgpv[ipr], (wchar *)pv, CharsInPara(pdp, ipr));
			}
			else
			{
				memmove(pv, pdp->rgpv[ipr], cbParaNew);
			}
			pdp->rgpv[ipr] = pv;
			pdp->rgcch[ipr] |= kpmParaInMem;
		}
	}
	return pv;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UINT CZDoc::CharFromPara(UINT ipr)
{
	if (!m_pdpFirst || m_fAccessError)
		return 0;

	UINT iprT = 0;
	UINT cchBefore = 0;
	DocPart * pdp = GetPart(ipr, true, NULL, &iprT, &cchBefore);
	while (iprT--)
		cchBefore += CharsInPara(pdp, iprT);
	return cchBefore;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZDoc::CloseFileHandles()
{
	if (m_pvFile)
	{
		if (m_fTextFromDisk)
			UnmapViewOfFile(m_pvFile);
		else
			delete m_pvFile;
		m_pvFile = NULL;
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::Copy(UINT ichMin, UINT ichStop, bool fMakeUREntry)
{
	if (m_fAccessError || ichStop <= ichMin)
		return false;
	CWaitCursor wc;
	bool fSuccess = false;

	AssertPtrN(m_pzdo);
	if (m_pzdo)
	{
		m_pzdo->Release();
		m_pzdo = NULL;
	}
	UINT cch = GetText(ichMin, ichStop - ichMin, &m_pzdo);
	if (m_pzdo)
	{
		EmptyClipboard();
		if (SUCCEEDED(OleSetClipboard(m_pzdo)))
		{
			CUndoRedo * pur = NULL;
			if (fMakeUREntry)
				pur = new CUndoRedo(kurtCut, ichMin, ichStop - ichMin, 0);
			if (pur)
			{
				pur->m_pzdo = m_pzdo;
				m_pzdo->AddRef();
				m_stackUndo.push(pur);
			}
			ClearRedo();
			fSuccess = true;
		}
	}
	return fSuccess;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::Cut(UINT ichMin, UINT ichStop)
{
	bool fSuccess = Copy(ichMin, ichStop, true);
	if (fSuccess)
		DeleteText(ichMin, ichStop, false);
	return fSuccess;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::DeleteText(UINT ichMin, UINT ichStop, bool fClearRedo, bool fRedraw)
{
	static int totalcalls = 0;
	totalcalls++;
	if (totalcalls >= 3300)
		totalcalls = totalcalls;
	if (totalcalls < 3500)
	{
		char szMsg[4000];
		sprintf_s(szMsg, "DeleteText - %d\n", totalcalls);
		OutputDebugString(szMsg);
	}
	//OutputDebugString("DeleteText\n");
	if (m_fAccessError)
		return false;
	if (ichMin == ichStop)
		return true;

	if (ichMin > ichStop)
	{
		UINT nT = ichMin;
		ichMin = ichStop;
		ichStop = nT;
	}
	if (fClearRedo)
		ClearRedo();
	m_fModified = true;

	UINT iprStart = 0;
	UINT iprStop = 0;
	void * pvStart;
	void * pvStop;
	DocPart * pdpStart = GetPart(ichMin, false, &pvStart, &iprStart);
	DocPart * pdpStop = GetPart(ichStop, false, &pvStop, &iprStop);

	bool fBefore8to16 = m_ft == kftUnicode8 && !IsParaInMem(pdpStart, iprStart);
	bool fAfter8to16 = m_ft == kftUnicode8 && !IsParaInMem(pdpStop, iprStop);

	UINT cbBefore = (char *)pvStart - (char *)pdpStart->rgpv[iprStart];
	UINT cbAfter = (char *)pdpStop->rgpv[iprStop] + CharsInPara(pdpStop, iprStop) - (char *)pvStop;
	UINT cchBefore = cbBefore;
	UINT cchAfter = cbAfter;
	if (m_ft != kftAnsi)
	{
		if (fBefore8to16)
			cbBefore *= 2;
		else
			cchBefore = cbBefore / 2;

		if (fAfter8to16)
		{
			cbAfter *= 2;
		}
		else
		{
			cbAfter += CharsInPara(pdpStop, iprStop);
			cchAfter = cbAfter / 2;
		}
	}

#ifdef _DEBUG
	UINT cchDeleted = 0;
#endif

	// If the part being deleted from the first paragraph is a LF from a CRLF combination,
	// take care of that case here, and adjust the start of the selection to the new paragraph.
	if (cchBefore > PrintCharsInPara(pdpStart, iprStart))
	{
		pdpStart->rgcch[iprStart] = (pdpStart->rgcch[iprStart] & ~kpmEndOfPara) | kpmOneEndOfPara;
		pdpStart->cch--;
		cchBefore = cbBefore = 0;
		if (++iprStart >= pdpStart->cpr)
		{
			pdpStart = pdpStart->pdpNext;
			AssertPtr(pdpStart);
			iprStart = 0;
		}
#ifdef _DEBUG
		cchDeleted++;
#endif
	}

	// Create a new paragraph from the beginning of the first paragraph and the
	// end of the final paragraph and replace the first paragraph with it.
	if (cbBefore || cbAfter)
	{
		char * prgch = new char[cbBefore + cbAfter];
		if (!prgch)
			return false;
		if (fBefore8to16)
			Convert8to16((char *)pdpStart->rgpv[iprStart], (wchar *)prgch, cchBefore);
		else
			memmove(prgch, pdpStart->rgpv[iprStart], cbBefore);
		if (fAfter8to16)
			Convert8to16((char *)pvStop, (wchar *)prgch + cchBefore, cchAfter);
		else
			memmove(prgch + cbBefore, pvStop, cbAfter);
		if (IsParaInMem(pdpStart, iprStart))
			delete pdpStart->rgpv[iprStart];
		pdpStart->rgpv[iprStart] = prgch;
		UINT cchAtEnd = CharsAtEnd(pdpStop, iprStop);
		// This is needed when deleting a single CR in a CRLF combination.
		if (pdpStart == pdpStop && iprStart == iprStop && cchAfter < cchAtEnd)
			cchAtEnd -= cchAfter;
		pdpStart->cch += cchAfter - (CharsInPara(pdpStart, iprStart) - cchBefore);
#ifdef _DEBUG
		cchDeleted += (CharsInPara(pdpStart, iprStart) - cchBefore) - cchAfter;
#endif
		pdpStart->rgcch[iprStart] = cchBefore + cchAfter - cchAtEnd + kpmParaInMem + (cchAtEnd << kpmEndOfParaShift);
	}

	if (iprStart != iprStop || pdpStart != pdpStop)
	{
		// The selection is in more than one paragraph.
		CWaitCursor wc;

		if (cbBefore || cbAfter)
		{
			// We already created the new paragraph that will replace the first paragraph, so
			// start deleting at the next paragraph.
			if (++iprStart >= pdpStart->cpr)
			{
				pdpStart = pdpStart->pdpNext;
				AssertPtr(pdpStart);
				iprStart = 0;
			}
		}
		if (cbBefore || cbAfter || pvStop != pdpStop->rgpv[iprStop])
		{
			// We have already moved everything we need from the last paragraph to the first paragraph,
			// so delete the whole paragraph.
			if (++iprStop >= pdpStop->cpr)
			{
				pdpStop = pdpStop->pdpNext;
				AssertPtrN(pdpStop);
				iprStop = 0;
			}
		}
		// The start and end points to delete are both on paragraph boundaries.

		DocPart * pdpLast = NULL;
		while (pdpStart != pdpStop)
		{
			// Loop until we reach the final part
			AssertPtr(pdpStart);
			UINT cpr = pdpStart->cpr;
			for (UINT ipr = iprStart; ipr < cpr; ipr++)
			{
				if (IsParaInMem(pdpStart, ipr))
					delete pdpStart->rgpv[ipr];
#ifdef _DEBUG
				cchDeleted += CharsInPara(pdpStart, ipr);
#endif
				pdpStart->cch -= CharsInPara(pdpStart, ipr);
			}

			// Delete the entire part from the linked list under the following conditions:
			// 1. We've deleted every paragraph in the part.
			// 2. This is not the only part in the linked list
			// 3. This is not the last part in the linked list and the previous part is full and
			//    this part does not have any characters in it.
			if (iprStart == 0 &&
				(pdpStart->pdpPrev || pdpStart->pdpNext) &&
				!(!pdpStart->pdpNext && pdpStart->pdpPrev && pdpStart->pdpPrev->cpr == kcprInPart && pdpStart->cch))
			{
				// Delete the entire part from the linked list.
				Assert(pdpStart->cch == 0);
				if (pdpStart->pdpPrev)
					pdpStart->pdpPrev->pdpNext = pdpStart->pdpNext;
				else
					m_pdpFirst = pdpStart->pdpNext;
				if (pdpStart->pdpNext)
					pdpStart->pdpNext->pdpPrev = pdpStart->pdpPrev;
				DocPart * pdpT = pdpStart;
				pdpStart = pdpStart->pdpNext;
				delete pdpT;
				m_cpr -= cpr;
			}
			else
			{
				// Adjust the statistics stored in the part.
				memset(&pdpStart->rgpv[iprStart], 0, sizeof(void *) * (kcprInPart - iprStart));
				memset(&pdpStart->rgcch[iprStart], 0, sizeof(UINT) * (kcprInPart - iprStart));
				pdpStart->cpr = iprStart;
				Assert(m_cpr >= cpr - iprStart);
				m_cpr -= (cpr - iprStart);
				pdpLast = pdpStart;
				pdpStart = pdpStart->pdpNext;
				// 2005-09-08 DZ - I don't think this Assert is correct. If we're deleting something
				// across two parts, pdpStop can be adjusted to the next part if it's the last para
				// of the second part, so pdpStart will not always equal pdpStop.
				/*Assert(pdpStart == pdpStop ||
					(pdpStop == NULL && iprStop == 0 && pdpStart->pdpNext == NULL));*/
			}
			AssertPtrN(pdpStart);
			iprStart = 0;
		}

		if (iprStart < iprStop)
		{
			// Delete until we reach the final paragraph
			AssertPtr(pdpStart);
			Assert(pdpStart == pdpStop);
			Assert(m_cpr >= iprStop - iprStart);
			pdpStart->cpr -= (iprStop - iprStart);
			m_cpr -= (iprStop - iprStart);
			for (UINT ipr = iprStart; ipr < iprStop; ipr++)
			{
				if (IsParaInMem(pdpStart, ipr))
					delete pdpStart->rgpv[ipr];
#ifdef _DEBUG
				cchDeleted += CharsInPara(pdpStart, ipr);
#endif
				pdpStart->cch -= CharsInPara(pdpStart, ipr);
			}
			memmove(&pdpStart->rgpv[iprStart], &pdpStart->rgpv[iprStop], sizeof(void *) * (kcprInPart - iprStop));
			memmove(&pdpStart->rgcch[iprStart], &pdpStart->rgcch[iprStop], sizeof(UINT) * (kcprInPart - iprStop));
			memset(&pdpStart->rgpv[pdpStart->cpr], 0, sizeof(void *) * (kcprInPart - pdpStart->cpr));
			memset(&pdpStart->rgcch[pdpStart->cpr], 0, sizeof(UINT) * (kcprInPart - pdpStart->cpr));
		}

		if (pdpLast && !pdpLast->pdpNext && !pdpStop && !cbBefore && !cbAfter)
		{
			// Account for the extra empty paragraph at the end of the file.
			pdpLast->cpr++;
			m_cpr++;
		}
	}
	else if (cbBefore == 0 && cbAfter == 0 && CharsInPara(pdpStart, iprStart) > 0)
	{
		// The selection was within the last paragraph and the whole paragraph was deleted.
		Assert(pdpStop && !pdpStop->pdpNext && iprStart == pdpStart->cpr - 1);
		pdpStart->rgcch[iprStart] -= (ichStop - ichMin);
		pdpStart->cch -= (ichStop - ichMin);
#ifdef _DEBUG
		cchDeleted += (ichStop - ichMin);
#endif
	}
	m_cch -= (ichStop - ichMin);
	Assert(cchDeleted == ichStop - ichMin);

	Assert(m_cpr > 0);
	/*if (m_cpr == 0)
	{
		Assert(false);
		AssertPtr(m_pdpFirst);
		Assert(!m_pdpFirst->pdpPrev && !m_pdpFirst->pdpNext && !m_cch && !m_pdpFirst->cch && !m_pdpFirst->cpr);
		m_pdpFirst->cpr = m_cpr = 1;
	}*/

	AssertPtr(m_pzf);
	m_pzf->NotifyDelete(ichMin, ichStop, fRedraw);

	VerifyMemory();

	return true;
}


/*----------------------------------------------------------------------------------------------
	If this returns true, pfi contains information about the match.
----------------------------------------------------------------------------------------------*/
bool CZDoc::Find(FindItem * pfi)
{
	AssertPtr(pfi);
	AssertPtr(pfi->pwszFindWhat);
	if (m_fAccessError)
		return false;

	if (pfi->ichStart > m_cch)
		pfi->ichStart = m_cch;
	if (pfi->ichStop > m_cch)
		pfi->ichStop = m_cch;
	CFindText ft(this);
	return ft.Find(pfi);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UINT CZDoc::DoReplaceAll(FindItem * pfi)
{
	AssertPtr(pfi);
	UINT cReplaced = 0;
	UINT ichSelStart;
	GetSelection(&ichSelStart, NULL);
	CUndoRedo * pur = new CUndoRedo(kurtReplaceAll, ichSelStart, 1, 1);
	if (!pur)
		return 0;
	ClearRedo();

	// Get dimensions for progress bar.
	HWND hwndStatus = m_pzef->GetStatusHwnd();
	RECT rect;
	::SendMessage(hwndStatus, SB_GETRECT, ksboMessage, (LPARAM)&rect);
	::InflateRect(&rect, -1, -2);
	HWND hwndProgress = ::CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, NULL,
		WS_CHILD | WS_VISIBLE, rect.left, rect.top, rect.right - rect.left,
		rect.bottom - rect.top, hwndStatus, 0, 0, NULL);
	UINT iFirstStart = pfi->ichStart;
	if (pfi->ichStop == -1)
		pfi->ichStop = m_cch;
	::SendMessage(hwndProgress, PBM_SETRANGE32, iFirstStart, pfi->ichStop);
	int cchReplace = pfi->cchReplaceWith;
	CUndoRedo::ReplaceAllInfo *& prai = pur->m_prai;
	while (Find(pfi))
	{
		if (!pfi->fFindIsReplace)
		{
			if (!prai)
				prai = new CUndoRedo::ReplaceAllInfo();
			UINT izdo = prai->AddReplaceAllItem(this, pfi->ichMatch, pfi->cchMatch, true);
			if (izdo == -1)
			{
				::MessageBox(m_pzef->GetHwnd(),
					"An error occurred while performing the Replace All operation.",
					g_pszAppName, MB_OK | MB_ICONEXCLAMATION);
				break;
			}
			prai->m_prgral[cReplaced].m_cch = cchReplace;
			InsertText(pfi->ichMatch, pfi->pwszReplaceWith, cchReplace, pfi->cchMatch,
				false, kurtEmpty);
		}
		pfi->ichStart = pfi->ichMatch + cchReplace;
		pfi->ichStop += cchReplace - pfi->cchMatch;
		if (!(cReplaced % 50))
		{
			::SendMessage(hwndProgress, PBM_SETRANGE32, iFirstStart, pfi->ichStop);
			::SendMessage(hwndProgress, PBM_SETPOS, pfi->ichMatch, 0);
		}
		cReplaced++;
	}
	::DestroyWindow(hwndProgress);
	if (pur)
	{
		if (cReplaced == 0)
		{
			delete pur;
			pur = NULL;
		}
		else
		{
			m_stackUndo.push(pur);
		}
	}
	if (cReplaced > 0)
		SetSelection(pfi->ichMatch, pfi->ichMatch + pfi->cchReplaceWith, true, true);
	return cReplaced;
}


void CZDoc::VerifyMemory()
{
	DocPart * pdp = m_pdpFirst;
	while (pdp)
	{
		AssertPtr(pdp);
		Assert(pdp->cpr <= kcprInPart);
		UINT cchInPart = 0;
		for (UINT ipr = 0; ipr < pdp->cpr; ipr++)
		{
			void * pv = pdp->rgpv[ipr];
			UINT cch = CharsInPara(pdp, ipr);
			UINT cb = cch;
			if (!IsAnsi(pdp, ipr))
				cb *= 2;
			AssertArray((char *)pv, cb);

			cchInPart += cch;
		}
		Assert(cchInPart == pdp->cch);

		pdp = pdp->pdpNext;
	}
}

/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UINT CZDoc::GetMemoryUsed()
{
	DocPart * pdp = m_pdpFirst;
	int cb = 0;
	int cdp = 0;
	int ipr;

	while (pdp)
	{
		cdp++;
		for (ipr = pdp->cpr; --ipr >= 0; )
		{
			if (IsParaInMem(pdp, ipr))
				cb += (CharsInPara(pdp, ipr) << m_ft != kftAnsi);
		}
		pdp = pdp->pdpNext;
	}
	return cb + (cdp * sizeof(DocPart)) + sizeof(this);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UndoRedoType CZDoc::GetRedoType()
{
	if (m_stackRedo.empty())
		return kurtEmpty;
	return m_stackRedo.top()->m_urt;
}


/*----------------------------------------------------------------------------------------------
	If m_ft is kftAnsi, the text stored in ppv will be ANSI.
	Otherwise, it will be 16-bit Unicode.
	NOTE: This method allocates new memory for ppv. Make sure you free it
		when you're done using it.
----------------------------------------------------------------------------------------------*/
UINT CZDoc::GetText(UINT ichMin, UINT cch, void ** ppv)
{
	UINT cpr;
	void * pvLastPara;
	return GetTextCore(ichMin, cch, ppv, &cpr, &pvLastPara);
}


/*----------------------------------------------------------------------------------------------
	If m_ft is kftAnsi, the text stored in ppv will be ANSI.
	Otherwise, it will be 16-bit Unicode.
	Returns the number of characters that were copied.
----------------------------------------------------------------------------------------------*/
UINT CZDoc::GetText(UINT ichMin, UINT cch, CZDataObject ** ppzdo)
{
	if (cch == 0)
	{
		*ppzdo = NULL;
		return 0;
	}
	UINT cpr;
	void * pv;
	void * pvLastPara;
	cch = GetTextCore(ichMin, cch, &pv, &cpr, &pvLastPara);
	if (cch)
	{
		if (FAILED(CZDataObject::Create(pv, cch, m_ft == kftAnsi, cpr, pvLastPara, (IZDataObject **)ppzdo)))
			cch = 0;
		delete pv;
	}
	return cch;
}


/*----------------------------------------------------------------------------------------------
	If m_ft is kftAnsi, the text stored in ppv will be ANSI.
	Otherwise, it will be 16-bit Unicode.
	Returns the number of characters that were copied (cch).
	NOTE: This method allocates new memory for ppv. Make sure you free it
		when you're done using it.
----------------------------------------------------------------------------------------------*/
UINT CZDoc::GetTextCore(UINT ichMin, UINT cch, void ** ppv, UINT * pcpr, void ** ppvLastPara)
{
	AssertPtr(ppv);
	AssertPtr(pcpr);
	AssertPtr(ppvLastPara);
	Assert(ichMin + cch <= m_cch);

	*ppv = NULL;
	*pcpr = 0;
	*ppvLastPara = NULL;
	if (m_fAccessError || !cch)
		return 0;

	void * pv;
	void * pvLastPara;
	int cpr = 0;
	UINT ipr;
	DocPart * pdp = GetPart(ichMin, false, &pv, &ipr);
	UINT ichInPara;
	UINT cchToCopy = cch;
	UINT cchInPara;
	if (m_ft == kftAnsi)
	{
		char * prgchDst = new char[cch + 1]; // space for NULL at the end
		if (!prgchDst)
			return 0;
		*ppv = prgchDst;
		ichInPara = (char *)pv - (char *)pdp->rgpv[ipr];

		while (cchToCopy)
		{
			cpr++;
			pvLastPara = prgchDst;
			cchInPara = min(cchToCopy, CharsInPara(pdp, ipr) - ichInPara);
			memmove(prgchDst, pv, cchInPara);
			prgchDst += cchInPara;
			if ((cchToCopy -= cchInPara) == 0)
				break;
			if (++ipr >= pdp->cpr)
			{
				pdp = pdp->pdpNext;
				AssertPtr(pdp);
				ipr = 0;
			}
			pv = pdp->rgpv[ipr];
			ichInPara = 0;
		}
		Assert(prgchDst - (char *)*ppv == cch);
		*prgchDst = 0;
	}
	else
	{
		Assert(m_ft == kftUnicode16 || m_ft == kftUnicode8);
		wchar * prgwchDst = new wchar[cch + 1]; // space for NULL at the end
		if (!prgwchDst)
			return 0;
		*ppv = prgwchDst;
		if (m_ft == kftUnicode8 && !IsParaInMem(pdp, ipr))
			ichInPara = (char *)pv - (char *)pdp->rgpv[ipr];
		else
			ichInPara = (wchar *)pv - (wchar *)pdp->rgpv[ipr];

		while (cchToCopy)
		{
			cpr++;
			pvLastPara = prgwchDst;
			cchInPara = min(cchToCopy, CharsInPara(pdp, ipr) - ichInPara);
			if (m_ft == kftUnicode16 || IsParaInMem(pdp, ipr))
			{
				// Already Unicode, so just copy it to the buffer.
				memmove(prgwchDst, pv, cchInPara << 1);
			}
			else
			{
				// Need to convert to Unicode.
				Convert8to16((char *)pv, prgwchDst, cchInPara);
			}
			prgwchDst += cchInPara;
			if ((cchToCopy -= cchInPara) == 0)
				break;
			if (++ipr == pdp->cpr)
			{
				pdp = pdp->pdpNext;
				AssertPtr(pdp);
				ipr = 0;
			}
			pv = pdp->rgpv[ipr];
			ichInPara = 0;
		}
		Assert(prgwchDst - (wchar *)*ppv == cch);
		*prgwchDst = 0;
	}
	*pcpr = cpr;
	*ppvLastPara = pvLastPara;
	return cch;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
CUndoRedo * CZDoc::GetUndo()
{
	if (!m_stackUndo.empty())
		return m_stackUndo.top();
	Assert(false);
	return NULL;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UndoRedoType CZDoc::GetUndoType()
{
	if (m_stackUndo.empty())
		return kurtEmpty;
	return m_stackUndo.top()->m_urt;
}


/*----------------------------------------------------------------------------------------------
	The buffer pointed to be ppv must be freed by the caller if it is not NULL.
	The return value will be 0 if ppv is NULL.
----------------------------------------------------------------------------------------------*/
UINT CZDoc::GetWordAtChar(UINT ich, void ** ppv, UINT * pich, UINT * pcch)
{
	AssertPtr(ppv);
	AssertPtr(pich);
	AssertPtr(pcch);
	if (ppv)
		*ppv = NULL;
	if (pich)
		*pich = 0;
	if (pcch)
		*pcch = 0;
	void * pv;
	UINT ipr;
	DocPart * pdp = GetPart(ich, false, &pv, &ipr);
	UINT cch = 0;
	if (IsAnsi(pdp, ipr))
	{
		if (!pv || !isalpha(*((char *)pv)))
			return 0;

		char * prgchStart = (char *)pv;
		while (prgchStart > pdp->rgpv[ipr] && isalpha(*(--prgchStart)));
		if (prgchStart != pdp->rgpv[ipr] || !isalpha(*prgchStart))
			prgchStart++;

		char * prgchStop = (char *)pv;
		char * prgchLimit = (char *)pdp->rgpv[ipr] + PrintCharsInPara(pdp, ipr);
		while (prgchStop < prgchLimit && isalpha(*(++prgchStop)));

		cch = prgchStop - prgchStart;
		if (cch && ppv)
		{
			*ppv = new char[cch + 1];
			if (!*ppv)
				return 0;
			memmove(*ppv, prgchStart, cch);
			((char *)*ppv)[cch] = 0;
		}
		if (pich)
			*pich = ich + prgchStart - (char *)pv;
	}
	else
	{
		if (!pv || !iswalpha(*((wchar *)pv)))
			return 0;

		wchar * prgchStart = (wchar *)pv;
		while (prgchStart > pdp->rgpv[ipr] && iswalpha(*(--prgchStart)));
		if (prgchStart != pdp->rgpv[ipr] || !iswalpha(*prgchStart))
			prgchStart++;

		wchar * prgchStop = (wchar *)pv;
		wchar * prgchLimit = (wchar *)pdp->rgpv[ipr] + PrintCharsInPara(pdp, ipr);
		while (prgchStop < prgchLimit && iswalpha(*(++prgchStop)));

		cch = prgchStop - prgchStart;
		if (cch && ppv)
		{
			*ppv = new wchar[cch + 1];
			if (!*ppv)
				return 0;
			memmove(*ppv, prgchStart, cch << 1);
			((wchar *)*ppv)[cch] = 0;
		}
		if (pich)
			*pich = ich + prgchStart - (wchar *)pv;
	}
	if (pcch)
		*pcch = cch;
	return cch;
}


/*----------------------------------------------------------------------------------------------
	pur cannot be NULL. If pur->m_ichStart == -1, the undo entry has to be filled out; otherwise
	look at pur->m_pichLocations to see if the current paragraph should be changed (this is
	only for skipping a paragraph when undoing a decrease indent operation if the paragraph
	did not start with a tab).
----------------------------------------------------------------------------------------------*/
bool CZDoc::IndentParas(UINT iprStart, UINT iprStop, bool fIndent)
{
	if (iprStart > iprStop)
		return false;
	CUndoRedo * pur = new CUndoRedo(fIndent ? kurtIndentIncrease : kurtIndentDecrease, 0, 0, 0);
	if (!pur)
		return false;

	UINT ipr;
	UINT ichMax;
	UINT iprCurrent = iprStart;
	DocPart * pdp = GetPart(iprStop + 1, true, NULL, &ipr, &ichMax);
	for (UINT iprT = 0; iprT < ipr; iprT++)
		ichMax += CharsInPara(pdp, iprT);
	pur->m_cch = ichMax;

	UINT ichMin;
	pdp = GetPart(iprCurrent, true, NULL, &ipr, &ichMin);
	for (UINT iprT = 0; iprT < ipr; iprT++)
		ichMin += CharsInPara(pdp, iprT);
	int cchSpacesInTab = g_fg.m_ri.m_cchSpacesInTab;

	CUndoRedo::ReplaceAllInfo *& prai = pur->m_prai;
	UINT ich = ichMin;
	UINT iLocation = 0;
	while (iprCurrent++ <= iprStop)
	{
		// Only look at paragraphs that contain at least one character.
		UINT cchPara = CharsInPara(pdp, ipr);
		UINT cchPrintChars = PrintCharsInPara(pdp, ipr);
		if (cchPrintChars > 0)
		{
			int nShift = (IsAnsi(pdp, ipr)) ? 0 : 1;
			void * pvParaMin = pdp->rgpv[ipr];
			void * pvParaLim = (char *)pvParaMin + (cchPrintChars << nShift);

			// Get the number of spaces at the beginning of the paragraph.
			int cchSpaces = 0;
			int cchTabs = 0;
			while (pvParaMin < pvParaLim)
			{
				wchar ch = (nShift == 0) ? *((char *)pvParaMin) : *((wchar *)pvParaMin);
				if (ch == 32)
					cchSpaces++;
				else if (ch == 9)
					cchTabs++;
				else
					break;
				pvParaMin = (char *)pvParaMin + (1 << nShift);
			}

			if (!prai)
				prai = new CUndoRedo::ReplaceAllInfo();

			void * pvReplaceWith = NULL;
			int cchInsert = (cchSpaces / cchSpacesInTab); // # of spaces we can convert into tabs.
			int cchReplace = (cchInsert * cchSpacesInTab) + (cchSpaces % cchSpacesInTab);
			if (fIndent)
			{
				if (cchSpaces > 0 && cchTabs > 0)
				{
					cchReplace += cchTabs;
					cchInsert += cchTabs;
				}
				cchInsert++;
			}
			else
			{
				if (cchReplace == 0 && cchTabs > 0)
				{
					// The paragraph starts with a tab, so we can remove it.
					cchReplace = 1;
				}
			}

			if (cchInsert > 0)
			{
				int i = 0;
				if (nShift == 0)
				{
					pvReplaceWith = new char[cchInsert + 1];
					for ( ; i < cchInsert; i++)
						((char *)pvReplaceWith)[i] = 9;
					((char *)pvReplaceWith)[i] = 0;
				}
				else
				{
					pvReplaceWith = new wchar[cchInsert + 1];
					for ( ; i < cchInsert; i++)
						((wchar *)pvReplaceWith)[i] = 9;
					((wchar *)pvReplaceWith)[i] = 0;
				}
			}

			UINT izdo = prai->AddReplaceAllItem(this, ich, cchReplace, true);
			if (izdo == -1)
			{
				::MessageBox(m_pzef->GetHwnd(),
					"An error occurred while performing the Indent Paragraphs operation.",
					g_pszAppName, MB_OK | MB_ICONEXCLAMATION);
				delete pur;
				return false;
			}
			prai->m_prgral[iLocation].m_cch = cchInsert;
			InsertText(ich, pvReplaceWith, cchInsert, cchReplace, nShift == 0, kurtEmpty);
			if (pvReplaceWith)
				delete pvReplaceWith;

			ichMax += (cchInsert - cchReplace);
			iLocation++;
		}

		ich += CharsInPara(pdp, ipr);

		// Move to the next paragraph.
		if (++ipr >= pdp->cpr)
		{
			pdp = pdp->pdpNext;
			if (!pdp)
			{
				if (iprCurrent <= iprStop)
				{
					delete pur;
					return false;
				}
			}
			ipr = 0;
		}
	}
	if (ichMax < m_cch)
	{
		pur->m_cch--;
		ichMax--;
	}
	pur->m_ichMin = ichMin;
	pur->m_cchReplaced = ichMax;
	m_stackUndo.push(pur);
	ClearRedo();
	m_fModified = true;
	SetSelection(ichMin, ichMax, true, true);
	return true;
}


/*----------------------------------------------------------------------------------------------
	Returns the number of characters inserted into the document.
----------------------------------------------------------------------------------------------*/
int CZDoc::InsertText(UINT ich, CZDataObject * pzdo, UINT cchReplace, UndoRedoType urt)
{
	AssertPtrN(pzdo);

	if (!pzdo)
		return InsertText(ich, NULL, 0, cchReplace, true, urt);
	return InsertText(ich, pzdo->m_pv, pzdo->m_cb >> (1 - pzdo->m_fAnsi), cchReplace,
		pzdo->m_fAnsi, urt);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CZDoc::InsertText(UINT ich, void * pvInsert, UINT cch, UINT cchReplace, bool fAnsi,
	UndoRedoType urt)
{
#ifdef _DEBUG
	if (m_ft == kftAnsi)
		AssertArray((char *)pvInsert, cch);
	else
		AssertArray((char *)pvInsert, cch);
	Assert(cch >= 0);
#endif

	if ((cch == 0 && !cchReplace) || m_fAccessError)
		return 0;

	ZSmartPtr zsp;
	if (m_ft == kftAnsi)
	{
		if (!fAnsi)
		{
			void * pvNew = new char[cch];
			if (!pvNew)
				return 0;
			Convert16to8((wchar *)pvInsert, (char *)pvNew, cch);
			pvInsert = pvNew;
			zsp.Attach(pvInsert);
		}
	}
	else if (fAnsi)
	{
		void * pvNew = new wchar[cch];
		if (!pvNew)
			return 0;
		Convert8to16((char *)pvInsert, (wchar *)pvNew, cch);
		pvInsert = pvNew;
		zsp.Attach(pvInsert);
	}
	// Create an CUndoRedo entry if needed
	CUndoRedo * pur = NULL;
	if (urt == kurtPaste || urt == kurtReplace/* || urt == kurtSpell*/)
	{
		CZDataObject * pzdo = NULL;
		if (cchReplace)
		{
			UINT cch = GetText(ich, cchReplace, &pzdo);
			Assert(cch == cchReplace);
			if (!pzdo && cchReplace)
				return 0;
		}
		pur = new CUndoRedo(urt, ich, -1, cchReplace);
		if (!pur)
		{
			if (pzdo)
				pzdo->Release();
			return 0;
		}
		pur->m_pzdo = pzdo;
	}
	else if (urt == kurtMove)
	{
		// Other program to ZEdit (Copy and Move)
		if (NULL != (pur = new CUndoRedo(kurtMove, ich, -1, 0)))
			pur->m_mt = kmtPaste;
	}
	else if (urt == kurtMove_1)
	{
		// ZEdit to ZEdit (Move)
		// We are using cchReplace here to store the index of where the text came from.
		if (NULL != (pur = new CUndoRedo(kurtMove, ich, -1, cchReplace)))
			pur->m_mt = kmtPasteCut;
		cchReplace = 0;
	}
	else if (urt == kurtMove_2)
	{
		// ZEdit to ZEdit (Copy)
		// We are using cchReplace here to store the index of where the text came from.
		//if (NULL != (pur = new CUndoRedo(kurtMove, dwStart, -1, m_dwStartSelection)))
		if (NULL != (pur = new CUndoRedo(kurtMove, ich, -1, cchReplace)))
			pur->m_mt = kmtCopy;
		cchReplace = 0;
	}
	if (urt != kurtEmpty)
		ClearRedo();
	m_fModified = true;

	if (cchReplace)
		DeleteText(ich, ich + cchReplace, false, false);

	if (cch)
	{
		int nCharSize = 0;
		if (m_ft != kftAnsi)
			nCharSize = 1;

#ifdef _DEBUG
		DocPart * pdpStart = NULL;
#endif
		DocPart * pdp = NULL;
		UINT cchOld = m_cch;
		UINT iprStart;
		UINT iNewPara = 0;
		if (m_ft == kftAnsi)
		{
			char * prgchInsert = (char *)pvInsert;
			char * prgch = prgchInsert;
			char * prgchLim = prgch + cch;
			char * prgchSelStart;
			pdp = GetPart(ich, false, (void **)&prgchSelStart, &iprStart);
#ifdef _DEBUG
			pdpStart = pdp;
#endif
			int cchBefore = prgchSelStart - (char *)pdp->rgpv[iprStart];
			int cchPara = CharsInPara(pdp, iprStart);
			int cchAfter = cchPara - cchBefore;

			if (*prgch == 10 &&
				(cchBefore == 0 || prgchSelStart[-1] == 13) &&
				(iprStart > 0 || pdp->pdpPrev))
			{
				// The insert text starts with a LF and is being inserted right after a CR,
				// so don't insert a new paragraph.
				UINT iprT = iprStart;
				DocPart * pdpT = pdp;
				if (iprT-- == 0)
				{
					pdpT = pdpT->pdpPrev;
					AssertPtr(pdpT);
					iprT = pdpT->cpr - 1;
				}

				UINT cchParaT = CharsInPara(pdpT, iprT);
				if (!ChangeParaSize(pdpT, iprT, cchParaT + 1))
					return 0;
				*((char *)pdpT->rgpv[iprT] + cchParaT) = 10;
				pdpT->cch++;
				m_cch++;
				pdpT->rgcch[iprT] = (pdpT->rgcch[iprT] & ~kpmEndOfPara) |
					(2 << kpmEndOfParaShift);

				prgchInsert = ++prgch;
			}

			// TODO: Fix this section.
			/*if (prgchLim[-1] == 13 &&
				cchAfter == 1 &&
				(iprStart < pdp->cpr || pdp->pdpNext))
			{
				// The insert text ends with a CR and is being inserted right before a LF,
				// so don't insert a new paragraph.
				UINT iprT = iprStart;
				DocPart * pdpT = pdp;
				if (++iprT >= pdpT->cpr)
				{
					pdpT = pdpT->pdpNext;
					AssertPtr(pdpT);
					iprT = 0;
				}

				UINT cchParaT = CharsInPara(pdpT, iprT);
				if (!ChangeParaSize(pdpT, iprT, cchParaT + 1))
					return 0;
				*((char *)pdpT->rgpv[iprT] + cchParaT) = 10;
				pdpT->cch++;
				m_cch++;
				pdpT->rgcch[iprT] = (pdpT->rgcch[iprT] & ~kpmEndOfPara) |
					(2 << kpmEndOfParaShift);

				prgchLim--;
			}*/

			int cchEndOfInsert = 0;
			int cchEndOfFirstPara = CharsAtEnd(pdp, iprStart);
			int cchInsert;
			if (prgch < prgchLim)
			{
				// Find the end of the first paragraph in the text to insert.
				while (prgch < prgchLim && (*prgch > 13 || (*prgch != 13 && *prgch != 10)))
					prgch++;
				cchEndOfInsert = 1;
				if (prgch == prgchLim)
				{
					// The insert text does not contain any CR or LF characters so it will fit in one para.
					cchEndOfInsert = 0;
				}
				else if (++prgch < prgchLim && prgch[-1] == 13 && *prgch == 10)
				{
					// The insert text contains a CR-LF combination, meaning a paragraph break.
					prgch++;
					cchEndOfInsert = 2;
				}
				if (prgch > prgchLim)
					prgch = prgchLim;
				cchInsert = prgch - prgchInsert;
				Assert(cchInsert > 0);
				if (IsParaInMem(pdp, iprStart))
				{
					void * pv = realloc(pdp->rgpv[iprStart], cchPara + cchInsert);
					if (!pv)
						return 0;
					pdp->rgpv[iprStart] = pv;
				}
				else
				{
					void * pv = new char[cchPara + cchInsert];
					if (!pv)
						return 0;
					memmove(pv, pdp->rgpv[iprStart], cchPara);
					pdp->rgpv[iprStart] = pv;
					pdp->rgcch[iprStart] |= kpmParaInMem;
				}
				char * prgchT = (char *)pdp->rgpv[iprStart] + cchBefore;
				prgchSelStart = prgchT + cchInsert;
				memmove(prgchSelStart, prgchT, cchAfter);
				memmove(prgchT, prgchInsert, cchInsert);
				pdp->rgcch[iprStart] += cchInsert;
				pdp->cch += cchInsert;
				m_cch += cchInsert;
			}

			if (cchEndOfInsert != 0)
			{
				// We are inserting more than one paragraph.
				CWaitCursor wc;
				// The next line replaces the end of para count of the first paragraph to be
				// the end of para count of the insertion text.
				// Basically, we're replacing the end of para count, subtracting the characters
				// that will be moved to the start of a new paragraph, and subtracting the end of para
				// count, since it was included up above in cchInsert.
				pdp->rgcch[iprStart] = ((pdp->rgcch[iprStart] & ~kpmEndOfPara) |
					(cchEndOfInsert << kpmEndOfParaShift)) -
					(cchAfter - cchEndOfFirstPara) - cchEndOfInsert;
				pdp->cch -= cchAfter;

				// Move everything after the current paragraph in this part to a new part.
				// If iprStart >= pdp->cpr, everything after the current paragraph
				// is already in a new part, so we don't have to do anything.
				if (++iprStart < pdp->cpr)
				{
					DocPart * pdpNew = new DocPart(pdp, pdp->pdpNext);
					if (!pdpNew)
						return 0;

					UINT cprToMove = pdp->cpr - iprStart;
					memmove(pdpNew->rgpv, &pdp->rgpv[iprStart], sizeof(void *) * cprToMove);
					memmove(pdpNew->rgcch, &pdp->rgcch[iprStart], sizeof(UINT) * cprToMove);
					memset(&pdp->rgpv[iprStart], 0, sizeof(void *) * cprToMove);
					memset(&pdp->rgcch[iprStart], 0, sizeof(UINT) * cprToMove);
					pdpNew->cpr = cprToMove;
					pdp->cpr = iprStart;
					for (UINT ipr = 0; ipr < cprToMove; ipr++)
					{
						int cchInPara = CharsInPara(pdpNew, ipr);
						pdp->cch -= cchInPara;
						pdpNew->cch += cchInPara;
					}
				}

				// Copy the new paragraphs into pdpNew starting at iprStart.
				UINT iprNew = iprStart - 1;
				int cchEndOfPara = 0;
				if (prgch != prgchLim)
				{
					while (prgch < prgchLim)
					{
						cchEndOfPara = 1;
						prgchInsert = prgch;
						while (prgch < prgchLim && (*prgch > 13 || (*prgch != 13 && *prgch != 10)))
							prgch++;
						if (prgch == prgchLim)
						{
							cchEndOfPara = 0;
						}
						// TODO: Fix this code here.
						/*else if (prgch == prgchLim - 1 && *prgch == 13 && *prgchSelStart == 10)
						{
							// The insert text ends with a CR and is being inserted right before a LF,
							// so don't insert a new paragraph.
							prgch++;
							cchEndOfInsert = 0;
							// Adjust the character count, so adding cchInsert below will bring about the
							// correct value.
							// Basically, we are changing the end of para count to 2 instead of 1, and
							// subtracting the 1 (cchInsert) that will be added on below.
							pdp->rgcch[iprStart] = ((pdp->rgcch[iprStart] & ~kpmEndOfPara) - 1) |
								(2 << kpmEndOfParaShift);
						}*/
						else if (++prgch < prgchLim && *prgch == 10 && prgch[-1] == 13)
						{
							cchEndOfPara = 2;
							prgch++;
						}
						if (prgch > prgchLim)
							prgch = prgchLim;
						cchInsert = prgch - prgchInsert;
						if (++iprNew >= kcprInPart)
						{
							Assert(pdp->cpr == kcprInPart);
							pdp = new DocPart(pdp, pdp->pdpNext);
							if (!pdp)
								return 0;
							iprNew = 0;
						}
						Assert(cchInsert > 0);
						char * prgchNewPara = new char[cchInsert];
						if (!prgchNewPara)
							return 0;
						pdp->rgpv[iprNew] = prgchNewPara;
						memmove(prgchNewPara, prgchInsert, cchInsert);
						pdp->rgcch[iprNew] = cchInsert - cchEndOfPara + kpmParaInMem +
							(cchEndOfPara << kpmEndOfParaShift);

						pdp->cpr++;
						m_cpr++;
						pdp->cch += cchInsert;
						m_cch += cchInsert;
					}
				}
				else
				{
					if (++iprNew >= kcprInPart)
					{
						pdp = new DocPart(pdp, pdp->pdpNext);
						if (!pdp)
							return 0;
						iprNew = 0;
					}
					cchInsert = 0;
					pdp->cpr++;
					m_cpr++;
				}
				if (cchEndOfPara)
				{
					if (pdp->cpr >= kcprInPart)
					{
						pdp = new DocPart(pdp, pdp->pdpNext);
						if (!pdp)
							return 0;
						iprNew = 0;
					}
					pdp->cpr++;
					m_cpr++;
				}

				if (cchAfter)
				{
					// Move the end of the previous paragraph into the first paragraph in the new part.
					Assert(pdp->rgcch[iprNew] == 0 || IsParaInMem(pdp, iprNew));
					if (cchEndOfPara)
					{
						if (++iprNew >= kcprInPart)
						{
							pdp = new DocPart(pdp, pdp->pdpNext);
							if (!pdp)
								return 0;
							iprNew = 0;
						}
						cchInsert = 0;
					}
					void * pvNewPara = realloc(pdp->rgpv[iprNew], cchInsert + cchAfter);
					if (!pvNewPara)
						return 0;
					pdp->rgpv[iprNew] = pvNewPara;
					memmove((char *)pvNewPara + cchInsert, prgchSelStart, cchAfter);
					pdp->cch += cchAfter;
					if (pdp->rgcch[iprNew] == 0)
						pdp->rgcch[iprNew] |= kpmParaInMem;
					pdp->rgcch[iprNew] += (cchAfter - cchEndOfFirstPara) + (cchEndOfFirstPara << kpmEndOfParaShift);
				}
			}
		}
		else
		{
			wchar * prgchInsert = (wchar *)pvInsert;
			wchar * prgch = prgchInsert;
			wchar * prgchLim = prgch + cch;
			wchar * prgchSelStart;
			pdp = GetPart(ich, false, (void **)&prgchSelStart, &iprStart);
#ifdef _DEBUG
			pdpStart = pdp;
#endif
			// Find the end of the first paragraph in the text to insert.
			while (prgch < prgchLim && (*prgch > 13 || (*prgch != 13 && *prgch != 10)))
				prgch++;
			UINT cchEndOfFirstPara = CharsAtEnd(pdp, iprStart);
			int cchEndOfInsert = 1;
			if (prgch == prgchLim)
			{
				// The insert text does not contain any CR or LF characters so it will fit in one para.
				cchEndOfInsert = 0;
			}
			else if (++prgch < prgchLim && *prgch == 10 && prgch[-1] == 13)
			{
				// The insert text contains a CR-LF combination, meaning a paragraph break.
				prgch++;
				cchEndOfInsert = 2;
			}
			if (prgch > prgchLim)
				prgch = prgchLim;
			int cchInsert = prgch - prgchInsert;
			Assert(cchInsert > 0);

			bool fFirstParaIsAnsi = false;
			if (m_ft == kftUnicode8 && !IsParaInMem(pdp, iprStart))
				fFirstParaIsAnsi = true;
			int cchBefore;
			if (fFirstParaIsAnsi)
				cchBefore = (char *)prgchSelStart - (char *)pdp->rgpv[iprStart];
			else
				cchBefore = prgchSelStart - (wchar *)pdp->rgpv[iprStart];
			int cchPara = CharsInPara(pdp, iprStart);
			int cchAfter = cchPara - cchBefore;

			if (IsParaInMem(pdp, iprStart))
			{
				void * pv = realloc(pdp->rgpv[iprStart], (cchPara + cchInsert) * 2);
				if (!pv)
					return 0;
				pdp->rgpv[iprStart] = pv;
			}
			else
			{
				void * pv = new wchar[cchPara + cchInsert];
				if (!pv)
					return 0;
				if (fFirstParaIsAnsi)
					Convert8to16((char *)pdp->rgpv[iprStart], (wchar *)pv, cchPara);
				else
					memmove(pv, pdp->rgpv[iprStart], cchPara * 2);
				pdp->rgpv[iprStart] = pv;
				pdp->rgcch[iprStart] |= kpmParaInMem;
			}
			wchar * prgchT = (wchar *)pdp->rgpv[iprStart] + cchBefore;
			prgchSelStart = prgchT + cchInsert;
			memmove(prgchSelStart, prgchT, cchAfter * 2);
			memmove(prgchT, prgchInsert, cchInsert * 2);
			pdp->rgcch[iprStart] += cchInsert;
			pdp->cch += cchInsert;
			m_cch += cchInsert;

			if (cchEndOfInsert != 0)
			{
				// We are inserting more than one paragraph.
				CWaitCursor wc;
				// The next line replaces the end of para count of the first paragraph to be
				// the end of para count of the insertion text.
				pdp->rgcch[iprStart] = ((pdp->rgcch[iprStart] & ~kpmEndOfPara) |
					(cchEndOfInsert << kpmEndOfParaShift)) - (cchAfter - cchEndOfFirstPara) - cchEndOfInsert;
				pdp->cch -= cchAfter;

				if (++iprStart < pdp->cpr)
				{
					// Move everything after the current paragraph into a new part.
					DocPart * pdpNew = new DocPart(pdp, pdp->pdpNext);
					if (!pdpNew)
						return 0;

					UINT cprToMove = pdp->cpr - iprStart;
					memmove(pdpNew->rgpv, &pdp->rgpv[iprStart], sizeof(void *) * cprToMove);
					memmove(pdpNew->rgcch, &pdp->rgcch[iprStart], sizeof(UINT) * cprToMove);
					memset(&pdp->rgpv[iprStart], 0, sizeof(void *) * cprToMove);
					memset(&pdp->rgcch[iprStart], 0, sizeof(UINT) * cprToMove);
					pdpNew->cpr = cprToMove;
					pdp->cpr = iprStart;
					for (UINT ipr = 0; ipr < cprToMove; ipr++)
					{
						int cchInPara = CharsInPara(pdpNew, ipr);
						pdp->cch -= cchInPara;
						pdpNew->cch += cchInPara;
					}
				}

				// Copy the new paragraphs into pdpNew
				UINT iprNew = iprStart - 1;
				int cchEndOfPara = 0;
				if (prgch != prgchLim)
				{
					while (prgch < prgchLim)
					{
						cchEndOfPara = 1;
						prgchInsert = prgch;
						while (prgch < prgchLim && (*prgch > 13 || (*prgch != 13 && *prgch != 10)))
							prgch++;
						if (prgch == prgchLim)
						{
							cchEndOfPara = 0;
						}
						else if (++prgch < prgchLim && *prgch == 10 && prgch[-1] == 13)
						{
							cchEndOfPara = 2;
							prgch++;
						}
						if (prgch > prgchLim)
							prgch = prgchLim;
						cchInsert = prgch - prgchInsert;
						if (++iprNew >= kcprInPart)
						{
							Assert(pdp->cpr == kcprInPart);
							pdp = new DocPart(pdp, pdp->pdpNext);
							if (!pdp)
								return 0;
							iprNew = 0;
						}
						Assert(cchInsert > 0);
						wchar * prgchNewPara = new wchar[cchInsert];
						if (!prgchNewPara)
							return 0;
						pdp->rgpv[iprNew] = prgchNewPara;
						memmove(prgchNewPara, prgchInsert, cchInsert * 2);
						pdp->rgcch[iprNew] = cchInsert - cchEndOfPara + kpmParaInMem +
							(cchEndOfPara << kpmEndOfParaShift);

						pdp->cpr++;
						m_cpr++;
						pdp->cch += cchInsert;
						m_cch += cchInsert;
					}
				}
				else
				{
					if (++iprNew >= kcprInPart)
					{
						pdp = new DocPart(pdp, pdp->pdpNext);
						if (!pdp)
							return 0;
						iprNew = 0;
					}
					cchInsert = 0;
					pdp->cpr++;
					m_cpr++;
				}
				if (cchEndOfPara)
				{
					pdp->cpr++;
					m_cpr++;
				}

				if (cchAfter)
				{
					// Move the end of the previous paragraph into the first paragraph in the new part.
					Assert(pdp->rgcch[iprNew] == 0 || IsParaInMem(pdp, iprNew));
					if (cchEndOfPara)
					{
						if (++iprNew >= kcprInPart)
						{
							pdp = new DocPart(pdp, pdp->pdpNext);
							if (!pdp)
								return 0;
							iprNew = 0;
						}
						cchInsert = 0;
					}
					void * pvNewPara = realloc(pdp->rgpv[iprNew], (cchInsert + cchAfter) * 2);
					if (!pvNewPara)
						return 0;
					pdp->rgpv[iprNew] = pvNewPara;
					memmove((wchar *)pvNewPara + cchInsert, prgchSelStart, cchAfter * 2);
					pdp->cch += cchAfter;
					if (pdp->rgcch[iprNew] == 0)
					{
						// TODO: Is this right? Look above in the other section.
						pdp->rgcch[iprNew] |= kpmParaInMem;
					}
					pdp->rgcch[iprNew] += (cchAfter - cchEndOfFirstPara) + (cchEndOfFirstPara << kpmEndOfParaShift);
				}
			}
		}

		Assert(m_cpr > 0);
#ifdef _DEBUG
		DocPart * pdpT = pdpStart;
		pdp = pdp->pdpNext;
		while (pdpT != pdp)
		{
			AssertPtr(pdpT);
			UINT iPartInPara = -1;
			UINT cchInPart = 0;
			Assert(pdpT->cpr <= kcprInPart);
			while (++iPartInPara < pdpT->cpr)
				cchInPart += CharsInPara(pdpT, iPartInPara);
			Assert(cchInPart == pdpT->cch);
			pdpT = pdpT->pdpNext;
		}
		Assert(cch == m_cch - cchOld);
#endif
		AssertPtr(m_pzf);
		m_pzf->NotifyInsert(ich, cch);
	}

	// Finish the CUndoRedo entry if needed
	if (pur)
	{
		pur->m_cch = cch;

		AssertPtr(m_pzvCurrent);
		if (pur->m_urt == kurtMove)// && pur->m_mt == kmtCopy)
			m_pzvCurrent->SetSelection(ich, ich + cch, true, true);
		else
			m_pzvCurrent->SetSelection(ich + cch, ich + cch, true, true);

		m_stackUndo.push(pur);
	}

	VerifyMemory();

	return cch;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZDoc::OnColumnMarkerDrag()
{
	AssertPtr(m_pzf);
	m_pzf->OnColumnMarkerDrag();
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::OnChar(UINT ichSelStart, UINT ichSelStop, UINT ch, bool fOvertype,
	bool & fOnlyEditing)
{
	if (ch == VK_BACK || ch == VK_ESCAPE)
		return true;

	int nShift = ::GetAsyncKeyState(VK_SHIFT);
	bool fControl = ::GetAsyncKeyState(VK_CONTROL) != 0;
	bool fSelection = ichSelStart != ichSelStop;

	if (ch == VK_TAB && fSelection)
	{
		UINT iprStart = ParaFromChar(ichSelStart);
		UINT iprStop = ParaFromChar(ichSelStop);
		if (iprStart < iprStop)
		{
			void * pv;
			UINT ipr;
			DocPart * pdp = GetPart(ichSelStop, false, &pv, &ipr);
			if (pdp->rgpv[ipr] == pv)
				iprStop--;
			IndentParas(iprStart, iprStop, !(nShift & 0x8000000));
			return 0;
		}		
	}

	if (ch == 13)
		ch = 10;

	// These are used for creating the undo entry later
	CZDataObject * pzdo;
	UINT cch = GetText(ichSelStart, ichSelStop - ichSelStart, &pzdo);
	static UINT iLastInsertionPoint = 0;

	if (fSelection)
		DeleteText(ichSelStart, ichSelStop, false, false);
	void * pv;
	UINT iprInPart = 0;
	DocPart * pdpStart = GetPart(ichSelStart, false, &pv, &iprInPart);
	UINT cchInPara = CharsInPara(pdpStart, iprInPart);
	UINT cchAdded;

	if (ch == 10)
	{
		// Insert a paragraph.
		AssertPtr(m_pcsi);
		DocPart * pdpStop = pdpStart;
		UINT iStopParaInPart = iprInPart;
		UINT cchAtEnd = CharsAtEnd(pdpStart, iprInPart);
		bool fCR = false;
		if (cchInPara)
		{
			Assert(cchInPara >= cchAtEnd);
			//if (m_ft == kftAnsi)
			if (IsAnsi(pdpStart, iprInPart))
				fCR = ((char *)pdpStart->rgpv[iprInPart])[cchInPara - cchAtEnd] == 13;
			else
				fCR = ((wchar *)pdpStart->rgpv[iprInPart])[cchInPara - cchAtEnd] == 13;
		}
		if (pdpStart->cpr >= kcprInPart)
		{
			// There is not enough room in the current part to insert the new paragraph
			if (pdpStart->pdpNext && pdpStart->pdpNext->cpr < kcprInPart)
			{
				/*	Move the last paragraph in this doc part to the beginning of the
					next doc part to make room for the new paragraph. Then shift the
					paragraphs after the selection down one position to create the
					empty paragraph. */
				DocPart * pdpNext = pdpStart->pdpNext;
				memmove(pdpNext->rgpv + 1, pdpNext->rgpv, sizeof(void *) * pdpNext->cpr);
				memmove(pdpNext->rgcch + 1, pdpNext->rgcch, sizeof(UINT) * pdpNext->cpr);
				UINT cprToMove = pdpStart->cpr - 1;
				pdpNext->rgpv[0] = pdpStart->rgpv[cprToMove];
				pdpNext->rgcch[0] = pdpStart->rgcch[cprToMove];
				cprToMove -= iprInPart;
				memmove(pdpStart->rgpv + iprInPart + 1, pdpStart->rgpv + iprInPart, sizeof(void *) * cprToMove);
				memmove(pdpStart->rgcch + iprInPart + 1, pdpStart->rgcch + iprInPart, sizeof(UINT) * cprToMove);
				pdpNext->cpr++;
				if (++iStopParaInPart >= kcprInPart)
				{
					pdpStop = pdpStop->pdpNext;
					AssertPtr(pdpStop);
					iStopParaInPart = 0;
				}
				else
				{
					pdpNext->cch += CharsInPara(pdpNext, 0);
					pdpStart->cch -= CharsInPara(pdpNext, 0);
				}
			}
			else if (pdpStart->pdpPrev && pdpStart->pdpPrev->cpr < kcprInPart)
			{
				/*	Move the first paragraph in this doc part to the end of the
					previous doc part to make room for the new paragraph. Then shift
					the paragraphs before the selection up one position to create the
					empty paragraph. */
				DocPart * pdpPrev = pdpStart->pdpPrev;
				pdpPrev->rgpv[pdpPrev->cpr] = pdpStart->rgpv[0];
				pdpPrev->rgcch[pdpPrev->cpr++] = pdpStart->rgcch[0];
				pdpPrev->cch += CharsInPara(pdpStart, 0);
				pdpStart->cch -= CharsInPara(pdpStart, 0);
				memmove(pdpStart->rgpv, pdpStart->rgpv + 1, sizeof(void *) * iprInPart);
				memmove(pdpStart->rgcch, pdpStart->rgcch + 1, sizeof(UINT) * iprInPart);
				if (--iprInPart == -1)
				{
					pdpStart = pdpStart->pdpPrev;
					AssertPtr(pdpStart);
					iprInPart = pdpStart->cpr - 1;
				}
			}
			else
			{
				// Create a new part
				DocPart * pdpNew = new DocPart(pdpStart, pdpStart->pdpNext);
				if (!pdpNew)
					goto LOutOfMemory;
				if ((pdpNew->cpr = pdpStart->cpr - iprInPart - 1) > 0)
				{
					memmove(pdpNew->rgpv, pdpStart->rgpv + iprInPart + 1, sizeof(void *) * pdpNew->cpr);
					memmove(pdpNew->rgcch, pdpStart->rgcch + iprInPart + 1, sizeof(UINT) * pdpNew->cpr);
					memset(pdpStart->rgpv + iprInPart + 1, 0, sizeof(void *) * pdpNew->cpr);
					memset(pdpStart->rgcch + iprInPart + 1, 0, sizeof(UINT) * pdpNew->cpr);
					pdpStart->rgpv[iprInPart + 1] = pdpStart->rgpv[iprInPart];
					pdpStart->rgcch[iprInPart + 1] = pdpStart->rgcch[iprInPart];
					pdpStart->cpr -= pdpNew->cpr;
					UINT cch = 0;
					for (int ipr = pdpNew->cpr; --ipr >= 0; )
						cch += CharsInPara(pdpNew, ipr);
					pdpNew->cch = cch;
					pdpStart->cch -= cch;
					++iStopParaInPart;
					++pdpStart->cpr;
				}
				else
				{
					// TODO: Make sure this works correctly.
					pdpNew->rgpv[0] = pdpStart->rgpv[iprInPart];
					pdpNew->rgcch[0] = pdpStart->rgcch[iprInPart];
					pdpStop = pdpNew;
					iStopParaInPart = 0;
					pdpNew->cpr++;
				}
			}
		}
		else
		{
			UINT cprToMove = pdpStart->cpr++ - iprInPart;
			memmove(pdpStart->rgpv + iprInPart + 1, pdpStart->rgpv + iprInPart, sizeof(void *) * cprToMove);
			memmove(pdpStart->rgcch + iprInPart + 1, pdpStart->rgcch + iprInPart, sizeof(UINT) * cprToMove);
			iStopParaInPart++;
		}
		bool fSplit = false;
		bool fEndOfPara;
		bool fAnsi = IsAnsi(pdpStart, iprInPart);
		if (fAnsi)
		{
			Assert(pv <= (char *)pdpStart->rgpv[iprInPart] + cchInPara - cchAtEnd);
			fEndOfPara = (char *)pdpStart->rgpv[iprInPart] + cchInPara - cchAtEnd == pv;
		}
		else
		{
			Assert(pv <= (wchar *)pdpStart->rgpv[iprInPart] + cchInPara - cchAtEnd);
			fEndOfPara = (wchar *)pdpStart->rgpv[iprInPart] + cchInPara - cchAtEnd == pv;
		}
		if (!fEndOfPara)
		{
			UINT cbBefore = (char *)pv - (char *)pdpStart->rgpv[iprInPart];
			if (cbBefore > 0)
			{
				bool f8to16 = m_ft == kftUnicode8 && !IsParaInMem(pdpStart, iprInPart);
				UINT cchBefore = cbBefore;
				UINT cchAfter = cchInPara - cchBefore;
				UINT cbAfter;
				if (fAnsi)
				{
					cbAfter = f8to16 ? cchAfter * 2 : cchAfter;
					if (f8to16)
						cbBefore *= 2;
				}
				else
				{
					cbAfter = (cchInPara * 2) - cbBefore;
					Assert(cbAfter % 2 == 0);
					cchAfter = cbAfter / 2;
					Assert(cbBefore % 2 == 0);
					cchBefore = cbBefore / 2;
				}
				// Fix the new paragraph.
				pdpStop->rgpv[iStopParaInPart] = new char[cbAfter];
				if (!pdpStop->rgpv[iStopParaInPart])
					goto LOutOfMemory;
				if (f8to16)
					Convert8to16((char *)pv, (wchar *)pdpStop->rgpv[iStopParaInPart], cchAfter);
				else
					memmove(pdpStop->rgpv[iStopParaInPart], pv, cbAfter);
				pdpStop->rgcch[iStopParaInPart] = cchAfter - cchAtEnd + kpmParaInMem + (cchAtEnd << kpmEndOfParaShift);
				pdpStop->cch += (cchAfter - cchAtEnd);

				// Fix the old paragraph.
				// We are assuming the worst (Unicode) so allow 4 bytes for the EOL characters.
				void * pvNewPara;
				if (IsParaInMem(pdpStart, iprInPart))
				{
					Assert(!f8to16);
					pvNewPara = realloc(pdpStart->rgpv[iprInPart], cbBefore + 4);
					if (!pvNewPara)
						goto LOutOfMemory;
				}
				else
				{
					pvNewPara = new char[cbBefore + 4];
					if (!pvNewPara)
						goto LOutOfMemory;
					if (f8to16)
						Convert8to16((char *)pdpStart->rgpv[iprInPart], (wchar *)pvNewPara, cchBefore);
					else
						memmove(pvNewPara, pdpStart->rgpv[iprInPart], cbBefore);
					pdpStart->rgcch[iprInPart] |= kpmParaInMem;
				}
				pv = (char *)pvNewPara + cbBefore;
				pdpStart->rgpv[iprInPart] = pvNewPara;

				// Always add CRLF regardless of what was there before.
				// TODO: Is this what I want to always do or should it try to be a little smarter?
				if (fAnsi && !f8to16)
					memmove(pv, g_pszEndOfLine[2], 2);
				else
					memmove(pv, g_pwszEndOfLine[2], 4);
				pdpStart->rgcch[iprInPart] = (pdpStart->rgcch[iprInPart] & ~kpmEndOfPara) | kpmTwoEndOfPara;

				pdpStart->rgcch[iprInPart] -= (cchAfter - cchAtEnd);
				pdpStart->cch -= (cchAfter - cchAtEnd);
				cchAtEnd = 2;
			}
			else
			{
				pdpStart->cch -= CharsInPara(pdpStart, iprInPart);
				pdpStop->cch += CharsInPara(pdpStop, iStopParaInPart);
				// If it's a UTF-8 file, we want to treat it as ANSI so that if it brings the line
				// into memory later, the CrLf gets converted properly to Unicode.
				//if (fAnsi)
				if (m_ft == kftUnicode8 || m_ft == kftAnsi)
					pdpStart->rgpv[iprInPart] = (void *)g_pszEndOfLine[2];
				else
					pdpStart->rgpv[iprInPart] = (void *)g_pwszEndOfLine[2];
				cchAtEnd = 2;
				pdpStart->rgcch[iprInPart] = kpmTwoEndOfPara;
				pdpStop = pdpStart;
			}
		}
		else
		{
			if (cchAtEnd)
			{
				// If it's a UTF-8 file, we want to treat it as ANSI so that if it brings the line
				// into memory later, the CrLf gets converted properly to Unicode.
				//if (fAnsi)
				if (m_ft == kftUnicode8 || m_ft == kftAnsi)
				{
					if (cchAtEnd == 2)
						pdpStop->rgpv[iStopParaInPart] = (void *)g_pszEndOfLine[2];
					else
						pdpStop->rgpv[iStopParaInPart] = (void *)g_pszEndOfLine[fCR];
				}
				else
				{
					if (cchAtEnd == 2)
						pdpStop->rgpv[iStopParaInPart] = (void *)g_pwszEndOfLine[2];
					else
						pdpStop->rgpv[iStopParaInPart] = (void *)g_pwszEndOfLine[fCR];
				}
				pdpStop->rgcch[iStopParaInPart] = cchAtEnd << kpmEndOfParaShift;
			}
			else
			{
				// This should only happen at the end of the file.
				Assert(pdpStop->pdpNext == NULL && iStopParaInPart == pdpStop->cpr - 1);
				// TODO: This should change if we support non CRLF end of lines.
				UINT cbBefore = (char *)pv - (char *)pdpStart->rgpv[iprInPart];
				UINT cchBefore = cbBefore;
				bool f8to16 = m_ft == kftUnicode8 && !IsParaInMem(pdpStart, iprInPart);
				UINT cbNewEndOfLine = 2;
				if (fAnsi && f8to16)
					cbBefore *= 2;
				if (!fAnsi || f8to16)
					cbNewEndOfLine = 4;
				void * pvNewPara = new char[cbBefore + cbNewEndOfLine];
				if (!pvNewPara)
					goto LOutOfMemory;
				if (f8to16)
					Convert8to16((char *)pdpStart->rgpv[iprInPart], (wchar *)pvNewPara, cchBefore);
				else
					memmove(pvNewPara, pdpStart->rgpv[iprInPart], cbBefore);
				if (fAnsi && !f8to16)
					memmove((char *)pvNewPara + cbBefore, g_pszEndOfLine[2], 2);
				else
					memmove((char *)pvNewPara + cbBefore, g_pwszEndOfLine[2], 4);
				if (IsParaInMem(pdpStart, iprInPart))
					delete pdpStart->rgpv[iprInPart];
				pdpStart->rgpv[iprInPart] = pvNewPara;
				pdpStart->rgcch[iprInPart] |= (kpmParaInMem + kpmTwoEndOfPara);
				pdpStop->rgpv[iStopParaInPart] = 0;
				pdpStop->rgcch[iStopParaInPart] = 0;
				if (iStopParaInPart == 0)
				{
					Assert(pdpStop && pdpStop == pdpStart->pdpNext && !pdpStop->pdpNext);
					Assert(pdpStop->cch == 0 && pdpStop->cpr == 1);
					pdpStop = pdpStart;
				}
				Assert(pdpStart == pdpStop);
				cchAtEnd = 2;
			}
		}
		m_cpr++;

		// Update the scrollbar
		/*SCROLLINFO si = {sizeof(si), SIF_POS | SIF_PAGE | SIF_RANGE};
		if (m_fWrap)
		{
			si.nPos = m_dwFirstChar;
			m_dwFirstLastChar = ShowCharAtLine(m_dwTotalParas, m_dwFileLength, g_fg.m_cLines - 2);
			si.nPage = m_dwFileLength - m_dwFirstLastChar;
			si.nMax = m_dwFirstLastChar ? m_dwFileLength - 1 : 0;
		}
		else
		{
			si.nPos = m_dwFirstPara;
			si.nPage = g_fg.m_cLines;
			si.nMax = m_dwTotalParas - 1 + si.nPage;
		}
		m_pzf->SetScrollInfo(m_pzvCurrent, SB_VERT, &si);*/
		pdpStop->cch += cchAtEnd;
		m_cch += cchAtEnd;
		cchAdded = cchAtEnd;

		// Complete the undo entry for this
		CUndoRedo * pur;
		pur = new CUndoRedo(kurtTyping, ichSelStart, cchAdded, ichSelStop - ichSelStart);
		if (pur)
		{
			pur->m_pzdo = pzdo;
			pzdo = NULL;
			m_stackUndo.push(pur);
		}
	}
	else
	{
		// TODO: Typing can cause the file to be long enough to change the scrollbar. Right now
		// the scrollbars aren't touched when typing a normal character. Something needs to be
		// done so that the text doesn't disappear off the bottom of the screen without a
		// scrollbar appearing.

		// Insert a character
		// If the paragraph is not already in memory, move it into memory.
		cchAdded = 1;
		void * pvOldPara = pdpStart->rgpv[iprInPart];
		bool fInMemory = true;
		if (!IsParaInMem(pdpStart, iprInPart))
		{
			pvOldPara = NULL;
			fInMemory = false;
			pdpStart->rgcch[iprInPart] += kpmParaInMem;
		}
		if (m_ft == kftAnsi)
		{
			char * pchNewPara = (char *)realloc(pvOldPara, cchInPara + 1);
			if (!pchNewPara)
				goto LOutOfMemory;
			if (!fInMemory)
				memmove(pchNewPara, pdpStart->rgpv[iprInPart], cchInPara);
			pv = (char *)pv - (char *)pdpStart->rgpv[iprInPart] + pchNewPara;
			pdpStart->rgpv[iprInPart] = pchNewPara;
		}
		else
		{
			wchar * pchNewPara = (wchar *)realloc(pvOldPara, (cchInPara + 1) << 1);
			if (!pchNewPara)
				goto LOutOfMemory;
			if (!fInMemory)
			{
				if (m_ft == kftUnicode8)
				{
					Convert8to16((char *)pdpStart->rgpv[iprInPart], pchNewPara, cchInPara);
					// TODO: This next line is probably not right.
					pv = (char *)pv - (char *)pdpStart->rgpv[iprInPart] + pchNewPara;
					//pv = ((char *)pv - (char *)pdpStart->rgpv[iprInPart]) * 2 + pchNewPara;
				}
				else
				{
					memmove(pchNewPara, pdpStart->rgpv[iprInPart], cchInPara << 1);
					pv = (wchar *)pv - (wchar *)pdpStart->rgpv[iprInPart] + pchNewPara;
				}
			}
			else
			{
				pv = (wchar *)pv - (wchar *)pdpStart->rgpv[iprInPart] + pchNewPara;
			}
			pdpStart->rgpv[iprInPart] = pchNewPara;
		}
		bool fReplaceChar = fOvertype && ichSelStart == ichSelStop;
		if (fReplaceChar)
		{
			// Don't replace the char if we're at the end of a paragraph.
			UINT cbPrintChars = PrintCharsInPara(pdpStart, iprInPart);
			if (m_ft != kftAnsi)
				cbPrintChars <<= 1;
			fReplaceChar = (pv != (char *)pdpStart->rgpv[iprInPart] + cbPrintChars);
		}
		if (fReplaceChar)
		{
			if (m_ft == kftAnsi)
				*((char *)pv) = ch;
			else
				*((wchar *)pv) = ch;
		}
		else
		{
			/*if (cchInPara == -1)
			{
				pdpStart->rgcch[dwStartOffset]++;
				dwParaLength++;
			}*/
			if (m_ft == kftAnsi)
			{
				memmove((char *)pv + 1, pv, (char *)pdpStart->rgpv[iprInPart] - (char *)pv + cchInPara);
				*((char *)pv) = ch;
			}
			else
			{
				// TODO: Fix this when we're adding a character at the beginning of a file that has a
				// byte marker at the beginning.
				/*if (m_ft == kftUnicode8 && pdpStart->pdpPrev == NULL && iprInPart == 0 && cchInPara >= 2 && *((wchar *)pv) == 0xFEFF)
				{
					ichSelStop = ++ichSelStart;
					pv = (wchar *)pv + 1;
				}*/
				memmove((wchar *)pv + 1, pv, ((wchar *)pdpStart->rgpv[iprInPart] - (wchar *)pv + cchInPara) << 1);
				*((wchar *)pv) = ch;
			}
			pdpStart->rgcch[iprInPart]++;
			pdpStart->cch++;
			m_cch++;
		}

		// Complete the undo entry for this
		CUndoRedo * pur;
		if (fOnlyEditing &&
			m_stackUndo.size() &&
			(pur = m_stackUndo.top())->m_urt == kurtTyping &&
			ichSelStart == iLastInsertionPoint + 1 &&
			ichSelStart == ichSelStop)
		{
			++pur->m_cch;
		}
		else
		{
			fOnlyEditing = true;
			pur = new CUndoRedo(kurtTyping, ichSelStart, 1, ichSelStop - ichSelStart);
			if (pur)
			{
				pur->m_pzdo = pzdo;
				pzdo = NULL;
				m_stackUndo.push(pur);
			}
		}
		iLastInsertionPoint = ichSelStart;
	}

	m_pzf->NotifyDelete(ichSelStart, ichSelStop);
	m_pzf->NotifyInsert(ichSelStart, cchAdded);

	/*m_dwDragStart = m_dwStopSelection = dwLastInsertionPoint = ++m_dwStartSelection;
	if (fCharOnSingleLine)
	{
		HDC hdc = GetDC(m_hwnd);
		DrawSelection(hdc);
		ReleaseDC(m_hwnd, hdc);
	}
	else
	{
		// TODO: Do something a little more intelligent for this case. Basically this case applies when
		// the <Enter> key was hit or the character typed was not on the last line of the paragraph.
		InvalidateRect(m_hwnd, NULL, FALSE);
		UpdateWindow(m_hwnd);
	}
	m_ptRealPos = m_ptCaretPos = PointFromChar(m_dwStartSelection);
	RECT rect;
	GetClientRect(m_hwnd, &rect);
	if (m_ptRealPos.y < 0)
		OnVScroll(SB_LINEUP, 0, 0);
	else if (m_ptRealPos.y > rect.bottom - g_fg.m_rcMargin.bottom - g_fg.m_tm.tmHeight)
		OnVScroll(SB_LINEDOWN, 0, 0);
	else
		SetCaretPos(m_ptCaretPos.x, m_ptCaretPos.y);
	ShowCaret(m_hwnd);*/
	ClearRedo();
	m_fModified = true;
	if (pzdo)
		pzdo->Release();
	return 0;

LOutOfMemory:
	if (pzdo)
		pzdo->Release();
	return 1;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZDoc::UpdateModified(int iFile, bool fForceUpdate)
{
	if (fForceUpdate || m_fModified != m_fModifiedOld)
	{
		char szBuffer[MAX_PATH];
		lstrcpy(szBuffer, m_szFilename);
		if (m_fModified)
			strcat_s(szBuffer, " *");

		if (m_pzef->GetTabMgrHwnd())
		{
			HWND hwndList = GetDlgItem(m_pzef->GetTabMgrHwnd(), IDC_LISTVIEW);
			LVITEM lvi = {LVIF_TEXT};
			lvi.iItem = iFile;
			lvi.pszText = szBuffer;
			ListView_SetItem(hwndList, &lvi);
			ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);
		}

		m_pzef->GetTab()->Refresh(false);
	}
	m_fModifiedOld = m_fModified;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::OnFontChanged()
{
	AssertPtr(m_pzf);
	m_pzf->OnFontChanged();
	return true;
}


/*----------------------------------------------------------------------------------------------
	pzf can only be NULL if we are reopening a file.
----------------------------------------------------------------------------------------------*/
bool CZDoc::OpenFile(char * pszFilename, CZFrame * pzf, HWND hwndProgress)
{
	AssertPtr(pszFilename);
	AssertPtrN(pzf);
#ifdef _DEBUG
	if (!pzf)
	{
		AssertPtr(m_pcsi);
		AssertPtr(m_pzf);
		AssertPtr(m_pzvCurrent);
	}
#endif // _DEBUG

	// If a file is already open, free the memory it was using
	m_cch = m_cpr = m_cTimeToLoad = 0;
	CloseFileHandles();
	_beginthread(DeleteMemory, 0, (void *)m_pdpFirst);

	m_pvFile = NULL;
	m_pdpFirst = new DocPart;
	if (!m_pdpFirst)
		return false;

	if (pzf)
	{
		char * pszExt = strrchr(pszFilename, '.');
		m_pcsi = g_cs.GetColorScheme(pszExt ? pszExt + 1 : pszExt);

		AttachFrame(pzf);
	}
	if (!_stricmp(pszFilename, "Untitled"))
	{
		if (m_ft == kftDefault || m_ft == kftNone)
			//m_ft = kftAnsi;
			m_ft = (FileType)g_fg.m_defaultEncoding;
		m_cpr = m_pdpFirst->cpr = 1;
		return true;
	}

	struct stat statFile;
	if (stat(pszFilename, &statFile) == -1)
		return false; // Could not find the file

	if (statFile.st_size == 0)
	{
		if (m_ft == kftDefault)
			//m_ft = kftAnsi;
			m_ft = (FileType)g_fg.m_defaultEncoding;
		m_cpr = m_pdpFirst->cpr = 1;
		return true;
	}

	// Open up the source file and map to/read into memory
	HANDLE hFile = ::CreateFile(pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	::GetFileTime(hFile, NULL, NULL, &m_ftModified);
	m_fTextFromDisk = true;
	void * pvFile = NULL;
	if (hFile != INVALID_HANDLE_VALUE)
	{
		// If the file size < g_fg.m_cchFileLimit, copy the file to memory so the handles
		// to the original file can be freed and the file will no longer be locked.
		if ((UINT)statFile.st_size < g_fg.m_cchFileLimit)
			pvFile = new char[statFile.st_size];
		if (pvFile)
		{
			DWORD cbRead;
			::ReadFile(hFile, pvFile, statFile.st_size, &cbRead, NULL);
			m_fTextFromDisk = false;
		}
		else
		{
			HANDLE hFileMap = ::CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, statFile.st_size, NULL);
			if (hFileMap)
			{
				pvFile = ::MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
				::CloseHandle(hFileMap);
			}
		}
		::CloseHandle(hFile);
	}
	if (!pvFile)
		return false;

	return OpenFile(pszFilename, pvFile, statFile.st_size, hwndProgress);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::OpenFile(char * pszFilename, void * prgchFileContents, DWORD cbSize,
	CZFrame * pzf)
{
	AssertPtr(pszFilename);
	AssertPtr(pzf);

	// If a file is already open, free the memory it was using
	m_cch = m_cpr = m_cTimeToLoad = 0;
	CloseFileHandles();
	_beginthread(DeleteMemory, 0, (void *)m_pdpFirst);
	lstrcpy(m_szFilename, pszFilename);

	m_pvFile = NULL;
	m_pdpFirst = new DocPart;
	if (!m_pdpFirst)
		return false;

	char * pszExt = strrchr(pszFilename, '.');
	m_pcsi = g_cs.GetColorScheme(pszExt ? pszExt + 1 : pszExt);

	AttachFrame(pzf);

	// Get dimensions for progress bar
	HWND hwndStatus = m_pzef->GetStatusHwnd();
	RECT rect;
	::SendMessage(hwndStatus, SB_GETRECT, ksboMessage, (long) &rect);
	::InflateRect(&rect, -1, -2);

	HWND hwndProgress = ::CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		hwndStatus, 0, g_fg.m_hinst, NULL);

	bool fSuccess = OpenFile(pszFilename, prgchFileContents, cbSize, hwndProgress);

	::DestroyWindow(hwndProgress);

	return fSuccess;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::OpenFile(char * pszFilename, void * prgchFileContents, DWORD cbSize,
	HWND hwndProgress)
{
	m_pvFile = prgchFileContents;
	if (m_ft == kftDefault || m_ft == kftUnicode16)
	{
		// Try to determine what type of file it is
		if (*((wchar *)m_pvFile) == 0xFEFF && (cbSize % 2) == 0)
			m_ft = kftUnicode16;
		else if (cbSize >= 3 && strncmp((char *)m_pvFile, g_pszUTF8Marker, 3) == 0)
			m_ft = kftUnicode8;
		else if (m_ft == kftDefault)
			//m_ft = kftAnsi;
			m_ft = (FileType)g_fg.m_defaultEncoding;
	}

	if (m_ft == kftUnicode16)
		::SendMessage(hwndProgress, PBM_SETRANGE32, 0, cbSize >> 1);
	else
		::SendMessage(hwndProgress, PBM_SETRANGE32, 0, cbSize);

	UINT cpr = 0;
	UINT cchInPara = 0;
	bool fUpdate = true;
	DocPart * pdp = m_pdpFirst;
	if (m_ft != kftUnicode16)
	{
		// This section handles ANSI and UTF-8 files
		pdp->rgpv[0] = m_pvFile;
		char * prgch = (char *)m_pvFile - 1;
		char * prgchPara = (char *)m_pvFile;
		char * prgchLim = prgchPara + cbSize;
		char ch;
		while (++prgch < prgchLim)
		{
			ch = *prgch;
			if (ch < 14 && (ch == 10 || ch == 13))
			{
				bool fConverted = false;
				if (m_ft == kftUnicode8)
				{
					// See if this paragraph needs to be copied to memory and converted to UTF-16.
					char * prgchT= prgchPara;
					for (; prgchT < prgch; prgchT++)
					{
						if (*prgchT & 0x80)
							break;
					}
					if (prgchT != prgch)
						fConverted = true;
				}
				if (fConverted)
				{
					/*	There are at least two characters that will end up as one Unicode
						character, so this will allocate more than enough memory for the
						conversion, including space for at most two end-of-line characters. */
					wchar * prgwchPara = new wchar[((prgch - prgchPara) << 1) + 1];
					if (!prgwchPara)
						return false;
					/*	Include the first 0xA or 0xD in the conversion. If the end-of-line is
						CRLF, the additional character (0xA) will be added later. */
					int cch = UTF8ToUTF16(prgchPara, prgch - prgchPara + 1, (wchar *)prgwchPara);
					pdp->rgpv[cpr] = prgwchPara;
					pdp->rgcch[cpr] = cch - 1 + kpmParaInMem + kpmOneEndOfPara;
					cchInPara += cch;
				}
				else
				{
					pdp->rgcch[cpr] = prgch - prgchPara + kpmOneEndOfPara;
					cchInPara += prgch - prgchPara + 1;
				}

				if (++cpr == kcprInPart)
				{
					SendMessage(hwndProgress, PBM_SETPOS, prgch - (char *)m_pvFile, 0);
					m_cpr += (pdp->cpr = cpr);
					m_cch += (pdp->cch = cchInPara);
					cpr = cchInPara = 0;
					pdp = new DocPart(pdp);
					if (!pdp)
						return false;
					if (fUpdate)
					{
						fUpdate = false;
						::ShowWindow(GetHwnd(), SW_SHOW);
						::InvalidateRect(m_pzvCurrent->GetHwnd(), NULL, FALSE);
						::UpdateWindow(m_pzvCurrent->GetHwnd());
					}
				}

				// See if the next character is a LF character.
				if (prgch + 1 < prgchLim && prgch[1] == 10)
				{
					if (cpr == 0)
					{
						pdp->pdpPrev->rgcch[kcprInPart - 1] &= ~kpmOneEndOfPara; // Turn off kpmOneEndOfPara bit
						pdp->pdpPrev->rgcch[kcprInPart - 1] |= kpmTwoEndOfPara; // Turn on kpmTwoEndOfPara bit
						pdp->pdpPrev->cch++;
						m_cch++;
					}
					else
					{
						pdp->rgcch[cpr - 1] &= ~kpmOneEndOfPara; // Turn off kpmOneEndOfPara bit
						pdp->rgcch[cpr - 1] |= kpmTwoEndOfPara; // Turn on kpmTwoEndOfPara bit
						pdp->cch++;
						cchInPara++;
					}
					if (fConverted && m_ft == kftUnicode8)
					{
						// This is where we add the extra character (LF) in the buffer allocated above.
						if (cpr == 0)
							((wchar *)pdp->pdpPrev->rgpv[kcprInPart - 1])[PrintCharsInPara(pdp->pdpPrev, kcprInPart - 1) + 1] = 0xA;
						else
							((wchar *)pdp->rgpv[cpr - 1])[PrintCharsInPara(pdp, cpr - 1) + 1] = 0xA;
						fConverted = false;
					}
					pdp->rgpv[cpr] = prgchPara = ++prgch + 1;
				}
				else
				{
					pdp->rgpv[cpr] = prgchPara = prgch + 1;
				}
			}
		}

		if (m_ft == kftAnsi)
		{
			cchInPara += (pdp->rgcch[cpr] = prgch - prgchPara);
		}
		else
		{
			// See if the final paragraph needs to be copied to memory and converted to UTF-16.
			char * prgchT = prgchPara;
			for ( ; prgchT < prgch; prgchT++)
			{
				if (*prgchT & 0x80)
					break;
			}
			if (prgchT == prgch)
			{
				cchInPara += (pdp->rgcch[cpr] = prgch - prgchPara);
			}
			else
			{
				/*	There are at least two characters that will end up as one Unicode character,
					so this will allocate more than enough memory for the conversion. */
				wchar * prgwchPara = new wchar[(prgch - prgchPara) << 1];
				if (!prgwchPara)
					return false;
				int cch = UTF8ToUTF16(prgchPara, prgch - prgchPara, (wchar *)prgwchPara);
				pdp->rgpv[cpr] = prgwchPara;
				cchInPara += cch;
				pdp->rgcch[cpr] = cch + kpmParaInMem;
			}
		}
	}
	else
	{
		// This section handles UTF-16 files
		wchar * prgch = (wchar *)m_pvFile;
		//if (*prgch != 0xFEFF)
			prgch--;
		wchar * prgchPara = (wchar *)(pdp->rgpv[0] = prgch + 1);
		wchar * prgchLim = (wchar *)m_pvFile + (cbSize >> 1);
		wchar ch;
		while (++prgch < prgchLim)
		{
			ch = *prgch;
			if (ch < 14 && (ch == 10 || ch == 13))
			{
				pdp->rgcch[cpr] = prgch - prgchPara + kpmOneEndOfPara;
				cchInPara += prgch - prgchPara + 1;
				if (++cpr == kcprInPart)
				{
					::SendMessage(hwndProgress, PBM_SETPOS, prgch - (wchar *)m_pvFile, 0);
					m_cpr += (pdp->cpr = cpr);
					m_cch += (pdp->cch = cchInPara);
					cpr = cchInPara = 0;
					pdp = new DocPart(pdp);
					if (!pdp)
						return false;
					if (fUpdate)
					{
						fUpdate = false;
						::ShowWindow(GetHwnd(), SW_SHOW);
						::InvalidateRect(m_pzvCurrent->GetHwnd(), NULL, FALSE);
						::UpdateWindow(m_pzvCurrent->GetHwnd());
					}
				}

				// See if the next character is a LF character.
				if (prgch + 1 < prgchLim && prgch[1] == 10)
				{
					if (cpr == 0)
					{
						pdp->pdpPrev->rgcch[kcprInPart - 1] &= ~kpmOneEndOfPara; // Turn off kpmOneEndOfPara bit
						pdp->pdpPrev->rgcch[kcprInPart - 1] |= kpmTwoEndOfPara; // Turn on kpmTwoEndOfPara bit
						pdp->pdpPrev->cch++;
						m_cch++;
					}
					else
					{
						pdp->rgcch[cpr - 1] &= ~kpmOneEndOfPara; // Turn off kpmOneEndOfPara bit
						pdp->rgcch[cpr - 1] |= kpmTwoEndOfPara; // Turn on kpmTwoEndOfPara bit
						pdp->cch++;
						cchInPara++;
					}
					pdp->rgpv[cpr] = prgchPara = ++prgch + 1;
				}
				else
				{
					pdp->rgpv[cpr] = prgchPara = prgch + 1;
				}
			}
		}
		cchInPara += (pdp->rgcch[cpr] = prgch - prgchPara);
	}

	// TODO: Which of these should it be?
	//m_cpr += (pdp->cpr = ++cpr);
	//m_cpr += (pdp->cpr = cpr);
	m_cpr += ++cpr;
	if (cpr > kcprInPart)
	{
		Assert(cpr == kcprInPart + 1);
		pdp->cpr = kcprInPart;
		pdp = new DocPart(pdp, pdp->pdpNext);
		if (!pdp)
			return false;
		pdp->cpr = 1;
	}
	else
	{
		pdp->cpr = cpr;
	}
	m_cch += (pdp->cch = cchInPara);
	Assert(m_ft == kftBinary ||
		m_ft == kftUnicode8 ||
		(m_ft == kftAnsi && m_cch == cbSize) ||
		(m_ft == kftUnicode16 && m_cch + (wchar *)m_pdpFirst->rgpv[0] - (wchar *)m_pvFile == (cbSize >> 1)));

	m_pzvCurrent->UpdateScrollBars();

	if (fUpdate)
	{
		::ShowWindow(GetHwnd(), SW_SHOW);
		::InvalidateRect(m_pzvCurrent->GetHwnd(), NULL, FALSE);
		::UpdateWindow(m_pzvCurrent->GetHwnd());
	}
	m_fModified = false;
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::ReOpenFile(FileType ft)
{
	if (ft == kftNone)
		return false;

	// Get dimensions for progress bar
	RECT rect;
	HWND hwndStatus = m_pzef->GetStatusHwnd();
	::SendMessage(hwndStatus, SB_GETRECT, ksboMessage, (long)&rect);
	::InflateRect(&rect, -1, -2);
	HWND hwndProgress = NULL;

	if (m_fModified)
	{
		int nResponse = ::MessageBox(m_pzef->GetHwnd(),
			"Do you want to save your changes first?", g_pszAppName,
			MB_YESNOCANCEL | MB_ICONQUESTION);
		if (nResponse == IDCANCEL)
			return true;
		if (nResponse == IDYES)
		{
			CWaitCursor wc;
			hwndProgress = ::CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE,
				rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hwndStatus, 0, 0, NULL);
			::SendMessage(hwndProgress, PBM_SETPOS, 0, 0);
			::SendMessage(hwndProgress, PBM_SETRANGE32, 0, 0);

			bool fSuccess = SaveFile(m_szFilename, kftDefault, hwndProgress);
			if (!fSuccess)
			{
				::MessageBox(m_pzef->GetHwnd(), "The file could not be saved.",
					g_pszAppName, MB_OK | MB_ICONWARNING);
				::DestroyWindow(hwndProgress);
				return false;
			}
		}
	}

	CWaitCursor wc;
	bool fWrap = GetWrap();
	if (!hwndProgress)
	{
		hwndProgress = ::CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE,
			rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hwndStatus, 0, 0, NULL);
	}
	::SendMessage(hwndProgress, PBM_SETPOS, 0, 0);
	::SendMessage(hwndProgress, PBM_SETRANGE32, 0, 0);

	::GetClientRect(m_pzef->GetHwnd(), &rect);
	m_ft = ft;
	// Get the selection so we can reset it after the file is reopened.
	UINT ichSelStart, ichSelStop;
	GetSelection(&ichSelStart, &ichSelStop);
	bool fSuccess = OpenFile(m_szFilename, NULL, hwndProgress);
	if (fSuccess)
	{
		::InvalidateRect(m_pzvCurrent->GetHwnd(), NULL, FALSE);
		::UpdateWindow(m_pzvCurrent->GetHwnd());
		m_pzef->SetStatusText(ksboFileType, g_pszFileType[ft]);

		SetSelection(ichSelStart, ichSelStop, true, true);
	}

	::DestroyWindow(hwndProgress);
	return fSuccess;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UINT CZDoc::ParaFromChar(UINT ich)
{
	if (m_fAccessError)
		return 0;
	UINT ipr = 0;
	UINT cprBefore = 0;
	DocPart * pdp = GetPart(ich, false, NULL, &ipr, NULL, &cprBefore);
	return cprBefore + ipr;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::ParseHFString(char * pszPrint, char szResult[3][100], HeaderFooterInfo * phfi)
{
	AssertPtr(pszPrint);
	AssertPtr(phfi);
	AssertPtr(phfi->pszDate);
	AssertPtr(phfi->pszFilename);
	AssertPtr(phfi->pszPathname);
	AssertPtr(phfi->pszTime);
	AssertArray(szResult, sizeof(char[3][100]));

	char ch;
	char * prgch = szResult[0];
	memset(szResult, 0, sizeof(char[3][100]));

	while ((ch = *pszPrint++) != 0)
	{
		if (ch == '^')
		{
			ch = tolower(*(pszPrint++));
			if (ch == 'l')
				prgch = szResult[0];
			else if (ch == 'c')
				prgch = szResult[1];
			else if (ch == 'r')
				prgch = szResult[2];
			else if (ch == 'f')
				strcpy_s(prgch, 100, phfi->pszFilename);
			else if (ch == 'p')
				strcpy_s(prgch, 100, phfi->pszPathname);
			else if (ch == 'g')
				_itoa_s(phfi->iPage, prgch, 100, 10);
			else if (ch == 't')
				strcpy_s(prgch, 100, phfi->pszTime);
			else if (ch == 'd')
				strcpy_s(prgch, 100, phfi->pszDate);
			else
			{
				*prgch++ = '^';
				*prgch = *(pszPrint - 1);
			}
			prgch += strlen(prgch);
		}
		else
		{
			*prgch++ = ch;
		}
	}
	*prgch = 0;
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::Paste(UINT ichSelStart, UINT ichSelStop)
{
	if (m_fAccessError)
		return false;

	CZDataObject * pzdo;
	if (S_OK != (CZDataObject::CreateFromDataObject(m_pzef, NULL, m_ft == kftAnsi, &pzdo)))
		return false;
	AssertPtr(pzdo);
	InsertText(ichSelStart, pzdo, ichSelStop - ichSelStart, kurtPaste);
	pzdo->Release();
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::Redo()
{
	if (!m_stackRedo.empty())
	{
		CZDataObject * pzdo = NULL;
		UINT cch;
		m_fModified = true;
		CUndoRedo * pur = m_stackRedo.top();
		m_stackRedo.pop();

		UndoRedoType urt = pur->m_urt;
		if (urt == kurtMove)
		{
			if (pur->m_mt == kmtPaste)
				urt = kurtPaste;
			else if (pur->m_mt == kmtCut)
				urt = kurtCut;
		}

		switch (urt)
		{
			case kurtCut:
			case kurtDelete:
				DeleteText(pur->m_ichMin, pur->m_ichMin + pur->m_cch, false, false);
				SetSelection(pur->m_ichMin, pur->m_ichMin, true, true);
				break;

			case kurtPaste:
			case kurtReplace:
			case kurtTyping:
#ifdef SPELLING
			case kurtSpell:
#endif // SPELLING
			case kurtSmartIndent:
				GetText(pur->m_ichMin, pur->m_cchReplaced, &pzdo);
				DeleteText(pur->m_ichMin, pur->m_ichMin + pur->m_cchReplaced, false, false);
				if (pur->m_pzdo)
				{
					cch = InsertText(pur->m_ichMin, pur->m_pzdo, 0, kurtEmpty);
					pur->m_pzdo->Release();
				}
				else
				{
					cch = pur->m_cch;
				}
				pur->m_pzdo = pzdo;
				if (pur->m_urt == kurtMove)
					SetSelection(pur->m_ichMin, pur->m_ichMin + cch, true, true);
				else
					SetSelection(pur->m_ichMin + pur->m_cch, pur->m_ichMin + cch, true, true);
				break;

			case kurtReplaceAll:
			case kurtIndentIncrease:
			case kurtIndentDecrease:
				{
					CWaitCursor ec;
					CUndoRedo::ReplaceAllInfo * prai = pur->m_prai;
					AssertPtr(prai);
					CUndoRedo::ReplaceAllInfo * praiNew = new CUndoRedo::ReplaceAllInfo();
					if (prai->m_crals > 0)
					{
						CUndoRedo::ReplaceAllLocation * pral = prai->m_prgral - 1;
						CUndoRedo::ReplaceAllLocation * pralLim = pral + prai->m_crals + 1;
						while (++pral < pralLim)
						{
							// NOTE: pzdo can be NULL.
							CZDataObject * pzdo = prai->m_prgzdo[pral->m_izdo];
							AssertPtrN(pzdo);
							UINT izdo = praiNew->AddReplaceAllItem(this, pral->m_ich,
								pral->m_cch, false);
							if (izdo == -1)
							{
								::MessageBox(m_pzef->GetHwnd(),
									"An error occurred while performing the Redo operation.",
									g_pszAppName, MB_OK | MB_ICONEXCLAMATION);
								break;
							}
							InsertText(pral->m_ich, pzdo, pral->m_cch, kurtEmpty);
							pral->m_cch = pzdo ? pzdo->GetCharCount() : 0;
							pral->m_izdo = izdo;
						}
					}
					// Swap the two arrays of CZDataObject pointers.
					UINT czdoT = pur->m_prai->m_czdo;
					CZDataObject ** prgzdoT = pur->m_prai->m_prgzdo;
					pur->m_prai->m_czdo = praiNew->m_czdo;
					pur->m_prai->m_prgzdo = praiNew->m_prgzdo;
					praiNew->m_czdo = czdoT;
					praiNew->m_prgzdo = prgzdoT;
					delete praiNew;

					if (urt == kurtIndentIncrease || urt == kurtIndentDecrease)
						SetSelection(pur->m_ichMin, pur->m_cchReplaced, true, true);
					else
						SetSelection(pur->m_ichMin, pur->m_ichMin, true, true);
					break;
				}

			case kurtMove:
				{
					void * pv;
					GetText(pur->m_cchReplaced, pur->m_cch, &pv);
					if (!pv)
					{
						m_stackRedo.push(pur);
						return false;
					}
					if (pur->m_mt != kmtCopy)
						DeleteText(pur->m_cchReplaced, pur->m_cchReplaced + pur->m_cch, false, false);
					cch = InsertText(pur->m_ichMin, pv, pur->m_cch, 0, m_ft == kftAnsi, kurtEmpty);
					SetSelection(pur->m_ichMin, pur->m_ichMin + cch, true, true);
					delete pv;
					break;
				}

			default:
				return false;
		}
		
		m_stackUndo.push(pur);
		m_pzef->UpdateTextPos();
		return true;
	}
	return false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::SaveFile(char * pszFilename, FileType ft, HWND hwndProgress)
{
	if (ft == kftNone || ft == kftDefault)
		ft = m_ft;
	DocPart * pdp = m_pdpFirst;
	int nParaSize = 2;
	if (ft == kftUnicode16)
		nParaSize = 4;

	char szPath[MAX_PATH];
	char szTempFile[MAX_PATH];
	lstrcpy(szPath, pszFilename);
	char * pSlash = strrchr(szPath, '\\');
	if (pSlash)
		*pSlash = '\0';
	::GetTempFileName(szPath, "drz", 0, szTempFile);
	HANDLE hFileTemp = ::CreateFile(szTempFile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFileTemp == INVALID_HANDLE_VALUE)
		return false;
	::SendMessage(hwndProgress, PBM_SETRANGE32, 0, m_cch);

#ifdef _DEBUG
	struct _timeb time1, time2;
	_ftime_s(&time1);
#endif // _DEBUG

	bool fSuccess;
	if (m_ft == kftAnsi)
	{
		if (ft == kftAnsi)
			fSuccess = WriteANSItoANSIFile(hFileTemp, hwndProgress);
		else if (ft == kftUnicode8)
			fSuccess = WriteANSItoUTF8File(hFileTemp, hwndProgress);
		else
			fSuccess = WriteANSItoUTF16File(hFileTemp, hwndProgress);
	}
	else if (m_ft == kftUnicode8)
	{
		if (ft == kftAnsi)
			fSuccess = WriteUTF8toANSIFile(hFileTemp, hwndProgress);
		else if (ft == kftUnicode8)
			fSuccess = WriteUTF8toUTF8File(hFileTemp, hwndProgress);
		else
			fSuccess = WriteUTF8toUTF16File(hFileTemp, hwndProgress);
	}
	else
	{
		if (ft == kftAnsi)
			fSuccess = WriteUTF16toANSIFile(hFileTemp, hwndProgress);
		else if (ft == kftUnicode8)
			fSuccess = WriteUTF16toUTF8File(hFileTemp, hwndProgress);
		else
			fSuccess = WriteUTF16toUTF16File(hFileTemp, hwndProgress);
	}
	m_ft = ft;

#ifdef _DEBUG
	_ftime_s(&time2);
	DWORD ms = (DWORD)(((time2.time - time1.time) * 1000) + time2.millitm - time1.millitm);
#endif // _DEBUG

	m_fModified = false;
	lstrcpy(m_szFilename, pszFilename);

	// Close all file handles
	::SetEndOfFile(hFileTemp);
	::CloseHandle(hFileTemp);
	CloseFileHandles();

	// Make sure the new file got created successfully.
	struct stat sFile;
	if (stat(szTempFile, &sFile) == -1)
	{
		::MessageBox(NULL, "The temporary file could not be created.", "ZEdit", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	// Delete the old file and rename the new file
	/*if (remove(pszFilename) != 0)
	{
		MessageBox(NULL, "The old file could not be deleted.", "ZEdit", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	if (rename(szTempFile, pszFilename) != 0)
	{
		MessageBox(NULL, "The temporary file could not be renamed.", "ZEdit", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}*/
	char szMessage[MAX_PATH];
	if (!::CopyFile(szTempFile, pszFilename, false))
	{
		sprintf_s(szMessage, "The temp file could not be copied over the old file. Error = %d", GetLastError());
		::MessageBox(NULL, szMessage, "ZEdit", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	if (!::DeleteFile(szTempFile))
	{
		sprintf_s(szMessage, "The temp file could not be deleted. Error = %d", GetLastError());
		::MessageBox(NULL, szMessage, "ZEdit", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	/*if (rename(szTempFile, pszFilename) != 0)
	{
		sprintf(szMessage, "The old file could not be deleted. Error = %d", GetLastError());
		MessageBox(NULL, szMessage, "ZEdit", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}*/

	HANDLE hFile = ::CreateFile(pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	m_pvFile = NULL;
	m_fTextFromDisk = true;
	if (hFile != INVALID_HANDLE_VALUE)
	{
		// Update the time stored for this file.
		::GetFileTime(hFile, NULL, NULL, &m_ftModified);

		// If the file size < g_fg.m_cchFileLimit, copy the file to memory so the handles
		// to the original file can be freed and the file will no longer be locked.
		if ((UINT)sFile.st_size < g_fg.m_cchFileLimit)
		{
			DWORD dwBytesRead;
			m_pvFile = new char[sFile.st_size];
			if (m_pvFile)
			{
				::ReadFile(hFile, m_pvFile, sFile.st_size, &dwBytesRead, NULL);
				m_fTextFromDisk = false;
			}
		}
		else
		{
			HANDLE hFileMap = ::CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
			if (hFileMap)
			{
				m_pvFile = ::MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
				::CloseHandle(hFileMap);
			}
		}
		::CloseHandle(hFile);
	}
	if (!m_pvFile)
	{
		::MessageBox(NULL, "The new file could not be opened.", "ZEdit", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	// Adjust the pointers from the temp file to the newly mapped file
	UINT cchOffset = (char *)m_pvFile - (char *)0;
	pdp = m_pdpFirst;
	while (pdp)
	{
		for (int ipr = pdp->cpr; --ipr >= 0; )
		{
			if (ft != kftUnicode8 || !IsParaInMem(pdp, ipr))
				pdp->rgpv[ipr] = (char *)pdp->rgpv[ipr] + cchOffset;
		}
		pdp = pdp->pdpNext;
	}

	// Update the color scheme for this file.
	char * pszExt = strrchr(pszFilename, '.');
	m_pcsi = g_cs.GetColorScheme(pszExt ? pszExt + 1 : pszExt);
	AssertPtr(m_pcsi);

	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::Sort(UINT iprStart, UINT iprStop, bool fCaseSensitive, bool fDescending)
{
	// TODO: Provide a way to undo this eventually ?!?
	ClearRedo();
	ClearUndo();

	m_fModified = true;
	if (iprStop >= m_cpr)
		iprStop = m_cpr - 1;
	if (iprStart > iprStop)
		return false;
	UINT iStartPara;
	DocPart * pdpStart = GetPart(iprStart, true, NULL, &iStartPara);
	DocPart * pdp = pdpStart;
	UINT iprInPart = iStartPara;
	UINT cpr = iprStop - iprStart + 1;
	SortInfo * psi = new SortInfo[cpr];
	if (!psi)
		return false;
	SortInfo * psiPos = psi;
	DocPart * pdpLast = NULL;
	for (UINT ipr = 0; ipr < cpr; ipr++)
	{
		// If the last paragraph in the selection is empty, don't include it in the sort.
		if (ipr == cpr - 1 && CharsInPara(pdp, iprInPart) == 0)
		{
			iprStop--;
			cpr--;
			if (iprStart > iprStop)
				return false;
			break;
		}

		psiPos->pdp = pdp;
		psiPos->pv = pdp->rgpv[iprInPart];
		psiPos->cch = pdp->rgcch[iprInPart];
		if (++iprInPart >= pdp->cpr)
		{
			pdpLast = pdp;
			iprInPart = 0;
			pdp = pdp->pdpNext;
			if (!pdp && ipr != cpr - 1)
			{
				delete psi;
				return false;
			}
		}
		psiPos++;
	}

	CWaitCursor wc;
	CSort sort;
	try
	{
		if (m_ft == kftAnsi)
			sort.Sort(psi, cpr, 0, fCaseSensitive ? kctCompare8 : kctCompare8I);
		else if (m_ft == kftUnicode16)
			sort.Sort(psi, cpr, 0, fCaseSensitive ? kctCompare16 : kctCompare16I);
		else
			sort.Sort(psi, cpr, 0, fCaseSensitive ? kctCompare8_16 : kctCompare8_16I);
	}
	catch (...)
	{
		int a;
		a = 0;
		::MessageBox(m_pzef->GetHwnd(),
			"An error occurred while sorting the file.",
			g_pszAppName, MB_OK | MB_ICONINFORMATION);
	}

	if (iprStop == m_cpr - 1)
	{
		// We need to adjust the end of line characters for the old last line
		// and the new last line.
		Assert(pdpLast || pdp);

		if (--iprInPart == -1)
		{
			AssertPtr(pdpLast);
			iprInPart = pdpLast->cpr - 1;
			pdp = pdpLast;
		}

		// We need to find the index of the old last line in psi.
		SortInfo * psiNew = psi + cpr - 1;
		SortInfo * psiOld = psi;
		char * prgchTo = (char *)pdp->rgpv[iprInPart]; // Old last line.
		while (psiOld && psiOld->pv != prgchTo)
			psiOld++;
		AssertPtr(psiOld);
		Assert(CharsInPara(psiOld->cch) == PrintCharsInPara(psiOld->cch));

		if (psiOld != psiNew)
		{
			// Add CRLF characters to the end of the old last line.
			int cchNewPara = CharsInPara(psiOld->cch) + 2;
			char * prgchNew;
			if (m_ft == kftAnsi)
			{
				if ((prgchNew = new char[cchNewPara]) != NULL)
					memmove(prgchNew, prgchTo, cchNewPara - 2);
			}
			else if (m_ft == kftUnicode8 && !IsParaInMem(psiOld->cch))
			{
				if ((prgchNew = new char[prgchTo, cchNewPara * 2]) != NULL)
					Convert8to16(prgchTo, (wchar *)prgchNew, cchNewPara - 2);
			}
			else
			{
				if ((prgchNew = new char[prgchTo, cchNewPara * 2]) != NULL)
					memmove(prgchNew, prgchTo, cchNewPara * 2 - 4);
			}
			if (!prgchNew)
			{
				delete psi;
				return false;
			}
			if (m_ft == kftAnsi)
				memmove(prgchNew + cchNewPara - 2, g_pszEndOfLine[2], 2);
			else
				memmove((wchar *)prgchNew + cchNewPara - 2, g_pwszEndOfLine[2], 4);
			if (IsParaInMem(psiOld->cch))
				delete psiOld->pv;
			psiOld->cch |= kpmParaInMem;
			psiOld->pv = prgchNew;

			// Update the character count in the new and old last lines.
			Assert((psiNew->cch & (3 << kpmEndOfParaShift)) == kpmTwoEndOfPara);
			Assert((psiOld->cch & (3 << kpmEndOfParaShift)) == 0);
			psiNew->cch &= ~kpmTwoEndOfPara;
			psiOld->cch |= kpmTwoEndOfPara;
		}
	}

	pdp = pdpStart;
	iprInPart = iStartPara;
	psiPos = psi;
	if (fDescending)
		psiPos += cpr - 1;
	for (UINT ipr = 0; ipr < cpr; ipr++)
	{
		AssertPtr(pdp);
		pdp->rgpv[iprInPart] = psiPos->pv;
		pdp->rgcch[iprInPart] = psiPos->cch;
		pdp->cch += CharsInPara(psiPos->cch);
		psiPos->pdp->cch -= CharsInPara(psiPos->cch);
		if (++iprInPart >= pdp->cpr)
		{
			iprInPart = 0;
			pdp = pdp->pdpNext;
		}
		if (fDescending)
			psiPos--;
		else
			psiPos++;
	}

	delete psi;
	SetSelection(CharFromPara(iprStart), CharFromPara(iprStop + 1), true, true);
	return true;
}

/*----------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------*/
/*bool CZDoc::SetSelectionFromFileFind(DWORD ichMatch, DWORD dwLine, DWORD ichInPara)
{
	UINT ichSelStart = ichMatch;
	void * pv;
	UINT ipr;
	DocPart * pdp = GetPart(dwLine, true, &pv, &ipr);
	if (!IsAnsi(pdp, ipr))
	{
		DWORD icbInPara = ichInPara;
		wchar * pch = (wchar*)pv;
		wchar * pchStop = pch + CharsInPara(pdp, ipr);
		//while (pch < pchStop)
		while (icbInPara > 0)
		{
			wchar ch = *pch++;
			if (ch >= kulSurrogateHighStart && ch <= kulSurrogateHighEnd && pch < pchStop)
			{
				wchar ch2 = *pch;
				if (ch2 >= kulSurrogateLowStart && ch2 <= kulSurrogateLowEnd)
				{
					ch = ((ch - kulSurrogateHighStart) << knHalfShift) +
						(ch2 - kulSurrogateLowStart) + kulHalfBase;
					++pch;
				}
			}
			int cbToWrite = 2;
			if (ch < 0x80)
				cbToWrite = 1;
			else if (ch < 0x800)
				cbToWrite = 2;
			else if (ch < 0x10000)
				cbToWrite = 3;
			else if (ch < 0x200000)
				cbToWrite = 4;
			else if (ch < 0x4000000)
				cbToWrite = 5;
			else if (ch <= kulMaximumUCS4)
				cbToWrite = 6;

			icbInPara -= cbToWrite;
			ichSelStart -= (cbToWrite - 1);
		}
	}
	UINT ichSelStop = ichSelStart + pfifi->m_cchFindWhat;
	return SetSelection(ichSelStart, ichSelStop, true, true);
}*/


#ifdef SPELLING

/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::SpellCheck(UINT ichMin, UINT * pichBad, int * pcch)
{
	if (!m_pzef->CanSpellCheck())
		return false;
	if (pichBad)
		*pichBad = -1;
	if (pcch)
		*pcch = 0;
	if (ichMin > m_cch)
		return false;
	void * pv;
	UINT iprInPart;
	UINT ich;
	DocPart * pdp = GetPart(ichMin, false, &pv, &iprInPart, &ich);
	for (int i = 0; i < iprInPart; i++)
		ich += CharsInPara(pdp, i);

	CSpellCheck * psc = m_pzef->GetSpellCheck();
	int ichsMinBad;
	int ichsLimBad;
	int scrs;
	HRESULT hr;
	UINT cchPara = CharsInPara(pdp, iprInPart);
	UINT cchOffset;
	if (IsAnsi(pdp, iprInPart))
		cchOffset = ((char *)pv - (char *)pdp->rgpv[iprInPart]);
	else
		cchOffset = ((wchar *)pv - (wchar *)pdp->rgpv[iprInPart]);
	cchPara -= cchOffset;
	ich += cchOffset;
	while (pdp)
	{
		if (IsAnsi(pdp, iprInPart))
			hr = psc->Check((char *)pv, cchPara, &ichsMinBad, &ichsLimBad, &scrs);
		else
			hr = psc->Check((wchar *)pv, cchPara, &ichsMinBad, &ichsLimBad, &scrs);
		if (FAILED(hr))
			return false;
		if (ichsMinBad != -1)
		{
			if (pichBad)
				*pichBad = ich + ichsMinBad;
			if (pcch)
				*pcch = ichsLimBad - ichsMinBad;
			return true;
		}
		ich += cchPara;
		if (++iprInPart >= pdp->cpr)
		{
			pdp = pdp->pdpNext;
			if (!pdp)
				return true; // end of document
			iprInPart = 0;
		}
		pv = pdp->rgpv[iprInPart];
		cchPara = CharsInPara(pdp, iprInPart);
	}
	return false;
}

#endif // SPELLING


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::Undo()
{
	if (!m_stackUndo.empty())
	{
		CZDataObject * pzdo = NULL;
		UINT cch;
		m_fModified = true;
		CUndoRedo * pur = m_stackUndo.top();
		m_stackUndo.pop();

		UndoRedoType urt = pur->m_urt;
		if (urt == kurtMove)
		{
			if (pur->m_mt == kmtPaste)
				urt = kurtPaste;
			else if (pur->m_mt == kmtCut)
				urt = kurtCut;
		}

		switch (urt)
		{
			case kurtCut:
			case kurtDelete:
				cch = InsertText(pur->m_ichMin, pur->m_pzdo, 0, kurtEmpty);
				SetSelection(pur->m_ichMin, pur->m_ichMin + cch, true, true);
				break;
				
			case kurtPaste:
			case kurtReplace:
			case kurtTyping:
#ifdef SPELLING
			case kurtSpell:
#endif // SPELLING
			case kurtSmartIndent:
				AssertPtrN(pur->m_pzdo);
				GetText(pur->m_ichMin, pur->m_cch, &pzdo);
				DeleteText(pur->m_ichMin, pur->m_ichMin + pur->m_cch, false, false);
				if (pur->m_pzdo)
				{
					cch = InsertText(pur->m_ichMin, pur->m_pzdo, 0, kurtEmpty);
					pur->m_pzdo->Release();
				}
				else
				{
					cch = pur->m_cchReplaced;
				}
				pur->m_pzdo = pzdo;
				SetSelection(pur->m_ichMin, pur->m_ichMin + cch, true, true);
				break;

			case kurtReplaceAll:
			case kurtIndentIncrease:
			case kurtIndentDecrease:
				{
					CWaitCursor ec;
					CUndoRedo::ReplaceAllInfo * prai = pur->m_prai;
					AssertPtr(prai);
					CUndoRedo::ReplaceAllInfo * praiNew = new CUndoRedo::ReplaceAllInfo();
					if (prai->m_crals > 0)
					{
						CUndoRedo::ReplaceAllLocation * pralMin = prai->m_prgral;
						CUndoRedo::ReplaceAllLocation * pral = pralMin + prai->m_crals;
						while (--pral >= pralMin)
						{
							// NOTE: pzdo can be NULL.
							CZDataObject * pzdo = prai->m_prgzdo[pral->m_izdo];
							AssertPtrN(pzdo);
							UINT izdo = praiNew->AddReplaceAllItem(this, pral->m_ich,
								pral->m_cch, false);
							if (izdo == -1)
							{
								::MessageBox(m_pzef->GetHwnd(),
									"An error occurred while performing the Undo operation.",
									g_pszAppName, MB_OK | MB_ICONEXCLAMATION);
								break;
							}
							InsertText(pral->m_ich, pzdo, pral->m_cch, kurtEmpty);
							pral->m_cch = pzdo ? pzdo->GetCharCount() : 0;
							pral->m_izdo = izdo;
						}
					}
					// Swap the two arrays of CZDataObject pointers.
					UINT czdoT = pur->m_prai->m_czdo;
					CZDataObject ** prgzdoT = pur->m_prai->m_prgzdo;
					pur->m_prai->m_czdo = praiNew->m_czdo;
					pur->m_prai->m_prgzdo = praiNew->m_prgzdo;
					praiNew->m_czdo = czdoT;
					praiNew->m_prgzdo = prgzdoT;
					delete praiNew;

					if (urt == kurtIndentIncrease || urt == kurtIndentDecrease)
						SetSelection(pur->m_ichMin, pur->m_cch, true, true);
					else
						SetSelection(pur->m_ichMin, pur->m_ichMin, true, true);
					break;
				}

			case kurtMove:
				if (pur->m_mt == kmtCopy)
				{
					DeleteText(pur->m_ichMin, pur->m_ichMin + pur->m_cch, false, false);
					cch = pur->m_cch;
				}
				else
				{
					void * pv;
					GetText(pur->m_ichMin, pur->m_cch, &pv);
					if (!pv)
					{
						m_stackUndo.push(pur);
						return false;
					}
					DeleteText(pur->m_ichMin, pur->m_ichMin + pur->m_cch, false, false);
					cch = InsertText(pur->m_cchReplaced, pv, pur->m_cch, 0, m_ft == kftAnsi, kurtEmpty);
					delete pv;
				}
				SetSelection(pur->m_cchReplaced, pur->m_cchReplaced + pur->m_cch, true, true);
				break;

			default:
				return false;
		}

		m_stackRedo.push(pur);
		if (m_stackUndo.empty())
			m_fModified = false;
		m_pzef->UpdateTextPos();
		return true;
	}
	return false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZDoc::DeleteMemory(void * pv)
{
	if (pv)
	{
		DocPart * pdp = (DocPart *)pv;
		DocPart * pdpT;
		while ((pdpT = pdp) != NULL)
		{
			pdp = pdp->pdpNext;
			for (int ipr = pdpT->cpr; --ipr >= 0; )
			{
				if (IsParaInMem(pdpT, ipr))
					delete pdpT->rgpv[ipr];
			}
			delete pdpT;
		}
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::CheckFileTime()
{
	if (lstrcmp(m_szFilename, "Untitled") == 0)
		return false;
	HANDLE hFile = ::CreateFile(m_szFilename, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	FILETIME ft;
	if (!::GetFileTime(hFile, NULL, NULL, &ft))
		return false;
	::CloseHandle(hFile);
	if (::CompareFileTime(&ft, &m_ftModified) == 0)
		return false;

	// The file has been modified outside of ZEdit.
	char szMessage[MAX_PATH * 2];
	wsprintf(szMessage, "%s\n\nThis file has been modified outside of ZEdit.", m_szFilename);
	if (m_fModified)
		lstrcat(szMessage, "\nDo you want to reload it and lose the changes made in ZEdit?");
	else
		lstrcat(szMessage, " Do you want to reload it?");
	if (::MessageBox(NULL, szMessage, g_pszAppName, MB_YESNO | MB_ICONQUESTION |
		MB_DEFBUTTON2 | MB_TASKMODAL) == IDYES)
	{
		AssertPtr(m_pzvCurrent);
		UINT ichSelStart;
		UINT ichSelStop;
		m_pzvCurrent->GetSelection(&ichSelStart, &ichSelStop);
		UINT iSel = m_pzvCurrent->m_ichDragStart;
		if (iSel == ichSelStart)
			iSel = ichSelStop;
		else
			iSel = ichSelStart;
		// TODO: Make sure this works when multiple views are open.
		OpenFile(m_szFilename, NULL, NULL);
		SetSelection(iSel, iSel, true, true);
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZDoc::ClearRedo()
{
	CUndoRedo * pur;
	while (!m_stackRedo.empty())
	{
		pur = m_stackRedo.top();
		m_stackRedo.pop();
		delete pur;
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZDoc::ClearUndo()
{
	CUndoRedo * pur;
	while (!m_stackUndo.empty())
	{
		pur = m_stackUndo.top();
		m_stackUndo.pop();
		delete pur;
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
DocPart * CZDoc::GetPart(UINT cv, bool fParas, void ** ppv, UINT * pipr, UINT * pcchBefore,
	UINT * pcprBefore)
{
	AssertPtr(m_pdpFirst);
	AssertPtrN(ppv);
	AssertPtrN(pipr);
	AssertPtrN(pcchBefore);
	AssertPtrN(pcprBefore);
	if (m_fAccessError)
		return m_pdpFirst;

	if (fParas)
	{
		if (cv > m_cpr)
			cv = m_cpr;
	}
	else
	{
		if (cv > m_cch)
			cv = m_cch;
	}
	DocPart * pdp = m_pdpFirst;
	UINT cpr = pdp->cpr;
	UINT cch = pdp->cch;
	bool fEOF = false;
	if (fParas)
	{
		while (cpr <= cv)
		{
			if (!pdp->pdpNext)
			{
				Assert(cch == m_cch && cpr == cv && cv == m_cpr);
				fEOF = true;
				break;
			}
			pdp = pdp->pdpNext;
			cpr += pdp->cpr;
			cch += pdp->cch;
		}
	}
	else
	{
		while (cch <= cv)
		{
			if (!pdp->pdpNext)
			{
				Assert(cpr == m_cpr && cch == cv && cv == m_cch);
				fEOF = true;
				break;
			}
			pdp = pdp->pdpNext;
			cpr += pdp->cpr;
			cch += pdp->cch;
		}
	}

	if (pcchBefore)
		*pcchBefore = cch - pdp->cch;
	UINT ipr = 0;
	if (fParas)
	{
		ipr = cv - (cpr - pdp->cpr);
		Assert(ipr < kcprInPart || fEOF);
		if (ppv)
			*ppv = fEOF ? NULL : pdp->rgpv[ipr];
		if (pipr)
			*pipr = ipr;
	}
	else
	{
		cv -= (cch - pdp->cch);
		// INVALID COMMENT: This was a problem when returning the character offset of a LF at the end of
		// a paragraph. It went on to return the index of the next paragraph instead of
		// the current one.
		while (cv > 0 && ipr < pdp->cpr && cv >= CharsInPara(pdp, ipr))
		//while (cv > 0 && ipr < pdp->cpr && cv > CharsInPara(pdp, ipr))
			cv -= CharsInPara(pdp, ipr++);
		if (ipr >= pdp->cpr)
		{
			// This should only happen when getting the index of a character in the last paragraph.
			Assert(pdp->pdpNext == NULL);
			if (pdp->cpr > 0)
				ipr = pdp->cpr - 1;
			else
				ipr = 0;
			cv = CharsInPara(pdp, ipr);
		}
		if (ppv)
		{
			if (IsAnsi(pdp, ipr))
				*ppv = ((char *)pdp->rgpv[ipr]) + cv;
			else
				*ppv = ((wchar *)pdp->rgpv[ipr]) + cv;
		}
		if (pipr)
			*pipr = ipr;
	}
	if (pcprBefore)
		*pcprBefore = cpr - pdp->cpr;
#ifdef _DEBUG
	AssertPtr(pdp);
	Assert(!pipr || *pipr <= kcprInPart || fEOF);
	if (ppv)
	{
		if (IsAnsi(pdp, ipr))
			AssertPtrN((char *)(*ppv));
		else
			AssertPtrN((wchar *)(*ppv));
	}
#endif
	return pdp;
}


/***********************************************************************************************
	CUndoRedo methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CUndoRedo::CUndoRedo(UndoRedoType urt, UINT ichStart, UINT cch, UINT cchReplaced)
{
	m_urt = urt;
	m_ichMin = ichStart;
	m_cch = cch;
	m_cchReplaced = cchReplaced;
	m_pzdo = NULL;
	m_prai = NULL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CUndoRedo::~CUndoRedo()
{
	if (m_pzdo)
	{
		m_pzdo->Release();
		m_pzdo = NULL;
	}
	if (m_urt == kurtReplaceAll && m_prai)
	{
		delete m_prai;
		m_prai = NULL;
	}
}


/***********************************************************************************************
	CUndoRedo::ReplaceAllInfo methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
CUndoRedo::ReplaceAllInfo::ReplaceAllInfo()
{
	m_crals = 0;
	m_prgral = NULL;
	m_czdo = 0;
	m_prgzdo = NULL;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
CUndoRedo::ReplaceAllInfo::~ReplaceAllInfo()
{
	if (m_prgral)
	{
		delete m_prgral;
		m_prgral = NULL;
	}
	if (m_prgzdo)
	{
		for (UINT izdo = 0; izdo < m_czdo; izdo++)
		{
			CZDataObject * pzdo = m_prgzdo[izdo];
			AssertPtrN(pzdo);
			if (pzdo)
				pzdo->Release();
		}
		delete m_prgzdo;
		m_prgzdo = NULL;
	}
}


/*----------------------------------------------------------------------------------------------
	fUpdateLocation can be false to just get a list of unique CZDataObject objects.
----------------------------------------------------------------------------------------------*/
UINT CUndoRedo::ReplaceAllInfo::AddReplaceAllItem(CZDoc * pzd, UINT ichMatch, UINT cchMatch,
	bool fUpdateLocation)
{
	AssertPtr(pzd);

	CZDataObject * pzdoNew = NULL;
	if (pzd->GetText(ichMatch, cchMatch, &pzdoNew) != cchMatch)
		return false;

	// NOTE: pzdoNew can be NULL if cchMatch = 0.

	UINT izdoNew = -1;
	if (m_czdo)
	{
		// See if this is the same as an existing one.
		BOOL fAnsiNew;
		void * pvNew;
		UINT cchNew;
		bool fDelete;
		if (pzdoNew)
		{
			if (FAILED(pzdoNew->get_IsAnsi(&fAnsiNew)) ||
				FAILED(pzdoNew->GetText(fAnsiNew != 0, &pvNew, &cchNew, &fDelete)))
			{
				pzdoNew->Release();
				return false;
			}
		}

		UINT izdo = 0;
		for ( ; izdo < m_czdo; izdo++)
		{
			BOOL fAnsi;
			void * pv;
			UINT cch;
			CZDataObject * pzdo = m_prgzdo[izdo];
			AssertPtrN(pzdo);
			if (pzdo == NULL)
			{
				if (pzdoNew == NULL)
					break;
				continue;
			}
			if (FAILED(pzdo->get_IsAnsi(&fAnsi)) ||
				FAILED(pzdo->GetText(fAnsi != 0, &pv, &cch, &fDelete)))
			{
				pzdoNew->Release();
				return false;
			}
			int nShift = fAnsi ? 0 : 1;
			if (fAnsi == fAnsiNew && cch == cchNew && memcmp(pv, pvNew, cch << nShift) == 0)
				break;
		}
		if (izdo < m_czdo)
		{
			if (pzdoNew)
				pzdoNew->Release();
			pzdoNew = NULL;
			izdoNew = izdo;
		}
	}

	if (izdoNew == -1)
	{
		CZDataObject ** ppzdo = (CZDataObject **)realloc(m_prgzdo,
			sizeof(CZDataObject *) * (m_czdo + 1));
		if (!ppzdo)
		{
			pzdoNew->Release();
			return false;
		}
		izdoNew = m_czdo;
		m_prgzdo = ppzdo;
		ppzdo[m_czdo++] = pzdoNew;
	}

	if (fUpdateLocation)
	{
		UINT crals = m_crals;
		if (!(crals % 50))
		{
			ReplaceAllLocation * ppral = (ReplaceAllLocation *)realloc(m_prgral,
				sizeof(ReplaceAllLocation) * (crals + 50));
			if (!ppral)
				return false;
			m_prgral = ppral;
		}
		ReplaceAllLocation & ral = m_prgral[crals];
		ral.m_ich = ichMatch;
		ral.m_izdo = izdoNew;
		m_crals++;
	}

	return izdoNew;
}