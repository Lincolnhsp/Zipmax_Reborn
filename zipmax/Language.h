// Language.h
//
// Textos do ZipMax em varios idiomas.
//
// Os textos em ingles ficam embutidos no programa (string table do ZipMax.rc) e sao sempre o
// padrao. Os arquivos da subpasta "language" (language\<Nome>.lng, UTF-8) substituem esses textos.
// O idioma e escolhido no zipmax.ini:   language = Portugues-Brasil   (ou "auto" = idioma do Windows)

#pragma once

#include <map>
#include <vector>

class CLang
{
	public:

		// strMainFolder = pasta do ZipMax.exe (com "\" no final); strLanguage = nome do arquivo
		// sem ".lng" ("" ou "English" = ingles embutido, "auto" = idioma do Windows)
		static void Init(const CString& strMainFolder, const CString& strLanguage);

		// texto de um IDS_xxx: do arquivo de idioma ou, se nao existir la, o ingles embutido
		static CString Get(UINT nID);

		// texto traduzido de um controle de dialogo (ex.: IDC_BUTTON_PAUSE); FALSE se nao houver
		static BOOL TryGet(UINT nID, CString *pstrText);

		// traduz o texto dos controles de um dialogo (os que tiverem chave IDC_xxx no arquivo de idioma) e,
		// se nCaptionId != 0, o titulo da janela (IDS_xxx). Controles sem traducao ficam como no .rc (ingles).
		static void TranslateDialog(CWnd *pDlg, UINT nCaptionId, const UINT *pnControlIds, int nCount);

		// idiomas disponiveis: "English" (embutido) + um por arquivo language\*.lng. pvecNames recebe o nome do arquivo
		// (sem .lng; e o valor da chave "language" do zipmax.ini) e pvecDisplay o nome para mostrar no menu
		// (chave IDS_LANGNAME do arquivo; se faltar, o proprio nome do arquivo).
		static void ListLanguages(const CString& strMainFolder, std::vector<CString> *pvecNames, std::vector<CString> *pvecDisplay);

		// nome do idioma em uso ("English" se nenhum arquivo foi carregado)
		static const CString& GetName();

	private:

		static BOOL ParseFile(const CString& strFile, std::map<UINT, CString> *pmapOut);
		static BOOL LoadFile(const CString& strFile);
		static UINT FindId(const CString& strKey);

		static std::map<UINT, CString> s_mapStrings;
		static CString s_strName;
};
