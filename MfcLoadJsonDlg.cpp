
#include "pch.h"

#include "Export.hpp"
#include "JsonParser.hpp"
#include "MfcLoadJson.h"
#include "MfcLoadJsonDlg.h"
#include "resource.h"
#include "Types.hpp"

#include <boost/system/detail/error_code.hpp>

#include <afxdialogex.h>
#include <atlconv.h>

#include <fstream>
#include <ios>
#include <iterator>
#include <string>

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

BEGIN_MESSAGE_MAP(CMfcLoadJsonDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_EXPORT, OnExport)
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

	m_TreeCtrl.SubclassDlgItem(IDC_TREE, this);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMfcLoadJsonDlg::OnOK()
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

void CMfcLoadJsonDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
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
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMfcLoadJsonDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMfcLoadJsonDlg::OnExport()
{
	CFileDialog Dlg(FALSE, L"json", nullptr,
		OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, szJsonFilter);

	if (Dlg.DoModal() == IDOK)
	{
		const HTREEITEM hItem = m_TreeCtrl.GetSelectedItem() ?
			m_TreeCtrl.GetSelectedItem() :
			m_TreeCtrl.GetRootItem();
		std::string json = Export(hItem, m_TreeCtrl, whitespace::yes);
		std::ofstream os;

		// Enable exceptions on error.
		os.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
		os.open(Dlg.GetPathName(), std::ios_base::out | std::ios_base::trunc);
		os << json;
		os.close();
	}
}
