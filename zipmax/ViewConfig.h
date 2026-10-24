// ViewConfig.h
//
// Configuracao da JANELA PRINCIPAL: quais colunas da lista e quais itens da base da janela aparecem e em que
// ordem. E editada em Configuracoes > Caixa de dialogo (CViewSettingsDlg) e guardada no registro
// (HKCU\Software\ZipMax, secao "View"), junto com a posicao/tamanho da janela.
//
// Padrao = visual original do ZipMax: colunas File, Status e Gained Free Bytes; barra de progresso classica,
// texto da acao atual e texto de resumo. Tudo o que foi acrescentado depois (Size Before, Size After, informacoes
// do metodo, barra verde) fica desligado ate o usuario ligar.
//
// COMO ACRESCENTAR UMA COLUNA OU ITEM NOVO: veja docs\DEVELOPMENT.md ("Colunas e itens da janela").

#pragma once

// Colunas da lista. O numero e o indice LOGICO da coluna (e o subitem) e nunca muda; a posicao na tela
// vem de m_nColumnOrder.
enum
{
	COL_FILE = 0,
	COL_STATUS,
	COL_SIZEBEFORE,
	COL_SIZEAFTER,
	COL_INFO,
	COL_GAINED,
	NUMCOLUMNS
};

// Itens da base da janela, na ordem em que aparecem por padrao (de cima para baixo).
enum
{
	ITEM_PROGRESS = 0,		// barra de progresso classica (a linha tambem tem o botao Pausar, que nunca some)
	ITEM_STATUS,			// texto da acao atual
	ITEM_SUMMARY,			// texto de resumo (Done, arquivos/tempo restantes, bytes economizados)
	ITEM_TOTALBAR,			// barra de progresso total (verde)
	NUMITEMS
};


class CViewConfig
{
	public:

		CViewConfig();

		void SetDefaults();
		void Load(CWinApp *pApp);				// le do registro (valores invalidos/ausentes = padrao)
		void Save(CWinApp *pApp) const;			// grava no registro

	public:

		BOOL m_bColumnVisible[NUMCOLUMNS];		// a coluna Arquivo (COL_FILE) e sempre visivel
		int m_nColumnOrder[NUMCOLUMNS];			// coluna logica mostrada em cada posicao, da esquerda para a direita
		BOOL m_bItemVisible[NUMITEMS];
		int m_nItemOrder[NUMITEMS];				// item mostrado em cada posicao, de cima para baixo

	public:

		static UINT ColumnNameId(int nColumn);	// texto (IDS_xxx) do nome da coluna
		static UINT ItemNameId(int nItem);		// texto (IDS_xxx) do nome do item

		static BOOL IsValidOrder(const int *pnOrder, int nCount);		// e uma permutacao de 0..nCount-1 ?
		static CString OrderToString(const int *pnOrder, int nCount);	// "0,1,2,3"
		static BOOL StringToOrder(const CString& str, int *pnOrder, int nCount);
};
