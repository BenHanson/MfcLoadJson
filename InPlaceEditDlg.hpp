#pragma once

#include <memory>

class CInPlaceEditDlg : public CDialog
{
public:
    enum class DataType { ComboBox };
    CString m_strText;
    int m_iCurSel = -1;
    DWORD_PTR m_dwData = 0;
    bool m_bOK = false;

    CInPlaceEditDlg();

    void Create(const DataType dataType, const CRect& rect, CWnd* pParent);
    void OnOK() override;

protected:
    void AdjustRect();

    BOOL OnInitDialog() override;
    void OnCancel() override;
    void PostNcDestroy() override;

    afx_msg void OnCtrlKillFocus();
    afx_msg void OnClose();
    DECLARE_MESSAGE_MAP()

private:
    DataType mDataType{};
    CRect m_rect;
    std::unique_ptr<CWnd> m_pCtrl;
};
