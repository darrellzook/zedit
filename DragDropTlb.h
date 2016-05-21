/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Sat Nov 06 13:30:59 1999
 */
/* Compiler settings for C:\My Documents\Visual Studio Projects\ZEdit\DragDropTlb.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __DragDropTlb_h__
#define __DragDropTlb_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IZDataObject_FWD_DEFINED__
#define __IZDataObject_FWD_DEFINED__
typedef interface IZDataObject IZDataObject;
#endif 	/* __IZDataObject_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IZDataObject_INTERFACE_DEFINED__
#define __IZDataObject_INTERFACE_DEFINED__

/* interface IZDataObject */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IZDataObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AE8BFEC0-93CF-11d3-AFE4-40904EC1DD0F")
    IZDataObject : public IDataObject
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ int __RPC_FAR *pcch) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ByteCount( 
            /* [retval][out] */ int __RPC_FAR *pcb) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsAnsi( 
            /* [retval][out] */ BOOL __RPC_FAR *pfAnsi) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IZDataObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IZDataObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IZDataObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IZDataObject __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetData )( 
            IZDataObject __RPC_FAR * This,
            /* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
            /* [out] */ STGMEDIUM __RPC_FAR *pmedium);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDataHere )( 
            IZDataObject __RPC_FAR * This,
            /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [out][in] */ STGMEDIUM __RPC_FAR *pmedium);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryGetData )( 
            IZDataObject __RPC_FAR * This,
            /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCanonicalFormatEtc )( 
            IZDataObject __RPC_FAR * This,
            /* [unique][in] */ FORMATETC __RPC_FAR *pformatectIn,
            /* [out] */ FORMATETC __RPC_FAR *pformatetcOut);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetData )( 
            IZDataObject __RPC_FAR * This,
            /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
            /* [in] */ BOOL fRelease);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumFormatEtc )( 
            IZDataObject __RPC_FAR * This,
            /* [in] */ DWORD dwDirection,
            /* [out] */ IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DAdvise )( 
            IZDataObject __RPC_FAR * This,
            /* [in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [in] */ DWORD advf,
            /* [unique][in] */ IAdviseSink __RPC_FAR *pAdvSink,
            /* [out] */ DWORD __RPC_FAR *pdwConnection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DUnadvise )( 
            IZDataObject __RPC_FAR * This,
            /* [in] */ DWORD dwConnection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumDAdvise )( 
            IZDataObject __RPC_FAR * This,
            /* [out] */ IEnumSTATDATA __RPC_FAR *__RPC_FAR *ppenumAdvise);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Size )( 
            IZDataObject __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pcch);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ByteCount )( 
            IZDataObject __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pcb);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsAnsi )( 
            IZDataObject __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pfAnsi);
        
        END_INTERFACE
    } IZDataObjectVtbl;

    interface IZDataObject
    {
        CONST_VTBL struct IZDataObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IZDataObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IZDataObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IZDataObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IZDataObject_GetData(This,pformatetcIn,pmedium)	\
    (This)->lpVtbl -> GetData(This,pformatetcIn,pmedium)

#define IZDataObject_GetDataHere(This,pformatetc,pmedium)	\
    (This)->lpVtbl -> GetDataHere(This,pformatetc,pmedium)

#define IZDataObject_QueryGetData(This,pformatetc)	\
    (This)->lpVtbl -> QueryGetData(This,pformatetc)

#define IZDataObject_GetCanonicalFormatEtc(This,pformatectIn,pformatetcOut)	\
    (This)->lpVtbl -> GetCanonicalFormatEtc(This,pformatectIn,pformatetcOut)

#define IZDataObject_SetData(This,pformatetc,pmedium,fRelease)	\
    (This)->lpVtbl -> SetData(This,pformatetc,pmedium,fRelease)

#define IZDataObject_EnumFormatEtc(This,dwDirection,ppenumFormatEtc)	\
    (This)->lpVtbl -> EnumFormatEtc(This,dwDirection,ppenumFormatEtc)

#define IZDataObject_DAdvise(This,pformatetc,advf,pAdvSink,pdwConnection)	\
    (This)->lpVtbl -> DAdvise(This,pformatetc,advf,pAdvSink,pdwConnection)

#define IZDataObject_DUnadvise(This,dwConnection)	\
    (This)->lpVtbl -> DUnadvise(This,dwConnection)

#define IZDataObject_EnumDAdvise(This,ppenumAdvise)	\
    (This)->lpVtbl -> EnumDAdvise(This,ppenumAdvise)


#define IZDataObject_get_Size(This,pcch)	\
    (This)->lpVtbl -> get_Size(This,pcch)

#define IZDataObject_get_ByteCount(This,pcb)	\
    (This)->lpVtbl -> get_ByteCount(This,pcb)

#define IZDataObject_get_IsAnsi(This,pfAnsi)	\
    (This)->lpVtbl -> get_IsAnsi(This,pfAnsi)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IZDataObject_get_Size_Proxy( 
    IZDataObject __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *pcch);


void __RPC_STUB IZDataObject_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IZDataObject_get_ByteCount_Proxy( 
    IZDataObject __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *pcb);


void __RPC_STUB IZDataObject_get_ByteCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IZDataObject_get_IsAnsi_Proxy( 
    IZDataObject __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pfAnsi);


void __RPC_STUB IZDataObject_get_IsAnsi_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IZDataObject_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
