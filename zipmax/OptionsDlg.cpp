// OptionsDlg.cpp - veja OptionsDlg.h

#include "stdafx.h"
#include "resource.h"
#include "ZipMax.h"
#include "OptionsDlg.h"
#include "IniFile.h"
#include "Language.h"
#include "misc.h"


BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_OPT_LIST, OnListChanged)
	ON_EN_CHANGE(IDC_OPT_VALUE, OnValueChanged)
	ON_CBN_SELCHANGE(IDC_OPT_COMBO, OnComboChanged)
	ON_BN_CLICKED(IDC_OPT_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_OPT_ADDPACKER, OnAddPacker)
	ON_BN_CLICKED(IDC_OPT_REMOVEPACKER, OnRemovePacker)
END_MESSAGE_MAP()


// parametros liga/desliga do zipmax.ini (chave, descricao)
static const struct { const wchar_t *pszKey; UINT nDescId; } kBoolOptions[] =
{
	{ L"work-recursive",		IDS_OPT_WORKRECURSIVE },
	{ L"skip-internzip",		IDS_OPT_SKIPINTERN },
	{ L"logfile",				IDS_OPT_LOGFILE },
	{ L"nobeep",				IDS_OPT_NOBEEP },
	{ L"setcurrenttime",		IDS_OPT_SETCURRENTTIME },
	{ L"showpackerwindow",		IDS_OPT_SHOWPACKERWINDOW },
	{ L"skipreadonly",			IDS_OPT_SKIPREADONLY },
	{ L"skiparchive",			IDS_OPT_SKIPARCHIVE },
	{ L"setcurrentreadonly",	IDS_OPT_SETCURRENTREADONLY }
};

// valores aceitos em thread-priority
static const wchar_t *const kPriorities[] =
{
	L"time_critical", L"highest", L"above_normal", L"normal", L"below_normal", L"lowest", L"idle"
};


COptionsDlg::COptionsDlg(CWnd *pParent)
	: CDialog(COptionsDlg::IDD, pParent), m_bBusy(FALSE), m_nSel(-1)
{
}


void COptionsDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPT_LIST, m_list);
	DDX_Control(pDX, IDC_OPT_COMBO, m_combo);
}


//////////////////////////////////////////////////////////////////////////
// carga das opcoes a partir do zipmax.ini

void COptionsDlg::AddOption(const CString& strKey, OptType type, BOOL bOn, const CString& strValue, UINT nDescId, int nPacker)
{
	OPTION opt;
	opt.strKey = strKey;
	opt.type = type;
	opt.bOn = bOn;
	opt.strValue = strValue;
	opt.nDescId = nDescId;
	opt.nPacker = nPacker;

	m_opts.push_back(opt);
}


void COptionsDlg::LoadOptions()
{
	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();

	m_opts.clear();

	CIniFile ini;
	const BOOL bLoaded = ini.Load(std::wstring((LPCTSTR)(pApp->g_strMainFolder + CLang::Get(IDS_INIFILE))));

	// le o valor ATIVO de uma chave (como esta escrito no arquivo)
	struct Reader
	{
		const CIniFile& ini;
		BOOL bLoaded;

		BOOL Get(const CString& strKey, CString *pstrValue) const
		{
			std::wstring strValue;

			if (bLoaded && ini.Get(std::wstring((LPCTSTR)strKey), &strValue))
			{
				*pstrValue = strValue.c_str();
				return TRUE;
			}

			return FALSE;
		}
	} rd = { ini, bLoaded };

	CString strValue;

	// liga/desliga

	for (int i = 0; i < (int)(sizeof(kBoolOptions) / sizeof(kBoolOptions[0])); i++)
	{
		const BOOL bHas = rd.Get(CString(kBoolOptions[i].pszKey), &strValue);
		AddOption(CString(kBoolOptions[i].pszKey), OPT_BOOL, bHas && strValue == _T("1"), _T(""), kBoolOptions[i].nDescId);
	}

	// prioridade, idioma, 7z.dll

	BOOL bHas = rd.Get(_T("thread-priority"), &strValue);
	AddOption(_T("thread-priority"), OPT_PRIORITY, bHas, bHas ? strValue : CString(_T("normal")), IDS_OPT_PRIORITY);

	bHas = rd.Get(_T("language"), &strValue);
	AddOption(_T("language"), OPT_LANGUAGE, bHas, bHas ? strValue : CString(_T("English")), IDS_OPT_LANGUAGE);

	bHas = rd.Get(_T("sevenzip-dll"), &strValue);
	AddOption(_T("sevenzip-dll"), OPT_PATH, bHas, bHas ? strValue : CString(_T("")), IDS_OPT_SEVENZIPDLL);

	// empacotadores externos (packer-exe-N / packer-cmd-N; o antigo packer-exe sem numero conta como o 1)

	for (int n = 1; n <= MAXEXTERNAL; n++)
	{
		CString strKeyExe, strKeyCmd, strExe, strCmd;
		strKeyExe.Format(_T("packer-exe-%d"), n);
		strKeyCmd.Format(_T("packer-cmd-%d"), n);

		BOOL bHasExe = rd.Get(strKeyExe, &strExe);
		BOOL bHasCmd = rd.Get(strKeyCmd, &strCmd);

		if (! bHasExe && n == 1)
		{
			bHasExe = rd.Get(_T("packer-exe"), &strExe);
			bHasCmd = rd.Get(_T("packer-cmd"), &strCmd);
		}

		if (! bHasExe)
			break;

		AddOption(strKeyExe, OPT_PACKEREXE, TRUE, strExe, IDS_OPT_PACKEREXE, n);
		AddOption(strKeyCmd, OPT_PACKERCMD, TRUE, strCmd, IDS_OPT_PACKERCMD, n);
	}

	if (CountPackers() == 0)
	{
		AddOption(_T("packer-exe-1"), OPT_PACKEREXE, TRUE, _T("7za.exe"), IDS_OPT_PACKEREXE, 1);
		AddOption(_T("packer-cmd-1"), OPT_PACKERCMD, TRUE, _T("a -mpass=4 -mfb=255 -r -y -tzip %1 %2"), IDS_OPT_PACKERCMD, 1);
	}

	// ultimo passo (finalstep)

	bHas = rd.Get(_T("finalstep-exe"), &strValue);
	AddOption(_T("finalstep-exe"), OPT_PATH, bHas, bHas ? strValue : CString(_T("")), IDS_OPT_FINALEXE);

	bHas = rd.Get(_T("finalstep-cmd"), &strValue);
	AddOption(_T("finalstep-cmd"), OPT_TEXT, bHas, bHas ? strValue : CString(_T("%1")), IDS_OPT_FINALCMD);
}


int COptionsDlg::CountPackers() const
{
	int nCount = 0;

	for (size_t i = 0; i < m_opts.size(); i++)
		if (m_opts[i].type == OPT_PACKEREXE)
			nCount++;

	return nCount;
}


int COptionsDlg::LastPackerRow() const
{
	int nRow = -1;

	for (size_t i = 0; i < m_opts.size(); i++)
		if (m_opts[i].type == OPT_PACKERCMD)
			nRow = (int)i;

	return nRow;
}


//////////////////////////////////////////////////////////////////////////
// janela

CString COptionsDlg::DescText(const OPTION& opt) const
{
	if (opt.type == OPT_PACKEREXE || opt.type == OPT_PACKERCMD)
	{
		CString str;
		str.Format(CLang::Get(opt.nDescId), opt.nPacker);
		return str;
	}

	return CLang::Get(opt.nDescId);
}


CString COptionsDlg::ValueText(const OPTION& opt) const
{
	if (opt.type == OPT_BOOL)
		return opt.bOn ? CString(_T("1")) : CString(_T("0"));

	return opt.strValue;
}


BOOL COptionsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	const UINT nControls[] = { IDOK, IDCANCEL, IDC_OPT_VALUELABEL, IDC_OPT_BROWSE, IDC_OPT_ADDPACKER, IDC_OPT_REMOVEPACKER };
	CLang::TranslateDialog(this, IDS_DLG_OPTIONS, nControls, sizeof(nControls) / sizeof(nControls[0]));

	SetDlgItemText(IDC_OPT_NOTE, CLang::Get(IDS_OPT_NOTE));

	m_list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	CRect rect;
	m_list.GetClientRect(&rect);
	const int nTotal = rect.Width() - ::GetSystemMetrics(SM_CXVSCROLL) - 4;
	const int nW0 = nTotal * 54 / 100;
	const int nW1 = nTotal * 28 / 100;

	m_list.InsertColumn(0, CLang::Get(IDS_OPT_COL_SETTING), LVCFMT_LEFT, nW0);
	m_list.InsertColumn(1, CLang::Get(IDS_OPT_COL_VALUE), LVCFMT_LEFT, nW1);
	m_list.InsertColumn(2, CLang::Get(IDS_OPT_COL_KEY), LVCFMT_LEFT, nTotal - nW0 - nW1);

	LoadOptions();
	FillList();
	SelectRow(0);

	return TRUE;
}


void COptionsDlg::FillList()
{
	m_bBusy = TRUE;

	m_list.DeleteAllItems();

	for (int i = 0; i < (int)m_opts.size(); i++)
	{
		const OPTION& opt = m_opts[i];

		m_list.InsertItem(i, DescText(opt));
		m_list.SetItemText(i, 1, ValueText(opt));
		m_list.SetItemText(i, 2, opt.strKey);
		m_list.SetCheck(i, opt.bOn);
	}

	m_bBusy = FALSE;
}


void COptionsDlg::SelectRow(int nRow)
{
	if (nRow < 0 || nRow >= m_list.GetItemCount())
		return;

	m_list.SetItemState(nRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_list.EnsureVisible(nRow, FALSE);
}


void COptionsDlg::UpdateEditor()
{
	if (m_nSel < 0 || m_nSel >= (int)m_opts.size())
		return;

	const OPTION& opt = m_opts[m_nSel];

	const BOOL bCombo = (opt.type == OPT_PRIORITY || opt.type == OPT_LANGUAGE);
	const BOOL bEdit = (opt.type == OPT_TEXT || opt.type == OPT_PATH || opt.type == OPT_PACKEREXE || opt.type == OPT_PACKERCMD);
	const BOOL bBrowse = (opt.type == OPT_PATH || opt.type == OPT_PACKEREXE);

	m_bBusy = TRUE;

	GetDlgItem(IDC_OPT_VALUELABEL)->ShowWindow((bEdit || bCombo) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_OPT_VALUE)->ShowWindow(bEdit ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_OPT_BROWSE)->ShowWindow(bBrowse ? SW_SHOW : SW_HIDE);
	m_combo.ShowWindow(bCombo ? SW_SHOW : SW_HIDE);

	if (bEdit)
		SetDlgItemText(IDC_OPT_VALUE, opt.strValue);

	if (bCombo)
	{
		m_combo.ResetContent();

		if (opt.type == OPT_PRIORITY)
		{
			for (int i = 0; i < (int)(sizeof(kPriorities) / sizeof(kPriorities[0])); i++)
				m_combo.AddString(CString(kPriorities[i]));
		}
		else
		{
			// English (built-in) + the files of the language folder

			m_combo.AddString(_T("English"));

			CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();
			CFileFind find;
			BOOL bMore = find.FindFile(pApp->g_strMainFolder + _T("language\\*.lng"));

			while (bMore)
			{
				bMore = find.FindNextFile();

				if (find.IsDirectory())
					continue;

				const CString strName = find.GetFileTitle();

				if (m_combo.FindStringExact(-1, strName) == CB_ERR)
					m_combo.AddString(strName);
			}

			find.Close();
		}

		int nIndex = m_combo.FindStringExact(-1, opt.strValue);

		if (nIndex == CB_ERR)
			nIndex = m_combo.AddString(opt.strValue);

		m_combo.SetCurSel(nIndex);
	}

	SetDlgItemText(IDC_OPT_HINT, CLang::Get(opt.type == OPT_BOOL ? IDS_OPT_HINT_BOOL : IDS_OPT_HINT_TEXT));

	GetDlgItem(IDC_OPT_REMOVEPACKER)->EnableWindow(CountPackers() > 1);

	m_bBusy = FALSE;
}


//////////////////////////////////////////////////////////////////////////
// notificacoes

void COptionsDlg::OnListChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	if (m_bBusy)
		return;

	const NMLISTVIEW *p = (const NMLISTVIEW *)pNMHDR;

	if (p->iItem < 0 || p->iItem >= (int)m_opts.size() || (p->uChanged & LVIF_STATE) == 0)
		return;

	// linha selecionada

	if ((p->uNewState & LVIS_SELECTED) != 0 && (p->uOldState & LVIS_SELECTED) == 0)
	{
		m_nSel = p->iItem;
		UpdateEditor();
	}

	// caixa marcada/desmarcada (estado 1 = desmarcada, 2 = marcada; 0 = o item acabou de ser criado)

	const UINT nOld = (p->uOldState & LVIS_STATEIMAGEMASK) >> 12;
	const UINT nNew = (p->uNewState & LVIS_STATEIMAGEMASK) >> 12;

	if (nOld != 0 && nNew != 0 && nOld != nNew)
	{
		const BOOL bOn = (nNew == 2);
		OPTION& opt = m_opts[p->iItem];

		opt.bOn = bOn;

		if (opt.type == OPT_BOOL)
			m_list.SetItemText(p->iItem, 1, ValueText(opt));

		// o programa e a linha de comando de um empacotador ligam/desligam juntos

		if (opt.type == OPT_PACKEREXE || opt.type == OPT_PACKERCMD)
		{
			const int nPartner = (opt.type == OPT_PACKEREXE) ? p->iItem + 1 : p->iItem - 1;

			if (nPartner >= 0 && nPartner < (int)m_opts.size())
			{
				m_bBusy = TRUE;
				m_opts[nPartner].bOn = bOn;
				m_list.SetCheck(nPartner, bOn);
				m_bBusy = FALSE;
			}
		}
	}
}


void COptionsDlg::OnValueChanged()
{
	if (m_bBusy || m_nSel < 0 || m_nSel >= (int)m_opts.size())
		return;

	CString str;
	GetDlgItemText(IDC_OPT_VALUE, str);

	m_opts[m_nSel].strValue = str;
	m_list.SetItemText(m_nSel, 1, str);
}


void COptionsDlg::OnComboChanged()
{
	if (m_bBusy || m_nSel < 0 || m_nSel >= (int)m_opts.size())
		return;

	const int nIndex = m_combo.GetCurSel();

	if (nIndex == CB_ERR)
		return;

	CString str;
	m_combo.GetLBText(nIndex, str);

	m_opts[m_nSel].strValue = str;
	m_list.SetItemText(m_nSel, 1, str);
}


void COptionsDlg::OnBrowse()
{
	if (m_nSel < 0 || m_nSel >= (int)m_opts.size())
		return;

	CFileDialog dlg(TRUE, NULL, m_opts[m_nSel].strValue, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, CLang::Get(IDS_OPT_BROWSE_FILTER), this);

	if (dlg.DoModal() == IDOK)
		SetDlgItemText(IDC_OPT_VALUE, dlg.GetPathName());		// (o EN_CHANGE guarda o valor)
}


void COptionsDlg::OnAddPacker()
{
	const int nNew = CountPackers() + 1;

	if (nNew > MAXEXTERNAL)
		return;

	CString strKeyExe, strKeyCmd;
	strKeyExe.Format(_T("packer-exe-%d"), nNew);
	strKeyCmd.Format(_T("packer-cmd-%d"), nNew);

	OPTION exe, cmd;
	exe.strKey = strKeyExe; exe.type = OPT_PACKEREXE; exe.bOn = TRUE; exe.strValue = _T("7za.exe"); exe.nDescId = IDS_OPT_PACKEREXE; exe.nPacker = nNew;
	cmd.strKey = strKeyCmd; cmd.type = OPT_PACKERCMD; cmd.bOn = TRUE; cmd.strValue = _T("a -mx -mpass=4 -r -y -tzip %1 %2"); cmd.nDescId = IDS_OPT_PACKERCMD; cmd.nPacker = nNew;

	const int nAt = LastPackerRow() + 1;

	m_opts.insert(m_opts.begin() + nAt, cmd);
	m_opts.insert(m_opts.begin() + nAt, exe);

	FillList();
	m_nSel = -1;
	SelectRow(nAt);
}


void COptionsDlg::OnRemovePacker()
{
	if (CountPackers() <= 1)
		return;

	const int nLast = LastPackerRow();

	if (nLast < 1)
		return;

	m_opts.erase(m_opts.begin() + (nLast - 1), m_opts.begin() + nLast + 1);

	FillList();
	m_nSel = -1;
	SelectRow(nLast - 2);
}


//////////////////////////////////////////////////////////////////////////
// confirmar: validar, gravar no zipmax.ini e recarregar

BOOL COptionsDlg::ValidateOptions()
{
	const CString strTitle = CLang::Get(IDS_DLG_OPTIONS);
	int nValid = 0;

	for (size_t i = 0; i < m_opts.size(); i++)
	{
		const OPTION& opt = m_opts[i];

		if (opt.type == OPT_PACKEREXE && opt.bOn && i + 1 < m_opts.size())
		{
			CString strExe = opt.strValue;
			CString strCmd = m_opts[i + 1].strValue;
			strExe.Trim();
			strCmd.Trim();

			if (strExe.IsEmpty() || strCmd.IsEmpty() || strCmd.Find(_T("%1")) == -1 || strCmd.Find(_T("%2")) == -1)
			{
				MessageBox(CLang::Get(IDS_OPT_NEEDPACKER), strTitle, MB_ICONWARNING | MB_OK);
				return FALSE;
			}

			if (! CMisc::FileExists(strExe))
			{
				MessageBox(CLang::Get(IDS_CANTDETECT) + strExe, strTitle, MB_ICONWARNING | MB_OK);
				return FALSE;
			}

			nValid++;
		}

		if (opt.strKey == _T("finalstep-exe") && opt.bOn)
		{
			CString strExe = opt.strValue;
			strExe.Trim();

			if (! strExe.IsEmpty() && ! CMisc::FileExists(strExe))
			{
				MessageBox(CLang::Get(IDS_CANTDETECT) + strExe, strTitle, MB_ICONWARNING | MB_OK);
				return FALSE;
			}
		}
	}

	if (nValid == 0)
	{
		MessageBox(CLang::Get(IDS_OPT_NEEDPACKER), strTitle, MB_ICONWARNING | MB_OK);
		return FALSE;
	}

	return TRUE;
}


static void AddIniLine(std::vector<std::wstring> *pvec, const CString& strKey, const CString& strValue, BOOL bActive)
{
	std::wstring str;

	if (! bActive)
		str += L"// ";

	str += (LPCTSTR)strKey;
	str += L" = ";
	str += (LPCTSTR)strValue;

	pvec->push_back(str);
}


void COptionsDlg::BuildBlock(std::vector<std::wstring> *pvecBlock) const
{
	pvecBlock->clear();

	CString strCmd, strExe, strKeyExe, strKeyCmd;

	// 1) tudo, menos os empacotadores, na ordem da lista

	for (size_t i = 0; i < m_opts.size(); i++)
	{
		const OPTION& opt = m_opts[i];

		if (opt.type == OPT_PACKEREXE || opt.type == OPT_PACKERCMD)
			continue;

		CString strValue = opt.strValue;
		strValue.Trim();

		if (opt.type == OPT_BOOL)
			AddIniLine(pvecBlock, opt.strKey, opt.bOn ? CString(_T("1")) : CString(_T("0")), TRUE);
		else
			AddIniLine(pvecBlock, opt.strKey, strValue, opt.bOn && ! strValue.IsEmpty());		// desligado ou vazio = comentado
	}

	// 2) empacotadores ligados, renumerados 1..n (o ZipMax le a sequencia ate faltar um numero)

	int nNumber = 0;

	for (size_t i = 0; i + 1 < m_opts.size(); i++)
	{
		if (m_opts[i].type != OPT_PACKEREXE || ! m_opts[i].bOn)
			continue;

		nNumber++;
		strKeyExe.Format(_T("packer-exe-%d"), nNumber);
		strKeyCmd.Format(_T("packer-cmd-%d"), nNumber);

		strExe = m_opts[i].strValue;
		strCmd = m_opts[i + 1].strValue;
		strExe.Trim();
		strCmd.Trim();

		AddIniLine(pvecBlock, strKeyExe, strExe, TRUE);
		AddIniLine(pvecBlock, strKeyCmd, strCmd, TRUE);
	}

	// 3) empacotadores desligados ficam comentados (nao se perdem)

	for (size_t i = 0; i + 1 < m_opts.size(); i++)
	{
		if (m_opts[i].type != OPT_PACKEREXE || m_opts[i].bOn)
			continue;

		nNumber++;
		strKeyExe.Format(_T("packer-exe-%d"), nNumber);
		strKeyCmd.Format(_T("packer-cmd-%d"), nNumber);

		AddIniLine(pvecBlock, strKeyExe, m_opts[i].strValue, FALSE);
		AddIniLine(pvecBlock, strKeyCmd, m_opts[i + 1].strValue, FALSE);
	}
}


BOOL COptionsDlg::SaveIni()
{
	CZipMaxApp *pApp = (CZipMaxApp *)AfxGetApp();
	const CString strIni = pApp->g_strMainFolder + CLang::Get(IDS_INIFILE);

	CIniFile ini;
	ini.Load(std::wstring((LPCTSTR)strIni));		// (se ainda nao existir, comeca vazio)

	std::vector<std::wstring> vecBlock;
	BuildBlock(&vecBlock);
	ini.ReplaceManagedBlock(vecBlock);

	if (! ini.Save(std::wstring((LPCTSTR)strIni)))
	{
		MessageBox(CLang::Get(IDS_OPT_SAVEFAIL) + strIni, CLang::Get(IDS_DLG_OPTIONS), MB_ICONWARNING | MB_OK);
		return FALSE;
	}

	return TRUE;
}


void COptionsDlg::OnOK()
{
	for (int i = 0; i < (int)m_opts.size() && i < m_list.GetItemCount(); i++)
		m_opts[i].bOn = m_list.GetCheck(i) ? TRUE : FALSE;

	if (! ValidateOptions())
		return;

	if (! SaveIni())
		return;

	((CZipMaxApp *)AfxGetApp())->ReloadSettings();

	CDialog::OnOK();
}
