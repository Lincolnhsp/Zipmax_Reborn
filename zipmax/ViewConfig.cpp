// ViewConfig.cpp - veja ViewConfig.h

#include "stdafx.h"
#include "ViewConfig.h"
#include "resource.h"


CViewConfig::CViewConfig()
{
	SetDefaults();
}


void CViewConfig::SetDefaults()
{
	// visual original do ZipMax

	for (int i = 0; i < NUMCOLUMNS; i++)
	{
		m_bColumnVisible[i] = (i == COL_FILE || i == COL_STATUS || i == COL_GAINED);
		m_nColumnOrder[i] = i;
	}

	for (int i = 0; i < NUMITEMS; i++)
	{
		m_bItemVisible[i] = (i != ITEM_TOTALBAR);
		m_nItemOrder[i] = i;
	}
}


UINT CViewConfig::ColumnNameId(int nColumn)
{
	switch (nColumn)
	{
		case COL_FILE:			return IDS_FILE;
		case COL_STATUS:		return IDS_STATUS;
		case COL_SIZEBEFORE:	return IDS_SIZEBEFORE;
		case COL_SIZEAFTER:		return IDS_SIZEAFTER;
		case COL_INFO:			return IDS_METHODINFO;
		case COL_GAINED:		return IDS_GAINEDBYTES;
	}

	return 0;
}


UINT CViewConfig::ItemNameId(int nItem)
{
	switch (nItem)
	{
		case ITEM_PROGRESS:		return IDS_VI_PROGRESS;
		case ITEM_STATUS:		return IDS_VI_STATUS;
		case ITEM_SUMMARY:		return IDS_VI_SUMMARY;
		case ITEM_TOTALBAR:		return IDS_VI_TOTALBAR;
	}

	return 0;
}


BOOL CViewConfig::IsValidOrder(const int *pnOrder, int nCount)
{
	BOOL bSeen[NUMCOLUMNS > NUMITEMS ? NUMCOLUMNS : NUMITEMS];

	if (nCount < 1 || nCount > (int)(sizeof(bSeen) / sizeof(bSeen[0])))
		return FALSE;

	for (int i = 0; i < nCount; i++)
		bSeen[i] = FALSE;

	for (int i = 0; i < nCount; i++)
	{
		if (pnOrder[i] < 0 || pnOrder[i] >= nCount || bSeen[pnOrder[i]])
			return FALSE;

		bSeen[pnOrder[i]] = TRUE;
	}

	return TRUE;
}


CString CViewConfig::OrderToString(const int *pnOrder, int nCount)
{
	CString str, strNum;

	for (int i = 0; i < nCount; i++)
	{
		strNum.Format(_T("%d"), pnOrder[i]);

		if (i > 0)
			str += _T(",");

		str += strNum;
	}

	return str;
}


BOOL CViewConfig::StringToOrder(const CString& str, int *pnOrder, int nCount)
{
	int nTemp[NUMCOLUMNS > NUMITEMS ? NUMCOLUMNS : NUMITEMS];

	if (nCount < 1 || nCount > (int)(sizeof(nTemp) / sizeof(nTemp[0])))
		return FALSE;

	int nPos = 0;
	int nFound = 0;

	CString strPart = str.Tokenize(_T(","), nPos);

	while (nPos != -1 || ! strPart.IsEmpty())
	{
		if (nFound >= nCount)
			return FALSE;

		nTemp[nFound++] = _ttoi(strPart);

		if (nPos == -1)
			break;

		strPart = str.Tokenize(_T(","), nPos);
	}

	if (nFound != nCount || ! IsValidOrder(nTemp, nCount))
		return FALSE;

	for (int i = 0; i < nCount; i++)
		pnOrder[i] = nTemp[i];

	return TRUE;
}


void CViewConfig::Load(CWinApp *pApp)
{
	SetDefaults();

	int nColMask = 0;
	int nItemMask = 0;

	for (int i = 0; i < NUMCOLUMNS; i++)
		if (m_bColumnVisible[i])
			nColMask |= (1 << i);

	for (int i = 0; i < NUMITEMS; i++)
		if (m_bItemVisible[i])
			nItemMask |= (1 << i);

	nColMask = pApp->GetProfileInt(_T("View"), _T("ColumnVisible"), nColMask);
	nItemMask = pApp->GetProfileInt(_T("View"), _T("ItemVisible"), nItemMask);

	for (int i = 0; i < NUMCOLUMNS; i++)
		m_bColumnVisible[i] = ((nColMask & (1 << i)) != 0);

	for (int i = 0; i < NUMITEMS; i++)
		m_bItemVisible[i] = ((nItemMask & (1 << i)) != 0);

	m_bColumnVisible[COL_FILE] = TRUE;				// sempre visivel

	const CString strColOrder = pApp->GetProfileString(_T("View"), _T("ColumnOrder"), OrderToString(m_nColumnOrder, NUMCOLUMNS));
	const CString strItemOrder = pApp->GetProfileString(_T("View"), _T("ItemOrder"), OrderToString(m_nItemOrder, NUMITEMS));

	StringToOrder(strColOrder, m_nColumnOrder, NUMCOLUMNS);		// (se invalido, fica o padrao)
	StringToOrder(strItemOrder, m_nItemOrder, NUMITEMS);
}


void CViewConfig::Save(CWinApp *pApp) const
{
	int nColMask = 0;
	int nItemMask = 0;

	for (int i = 0; i < NUMCOLUMNS; i++)
		if (m_bColumnVisible[i])
			nColMask |= (1 << i);

	for (int i = 0; i < NUMITEMS; i++)
		if (m_bItemVisible[i])
			nItemMask |= (1 << i);

	pApp->WriteProfileInt(_T("View"), _T("ColumnVisible"), nColMask);
	pApp->WriteProfileInt(_T("View"), _T("ItemVisible"), nItemMask);
	pApp->WriteProfileString(_T("View"), _T("ColumnOrder"), OrderToString(m_nColumnOrder, NUMCOLUMNS));
	pApp->WriteProfileString(_T("View"), _T("ItemOrder"), OrderToString(m_nItemOrder, NUMITEMS));
}
