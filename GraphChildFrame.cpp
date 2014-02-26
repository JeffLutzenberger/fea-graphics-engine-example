// GraphChildFrame.cpp : implementation file
//
#include "stdafx.h"
#pragma hdrstop

#include "Main.h"
#include "MainFrm.h"
#include "GraphChildFrame.h"
#include "VAMain_Resource.h"
#include "VAStatus.h"
#include "GraphicView.h"
#include "MemberView.h"
#include "ReptView.h"
#include "ObjGridView.h"
#include "Report.h"
#include "PlateView.h"
#include "NodeView.h"
#include "VaDocument.h"
#include "Dialog/Dialogs.h"
#include "User/Message.h"    // Our Message IDs
#include "Control.h" // theController
#include "User/AutoDesign.h" // SetupAutoDesignGroups()
#include "User/CombinedMembers.h"
#include "Model.h"
#include "project.h"
#include "engineering\FEM Engine\Engine.h" // the Engine!

#include "ui/ProgressDlg.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CGraphChildFrame

IMPLEMENT_DYNCREATE(CGraphChildFrame, SECMDIChildWnd)

CGraphChildFrame::CGraphChildFrame()
{
}

CGraphChildFrame::~CGraphChildFrame()
{
}

void CGraphChildFrame::OnUpdateFrameTitle(BOOL /*bAddToTitle*/ )
{
	return;
}


void CGraphChildFrame::OnSize( UINT type, int cx, int cy )
{
	SECMDIChildWnd::OnSize( type, cx, cy );
	return;
}  // end COldGraphicView::OnSize

void CGraphChildFrame::OnNcLButtonDown( UINT u, CPoint p )
{
	CGraphicView* pG = dynamic_cast<CGraphicView*>(GetActiveView());
	if( pG ) pG->IncrementMouseSinceActivate();
	SECMDIChildWnd::OnNcLButtonDown( u, p );
}

int CGraphChildFrame::OnMouseActivate( CWnd* /*pDesktopWnd*/, UINT /*nHitTest*/, UINT /*message*/ )
{
	if( IsProgressDialogActive() ) 
		return MA_NOACTIVATEANDEAT;
	else
		return MA_ACTIVATE;

}

void CGraphChildFrame::OnClose() 
{
	// TeK Change 7/26/2007: Member Graphs can just close!
//	MDIDestroy();
	SECMDIChildWnd::OnClose();
}

BEGIN_MESSAGE_MAP(CGraphChildFrame, SECMDIChildWnd)
	//{{AFX_MSG_MAP(CGraphChildFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	  ON_WM_SIZE()
	  ON_WM_NCLBUTTONDOWN()
	  ON_WM_CLOSE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphChildFrame message handlers


/////////////////////////////////////////////////////////////////////////////
// CGraphicTabControl

IMPLEMENT_DYNCREATE(CGraphicTabControl, SECTabControl )

CGraphicTabControl::CGraphicTabControl()
{
	m_bChangingClient = false;
}

CGraphicTabControl::~CGraphicTabControl()
{
}

int CGraphicTabControl::TotalTabWidth(void) const {
	int w = 0;
		for( int i = 0; i < GetTabCount(); i++ ) {
			w += GetTab( i ).m_nWidth;
		}
	return w;
}

void CGraphicTabControl::ChangeTabClient( int iTab, CWnd* pClient )
{
	if( m_bChangingClient )
		return;
	ASSERT_RETURN( iTab >= 0 && iTab < GetTabCount() && pClient != NULL );

	if( TabExists( pClient ) ) {
		// make sure window is shown and update tab bar layout
		pClient->ShowWindow(SW_SHOW);
		RecalcLayout();
		UpdateWindow();
		return;  // nothing to do
	}

	SECTab* ptab = GetTabPtr(iTab);
	ASSERT_RETURN( ptab );
	CWnd* pOldClient = (CWnd*)ptab->m_pClient;
	// check to see if we really need to do anything
	if( pOldClient == pClient ) {
		pClient->ShowWindow( SW_SHOW );
		return;
	} 
	m_bChangingClient = true;
	// hide both windows
	pClient->ShowWindow(SW_HIDE);
	pOldClient->ShowWindow(SW_HIDE);
	CString tabName = ptab->m_csLabel;
	InsertTab( iTab, tabName, pClient, NULL, NULL );
	// remove old tab
	ActivateTab( iTab );
	DeleteTab( iTab + 1 );
	pClient->ShowWindow(SW_SHOW);
	RecalcLayout();
	UpdateWindow();
	m_bChangingClient = false;
	return;
}


BEGIN_MESSAGE_MAP(CGraphicTabControl, SECTabControl)
	//{{AFX_MSG_MAP(CGraphicTabControl)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP

END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGraphicTabWindow

IMPLEMENT_DYNCREATE(CGraphicTabWindow, SECTabWnd )

CGraphicTabWindow::CGraphicTabWindow()
{
	m_pMyTabCtrl = NULL;
}

CGraphicTabWindow::~CGraphicTabWindow()
{
}

BOOL CGraphicTabWindow::CreateTabCtrl(DWORD dwStyle, UINT nID)
{
	ASSERT_VALID(this);

	m_pMyTabCtrl = new CGraphicTabControl();

	m_pTabCtrl = dynamic_cast<SECTabControlBase*>(m_pMyTabCtrl);

	ASSERT_VALID(this);
	VERIFY(m_pTabCtrl->Create(WS_VISIBLE | m_dwTabCtrlStyle,
		CRect(0, 0, 0, 0), this, nID));

	dwStyle; // UNUSED
	return (m_pTabCtrl != NULL);
}

int CGraphicTabWindow::TotalTabWidth(void) const {
	if( m_pMyTabCtrl )
		return m_pMyTabCtrl->TotalTabWidth();
	else {
		//Happens on startup...ASSERT(FALSE);
		return 0;
	}
}

void CGraphicTabWindow::AdjustScrollBar( int cx ) {
	m_cxTabCtrl = TotalTabWidth();
	if( m_cxTabCtrl < 1 || cx < 1 )
		return;
	if( m_cxTabCtrl > (cx - 60) )
		m_cxTabCtrl = (cx - 60);
	RecalcLayout();
	return;
}

BOOL CGraphicTabWindow::ActivateTab( int iTab )
{
	return SECTabWnd::ActivateTab( iTab );
}

BOOL CGraphicTabWindow::ActivateTab(CWnd* pWnd, int nIndex)
{
    ASSERT_VALID(this);
    ASSERT_VALID(pWnd);
    if(!TabExists(pWnd))
		return FALSE;
	// jump around all the scroll bar junk which just causes ASSERTs and get to the meat of the matter
	return SECTabWndBase::ActivateTab(pWnd, nIndex);
}


BEGIN_MESSAGE_MAP(CGraphicTabWindow, SECTabWnd)
	//{{AFX_MSG_MAP(CGraphicTabWindow)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP

END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGraphicWindowChildFrame

IMPLEMENT_DYNCREATE(CGraphicWindowChildFrame, SECMDIChildWnd)

CGraphicWindowChildFrame::CGraphicWindowChildFrame() 
{
	m_pGraphicWindow = NULL;
	m_pReportWindow = new CReportView( *new CReport("Empty?", QUICK_REPORT ) );
	m_pGridWindow = new CObjGridView();
	m_pMemberWindow = new CMemberView();
	m_pMultiMemberWindow = new CMultiMemberView();
}

CGraphicWindowChildFrame::~CGraphicWindowChildFrame()
{
	// TeK Change 9/18/2009: Assume MFC deletes these, we just set to NULL (for lint)
	m_pGraphicWindow = NULL;
	m_pReportWindow = NULL;
	m_pGridWindow = NULL;
	m_pMemberWindow = NULL;
	m_pMultiMemberWindow = NULL;	
	m_tabWnd.DestroyWindow();
}

void CGraphicWindowChildFrame::OnUpdateFrameTitle(BOOL /*bAddToTitle*/ )
{
	SetIcon( theApp.LoadIcon( MAKEINTRESOURCE( IDI_MODEL )), TRUE );
	return;
}

BOOL CGraphicWindowChildFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext *pContext)															
{
	BOOL rtn_val = FALSE;
	rtn_val = m_tabWnd.Create(this, WS_CHILD | WS_VISIBLE | TWS_TABS_ON_BOTTOM | TWS_NOACTIVE_TAB_ENLARGED | TWS_DRAW_STUDIO_LIKE );
	// TeK Change 2/16/2007: Tab names must be different than menu names, also for VA55 customers we don't
	// want to change so much they get lost...
	SECTab* rtn = m_tabWnd.AddTab(RUNTIME_CLASS(CGraphicView), WindowTypeName[MODEL_WINDOW], pContext );
	// if the graphics window could not open, return false
	if( (NULL == rtn) || !rtn->m_pClient ) {
		return FALSE;
	}
	CWnd* pW; 
	BOOL b;
	LPCTSTR label;
	void* pExtra;
	// get the pointer to the window we just created
	m_tabWnd.GetTabInfo(0, label, b, pW, pExtra);
	m_pGraphicWindow = dynamic_cast<CGraphicView*>(pW);
	m_tabWnd.AddTab(pW, WindowTypeName[POST_WINDOW] );
	rtn = m_tabWnd.AddTab(pW, WindowTypeName[DESIGN_WINDOW] );
	// new in VA65 - report tab and member graph tab
	RECT r;
	GetClientRect( &r );
	m_pReportWindow->Create( NULL, "Report View", WS_CHILD | WS_VISIBLE, r, &m_tabWnd, 1001 ); 
	// put the report in the tab, might change to a grid later on
	m_tabWnd.AddTab( m_pReportWindow, WindowTypeName[RICH_TEXT_WINDOW] );
	m_pMemberWindow->Create( NULL, "Member Graphic View", WS_CHILD | WS_VISIBLE, r, &m_tabWnd, 1002 );
	// we'll put the single member view in the tab for now - later we might change
	m_tabWnd.AddTab(m_pMemberWindow, WindowTypeName[MEMBER_GRAPHIC_WINDOW] );
	m_pMultiMemberWindow->Create( NULL, "Multi Member Graphic View", WS_CHILD | WS_VISIBLE, r, &m_tabWnd, 1003 );
	m_pGridWindow->Create(NULL, "Grid View", WS_CHILD | WS_VISIBLE, r, &m_tabWnd, 1004 );

	int tabToUse = MODEL_WINDOW;
	if( m_pGraphicWindow ) {
		if( m_pGraphicWindow->isA() == MODEL_WINDOW ) {
			tabToUse = MODEL_WINDOW;
		} //  On creation we don't activate a result window unless we have results - causes analysis during reading in files
		else if( m_pGraphicWindow->isA() == POST_WINDOW && theProject.haveAnyResults() ) {
			tabToUse = POST_WINDOW;
		}
		else if( m_pGraphicWindow->isA() == DESIGN_WINDOW ) {
			tabToUse = DESIGN_WINDOW;
		}
		
	}
	m_tabWnd.ActivateTab(tabToUse);
	m_tabWnd.ScrollToTab(tabToUse);

	CFont font, selFont;
	int pointSize = 8;
	CDC* hDC;
	hDC = GetDC();
	int fontSize = -MulDiv( pointSize, hDC->GetDeviceCaps( LOGPIXELSY), 72);
	ReleaseDC( hDC );
	font.CreateFont( fontSize,
		0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH, "Arial" );
	m_tabWnd.SetFontSelectedTab( &font );
	m_tabWnd.SetFontUnselectedTab( &font );
	selFont.CreateFont( fontSize,
		0, 0, 0, FW_BLACK, true, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH, "Arial" );
	m_tabWnd.SetFontActiveTab( &selFont );

	return rtn_val;
}

void CGraphicWindowChildFrame::OnSize( UINT type, int cx, int cy )
{
	SECMDIChildWnd::OnSize( type, cx, cy );
	if( ::IsWindow(m_hWnd) == FALSE )
		return;

	// now make sure the horizontal scroll bar does not interfere with the tabs
	if( (cx > 0) ) {
		m_tabWnd.AdjustScrollBar( cx );
	}

	return;
}  // end CGraphicWindowChildFrame::OnSize

BOOL CGraphicWindowChildFrame::DestroyWindow()
{
	m_pReportWindow->DestroyWindow();
	m_pMemberWindow->DestroyWindow();
	m_pMultiMemberWindow->DestroyWindow();
	m_pGridWindow->DestroyWindow();
	return SECMDIChildWnd::DestroyWindow();

}

void CGraphicWindowChildFrame::OnNcLButtonDown( UINT u, CPoint p )
{
	CGraphicView* pG = dynamic_cast<CGraphicView*>(GetActiveView());
	if( pG ) pG->IncrementMouseSinceActivate();
	SECMDIChildWnd::OnNcLButtonDown( u, p );
}


int CGraphicWindowChildFrame::OnMouseActivate( CWnd* /*pDesktopWnd*/, UINT /*nHitTest*/, UINT /*message*/ )
{
	if( IsProgressDialogActive() ) 
		return MA_NOACTIVATEANDEAT;
	else
		return MA_ACTIVATE;

}

void CGraphicWindowChildFrame::OnClose() 
{
	CMainFrame* pMF = VAWindow();
	if( pMF && (pMF->CountGraphicViews() > 1) ) {
		MDIDestroy();
	}
	else {
		if( theDocument && theDocument->CanClose() ) {
			SECMDIChildWnd::OnClose();
		}
	} 
	return;
}

void CGraphicWindowChildFrame::moveBack()
{
	ASSERT_RETURN( backTabCount() > 1 );
	int currentTab = m_backTabList[backTabCount()];
	m_backTabList.remove(backTabCount());
	int previousTab =  m_backTabList[backTabCount()];
	m_backTabList.remove(backTabCount());
	SetTab( previousTab );
	m_forwardTabList.add( currentTab );
}

void CGraphicWindowChildFrame::moveForward()
{
	ASSERT_RETURN( forwardTabCount() > 0 );
	int newTab = m_forwardTabList[forwardTabCount()];
	m_forwardTabList.remove(forwardTabCount());
	SetTab( newTab );
}

LRESULT CGraphicWindowChildFrame::OnTabChange(WPARAM wParam, LPARAM /*lParam*/)
{
	// add this tab move to the back list
	int newTab = (int)wParam;
	int prevTab = -1;
	if( backTabCount() > 0 ) {
		prevTab = m_backTabList[backTabCount()];
	}
	if( prevTab != newTab )
		m_backTabList.add( newTab );

	switch( wParam ) {
		case 0:  // Model Window Selected
			ASSERT_RETURN_ITEM( m_pGraphicWindow, FALSE );
			m_pGraphicWindow->ShowWindow( SW_SHOW );
			m_pMemberWindow->ShowWindow( SW_HIDE );
			m_pMultiMemberWindow->ShowWindow( SW_HIDE );
			m_pReportWindow->ShowWindow( SW_HIDE );
			m_pGridWindow->ShowWindow( SW_HIDE );
			windowFilter().windowType = MODEL_WINDOW;
			VAHelpPane.updateActiveWindow( windowFilter().windowType );
			m_pGraphicWindow->SetAServiceCase();
			SetIcon( theApp.LoadIcon( MAKEINTRESOURCE( IDI_MODEL )), TRUE );
			break;
		case 1:  // Results Window Selected
			ASSERT_RETURN_ITEM( m_pGraphicWindow, FALSE );
			m_pGraphicWindow->ShowWindow( SW_SHOW );
			m_pMemberWindow->ShowWindow( SW_HIDE );
			m_pMultiMemberWindow->ShowWindow( SW_HIDE );
			m_pReportWindow->ShowWindow( SW_HIDE );
			m_pGridWindow->ShowWindow( SW_HIDE );
			windowFilter().windowType = POST_WINDOW;
			VAHelpPane.updateActiveWindow( windowFilter().windowType );
			SetIcon( theApp.LoadIcon( MAKEINTRESOURCE( IDI_RESULT )), TRUE );
			if( theProject.useBackgroundEngine() == false ) {
				if( !AnalyzeIfNoResults() ) {
					; // assume analysis engine has displayed a message on failure...
				}
			}
			m_pGraphicWindow->SetAResultCase( false );
			break;
		case 2:  // Design Window Selected
			ASSERT_RETURN_ITEM( m_pGraphicWindow, FALSE );
			m_pGraphicWindow->ShowWindow( SW_SHOW );
			m_pMemberWindow->ShowWindow( SW_HIDE );
			m_pMultiMemberWindow->ShowWindow( SW_HIDE );
			m_pReportWindow->ShowWindow( SW_HIDE );
			m_pGridWindow->ShowWindow( SW_HIDE );
			// TeK Add 4/26/2007: Only if we have something to design!
			if( theModel.nodes() < 2 ) {
				AfxMessageBox( "Please create a model, then you may work on design!", MB_ICONINFORMATION );
				windowFilter().windowType = MODEL_WINDOW;
				SetTab( 0 );
			}
			else if( theController.designSoftwareIsPresent() ) {
				// for speed improvement, lets stop the feaBacgroundProcessor, first see if it is working
				if( theEngine.FEABackgroundProcessorHasWorkToDo() ) {
						VAHelpPane.showTempTip( "The model has not been set up for analysis."
							" Analyze the project first!", true );
				}
				else {
					theEngine.stopSetupFEABackgroundProcessor();
					SetupAutoDesignGroups();
					theController.doAllUnityChecks();
					theEngine.startSetupFEABackgroundProcessor();
					windowFilter().windowType = DESIGN_WINDOW;
					VAHelpPane.updateActiveWindow( windowFilter().windowType );
					if( !theController.designCasesHaveValidResults() ) {
						VAHelpPane.showTempTip( "There are no 'design' load combinations with results."
							" Check design settings in Load Case Manager, or Analyze!", true );
					}
				}
				Invalidate();
				SetIcon( theApp.LoadIcon( MAKEINTRESOURCE( IDI_DESIGN )), TRUE );
			}
			else {
				AfxMessageBox( "You have not purchased the optional design features, please visit www.iesweb.com or call 1-800-707-0816.", 
					MB_ICONINFORMATION | MB_OK );
				SetTab(0);
				return LRESULT(0);
			}
			break;
		case 3:  // reportView or gridView
			SetReportView();
			break;
		case 4:  // memberView (or multi)
			{
			if( m_pGraphicWindow ) m_pGraphicWindow->ShowWindow( SW_HIDE );
			m_pMemberWindow->ShowWindow( SW_HIDE );
			m_pMultiMemberWindow->ShowWindow( SW_HIDE );
			m_pReportWindow->ShowWindow( SW_HIDE );
			m_pGridWindow->ShowWindow( SW_HIDE );

			// we've got to decide whether or not we show the single member view
			// or the multi member view
			// lets see if we've got a selected chain
			if( m_pGraphicWindow ) {
				m_pGraphicWindow->SetAResultCase( false );
				CLoadCase* aLoadCase = m_pGraphicWindow->windowFilter().loadCase();
				const CResultCase* aResultCase = m_pGraphicWindow->windowFilter().resultCase();
				 // see if we have a chain
				 int count = theModel.elements( MEMBER_ELEMENT, OnlySelected );
				 bool forceSingleMemberPlot = false;
				 CCombineMembers cm(true);
				 bool haveAChain = (OK_TO_COMBINE == cm.CanCombine()); //areSelectedMembersAChain( true );
				 if( count > 1 && haveAChain ) {
					 // make sure we don't have too many members
					int graphPoints = 0;
					//const TPointerArray<const CMember>& chain = theMemberChain();
					const CMemberArray& chain = cm.GetChain();
					if( aResultCase != NULL ) {
						for( int i = 1; i <= count; i++ ) {
							const CMember* pM = chain[i];
							const CResult* pMFR = aResultCase->result( *pM );
							if( pMFR )
								graphPoints += ((CResult*)pMFR)->locations();
						}
					}
					if( graphPoints >= MAX_GRAPH_DATA_COUNT || count+1 >= MAX_X_ANNOTATION_COUNT ) {
						MessageBox( "You have selected too many members. Please select fewer members for your multi-member plot.", "VisualAnalysis",
							MB_OK | MB_ICONEXCLAMATION );
						forceSingleMemberPlot = true;
					 }
					else {  // we can set up the multi member view
						m_pMultiMemberWindow->memberChain().flush();
						for( int i = 1; i <= chain.getItemsInContainer(); i++ )
							m_pMultiMemberWindow->memberChain().add( chain[i] );
						m_pMultiMemberWindow->windowFilter().windowType = MULTIMEMBER_GRAPHIC_WINDOW;
						SetupMemberPlotFilter( m_pMultiMemberWindow->windowFilter(), aLoadCase, aResultCase, NULL );
						m_tabWnd.GetTabCtrl()->ChangeTabClient( 4, m_pMultiMemberWindow );
					}
				 }

				 if( forceSingleMemberPlot || haveAChain == false || count <= 1 )  {  // create a single member view 
						CMember* pM = (CMember*)theModel.element( MEMBER_ELEMENT, 1, (count > 0) );
						m_pMemberWindow->windowFilter().windowType = MEMBER_GRAPHIC_WINDOW;
						SetupMemberPlotFilter( m_pMemberWindow->windowFilter(), aLoadCase, aResultCase, pM );
						m_tabWnd.GetTabCtrl()->ChangeTabClient( 4, m_pMemberWindow );
				 }
		   }  // m_pGraphicWindow
		   else {  // this should not happen
			   ASSERT( FALSE );
				m_pMemberWindow->ShowWindow( SW_HIDE );
				m_pMultiMemberWindow->ShowWindow( SW_HIDE );
				m_pReportWindow->ShowWindow( SW_HIDE );
			}
			}
			break;
		default:
			ASSERT( FALSE );
			break;
	}
	CWnd* theClient = AfxGetMainWnd();
	if( theClient ) {
		theClient->SendMessage( VIEW_MANAGER_CHANGE_MESSAGE, 0, 0 );
	}
	if( wParam == 3 ) {
		m_tabWnd.GetTabCtrl()->SetFocus();
	}

	// TeK Add 10/13/2009: Prevent the entire tab row from disappearing!
	//        UpdateWindows didn't work?, try ShowWindow!
	// RDV Note 10/1/09 - Add this to hopefully clear up the tab visibility issues reported
	//m_tabWnd.UpdateWindow();
//#pragma message( TEK "Tab windows still disappear, sometimes, for some customers..." )
	m_tabWnd.ShowWindow( SW_SHOW );

	return FALSE;
}


void DoMouseClick( int X, int Y)
{
//Call the imported function with the cursor's current position
mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, X, Y, 0, 0);
} 

void CGraphicWindowChildFrame::SetReportView()
{
	// TeK Change 9/18/2009, added some pointer checking
	// make sure view is set up for the proper type of report view
	if( m_pGraphicWindow ) {
		m_pGraphicWindow->ShowWindow( SW_HIDE );
	}
	if( m_pMemberWindow ) {
		m_pMemberWindow->ShowWindow( SW_HIDE );
	}
	if( m_pMultiMemberWindow ) {
		m_pMultiMemberWindow->ShowWindow( SW_HIDE );
	}
	if( m_pReportWindow ) {
		CReport* pReport = m_pReportWindow->report();
		CGraphicTabControl* pTab = m_tabWnd.GetTabCtrl();
		if( pReport && pTab ) {
			if( pReport->showAsGrid() ) {
				m_pReportWindow->ShowWindow( SW_HIDE );
				m_pGridWindow->setReport( pReport );
				pTab->ChangeTabClient( 3, m_pGridWindow );
			}
			else {
				m_pGridWindow->ShowWindow( SW_HIDE );
				pTab->ChangeTabClient( 3, m_pReportWindow );
			}
		}
	}
	return;
}

void CGraphicWindowChildFrame::SetupMemberPlotFilter( CWindowFilter& filter, const CLoadCase* aLoadCase, const CResultCase* aResultCase, CMember* pM )
{
	filter.loadCase( (CLoadCase*)aLoadCase );
	filter.graphMember( pM );
	filter.resultCase( aResultCase );
	return;
}


CWindowFilter& CGraphicWindowChildFrame::windowFilter()
{
	// RDV Note - this is not really safe but we'll do it anyway
	int tab = -1;
	m_tabWnd.GetActiveTab( tab );
	switch( tab )
	{
		case 0:
		case 1:
		case 2:	
			if( m_pGraphicWindow )
				return m_pGraphicWindow->windowFilter();
			else
				ASSERT( FALSE );
		case 3:  // grids use the report window's filter
			return m_pReportWindow->windowFilter();
		case 4:
		default:
			if( m_pMemberWindow->IsWindowVisible() )
				return m_pMemberWindow->windowFilter();
			else
				return m_pMultiMemberWindow->windowFilter();
	}
}


void CGraphicWindowChildFrame::SetTab( UINT tab ) {
	m_tabWnd.ScrollToTab(tab);
	m_tabWnd.ActivateTab(tab);
	return;
}


bool CGraphicWindowChildFrame::AnalyzeIfNoResults( void )
{
	if( theModel.nodes() < 2 && theModel.meshed_nodes() < 2 ) {
		AfxMessageBox( "Please create a model and loads, then you may view analysis results!", MB_ICONINFORMATION );
		windowFilter().windowType = MODEL_WINDOW;
		return false;
	}
	else if( !theProject.haveAnyResults() ) {
		// Attempt to analyze to get results...
		CMainFrame* pMain = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
		if( pMain ) {
			m_pGraphicWindow->ClearAnalysisMessages();
			m_pGraphicWindow->ShowAnalysisTextWindow( SW_SHOW );
			pMain->Analyze();
			m_pGraphicWindow->SetAResultCase( true );
			if( NULL == m_pGraphicWindow->CurrentResultCase() ) {
				// Assume that some error report will get shown...
				return false;
			}
		}
	}
	return true;  // good to go!
}


BEGIN_MESSAGE_MAP(CGraphicWindowChildFrame, SECMDIChildWnd)
	//{{AFX_MSG_MAP(CGraphicWindowChildFrame)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_WM_SIZE()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_CLOSE()
	ON_MESSAGE( TCM_TABSEL, OnTabChange )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphicWindowChildFrame message handlers