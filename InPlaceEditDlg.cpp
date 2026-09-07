#include "pch.h"

#include "enums.hpp"
#include "InPlaceEditDlg.hpp"

#include <bit>
#include <vector>

constexpr int RES_ID_COMBO = 128;

#ifdef min
#undef min
#endif

CInPlaceEditDlg::CInPlaceEditDlg() :
    CDialog()
{
}

void CInPlaceEditDlg::Create(const DataType dataType,
	const CRect& rect, CWnd* pParent)
{
	MSG Msg{};

	// Make sure any previous CInPlaceEditDlg is really destroyed.
	while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	m_iCurSel = -1;
	m_dwData = 0;
	m_bOK = false;
	mDataType = dataType;
	m_rect = rect;
	AdjustRect();
	m_pParentWnd = pParent;

	// Dynamically create dialog
	// See https://www.codeproject.com/articles/An-introduction-to-lex-and-yacc-part-2
	// for how to create a dialog dynamically
	std::vector<char> memory;
	DLGTEMPLATE* pDlgTemplate = nullptr;
	std::size_t size = sizeof(DLGTEMPLATE);
	CFont* pFont = m_pParentWnd->GetFont();
	LOGFONT lf{}; pFont->GetLogFont(&lf);
	CStringW strFontName(lf.lfFaceName);

	memory.resize(size);
	pDlgTemplate = std::bit_cast<DLGTEMPLATE*>(&memory.front());
	pDlgTemplate->style = DS_SETFONT | DS_SETFOREGROUND | WS_POPUP |
		WS_VISIBLE;
	pDlgTemplate->dwExtendedStyle = 0;
	pDlgTemplate->cdit = 0;
	pDlgTemplate->x = m_rect.left & 0xffff;
	pDlgTemplate->y = m_rect.top & 0xffff;
	pDlgTemplate->cx = m_rect.Width() & 0xffff;
	pDlgTemplate->cy = m_rect.Height() & 0xffff;
	memory.resize(size + sizeof(WORD));

	// No Menu
	*(std::bit_cast<WORD*>(&memory[size])) = 0;
	size = memory.size();
	memory.resize(size + sizeof(WORD));

	// Default Windows Class
	*(std::bit_cast<WORD*>(&memory[size])) = 0;
	size = memory.size();
	memory.resize(size + sizeof(wchar_t));

	// Caption
	*(std::bit_cast<wchar_t*>(&memory[size])) = 0;
	size = memory.size();
	memory.resize(size + sizeof(WORD));

	// Font Size
	*(std::bit_cast<WORD*>(&memory[size])) = 8;
	size = memory.size();
	memory.resize(size + sizeof(wchar_t) * (strFontName.GetLength() + 1));

	// Font Name
	::memcpy(std::bit_cast<wchar_t*>(&memory[size]),
		static_cast<const wchar_t*>(strFontName),
		sizeof(wchar_t) * (strFontName.GetLength() + 1));

	// Modeless dialog
	CreateIndirect(std::bit_cast<DLGTEMPLATE*>(&memory.front()), m_pParentWnd);
}

void CInPlaceEditDlg::AdjustRect()
{
	switch (mDataType)
	{
	case DataType::ComboBox:
		m_rect.right += static_cast<LONG>
			(1.5 * static_cast<double>(::GetSystemMetrics(SM_CXVSCROLL)));
		m_rect.bottom += 6;
		break;
	default:
		// Should not happen
		ASSERT(0);
		break;
	}
}

BEGIN_MESSAGE_MAP(CInPlaceEditDlg, CDialog)
	ON_CBN_KILLFOCUS(RES_ID_COMBO, OnCtrlKillFocus)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

BOOL CInPlaceEditDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	switch (mDataType)
	{
	case DataType::ComboBox:
	{
		CComboBox* pCombo = nullptr;

		m_pCtrl = std::make_unique<CComboBox>();
		pCombo = static_cast<CComboBox*>(m_pCtrl.get());
		pCombo->Create(WS_VISIBLE | WS_BORDER | WS_CHILD | WS_TABSTOP |
			WS_VSCROLL | CBS_DROPDOWNLIST, m_rect, this,
			RES_ID_COMBO);
		m_pParentWnd->GetParent()->SendMessage(WM_POPULATE_DATA, 0,
			std::bit_cast<LPARAM>(pCombo));

		// Set the dimensions of the drop down list.
		int nListItemHeight = pCombo->GetItemHeight(0);
		// The drop down height needs to be set to 1 more than the number
		// of entries to display. If you don't then a combo with just one
		// entry in drop down doesn't get displayed at all. Limit the
		// number of items to display to be 11 [- 1 = 10].
		const int nMaxEntries = std::min(11, pCombo->GetCount() + 1);

		CRect rectScreen = m_rect;
		auto pDC = pCombo->GetDC();
		int max = pCombo->GetDroppedWidth();

		for (int idx = 0, count = pCombo->GetCount(); idx < count; ++idx)
		{
			CString strTemp;
			CSize curr;

			pCombo->GetLBText(idx, strTemp);
			curr = pDC->GetTextExtent(strTemp);

			if (curr.cx > max)
				max = curr.cx;
		}

		pCombo->ReleaseDC(pDC);
		m_pParentWnd->ClientToScreen(rectScreen);
		SetWindowPos(nullptr, rectScreen.left, rectScreen.top,
			rectScreen.Width(), rectScreen.Height(),
			SWP_NOZORDER | SWP_NOACTIVATE);
		pCombo->SetWindowPos(nullptr, 0, 0, m_rect.Width(),
			m_rect.Height() + ((nListItemHeight + 2) * nMaxEntries),
			SWP_NOZORDER | SWP_NOACTIVATE);
		pCombo->SetDroppedWidth(max);
		pCombo->SelectString(-1, m_strText);
		break;
	}
	default:
		// Should not happen
		ASSERT(0);
		break;
	}

	m_pCtrl->SetFont(m_pParentWnd->GetFont());
	m_pCtrl->SetFocus();
	return FALSE;
}

void CInPlaceEditDlg::OnOK()
{
	switch (mDataType)
	{
	case DataType::ComboBox:
	{
		const CComboBox* pComboBox = static_cast<CComboBox*>(m_pCtrl.get());

		pComboBox->GetWindowText(m_strText);
		m_iCurSel = pComboBox->GetCurSel();

		if (m_iCurSel != CB_ERR)
			m_dwData = pComboBox->GetItemData(m_iCurSel);

		break;
	}
	default:
		// Should not happen
		ASSERT(0);
		break;
	}

	m_bOK = true;
	// Close dialog before passing on WM_FINISHED_EDITING
	// to allow that routine to issue another edit request.
	// PostMessage not SendMessage, otherwise boom!
	PostMessage(WM_CLOSE);
	m_pParentWnd->GetParent()->PostMessage(WM_FINISHED_EDITING);
}

void CInPlaceEditDlg::OnCancel()
{
	m_bOK = false;
	// PostMessage not SendMessage, otherwise boom!
	PostMessage(WM_CLOSE);
}

void CInPlaceEditDlg::PostNcDestroy()
{
	m_hWnd = nullptr;
}

void CInPlaceEditDlg::OnCtrlKillFocus()
{
	if (!m_bOK)
	{
		CString str;

		switch (mDataType)
		{
		case DataType::ComboBox:
		{
			const CComboBox* pComboBox =
				static_cast<CComboBox*>(m_pCtrl.get());

			pComboBox->GetWindowText(str);

			if (str != m_strText)
				OnOK();
			else
				OnCancel();

			break;
		}
		default:
			// Should not happen
			ASSERT(0);
			break;
		}
	}
}

void CInPlaceEditDlg::OnClose()
{
	DestroyWindow();
}
