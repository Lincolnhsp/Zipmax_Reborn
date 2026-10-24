// IniFile.h
//
// Leitura e gravacao da secao [ZIPMAX] do zipmax.ini SEM perder os comentarios do arquivo.
//
// Usado pela tela Configuracoes > Opcoes. Esta classe nao depende de MFC (so da biblioteca padrao),
// para poder ser testada isoladamente.
//
// Como a gravacao funciona
//   * Os parametros que a tela de opcoes controla ("chaves gerenciadas", ver IsManagedKey) sao removidos de
//     onde estavam e regravados juntos, num bloco logo abaixo do cabecalho [ZIPMAX], entre dois marcadores.
//   * Todo o resto do arquivo (comentarios, exemplos, outras secoes) e preservado como estava.
//   * Antes de gravar, o arquivo anterior e copiado para zipmax.ini.bak.
//   * O arquivo e gravado em UTF-8 com BOM (o ZipMax le UTF-8, UTF-16 ou ANSI).

#pragma once

#include <string>
#include <vector>

class CIniFile
{
	public:

		// Le o arquivo. false se nao foi possivel le-lo.
		bool Load(const std::wstring& strPath);

		// Grava o arquivo (e o .bak do arquivo anterior). false se nao foi possivel gravar.
		bool Save(const std::wstring& strPath) const;

		// Valor (como esta escrito no arquivo, sem mudar maiusculas/minusculas) de uma chave ATIVA da secao
		// [ZIPMAX]. Retorna false se a chave nao existe ou esta comentada. strKey em qualquer caixa.
		bool Get(const std::wstring& strKey, std::wstring *pstrValue) const;

		// Remove as linhas ativas das chaves gerenciadas e escreve vecBlock (linhas sem fim de linha) logo
		// abaixo do cabecalho [ZIPMAX] (que e criado se nao existir). Chamar de novo substitui o bloco anterior.
		void ReplaceManagedBlock(const std::vector<std::wstring>& vecBlock);

		// Define uma unica chave da secao [ZIPMAX] sem mexer no resto: se a chave ja esta ativa, troca o valor na mesma
		// linha; senao escreve "chave = valor" dentro do bloco gerenciado (ou logo abaixo do cabecalho, se nao houver
		// bloco). O cabecalho e criado se nao existir. Usado pelo menu Configuracoes > Idioma.
		void SetValue(const std::wstring& strKey, const std::wstring& strValue);

		// true se a chave (em minusculas) e controlada pela tela de opcoes
		static bool IsManagedKey(const std::wstring& strKeyLower);

		// marcadores do bloco gerenciado
		static const wchar_t *BlockBegin();
		static const wchar_t *BlockEnd();

		const std::vector<std::wstring>& Lines() const { return m_vecLines; }


	private:

		std::vector<std::wstring> m_vecLines;

		bool FindSection(size_t *pnHeader, size_t *pnEnd) const;		// [ZIPMAX]: indice do cabecalho e fim (exclusivo)
		static bool ParseActiveLine(const std::wstring& strLine, std::wstring *pstrKeyLower, std::wstring *pstrValue);
};
