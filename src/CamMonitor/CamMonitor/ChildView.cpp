// ChildView.cpp : implementation of the CChildView class
//
#include "stdafx.h"
#include "ChildView.h"
#include "Bitmap.h"
#include "resource.h"
#include "AppDefine.h"
#include "MainFrm.h"
#include "BaseTab.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CChildView, CZoomView)

CChildView::CChildView()
	: CZoomView()
{
}

CChildView::~CChildView()
{
}

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CZoomView::PreCreateWindow(cs))
		return FALSE;

	return TRUE;
}

void CChildView::OnInitialUpdate()
{
	CZoomView::OnInitialUpdate();
}

void CChildView::OnDraw(CDC* pDC)
{
	CBaseTab* pTab = GET_ACTIVE_TAB();
	if (!pTab)
		return;

	// image
	LockImage* pLockImage = pTab->GetLockImage();
	// ***Image Lock***
	CBitmapImage* p = (CBitmapImage*)pLockImage->GetLockData();
	CRect rc(CPoint(0, 0), CSize(p->GetWidth(), p->GetHeight()));

	StretchDIBits(pDC->GetSafeHdc(),
		0, 0, rc.Width(), rc.Height(),
		0, 0, rc.Width(), rc.Height(),
		p->GetBuffer(), p->GetBitmapInfo(), DIB_RGB_COLORS, SRCCOPY);

	/////////////////////
	// text
	CString s;
	int oldBkMode = pDC->SetBkMode(TRANSPARENT);
	int oldTextColor = pDC->SetTextColor(RGB(0, 255, 0));

	LockBuffer* pLockTextInfo = pTab->GetLockTextInfo();
	// ***Text Lock***
	PTEXTINFO pTextInfo = (PTEXTINFO)pLockTextInfo->GetLockData();
	s.Format(IDS_DRAWTEXT_CAMERA, 
		pTextInfo->nSeqNo,
		pTextInfo->nTemp);
	pDC->DrawText(s, rc, DT_WORDBREAK);
	pLockTextInfo->Unlock();
	// ***Text Unlock***

	pDC->SetTextColor(oldTextColor);
	pDC->SetBkMode(oldBkMode);
	/////////////////////

	pLockImage->Unlock();
	// ***Image Unlock***
}

BEGIN_MESSAGE_MAP(CChildView, CZoomView)
	ON_COMMAND(ID_EDIT_ZOOM_IN, &CChildView::OnZoomIn)
	ON_COMMAND(ID_EDIT_ZOOM_OUT, &CChildView::OnZoomOut)
	ON_COMMAND(ID_EDIT_ZOOM_FIT, &CChildView::OnZoomFit)
	ON_COMMAND(ID_EDIT_ZOOM_DEFAULT, &CChildView::OnZoomDefault)
END_MESSAGE_MAP()

void CChildView::OnZoomIn()
{
	TRACE(_T("On zoom in command\n"));
	CBaseTab* pTab = GET_ACTIVE_TAB();
	if (!pTab)
		return;
	// ***Lock***
	LockImage* pLock = pTab->GetLockImage();
	CBitmapImage* p = (CBitmapImage*)pLock->GetLockData();
	ZoomIn(CSize(p->GetWidth(), p->GetHeight()));
	pLock->Unlock();
	// ***Unlock***
}

void CChildView::OnZoomOut()
{
	TRACE(_T("On zoom out command\n"));
	CBaseTab* pTab = GET_ACTIVE_TAB();
	if (!pTab)
		return;
	// ***Lock***
	LockImage* pLock = pTab->GetLockImage();
	CBitmapImage* p = (CBitmapImage*)pLock->GetLockData();
	ZoomOut(CSize(p->GetWidth(), p->GetHeight()));
	pLock->Unlock();
	// ***Unlock***
}

void CChildView::OnZoomFit()
{
	TRACE(_T("On zoom FIT command\n"));
	CBaseTab* pTab = GET_ACTIVE_TAB();
	if (!pTab)
		return;
	// ***Lock***
	LockImage* pLock = pTab->GetLockImage();
	CBitmapImage* p = (CBitmapImage*)pLock->GetLockData();
	ZoomFit(CSize(p->GetWidth(), p->GetHeight()));
	pLock->Unlock();
	// ***Unlock***
}

void CChildView::OnZoomDefault()
{
	TRACE(_T("On zoom default command\n"));
	CBaseTab* pTab = GET_ACTIVE_TAB();
	if (!pTab)
		return;
	// ***Lock***
	LockImage* pLock = pTab->GetLockImage();
	CBitmapImage* p = (CBitmapImage*)pLock->GetLockData();
	ZoomDefault(CSize(p->GetWidth(), p->GetHeight()));
	pLock->Unlock();
	// ***Unlock***
}
