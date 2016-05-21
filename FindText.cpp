#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "common.h"


/***********************************************************************************************
	CFindText methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CFindText::CFindText(CZDoc * pzd)
{
	memset(this, 0, sizeof(CFindText));
	m_pzd = pzd;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CFindText::~CFindText()
{
}


/*----------------------------------------------------------------------------------------------
	This method returns true if it found whitespace.
	cchOffset should be -1, 0, or 1.
----------------------------------------------------------------------------------------------*/
bool CFindText::CheckForWholeWord(bool fBefore, int cchOffset)
{
	bool fAnsi = m_pzd->IsAnsi(m_pdpStart, m_iprMin);
	void * pvParaStart = m_pdpStart->rgpv[m_iprMin];

	if (fBefore)
	{
		// Check the previous character to see if it's whitespace.
		if (fAnsi)
		{
			if (m_pvPos <= (char *)pvParaStart - cchOffset)
				return true;

			char ch = *((char *)m_pvPos + (2 * cchOffset));
			if (isspace(ch) != 0)
				return true;
			if (ispunct(ch) != 0)
				return true;
		}
		else
		{
			if (m_pvPos <= (wchar *)pvParaStart + cchOffset)
				return true;

			wchar ch = *((wchar *)m_pvPos + (2 * cchOffset));
			if (iswspace(ch) != 0)
				return true;
			if (iswpunct(ch) != 0)
				return true;
		}
	}
	else
	{
		// Check the next character to see if it's whitespace.
		if (fAnsi)
		{
			if (m_pvPos >= (char *)pvParaStart + PrintCharsInPara(m_pdpStart, m_iprMin) - cchOffset - 1)
				return true;

			char ch = *((char *)m_pvPos + (2 * cchOffset));
			if (isspace(ch) != 0)
				return true;
			if (ispunct(ch) != 0)
				return true;
		}
		else
		{
			if (m_pvPos >= (wchar *)pvParaStart + PrintCharsInPara(m_pdpStart, m_iprMin) + cchOffset)
				return true;

			wchar ch = *((wchar *)m_pvPos + (2 * cchOffset));
			if (iswspace(ch) != 0)
				return true;
			if (iswpunct(ch) != 0)
				return true;
		}
	}

	return false;
}


/*----------------------------------------------------------------------------------------------
	This method returns true if a match has been found.
	If a match hasn't been found, the search position in the document is moved back
	if necessary and the position in the Find What text is reset to the beginning.
----------------------------------------------------------------------------------------------*/
bool CFindText::CheckCharDown(bool fEqual)
{
	if (fEqual)
	{
		if (m_cMatched == 0)
		{
			// Store information on where to start searching again if this is not a match
			m_pvOldPos = m_pvPos;
			m_pvOldLimit = m_pvLimit;
			m_pdpOld = m_pdpStart;
			m_iprOld = m_iprMin;
			m_ichOldFound = m_ichFound;

			if (m_fWholeWord && !CheckForWholeWord(true, -1))
				return CheckCharDown(false);
		}
		m_cMatched++;
		// Update the position in the Find What text and return true if we've found a match.
		if (++m_pwszFindPos - m_pwszFindWhat == m_cchFind)
		{
			if (m_fWholeWord && !CheckForWholeWord(false, 0))
				return CheckCharDown(false);
			return true;
		}
	}
	else
	{
		// A match was not made, so go back to the previously stored position and start searching again.
		if (m_cMatched > 0 && m_pvOldPos != m_pvOldLimit)
		{
			m_pvPos = m_pvOldPos;
			m_pvLimit = m_pvOldLimit;
			m_pdpStart = m_pdpOld;
			m_iprMin = m_iprOld;
			m_ichFound = m_ichOldFound;

			m_pvStart = m_pdpStart->rgpv[m_iprMin];
		}
		m_cMatched = 0;
		m_pwszFindPos = m_pwszFindWhat;
	}
	return false;
}


/*----------------------------------------------------------------------------------------------
	This method returns true if a match has been found.
	If a match hasn't been found, the search position in the document is moved forward
	if necessary and the position in the Find What text is reset to the end.
----------------------------------------------------------------------------------------------*/
bool CFindText::CheckCharUp(bool fEqual)
{
	if (fEqual)
	{
		if (m_cMatched == 0)
		{
			// Store information on where to start searching again if this is not a match
			m_pvOldPos = m_pvPos;
			m_pvOldLimit = m_pvLimit;
			m_pdpOld = m_pdpStart;
			m_iprOld = m_iprMin;
			m_ichOldFound = m_ichFound;

			if (m_fWholeWord && !CheckForWholeWord(false, 1))
				return CheckCharUp(false);
		}
		m_cMatched++;
		// Update the position in the Find What text and return true if we've found a match.
		if (m_pwszFindPos-- == m_pwszFindWhat)
		{
			if (m_fWholeWord && !CheckForWholeWord(true, 0))
				return CheckCharUp(false);
			return true;
		}
	}
	else
	{
		// A match was not made, so go back to the previously stored position and start searching again.
		if (m_cMatched > 0 && m_pvOldPos != m_pvOldLimit)
		{
			m_pvPos = m_pvOldPos;
			m_pvLimit = m_pvOldLimit;
			m_pdpStart = m_pdpOld;
			m_iprMin = m_iprOld;
			m_ichFound = m_ichOldFound;

			m_pvStart = m_pdpStart->rgpv[m_iprMin];
		}
		m_cMatched = 0;
		m_pwszFindPos = m_pwszFindWhat + m_cchFind - 1;
	}
	return false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CFindText::Find(FindItem * pfi)
{
	AssertPtr(pfi);
	AssertPtr(pfi->pwszFindWhat);
	AssertPtr(m_pzd);
	if (pfi->ichStart == pfi->ichStop)
		return false;

	bool fUnicode;
	bool fEqual;
	bool fFinal = false;
	bool fMatchCase = (pfi->nFlags & kffMatchCase) != 0;
	m_fWholeWord = (pfi->nFlags & kffWholeWord) != 0;
	int nShift;
	m_cMatched = 0;
	m_pwszFindWhat = pfi->pwszFindWhat;
	m_cchFind = pfi->cchFindWhat;

	if (pfi->ichStart < pfi->ichStop)
	{
		// Look forwards through the search string.
		m_pwszFindPos = m_pwszFindWhat;
		m_pdpStart = m_pzd->GetPart(pfi->ichStart, false, &m_pvStart, &m_iprMin, &m_ichFound);
		m_pdpStop = m_pzd->GetPart(pfi->ichStop, false, &m_pvStop, &m_iprLim);
		fUnicode = !m_pzd->IsAnsi(m_pdpStart, m_iprMin);
		nShift = fUnicode ? 1 : 0;
		m_iprOld = m_iprMin;
		for (UINT ipr = 0; ipr < m_iprMin; ipr++)
			m_ichFound += CharsInPara(m_pdpStart, ipr);
		m_pvLimit = (char *)m_pdpStart->rgpv[m_iprMin] + (PrintCharsInPara(m_pdpStart, m_iprMin) << nShift);
		m_pvPos = m_pvStart;

		// Loop until a match is found or until pfi->ichStop is reached.
		while (true)
		{
			// If we are on the last paragraph, make sure we don't go past the
			// last character to search on.
			if (m_pdpStart == m_pdpStop && m_iprMin == m_iprLim &&
				m_pvStart <= m_pvStop && m_pvStop < m_pvLimit)
			{
				m_pvLimit = m_pvStop;
				fFinal = true;
			}

			// Loop within a paragraph.
			while (m_pvPos < m_pvLimit)
			{
				// See if the current character in the para matches with the current
				// find position.
				if (*m_pwszFindPos == '^')
				{
					// The only thing after a "^" should be another "^" (meaning we need
					// to look for a ^), or a P (meaning we need to look for a paragraph
					// mark). Since we're still in the middle of the paragraph of text,
					// anything other than another ^ means we didn't find a match.
					// NOTE: All other special characters should have already been replaced.
					if (*++m_pwszFindPos != '^')
					{
						// Force a reset of the search information.
						int cMatched = m_cMatched;
						CheckCharDown(false);
						if (cMatched == 0)
							m_pvPos = (char *)m_pvPos + 1 + nShift;
						continue;
					}
				}
				if (fUnicode)
				{
					if (fMatchCase)
						fEqual = (*(wchar *)m_pvPos == *m_pwszFindPos);
					else
						fEqual = (towupper(*(wchar *)m_pvPos) == *m_pwszFindPos);
					m_pvPos = (wchar *)m_pvPos + 1;
				}
				else
				{
					if (fMatchCase)
						fEqual = (*(char *)m_pvPos == (char)*m_pwszFindPos);
					else
						fEqual = (toupper(*(char *)m_pvPos) == (char)*m_pwszFindPos);
					m_pvPos = (char *)m_pvPos + 1;
				}
				if (CheckCharDown(fEqual))
				{
					// We found a match so return the position
					pfi->ichMatch = m_ichFound + ((((char *)m_pvPos - (char *)m_pdpStart->rgpv[m_iprMin]) >> nShift) - m_cMatched);
					pfi->cchMatch = m_cMatched;
					return true;
				}
			}

			// The end of the paragraph was reached.
			int cchAtEnd = CharsAtEnd(m_pdpStart, m_iprMin);
			if (!fFinal && cchAtEnd > 0)
			{
				// Look for ^p in the Find What text.
				bool fFound = false;
				int cchExtra = 1;
				if (*m_pwszFindPos == '^')
				{
					fEqual = towlower(*++m_pwszFindPos) == 'p';
					fFound = CheckCharDown(fEqual);
					if (fEqual)
					{
						cchExtra = cchAtEnd;
						m_cMatched += cchExtra - 1;
					}
				}
				else
				{
					// Check the first end-of-paragraph character to see if it matches.
					if (fUnicode)
						fEqual = *(wchar *)m_pvPos == *m_pwszFindPos;
					else
						fEqual = *(char *)m_pvPos == (char)*m_pwszFindPos;
					fFound = CheckCharDown(fEqual);
					if (!fFound && fEqual && cchAtEnd == 2)
					{
						// Check the last end-of-paragraph to see if it matches. We only want to do this check
						// if the first end-of-paragraph matches but we still haven't matched the entire search string.
						if (fUnicode)
							fEqual = *((wchar *)m_pvPos + 1) == *m_pwszFindPos;
						else
							fEqual = *((char *)m_pvPos + 1) == (char)*m_pwszFindPos;
						fFound = CheckCharDown(fEqual);
						cchExtra++;
					}
				}
				if (fFound)
				{
					// We found a match, so return the position.
					pfi->ichMatch = m_ichFound + ((((char *)m_pvPos - (char *)m_pdpStart->rgpv[m_iprMin]) >> nShift) - m_cMatched + cchExtra);
					pfi->cchMatch = m_cMatched;
					return true;
				}
			}
			m_ichFound += CharsInPara(m_pdpStart, m_iprMin);
			if (m_pdpStart != m_pdpStop)
			{
				// Go on to the next paragraph in this part.
				if (++m_iprMin >= m_pdpStart->cpr)
				{
					// Start looking through the next part.
					m_pdpStart = m_pdpStart->pdpNext;
					AssertPtr(m_pdpStart);
					m_iprMin = 0;
				}
			}
			else
			{
				// If we go past the last paragraph to look in, we didn't find a match.
				if (++m_iprMin > m_iprLim)
					return false;
			}
			fUnicode = !m_pzd->IsAnsi(m_pdpStart, m_iprMin);
			nShift = fUnicode ? 1 : 0;
			m_pvStart = m_pvPos = m_pdpStart->rgpv[m_iprMin];
			m_pvLimit = (char *)m_pvStart + (PrintCharsInPara(m_pdpStart, m_iprMin) << nShift);
		}
	}
	else
	{
		// Look backwards through the search string.
		m_pwszFindPos = m_pwszFindWhat + m_cchFind - 1;
		// This caused problems when searching for a ^p and starting at the beginning of a paragraph.
		//m_pdpStart = m_pzd->GetPart(pfi->ichStart - 1, false, &m_pvStart, &m_iprMin, &m_ichFound);
		m_pdpStart = m_pzd->GetPart(pfi->ichStart, false, &m_pvStart, &m_iprMin, &m_ichFound);
		m_pdpStop = m_pzd->GetPart(pfi->ichStop, false, &m_pvStop, &m_iprLim);
		fUnicode = !m_pzd->IsAnsi(m_pdpStart, m_iprMin);
		nShift = fUnicode ? 1 : 0;
		m_iprOld = m_iprMin;
		for (UINT ipr = 0; ipr < m_iprMin; ipr++)
			m_ichFound += CharsInPara(m_pdpStart, ipr);
		m_pvLimit = m_pdpStart->rgpv[m_iprMin];

		// We want to actually start looking at the previous character.
		m_pvStart = (char *)m_pvStart - 1 - nShift;
		m_pvPos = m_pvStart;

		void * pvParaLimit = m_pvLimit;

		// Loop until a match is found or until pfi->ichStop is reached.
		while (true)
		{
			// If we are on the last paragraph, make sure we don't go past the
			// last character to search on.
			if (m_pdpStart == m_pdpStop && m_iprMin == m_iprLim)
			{
				if (m_pdpStart == m_pdpStop && m_iprMin == m_iprLim && 
					m_pvStop >= m_pvLimit)
					//m_pvStart >= m_pvStop && m_pvStop >= m_pvLimit)
				{
					m_pvLimit = m_pvStop;
					fFinal = true;
				}
			}

			// Loop within a paragraph.
			while (m_pvPos >= m_pvLimit || (fFinal && m_cMatched > 0 && m_pvPos >= pvParaLimit))
			{
				// See if the current character in the para matches with the current
				// find position.
				if (m_pwszFindPos > m_pwszFindWhat && m_pwszFindPos[-1] == '^')
				{
					// The only thing after a "^" should be another "^" (meaning we need
					// to look for a ^), or a P (meaning we need to look for a paragraph
					// mark). Since we're still in the middle of the paragraph of text,
					// anything other than another ^ means we didn't find a match.
					// NOTE: All other special characters should have already been replaced.
					if (*m_pwszFindPos-- != '^')
					{
						// Force a reset of the search information.
						int cMatched = m_cMatched;
						CheckCharUp(false);
						if (cMatched == 0)
							m_pvPos = (char *)m_pvPos - 1 - nShift;
						continue;
					}
				}
				if (fUnicode)
				{
					if (fMatchCase)
						fEqual = (*(wchar *)m_pvPos == *m_pwszFindPos);
					else
						fEqual = (towupper(*(wchar *)m_pvPos) == *m_pwszFindPos);
					m_pvPos = (wchar *)m_pvPos - 1;
				}
				else
				{
					if (fMatchCase)
						fEqual = (*(char *)m_pvPos == (char)*m_pwszFindPos);
					else
						fEqual = (toupper(*(char *)m_pvPos) == (char)*m_pwszFindPos);
					m_pvPos = (char *)m_pvPos - 1;
				}
				if (CheckCharUp(fEqual))
				{
					// We found a match so return the position
					pfi->ichMatch = m_ichFound + ((((char *)m_pvPos - (char *)m_pdpStart->rgpv[m_iprMin]) >> nShift)) + 1;
					pfi->cchMatch = m_cMatched;
					return true;
				}
			}

			// The beginning of the paragraph was reached.
			if (m_pdpStart != m_pdpStop)
			{
				// Go on to the previous paragraph in this part.
				if (m_iprMin-- == 0)
				{
					// Start looking through the previous part.
					m_pdpStart = m_pdpStart->pdpPrev;
					AssertPtr(m_pdpStart);
					m_iprMin = m_pdpStart->cpr - 1;
				}
			}
			else
			{
				// If we go past the last paragraph to look in, we didn't find a match.
				if (m_iprMin-- <= m_iprLim)
					return false;
			}
			fUnicode = !m_pzd->IsAnsi(m_pdpStart, m_iprMin);
			nShift = fUnicode ? 1 : 0;
			m_pvStart = m_pvPos = (char *)m_pdpStart->rgpv[m_iprMin] + ((PrintCharsInPara(m_pdpStart, m_iprMin) - 1) << nShift);
			m_pvLimit = m_pdpStart->rgpv[m_iprMin];
			m_ichFound -= CharsInPara(m_pdpStart, m_iprMin);

			pvParaLimit = m_pvLimit;

			int cchAtEnd = CharsAtEnd(m_pdpStart, m_iprMin);
			if (!fFinal && cchAtEnd > 0)
			{
				// Look for ^p in the Find What text.
				void * pvPos = (char *)m_pvPos + (cchAtEnd << nShift);
				bool fFound = false;
				int cchExtra = 1;
				if (m_pwszFindPos > m_pwszFindWhat && m_pwszFindPos[-1] == '^')
				{
					fEqual = towlower(*m_pwszFindPos--) == 'p';
					fFound = CheckCharUp(fEqual);
					if (fEqual)
					{
						cchExtra = cchAtEnd;
						m_cMatched += cchExtra - 1;
					}
				}
				else
				{
					// Check the last end-of-paragraph character to see if it matches.
					if (fUnicode)
						fEqual = *(wchar *)pvPos == *m_pwszFindPos;
					else
						fEqual = *(char *)pvPos == (char)*m_pwszFindPos;
					fFound = CheckCharUp(fEqual);
					if (!fFound && cchAtEnd == 2)
					{
						// Check the first end-of-paragraph character to see if it matches.
						if (fUnicode)
							fEqual = *((wchar *)pvPos - 1) == *m_pwszFindPos;
						else
							fEqual = *((char *)pvPos - 1) == (char)*m_pwszFindPos;
						fFound = CheckCharUp(fEqual);
						cchExtra++;
					}
				}
				if (fFound)
				{
					// We found a match so return the position.
					//pfi->ichMatch = m_ichFound + ((((char *)m_pvPos - (char *)m_pdpStart->rgpv[m_iprMin]) >> nShift) - m_cMatched + cchExtra);
					pfi->ichMatch = m_ichFound + ((((char *)pvPos - (char *)m_pdpStart->rgpv[m_iprMin]) >> nShift)) + 1 - cchExtra;
					pfi->cchMatch = m_cMatched;
					return true;
				}
			}
		}
	}
	return false;
}