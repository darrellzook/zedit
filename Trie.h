///////////////////////////////////////////////////////////////////////////
// Declaration of the CTrieLevel and CTrieElement classes
///////////////////////////////////////////////////////////////////////////

#ifndef _TRIE_H_
#define _TRUE_H_

#include "Common.h"

class CTrieLevel
{
public:
	CTrieLevel(wchar ch);
	~CTrieLevel();
	bool AddWord(char * pszs, COLORREF cr, BOOL fCaseMatters);
	bool AddWord(wchar * pszw, COLORREF cr, BOOL fCaseMatters);
	bool FindWord(char * prgch, int cchLim, const char * pszDelim, COLORREF * pcr, int * pcch, BOOL fCaseMatters);
	bool FindWord(wchar * prgwch, int cchLim, const char * pszDelim, COLORREF * pcr, int* pcch, BOOL fCaseMatters);

protected:
	bool FindElement(wchar ch, int * pite);

	COLORREF m_cr;
	int m_cte;
	CTrieElement ** m_ppte;
};

class CTrieElement
{
	friend CTrieLevel;

public:
	CTrieElement(wchar ch);
	~CTrieElement();
	bool CreateSubLevel();

protected:
	wchar m_ch;
	CTrieLevel * m_ptlSub;
};

#endif // !_TRIE_H_