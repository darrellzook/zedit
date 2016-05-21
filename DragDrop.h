// Declaration of the drag/drop classes.

#ifndef _DRAGDROP_H_
#define _DRAGDROP_H_

#include "Common.h"
#include "DragDropTlb.h"


/*----------------------------------------------------------------------------------------------
	CZDataObject declaration.
	Hungarian: zdo
----------------------------------------------------------------------------------------------*/
class CZDataObject : public IZDataObject
{
	friend CZDoc;

public:
	static HRESULT Create(IZDataObject ** ppzdo);
	static HRESULT Create(void * pv, int cch, bool fAnsi, int cpr, void * pvLastPara,
		IZDataObject ** ppzdo);
	static HRESULT CreateFromDataObject(CZEditFrame * pzef, IDataObject * pdo, bool fAnsi,
		CZDataObject ** ppzdo);

	// IUnknown methods.
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	// IDataObject methods.
	STDMETHOD(GetData)(FORMATETC * pFormatetc, STGMEDIUM * pmedium);
	STDMETHOD(GetDataHere)(FORMATETC * pFormatetc, STGMEDIUM * pmedium);
	STDMETHOD(QueryGetData)(FORMATETC * pFormatetc);
	STDMETHOD(GetCanonicalFormatEtc)(FORMATETC * pFormatetcIn, FORMATETC * pFormatetcOut);
	STDMETHOD(SetData)(FORMATETC * pFormatetc, STGMEDIUM * pmedium, BOOL fRelease);
	STDMETHOD(EnumFormatEtc)(DWORD dwDirection, IEnumFORMATETC ** ppenumFormatetc);
	STDMETHOD(DAdvise)(FORMATETC * pFormatetc, DWORD advf, IAdviseSink * pAdvSink,
		DWORD * pdwConnection);
	STDMETHOD(DUnadvise)(DWORD dwConnection);
	STDMETHOD(EnumDAdvise)(IEnumSTATDATA ** ppenumAdvise);

	// IZDataObject methods.
	STDMETHOD(get_Size)(int * pcch);
	STDMETHOD(get_ByteCount)(int * pcb);
	STDMETHOD(get_IsAnsi)(BOOL * pfAnsi);

	// CZDataObject methods.
	HRESULT GetText(bool fAnsi, void ** ppv, UINT * pcch, bool * pfDelete);
	int GetCharCount();

protected:
	long m_cref;
	bool m_fAnsi;
	void * m_pv;
	void * m_pvLastPara;
	int m_cb;
	int m_cpr;

	CZDataObject();
	~CZDataObject();

	static HRESULT CopyMem(void * pvSrc, int cb, void ** ppvDst, HANDLE * phDst, bool fFromAnsi,
		bool fToAnsi);
};


/*----------------------------------------------------------------------------------------------
	CZDropSource declaration.
	Hungarian: zds
----------------------------------------------------------------------------------------------*/
class CZDropSource : public IDropSource
{
public:
	static HRESULT Create(IDropSource ** ppds);

	// IUnknown methods.
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	// IDropSource methods.
	STDMETHOD(GiveFeedback)(DWORD dwEffect);
	STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState);

protected:
	long m_cref;

	CZDropSource();
	~CZDropSource();
};


/*----------------------------------------------------------------------------------------------
	CZDropTarget declaration.
	Hungarian: zdt
----------------------------------------------------------------------------------------------*/
class CZDropTarget : public IDropTarget
{
public:
	static HRESULT Create(CZEditFrame * pzef, IDropTarget ** ppdt);

	// IUnknown methods.
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	// IDropTarget methods.
	STDMETHOD(DragEnter)(IDataObject * pDataObject, DWORD grfKeyState, POINTL ptl,
		DWORD * pdwEffect);
	STDMETHOD(DragLeave)(void);
	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL ptl, DWORD * pdwEffect);
	STDMETHOD(Drop)(IDataObject * pDataObject, DWORD grfKeyState, POINTL ptl, DWORD * pdwEffect);

protected:
	long m_cref;
	CZEditFrame * m_pzef;
	DWORD m_dwEffect;
	DWORD m_grfKeyState;
	bool m_fFile;
	bool m_fOutlookAttachment;
	bool m_fText;

	CZDropTarget(CZEditFrame * pzef);
	~CZDropTarget();
};


/*----------------------------------------------------------------------------------------------
	CZEnumFORMATETC declaration.
----------------------------------------------------------------------------------------------*/
class CZEnumFORMATETC : public IEnumFORMATETC
{
public:
	static HRESULT Create(IEnumFORMATETC ** ppefe);

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	// IEnumFORMATETC methods
	STDMETHOD(Next)(ULONG cfe, FORMATETC * rgfe, ULONG * pcfeFetched);
	STDMETHOD(Skip)(ULONG cfe);
	STDMETHOD(Reset)(void);
	STDMETHOD(Clone)(IEnumFORMATETC ** ppefe);

protected:
	long m_cref;
	ULONG m_ife;
	FORMATETC m_rgfe[3];

	CZEnumFORMATETC();
	~CZEnumFORMATETC();
};


#endif // !_DRAGDROP_H_