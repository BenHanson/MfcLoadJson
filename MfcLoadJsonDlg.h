#pragma once

#include <string>

// CMfcLoadJsonDlg dialog
class CMfcLoadJsonDlg : public CDialogEx
{
// Construction
public:
	CMfcLoadJsonDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCLOADJSON_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	CTreeCtrl m_TreeCtrl;

	void LoadJSON(const std::string& json);

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnExport();
	DECLARE_MESSAGE_MAP()
};
