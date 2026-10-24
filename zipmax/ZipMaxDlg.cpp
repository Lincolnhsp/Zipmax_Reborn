// ZipMaxDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ZipMax.h"
#include "ZipMaxDlg.h"
#include "misc.h"
#include "Language.h"
#include "OptionsDlg.h"
#include "ViewSettingsDlg.h"
#include "AboutDlg.h"
#include "IniFile.h"
#include "zipstructs.h"
#include <algorithm>
#include <map>
#include <new>
#include <string>

#define DATABUFFERSIZE 8*1024*1024	// buffersize for WriteLFH(...) 8 MB


//////////////////////////////////////////////////////////////////////////


CString CZipMaxDlg::s_strLastZipError;


void CZipMaxDlg::SetLastZipError(const std::wstring& str)
{
	s_strLastZipError = str.c_str();
}


CZipEntry::CZipEntry()
{
	// standard init...

	m_pHeader = NULL;
	m_strEntryName = _T("");
	m_strArchiveName = _T("");
}


bool CZipEntry::operator < (const CZipEntry& entry) const
{
	// operator used for stable_sort....
	// sort m_strEntryName

	return (m_strEntryName.CompareNoCase(entry.m_strEntryName) < 0);
}


//////////////////////////////////////////////////////////////////////////



// CZipMaxDlg dialog



CZipMaxDlg::CZipMaxDlg(CWnd* pParent /*=NULL*/)	: CDialog(CZipMaxDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// initialize the members....

	ZeroMemory(&m_param, sizeof (THREADPARAM));

	m_vecFiles.clear();
	m_pListCtrl = NULL;
	m_pStatus1 = NULL;
	m_pStatus2 = NULL;
	m_pThread = NULL;
	m_pProgCtrl = NULL;
	m_pButtonPause = NULL;
}

void CZipMaxDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	return;
}

BEGIN_MESSAGE_MAP(CZipMaxDlg, CDialog)
	//{{AFX_MSG_MAP(CZipMaxDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DROPFILES()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST, OnColumnClick)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, OnBnClickedButtonPause)
	ON_WM_INITMENUPOPUP()
	ON_COMMAND(IDM_FILE_ADDFILES, OnFileAddFiles)
	ON_COMMAND(IDM_FILE_ADDFOLDER, OnFileAddFolder)
	ON_COMMAND(IDM_FILE_ADDFOLDERSUB, OnFileAddFolderSub)
	ON_COMMAND(IDM_FILE_CLEARLIST, OnFileClearList)
	ON_COMMAND(IDM_FILE_EXIT, OnFileExit)
	ON_COMMAND(IDM_SET_OPTIONS, OnSetOptions)
	ON_COMMAND(IDM_SET_VIEW, OnSetView)
	ON_COMMAND(IDM_HELP_ABOUT, OnHelpAbout)
	ON_COMMAND_RANGE(IDM_LANG_FIRST, IDM_LANG_LAST, OnSetLanguage)
END_MESSAGE_MAP()


// CZipMaxDlg message handlers

BOOL CZipMaxDlg::OnInitDialog()
{
	CString strTitle;
	strTitle = CLang::Get(IDS_STRING_TITLE);
	SetWindowText(strTitle);

	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);


	// get pointer to controls

	m_pProgCtrl = (CProgressCtrl *)GetDlgItem(IDC_PROGRESS);
	m_pListCtrl = (CListCtrl *)GetDlgItem(IDC_LIST);
	m_pStatus1 = (CStatic *)GetDlgItem(IDC_STATUS1);
	m_pStatus2 = (CStatic *)GetDlgItem(IDC_STATUS2);
	m_pButtonPause = (CButton *)GetDlgItem(IDC_BUTTON_PAUSE);


	// set statuslines

	CString q;
	q = CLang::Get(IDS_DROPFILES);
	m_pStatus1->SetWindowText(q);

	UpdateStatus(m_pStatus2);


	// prepare listcontrol, add 6 columns:
	// File | Status | Size Before | Size After | Type/Compression/Dictionary/Method | Gained Free Bytes

	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

	LVCOLUMN column;
	CString str;

	const int nWidths[NUMCOLUMNS] = { pApp->g_nWidth0, pApp->g_nWidth1, pApp->g_nWidth2, pApp->g_nWidth3, pApp->g_nWidth4, pApp->g_nWidth5 };

	for (int nCol = 0; nCol < NUMCOLUMNS; nCol++)
	{
		::ZeroMemory(&column, sizeof(LVCOLUMN));
		column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
		GetColumnInfo(nCol, &column, str);
		column.cx = nWidths[nCol];
		m_pListCtrl->InsertColumn(nCol, &column);
	}


	// green total progress bar (at the bottom of the window)

	m_TotalBar.Create(this, CRect(0, 0, 10, 10), IDC_PROGRESS_TOTAL);
	m_TotalBar.SetPos(0);


	// translated text of the pause button (if the language file has one)

	CString strPause;
	if (CLang::TryGet(IDC_BUTTON_PAUSE, &strPause))
		m_pButtonPause->SetWindowText(strPause);


	m_pListCtrl->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

	m_bmStatus[0].LoadBitmap(IDB_STATUS_GREEN);
	m_bmStatus[1].LoadBitmap(IDB_STATUS_YELLOW);
	m_bmStatus[2].LoadBitmap(IDB_STATUS_RED);
	m_bmArrowUp.LoadBitmap(IDB_ARROW_UP);
	m_bmArrowDown.LoadBitmap(IDB_ARROW_DOWN);
	m_ImageList.Create(14,14,ILC_MASK,5,0);
	m_ImageList.Add(&m_bmStatus[0],RGB(192,192,192));
	m_ImageList.Add(&m_bmStatus[1],RGB(192,192,192));
	m_ImageList.Add(&m_bmStatus[2],RGB(192,192,192));
	m_ImageList.Add(&m_bmArrowUp,RGB(192,192,192));
	m_ImageList.Add(&m_bmArrowDown,RGB(192,192,192));
	m_ImageList.SetBkColor(CLR_NONE);
	m_pListCtrl->SetImageList(&m_ImageList,LVSIL_SMALL);
	m_nLastColumn = -1;

	// menu texts (language) + the columns/items chosen in Settings > Dialog Box

	ApplyMenuLanguage();
	BuildLanguageMenu();
	ApplyView();


	// resize/move window to stored position

	MoveWindow(pApp->g_rectOutput);


	// get command line filenames (vector is cleared in GetAllCmdLineZip())

	CMisc::GetAllCmdLineZip(pApp->m_lpCmdLine, &m_vecFiles, pApp->g_bSubfolders);

	if ((DWORD)m_vecFiles.size() > 0)
		StartZipping ();

	return TRUE;
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CZipMaxDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
		CDialog::OnPaint();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.

HCURSOR CZipMaxDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CZipMaxDlg::OnDropFiles(HDROP hDropInfo)
{
	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();


	// don't accept dropped files when already processing some

	if (pApp->g_bThreadRunning)
	{
		CDialog::OnDropFiles(hDropInfo);
		return;
	}

	ResetSortArrow();

	// get dropped filenames (vector is cleared in GetAllDroppedZip())

	CMisc::GetAllDroppedZip(hDropInfo, &m_vecFiles, pApp->g_bSubfolders);

	CDialog::OnDropFiles(hDropInfo);


	// start zipping

	StartZipping ();

	return;
}


void CZipMaxDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	LayoutControls();

	return;
}


void CZipMaxDlg::OnCancel()
{
	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();
	CListCtrl *pListCtrl = (CListCtrl *)GetDlgItem(IDC_LIST);

	CString p, q, q1, r;
	p = CLang::Get(IDS_STOPQUESTION);
	q = CLang::Get(IDS_QUIT);
	r = CLang::Get(IDS_PAUSEACTIVE);

	if (pApp->g_bPause)
	{
		MessageBox(r, q, MB_ICONWARNING | MB_OK);
		return;
	}

	if (pApp->g_bThreadRunning)
	{

		// pause thread

		if (m_pThread != NULL)
			m_pThread->SuspendThread();

		// show messagebox, ask user if he really wants to quit

		if (MessageBox(p, q, MB_ICONQUESTION | MB_YESNO) == IDYES)
			pApp->g_bStop = TRUE;	// yes...so set the thread stop flag


		// resume thread

		if (m_pThread != NULL)
			m_pThread->ResumeThread();
	}
	else
	{
		// maximize window and store the current
		// position, size, column widths

		ShowWindow(SW_NORMAL);

		SaveColumnState();

		GetWindowRect(&pApp->g_rectOutput);

		// quit

		CDialog::OnCancel();
	}

	return;
}



BOOL CZipMaxDlg::UnzipAllFiles(const CString& strArchiveName, const CString& strDestinationFolder, std::vector<Zip7::ZipHeader> *pvecHeaders)
{
	// unzip files from a archive to a destinationfolder (uses 7-Zip)

	// returns: TRUE if unzip process was successfull, FALSE otherwise

	// const CString& strArchiveName       = full path/filename of the zipfile
	// const CString& strDestinationFolder = full destinationfoldername
	// std::vector<Zip7::ZipHeader> *pvecHeaders = point to a vector which is filled with headerinformation of the archive

	std::wstring strError;

	if (! Zip7::ExtractAll((LPCTSTR)strArchiveName, (LPCTSTR)strDestinationFolder, &strError))
	{
		SetLastZipError(strError);
		return FALSE;
	}

	_flushall();

	// fill in the header information and quit

	return GetHeaders(strArchiveName, pvecHeaders);
}


BOOL CZipMaxDlg::InternalZipFiles(const CString& strArchiveName, const CString& strSourceFolder, std::vector<Zip7::ZipHeader> *pvecHeaders)
{
	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();


	// skip Internal Zipping ?

	if (pApp->g_bSkipIntern)
		return TRUE;


	// change to sourcefolder (with the unzipped files)
	// (the "finalstep" tool is started from there, like the external packers)

	TCHAR cBuffer[_MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, cBuffer);

	if (! SetCurrentDirectory(strSourceFolder))
		return FALSE;


	// get files from the source folder (relative names, files only)

	std::vector<CString> vecFileList;
	CMisc::ListFilesRelative(strSourceFolder, &vecFileList);

	std::vector<std::wstring> vecRelFiles;
	vecRelFiles.reserve(vecFileList.size());

	for (size_t i = 0; i < vecFileList.size(); i++)
		vecRelFiles.push_back((LPCTSTR)vecFileList[i]);


	// create new zipfile with the maximum deflate settings of 7-Zip
	// (this replaces the old "smartest" ZipArchive/zlib compression)

	const Zip7::CompressOptions options;		// level 9, 15 passes, 258 fast bytes
	std::wstring strError;

	const BOOL bZipOk = Zip7::CompressFiles((LPCTSTR)strArchiveName, (LPCTSTR)strSourceFolder, vecRelFiles, options, &strError);

	if (! bZipOk)
		SetLastZipError(strError);

	_flushall();

	DWORD n = bZipOk ? 0 : 1;

	// Run the final command, if any.
	if ((n == 0) && (pApp->g_strFinalExe != _T("")))
	{
		CString strCommand = pApp->g_strFinalCommandLine;
		strCommand.Replace(_T("%1"), _T('\"') + strArchiveName + _T('\"'));
		strCommand.Replace(_T("%2"), _T("*"));
		n = CMisc::RunCmd(pApp->g_strFinalExe + _T(" ") + strCommand, GetThreadPriority(), pApp->g_bShowPackerWindow, &pApp->g_pi);
	}

	// get headerinformation from the new created zipfile

	const BOOL bReturn = (n == 0) ? GetHeaders(strArchiveName, pvecHeaders) : FALSE;

	// change to the old folder

	SetCurrentDirectory(cBuffer);

	return bReturn;
}


BOOL CZipMaxDlg::ExternalZipFiles(const CString& strArchiveName, const CString& strSourceFolder, std::vector<Zip7::ZipHeader> *pvecHeaders, const DWORD nExternal)
{
	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();


	// change to sourcefolder

	TCHAR cBuffer[_MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, cBuffer);

	if (! SetCurrentDirectory(strSourceFolder))
		return FALSE;


	// replace the commandline options with the current values

	CString strCommand = pApp->g_strCommandLine[nExternal];
	strCommand.Replace(_T("%1"), _T('\"') + strArchiveName + _T('\"'));
	strCommand.Replace(_T("%2"), _T("*"));


	// run the external packer

	DWORD n = CMisc::RunCmd(pApp->g_strPackerExe[nExternal] + _T(" ") + strCommand, GetThreadPriority(), pApp->g_bShowPackerWindow, &pApp->g_pi);

	// Run the final command, if any.
	if ((n == 0) && (pApp->g_strFinalExe != _T("")))
	{
		strCommand = pApp->g_strFinalCommandLine;
		strCommand.Replace(_T("%1"), _T('\"') + strArchiveName + _T('\"'));
		strCommand.Replace(_T("%2"), _T("*"));
		n = CMisc::RunCmd(pApp->g_strFinalExe + _T(" ") + strCommand, GetThreadPriority(), pApp->g_bShowPackerWindow, &pApp->g_pi);
	}


	// get headerinformation from the new created zipfile

	const BOOL bReturn = (BOOL) (n == 0) ? GetHeaders(strArchiveName, pvecHeaders) : FALSE;


	// change to old folder

	SetCurrentDirectory(cBuffer);

	return bReturn;
}


BOOL CZipMaxDlg::GetHeaders(const CString& strArchiveName, std::vector<Zip7::ZipHeader> *pvecHeaders)
{
	// reads the central directory of a zipfile (refuses spanned and zip64 archives)

	pvecHeaders->clear();

	std::wstring strError;

	if (! Zip7::ReadHeaders((LPCTSTR)strArchiveName, pvecHeaders, &strError))
	{
		SetLastZipError(strError);
		return FALSE;
	}

	return TRUE;
}


BOOL CZipMaxDlg::WriteLFH(CStdioFile *pFile, CZipEntry *pZipEntry, DWORD *pOffset, CString *pName)
{
	const Zip7::ZipHeader *pHeader = pZipEntry->m_pHeader;

	if (pHeader->rawName.size() > 0xFFFF)
		return FALSE;

	const WORD nFileNameSize = (WORD)pHeader->rawName.size();


	// allocate some memory for the compressed data

	BYTE *pDataBuffer = new (std::nothrow) BYTE[DATABUFFERSIZE];

	if (pDataBuffer == NULL)
		return FALSE;


	// fill localfileheader structure

	CZipStructs::ZIP_LFH lfh;

	lfh.version_needed_to_extract = pHeader->versionNeeded;		// should match the value from cfh anyway
	lfh.general_purpose_bit_flag = pHeader->flags & 0xfff7;		// remove possible datadescriptors
	lfh.compression_method = pHeader->method;					// should match the value from cfh anyway
	lfh.last_mod_file_time = pHeader->modTime;					// should match the value from cfh anyway
	lfh.last_mod_file_date = pHeader->modDate;					// should match the value from cfh anyway
	lfh.crc32 = pHeader->crc32;									// use value from central dir because of the removal of datadescriptors
	lfh.compressed_size = pHeader->comprSize;					// use value from central dir because of the removal of datadescriptors
	lfh.uncompressed_size = pHeader->uncomprSize;				// use value from central dir because of the removal of datadescriptors
	lfh.filename_length = nFileNameSize;						// should match the value from cfh anyway
	lfh.extra_field_length = 0;									// remove any extrafields

	BOOL bOk = TRUE;

	try
	{
		// write localfileheader and return the lfh offset

		const ULONGLONG nPos = pFile->GetPosition();

		if (nPos > 0xFFFFFFFFULL)
			bOk = FALSE;							// zip64 is not supported
		else
		{
			*pOffset = (DWORD)nPos;

			const DWORD id = (DWORD) ID_PKZIPLFH;
			pFile->Write(&id, sizeof(DWORD));
			pFile->Write(&lfh, sizeof(CZipStructs::ZIP_LFH));
		}
	}
	catch (CFileException *e)
	{
		e->Delete();
		bOk = FALSE;
	}

	if (bOk)
	{
		CStdioFile stioFile;
		CFileException e0;
		if (stioFile.Open(pZipEntry->m_strArchiveName, CFile::modeRead | CFile::typeBinary | CFile::shareExclusive, &e0))
		{
			try
			{
				// jump to the old lfh and check it

				stioFile.Seek((LONGLONG)pHeader->localHeaderOffset, CFile::begin);

				DWORD nOldId = 0;
				CZipStructs::ZIP_LFH lfhOld;

				if (stioFile.Read(&nOldId, sizeof(DWORD)) != sizeof(DWORD) || nOldId != (DWORD)ID_PKZIPLFH ||
					stioFile.Read(&lfhOld, sizeof(CZipStructs::ZIP_LFH)) != sizeof(CZipStructs::ZIP_LFH))
					bOk = FALSE;

				if (bOk)
				{
					// skip the stored filename and the extrafield (use the real lengths of the old lfh)

					stioFile.Seek((LONGLONG)lfhOld.filename_length + (LONGLONG)lfhOld.extra_field_length, CFile::current);


					// write the filename from the central directory

					pFile->Write(pHeader->rawName.data(), nFileNameSize);

					*pName = CString(pHeader->rawName.c_str(), (int)nFileNameSize);


					// write compressed data

					ULONGLONG nCompressedSize = pHeader->comprSize;

					while (bOk && nCompressedSize != 0)
					{
						const UINT nChunk = (nCompressedSize < (ULONGLONG)DATABUFFERSIZE) ? (UINT)nCompressedSize : (UINT)DATABUFFERSIZE;

						if (stioFile.Read(pDataBuffer, nChunk) != nChunk)
							bOk = FALSE;			// source is shorter than its header says
						else
						{
							pFile->Write(pDataBuffer, nChunk);
							nCompressedSize -= nChunk;
						}
					}

					pFile->Flush();
				}

				stioFile.Close();
			}
			catch (CFileException *e)
			{
				e->Delete();
				bOk = FALSE;
			}
		}
		else
		{
			bOk = FALSE;
		}
	}

	delete [] pDataBuffer;

	return bOk;
}


BOOL CZipMaxDlg::WriteCFH(CStdioFile *pFile, CZipEntry *pZipEntry, const DWORD nOffset)
{
	const Zip7::ZipHeader *pHeader = pZipEntry->m_pHeader;

	// fill central directory entry

	CZipStructs::ZIP_CFH cfh;

	cfh.version_made_by = pHeader->versionMadeBy;
	cfh.version_needed_to_extract = pHeader->versionNeeded;
	cfh.general_purpose_bit_flag = pHeader->flags & 0xfff7;		// no datadescriptors
	cfh.compression_method = pHeader->method;
	cfh.last_mod_file_time = pHeader->modTime;
	cfh.last_mod_file_date = pHeader->modDate;
	cfh.crc32 = pHeader->crc32;
	cfh.compressed_size = pHeader->comprSize;
	cfh.uncompressed_size = pHeader->uncomprSize;
	cfh.filename_length = (WORD)pHeader->rawName.size();
	cfh.extra_field_length = 0;									// no extrafields
	cfh.file_comment_length = 0;								// no comments
	cfh.disk_number_start = 0;									// no diskspanning zips
	cfh.internal_file_attributes = pHeader->internalAttr;
	cfh.external_file_attributes = pHeader->externalAttr;
	cfh.relative_offset_of_local_header = nOffset;


	// write central directory entry

	BOOL bOk = TRUE;

	try
	{
		const DWORD id = ID_PKZIPCFH;
		pFile->Write(&id, sizeof(DWORD));
		pFile->Write(&cfh, sizeof(CZipStructs::ZIP_CFH));
		pFile->Write(pHeader->rawName.data(), cfh.filename_length);
		pFile->Flush();
	}
	catch (CFileException *e)
	{
		e->Delete();
		bOk = FALSE;
	}

	return bOk;
}


BOOL CZipMaxDlg::WriteEOZ(CStdioFile *pFile, const DWORD nCentralDirStart, const DWORD nCentralDirSize, const DWORD nEntries)
{

	// fill end-of-zip structure

	CZipStructs::ZIP_EOF eof;

	eof.end_of_central_dir_signature = (DWORD)ID_PKZIPEND;        // id
	eof.number_of_this_disk = 0;                    // no diskspanning
	eof.number_of_the_disk_with_the_start_of_the_central_directory = 0; // no diskspanning
	eof.total_number_of_entries_in_the_central_dir_on_this_disk = nEntries;
	eof.total_number_of_entries_in_the_central_dir = nEntries;
	eof.size_of_the_central_directory = nCentralDirSize;
	eof.offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number = nCentralDirStart;
	eof.zipfile_comment_length = 0;                   // no comments

	BOOL bOk = TRUE;


	// write end-of-zip structure

	try
	{
		pFile->Write(&eof, sizeof(CZipStructs::ZIP_EOF));
		pFile->Flush();
	}
	catch (CFileException *e)
	{
		e->Delete();
		bOk = FALSE;
	}

	return bOk;
}


BOOL CZipMaxDlg::BuildNewZip(const CString& strArchiveName, std::vector<CZipEntry> *pvecEntries)
{
	BOOL bOk = TRUE;

	CStdioFile stdioFile;
	CFileException e0;

	const DWORD nCount = (DWORD)pvecEntries->size();

	if (nCount > 0xFFFF)
		return FALSE;								// more entries than a non-zip64 archive can hold

	std::vector<DWORD> vecLFHOffsets;
	vecLFHOffsets.reserve(nCount);

	if (stdioFile.Open(strArchiveName, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary | CFile::shareExclusive, &e0))
	{
		try
		{
			DWORD i;

			for (i = 0; i < nCount; i++)
			{

				// write all localfileheaders, remember offsets

				CString strName;
				DWORD nOffset = 0;

				if (WriteLFH(&stdioFile, &((*pvecEntries)[i]), &nOffset, &strName))
					vecLFHOffsets.push_back(nOffset);
				else
				{
					stdioFile.Flush();
					stdioFile.Close();
					CMisc::RemoveAFile(strArchiveName);
					return FALSE;
				}
			}


			// more or less offsets than entries ?
			// dummy check...shouldn't happen anyway

			if ((DWORD)vecLFHOffsets.size() != nCount)
			{
				stdioFile.Flush();
				stdioFile.Close();
				CMisc::RemoveAFile(strArchiveName);
				return FALSE;
			}


			// remember start of the central dir

			const ULONGLONG nStartOfCentralDir = stdioFile.GetPosition();

			if (nStartOfCentralDir > 0xFFFFFFFFULL)
				bOk = FALSE;


			// write all central directory entries

			for (i = 0; bOk && i < nCount; i++)
			{
				if (! WriteCFH(&stdioFile, &((*pvecEntries)[i]), vecLFHOffsets[i]))
					bOk = FALSE;
			}


			// write end-of-zipfile

			if (bOk)
			{
				const ULONGLONG nCentralDirSize = stdioFile.GetPosition() - nStartOfCentralDir;
				bOk &= WriteEOZ(&stdioFile, (DWORD)nStartOfCentralDir, (DWORD)nCentralDirSize, nCount);
			}

		}
		catch (CFileException *e)
		{
			e->Delete();
			bOk = FALSE;
		}

		try
		{
			stdioFile.Flush();
			stdioFile.Close();
		}
		catch (CFileException *e)
		{
			e->Delete();
			bOk = FALSE;
		}

		_flushall();
	}
	else
		bOk = FALSE;

	if (! bOk)
		CMisc::RemoveAFile(strArchiveName);		// if anything went wrong, remove the temporary file

	return bOk;
}



void CZipMaxDlg::UpdateList(CListCtrl *pListCtrl, const DWORD nIndex, const CString& strFile, const CString& strStatus, const CString& strBefore, const CString& strAfter, const CString& strInfo, const CString& strGain)
{
	// add an item (one row, 6 columns) to the list

	LV_ITEM item;
	::ZeroMemory(&item, sizeof(LV_ITEM));

	int nStatusIndex = 0;

	if (strGain == _T("-"))
		nStatusIndex = 2;

	else if
		(strGain == _T("0"))
			nStatusIndex = 1;

	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.iImage = nStatusIndex;
	item.iItem = nIndex;
	item.lParam = 0;
	item.pszText = (LPTSTR)LPCTSTR(strFile);
	item.cchTextMax = strFile.GetLength();

	if (pListCtrl != NULL)
	{
		pListCtrl->InsertItem(&item);
		pListCtrl->SetItem(nIndex, 1, LVIF_TEXT, strStatus, 0, 0, 0, 0);
		pListCtrl->SetItem(nIndex, 2, LVIF_TEXT, strBefore, 0, 0, 0, 0);
		pListCtrl->SetItem(nIndex, 3, LVIF_TEXT, strAfter, 0, 0, 0, 0);
		pListCtrl->SetItem(nIndex, 4, LVIF_TEXT, strInfo, 0, 0, 0, 0);
		pListCtrl->SetItem(nIndex, 5, LVIF_TEXT, strGain, 0, 0, 0, 0);
		pListCtrl->EnsureVisible(nIndex, FALSE); // scroll to the new entry
	}

	return;
}


CString CZipMaxDlg::BuildArchiveInfo(const CString& strArchiveName)
{
	// "ZIP | Deflate | Dict: 32 KB | Method: Deflate:Maximum" - describes the compression of a zipfile

	Zip7::ArchiveInfo info;
	std::wstring strError;

	if (! Zip7::DescribeArchive((LPCTSTR)strArchiveName, &info, &strError))
		return CString(_T("-"));

	CString strDict, strMethod;
	strDict.Format(CLang::Get(IDS_INFODICT), info.dictionary.c_str());
	strMethod.Format(CLang::Get(IDS_INFOMETHOD), info.method.c_str());

	return CString(info.type.c_str()) + _T(" | ") + CString(info.compression.c_str()) + _T(" | ") + strDict + _T(" | ") + strMethod;
}


void CZipMaxDlg::ReportProgress(THREADPARAM *p, const DWORD nIndex, const UINT64 nCurrentSize, const double dFraction)
{
	// moves the green total progress bar
	// nCurrentSize/dFraction = how much of the file that is being processed right now is already done (0.0 - 1.0)
	// the weight of a file is the size of its zipfile (or 1 file = 1 part if all sizes are 0)

	if (p->pTotalBar == NULL)
		return;

	double dFrac = dFraction;

	if (dFrac < 0.0)
		dFrac = 0.0;

	if (dFrac > 1.0)
		dFrac = 1.0;

	double dTotal;

	if (p->nTotalSize > 0)
		dTotal = ((double)p->nDoneSize + (double)nCurrentSize * dFrac) / (double)p->nTotalSize;
	else
	{
		const DWORD nCount = (DWORD)p->pvecFiles->size();
		dTotal = (nCount == 0) ? 0.0 : ((double)nIndex + dFrac) / (double)nCount;
	}

	p->pTotalBar->SetPos((int)(dTotal * 1000.0 + 0.5));
}


void CZipMaxDlg::WriteLog(CFile *pFile, const CString& str)
{
	// write a line to the logfile (UTF-8, so that file names in any language are written correctly)

	if (pFile != NULL)
	{
		try
		{
			CString strLine = str + _T("\n");
			strLine.Replace(_T("\n"), _T("\r\n"));		// CRLF (the file is written in binary mode)

			const int nLen = ::WideCharToMultiByte(CP_UTF8, 0, strLine, strLine.GetLength(), NULL, 0, NULL, NULL);

			if (nLen > 0)
			{
				std::string strUtf8((size_t)nLen, '\0');
				::WideCharToMultiByte(CP_UTF8, 0, strLine, strLine.GetLength(), &strUtf8[0], nLen, NULL, NULL);
				pFile->Write(strUtf8.data(), (UINT)nLen);
			}
		}
		catch (CFileException *e)
		{
			e->Delete();
		}
	}

	return;
}


int CZipMaxDlg::GetThreadPriority()
{

	// returns the priority which was set by zipmax.ini

	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

	int nPriority = THREAD_PRIORITY_NORMAL;

	if (pApp->g_strPriority.CompareNoCase(_T("ABOVE_NORMAL")) == 0)
		nPriority = THREAD_PRIORITY_ABOVE_NORMAL;

	if (pApp->g_strPriority.CompareNoCase(_T("BELOW_NORMAL")) == 0)
		nPriority = THREAD_PRIORITY_BELOW_NORMAL;

	if (pApp->g_strPriority.CompareNoCase(_T("HIGHEST")) == 0)
		nPriority = THREAD_PRIORITY_HIGHEST;

	if (pApp->g_strPriority.CompareNoCase(_T("IDLE")) == 0)
		nPriority = THREAD_PRIORITY_IDLE;

	if (pApp->g_strPriority.CompareNoCase(_T("LOWEST")) == 0)
		nPriority = THREAD_PRIORITY_LOWEST;

	if (pApp->g_strPriority.CompareNoCase(_T("TIME_CRITICAL")) == 0)
		nPriority = THREAD_PRIORITY_TIME_CRITICAL;

	return nPriority;
}


BOOL CZipMaxDlg::TestNewZip(const CString& strArchiveName)
{
	// opens an archive and tests each file (uses 7-Zip)
	// returns TRUE if all files are ok, FALSE otherwise
	// const CString& strArchiveName  = full archivename

	std::wstring strError;

	if (! Zip7::TestArchive((LPCTSTR)strArchiveName, &strError))
	{
		SetLastZipError(strError);
		return FALSE;
	}

	return TRUE;
}


BOOL CZipMaxDlg::SafeRename(const CString& strSource)
{
	// a secure way to update the old file with the new one...
	// rename old to temp, new to old, remove temp....

	BOOL bOk = TRUE;

	CString strTmpExt, strTmpExt2;
	strTmpExt = CLang::Get(IDS_TEMPEXTENSION);
	strTmpExt2 = CLang::Get(IDS_TEMPEXTENSION2);

	try
	{
		CFile::Rename(strSource, strSource + strTmpExt2);
	}
	catch (CFileException *e)
	{
		e->Delete();
		bOk = FALSE;
	}
	if (bOk)
	{
		try
		{
			CFile::Rename(strSource + strTmpExt, strSource);
		}
		catch (CFileException *e)
		{
			e->Delete();
			bOk = FALSE;
		}
	}
	if (bOk)
		CMisc::RemoveAFile(strSource + strTmpExt2);
	else
	{
		CMisc::RemoveAFile(strSource + strTmpExt);

		try
		{
			CFile::Rename(strSource + strTmpExt2, strSource);
		}
		catch (CFileException *e)
		{
			e->Delete();
		}
	}

	return bOk;
}


void CZipMaxDlg::UpdateStatus(CStatic *pStatus, const DWORD nIndex, const DWORD nTotalFiles, const DWORD nCurrentTotal, const DWORD nRemainingSecs, const DWORD nDoneSize)
{
	// set a status line with remaining files, est. time, etc...

	CString s;
	s = CLang::Get(IDS_SEPARATOR);

	CString strFiles, strPercent, strPercentSaved, strCurrentTotal, strTime, q;

	q = CLang::Get(IDS_REMAININGFILES);
	strFiles = q + CMisc::NumericToString(nTotalFiles - nIndex) + s;

	q = CLang::Get(IDS_DONEPERCENT);
	const double dPercent = 100.0 * (double) nIndex / (double) nTotalFiles;
	strPercent.Format(q, (nTotalFiles == 0) ? 0 : (DWORD) dPercent);
	strPercent += s;

	const DWORD nSeconds = nRemainingSecs % 60;
	const DWORD nMinutes = (nRemainingSecs / 60) % 60;
	const DWORD nHours = (nRemainingSecs / 3600);

	q = CLang::Get(IDS_REMAININGTIME);
	strTime.Format(q + s, nHours, nMinutes, nSeconds);

	q = CLang::Get(IDS_SAVEDBYTES);
	strCurrentTotal = q + CMisc::NumericToString(nCurrentTotal);

	q = CLang::Get(IDS_PERCENTSAVEDBYTES);
	const double dPercentSaved = (nDoneSize == 0) ? 0 : (100.0 * (double) nCurrentTotal / (double) nDoneSize);
	strPercentSaved.Format (q, dPercentSaved);

	pStatus->SetWindowText(strFiles + strPercent + strTime + strCurrentTotal + strPercentSaved);

	return;
}


UINT CZipMaxDlg::WorkThread(LPVOID pParam)
{
	// Main Zipping thread. Anything is done here :O)
	// I know it should be a lot smaller....more subroutines etc...


	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

	// set thread flag

	pApp->g_bThreadRunning = TRUE;

	THREADPARAM *p = (THREADPARAM *)pParam;


	// prepare some foldernames

	CString t,strExternFolder[MAXEXTERNAL];
	t = CLang::Get(IDS_TEMPUNZIP);
	const CString strUnzipFolder = *(p->pstrMainFolder) + t;

	t = CLang::Get(IDS_TEMPEXTERN);
	DWORD x;
	for (x = 0; x < pApp->g_dwNumExtern; x++)
		strExternFolder[x].Format(*(p->pstrMainFolder) + t, x + 1);

	t = CLang::Get(IDS_TEMPINTERN);
	const CString strInternFolder = *(p->pstrMainFolder) + t;


	CString q, strError, strTmpExt; // for statustexts && logfile and tempextension
	strTmpExt = CLang::Get(IDS_TEMPEXTENSION);


	// prepare logfile

	CFile *pFile = NULL;
	CFile logFile;
	if (pApp->g_bLogFile)
	{
		CFileException e0;
		q = CLang::Get(IDS_LOGFILE);
		if (logFile.Open(*(p->pstrMainFolder) + q, CFile::modeWrite | CFile::modeCreate | CFile::modeNoTruncate | CFile::typeBinary | CFile::shareDenyWrite, &e0))
		{
			try
			{
				if (logFile.GetLength() == 0)
				{
					const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
					logFile.Write(bom, 3);			// UTF-8 byte order mark for a new logfile
				}

				logFile.SeekToEnd();
				pFile = &logFile;
			}
			catch (CFileException *e)
			{
				e->Delete();
			}
		}
	}


	// let's go

	DWORD nTotalGain = 0;

	UpdateStatus(p->pStatus2, 0, (DWORD)p->pvecFiles->size(), nTotalGain, p->nRemainingSize >> 16); // let's start with a default value of 1sec/64kbyte

	for (DWORD i = 0; i < (DWORD)p->pvecFiles->size(); i++)
	{
		if (pApp->g_bStop)
			break;

	    CString cMinStr;
		s_strLastZipError.Empty();
		BOOL bFileListed = FALSE;

		std::vector<Zip7::ZipHeader> vecHeadersOld;
		std::vector<Zip7::ZipHeader> vecHeadersIntern;
		std::vector<Zip7::ZipHeader> vecHeadersExtern[MAXEXTERNAL];

		CString strExternFile[MAXEXTERNAL];
		const CString strSource = (*p->pvecFiles)[i];
		const CString strInternFile = strInternFolder + CMisc::GetPureFileName(strSource);

		for (x = 0; x < pApp->g_dwNumExtern; x++)
			strExternFile[x] = strExternFolder[x] + CMisc::GetPureFileName(strSource);


		// cleanup + create folders

		CMisc::RemoveAFolder(strUnzipFolder);
		for (x = 0; x < pApp->g_dwNumExtern; x++)
			CMisc::RemoveAFolder(strExternFolder[x]);
		CMisc::RemoveAFolder(strInternFolder);

		CreateDirectory(strUnzipFolder, NULL);
		for (x = 0; x < pApp->g_dwNumExtern; x++)
			CreateDirectory(strExternFolder[x], NULL);
		CreateDirectory(strInternFolder, NULL);


		BOOL bFolderOk = TRUE;
		bFolderOk &= CMisc::IsFolderEmpty(strUnzipFolder);
		for (x = 0; x < pApp->g_dwNumExtern; x++)
			bFolderOk &= CMisc::IsFolderEmpty(strExternFolder[x]);
		bFolderOk &= CMisc::IsFolderEmpty(strInternFolder);


		UINT64 nCurrentSize = 0;
		CMisc::GetFileSize(strSource, &nCurrentSize);
		CMisc::GetTimeSpan();

		// weights of the processing steps of this file (for the green total progress bar)

		const double dWUnzip = 1.0;
		const double dWIntern = pApp->g_bSkipIntern ? 0.0 : 2.0;
		const double dWExtern = 3.0;
		const double dWBuild = 1.5;
		const double dWTotal = dWUnzip + dWIntern + dWExtern * (double)pApp->g_dwNumExtern + dWBuild;
		double dWDone = 0.0;

		ReportProgress(p, i, nCurrentSize, 0.0);

		// -- START: Special file skipping code -- 
		BOOL bSkipFile = FALSE;
		CFileStatus rStatus;
		CFile::GetStatus(strSource, rStatus);

		// If requested, check if file is Archive, skip if it is
		if (pApp->g_bSkipArchive && (rStatus.m_attribute & CFile::archive)) {
			q = CLang::Get(IDS_SKIPPEDA);
			strError = CLang::Get(IDS_IDSARCHIVESET);
			bSkipFile = true;
		}
		// If requested, check if file is ReadOnly, skip if it is
		else {
			if (pApp->g_bSkipReadOnly && (rStatus.m_attribute & CFile::readOnly)) {
				q = CLang::Get(IDS_SKIPPEDR);
				strError = CLang::Get(IDS_IDSREADONLYSET);
				bSkipFile = true;
			}
			// If requested, mark file for ReadOnly (it is unchecked later on replacement) @@what if no savings?
			else {
				if (pApp->g_bSetCurrentReadOnly) {
					rStatus.m_attribute |= CFile::readOnly;
					try {
						CFile::SetStatus(strSource, rStatus);
					}
					catch (CFileException *e) {
						e->Delete();
					}
				}
			}
		}
		if (bSkipFile) {
			const CString strSize = CMisc::NumericToString64(nCurrentSize);
			UpdateList(p->pListCtrl, i, strSource, q, strSize, strSize, _T("-"), _T("0"));
			WriteLog(pFile, q + _T(": ") + strSource + _T(" -> ") + strError + _T("\n"));

			// Make it skip writing different 'real' error below (TODO: Restructure if statements/merge this in below)
			bFileListed = true;		
			bFolderOk = false;
		}
		// -- END: Special file skipping code -- 

		if (bFolderOk)
		{
			// unzip files

			q = CLang::Get(IDS_UNZIP);
			p->pStatus1->SetWindowText(q + CMisc::GetPureFileName(strSource));
			if (UnzipAllFiles(strSource, strUnzipFolder, &vecHeadersOld))
			{
				ReportProgress(p, i, nCurrentSize, (dWDone += dWUnzip) / dWTotal);

				// internal zip

				if (pApp->g_bStop)
					break;

				q = CLang::Get(IDS_INTERNAL);
				p->pStatus1->SetWindowText(q + CMisc::GetPureFileName(strSource));
				if (InternalZipFiles(strInternFile, strUnzipFolder, &vecHeadersIntern))
				{
					ReportProgress(p, i, nCurrentSize, (dWDone += dWIntern) / dWTotal);

					// external zip

					BOOL bOK = TRUE;
					CString strStatus;
					q = CLang::Get(IDS_EXTERNAL);

					for (x = 0; x < pApp->g_dwNumExtern; x++)
					{
						if (pApp->g_bStop)
							break;

						strStatus.Format(q + CMisc::GetPureFileName(strSource), x + 1, pApp->g_dwNumExtern);
						p->pStatus1->SetWindowText(strStatus);

						if (! ExternalZipFiles(strExternFile[x], strUnzipFolder, &vecHeadersExtern[x], x))
						{
							bOK = FALSE;
							break;
						}

						ReportProgress(p, i, nCurrentSize, (dWDone += dWExtern) / dWTotal);
					}

					if (pApp->g_bStop)
						break;

					if (bOK)
					{
						// analyze files

						if (pApp->g_bStop)
							break;

						q = CLang::Get(IDS_ANALYZE);
						p->pStatus1->SetWindowText(q + CMisc::GetPureFileName(strSource));

						const DWORD nCount = (DWORD)vecHeadersOld.size();

						BOOL bError = FALSE;
						std::vector<CZipEntry> vecEntries;
						vecEntries.reserve(nCount);

						// lookup tables: lowercase entry name -> index of the first entry with that name

						std::vector< std::map<std::wstring, size_t> > vecMapExtern(pApp->g_dwNumExtern);
						std::map<std::wstring, size_t> mapIntern;

						for (x = 0; x < pApp->g_dwNumExtern; x++)
							for (size_t k = 0; k < vecHeadersExtern[x].size(); k++)
								vecMapExtern[x].insert(std::make_pair(vecHeadersExtern[x][k].key, k));

						for (size_t k = 0; k < vecHeadersIntern.size(); k++)
							mapIntern.insert(std::make_pair(vecHeadersIntern[k].key, k));

						for (DWORD h = 0; h < nCount; h++)
						{
							const Zip7::ZipHeader& headerOld = vecHeadersOld[h];

							if (! headerOld.IsDirectory()) // skip pure subfolders
							{
								const CString strName = CMisc::WideToCString(headerOld.name);

								// search the currentfile in the external zips

								DWORD nIndexExternal[MAXEXTERNAL];

								for (x = 0; x < pApp->g_dwNumExtern; x++)
								{
									std::map<std::wstring, size_t>::const_iterator it = vecMapExtern[x].find(headerOld.key);

									if (it == vecMapExtern[x].end())
									{
										bError = TRUE;
										break;
									}

									nIndexExternal[x] = (DWORD)it->second;
								}

								if (bError)
									break;

								// search the currentfile in the internal zip

								DWORD nIndexInternal = 0;

								if (! pApp->g_bSkipIntern)
								{
									std::map<std::wstring, size_t>::const_iterator it = mapIntern.find(headerOld.key);

									if (it == mapIntern.end())
									{
										bError = TRUE;
										break;
									}

									nIndexInternal = (DWORD)it->second;
								}


								// get the sizes

								const DWORD nOldSize = headerOld.comprSize;
								const DWORD nInternSize = pApp->g_bSkipIntern ? nOldSize + 1 : vecHeadersIntern[nIndexInternal].comprSize;

								DWORD nExternSize[MAXEXTERNAL];
								for (x = 0; x < pApp->g_dwNumExtern; x++)
									nExternSize[x] = vecHeadersExtern[x][nIndexExternal[x]].comprSize;


								// compare the sizes and choose the smallest one

								CZipEntry entry;
								entry.m_pHeader = &vecHeadersOld[h];
								entry.m_strArchiveName = strSource;
								entry.m_strEntryName = strName;

								DWORD nMinSize = nOldSize;

								if (nInternSize < nMinSize)
								{
									entry.m_pHeader = &vecHeadersIntern[nIndexInternal];
									entry.m_strArchiveName = strInternFile;
									nMinSize = nInternSize;
								}

								for (x = 0; x < pApp->g_dwNumExtern; x++)
								{
									if (nExternSize[x] < nMinSize)
									{
										entry.m_pHeader = &vecHeadersExtern[x][nIndexExternal[x]];
										entry.m_strArchiveName = strExternFile[x];
										nMinSize = nExternSize[x];
									}
								}

								// Determine all the ones that have the minimum size.
								cMinStr += _T("\n    ") + strName + _T("  --> ");
								if (nOldSize == nMinSize)
									cMinStr += _T(" ORG");

								if (nInternSize == nMinSize)
									cMinStr += _T(" INT");

								for (x = 0; x < pApp->g_dwNumExtern; x++)
								{
									if (nExternSize[x] == nMinSize)
									{
										CString cExtNumber;
										cExtNumber.Format(_T(" %03u"),x+1);
										cMinStr += cExtNumber;
									}
								}

								vecEntries.push_back(entry);
							}
						}

						if (! bError && (DWORD)vecEntries.size() > 0)
						{
							// sort filenames alphabetically

							std::stable_sort(vecEntries.begin(), vecEntries.end());

							// create the mixed zipfile

							BOOL bOk = BuildNewZip(strSource + strTmpExt, &vecEntries);

							// do an additional zipfiletest

							if (bOk)
								bOk &= TestNewZip(strSource + strTmpExt);

							ReportProgress(p, i, nCurrentSize, (dWDone += dWBuild) / dWTotal);

							if (bOk)
							{
								CFileStatus stOld, stNew;
								bOk &= CFile::GetStatus(strSource + strTmpExt, stNew);
								bOk &= CMisc::SetFileWriteable(strSource, &stOld);

								const INT64 nGainSigned = (INT64)stOld.m_size - (INT64)stNew.m_size;
								const DWORD nGain = (nGainSigned > 0) ? (DWORD)nGainSigned : 0;

								if (bOk)
								{
									// calculate gain

									if (nGain > 0)
									{
										if (pApp->g_bStop)
											break;

										q = CLang::Get(IDS_UPDATE);
										p->pStatus1->SetWindowText(q + CMisc::GetPureFileName(strSource));

										nTotalGain += nGain;

										// replace old file with new file

										if (! SafeRename(strSource))
										{
											bError = TRUE;
											strError = CLang::Get(IDS_CANTREPLACE);
										}
									}
								}


								// insert file to list

								if (! bError)
								{
									// reset time/date, set archive flag

									CFileStatus status;
									if (CFile::GetStatus(strSource, status))
									{
										if (! pApp->g_bSetCurrentTime)
										{
											status.m_atime = stOld.m_atime;
											status.m_ctime = stOld.m_ctime;
											status.m_mtime = stOld.m_mtime;
										}
										else
										{
											status.m_ctime = CTime::GetCurrentTime();
											status.m_atime = status.m_ctime;
											status.m_mtime = status.m_ctime;
										}

										status.m_attribute |= CFile::archive;

										try
										{
											CFile::SetStatus(strSource, status);
										}
										catch (CFileException *e)
										{
											e->Delete();
										}
									}

									CString strGain = CMisc::NumericToString(nGain);
									q = CLang::Get(IDS_OK);

									// size before/after + description of the compression of the file that is now on disk
									const CString strBefore = CMisc::NumericToString64(nCurrentSize);
									const CString strAfter = CMisc::NumericToString64((nGain <= nCurrentSize) ? nCurrentSize - nGain : 0);
									const CString strInfo = BuildArchiveInfo(strSource);

									UpdateList(p->pListCtrl, i, strSource, q, strBefore, strAfter, strInfo, strGain);
									bFileListed = TRUE;
									WriteLog(pFile, strSource + _T(" (") + strGain +_T(")") + cMinStr);
								}
							}
							else
								strError = CLang::Get(IDS_CANTCREATE);

							// if tempfile still exists (could that happen anyway ?), remove it
							CMisc::RemoveAFile(strSource + strTmpExt);
						}
					}
					else
						strError = CLang::Get(IDS_EXTERNALFAIL);
				}
				else
					strError = CLang::Get(IDS_INTERNALFAIL);
			}
			else
				strError = CLang::Get(IDS_UNZIPFAIL);
		}
		else
			strError = CLang::Get(IDS_CANTCLEANUP);

		if (! bFileListed)
		{
			// show file as failed

			q = CLang::Get(IDS_ERROR);
			UpdateList(p->pListCtrl, i, strSource, q, CMisc::NumericToString64(nCurrentSize), _T("-"), _T("-"), _T("-"));
			CString strDetail;
			if (! s_strLastZipError.IsEmpty())
				strDetail = _T(" [") + s_strLastZipError + _T("]");
			WriteLog(pFile, q + _T(": ") + strSource + _T(" -> ") + strError + strDetail + _T("\n"));
		}


		// calculate remaining time, increase progressbarvalue + update statusbar

		// maybe it should better be calculated on the sum of the decompressed file
		// sizes instead of using the size of the zipfiles
		// ...but then you have to get all fileheaders...

		p->nRemainingSize -= nCurrentSize;
		p->nDoneSize += nCurrentSize;
		p->nDoneTime += (DWORD) CMisc::GetTimeSpan(FALSE).GetTotalSeconds();

		p->pProgressCtrl->StepIt();
		ReportProgress(p, i + 1, 0, 0.0);
		const double dRemainingTime = (double)p->nDoneTime * (double)p->nRemainingSize / ((p->nDoneSize == 0) ? (double)1.0 : (double)p->nDoneSize);
		UpdateStatus(p->pStatus2, i + 1, (DWORD)p->pvecFiles->size(), nTotalGain, (DWORD)dRemainingTime, p->nDoneSize);
	}


	// clean up folders

	CMisc::RemoveAFolder(strUnzipFolder);

	for (x = 0; x < pApp->g_dwNumExtern; x++)
		CMisc::RemoveAFolder(strExternFolder[x]);

	CMisc::RemoveAFolder(strInternFolder);


	// show total gain and beep :O)

	p->pProgressCtrl->SetPos(0);

	if (! pApp->g_bStop)
		p->pTotalBar->SetPos(1000);		// all files done: 100%

	CString qq;
	qq = CLang::Get(IDS_TOTAL);
	CString strTotalGain = CMisc::NumericToString(nTotalGain);
	CString strTotal;
	strTotal.Format(qq, (LPCTSTR)strTotalGain);
	p->pStatus1->SetWindowText(strTotal);

	if (! pApp->g_bNoBeep)
	{
		Beep(1000, 100);
		Beep(1500, 100);
		Beep(2000, 100);
	}

	WriteLog(pFile, _T("\n") + strTotal + _T("\n--------"));


	// close logfile (if any)

	if (pFile != NULL)
	{
		pFile->Flush();
		pFile->Close();
	}


	// clear thread flag and end

	pApp->g_bThreadRunning = FALSE;

	return 0;
}


void CZipMaxDlg::StartZipping ()
{
	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

	DWORD nListSize = (int)m_vecFiles.size();

	// init statusline, progressbar, etc..

	m_pListCtrl->DeleteAllItems();
	m_pStatus1->SetWindowText(_T(""));
	m_pProgCtrl->SetPos(0);
	m_pProgCtrl->SetStep(1);
	m_pProgCtrl->SetRange(0, nListSize);
	m_TotalBar.SetPos(0);


	// any files at all ? if not, do nothing

	if (nListSize == 0)
		return;


	// get total filesize (only for remaining time calculation)

	UINT64 nTotalSize = 0;
	UINT64 nSize = 0;

	for (DWORD i = 0; i < nListSize; i++)
	{
		CMisc::GetFileSize(m_vecFiles[i], &nSize);
		nTotalSize += nSize;
	}


	// fill thread-Parameter

	m_param.pstrMainFolder = &(pApp->g_strMainFolder);
	m_param.pProgressCtrl = m_pProgCtrl;
	m_param.pTotalBar = &m_TotalBar;
	m_param.nTotalSize = nTotalSize;
	m_param.pListCtrl = m_pListCtrl;
	m_param.pStatus1 = m_pStatus1;
	m_param.pStatus2 = m_pStatus2;
	m_param.pvecFiles = &m_vecFiles;
	m_param.nRemainingSize = nTotalSize;
	m_param.nDoneSize = (UINT64) 0;
	m_param.nDoneTime = (DWORD) 0;


	// reset stopflag

	pApp->g_bStop = FALSE;


	// start thread

	m_pThread = AfxBeginThread(WorkThread, &m_param, GetThreadPriority());

	return;
}

int CALLBACK CZipMaxDlg::CompareProc (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	LPCTSTR t1,t2;
	SORTINFO *si;
	CListCtrl *pListCtrl;
	CString s1,s2,cstrDashes;
	int nColumn,nSortDirection,nLen;

	si = (SORTINFO *)lParamSort;
	pListCtrl = si->pListCtrl;
	nColumn = si->nColumn;
	nSortDirection = si->nSortDirection;
	s1 = pListCtrl->GetItemText((int)lParam1,nColumn);
	s2 = pListCtrl->GetItemText((int)lParam2,nColumn);

	// If it is column 2, 3 or 5 (which have a number with a thousands separator), then simply
	// sorting the strings does not work (e.g. the strings 123 89 7 would be sorted
	// as 123 7 89). An easy solution is to prepend dashes to the shortest of the
	// pair of strings so that both strings have the same length. We use dashes so
	// that entries that have a dash in this column appear at the top when sorting
	// in ascending order.
	if (nColumn == 2 || nColumn == 3 || nColumn == 5)
	{
		nLen = (std::max)(s1.GetLength(),s2.GetLength());
		cstrDashes = CString(_T('-'),nLen);
		s1 = (cstrDashes + s1).Right(nLen);
		s2 = (cstrDashes + s2).Right(nLen);
	}

	t1 = (LPCTSTR)s1;
	t2 = (LPCTSTR)s2;

	return nSortDirection * _tcsicmp(t1,t2);
}

void CZipMaxDlg::GetColumnInfo (int nColumn, LVCOLUMN *pcolumn, CString &str)
{
	// text + alignment of the columns

	switch (nColumn)
	{
		case 0:
			str = CLang::Get(IDS_FILE);
			pcolumn->fmt = LVCFMT_LEFT;
			break;
		case 1:
			str = CLang::Get(IDS_STATUS);
			pcolumn->fmt = LVCFMT_CENTER;
			break;
		case 2:
			str = CLang::Get(IDS_SIZEBEFORE);
			pcolumn->fmt = LVCFMT_RIGHT;
			break;
		case 3:
			str = CLang::Get(IDS_SIZEAFTER);
			pcolumn->fmt = LVCFMT_RIGHT;
			break;
		case 4:
			str = CLang::Get(IDS_METHODINFO);
			pcolumn->fmt = LVCFMT_LEFT;
			break;
		case 5:
			str = CLang::Get(IDS_GAINEDBYTES);
			pcolumn->fmt = LVCFMT_RIGHT;
			break;
		default:
			return;
	}

	pcolumn->pszText = (LPTSTR)(LPCTSTR)str;

	return;
}

void CZipMaxDlg::OnColumnClick (NMHDR *pNMHDR, LRESULT *pResult)
{
	SORTINFO si;
	CString str;
	LVCOLUMN column;
	CZipMaxApp *pApp;
	NMLISTVIEW *pNMListView;
	int i,nNumInList,nColumn;

	// Don't try sorting columns while busy or when the list is empty.
	pApp = (CZipMaxApp *)AfxGetApp();
	nNumInList = m_pListCtrl->GetItemCount();
	if ((pApp->g_bThreadRunning) || (nNumInList == 0))
	{
		*pResult = FALSE;
		return;
	}

	pNMListView = (NMLISTVIEW *)pNMHDR;
	nColumn = pNMListView->iSubItem;
	m_nSortDirection = (nColumn == m_nLastColumn) ? -m_nSortDirection : 1;

	// lParam1 and lParam2 in CompareProc() give the lParam of the respective items,
	// so we make sure that the lParam of each item is its index in the current list.
	for (i = 0; i < nNumInList; i++)
		m_pListCtrl->SetItem(i,0,LVIF_PARAM,NULL,0,0,0,i);

	si.pListCtrl = m_pListCtrl;
	si.nColumn = nColumn;
	si.nSortDirection = m_nSortDirection;
	m_pListCtrl->SortItems(CompareProc,(LPARAM)&si);

	if ((m_nLastColumn >= 0) && (nColumn != m_nLastColumn))
	{
		GetColumnInfo(m_nLastColumn,&column,str);
		column.mask = LVCF_FMT;
		m_pListCtrl->SetColumn(m_nLastColumn,&column);
	}

	GetColumnInfo(nColumn,&column,str);
	column.mask = LVCF_FMT | LVCF_IMAGE;
	column.iImage = (m_nSortDirection == 1) ? 3 : 4;
	column.fmt |= LVCFMT_IMAGE | LVCFMT_COL_HAS_IMAGES | LVCFMT_BITMAP_ON_RIGHT;
	m_pListCtrl->SetColumn(nColumn,&column);

	m_nLastColumn = nColumn;

	*pResult = TRUE;

	return;
}

void CZipMaxDlg::OnBnClickedButtonPause()
{
	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

	if (pApp->g_pi.hThread == 0)
		return;

	pApp->g_bPause = ! pApp->g_bPause;

	if (pApp->g_bPause)
		SuspendThread(pApp->g_pi.hThread);
	else
		ResumeThread(pApp->g_pi.hThread);

	if (m_pStatus1 != NULL && IsWindow(m_pStatus1->m_hWnd))
	{
		if (pApp->g_bPause)
		{
			CString s;
			s = CLang::Get(IDS_PAUSETEXT);
			m_pStatus1->GetWindowText(pApp->g_strOldStatus1);
			m_pStatus1->SetWindowText(s);
		}
		else
			m_pStatus1->SetWindowText(pApp->g_strOldStatus1);

		m_pStatus1->Invalidate();
	}

	return;
}


//////////////////////////////////////////////////////////////////////////
// File list: everything that starts a job (drag&drop, command line, File menu) ends up in AddFilesAndStart()


void CZipMaxDlg::ResetSortArrow()
{
	// Remove sort arrows (if any), because a new job will not have anything sorted.

	if (m_nLastColumn >= 0)
	{
		CString str;
		LVCOLUMN column;

		GetColumnInfo(m_nLastColumn, &column, str);
		column.mask = LVCF_FMT;
		m_pListCtrl->SetColumn(m_nLastColumn, &column);
		m_nLastColumn = -1;
	}
}


BOOL CZipMaxDlg::CheckNotBusy()
{
	// the File/Settings commands that change the job are not available while zipping

	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

	if (pApp->g_bThreadRunning)
	{
		MessageBox(CLang::Get(IDS_BUSY), CLang::Get(IDS_STRING_TITLE), MB_ICONINFORMATION | MB_OK);
		return FALSE;
	}

	return TRUE;
}


void CZipMaxDlg::AddFilesAndStart(const std::vector<CString>& vecFiles)
{
	if (vecFiles.empty())
	{
		MessageBox(CLang::Get(IDS_NOZIPFOUND), CLang::Get(IDS_STRING_TITLE), MB_ICONINFORMATION | MB_OK);
		return;
	}

	ResetSortArrow();

	m_vecFiles = vecFiles;

	StartZipping();
}


//////////////////////////////////////////////////////////////////////////
// Menu


static void SetMenuText(HMENU hMenu, UINT nItem, BOOL bByPosition, const CString& str)
{
	// changes only the text of a menu item

	MENUITEMINFO mii;
	::ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = (LPTSTR)(LPCTSTR)str;

	::SetMenuItemInfo(hMenu, nItem, bByPosition, &mii);
}


void CZipMaxDlg::ApplyMenuLanguage()
{
	// the menu comes from the resource (English); the texts are replaced with the ones of the language file

	CMenu *pMenu = GetMenu();

	if (pMenu == NULL)
		return;

	SetMenuText(pMenu->GetSafeHmenu(), 0, TRUE, CLang::Get(IDS_MENU_FILE));
	SetMenuText(pMenu->GetSafeHmenu(), 1, TRUE, CLang::Get(IDS_MENU_SETTINGS));
	SetMenuText(pMenu->GetSafeHmenu(), 2, TRUE, CLang::Get(IDS_MENU_ABOUT));

	CMenu *pFile = pMenu->GetSubMenu(0);
	CMenu *pSettings = pMenu->GetSubMenu(1);
	CMenu *pAbout = pMenu->GetSubMenu(2);

	if (pFile != NULL)
	{
		SetMenuText(pFile->GetSafeHmenu(), IDM_FILE_ADDFILES, FALSE, CLang::Get(IDS_MI_ADDFILES));
		SetMenuText(pFile->GetSafeHmenu(), IDM_FILE_ADDFOLDER, FALSE, CLang::Get(IDS_MI_ADDFOLDER));
		SetMenuText(pFile->GetSafeHmenu(), IDM_FILE_ADDFOLDERSUB, FALSE, CLang::Get(IDS_MI_ADDFOLDERSUB));
		SetMenuText(pFile->GetSafeHmenu(), IDM_FILE_CLEARLIST, FALSE, CLang::Get(IDS_MI_CLEARLIST));
		SetMenuText(pFile->GetSafeHmenu(), IDM_FILE_EXIT, FALSE, CLang::Get(IDS_MI_EXIT));
	}

	if (pSettings != NULL)
	{
		SetMenuText(pSettings->GetSafeHmenu(), IDM_SET_OPTIONS, FALSE, CLang::Get(IDS_MI_OPTIONS));
		SetMenuText(pSettings->GetSafeHmenu(), IDM_SET_VIEW, FALSE, CLang::Get(IDS_MI_VIEW));
		SetMenuText(pSettings->GetSafeHmenu(), 3, TRUE, CLang::Get(IDS_MENU_LANGUAGE));		// (position 3 = the Language submenu)
	}

	if (pAbout != NULL)
		SetMenuText(pAbout->GetSafeHmenu(), IDM_HELP_ABOUT, FALSE, CLang::Get(IDS_MI_ABOUT));

	DrawMenuBar();
}


void CZipMaxDlg::RefreshLanguage()
{
	// the language was changed (Settings > Options): refresh the texts that are already on the screen

	SetWindowText(CLang::Get(IDS_STRING_TITLE));

	ApplyMenuLanguage();
	BuildLanguageMenu();

	for (int nCol = 0; nCol < NUMCOLUMNS; nCol++)
	{
		CString str;
		LVCOLUMN column;

		::ZeroMemory(&column, sizeof(LVCOLUMN));
		GetColumnInfo(nCol, &column, str);
		column.mask = LVCF_TEXT;
		m_pListCtrl->SetColumn(nCol, &column);
	}

	CString strPause;

	if (! CLang::TryGet(IDC_BUTTON_PAUSE, &strPause))
		strPause = _T("P");

	m_pButtonPause->SetWindowText(strPause);
}


void CZipMaxDlg::OnInitMenuPopup(CMenu *pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	CDialog::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	if (bSysMenu || pPopupMenu == NULL)
		return;

	// what changes the job is not available while zipping (Exit, the view settings and About always are)

	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();
	const UINT nFlags = MF_BYCOMMAND | (pApp->g_bThreadRunning ? MF_GRAYED : MF_ENABLED);

	pPopupMenu->EnableMenuItem(IDM_FILE_ADDFILES, nFlags);
	pPopupMenu->EnableMenuItem(IDM_FILE_ADDFOLDER, nFlags);
	pPopupMenu->EnableMenuItem(IDM_FILE_ADDFOLDERSUB, nFlags);
	pPopupMenu->EnableMenuItem(IDM_FILE_CLEARLIST, nFlags);
	pPopupMenu->EnableMenuItem(IDM_SET_OPTIONS, nFlags);

	for (UINT nId = IDM_LANG_FIRST; nId <= IDM_LANG_LAST; nId++)
		pPopupMenu->EnableMenuItem(nId, nFlags);
}


void CZipMaxDlg::OnFileAddFiles()
{
	if (! CheckNotBusy())
		return;

	// the dialog may return thousands of names: give it a big buffer

	const DWORD nBufferSize = 1024 * 1024;
	std::vector<TCHAR> vecBuffer(nBufferSize, 0);

	CFileDialog dlg(TRUE, _T("zip"), NULL, OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER, CLang::Get(IDS_FILTER_ZIP), this);
	dlg.m_ofn.lpstrFile = &vecBuffer[0];
	dlg.m_ofn.nMaxFile = nBufferSize;

	if (dlg.DoModal() != IDOK)
		return;

	std::vector<CString> vecFiles;
	POSITION pos = dlg.GetStartPosition();

	while (pos != NULL)
	{
		const CString strFile = dlg.GetNextPathName(pos);

		if (strFile.Right(4).CompareNoCase(_T(".zip")) == 0)		// (only zipfiles, like drag&drop)
			vecFiles.push_back(strFile);
	}

	AddFilesAndStart(vecFiles);
}


static void AddFolderZip(CZipMaxDlg *pDlg, const BOOL bSubfolders, std::vector<CString> *pvecFiles)
{
	// asks for a folder and collects its zipfiles. pvecFiles stays empty if the user cancels.

	pvecFiles->clear();

	const CString strTitle = CLang::Get(IDS_PICKFOLDER);

	CFolderPickerDialog dlg(NULL, 0, pDlg, 0);
	dlg.m_ofn.lpstrTitle = (LPCTSTR)strTitle;

	if (dlg.DoModal() != IDOK)
		return;

	CMisc::ScanFolderZip(dlg.GetPathName(), bSubfolders, pvecFiles);

	if (pvecFiles->empty())
		pDlg->MessageBox(CLang::Get(IDS_NOZIPFOUND), CLang::Get(IDS_STRING_TITLE), MB_ICONINFORMATION | MB_OK);
}


void CZipMaxDlg::OnFileAddFolder()
{
	if (! CheckNotBusy())
		return;

	std::vector<CString> vecFiles;
	AddFolderZip(this, FALSE, &vecFiles);

	if (! vecFiles.empty())
		AddFilesAndStart(vecFiles);
}


void CZipMaxDlg::OnFileAddFolderSub()
{
	if (! CheckNotBusy())
		return;

	std::vector<CString> vecFiles;
	AddFolderZip(this, TRUE, &vecFiles);

	if (! vecFiles.empty())
		AddFilesAndStart(vecFiles);
}


void CZipMaxDlg::OnFileClearList()
{
	if (! CheckNotBusy())
		return;

	ResetSortArrow();

	m_vecFiles.clear();
	m_pListCtrl->DeleteAllItems();
	m_TotalBar.SetPos(0);
	m_pProgCtrl->SetPos(0);

	m_pStatus1->SetWindowText(CLang::Get(IDS_DROPFILES));
	UpdateStatus(m_pStatus2);
}


void CZipMaxDlg::OnFileExit()
{
	OnCancel();			// (the same as closing the window: asks first if it is still zipping)
}


void CZipMaxDlg::OnSetOptions()
{
	if (! CheckNotBusy())
		return;

	COptionsDlg dlg(this);

	if (dlg.DoModal() == IDOK)
		RefreshLanguage();		// (zipmax.ini was saved and reloaded by the dialog)
}


void CZipMaxDlg::OnSetView()
{
	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

	SaveColumnState();			// current widths/order -> pApp (so nothing is lost when columns are hidden)

	CViewSettingsDlg dlg(pApp->g_view, this);

	if (dlg.DoModal() == IDOK)
	{
		pApp->g_view = dlg.m_cfg;
		pApp->g_view.Save(pApp);

		ApplyView();
	}
}


void CZipMaxDlg::OnHelpAbout()
{
	CAboutDlg dlg(this);
	dlg.DoModal();
}


//////////////////////////////////////////////////////////////////////////
// Dynamic window: visible columns/items and their order (CViewConfig, Settings > Dialog Box)


void CZipMaxDlg::SaveColumnState()
{
	// stores the widths of the visible columns (hidden ones have width 0 and keep their old width) and the column order

	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

	int *pnWidths[NUMCOLUMNS] = { &pApp->g_nWidth0, &pApp->g_nWidth1, &pApp->g_nWidth2, &pApp->g_nWidth3, &pApp->g_nWidth4, &pApp->g_nWidth5 };

	for (int nCol = 0; nCol < NUMCOLUMNS; nCol++)
	{
		const int nWidth = m_pListCtrl->GetColumnWidth(nCol);

		if (nWidth > 0)
			*pnWidths[nCol] = nWidth;
	}

	int nOrder[NUMCOLUMNS];

	if (m_pListCtrl->GetColumnOrderArray(nOrder, NUMCOLUMNS) && CViewConfig::IsValidOrder(nOrder, NUMCOLUMNS))
		for (int i = 0; i < NUMCOLUMNS; i++)
			pApp->g_view.m_nColumnOrder[i] = nOrder[i];
}


void CZipMaxDlg::ApplyView()
{
	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();
	const CViewConfig& view = pApp->g_view;

	const int nWidths[NUMCOLUMNS] = { pApp->g_nWidth0, pApp->g_nWidth1, pApp->g_nWidth2, pApp->g_nWidth3, pApp->g_nWidth4, pApp->g_nWidth5 };

	// columns: hidden = width 0 (the subitem numbers never change, so UpdateList() doesn't care)

	for (int nCol = 0; nCol < NUMCOLUMNS; nCol++)
	{
		int nWidth = nWidths[nCol];

		if (nWidth < 20)
			nWidth = 20;

		m_pListCtrl->SetColumnWidth(nCol, view.m_bColumnVisible[nCol] ? nWidth : 0);
	}

	int nOrder[NUMCOLUMNS];

	for (int i = 0; i < NUMCOLUMNS; i++)
		nOrder[i] = view.m_nColumnOrder[i];

	m_pListCtrl->SetColumnOrderArray(NUMCOLUMNS, nOrder);

	// items at the bottom of the window

	LayoutControls();
}


void CZipMaxDlg::LayoutControls()
{
	// list on top, the visible items below it in the chosen order

	CWnd *pList = GetDlgItem(IDC_LIST);
	CWnd *pProgress = GetDlgItem(IDC_PROGRESS);
	CWnd *pStatus1 = GetDlgItem(IDC_STATUS1);
	CWnd *pStatus2 = GetDlgItem(IDC_STATUS2);
	CWnd *pPause = GetDlgItem(IDC_BUTTON_PAUSE);

	if (pList == NULL || ! ::IsWindow(pList->m_hWnd) || pProgress == NULL || pStatus1 == NULL || pStatus2 == NULL || pPause == NULL)
		return;

	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();
	const CViewConfig& view = pApp->g_view;

	CRect rectClient;
	GetClientRect(&rectClient);

	const int kHeight[NUMITEMS] = { 16, 16, 16, 18 };		// ITEM_PROGRESS (+ Pause button), ITEM_STATUS, ITEM_SUMMARY, ITEM_TOTALBAR

	// the progress row is always there (it holds the Pause button); the other items only when they are visible

	int nBottom = 0;

	for (int i = 0; i < NUMITEMS; i++)
		if (i == ITEM_PROGRESS || view.m_bItemVisible[i])
			nBottom += kHeight[i];

	CRect rectList = rectClient;
	rectList.bottom = rectClient.bottom - nBottom;

	if (rectList.bottom < rectList.top)
		rectList.bottom = rectList.top;

	pList->MoveWindow(rectList);

	int y = rectList.bottom;

	for (int nPos = 0; nPos < NUMITEMS; nPos++)
	{
		const int nItem = view.m_nItemOrder[nPos];
		const BOOL bVisible = view.m_bItemVisible[nItem];

		switch (nItem)
		{
			case ITEM_PROGRESS:
			{
				const CRect rectBar(rectClient.left, y, rectClient.right - 16, y + kHeight[ITEM_PROGRESS]);
				const CRect rectPause(rectBar.right, y, rectClient.right, y + kHeight[ITEM_PROGRESS]);

				pProgress->MoveWindow(rectBar);
				pProgress->ShowWindow(bVisible ? SW_SHOW : SW_HIDE);
				pPause->MoveWindow(rectPause);

				y += kHeight[ITEM_PROGRESS];
				break;
			}

			case ITEM_STATUS:
			case ITEM_SUMMARY:
			{
				CWnd *pStatus = (nItem == ITEM_STATUS) ? pStatus1 : pStatus2;

				if (bVisible)
				{
					pStatus->MoveWindow(CRect(rectClient.left, y, rectClient.right, y + kHeight[nItem]));
					pStatus->ShowWindow(SW_SHOW);
					y += kHeight[nItem];
				}
				else
					pStatus->ShowWindow(SW_HIDE);

				break;
			}

			case ITEM_TOTALBAR:
			{
				if (::IsWindow(m_TotalBar.m_hWnd))
				{
					if (bVisible)
					{
						m_TotalBar.MoveWindow(CRect(rectClient.left, y, rectClient.right, y + kHeight[ITEM_TOTALBAR]));
						m_TotalBar.ShowWindow(SW_SHOW);
					}
					else
						m_TotalBar.ShowWindow(SW_HIDE);
				}

				if (bVisible)
					y += kHeight[ITEM_TOTALBAR];

				break;
			}
		}
	}

	pList->Invalidate();
	pProgress->Invalidate();
	pStatus1->Invalidate();
	pStatus2->Invalidate();
	pPause->Invalidate();
	m_TotalBar.Invalidate();
}


BOOL CZipMaxDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	// hidden columns (width 0) must not be opened by dragging/double-clicking their header divider

	const NMHDR *pHdr = (const NMHDR *)lParam;

	if (pHdr != NULL && (pHdr->code == HDN_BEGINTRACKA || pHdr->code == HDN_BEGINTRACKW ||
						 pHdr->code == HDN_DIVIDERDBLCLICKA || pHdr->code == HDN_DIVIDERDBLCLICKW))
	{
		const NMHEADER *pHeader = (const NMHEADER *)lParam;
		CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

		if (pHeader->iItem >= 0 && pHeader->iItem < NUMCOLUMNS && ! pApp->g_view.m_bColumnVisible[pHeader->iItem])
		{
			*pResult = TRUE;
			return TRUE;
		}
	}

	return CDialog::OnNotify(wParam, lParam, pResult);
}


//////////////////////////////////////////////////////////////////////////
// Settings > Language


void CZipMaxDlg::BuildLanguageMenu()
{
	// fills the Language submenu (Settings, position 3) with English + one item per file of the language folder
	// and puts the check mark on the language in use

	CMenu *pMenu = GetMenu();

	if (pMenu == NULL)
		return;

	CMenu *pSettings = pMenu->GetSubMenu(1);
	CMenu *pLanguage = (pSettings != NULL) ? pSettings->GetSubMenu(3) : NULL;

	if (pLanguage == NULL)
		return;

	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

	std::vector<CString> vecDisplay;
	CLang::ListLanguages(pApp->g_strMainFolder, &m_vecLanguageNames, &vecDisplay);

	while (pLanguage->GetMenuItemCount() > 0)
		pLanguage->DeleteMenu(0, MF_BYPOSITION);

	const int nMax = (int)(IDM_LANG_LAST - IDM_LANG_FIRST + 1);

	if ((int)m_vecLanguageNames.size() > nMax)
	{
		m_vecLanguageNames.resize(nMax);
		vecDisplay.resize(nMax);
	}

	int nCurrent = 0;

	for (int i = 0; i < (int)m_vecLanguageNames.size(); i++)
	{
		CString strText = vecDisplay[i];
		strText.Replace(_T("&"), _T("&&"));		// (a & in a menu text would become a keyboard shortcut)

		pLanguage->AppendMenu(MF_STRING, IDM_LANG_FIRST + i, strText);

		if (m_vecLanguageNames[i].CompareNoCase(CLang::GetName()) == 0)
			nCurrent = i;
	}

	pLanguage->CheckMenuRadioItem(IDM_LANG_FIRST, IDM_LANG_FIRST + (UINT)m_vecLanguageNames.size() - 1, IDM_LANG_FIRST + nCurrent, MF_BYCOMMAND);

	DrawMenuBar();
}


void CZipMaxDlg::OnSetLanguage(UINT nID)
{
	if (! CheckNotBusy())
		return;

	const int nIndex = (int)(nID - IDM_LANG_FIRST);

	if (nIndex < 0 || nIndex >= (int)m_vecLanguageNames.size())
		return;

	const CString strName = m_vecLanguageNames[nIndex];

	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

	// 1) remember the choice: "language = <name>" in zipmax.ini (everything else in the file stays as it is)

	const CString strIni = pApp->g_strMainFolder + CLang::Get(IDS_INIFILE);

	CIniFile ini;
	ini.Load(std::wstring((LPCTSTR)strIni));
	ini.SetValue(L"language", std::wstring((LPCTSTR)strName));

	const BOOL bSaved = ini.Save(std::wstring((LPCTSTR)strIni));

	// 2) use it now

	pApp->g_strLanguage = strName;
	CLang::Init(pApp->g_strMainFolder, strName);

	CMisc::bIniFileRead = FALSE;		// zipmax.ini changed: read it again the next time it is needed

	RefreshLanguage();					// menu, column titles, Pause button (also rebuilds this menu)

	if (! bSaved)
		MessageBox(CLang::Get(IDS_OPT_SAVEFAIL) + strIni, CLang::Get(IDS_STRING_TITLE), MB_ICONWARNING | MB_OK);
}
