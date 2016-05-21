#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"

#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <process.h>

const ULONG kulReplacementCharacter = 0x0000FFFDUL;
const ULONG kulMaximumSimpleUniChar = 0x0000FFFFUL;
const ULONG kulMaximumUniChar = 0x0010FFFFUL;
const ULONG kulMaximumUCS4 = 0x7FFFFFFFUL;

const int knHalfShift = 10;
const ULONG kulHalfBase = 0x0010000UL;
const ULONG kulHalfMask = 0x3FFUL;
const ULONG kulSurrogateHighStart = 0xD800UL;
const ULONG kulSurrogateHighEnd = 0xDBFFUL;
const ULONG kulSurrogateLowStart = 0xDC00UL;
const ULONG kulSurrogateLowEnd = 0xDFFFUL;

ULONG krgulOffsetsFromUTF8[6] = {
	0x00000000UL, 0x00003080UL, 0x000E2080UL,
	0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

char krgcbFromUTF8[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

unsigned char krguchFirstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};

const char * kpszInvalidChars = "This file contained some characters that could not be "
	"represented in an ANSI file. The top byte of these characters was ignored.";


/*----------------------------------------------------------------------------------------------
	No bounds checking on pDstStart is performed by this function, so
	the caller must insure that enough space is allocated to store the result.
	The easiest way to make sure is to set pDstStart to a buffer that is
	double the size of the buffer (in wide characters) pointed to by pSrcStart.
----------------------------------------------------------------------------------------------*/
int CZDoc::UTF8ToUTF16(char * prgchSrc, int cchSrc, wchar * prgwchDst)
{
	char * prgchSrcStop = prgchSrc + cchSrc;
	wchar * prgwch = prgwchDst;
	ULONG ch;
	unsigned short cExtraBytesToWrite;

	while (prgchSrc < prgchSrcStop)
	{
		ch = 0;
		switch (cExtraBytesToWrite = krgcbFromUTF8[(UCHAR)*prgchSrc])
		{
			case 5: ch += (UCHAR)*prgchSrc++; ch <<= 6;
			case 4: ch += (UCHAR)*prgchSrc++; ch <<= 6;
			case 3: ch += (UCHAR)*prgchSrc++; ch <<= 6;
			case 2: ch += (UCHAR)*prgchSrc++; ch <<= 6;
			case 1: ch += (UCHAR)*prgchSrc++; ch <<= 6;
			case 0: ch += (UCHAR)*prgchSrc++;
		}
		ch -= krgulOffsetsFromUTF8[cExtraBytesToWrite];

		if (ch <= kulMaximumSimpleUniChar)
		{
			*prgwch++ = (wchar)ch;
		}
		else if (ch > kulMaximumUniChar)
		{
			*prgwch++ = kulReplacementCharacter;
		}
		else
		{
			ch -= kulHalfBase;
			*prgwch++ = (wchar)((ch >> knHalfShift) + kulSurrogateHighStart);
			*prgwch++ = (wchar)((ch & kulHalfMask) + kulSurrogateLowStart);
		}
	}
	return prgwch - prgwchDst;
}


/*----------------------------------------------------------------------------------------------
	No bounds checking on pDstStart is performed by this function, so
	the caller must insure that enough space is allocated to store the result.
	The easiest way to make sure is to set pDstStart to a buffer that is
	six times the size of the buffer (in characters) pointed to by pSrcStart.
----------------------------------------------------------------------------------------------*/
int CZDoc::UTF16ToUTF8(wchar * prgwchSrc, int cchSrc, char * prgchDst)
{
	wchar * prgwchSrcStop = prgwchSrc + cchSrc;
	char * prgch = prgchDst;
	ULONG ch, ch2;
	unsigned short cbToWrite = 0;
	const ULONG kulByteMask = 0xBF;
	const ULONG kulByteMark = 0x80;

	while (prgwchSrc < prgwchSrcStop)
	{
		ch = *prgwchSrc++;
		if (ch >= kulSurrogateHighStart && ch <= kulSurrogateHighEnd && prgwchSrc < prgwchSrcStop)
		{
			ch2 = *prgwchSrc;
			if (ch2 >= kulSurrogateLowStart && ch2 <= kulSurrogateLowEnd)
			{
				ch = ((ch - kulSurrogateHighStart) << knHalfShift) +
					 (ch2 - kulSurrogateLowStart) + kulHalfBase;
				++prgwchSrc;
			}
		}
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
		else
		{
			cbToWrite = 2;
			ch = kulReplacementCharacter;
		}

		prgch += cbToWrite;
		switch (cbToWrite)
		{
			case 6: *--prgch = (char)((ch | kulByteMark) & kulByteMask); ch >>= 6;
			case 5: *--prgch = (char)((ch | kulByteMark) & kulByteMask); ch >>= 6;
			case 4: *--prgch = (char)((ch | kulByteMark) & kulByteMask); ch >>= 6;
			case 3: *--prgch = (char)((ch | kulByteMark) & kulByteMask); ch >>= 6;
			case 2: *--prgch = (char)((ch | kulByteMark) & kulByteMask); ch >>= 6;
			case 1: *--prgch = (char)(ch | krguchFirstByteMark[cbToWrite]);
		}
		prgch += cbToWrite;
	}
	return prgch - prgchDst;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::WriteANSItoANSIFile(HANDLE hFile, HWND hwndProgress)
{
	UINT cpr;
	UINT cch;
	DWORD dwBytesWritten;
	char * pFileOffsetPos = 0;
	char rgchBuffer[10000];
	char * prgchPos = rgchBuffer;
	char * prgchStop = rgchBuffer + sizeof(rgchBuffer);
	DocPart * pdp = m_pdpFirst;

	while (pdp)
	{
		cpr = pdp->cpr;
		for (UINT ipr = 0; ipr < cpr; ipr++)
		{
			cch = CharsInPara(pdp, ipr);
			if (prgchPos + cch < prgchStop)
			{
				memmove(prgchPos, pdp->rgpv[ipr], cch);
				prgchPos += cch;
			}
			else
			{
				if (prgchPos != rgchBuffer)
					WriteFile(hFile, rgchBuffer, prgchPos - rgchBuffer, &dwBytesWritten, NULL);
				WriteFile(hFile, pdp->rgpv[ipr], cch, &dwBytesWritten, NULL);
				prgchPos = rgchBuffer;
			}
			if (IsParaInMem(pdp, ipr))
			{
				delete pdp->rgpv[ipr];
				pdp->rgcch[ipr] &= ~kpmParaInMem;
			}
			pdp->rgpv[ipr] = pFileOffsetPos;
			pFileOffsetPos += cch;
		}
		SendMessage(hwndProgress, PBM_DELTAPOS, pdp->cch, 0);
		pdp = pdp->pdpNext;
	}
	if (prgchPos != rgchBuffer)
		WriteFile(hFile, rgchBuffer, prgchPos - rgchBuffer, &dwBytesWritten, NULL);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::WriteANSItoUTF8File(HANDLE hFile, HWND hwndProgress)
{
	UINT cpr;
	UINT cch;
	DWORD dwBytesWritten;
	char rgchBuffer[2048];
	char * pFileOffsetPos = 0;
	char * prgchBuffer;
	char * prgchDst;
	char * prgchPos;
	char * prgchStop;
	ULONG ch;
	DocPart * pdp = m_pdpFirst;

	while (pdp)
	{
		cpr = pdp->cpr;
		for (UINT ipr = 0; ipr < cpr; ipr++)
		{
			cch = CharsInPara(pdp, ipr);
			if (cch <= 1024)
				prgchBuffer = rgchBuffer;
			else if (!(prgchBuffer = new char[cch << 1]))
				return false;
			prgchPos = (char *)pdp->rgpv[ipr];
			prgchStop = prgchPos + cch;
			prgchDst = prgchBuffer;
			while (prgchPos < prgchStop)
			{
				/*	This is taken from UTF16ToUTF8 (defined above). Since we're going
					from ANSI to UTF-8, we only have two cases instead of six. */
				ch = (UCHAR)*prgchPos++;
				if (ch < 0x80)
				{
					*prgchDst++ = (char)ch;
				}
				else
				{
					*prgchDst++ = (char)((ch >> 6) | 0xC0);
					*prgchDst++ = (char)(ch & 0xBF);
				}
			}
			WriteFile(hFile, prgchBuffer, prgchDst - prgchBuffer, &dwBytesWritten, NULL);
			if (prgchBuffer != rgchBuffer)
				delete prgchBuffer;

			// Do stuff necessary for reading the new file as UTF-8
			if (prgchDst - prgchBuffer == cch)
			{
				// Everything in this paragraph was ANSI, so it doesn't have to be read into memory.
				if (IsParaInMem(pdp, ipr))
				{
					delete pdp->rgpv[ipr];
					pdp->rgcch[ipr] &= ~kpmParaInMem;
				}
				pdp->rgpv[ipr] = pFileOffsetPos;
			}
			else
			{
				// This string needs to be put into memory as UTF16.
				if (!(prgchBuffer = (char *)new wchar[cch]))
					return false;
				Convert8to16((char *)pdp->rgpv[ipr], (wchar *)prgchBuffer, cch);
				if (IsParaInMem(pdp, ipr))
					delete pdp->rgpv[ipr];
				else
					pdp->rgcch[ipr] |= kpmParaInMem;
				pdp->rgpv[ipr] = prgchBuffer;
			}
			pFileOffsetPos += (prgchDst - prgchBuffer);
		}
		SendMessage(hwndProgress, PBM_DELTAPOS, pdp->cch, 0);
		pdp = pdp->pdpNext;
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::WriteANSItoUTF16File(HANDLE hFile, HWND hwndProgress)
{
	UINT cpr;
	UINT cch;
	DWORD dwBytesWritten;
	wchar rgwchBuffer[1024];
	wchar * prgwchBuffer;
	wchar * pFileOffsetPos = (wchar *)0;
	DocPart * pdp = m_pdpFirst;

	WriteFile(hFile, "ÿþ", 2, &dwBytesWritten, NULL);
	while (pdp)
	{
		cpr = pdp->cpr;
		for (UINT ipr = 0; ipr < cpr; ipr++)
		{
			cch = CharsInPara(pdp, ipr);
			if (cch <= 512)
			{
				prgwchBuffer = rgwchBuffer;
			}
			else
			{
				prgwchBuffer = (wchar *)new wchar[cch];
				if (!prgwchBuffer)
					return false;
			}
			Convert8to16((char *)pdp->rgpv[ipr], prgwchBuffer, cch);
			WriteFile(hFile, prgwchBuffer, cch << 1, &dwBytesWritten, NULL);
			if (prgwchBuffer != rgwchBuffer)
				delete prgwchBuffer;
			if (IsParaInMem(pdp, ipr))
			{
				delete pdp->rgpv[ipr];
				pdp->rgcch[ipr] &= ~kpmParaInMem;
			}
			pdp->rgpv[ipr] = pFileOffsetPos;
			pFileOffsetPos += cch;
		}
		SendMessage(hwndProgress, PBM_DELTAPOS, pdp->cch, 0);
		pdp = pdp->pdpNext;
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::WriteUTF8toANSIFile(HANDLE hFile, HWND hwndProgress)
{
	UINT cpr;
	UINT cch;
	DWORD dwBytesWritten;
	char rgchBuffer[1024];
	char * prgchBuffer;
	char * prgchDst;
	char * pFileOffsetPos = 0;
	wchar * prgwchSrc;
	wchar * prgwchStop;
	wchar wch;
	bool fInvalid = false;
	DocPart * pdp = m_pdpFirst;

	while (pdp)
	{
		cpr = pdp->cpr;
		for (UINT ipr = 0; ipr < cpr; ipr++)
		{
			cch = CharsInPara(pdp, ipr);
			if (IsParaInMem(pdp, ipr))
			{
				// The paragraph is 16-bit, so convert to ANSI and write it out to the file.
				if (cch <= 1024)
					prgchBuffer = rgchBuffer;
				else if (!(prgchBuffer = new char[cch]))
					return false;
				prgwchSrc = (wchar *)pdp->rgpv[ipr];
				prgwchStop = prgwchSrc + cch;
				prgchDst = prgchBuffer;
				while (prgwchSrc < prgwchStop)
				{
					if ((wch = *prgwchSrc++) > 0xFF)
						fInvalid = true;
					*prgchDst++ = (char)wch;
				}
				WriteFile(hFile, prgchBuffer, cch, &dwBytesWritten, NULL);
				if (prgchBuffer != rgchBuffer)
					delete prgchBuffer;
				delete pdp->rgpv[ipr];
				pdp->rgcch[ipr] &= ~kpmParaInMem;
			}
			else
			{
				// The paragraph is already 8-bit, so just write it out to the file.
				WriteFile(hFile, pdp->rgpv[ipr], cch, &dwBytesWritten, NULL);
			}
			pdp->rgpv[ipr] = pFileOffsetPos;
			pFileOffsetPos += cch;
		}
		SendMessage(hwndProgress, PBM_DELTAPOS, pdp->cch, 0);
		pdp = pdp->pdpNext;
	}
	if (fInvalid)
		MessageBox(NULL, kpszInvalidChars, "Conversion Error", MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::WriteUTF8toUTF8File(HANDLE hFile, HWND hwndProgress)
{
	UINT cpr;
	UINT cch;
	UINT cchDst;
	DWORD dwBytesWritten;
	char rgchBuffer[6144];
	char * prgchBuffer;
	char * pFileOffsetPos = 0;
	DocPart * pdp = m_pdpFirst;

	while (pdp)
	{
		cpr = pdp->cpr;
		for (UINT ipr = 0; ipr < cpr; ipr++)
		{
			cch = CharsInPara(pdp, ipr);
			if (IsParaInMem(pdp, ipr))
			{
				// The paragraph is 16-bit, so convert to UTF-8 and write it out to the file.
				if (cch <= 1024) // assume worst case => 1 UTF-16 character = 6 UTF-8 characters
					prgchBuffer = rgchBuffer;
				else if (!(prgchBuffer = new char[cch * 6]))
					return false;
				cchDst = UTF16ToUTF8((wchar *)pdp->rgpv[ipr], cch, prgchBuffer);
				/*	If the paragraph contains at least one non-ANSI character, keep the
					paragraph in memory and don't change the pointer for the paragraph. */
				WriteFile(hFile, prgchBuffer, cchDst, &dwBytesWritten, NULL);
				if (cchDst == cch)
				{
					// The paragraph contained only ANSI characters.
					delete pdp->rgpv[ipr];
					pdp->rgpv[ipr] = pFileOffsetPos;
					pdp->rgcch[ipr] &= ~kpmParaInMem;
				}
				if (prgchBuffer != rgchBuffer)
					delete prgchBuffer;
				if (cchDst == 0)
					return false;
			}
			else
			{
				// The paragraph consists only of 8-bit characters, so just write it out to the file.
				WriteFile(hFile, pdp->rgpv[ipr], cch, &dwBytesWritten, NULL);
				pdp->rgpv[ipr] = pFileOffsetPos;
				cchDst = cch;
			}
			pFileOffsetPos += cchDst;
		}
		SendMessage(hwndProgress, PBM_DELTAPOS, pdp->cch, 0);
		pdp = pdp->pdpNext;
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::WriteUTF8toUTF16File(HANDLE hFile, HWND hwndProgress)
{
	UINT cpr;
	UINT cch;
	DWORD dwBytesWritten;
	wchar rgwchBuffer[1024];
	wchar * prgwchBuffer;
	wchar * pFileOffsetPos = (wchar *)0;
	DocPart * pdp = m_pdpFirst;

	WriteFile(hFile, "ÿþ", 2, &dwBytesWritten, NULL);
	while (pdp)
	{
		cpr = pdp->cpr;
		for (UINT ipr = 0; ipr < cpr; ipr++)
		{
			cch = CharsInPara(pdp, ipr);
			if (IsParaInMem(pdp, ipr))
			{
				// The paragraph consists only of 16-bit characters, so just write it out to the file.
				WriteFile(hFile, pdp->rgpv[ipr], cch << 1, &dwBytesWritten, NULL);
				delete pdp->rgpv[ipr];
				pdp->rgcch[ipr] &= ~kpmParaInMem;
			}
			else
			{
				if (cch <= 1024)
					prgwchBuffer = rgwchBuffer;
				else if (!(prgwchBuffer = new wchar[cch]))
					return false;
				Convert8to16((char *)pdp->rgpv[ipr], prgwchBuffer, cch);
				WriteFile(hFile, prgwchBuffer, cch << 1, &dwBytesWritten, NULL);
				if (prgwchBuffer != rgwchBuffer)
					delete prgwchBuffer;
			}
			pdp->rgpv[ipr] = pFileOffsetPos;
			pFileOffsetPos += cch;
		}
		SendMessage(hwndProgress, PBM_DELTAPOS, pdp->cch, 0);
		pdp = pdp->pdpNext;
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::WriteUTF16toANSIFile(HANDLE hFile, HWND hwndProgress)
{
	UINT cpr;
	UINT cch;
	DWORD dwBytesWritten;
	char rgchBuffer[1024];
	char * prgchBuffer;
	char * prgchDst;
	char * pFileOffsetPos = 0;
	wchar * prgwchSrc;
	wchar * prgwchStop;
	wchar wch;
	bool fInvalid = false;
	DocPart * pdp = m_pdpFirst;

	while (pdp)
	{
		cpr = pdp->cpr;
		for (UINT ipr = 0; ipr < cpr; ipr++)
		{
			cch = CharsInPara(pdp, ipr);
			if (cch <= 1024)
				prgchBuffer = rgchBuffer;
			else if (!(prgchBuffer = new char[cch]))
				return false;
			prgwchSrc = (wchar *)pdp->rgpv[ipr];
			prgwchStop = prgwchSrc + cch;
			prgchDst = prgchBuffer;
			while (prgwchSrc < prgwchStop)
			{
				if ((wch = *prgwchSrc++) > 0xFF)
					fInvalid = true;
				*prgchDst++ = (char)wch;
			}
			WriteFile(hFile, prgchBuffer, cch, &dwBytesWritten, NULL);
			if (prgchBuffer != rgchBuffer)
				delete prgchBuffer;
			if (IsParaInMem(pdp, ipr))
			{
				delete pdp->rgpv[ipr];
				pdp->rgcch[ipr] &= ~kpmParaInMem;
			}
			pdp->rgpv[ipr] = pFileOffsetPos;
			pFileOffsetPos += cch;
		}
		SendMessage(hwndProgress, PBM_DELTAPOS, pdp->cch, 0);
		pdp = pdp->pdpNext;
	}
	if (fInvalid)
		MessageBox(NULL, kpszInvalidChars, "Conversion Error", MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::WriteUTF16toUTF8File(HANDLE hFile, HWND hwndProgress)
{
	UINT cpr;
	UINT cch;
	UINT cchDst;
	DWORD dwBytesWritten;
	char rgchBuffer[6144];
	char * prgchBuffer;
	char * pFileOffsetPos = 0;
	DocPart * pdp = m_pdpFirst;

	while (pdp)
	{
		cpr = pdp->cpr;
		for (UINT ipr = 0; ipr < cpr; ipr++)
		{
			cch = CharsInPara(pdp, ipr);
			if (cch <= 1024)
				prgchBuffer = rgchBuffer;
			else if (!(prgchBuffer = new char[cch * 6]))
				return false;
			cchDst = UTF16ToUTF8((wchar *)pdp->rgpv[ipr], cch, prgchBuffer);
			if (cchDst > 0)
			{
				WriteFile(hFile, prgchBuffer, cchDst, &dwBytesWritten, NULL);
				if (cchDst == cch)
				{
					// The paragraph contained only ANSI characters.
					if (IsParaInMem(pdp, ipr))
						delete pdp->rgpv[ipr];
					pdp->rgpv[ipr] = pFileOffsetPos;
					pdp->rgcch[ipr] &= ~kpmParaInMem;
				}
				else
				{
					/*	The paragraph contained non-ANSI characters, to load it in
						memory if it's not already there. */
					if (!IsParaInMem(pdp, ipr))
					{
						wchar * prgwchBuffer;
						if (!(prgwchBuffer = new wchar[cch]))
						{
							if (prgchBuffer != rgchBuffer)
								delete prgchBuffer;
							return false;
						}
						memmove(prgwchBuffer, pdp->rgpv[ipr], cch << 1);
						pdp->rgpv[ipr] = prgwchBuffer;
						pdp->rgcch[ipr] |= kpmParaInMem;
					}
				}
				if (prgchBuffer != rgchBuffer)
					delete prgchBuffer;
			}
			else
			{
				if (prgchBuffer != rgchBuffer)
					delete prgchBuffer;
				return false;
			}
			pFileOffsetPos += cchDst;
		}
		SendMessage(hwndProgress, PBM_DELTAPOS, pdp->cch, 0);
		pdp = pdp->pdpNext;
	}
	return true;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZDoc::WriteUTF16toUTF16File(HANDLE hFile, HWND hwndProgress)
{
	UINT cpr;
	UINT cch;
	DWORD dwBytesWritten;
	wchar * pFileOffsetPos = (wchar *)0;
	wchar rgwchText[10000];
	wchar * prgwchText = rgwchText;
	wchar * prgwchStop = rgwchText + (sizeof(rgwchText) >> 1);
	DocPart * pdp = m_pdpFirst;

	if (pdp != NULL && pdp->cpr > 0 && CharsInPara(pdp, 0) > 0 && ((wchar *)pdp->rgpv[0])[0] != (wchar)0xFEFF)
		WriteFile(hFile, "ÿþ", 2, &dwBytesWritten, NULL);

	int idp = 0;
	while (pdp)
	{
		cpr = pdp->cpr;
		for (UINT ipr = 0; ipr < cpr; ipr++)
		{
			cch = CharsInPara(pdp, ipr);
			if (prgwchText + cch < prgwchStop)
			{
				memmove(prgwchText, pdp->rgpv[ipr], cch << 1);
				prgwchText += cch;
			}
			else
			{
				if (prgwchText != rgwchText)
					WriteFile(hFile, rgwchText, (prgwchText - rgwchText) << 1, &dwBytesWritten, NULL);
				WriteFile(hFile, pdp->rgpv[ipr], cch << 1, &dwBytesWritten, NULL);
				prgwchText = rgwchText;
			}
			if (IsParaInMem(pdp, ipr))
			{
				delete pdp->rgpv[ipr];
				pdp->rgcch[ipr] &= ~kpmParaInMem;
			}
			pdp->rgpv[ipr] = pFileOffsetPos;
			pFileOffsetPos += cch;
		}
		SendMessage(hwndProgress, PBM_DELTAPOS, pdp->cch, 0);
		pdp = pdp->pdpNext;
		idp++;
	}
	if (prgwchText != rgwchText)
		WriteFile(hFile, rgwchText, (prgwchText - rgwchText) << 1, &dwBytesWritten, NULL);
	return true;
}