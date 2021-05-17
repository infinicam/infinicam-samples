#pragma once

class CBitmapImage;

class CZoomView : public CScrollView
{
protected:
	CZoomView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CZoomView)
	virtual ~CZoomView();

	virtual void OnDraw(CDC* pDC);
	virtual void OnInitialUpdate();

	int  SetMapMode(CDC* pDC);
	void SetZoomScale(float fScale, CSize sizeImage);

	void ZoomIn(CSize imgSize);
	void ZoomOut(CSize imgSize);
	void ZoomFit(CSize imgSize);
	void ZoomDefault(CSize imgSize);

protected:
	float m_fZoomFactor;
	float m_fMinZoomFactor;
	float m_fMaxZoomFactor;

private:
	CDC* m_pdcMemory;
	CBitmap* m_pBitmap;
	BOOL m_bInitialSize;
	CSize m_layout;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

