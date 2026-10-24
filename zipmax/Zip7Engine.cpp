// Zip7Engine.cpp
//
// Implementacao da camada 7-Zip do ZipMax. Baseada no exemplo oficial
// CPP/7zip/UI/Client7z/Client7z.cpp (7-Zip 26.04), que mostra como usar o 7z.dll
// (IInArchive / IOutArchive) a partir de um programa C++.
//
// Este arquivo NAO usa o cabecalho pre-compilado do MFC (stdafx.h).

#include "Common/Common.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "Common/MyWindows.h"

#ifndef ZIPMAX_STATIC_7Z
	// No modo DLL este e o unico .cpp que define os GUIDs (IID_IInArchive, ...).
	// No modo estatico os GUIDs ja sao definidos pelo proprio codigo do 7-Zip.
	#include "Common/MyInitGuid.h"
#endif

#include "Common/Defs.h"
#include "Common/IntToString.h"
#include "Common/StringConvert.h"
#include "Common/UTFConvert.h"

#include "Windows/DLL.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"

#include "7zip/Common/FileStreams.h"
#include "7zip/Archive/IArchive.h"
#include "7zip/IPassword.h"

#include "Zip7Engine.h"

#ifdef _WIN32
// Exigido por CPP/Windows/DLL.cpp do 7-Zip (NULL = o proprio .exe)
extern HINSTANCE g_hInstance;
HINSTANCE g_hInstance = NULL;
#endif

using namespace NWindows;
using namespace NFile;
using namespace NDir;

Z7_DIAGNOSTIC_IGNORE_CAST_FUNCTION

namespace Zip7
{

//////////////////////////////////////////////////////////////////////////
// GUID do formato ZIP: {23170F69-40C1-278A-1000-000110010000}
// (declarado "na mao" para funcionar igual nos modos DLL e estatico)

static const GUID kClsidZip =
	{ 0x23170F69, 0x40C1, 0x278A, { 0x10, 0x00, 0x00, 0x01, 0x10, 0x01, 0x00, 0x00 } };


//////////////////////////////////////////////////////////////////////////
// Carga do motor (7z.dll ou codigo estatico)

#ifdef ZIPMAX_STATIC_7Z
	// Vem do codigo do 7-Zip compilado junto (CPP/7zip/Archive/DllExports2.cpp)
	extern "C" HRESULT WINAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject);
	static Func_CreateObject g_pfnCreateObject = CreateObject;
#else
	static NDLL::CLibrary *g_pLib = NULL;
	static Func_CreateObject g_pfnCreateObject = NULL;
#endif


// Descricao do motor em uso ("static" ou o caminho do 7z.dll), para as mensagens de erro
static std::wstring g_strEngineInfo = L"not initialized";


static std::wstring Widen(const char *psz)
{
	std::wstring w;

	while (*psz != 0)
		w += (wchar_t)(unsigned char)*psz++;

	return w;
}


// Monta uma mensagem de erro (Unicode). Formatos aceitos: %s = const wchar_t *, %lu e %08lX = unsigned long.
// (formatador proprio: "%s" em printf de wide-char significa coisas diferentes no MSVC e no glibc)
static void SetError(std::wstring *pError, const char *fmt, ...)
{
	if (pError == NULL)
		return;

	std::wstring r;

	va_list args;
	va_start(args, fmt);

	while (*fmt != 0)
	{
		if (*fmt != '%')
		{
			r += (wchar_t)(unsigned char)*fmt++;
			continue;
		}

		fmt++;

		if (*fmt == '%')
		{
			r += L'%';
			fmt++;
		}
		else if (*fmt == 's')
		{
			const wchar_t *p = va_arg(args, const wchar_t *);
			r += (p != NULL) ? p : L"";
			fmt++;
		}
		else if (strncmp(fmt, "lu", 2) == 0)
		{
			char buf[32];
			snprintf(buf, sizeof(buf), "%lu", va_arg(args, unsigned long));
			r += Widen(buf);
			fmt += 2;
		}
		else if (strncmp(fmt, "08lX", 4) == 0)
		{
			char buf[32];
			snprintf(buf, sizeof(buf), "%08lX", va_arg(args, unsigned long));
			r += Widen(buf);
			fmt += 4;
		}
	}

	va_end(args);

	*pError = r;
}


static FString PathToFString(const wchar_t *strPath)
{
	return us2fs(UString(strPath));
}


#ifndef ZIPMAX_STATIC_7Z

static bool TryLoad(const FString &strDll)
{
	NDLL::CLibrary *pLib = new NDLL::CLibrary;

	if (!pLib->Load(strDll))
	{
		delete pLib;
		return false;
	}

	Func_CreateObject pfn = Z7_GET_PROC_ADDRESS(Func_CreateObject, pLib->Get_HMODULE(), "CreateObject");

	if (pfn == NULL)
	{
		delete pLib;
		return false;
	}

	g_pLib = pLib;
	g_pfnCreateObject = pfn;
	g_strEngineInfo = std::wstring(L"DLL: ") + fs2us(strDll).Ptr();
	return true;
}

#endif


bool Initialize(const wchar_t *dllPath, std::wstring *pError)
{
#ifdef ZIPMAX_STATIC_7Z
	(void)dllPath;
	(void)pError;
	g_strEngineInfo = L"static";
	return true;
#else
	if (g_pfnCreateObject != NULL)
		return true;

	std::wstring strTried;

	// 1) caminho explicito (zipmax.ini)
	if (dllPath != NULL && dllPath[0] != 0)
	{
		if (TryLoad(PathToFString(dllPath)))
			return true;
		strTried += dllPath;
		strTried += L"; ";
	}

	// 2) pasta do ZipMax.exe
	#ifdef _WIN32
	{
		const FString strFile = NDLL::GetModuleDirPrefix() + FTEXT("7z.dll");

		if (TryLoad(strFile))
			return true;

		strTried += fs2us(strFile).Ptr();
		strTried += L"; ";
	}
	#endif

	// 3) pesquisa padrao do Windows (PATH) e instalacao padrao do 7-Zip
	#ifdef _WIN32
	{
		if (TryLoad(FTEXT("7z.dll")))
			return true;
		strTried += L"7z.dll (PATH); ";

		if (TryLoad(FTEXT("C:\\Program Files\\7-Zip\\7z.dll")))
			return true;
		strTried += L"C:\\Program Files\\7-Zip\\7z.dll; ";
	}
	#endif

	SetError(pError, "Can't load the 7-Zip library. Tried: %s", strTried.c_str());
	return false;
#endif
}


void Shutdown()
{
#ifndef ZIPMAX_STATIC_7Z
	g_pfnCreateObject = NULL;
	delete g_pLib;
	g_pLib = NULL;
#endif
}


//////////////////////////////////////////////////////////////////////////
// ZipHeader / CompressOptions

ZipHeader::ZipHeader()
	: versionMadeBy(0), versionNeeded(0), flags(0), method(0), modTime(0), modDate(0),
	  internalAttr(0), crc32(0), comprSize(0), uncomprSize(0), externalAttr(0), localHeaderOffset(0)
{
}

bool ZipHeader::IsDirectory() const
{
	if (rawName.empty())
		return false;

	const char c = rawName[rawName.size() - 1];
	return (c == '/' || c == '\\');
}

CompressOptions::CompressOptions()
	: level(9), passes(15), fastBytes(258)
{
}


//////////////////////////////////////////////////////////////////////////
// Helfer

static bool IsDirectorySeparator(wchar_t c)
{
	return c == L'/' || c == L'\\';
}


// Transforma o caminho de um item do zip num caminho relativo seguro (sem "..", sem
// drive, sem ':'). Retorna false se o caminho nao for aceitavel.
static bool MakeSafeRelativePath(const UString &strIn, UString &strOut)
{
	strOut.Empty();

	unsigned i = 0;
	const unsigned n = strIn.Len();

	while (i < n)
	{
		unsigned j = i;
		while (j < n && !IsDirectorySeparator(strIn[j]))
			j++;

		const UString strPart = strIn.Mid(i, j - i);

		if (strPart.IsEmpty() || strPart.IsEqualTo("."))
		{
			// ignora "//" e "./"
		}
		else if (strPart.IsEqualTo(".."))
			return false;
		else if (strPart.Find(L':') >= 0)
			return false;
		else
		{
			if (!strOut.IsEmpty())
				strOut.Add_PathSepar();
			strOut += strPart;
		}

		i = j + 1;
	}

	return true;
}


static std::wstring ToStdWString(const UString &s)
{
	return std::wstring(s.Ptr());
}


static std::wstring DecodeEntryName(const std::string &strRaw, const bool bUtf8)
{
	AString a;
	a = strRaw.c_str();

	UString u;

	if (bUtf8)
	{
		if (!ConvertUTF8ToUnicode(a, u))
			u = MultiByteToUnicodeString(a, CP_OEMCP);
	}
	else
		u = MultiByteToUnicodeString(a, CP_OEMCP);

	std::wstring w = ToStdWString(u);

	for (size_t i = 0; i < w.size(); i++)
		if (w[i] == L'\\')
			w[i] = L'/';

	return w;
}


static std::wstring ToLowerKey(const std::wstring &w)
{
	std::wstring k = w;

	for (size_t i = 0; i < k.size(); i++)
		k[i] = (wchar_t)towlower((wint_t)k[i]);

	return k;
}


//////////////////////////////////////////////////////////////////////////
// Callback de abertura

class CArchiveOpenCallback Z7_final:
	public IArchiveOpenCallback,
	public ICryptoGetTextPassword,
	public CMyUnknownImp
{
	Z7_IFACES_IMP_UNK_2(IArchiveOpenCallback, ICryptoGetTextPassword)
};

Z7_COM7F_IMF(CArchiveOpenCallback::SetTotal(const UInt64 * /* files */, const UInt64 * /* bytes */))
{
	return S_OK;
}

Z7_COM7F_IMF(CArchiveOpenCallback::SetCompleted(const UInt64 * /* files */, const UInt64 * /* bytes */))
{
	return S_OK;
}

Z7_COM7F_IMF(CArchiveOpenCallback::CryptoGetTextPassword(BSTR * /* password */))
{
	// o ZipMax nao trabalha com arquivos protegidos por senha
	return E_ABORT;
}


//////////////////////////////////////////////////////////////////////////
// Callback de extracao / teste

class CArchiveExtractCallback Z7_final:
	public IArchiveExtractCallback,
	public ICryptoGetTextPassword,
	public CMyUnknownImp
{
	Z7_IFACES_IMP_UNK_2(IArchiveExtractCallback, ICryptoGetTextPassword)
	Z7_IFACE_COM7_IMP(IProgress)

	CMyComPtr<IInArchive> _archiveHandler;
	FString _directoryPath;			// pasta de destino (com separador no final)
	FString _diskFilePath;			// arquivo atual no disco
	bool _extractMode;
	bool _testMode;

	struct CProcessedFileInfo
	{
		FILETIME MTime;
		UInt32 Attrib;
		bool isDir;
		bool MTime_Defined;
		bool Attrib_Defined;
	} _info;

	COutFileStream *_outFileStreamSpec;
	CMyComPtr<ISequentialOutStream> _outFileStream;

public:
	void Init(IInArchive *archiveHandler, const FString &directoryPath, bool bTestMode)
	{
		NumErrors = 0;
		LastOperationResult = 0;
		UnsafePath = false;
		_archiveHandler = archiveHandler;
		_directoryPath = directoryPath;
		_testMode = bTestMode;
		_extractMode = false;
		_outFileStreamSpec = NULL;

		if (!_testMode)
			NName::NormalizeDirPathPrefix(_directoryPath);
	}

	UInt64 NumErrors;
	Int32 LastOperationResult;
	bool UnsafePath;

	CArchiveExtractCallback() : _extractMode(false), _testMode(false), _outFileStreamSpec(NULL),
		NumErrors(0), LastOperationResult(0), UnsafePath(false) {}
};

Z7_COM7F_IMF(CArchiveExtractCallback::SetTotal(UInt64 /* size */))
{
	return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::SetCompleted(const UInt64 * /* completeValue */))
{
	return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode))
{
	*outStream = NULL;
	_outFileStream.Release();
	_outFileStreamSpec = NULL;
	_extractMode = false;

	if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
		return S_OK;						// teste ou "skip": nao precisa de fluxo de saida

	// ---- nome do item
	UString strPath;
	{
		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidPath, &prop))

		if (prop.vt == VT_EMPTY)
			strPath = L"[Content]";
		else
		{
			if (prop.vt != VT_BSTR)
				return E_FAIL;
			strPath = prop.bstrVal;
		}
	}

	UString strSafe;
	if (!MakeSafeRelativePath(strPath, strSafe))
	{
		UnsafePath = true;					// ex.: "..\\..\\x" -> nunca grava fora da pasta de destino
		return E_ABORT;
	}

	// ---- atributos
	{
		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidAttrib, &prop))

		if (prop.vt == VT_EMPTY)
		{
			_info.Attrib = 0;
			_info.Attrib_Defined = false;
		}
		else
		{
			if (prop.vt != VT_UI4)
				return E_FAIL;
			_info.Attrib = prop.ulVal;
			_info.Attrib_Defined = true;
		}
	}

	// ---- diretorio?
	{
		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidIsDir, &prop))

		if (prop.vt == VT_BOOL)
			_info.isDir = VARIANT_BOOLToBool(prop.boolVal);
		else if (prop.vt == VT_EMPTY)
			_info.isDir = false;
		else
			return E_FAIL;
	}

	// ---- data/hora
	{
		_info.MTime_Defined = false;

		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidMTime, &prop))

		if (prop.vt == VT_FILETIME)
		{
			_info.MTime = prop.filetime;
			_info.MTime_Defined = true;
		}
		else if (prop.vt != VT_EMPTY)
			return E_FAIL;
	}

	// ---- cria as pastas e o arquivo
	{
		const int nSlash = strSafe.ReverseFind_PathSepar();
		if (nSlash >= 0)
			CreateComplexDir(_directoryPath + us2fs(strSafe.Left((unsigned)nSlash)));
	}

	const FString strFull = _directoryPath + us2fs(strSafe);
	_diskFilePath = strFull;

	_extractMode = true;

	if (_info.isDir)
	{
		CreateComplexDir(strFull);
		return S_OK;
	}

	{
		NFind::CFileInfo fi;
		if (fi.Find(strFull))
		{
			if (!DeleteFileAlways(strFull))
				return E_ABORT;
		}
	}

	_outFileStreamSpec = new COutFileStream;
	CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);

	if (!_outFileStreamSpec->Create_ALWAYS(strFull))
	{
		_outFileStreamSpec = NULL;
		return E_ABORT;
	}

	_outFileStream = outStreamLoc;
	*outStream = outStreamLoc.Detach();

	return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::PrepareOperation(Int32 /* askExtractMode */))
{
	return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::SetOperationResult(Int32 operationResult))
{
	if (operationResult != NArchive::NExtract::NOperationResult::kOK)
	{
		NumErrors++;
		LastOperationResult = operationResult;
	}

	if (_outFileStream)
	{
		if (_info.MTime_Defined)
		{
		#ifdef _WIN32
			CFiTime ft = _info.MTime;
		#else
			CFiTime ft;
			FILETIME_To_timespec(_info.MTime, ft);
		#endif
			_outFileStreamSpec->SetMTime(&ft);
		}

		RINOK(_outFileStreamSpec->Close())
	}

	_outFileStream.Release();
	_outFileStreamSpec = NULL;

	if (_extractMode && _info.Attrib_Defined)
		SetFileAttrib_PosixHighDetect(_diskFilePath, _info.Attrib);

	return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::CryptoGetTextPassword(BSTR * /* password */))
{
	return E_ABORT;
}


//////////////////////////////////////////////////////////////////////////
// Abrir um arquivo ZIP

static bool OpenZip(const wchar_t *strArchive, CMyComPtr<IInArchive> &archive, std::wstring *pError)
{
	if (g_pfnCreateObject == NULL)
	{
		SetError(pError, "7-Zip engine not initialized");
		return false;
	}

	{
		const HRESULT hrCreate = g_pfnCreateObject(&kClsidZip, &IID_IInArchive, (void **)&archive);

		if (hrCreate != S_OK || !archive)
		{
			SetError(pError, "Can't create the 7-Zip ZIP handler (0x%08lX; engine: %s)", (unsigned long)hrCreate, g_strEngineInfo.c_str());
			return false;
		}
	}

	CInFileStream *fileSpec = new CInFileStream;
	CMyComPtr<IInStream> file = fileSpec;

	if (!fileSpec->Open(PathToFString(strArchive)))
	{
		SetError(pError, "Can't open file: %s", strArchive);
		archive.Release();
		return false;
	}

	CArchiveOpenCallback *openCallbackSpec = new CArchiveOpenCallback;
	CMyComPtr<IArchiveOpenCallback> openCallback(openCallbackSpec);

	const UInt64 scanSize = 1 << 23;

	if (archive->Open(file, &scanSize, openCallback) != S_OK)
	{
		SetError(pError, "Not a valid ZIP file: %s", strArchive);
		archive.Release();
		return false;
	}

	// ZipMax so aceita zips de um unico volume (como antes)
	{
		NCOM::CPropVariant prop;
		if (archive->GetArchiveProperty(kpidIsVolume, &prop) == S_OK && prop.vt == VT_BOOL && VARIANT_BOOLToBool(prop.boolVal))
		{
			SetError(pError, "Disk-spanning ZIP files are not supported: %s", strArchive);
			archive->Close();
			archive.Release();
			return false;
		}
	}

	return true;
}


static bool RunExtract(const wchar_t *strArchive, const wchar_t *strDestDir, const bool bTest, std::wstring *pError)
{
	CMyComPtr<IInArchive> archive;

	if (!OpenZip(strArchive, archive, pError))
		return false;

	CArchiveExtractCallback *cbSpec = new CArchiveExtractCallback;
	CMyComPtr<IArchiveExtractCallback> cb(cbSpec);
	cbSpec->Init(archive, bTest ? FString() : PathToFString(strDestDir), bTest);

	const HRESULT hr = archive->Extract(NULL, (UInt32)(Int32)(-1), bTest ? 1 : 0, cb);

	archive->Close();

	if (hr != S_OK)
	{
		if (cbSpec->UnsafePath)
			SetError(pError, "Unsafe path inside archive: %s", strArchive);
		else
			SetError(pError, bTest ? "Test failed (0x%08lX): %s" : "Extract failed (0x%08lX): %s", (unsigned long)hr, strArchive);
		return false;
	}

	if (cbSpec->NumErrors != 0)
	{
		SetError(pError, "%s: data/CRC errors in archive (code %lu)", strArchive, (unsigned long)cbSpec->LastOperationResult);
		return false;
	}

	return true;
}


bool ExtractAll(const wchar_t *strArchive, const wchar_t *strDestDir, std::wstring *pError)
{
	return RunExtract(strArchive, strDestDir, false, pError);
}

bool TestArchive(const wchar_t *strArchive, std::wstring *pError)
{
	return RunExtract(strArchive, L"", true, pError);
}


//////////////////////////////////////////////////////////////////////////
// Compressao (cria um zip novo)

struct CDirItem : public NFind::CFileInfoBase
{
	UString Path_For_Handler;		// nome dentro do zip
	FString FullPath;				// caminho completo no disco

	CDirItem(const NFind::CFileInfo &fi) : CFileInfoBase(fi) {}
};

class CArchiveUpdateCallback Z7_final:
	public IArchiveUpdateCallback2,
	public ICryptoGetTextPassword2,
	public CMyUnknownImp
{
	Z7_IFACES_IMP_UNK_2(IArchiveUpdateCallback2, ICryptoGetTextPassword2)
	Z7_IFACE_COM7_IMP(IProgress)
	Z7_IFACE_COM7_IMP(IArchiveUpdateCallback)

public:
	const CObjectVector<CDirItem> *DirItems;
	FStringVector FailedFiles;

	CArchiveUpdateCallback() : DirItems(NULL) {}

	void Init(const CObjectVector<CDirItem> *dirItems)
	{
		DirItems = dirItems;
		FailedFiles.Clear();
	}
};

Z7_COM7F_IMF(CArchiveUpdateCallback::SetTotal(UInt64 /* size */))
{
	return S_OK;
}

Z7_COM7F_IMF(CArchiveUpdateCallback::SetCompleted(const UInt64 * /* completeValue */))
{
	return S_OK;
}

Z7_COM7F_IMF(CArchiveUpdateCallback::GetUpdateItemInfo(UInt32 /* index */, Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive))
{
	if (newData)
		*newData = BoolToInt(true);
	if (newProperties)
		*newProperties = BoolToInt(true);
	if (indexInArchive)
		*indexInArchive = (UInt32)(Int32)-1;
	return S_OK;
}

Z7_COM7F_IMF(CArchiveUpdateCallback::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
	NCOM::CPropVariant prop;

	if (propID == kpidIsAnti)
	{
		prop = false;
		prop.Detach(value);
		return S_OK;
	}

	{
		const CDirItem &di = (*DirItems)[index];

		switch (propID)
		{
			case kpidPath:		prop = di.Path_For_Handler; break;
			case kpidIsDir:		prop = di.IsDir(); break;
			case kpidSize:		prop = di.Size; break;
			case kpidCTime:		PropVariant_SetFrom_FiTime(prop, di.CTime); break;
			case kpidATime:		PropVariant_SetFrom_FiTime(prop, di.ATime); break;
			case kpidMTime:		PropVariant_SetFrom_FiTime(prop, di.MTime); break;
			case kpidAttrib:	prop = (UInt32)di.GetWinAttrib(); break;
			case kpidPosixAttrib:	prop = (UInt32)di.GetPosixAttrib(); break;
		}
	}

	prop.Detach(value);
	return S_OK;
}

Z7_COM7F_IMF(CArchiveUpdateCallback::GetStream(UInt32 index, ISequentialInStream **inStream))
{
	const CDirItem &dirItem = (*DirItems)[index];

	if (dirItem.IsDir())
		return S_OK;

	CInFileStream *inStreamSpec = new CInFileStream;
	CMyComPtr<ISequentialInStream> inStreamLoc(inStreamSpec);

	if (!inStreamSpec->Open(dirItem.FullPath))
	{
		FailedFiles.Add(dirItem.FullPath);
		return S_FALSE;						// 7-Zip pula o arquivo; nos tratamos como erro depois
	}

	*inStream = inStreamLoc.Detach();
	return S_OK;
}

Z7_COM7F_IMF(CArchiveUpdateCallback::SetOperationResult(Int32 /* operationResult */))
{
	return S_OK;
}

Z7_COM7F_IMF(CArchiveUpdateCallback::GetVolumeSize(UInt32 /* index */, UInt64 * /* size */))
{
	return S_FALSE;							// sem volumes
}

Z7_COM7F_IMF(CArchiveUpdateCallback::GetVolumeStream(UInt32 /* index */, ISequentialOutStream ** /* volumeStream */))
{
	return E_NOTIMPL;
}

Z7_COM7F_IMF(CArchiveUpdateCallback::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password))
{
	*passwordIsDefined = BoolToInt(false);
	*password = NULL;
	return S_OK;
}


bool CompressFiles(const wchar_t *strArchive, const wchar_t *strBaseDir, const std::vector<std::wstring> &vecRelFiles, const CompressOptions &opt, std::wstring *pError)
{
	if (g_pfnCreateObject == NULL)
	{
		SetError(pError, "7-Zip engine not initialized");
		return false;
	}

	// ---- lista de itens
	FString strBase = PathToFString(strBaseDir);
	NName::NormalizeDirPathPrefix(strBase);

	CObjectVector<CDirItem> dirItems;

	for (size_t i = 0; i < vecRelFiles.size(); i++)
	{
		const FString strRel = PathToFString(vecRelFiles[i].c_str());
		const FString strFull = strBase + strRel;

		NFind::CFileInfo fi;
		if (!fi.Find(strFull) || fi.IsDir())
		{
			SetError(pError, "Can't find file: %s", vecRelFiles[i].c_str());
			return false;
		}

		CDirItem di(fi);
		di.FullPath = strFull;

		UString strName = fs2us(strRel);
		for (unsigned k = 0; k < strName.Len(); k++)
			if (strName[k] == L'\\')
				strName.ReplaceOneCharAtPos(k, L'/');

		di.Path_For_Handler = strName;
		dirItems.Add(di);
	}

	// ---- cria o handler de escrita
	CMyComPtr<IOutArchive> outArchive;

	{
		const HRESULT hrCreate = g_pfnCreateObject(&kClsidZip, &IID_IOutArchive, (void **)&outArchive);

		if (hrCreate != S_OK || !outArchive)
		{
			SetError(pError, "Can't create the 7-Zip ZIP writer (0x%08lX; engine: %s)", (unsigned long)hrCreate, g_strEngineInfo.c_str());
			return false;
		}
	}

	// ---- propriedades: Deflate puro (zip 100% padrao), no maximo
	{
		const wchar_t *names[] = { L"x", L"m", L"fb", L"pass" };
		const unsigned kNumProps = sizeof(names) / sizeof(names[0]);

		NCOM::CPropVariant values[kNumProps];
		values[0] = (UInt32)opt.level;
		values[1] = L"Deflate";
		values[2] = (UInt32)opt.fastBytes;
		values[3] = (UInt32)opt.passes;

		CMyComPtr<ISetProperties> setProperties;
		outArchive->QueryInterface(IID_ISetProperties, (void **)&setProperties);

		if (!setProperties)
		{
			SetError(pError, "ISetProperties not supported by the ZIP handler");
			return false;
		}

		const HRESULT hr = setProperties->SetProperties(names, values, kNumProps);
		if (hr != S_OK)
		{
			SetError(pError, "Invalid compression settings (0x%08lX)", (unsigned long)hr);
			return false;
		}
	}

	// ---- arquivo de saida
	COutFileStream *outFileStreamSpec = new COutFileStream;
	CMyComPtr<IOutStream> outFileStream = outFileStreamSpec;

	if (!outFileStreamSpec->Create_NEW(PathToFString(strArchive)))
	{
		// Create_NEW falha se ja existir; o ZipMax sempre cria um nome novo, mas por seguranca:
		DeleteFileAlways(PathToFString(strArchive));

		if (!outFileStreamSpec->Create_NEW(PathToFString(strArchive)))
		{
			SetError(pError, "Can't create file: %s", strArchive);
			return false;
		}
	}

	CArchiveUpdateCallback *updateCallbackSpec = new CArchiveUpdateCallback;
	CMyComPtr<IArchiveUpdateCallback2> updateCallback(updateCallbackSpec);
	updateCallbackSpec->Init(&dirItems);

	const HRESULT result = outArchive->UpdateItems(outFileStream, dirItems.Size(), updateCallback);

	outFileStream.Release();
	outArchive.Release();

	if (result != S_OK)
	{
		SetError(pError, "7-Zip update error (0x%08lX): %s", (unsigned long)result, strArchive);
		DeleteFileAlways(PathToFString(strArchive));
		return false;
	}

	if (updateCallbackSpec->FailedFiles.Size() != 0)
	{
		SetError(pError, "Can't read some files while compressing: %s", strArchive);
		DeleteFileAlways(PathToFString(strArchive));
		return false;
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////
// Leitura do "central directory"

#ifdef _MSC_VER
	#define ZIP7_FSEEK64 _fseeki64
	#define ZIP7_FTELL64 _ftelli64
#else
	#define ZIP7_FSEEK64 fseeko
	#define ZIP7_FTELL64 ftello
#endif

static zip7_u16 Rd16(const unsigned char *p)
{
	return (zip7_u16)(p[0] | (p[1] << 8));
}

static zip7_u32 Rd32(const unsigned char *p)
{
	return (zip7_u32)p[0] | ((zip7_u32)p[1] << 8) | ((zip7_u32)p[2] << 16) | ((zip7_u32)p[3] << 24);
}

static const zip7_u32 kSigEOCD = 0x06054b50;
static const zip7_u32 kSigCFH = 0x02014b50;
static const zip7_u32 kSigZip64Locator = 0x07064b50;

bool ReadHeaders(const wchar_t *strArchive, std::vector<ZipHeader> *pvecHeaders, std::wstring *pError)
{
	pvecHeaders->clear();

	FILE *f = NULL;

#ifdef _WIN32
	f = _wfopen(strArchive, L"rb");
#else
	f = fopen(UnicodeStringToMultiByte(UString(strArchive)).Ptr(), "rb");
#endif

	if (f == NULL)
	{
		SetError(pError, "Can't open file: %s", strArchive);
		return false;
	}

	bool bOk = false;
	std::vector<unsigned char> tail, cd;

	do
	{
		if (ZIP7_FSEEK64(f, 0, SEEK_END) != 0)
			break;

		const long long nFileSize = (long long)ZIP7_FTELL64(f);

		if (nFileSize < 22)
		{
			SetError(pError, "Not a valid ZIP file: %s", strArchive);
			break;
		}

		// ---- procura o "end of central directory"
		const long long nTail = (nFileSize < 65557) ? nFileSize : 65557;
		tail.resize((size_t)nTail);

		if (ZIP7_FSEEK64(f, nFileSize - nTail, SEEK_SET) != 0 || fread(&tail[0], 1, (size_t)nTail, f) != (size_t)nTail)
			break;

		long long nEocd = -1;

		for (long long i = nTail - 22; i >= 0; i--)
		{
			if (Rd32(&tail[(size_t)i]) == kSigEOCD)
			{
				const long long nComment = Rd16(&tail[(size_t)i + 20]);

				if (i + 22 + nComment <= nTail)
				{
					nEocd = i;
					break;
				}
			}
		}

		if (nEocd < 0)
		{
			SetError(pError, "Not a valid ZIP file (no end of central directory): %s", strArchive);
			break;
		}

		const unsigned char *e = &tail[(size_t)nEocd];

		const zip7_u16 nDisk = Rd16(e + 4);
		const zip7_u16 nDiskCD = Rd16(e + 6);
		const zip7_u16 nEntriesDisk = Rd16(e + 8);
		const zip7_u16 nEntries = Rd16(e + 10);
		const zip7_u32 nCdSize = Rd32(e + 12);
		const zip7_u32 nCdOffset = Rd32(e + 16);

		if (nDisk != 0 || nDiskCD != 0 || nEntriesDisk != nEntries)
		{
			SetError(pError, "Disk-spanning ZIP files are not supported: %s", strArchive);
			break;
		}

		if (nEntries == 0xFFFF || nCdSize == 0xFFFFFFFFu || nCdOffset == 0xFFFFFFFFu ||
			(nEocd >= 20 && Rd32(&tail[(size_t)nEocd - 20]) == kSigZip64Locator))
		{
			SetError(pError, "ZIP64 archives are not supported: %s", strArchive);
			break;
		}

		const long long nEocdAbs = nFileSize - nTail + nEocd;

		// ---- le o central directory (considera dados antes do zip, ex.: stub)
		long long nCdStart = (long long)nCdOffset;
		const long long nCdStartGuess = nEocdAbs - (long long)nCdSize;

		if (nCdStartGuess >= 0 && nCdStartGuess != nCdStart)
		{
			unsigned char sig[4];
			if (ZIP7_FSEEK64(f, nCdStartGuess, SEEK_SET) == 0 && fread(sig, 1, 4, f) == 4 && Rd32(sig) == kSigCFH)
				nCdStart = nCdStartGuess;
		}

		const long long nShift = nCdStart - (long long)nCdOffset;

		if (nCdSize > 0)
		{
			cd.resize(nCdSize);

			if (nCdStart < 0 || nCdStart + (long long)nCdSize > nFileSize ||
				ZIP7_FSEEK64(f, nCdStart, SEEK_SET) != 0 || fread(&cd[0], 1, nCdSize, f) != nCdSize)
			{
				SetError(pError, "Corrupt ZIP central directory: %s", strArchive);
				break;
			}
		}

		pvecHeaders->reserve(nEntries);

		size_t pos = 0;
		bool bEntriesOk = true;

		for (unsigned i = 0; i < nEntries; i++)
		{
			if (pos + 46 > cd.size() || Rd32(&cd[pos]) != kSigCFH)
			{
				bEntriesOk = false;
				break;
			}

			const unsigned char *p = &cd[pos];

			ZipHeader h;
			h.versionMadeBy = Rd16(p + 4);
			h.versionNeeded = Rd16(p + 6);
			h.flags = Rd16(p + 8);
			h.method = Rd16(p + 10);
			h.modTime = Rd16(p + 12);
			h.modDate = Rd16(p + 14);
			h.crc32 = Rd32(p + 16);
			h.comprSize = Rd32(p + 20);
			h.uncomprSize = Rd32(p + 24);

			const size_t nName = Rd16(p + 28);
			const size_t nExtra = Rd16(p + 30);
			const size_t nCom = Rd16(p + 32);

			h.internalAttr = Rd16(p + 36);
			h.externalAttr = Rd32(p + 38);

			const zip7_u32 nLocal = Rd32(p + 42);

			if (pos + 46 + nName + nExtra + nCom > cd.size())
			{
				bEntriesOk = false;
				break;
			}

			if (h.comprSize == 0xFFFFFFFFu || h.uncomprSize == 0xFFFFFFFFu || nLocal == 0xFFFFFFFFu)
			{
				SetError(pError, "ZIP64 archives are not supported: %s", strArchive);
				bEntriesOk = false;
				break;
			}

			const long long nLocalAbs = (long long)nLocal + nShift;

			if (nLocalAbs < 0 || nLocalAbs > 0xFFFFFFFFLL)
			{
				bEntriesOk = false;
				break;
			}

			h.localHeaderOffset = (zip7_u32)nLocalAbs;
			h.rawName.assign((const char *)(p + 46), nName);
			h.name = DecodeEntryName(h.rawName, (h.flags & 0x0800) != 0);
			h.key = ToLowerKey(h.name);

			pvecHeaders->push_back(h);

			pos += 46 + nName + nExtra + nCom;
		}

		if (!bEntriesOk)
		{
			if (pError != NULL && pError->empty())
				SetError(pError, "Corrupt ZIP central directory: %s", strArchive);
			break;
		}

		bOk = true;
	}
	while (false);

	fclose(f);

	if (!bOk)
	{
		pvecHeaders->clear();

		if (pError != NULL && pError->empty())
			SetError(pError, "Can't read ZIP headers: %s", strArchive);
	}

	return bOk;
}



//////////////////////////////////////////////////////////////////////////
// Descricao do metodo de compressao

static std::wstring MethodBaseName(zip7_u16 nMethod)
{
	switch (nMethod)
	{
		case 0:		return L"Store";
		case 1:		return L"Shrink";
		case 2: case 3: case 4: case 5:		return L"Reduce";
		case 6:		return L"Implode";
		case 8:		return L"Deflate";
		case 9:		return L"Deflate64";
		case 10:	return L"PKImploding";
		case 12:	return L"BZip2";
		case 14:	return L"LZMA";
		case 93:	return L"zstd";
		case 95:	return L"xz";
		case 98:	return L"PPMd";
		case 99:	return L"AES";
		default:
		{
			char buf[32];
			snprintf(buf, sizeof(buf), "Method %u", (unsigned)nMethod);
			return Widen(buf);
		}
	}
}


static std::wstring SizeToText(unsigned long long n)
{
	char buf[64];

	if (n >= (1ULL << 30) && (n % (1ULL << 30)) == 0)
		snprintf(buf, sizeof(buf), "%llu GB", n >> 30);
	else if (n >= (1ULL << 20) && (n % (1ULL << 20)) == 0)
		snprintf(buf, sizeof(buf), "%llu MB", n >> 20);
	else if (n >= (1ULL << 10) && (n % (1ULL << 10)) == 0)
		snprintf(buf, sizeof(buf), "%llu KB", n >> 10);
	else
		snprintf(buf, sizeof(buf), "%llu B", n);

	return Widen(buf);
}


// Dicionario (janela) de um item. Deflate/Deflate64 sao fixos; LZMA e BZip2 estao no inicio do fluxo.
// Retorna 0 se nao se aplica ou nao foi possivel ler.
static unsigned long long ReadDictionarySize(FILE *f, const ZipHeader &h)
{
	if (h.method == 8)
		return 32ULL << 10;

	if (h.method == 9)
		return 64ULL << 10;

	if (h.method != 12 && h.method != 14)
		return 0;

	unsigned char lfh[30];

	if (ZIP7_FSEEK64(f, (long long)h.localHeaderOffset, SEEK_SET) != 0 || fread(lfh, 1, 30, f) != 30 || Rd32(lfh) != 0x04034b50)
		return 0;

	const long long nData = (long long)h.localHeaderOffset + 30 + Rd16(lfh + 26) + Rd16(lfh + 28);

	unsigned char buf[9];

	if (ZIP7_FSEEK64(f, nData, SEEK_SET) != 0)
		return 0;

	if (h.method == 12)
	{
		// "BZh1".."BZh9": tamanho do bloco em unidades de 100 KB
		if (fread(buf, 1, 4, f) != 4 || buf[0] != 'B' || buf[1] != 'Z' || buf[2] != 'h' || buf[3] < '1' || buf[3] > '9')
			return 0;

		return (unsigned long long)(buf[3] - '0') * (100ULL << 10);	// 100 KB .. 900 KB
	}

	// LZMA no zip: versao(2) + tamanho das propriedades(2) + props(5: lc/lp/pb + dicionario de 4 bytes)
	if (fread(buf, 1, 9, f) != 9 || Rd16(buf + 2) < 5)
		return 0;

	return (unsigned long long)Rd32(buf + 5);
}


bool DescribeArchive(const wchar_t *strArchive, ArchiveInfo *pInfo, std::wstring *pError)
{
	pInfo->type = L"ZIP";
	pInfo->compression.clear();
	pInfo->dictionary.clear();
	pInfo->method.clear();

	std::vector<ZipHeader> vecHeaders;

	if (!ReadHeaders(strArchive, &vecHeaders, pError))
		return false;

	// ---- nomes dos metodos como o 7-Zip os mostra (ex.: "Deflate:Maximum", "Store")
	std::vector<std::wstring> vecMethodText(vecHeaders.size());

	{
		CMyComPtr<IInArchive> archive;

		if (OpenZip(strArchive, archive, NULL))
		{
			UInt32 nItems = 0;

			if (archive->GetNumberOfItems(&nItems) == S_OK && nItems == vecHeaders.size())
			{
				for (UInt32 i = 0; i < nItems; i++)
				{
					NCOM::CPropVariant prop;

					if (archive->GetProperty(i, kpidMethod, &prop) == S_OK && prop.vt == VT_BSTR)
						vecMethodText[i] = UString(prop.bstrVal).Ptr();
				}
			}

			archive->Close();
		}
	}

	// ---- agrega (ignora pastas e itens vazios sem metodo proprio)
	std::vector<std::wstring> vecBase, vecFull;
	std::vector<unsigned long long> vecDict;			// maior dicionario por algoritmo (mesmo indice de vecBase)

	FILE *f = NULL;
#ifdef _WIN32
	f = _wfopen(strArchive, L"rb");
#else
	f = fopen(UnicodeStringToMultiByte(UString(strArchive)).Ptr(), "rb");
#endif

	for (size_t i = 0; i < vecHeaders.size(); i++)
	{
		const ZipHeader &h = vecHeaders[i];

		if (h.IsDirectory())
			continue;

		const std::wstring strBase = MethodBaseName(h.method);

		std::wstring strFull = vecMethodText[i];
		if (strFull.empty())
			strFull = strBase;

		size_t nBase = 0;
		while (nBase < vecBase.size() && vecBase[nBase] != strBase)
			nBase++;

		if (nBase == vecBase.size())
		{
			vecBase.push_back(strBase);
			vecDict.push_back(0);
		}

		size_t nFull = 0;
		while (nFull < vecFull.size() && vecFull[nFull] != strFull)
			nFull++;

		if (nFull == vecFull.size())
			vecFull.push_back(strFull);

		if (f != NULL)
		{
			const unsigned long long nDict = ReadDictionarySize(f, h);

			if (nDict > vecDict[nBase])
				vecDict[nBase] = nDict;
		}
	}

	if (f != NULL)
		fclose(f);

	for (size_t i = 0; i < vecBase.size(); i++)
	{
		if (i != 0)
			pInfo->compression += L"/";
		pInfo->compression += vecBase[i];

		if (vecDict[i] != 0)
		{
			if (!pInfo->dictionary.empty())
				pInfo->dictionary += L"/";
			pInfo->dictionary += SizeToText(vecDict[i]);
		}
	}

	for (size_t i = 0; i < vecFull.size(); i++)
	{
		if (i != 0)
			pInfo->method += L", ";
		pInfo->method += vecFull[i];
	}

	if (pInfo->compression.empty())
		pInfo->compression = L"-";

	if (pInfo->dictionary.empty())
		pInfo->dictionary = L"-";

	if (pInfo->method.empty())
		pInfo->method = L"-";

	return true;
}

}	// namespace Zip7
