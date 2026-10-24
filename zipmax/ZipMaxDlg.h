// ZipMaxDlg.h : header file
//

#pragma once


#include <vector>
#include <string>
#include "Zip7Engine.h"
#include "GreenProgress.h"
#include "ViewConfig.h"


//////////////////////////////////////////////////////////////////////////

// (the columns are COL_FILE ... COL_GAINED and the bottom items ITEM_PROGRESS ... ITEM_TOTALBAR, see ViewConfig.h)


class CZipEntry		// used for single zipfile entries
{
	public:

		CZipEntry();


	public:

		const Zip7::ZipHeader *m_pHeader;		// headerinformation for current entry
		CString m_strArchiveName;		// zipfilename
		CString m_strEntryName;			// current fileentryname (only used for sorting)


	public:

		// operator used for sorting entries
		bool operator < (const CZipEntry& entry) const;
};



//////////////////////////////////////////////////////////////////////////


class CZipMaxDlg : public CDialog
{
	public:

		CZipMaxDlg(CWnd* pParent = NULL);

		enum { IDD = IDD_ZIPMAX_DIALOG };

	public:

		typedef struct tagThreadParam			// parameters for threadfunction
		{
			DWORD nDoneTime;
			UINT64 nDoneSize;
			UINT64 nRemainingSize;				// remaining size (for statistics only)
			std::vector<CString> *pvecFiles;	// pointer to the filelist
			CString *pstrMainFolder;			// pointer to the zipmax mainfolder name
			CStatic *pStatus1;					// pointer to controls (1x progress + 2x status + 1x list)
			CStatic *pStatus2;
			CProgressCtrl *pProgressCtrl;
			CListCtrl *pListCtrl;
			CGreenProgress *pTotalBar;			// green total progress bar
			UINT64 nTotalSize;					// sum of the sizes of all zipfiles (weight of the total progress)
		} THREADPARAM;

	private:

		typedef struct							// Used for sorting the columns in the listview.
		{
			CListCtrl *pListCtrl;
			int nColumn;
			int nSortDirection;
		} SORTINFO;

		std::vector<CString> m_vecFiles;		// the filelist
		CWinThread *m_pThread;					// pointer to started thread
		THREADPARAM m_param;					// thread parameter block

		CListCtrl *m_pListCtrl;					// pointers to the controls
		CStatic *m_pStatus1;
		CStatic *m_pStatus2;
		CProgressCtrl *m_pProgCtrl;
		std::vector<CString> m_vecLanguageNames;	// language file names, same order as the items of the Language menu
		CGreenProgress m_TotalBar;				// green progress bar with the percentage inside
		CButton *m_pButtonPause;

		int m_nSortDirection;
		int m_nLastColumn;
		CImageList m_ImageList;
		CBitmap m_bmStatus[3];
		CBitmap m_bmArrowUp;
		CBitmap m_bmArrowDown;

	private:

		void OnCancel();
		void StartZipping ();

		// file list handling (drag&drop, command line and the File menu all end up here)
		void AddFilesAndStart(const std::vector<CString>& vecFiles);
		void ResetSortArrow();
		BOOL CheckNotBusy();

		// the window: menu, language, which columns/items are shown and where (see ViewConfig.h)
		void ApplyMenuLanguage();
		void BuildLanguageMenu();			// Settings > Language: one item per language file
		void RefreshLanguage();
		void ApplyView();
		void LayoutControls();
		void SaveColumnState();
		void GetColumnInfo (int nColumn, LVCOLUMN *pcolumn, CString &str);
		static int CALLBACK CompareProc (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);


		// static because of threading....

		static int GetThreadPriority();

		static UINT WorkThread(LPVOID pParam);	// the main thread routine

		static BOOL WriteLFH(CStdioFile *pFile, CZipEntry *pZipEntry, DWORD *pOffset, CString *pName);
		static BOOL WriteCFH(CStdioFile *pFile, CZipEntry *pZipEntry, const DWORD nOffset);
		static BOOL WriteEOZ(CStdioFile *pFile, const DWORD nCentralDirStart, const DWORD nCentralDirSize, const DWORD nEntries);

		static BOOL InternalZipFiles(const CString& strArchiveName, const CString& strSourceFolder, std::vector<Zip7::ZipHeader> *pvecHeaders);
		static BOOL ExternalZipFiles(const CString& strArchiveName, const CString& strSourceFolder, std::vector<Zip7::ZipHeader> *pvecHeaders, const DWORD nExternal);
		static BOOL UnzipAllFiles(const CString& strArchiveName, const CString& strDestinationFolder, std::vector<Zip7::ZipHeader> *pvecHeaders);


		static BOOL GetHeaders(const CString& strArchiveName, std::vector<Zip7::ZipHeader> *pvecHeaders);
		static BOOL BuildNewZip(const CString& strArchiveName, std::vector<CZipEntry> *pvecEntries);
		static BOOL TestNewZip(const CString& strArchiveName);
		static BOOL SafeRename(const CString& strSource);

		static void UpdateList(CListCtrl *pListCtrl, const DWORD nIndex, const CString& strFile, const CString& strStatus, const CString& strBefore, const CString& strAfter, const CString& strInfo, const CString& strGain);
		static CString BuildArchiveInfo(const CString& strArchiveName);
		static void ReportProgress(THREADPARAM *p, const DWORD nIndex, const UINT64 nCurrentSize, const double dFraction);
		static void UpdateStatus(CStatic *pStatus, const DWORD nIndex = 0, const DWORD nTotalFiles = 0, const DWORD nCurrentTotal = 0, const DWORD nRemainingSecs = 0, const DWORD nDoneSize = 0);
		static void WriteLog(CFile *pFile, const CString& str);
		static void SetLastZipError(const std::wstring& str);

		static CString s_strLastZipError;		// last error text reported by the 7-Zip engine (for the logfile)


	protected:

		HICON m_hIcon;

	protected:

		virtual void DoDataExchange(CDataExchange* pDX);
		virtual BOOL OnInitDialog();
		afx_msg void OnPaint();
		afx_msg HCURSOR OnQueryDragIcon();
		afx_msg void OnDropFiles(HDROP hDropInfo);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnColumnClick (NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnBnClickedButtonPause();
		afx_msg void OnInitMenuPopup(CMenu *pPopupMenu, UINT nIndex, BOOL bSysMenu);

		// menu commands
		afx_msg void OnFileAddFiles();
		afx_msg void OnFileAddFolder();
		afx_msg void OnFileAddFolderSub();
		afx_msg void OnFileClearList();
		afx_msg void OnFileExit();
		afx_msg void OnSetOptions();
		afx_msg void OnSetView();
		afx_msg void OnHelpAbout();
		afx_msg void OnSetLanguage(UINT nID);

		virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult);

		DECLARE_MESSAGE_MAP()
};
