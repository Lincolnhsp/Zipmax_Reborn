// GreenProgress.cpp

#include "stdafx.h"
#include "GreenProgress.h"


BEGIN_MESSAGE_MAP(CGreenProgress, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


CGreenProgress::CGreenProgress()
	: m_nPermille(0)
{
}


BOOL CGreenProgress::Create(CWnd *pParent, const CRect& rect, UINT nID)
{
	const CString strClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(NULL, IDC_ARROW), NULL, NULL);

	return CWnd::Create(strClass, _T(""), WS_CHILD | WS_VISIBLE, rect, pParent, nID);
}


void CGreenProgress::SetPos(int nPermille)
{
	if (nPermille < 0)
		nPermille = 0;

	if (nPermille > 1000)
		nPermille = 1000;

	if (::InterlockedExchange(&m_nPermille, (LONG)nPermille) == (LONG)nPermille)
		return;

	if (m_hWnd != NULL)
		::InvalidateRect(m_hWnd, NULL, FALSE);
}


int CGreenProgress::GetPos() const
{
	return (int)m_nPermille;
}


BOOL CGreenProgress::OnEraseBkgnd(CDC *pDC)
{
	return TRUE;		// tudo e desenhado no OnPaint (sem piscar)
}


void CGreenProgress::OnPaint()
{
	CPaintDC dcPaint(this);

	CRect rectClient;
	GetClientRect(&rectClient);

	if (rectClient.Width() <= 0 || rectClient.Height() <= 0)
		return;

	// desenha tudo numa imagem na memoria e copia de uma vez

	CDC dc;
	dc.CreateCompatibleDC(&dcPaint);

	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&dcPaint, rectClient.Width(), rectClient.Height());
	CBitmap *pOldBitmap = dc.SelectObject(&bitmap);

	// fundo + moldura afundada

	dc.FillSolidRect(rectClient, ::GetSysColor(COLOR_3DFACE));
	dc.DrawEdge(rectClient, EDGE_SUNKEN, BF_RECT);

	CRect rectInner = rectClient;
	rectInner.DeflateRect(2, 2);

	dc.FillSolidRect(rectInner, ::GetSysColor(COLOR_WINDOW));

	// parte cheia (verde)

	const int nPermille = (int)m_nPermille;
	const int nFillWidth = (int)((__int64)rectInner.Width() * nPermille / 1000);

	CRect rectFill = rectInner;
	rectFill.right = rectFill.left + nFillWidth;

	if (nFillWidth > 0)
	{
		dc.FillSolidRect(rectFill, RGB(0, 168, 0));

		// brilho leve na metade de cima
		CRect rectShine = rectFill;
		rectShine.bottom = rectShine.top + rectShine.Height() / 2;
		dc.FillSolidRect(rectShine, RGB(40, 196, 40));
	}

	// porcentagem escrita no centro: branca sobre o verde, preta sobre o resto

	CString strText;
	strText.Format(_T("%d%%"), nPermille / 10);

	CFont *pFont = (GetParent() != NULL) ? GetParent()->GetFont() : NULL;
	CFont *pOldFont = (pFont != NULL) ? dc.SelectObject(pFont) : NULL;

	dc.SetBkMode(TRANSPARENT);

	// parte cheia
	{
		CRgn rgn;
		rgn.CreateRectRgnIndirect(&rectFill);
		dc.SelectClipRgn(&rgn);
		dc.SetTextColor(RGB(255, 255, 255));
		dc.DrawText(strText, rectInner, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	// parte vazia
	{
		CRect rectEmpty = rectInner;
		rectEmpty.left = rectFill.right;

		CRgn rgn;
		rgn.CreateRectRgnIndirect(&rectEmpty);
		dc.SelectClipRgn(&rgn);
		dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
		dc.DrawText(strText, rectInner, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	dc.SelectClipRgn(NULL);

	if (pOldFont != NULL)
		dc.SelectObject(pOldFont);

	dcPaint.BitBlt(0, 0, rectClient.Width(), rectClient.Height(), &dc, 0, 0, SRCCOPY);

	dc.SelectObject(pOldBitmap);
}
