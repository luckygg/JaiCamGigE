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

	//----- ���� �� ���� -----//
	bool OnConnect(int idx, bool bColorConvert=false);
	bool OnDisconnect();

	//----- ���� ��� ���� -----//
	bool OnStartAcquisition();
	bool OnStopAcquisition();

	//----- ���� ��� ��� ���� -----//
	bool SetContinuousMode();
	bool SetSoftTriggerMode();
	bool SetHardTriggerMode();
	bool OnTriggerEvent();

	//----- ���� ����-----//
	bool OnSaveImage(CString strPath);	// �̹��� ����.

	//----- ���� ���� / �ҷ����� -----//
	bool SetUserSetSelector(USER User);
	bool OnUserSetSave();
	bool OnUserSetLoad();
	
	//----- ȭ��Ʈ�뷱�� -----//
	bool OnCalculateWhiteBalance();

	//----- Ȯ�� �� ��ȯ �Լ� -----//
	bool IsConnected() { return m_isConnected; }
	bool IsActive() { return m_isActived; }
	bool GetDeviceUserID(CString &strValue);				// Device User ID ��ȯ.
	bool GetDeviceModelName(CString &strValue);				// Device Model Name ��ȯ.
	bool GetSerialNumber(CString &strValue);				// Serial Number ��ȯ.
	bool GetOffsetX(int &nValue);							// Offset X ��ȯ.
	bool GetOffsetY(int &nValue);							// Offset Y ��ȯ.
	bool GetAcquisitionMode(CString &strValue);				// Acquisition Mode ��ȯ.
	bool GetAcquisitionFrameRate(double &dValue);			// Frame Rate ��ȯ.
	bool GetTriggerMode(CString &strValue);					// Trigger Mode ��ȯ.
	bool GetTriggerSource(CString &strValue);				// Trigger Source ��ȯ.
	bool GetExposureMode(CString &strValue);				// Exposure Mode ��ȯ.
	bool GetExposureTimeRaw(int &nValue);					// Exposure Time ��ȯ.
	bool GetPixelFormat(CString &strValue);					// Pixel Format ��ȯ.
	CString GetIPAddress() { return m_strIP; }				// IP Address ��ȯ.
	CString GetMACAddress() { return m_strMAC; }			// MAC Address ��ȯ.
	int  GetWidth()  { return m_nWidth ; }					// Image Width ��ȯ.
	int  GetHeight() { return m_nHeight; }					// Image Height ��ȯ.
	int  GetBPP()	 { return m_nBpp;	 }					// Image Bit per Pixel ��ȯ.
	BYTE* GetImageBuffer() { return m_pbyBuffer; }			// Buffer ��ȯ.
	//CString GetLastErrorMessage() { return m_strErrorMsg; }	// ������ ���� �޽��� ��ȯ.

	//----- ���� �Լ� -----//
	bool SetDeviceUserID(CString strValue);					// Device User ID ����.
	bool SetOffsetX(int nValue);							// Offset X ����.
	bool SetOffsetY(int nValue);							// Offset Y ����.
	bool SetAcquisitionFrameRate(double dValue);			// Frame Rate ����.
	bool SetAcquisitionMode(CString strValue);				// Acquistiion Mode ����.
	bool SetTriggerMode(TRGMODE Mode);						// Trigger Mode ����.
	bool SetTriggerSource(TRGSRC Src);						// Trigger Source ����.
	bool SetTriggerOverlap(TRGOVL Ovl);						// Trigger Overlap ����.
	bool SetExposureMode(EXPMODE Mode);						// Exposure Mode ����.
	bool SetExposureTime(int nValue);					// Exposure Time ����.

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