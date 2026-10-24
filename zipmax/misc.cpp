#include "stdafx.h"
#include "misc.h"

CString CMisc::GetPureFileName(const CString& str)
{
	// cut a fullpathname down to the pure filename (e.g.: c:\temp\test.bin to test.bin)
	// returns the pure filename
	// const CString& str = pathname to cut down

	int nLen = str.GetLength();

	const int nLenOld = nLen;

	while (nLen-- > 0)
		if (str[nLen] == _T('\\') || str[nLen] == _T('/') || str[nLen] == _T(':'))
			break;

	return (str.Right(nLenOld - nLen - 1));
}


void CMisc::GetAllCmdLineZip(const CString& strCmdLine, std::vector<CString> *pvecCmdLineFiles, const BOOL bSubFolders)
{
	// get all command line zipped files. if a folder was in the command line, check that folder as well
	// returns nothing (the vector will be filled)
	// const CString& strCmdLine        = command line string
	// std::vector<CString> *pvecDroppedFiles = pointer to a vector which will hold the found filenames
	// const BOOL bSubFolders         = flag to recursively scan folders

	pvecCmdLineFiles->clear();

	CString strFullLine = strCmdLine;
	strFullLine.Trim();

	if (strFullLine == _T(""))
		return;

	int nIndex = 0;
	int nStart = 0;
	int nQuotes = 0;
	CString strEntry;

	while (nIndex < strFullLine.GetLength())
	{
		if (strFullLine[nIndex] == _T('\"'))
			nQuotes++;

		if (strFullLine[nIndex] == _T(' ') && nQuotes % 2 == 0)
		{
			strEntry = TrimEntry(strFullLine.Mid(nStart, nIndex - nStart));

			if (IsFolder(strEntry) || IsFile(strEntry))
			{
				nStart = nIndex + 1;
				AddEntry(strEntry, pvecCmdLineFiles, bSubFolders);
			}
		}

		nIndex++;
	}

	AddEntry(TrimEntry(strFullLine.Mid(nStart, nIndex - nStart)), pvecCmdLineFiles, bSubFolders);

	return;
}


CString CMisc::TrimEntry(const CString& strEntry)
{
	// remove possible quotes at the beginning or end of a filename
	// returns the trimmed string
	// const CString& strEntry    = the string which should be trimmed

	CString str = strEntry;

	if (str.Left(1) == _T('\"'))
		str = str.Right(str.GetLength() - 1);

	if (str.Right(1) == _T('\"'))
		str = str.Left(str.GetLength() - 1);

	return str;
}


void CMisc::AddEntry(const CString& strEntry, std::vector<CString> *pvecCmdLineFiles, const BOOL bSubFolders)
{
	// adds an entry to the filelist
	// returns nothing (the vector will be filled)
	// const CString& strEntry          = the filename to add
	// std::vector<CString> *pvecDroppedFiles = pointer to a vector which will hold the found filenames
	// const BOOL bSubFolders         = flag to recursively scan folders

	TCHAR cPathName[_MAX_PATH],cTempPathName[_MAX_PATH],*dummyptr;

	if (GetFullPathName(strEntry,_MAX_PATH,cTempPathName,&dummyptr))
	{
		if ((GetLongPathName(cTempPathName, cPathName, _MAX_PATH) != 0))
		{
			const CString strLong(cPathName);

			if (IsFolder(strLong) && bSubFolders)
				ScanDroppedSubFolderZip(strLong + _T("\\"), pvecCmdLineFiles);
			else
				if (strLong.Right(4).CompareNoCase(_T(".zip")) == 0)
					pvecCmdLineFiles->push_back(strLong);
		}
	}

	return;
}



void CMisc::GetAllDroppedZip(HDROP hDropInfo, std::vector<CString> *pvecDroppedFiles, const BOOL bSubFolders)
{
	// get all drag&dropped zipped files. if a folder was dropped, check that folder as well
	// returns nothing (the vector will be filled)
	// HDROP hDropInfo              = handle to drag&drop info
	// std::vector<CString> *pvecDroppedFiles = pointer to a vector which will hold the found filenames
	// const BOOL bSubFolders         = flag to recursively scan folders

	const UINT nFiles = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);

	pvecDroppedFiles->clear();
	pvecDroppedFiles->reserve(nFiles);

	for (int i = 0; i < (int)nFiles; i++)
	{
		const UINT nLen = DragQueryFile(hDropInfo, i, NULL, 0);
		CString strFileName;

		if (nLen > 0 && DragQueryFile(hDropInfo, i, strFileName.GetBuffer(nLen + 1), nLen + 1) > 0)
		{
			strFileName.ReleaseBuffer();

			if (IsFolder(strFileName) && bSubFolders)
				ScanDroppedSubFolderZip(strFileName + _T("\\"), pvecDroppedFiles);
			else
				if (strFileName.Right(4).CompareNoCase(_T(".zip")) == 0)
					pvecDroppedFiles->push_back(strFileName);
		}
	}

	return;
}


void CMisc::ScanDroppedSubFolderZip(const CString& strFolderName, std::vector<CString> *pvecDroppedFiles)
{
	// recursive function to scan folders
	// returns nothing
	// const CString& strFolderName       = current folder (ending with "\\")
	// std::vector<CString> *pvecDroppedFiles = pointer to a vector which holds the found filenames

	CFileFind findFile;

	BOOL bMore = findFile.FindFile(strFolderName + _T("*.*"));

	while (bMore)
	{
		bMore = findFile.FindNextFile();

		if (! findFile.IsDots())
		{
			if (findFile.IsDirectory())
				ScanDroppedSubFolderZip(findFile.GetFilePath() + _T("\\"), pvecDroppedFiles);
			else
			{
				const CString strFileName = findFile.GetFilePath();

				if (strFileName.Right(4).CompareNoCase(_T(".zip")) == 0)
					pvecDroppedFiles->push_back(strFileName);
			}
		}
	}

	findFile.Close();

	return;
}


void CMisc::ScanFolderZip(const CString& strFolderName, const BOOL bSubfolders, std::vector<CString> *pvecFiles)
{
	// finds the zipfiles of a folder (used by the File menu)
	// const CString& strFolderName   = the folder (with or without ending "\\")
	// const BOOL bSubfolders         = TRUE: also all subfolders, FALSE: only this folder
	// std::vector<CString> *pvecFiles = receives the filenames (the vector is cleared first)

	pvecFiles->clear();

	CString strFolder = strFolderName;

	if (strFolder.Right(1) != _T("\\"))
		strFolder += _T("\\");

	if (bSubfolders)
	{
		ScanDroppedSubFolderZip(strFolder, pvecFiles);
		return;
	}

	CFileFind findFile;

	BOOL bMore = findFile.FindFile(strFolder + _T("*.*"));

	while (bMore)
	{
		bMore = findFile.FindNextFile();

		if (findFile.IsDots() || findFile.IsDirectory())
			continue;

		const CString strFileName = findFile.GetFilePath();

		if (strFileName.Right(4).CompareNoCase(_T(".zip")) == 0)
			pvecFiles->push_back(strFileName);
	}

	findFile.Close();
}


CString CMisc::GetPurePathName(const CString& str)
{
	// get the pure pathname of a string which represents a full filename (e.g. c:\temp\test.bin returns c:\temp\test\)
	// returns the cut down string
	// const CString& str = string to cut down

	int nLen = str.GetLength();

	while (nLen-- > 0)
		if (str[nLen] == _T('\\') || str[nLen] == _T('/') || str[nLen] == _T(':'))
			break;

	return str.Left(++nLen);
}


CString CMisc::GetFullFileNameNoDrive(const CString& str)
{
	// gets the full filename but without a possible drive letter
	// returns the fullfilename
	// const CString& str = string to cut down

	CString strReturn = str;

	int nIndex = strReturn.Find(_T(':'));

	if (nIndex == -1)
	{
		int nIndexNet = strReturn.Find(_T("\\\\"));

		if (nIndexNet != -1)
		{
			strReturn = strReturn.Mid(nIndex + 3);
			int nIndexPath = strReturn.Find(_T("\\"));
			if (nIndexPath == -1)
				nIndexPath = strReturn.Find(_T("/"));
			if (nIndexPath != -1)
				strReturn = strReturn.Mid(nIndexPath + 1);
		}
	}
	else
		strReturn = strReturn.Mid(nIndex + 2);

	return strReturn;
}

BOOL CMisc::SetFileToCurrentTime(const CString& strFileName)
{
	// sets the file date/timestamp to the current date/time
	// returns TRUE when operation succeeded, otherwise FALSE
	// const CString& strFileName = full filename

	if ((int)strFileName.GetLength() >= (int)MAX_PATH)
		return FALSE;

	HANDLE hFile = ::CreateFile(strFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;

	FILETIME ft;
    SYSTEMTIME st;
    BOOL f = FALSE;

	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft);
	f = ::SetFileTime(hFile, &ft, &ft, &ft);

	::CloseHandle(hFile);

	return f;
}

static CString LongPath(const CString& str)
{
	// adds the \\?\ prefix, so that paths longer than MAX_PATH work too

	if (str.Left(4) == _T("\\\\?\\"))
		return str;

	if (str.Left(2) == _T("\\\\"))
		return _T("\\\\?\\UNC\\") + str.Mid(2);

	if (str.GetLength() >= 3 && str[1] == _T(':'))
		return _T("\\\\?\\") + str;

	return str;
}


BOOL CMisc::RemoveAFile(const CString& strName)
{
	// removes a file (also read only/hidden/system files and very long names)
	// returns TRUE if the file was successfully removed
	// const CString& strName = the full filename

	const CString strLong = LongPath(strName);

	const DWORD nAttr = ::GetFileAttributes(strLong);

	if (nAttr == INVALID_FILE_ATTRIBUTES || (nAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
		return FALSE;

	::SetFileAttributes(strLong, FILE_ATTRIBUTE_NORMAL);

	return ::DeleteFile(strLong);
}


BOOL CMisc::RemoveAFolder(const CString& strName)
{
	// removes a folder recursively (all files, also read only/hidden/system ones and very long names)
	// returns TRUE if the folder was successfully removed
	// const CString& strName = the full foldername

	CString str = strName;

	while (str.GetLength() > 3 && str.Right(1) == _T("\\"))
		str = str.Left(str.GetLength() - 1);

	const CString strLong = LongPath(str);

	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(strLong + _T("\\*"), &fd);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			const CString strItem = fd.cFileName;

			if (strItem == _T(".") || strItem == _T(".."))
				continue;

			const CString strFull = str + _T("\\") + strItem;

			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
				{
					// junction/symlink: remove the link itself, never follow it
					::SetFileAttributes(LongPath(strFull), FILE_ATTRIBUTE_NORMAL);
					::RemoveDirectory(LongPath(strFull));
				}
				else
					RemoveAFolder(strFull);
			}
			else
				RemoveAFile(strFull);
		}
		while (::FindNextFile(hFind, &fd));

		::FindClose(hFind);
	}

	::SetFileAttributes(strLong, FILE_ATTRIBUTE_NORMAL);

	return ::RemoveDirectory(strLong);
}


unsigned long CMisc::CountFilesInFolder(const CString& strName, std::vector<CString> *pvecFiles, const BOOL bSubfolders, const BOOL bInit)
{
	// count files in folders
	// returns the number of found files
	// const CString& strName     = the foldername which you like to scan
	// std::vector<CString> *pvecFiles  = pointer to a vector which stores the found filenames
	// const BOOL bSubfolders     = flag to scan recursively
	// const BOOL bInit         = set to TRUE

    static unsigned long nCount = 0;

    nCount = bInit ? 0 : nCount;

	if (bInit && pvecFiles != NULL)
		pvecFiles->clear();

    CFileFind findFile;

    BOOL bMore = findFile.FindFile(strName + _T("*.*"));

    while (bMore)
    {
        bMore = findFile.FindNextFile();

        if (! findFile.IsDots() && ! findFile.IsSystem())
        {
            if (findFile.IsDirectory())
            {
				if (bSubfolders)
					CountFilesInFolder(findFile.GetFilePath() + _T("\\"), pvecFiles, bSubfolders, FALSE);
            }
            else
                nCount++;
				if (pvecFiles != NULL)
					pvecFiles->push_back(findFile.GetFilePath());
        }
    }

    findFile.Close();

    return nCount;
}


BOOL CMisc::SetFileWriteable(const CString& strFileName, CFileStatus *pStatus)
{
	// set an existing file writeable
	// returns TRUE for success
	// const CString& strName = name of the file
	// CFileStatus *pStatus   = pointer to a status object which can be used afterwards

	if (! CFile::GetStatus(strFileName, *pStatus))
		return FALSE;

	if ((pStatus->m_attribute & CFile::readOnly) == CFile::readOnly)
	{
		pStatus->m_attribute ^= CFile::readOnly;

		try
		{
			CFile::SetStatus(strFileName, *pStatus);
		}
		catch (CFileException *e)
		{
			e->Delete();
			return FALSE;
		}
	}

	return TRUE;
}


BOOL CMisc::IsFile(const CString& strName)
{
	// checks if a file is a file or not :O)
	// returns TRUE if it is file (not a folder)
	// const CString& strName = full name of file/folder

    BOOL bReturn = FALSE;

    CFileFind ff;

    if (ff.FindFile(strName))
    {
		ff.FindNextFile();

		bReturn = (! ff.IsDots() && ! ff.IsDirectory() && ! ff.IsSystem());
        ff.Close();
    }

    return bReturn;
}


BOOL CMisc::IsFolder(const CString& strName)
{
	// checks if a file is a folder or not
	// returns TRUE if it is folder
	// const CString& strName = full name of file/folder

    BOOL bReturn = FALSE;

    CFileFind ff;

    if (ff.FindFile(strName + ((strName.Right(1) == _T('\\')) ? _T("*.*") : _T("\\*.*"))))
    {
        ff.Close();
        bReturn = TRUE;
    }

    return bReturn;
}


DWORD CMisc::RunCmd(const CString& strCommand, const int nPriority, const BOOL bShowWindow, PROCESS_INFORMATION *pPI)
{
	// run a command without showing a window
	// returns the process returnvalue
	// const CString& strCommand = full command
	// const int nPriority = priority of the thread which will be created

	DWORD nProcessPriority = NORMAL_PRIORITY_CLASS;

	switch (nPriority)
	{
		case THREAD_PRIORITY_TIME_CRITICAL:
			nProcessPriority = REALTIME_PRIORITY_CLASS;
			break;

		case THREAD_PRIORITY_HIGHEST:
			nProcessPriority = HIGH_PRIORITY_CLASS;
			break;

		case THREAD_PRIORITY_IDLE:
			nProcessPriority = IDLE_PRIORITY_CLASS;
	}


	DWORD n = 1;

	_flushall();
	ZeroMemory(pPI, sizeof(PROCESS_INFORMATION));
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = bShowWindow ? SW_NORMAL : SW_HIDE;
	CString strWritableCommand = strCommand;
	if (CreateProcess(NULL, strWritableCommand.GetBuffer(), NULL, NULL, FALSE, nProcessPriority, NULL, NULL, &si, pPI))
	{
		SetThreadPriority(pPI->hThread, nPriority);
		WaitForSingleObject(pPI->hProcess, INFINITE);
		GetExitCodeProcess(pPI->hProcess, &n);
		CloseHandle(pPI->hProcess);
		CloseHandle(pPI->hThread);
	}
	strWritableCommand.ReleaseBuffer();
	ZeroMemory(pPI, sizeof(PROCESS_INFORMATION));
	_flushall();

	return n;
}


DWORD CMisc::nNumIniLines;
BOOL CMisc::bIniFileRead = FALSE;
CString CMisc::strIniFile[MAXNUMINILINES+1];

BOOL CMisc::ReadTextFile(const CString& strFileName, CString *pstrText)
{
	// reads a whole text file as Unicode text. Understands UTF-16 (BOM), UTF-8 (with or without BOM)
	// and, if the content is not valid UTF-8, the ANSI code page of the system.
	// returns FALSE if the file can't be read

	pstrText->Empty();

	CFile file;

	if (! file.Open(strFileName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
		return FALSE;

	std::string strData;

	try
	{
		const ULONGLONG nLen = file.GetLength();

		if (nLen > 16 * 1024 * 1024)
			return FALSE;

		strData.resize((size_t)nLen);

		if (nLen > 0 && file.Read(&strData[0], (UINT)nLen) != (UINT)nLen)
			return FALSE;
	}
	catch (CFileException *e)
	{
		e->Delete();
		return FALSE;
	}

	file.Close();

	if (strData.empty())
		return TRUE;

	// UTF-16 little endian with BOM

	if (strData.size() >= 2 && (unsigned char)strData[0] == 0xFF && (unsigned char)strData[1] == 0xFE)
	{
		*pstrText = CString((const wchar_t *)(strData.data() + 2), (int)((strData.size() - 2) / 2));
		return TRUE;
	}

	// UTF-8 (skip the BOM)

	size_t nStart = 0;

	if (strData.size() >= 3 && (unsigned char)strData[0] == 0xEF && (unsigned char)strData[1] == 0xBB && (unsigned char)strData[2] == 0xBF)
		nStart = 3;

	const char *pData = strData.data() + nStart;
	const int nData = (int)(strData.size() - nStart);

	if (nData == 0)
		return TRUE;

	UINT nCodePage = CP_UTF8;
	int nWide = ::MultiByteToWideChar(nCodePage, MB_ERR_INVALID_CHARS, pData, nData, NULL, 0);

	if (nWide <= 0)
	{
		nCodePage = CP_ACP;
		nWide = ::MultiByteToWideChar(nCodePage, 0, pData, nData, NULL, 0);
	}

	if (nWide <= 0)
		return FALSE;

	::MultiByteToWideChar(nCodePage, 0, pData, nData, pstrText->GetBuffer(nWide), nWide);
	pstrText->ReleaseBuffer(nWide);

	return TRUE;
}


void CMisc::ReadIniFile(const CString& strFileName, int nLineLimit)
{
	CString strText;

	if (ReadTextFile(strFileName, &strText))
	{
		std::vector<CString> vecLines;

		int nPos = 0;
		CString strLine = strText.Tokenize(_T("\n"), nPos);

		while (nPos != -1 || ! strLine.IsEmpty())
		{
			vecLines.push_back(strLine);

			if (nPos == -1)
				break;

			strLine = strText.Tokenize(_T("\n"), nPos);
		}

		// (Tokenize skips empty lines, which is fine: empty lines are not stored anyway)

		nNumIniLines = 0;
		BOOL bLimit = TRUE;
		if ((nLineLimit == 0) || (nLineLimit > MAXNUMINILINES))
			nLineLimit = MAXNUMINILINES;

		size_t nLinePos = 0;

		while ((GetNextLine(vecLines, &nLinePos, &strIniFile[nNumIniLines]) == 1) && bLimit)
		{
			strIniFile[nNumIniLines].MakeLower();
			strIniFile[nNumIniLines].Trim();

			if (strIniFile[nNumIniLines] != _T("")) nNumIniLines++; // No point in storing empty lines.
			bLimit = (nLineLimit == 0) ? TRUE : (int)nNumIniLines < nLineLimit;
		}
	}

	bIniFileRead = TRUE;

	return;
}


CString CMisc::GetKeywordInFile(const CString& strFileName, CString strSection, CString strKeyword, BOOL *pSectionExists, const int nLineLimit)
{
	// Get the value of a given keyword in a given file after a given section [SECTION]
	// returns the found value
	// const CString& strFileName = filename of the "ini" file
	// CString strSection     = name of the section (e.g "[paths]")
	// CString strKeyword     = name of the keyword (e.g. "rompath =")
	// BOOL *pSectionExists     = pointer to a flag which is set to TRUE if the section was found
	// const int nLineLimit     = max number of lines to scan or 0 for all lines

  if (!bIniFileRead)
	  ReadIniFile(strFileName, nLineLimit);

	DWORD nLines;

	CString strReturn;
	CString strValue;

	BOOL bFound = FALSE;

	if (pSectionExists != NULL)
		*pSectionExists = FALSE;

	strKeyword.MakeLower();
	strSection.MakeLower();

	for (nLines = 0; (nLines < nNumIniLines) && (strIniFile[nLines].Find(strSection) != 0); nLines++);
	bFound = nLines < nNumIniLines;

	if (bFound)
	{
		if (pSectionExists != NULL)
			*pSectionExists = TRUE;

	    nLines++;
		bFound = FALSE;

		while (! bFound && (nLines < nNumIniLines))
		{
			CString CurrentWordLo = strIniFile[nLines];

			if (CurrentWordLo.Find(_T('[')) == 0)
				break;

			if (CurrentWordLo.Find(strKeyword) == 0)
			{
				// test if it is really a word separated by whitespaces...
				const CString t1 = CurrentWordLo.Mid(strKeyword.GetLength()) + _T(" ");	// + " " for endofline check
				CString t2 = t1;
				t2.TrimLeft();
				if (t1 != t2)
				{
					strValue = strIniFile[nLines];
					bFound = TRUE;
				}
			}
			
			nLines++;
		}
	}


	if (bFound)
	{
		const int nIndex = strValue.Find(_T('='));
		if (nIndex != -1)
		{
			strReturn = strValue.Right(strValue.GetLength() - nIndex - 1);
			strReturn.TrimLeft();
		}
	}

	return strReturn;
}


int CMisc::GetNextLine(const std::vector<CString>& vecLines, size_t *pnPos, CString *pstrLine, const BOOL bTestComment)
{
	// gets the next line from an open file and skips comment lines
	// returns -1 when an error happens, 0 when the file has ended, 1 for success
	// CStdioFile *pstdioFile = pointer to an open CStdioFile (opened in textmode)
	// CString *pstrLine    = pointer to a string which holds the read line
	// const BOOL bTestComment  = set to TRUE if you want to skip characters after commentslashes (//)

	CString strLine;
	BOOL bLineOk = FALSE;

	while (! bLineOk)
	{
		if (*pnPos >= vecLines.size())
			return 0;

		strLine = vecLines[(*pnPos)++];
		strLine.TrimRight(_T("\r"));

		CString strRest = strLine;
		int nComment = strRest.Find(_T("//"));

		while (nComment != -1  && bTestComment)
		{
			BOOL bCommentInQuotes = FALSE;

			int n = 0;
			int nQuotes = -1;
			while (n < nComment)
			{
				if (n == 0)
					nQuotes += (strRest[n] == _T('\"')) ? 1 : 0;
				else
					nQuotes += (strRest[n] == _T('\"') && strRest[n - 1] != _T('\\')) ? 1 : 0;

				n++;
			}

			bCommentInQuotes = (nQuotes + 1 % 2 == 1);

			strRest = strRest.Mid(nComment);

			if (! bCommentInQuotes)
			{
				strLine = strLine.Left(strLine.GetLength() - strRest.GetLength());
				break;
			}

			BOOL bRealQuote = FALSE;

			while (! bRealQuote)
			{
				const int nNextQuote = strRest.Find(_T('\"'));

				bRealQuote = (nNextQuote < 1) ? TRUE : (strRest[nNextQuote - 1] != _T('\\'));

				if (nNextQuote != -1)
					strRest = strRest.Mid(nNextQuote + 1);
			}

			nComment = strRest.Find(_T("//"));
		}

		strLine.Trim();

		*pstrLine = strLine;

		bLineOk = TRUE;
	}

	return 1;
}


BOOL CMisc::IsFolderEmpty(const CString& strFolder)
{
	// check if a given folder is empty or not
	// returns TRUE if the folder exists and is empty, otherwise FALSE
	// const CString& strFolder  = full name of the folder with or without ending "\"

	CString str = strFolder;

	while (str.GetLength() > 3 && str.Right(1) == _T("\\"))
		str = str.Left(str.GetLength() - 1);

	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(LongPath(str) + _T("\\*"), &fd);

	// folder doesn't exists ?

	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;


	// check for at least one entry

	BOOL bFoundNone = TRUE;

	do
	{
		const CString strItem = fd.cFileName;

		if (strItem != _T(".") && strItem != _T(".."))
		{
			bFoundNone = FALSE;
			break;
		}
	}
	while (::FindNextFile(hFind, &fd));

	::FindClose(hFind);

	return bFoundNone;
}


CTimeSpan CMisc::GetTimeSpan(const BOOL bInit)
{
	// returns the elapsed time since the first GetTimeSpan call
	// const BOOL bInit = TRUE for "StartTimer", FALSE for "StopTimer"

	static CTime timeStart;

	timeStart = bInit ? CTime::GetCurrentTime() : timeStart;

	CTime timeStop = CTime::GetCurrentTime();

	CTimeSpan timeElapsed = timeStop - timeStart;

	return timeElapsed;
}


BOOL CMisc::GetFileSize(const CString& strFileName, UINT64 *pnSize)
{
	// get the size of an existing file
	// returns TRUE for success, FALSE otherwise, pnSize is filled with the filelength value
	// const CString& strFileName = full filename

	*pnSize = (UINT64) 0;

	HANDLE hFile = ::CreateFile(strFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE || hFile == NULL)
		return FALSE;

	DWORD highSize = 0;
	const DWORD lowSize = ::GetFileSize(hFile, &highSize);
	if (lowSize == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
	{
		::CloseHandle(hFile);
		return FALSE;
	}

	::CloseHandle(hFile);

	*pnSize = (UINT64)(lowSize | ((UINT64)highSize << 32));

	return TRUE;
}


CString CMisc::NumericToString(const int nValue)
{
	// convert a numeric value to a string with correct "thousand"-point
	// returns the converted string
	// const int nValue = the value to convert

	CString strReturn;
	strReturn.Format(_T("%d"), nValue);

	TCHAR c[16];
	if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, c, 16) != 0)
	{
		CString s;
		const CString strPoint = CString(c);
		int n = strReturn.GetLength();
		int nCount = 0;

		while (n > 0)
		{
			s = strReturn[--n] + s;
			if (nCount++ == 2 && n > 0)
			{
				s = strPoint + s;
				nCount = 0;
			}
		}

		strReturn = s;
	}

	return strReturn;
}


BOOL CMisc::FileExists(const CString& strFileName)
{
	// search several common paths and check if the given file is available
	// returns TRUE, if the file was found
	// const CString& strFileName = the file to look for

	BOOL bFound = FALSE;


	// search in current folder

	bFound |= IsFile(strFileName);


	// search in system/windows folder

	TCHAR cBuffer[MAX_PATH];

	GetSystemDirectory(cBuffer, MAX_PATH);
	bFound |= IsFile(CString(cBuffer) + _T('\\') + strFileName);

	GetWindowsDirectory(cBuffer, MAX_PATH);
	bFound |= IsFile(CString(cBuffer) + _T('\\') + strFileName);


	// search in %PATH%

	const TCHAR *pathvar = _tgetenv(_T("PATH"));

	if (pathvar != NULL)
	{
		const CString strPath = CString(pathvar) + _T(";");	// + ";" to get the last entry :)

		int n = 0;
		int nStart = 0;
		while (n < strPath.GetLength())
		{
			if (strPath[n] == _T(';'))
			{
				CString s = strPath.Mid(nStart, n - nStart);
				nStart = n + 1;

				// trim the found entry and add a backslash if needed
				s.Trim();
				if (s != _T(""))
				{
					if (s.Right(1) != _T("\\"))
						s += _T("\\");

					bFound |= IsFile(s + strFileName);
				}
			}

			n++;
		}
	}


	return bFound;
}


CString CMisc::WideToCString(const std::wstring& strWide)
{
	// (zipentry names are decoded to Unicode by the engine; kept as a function for the callers)

	return CString(strWide.c_str());
}


void CMisc::ListFilesRelative(const CString& strFolder, std::vector<CString> *pvecFiles)
{
	// recursively lists all files below strFolder (files only, no folders)
	// the names are relative to strFolder
	// strFolder has to end with "\\"

	CFileFind findFile;

	BOOL bMore = findFile.FindFile(strFolder + _T("*.*"));

	while (bMore)
	{
		bMore = findFile.FindNextFile();

		if (findFile.IsDots())
			continue;

		const CString strFound = findFile.GetFilePath();

		if (findFile.IsDirectory())
		{
			std::vector<CString> vecSub;
			ListFilesRelative(strFound + _T("\\"), &vecSub);

			const CString strPrefix = strFound.Mid(strFolder.GetLength()) + _T("\\");

			for (size_t i = 0; i < vecSub.size(); i++)
				pvecFiles->push_back(strPrefix + vecSub[i]);
		}
		else
			pvecFiles->push_back(findFile.GetFileName());
	}

	findFile.Close();
}


CString CMisc::NumericToString64(const UINT64 nValue)
{
	// like NumericToString(), but for 64 bit values (file sizes)

	CString strReturn;
	strReturn.Format(_T("%I64u"), nValue);

	TCHAR c[16];
	if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, c, 16) != 0)
	{
		CString s;
		const CString strPoint = CString(c);
		int n = strReturn.GetLength();
		int nCount = 0;

		while (n > 0)
		{
			s = strReturn[--n] + s;
			if (nCount++ == 2 && n > 0)
			{
				s = strPoint + s;
				nCount = 0;
			}
		}

		strReturn = s;
	}

	return strReturn;
}
