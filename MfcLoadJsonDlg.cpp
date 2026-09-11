
#include "pch.h"

#include "enums.hpp"
#include "Export.hpp"
#include "JsonParser.hpp"
#include "MfcLoadJsonDlg.h"
#include "resource.h"
#include "Types.hpp"

#include <boost/system/detail/error_code.hpp>

#include <afxdialogex.h>
#include <atlconv.h>

#include <fstream>
#include <ios>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

constexpr wchar_t szJsonFilter[] = L"JSON (*.json)|*.json|All Files (*)|*||";

// CAboutDlg dialog used for App About
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CMfcLoadJsonDlg dialog
CMfcLoadJsonDlg::CMfcLoadJsonDlg(CWnd* pParent /*=nullptr*/) :
	CDialogEx(IDD_MFCLOADJSON_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMfcLoadJsonDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CMfcLoadJsonDlg::CopyToClipboard(const CString& str)
{
	if (!OpenClipboard())
	{
		AfxMessageBox(_T("Cannot open the Clipboard"));
		return;
	}

	// Remove the current Clipboard contents
	if (!EmptyClipboard())
	{
		AfxMessageBox(_T("Cannot empty the Clipboard"));
		return;
	}

	const size_t cbStr = (str.GetLength() + 1) * sizeof(TCHAR);

	if (HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, cbStr);
		hData)
	{
		memcpy_s(GlobalLock(hData), cbStr, str, cbStr);
		GlobalUnlock(hData);

		// For the appropriate data formats...
		UINT uiFormat = (sizeof(TCHAR) == sizeof(WCHAR)) ? CF_UNICODETEXT : CF_TEXT;

		if (::SetClipboardData(uiFormat, hData) == nullptr)
			AfxMessageBox(_T("Unable to set Clipboard data"));
	}

	CloseClipboard();
}

void CMfcLoadJsonDlg::LoadJSON(const std::string& json)
{
	json_parser p(m_TreeCtrl);
	boost::system::error_code ec;

	m_TreeCtrl.SetRedraw(FALSE);
	m_TreeCtrl.DeleteAllItems();
	p.write(json.data(), json.size(), ec);

	if (ec)
	{
		m_TreeCtrl.DeleteAllItems();
		AfxMessageBox(L"File failed to parse");
	}
	else
		m_TreeCtrl.Expand(m_TreeCtrl.GetRootItem(), TVE_EXPAND);

	m_TreeCtrl.SetRedraw();
}

bool CMfcLoadJsonDlg::ValidateText(const HTREEITEM hItem,
	const wchar_t* pszText)
{
	bool ret = false;
	auto type = m_TreeCtrl.GetItemData(hItem) & ~(json_type::Array);

	switch (type)
	{
	case json_type::Boolean:
	{
		const std::wstring_view text{ pszText };

		ret = text == L"true" || text == L"false";
		break;
	}
	case json_type::Number:
	{
		// https://datatracker.ietf.org/doc/html/rfc8259#section-6
		const std::wregex rx{ LR"(-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][-+]?\d+)?)" };

		ret = std::regex_match(pszText, rx);
		break;
	}
	case json_type::String:
	{
		// https://datatracker.ietf.org/doc/html/rfc8259#section-7
		const std::wregex rx{ LR"(([\x20\x21\x23-5b\x5d-\ud7ff\ue000-\uffff]|)"
			LR"([\ud800-\udbff][\udc00-\udfff]|)"
			LR"(\\(["\\/bfnrt]|u[\dA-Fa-f]{4}))*)"};

		ret = std::regex_match(pszText, rx);
		break;
	}
	default:
		break;
	}

	return ret;
}

CString CMfcLoadJsonDlg::Unescape(const CString& strText)
{
	CString strUnescaped;

	for (int idx = 0, len = strText.GetLength(); idx < len; ++idx)
	{
		auto c = strText[idx];

		if (c == '\\')
		{
			++idx;
			c = strText[idx];

			switch (c)
			{
			case '"':
			case '\\':
			case '/':
				strUnescaped += c;
				break;
			case 'b':
				strUnescaped += '\b';
				break;
			case 'f':
				strUnescaped += '\f';
				break;
			case 'n':
				strUnescaped += '\n';
				break;
			case 'r':
				strUnescaped += '\r';
				break;
			case 't':
				strUnescaped += '\t';
				break;
			case 'u':
			{
				wchar_t ch = L'\0';

				++idx;

				for (int count = 0; count < 4; ++count)
				{
					auto c = strText[idx + count];

					ch *= 16;

					if (c >= '0' && c <= '9')
						ch += c - '0';
					else if (c >= 'A' && c <= 'F')
						ch += 10 + c - 'A';
					else if (c >= 'a' && c <= 'f')
						ch += 10 + c - 'a';
				}

				strUnescaped += ch;
				idx += 3;
				break;
			}
			default:
				// Should not happen
				ASSERT(0);
				break;
			}
		}
		else
			strUnescaped += c;
	}

	return strUnescaped;
}

void CMfcLoadJsonDlg::DoExport(const HTREEITEM hItem)
{
	CFileDialog Dlg(FALSE, L"json", nullptr,
		OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, szJsonFilter);

	if (Dlg.DoModal() == IDOK)
	{
		std::string json = Export(hItem, m_TreeCtrl, whitespace::yes);
		std::ofstream os;

		// Enable exceptions on error.
		os.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
		os.open(Dlg.GetPathName(), std::ios_base::out | std::ios_base::trunc);
		os << json;
		os.close();
	}
}

BEGIN_MESSAGE_MAP(CMfcLoadJsonDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(IDM_COPY, OnCopy)
	ON_COMMAND(IDM_EDIT, OnEdit)
	ON_COMMAND(IDM_EXPORT, OnExport)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, OnLoad)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT, OnExportAll)
	ON_NOTIFY(NM_RCLICK, IDC_TREE, OnRClickTree)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_TREE, OnBeginLabelEdit)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_TREE, OnEndLabelEdit)
	ON_MESSAGE(WM_POPULATE_DATA, OnPopulateData)
	ON_MESSAGE(WM_FINISHED_EDITING, OnFinishedEditing)
END_MESSAGE_MAP()

// CMfcLoadJsonDlg message handlers
BOOL CMfcLoadJsonDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	if (CMenu* pSysMenu = GetSystemMenu(FALSE); pSysMenu != nullptr)
	{
		CString strAboutMenu;
		BOOL bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);

		ASSERT(bNameValid);

		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),
		MAKEINTRESOURCE(IDR_ACCELERATOR1));

	COLORMAP cm[] =
	{
		{RGB(0xfe, 0x05, 0xfe), ::GetSysColor(COLOR_MENU)}
	};

	m_bmEdit.LoadMappedBitmap(IDB_EDIT, 0, cm, sizeof(cm) / sizeof(COLORMAP));
	m_bmExport.LoadMappedBitmap(IDB_EXPORT, 0, cm, sizeof(cm) / sizeof(COLORMAP));
	m_bmCopy.LoadMappedBitmap(IDB_COPY, 0, cm, sizeof(cm) / sizeof(COLORMAP));

	m_TreeCtrl.SubclassDlgItem(IDC_TREE, this);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CMfcLoadJsonDlg::PreTranslateMessage(MSG* pMsg)
{
	if (m_hAccel)
		if (::TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
			return TRUE;

	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
		case VK_RETURN:
		{
			if (const CEdit* edit = m_TreeCtrl.GetEditControl(); edit)
			{
				bool valid = true;

				if (pMsg->wParam == VK_RETURN)
				{
					CString text;

					edit->GetWindowText(text);
					valid = ValidateText(m_TreeCtrl.GetSelectedItem(), text);
				}

				if (valid)
					edit->SendMessage(WM_KEYDOWN, pMsg->wParam, pMsg->lParam);

				return TRUE;
			}

			break;
		}
		default:
			break;
		}

		break;
	case WM_MOUSEWHEEL:
		if (m_InPlaceDlg.m_hWnd)
			m_InPlaceDlg.OnOK();

		break;
	default:
		break;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CMfcLoadJsonDlg::OnOK()
{
	// Prevent dialog closing when the return key is pressed
}

void CMfcLoadJsonDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
		CDialogEx::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CMfcLoadJsonDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect; GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
		CDialogEx::OnPaint();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMfcLoadJsonDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMfcLoadJsonDlg::OnRClickTree(NMHDR*, LRESULT* pResult)
{
	// We have to do this instead of calling GetSelectedItem()
	// in order to get the correct selection
	CPoint pt;
	GetCursorPos(&pt); m_TreeCtrl.ScreenToClient(&pt);
	UINT unFlags = 0;
	const HTREEITEM hItem = m_TreeCtrl.HitTest(pt, &unFlags);

	if (!hItem)
		return;

	const DWORD_PTR dwFlags = m_TreeCtrl.GetItemData(hItem);
	CMenu menu;

	if (hItem)
		m_TreeCtrl.SelectItem(hItem);

	menu.CreatePopupMenu();

	menu.AppendMenu(MF_STRING | ((dwFlags & json_type::Scalar) ?
		MF_ENABLED :
		MF_DISABLED),
		IDM_EDIT, L"Edit\tF2");
	menu.SetMenuItemBitmaps(IDM_EDIT, MF_BYCOMMAND, &m_bmEdit, nullptr);
	menu.AppendMenu(MF_STRING, IDM_EXPORT, L"Export\tCtrl+E");
	menu.SetMenuItemBitmaps(IDM_EXPORT, MF_BYCOMMAND, &m_bmExport, nullptr);
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, IDM_COPY, L"Copy\tCtrl+C");
	menu.SetMenuItemBitmaps(IDM_COPY, MF_BYCOMMAND, &m_bmCopy, nullptr);

	const DWORD pos = GetMessagePos();
	const CPoint clickPos(GET_X_LPARAM(pos), GET_Y_LPARAM(pos));

	menu.TrackPopupMenu(TPM_CENTERALIGN | TPM_RIGHTBUTTON,
		clickPos.x, clickPos.y, this, nullptr);
}

void CMfcLoadJsonDlg::OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult)
{
	const auto pDispInfo = std::bit_cast<LPNMTVDISPINFO>(pNMHDR);
	const auto type = static_cast<json_type>
		(m_TreeCtrl.GetItemData(pDispInfo->item.hItem));

	switch (type)
	{
	case json_type::Boolean:
	{
		CRect rect;

		TreeView_GetItemRect(m_TreeCtrl.m_hWnd,
			pDispInfo->item.hItem, &rect, TRUE);
		m_InPlaceDlg.m_strText = pDispInfo->item.pszText;
		m_InPlaceDlg.Create(CInPlaceEditDlg::DataType::ComboBox,
			rect, &m_TreeCtrl);
		m_hInplaceItem = pDispInfo->item.hItem;
		// Disable built in editing
		*pResult = TRUE;
		break;
	}
	case json_type::Number:
		// Enable built in editing
		*pResult = FALSE;
		break;
	case json_type::String:
	{
		CString strText = m_TreeCtrl.GetItemText(pDispInfo->item.hItem);
		CString strEscaped;

		for (int idx = 0, len = strText.GetLength(); idx < len; ++idx)
		{
			const auto c = strText[idx];

			if (c == '"' || c == '\\' || c == '/')
				strEscaped.AppendFormat(LR"(\%c)", c);
			else
				strEscaped += c;
		}

		if (auto edit = m_TreeCtrl.GetEditControl(); edit)
			edit->SetWindowText(strEscaped);

		// Enable built in editing
		*pResult = FALSE;
		break;
	}
	default:
		// Disable built in editing
		*pResult = TRUE;
		break;
	}
}

void CMfcLoadJsonDlg::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult)
{
	// If we accept the changes, we will set them ourselves
	*pResult = FALSE;

	if (const auto pInfo = std::bit_cast<TV_DISPINFO*>(pNMHDR);
		pInfo->item.pszText)
	{
		if (!ValidateText(pInfo->item.hItem, pInfo->item.pszText))
		{
			return;
		}

		if (m_TreeCtrl.GetItemData(pInfo->item.hItem) & json_type::String)
		{
			if (auto edit = m_TreeCtrl.GetEditControl(); edit)
			{
				// Unescape JSON string
				CString strText;
				CString strUnescaped;

				edit->GetWindowText(strText);
				strUnescaped = Unescape(strText);
				m_TreeCtrl.SetItemText(pInfo->item.hItem, strUnescaped);
			}
			else
			{
				// Shouldn't happen
				ASSERT(FALSE);
				*pResult = TRUE;
			}
		}
	}
}

LRESULT CMfcLoadJsonDlg::OnPopulateData(WPARAM, LPARAM lParam)
{
	if (const auto item = m_TreeCtrl.GetSelectedItem();
		static_cast<json_type>(m_TreeCtrl.GetItemData(item)) &
		json_type::Boolean)
	{
		auto pCombo = std::bit_cast<CComboBox*>(lParam);

		pCombo->AddString(L"true");
		pCombo->AddString(L"false");
	}

	return 0;
}

LRESULT CMfcLoadJsonDlg::OnFinishedEditing(WPARAM, LPARAM)
{
	if (m_InPlaceDlg.m_bOK)
		m_TreeCtrl.SetItemText(m_hInplaceItem, m_InPlaceDlg.m_strText);

	return 0;
}

void CMfcLoadJsonDlg::OnCopy()
{
	const HTREEITEM hItem = m_TreeCtrl.GetSelectedItem();
	const CString str = m_TreeCtrl.GetItemText(hItem);

	CopyToClipboard(str);
}

void CMfcLoadJsonDlg::OnEdit()
{
	const HTREEITEM hItem = m_TreeCtrl.GetSelectedItem();

	if (hItem)
	{
		auto editable = !m_TreeCtrl.ItemHasChildren(hItem);

		if (editable)
			m_TreeCtrl.EditLabel(hItem);
	}
}

void CMfcLoadJsonDlg::OnLoad()
{
	CFileDialog Dlg(TRUE, L"json", nullptr, OFN_HIDEREADONLY, szJsonFilter);

	if (Dlg.DoModal() == IDOK)
	{
		std::ifstream file(CT2A(Dlg.GetPathName()));
		const std::string json{ std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>() };

		LoadJSON(json);
	}
}

void CMfcLoadJsonDlg::OnExport()
{
	const HTREEITEM hItem = m_TreeCtrl.GetSelectedItem() ?
		m_TreeCtrl.GetSelectedItem() :
		m_TreeCtrl.GetRootItem();

	DoExport(hItem);
}

void CMfcLoadJsonDlg::OnExportAll()
{
	DoExport(m_TreeCtrl.GetRootItem());
}
