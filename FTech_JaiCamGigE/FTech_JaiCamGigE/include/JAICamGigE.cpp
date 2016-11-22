#include "StdAfx.h"
#include "JAICamGigE.h"

StCameraInfo g_stCamInfo;
CString g_strErrorMsg;
CString GetErrorMsg(J_STATUS_TYPE ErrCode);

bool CJAICam::SearchAndGetDeviceCount(int &nValue)
{
	FACTORY_HANDLE	hFactory = NULL;
	uint32_t iNumDev=0;
	bool8_t bHasChange=0;
	J_STATUS_TYPE status = J_ST_SUCCESS;

	status = J_Factory_Open((int8_t*)"", &hFactory);
	if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }
	
	status = J_Factory_UpdateCameraList(hFactory, &bHasChange);
	if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

	status = J_Factory_GetNumOfCameras(hFactory, &iNumDev);
	if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

	nValue = iNumDev;
	//////////////////////////////////////////////////////////////////////////
	g_stCamInfo.Clear();
	
	bool bBreak=false;
	for (int i=0; i<iNumDev; i++)
	{
		uint32_t iSize=0;
		char sCameraId[J_CAMERA_ID_SIZE] = {0,};
		char IP[J_CAMERA_INFO_SIZE] = {0,};
		char MAC[J_CAMERA_INFO_SIZE] = {0,};
		char Model[J_CAMERA_INFO_SIZE] = {0,};
		char SN[J_CAMERA_INFO_SIZE] = {0,};

		iSize = (uint32_t)sizeof(sCameraId);
		status = J_Factory_GetCameraIDByIndex(hFactory, i, sCameraId, &iSize);
		if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); bBreak=true; break; }

		iSize = (uint32_t)sizeof(sCameraId);
		status = J_Factory_GetCameraInfo(hFactory, (int8_t*)sCameraId, CAM_INFO_MODELNAME, Model, &iSize);
		if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); bBreak=true; break; }
		g_stCamInfo.ModelName.Add((CString)Model);

		iSize = (uint32_t)sizeof(sCameraId);
		status = J_Factory_GetCameraInfo(hFactory, (int8_t*)sCameraId, CAM_INFO_SERIALNUMBER, SN, &iSize);
		if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); bBreak=true; break; }
		g_stCamInfo.SN.Add((CString)SN);

		if(0 == strncmp("TL=>GevTL , INT=>FD", sCameraId, 19))
			g_stCamInfo.DriverType.Add(_T("FD"));
		else
			g_stCamInfo.DriverType.Add(_T("SD"));

		iSize = (uint32_t)sizeof(sCameraId);
		status = J_Factory_GetCameraInfo(hFactory, (int8_t*)sCameraId, CAM_INFO_IP, IP, &iSize);
		if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); bBreak=true; break; }
		g_stCamInfo.IP.Add((CString)IP);

		iSize = (uint32_t)sizeof(sCameraId);
		status = J_Factory_GetCameraInfo(hFactory, (int8_t*)sCameraId, CAM_INFO_MAC, MAC, &iSize);
		if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); bBreak=true; break; }
		g_stCamInfo.MAC.Add((CString)MAC);
	}
	//////////////////////////////////////////////////////////////////////////
	if(hFactory != NULL)
	{
		status = J_Factory_Close(hFactory);
		if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

		hFactory = NULL;
	}

	return true;
}

CJAICam::CJAICam(void)
{
	m_hFactory		= NULL;
	m_hCamera		= NULL;
	m_hThread		= NULL;
	m_pbyBuffer		= NULL;
	m_pBitmapInfo	= NULL;
	m_ImgColor.pImageBuffer = NULL;

	m_nWidth	= 0;
	m_nHeight	= 0;
	m_nBpp		= 0;
	m_nGainR	= 4096;
	m_nGainG	= 4096;
	m_nGainB	= 4096;
	
	m_isConnected	= false;
	m_isActived		= false;
	m_isColorConvert= false;
	m_is3CCD		= false;

	g_strErrorMsg	= _T("");
	m_strIP			= _T("");
	m_strMAC		= _T("");
	
	m_hGrabDone = CreateEvent(NULL,TRUE,FALSE,NULL);
	ResetEvent(m_hGrabDone);
	OpenFactory();
}


CJAICam::~CJAICam(void)
{
	CloseFactory();

	if (m_ImgColor.pImageBuffer != NULL)
	{
		J_Image_Free(&m_ImgColor);
		m_ImgColor.pImageBuffer = NULL;
	}

	if (m_pBitmapInfo != NULL)
	{
		delete m_pBitmapInfo;
		m_pBitmapInfo = NULL;
	}

	if (m_pbyBuffer != NULL)
	{
		delete []m_pbyBuffer;
		m_pbyBuffer = NULL;
	}
}

bool CJAICam::OpenFactory()
{
	J_STATUS_TYPE status = J_ST_SUCCESS;

	status = J_Factory_Open((int8_t*)"", &m_hFactory);
	if(status != J_ST_SUCCESS)
	{
		g_strErrorMsg = GetErrorMsg(status);
		return false;
	}

	return true;
}

bool CJAICam::CloseFactory()
{
	J_STATUS_TYPE status = J_ST_SUCCESS;

	if(m_hFactory != NULL)
	{
		status = J_Factory_Close(m_hFactory);
		if(status != J_ST_SUCCESS)
		{
			g_strErrorMsg = GetErrorMsg(status);
			return false;
		}

		m_hFactory = NULL;
	}

	return true;
}

bool CJAICam::OnConnect(int idx, bool bColorConvert)
{
	J_STATUS_TYPE	status = J_ST_SUCCESS;
	uint32_t		iCameraIdSize = J_CAMERA_ID_SIZE;
	int64_t			int64Val=0;
	char			sCameraId[J_CAMERA_ID_SIZE];    // Camera ID
	bool8_t			bHasChanged = false;
	NODE_HANDLE hNode;

	status = J_Factory_GetCameraIDByIndex(m_hFactory, idx, (int8_t*)sCameraId, &iCameraIdSize);
	if(status != J_ST_SUCCESS)
	{
		g_strErrorMsg = GetErrorMsg(status);
		return false;
	}

	char IP[J_CAMERA_INFO_SIZE] = {0,};
	char MAC[J_CAMERA_INFO_SIZE] = {0,};
	char MODEL[J_CAMERA_INFO_SIZE] = {0,};
	uint32_t nCameraInfoSize = J_CAMERA_INFO_SIZE;

	status = J_Factory_GetCameraInfo(m_hFactory, (int8_t*)sCameraId, CAM_INFO_IP, IP, &nCameraInfoSize);
	if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }
	m_strIP = (CString)IP;

	nCameraInfoSize = J_CAMERA_INFO_SIZE;
	status = J_Factory_GetCameraInfo(m_hFactory, (int8_t*)sCameraId, CAM_INFO_MAC, MAC, &nCameraInfoSize);
	if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }
	m_strMAC = (CString)MAC;

	nCameraInfoSize = J_CAMERA_INFO_SIZE;
	status = J_Factory_GetCameraInfo(m_hFactory, (int8_t*)sCameraId, CAM_INFO_MODELNAME, MODEL, &nCameraInfoSize);
	if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }
	CString model = _T("");
	model = (CString)MODEL; 

	if (model.Find(_T("AT")) != -1) m_is3CCD = true;

	//open the selected camera
	status = J_Camera_Open(m_hFactory, (int8_t*)sCameraId, &m_hCamera);
	if(status != J_ST_SUCCESS)
	{
		g_strErrorMsg = GetErrorMsg(status);
		return false;
	}

	status = J_Camera_GetValueInt64(m_hCamera, (int8_t*)"Width", &int64Val);
	if(status != J_ST_SUCCESS)
	{
		g_strErrorMsg = GetErrorMsg(status);
		return false;
	}
	m_nWidth = (int)int64Val;

	status = J_Camera_GetValueInt64(m_hCamera, (int8_t*)"Height", &int64Val);
	if(status != J_ST_SUCCESS)
	{
		g_strErrorMsg = GetErrorMsg(status);
		return false;
	}
	m_nHeight = (int)int64Val;
	
	status = J_Camera_GetValueInt64(m_hCamera, (int8_t*)"PixelFormat", &int64Val);
	if(status != J_ST_SUCCESS)
	{
		g_strErrorMsg = GetErrorMsg(status);
		return false;
	}
	m_nBpp = J_BitsPerPixel(int64Val);

	m_isColorConvert = bColorConvert;
	m_isColorConvert ? m_nBpp *= 3 : m_nBpp;

	m_pbyBuffer = new BYTE[m_nWidth * m_nHeight * m_nBpp / 8];
	memset(m_pbyBuffer, 0, m_nWidth * m_nHeight * m_nBpp / 8);

	OnCreateBmpInfo(m_nWidth, m_nHeight, m_nBpp);

	m_isConnected = true;
	return TRUE;
}

bool CJAICam::OnDisconnect()
{
	J_STATUS_TYPE status = J_ST_SUCCESS;

	if(m_hCamera != NULL)
	{
		if (m_isActived == true)
		{
			OnStopAcquisition();
		}

		status = J_Camera_Close(m_hCamera);
		if(status != J_ST_SUCCESS)
		{
			g_strErrorMsg = GetErrorMsg(status);
			return false;
		}

		m_hCamera = NULL;
	}

	if (m_pBitmapInfo != NULL)
	{
		delete []m_pBitmapInfo;
		m_pBitmapInfo = NULL;
	}

	if (m_pbyBuffer != NULL)
	{
		delete []m_pbyBuffer;
		m_pbyBuffer = NULL;
	}

	m_isConnected = false;
	m_nWidth	= 0;
	m_nHeight	= 0;
	m_nBpp		= 0;

	return TRUE;
}

bool CJAICam::OnStartAcquisition()
{
	J_STATUS_TYPE status = J_ST_SUCCESS;

	status = J_Image_OpenStream(m_hCamera, 0, reinterpret_cast<J_IMG_CALLBACK_OBJECT>(this), reinterpret_cast<J_IMG_CALLBACK_FUNCTION>(&CJAICam::StreamCBFunc), &m_hThread, (m_nWidth*m_nHeight*m_nBpp)/8);
	if(status != J_ST_SUCCESS)
	{
		g_strErrorMsg = GetErrorMsg(status);
		return false;
	}

	status = J_Camera_ExecuteCommand(m_hCamera, (int8_t*)"AcquisitionStart");
	if(status != J_ST_SUCCESS)
	{
		g_strErrorMsg = GetErrorMsg(status);
		return false;
	}

	m_isActived = true;

	return true;
}

void CJAICam::StreamCBFunc(J_tIMAGE_INFO * pAqImageInfo)
{
	if (m_isColorConvert == true || m_is3CCD == true) 
	{
		if(m_ImgColor.pImageBuffer != NULL)
		{
			if((m_ImgColor.iSizeX != pAqImageInfo->iSizeX) || (m_ImgColor.iSizeY != pAqImageInfo->iSizeY))
			{
				J_Image_Free(&m_ImgColor);
				m_ImgColor.pImageBuffer = NULL;
			}
		}

		if(m_ImgColor.pImageBuffer == NULL)
			J_Image_Malloc(pAqImageInfo, &m_ImgColor);

		J_Image_FromRawToImageEx(pAqImageInfo, &m_ImgColor, BAYER_STANDARD_MULTI,m_nGainR,m_nGainG,m_nGainB); //BAYER_STANDARD

		memcpy(m_pbyBuffer, m_ImgColor.pImageBuffer, m_ImgColor.iImageSize);
	}
	else
	{
		memcpy(m_pbyBuffer, pAqImageInfo->pImageBuffer, pAqImageInfo->iImageSize);
	}

	SetEvent(m_hGrabDone);
}

bool CJAICam::OnStopAcquisition()
{
	J_STATUS_TYPE status = J_ST_SUCCESS;

	// Stop Acquisition
	if (m_hCamera != NULL) 
	{
		status = J_Camera_ExecuteCommand(m_hCamera, (int8_t*)"AcquisitionStop");
		if(status != J_ST_SUCCESS)
		{
			g_strErrorMsg = GetErrorMsg(status);
			return false;
		}
	}

	if(m_hThread != NULL)
	{
		// Close stream
		status = J_Image_CloseStream(m_hThread);
		if(status != J_ST_SUCCESS)
		{
			g_strErrorMsg = GetErrorMsg(status);
			return false;
		}

		m_hThread = NULL;
	}

	m_isActived = false;

	return true;
}


bool CJAICam::SetHardTriggerMode()
{
	J_STATUS_TYPE status = J_ST_SUCCESS;
	
	if(m_hCamera != NULL)
	{
		NODE_HANDLE hNode;
		uint32_t iNumOfEnumEntries = 0;
		int64_t int64Val = 0;

		hNode = NULL;
		status = J_Camera_GetNodeByName(m_hCamera, (int8_t*)"TriggerSelector", &hNode);
		if ((status == J_ST_SUCCESS) && (hNode != NULL))
		{
			status = J_Camera_SetValueString(m_hCamera, (int8_t*)"TriggerSelector", (int8_t*)"FrameStart");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

			status = J_Camera_SetValueString(m_hCamera, (int8_t*)"TriggerMode", (int8_t*)"On");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

			status = J_Camera_SetValueString(m_hCamera,(int8_t*)"TriggerSource", (int8_t*)"Line5");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

			status = J_Camera_SetValueString(m_hCamera,(int8_t*)"SequenceMode", (int8_t*)"Off");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }
		}
		else
		{
			g_strErrorMsg = GetErrorMsg(status);
			return false;
		}
	}
	return true;
}

bool CJAICam::SetSoftTriggerMode()
{
	J_STATUS_TYPE status = J_ST_SUCCESS;

	if(m_hCamera != NULL)
	{
		NODE_HANDLE hNode;
		uint32_t iNumOfEnumEntries = 0;
		int64_t int64Val = 0;

		hNode = NULL;
		status = J_Camera_GetNodeByName(m_hCamera, (int8_t*)"TriggerSelector", &hNode);
		if ((status == J_ST_SUCCESS) && (hNode != NULL))
		{
			status = J_Camera_SetValueString(m_hCamera, (int8_t*)"TriggerSelector", (int8_t*)"FrameStart");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

			status = J_Camera_SetValueString(m_hCamera, (int8_t*)"TriggerMode", (int8_t*)"On");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

			status = J_Camera_SetValueString(m_hCamera,(int8_t*)"TriggerSource", (int8_t*)"Software");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

			status = J_Camera_SetValueString(m_hCamera,(int8_t*)"SequenceMode", (int8_t*)"Off");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }
		}
		else
		{
			status = J_Camera_SetValueString(m_hCamera,(int8_t*)"ExposureMode", (int8_t*)"EdgePreSelect");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

			status = J_Camera_SetValueString(m_hCamera,(int8_t*)"LineSelector", (int8_t*)"CameraTrigger0");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

			status = J_Camera_SetValueString(m_hCamera,(int8_t*)"LineSource", (int8_t*)"SoftwareTrigger0");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }
		}
	} 

	return true;
}

bool CJAICam::SetContinuousMode()
{
	J_STATUS_TYPE status = J_ST_SUCCESS;

	if(m_hCamera != NULL)
	{
		NODE_HANDLE hNode = NULL;
		status = J_Camera_GetNodeByName(m_hCamera, (int8_t*)"TriggerSelector", &hNode);

		if ((status == J_ST_SUCCESS) && (hNode != NULL))
		{
			status = J_Camera_SetValueString(m_hCamera, (int8_t*)"TriggerSelector", (int8_t*)"FrameStart");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

			status = J_Camera_SetValueString(m_hCamera, (int8_t*)"TriggerMode", (int8_t*)"Off");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }
		}
		else
		{
			status = J_Camera_SetValueString(m_hCamera,(int8_t*)"ExposureMode", (int8_t*)"Continuous");
			if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }
		}
	} 

	return true;
}

bool CJAICam::OnTriggerEvent()
{
	J_STATUS_TYPE status = J_ST_SUCCESS;

	NODE_HANDLE hNode = NULL;
	status = J_Camera_GetNodeByName(m_hCamera, (int8_t*)"TriggerSoftware", &hNode);
	if ((status == J_ST_SUCCESS) && (hNode != NULL))
	{
		status = J_Camera_SetValueString(m_hCamera, (int8_t*)"TriggerSelector", (int8_t*)"FrameStart");
		if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

		status = J_Camera_ExecuteCommand(m_hCamera, (int8_t*)"TriggerSoftware");
		if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }
	}
	else
	{
		status = J_Camera_SetValueInt64(m_hCamera, (int8_t*)"SoftwareTrigger0", 1);
		if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }

		status = J_Camera_SetValueInt64(m_hCamera, (int8_t*)"SoftwareTrigger0", 0);
		if(status != J_ST_SUCCESS) { g_strErrorMsg = GetErrorMsg(status); return false; }
	}

	return true;
}

bool CJAICam::OnCalculateWhiteBalance()
{
	J_STATUS_TYPE	status = J_ST_SUCCESS;
	J_tBGR48		resultcolor;
	RECT			pMeasureRect;
	int64_t			int64Val;
	int				iWidth, iHeight;

	status = J_Camera_GetValueInt64(m_hCamera, (int8_t*)"Width", &int64Val);
	if (status != J_ST_SUCCESS) 
	{ 
		g_strErrorMsg = GetErrorMsg(status); 
		return false; 
	}
	iWidth = (int)int64Val;

	status = J_Camera_GetValueInt64(m_hCamera, (int8_t*)"Height", &int64Val);
	if (status != J_ST_SUCCESS) 
	{ 
		g_strErrorMsg = GetErrorMsg(status); 
		return false; 
	}
	iHeight = (int)int64Val;

	pMeasureRect.top = (long)abs(2* iWidth / 5);
	pMeasureRect.left = (long)abs(2* iHeight / 5);
	pMeasureRect.right = (long)abs(3 * iWidth / 5);
	pMeasureRect.bottom = (long)abs(3 * iHeight / 5);

	status = J_Image_GetAverage(&m_ImgColor, &pMeasureRect, &resultcolor);
	if (status != J_ST_SUCCESS) 
	{ 
		g_strErrorMsg = GetErrorMsg(status); 
		return false; 
	}

	m_nGainR = 4096 * resultcolor.G16 / resultcolor.R16;
	m_nGainB = 4096 * resultcolor.G16 / resultcolor.B16;
	m_nGainG = 4096;

	return true;
}

bool CJAICam::OnSaveImage(CString strPath)
{
	if (strPath.IsEmpty()) return false;

	if (m_pbyBuffer == NULL) return false;

	BITMAPFILEHEADER dib_format_layout;
	ZeroMemory(&dib_format_layout, sizeof(BITMAPFILEHEADER));
	dib_format_layout.bfType = 0x4D42;//*(WORD*)"BM";
	if (m_nBpp == 8)
		dib_format_layout.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(m_pBitmapInfo->bmiColors) * 256;
	else
		dib_format_layout.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	
	dib_format_layout.bfSize = dib_format_layout.bfOffBits + m_pBitmapInfo->bmiHeader.biSizeImage;

	wchar_t* wchar_str;     
	char*    char_str;      
	int      char_str_len;  
	wchar_str = strPath.GetBuffer(strPath.GetLength());

	char_str_len = WideCharToMultiByte(CP_ACP, 0, wchar_str, -1, NULL, 0, NULL, NULL);
	char_str = new char[char_str_len];
	WideCharToMultiByte(CP_ACP, 0, wchar_str, -1, char_str, char_str_len, 0,0);  

	FILE *p_file;
	fopen_s(&p_file,char_str, "wb");
	if(p_file != NULL)
	{
		fwrite(&dib_format_layout, 1, sizeof(BITMAPFILEHEADER), p_file);
		fwrite(m_pBitmapInfo, 1, sizeof(BITMAPINFOHEADER), p_file);
		if (m_nBpp == 8)
			fwrite(m_pBitmapInfo->bmiColors, sizeof(m_pBitmapInfo->bmiColors), 256, p_file);

		fwrite(m_pbyBuffer, 1, m_nWidth * m_nHeight * m_nBpp/8, p_file);
		fclose(p_file);
	}

	delete char_str;

	return true;
}

CString GetErrorMsg(J_STATUS_TYPE ErrCode)
{
	CString strErrMsg=_T("");
	switch(ErrCode)
	{
	case J_ST_SUCCESS             :	strErrMsg = _T("OK."								); break; 
	case J_ST_INVALID_BUFFER_SIZE :	strErrMsg = _T("Invalid buffer size."				); break; 
	case J_ST_INVALID_HANDLE      :	strErrMsg = _T("Invalid handle."					); break; 
	case J_ST_INVALID_ID          :	strErrMsg = _T("Invalid ID."						); break; 
	case J_ST_ACCESS_DENIED       :	strErrMsg = _T("Access denied."						); break; 
	case J_ST_NO_DATA             :	strErrMsg = _T("No data."							); break; 
	case J_ST_ERROR               :	strErrMsg = _T("Generic error code."				); break; 
	case J_ST_INVALID_PARAMETER   :	strErrMsg = _T("Invalid parameter."					); break; 
	case J_ST_TIMEOUT             :	strErrMsg = _T("Timeout."							); break; 
	case J_ST_INVALID_FILENAME    :	strErrMsg = _T("Invalid file name."					); break; 
	case J_ST_INVALID_ADDRESS     :	strErrMsg = _T("Invalid address."					); break; 
	case J_ST_FILE_IO             :	strErrMsg = _T("File IO error."						); break; 
	case J_ST_GC_ERROR            :	strErrMsg = _T("GenICam error."						); break; 
	case J_ST_VALIDATION_ERROR    :	strErrMsg = _T("Settings File Validation Error."	); break; 
	case J_ST_VALIDATION_WARNING  :	strErrMsg = _T("Settings File Validation Warning."	); break; 
	}

	return strErrMsg;
}

void CJAICam::OnCreateBmpInfo(int nWidth, int nHeight, int nBpp)
{
	if (m_pBitmapInfo != NULL)
	{
		delete []m_pBitmapInfo;
		m_pBitmapInfo = NULL;
	}

	if (nBpp == 8)
		m_pBitmapInfo = (BITMAPINFO *) new BYTE[sizeof(BITMAPINFO) + 255*sizeof(RGBQUAD)];
	else if (nBpp == 24)
		m_pBitmapInfo = (BITMAPINFO *) new BYTE[sizeof(BITMAPINFO)];

	m_pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_pBitmapInfo->bmiHeader.biPlanes = 1;
	m_pBitmapInfo->bmiHeader.biBitCount = nBpp;
	m_pBitmapInfo->bmiHeader.biCompression = BI_RGB;

	if (nBpp == 8)
		m_pBitmapInfo->bmiHeader.biSizeImage = 0;
	else if (nBpp == 24)
		m_pBitmapInfo->bmiHeader.biSizeImage = (((nWidth * 24 + 31) & ~31) >> 3) * nHeight;

	m_pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
	m_pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	m_pBitmapInfo->bmiHeader.biClrUsed = 0;
	m_pBitmapInfo->bmiHeader.biClrImportant = 0;

	if (nBpp == 8)
	{
		for (int i = 0 ; i < 256 ; i++)
		{
			m_pBitmapInfo->bmiColors[i].rgbBlue = (BYTE)i;
			m_pBitmapInfo->bmiColors[i].rgbGreen = (BYTE)i;
			m_pBitmapInfo->bmiColors[i].rgbRed = (BYTE)i;
			m_pBitmapInfo->bmiColors[i].rgbReserved = 0;
		}
	}

	m_pBitmapInfo->bmiHeader.biWidth = nWidth;
	m_pBitmapInfo->bmiHeader.biHeight = -nHeight;
}

bool CJAICam::SetUserSetSelector(USER User)
{
	if(m_hCamera == NULL)
	{
		g_strErrorMsg = _T("Device is not connected.");
		return false;
	}

	char value[MAX_PATH] = {0,};
	
	switch (User)
	{
	case USER_Default	: sprintf_s(value,"%s","Default"); break;
	case USER_UserSet1	: sprintf_s(value,"%s","UserSet1"); break;
	case USER_UserSet2	: sprintf_s(value,"%s","UserSet2"); break;
	case USER_UserSet3	: sprintf_s(value,"%s","UserSet3"); break;
	}

	return SetValueString((int8_t*)"UserSetSelector", (CString)value);
}

bool CJAICam::OnUserSetSave()
{
	return OnExecuteCommand((int8_t*)"UserSetSave");
}

bool CJAICam::OnUserSetLoad()
{
	return OnExecuteCommand((int8_t*)"UserSetLoad");
}

bool CJAICam::GetDeviceUserID(CString &strValue)
{
	return GetValueString((int8_t*)"DeviceUserID", strValue);
}

bool CJAICam::SetDeviceUserID(CString strValue)
{
	return SetValueString((int8_t*)"DeviceUserID", strValue);
}

bool CJAICam::GetDeviceModelName(CString &strValue)
{
	return GetValueString((int8_t*)"DeviceModelName", strValue);
}

bool CJAICam::GetSerialNumber(CString &strValue)
{
	return GetValueString((int8_t*)"DeviceID", strValue);
}

bool CJAICam::GetOffsetX(int &nValue)
{
	return GetValueInt((int8_t*)"OffsetX", nValue);
}

bool CJAICam::GetOffsetY(int &nValue)
{
	return GetValueInt((int8_t*)"OffsetY", nValue);
}

bool CJAICam::GetAcquisitionMode(CString &strValue)
{
	return GetValueString((int8_t*)"AcquisitionMode", strValue);
}

bool CJAICam::GetAcquisitionFrameRate(double &dValue)
{
	return GetValueDouble((int8_t*)"AcquisitionFrameRate", dValue);
}

bool CJAICam::GetTriggerMode(CString &strValue)
{
	return GetValueString((int8_t*)"TriggerMode", strValue);
}

bool CJAICam::GetTriggerSource(CString &strValue)
{
	return GetValueString((int8_t*)"TriggerSource", strValue);
}

bool CJAICam::GetExposureMode(CString &strValue)
{
	return GetValueString((int8_t*)"ExposureMode", strValue);
}

bool CJAICam::GetExposureTimeRaw(int &nValue)
{
	return GetValueInt((int8_t*)"ExposureTimeRaw", nValue);
}

bool CJAICam::GetPixelFormat(CString &strValue)
{
	return GetValueString((int8_t*)"PixelFormat", strValue);
}

bool CJAICam::SetOffsetX(int nValue)
{
	return SetValueInt((int8_t*)"OffsetX", nValue);
}

bool CJAICam::SetOffsetY(int nValue)
{
	return SetValueInt((int8_t*)"OffsetY", nValue);
}

bool CJAICam::SetAcquisitionFrameRate(double dValue)
{
	return SetValueDouble((int8_t*)"AcquisitionFrameRate", dValue);
}

bool CJAICam::SetAcquisitionMode(CString strValue)
{
	return SetValueString((int8_t*)"AcquisitionMode", strValue);
}

bool CJAICam::SetTriggerMode(TRGMODE Mode)
{
	char value[MAX_PATH] = {0,};
	switch(Mode)
	{
		case TRG_Off : sprintf_s(value,"%s","Off"); break;
		case TRG_On  : sprintf_s(value,"%s","On"); break;
	}
	return SetValueString((int8_t*)"TriggerMode", CString(value));
}

bool CJAICam::SetTriggerSource(TRGSRC Src)
{
	char value[MAX_PATH] = {0,};
	switch(Src)
	{
	case SRC_Line5			: sprintf_s(value,"%s","Line5"			); break;
	case SRC_Line6			: sprintf_s(value,"%s","Line6"			); break;
	case SRC_SW				: sprintf_s(value,"%s","Software"		); break;
	case SRC_Output0		: sprintf_s(value,"%s","UserOutput0"	); break;
	case SRC_Output1		: sprintf_s(value,"%s","UserOutput1"	); break;
	case SRC_Output2		: sprintf_s(value,"%s","UserOutput2"	); break;
	case SRC_Output3		: sprintf_s(value,"%s","UserOutput3"	); break;
	case SRC_PG0			: sprintf_s(value,"%s","PulseGenerator0"); break;
	case SRC_PG1			: sprintf_s(value,"%s","PulseGenerator1"); break;
	case SRC_PG2			: sprintf_s(value,"%s","PulseGenerator2"); break;
	case SRC_PG3			: sprintf_s(value,"%s","PulseGenerator3"); break;
	case SRC_NAND1			: sprintf_s(value,"%s","NAND1Output"	); break;
	case SRC_NAND2			: sprintf_s(value,"%s","NAND2Output"	); break;
	case SRC_ACTION1		: sprintf_s(value,"%s","Action1"		); break;
	case SRC_ACTION2		: sprintf_s(value,"%s","Action2"		); break;
	case SRC_NOTCONNECTED	: sprintf_s(value,"%s","NotConnected"	); break;
	}
	return SetValueString((int8_t*)"TriggerSource", CString(value));
}

bool CJAICam::SetExposureMode(EXPMODE Mode)
{
	char value[MAX_PATH] = {0,};
	switch(Mode)
	{
	case EXP_Timed	: sprintf_s(value,"%s","Timed"); break;
	case EXP_PWC	: sprintf_s(value,"%s","TriggerWidth"); break;
	}
	return SetValueString((int8_t*)"ExposureMode", CString(value));
}

bool CJAICam::SetExposureTime(int nValue)
{
	return SetValueInt((int8_t*)"ExposureTimeRaw", nValue);
}

bool CJAICam::GetValueString(int8_t* pNodeName, CString &strValue)
{
	if(m_hCamera == NULL)
	{
		g_strErrorMsg = _T("Device is not connected.");
		return false;
	}

	J_STATUS_TYPE status = J_ST_SUCCESS;

	char value[MAX_PATH] = {0,};
	uint32_t size=sizeof(value);
	status = J_Camera_GetValueString(m_hCamera, pNodeName, value, &size);
	if(status != J_ST_SUCCESS)
	{
		g_strErrorMsg = GetErrorMsg(status);
		return false;
	}	

	strValue = (CString)value;

	return true;
}

bool CJAICam::SetValueString(int8_t* pNodeName, CString strValue)
{
	if(m_hCamera == NULL)
	{
		g_strErrorMsg = _T("Device is not connected.");
		return false;
	}

	J_STATUS_TYPE status = J_ST_SUCCESS;

	char path[MAX_PATH] = {0,};
	CStringToChar(strValue,path);

	status = J_Camera_SetValueString(m_hCamera, pNodeName, (int8_t*)path);
	if(status != J_ST_SUCCESS)
	{
		g_strErrorMsg = GetErrorMsg(status);
		return false;
	}

	return true;
}

bool CJAICam::GetValueInt(int8_t* pNodeName, int &nValue)
{
	if(m_hCamera == NULL)
	{
		g_strErrorMsg = _T("Device is not connected.");
		return false;
	}

	J_STATUS_TYPE status = J_ST_SUCCESS;
	int64_t value=0;
	status = J_Camera_GetValueInt64(m_hCamera, pNodeName, &value);
	if (status != J_ST_SUCCESS) 
	{ 
		g_strErrorMsg = GetErrorMsg(status); 
		return false; 
	}

	nValue = (int)value;

	return true;
}

bool CJAICam::SetValueInt(int8_t* pNodeName, int nValue)
{
	if(m_hCamera == NULL)
	{
		g_strErrorMsg = _T("Device is not connected.");
		return false;
	}

	J_STATUS_TYPE status = J_ST_SUCCESS;
	
	status = J_Camera_SetValueInt64(m_hCamera, pNodeName, nValue);
	if (status != J_ST_SUCCESS) 
	{ 
		g_strErrorMsg = GetErrorMsg(status); 
		return false; 
	}

	return true;
}

bool CJAICam::GetValueDouble(int8_t* pNodeName, double &dValue)
{
	if(m_hCamera == NULL)
	{
		g_strErrorMsg = _T("Device is not connected.");
		return false;
	}

	J_STATUS_TYPE status = J_ST_SUCCESS;
	double value=0;
	status = J_Camera_GetValueDouble(m_hCamera, pNodeName, &value);
	if (status != J_ST_SUCCESS) 
	{ 
		g_strErrorMsg = GetErrorMsg(status); 
		return false; 
	}

	dValue = value;

	return true;
}

bool CJAICam::SetValueDouble(int8_t* pNodeName, double dValue)
{
	if(m_hCamera == NULL)
	{
		g_strErrorMsg = _T("Device is not connected.");
		return false;
	}

	J_STATUS_TYPE status = J_ST_SUCCESS;

	status = J_Camera_SetValueDouble(m_hCamera, pNodeName, dValue);
	if (status != J_ST_SUCCESS) 
	{ 
		g_strErrorMsg = GetErrorMsg(status); 
		return false; 
	}

	return true;
}

bool CJAICam::OnExecuteCommand(int8_t* pNodeName)
{
	if(m_hCamera == NULL)
	{
		g_strErrorMsg = _T("Device is not connected.");
		return false;
	}

	J_STATUS_TYPE status = J_ST_SUCCESS;

	status = J_Camera_ExecuteCommand(m_hCamera, pNodeName);
	if(status != J_ST_SUCCESS)
	{
		g_strErrorMsg = GetErrorMsg(status);
		return false;
	}	

	return true;
}