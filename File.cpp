#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#include "Common.h"
#include <shlobj.h>
#include <io.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
int CZDoc::AccessDeniedHandler(int nException, DWORD * pdwError, void * pv)
{
	static BOOL fInErrorRecovery = FALSE;

	CZEditFrame * pzef = g_pzea->GetCurrentFrame();
	AssertPtr(pzef);
	CZDoc * pzd = pzef->GetCurrentDoc();
	AssertPtr(pzd);
	pzd->SetAccessError(TRUE);
	if (fInErrorRecovery)
	{
		// An error occurred while trying to save the file. The only thing left to do is close the file.
		::MessageBox(pzef->GetHwnd(),
			"The file could not be saved successfully. It will now be closed.",
			g_pszAppName, MB_OK | MB_ICONSTOP);
		pzef->CloseFile(-1);
		return EXCEPTION_EXECUTE_HANDLER;
	}
	if (::MessageBox(pzef->GetHwnd(), 
		"A fatal exception has occurred.\n"
		"There is a good chance that the current file can be saved without any data loss.\n"
		"Choose OK to select a filename to save the file to. "
		"(The new file will then be re-opened.)\n"
		"Otherwise, choose Cancel to close the file and lose all changes.",
		g_pszAppName, MB_OKCANCEL | MB_ICONSTOP) == IDOK)
	{
		char szFilename[MAX_PATH] = {0};
		strcpy(szFilename, pzd->GetFilename());
		FileType ft = pzef->GetFile(true, szFilename);
		if (ft != -1)
		{
			RECT rect;
			HWND hwndStatus = pzef->GetStatusHwnd();
			::SendMessage(hwndStatus, SB_GETRECT, ksboMessage, (long) &rect);
			::InflateRect(&rect, -1, -2);
			HWND hwndProgress = ::CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, NULL, 
				WS_CHILD | WS_VISIBLE, rect.left, rect.top, rect.right - rect.left,
				rect.bottom - rect.top, hwndStatus, 0, g_fg.m_hinst, NULL);
			fInErrorRecovery = TRUE;
			pzd->SaveFile(szFilename, ft, hwndProgress);
			::DestroyWindow(hwndProgress);
			pzef->CloseFile(-1);
			szFilename[lstrlen(szFilename) + 1] = 0;
			pzef->ReadFile(szFilename, ft);
			fInErrorRecovery = FALSE;
		}
	}
	pzef->CloseFile(-1);
	return EXCEPTION_EXECUTE_HANDLER;
}


/*----------------------------------------------------------------------------------------------
	Add the specified file to the linked list of open files and to the tab control.
	iFile is the index of the tab to insert the new tab in front of (0 means the first tab).
	When the function returns, iFile could be different if an existing tab was reused.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::InsertFile(int & iFile, CZDoc * pzdFile, char * pszFilename)
{
	Assert(iFile >= 0 && iFile <= m_pzt->GetItemCount());

	bool fNewWindow = false;
	if (m_pzt->GetItemCount() == 1 &&
		m_pzdCurrent && !m_pzdCurrent->GetModify() &&
		lstrcmp(m_pzdCurrent->GetFilename(), "Untitled") == 0)
	{
		// The only file that is open is untitled and hasn't been saved or modified,
		// so we can reuse the tab.
		if (m_pzdCurrent)
			delete m_pzdCurrent;
		m_pzdCurrent = m_pzdFirst = pzdFile;
		pzdFile->Next(NULL);
		pzdFile->Prev(NULL);
		iFile = 0;
	}
	else
	{
		// A new tab must be created for the new file.
		fNewWindow = true;
	}

	// Insert node into linked list of open documents.
	if (fNewWindow)
	{
		// Remove pzdFile from the current linked list it belongs to.
		CZDoc * pzdPrev = pzdFile->Prev();
		CZDoc * pzdNext = pzdFile->Next();
		if (pzdPrev)
			pzdPrev->Next(pzdNext);
		if (pzdNext)
			pzdNext->Prev(pzdPrev);

		// Add it to this window's list.
		if (iFile == 0)
		{
			pzdFile->Next(m_pzdFirst);
			if (m_pzdFirst)
				m_pzdFirst->Prev(pzdFile);
			m_pzdFirst = pzdFile;
			pzdFile->Prev(NULL);
		}
		else
		{
			CZDoc * pzd = m_pzdFirst;
			for (int i = 0; i < iFile - 1; i++)
				pzd = pzd->Next();
			AssertPtr(pzd); // pzdFile will be inserted after pzd in the linked list.
			CZDoc * pzdNext = pzd->Next(); // pzdNext can be NULL.
			pzdFile->Next(pzdNext);
			pzdFile->Prev(pzd);
			if (pzdNext)
				pzdNext->Prev(pzdFile);
			pzd->Next(pzdFile);
		}
	}

	m_pzt->SetItemImage(iFile, pszFilename, fNewWindow);

	return true;
}


/*----------------------------------------------------------------------------------------------
	Remove the specified file from the linked list of open files and from the tab control.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::RemoveFile(int iFile, CZDoc * pzdFile, CZDoc * pzdPrev, CZDoc * pzdNext)
{
	AssertPtr(pzdFile);
	AssertPtrN(pzdPrev);
	AssertPtrN(pzdNext);

	m_pzt->RemoveItem(iFile);

	// Remove the document from the linked list of open documents.
	if (pzdPrev)
		pzdPrev->Next(pzdNext);
	else
		m_pzdFirst = pzdNext;
	if (pzdNext)
		pzdNext->Prev(pzdPrev);
	pzdFile->Next(NULL);
	pzdFile->Prev(NULL);

	if (pzdFile == m_pzdCurrent)
	{
		// Select a different tab.
		if (pzdNext)
			SelectFile(iFile);
		else if (pzdPrev)
			SelectFile(iFile - 1);
	}

	if (!m_pzdCurrent || m_pzt->GetItemCount() == 0)
	{
		// We've closed the last document, so open a new one.
		m_pzdCurrent = NULL;
		OnCommand(MAKELONG(ID_FILE_NEW, 0), 0);
	}
	::InvalidateRect(m_hwnd, NULL, TRUE);

	char szMessage[MAX_PATH] = {0};
	wsprintf(szMessage, "File %d of %d", m_pzt->GetCurSel() + 1, m_pzt->GetItemCount());
	SetStatusText(ksboFiles, szMessage);

	return true;
}


/*----------------------------------------------------------------------------------------------
	Opens a file and adds a tab for the file.
	If prgchFileContents is not NULL, it will contain the information for the file. This can
	be used for drag/drop operations where the file isn't actually stored on the file system.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::CreateNewFile(char * pszFilename, HWND hwndProgress, FileType ft,
	char * prgchFileContents, DWORD cbSize)
{
	if (ft == kftNone)
		return false;
	if (ft == kftDefault)
	{
		// Based on the extension, see if this should be opened as a binary file.
		char * pszExt = strrchr(pszFilename, '.');
		if (pszExt++)
		{
			char szExt[][10] = { "exe", "dll", "com" };
			int cExt = sizeof(szExt) / (sizeof(char) * 10);
			for (int iExt = 0; iExt < cExt; iExt++)
			{
				if (stricmp(szExt[iExt], pszExt) == 0)
				{
					ft = kftBinary;
					break;
				}
			}
		}
		// 2015-11-30 dzook: Changed to use default for existing files instead of just new files.
		//if (ft == kftDefault && strcmp("Untitled", pszFilename) == 0)
		/*if (ft == kftDefault)
			ft = (FileType)g_fg.m_defaultEncoding; // Default new files to the selected default encoding.*/
	}
	CZDoc * pzdNew = new CZDoc(this, pszFilename, ft);
	if (!pzdNew)
		return false;

	int iFile = m_pzt->GetCurSel() + 1;
	if (!InsertFile(iFile, pzdNew, pszFilename))
		return false;

	// Create a new frame and open the file.
	CZFrame * pzf = new CZFrame();
	bool fSuccess = (pzf != NULL) && pzf->Create(this, pzdNew);
	if (fSuccess)
	{
		if (prgchFileContents)
			fSuccess = pzdNew->OpenFile(pszFilename, prgchFileContents, cbSize, pzf);
		else
			fSuccess = pzdNew->OpenFile(pszFilename, pzf, hwndProgress);
	}

	SelectFile(iFile);
	return fSuccess;
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
UINT CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uiMsg)
	{
	case WM_INITDIALOG:
		{
			OPENFILENAME * pofn = (OPENFILENAME *)lParam;
			AssertPtr(pofn);
			FileOpenInfo * pfoi = (FileOpenInfo *)pofn->lCustData;
			AssertPtr(pfoi);
			pfoi->pzef->Animate(::GetParent(hdlg), pfoi->iCommand, true, true);
			::SetWindowLong(hdlg, GWL_USERDATA, pofn->lCustData);
		}
		break;

	case IDOK:
	case IDCANCEL:
		{
			FileOpenInfo * pfoi = (FileOpenInfo *)::GetWindowLong(hdlg, GWL_USERDATA);
			AssertPtr(pfoi);
			pfoi->pzef->Animate(GetParent(hdlg), pfoi->iCommand, true, false);
		}
		break;

	case WM_NOTIFY:
		if (((NMHDR *)lParam)->code == CDN_FILEOK)
		{
			OFNOTIFY * pofn = (OFNOTIFY *)lParam;
			if (!(pofn->lpOFN->Flags & OFN_FILEMUSTEXIST))
			{
				// This is a Save As dialog.
				struct stat sFile;
				char szMessage[MAX_PATH + 100];
				char * pszFile = pofn->lpOFN->lpstrFile;
				if (!strchr(pszFile, '.'))
					lstrcat(pszFile, ".txt");

				int nT = stat(pszFile, &sFile);
				if (nT != -1 && !(sFile.st_mode & _S_IWRITE))
				{
					wsprintf(szMessage, "%s\nThis file exists with Read Only attributes.\n"
						"Please use a different file name.", pszFile);
					MessageBox(hdlg, szMessage, "Save As",
						MB_OK | MB_ICONWARNING);
					SetWindowLong(hdlg, DWL_MSGRESULT, 1);
					return 1;
				}
				if (nT != -1)
				{
					wsprintf(szMessage, "%s already exists.\nDo you want to replace it?", pszFile);
					if (MessageBox(hdlg, szMessage, "Save As", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
					{
						SetWindowLong(hdlg, DWL_MSGRESULT, 1);
						return 1;
					}
				}
			}
		}
		break;
	}
	return 0;
}


/*----------------------------------------------------------------------------------------------
	Prompt the user to select a filename to save/open. This returns the type of the file.
----------------------------------------------------------------------------------------------*/
FileType CZEditFrame::GetFile(bool fSave, char * pszFilename)
{
	char szInitialDir[MAX_PATH];
	// Changing the size here means it needs to be changed in Virtual.cpp
	// where this method gets called.
	char szFilename[4000] = { 0 };
	const char * pszFilter =
		"Text Files (*.txt)\0*.txt\0"
		"UTF-8 Unicode (*.xml;*.txt)\0*.xml;*.txt\0"
		"UTF-16 Unicode (*.xml;*.txt)\0*.xml;*.txt\0"
		"Binary (*.*)\0*.*\0"
		"All Files (*.*)\0*.*\0\0";
	static OPENFILENAME of = { sizeof(OPENFILENAME), m_hwnd, g_fg.m_hinst,
		pszFilter, 0, 0, 5, 0, sizeof(szFilename), 0, 0, 0, 0,
		OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLEHOOK |
		OFN_PATHMUSTEXIST | OFN_SHAREAWARE | OFN_ENABLESIZING, 0, 0, NULL, 0, OFNHookProc };

	if (!g_fg.m_fCanUseVer5)
	{
		// Since SetLayeredWindowAttributes is only valid for Windows XP +,
		// it should be a valid test for the new Open dialogs as well.
		of.lStructSize = CDSIZEOF_STRUCT(OPENFILENAMEA,lpTemplateName);
	}

	lstrcpy(szInitialDir, pszFilename);
	char * pSlash = strrchr(szInitialDir, '\\');
	if (pSlash)
	{
		*pSlash = 0;
		lstrcpy(szFilename, pSlash + 1);
	}
	else
	{
		lstrcpy(szFilename, pszFilename);
	}
	of.lpstrFile = szFilename;
	of.lpstrInitialDir = szInitialDir;

	FileOpenInfo foi;
	foi.pzef = this;
	foi.iCommand = fSave ? ID_FILE_SAVE : ID_FILE_OPEN;
	of.lCustData = (LPARAM)&foi;

	if (fSave)
	{
		// Get filename to save.
		of.nFilterIndex = m_pzdCurrent->GetFileType() + 1;
		// Because of a problem with the GetSaveFileName, the following flags are
		// not set, and these two issues get taken care of in the OFNHookProc method.
		// The problem happens when the user types a filename without any extension.
		// If adding the default extension results in one of these two conflicts, it
		// is not caught by using the following flags.
		//of.Flags |= OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN;
		if (!::GetSaveFileName(&of))
			return kftNone;
	}
	else
	{
		// Get filename to open.
		of.Flags |= OFN_FILEMUSTEXIST;
		if (!::GetOpenFileName(&of))
			return kftNone;
	}

	WIN32_FIND_DATA wfd;
	HANDLE hFile = ::FindFirstFile(szFilename, &wfd);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// This means that the user selected multiple files.
			char szFullFilenames[4000];
			char * pszFileSrc = szFilename  + strlen(szFilename) + 1;
			char * pszFileDst = szFullFilenames;
			while (*pszFileSrc)
			{
				sprintf(pszFileDst, "%s\\%s", szFilename, pszFileSrc);
				pszFileSrc += strlen(pszFileSrc) + 1;
				pszFileDst += strlen(pszFileDst) + 1;
			}
			*pszFileDst = 0;
			memcpy(pszFilename, szFullFilenames, pszFileDst - szFullFilenames + 1);
			return (FileType)(of.nFilterIndex - 1);
		}
		::FindClose(hFile);
	}
	strcpy(pszFilename, szFilename);
	return (FileType)(of.nFilterIndex - 1);
}


/*----------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::ReadFile(char * pszFilenames, FileType ft)
{
	if (ft == kftNone)
		return false;
	char * pPos = pszFilenames;
	FILE * pFile = NULL;
	char szDirectory[MAX_PATH] = {0};
	char szRealName[MAX_PATH] = {0};

	CWaitCursor wc;

	bool fSelectLine = false;
	int iLine = 0;
	char * pLine = strstr(pszFilenames, " -g");
	if (!pLine)
		pLine = strstr(pszFilenames, " -G");
	if (!pLine)
		pLine = strstr(pszFilenames, " \\g");
	if (!pLine)
		pLine = strstr(pszFilenames, " \\G");
	if (pLine)
	{
		char * pAfterLine = NULL;
		iLine = strtol(pLine + 3, &pAfterLine, 10) - 1;
		fSelectLine = true;
		if (pAfterLine != NULL)
			memmove(pLine, pAfterLine, strlen(pAfterLine) + 2);
	}
	if (iLine < 0)
		iLine = 0;

	// Clear out all quotes in the string.
	int nOffset = 0;
	while (*pPos != 0 || *(pPos + 1) != 0)
	{
		if (*pPos == '"')
			nOffset++;
		else
			*(pPos - nOffset) = *pPos;
		pPos++;
	}

	// Make sure two null characters end the string.
	if (nOffset != 0)
	{
		*(pPos - nOffset) = 0;
		*(pPos - nOffset + 1) = 0;
	}

	// Find directory for files opened from Open dialog box
	DWORD dwFlags = ::GetFileAttributes(pszFilenames);
	if (dwFlags != 0xFFFFFFFF && dwFlags & FILE_ATTRIBUTE_DIRECTORY)
	{
		wsprintf(szDirectory, "%s\\", pszFilenames);
		if (*(pszFilenames + lstrlen(pszFilenames) + 1) != '\0')
			pszFilenames += lstrlen(pszFilenames) + 1;
	}

	char message[MAX_PATH] = {0};
	::InvalidateRect(m_hwnd, NULL, FALSE);
	::UpdateWindow(m_hwnd);

	// Get dimensions for progress bar
	RECT rect;
	::SendMessage(m_hwndStatus, SB_GETRECT, ksboMessage, (long) &rect);
	::InflateRect(&rect, -1, -2);

	HWND hwndProgress = ::CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, NULL,
		WS_CHILD | WS_VISIBLE, rect.left, rect.top, rect.right - rect.left,
		rect.bottom - rect.top, m_hwndStatus, 0, g_fg.m_hinst, NULL);
	bool fSuccess = true;
	while (*pszFilenames != 0)
	{
		::SendMessage(hwndProgress, PBM_SETPOS, 0, 0);
		::SendMessage(hwndProgress, PBM_SETRANGE32, 0, 0);
		::ZeroMemory(szRealName, MAX_PATH);
		if (*szDirectory != 0)
			lstrcpy(szRealName, szDirectory);
		lstrcat(szRealName, pszFilenames);

		// Make sure the long (not short) filename is found if possible
		IShellFolder * psf = NULL;
		if (SUCCEEDED(::SHGetDesktopFolder(&psf)))
		{
			wchar wszRealName[MAX_PATH];
			char szLongName[MAX_PATH];
			ITEMIDLIST * pidl;
			::MultiByteToWideChar(CP_ACP, 0, szRealName, MAX_PATH, wszRealName, MAX_PATH);
			if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, wszRealName, NULL,  &pidl, NULL)))
			{
				if (::SHGetPathFromIDList(pidl, szLongName))
					lstrcpy(szRealName, szLongName);
				IMalloc * pm = NULL;
				if (SUCCEEDED(::SHGetMalloc(&pm)))
				{
					pm->Free(pidl);
					pm->Release();
					pm = NULL;
				}
			}
			psf->Release();
			psf = NULL;
		}

		// Switch to the proper tab if the file is already open
		int nFound = 0;
		int nTab = 1;
		CZDoc * pzd = m_pzdFirst;
		while (pzd && !nFound)
		{
			if (!lstrcmpi(pszFilenames, pzd->GetFilename()))
			{
				nFound = nTab;
			}
			else
			{
				pzd = pzd->Next();
				nTab++;
			}
		}
		if (nFound)
		{
			if (m_pzt->GetCurSel() != nFound - 1)
				SelectFile(nFound - 1);
			if (fSelectLine)
				SelectLine(iLine);

			pszFilenames = pszFilenames + lstrlen(pszFilenames);
			if (*pszFilenames == 0)
				pszFilenames++;
			continue;
		}

		if (ft == -1)
			ft = kftDefault;
		if (!CreateNewFile(szRealName, hwndProgress, ft))
		{
			char szMessage[MAX_PATH] = {0};
			wsprintf(szMessage, "Could not open %s", szRealName);
			::MessageBox(m_hwnd, szMessage, g_pszAppName, MB_ICONEXCLAMATION);
			CloseFile(-1);
			fSuccess = false;
		}
		else
		{
			SetStatusText(ksboMessage, "Ready");
			SetStatusText(ksboFileType, g_pszFileType[m_pzdCurrent->GetFileType()]);

			// Update the MRU file list
			int nFound = 0;
			while (nFound < kcMaxFiles && lstrcmpi(m_szFileList[nFound++], szRealName) != 0);
			memmove(m_szFileList[1], m_szFileList[0], sizeof(m_szFileList[0]) * --nFound);
			::ZeroMemory(m_szFileList[0], sizeof(m_szFileList[0]));
			lstrcpy(m_szFileList[0], szRealName);
			m_szFileList[0][sizeof(m_szFileList[0]) - 1] = ft;

			if (fSelectLine)
				SelectLine(iLine);
		}

		// Advance szFilenames to the next file if one exists
		pszFilenames = pszFilenames + lstrlen(pszFilenames);
		if (*pszFilenames == 0)
			pszFilenames++;

		wsprintf(message, "File %d of %d", m_pzt->GetCurSel() + 1, m_pzt->GetItemCount());
		SetStatusText(ksboFiles, message);
	}
	::DestroyWindow(hwndProgress);
	::GetClientRect(m_hwnd, &rect);
	::SetFocus(m_pzdCurrent->GetHwnd());
	return fSuccess;
}


/*----------------------------------------------------------------------------------------------
	Save the specified file.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::SaveFile(CZDoc * pzd, int iFile, bool fShowDlg, bool * pfCancel)
{
	AssertPtr(pzd);
	AssertPtrN(pfCancel);
	if (pfCancel)
		*pfCancel = false;
	// Make sure the file is not read only
	DWORD dwAttributes = ::GetFileAttributes(pzd->GetFilename());
	if ((dwAttributes != -1) && (dwAttributes & FILE_ATTRIBUTE_READONLY))
	{
		if (!fShowDlg)
		{
			::MessageBox(m_hwnd,
				"This file is read-only. You must save to a different filename.",
				g_pszAppName, MB_OK | MB_ICONWARNING);
		}
		fShowDlg = true;
	}

	FileType ft = pzd->GetFileType();
	// If SaveAs was selected or the current file has no name, get a name
	if (fShowDlg || (lstrcmp(pzd->GetFilename(), "Untitled") == 0))
	{
		int iCurSel = m_pzt->GetCurSel();
		if (iCurSel != iFile)
			SelectFile(iFile);
		ft = GetFile(true, pzd->GetFilename());
		// TODO: This value should be passed in as another parameter.
		if (ft == kftNone)
		{
			if (pfCancel)
				*pfCancel = true;
			return true;
		}
		else if (ft == kftDefault)
			ft = kftAnsi;
		::UpdateWindow(m_hwnd);
	}

	CWaitCursor wc;

	// Get dimensions for progress bar.
	RECT rect;
	::SendMessage(m_hwndStatus, SB_GETRECT, ksboMessage, (long) &rect);
	::InflateRect(&rect, -1, -2);
	HWND hwndProgress = ::CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, NULL, 
		WS_CHILD | WS_VISIBLE, rect.left, rect.top, rect.right - rect.left,
		rect.bottom - rect.top, m_hwndStatus, 0, g_fg.m_hinst, NULL);

	char * pszFilename = pzd->GetFilename();
	pzd->SaveFile(pszFilename, ft, hwndProgress);
	::DestroyWindow(hwndProgress);
	SetStatusText(ksboFileType, g_pszFileType[pzd->GetFileType()]);

	pzd->SetModify(false);
	m_pzt->SetItemImage(iFile, pszFilename, false);

	char * prgchFilePart = strrchr(pszFilename, '\\');
	prgchFilePart = prgchFilePart ? prgchFilePart + 1 : pszFilename;

	char szTitle[MAX_PATH] = {0};
	wsprintf(szTitle, "%s - %s", prgchFilePart, g_pszAppName);
	::SetWindowText(m_hwnd, szTitle);
	::InvalidateRect(m_hwnd, NULL, TRUE);
	::UpdateWindow(m_hwnd);

	// Update the MRU file list
	int iFound = 0;
	while (iFound < kcMaxFiles && lstrcmpi(m_szFileList[iFound++], pzd->GetFilename()) != 0)
		;
	memmove(m_szFileList[1], m_szFileList[0], sizeof(m_szFileList[0]) * --iFound);
	::ZeroMemory(m_szFileList[0], sizeof(m_szFileList[0]));
	lstrcpy(m_szFileList[0], pzd->GetFilename());
	m_szFileList[0][sizeof(m_szFileList[0]) - 1] = ft;
	return true;
}


/*----------------------------------------------------------------------------------------------
	Loop through all the open files and save any modified ones.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::SaveAll()
{
	CZDoc * pzd = m_pzdFirst;
	while (pzd->Next())
		pzd = pzd->Next();
	int iFile = m_pzt->GetItemCount() - 1;
	int iCurSel = m_pzt->GetCurSel();

	// Look at each file starting at the last one and save all the ones that have been modified.
	while (pzd)
	{
		if (pzd->GetModify())
		{
			if (m_pzt->GetCurSel() != iFile)
				SelectFile(iFile);
			bool fCancel;
			bool fSuccess = SaveFile(pzd, iFile, false, &fCancel);

			pzd->UpdateModified(iFile, true);

			if (!fSuccess)
			{
				char szMessage[MAX_PATH];
				wsprintf(szMessage, "Could not save %s!", pzd->GetFilename());
				::MessageBox(m_hwnd, szMessage, g_pszAppName, MB_ICONEXCLAMATION); 
				return false;
			}
			if (fCancel)
				return false;
		}
		pzd = pzd->Prev();
		iFile--;
	}
	m_pzt->SetCurSel(iCurSel);
	return true;
}


/*----------------------------------------------------------------------------------------------
	Close the specified document. If iFile < 0, the current document is closed.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::CloseFile(int iFile)
{
	CZDoc * pzdFile;

	if (iFile < 0)
	{
		pzdFile = m_pzdCurrent;
		iFile = m_pzt->GetCurSel();
	}
	else
	{
		iFile = min(iFile, m_pzt->GetItemCount() - 1);
		pzdFile = m_pzdFirst;
		int iT = iFile;
		while (iT-- > 0 && pzdFile)
			pzdFile = pzdFile->Next();
	}
	if (!pzdFile)
		return false;

	int nResponse = IDNO;
	if (pzdFile->GetModify())
	{
		int iCurSel = m_pzt->GetCurSel();
		if (iCurSel != iFile)
			SelectFile(iFile);
		char szBuffer[MAX_PATH] = {0};
		wsprintf(szBuffer, "Save changes to %s?", pzdFile->GetFilename());
		nResponse = ::MessageBox(NULL, szBuffer, g_pszAppName,
			MB_ICONQUESTION | MB_YESNOCANCEL | MB_TASKMODAL);
	}
	else if (lstrcmp(pzdFile->GetFilename(), "Untitled") == 0)
	{
		// If the only file open is an unmodified empty file, return.
		if (m_pzt->GetItemCount() == 1)
			return true;
	}

	if (nResponse == IDCANCEL)
		return false;

	if (nResponse == IDYES)
	{
		bool fCancel;
		bool fSuccess = SaveFile(pzdFile, iFile, false, &fCancel);
		if (!fSuccess)
		{
			char szMessage[MAX_PATH];
			wsprintf(szMessage, "Could not save %s!", pzdFile->GetFilename());
			::MessageBox(m_hwnd, szMessage, g_pszAppName, MB_ICONEXCLAMATION); 
			return false;
		}
		if (fCancel)
			return false;
	}

	if (!RemoveFile(iFile, pzdFile, pzdFile->Prev(), pzdFile->Next()))
		return false;
	delete pzdFile;

	return true;
}


/*----------------------------------------------------------------------------------------------
	Close all open documents.
	If iExceptFile is not -1, the corresponding file will not be closed.
----------------------------------------------------------------------------------------------*/
bool CZEditFrame::CloseAll(bool fExiting, int iExceptFile)
{
	int cFiles = m_pzt->GetItemCount();
	if (!cFiles || !m_pzdFirst)
		return true;

	// Go to the last document because we want to close them in reverse order.
	CZDoc * pzdLast = m_pzdFirst;
	while (pzdLast->Next())
		pzdLast = pzdLast->Next();

	int iFile = cFiles - 1;
	for (CZDoc * pzd = pzdLast; pzd; pzd = pzd->Prev(), iFile--)
	{
		if (iFile == iExceptFile)
		{
			// We don't need to prompt for this one because we're not going to close it.
			continue;
		}

		int nResponse = IDNO;
		if (pzd->GetModify())
		{
			if (iFile != m_pzt->GetCurSel())
				SelectFile(iFile);
			char szBuffer[MAX_PATH] = { 0 };
			wsprintf(szBuffer, "Save changes to %s?", pzd->GetFilename());
			nResponse = ::MessageBox(NULL, szBuffer, g_pszAppName,
				MB_ICONQUESTION | MB_YESNOCANCEL | MB_TASKMODAL);
		}

		if (nResponse == IDCANCEL)
			return false;
		if (nResponse == IDYES)
			SaveFile(pzd, iFile, false, NULL);
	}

	if (iExceptFile == -1)
	{
		m_pzt->DeleteAllItems();
		if (m_hwndTabMgrDlg)
		{
			HWND hwndList = ::GetDlgItem(m_hwndTabMgrDlg, IDC_LISTVIEW);
			ListView_DeleteAllItems(hwndList);
		}

		while (m_pzdFirst)
		{
			CZDoc * pzd = m_pzdFirst;
			m_pzdFirst = m_pzdFirst->Next();
			delete pzd;
		}
		m_pzdCurrent = m_pzdFirst = NULL;
		if (!fExiting)
			OnCommand(MAKELONG(ID_FILE_NEW, 0), 0);
	}
	else
	{
		HWND hwndList = NULL;
		if (m_hwndTabMgrDlg)
			hwndList = ::GetDlgItem(m_hwndTabMgrDlg, IDC_LISTVIEW);

		// We have to remove the tabs before we clear out the linked list of documents
		// or we'll have problems. This part cleans the tabs up.
		CZDoc * pzd = pzdLast;
		CZDoc * pzdKeep = NULL;
		for (int iFile = cFiles - 1; iFile >= 0; iFile--)
		{
			AssertPtr(pzd);
			if (iFile == iExceptFile)
			{
				pzdKeep = pzd;
			}
			else
			{
				m_pzt->RemoveItem(iFile);
				if (hwndList)
					ListView_DeleteItem(hwndList, iFile);
			}
			pzd = pzd->Prev();
		}
		AssertPtr(pzdKeep);

		// This part cleans up the linked list.
		pzd = m_pzdFirst;
		while (pzd)
		{
			if (pzd == pzdKeep)
			{
				pzd = pzd->Next();
				pzdKeep->Prev(NULL);
				pzdKeep->Next(NULL);
			}
			else
			{
				CZDoc * pzdT = pzd;
				pzd = pzd->Next();
				delete pzdT;
			}
		}

		m_pzdFirst = m_pzdCurrent = pzdKeep;
		SelectFile(0);
	}

	return true;
}


/*----------------------------------------------------------------------------------------------
	Update information about the current file.
----------------------------------------------------------------------------------------------*/
void CZEditFrame::SelectFile(int iFile)
{
	if (iFile < 0)
		iFile = 0;
	int cFiles = m_pzt->GetItemCount();
	if (iFile >= cFiles)
		iFile = cFiles - 1;
	m_pzt->SetCurSel(iFile);

	HMENU hSubMenu = ::GetSubMenu(::GetMenu(m_hwnd), 2);
	::EnableMenuItem(hSubMenu, ID_WINDOW_PREV,
		MF_BYCOMMAND | (iFile == 0 ? MF_GRAYED : MF_ENABLED));
	::EnableMenuItem(hSubMenu, ID_WINDOW_NEXT,
		MF_BYCOMMAND | (iFile == cFiles - 1 ? MF_GRAYED : MF_ENABLED));

	CZDoc * pzdNew = m_pzdFirst;
	for (int i = 0; i < iFile; i++)
		pzdNew = pzdNew->Next();
	AssertPtr(pzdNew);

	HWND hwndOldFrame = m_pzdCurrent ? m_pzdCurrent->GetFrame()->GetHwnd() : NULL;
	HWND hwndNewFrame = pzdNew->GetFrame()->GetHwnd();

	RECT rc;
	m_pzt->GetClientRect(&rc);
	::MoveWindow(hwndNewFrame, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
	::ShowWindow(hwndNewFrame, SW_SHOW);
	::SetFocus(hwndNewFrame);
	if (hwndOldFrame && hwndNewFrame != hwndOldFrame)
		::ShowWindow(hwndOldFrame, SW_HIDE);

	m_pzdCurrent = pzdNew;

	char szBuffer[MAX_PATH] = { 0 };
	char * pszFile = strrchr(m_pzdCurrent->GetFilename(), '\\');
	wsprintf(szBuffer, "%s - %s", pszFile == NULL ? "Untitled" : pszFile + 1, g_pszAppName);
	::SetWindowText(m_hwnd, szBuffer);
	SetStatusText(ksboWrap, m_pzdCurrent->GetWrap() ? "Wrap On" : "Wrap Off");
	wsprintf(szBuffer, "File %d of %d", iFile + 1, cFiles);
	SetStatusText(ksboFiles, szBuffer);
	SetStatusText(ksboInsert, m_pzdCurrent->GetOvertype() ? "OVR" : "INS");
	SetStatusText(ksboFileType, g_pszFileType[m_pzdCurrent->GetFileType()]);
	UpdateTextPos();
	pzdNew->UpdateModified(iFile, true);
}


/*----------------------------------------------------------------------------------------------
	Update the default encoding used for new files.
----------------------------------------------------------------------------------------------*/
void CZEditFrame::SetDefaultEncoding(int iEncoding)
{
	g_fg.m_defaultEncoding = iEncoding;
}


/*----------------------------------------------------------------------------------------------
	Update the statusbar to show the curent text position.
----------------------------------------------------------------------------------------------*/
void CZEditFrame::UpdateTextPos()
{
	// Don't update position if the Find dialog box is busy
	if (m_pfd->IsBusy())
		return;

	m_pfd->ResetFind();
	SetStatusText(ksboLineCol, m_pzdCurrent->GetCurrentView()->GetCurPosString());

	if (g_fg.m_hwndViewCodes)
	{
		AssertPtr(m_pzdCurrent);
		CZView * pzv = m_pzdCurrent->GetCurrentView();
		AssertPtr(pzv);
		UINT ichOver;
		pzv->GetSelection(&ichOver);
		int cchCodesBefore = g_fg.m_cchCodesBefore;
		if (ichOver < (UINT)cchCodesBefore)
			cchCodesBefore = (int)ichOver;
		int cchCodesAfter = g_fg.m_cchCodesAfter;
		if (ichOver + 1 + cchCodesAfter > m_pzdCurrent->GetCharCount())
			cchCodesAfter = m_pzdCurrent->GetCharCount() - ichOver - 1;
		UINT ichStart = ichOver - cchCodesBefore;
		UINT cch = cchCodesBefore + 1 + cchCodesAfter;
		Assert(ichStart + cch <= m_pzdCurrent->GetCharCount());
		if (cch == 0)
		{
			::SetDlgItemText(g_fg.m_hwndViewCodes, IDC_VALUES, ">");
		}
		else
		{
			void * pv;
			cch = m_pzdCurrent->GetText(ichStart, cch, &pv);
			if (cch > 0 && pv)
			{
				char * prgch = new char[cch * 10 + 1];
				if (prgch)
				{
					FileType ft = m_pzdCurrent->GetFileType();
					bool fAnsi = (ft != kftUnicode8) && (ft != kftUnicode16);
					char * prgchDst = prgch;
					if (fAnsi)
					{
						char * prgchPos = (char *)pv;
						char * prgchLim = prgchPos + cch;
						while (prgchPos < prgchLim)
						{
							if (cchCodesBefore-- == 0)
							{
								*prgchDst++ = '>';
								*prgchDst++ = ' ';
							}
							prgchDst += wsprintf(prgchDst, "%02x ", (*prgchPos++ & 0xFF));
						}
					}
					else
					{
						wchar_t * prgchPos = (wchar_t*)pv;
						wchar_t * prgchLim = prgchPos + cch;
						while (prgchPos < prgchLim)
						{
							ULONG ch = *prgchPos++;
							if (cchCodesBefore-- == 0)
							{
								*prgchDst++ = '>';
								*prgchDst++ = ' ';
							}
							// Make sure we have another character before jumping to the surrogate
							// section below.
							if (ch < 0xd800 || ch > 0xdbff || prgchPos >= prgchLim)
							{
								prgchDst += wsprintf(prgchDst, "%04x ", (ch & 0xFFFF));
							}
							else
							{
								cchCodesBefore--;
								ULONG ch2 = *prgchPos++;
								ULONG ch3 = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000;
								prgchDst += wsprintf(prgchDst, "%04x %04x (%x) ", (ch & 0xFFFF), (ch2 & 0xFFFF), ch3);
							}
						}
					}
					if (cchCodesBefore == 0)
					{
						*prgchDst++ = '>';
						*prgchDst = 0;
					}
					::SetDlgItemText(g_fg.m_hwndViewCodes, IDC_VALUES, prgch);
					delete prgch;
				}
				delete pv;
			}
		}
	}
}

/*----------------------------------------------------------------------------------------------
	Select the requested line in the current file.
----------------------------------------------------------------------------------------------*/
void CZEditFrame::SelectLine(int iLine)
{
	char szBuffer[MAX_PATH];
	DWORD dwStartChar = m_pzdCurrent->CharFromPara(iLine);
	UINT iStartPara;
	DocPart * pdpStart = m_pzdCurrent->GetPart(iLine, true, NULL, &iStartPara);
	DWORD cchInLine = CharsInPara(pdpStart, iStartPara);
	DWORD cchAtEnd = CharsAtEnd(pdpStart, iStartPara);
	DWORD dwStopChar = m_pzdCurrent->CharFromPara(iLine + 1);
	if (dwStopChar >= dwStartChar + cchAtEnd)
		dwStopChar -= cchAtEnd;
	m_pzdCurrent->SetSelection(dwStartChar, dwStopChar, true, true);
	if (m_pzdCurrent->GetFileType() == kftBinary)
		sprintf(szBuffer, "Line %d,  Col %d", iLine + 1, cchInLine - 1);
	else
		sprintf(szBuffer, "Para %d,  Col %d", iLine + 1, cchInLine - 1);
	SetStatusText(ksboLineCol, szBuffer);
}