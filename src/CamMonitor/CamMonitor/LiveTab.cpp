#include "stdafx.h"
#include "LiveTab.h"
#include "resource.h"
#include "AppDefine.h"
#include "CamMonitor.h"
#include "cih.h"
#include "ChildView.h"
#include "SelectCameraDialog.h"
#include "SaveLiveDialog.h"
#include "Util.h"

IMPLEMENT_DYNAMIC(CLiveTab, CBaseTab)

CLiveTab::CLiveTab(CWnd* pParent)
	: CBaseTab(CLiveTab::IDD, _T("LIVE"), pParent)
	, m_lockTextInfo(sizeof(TEXTINFO))
	, m_lockRec(sizeof(BOOL))
	, m_nRecFrameCount(0)
	, m_xvAcqMode(ACQUISITION_MODE_SINGLE)
	, m_xvSyncIn(PUC_SYNC_INTERNAL)
	, m_xvSyncOutSignal(PUC_SIGNAL_POSI)
	, m_xvSyncOutDelay(0)
	, m_xvSyncOutWidth(0)
	, m_xvLEDMode(FALSE)
	, m_xvFanState(FALSE)
	, m_xvExposeOn(0)
	, m_xvExposeOff(0)
	, m_csvFile(nullptr)
{
}

CLiveTab::~CLiveTab()
{
}

void CLiveTab::DoDataExchange(CDataExchange* pDX)
{
	CBaseTab::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_ACQUISITION_SINGLE, m_xvAcqMode);
	DDX_Control(pDX, IDC_REC, m_buttonRec);
	DDX_Control(pDX, IDC_FRAMERATE, m_comboFramerate);
	DDX_Control(pDX, IDC_SHUTTERFPS, m_comboShutterFps);
	DDX_Control(pDX, IDC_RESOLUTION, m_comboReso);
	DDX_Radio(pDX, IDC_SYNC_IN_INTERNAL, m_xvSyncIn);
	DDX_Radio(pDX, IDC_SYNC_OUT_SIGNAL_POSI, m_xvSyncOutSignal);
	DDX_Text(pDX, IDC_SYNC_OUT_DELAY, m_xvSyncOutDelay);
	DDX_Text(pDX, IDC_SYNC_OUT_WIDTH, m_xvSyncOutWidth);
	DDX_Control(pDX, IDC_SYNC_OUT_MAGNIFICATION, m_comboSyncOutMag);
	DDX_Check(pDX, IDC_LED_MODE, m_xvLEDMode);
	DDX_Check(pDX, IDC_FAN_CTRL, m_xvFanState);
	DDX_Control(pDX, IDC_QUANTIZATION, m_comboQuantization);
	DDX_Text(pDX, IDC_EXPOSE_ON, m_xvExposeOn);
	DDX_Text(pDX, IDC_EXPOSE_OFF, m_xvExposeOff);
	DDX_Text(pDX, IDC_DECODE_POS_X, m_xvDecodePos.x);
	DDX_Text(pDX, IDC_DECODE_POS_Y, m_xvDecodePos.y);
	DDX_Text(pDX, IDC_DECODE_POS_W, m_xvDecodeSize.cx);
	DDX_Text(pDX, IDC_DECODE_POS_H, m_xvDecodeSize.cy);
}

BOOL CLiveTab::OpenCamera(UINT32 nDeviceNo, UINT32 nSingleXferTimeOut, UINT32 nContinuousXferTimeOut, UINT32 nRingBufCount)
{
	CString msg;
	PUCRESULT result;
	
	result = m_camera.OpenDevice(nDeviceNo, nSingleXferTimeOut, nContinuousXferTimeOut, nRingBufCount);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("OpenDevice"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	m_camera.SetCallbackSingle(_SingleProc, this);
	m_camera.SetCallbackContinuous(_ContinuousProc, this);
	return TRUE;
}

void CLiveTab::CloseCamera()
{
	StopRec();
	StopLive();
	m_camera.CloseDevice();
}

void CLiveTab::StartLive()
{
	CString msg;
	PUCRESULT result;
	
	result = m_camera.StartLive();
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("StartLive"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	SetTimer(TIMER_ID_UPDATE_TEMP, UPDATE_TEMP_INTERVAL, NULL);
}

void CLiveTab::StopLive()
{
	CString msg;
	PUCRESULT result;
	
	result = m_camera.StopLive();
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("StopLive"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	KillTimer(TIMER_ID_UPDATE_TEMP);
}

void CLiveTab::StartRec()
{
	UpdateData();

	// ***Lock***
	BOOL* pRec = (BOOL*)m_lockRec.GetLockData();
	*pRec = TRUE;
	m_nRecFrameCount = 0;

	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
	if (df.cameraSaveFileType == SAVE_FILE_TYPE_RAW)
	{
		CString filepath;
		filepath.Format(_T("%s\\%s.mdat"), (LPCTSTR)df.cameraSaveFolderPath, (LPCTSTR)df.cameraSaveFileName);

		FILE* fp = NULL;
		errno_t error = _tfopen_s(&fp, filepath, _T("wbS"));
		if (!fp || error != 0)
		{
			TRACE(_T("fopen error\n"));
		}
		else
		{
			m_mdatFile = fp;
		}
	}
	if (df.cameraSaveFileType == SAVE_FILE_TYPE_CSV)
	{
		CString filepath;
		filepath.Format(_T("%s\\%s.csv"), (LPCTSTR)df.cameraSaveFolderPath, (LPCTSTR)df.cameraSaveFileName);

		FILE* fp = NULL;
		errno_t error = _tfopen_s(&fp, filepath, _T("w"));
		if (!fp || error != 0)
		{
			TRACE(_T("fopen error\n"));
		}
		else
		{
			_ftprintf(fp, _T("Frame No,Sequence No,Difference from previous frame,Drop\n"));
			m_csvFile = fp;
		}
	}

	m_lockRec.Unlock();
	// ***Unlock***

}

void CLiveTab::StopRec()
{
	// ***Lock***
	BOOL* pRec = (BOOL*)m_lockRec.GetLockData();
	if (*pRec)
	{
		*pRec = FALSE;
		m_lockRec.Unlock();

		CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
		if (df.cameraSaveFileType == SAVE_FILE_TYPE_RAW)
		{
			WriteCIH();
		}

		if (m_csvFile) {
			fclose(m_csvFile);
			m_csvFile = nullptr;
		}

		if (m_mdatFile) {
			fclose(m_mdatFile);
			m_mdatFile = nullptr;
		}
	}
	else
	{
		m_lockRec.Unlock();
	}
	// ***Unlock***
}

void CLiveTab::WriteCIH()
{
	CIH cih;
	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();

	PUC_GetFramerateShutter(m_camera.GetHandle(), &cih.m_framerate, &cih.m_shutterFps);
	PUC_GetExposeTime(m_camera.GetHandle(), &cih.m_exposeOn, &cih.m_exposeOff);
	PUC_GetResolution(m_camera.GetHandle(), &cih.m_width, &cih.m_height);
	PUC_GetXferDataSize(m_camera.GetHandle(), PUC_DATA_COMPRESSED, &cih.m_frameSize);
	cih.m_frameCount = m_nRecFrameCount;
	memcpy(cih.m_quantization, m_camera.GetQuantization(), sizeof(cih.m_quantization));
	cih.m_filetype = df.cameraSaveFileType;
	PUC_GetColorType(m_camera.GetHandle(), &cih.m_colortype);

	CString filepath;
	static INT64 n = 0;
	filepath.Format(_T("%s\\%s.cih"), (LPCTSTR)df.cameraSaveFolderPath, (LPCTSTR)df.cameraSaveFileName);

	cih.Write(filepath);
}

void CLiveTab::DecodeImage(PUCHAR pBuffer)
{
	// ***Lock***
	CBitmapImage* pImage = (CBitmapImage*)m_lockImage.GetLockData();
	pImage->FillBlack();
	PUC_DecodeData(
		pImage->GetBuffer() + (m_xvDecodePos.y * pImage->GetLineByte() + m_xvDecodePos.x),
		m_xvDecodePos.x,
		m_xvDecodePos.y,
		m_xvDecodeSize.cx,
		m_xvDecodeSize.cy,
		pImage->GetLineByte(),
		pBuffer,
		m_camera.GetQuantization());

	m_lockImage.Unlock();
	// ***Unlock***
}

void CLiveTab::_SingleProc(PPUC_XFER_DATA_INFO pInfo, void* pArg)
{
	CLiveTab* pObj = (CLiveTab*)pArg;
	pObj->SingleProc(pInfo);
}

void CLiveTab::SingleProc(PPUC_XFER_DATA_INFO pInfo)
{
	// Update Live View
	PTEXTINFO pTextInfo = (PTEXTINFO)m_lockTextInfo.GetLockData();
	pTextInfo->nSeqNo = pInfo->nSequenceNo;
	m_lockTextInfo.Unlock();

	DecodeImage(pInfo->pData);

	::PostMessage(GetSafeHwnd(), WM_USER_UPDATE_VIEW, (WPARAM)0, (LPARAM)0);
}

void CLiveTab::_ContinuousProc(PPUC_XFER_DATA_INFO pInfo, void* pArg)
{
	CLiveTab* pObj = (CLiveTab*)pArg;
	pObj->ContinuousProc(pInfo);
}

void CLiveTab::ContinuousProc(PPUC_XFER_DATA_INFO pInfo)
{
	// Save
	BOOL* pRec = (BOOL*)m_lockRec.GetLockData();
	if (*pRec)
	{
		CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
		FILE* fp = NULL;
		errno_t error;
		static INT32 nPreSeqNo = 0;

		if (df.cameraSaveFileType == SAVE_FILE_TYPE_RAW)
		{
			fp = m_mdatFile;

			if (fp)
			{
				fwrite(pInfo->pData, pInfo->nDataSize, 1, fp);
				m_nRecFrameCount++;
			}
		}
		else if (df.cameraSaveFileType == SAVE_FILE_TYPE_CSV)
		{
			fp = m_csvFile;

			if (fp)
			{
				if ((pInfo->nSequenceNo - nPreSeqNo) == 0 || (pInfo->nSequenceNo - nPreSeqNo) == 1)
					_ftprintf(fp, _T("%lld,%d,%d\n"), m_nRecFrameCount, pInfo->nSequenceNo, pInfo->nSequenceNo - nPreSeqNo);
				else
					_ftprintf(fp, _T("%lld,%d,%d,*\n"), m_nRecFrameCount, pInfo->nSequenceNo, pInfo->nSequenceNo - nPreSeqNo);

				m_nRecFrameCount++;
			}
		}

		nPreSeqNo = pInfo->nSequenceNo;
	}
	m_lockRec.Unlock();
}

void CLiveTab::UpdateBuffer()
{
	m_camera.UpdateBuffer();

	UINT32 w, h;
	PUC_COLOR_TYPE colorType;
	PUC_GetResolution(m_camera.GetHandle(), &w, &h);
	PUC_GetColorType(m_camera.GetHandle(), &colorType);
	m_lockImage.SetWidth(w);
	m_lockImage.SetHeight(h);
	m_lockImage.SetColor(colorType != PUC_COLOR_MONO);
	m_lockImage.Create();
}

void CLiveTab::UpdateControlState()
{
	CString s;
	BOOL bOpened = m_camera.IsOpened();
	BOOL bContinuous = m_xvAcqMode == ACQUISITION_MODE_CONTINUOUS;
	BOOL bExternal = m_xvSyncIn == PUC_SYNC_EXTERNAL;

	// ***Lock***
	BOOL bRecording = *(BOOL*)m_lockRec.GetLockData();
	m_lockRec.Unlock();
	// ***Unlock***

	GetDlgItem(IDC_OPENCAMERA)->EnableWindow(!bRecording);
	m_buttonRec.EnableWindow(bOpened && bContinuous && !bRecording);
	GetDlgItem(IDC_STOP)->EnableWindow(bRecording);
	GetDlgItem(IDC_ACQUISITION_SINGLE)->EnableWindow(bOpened && !bRecording);
	GetDlgItem(IDC_ACQUISITION_CONTINUOUS)->EnableWindow(bOpened && !bRecording);
	m_comboFramerate.EnableWindow(bOpened && !bContinuous);
	m_comboShutterFps.EnableWindow(bOpened && !bContinuous);
	m_comboReso.EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SYNC_IN_INTERNAL)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SYNC_IN_EXTERNAL)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SYNC_OUT_SIGNAL_POSI)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SYNC_OUT_SIGNAL_NEGA)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SYNC_OUT_DELAY)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SYNC_OUT_WIDTH)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SYNC_OUT_MAGNIFICATION)->EnableWindow(bOpened && !bContinuous && !bExternal);
	GetDlgItem(IDC_LED_MODE)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_FAN_CTRL)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_QUANTIZATION)->EnableWindow(FALSE);
	GetDlgItem(IDC_EXPOSE_ON)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_EXPOSE_OFF)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_DECODE_POS_X)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_DECODE_POS_Y)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_DECODE_POS_W)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_DECODE_POS_H)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SNAPSHOT)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SAVE_TO)->EnableWindow(bOpened && !bContinuous);

	// Record button color
	if (bRecording)
	{
		m_buttonRec.SetFaceColor(RGB(242, 121, 8), true);
		m_buttonRec.SetTextColor(RGB(255, 255, 255));
		s.LoadString(IDS_RECORDING);
		m_buttonRec.SetWindowText(s);
	}
	else
	{
		m_buttonRec.SetFaceColor(RGB(94, 124, 146), true);
		m_buttonRec.SetTextColor(RGB(255, 255, 255));
		s.LoadString(IDS_REC);
		m_buttonRec.SetWindowText(s);
	}

	Invalidate();
}

void CLiveTab::UpdateCameraSerialNo()
{
	UINT32 nName = 0;
	UINT32 nType = 0;
	UINT32 nVer = 0;
	UINT64 nSerialNo = 0;
	CString s;

	PUC_GetDeviceName(m_camera.GetHandle(), &nName);
	PUC_GetDeviceType(m_camera.GetHandle(), &nType);
	PUC_GetDeviceVersion(m_camera.GetHandle(), &nVer);
	PUC_GetSerialNo(m_camera.GetHandle(), &nSerialNo);

	s.Format(_T("NAME:%x TYPE:%x Ver:%u Ser:%llx"), nName, nType, nVer, nSerialNo);
	GetDlgItem(IDC_STATIC_SERIAL)->SetWindowText(s);
}

void CLiveTab::UpdateFramerateComboBox()
{
	CString s;
	UINT32 nFramerate, nShutterFps;
	UINT32 nTmp;

	m_comboFramerate.ResetContent();

	if (!IsCameraOpened())
		return;

	PUC_GetFramerateShutter(m_camera.GetHandle(), &nFramerate, &nShutterFps);

	for (int i = 0; i < MAX_FRAMERATE_COUNT; i++)
	{
		nTmp = FramerateShutterTable[i].nFramerate;

		s.Format(_T("%u fps"), nTmp);
		m_comboFramerate.AddString(s);
		m_comboFramerate.SetItemData(m_comboFramerate.GetCount() - 1, nTmp);

		if (nTmp == nFramerate)
			m_comboFramerate.SetCurSel(m_comboFramerate.GetCount() - 1);
	}
}

void CLiveTab::UpdateShutterFpsComboBox()
{
	CString s;
	UINT32 nCurrentFps = 0;
	UINT32 nFramerate, nShutterFps;

	m_comboShutterFps.ResetContent();

	if (!IsCameraOpened())
		return;

	PUC_GetFramerateShutter(m_camera.GetHandle(), &nFramerate, &nShutterFps);

	nCurrentFps = m_comboFramerate.GetItemData(m_comboFramerate.GetCurSel());
	
	for (int i = 0; i < MAX_FRAMERATE_COUNT; i++)
	{
		if (nCurrentFps != FramerateShutterTable[i].nFramerate)
			continue;

		for (int j = 0; j < FramerateShutterTable[i].itemList.nSize; j++)
		{
			s.Format(_T("1/%u fps"), FramerateShutterTable[i].itemList.items[j]);
			m_comboShutterFps.AddString(s);
			m_comboShutterFps.SetItemData(m_comboShutterFps.GetCount() - 1, FramerateShutterTable[i].itemList.items[j]);

			if (FramerateShutterTable[i].itemList.items[j] == nShutterFps)
				m_comboShutterFps.SetCurSel(m_comboShutterFps.GetCount() - 1);
		}
	}
}

void CLiveTab::UpdateResoComboBox()
{
	CString s;
	UINT32 w, h, nCurMaxW, nCurMaxH, n;
	PUC_RESO_LIMIT_INFO info = { 0 };
	
	m_comboReso.ResetContent();

	if (!IsCameraOpened())
		return;

	PUC_GetResolution(m_camera.GetHandle(), &w, &h);
	PUC_GetMaxResolution(m_camera.GetHandle(), &nCurMaxW, &nCurMaxH);
	PUC_GetResolutionLimit(m_camera.GetHandle(), &info);

	n = nCurMaxH;
	while (1)
	{
		s.Format(_T("%u x %u"), nCurMaxW, n);
		m_comboReso.AddString(s);
		m_comboReso.SetItemData(m_comboReso.GetCount() - 1, MAKE_RESO(nCurMaxW, n));

		if (n == h)
			m_comboReso.SetCurSel(m_comboReso.GetCount() - 1);

		n -= info.nUnitHeight;
		if (n < info.nMinHeight)
			break;
	}
}

void CLiveTab::UpdateSyncOutSignalRadio()
{
	if (!IsCameraOpened())
		return;

	PUC_GetSyncInMode(m_camera.GetHandle(), (PUC_SYNC_MODE*)&m_xvSyncIn);
	UpdateData(FALSE);
}

void CLiveTab::UpdateSyncInRadio()
{
	if (!IsCameraOpened())
		return;

	PUC_GetSyncOutSignal(m_camera.GetHandle(), (PUC_SIGNAL*)& m_xvSyncOutSignal);
	UpdateData(FALSE);
}

void CLiveTab::UpdateSyncOutDelayEdit()
{
	if (!IsCameraOpened())
		return;

	PUC_GetSyncOutDelay(m_camera.GetHandle(), (UINT32*)&m_xvSyncOutDelay);
	UpdateData(FALSE);
}

void CLiveTab::UpdateSyncOutWidthEdit()
{
	if (!IsCameraOpened())
		return;

	PUC_GetSyncOutWidth(m_camera.GetHandle(), (UINT32*)&m_xvSyncOutWidth);
	UpdateData(FALSE);
}

void CLiveTab::UpdateSyncOutMagComboBox()
{
	CString s;
	UINT32 nMag;

	m_comboSyncOutMag.ResetContent();

	if (!IsCameraOpened())
		return;

	PUC_GetSyncOutMagnification(m_camera.GetHandle(), &nMag);

	s.Format(_T("x 0.5"));
	m_comboSyncOutMag.AddString(s);
	m_comboSyncOutMag.SetItemData(m_comboSyncOutMag.GetCount() - 1, PUC_SYNC_OUT_MAGNIFICATION_0_5);
	if (nMag == PUC_SYNC_OUT_MAGNIFICATION_0_5)
		m_comboSyncOutMag.SetCurSel(m_comboSyncOutMag.GetCount() - 1);

	s.Format(_T("x 1"));
	m_comboSyncOutMag.AddString(s);
	m_comboSyncOutMag.SetItemData(m_comboSyncOutMag.GetCount() - 1, 1);
	if (nMag == 1)
		m_comboSyncOutMag.SetCurSel(m_comboSyncOutMag.GetCount() - 1);

	s.Format(_T("x 2"));
	m_comboSyncOutMag.AddString(s);
	m_comboSyncOutMag.SetItemData(m_comboSyncOutMag.GetCount() - 1, 2);
	if (nMag == 2)
		m_comboSyncOutMag.SetCurSel(m_comboSyncOutMag.GetCount() - 1);

	s.Format(_T("x 4"));
	m_comboSyncOutMag.AddString(s);
	m_comboSyncOutMag.SetItemData(m_comboSyncOutMag.GetCount() - 1, 4);
	if (nMag == 4)
		m_comboSyncOutMag.SetCurSel(m_comboSyncOutMag.GetCount() - 1);

	s.Format(_T("x 10"));
	m_comboSyncOutMag.AddString(s);
	m_comboSyncOutMag.SetItemData(m_comboSyncOutMag.GetCount() - 1, 10);
	if (nMag == 10)
		m_comboSyncOutMag.SetCurSel(m_comboSyncOutMag.GetCount() - 1);
}

void CLiveTab::UpdateLEDModeCheckBox()
{
	if (!IsCameraOpened())
		return;

	PUC_GetLEDMode(m_camera.GetHandle(), (PUC_MODE*)&m_xvLEDMode);
	UpdateData(FALSE);
}

void CLiveTab::UpdateFANCtrlCheckBox()
{
	if (!IsCameraOpened())
		return;

	PUC_GetFanState(m_camera.GetHandle(), (PUC_MODE*)&m_xvFanState);
	UpdateData(FALSE);
}

void CLiveTab::UpdateQuantization()
{
	CString s;

	int nCurSel = m_comboQuantization.GetCurSel();
	if (nCurSel == CB_ERR)
		nCurSel = 0;

	m_comboQuantization.ResetContent();

	if (!IsCameraOpened())
		return;

	s.LoadString(IDS_QUALITY_NORMAL);
	m_comboQuantization.AddString(s);
	m_comboQuantization.SetItemData(m_comboQuantization.GetCount() - 1, IMAGE_QUALITY_NORMAL);

	s.LoadString(IDS_QUALITY_LOW);
	m_comboQuantization.AddString(s);
	m_comboQuantization.SetItemData(m_comboQuantization.GetCount() - 1, IMAGE_QUALITY_LOW);

	s.LoadString(IDS_QUALITY_HIGH);
	m_comboQuantization.AddString(s);
	m_comboQuantization.SetItemData(m_comboQuantization.GetCount() - 1, IMAGE_QUALITY_HIGH);

	m_comboQuantization.SetCurSel(m_comboQuantization.GetCount() - 1);
}

void CLiveTab::UpdateExposeTime()
{
	if (!IsCameraOpened())
		return;

	PUC_GetExposeTime(m_camera.GetHandle(), (UINT32*)&m_xvExposeOn, (UINT32*)&m_xvExposeOff);
	UpdateData(FALSE);
}

void CLiveTab::ResetDecodePos()
{
	if (!IsCameraOpened())
		return;

	UINT32 w, h;
	PUC_GetResolution(m_camera.GetHandle(), &w, &h);

	m_xvDecodePos.x = m_xvDecodePosBK.x = 0;
	m_xvDecodePos.y = m_xvDecodePosBK.y = 0;
	m_xvDecodeSize.cx = m_xvDecodeSizeBK.cx = w;
	m_xvDecodeSize.cy = m_xvDecodeSizeBK.cy = h;

	UpdateData(FALSE);
}

void CLiveTab::ConfirmDecodePos()
{
	if (!IsCameraOpened())
		return;

	UINT32 w, h;
	PUC_GetResolution(m_camera.GetHandle(), &w, &h);

	BOOL bIllegal = FALSE;

	if (m_xvDecodePos.x != 0 && (m_xvDecodePos.x % 8) != 0)
		bIllegal = TRUE;
	if (m_xvDecodePos.y != 0 && (m_xvDecodePos.y % 8) != 0)
		bIllegal = TRUE;
	if (m_xvDecodeSize.cx == 0)
		bIllegal = TRUE;
	if (m_xvDecodeSize.cy == 0)
		bIllegal = TRUE;
	if ((m_xvDecodePos.x + m_xvDecodeSize.cx) > w)
		bIllegal = TRUE;
	if ((m_xvDecodePos.x + m_xvDecodeSize.cy) > h)
		bIllegal = TRUE;

	if (bIllegal)
	{
		m_xvDecodePos.x = m_xvDecodePosBK.x;
		m_xvDecodePos.y = m_xvDecodePosBK.y;
		m_xvDecodeSize.cx = m_xvDecodeSizeBK.cx;
		m_xvDecodeSize.cy = m_xvDecodeSizeBK.cy;
	}
	else
	{
		m_xvDecodePosBK.x = m_xvDecodePos.x;
		m_xvDecodePosBK.y = m_xvDecodePos.y;
		m_xvDecodeSizeBK.cx = m_xvDecodeSize.cx;
		m_xvDecodeSizeBK.cy = m_xvDecodeSize.cy;
	}

	UpdateData(FALSE);
}

BEGIN_MESSAGE_MAP(CLiveTab, CBaseTab)
	ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
	ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_OPENCAMERA, &CLiveTab::OnBnClickedOpencamera)
	ON_BN_CLICKED(IDC_REC, &CLiveTab::OnBnClickedRec)
	ON_BN_CLICKED(IDC_ACQUISITION_SINGLE, &CLiveTab::OnBnClickedAcquisitionSingle)
	ON_BN_CLICKED(IDC_ACQUISITION_CONTINUOUS, &CLiveTab::OnBnClickedAcquisitionContinuous)
	ON_BN_CLICKED(IDC_STOP, &CLiveTab::OnBnClickedStop)
	ON_CBN_SELCHANGE(IDC_FRAMERATE, &CLiveTab::OnCbnSelchangeFramerate)
	ON_CBN_SELCHANGE(IDC_SHUTTERFPS, &CLiveTab::OnCbnSelchangeShutterFps)
	ON_CBN_SELCHANGE(IDC_RESOLUTION, &CLiveTab::OnCbnSelchangeResolution)
	ON_BN_CLICKED(IDC_SYNC_IN_INTERNAL, &CLiveTab::OnBnClickedSyncIn)
	ON_BN_CLICKED(IDC_SYNC_IN_EXTERNAL, &CLiveTab::OnBnClickedSyncIn)
	ON_BN_CLICKED(IDC_SYNC_OUT_SIGNAL_POSI, &CLiveTab::OnBnClickedSyncOutSignal)
	ON_BN_CLICKED(IDC_SYNC_OUT_SIGNAL_NEGA, &CLiveTab::OnBnClickedSyncOutSignal)
	ON_EN_KILLFOCUS(IDC_SYNC_OUT_DELAY, &CLiveTab::OnEnKillfocusSyncOutDelay)
	ON_EN_KILLFOCUS(IDC_SYNC_OUT_WIDTH, &CLiveTab::OnEnKillfocusSyncOutWidth)
	ON_CBN_SELCHANGE(IDC_SYNC_OUT_MAGNIFICATION, &CLiveTab::OnCbnSelchangeSyncOutMag)
	ON_BN_CLICKED(IDC_LED_MODE, &CLiveTab::OnBnClickedLEDMode)
	ON_BN_CLICKED(IDC_FAN_CTRL, &CLiveTab::OnBnClickedFANCtrl)
	ON_CBN_SELCHANGE(IDC_QUANTIZATION, &CLiveTab::OnCbnSelchangeQuantization)
	ON_EN_KILLFOCUS(IDC_EXPOSE_ON, &CLiveTab::OnEnKillfocusExposeOn)
	ON_EN_KILLFOCUS(IDC_EXPOSE_OFF, &CLiveTab::OnEnKillfocusExposeOff)
	ON_EN_KILLFOCUS(IDC_DECODE_POS_X, &CLiveTab::OnEnKillfocusDecodePos)
	ON_EN_KILLFOCUS(IDC_DECODE_POS_Y, &CLiveTab::OnEnKillfocusDecodePos)
	ON_EN_KILLFOCUS(IDC_DECODE_POS_W, &CLiveTab::OnEnKillfocusDecodePos)
	ON_EN_KILLFOCUS(IDC_DECODE_POS_H, &CLiveTab::OnEnKillfocusDecodePos)
	ON_BN_CLICKED(IDC_SNAPSHOT, &CLiveTab::OnBnClickedSnapshot)
	ON_BN_CLICKED(IDC_SAVE_TO, &CLiveTab::OnBnClickedSaveTo)
END_MESSAGE_MAP()

LRESULT CLiveTab::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = CBaseTab::OnInitDialog(wParam, lParam);
	if (!ret)
		return ret;

	UpdateControlState();

	OnBnClickedAcquisitionSingle();
	return FALSE;
}

void CLiveTab::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		UpdateControlState();
		StartLive();
	}
	else
	{
		StopRec();
		StopLive();
	}
}

void CLiveTab::OnDestroy()
{
	CloseCamera();
}

void CLiveTab::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_ID_UPDATE_TEMP)
	{
		if (m_camera.IsOpened() && 
			m_camera.GetAcquisitionMode() != ACQUISITION_MODE_CONTINUOUS)
		{
			PTEXTINFO pTextInfo = (PTEXTINFO)m_lockTextInfo.GetLockData();
			PUC_GetSensorTemperature(m_camera.GetHandle(), &pTextInfo->nTemp);
			m_lockTextInfo.Unlock();
		}
	}
	CBaseTab::OnTimer(nIDEvent);
}

void CLiveTab::OnBnClickedOpencamera()
{
	CSelectCameraDialog dlg;
	PUCRESULT result;
	CString msg;

	if (dlg.DoModal() != IDOK)
		goto EXIT_LABEL;

	StopRec();
	StopLive();

	if (!OpenCamera(dlg.GetDeviceNo(), dlg.GetSingleXferTimeOut(), dlg.GetContinuousXferTimeOut(), dlg.GetRingBufCount()))
		goto EXIT_LABEL;

	UpdateBuffer();

EXIT_LABEL:
	UpdateCameraSerialNo();
	UpdateFramerateComboBox();
	UpdateShutterFpsComboBox();
	UpdateResoComboBox();
	UpdateSyncInRadio();
	UpdateSyncOutSignalRadio();
	UpdateSyncOutDelayEdit();
	UpdateSyncOutWidthEdit();
	UpdateSyncOutMagComboBox();
	UpdateLEDModeCheckBox();
	UpdateFANCtrlCheckBox();
	UpdateQuantization();
	UpdateExposeTime();
	ResetDecodePos();

	UpdateControlState();
	StartLive();
}

void CLiveTab::OnBnClickedRec()
{
	StartRec();
	UpdateControlState();
}

void CLiveTab::OnBnClickedStop()
{
	StopRec();
	UpdateControlState();
}

void CLiveTab::OnBnClickedAcquisitionSingle()
{
	StopLive();

	m_xvAcqMode = ACQUISITION_MODE_SINGLE;
	UpdateData(FALSE);
	
	m_camera.SetAcquisitionMode((ACQUISITION_MODE)m_xvAcqMode);

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnBnClickedAcquisitionContinuous()
{
	StopLive();

	m_xvAcqMode = ACQUISITION_MODE_CONTINUOUS;
	UpdateData(FALSE);

	m_camera.SetAcquisitionMode((ACQUISITION_MODE)m_xvAcqMode);

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnCbnSelchangeFramerate()
{
	UINT32 nFramerate = m_comboFramerate.GetItemData(m_comboFramerate.GetCurSel());
	PUCRESULT result;
	CString msg;

	StopLive();

	result = PUC_SetFramerateShutter(m_camera.GetHandle(), nFramerate, nFramerate);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetFramerateShutter"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	UpdateFramerateComboBox();
	UpdateShutterFpsComboBox();
	UpdateResoComboBox();
	UpdateExposeTime();
	UpdateSyncOutWidthEdit();
	UpdateSyncOutMagComboBox();

	UpdateBuffer();
	ResetDecodePos();

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnCbnSelchangeShutterFps()
{
	UINT32 nFramerate = m_comboFramerate.GetItemData(m_comboFramerate.GetCurSel());
	UINT32 nShutterFps = m_comboShutterFps.GetItemData(m_comboShutterFps.GetCurSel());
	PUCRESULT result;
	CString msg;

	StopLive();

	result = PUC_SetFramerateShutter(m_camera.GetHandle(), nFramerate, nShutterFps);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetFramerateShutter"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	UpdateFramerateComboBox();
	UpdateShutterFpsComboBox();
	UpdateResoComboBox();
	UpdateExposeTime();
	UpdateSyncOutWidthEdit();
	UpdateSyncOutMagComboBox();

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnCbnSelchangeResolution()
{
	UINT32 nResolution = m_comboReso.GetItemData(m_comboReso.GetCurSel());
	PUCRESULT result;
	CString msg;

	StopLive();

	result = PUC_SetResolution(m_camera.GetHandle(), RESO_W(nResolution), RESO_H(nResolution));
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetResolution"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	UpdateResoComboBox();

	UpdateBuffer();
	ResetDecodePos();

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnBnClickedSyncIn()
{
	PUCRESULT result;
	CString msg;

	StopLive();

	UpdateData(TRUE);

	result = PUC_SetSyncInMode(m_camera.GetHandle(), (PUC_SYNC_MODE)m_xvSyncIn);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetSyncInMode"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	UpdateFramerateComboBox();
	UpdateShutterFpsComboBox();
	UpdateResoComboBox();
	UpdateExposeTime();
	UpdateSyncOutMagComboBox();

	UpdateBuffer();
	ResetDecodePos();

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnBnClickedSyncOutSignal()
{
	PUCRESULT result;
	CString msg;

	StopLive();

	UpdateData(TRUE);

	result = PUC_SetSyncOutSignal(m_camera.GetHandle(), (PUC_SIGNAL)m_xvSyncOutSignal);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetSyncOutSignal"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnEnKillfocusSyncOutDelay()
{
	PUCRESULT result;
	CString msg;

	StopLive();

	UpdateData(TRUE);

	result = PUC_SetSyncOutDelay(m_camera.GetHandle(), m_xvSyncOutDelay);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetSyncOutDelay"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnEnKillfocusSyncOutWidth()
{
	PUCRESULT result;
	CString msg;

	StopLive();

	UpdateData(TRUE);

	result = PUC_SetSyncOutWidth(m_camera.GetHandle(), m_xvSyncOutWidth);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetSyncOutWidth"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	UpdateSyncOutMagComboBox();

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnCbnSelchangeSyncOutMag()
{
	UINT32 nMag = m_comboSyncOutMag.GetItemData(m_comboSyncOutMag.GetCurSel());
	PUCRESULT result;
	CString msg;

	StopLive();

	result = PUC_SetSyncOutMagnification(m_camera.GetHandle(), nMag);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetSyncOutMagnification"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	UpdateSyncOutWidthEdit();

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnBnClickedLEDMode()
{
	PUCRESULT result;
	CString msg;

	StopLive();

	UpdateData(TRUE);

	result = PUC_SetLEDMode(m_camera.GetHandle(), (PUC_MODE)m_xvLEDMode);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetLEDMode"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnBnClickedFANCtrl()
{
	PUCRESULT result;
	CString msg;

	StopLive();

	UpdateData(TRUE);

	result = PUC_SetFanState(m_camera.GetHandle(), (PUC_MODE)m_xvFanState);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetFanCtrl"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnCbnSelchangeQuantization()
{
	QUANTIZATION_MODE q = (QUANTIZATION_MODE)m_comboQuantization.GetItemData(m_comboQuantization.GetCurSel());
	PUCRESULT result;
	CString msg;

	StopLive();

	result = m_camera.SetQuantization(q);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetQuantization"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	StartLive();
	UpdateControlState();
}

void CLiveTab::SetExposeTime()
{
	PUCRESULT result;
	CString msg;

	StopLive();

	result = PUC_SetExposeTime(m_camera.GetHandle(), m_xvExposeOn, m_xvExposeOff);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetExposeTime"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

	UpdateExposeTime();
	UpdateResoComboBox();
	UpdateSyncOutMagComboBox();

	UpdateBuffer();
	ResetDecodePos();

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnEnKillfocusExposeOn()
{
	UpdateData(TRUE);
	SetExposeTime();
}

void CLiveTab::OnEnKillfocusExposeOff()
{
	UpdateData(TRUE);
	SetExposeTime();
}

void CLiveTab::OnEnKillfocusDecodePos()
{
	StopLive();
	UpdateData(TRUE);
	ConfirmDecodePos();
	StartLive();
}

void CLiveTab::OnBnClickedSnapshot()
{
	PUCRESULT result;
	CBitmapImage* pImage = NULL;
	CString filepath, sTmp;
	UINT64 nSerialNo;

	result = PUC_GetSerialNo(m_camera.GetHandle(), &nSerialNo);
	if (PUC_CHK_FAILED(result))
	{
		AfxMessageBox(IDS_ERROR_SAVE, MB_OK | MB_ICONERROR);
		return;
	}
	sTmp.Format(_T("%llx"), nSerialNo);
	sTmp = MakeUpper(sTmp);

	pImage = m_camera.GetSingleImageStart();
	if (!pImage)
	{
		m_camera.GetSingleImageEnd();
		AfxMessageBox(IDS_ERROR_SAVE, MB_OK | MB_ICONERROR);
		return;
	}

	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();

	filepath.Format(_T("%s\\%s.bmp"), (LPCTSTR)df.cameraSaveFolderPath, (LPCTSTR)sTmp);
	if (!pImage->Save(filepath))
	{
		m_camera.GetSingleImageEnd();
		AfxMessageBox(IDS_ERROR_SAVE, MB_OK | MB_ICONERROR);
		return;
	}

	m_camera.GetSingleImageEnd();
	AfxMessageBox(IDS_COMPLETED, MB_OK | MB_ICONINFORMATION);
}

void CLiveTab::OnBnClickedSaveTo()
{
	CSaveLiveDialog dlg(this);
	if (dlg.DoModal() != IDOK)
		return;
}
