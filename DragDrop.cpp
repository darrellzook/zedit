#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "DragDrop.h"
#include "DragDropTlb_i.c"


/***********************************************************************************************
	CZDataObject methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CZDataObject::CZDataObject()
{
	m_cref = 1;
	m_fAnsi = true;
	m_pv = m_pvLastPara = NULL;
	m_cb = m_cpr = 0;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CZDataObject::~CZDataObject()
{
	if (m_pv)
		delete m_pv;
}


/*----------------------------------------------------------------------------------------------
	Static methods to create a new IDataObject.
----------------------------------------------------------------------------------------------*/
HRESULT CZDataObject::Create(IZDataObject ** ppzdo)
{
	AssertPtrN(ppzdo);
	if (!ppzdo)
		return E_POINTER;

	*ppzdo = NULL;
	CZDataObject * pzdo = new CZDataObject();
	if (!pzdo)
		return E_OUTOFMEMORY;
	HRESULT hr = pzdo->QueryInterface(IID_IDataObject, (void **)ppzdo);
	pzdo->Release();
	return hr;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
HRESULT CZDataObject::Create(void * pv, int cch, bool fAnsi, int cpr, void * pvLastPara,
	IZDataObject ** ppzdo)
{
#ifdef _DEBUG
	if (fAnsi)
		AssertArray((char *)pv, cch);
	else
		AssertArray((wchar *)pv, cch);
#endif
	AssertPtrN(ppzdo);
	if (!ppzdo)
		return E_POINTER;
	*ppzdo = NULL;

	HRESULT hr;
	CZDataObject * pzdo;
	if (FAILED(hr = CZDataObject::Create((IZDataObject **)&pzdo)))
		return hr;

	pzdo->m_fAnsi = fAnsi;
	if (fAnsi)
		pzdo->m_cb = cch;
	else
		pzdo->m_cb = cch << 1;
	if (cch)
	{
		pzdo->m_pv = new char[pzdo->m_cb];
		if (!pzdo->m_pv)
		{
			pzdo->Release();
			return E_OUTOFMEMORY;
		}
		memmove(pzdo->m_pv, pv, pzdo->m_cb);
	}
	pzdo->m_cpr = cpr;
	pzdo->m_pvLastPara = pvLastPara;
	*ppzdo = pzdo;
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
HRESULT CZDataObject::CreateFromDataObject(CZEditFrame * pzef, IDataObject * pdo, bool fAnsi,
	CZDataObject ** ppzdo)
{
	AssertPtrN(pdo);
	AssertPtrN(ppzdo);
	if (!ppzdo)
		return E_POINTER;
	*ppzdo = NULL;

	bool fFromClipboard = false;
	if (!pdo)
	{
		HRESULT hr = ::OleGetClipboard(&pdo);
		if (FAILED(hr))
			return hr;
		fFromClipboard = true;
	}

	// See if the IDataObject is an IZDataObject.
	HRESULT hr;
	if (FAILED(hr = pdo->QueryInterface(IID_IZDataObject, (void **)ppzdo)))
	{
		// It wasn't, so see if we can create one from what is in the IDataObject.
		// See if this is an attachment from Microsoft Outlook.
		if (g_fg.OpenOutlookAttachment(pzef, pdo))
			return S_FALSE;

		// Now try the corresponding text for this file type.
		FORMATETC fe = { fAnsi ? CF_TEXT : CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1,
			TYMED_HGLOBAL };
		STGMEDIUM sm = { 0 };
		if (FAILED(hr = pdo->GetData(&fe, &sm)))
		{
			// Now try the other kind of text.
			fe.cfFormat = fAnsi ? CF_UNICODETEXT : CF_TEXT;
			hr = pdo->GetData(&fe, &sm);
		}
		if (SUCCEEDED(hr) && sm.tymed == TYMED_HGLOBAL && sm.hGlobal)
		{
			CZDataObject * pzdo = new CZDataObject();
			if (pzdo)
			{
				pzdo->m_fAnsi = fAnsi;
				void * pv = ::GlobalLock(sm.hGlobal);
				// TODO: Make sure the unicode part is correct.
				//pzdo->m_cb = ::GlobalSize(sm.hGlobal);
				pzdo->m_cb = fAnsi ? lstrlen((char *)pv) : (lstrlenW((wchar *)pv) * 2);
				if (pv && SUCCEEDED(hr = pzdo->CopyMem(pv, pzdo->m_cb, &pzdo->m_pv, NULL,
					fAnsi, fAnsi)))
				{
					// TODO: If this turns out to be too slow, it can be optimized quite a bit.
					// Right now it loops through the text on the clipboard twice: one in
					// pzdo->CopyMem and once below to find the paragraph count. These could
					// be combined into one loop if needed.
					if (SUCCEEDED(hr = pzdo->QueryInterface(IID_IZDataObject, (void **)ppzdo)))
					{
						// Count the number of paragraphs in the text.
						void * pvLastPara = pv;
						int cpr = 0;
						if (fAnsi)
						{
							char * prgch = (char *)pv;
							char * prgchLim = prgch + pzdo->m_cb;
							while (prgch < prgchLim)
							{
								if (*prgch++ == 10)
								{
									cpr++;
									pvLastPara = prgch;
								}
							}
						}
						else
						{
							wchar * prgch = (wchar *)pv;
							wchar * prgchLim = (wchar *)((char *)prgch + pzdo->m_cb);
							while (prgch < prgchLim)
							{
								if (*prgch++ == 10)
								{
									cpr++;
									pvLastPara = prgch;
								}
							}
						}
						pzdo->m_cpr = cpr;
						pzdo->m_pvLastPara = min(pvLastPara, (char *)pv + pzdo->m_cb);
					}
				}
				::GlobalUnlock(sm.hGlobal);
				pzdo->Release();
			}
			::GlobalFree(sm.hGlobal);
		}
	}
	if (fFromClipboard)
		pdo->Release();
	return hr;
}


/*----------------------------------------------------------------------------------------------
	AddRef.
----------------------------------------------------------------------------------------------*/
ULONG CZDataObject::AddRef(void)
{
	return InterlockedIncrement(&m_cref);
}


/*----------------------------------------------------------------------------------------------
	Release.
----------------------------------------------------------------------------------------------*/
ULONG CZDataObject::Release(void)
{
	long lw = InterlockedDecrement(&m_cref);
	if (lw == 0)
	{
		m_cref = 1;
		delete this;
	}
	return lw;
}


/*----------------------------------------------------------------------------------------------
	QueryInterface.
	Implements: IUnknown, IDataObject
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::QueryInterface(REFIID riid, void ** ppv)
{
	AssertPtrN(ppv);
	if (!ppv)
		return E_POINTER;

	*ppv = NULL;
	if (riid == IID_IUnknown)
		*ppv = static_cast<IUnknown *>(this);
	else if (riid == IID_IDataObject)
		*ppv = static_cast<IDataObject *>(this);
	else if (riid == IID_IZDataObject)
		*ppv = static_cast<IZDataObject *>(this);
	else
		return E_NOINTERFACE;
	reinterpret_cast<IUnknown *>(*ppv)->AddRef();
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	Protected member function that copies a block of global memory into a newly
		allocated block of the same size.

	Either ppvDst or phDst should be NULL.
----------------------------------------------------------------------------------------------*/
HRESULT CZDataObject::CopyMem(void * pvSrc, int cb, void ** ppvDst, HANDLE * phDst, bool fFromAnsi,
	bool fToAnsi)
{
	// Returns a copy of the global memory
	Assert(pvSrc);
	Assert((ppvDst && !phDst) || (!ppvDst && phDst));
	if (phDst)
		*phDst = NULL;
	if (ppvDst)
		*ppvDst = NULL;
	if (0 == cb)
		return S_FALSE;

	UINT cch = cb;
	if (!fFromAnsi)
	{
		Assert(cch % 2 == 0);
		cch >>= 1;
	}
	else if (fFromAnsi && !fToAnsi)
		cb <<= 1;

	HANDLE hDst = NULL;
	void * pvDst = NULL;
	if (phDst)
	{
		hDst = GlobalAlloc(GMEM_MOVEABLE, cb + 2);
		if (!hDst)
			return E_OUTOFMEMORY;
		pvDst = GlobalLock(hDst);
	}
	else
	{
		pvDst = new char[cb];
	}
	if (!pvDst)
	{
		if (hDst)
			GlobalFree(hDst);
		return E_OUTOFMEMORY;
	}

	if (fFromAnsi == fToAnsi) // Ansi to Ansi or Unicode to Unicode
		memmove(pvDst, pvSrc, cb);
	else if (fFromAnsi && !fToAnsi) // Ansi to Unicode
		Convert8to16((char *)pvSrc, (wchar *)pvDst, cch);
	else // Unicode to Ansi
		Convert16to8((wchar *)pvSrc, (char *)pvDst, cch);

	if (phDst)
	{
		if (fToAnsi)
			((char *)pvDst)[cch] = 0;
		else
			((wchar *)pvDst)[cch] = 0;
		*phDst = hDst;
		if (hDst)
			GlobalUnlock(hDst);
	}
	else
	{
		*ppvDst = pvDst;
	}
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	This only returns a text string (CF_TEXT and CF_UNICODETEXT) from the CZDataObject.

	Also: See Microsoft documentation for IDataObject::GetData
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::GetData(FORMATETC * pFormatetc, STGMEDIUM * pmedium)
{
	if (!pFormatetc || !pmedium)
		return E_INVALIDARG;
	if (!m_pv)
		return E_UNEXPECTED;
	HRESULT hr;
	if (FAILED(hr = QueryGetData(pFormatetc)))
		return hr;
	pmedium->tymed = TYMED_HGLOBAL;
	pmedium->pUnkForRelease = NULL;
	return CopyMem(m_pv, m_cb, NULL, &pmedium->hGlobal, m_fAnsi, pFormatetc->cfFormat == CF_TEXT);
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDataObject::GetDataHere.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::GetDataHere(FORMATETC * pFormatetc, STGMEDIUM * pmedium)
{
	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDataObject::QueryGetData.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::QueryGetData(FORMATETC * pFormatetc)
{
	if (!pFormatetc)
		return E_INVALIDARG;
	if (!m_pv)
		return E_UNEXPECTED;
	if (pFormatetc->cfFormat != CF_TEXT &&
		pFormatetc->cfFormat != CF_UNICODETEXT)
	{
		return DV_E_FORMATETC;
	}
	if (pFormatetc->dwAspect != DVASPECT_CONTENT)
		return DV_E_DVASPECT;
	if (pFormatetc->lindex != -1)
		return DV_E_LINDEX;
	if (pFormatetc->tymed != TYMED_HGLOBAL)
		return DV_E_TYMED;
	if (pFormatetc->ptd != NULL)
		return E_NOTIMPL;
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDataObject::GetCanonicalFormatEtc.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::GetCanonicalFormatEtc(FORMATETC * pFormatetcIn,
	FORMATETC * pFormatetcOut)
{
	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDataObject::SetData.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::SetData(FORMATETC * pFormatetc, STGMEDIUM * pmedium, BOOL fRelease)
{
	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDataObject::EnumFormatEtc.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC ** ppenumFormatetc)
{
	return CZEnumFORMATETC::Create(ppenumFormatetc);
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDataObject::DAdvise.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::DAdvise(FORMATETC * pFormatetc, DWORD advf, IAdviseSink * pAdvSink,
	DWORD * pdwConnection)
{
	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDataObject::DUnadvise.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::DUnadvise(DWORD dwConnection)
{
	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDataObject::EnumDAdvise.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::EnumDAdvise(IEnumSTATDATA ** ppenumAdvise)
{
	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::get_Size(int * pcch)
{
	AssertPtrN(pcch);
	if (!pcch)
		return E_POINTER;
	if (m_fAnsi)
		*pcch = m_cb;
	else
		*pcch = m_cb >> 1;
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::get_ByteCount(int * pcb)
{
	AssertPtrN(pcb);
	if (!pcb)
		return E_POINTER;
	*pcb = m_cb;
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDataObject::get_IsAnsi(BOOL * pfAnsi)
{
	AssertPtrN(pfAnsi);
	if (!pfAnsi)
		return E_POINTER;
	*pfAnsi = m_fAnsi;
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
HRESULT CZDataObject::GetText(bool fAnsi, void ** ppv, UINT * pcch, bool * pfDelete)
{
	Assert(ppv);
	AssertPtr(pcch);
	AssertPtr(pfDelete);

	if (fAnsi == m_fAnsi)
	{
		*ppv = m_pv;
		if (fAnsi)
			*pcch = m_cb;
		else
			*pcch = m_cb >> 1;
		*pfDelete = false;
		return S_OK;
	}

	*pfDelete = true;
	if (fAnsi)
		return CopyMem(m_pv, m_cb, ppv, NULL, false, true);
	return CopyMem(m_pv, m_cb, ppv, NULL, true, false);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CZDataObject::GetCharCount()
{
	if (m_fAnsi)
		return m_cb;
	Assert(m_cb % 2 == 0); // Make sure we have an even number of bytes.
	return m_cb >> 1;
}


/***********************************************************************************************
	CZDropSource methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CZDropSource::CZDropSource()
{
	m_cref = 1;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CZDropSource::~CZDropSource()
{
}


/*----------------------------------------------------------------------------------------------
	Static method to create a new IDropSource.
----------------------------------------------------------------------------------------------*/
HRESULT CZDropSource::Create(IDropSource ** ppds)
{
	if (!ppds)
		return E_POINTER;
	*ppds = NULL;
	CZDropSource * pds = new CZDropSource();
	if (!pds)
		return E_OUTOFMEMORY;
	HRESULT hr = pds->QueryInterface(IID_IDropSource, (void **)ppds);
	pds->Release();
	return hr;
}


/*----------------------------------------------------------------------------------------------
	AddRef.
----------------------------------------------------------------------------------------------*/
ULONG CZDropSource::AddRef(void)
{
	return InterlockedIncrement(&m_cref);
}


/*----------------------------------------------------------------------------------------------
	Release.
----------------------------------------------------------------------------------------------*/
ULONG CZDropSource::Release(void)
{
	long lw = InterlockedDecrement(&m_cref);
	if (lw == 0)
	{
		m_cref = 1;
		delete this;
	}
	return lw;
}


/*----------------------------------------------------------------------------------------------
	QueryInterface.
	Implements: IUnknown, IDropSource
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDropSource::QueryInterface(REFIID riid, void ** ppv)
{
	if (!ppv)
		return E_POINTER;
	if (riid == IID_IUnknown)
		*ppv = static_cast<IUnknown *>(this);
	else if (riid == IID_IDropSource)
		*ppv = static_cast<IDropSource *>(this);
	else
		return E_NOINTERFACE;
	reinterpret_cast<IUnknown *>(*ppv)->AddRef();
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDropSource::GiveFeedback.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDropSource::GiveFeedback(DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDropSource::QueryContinueDrag.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	if (fEscapePressed)
		return DRAGDROP_S_CANCEL;
	if (!(grfKeyState & MK_LBUTTON))
		return DRAGDROP_S_DROP;
	return S_OK;
}


/***********************************************************************************************
	CZEnumFORMATETC methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CZEnumFORMATETC::CZEnumFORMATETC()
{
	m_cref = 1;
	m_ife = 0;
	m_rgfe[0].cfFormat = CF_TEXT;
	m_rgfe[1].cfFormat = CF_UNICODETEXT;
	m_rgfe[0].dwAspect = m_rgfe[1].dwAspect = DVASPECT_CONTENT;
	m_rgfe[0].lindex = m_rgfe[1].lindex = -1;
	m_rgfe[0].ptd = m_rgfe[1].ptd = NULL;
	m_rgfe[0].tymed = m_rgfe[1].tymed = TYMED_HGLOBAL;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CZEnumFORMATETC::~CZEnumFORMATETC()
{
}


/*----------------------------------------------------------------------------------------------
	Static method to create a new IEnumFORMATETC.
----------------------------------------------------------------------------------------------*/
HRESULT CZEnumFORMATETC::Create(IEnumFORMATETC ** ppefe)
{
	if (!ppefe)
		return E_POINTER;
	*ppefe = NULL;
	CZEnumFORMATETC * pefe = new CZEnumFORMATETC();
	if (!pefe)
		return E_OUTOFMEMORY;
	HRESULT hr = pefe->QueryInterface(IID_IEnumFORMATETC, (void **)ppefe);
	pefe->Release();
	return hr;
}


/*----------------------------------------------------------------------------------------------
	AddRef.
----------------------------------------------------------------------------------------------*/
ULONG CZEnumFORMATETC::AddRef(void)
{
	return InterlockedIncrement(&m_cref);
}


/*----------------------------------------------------------------------------------------------
	Release.
----------------------------------------------------------------------------------------------*/
ULONG CZEnumFORMATETC::Release(void)
{
	long lw = InterlockedDecrement(&m_cref);
	if (lw == 0)
	{
		m_cref = 1;
		delete this;
	}
	return lw;
}


/*----------------------------------------------------------------------------------------------
	QueryInterface.
	Implements: IUnknown, IEnumFORMATETC
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZEnumFORMATETC::QueryInterface(REFIID riid, void ** ppv)
{
	if (!ppv)
		return E_POINTER;
	if (riid == IID_IUnknown)
		*ppv = static_cast<IUnknown *>(this);
	else if (riid == IID_IEnumFORMATETC)
		*ppv = static_cast<IEnumFORMATETC *>(this);
	else
		return E_NOINTERFACE;
	reinterpret_cast<IUnknown *>(*ppv)->AddRef();
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IEnumFORMATETC::Next.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZEnumFORMATETC::Next(ULONG cfe, FORMATETC * rgfe, ULONG * pcfeFetched)
{
	ULONG ifeDst = 0;
	ULONG ifeStop = min(m_ife + cfe, 2);
	if (pcfeFetched)
		*pcfeFetched = ifeStop - m_ife;
	for ( ; m_ife < ifeStop; m_ife++, ifeDst++)
		rgfe[ifeDst] = m_rgfe[m_ife];
	if (cfe == ifeDst)
		return S_OK;
	else
		return S_FALSE;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IEnumFORMATETC::Skip.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZEnumFORMATETC::Skip(ULONG cfe)
{
	m_ife = min(max(0, m_ife + cfe), 2);
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IEnumFORMATETC::Reset.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZEnumFORMATETC::Reset(void)
{
	m_ife = 0;
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IEnumFORMATETC::Clone.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZEnumFORMATETC::Clone(IEnumFORMATETC ** ppefe)
{
	if (!ppefe)
		return E_INVALIDARG;
	*ppefe = NULL;
	HRESULT hr;
	if (FAILED(hr = CZEnumFORMATETC::Create(ppefe)))
		return hr;
	return (*ppefe)->Skip(m_ife);
}


/***********************************************************************************************
	CZDropTarget methods.
***********************************************************************************************/

/*----------------------------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------------------------*/
CZDropTarget::CZDropTarget(CZEditFrame * pzef)
{
	m_cref = 1;
	m_fText = false;
	m_fFile = false;
	m_fOutlookAttachment = false;
	m_pzef = pzef;
	m_dwEffect = DROPEFFECT_NONE;
}


/*----------------------------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------------------------*/
CZDropTarget::~CZDropTarget()
{
}


/*----------------------------------------------------------------------------------------------
	Static method to create a new IDropTarget.
----------------------------------------------------------------------------------------------*/
HRESULT CZDropTarget::Create(CZEditFrame * pzef, IDropTarget ** ppdt)
{
	if (!pzef || !ppdt)
		return E_POINTER;
	*ppdt = NULL;
	CZDropTarget * pdt = new CZDropTarget(pzef);
	if (!pdt)
		return E_OUTOFMEMORY;
	HRESULT hr = pdt->QueryInterface(IID_IDropTarget, (void**)ppdt);
	pdt->Release();
	return hr;
}


/*----------------------------------------------------------------------------------------------
	AddRef.
----------------------------------------------------------------------------------------------*/
ULONG CZDropTarget::AddRef(void)
{
	return InterlockedIncrement(&m_cref);
}


/*----------------------------------------------------------------------------------------------
	Release.
----------------------------------------------------------------------------------------------*/
ULONG CZDropTarget::Release(void)
{
	long lw = InterlockedDecrement(&m_cref);
	if (lw == 0)
	{
		m_cref = 1;
		delete this;
	}
	return lw;
}


/*----------------------------------------------------------------------------------------------
	QueryInterface.
	Implements: IUnknown, IDropTarget
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDropTarget::QueryInterface(REFIID riid, void ** ppv)
{
	if (!ppv)
		return E_POINTER;
	if (riid == IID_IUnknown)
		*ppv = static_cast<IUnknown *>(this);
	else if (riid == IID_IDropTarget)
		*ppv = static_cast<IDropTarget *>(this);
	else
		return E_NOINTERFACE;
	reinterpret_cast<IUnknown *>(*ppv)->AddRef();
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDropTarget::DragEnter.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDropTarget::DragEnter(IDataObject * pdo, DWORD grfKeyState, POINTL ptl,
	DWORD * pdwEffect)
{
	/*IEnumFORMATETC * penum = NULL;
	if (SUCCEEDED(pdo->EnumFormatEtc(DATADIR_GET, &penum)))
	{
		FORMATETC rgfe[20];
		DWORD cfe;
		if (SUCCEEDED(penum->Next(20, rgfe, &cfe)))
		{
			for (DWORD ife = 0; ife < cfe; ife++)
			{
				char szFormat[MAX_PATH];
				int cch = ::GetClipboardFormatName(rgfe[ife].cfFormat, szFormat, sizeof(szFormat));
				cch = 0;
			}
		}
		penum->Release();
	}*/

	// Word 2000 returns S_FALSE for this next call even when dragging text into
	// the window, but Windows Explorer returns S_OK, so we have to check specifically
	// for S_OK instead of using SUCCEEDED.
	FORMATETC feFile = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	if (pdo->QueryGetData(&feFile) == S_OK)
	{
		m_fFile = true;
		return DragOver(grfKeyState, ptl, pdwEffect);
	}

	FORMATETC fe = { g_fg.GetClipFileGroup(), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	if (pdo->QueryGetData(&fe) == S_OK)
	{
		m_fOutlookAttachment = true;
		return DragOver(grfKeyState, ptl, pdwEffect);
	}

	// TODO: There were some problems dragging text into ZEdit: doesn't show the caret.
	FORMATETC feText = { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	m_fText = (pdo->QueryGetData(&feText) == S_OK);
	if (!m_fText)
	{
		feText.cfFormat = CF_TEXT;
		m_fText = (pdo->QueryGetData(&feText) == S_OK);
	}
	if (!m_fText)
	{
		feText.cfFormat = CF_OEMTEXT;
		m_fText = (pdo->QueryGetData(&feText) == S_OK);
	}
	if (m_fText)
	{
		HWND hwndEdit = m_pzef->GetCurrentDoc()->GetHwnd();
		RECT rect;
		::GetWindowRect(hwndEdit, &rect);
		POINT pt = { ptl.x, ptl.y };
		if (::PtInRect(&rect, pt))
		{
			::CreateCaret(hwndEdit, (HBITMAP)1, 2, g_fg.m_ri.m_tm.tmHeight);
			::ShowCaret(hwndEdit);
		}
		else
		{
			::HideCaret(hwndEdit);
		}
	}

	return DragOver(grfKeyState, ptl, pdwEffect);
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDropTarget::DragLeave.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDropTarget::DragLeave(void)
{
	if (m_fFile)
	{
		m_fFile = false;
	}
	else if (m_fOutlookAttachment)
	{
		m_fOutlookAttachment = false;
	}
	else if (m_fText)
	{
		m_fText = false;
		::HideCaret(m_pzef->GetCurrentDoc()->GetHwnd());
		::DestroyCaret();
	}
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDropTarget::DragOver.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDropTarget::DragOver(DWORD grfKeyState, POINTL ptl, DWORD * pdwEffect)
{
	m_grfKeyState = grfKeyState;
	if (!pdwEffect)
		return E_INVALIDARG;

	*pdwEffect = DROPEFFECT_NONE;
	if (m_fFile || m_fOutlookAttachment)
	{
		*pdwEffect = DROPEFFECT_COPY;
	}
	else if (m_fText)
	{
		// TODO: Handle multiple visible windows somehow.
		POINT pt = { ptl.x, ptl.y };
		CZDoc * pzd = m_pzef->GetCurrentDoc();
		RECT rect;
		CZView * pzv = pzd->GetCurrentView();
		::GetWindowRect(pzv->GetHwnd(), &rect);
		if (::PtInRect(&rect, pt))
		{
			::ScreenToClient(pzv->GetHwnd(), &pt);
			pt = pzv->PointFromChar(pzv->CharFromPoint(pt, false));
			::SetCaretPos(pt.x, pt.y);
			if (grfKeyState & MK_CONTROL)
				*pdwEffect = DROPEFFECT_COPY;
			else
				*pdwEffect = DROPEFFECT_MOVE;
		}
		else
		{
			*pdwEffect = DROPEFFECT_NONE;
			::SetCaretPos(-100, -100);
		}
	}
	m_dwEffect = *pdwEffect;
	return S_OK;
}


/*----------------------------------------------------------------------------------------------
	See Microsoft documentation for IDropTarget::Drop.
----------------------------------------------------------------------------------------------*/
STDMETHODIMP CZDropTarget::Drop(IDataObject * pdo, DWORD grfKeyState, POINTL ptl,
	DWORD * pdwEffect)
{
	if (!pdo || !pdwEffect)
	{
		DragLeave();
		return E_INVALIDARG;
	}
	if (DROPEFFECT_NONE == m_dwEffect)
		return S_OK;

	// Set this to a default value in case of an error below.
	*pdwEffect = DROPEFFECT_NONE;

	HRESULT hr;
	if (m_fFile)
	{
		FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM medium;
		if (SUCCEEDED(hr = pdo->GetData(&fe, &medium)))
		{
			m_pzef->OnDropFiles((HDROP)medium.hGlobal, m_grfKeyState & MK_RBUTTON);
			::ReleaseStgMedium(&medium);
		}
	}
	else if (m_fOutlookAttachment)
	{
		// This is an Outlook attachment.
		if (g_fg.OpenOutlookAttachment(m_pzef, pdo))
		{
			*pdwEffect = m_dwEffect;
			return S_OK;
		}
	}
	else if (m_fText)
	{
		CZDoc * pzd = m_pzef->GetCurrentDoc();
		POINT pt = { ptl.x, ptl.y };
		CZView * pzv = pzd->GetCurrentView();
		CZDataObject * pzdo;
		bool fAnsi = pzd->GetFileType() == kftAnsi;
		HRESULT hr = CZDataObject::CreateFromDataObject(m_pzef, pdo, fAnsi, &pzdo);
		if (FAILED(hr))
		{
			::MessageBox(pzv->GetHwnd(), "The dropped text could not be accessed", "Error",
				MB_OK | MB_ICONWARNING);
			return hr;
		}

		::ScreenToClient(pzv->GetHwnd(), &pt);
		UINT ich = pzv->CharFromPoint(pt, false);
		UINT iStartSel;
		UINT iStopSel;
		pzv->GetSelection(&iStartSel, &iStopSel);
		if (pzv->IsDropSource())
		{
			if (m_dwEffect & DROPEFFECT_COPY)
			{
				pzd->InsertText(ich, pzdo, iStartSel, kurtMove_2);
			}
			else if (m_dwEffect & DROPEFFECT_MOVE)
			{
				// See if the user dropped outside of the current selection.
				if (ich < iStartSel || ich > iStopSel)
				{
					pzd->DeleteText(iStartSel, iStopSel, false, false);
					if (ich > iStartSel)
						ich -= (iStopSel - iStartSel);
					pzd->InsertText(ich, pzdo, iStartSel, kurtMove_1);
					pzv->ClearDropSource();
				}
				else
				{
					// Otherwise ignore the drop.
					pzv->SetSelection(ich, ich, true, false);
					pzv->ClearDropSource();
				}
			}
		}
		else
		{
			if (m_dwEffect & DROPEFFECT_COPY || m_dwEffect & DROPEFFECT_MOVE)
				pzd->InsertText(ich, pzdo, 0, kurtMove);
		}
		pzdo->Release();
	}
	*pdwEffect = m_dwEffect;
	DragLeave();
	return hr;
}