#if !defined(AFX_GRAPHCHILDFRAME_H__47376CBD_85E2_11D1_BE3B_0040332D77C8__INCLUDED_)
#define AFX_GRAPHCHILDFRAME_H__47376CBD_85E2_11D1_BE3B_0040332D77C8__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GraphChildFrame.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGraphChildFrame frame - Used for Member/Plate/Node Graphs
#include "GraphicView.h"
#include "ReptView.h"
#include "MemberView.h"

class CObjGridView;

class CGraphChildFrame : public SECMDIChildWnd
{
	DECLARE_DYNCREATE(CGraphChildFrame)
public:
	CGraphChildFrame();           // protected constructor used by dynamic creation
	virtual ~CGraphChildFrame();

// Attributes
public:
	void OnSize( UINT type, int cx, int cy );

// Operations
public:
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphChildFrame)
	//}}AFX_VIRTUAL
	void OnUpdateFrameTitle(BOOL bAddToTitle);
	void OnNcLButtonDown( UINT u, CPoint p );
	int OnMouseActivate( CWnd* pDesktopWnd, UINT nHitTest, UINT message );
	virtual CDocument* GetActiveDocument() { return NULL; }
	void OnClose( void );
// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGraphChildFrame)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


// Tab window class  - override the StingRay class so we can control scroll bar
// better

class CGraphicTabControl : public SECTabControl
{
	DECLARE_DYNCREATE(CGraphicTabControl)
public:
	CGraphicTabControl();           // protected constructor used by dynamic creation
	virtual ~CGraphicTabControl();

	// Attributes
public:
	int TotalTabWidth(void) const;
	void ChangeTabClient( int iTab, CWnd* pClient );
	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphicTabControl)
	//}}AFX_VIRTUAL
	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGraphicTabControl)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	bool	m_bChangingClient;
	DECLARE_MESSAGE_MAP()
};


class CGraphicTabWindow : public SECTabWnd
{
	DECLARE_DYNCREATE(CGraphicTabWindow)
public:
	CGraphicTabWindow();           // protected constructor used by dynamic creation
	virtual ~CGraphicTabWindow();

	// Attributes
public:
	int TotalTabWidth(void) const;
	void AdjustScrollBar( int cx );
	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphicTabWindow)
	//}}AFX_VIRTUAL
	// Implementation
	CGraphicTabControl * GetTabCtrl() { return m_pMyTabCtrl; };
	BOOL ActivateTab( CWnd* pClient, int iTab );
	BOOL ActivateTab( int iTab );

protected:
	BOOL CreateTabCtrl(DWORD dwStyle, UINT nID);

	// Generated message map functions
	//{{AFX_MSG(CGraphicTabWindow)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

private:
	CGraphicTabControl *m_pMyTabCtrl;

	DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////
// CGraphicWindowChildFrame frame - Used for main graphic window

class CGraphicWindowChildFrame : public SECMDIChildWnd
{
	DECLARE_DYNCREATE(CGraphicWindowChildFrame)
public:
	CGraphicWindowChildFrame();           // protected constructor used by dynamic creation
	virtual ~CGraphicWindowChildFrame();

	// Attributes
public:
	void OnSize( UINT type, int cx, int cy );

	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphChildFrame)
	//}}AFX_VIRTUAL
	void OnUpdateFrameTitle(BOOL bAddToTitle);
	void OnNcLButtonDown( UINT u, CPoint p );
	int OnMouseActivate( CWnd* pDesktopWnd, UINT nHitTest, UINT message );
	BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext *pContext);															
	virtual CDocument* GetActiveDocument() { return NULL; }
	void OnClose( void );
	virtual BOOL DestroyWindow();
	CGraphicTabWindow* tabWindow( void ) { return &m_tabWnd; };
	CReportView* reportView( void ) { return m_pReportWindow; };
	CObjGridView* gridView( void ) { return m_pGridWindow; };
	CWindowFilter& windowFilter();   // return the current filter being used in the graphic window
	// Implementation
	void SetTab( UINT tab );

	void SetReportView();  // make sure we have grid or text report depending on filter

	// back/forward tabbing
	int	backTabCount() { return m_backTabList.getItemsInContainer(); }; 
	int	forwardTabCount() { return m_forwardTabList.getItemsInContainer(); }; 
	void moveBack();
	void moveForward();

protected:

	afx_msg LRESULT OnTabChange(WPARAM wParam, LPARAM lParam);
	bool AnalyzeIfNoResults( void );

	void SetupMemberPlotFilter( CWindowFilter& filter, const CLoadCase* aLoadCase, 
		const CResultCase* aResultCase, CMember* pM );

	CGraphicTabWindow	m_tabWnd;
	CGraphicView* m_pGraphicWindow;

	// new in VA65
	CMemberView* m_pMemberWindow;
	CMultiMemberView* m_pMultiMemberWindow;
	CReportView* m_pReportWindow;
	CObjGridView* m_pGridWindow;

	CIntArray m_backTabList;
	CIntArray m_forwardTabList;

	// Generated message map functions
	//{{AFX_MSG(CGraphicWindowChildFrame)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHCHILDFRAME_H__47376CBD_85E2_11D1_BE3B_0040332D77C8__INCLUDED_)
