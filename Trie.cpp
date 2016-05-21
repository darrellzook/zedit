#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"

/***********************************************************************************************
	CTrieLevel methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CTrieLevel::CTrieLevel(wchar ch)
{
	m_cte = 0;
	m_cr = (COLORREF)-1;
	m_ppte = NULL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CTrieLevel::~CTrieLevel()
{
	if (m_ppte)
	{
		for (int ite = 0; ite < m_cte; ite++)
			delete m_ppte[ite];
		delete m_ppte;
	}
}


/*----------------------------------------------------------------------------------------------
	Parameters:
		pszs: 8-bit key word to be added to the trie
----------------------------------------------------------------------------------------------*/
bool CTrieLevel::AddWord(char * pszs, COLORREF cr, BOOL fCaseMatters)
{
	AssertPtr(pszs);
	Assert(*pszs);

	int ite;
	char ch = fCaseMatters ? *pszs : tolower(*pszs);
	if (!(FindElement(ch, &ite)))
	{
		CTrieElement ** ppte = (CTrieElement **)realloc(m_ppte, sizeof(CTrieElement *) * ++m_cte);
		if (!ppte)
			return false;
		m_ppte = ppte;
		if (ite == -1)
			ite = 0;
		else
			memmove(&m_ppte[ite + 1], &m_ppte[ite], sizeof(CTrieElement *) * (m_cte - ite - 1));
		m_ppte[ite] = new CTrieElement(ch);
		if (!m_ppte[ite])
			return false;
		if (!m_ppte[ite]->CreateSubLevel())
			return false;
	}
	if (pszs[1] && !isspace(pszs[1]))
		return m_ppte[ite]->m_ptlSub->AddWord(pszs + 1, cr, fCaseMatters);
	else
		m_ppte[ite]->m_ptlSub->m_cr = cr;
	return true;
}


/*----------------------------------------------------------------------------------------------
	Parameters:
		pszw: 16-bit key word to be added to the trie
----------------------------------------------------------------------------------------------*/
bool CTrieLevel::AddWord(wchar * pszw, COLORREF cr, BOOL fCaseMatters)
{
	AssertPtr(pszw);
	Assert(*pszw);

	int ite;
	wchar ch = fCaseMatters ? *pszw : towlower(*pszw);
	if (!(FindElement(ch, &ite)))
	{
		CTrieElement ** ppte = (CTrieElement **)realloc(m_ppte, sizeof(CTrieElement *) * ++m_cte);
		if (!ppte)
			return false;
		m_ppte = ppte;
		if (ite == -1)
			ite = 0;
		else
			memmove(&m_ppte[ite + 1], &m_ppte[ite], sizeof(CTrieElement *) * (m_cte - ite - 1));
		m_ppte[ite] = new CTrieElement(ch);
		if (!m_ppte[ite])
			return false;
		if (!m_ppte[ite]->CreateSubLevel())
			return false;
	}
	if (pszw[1] && !iswspace(pszw[1]))
		return m_ppte[ite]->m_ptlSub->AddWord(pszw + 1, cr, fCaseMatters);
	else
		m_ppte[ite]->m_ptlSub->m_cr = cr;
	return true;
}


/*----------------------------------------------------------------------------------------------
	Parameters:
		ch:	Character to search for in the current trie level
		pite: Will contain the position of the element if it is found, or
			the position it should be added to
----------------------------------------------------------------------------------------------*/
bool CTrieLevel::FindElement(wchar ch, int * pite)
{
	AssertPtr(pite);

	int iLower = 0;
	int iUpper = m_cte - 1;
	int iMiddle = 0;
	while (iLower <= iUpper)
	{
		iMiddle = (iLower + iUpper) >> 1;
		if (ch < m_ppte[iMiddle]->m_ch)
			iUpper = iMiddle - 1;
		else if (ch > m_ppte[iMiddle]->m_ch)
			iLower = iMiddle + 1;
		else
		{
			*pite = iMiddle;
			return true;
		}
	}
	*pite = m_ppte ? iLower : -1;
	return false;
}


/*----------------------------------------------------------------------------------------------
	Parameters:
		prgch:  8-bit key string to find
		cchLim: Length of prgch
		pcch:   Will contain the length of the matched word if there is one.
			This can be NULL if the length is not needed. If it is not
			NULL, it should be initialized to 0 before the call.
----------------------------------------------------------------------------------------------*/
bool CTrieLevel::FindWord(char * prgch, int cchLim, const char * pszDelim, COLORREF * pcr,
	int * pcch, BOOL fCaseMatters)
{
	AssertArray(prgch, cchLim);
	AssertPtr(pcr);
	AssertPtr(pszDelim);
	int ite;

	*pcr = m_cr;
	if (!cchLim || !FindElement(fCaseMatters ? *prgch : tolower(*prgch), &ite))
		return m_cr != (COLORREF)-1;
	if (pcch)
		(*pcch)++;
	return m_ppte[ite]->m_ptlSub->FindWord(prgch + 1, cchLim - 1, pszDelim, pcr, pcch, fCaseMatters);
}


/*----------------------------------------------------------------------------------------------
	Parameters:
		prgwch: 16-bit key string to find
		cchLim: Length of prgwch
		pcch:   Will contain the length of the matched word if there is one.
			This can be NULL if the length is not needed. If it is not
			NULL, it should be initialized to 0 before the call.
----------------------------------------------------------------------------------------------*/
bool CTrieLevel::FindWord(wchar * prgwch, int cchLim, const char * pszDelim, COLORREF * pcr,
	int * pcch, BOOL fCaseMatters)
{
	AssertArray(prgwch, cchLim);
	AssertPtr(pcr);
	AssertPtr(pszDelim);
	int ite;

	*pcr = m_cr;
	if (!cchLim || !FindElement(fCaseMatters ? *prgwch : towlower(*prgwch), &ite))
		return m_cr != (COLORREF)-1;
	if (pcch)
		(*pcch)++;
	return m_ppte[ite]->m_ptlSub->FindWord(prgwch + 1, cchLim - 1, pszDelim, pcr, pcch, fCaseMatters);
}


/***********************************************************************************************
	CTrieElement methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CTrieElement::CTrieElement(wchar ch)
{
	m_ch = ch;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CTrieElement::~CTrieElement()
{
	if (m_ptlSub)
		delete m_ptlSub;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CTrieElement::CreateSubLevel()
{
	m_ptlSub = new CTrieLevel(m_ch);
	return (m_ptlSub != NULL);
}