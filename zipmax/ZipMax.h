// ZipMax.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "ViewConfig.h"


#define MAXEXTERNAL 999

class CZipMaxApp : public CWinApp
{
	public:

		CZipMaxApp();

	public:

		// "global" variables e.g. ini entries/options

		CString g_strMainFolder;

		CString g_strPackerExe[MAXEXTERNAL];
		CString g_strCommandLine[MAXEXTERNAL];
		CString g_strFinalExe;
	    CString g_strFinalCommandLine;
		CString g_strPriority;
		CString g_strLanguage;			// language file name (zipmax.ini: language), see CLang
		CString g_strSevenZipDll;		// optional path of 7z.dll (zipmax.ini: sevenzip-dll)

		DWORD g_dwNumExtern;

		BOOL g_bSubfolders;
		BOOL g_bSkipIntern;
		BOOL g_bThreadRunning;
		BOOL g_bStop;
		BOOL g_bLogFile;
		BOOL g_bNoBeep;
		BOOL g_bSetCurrentTime;
		BOOL g_bShowPackerWindow;
		BOOL g_bSkipReadOnly;		// Skips files with Readonly attribute set
		BOOL g_bSkipArchive;		// Skips files with Archive attribute set
		BOOL g_bSetCurrentReadOnly;	// Marks files ReadOnly while processing, resets when done


		// "global" variables to remember gui positions

		CViewConfig g_view;			// which columns/items the window shows, and in which order (Settings > Dialog Box)

		CRect g_rectOutput;
		int g_nWidth0;
		int g_nWidth1;
		int g_nWidth2;
		int g_nWidth3;
		int g_nWidth4;
		int g_nWidth5;

		PROCESS_INFORMATION g_pi;
		BOOL g_bPause;
		CString g_strOldStatus1;


	public:

		virtual BOOL InitInstance();

		// reads zipmax.ini into the g_ variables above and checks it. FALSE (after a message) if ZipMax can't work with it.
		BOOL LoadSettings();

		// called after Settings > Options saved zipmax.ini: reloads it and applies language and 7z.dll changes
		void ReloadSettings();

		DECLARE_MESSAGE_MAP()
};

extern CZipMaxApp theApp;
