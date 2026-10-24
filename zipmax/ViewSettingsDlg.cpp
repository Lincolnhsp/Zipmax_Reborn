// ViewSettingsDlg.cpp - veja ViewSettingsDlg.h

#include "stdafx.h"
#include "resource.h"
#include "ViewSettingsDlg.h"
#include "Language.h"


BEGIN_MESSAGE_MAP(CViewSettingsDlg, CDialog)
	ON_BN_CLICKED(IDC_VIEW_COL_UP, OnColUp)
	ON_BN_CLICKED(IDC_VIEW_COL_DOWN, OnColDown)
	ON_BN_CLICKED(IDC_VIEW_ITEM_UP, OnItemUp)
	ON_BN_CLICKED(IDC_VIEW_ITEM_DOWN, OnItemDown)
	ON_BN_CLICKED(IDC_VIEW_DEFAULTS, OnDefaults)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_VIEW_COLS, OnColsChanged)
END_MESSAGE_MAP()


CViewSettingsDlg::CViewSettingsDlg(const CViewConfig& cfg, CWnd *pParent)
	: CDialog(CViewSettingsDlg::IDD, pParent), m_cfg(cfg), m_bBusy(FALSE)
{
}


void CViewSettingsDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VIEW_COLS, m_listCols);
	DDX_Control(pDX, IDC_VIEW_ITEMS, m_listItems);
}


void CViewSettingsDlg::PrepareList(CListCtrl& list)
{
	list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

	CRect rect;
	list.GetClientRect(&rect);
	list.InsertColumn(0, _T(""), LVCFMT_LEFT, rect.Width() - 2);
}


BOOL CViewSettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	const UINT nControls[] = { IDOK, IDCANCEL, IDC_VIEW_COL_UP, IDC_VIEW_COL_DOWN, IDC_VIEW_ITEM_UP, IDC_VIEW_ITEM_DOWN, IDC_VIEW_DEFAULTS };
	CLang::TranslateDialog(this, IDS_DLG_VIEW, nControls, sizeof(nControls) / sizeof(nControls[0]));

	SetDlgItemText(IDC_VIEW_COLSLABEL, CLang::Get(IDS_VIEW_COLUMNS));
	SetDlgItemText(IDC_VIEW_ITEMSLABEL, CLang::Get(IDS_VIEW_ITEMS));
	SetDlgItemText(IDC_VIEW_HINT, CLang::Get(IDS_VIEW_HINT));

	PrepareList(m_listCols);
	PrepareList(m_listItems);

	Fill();

	return TRUE;
}


void CViewSettingsDlg::Fill()
{
	m_bBusy = TRUE;

	m_listCols.DeleteAllItems();
	m_listItems.DeleteAllItems();

	for (int nPos = 0; nPos < NUMCOLUMNS; nPos++)
	{
		const int nCol = m_cfg.m_nColumnOrder[nPos];

		const int nRow = m_listCols.InsertItem(nPos, CLang::Get(CViewConfig::ColumnNameId(nCol)));
		m_listCols.SetItemData(nRow, (DWORD_PTR)nCol);
		m_listCols.SetCheck(nRow, m_cfg.m_bColumnVisible[nCol]);
	}

	for (int nPos = 0; nPos < NUMITEMS; nPos++)
	{
		const int nItem = m_cfg.m_nItemOrder[nPos];

		const int nRow = m_listItems.InsertItem(nPos, CLang::Get(CViewConfig::ItemNameId(nItem)));
		m_listItems.SetItemData(nRow, (DWORD_PTR)nItem);
		m_listItems.SetCheck(nRow, m_cfg.m_bItemVisible[nItem]);
	}

	m_listCols.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_listItems.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	m_bBusy = FALSE;
}


void CViewSettingsDlg::Collect()
{
	for (int nPos = 0; nPos < NUMCOLUMNS; nPos++)
	{
		const int nCol = (int)m_listCols.GetItemData(nPos);
		m_cfg.m_nColumnOrder[nPos] = nCol;
		m_cfg.m_bColumnVisible[nCol] = m_listCols.GetCheck(nPos) ? TRUE : FALSE;
	}

	for (int nPos = 0; nPos < NUMITEMS; nPos++)
	{
		const int nItem = (int)m_listItems.GetItemData(nPos);
		m_cfg.m_nItemOrder[nPos] = nItem;
		m_cfg.m_bItemVisible[nItem] = m_listItems.GetCheck(nPos) ? TRUE : FALSE;
	}

	m_cfg.m_bColumnVisible[COL_FILE] = TRUE;		// a coluna Arquivo e sempre visivel
}


void CViewSettingsDlg::OnOK()
{
	Collect();
	CDialog::OnOK();
}


void CViewSettingsDlg::MoveSelected(CListCtrl& list, int nDelta)
{
	// troca a linha selecionada com a vizinha (texto, caixa de selecao e dado)

	POSITION pos = list.GetFirstSelectedItemPosition();

	if (pos == NULL)
		return;

	const int i = list.GetNextSelectedItem(pos);
	const int j = i + nDelta;

	if (j < 0 || j >= list.GetItemCount())
		return;

	m_bBusy = TRUE;

	const CString strI = list.GetItemText(i, 0);
	const CString strJ = list.GetItemText(j, 0);
	const DWORD_PTR dataI = list.GetItemData(i);
	const DWORD_PTR dataJ = list.GetItemData(j);
	const BOOL bCheckI = list.GetCheck(i);
	const BOOL bCheckJ = list.GetCheck(j);

	list.SetItemText(i, 0, strJ);
	list.SetItemData(i, dataJ);
	list.SetCheck(i, bCheckJ);

	list.SetItemText(j, 0, strI);
	list.SetItemData(j, dataI);
	list.SetCheck(j, bCheckI);

	list.SetItemState(i, 0, LVIS_SELECTED | LVIS_FOCUSED);
	list.SetItemState(j, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	list.EnsureVisible(j, FALSE);

	m_bBusy = FALSE;
}


void CViewSettingsDlg::OnColUp()	{ MoveSelected(m_listCols, -1); }
void CViewSettingsDlg::OnColDown()	{ MoveSelected(m_listCols, +1); }
void CViewSettingsDlg::OnItemUp()	{ MoveSelected(m_listItems, -1); }
void CViewSettingsDlg::OnItemDown()	{ MoveSelected(m_listItems, +1); }


void CViewSettingsDlg::OnDefaults()
{
	m_cfg.SetDefaults();
	Fill();
}


void CViewSettingsDlg::OnColsChanged(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;

	if (m_bBusy)
		return;

	// a coluna Arquivo nao pode ser escondida: se desmarcarem, marca de novo

	for (int nRow = 0; nRow < m_listCols.GetItemCount(); nRow++)
	{
		if ((int)m_listCols.GetItemData(nRow) == COL_FILE && ! m_listCols.GetCheck(nRow))
		{
			m_bBusy = TRUE;
			m_listCols.SetCheck(nRow, TRUE);
			m_bBusy = FALSE;
		}
	}
}
