#pragma once

#include "ZoomView.h"

class CChildView : public CZoomView
{
public:
	CChildView();
	DECLARE_DYNCREATE(CChildView)
	virtual ~CChildView();

public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnDraw(CDC* pDC);
	virtual void OnInitialUpdate();
	virtual void DrawRotatedImage(CDC* pDC, CBitmapImage* pImage, int rotationCount, int x, int y);

private:
	int m_rotationCount; // 0: 0 deg, 1: 90 deg, 2: 180 deg, 3: 270 deg

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg void OnZoomFit();
	afx_msg void OnZoomDefault();
	afx_msg void OnRotate();
};

