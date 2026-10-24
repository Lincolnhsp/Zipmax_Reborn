// ViewSettingsDlg.h - Configuracoes > Caixa de dialogo
//
// Duas listas com caixas de selecao: as COLUNAS da lista de arquivos e os ITENS da base da janela.
// Marcar uma caixa mostra o item, desmarcar esconde. Os botoes Subir/Descer mudam a ordem
// (colunas: da esquerda para a direita; itens: de cima para baixo). "Restaurar padroes" volta ao visual original.
// O resultado fica em m_cfg (o chamador aplica na janela e grava no registro).

#pragma once

#include "ViewConfig.h"

class CViewSettingsDlg : public CDialog
{
	public:

		CViewSettingsDlg(const CViewConfig& cfg, CWnd *pParent = NULL);

		enum { IDD = IDD_VIEWSETTINGS };

		CViewConfig m_cfg;				// entrada e saida

	protected:

		CListCtrl m_listCols;
		CListCtrl m_listItems;
		BOOL m_bBusy;					// TRUE enquanto o programa mexe nas listas (ignora as notificacoes)

		virtual void DoDataExchange(CDataExchange *pDX);
		virtual BOOL OnInitDialog();
		virtual void OnOK();

		afx_msg void OnColUp();
		afx_msg void OnColDown();
		afx_msg void OnItemUp();
		afx_msg void OnItemDown();
		afx_msg void OnDefaults();
		afx_msg void OnColsChanged(NMHDR *pNMHDR, LRESULT *pResult);

		DECLARE_MESSAGE_MAP()

	private:

		void Fill();					// listas <- m_cfg
		void Collect();					// m_cfg <- listas
		void MoveSelected(CListCtrl& list, int nDelta);
		static void PrepareList(CListCtrl& list);
};
