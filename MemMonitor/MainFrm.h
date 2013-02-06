
// MainFrm.h : CMainFrame Ŭ������ �������̽�
//

#pragma once
#include "FileView.h"
#include "MemoryView.h"
#include "OutputWnd.h"
#include "PropertiesWnd.h"

class CMainFrame : public CFrameWndEx
{
protected: // serialization������ ��������ϴ�.
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Ư���Դϴ�.
public:

// �۾��Դϴ�.
public:

// �������Դϴ�.
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, 
		CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);

// �����Դϴ�.
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // ��Ʈ�� ������ ���Ե� ����Դϴ�.
	CMFCMenuBar				m_wndMenuBar;
//	CMFCToolBar				m_wndToolBar;
//	CMFCStatusBar			m_wndStatusBar;
//	CFileView						m_wndFileView;
	CMFCToolBarImages	m_UserImages;
	CMemoryView				m_wndMemoryView;
	COutputWnd				m_wndOutput;
	CPropertiesWnd			m_wndProperties;		// main properties

	typedef std::list<CPropertiesWnd*>	PropertyList;
	typedef PropertyList::iterator PropertyItor;
	PropertyList					m_PropertyList;

public:
	bool							OpenScript( const std::string &openScriptFileName );

	bool							AddPropertyWnd( const CString &symbolTypeName,
																CRect rect=CRect(0, 0, 200, 200));

	void							RefreshPropertyWndComboBox();

	CPropertiesWnd&		GetPropertyWnd() { return m_wndProperties; }
	CMemoryView&		GetMemoryView() { return m_wndMemoryView; }
	COutputWnd&			GetOutputWnd() { return m_wndOutput; }	

// ������ �޽��� �� �Լ�
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnMemorytreeWindow();
	afx_msg void OnOutputWindow();
	afx_msg void OnPropertyWindow();
	afx_msg void OnClose();
};
