// AboutDlg.cpp - veja AboutDlg.h

#include "stdafx.h"
#include "resource.h"
#include "AboutDlg.h"
#include "Language.h"
#include "Version.h"


CAboutDlg::CAboutDlg(CWnd *pParent)
	: CDialog(CAboutDlg::IDD, pParent)
{
}


BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	const UINT nControls[] = { IDOK };
	CLang::TranslateDialog(this, IDS_DLG_ABOUT, nControls, 1);

	CString strText = CString(_T("ZipMax Reborn ")) + ZIPMAX_REBORN_VERSION + _T("\n\n") + CLang::Get(IDS_ABOUT_TEXT);
	strText.Replace(_T("\n"), _T("\r\n"));

	SetDlgItemText(IDC_ABOUT_TEXT, strText);

	CEdit *pEdit = (CEdit *)GetDlgItem(IDC_ABOUT_TEXT);

	if (pEdit != NULL)
		pEdit->SetSel(0, 0);			// (sem o texto todo selecionado)

	GetDlgItem(IDOK)->SetFocus();

	return FALSE;						// o foco foi definido acima
}
