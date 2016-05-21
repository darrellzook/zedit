/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Sat Nov 06 13:30:59 1999
 */
/* Compiler settings for C:\My Documents\Visual Studio Projects\ZEdit\DragDropTlb.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

// {AE8BFEC0-93CF-11D3-AFE4-40904EC1DD0F}
static const IID IID_IZDataObject = {0xAE8BFEC0,0x93CF,0x11D3,{0xAF,0xE4,0x40,0x90,0x4E,0xC1,0xDD,0x0F}};

// {DFEE44F1-C5B1-42e1-8C8B-98DAA46101A3}
static const IID IID_IZFileDataObject = {0xDFEE44F1,0xC5B1,0x42E1,{0x8C,0x8B,0x98,0xDA,0xA4,0x61,0x01,0xA3}};


#ifdef __cplusplus
}
#endif

