#pragma once

#include "InPlaceEditDlg.hpp"

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
	void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support


// Implementation
protected:
	void CopyToClipboard(const CString& str);
	void LoadJSON(const std::string& json);
	bool ValidateText(const HTREEITEM hItem, const wchar_t* pszText);
	CString Unescape(const CString& strText);

	// Generated message map functions
	BOOL OnInitDialog() override;
	BOOL PreTranslateMessage(MSG* pMsg) override;
	void OnOK() override;
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnRClickTree(NMHDR*, LRESULT*);
	afx_msg void OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnPopulateData(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnFinishedEditing(WPARAM, LPARAM);
	afx_msg void OnCopy();
	afx_msg void OnEdit();
	afx_msg void OnLoad();
	afx_msg void OnExport();
	DECLARE_MESSAGE_MAP()

private:
	HACCEL m_hAccel{};
	CBitmap m_bmEdit;
	CBitmap m_bmExport;
	CBitmap m_bmCopy;
	HICON m_hIcon{};
	CTreeCtrl m_TreeCtrl;
	CInPlaceEditDlg m_InPlaceDlg;
	HTREEITEM m_hInplaceItem{};
};
