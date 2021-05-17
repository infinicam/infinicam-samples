#include "stdafx.h"
#include "CamMonitor.h"
#include "SelectCameraDialog.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(CSelectCameraDialog, CDialogEx)

CSelectCameraDialog::CSelectCameraDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SELECT_CAMERA, pParent)
	, m_xvSingleXferTimeOut(_T(""))
	, m_xvContinuousXferTimeOut(_T(""))
	, m_xvRingBufCount(_T(""))
	, m_nDeviceNo(0)
	, m_nSingleXferTimeOut(0)
	, m_nContinuousXferTimeOut(0)
	, m_nRingBufCount(0)
{

}

CSelectCameraDialog::~CSelectCameraDialog()
{
}

void CSelectCameraDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_CAMERA, m_comboCamera);
	DDX_Text(pDX, IDC_EDIT_SINGLE_XFER_TIMEOUT, m_xvSingleXferTimeOut);
	DDX_Text(pDX, IDC_EDIT_CONTINUOUS_XFER_TIMEOUT, m_xvContinuousXferTimeOut);
	DDX_Text(pDX, IDC_EDIT_RING_BUF_COUNT, m_xvRingBufCount);
}

void CSelectCameraDialog::UpdateControlState()
{
	GetDlgItem(IDOK)->EnableWindow(m_detectInfo.nDeviceCount != 0);
}

BOOL CSelectCameraDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString s;

	m_xvSingleXferTimeOut.Format(_T("%u"), PUC_XFER_TIMEOUT_AUTO);
	((CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_SINGLE_XFER_TIMEOUT))->SetRange32(0, USHORT_MAX);
	m_xvContinuousXferTimeOut.Format(_T("%u"), PUC_XFER_TIMEOUT_AUTO);
	((CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_CONTINUOUS_XFER_TIMEOUT))->SetRange32(0, USHORT_MAX);
	m_xvRingBufCount.Format(_T("%u"), DEFAULT_RING_BUF_COUNT);
	((CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_RING_BUF_COUNT))->SetRange32(0, USHORT_MAX);
	UpdateData(FALSE);

	// Detect Camera
	PUCRESULT result;

	result = PUC_DetectDevice(&m_detectInfo);
	if (PUC_CHK_FAILED(result))
		return FALSE;
	for (UINT32 i = 0; i < m_detectInfo.nDeviceCount; i++)
	{
		s.Format(_T("CAM %u"), m_detectInfo.nDeviceNoList[i]);
		m_comboCamera.AddString(s);
		m_comboCamera.SetItemData(i, (DWORD_PTR)m_detectInfo.nDeviceNoList[i]);
	}
	m_comboCamera.SetCurSel(0);

	UpdateControlState();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CSelectCameraDialog, CDialogEx)
END_MESSAGE_MAP()

void CSelectCameraDialog::OnOK()
{
	UpdateData();

	m_nDeviceNo = (UINT32)m_comboCamera.GetItemData(m_comboCamera.GetCurSel());
	m_nSingleXferTimeOut = (UINT32)_tstoi(m_xvSingleXferTimeOut);
	m_nContinuousXferTimeOut = (UINT32)_tstoi(m_xvContinuousXferTimeOut);
	m_nRingBufCount = (UINT32)_tstoi(m_xvRingBufCount);

	CDialogEx::OnOK();
}
