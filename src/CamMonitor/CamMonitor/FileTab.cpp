﻿#include "stdafx.h"
#include "FileTab.h"
#include "resource.h"
#include "AppDefine.h"
#include "CamMonitor.h"
#include "ChildView.h"
#include "PUCLIB.h"
#include "ProgressDialog.h"
#include "Util.h"
#include "SaveFileDialog.h"

#define PAGE_FRAME_COUNT 20

IMPLEMENT_DYNAMIC(CFileTab, CBaseTab)

CFileTab::CFileTab(CWnd* pParent)
	: CBaseTab(CFileTab::IDD, _T("FILE"), pParent)
	, m_bOpened(FALSE)
	, m_bPlaying(FALSE)
	, m_pData(NULL)
	, m_lockTextInfo(sizeof(TEXTINFO))
	, m_xvCurrentFrame(0)
	, m_xvStartFrame(0)
	, m_xvEndFrame(0)
{
}

CFileTab::~CFileTab()
{
}

void CFileTab::DoDataExchange(CDataExchange* pDX)
{
	CBaseTab::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_CURRENT_FRAME, m_xvCurrentFrame);
	DDX_Control(pDX, IDC_PLAY, m_buttonPlay);
	DDX_Text(pDX, IDC_START_FRAME, m_xvStartFrame);
	DDX_Text(pDX, IDC_END_FRAME, m_xvEndFrame);
	DDX_Control(pDX, IDC_SLIDER_PLAY, m_sldrFrameCtrl);
}

void CFileTab::StartLive()
{
	SetTimer(TIMER_ID_UPDATE_IMG, UPDATE_IMG_INTERVAL, NULL);
	m_bPlaying = TRUE;
}

void CFileTab::StopLive()
{
	KillTimer(TIMER_ID_UPDATE_IMG);
	m_bPlaying = FALSE;
}

void CFileTab::UpdateControlState()
{
	CString s;

	GetDlgItem(IDC_OPEN)->EnableWindow(!m_bPlaying);
	GetDlgItem(IDC_CURRENT_FRAME)->EnableWindow(m_bOpened && !m_bPlaying);
	GetDlgItem(IDC_START_FRAME)->EnableWindow(m_bOpened && !m_bPlaying);
	GetDlgItem(IDC_POST_START_FRAME)->EnableWindow(m_bOpened && !m_bPlaying);
	GetDlgItem(IDC_POST_END_FRAME)->EnableWindow(m_bOpened && !m_bPlaying);
	GetDlgItem(IDC_END_FRAME)->EnableWindow(m_bOpened && !m_bPlaying);
	GetDlgItem(IDC_FORWARD_FRAME)->EnableWindow(m_bOpened && !m_bPlaying);
	GetDlgItem(IDC_BACKWARD_FRAME)->EnableWindow(m_bOpened && !m_bPlaying);
	GetDlgItem(IDC_GO_TO_START)->EnableWindow(m_bOpened && !m_bPlaying);
	GetDlgItem(IDC_GO_TO_END)->EnableWindow(m_bOpened && !m_bPlaying);
	m_buttonPlay.EnableWindow(m_bOpened);
	m_sldrFrameCtrl.EnableWindow(m_bOpened && !m_bPlaying);
	GetDlgItem(IDC_SAVE_FRAME_INFO)->EnableWindow(m_bOpened && !m_bPlaying);
	GetDlgItem(IDC_SAVE_TO_FILE)->EnableWindow(m_bOpened && !m_bPlaying);

	if (m_bPlaying)
	{
		s.LoadString(IDS_PAUSE);
		m_buttonPlay.SetWindowText(s);
	}
	else
	{
		s.LoadString(IDS_PLAY);
		m_buttonPlay.SetWindowText(s);
	}
}

BOOL CFileTab::OpenCIH(const CString& filePath)
{
	CString s;
	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();

	CloseCIH();

	if (!m_cih.Read(filePath))
		return FALSE;

	m_bOpened = TRUE;

	// File info
	GetDlgItem(IDC_FILEPATH)->SetWindowText(m_cih.m_filepath);
	s.Format(_T("%u"), m_cih.m_framerate);
	GetDlgItem(IDC_FILE_FRAMERATE)->SetWindowText(s);
	s.Format(_T("%u x %u"), m_cih.m_width, m_cih.m_height);
	GetDlgItem(IDC_IMAGE_SIZE)->SetWindowText(s);
	s.Format(_T("%u"), m_cih.m_frameCount);
	GetDlgItem(IDC_FRAME_COUNT)->SetWindowText(s);

	// Playback
	m_xvCurrentFrame = m_xvStartFrame = 0;
	m_xvEndFrame = m_cih.m_frameCount - 1;
	m_sldrFrameCtrl.SetRange(m_xvStartFrame, m_xvEndFrame);
	m_sldrFrameCtrl.SetPageSize(m_cih.m_frameCount / PAGE_FRAME_COUNT);
	m_sldrFrameCtrl.SetPos(m_xvCurrentFrame);

	UpdateData(FALSE);

	// Update Spin Control
	((CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_START_FRAME))->SetRange32(0, INT32_MAX);
	((CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_END_FRAME))->SetRange32(0, INT32_MAX);
	((CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_CURRENT_FRAME))->SetRange32(0, INT32_MAX);
	((CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_START_FRAME))->SetPos32(m_xvStartFrame);
	((CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_END_FRAME))->SetPos32(m_xvEndFrame);
	((CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_CURRENT_FRAME))->SetPos32(m_xvCurrentFrame);

	UpdateControlState();

	df.fileOpenFolderPath = m_cih.m_dirpath;

	return TRUE;
}

void CFileTab::CloseCIH()
{
	StopLive();

	m_bOpened = FALSE;

	m_cih.Clear();

	UpdateControlState();
}

BOOL CFileTab::OpenDataFile()
{
	CloseDataFile();

	m_lockImage.SetWidth(m_cih.m_width);
	m_lockImage.SetHeight(m_cih.m_height);
	m_lockImage.SetColor(m_cih.m_colortype != PUC_COLOR_MONO);
	m_lockImage.Create();

	m_pData = new UINT8[m_cih.m_frameSize];

	ReadDataFileAndDecode(m_xvCurrentFrame);
	::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);

	return TRUE;
}

BOOL CFileTab::ReadDataFile(UINT32 nFrameNo)
{
	CString filePath;

	if (m_cih.m_filetype == SAVE_FILE_TYPE_RAW)
	{
		filePath.Format(_T("%s\\%s.mdat"), (LPCTSTR)m_cih.m_dirpath, (LPCTSTR)m_cih.m_filename);
		FILE* fp = NULL;
		errno_t error = _tfopen_s(&fp, filePath, _T("rb"));
		if (error != 0)
			return FALSE;
		INT64 offset = (INT64)m_cih.m_frameSize * nFrameNo;
		_fseeki64(fp, offset, SEEK_SET);
		fread(m_pData, m_cih.m_frameSize, 1, fp);
		fclose(fp);
	}

	return TRUE;
}

BOOL CFileTab::ReadDataFileAndDecode(UINT32 nFrameNo)
{
	if (!ReadDataFile(nFrameNo))
		return FALSE;

	DecodeImage();

	return TRUE;
}

void CFileTab::CloseDataFile()
{
	if (m_pData)
		delete[] m_pData;
	m_pData = NULL;

	::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);
}

void CFileTab::DecodeImage()
{
	// ***Lock***
	CBitmapImage* pImage = (CBitmapImage*)m_lockImage.GetLockData();
	PUC_DecodeData(
		pImage->GetBuffer(),
		0,
		0,
		m_cih.m_width,
		m_cih.m_height,
		pImage->GetLineByte(),
		m_pData,
		m_cih.m_quantization);

	PTEXTINFO pTextInfo = (PTEXTINFO)m_lockTextInfo.GetLockData();
	PUC_ExtractSequenceNo(m_pData, m_cih.m_width, m_cih.m_height, &pTextInfo->nSeqNo);
	m_lockTextInfo.Unlock();

	m_lockImage.Unlock();
	// ***Unlock***
}

UINT CFileTab::_SaveImage(INT64 nFrameNo, CWnd* pWnd)
{
	return dynamic_cast<CFileTab*>(pWnd)->SaveImage(nFrameNo);
}

UINT CFileTab::SaveImage(INT64 nFrameNo)
{
	if (!ReadDataFileAndDecode((UINT32)nFrameNo))
		return 0;

	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
	CString filepath;
	filepath.Format(_T("%s\\%s%09lld.bmp"), df.fileSaveFolderPath, df.fileSaveFileName, nFrameNo);

	// ***Lock***
	CBitmapImage* pImage = (CBitmapImage*)m_lockImage.GetLockData();
	BOOL bResult = pImage->Save(filepath);
	m_lockImage.Unlock();
	// ***Unlock***
	if (!bResult)
		return 0;

	return 1;
}

BOOL CFileTab::SaveCSV(const CString& filePath)
{
	FILE* fp = NULL;
	errno_t error;
	BOOL bRet = TRUE;
	PUCRESULT result;
	USHORT nSeqNo;
	INT32 nPreSeqNo = 0;

	error = _tfopen_s(&fp, filePath, _T("w"));
	if (error != 0)
	{
		bRet = FALSE;
		goto EXIT_LABEL;
	}

	// title
	_ftprintf(fp, _T("Frame No,Sequence No,Difference from previous frame,Drop\n"));

	for (INT64 i = 0; i < (INT64)m_cih.m_frameCount - 1; i++)
	{
		if (!ReadDataFile((UINT32)i))
		{
			bRet = FALSE;
			goto EXIT_LABEL;
		}

		// seq no
		result = PUC_ExtractSequenceNo(m_pData, m_cih.m_width, m_cih.m_height, &nSeqNo);
		if (PUC_CHK_FAILED(result))
		{
			bRet = FALSE;
			goto EXIT_LABEL;
		}

		if ((nSeqNo - nPreSeqNo) == 0 || (nSeqNo - nPreSeqNo) == 1)
			_ftprintf(fp, _T("%lld,%d,%d\n"), i, nSeqNo, nSeqNo - nPreSeqNo);
		else
			_ftprintf(fp, _T("%lld,%d,%d,*\n"), i, nSeqNo, nSeqNo - nPreSeqNo);

		nPreSeqNo = nSeqNo;
	}

EXIT_LABEL:
	if (fp)
		fclose(fp);

	return bRet;
}

BEGIN_MESSAGE_MAP(CFileTab, CBaseTab)
	ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
	ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_OPEN, &CFileTab::OnBnClickedOpen)
	ON_BN_CLICKED(IDC_PLAY, &CFileTab::OnBnClickedPlay)
	ON_BN_CLICKED(IDC_FORWARD_FRAME, &CFileTab::OnBnClickedForwardFrame)
	ON_BN_CLICKED(IDC_BACKWARD_FRAME, &CFileTab::OnBnClickedBackwardFrame)
	ON_BN_CLICKED(IDC_GO_TO_START, &CFileTab::OnBnClickedGoToStart)
	ON_BN_CLICKED(IDC_GO_TO_END, &CFileTab::OnBnClickedGoToEnd)
	ON_EN_KILLFOCUS(IDC_CURRENT_FRAME, &CFileTab::OnEnKillfocusCurrentFrame)
	ON_WM_HSCROLL()
	ON_EN_KILLFOCUS(IDC_START_FRAME, &CFileTab::OnEnKillfocusStartFrame)
	ON_EN_KILLFOCUS(IDC_END_FRAME, &CFileTab::OnEnKillfocusEndFrame)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_START_FRAME, &CFileTab::OnDeltaposSpinStartFrame)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_CURRENT_FRAME, &CFileTab::OnDeltaposSpinCurrentFrame)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_END_FRAME, &CFileTab::OnDeltaposSpinEndFrame)
	ON_BN_CLICKED(IDC_POST_START_FRAME, &CFileTab::OnBnClickedPostStartFrame)
	ON_BN_CLICKED(IDC_POST_END_FRAME, &CFileTab::OnBnClickedPostEndFrame)
	ON_BN_CLICKED(IDC_SAVE_FRAME_INFO, &CFileTab::OnBnClickedSaveFrameInfo)
	ON_BN_CLICKED(IDC_SAVE_TO_FILE, &CFileTab::OnBnClickedSaveToFile)
END_MESSAGE_MAP()

LRESULT CFileTab::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = CBaseTab::OnInitDialog(wParam, lParam);
	if (!ret)
		return ret;

	UpdateData(FALSE);

	UpdateControlState();

	return FALSE;
}

void CFileTab::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		UpdateControlState();
		::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);
	}
	else
	{
		StopLive();
	}
}

void CFileTab::OnDestroy()
{
	CloseDataFile();
	CloseCIH();
}

void CFileTab::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_ID_UPDATE_IMG)
	{
		if (m_xvCurrentFrame > m_xvEndFrame)
			m_xvCurrentFrame = m_xvStartFrame;
		else
			m_xvCurrentFrame++;
		UpdateData(FALSE);

		m_sldrFrameCtrl.SetPos(m_xvCurrentFrame);
		ReadDataFileAndDecode(m_xvCurrentFrame);
		::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);
	}

	CBaseTab::OnTimer(nIDEvent);
}

void CFileTab::OnBnClickedOpen()
{
	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();

	CString filter;
	filter.LoadString(IDS_OPEN_FILTER);

	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_NOCHANGEDIR, filter);
	dlg.m_ofn.lpstrInitialDir = df.fileOpenFolderPath;

	if (dlg.DoModal() != IDOK)
		return;

	if (!OpenCIH(dlg.GetPathName()))
	{
		AfxMessageBox(IDS_ERROR_FILEOPEN, MB_OK | MB_ICONERROR);
		return;
	}
	if (!OpenDataFile())
	{
		CloseCIH();
		AfxMessageBox(IDS_ERROR_FILEOPEN, MB_OK | MB_ICONERROR);
		return;
	}
}

void CFileTab::OnBnClickedPlay()
{
	if (!m_bPlaying)
	{
		StartLive();
	}
	else
	{
		StopLive();
	}

	UpdateControlState();
}

void CFileTab::OnBnClickedForwardFrame()
{
	if (m_xvCurrentFrame >= m_xvEndFrame)
		return;
	m_xvCurrentFrame++;

	UpdateData(FALSE);

	m_sldrFrameCtrl.SetPos(m_xvCurrentFrame);
	ReadDataFileAndDecode(m_xvCurrentFrame);

	::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);
}

void CFileTab::OnBnClickedBackwardFrame()
{
	if (m_xvCurrentFrame <= m_xvStartFrame)
		return;
	m_xvCurrentFrame--;

	UpdateData(FALSE);

	m_sldrFrameCtrl.SetPos(m_xvCurrentFrame);
	ReadDataFileAndDecode(m_xvCurrentFrame);

	::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);
}


void CFileTab::OnBnClickedGoToStart()
{
	m_xvCurrentFrame = m_xvStartFrame;
	UpdateData(FALSE);

	m_sldrFrameCtrl.SetPos(m_xvCurrentFrame);
	ReadDataFileAndDecode(m_xvCurrentFrame);

	::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);
}


void CFileTab::OnBnClickedGoToEnd()
{
	m_xvCurrentFrame = m_xvEndFrame;
	UpdateData(FALSE);

	m_sldrFrameCtrl.SetPos(m_xvCurrentFrame);
	ReadDataFileAndDecode(m_xvCurrentFrame);

	::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);
}

void CFileTab::OnEnKillfocusCurrentFrame()
{
	int backup = m_xvCurrentFrame;
	UpdateData(TRUE);

	if (m_xvCurrentFrame < 0 || m_xvCurrentFrame >= m_cih.m_frameCount)
	{
		m_xvCurrentFrame = backup;
		UpdateData(FALSE);
	}

	m_sldrFrameCtrl.SetPos(m_xvCurrentFrame);
	ReadDataFileAndDecode(m_xvCurrentFrame);
	::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);
}

void CFileTab::OnEnKillfocusStartFrame()
{
	int backup = m_xvStartFrame;
	UpdateData(TRUE);

	if (m_xvStartFrame < 0 || m_xvStartFrame > m_xvEndFrame)
	{
		m_xvStartFrame = backup;
		UpdateData(FALSE);
	}
}

void CFileTab::OnEnKillfocusEndFrame()
{
	int backup = m_xvEndFrame;
	UpdateData(TRUE);

	if (m_xvEndFrame >= m_cih.m_frameCount || m_xvEndFrame < m_xvStartFrame)
	{
		m_xvEndFrame = backup;
		UpdateData(FALSE);
	}
}

void CFileTab::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == (CScrollBar*)&m_sldrFrameCtrl)
	{
		BOOL bIsChange = FALSE;

		if (nSBCode == SB_THUMBPOSITION || nSBCode == SB_THUMBTRACK)
		{
			//m_xvCurrentFrame = (int)nPos;
			m_xvCurrentFrame = m_sldrFrameCtrl.GetPos();
			UpdateData(FALSE);
			bIsChange = TRUE;
		}
		else if (nSBCode == SB_PAGELEFT)
		{
			m_xvCurrentFrame -= m_sldrFrameCtrl.GetPageSize();
			if (m_xvCurrentFrame < m_xvStartFrame)
				m_xvCurrentFrame = m_xvStartFrame;
			UpdateData(FALSE);
			bIsChange = TRUE;
		}
		else if (nSBCode == SB_PAGERIGHT)
		{
			m_xvCurrentFrame += m_sldrFrameCtrl.GetPageSize();
			if (m_xvCurrentFrame > m_xvEndFrame)
				m_xvCurrentFrame = m_xvEndFrame;
			UpdateData(FALSE);
			bIsChange = TRUE;
		}

		if (bIsChange)
		{
			ReadDataFileAndDecode(m_xvCurrentFrame);
			::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);
			return;
		}
	}

	CBaseTab::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CFileTab::OnDeltaposSpinStartFrame(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	m_xvStartFrame += pNMUpDown->iDelta;
	if (m_xvStartFrame < 0)
		m_xvStartFrame = 0;
	else if (m_xvStartFrame > m_xvEndFrame)
		m_xvStartFrame = m_xvEndFrame;
	UpdateData(FALSE);
	*pResult = 0;
}

void CFileTab::OnDeltaposSpinCurrentFrame(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	m_xvCurrentFrame += pNMUpDown->iDelta;
	if (m_xvCurrentFrame < 0)
		m_xvCurrentFrame = 0;
	else if (m_xvCurrentFrame >= m_cih.m_frameCount)
		m_xvCurrentFrame = m_cih.m_frameCount - 1;
	UpdateData(FALSE);

	m_sldrFrameCtrl.SetPos(m_xvCurrentFrame);
	ReadDataFileAndDecode(m_xvCurrentFrame);
	::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);

	*pResult = 0;
}

void CFileTab::OnDeltaposSpinEndFrame(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	m_xvEndFrame += pNMUpDown->iDelta;
	if (m_xvEndFrame >= m_cih.m_frameCount)
		m_xvEndFrame = m_cih.m_frameCount - 1;
	else if (m_xvEndFrame < m_xvStartFrame)
		m_xvEndFrame = m_xvStartFrame;
	UpdateData(FALSE);
	*pResult = 0;
}

void CFileTab::OnBnClickedPostStartFrame()
{
	if (m_xvCurrentFrame > m_xvEndFrame)
		return;

	m_xvStartFrame = m_xvCurrentFrame;
	UpdateData(FALSE);
}

void CFileTab::OnBnClickedPostEndFrame()
{
	if (m_xvCurrentFrame < m_xvStartFrame)
		return;

	m_xvEndFrame = m_xvCurrentFrame;
	UpdateData(FALSE);
}

void CFileTab::OnBnClickedSaveFrameInfo()
{
	CString filter;
	filter.LoadString(IDS_SAVE_CSV_FILTER);

	CFileDialog dlg(FALSE, _T("csv"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);
	if (dlg.DoModal() != IDOK)
		return;

	if (SaveCSV(dlg.GetPathName()))
		AfxMessageBox(IDS_COMPLETED, MB_OK | MB_ICONINFORMATION);
	else
		AfxMessageBox(IDS_ERROR_OUTPUT_CSV, MB_OK | MB_ICONERROR);
}

void CFileTab::OnBnClickedSaveToFile()
{
	CSaveFileDialog dlg(this);
	if (dlg.DoModal() != IDOK)
		return;

	CProgressDialog progdlg((INT64)m_xvStartFrame, (INT64)m_xvEndFrame, _SaveImage, this);
	progdlg.DoModal();
}
