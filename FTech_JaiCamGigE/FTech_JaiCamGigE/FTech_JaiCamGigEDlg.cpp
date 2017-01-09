
// FTech_JaiCamGigEDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FTech_JaiCamGigE.h"
#include "FTech_JaiCamGigEDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFTech_JaiCamGigEDlg dialog


UINT DisplayThread(LPVOID param)
{
	CFTech_JaiCamGigEDlg* pMain = (CFTech_JaiCamGigEDlg*)param;

	DWORD dwResult=0;
	while(pMain->m_bThDspWork)
	{
		Sleep(0);

		dwResult = WaitForSingleObject(pMain->m_Camera.GetHandleGrabDone(),1000);
		if (dwResult == WAIT_OBJECT_0)
		{
			pMain->OnDisplay();
			pMain->m_Camera.ResetEventGrabDone();
		}
	}

	return 0;
}

CFTech_JaiCamGigEDlg::CFTech_JaiCamGigEDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFTech_JaiCamGigEDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_bThDspWork= false;
	m_pThDsp	= NULL;
	m_pBmpInfo	= NULL;
	m_nWidth	= 0;
	m_nHeight	= 0;
	m_nBpp		= 0;
}

void CFTech_JaiCamGigEDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFTech_JaiCamGigEDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONNECTION, &CFTech_JaiCamGigEDlg::OnBnClickedBtnConnection)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_ACQ, &CFTech_JaiCamGigEDlg::OnBnClickedBtnAcq)
	ON_BN_CLICKED(IDC_BTN_REFRESH, &CFTech_JaiCamGigEDlg::OnBnClickedBtnRefresh)
	ON_BN_CLICKED(IDC_BTN_AWB, &CFTech_JaiCamGigEDlg::OnBnClickedBtnAwb)
	ON_BN_CLICKED(IDC_BTN_EVENT, &CFTech_JaiCamGigEDlg::OnBnClickedBtnEvent)
	ON_BN_CLICKED(IDC_RBTN_CONT, &CFTech_JaiCamGigEDlg::OnBnClickedRbtnCont)
	ON_BN_CLICKED(IDC_RBTN_SOFT, &CFTech_JaiCamGigEDlg::OnBnClickedRbtnSoft)
	ON_BN_CLICKED(IDC_RBTN_HARD, &CFTech_JaiCamGigEDlg::OnBnClickedRbtnHard)
END_MESSAGE_MAP()


// CFTech_JaiCamGigEDlg message handlers

BOOL CFTech_JaiCamGigEDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	SetDlgItemInt(IDC_EDIT_FPS, 15);

	CheckDlgButton(IDC_RBTN_CONT, 1);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CFTech_JaiCamGigEDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	if (m_pBmpInfo != NULL)
	{
		delete []m_pBmpInfo;
		m_pBmpInfo = NULL;
	}

	if (m_pThDsp != NULL)
	{
		m_bThDspWork = false;

		DWORD dwResult=0;
		dwResult = WaitForSingleObject(m_pThDsp->m_hThread, INFINITE);
		if (dwResult == WAIT_OBJECT_0)
			OutputDebugString(_T("Display thread is terminated.\n"));
	}

	if (m_Camera.IsConnected() == true)
	{
		if (m_Camera.IsActive() == true)
			m_Camera.OnStopAcquisition();

		m_Camera.OnDisconnect();
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFTech_JaiCamGigEDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CFTech_JaiCamGigEDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CFTech_JaiCamGigEDlg::CreateBmpInfo(int nWidth, int nHeight, int nBpp)
{
	if (m_pBmpInfo != NULL) 
	{
		delete []m_pBmpInfo;
		m_pBmpInfo = NULL;
	}

	if (nBpp == 8)
		m_pBmpInfo = (BITMAPINFO *) new BYTE[sizeof(BITMAPINFO) + 255*sizeof(RGBQUAD)];
	else if (nBpp == 24)
		m_pBmpInfo = (BITMAPINFO *) new BYTE[sizeof(BITMAPINFO)];

	m_pBmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_pBmpInfo->bmiHeader.biPlanes = 1;
	m_pBmpInfo->bmiHeader.biBitCount = nBpp;
	m_pBmpInfo->bmiHeader.biCompression = BI_RGB;

	if (nBpp == 8)
		m_pBmpInfo->bmiHeader.biSizeImage = 0;
	else if (nBpp == 24)
		m_pBmpInfo->bmiHeader.biSizeImage = (((nWidth * 24 + 31) & ~31) >> 3) * nHeight;

	m_pBmpInfo->bmiHeader.biXPelsPerMeter = 0;
	m_pBmpInfo->bmiHeader.biYPelsPerMeter = 0;
	m_pBmpInfo->bmiHeader.biClrUsed = 0;
	m_pBmpInfo->bmiHeader.biClrImportant = 0;

	if (nBpp == 8)
	{
		for (int i = 0 ; i < 256 ; i++)
		{
			m_pBmpInfo->bmiColors[i].rgbBlue = (BYTE)i;
			m_pBmpInfo->bmiColors[i].rgbGreen = (BYTE)i;
			m_pBmpInfo->bmiColors[i].rgbRed = (BYTE)i;
			m_pBmpInfo->bmiColors[i].rgbReserved = 0;
		}
	}

	m_pBmpInfo->bmiHeader.biWidth = nWidth;
	m_pBmpInfo->bmiHeader.biHeight = -nHeight;
}

void CFTech_JaiCamGigEDlg::OnDisplay()
{
	CClientDC dc(GetDlgItem(IDC_VIEW_CAMERA));
	CRect rect;
	GetDlgItem(IDC_VIEW_CAMERA)->GetClientRect(&rect);

	dc.SetStretchBltMode(COLORONCOLOR); 

	BYTE *pBuffer = m_Camera.GetImageBuffer();
	StretchDIBits(dc.GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), 0, 0, m_nWidth, m_nHeight, pBuffer, m_pBmpInfo, DIB_RGB_COLORS, SRCCOPY);
}

void CFTech_JaiCamGigEDlg::OnBnClickedBtnConnection()
{
	CString caption=L"";
	GetDlgItemText(IDC_BTN_CONNECTION,caption);

	if (caption == L"Connect")
	{
		CListBox* pLB = (CListBox*)GetDlgItem(IDC_LTB_DEVICE);
		int index = pLB->GetCurSel();

		int state = IsDlgButtonChecked(IDC_CHK_COLOR);

		if (index == -1) { AfxMessageBox(_T("Please select camera on list.")); return; }

		bool color=false;
		state==0 ? color=false : color=true;
		bool ret = m_Camera.OnConnect(index,color);
		if (ret == true)
		{
			m_nWidth  = m_Camera.GetWidth();
			m_nHeight = m_Camera.GetHeight();
			m_nBpp	  = m_Camera.GetBPP();
		
			CreateBmpInfo(m_nWidth, m_nHeight ,m_nBpp);
			GetDlgItem(IDC_BTN_ACQ)->EnableWindow(TRUE);
			
			SetDlgItemText(IDC_BTN_CONNECTION, L"Disconnect");
			GetDlgItem(IDC_CHK_COLOR)->EnableWindow(FALSE);

			CString tmp;
			m_Camera.GetDeviceModelName(tmp);
			SetDlgItemText(IDC_LB_MODELNAME, tmp);
			m_Camera.GetSerialNumber(tmp);
			SetDlgItemText(IDC_LB_SN, tmp);
			tmp = m_Camera.GetIPAddress();
			SetDlgItemText(IDC_LB_IP, tmp);
			tmp = m_Camera.GetMACAddress();
			SetDlgItemText(IDC_LB_MAC, tmp);
		}
	}
	else
	{
		m_bThDspWork = false;

		DWORD dwResult=0;
		if (m_pThDsp != NULL)
		{
			dwResult = WaitForSingleObject(m_pThDsp->m_hThread, INFINITE);
			if (dwResult == WAIT_OBJECT_0)
				OutputDebugString(_T("Display thread is terminated.\n"));
		}

		if (m_Camera.IsActive() == true)
			m_Camera.OnStopAcquisition();

		m_Camera.OnDisconnect();
		
		GetDlgItem(IDC_BTN_ACQ)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHK_COLOR)->EnableWindow(TRUE);
		SetDlgItemText(IDC_BTN_CONNECTION, _T("Connect"));

		SetDlgItemText(IDC_LB_MODELNAME, _T(""));
		SetDlgItemText(IDC_LB_SN, _T(""));
		SetDlgItemText(IDC_LB_IP, _T(""));
		SetDlgItemText(IDC_LB_MAC, _T(""));
	}
}

void CFTech_JaiCamGigEDlg::OnBnClickedBtnAcq()
{
	CString caption=L"";
	GetDlgItemText(IDC_BTN_ACQ,caption);

	if (caption == L"Start")
	{
		bool ret = m_Camera.OnStartAcquisition();
		if (ret == true)
		{
			m_bThDspWork = true;
			m_pThDsp = AfxBeginThread(DisplayThread, this);
			if (m_pThDsp != NULL)
			{
				SetDlgItemText(IDC_BTN_ACQ, L"Stop");
				
			}
		}
	}
	else
	{
		m_bThDspWork = false;

		DWORD dwResult=0;
		dwResult = WaitForSingleObject(m_pThDsp->m_hThread, INFINITE);
		if (dwResult == WAIT_OBJECT_0)
			OutputDebugString(_T("Display thread is terminated.\n"));

		bool ret = m_Camera.OnStopAcquisition();
		if (ret == true)
		{
			SetDlgItemText(IDC_BTN_ACQ, L"Start");
		}
	}
}

void CFTech_JaiCamGigEDlg::OnBnClickedBtnRefresh()
{
	CListBox* pLB = (CListBox*)GetDlgItem(IDC_LTB_DEVICE);
	pLB->ResetContent();

	int nDevices=0;
	bool ret=false;
	ret = JAI_GIGE::CJaiCamGigE::SearchAndGetDeviceCount(nDevices);
	if (ret != true) { AfxMessageBox(JAI_GIGE::CJaiCamGigE::GetLastErrorMessage()); return; }

	for (int i=0; i<nDevices; i++)
	{
		CString Device=_T("");
		CString ModelName=_T("");
		ModelName = JAI_GIGE::CJaiCamGigE::GetDeviceModelName(i);

		CString IP=_T("");
		IP = JAI_GIGE::CJaiCamGigE::GetDeviceIP(i);

		CString DriverType=_T("");
		DriverType = JAI_GIGE::CJaiCamGigE::GetDeviceDriverType(i);

		CString SN=_T("");
		SN = JAI_GIGE::CJaiCamGigE::GetDeviceSN(i);

		Device.Format(_T("%s (%s) - %s , %s"), ModelName,SN,IP,DriverType);
		pLB->AddString(Device);
	}
}


void CFTech_JaiCamGigEDlg::OnBnClickedBtnAwb()
{
	m_Camera.OnCalculateWhiteBalance();
}


void CFTech_JaiCamGigEDlg::OnBnClickedBtnEvent()
{
	m_Camera.OnTriggerEvent();
}


void CFTech_JaiCamGigEDlg::OnBnClickedRbtnCont()
{
	m_Camera.SetContinuousMode();
}


void CFTech_JaiCamGigEDlg::OnBnClickedRbtnSoft()
{
	m_Camera.SetSoftTriggerMode();
}


void CFTech_JaiCamGigEDlg::OnBnClickedRbtnHard()
{
	m_Camera.SetHardTriggerMode();
}
