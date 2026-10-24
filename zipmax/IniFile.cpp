// IniFile.cpp - veja IniFile.h

#include "IniFile.h"

#include <stdio.h>
#include <string.h>
#include <wctype.h>

#ifdef _WIN32
	#include <windows.h>
#endif


//////////////////////////////////////////////////////////////////////////
// texto <-> bytes

static std::wstring Trim(const std::wstring& s)
{
	size_t a = 0, b = s.size();

	while (a < b && (s[a] == L' ' || s[a] == L'\t' || s[a] == L'\r' || s[a] == L'\n'))
		a++;

	while (b > a && (s[b - 1] == L' ' || s[b - 1] == L'\t' || s[b - 1] == L'\r' || s[b - 1] == L'\n'))
		b--;

	return s.substr(a, b - a);
}


static std::wstring Lower(const std::wstring& s)
{
	std::wstring r = s;

	for (size_t i = 0; i < r.size(); i++)
		r[i] = (wchar_t)towlower((wint_t)r[i]);

	return r;
}


// UTF-8 -> wide. false se os bytes nao formam UTF-8 valido.
static bool Utf8ToWide(const std::string& s, std::wstring *pw)
{
	pw->clear();

	for (size_t i = 0; i < s.size();)
	{
		const unsigned char c = (unsigned char)s[i];
		unsigned long cp;
		int n;

		if (c < 0x80)			{ cp = c;			n = 1; }
		else if ((c >> 5) == 6)	{ cp = c & 0x1F;	n = 2; }
		else if ((c >> 4) == 14)	{ cp = c & 0x0F;	n = 3; }
		else if ((c >> 3) == 30)	{ cp = c & 0x07;	n = 4; }
		else
			return false;

		if (i + n > s.size())
			return false;

		for (int k = 1; k < n; k++)
		{
			const unsigned char cc = (unsigned char)s[i + k];

			if ((cc >> 6) != 2)
				return false;

			cp = (cp << 6) | (cc & 0x3F);
		}

		if (sizeof(wchar_t) == 2 && cp >= 0x10000)
		{
			cp -= 0x10000;
			*pw += (wchar_t)(0xD800 + (cp >> 10));
			*pw += (wchar_t)(0xDC00 + (cp & 0x3FF));
		}
		else
			*pw += (wchar_t)cp;

		i += n;
	}

	return true;
}


static std::string WideToUtf8(const std::wstring& w)
{
	std::string s;

	for (size_t i = 0; i < w.size(); i++)
	{
		unsigned long cp = (unsigned long)w[i];

		if (sizeof(wchar_t) == 2 && cp >= 0xD800 && cp < 0xDC00 && i + 1 < w.size())
		{
			const unsigned long lo = (unsigned long)w[i + 1];
			cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
			i++;
		}

		if (cp < 0x80)
			s += (char)cp;
		else if (cp < 0x800)
		{
			s += (char)(0xC0 | (cp >> 6));
			s += (char)(0x80 | (cp & 0x3F));
		}
		else if (cp < 0x10000)
		{
			s += (char)(0xE0 | (cp >> 12));
			s += (char)(0x80 | ((cp >> 6) & 0x3F));
			s += (char)(0x80 | (cp & 0x3F));
		}
		else
		{
			s += (char)(0xF0 | (cp >> 18));
			s += (char)(0x80 | ((cp >> 12) & 0x3F));
			s += (char)(0x80 | ((cp >> 6) & 0x3F));
			s += (char)(0x80 | (cp & 0x3F));
		}
	}

	return s;
}


static std::wstring AnsiToWide(const std::string& s)
{
#ifdef _WIN32
	if (s.empty())
		return std::wstring();

	const int n = ::MultiByteToWideChar(CP_ACP, 0, s.data(), (int)s.size(), NULL, 0);
	std::wstring w((size_t)(n > 0 ? n : 0), L'\0');

	if (n > 0)
		::MultiByteToWideChar(CP_ACP, 0, s.data(), (int)s.size(), &w[0], n);

	return w;
#else
	std::wstring w;

	for (size_t i = 0; i < s.size(); i++)
		w += (wchar_t)(unsigned char)s[i];

	return w;
#endif
}


static FILE *OpenFile(const std::wstring& strPath, const wchar_t *pszMode)
{
#ifdef _WIN32
	return _wfopen(strPath.c_str(), pszMode);
#else
	std::string p = WideToUtf8(strPath);
	std::string m;

	for (const wchar_t *c = pszMode; *c != 0; c++)
		m += (char)*c;

	return fopen(p.c_str(), m.c_str());
#endif
}


//////////////////////////////////////////////////////////////////////////
// CIniFile

const wchar_t *CIniFile::BlockBegin()
{
	return L"// ---- ZipMax Reborn: settings managed by Settings > Options (begin) ----";
}

const wchar_t *CIniFile::BlockEnd()
{
	return L"// ---- ZipMax Reborn: settings managed by Settings > Options (end) ----";
}


bool CIniFile::Load(const std::wstring& strPath)
{
	m_vecLines.clear();

	FILE *f = OpenFile(strPath, L"rb");

	if (f == NULL)
		return false;

	std::string strData;
	char buf[4096];
	size_t n;

	while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
		strData.append(buf, n);

	fclose(f);

	std::wstring w;

	if (strData.size() >= 2 && (unsigned char)strData[0] == 0xFF && (unsigned char)strData[1] == 0xFE)
	{
		// UTF-16 little endian
		for (size_t i = 2; i + 1 < strData.size(); i += 2)
			w += (wchar_t)((unsigned char)strData[i] | ((unsigned char)strData[i + 1] << 8));
	}
	else
	{
		size_t nStart = 0;

		if (strData.size() >= 3 && (unsigned char)strData[0] == 0xEF && (unsigned char)strData[1] == 0xBB && (unsigned char)strData[2] == 0xBF)
			nStart = 3;

		const std::string strBody = strData.substr(nStart);

		if (! Utf8ToWide(strBody, &w))
			w = AnsiToWide(strBody);
	}

	// divide em linhas (aceita CRLF e LF)
	size_t nPos = 0;

	while (nPos < w.size())
	{
		size_t nEnd = w.find(L'\n', nPos);

		if (nEnd == std::wstring::npos)
			nEnd = w.size();

		std::wstring strLine = w.substr(nPos, nEnd - nPos);

		if (! strLine.empty() && strLine[strLine.size() - 1] == L'\r')
			strLine.erase(strLine.size() - 1);

		m_vecLines.push_back(strLine);
		nPos = nEnd + 1;
	}

	return true;
}


bool CIniFile::Save(const std::wstring& strPath) const
{
	// backup do arquivo anterior (se existir)

	{
		FILE *fOld = OpenFile(strPath, L"rb");

		if (fOld != NULL)
		{
			std::string strOld;
			char buf[4096];
			size_t n;

			while ((n = fread(buf, 1, sizeof(buf), fOld)) > 0)
				strOld.append(buf, n);

			fclose(fOld);

			FILE *fBak = OpenFile(strPath + L".bak", L"wb");

			if (fBak != NULL)
			{
				fwrite(strOld.data(), 1, strOld.size(), fBak);
				fclose(fBak);
			}
		}
	}

	std::wstring strAll;

	for (size_t i = 0; i < m_vecLines.size(); i++)
	{
		strAll += m_vecLines[i];
		strAll += L"\r\n";
	}

	const std::string strUtf8 = WideToUtf8(strAll);

	FILE *f = OpenFile(strPath, L"wb");

	if (f == NULL)
		return false;

	const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
	bool bOk = (fwrite(bom, 1, 3, f) == 3);
	bOk = bOk && (fwrite(strUtf8.data(), 1, strUtf8.size(), f) == strUtf8.size());

	if (fclose(f) != 0)
		bOk = false;

	return bOk;
}


bool CIniFile::ParseActiveLine(const std::wstring& strLine, std::wstring *pstrKeyLower, std::wstring *pstrValue)
{
	const std::wstring s = Trim(strLine);

	if (s.empty() || s[0] == L'/' || s[0] == L';' || s[0] == L'#' || s[0] == L'[')
		return false;

	const size_t nEq = s.find(L'=');

	if (nEq == std::wstring::npos)
		return false;

	*pstrKeyLower = Lower(Trim(s.substr(0, nEq)));
	*pstrValue = Trim(s.substr(nEq + 1));

	return ! pstrKeyLower->empty();
}


bool CIniFile::FindSection(size_t *pnHeader, size_t *pnEnd) const
{
	for (size_t i = 0; i < m_vecLines.size(); i++)
	{
		if (Lower(Trim(m_vecLines[i])) == L"[zipmax]")
		{
			size_t nEnd = i + 1;

			while (nEnd < m_vecLines.size())
			{
				const std::wstring s = Trim(m_vecLines[nEnd]);

				if (! s.empty() && s[0] == L'[')
					break;

				nEnd++;
			}

			*pnHeader = i;
			*pnEnd = nEnd;
			return true;
		}
	}

	return false;
}


bool CIniFile::Get(const std::wstring& strKey, std::wstring *pstrValue) const
{
	size_t nHeader, nEnd;

	if (! FindSection(&nHeader, &nEnd))
		return false;

	const std::wstring strWanted = Lower(strKey);

	for (size_t i = nHeader + 1; i < nEnd; i++)
	{
		std::wstring strKeyLower, strValue;

		if (ParseActiveLine(m_vecLines[i], &strKeyLower, &strValue) && strKeyLower == strWanted)
		{
			*pstrValue = strValue;
			return true;
		}
	}

	return false;
}


bool CIniFile::IsManagedKey(const std::wstring& k)
{
	static const wchar_t *const kKeys[] =
	{
		L"work-recursive", L"work-recursively", L"skip-internzip", L"logfile", L"nobeep", L"setcurrenttime",
		L"showpackerwindow", L"skipreadonly", L"skiparchive", L"setcurrentreadonly",
		L"thread-priority", L"language", L"sevenzip-dll", L"finalstep-exe", L"finalstep-cmd"
	};

	for (size_t i = 0; i < sizeof(kKeys) / sizeof(kKeys[0]); i++)
		if (k == kKeys[i])
			return true;

	// packer-exe, packer-cmd, packer-exe-<n>, packer-cmd-<n>
	for (int nKind = 0; nKind < 2; nKind++)
	{
		const std::wstring strPrefix = (nKind == 0) ? L"packer-exe" : L"packer-cmd";

		if (k.compare(0, strPrefix.size(), strPrefix) != 0)
			continue;

		if (k.size() == strPrefix.size())
			return true;

		if (k[strPrefix.size()] == L'-' && k.size() > strPrefix.size() + 1)
		{
			bool bDigits = true;

			for (size_t i = strPrefix.size() + 1; i < k.size(); i++)
				if (k[i] < L'0' || k[i] > L'9')
					bDigits = false;

			if (bDigits)
				return true;
		}
	}

	return false;
}


void CIniFile::SetValue(const std::wstring& strKey, const std::wstring& strValue)
{
	const std::wstring strLine = strKey + L" = " + strValue;
	const std::wstring strWanted = Lower(strKey);

	size_t nHeader, nEnd;

	if (! FindSection(&nHeader, &nEnd))
	{
		m_vecLines.insert(m_vecLines.begin(), std::wstring(L"[ZIPMAX]"));

		if (m_vecLines.size() > 1 && ! Trim(m_vecLines[1]).empty())
			m_vecLines.insert(m_vecLines.begin() + 1, std::wstring());

		FindSection(&nHeader, &nEnd);
	}

	// ja existe ativa: troca na mesma linha

	for (size_t i = nHeader + 1; i < nEnd; i++)
	{
		std::wstring strKeyLower, strOld;

		if (ParseActiveLine(m_vecLines[i], &strKeyLower, &strOld) && strKeyLower == strWanted)
		{
			m_vecLines[i] = strLine;
			return;
		}
	}

	// nao existe: dentro do bloco gerenciado, antes do marcador final

	for (size_t i = nHeader + 1; i < nEnd; i++)
	{
		if (Trim(m_vecLines[i]) == BlockEnd())
		{
			m_vecLines.insert(m_vecLines.begin() + i, strLine);
			return;
		}
	}

	// sem bloco: logo abaixo do cabecalho

	m_vecLines.insert(m_vecLines.begin() + nHeader + 1, strLine);
}


void CIniFile::ReplaceManagedBlock(const std::vector<std::wstring>& vecBlock)
{
	// 1) remove o bloco gravado anteriormente (entre os marcadores)

	for (size_t i = 0; i < m_vecLines.size(); i++)
	{
		if (Trim(m_vecLines[i]) == BlockBegin())
		{
			size_t j = i;

			while (j < m_vecLines.size() && Trim(m_vecLines[j]) != BlockEnd())
				j++;

			if (j < m_vecLines.size())
			{
				m_vecLines.erase(m_vecLines.begin() + i, m_vecLines.begin() + j + 1);

				// tira tambem a linha em branco que foi colocada depois do bloco
				if (i < m_vecLines.size() && Trim(m_vecLines[i]).empty())
					m_vecLines.erase(m_vecLines.begin() + i);
			}

			break;
		}
	}

	// 2) garante o cabecalho [ZIPMAX]

	size_t nHeader, nEnd;

	if (! FindSection(&nHeader, &nEnd))
	{
		m_vecLines.insert(m_vecLines.begin(), std::wstring(L"[ZIPMAX]"));

		if (m_vecLines.size() > 1 && ! Trim(m_vecLines[1]).empty())
			m_vecLines.insert(m_vecLines.begin() + 1, std::wstring());

		FindSection(&nHeader, &nEnd);
	}

	// 3) remove as linhas ativas das chaves gerenciadas (so dentro da secao)

	for (size_t i = nEnd; i > nHeader + 1; i--)
	{
		std::wstring strKeyLower, strValue;

		if (ParseActiveLine(m_vecLines[i - 1], &strKeyLower, &strValue) && IsManagedKey(strKeyLower))
			m_vecLines.erase(m_vecLines.begin() + (i - 1));
	}

	// 4) escreve o bloco logo abaixo do cabecalho

	std::vector<std::wstring> vecNew;
	vecNew.push_back(std::wstring());
	vecNew.push_back(BlockBegin());
	vecNew.insert(vecNew.end(), vecBlock.begin(), vecBlock.end());
	vecNew.push_back(BlockEnd());

	m_vecLines.insert(m_vecLines.begin() + nHeader + 1, vecNew.begin(), vecNew.end());

	// linha em branco depois do bloco
	const size_t nAfter = nHeader + 1 + vecNew.size();

	if (nAfter < m_vecLines.size() && ! Trim(m_vecLines[nAfter]).empty())
		m_vecLines.insert(m_vecLines.begin() + nAfter, std::wstring());
}
