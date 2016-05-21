#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"


/*----------------------------------------------------------------------------------------------
	Prompt the user to select the font.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::GetFont(bool fBinary)
{
	LOGFONT lf;
	if (fBinary)
		::GetObject(g_fg.m_hFontFixed, sizeof(LOGFONT), &lf);
	else
		::GetObject(g_fg.m_hFont, sizeof(LOGFONT), &lf);
	CHOOSEFONT cf = { sizeof(CHOOSEFONT), m_hwnd, NULL, &lf, 100, CF_EFFECTS |
		CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS,// | CF_SELECTSCRIPT,
		g_fg.m_cr[kclrText]};
	if (fBinary)
		cf.Flags |= CF_FIXEDPITCHONLY;

	if (::ChooseFont(&cf))
	{
		g_fg.m_cr[kclrText] = cf.rgbColors;
		g_pzea->SetFont(::CreateFontIndirect(cf.lpLogFont), fBinary);
		return true;
	}
	return false;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZEditApp::SetFont(HFONT hFont, bool fBinary)
{
	Assert(hFont);

	if (fBinary)
	{
		if (g_fg.m_hFontFixedOld)
			::DeleteObject(g_fg.m_hFontFixedOld);
		g_fg.m_hFontFixedOld = g_fg.m_hFontFixed;
		g_fg.m_hFontFixed = hFont;
	}
	else
	{
		if (g_fg.m_hFontOld)
			::DeleteObject(g_fg.m_hFontOld);
		g_fg.m_hFontOld = g_fg.m_hFont;
		g_fg.m_hFont = hFont;
	}

	// Calculate the text metrics for the new font.
	HDC hdc = ::GetDC(NULL);
	HFONT hFontOld = (HFONT)::SelectObject(hdc, hFont);
	if (fBinary)
		::GetTextMetrics(hdc, &g_fg.m_tmFixed);
	else
		g_fg.m_ri.Initialize(hdc, g_fg.m_ri.m_cchSpacesInTab);
	::SelectObject(hdc, hFontOld);
	::ReleaseDC(NULL, hdc);

	CZEditFrame ** ppzefT = m_prgzef;
	for (int izef = 0; izef < m_czef; izef++, ppzefT++)
		(*ppzefT)->OnFontChanged();
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
void CZEditFrame::OnFontChanged()
{
	CZDoc * pzd = m_pzdFirst;
	while (pzd)
	{
		pzd->OnFontChanged();
		pzd = pzd->Next();
	}
}