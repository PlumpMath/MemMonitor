
#include "stdafx.h"
#include "ViewTree.h"
#include "MainFrm.h"


#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CViewTree

CViewTree::CViewTree() : m_State(VIEW)
{
}

CViewTree::~CViewTree()
{
}

BEGIN_MESSAGE_MAP(CViewTree, CTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_SELCHANGING, &CViewTree::OnTvnSelchanging)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewTree �޽��� ó����

BOOL CViewTree::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	BOOL bRes = CTreeCtrl::OnNotify(wParam, lParam, pResult);

	NMHDR* pNMHDR = (NMHDR*)lParam;
	ASSERT(pNMHDR != NULL);

	if (pNMHDR && pNMHDR->code == TTN_SHOW && GetToolTips() != NULL)
	{
		GetToolTips()->SetWindowPos(&wndTop, -1, -1, -1, -1, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
	}

	return bRes;
}


//------------------------------------------------------------------------
// ���õ� ��尡 �ٲ� ���� ȣ���
//------------------------------------------------------------------------
void CViewTree::OnTvnSelchanging(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (VIEW != m_State) // View ������ ���� ó���Ѵ�.
		return;

	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	CString str = GetItemText(
		GetSymbolTreeItem(pNMTreeView->itemNew.hItem));
	CString strOld = GetItemText(
		GetSymbolTreeItem(pNMTreeView->itemOld.hItem)); // ������

	CMainFrame *pFrm = (CMainFrame*)::AfxGetMainWnd();
	pFrm->GetPropertyWnd().UpdateProperty( str );
	// ���� �ϳ� �̻��̶��, MainProperty�� ȭ�鿡 �������Ѵ�.
	pFrm->GetPropertyWnd().ShowPane(TRUE, TRUE, TRUE);

	*pResult = 0;
}


//------------------------------------------------------------------------
// �޸� Ʈ������ ���õ� hItem�� �޸� �ɺ����� ��Ʈ����
// ���� Tree ��带 �����Ѵ�.
//------------------------------------------------------------------------
HTREEITEM CViewTree::GetSymbolTreeItem(HTREEITEM hItem)
{
	HTREEITEM hTmp = hItem;
	while (hTmp)
	{
		HTREEITEM hParent = GetParentItem(hTmp );
		if (!hParent)
			break;
		else
			hTmp  = hParent;
	}
	return hTmp;
}


//------------------------------------------------------------------------
// ���õ� ����� Symbol Tree Item �� �����Ѵ�.
//------------------------------------------------------------------------
HTREEITEM CViewTree::GetSelectSymbolTreeItem()
{
	return GetSymbolTreeItem( GetSelectedItem() );	
}


//------------------------------------------------------------------------
// �����޸𸮿� �ִ� ������ �о ȭ�鿡 ǥ���Ѵ�.
//------------------------------------------------------------------------
void	CViewTree::UpdateMemoryTree()
{
	m_State = REFRESH;
	const std::string selectItemName = common::wstring2string(
		(LPCTSTR)GetItemText( GetSelectSymbolTreeItem() ) );

	DeleteAllItems();
	sharedmemory::MemoryList memList;
	sharedmemory::EnumerateMemoryInfo(memList);
	BOOST_FOREACH(sharedmemory::SMemoryInfo &info, memList)
	{
		const std::wstring wstr = common::string2wstring( info.name );
		const HTREEITEM hItem = InsertItem( wstr.c_str(), 0, 0, TVI_ROOT, TVI_SORT);

		InsertItem(common::formatw("size: %d", info.size).c_str(), hItem);
		InsertItem(common::formatw("ptr: 0x%x", (DWORD)info.ptr).c_str(),hItem);

		if (selectItemName == info.name)
			SelectItem(hItem);
	}

	m_State = VIEW;
}
