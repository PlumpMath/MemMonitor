
#include "stdafx.h"

#include "PropertiesWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "MemMonitor.h"
#include "Lib/DiaWrapper.h"
#include "dia2.h"
#include <boost/interprocess/streams/bufferstream.hpp>
#include "visualizer/PropertyMaker.h"
#include "visualizer/DefaultPropertyMaker.h"


using namespace dia;
using namespace sharedmemory;
using namespace boost::interprocess;


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar

CPropertiesWnd::CPropertiesWnd() :
	m_CurrentSymbolName(L"None")
,	m_State(VIEW)
{
}

CPropertiesWnd::~CPropertiesWnd()
{
}

BEGIN_MESSAGE_MAP(CPropertiesWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
	ON_COMMAND(ID_REFRESH, OnRefresh)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
	ON_CONTROL_RANGE(CBN_SELCHANGE, ID_COMBOBOX, ID_COMBOBOX, OnComboBoxSelChange)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar �޽��� ó����

void CPropertiesWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient,rectCombo;
	GetClientRect(rectClient);

	m_wndObjectCombo.GetWindowRect(&rectCombo);

	int cyCmb = rectCombo.Size().cy;
	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndObjectCombo.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), 200, 
		SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top + cyCmb, rectClient.Width(), cyTlb, 
		SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndPropList.SetWindowPos(NULL, rectClient.left, rectClient.top + cyCmb + cyTlb, rectClient.Width(), 
		rectClient.Height() -(cyCmb+cyTlb), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CPropertiesWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_SORT
													| WS_BORDER | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_wndObjectCombo.Create(dwViewStyle, rectDummy, this, ID_COMBOBOX))
	{
		TRACE0("�Ӽ� �޺� ���ڸ� ������ ���߽��ϴ�. \n");
		return -1;
	}
	m_wndObjectCombo.SetFont(CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT)));
	UpdateComboBox();

	if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD, rectDummy, this, 2))
	{
		TRACE0("�Ӽ� ǥ�� ������ ���߽��ϴ�. \n");
		return -1;      // ������ ���߽��ϴ�.
	}
	InitPropList();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
	m_wndToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE /* ��� */);
	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(theApp.m_bHiColorIcons ? IDB_PROPERTIES_HC : IDR_PROPERTIES, 0, 0, TRUE /* ��� */);

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);

	// ��� ����� �θ� �������� �ƴ� �� ��Ʈ���� ���� ����õ˴ϴ�.
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	// Ÿ�̸� ����
	SetTimer(ID_TIMER, REFRESH_TIME, NULL);

	AdjustLayout();
	return 0;
}

void CPropertiesWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPropertiesWnd::OnExpandAllProperties()
{
	m_wndPropList.ExpandAll();
}

void CPropertiesWnd::InitPropList()
{
	SetPropListFont();
	m_wndPropList.EnableHeaderCtrl(FALSE);
	//m_wndPropList.EnableDescriptionArea();
	m_wndPropList.SetVSDotNetLook();
	m_wndPropList.MarkModifiedProperties();
}


//------------------------------------------------------------------------
// �޺��ڽ� �ʱ�ȭ
// �����޸𸮿� ��ϵ� �����͸� �޺��ڽ��� �߰��Ѵ�.
//------------------------------------------------------------------------
void	CPropertiesWnd::UpdateComboBox()
{
	m_State = REFRESH;

	const int nItem = m_wndObjectCombo.GetCurSel();
	std::string selectName;
	if (nItem != LB_ERR)
	{
		CString comboSelectText;
		m_wndObjectCombo.GetLBText(nItem , comboSelectText);
			selectName = common::wstring2string((LPCTSTR)comboSelectText);
	}

	m_wndObjectCombo.ResetContent();
	m_wndObjectCombo.AddString( L"_None" ); // default

	sharedmemory::MemoryList memList;
	sharedmemory::EnumerateMemoryInfo(memList);
	BOOST_FOREACH(sharedmemory::SMemoryInfo &info, memList)
	{
		std::wstring wstr = common::string2wstring( info.name );
		const int idx = m_wndObjectCombo.AddString( wstr.c_str() );
		if (idx != LB_ERR && selectName == info.name)
			m_wndObjectCombo.SetCurSel(idx);
	}	

	if (m_wndObjectCombo.GetCurSel() == LB_ERR)
		m_wndObjectCombo.SetCurSel(0);

	m_State = VIEW;
}


//------------------------------------------------------------------------
// symbolName �� �̸��� ������ �޺��ڽ� ��Ʈ���� 
// ȭ�鿡 ���̰� �Ѵ�.
//------------------------------------------------------------------------
void	CPropertiesWnd::ChangeComboBoxFocus(const CString &symbolName)
{
	const int nItem = m_wndObjectCombo.FindString(0, symbolName);
	if (nItem == CB_ERR)
	{
		m_wndObjectCombo.SetCurSel(0);
		AfxMessageBox( common::formatw("\"%s\" Item�� ã�� ���߽��ϴ�.",
			(LPCWSTR)symbolName).c_str() );
		return;
	}
	m_wndObjectCombo.SetCurSel( nItem );
}


void CPropertiesWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wndPropList.SetFocus();
}

void CPropertiesWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CPropertiesWnd::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntPropList.CreateFontIndirect(&lf);

	m_wndPropList.SetFont(&m_fntPropList);
}

void CPropertiesWnd::SetVSDotNetLook(BOOL bSet)
{
	m_wndPropList.SetVSDotNetLook(bSet);
	m_wndPropList.SetGroupNameFullWidth(bSet);
}


//------------------------------------------------------------------------
// ���ڷ� �Ѿ�� symbolType��� Property�� �����Ѵ�.
//------------------------------------------------------------------------
void CPropertiesWnd::UpdateProperty(const CString &symbolName)
{
	if (m_CurrentSymbolName == symbolName)
		return;

	m_wndPropList.UpdateProperty( symbolName );

	std::string tmpStr = common::wstring2string((LPCWSTR)symbolName);
	std::string str = sharedmemory::ParseObjectName(tmpStr);

	CComPtr<IDiaSymbol> pSymbol = CDiaWrapper::Get()->FindType(str);
	if (!pSymbol)
	{
		::AfxMessageBox(
			common::formatw("\"%s\" �ش��ϴ� �ɺ��� Pdb���Ͽ� �����ϴ�.", 
				tmpStr.c_str()).c_str() );
		SetWindowTextW(L"Not Found");
		return;
	}

	sharedmemory::SMemoryInfo memInfo;
	if (!sharedmemory::FindMemoryInfo(tmpStr, memInfo))
	{
		::AfxMessageBox( 
				common::formatw("�����޸𸮿� %s Ÿ���� ������ �����ϴ�.",
					tmpStr.c_str()).c_str() );
		SetWindowTextW(L"Not Found");
		return;
	}

	// Outputâ�� ���
	global::PrintOutputWnd( "[" + tmpStr + " ] �ɺ� ����" );
	dbg::Print(  "%s �ɺ� ����", tmpStr.c_str() );
	//

	SetWindowTextW(symbolName);
	m_CurrentSymbolName = symbolName;
	ChangeComboBoxFocus(symbolName);
}


//------------------------------------------------------------------------
// ComboBox ������ ��ġ�� �ٲ�� ȣ��ȴ�.
//------------------------------------------------------------------------
void CPropertiesWnd::OnComboBoxSelChange(UINT nID)
{
	if (VIEW != m_State)
		return;

	const int nItem = m_wndObjectCombo.GetCurSel();
	CString curText;
	m_wndObjectCombo.GetLBText(nItem , curText);
	UpdateProperty( curText );
}


//------------------------------------------------------------------------
// Clicked Refresh Button
//------------------------------------------------------------------------
void CPropertiesWnd::OnRefresh()
{
	m_wndPropList.Refresh();
	UpdateComboBox();
	global::PrintOutputWnd( "Property Refresh" );
}


//------------------------------------------------------------------------
//  ���� �ð����� ������ �����Ѵ�.
//------------------------------------------------------------------------
void CPropertiesWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (ID_TIMER == nIDEvent)
	{
		// ȭ�鿡 ������ �ʰų�, �巡�� ���϶��� ������������ �ʴ´�.
		if (IsDragMode() || !CWnd::IsWindowVisible())
			return;
		m_wndPropList.Refresh();
	}
	CDockablePane::OnTimer(nIDEvent);
}


//------------------------------------------------------------------------
// 
//------------------------------------------------------------------------
void CPropertiesWnd::OnDestroy()
{
	CDockablePane::OnDestroy();

 	KillTimer(ID_TIMER);
// 	BOOST_FOREACH(SPropItem *item, m_PropList)
// 	{
// 		SAFE_DELETE(item);
// 	}
// 	m_PropList.clear();
}


//------------------------------------------------------------------------
// 
//------------------------------------------------------------------------
void CPropertiesWnd::OnLButtonDown(UINT nFlags, CPoint point)
{

	CDockablePane::OnLButtonDown(nFlags, point);
}
