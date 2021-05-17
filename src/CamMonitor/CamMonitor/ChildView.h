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

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg void OnZoomFit();
	afx_msg void OnZoomDefault();
};

