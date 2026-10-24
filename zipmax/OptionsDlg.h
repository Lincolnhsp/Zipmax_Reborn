// OptionsDlg.h - Configuracoes > Opcoes (zipmax.ini)
//
// Mostra TODOS os parametros do zipmax.ini numa lista com caixas de selecao:
//   * parametros liga/desliga (work-recursive, logfile, nobeep...): marcado = 1, desmarcado = 0;
//   * parametros com valor (thread-priority, language, sevenzip-dll, finalstep-*, packer-*): a caixa ativa ou
//     desativa a linha no ini (desativada = comentada com //) e o valor e editado abaixo da lista
//     (texto, caminho com botao Procurar, ou lista de opcoes).
// Os empacotadores externos (packer-exe-N / packer-cmd-N) podem ser acrescentados e removidos.
//
// Ao confirmar, o arquivo e gravado por CIniFile (comentarios preservados, backup em zipmax.ini.bak) e
// CZipMaxApp::ReloadSettings() recarrega tudo, sem precisar reiniciar o programa.
//
// COMO ACRESCENTAR UM PARAMETRO NOVO: veja docs\DEVELOPMENT.md ("Parametros do zipmax.ini").

#pragma once

#include <string>
#include <vector>

class COptionsDlg : public CDialog
{
	public:

		COptionsDlg(CWnd *pParent = NULL);

		enum { IDD = IDD_OPTIONS };

	protected:

		enum OptType
		{
			OPT_BOOL,				// 1 / 0
			OPT_TEXT,				// texto livre
			OPT_PATH,				// texto com botao Procurar (arquivo)
			OPT_PRIORITY,			// lista: prioridades da thread
			OPT_LANGUAGE,			// lista: arquivos language\*.lng
			OPT_PACKEREXE,			// packer-exe-N (com Procurar)
			OPT_PACKERCMD			// packer-cmd-N
		};

		struct OPTION
		{
			CString strKey;			// chave no ini (ex.: "logfile", "packer-exe-2")
			OptType type;
			BOOL bOn;				// caixa marcada
			CString strValue;		// valor (para OPT_BOOL: nao usado)
			UINT nDescId;			// texto (IDS_xxx) que descreve a configuracao
			int nPacker;			// numero do empacotador (so OPT_PACKEREXE / OPT_PACKERCMD)
		};

		std::vector<OPTION> m_opts;
		CListCtrl m_list;
		CComboBox m_combo;
		BOOL m_bBusy;				// TRUE enquanto o programa mexe nos controles (ignora as notificacoes)
		int m_nSel;					// linha selecionada

		virtual void DoDataExchange(CDataExchange *pDX);
		virtual BOOL OnInitDialog();
		virtual void OnOK();

		afx_msg void OnListChanged(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnValueChanged();
		afx_msg void OnComboChanged();
		afx_msg void OnBrowse();
		afx_msg void OnAddPacker();
		afx_msg void OnRemovePacker();

		DECLARE_MESSAGE_MAP()

	private:

		void LoadOptions();							// m_opts <- zipmax.ini
		void AddOption(const CString& strKey, OptType type, BOOL bOn, const CString& strValue, UINT nDescId, int nPacker = 0);
		void FillList();							// lista <- m_opts
		void UpdateEditor();						// mostra o editor certo para a linha selecionada
		void SelectRow(int nRow);
		CString DescText(const OPTION& opt) const;
		CString ValueText(const OPTION& opt) const;
		int CountPackers() const;
		int LastPackerRow() const;					// indice do ultimo OPT_PACKERCMD (ou -1)

		BOOL ValidateOptions();
		void BuildBlock(std::vector<std::wstring> *pvecBlock) const;
		BOOL SaveIni();
};
