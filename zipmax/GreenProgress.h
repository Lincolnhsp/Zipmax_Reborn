// GreenProgress.h
//
// Barra de progresso verde, com a porcentagem escrita dentro. Usada para mostrar o andamento
// total da compactacao. A posicao vai de 0 a 1000 (decimos de por cento) para encher suavemente.

#pragma once

class CGreenProgress : public CWnd
{
	public:

		CGreenProgress();

		BOOL Create(CWnd *pParent, const CRect& rect, UINT nID);

		// 0..1000 (1000 = 100%). Pode ser chamada de outra thread.
		void SetPos(int nPermille);
		int GetPos() const;

	protected:

		volatile LONG m_nPermille;

		afx_msg void OnPaint();
		afx_msg BOOL OnEraseBkgnd(CDC *pDC);

		DECLARE_MESSAGE_MAP()
};
