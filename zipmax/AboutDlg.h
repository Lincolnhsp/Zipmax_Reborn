// AboutDlg.h - caixa "Sobre" (Sobre > Sobre o ZipMax Reborn)
//
// O texto (o que e o ZipMax Reborn + a lista de melhorias por versao) vem do texto IDS_ABOUT_TEXT, que
// existe em ZipMax.rc (ingles) e em language\*.lng. A versao vem de Version.h.
// Ao implementar uma melhoria nova, acrescente-a ao IDS_ABOUT_TEXT e ao docs\CHANGELOG.md.

#pragma once

class CAboutDlg : public CDialog
{
	public:

		CAboutDlg(CWnd *pParent = NULL);

		enum { IDD = IDD_ABOUT };

	protected:

		virtual BOOL OnInitDialog();
};
