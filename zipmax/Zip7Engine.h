// Zip7Engine.h
//
// Camada de acesso ao 7-Zip usada pelo ZipMax (substitui a antiga ZipArchive).
//
// Esta interface NAO depende de MFC nem expoe tipos do 7-Zip. Todos os caminhos e mensagens sao
// Unicode (UTF-16, wchar_t), o que permite nomes de arquivo em qualquer idioma (japones etc.).
//
// Dois modos de build (escolhidos em tempo de compilacao):
//
//   * DLL (padrao)   : o 7z.dll e carregado em tempo de execucao (LoadLibrary).
//   * ZIPMAX_STATIC_7Z : o codigo do 7-Zip (handler de ZIP + Deflate) e compilado e ligado
//                        dentro do ZipMax.exe; nao precisa de 7z.dll.

#pragma once

#include <string>
#include <vector>

#if defined(_MSC_VER) && (_MSC_VER < 1600)
	typedef unsigned __int16 zip7_u16;
	typedef unsigned __int32 zip7_u32;
#else
	#include <stdint.h>
	typedef uint16_t zip7_u16;
	typedef uint32_t zip7_u32;
#endif

namespace Zip7
{
	// Cabecalho (entrada do "central directory") de um item de um zip, com os campos
	// "crus" como estao gravados no arquivo. O ZipMax precisa deles porque monta o zip
	// final copiando os fluxos comprimidos bit a bit (ver CZipMaxDlg::BuildNewZip).

	struct ZipHeader
	{
		std::string  rawName;			// nome como gravado no zip (bytes)
		std::wstring name;				// nome decodificado, com '/' como separador
		std::wstring key;				// 'name' em minusculas: chave para comparar nomes sem diferenciar maiusculas

		zip7_u16 versionMadeBy;
		zip7_u16 versionNeeded;
		zip7_u16 flags;
		zip7_u16 method;
		zip7_u16 modTime;				// formato DOS
		zip7_u16 modDate;				// formato DOS
		zip7_u16 internalAttr;
		zip7_u32 crc32;
		zip7_u32 comprSize;
		zip7_u32 uncomprSize;
		zip7_u32 externalAttr;
		zip7_u32 localHeaderOffset;

		ZipHeader();

		bool IsDirectory() const;		// termina com '/' ou '\\'
	};

	// Opcoes da compressao "interna" (a que antes era feita pelo ZipArchive/zlib).
	struct CompressOptions
	{
		unsigned level;					// 0..9  (padrao 9)
		unsigned passes;				// passadas do Deflate do 7-Zip (padrao 15)
		unsigned fastBytes;				// 3..258 (padrao 258)

		CompressOptions();
	};

	// Resumo do zip resultante, para a coluna de informacao da lista do ZipMax.
	// Exemplo: type="ZIP", compression="Deflate/Store", dictionary="32 KB", method="Deflate:Maximum, Store"
	struct ArchiveInfo
	{
		std::wstring type;				// "ZIP"
		std::wstring compression;		// algoritmos usados ("/" entre eles)
		std::wstring dictionary;		// tamanho do dicionario/janela ("-" se nao se aplica)
		std::wstring method;			// metodos exatos informados pelo 7-Zip (", " entre eles)
	};

	// Carrega o motor do 7-Zip. dllPath pode ser NULL/"" (usa a pasta do .exe, depois o
	// PATH, depois "C:\Program Files\7-Zip"). No build estatico nao faz nada.
	bool Initialize(const wchar_t *dllPath, std::wstring *pError);
	void Shutdown();

	// Descompacta TODOS os itens de strArchive em strDestDir (a pasta tem que existir).
	// Datas e atributos dos arquivos sao restaurados. Caminhos com ".." ou absolutos sao rejeitados.
	bool ExtractAll(const wchar_t *strArchive, const wchar_t *strDestDir, std::wstring *pError);

	// Testa todos os itens (CRC + descompressao). true somente se tudo estiver ok.
	bool TestArchive(const wchar_t *strArchive, std::wstring *pError);

	// Cria um zip novo (sobrescreve se existir) com os arquivos relativos a strBaseDir.
	bool CompressFiles(const wchar_t *strArchive, const wchar_t *strBaseDir, const std::vector<std::wstring> &vecRelFiles, const CompressOptions &opt, std::wstring *pError);

	// Descreve o metodo de compressao de um zip (usa o 7-Zip + leitura dos cabecalhos).
	bool DescribeArchive(const wchar_t *strArchive, ArchiveInfo *pInfo, std::wstring *pError);

	// Le o "central directory" do zip. Falha para zips multi-volume e zip64.
	bool ReadHeaders(const wchar_t *strArchive, std::vector<ZipHeader> *pvecHeaders, std::wstring *pError);
}
