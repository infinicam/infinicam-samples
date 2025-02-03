#include "stdafx.h"
#include "LiveTab.h"
#include "resource.h"
#include "AppDefine.h"
#include "CamMonitor.h"
#include "cih.h"
#include "ChildView.h"
#include "SelectCameraDialog.h"
#include "SaveLiveDialog.h"
#include "ProgressDialog.h"
#include "Util.h"

#pragma comment(lib, "SetupAPI.lib")

IMPLEMENT_DYNAMIC(CLiveTab, CBaseTab)

CLiveTab::CLiveTab(CWnd* pParent)
	: CBaseTab(CLiveTab::IDD, _T("LIVE"), pParent)
	, m_lockTextInfo(sizeof(TEXTINFO))
	, m_lockRec(sizeof(BOOL))
	, m_nRecFrameCount(0)
	, m_xvAcqMode(ACQUISITION_MODE_SINGLE)
	, m_xvSyncIn(PUC_SYNC_INTERNAL)
	, m_xvSyncInSignal(PUC_SIGNAL_POSI)
	, m_xvSyncOutSignal(PUC_SIGNAL_POSI)
	, m_xvSyncOutDelay(0)
	, m_xvSyncOutWidth(0)
	, m_xvLEDMode(FALSE)
	, m_xvFanState(FALSE)
	, m_xvExposeOn(0)
	, m_xvExposeOff(0)
	, m_csvFile(nullptr)
	, m_mdatFile(nullptr)
	, m_decodeMode(DECODE_CPU)
	, m_enableDecodeGPU(FALSE)
	, m_filepath("")
	, m_folderPath("")
	, m_isMemoryRecord(FALSE)
	, m_langID(LANG_JAPANESE)
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
	DDX_Radio(pDX, IDC_SYNC_IN_SIGNAL_POSI, m_xvSyncInSignal);
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
	DDX_Radio(pDX, IDC_DECODE_CPU, m_decodeMode);
}

BOOL CLiveTab::OpenCamera(UINT32 nDeviceNo, UINT32 nSingleXferTimeOut, UINT32 nContinuousXferTimeOut, UINT32 nRingBufCount)
{
	CString msg;
	PUCRESULT result;
	
	result = m_camera.OpenDevice(nDeviceNo, nSingleXferTimeOut, nContinuousXferTimeOut, nRingBufCount);
	if (PUC_CHK_FAILED(result))
	{
		result = PUC_ResetDevice(nDeviceNo);
		if (PUC_CHK_FAILED(result))
		{
			msg.FormatMessage(IDS_ERROR_CODE, _T("ResetDevice"), result);
			AfxMessageBox(msg, MB_OK | MB_ICONERROR);
			return FALSE;
		}

		// retry
		result = m_camera.OpenDevice(nDeviceNo, nSingleXferTimeOut, nContinuousXferTimeOut, nRingBufCount);
		if (PUC_CHK_FAILED(result))
		{
			msg.FormatMessage(IDS_ERROR_CODE, _T("OpenDevice"), result);
			AfxMessageBox(msg, MB_OK | MB_ICONERROR);
			return FALSE;
		}
	}

	result = PUC_GetAvailableGPUProcess();
	if (PUC_CHK_SUCCEEDED(result))
	{
		PUC_RESO_LIMIT_INFO info;
		PUC_GetResolutionLimit(m_camera.GetHandle(), &info);

		PUC_GPU_SETUP_PARAM param;
		param.width = info.nMaxWidth;
		param.height = info.nMaxHeight;

		result = PUC_SetupGPUDecode(param);
		if (PUC_CHK_SUCCEEDED(result))
		{
			m_enableDecodeGPU = TRUE;
			OnBnClickedDecodeGPU();
		}
	}
	else
	{
		m_enableDecodeGPU = FALSE;
		OnBnClickedDecodeCPU();
	}

	m_camera.SetCallbackSingle(_SingleProc, this);
	m_camera.SetCallbackContinuous(_ContinuousProc, this);

	return TRUE;
}

void CLiveTab::CloseCamera()
{
	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
	df.cameraFramerateIndex = m_comboFramerate.GetCurSel();
	df.cameraResolutionIndex = m_comboReso.GetCurSel();
	df.cameraShutterSpeedIndex = m_comboShutterFps.GetCurSel();

	StopRec();
	StopLive();

	PUC_TeardownGPUDecode();

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

	CTime time = CTime::GetCurrentTime();
	m_saveTimeStamp = time.Format(_T("%Y%m%d%H%M%S"));

	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
	if (df.cameraSaveFileType == SAVE_FILE_TYPE_RAW)
	{
		m_filepath.Format(_T("%s\\%s_%s.mdat"), (LPCTSTR)df.cameraSaveFolderPath, (LPCTSTR)df.cameraSaveFileName, (LPCTSTR)m_saveTimeStamp);

		FILE* fp = NULL;
		errno_t error = _tfopen_s(&fp, m_filepath, _T("wb"));
		if (!fp || error != 0)
		{
			TRACE(_T("fopen error\n"));
		}
		else
		{
			m_mdatFile = fp;
		}

		if (m_isMemoryRecord)
		{
			m_ringBuffer->Clear();
			CProgressCtrl* pProgress = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS_RINGBUFFER);
			pProgress->SetPos(0);
			SetTimer(TIMER_ID_UPDATE_RINGBUFFER, UPDATE_RINGBUFFER_INTERVAL, NULL);
			GetDlgItem(IDC_PROGRESS_RINGBUFFER)->ShowWindow(SW_SHOW);
		}
	}

	if (df.cameraSaveFileType == SAVE_FILE_TYPE_CSV)
	{
		m_filepath.Format(_T("%s\\%s_%s.csv"), (LPCTSTR)df.cameraSaveFolderPath, (LPCTSTR)df.cameraSaveFileName, (LPCTSTR)m_saveTimeStamp);

		FILE* fp = NULL;
		errno_t error = _tfopen_s(&fp, m_filepath, _T("w"));
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
		if (m_isMemoryRecord)
		{
			KillTimer(TIMER_ID_UPDATE_RINGBUFFER);

			FILE* fp = m_mdatFile;
			int saveFrameCount = 0;
			if (fp)
			{
				auto imageCount = m_ringBuffer->GetImageCount();
				CProgressDialog progressDialog(0, imageCount, this);
				progressDialog.Create(IDD_PROGRESS, this);
				progressDialog.ShowWindow(SW_SHOW);
				progressDialog.UpdateWindow();

				for (int i = 0; i < imageCount; i++)
				{
					BOOL isStorageAvailable = CheckDiskSpace(m_folderPath, 1024);
					if (!isStorageAvailable)
						break;

					fwrite(m_ringBuffer->GetImage(), m_ringBuffer->GetImageSize(), 1, fp);
					saveFrameCount++;
					progressDialog.SetPos(i);
					progressDialog.SetText(i, imageCount);
				}
				progressDialog.DestroyWindow();
			}
			
			m_nRecFrameCount = saveFrameCount;
			GetDlgItem(IDC_PROGRESS_RINGBUFFER)->ShowWindow(SW_HIDE);
		}

		*pRec = FALSE;
		m_lockRec.Unlock();
		BOOL isMdat = FALSE;

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
			isMdat = TRUE;
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
	PUC_GetXferDataSize(m_camera.GetHandle(), &cih.m_frameSize);
	cih.m_frameCount = m_nRecFrameCount;
	memcpy(cih.m_quantization, m_camera.GetQuantization(), sizeof(cih.m_quantization));
	cih.m_filetype = df.cameraSaveFileType;
	PUC_GetColorType(m_camera.GetHandle(), &cih.m_colortype);

	CString filepath;
	static INT64 n = 0;
	filepath.Format(_T("%s\\%s_%s.cih"), (LPCTSTR)df.cameraSaveFolderPath, (LPCTSTR)df.cameraSaveFileName, (LPCTSTR)m_saveTimeStamp);

	cih.Write(filepath);
	df.latestSaveCIHFullPath = filepath;
	df.Write();
}

void CLiveTab::DecodeImage(PUCHAR pBuffer)
{
	// ***Lock***
	CBitmapImage* pImage = (CBitmapImage*)m_lockImage.GetLockData();
	pImage->FillBlack();

	if (m_enableDecodeGPU && m_decodeMode == DECODE_GPU)
	{
		unsigned char* tmpDst = pImage->GetBuffer();
		UINT32 w, h;
		PUC_GetResolution(m_camera.GetHandle(), &w, &h);

		PUC_DecodeGPU(
			true,
			pBuffer,
			&tmpDst,
			pImage->GetLineByte()
		);
	}
	else if (m_decodeMode == DECODE_CPU) 
	{
		PUC_DecodeData(
			pImage->GetBuffer() + (m_xvDecodePos.y * pImage->GetLineByte() + m_xvDecodePos.x),
			m_xvDecodePos.x,
			m_xvDecodePos.y,
			m_xvDecodeSize.cx,
			m_xvDecodeSize.cy,
			pImage->GetLineByte(),
			pBuffer,
			m_camera.GetQuantization());
	}

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
	static INT32 nPreSeqNo = 0;

	// Save
	BOOL* pRec = (BOOL*)m_lockRec.GetLockData();
	
	// Do not save the same frame
	if (pInfo->nSequenceNo == nPreSeqNo)
	{
		m_lockRec.Unlock();
		return;
	}
	
	if (*pRec)
	{
		CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
		FILE* fp = NULL;
		errno_t error;

		if (df.cameraSaveFileType == SAVE_FILE_TYPE_RAW)
		{
			if (m_isMemoryRecord)
			{
				m_ringBuffer->AddImage(pInfo->pData);
			}
			else
			{
				fp = m_mdatFile;
				if (fp)
				{
					fwrite(pInfo->pData, pInfo->nDataSize, 1, fp);
					m_nRecFrameCount++;
				}
			}
		}
		else if (df.cameraSaveFileType == SAVE_FILE_TYPE_CSV)
		{
			fp = m_csvFile;

			if (fp)
			{
				if ((pInfo->nSequenceNo - nPreSeqNo) == 1)
					_ftprintf(fp, _T("%lld,%d,%d\n"), m_nRecFrameCount, pInfo->nSequenceNo, pInfo->nSequenceNo - nPreSeqNo);
				else
					_ftprintf(fp, _T("%lld,%d,%d,*\n"), m_nRecFrameCount, pInfo->nSequenceNo, pInfo->nSequenceNo - nPreSeqNo);

				m_nRecFrameCount++;
			}
		}

		nPreSeqNo = pInfo->nSequenceNo;

		BOOL isStorageAvailable = CheckDiskSpace(m_folderPath, 1024);
		if (!isStorageAvailable)
			OnBnClickedStop();
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

	if (m_enableDecodeGPU)
	{
		PUC_TeardownGPUDecode();

		PUC_GPU_SETUP_PARAM param;
		param.width = w;
		param.height = h;
		PUC_SetupGPUDecode(param);
	}
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

	GetDlgItem(IDC_OPENCAMERA)->EnableWindow(!bRecording && !bContinuous);
	GetDlgItem(IDC_RESETCAMERA)->EnableWindow(bOpened && !bRecording && !bContinuous);
	m_buttonRec.EnableWindow(bOpened && bContinuous && !bRecording);
	GetDlgItem(IDC_STOP)->EnableWindow(bRecording);
	GetDlgItem(IDC_ACQUISITION_SINGLE)->EnableWindow(bOpened && !bRecording);
	GetDlgItem(IDC_ACQUISITION_CONTINUOUS)->EnableWindow(bOpened && !bRecording);
	m_comboFramerate.EnableWindow(bOpened && !bContinuous);
	m_comboShutterFps.EnableWindow(bOpened && !bContinuous);
	m_comboReso.EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SYNC_IN_INTERNAL)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SYNC_IN_EXTERNAL)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SYNC_IN_SIGNAL_POSI)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SYNC_IN_SIGNAL_NEGA)->EnableWindow(bOpened && !bContinuous);
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
	GetDlgItem(IDC_RESET_SEQNO)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_DECODE_CPU)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_DECODE_GPU)->EnableWindow(bOpened && !bContinuous && m_enableDecodeGPU);
	GetDlgItem(IDC_RECORD_MODE_MEMORY)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_RECORD_MODE_STORAGE)->EnableWindow(bOpened && !bContinuous);
	GetDlgItem(IDC_SPIN_RECORDTIME)->EnableWindow(bOpened && !bContinuous);

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

	CButton* pAcqSingle = (CButton*)GetDlgItem(IDC_ACQUISITION_SINGLE);
	auto isSingle = pAcqSingle->GetCheck() == BST_CHECKED;
	GetDlgItem(IDC_REC)->ShowWindow(isSingle ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_STOP)->ShowWindow(isSingle ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_SAVE_TO)->ShowWindow(isSingle ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_RESET_SEQNO)->ShowWindow(isSingle ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_SNAPSHOT)->ShowWindow(isSingle ? SW_SHOW : SW_HIDE);

	GetDlgItem(IDC_TEXT_RECORD_TIME)->SetWindowTextW(ConvertToTimeFormat(CalcRecordTimeSecond()));

	Invalidate();
}

void CLiveTab::UpdateControlText()
{
	SetThreadUILanguage(m_langID);

	CString text;
	text.LoadString(IDS_TEXT_ACQUISITION_MODE);
	GetDlgItem(IDC_LABEL_ACQUISITION_MODE)->SetWindowText(text);
	text.LoadString(IDS_TEXT_SINGLE);
	GetDlgItem(IDC_ACQUISITION_SINGLE)->SetWindowText(text);
	text.LoadString(IDS_TEXT_CONTINUOUS);
	GetDlgItem(IDC_ACQUISITION_CONTINUOUS)->SetWindowText(text);

	text.LoadString(IDS_TEXT_RECORD_MODE);
	GetDlgItem(IDC_LABEL_RECORD_MODE)->SetWindowText(text);
	text.LoadString(IDS_TEXT_MEMORY);
	GetDlgItem(IDC_RECORD_MODE_MEMORY)->SetWindowText(text);
	text.LoadString(IDS_TEXT_STORAGE);
	GetDlgItem(IDC_RECORD_MODE_STORAGE)->SetWindowText(text);

	text.LoadString(IDS_TEXT_RECORD_TIME);
	GetDlgItem(IDC_LABEL_RECORD_TIME)->SetWindowText(text);

	text.LoadString(IDS_TEXT_SETUP);
	GetDlgItem(GROUP_SETTING)->SetWindowText(text);

	text.LoadString(IDS_TEXT_FRAMERATE);
	GetDlgItem(IDC_LABEL_FRAMERATE)->SetWindowText(text);
	text.LoadString(IDS_TEXT_SHUTTER_SPEED);
	GetDlgItem(IDC_LABEL_SHUTTER_SPEED)->SetWindowText(text);
	text.LoadString(IDS_TEXT_RESOLUTION);
	GetDlgItem(IDC_LABEL_RESOLUTION)->SetWindowText(text);

	text.LoadString(IDS_TEXT_ADVANCED_SETTING);
	GetDlgItem(IDC_CHECK_ADVANCED_SETTING)->SetWindowText(text);

	text.LoadString(IDS_TEXT_EXPOSE);
	GetDlgItem(IDC_LABEL_EXPOSE)->SetWindowText(text);

	text.LoadString(IDS_TEXT_DELAY);
	GetDlgItem(IDC_LABEL_DELAY)->SetWindowText(text);
	text.LoadString(IDS_TEXT_WIDTH);
	GetDlgItem(IDC_LABEL_WIDTH)->SetWindowText(text);
	text.LoadString(IDS_TEXT_MAGNIFICATION);
	GetDlgItem(IDC_LABEL_MAGNIFICATION)->SetWindowText(text);
	
	text.LoadString(IDS_TEXT_SAVE_TO);
	GetDlgItem(IDC_SAVE_TO)->SetWindowText(text);
	text.LoadString(IDS_TEXT_RESET_SEQNO);
	GetDlgItem(IDC_RESET_SEQNO)->SetWindowText(text);
	text.LoadString(IDS_TEXT_SNAPSHOT);
	GetDlgItem(IDC_SNAPSHOT)->SetWindowText(text);

	text.LoadString(IDS_TEXT_RECORD);
	GetDlgItem(IDC_REC)->SetWindowText(text);
	text.LoadString(IDS_TEXT_STOP);
	GetDlgItem(IDC_STOP)->SetWindowText(text);
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
			s.Format(_T("1/%u"), FramerateShutterTable[i].itemList.items[j]);
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

	PUC_GetSyncOutSignal(m_camera.GetHandle(), (PUC_SIGNAL*)&m_xvSyncOutSignal);
	UpdateData(FALSE);
}

void CLiveTab::UpdateSyncInRadio()
{
	if (!IsCameraOpened())
		return;

	PUC_GetSyncInMode(m_camera.GetHandle(), (PUC_SYNC_MODE*)&m_xvSyncIn, (PUC_SIGNAL*)&m_xvSyncInSignal);
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

void CLiveTab::UpdateLiveUI()
{
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
}

BEGIN_MESSAGE_MAP(CLiveTab, CBaseTab)

	ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
	ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_OPENCAMERA, &CLiveTab::OnBnClickedOpencamera)
	ON_BN_CLICKED(IDC_RESETCAMERA, &CLiveTab::OnBnClickedResetcamera)
	ON_BN_CLICKED(IDC_REC, &CLiveTab::OnBnClickedRec)
	ON_BN_CLICKED(IDC_ACQUISITION_SINGLE, &CLiveTab::OnBnClickedAcquisitionSingle)
	ON_BN_CLICKED(IDC_ACQUISITION_CONTINUOUS, &CLiveTab::OnBnClickedAcquisitionContinuous)
	ON_BN_CLICKED(IDC_STOP, &CLiveTab::OnBnClickedStop)
	ON_CBN_SELCHANGE(IDC_FRAMERATE, &CLiveTab::OnCbnSelchangeFramerate)
	ON_CBN_SELCHANGE(IDC_SHUTTERFPS, &CLiveTab::OnCbnSelchangeShutterFps)
	ON_CBN_SELCHANGE(IDC_RESOLUTION, &CLiveTab::OnCbnSelchangeResolution)
	ON_BN_CLICKED(IDC_SYNC_IN_INTERNAL, &CLiveTab::OnBnClickedSyncIn)
	ON_BN_CLICKED(IDC_SYNC_IN_EXTERNAL, &CLiveTab::OnBnClickedSyncIn)
	ON_BN_CLICKED(IDC_SYNC_IN_SIGNAL_POSI, &CLiveTab::OnBnClickedSyncInSignal)
	ON_BN_CLICKED(IDC_SYNC_IN_SIGNAL_NEGA, &CLiveTab::OnBnClickedSyncInSignal)
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
	ON_BN_CLICKED(IDC_RESET_SEQNO, &CLiveTab::OnBnClickedResetSeqNo)
	ON_BN_CLICKED(IDC_DECODE_CPU, &CLiveTab::OnBnClickedDecodeCPU)
	ON_BN_CLICKED(IDC_DECODE_GPU, &CLiveTab::OnBnClickedDecodeGPU)

	ON_BN_CLICKED(IDC_CHECK_ADVANCED_SETTING, &CLiveTab::OnBnClickedCheckAdvancedSetting)
	ON_BN_CLICKED(IDC_REOCRD_MODE_STORAGE, &CLiveTab::OnBnClickedRecordModeStorage)
	ON_BN_CLICKED(IDC_RECORD_MODE_MEMORY, &CLiveTab::OnBnClickedRecordModeMemory)
	ON_BN_CLICKED(IDC_RECORD_MODE_STORAGE, &CLiveTab::OnBnClickedRecordModeStorage)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_RECORDTIME, &CLiveTab::OnDeltaposSpinRecordtime)
END_MESSAGE_MAP()

std::string WideStringToString(const std::wstring& wideString) {
	int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string result(bufferSize - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, &result[0], bufferSize - 1, nullptr, nullptr);
	return result;
}

LRESULT CLiveTab::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = CBaseTab::OnInitDialog(wParam, lParam);
	if (!ret)
		return ret;

	auto deviceInfo = SetupDiGetClassDevsA(nullptr, "USB", nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);
	BOOL isOpened = FALSE;
	if (deviceInfo != INVALID_HANDLE_VALUE)
	{
		int deviceCount = 0;
		SP_DEVINFO_DATA deviceInfoData;
		deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfo, i, &deviceInfoData); ++i)
		{
			WCHAR buf[256];
			DWORD requiredSize = 0;

			if (SetupDiGetDeviceRegistryProperty(deviceInfo, &deviceInfoData, SPDRP_MFG, nullptr, reinterpret_cast<PBYTE>(buf), sizeof(buf), &requiredSize))
			{
				auto manufacturer = WideStringToString(std::wstring(buf));

				if (manufacturer == "Photron" || manufacturer == "PHOTRON" || manufacturer == "PHOTRON LIMITED")
					deviceCount++;
			}
		}

		SetupDiDestroyDeviceInfoList(deviceInfo);

		if (deviceCount == 1)
		{
			isOpened = OpenCamera(0, 0, 0, 1024);
		}
	}
	
	UpdateBuffer();
	UpdateLiveUI();
	StartLive();

	UpdateControlState();
	UpdateControlText();

	if (isOpened)
	{
		StopLive();

		CString msg;
		CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();

		m_comboFramerate.SetCurSel(df.cameraFramerateIndex);
		UINT32 nFramerate = m_comboFramerate.GetItemData(m_comboFramerate.GetCurSel());
		auto result = PUC_SetFramerateShutter(m_camera.GetHandle(), nFramerate, nFramerate);
		if (PUC_CHK_FAILED(result))
		{
			msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetFramerateShutter"), result);
			AfxMessageBox(msg, MB_OK | MB_ICONERROR);
		}

		UpdateFramerateComboBox();
		UpdateShutterFpsComboBox();
		UpdateResoComboBox();

		m_comboShutterFps.SetCurSel(df.cameraShutterSpeedIndex);
		UINT32 nShutterFps = m_comboShutterFps.GetItemData(m_comboShutterFps.GetCurSel());
		result = PUC_SetFramerateShutter(m_camera.GetHandle(), nFramerate, nShutterFps);
		if (PUC_CHK_FAILED(result))
		{
			msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetFramerateShutter"), result);
			AfxMessageBox(msg, MB_OK | MB_ICONERROR);
		}

		UpdateFramerateComboBox();
		UpdateShutterFpsComboBox();
		UpdateResoComboBox();

		m_comboReso.SetCurSel(df.cameraResolutionIndex);
		UINT32 nResolution = m_comboReso.GetItemData(m_comboReso.GetCurSel());
		result = PUC_SetResolution(m_camera.GetHandle(), RESO_W(nResolution), RESO_H(nResolution));
		if (PUC_CHK_FAILED(result))
		{
			msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetResolution"), result);
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

		OnBnClickedAcquisitionContinuous();
	}
	else
		OnBnClickedAcquisitionSingle();

	OnBnClickedCheckAdvancedSetting();
	OnBnClickedRecordModeStorage();

	CButton* pRecordStorage = (CButton*)GetDlgItem(IDC_RECORD_MODE_STORAGE);
	pRecordStorage->SetCheck(BST_CHECKED);
	GetDlgItem(IDC_PROGRESS_RINGBUFFER)->ShowWindow(SW_HIDE);

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
	else if (nIDEvent == TIMER_ID_UPDATE_RINGBUFFER)
	{
		if (m_isMemoryRecord && m_ringBuffer)
		{
			double fillPercentage = m_ringBuffer->GetFillPercentage();
			CProgressCtrl* pProgress = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS_RINGBUFFER);
			pProgress->SetPos(static_cast<int>(fillPercentage));
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
	UpdateLiveUI();
	StartLive();
}

void CLiveTab::OnBnClickedResetcamera()
{
	UINT32 nSingleXferTimeOut, nContinuousXferTimeout, nRingBufferCount;

	StopRec();
	StopLive();

	PUC_GetXferTimeOut(m_camera.GetHandle(), &nSingleXferTimeOut, &nContinuousXferTimeout);
	PUC_GetRingBufferCount(m_camera.GetHandle(), &nRingBufferCount);

	if (!OpenCamera(m_camera.GetDeviceNo(), nSingleXferTimeOut, nContinuousXferTimeout, nRingBufferCount))
		goto EXIT_LABEL;

	UpdateBuffer();

EXIT_LABEL:
	UpdateLiveUI();
	StartLive();
}

void CLiveTab::OnBnClickedRec()
{
	if (m_folderPath.IsEmpty())
	{
		AfxMessageBox(_T("Please set save file path."), MB_OK | MB_ICONERROR);
		return;
	}

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

	if (m_ringBuffer != nullptr)
	{
		m_ringBuffer.reset();
	}

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnBnClickedAcquisitionContinuous()
{
	StopLive();

	if (!m_folderPath.IsEmpty())
	{
		m_xvAcqMode = ACQUISITION_MODE_CONTINUOUS;
		UpdateData(FALSE);

		m_camera.SetAcquisitionMode((ACQUISITION_MODE)m_xvAcqMode);

		if (m_isMemoryRecord)
		{
			if(m_ringBuffer != nullptr)
				m_ringBuffer.reset();
			m_ringBuffer = CreateRingBuffer(m_camera.getImageDataSize());
		}
	}
	else
	{
		CButton* pAcqSingle = (CButton*)GetDlgItem(IDC_ACQUISITION_SINGLE);
		pAcqSingle->SetCheck(BST_CHECKED);
		CButton* pAcqContinuous = (CButton*)GetDlgItem(IDC_ACQUISITION_CONTINUOUS);
		pAcqContinuous->SetCheck(!BST_CHECKED);
	}

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

	result = PUC_SetSyncInMode(m_camera.GetHandle(), (PUC_SYNC_MODE)m_xvSyncIn, (PUC_SIGNAL)m_xvSyncInSignal);
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

void CLiveTab::OnBnClickedSyncInSignal()
{
	PUCRESULT result;
	CString msg;
	PUC_SIGNAL signal;

	StopLive();

	UpdateData(TRUE);

	int checked = CWnd::GetCheckedRadioButton(IDC_SYNC_IN_SIGNAL_POSI, IDC_SYNC_IN_SIGNAL_NEGA);

	if (checked == IDC_SYNC_IN_SIGNAL_POSI)
	{
		signal = PUC_SIGNAL_POSI;
	}
	else
	{
		signal = PUC_SIGNAL_NEGA;
	}

	result = PUC_SetSyncInMode(m_camera.GetHandle(), (PUC_SYNC_MODE)m_xvSyncIn, signal);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_SetSyncOutSignal"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}

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

	PUCRESULT result;
	CString msg;
	UINT32 nMinExpOnTime, nMinExpOffTime;

	result = PUC_GetMinExposeTime(m_camera.GetHandle(), &nMinExpOnTime, &nMinExpOffTime);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_GetMinExposeTime"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
		return;
	}

	if (m_xvExposeOn < nMinExpOnTime)
		m_xvExposeOn = nMinExpOnTime;

	if ((m_xvExposeOn + m_xvExposeOff) > 1000000000)
		m_xvExposeOn = 1000000000 - m_xvExposeOff;

	SetExposeTime();
}

void CLiveTab::OnEnKillfocusExposeOff()
{
	UpdateData(TRUE);

	PUCRESULT result;
	CString msg;
	UINT32 nMinExpOnTime, nMinExpOffTime;

	result = PUC_GetMinExposeTime(m_camera.GetHandle(), &nMinExpOnTime, &nMinExpOffTime);
	if (PUC_CHK_FAILED(result))
	{
		msg.FormatMessage(IDS_ERROR_CODE, _T("PUC_GetMinExposeTime"), result);
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
		return;
	}

	if (m_xvExposeOff < nMinExpOffTime)
		m_xvExposeOff = nMinExpOffTime;

	if ((m_xvExposeOn + m_xvExposeOff) > 1000000000)
		m_xvExposeOff = 1000000000 - m_xvExposeOn;

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

CString CLiveTab::ConvertToTimeFormat(ULONGLONG totalseconds) {
	ULONGLONG hours = totalseconds / 3600;
	ULONGLONG minutes = (totalseconds % 3600) / 60;
	ULONGLONG seconds = totalseconds % 60;

	CString formattedTime;
	formattedTime.Format(_T("%02llu:%02llu:%02llu"), hours, minutes, seconds);
	return formattedTime;
}

ULONGLONG CLiveTab::CalcRecordTimeSecond()
{
	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
	m_folderPath = (LPCTSTR)df.cameraSaveFolderPath;

	if (m_folderPath.IsEmpty() || !m_camera.IsOpened())
		return 0;

	CButton* pRecordMemory = (CButton*)GetDlgItem(IDC_RECORD_MODE_MEMORY);
	BOOL isMemory = pRecordMemory->GetCheck() == BST_CHECKED;
	ULONGLONG recordTime_seconds = 0;
	auto dataSize = m_camera.getImageDataSize();

	if (isMemory)
	{
		double size = 0;
		if (m_ringBuffer == nullptr)
		{
			// If ringbuffer is not allocated yet
			// Calculate using 80% of RAM as a guide
			size = GetAvailableRAMSpace() * 0.8 / dataSize;
		}
		else
		{
			size = m_ringBuffer->GetBufferSize();
		}

		recordTime_seconds = size / m_camera.getFramerate();
	}
	else
	{
		ULARGE_INTEGER freeBytesAvailabletoCaller;
		if (GetDiskFreeSpaceEx(m_folderPath, &freeBytesAvailabletoCaller, NULL, NULL))
		{
			auto usageSpace = freeBytesAvailabletoCaller.QuadPart * 0.8 - 1024 * 1024 * 1024;
			if (usageSpace < 0)
				usageSpace = 0;

			recordTime_seconds = usageSpace / (dataSize * m_camera.getFramerate());
		}
	}

	if (recordTime_seconds < 1)
	{
		recordTime_seconds = 0;
	}

	CSpinButtonCtrl* pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_RECORDTIME);
	auto currentPos = pSpin->GetPos32();
	pSpin->SetRange32(0, recordTime_seconds);
	pSpin->SetPos32(recordTime_seconds);

	return recordTime_seconds;
}

void CLiveTab::OnBnClickedSaveTo()
{
	CSaveLiveDialog dlg(this);
	if (dlg.DoModal() != IDOK)
		return;

	GetDlgItem(IDC_TEXT_RECORD_TIME)->SetWindowTextW(ConvertToTimeFormat(CalcRecordTimeSecond()));
}

void CLiveTab::OnBnClickedResetSeqNo()
{
	PUCRESULT result;

	result = PUC_ResetSequenceNo(m_camera.GetHandle());
	if (PUC_CHK_FAILED(result))
	{
		AfxMessageBox(IDS_ERROR_NOT_SUPPORTED, MB_OK | MB_ICONERROR);
		return;
	}
}

void CLiveTab::OnBnClickedDecodeCPU() 
{
	StopLive();

	m_decodeMode = DECODE_CPU;
	UpdateData(FALSE);

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnBnClickedDecodeGPU()
{
	StopLive();

	m_decodeMode = DECODE_GPU;
	UpdateData(FALSE);

	StartLive();
	UpdateControlState();
}

void CLiveTab::OnBnClickedCheckAdvancedSetting()
{
	CButton* pAdvancedSetting = (CButton*)GetDlgItem(IDC_CHECK_ADVANCED_SETTING);
	auto isChecked = pAdvancedSetting->GetCheck() == BST_CHECKED;

	GetDlgItem(IDC_EXPOSE_ON)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_EXPOSE_OFF)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_SYNC_IN_INTERNAL)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_SYNC_IN_EXTERNAL)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_SYNC_IN_SIGNAL_POSI)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_SYNC_IN_SIGNAL_NEGA)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_SYNC_OUT_SIGNAL_POSI)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_SYNC_OUT_SIGNAL_NEGA)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_SYNC_OUT_DELAY)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_SYNC_OUT_WIDTH)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_SYNC_OUT_MAGNIFICATION)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_QUANTIZATION)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_DECODE_POS_X)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_DECODE_POS_Y)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_DECODE_POS_W)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_DECODE_POS_H)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_DECODE_CPU)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_DECODE_GPU)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);

	GetDlgItem(IDC_STATIC5)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC6)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC7)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC8)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC9)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC11)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC12)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC13)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC14)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC15)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC16)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC17)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC18)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC19)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC20)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC21)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC22)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC23)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC24)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC25)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_STATIC27)->ShowWindow(isChecked ? SW_SHOW : SW_HIDE);
}

std::unique_ptr<RingBuffer> CLiveTab::CreateRingBuffer(UINT32 imageSize)
{
	CSpinButtonCtrl* pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_RECORDTIME);
	int nPos = pSpin->GetPos32();
	auto bufferSize = nPos * m_camera.getFramerate();

	while (bufferSize > 0)
	{
		try
		{
			return std::make_unique<RingBuffer>(bufferSize, imageSize);
		}
		catch(const std::bad_alloc&)
		{
			bufferSize = bufferSize * 90 / 100;
		}
	}

	throw std::runtime_error("Failed to allocate buffer");
}

double CLiveTab::GetAvailableRAMSpace()
{
	MEMORYSTATUSEX meminfo;
	meminfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&meminfo);
	return meminfo.ullAvailPhys;
}

void CLiveTab::OnBnClickedRecordModeMemory()
{
	m_isMemoryRecord = TRUE;

	m_ringBuffer.reset();
	GetDlgItem(IDC_TEXT_RECORD_TIME)->SetWindowTextW(ConvertToTimeFormat(CalcRecordTimeSecond()));
	GetDlgItem(IDC_SPIN_RECORDTIME)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_SPIN_RECORDTIME)->EnableWindow(TRUE);
}

void CLiveTab::OnBnClickedRecordModeStorage()
{
	m_isMemoryRecord = FALSE;

	m_ringBuffer.reset();
	GetDlgItem(IDC_TEXT_RECORD_TIME)->SetWindowTextW(ConvertToTimeFormat(CalcRecordTimeSecond()));
	GetDlgItem(IDC_SPIN_RECORDTIME)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SPIN_RECORDTIME)->EnableWindow(FALSE);
}

void CLiveTab::OnDeltaposSpinRecordtime(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	
	CSpinButtonCtrl* pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_RECORDTIME);
	int min, max;
	pSpin->GetRange32(min, max);
	int nPos = pSpin->GetPos32();
	int newPos = nPos + pNMUpDown->iDelta;

	if (newPos < min)
	{
		newPos = min;
	}
	else if (newPos > max)
	{
		newPos = max;
	}

	GetDlgItem(IDC_TEXT_RECORD_TIME)->SetWindowTextW(ConvertToTimeFormat(newPos));

	*pResult = 0;
}
