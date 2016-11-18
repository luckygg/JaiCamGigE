//----------------------------------------------------------
// JAI GigE Camera Class
//----------------------------------------------------------
// Programmed by William Kim
//----------------------------------------------------------
// Last Update : 2016-11-11 17:27
// Modified by William Kim
//----------------------------------------------------------

#pragma once

#include <Jai_Factory.h>
#pragma comment (lib,"Jai_Factory")


//----- enum type -----//
enum USER {USER_Default=0, USER_UserSet1, USER_UserSet2, USER_UserSet3};
enum TRGMODE {TRG_On=0, TRG_Off};
enum TRGSRC {SRC_Line5, SRC_Line6, SRC_SW, SRC_Output0, SRC_Output1, SRC_Output2, SRC_Output3, SRC_PG0, SRC_PG1, SRC_PG2, SRC_PG3, SRC_NAND1, SRC_NAND2, SRC_ACTION1, SRC_ACTION2, SRC_NOTCONNECTED};
enum TRGOVL {OVL_Off=0, OVL_ReadOut, OVL_PreFrm};
enum EXPMODE {EXP_Timed=0, EXP_PWC};
 
typedef struct StCameraInfo
{
	CStringArray ModelName;
	CStringArray SN;
	CStringArray DriverType;
	CStringArray IP;
	CStringArray MAC;

	void Clear() {
		ModelName.RemoveAll();
		SN.RemoveAll();
		DriverType.RemoveAll();
		IP.RemoveAll();
		MAC.RemoveAll();
	}
}StCameraInfo;

extern StCameraInfo g_stCamInfo;
extern CString g_strErrorMsg;
class CJAICam
{
public:
	CJAICam(void);
	~CJAICam(void);

	static bool SearchAndGetDeviceCount(int &nValue);
	
	static CString GetDeviceModelName(int idx) { 
		if (idx >= g_stCamInfo.ModelName.GetCount()) 
			return _T("Out of index.");
		return g_stCamInfo.ModelName.GetAt(idx); }
	static CString GetDeviceSN(int idx) { 
		if (idx >= g_stCamInfo.ModelName.GetCount()) 
			return _T("Out of index.");
		return g_stCamInfo.SN.GetAt(idx); }
	static CString GetDeviceDriverType(int idx) { 
		if (idx >= g_stCamInfo.ModelName.GetCount()) 
			return _T("Out of index.");
		return g_stCamInfo.DriverType.GetAt(idx); }
	static CString GetDeviceIP(int idx) { 
		if (idx >= g_stCamInfo.ModelName.GetCount()) 
			return _T("Out of index.");
		return g_stCamInfo.IP.GetAt(idx); }
	static CString GetDeviceMAC(int idx) { 
		if (idx >= g_stCamInfo.ModelName.GetCount()) 
			return _T("Out of index.");
		return g_stCamInfo.MAC.GetAt(idx); }
	static CString GetLastErrorMessage() { return g_strErrorMsg; }

	//----- 연결 및 해제 -----//
	bool OnConnect(int idx, bool bColorConvert=false);
	bool OnDisconnect();

	//----- 영상 취득 제어 -----//
	bool OnStartAcquisition();
	bool OnStopAcquisition();

	//----- 영상 취득 방식 선택 -----//
	bool SetContinuousMode();
	bool SetSoftTriggerMode();
	bool SetHardTriggerMode();
	bool OnTriggerEvent();

	//----- 영상 저장-----//
	bool OnSaveImage(CString strPath);	// 이미지 저장.

	//----- 설정 저장 / 불러오기 -----//
	bool SetUserSetSelector(USER User);
	bool OnUserSetSave();
	bool OnUserSetLoad();
	
	//----- 화이트밸런스 -----//
	bool OnCalculateWhiteBalance();

	//----- 확인 및 반환 함수 -----//
	bool IsConnected() { return m_isConnected; }
	bool IsActive() { return m_isActived; }
	bool GetDeviceUserID(CString &strValue);				// Device User ID 반환.
	bool GetDeviceModelName(CString &strValue);				// Device Model Name 반환.
	bool GetSerialNumber(CString &strValue);				// Serial Number 반환.
	bool GetOffsetX(int &nValue);							// Offset X 반환.
	bool GetOffsetY(int &nValue);							// Offset Y 반환.
	bool GetAcquisitionMode(CString &strValue);				// Acquisition Mode 반환.
	bool GetAcquisitionFrameRate(double &dValue);			// Frame Rate 반환.
	bool GetTriggerMode(CString &strValue);					// Trigger Mode 반환.
	bool GetTriggerSource(CString &strValue);				// Trigger Source 반환.
	bool GetExposureMode(CString &strValue);				// Exposure Mode 반환.
	bool GetExposureTimeRaw(int &nValue);					// Exposure Time 반환.
	bool GetPixelFormat(CString &strValue);					// Pixel Format 반환.
	CString GetIPAddress() { return m_strIP; }				// IP Address 반환.
	CString GetMACAddress() { return m_strMAC; }			// MAC Address 반환.
	int  GetWidth()  { return m_nWidth ; }					// Image Width 반환.
	int  GetHeight() { return m_nHeight; }					// Image Height 반환.
	int  GetBPP()	 { return m_nBpp;	 }					// Image Bit per Pixel 반환.
	BYTE* GetImageBuffer() { return m_pbyBuffer; }			// Buffer 반환.
	//CString GetLastErrorMessage() { return m_strErrorMsg; }	// 마지막 에러 메시지 반환.

	//----- 설정 함수 -----//
	bool SetDeviceUserID(CString strValue);					// Device User ID 설정.
	bool SetOffsetX(int nValue);							// Offset X 설정.
	bool SetOffsetY(int nValue);							// Offset Y 설정.
	bool SetAcquisitionFrameRate(double dValue);			// Frame Rate 설정.
	bool SetAcquisitionMode(CString strValue);				// Acquistiion Mode 설정.
	bool SetTriggerMode(TRGMODE Mode);						// Trigger Mode 설정.
	bool SetTriggerSource(TRGSRC Src);						// Trigger Source 설정.
	bool SetTriggerOverlap(TRGOVL Ovl);						// Trigger Overlap 설정.
	bool SetExposureMode(EXPMODE Mode);						// Exposure Mode 설정.
	bool SetExposureTime(int nValue);					// Exposure Time 설정.

	HANDLE GetHandleGrabDone() { return m_hGrabDone; }

	void ResetEventGrabDone() { ResetEvent(m_hGrabDone); }
private:
	bool OpenFactory();
	bool CloseFactory();
	void StreamCBFunc(J_tIMAGE_INFO * pAqImageInfo);
	void OnCreateBmpInfo(int nWidth, int nHeight, int nBpp);

	bool GetValueString(int8_t* pNodeName, CString &strValue);
	bool SetValueString(int8_t* pNodeName, CString strValue);
	bool GetValueInt(int8_t* pNodeName, int &nValue);
	bool SetValueInt(int8_t* pNodeName, int nValue);
	bool GetValueDouble(int8_t* pNodeName, double &dValue);
	bool SetValueDouble(int8_t* pNodeName, double dValue);
	bool OnExecuteCommand(int8_t* pNodeName);

	FACTORY_HANDLE	m_hFactory;
	CAM_HANDLE      m_hCamera;
	THRD_HANDLE     m_hThread;
	J_tIMAGE_INFO   m_ImgColor;
	HANDLE			m_hGrabDone;
	BYTE*			m_pbyBuffer;
	BITMAPINFO*		m_pBitmapInfo;

	//CString m_strErrorMsg;
	CString m_strIP;
	CString m_strMAC;
	int	m_nWidth;
	int	m_nHeight;
	int	m_nBpp;
	int	m_nGainR;
	int	m_nGainG;
	int	m_nGainB;
	bool m_isConnected;
	bool m_isActived;
	bool m_isColorConvert;
};

#ifdef _UNICODE
static bool CStringToChar(const CString strIn, char* pchOut)
{
	if (pchOut == NULL) return false;

	size_t szCvt = 0;
	wcstombs_s(&szCvt, pchOut, strIn.GetLength()+1, strIn, _TRUNCATE);

	return true;
}
#else
static bool CStringToChar(CString strIn, char* pchOut)
{
	if (pchOut == NULL) return false;

	strcpy(pchOut, CT2A(strIn));

	return true;
}

#endif