#include "stdafx.h"
#include "misc.h"
#include "Language.h"
#include "ZipMax.h"
#include "ZipMaxDlg.h"
#include "Zip7Engine.h"


// CZipMaxApp

BEGIN_MESSAGE_MAP(CZipMaxApp, CWinApp)
END_MESSAGE_MAP()


// CZipMaxApp construction

CZipMaxApp::CZipMaxApp()
{
	// zipmax folder

	g_strMainFolder = _T("");


	// default window/column sizes/widths

	g_rectOutput = CRect(50, 50, 550, 270);
	g_nWidth0 = 280;		// File
	g_nWidth1 = 78;			// Status
	g_nWidth2 = 95;			// Size Before
	g_nWidth3 = 95;			// Size After
	g_nWidth4 = 300;		// Type | Compression | Dictionary | Method
	g_nWidth5 = 130;		// Gained Free Bytes
	g_strLanguage = _T("");


	// default zipmax.ini values

	for (int x = 0; x < MAXEXTERNAL; x++)
	{
		g_strPackerExe[x] = _T("");
		g_strCommandLine[x] = _T("");
	}

	g_strFinalExe = _T("");
	g_strFinalCommandLine = _T("");

	g_bNoBeep = FALSE;
	g_bSetCurrentTime = FALSE;

	g_bSubfolders = FALSE;
	g_bSkipIntern = FALSE;
	g_strPriority = _T("NORMAL");
	g_bThreadRunning = FALSE;
	g_bStop = FALSE;
	g_bLogFile = FALSE;
	g_bShowPackerWindow = FALSE;
	g_bSkipReadOnly = FALSE;
	g_bSkipArchive = FALSE; 
	g_bSetCurrentReadOnly = FALSE;

	ZeroMemory(&g_pi, sizeof(PROCESS_INFORMATION));
	g_bPause = FALSE;
	g_strOldStatus1 = _T("");
}


// The one and only CZipMaxApp object

CZipMaxApp theApp;


// CZipMaxApp initialization

BOOL CZipMaxApp::LoadSettings()
{
	// reads zipmax.ini into the g_ variables and checks everything ZipMax needs.
	// returns TRUE if ZipMax can work with the settings, otherwise FALSE (after showing a message)

	CMisc::bIniFileRead = FALSE;		// forget the cached ini, read the file again

	// get values from zipmax.ini

	CString strIni,strNum;
	strIni = CLang::Get(IDS_INIFILE);
	strIni = g_strMainFolder + strIni;

	BOOL bKeyword1[MAXEXTERNAL], bKeyword2[MAXEXTERNAL];

	int nExtCount = 0;

	DWORD x = 0;
	while (nExtCount < MAXEXTERNAL)
	{
		bKeyword1[x] = bKeyword2[x] = FALSE;

		strNum.Format(_T("-%u"),nExtCount + 1);
		g_strPackerExe[x] = CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("PACKER-EXE") + strNum, &bKeyword1[x]);
		g_strCommandLine[x] = CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("PACKER-CMD") + strNum, &bKeyword2[x]);


		// if only PACKER-EXE is used (instead of PACKER-EXE-1), use it anyway
		if (x == 0 && g_strPackerExe[x] == _T(""))
		{
			g_strPackerExe[x] = CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("PACKER-EXE"), &bKeyword1[x]);
			g_strCommandLine[x] = CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("PACKER-CMD"), &bKeyword2[x]);
		}

		if (g_strPackerExe[x] != _T(""))
			x++;

		nExtCount++;
	}

	g_dwNumExtern = x;

	g_strFinalExe = CMisc::GetKeywordInFile(strIni,_T("[ZIPMAX]"),_T("FINALSTEP-EXE"));
	g_strFinalCommandLine = CMisc::GetKeywordInFile(strIni,_T("[ZIPMAX]"),_T("FINALSTEP-CMD"));

	g_strPriority = CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("THREAD-PRIORITY"));
	g_strSevenZipDll = CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("SEVENZIP-DLL"));
	g_bSubfolders = (CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("WORK-RECURSIVE")) == _T("1"));
	g_bSkipIntern = (CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("SKIP-INTERNZIP")) == _T("1"));
	g_bLogFile = (CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("LOGFILE")) == _T("1"));

	g_bNoBeep = (CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("NOBEEP")) == _T("1"));
	g_bSetCurrentTime = (CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("SETCURRENTTIME")) == _T("1"));

	g_bShowPackerWindow = (CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("SHOWPACKERWINDOW")) == _T("1"));

	g_bSkipReadOnly = (CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("SKIPREADONLY")) == _T("1"));
	g_bSkipArchive = (CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("SKIPARCHIVE")) == _T("1")); 
	g_bSetCurrentReadOnly = (CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("SETCURRENTREADONLY")) == _T("1"));


	// check if the ini is valid

	CString q;
	q = CLang::Get(IDS_INVALIDINI);
	BOOL bOk = TRUE;

	if ((g_dwNumExtern < 1))
		bOk = FALSE;
	else
	{
		for (x = 0; x < g_dwNumExtern; x++)
		{
			if (! bKeyword1[x] || ! bKeyword2[x])
				break;	// no packer/cmd line entries

			if (g_strPackerExe[x] == _T("") || g_strCommandLine[x] == _T(""))
				break;	// empty entries

			if (g_strCommandLine[x].Find(_T("%1")) == -1 || g_strCommandLine[x].Find(_T("%2")) == -1)
				break;	// no arguments
		}
	}

	if (x < g_dwNumExtern || ! bOk)
	{
		AfxMessageBox(q + strIni, MB_ICONWARNING | MB_OK);
		return FALSE;
	}

	{
		BOOL bFoundPacker = TRUE;
	    CString strLastPacker = _T("");

		for (DWORD i = 0; i < g_dwNumExtern; i++)
		{
			if (strLastPacker.CompareNoCase(g_strPackerExe[i]))
			{
				// Not the same, check it if exists.
				if (! CMisc::FileExists(g_strPackerExe[i]))
				{
					bFoundPacker = FALSE;
					q = CLang::Get(IDS_CANTDETECT);
					AfxMessageBox(q + g_strPackerExe[i], MB_ICONWARNING | MB_OK);
				}
				strLastPacker = g_strPackerExe[i];
			}
		}

		if ((g_strFinalExe != _T("")) && (!CMisc::FileExists(g_strFinalExe)))
			bFoundPacker = FALSE;

		return bFoundPacker;
	}
}


void CZipMaxApp::ReloadSettings()
{
	// called after Settings > Options wrote zipmax.ini (nothing is zipping at that moment)

	const CString strOldDll = g_strSevenZipDll;

	LoadSettings();

	// language (the program texts are read again; the window refreshes them in CZipMaxDlg::RefreshLanguage)

	CString strIni = CLang::Get(IDS_INIFILE);
	strIni = g_strMainFolder + strIni;
	g_strLanguage = CMisc::GetKeywordInFile(strIni, _T("[ZIPMAX]"), _T("LANGUAGE"));
	CLang::Init(g_strMainFolder, g_strLanguage);

	// another 7z.dll ?

	if (g_strSevenZipDll.CompareNoCase(strOldDll) != 0)
	{
		std::wstring strEngineError;

		Zip7::Shutdown();

		if (! Zip7::Initialize((LPCTSTR)g_strSevenZipDll, &strEngineError))
			AfxMessageBox(CLang::Get(IDS_7ZIPFAIL) + strEngineError.c_str(), MB_ICONWARNING | MB_OK);
	}
}


BOOL CZipMaxApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();


	// get foldername of zipmax (and convert it to 8.3)
	// and switch to zipmax directory

	TCHAR szHomePath[_MAX_PATH];
	::GetModuleFileName(::AfxGetInstanceHandle(), szHomePath, _MAX_PATH);
	CString strPath(szHomePath);
	strPath = CMisc::GetPurePathName(strPath);
	TCHAR szBuffer[_MAX_PATH];
	::GetShortPathName(strPath, szBuffer, _MAX_PATH - 1);
	g_strMainFolder = szBuffer;
	if (g_strMainFolder.Right(1) != _T("\\"))
		g_strMainFolder += _T('\\');

	SetCurrentDirectory((LPCTSTR)g_strMainFolder);



	// language (language\*.lng, chosen in zipmax.ini). Done first, so that even the very first
	// message box can already be translated.

	{
		CString strEarlyIni = CLang::Get(IDS_INIFILE);
		strEarlyIni = g_strMainFolder + strEarlyIni;
		g_strLanguage = CMisc::GetKeywordInFile(strEarlyIni, _T("[ZIPMAX]"), _T("LANGUAGE"));
		CLang::Init(g_strMainFolder, g_strLanguage);
	}


	// check if zipmax is already running

	CString strTitle;
	strTitle = CLang::Get(IDS_STRING_TITLE);

	if (::FindWindow(NULL, strTitle) != NULL)
	{
		CString s;
		s = CLang::Get(IDS_ALREADYRUNNING);
		AfxMessageBox(s, MB_ICONWARNING | MB_OK);
		return FALSE;
	}

	// redefine application name (to get a real registry entry name)

	::free((void*)m_pszAppName);
	m_pszAppName=::_tcsdup(_T("ZipMax"));


	// get stored registry values (get default values from constructor)

	SetRegistryKey(_T(""));
	g_rectOutput.left = GetProfileInt(_T("Windows"), _T("Left"), g_rectOutput.left);
	g_rectOutput.top = GetProfileInt(_T("Windows"), _T("Top"), g_rectOutput.top);
	g_rectOutput.right = GetProfileInt(_T("Windows"), _T("Right"), g_rectOutput.right);
	g_rectOutput.bottom = GetProfileInt(_T("Windows"), _T("Bottom"), g_rectOutput.bottom);
	g_nWidth0 = GetProfileInt(_T("Windows"), _T("Width0"), g_nWidth0);
	g_nWidth1 = GetProfileInt(_T("Windows"), _T("Width1"), g_nWidth1);
	g_nWidth2 = GetProfileInt(_T("Windows"), _T("Width2"), g_nWidth2);
	g_nWidth3 = GetProfileInt(_T("Windows"), _T("Width3"), g_nWidth3);
	g_nWidth4 = GetProfileInt(_T("Windows"), _T("Width4"), g_nWidth4);
	g_nWidth5 = GetProfileInt(_T("Windows"), _T("Width5"), g_nWidth5);
	g_view.Load(this);


	// get values from zipmax.ini and check them

	if (LoadSettings())
	{
		CString q;
		BOOL bFoundPacker = TRUE;

		// start the 7-Zip engine (7z.dll or the statically linked code)

		if (bFoundPacker)
		{
			std::wstring strEngineError;

			if (! Zip7::Initialize((LPCTSTR)g_strSevenZipDll, &strEngineError))
			{
				q = CLang::Get(IDS_7ZIPFAIL);
				AfxMessageBox(q + strEngineError.c_str(), MB_ICONWARNING | MB_OK);
				bFoundPacker = FALSE;
			}
		}

		if (bFoundPacker)
		{
			// open the main dialog....here we go...

			CZipMaxDlg dlg;
			m_pMainWnd = &dlg;
			dlg.DoModal();
		}
	}


	Zip7::Shutdown();


	// write registry entries

	WriteProfileInt(_T("Windows"), _T("Left"), g_rectOutput.left);
	WriteProfileInt(_T("Windows"), _T("Top"), g_rectOutput.top);
	WriteProfileInt(_T("Windows"), _T("Right"), g_rectOutput.right);
	WriteProfileInt(_T("Windows"), _T("Bottom"), g_rectOutput.bottom);
	WriteProfileInt(_T("Windows"), _T("Width0"), g_nWidth0);
	WriteProfileInt(_T("Windows"), _T("Width1"), g_nWidth1);
	WriteProfileInt(_T("Windows"), _T("Width2"), g_nWidth2);
	WriteProfileInt(_T("Windows"), _T("Width3"), g_nWidth3);
	WriteProfileInt(_T("Windows"), _T("Width4"), g_nWidth4);
	WriteProfileInt(_T("Windows"), _T("Width5"), g_nWidth5);
	g_view.Save(this);

	return FALSE;
}