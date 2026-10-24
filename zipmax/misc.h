#pragma once

#include <vector>
#include <string>

#define MAXNUMINILINES 32768

class CMisc
{
	public:

		// some useful functions...

		static BOOL FileExists(const CString& strFileName);
		static BOOL RemoveAFile(const CString& strName);
		static BOOL RemoveAFolder(const CString& strName);
		static BOOL SetFileWriteable(const CString& strName, CFileStatus *pStatus);

		static CString GetFullFileNameNoDrive(const CString& str);
		static CString GetPureFileName(const CString& strPath);
		static CString GetPurePathName(const CString& strFullPathName);

		static unsigned long CountFilesInFolder(const CString& strName, std::vector<CString> *pvecFiles, const BOOL bSubfolders = TRUE, const BOOL bInit = TRUE);
		static void GetAllCmdLineZip(const CString& strCmdLine, std::vector<CString> *pvecCmdLineFiles, const BOOL bSubFolders);
		static void GetAllDroppedZip(HDROP hDropInfo, std::vector<CString> *pvecDroppedFiles, const BOOL bSubFolders);
		static void ScanDroppedSubFolderZip(const CString& strFolderName, std::vector<CString> *pvecDroppedFiles);
		static void ScanFolderZip(const CString& strFolderName, const BOOL bSubfolders, std::vector<CString> *pvecFiles);
		static void AddEntry(const CString& strEntry, std::vector<CString> *pvecCmdLineFiles, const BOOL bSubFolders);
		static CString TrimEntry(const CString& strEntry);

		static BOOL IsFile(const CString& strName);
		static BOOL IsFolder(const CString& strName);
		static BOOL IsFolderEmpty(const CString& strFolder);
		static BOOL GetFileSize(const CString& strFileName, UINT64 *pnSize);
		static CTimeSpan GetTimeSpan(const BOOL bInit = TRUE);
		static BOOL SetFileToCurrentTime(const CString& strFileName);

		static DWORD RunCmd(const CString& strCommand, const int nPriority, const BOOL bShowWindow, PROCESS_INFORMATION *pPI);

		static CString GetKeywordInFile(const CString& strFileName, CString strSection, CString strKeyword, BOOL *pSectionExists = NULL, const int nLineLimit = 0);
		static int GetNextLine(const std::vector<CString>& vecLines, size_t *pnPos, CString *pstrLine, const BOOL bTestComment = TRUE);
		static BOOL ReadTextFile(const CString& strFileName, CString *pstrText);

		static CString NumericToString(const int nValue);
		static CString NumericToString64(const UINT64 nValue);

		static CString WideToCString(const std::wstring& strWide);
		static void ListFilesRelative(const CString& strFolder, std::vector<CString> *pvecFiles);

	    static void ReadIniFile(const CString& strFileName, int nLineLimit);
	    static CString strIniFile[MAXNUMINILINES+1];
	    static DWORD nNumIniLines;
	    static BOOL bIniFileRead;
};
