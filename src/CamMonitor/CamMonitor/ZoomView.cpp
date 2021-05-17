#include "stdafx.h"
#include "ZoomView.h"
#include "Bitmap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CZoomView, CScrollView)

CZoomView::CZoomView()
	: m_pdcMemory(new CDC)
	, m_pBitmap(new CBitmap)
	, m_bInitialSize(FALSE)
	, m_fZoomFactor(1.0f)
	, m_fMinZoomFactor(0.1f)
	, m_fMaxZoomFactor(3.0f)
{
}

CZoomView::~CZoomView()
{
	if (m_pdcMemory != NULL)
	{
		delete m_pdcMemory;
		m_pdcMemory = NULL;
	}

	if (m_pBitmap != NULL)
	{
		delete m_pBitmap;
		m_pBitmap = NULL;
	}
}

BEGIN_MESSAGE_MAP(CZoomView, CScrollView)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CZoomView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

	m_layout.cx = 1;
	m_layout.cy = 1;
	SetScrollSizes(MM_TEXT, m_layout);

	m_bInitialSize = TRUE;

	if (m_pdcMemory->GetSafeHdc() == NULL)
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		m_pdcMemory->CreateCompatibleDC(&dc);

		// makes bitmap same size as display window
		CRect clientRect(0, 0, 0, 0);
		GetClientRect(clientRect);
		if (m_pBitmap != NULL)
		{
			delete m_pBitmap;
			m_pBitmap = NULL;
		}
		m_pBitmap = new CBitmap();
		m_pBitmap->CreateCompatibleBitmap(&dc, clientRect.right, clientRect.bottom);
	}
}

void CZoomView::OnDraw(CDC* /*pDC*/)
{
	// TODO: Add your specialized code here and/or call the base class
}

void CZoomView::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	OnPrepareDC(&dc);
	SetMapMode(&dc);

	// Set the same viewport origin, because our bitmap
	// is created for drawing area only
	CPoint viewportOrg = dc.GetViewportOrg();
	m_pdcMemory->SetViewportOrg(viewportOrg);

	CBitmap* pOldBitmap = m_pdcMemory->SelectObject(m_pBitmap);
	CRect rectUpdate(0, 0, 0, 0);
	dc.GetClipBox(&rectUpdate);
	rectUpdate.InflateRect(1, 1, 1, 1);
	m_pdcMemory->SelectClipRgn(NULL);
	m_pdcMemory->IntersectClipRect(&rectUpdate);
	CBrush backgroundBrush((COLORREF) GetSysColor(COLOR_MENU));
	CBrush* pOldBrush = m_pdcMemory->SelectObject(&backgroundBrush);
	m_pdcMemory->PatBlt(rectUpdate.left, rectUpdate.top,
		rectUpdate.Width(), rectUpdate.Height(), PATCOPY);
	m_pdcMemory->SetStretchBltMode(HALFTONE);

	// draw section 
	OnDraw(m_pdcMemory);

	dc.BitBlt(rectUpdate.left, rectUpdate.top, rectUpdate.Width(), rectUpdate.Height(),
		m_pdcMemory, rectUpdate.left, rectUpdate.top, SRCCOPY);
	m_pdcMemory->SelectObject(pOldBitmap);
	m_pdcMemory->SelectObject(pOldBrush);
}

void CZoomView::OnSize(UINT nType, int cx, int cy)
{
	CScrollView::OnSize(nType, cx, cy);
	if (m_bInitialSize == FALSE)
		return;

	CPaintDC dc(this); // device context for painting
	OnPrepareDC(&dc);
	SetMapMode(&dc);

	CRect clientRect(0, 0, 0, 0);
	GetClientRect(&clientRect);

	if (m_pBitmap != NULL)
	{
		delete m_pBitmap;
		m_pBitmap = NULL;
	}
	m_pBitmap = new CBitmap;
	m_pBitmap->CreateCompatibleBitmap(&dc, clientRect.Width(), clientRect.Height());
	TRACE("Bitmap's size changed: %d, %d\n", clientRect.Width(), clientRect.Height());
	Invalidate(FALSE);
}

BOOL CZoomView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default

	return CScrollView::OnEraseBkgnd(pDC);
}

void CZoomView::SetZoomScale(float fScale, CSize sizeImage)
{
	if (fScale <= m_fMinZoomFactor)
		m_fZoomFactor = m_fMinZoomFactor;
	else if (fScale >= m_fMaxZoomFactor)
		m_fZoomFactor = m_fMaxZoomFactor;
	else
		m_fZoomFactor = fScale;

	m_layout.cx = (int)(m_fZoomFactor * sizeImage.cx);
	m_layout.cy = (int)(m_fZoomFactor * sizeImage.cy);

	SetScrollSizes(MM_TEXT, m_layout);

	SetMapMode(m_pdcMemory);

	Invalidate(FALSE);
}

int CZoomView::SetMapMode(CDC* pDC)
{
	int previousMode = pDC->SetMapMode(MM_ISOTROPIC);
	pDC->SetWindowExt(100, 100);
	pDC->SetViewportExt((int)(100 * m_fZoomFactor), (int)(100 * m_fZoomFactor));

	return previousMode;
}

void CZoomView::ZoomIn(CSize imgSize)
{
	if ((imgSize.cx == 0) || (imgSize.cy == 0))
		return;

	SetZoomScale(m_fZoomFactor + 0.1f, imgSize);
}

void CZoomView::ZoomOut(CSize imgSize)
{
	if ((imgSize.cx == 0) || (imgSize.cy == 0))
		return;

	SetZoomScale(m_fZoomFactor - 0.1f, imgSize);
}

void CZoomView::ZoomFit(CSize imgSize)
{
	if ((imgSize.cx == 0) || (imgSize.cy == 0))
		return;

	CRect rect;
	GetClientRect(&rect);
	float fscale = 1;
	fscale = min((float)rect.Height() / imgSize.cy, (float)rect.Width() / imgSize.cx);

	SetZoomScale(fscale, imgSize);
}

void CZoomView::ZoomDefault(CSize imgSize)
{
	if ((imgSize.cx == 0) || (imgSize.cy == 0))
		return;

	SetZoomScale(1.0f, imgSize);
}
