
// FTech_JaiCamGigEDlg.h : header file
//


#pragma once
#include "include/JAICamGigE.h"

// CFTech_JaiCamGigEDlg dialog
class CFTech_JaiCamGigEDlg : public CDialogEx
{
// Construction
public:
	CFTech_JaiCamGigEDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_FTECH_JAICAMGIGE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

public :
	CJAICam m_Camera;
	int m_nWidth;
	int m_nHeight;
	int m_nBpp;
	CWinThread* m_pThDsp;
	BITMAPINFO* m_pBmpInfo;
	bool m_bThDspWork;
	void CreateBmpInfo(int nWidth, int nHeight, int nBpp);
	void OnDisplay();

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnConnection();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnAcq();
	afx_msg void OnBnClickedBtnRefresh();
	afx_msg void OnBnClickedBtnAwb();
	afx_msg void OnBnClickedBtnEvent();
	afx_msg void OnBnClickedRbtnCont();
	afx_msg void OnBnClickedRbtnSoft();
	afx_msg void OnBnClickedRbtnHard();
};
