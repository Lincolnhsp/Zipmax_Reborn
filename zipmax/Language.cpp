// Language.cpp

#include "stdafx.h"
#include "Language.h"
#include "LangIds.h"
#include "misc.h"
#include "resource.h"


std::map<UINT, CString> CLang::s_mapStrings;
CString CLang::s_strName = _T("English");


static CString BuiltIn(UINT nID)
{
	CString s;
	s.LoadString(nID);
	return s;
}


// Assinatura dos %-codigos de uma string (ex.: "External (%u of %u)" -> "uu").
// Uma traducao so e aceita se tiver a mesma assinatura do ingles: um %d/%s a mais ou a menos
// faria o CString::Format() travar o programa.
static CString FormatSignature(const CString& str)
{
	CString sig;
	const int n = str.GetLength();

	for (int i = 0; i < n; i++)
	{
		if (str[i] != _T('%'))
			continue;

		i++;

		if (i >= n)
			break;

		if (str[i] == _T('%'))
		{
			sig += _T('%');
			continue;
		}

		while (i < n && _tcschr(_T("-+ #0123456789."), str[i]) != NULL)
			i++;

		while (i < n && _tcschr(_T("hlLIz"), str[i]) != NULL)
			i++;

		if (i < n)
			sig += str[i];
	}

	return sig;
}


UINT CLang::FindId(const CString& strKey)
{
	for (int i = 0; i < (int)(sizeof(g_LangIds) / sizeof(g_LangIds[0])); i++)
		if (strKey.CompareNoCase(CString(g_LangIds[i].pszName)) == 0)
			return g_LangIds[i].nID;

	return 0;
}


BOOL CLang::ParseFile(const CString& strFile, std::map<UINT, CString> *pmapOut)
{
	// o arquivo e lido como Unicode (UTF-8 com ou sem BOM, ou UTF-16)

	CString strData;

	if (! CMisc::ReadTextFile(strFile, &strData))
		return FALSE;

	int nStart = 0;
	const int nLength = strData.GetLength();

	while (nStart < nLength)
	{
		int nEnd = strData.Find(_T('\n'), nStart);

		if (nEnd < 0)
			nEnd = nLength;

		CString strLine = strData.Mid(nStart, nEnd - nStart);
		nStart = nEnd + 1;

		strLine.TrimRight(_T("\r \t"));
		strLine.TrimLeft(_T(" \t"));

		if (strLine.IsEmpty() || strLine[0] == _T(';') || strLine[0] == _T('#') || strLine.Left(2) == _T("//"))
			continue;

		const int nEq = strLine.Find(_T('='));

		if (nEq < 0)
			continue;

		CString strKey = strLine.Left(nEq);
		strKey.TrimRight(_T(" \t"));

		const UINT nID = FindId(strKey);

		if (nID == 0)
			continue;							// chave desconhecida: ignora

		CString strRest = strLine.Mid(nEq + 1);
		strRest.TrimLeft(_T(" \t"));

		CString strValue;

		if (! strRest.IsEmpty() && strRest[0] == _T('"'))
		{
			// texto entre aspas, com \" \\ \n \t
			BOOL bClosed = FALSE;
			int nVal = 1;

			while (nVal < strRest.GetLength())
			{
				const TCHAR c = strRest[nVal++];

				if (c == _T('\\') && nVal < strRest.GetLength())
				{
					const TCHAR e = strRest[nVal++];
					strValue += (e == _T('n')) ? _T('\n') : (e == _T('t')) ? _T('\t') : e;
				}
				else if (c == _T('"'))
				{
					bClosed = TRUE;
					break;
				}
				else
					strValue += c;
			}

			if (! bClosed)
				continue;
		}
		else
			strValue = strRest;

		// a traducao precisa ter os mesmos %-codigos do ingles embutido (quando ha ingles embutido)
		const CString strEnglish = BuiltIn(nID);

		if (! strEnglish.IsEmpty() && FormatSignature(strValue) != FormatSignature(strEnglish))
			continue;

		(*pmapOut)[nID] = strValue;
	}

	return TRUE;
}


BOOL CLang::LoadFile(const CString& strFile)
{
	return ParseFile(strFile, &s_mapStrings);
}


void CLang::ListLanguages(const CString& strMainFolder, std::vector<CString> *pvecNames, std::vector<CString> *pvecDisplay)
{
	pvecNames->clear();
	pvecDisplay->clear();

	// the built-in English is always there

	pvecNames->push_back(_T("English"));
	pvecDisplay->push_back(_T("English"));

	CFileFind find;
	BOOL bMore = find.FindFile(strMainFolder + _T("language\\*.lng"));

	while (bMore)
	{
		bMore = find.FindNextFile();

		if (find.IsDirectory())
			continue;

		const CString strName = find.GetFileTitle();

		if (strName.CompareNoCase(_T("English")) == 0)
			continue;

		CString strDisplay = strName;
		std::map<UINT, CString> mapFile;

		if (ParseFile(find.GetFilePath(), &mapFile))
		{
			const std::map<UINT, CString>::const_iterator it = mapFile.find(IDS_LANGNAME);

			if (it != mapFile.end() && ! it->second.IsEmpty())
				strDisplay = it->second;
		}

		pvecNames->push_back(strName);
		pvecDisplay->push_back(strDisplay);
	}

	find.Close();
}


void CLang::Init(const CString& strMainFolder, const CString& strLanguage)
{
	s_mapStrings.clear();
	s_strName = _T("English");

	CString strName = strLanguage;
	strName.Trim();

	if (strName.CompareNoCase(_T("auto")) == 0)
	{
		// idioma de exibicao do Windows: portugues -> Portugues-Brasil
		const LANGID id = ::GetUserDefaultUILanguage();
		strName = (PRIMARYLANGID(id) == LANG_PORTUGUESE) ? _T("Portugues-Brasil") : _T("English");
	}

	if (strName.IsEmpty() || strName.CompareNoCase(_T("English")) == 0)
		return;									// ingles embutido

	// o nome nao pode conter caminhos
	if (strName.FindOneOf(_T("\\/:")) != -1)
		return;

	if (LoadFile(strMainFolder + _T("language\\") + strName + _T(".lng")))
		s_strName = strName;
}


CString CLang::Get(UINT nID)
{
	const std::map<UINT, CString>::const_iterator it = s_mapStrings.find(nID);

	if (it != s_mapStrings.end())
		return it->second;

	return BuiltIn(nID);
}


BOOL CLang::TryGet(UINT nID, CString *pstrText)
{
	const std::map<UINT, CString>::const_iterator it = s_mapStrings.find(nID);

	if (it == s_mapStrings.end())
		return FALSE;

	*pstrText = it->second;
	return TRUE;
}


void CLang::TranslateDialog(CWnd *pDlg, UINT nCaptionId, const UINT *pnControlIds, int nCount)
{
	if (nCaptionId != 0)
		pDlg->SetWindowText(Get(nCaptionId));

	for (int i = 0; i < nCount; i++)
	{
		CString str;
		CWnd *pCtrl = pDlg->GetDlgItem(pnControlIds[i]);

		if (pCtrl != NULL && TryGet(pnControlIds[i], &str))
			pCtrl->SetWindowText(str);
	}
}


const CString& CLang::GetName()
{
	return s_strName;
}
