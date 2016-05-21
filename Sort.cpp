#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"


/***********************************************************************************************
	CSort methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CSort::swap(SortInfo rgsi[], int i, int j)
{
	SortInfo si = rgsi[i];
	rgsi[i] = rgsi[j];
	rgsi[j] = si;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CSort::vecswap(SortInfo rgsi[], int i, int j, int n)
{
	while (n-- > 0)
		swap(rgsi, i++, j++);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CSort::Compare8I(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch)
{
	wchar ch1;
	for ( ; lt <= gt; lt++)
	{
		if (PrintCharsInPara(rgsi[lt].cch) == 0)
			ch1 = 0;
		else
			ch1 = tolower(((char *)rgsi[lt].pv)[depth]);
		if (ch1 > ch)
			break;
		if (ch1 == ch)
			swap(rgsi, le++, lt);
	}
	for ( ; lt <= gt; gt--)
	{
		if (PrintCharsInPara(rgsi[gt].cch) == 0)
			ch1 = 0;
		else
			ch1 = tolower(((char *)rgsi[gt].pv)[depth]);
		if (ch1 < ch)
			break;
		if (ch1 == ch)
			swap(rgsi, gt, ge--);
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CSort::Compare8(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch)
{
	wchar ch1;
	for ( ; lt <= gt; lt++)
	{
		if (PrintCharsInPara(rgsi[lt].cch) == 0)
			ch1 = 0;
		else
			ch1 = ((char *)rgsi[lt].pv)[depth];
		if (ch1 > ch)
			break;
		if (ch1 == ch)
			swap(rgsi, le++, lt);
	}
	for ( ; lt <= gt; gt--)
	{
		if (PrintCharsInPara(rgsi[gt].cch) == 0)
			ch1 = 0;
		else
			ch1 = ((char *)rgsi[gt].pv)[depth];
		if (ch1 < ch)
			break;
		if (ch1 == ch)
			swap(rgsi, gt, ge--);
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CSort::Compare16I(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch)
{
	wchar ch1;
	for ( ; lt <= gt; lt++)
	{
		if (PrintCharsInPara(rgsi[lt].cch) == 0)
			ch1 = 0;
		else
			ch1 = towlower(((wchar *)rgsi[lt].pv)[depth]);
		if (ch1 > ch)
			break;
		if (ch1 == ch)
			swap(rgsi, le++, lt);
	}
	for ( ; lt <= gt; gt--)
	{
		if (PrintCharsInPara(rgsi[gt].cch) == 0)
			ch1 = 0;
		else
			ch1 = towlower(((wchar *)rgsi[gt].pv)[depth]);
		if (ch1 < ch)
			break;
		if (ch1 == ch)
			swap(rgsi, gt, ge--);
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CSort::Compare16(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch)
{
	wchar ch1;
	for ( ; lt <= gt; lt++)
	{
		if (PrintCharsInPara(rgsi[lt].cch) == 0)
			ch1 = 0;
		else
			ch1 = ((wchar *)rgsi[lt].pv)[depth];
		if (ch1 > ch)
			break;
		if (ch1 == ch)
			swap(rgsi, le++, lt);
	}
	for ( ; lt <= gt; gt--)
	{
		if (PrintCharsInPara(rgsi[gt].cch) == 0)
			ch1 = 0;
		else
			ch1 = ((wchar *)rgsi[gt].pv)[depth];
		if (ch1 < ch)
			break;
		if (ch1 == ch)
			swap(rgsi, gt, ge--);
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CSort::Compare8_16I(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch)
{
	wchar ch1;
	for ( ; lt <= gt; lt++)
	{
		if (PrintCharsInPara(rgsi[lt].cch) == 0)
		{
			ch1 = 0;
		}
		else
		{
			if (IsParaInMem(rgsi[lt].cch))
				ch1 = towlower(((wchar *)rgsi[lt].pv)[depth]);
			else
				ch1 = tolower(((char *)rgsi[lt].pv)[depth]);
		}
		if (ch1 == ch)
			swap(rgsi, le++, lt);
		if (ch1 > ch)
			break;
	}
	for ( ; lt <= gt; gt--)
	{
		if (PrintCharsInPara(rgsi[gt].cch) == 0)
		{
			ch1 = 0;
		}
		else
		{
			if (IsParaInMem(rgsi[gt].cch))
				ch1 = towlower(((wchar *)rgsi[gt].pv)[depth]);
			else
				ch1 = tolower(((char *)rgsi[gt].pv)[depth]);
		}
		if (ch1 == ch)
			swap(rgsi, gt, ge--);
		if (ch1 < ch)
			break;
	}
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CSort::Compare8_16(int & le, int & lt, int & ge, int & gt, SortInfo rgsi[], int depth, wchar ch)
{
	wchar ch1;
	for ( ; lt <= gt; lt++)
	{
		if (PrintCharsInPara(rgsi[lt].cch) == 0)
		{
			ch1 = 0;
		}
		else
		{
			if (IsParaInMem(rgsi[lt].cch))
				ch1 = ((wchar *)rgsi[lt].pv)[depth];
			else
				ch1 = ((char *)rgsi[lt].pv)[depth];
		}
		if (ch1 == ch)
			swap(rgsi, le++, lt);
		if (ch1 > ch)
			break;
	}
	for ( ; lt <= gt; gt--)
	{
		if (PrintCharsInPara(rgsi[gt].cch) == 0)
		{
			ch1 = 0;
		}
		else
		{
			if (IsParaInMem(rgsi[gt].cch))
				ch1 = ((wchar *)rgsi[gt].pv)[depth];
			else
				ch1 = ((char *)rgsi[gt].pv)[depth];
		}
		if (ch1 == ch)
			swap(rgsi, gt, ge--);
		if (ch1 < ch)
			break;
	}
}

/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CSort::Sort(SortInfo rgsi[], int n, int depth, CompareType ct)
{
	int le, lt, gt, ge, r;
	wchar ch;
#if 1
	if (n <= 1)
		return;
#else
	if (n <= 1000)
	{
		int i, j;
		for (i = 1; i < n; ++i)
		{
			for (j = i; j > 0; --j)
			{
				char * psz1 = (depth == PrintCharsInPara(rgsi[j - 1].cch)) ? NULL : (char *)(rgsi[j - 1].pv) + depth;
				char * psz2 = (depth == PrintCharsInPara(rgsi[j].cch)) ? NULL : (char *)(rgsi[j].pv) + depth;
				if (!psz1)
					break;
				else if (psz2 && strcmp(psz1, psz2) <= 0)
					break;
				swap(rgsi, j, j - 1);
			}
		}
		return;
	}
#endif
	swap(rgsi, 0, rand() % n);
	if (depth == PrintCharsInPara(rgsi[0].cch))
	{
		ch = 0;
	}
	else
	{
		CompareType ct1 = (CompareType)(ct % 3);
		if (ct1 == kctCompare8)
		{
			ch = ((char *)rgsi[0].pv)[depth];
			if (ct == kctCompare8I)
				ch = tolower((char)ch);
		}
		else if (ct1 == kctCompare16)
		{
			ch = ((wchar *)rgsi[0].pv)[depth];
			if (ct == kctCompare16I)
				ch = towlower(ch);
		}
		else
		{
			Assert(ct1 == kctCompare8_16);

			if (IsParaInMem(rgsi[0].cch))
			{
				ch = ((wchar *)rgsi[0].pv)[depth];
				if (ct == kctCompare8_16I)
					ch = towlower(ch);
			}
			else
			{
				ch = ((char *)rgsi[0].pv)[depth];
				if (ct == kctCompare8_16I)
					ch = tolower((char)ch);
			}
		}
	}
	le = lt = 1;
	gt = ge = n - 1;
	PFN_COMPARE pfnCompare = krgpfnCompare[ct - kctCompare8];
	for ( ; ; )
	{
		pfnCompare(le, lt, ge, gt, rgsi, depth, ch);
		if (lt > gt)
			break;
		swap(rgsi, lt++, gt--);
	}
	r = min(le, lt - le);
	vecswap(rgsi, 0, lt - r, r);
	r = min(ge - gt, n - ge - 1);
	vecswap(rgsi, lt, n - r, r);
	Sort(rgsi, lt - le, depth, ct);
	if (ch != 0)
		Sort(rgsi + lt - le, le + n - ge - 1, depth + 1, ct);
	Sort(rgsi + n - (ge - gt), ge - gt, depth, ct);
}