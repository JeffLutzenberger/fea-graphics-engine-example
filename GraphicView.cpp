// GraphicsWindow.cpp : implementation file


#include "stdafx.h"
#pragma hdrstop

#include "GraphicView.h"
#include "GraphChildFrame.h"
#include "Graphics/GraphicsHelpers.h"
#include "Graphics/GraphicsHelperClasses.h"
#include "Graphics/AVIGenerator.h"

#include "Servcase.h"
#include "User/Message.h"
#include "VAStatus.h" 

#include "project.h"
#include "model.h"
#include "inifont.h"
#include "IniSize.h"
#include "graphics/GridManager.h"

#include "User/Change.h"

#include "VAMain_Resource.h"

#include "WinFiltr.h"
#include "IniFilter.h"
#include "NamedFilter.h"

#include "Main.h"
#include "MainFrm.h"
#include "Design/GroupManager.h"
#include "Design/mesh.h"

#include "User/Control.h"   // CController

#include "engineering\FEM Engine\Engine.h" // the Engine!

#include "UI/ProgressDlg.h"

#include "Legacy/IESDlg.h"
#include "Dialog/VAHelpPane.h"
#include "Dialog/VADlg_Resource.h" // IDH_ constants neede for "What's This?" help

#include <gl/gl.h>
#include <gl/glu.h>
#include "wglext.h"		//WGL extensions
#include "Graphics/glext.h"		//GL extensions

extern CString CORE_LINK gGraphicsCardInfo; //defined in GraphicsHelpers.cpp
extern int CORE_LINK gOpenGLMajorVersion; //defined in GraphicsHelper.cpp

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

// CGraphicView

// this is the registered window class
#define CUSTOM_CLASSNAME _T("GL_WINDOW_CLASS")

int gGraphicWindowCount = 0;

//most cards now support Multisample antialiasing but we still need to check for it
bool gArbMultisampleSupported = false;
int gArbMultisampleFormat = 0;

IMPLEMENT_DYNCREATE(CGraphicView, CScrollView)

CGraphicView::CGraphicView() : 
CScrollView(),
m_bActive( false ),
m_bClosing( false ),
m_bPrinting( false ),
m_bInActivate( false ),
m_bFirstActivate( true ),
m_bForceDraw( false ),
m_bForceRotate( false ),
m_windowTitle( CString() ),
m_time( 0 ),
m_Filter( ),
//m_View( ),
m_MouseSinceActivate( 0 ),
m_MyTip( ),
// Our OpenGL window variables
m_hDC( ),
m_doubleBuffered( TRUE ),
m_hRC( ),
m_pDC( NULL ),
//m_pTextDC(NULL),
m_bHaveRenderingContext( false ),
//printing
//m_hOldDC( ),
m_hMemDC( ),
//m_hOldRC( ),
m_hMemRC( ),
m_bmi( ),
m_pBitmapBits( NULL ),
m_hDib( ),
m_szPage( CSize() ),
m_printScale( 1 ),
mGrid(), 
m_PopupMenu(),
m_mouseState( 0 ),  // initialize all to NULL
m_DraggingRect( CRect(0,0,0,0) ),
m_DragStart3D( CPoint3D() ),
m_DragEnd3D( CPoint3D() ),
m_DragStart2D( CPoint(0,0) ),
m_DragEnd2D( CPoint(0,0) ),
m_LastMouseButton( CPoint(0,0) ),
m_LastMouseMove( CPoint( 0, 0 ) ),
m_Current3DPt( CPoint3D() ),
m_CurSnapped2DPt( CPoint( 0, 0 ) ),
//m_pEditAnnotation( NULL ),
m_bHasBeenDrawnOnce( false ),
mMouseSinceActivate( 0 ),
mDrawGeneration( false ),
mCheckExtentChange( false ),
m_CurrentLayerBox( CAABB() ),
m_bDrawCurrentLayerBox ( false ),
mNodeBeingDragged( NULL ),
m_vertexBeingDragged( NULL ),
//results
m_pDraggingLegend( NULL ),
m_graphicsModel()
{
	// this constructor is called when we are creating a new graphic
	// view based on layout information
	// make this windows filter match what has been read in the main frame
	CMainFrame* pMain = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
	if( pMain ) {
		m_Filter = pMain->mFilter;
	// TeK Removed 9/30/2009
		//m_View = pMain->mViewData;
	}
	time( &m_time );
	m_windowTitle = "";
	m_Current3DPt = CPoint3D::undefined_point;
	//CreateExtrusions();

	//JL Added 2/15/2009 - query graphics card capabilities...
	glGetFloatv( GL_ALIASED_LINE_WIDTH_RANGE, &gMaxGLLineWidth );
}

CGraphicView::CGraphicView( LPSTR aTitle, CWindowFilter& theFilter)
: CScrollView(),
m_bActive( false ),
m_bClosing( false ),
m_bPrinting( false ),
m_bInActivate( false ),
m_bFirstActivate( true ),
m_bForceDraw( false ),
m_bForceRotate( false ),
m_windowTitle( CString() ),
m_time( 0 ),
m_Filter( theFilter ),
//m_View( theView ),
m_MouseSinceActivate( 0 ),
m_MyTip( ),
// Our OpenGL window variables
m_hDC( ),
m_doubleBuffered( TRUE ),
m_hRC( ),
m_pDC( NULL ),
//m_pTextDC( NULL ),
m_bHaveRenderingContext( false ),
//printing
//m_hOldDC( ),
m_hMemDC( ),
//m_hOldRC( ),
m_hMemRC( ),
m_bmi( ),
m_pBitmapBits( NULL ),
m_hDib( ),
m_szPage( CSize() ),
m_printScale( 1 ),
mGrid(), 
m_PopupMenu(),
m_mouseState( 0 ),  // initialize all to NULL
m_DraggingRect( CRect(0,0,0,0) ),
m_DragStart3D( CPoint3D() ),
m_DragEnd3D( CPoint3D() ),
m_DragStart2D( CPoint(0,0) ),
m_DragEnd2D( CPoint(0,0) ),
m_LastMouseButton( CPoint(0,0) ),
m_LastMouseMove( CPoint( 0, 0 ) ),
m_Current3DPt( CPoint3D() ),
m_CurSnapped2DPt( CPoint( 0, 0 ) ),
//m_pEditAnnotation( NULL ),
m_bHasBeenDrawnOnce( false ),
mMouseSinceActivate( 0 ),
mDrawGeneration( false ),
mCheckExtentChange( false ),
m_CurrentLayerBox( CAABB() ),
m_bDrawCurrentLayerBox ( false ),
mNodeBeingDragged( NULL ),
m_vertexBeingDragged( NULL ),
//results
m_pDraggingLegend( NULL ),
m_graphicsModel()
{
	time( &m_time );
	m_windowTitle = aTitle;
	m_Current3DPt = CPoint3D::undefined_point;
	//CreateExtrusions();

	//JL Added 2/15/2009 - query graphics card capabilities...
	glGetFloatv( GL_ALIASED_LINE_WIDTH_RANGE, &gMaxGLLineWidth );
}

CGraphicView::~CGraphicView()
{
	if( m_pDC )
		delete m_pDC;
	wglDeleteContext( m_hRC );
	gGraphicWindowCount--;
}


// CGraphicView diagnostics

#ifdef _DEBUG
void CGraphicView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CGraphicView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}
#endif //_DEBUG


// CGraphicView message handlers

BOOL CGraphicView::PreCreateWindow( CREATESTRUCT& cs )
{
	if( !CScrollView::PreCreateWindow( cs ) )
		return 0;

	// Register the window class if it has not already been registered.
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();
	// check if our class has been already registered (typical in MDI environment)
	if(!(::GetClassInfo(hInst, CUSTOM_CLASSNAME, &wndcls)))
	{
		// get default MFC class settings
		if(::GetClassInfo(hInst, cs.lpszClass, &wndcls))
		{
			// set our class name
			wndcls.lpszClassName = CUSTOM_CLASSNAME;
			// these styles are set for GL to work in MDI
			wndcls.style |= (CS_OWNDC | CS_HREDRAW | CS_VREDRAW /*| WS_CLIPCHILDREN | WS_CLIPSIBLINGS*/ );
			wndcls.hbrBackground = NULL;
			// try to register class (else throw exception)
			if (!AfxRegisterClass(&wndcls)) 
			{
				AfxThrowResourceException();
				return 0;
			}
		}
		// default MFC class not registered
		else 
		{
			AfxThrowResourceException();
			return 0;
		}
	}
	// set our class name in CREATESTRUCT
	cs.lpszClass = CUSTOM_CLASSNAME;
	// we're all set
	return 1;
}

BOOL CGraphicView::PreTranslateMessage( MSG* pMsg )
{
	CWnd* pw = GetFocus();
	if( this == pw ) {
		// RDV Note - we use up/down arrows for quick way to change load case combo so VK_CONTROL needed to delineate with rotation
		short s = GetKeyState(VK_CONTROL);
		if( s >= 0 ) {  // only do this for non control-up or down
		  switch (pMsg->message)
		  {
			 case WM_KEYUP:
				switch (pMsg->wParam)  // up/down arrows will cause the load case list to scroll
				{
				   case VK_UP:
					   CmUpArrowPress();
					   return TRUE;
				   case VK_DOWN:
					   CmDownArrowPress();
					  return TRUE;
				}
				break;
		  }
		}
	}

	m_MyTip.RelayEvent(pMsg);
	return CScrollView::PreTranslateMessage( pMsg );
}

void CGraphicView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	return;
}

// Initialize OpenGL when window is created.
int CGraphicView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;

	if ( !InitOpenGL( ) )
	{
		AfxMessageBox( "Error setting up OpenGL. Your graphics card may not support OpenGL. Please try updating your graphics card driver first." );
		return -1;
	}

	// Create the tool tip
	m_MyTip.Create(this);

	// Create a popup menu to be used with right mouse
	m_PopupMenu.CreatePopupMenu();

	// Setup the background processor and turn it on
	//m_backgroundEngine.startUpMultiThreadingEngine(this);
	//m_backgroundEngine.startBackgroundEngine();
	CAABB box;
	if( theModel.nodes() > 0 ){
		theModel.boundingBox( box );
		//m_Camera.FitTo( box );
	}

	m_nViewCountID = gGraphicWindowCount;
	gGraphicWindowCount++;

	SetWindowCaption();
	RECT r;
	GetClientRect( &r );
	m_analysisTextWindow.Create( ES_AUTOVSCROLL | ES_READONLY | ES_MULTILINE | WS_CHILD | WS_TABSTOP | WS_VSCROLL,  /*WS_VISIBLE | */
		CRect(r), this, 1 );
	
	return 0;
}

// Shutdown this view when window is destroyed.
void CGraphicView::OnDestroy()
{
	CScrollView::OnDestroy();

	wglMakeCurrent(0,0);
	wglDeleteContext(m_hRC);
	if( m_pDC )
	{
		delete m_pDC;
	}
	m_pDC = NULL;
}

//JL Note 10/5/2009 - moved to GraphicsHelper.cpp
//// OpenGL Setup
//void CGraphicView::QueryGraphicsCard( )
//{
//	int nameStackSize = -1;
//	glGetIntegerv( GL_NAME_STACK_DEPTH, &nameStackSize );
//	//TRACE( "Name stack size: %i\n",  nameStackSize );
//	gGraphicsCardInfo = "";
//	CString sVendor = (CString)glGetString( GL_VENDOR );
//	if( sVendor.GetLength() > 0 ) {
//		gGraphicsCardInfo = "Vendor: \r\n   ";
//		gGraphicsCardInfo += sVendor;
//		gGraphicsCardInfo += "\r\n\r\n";
//		gGraphicsCardInfo += "Graphics Card: \r\n   ";
//		gGraphicsCardInfo += (CString)glGetString( GL_RENDERER );
//		gGraphicsCardInfo += "\r\n\r\n";
//		gGraphicsCardInfo += "OpenGL Version: \r\n   ";
//		gGraphicsCardInfo += (CString)glGetString( GL_VERSION );
//		gGraphicsCardInfo += "\r\n\r\n";
//		gGraphicsCardInfo += "Supported Extensions: \r\n   ";
//		CString extensions = (CString)glGetString( GL_EXTENSIONS );
//		int str_pos = 0;
//		CString strToken = (CString)extensions.Tokenize( " ", str_pos );
//		while( strToken != "" )
//		{
//			gGraphicsCardInfo += strToken;
//			gGraphicsCardInfo += "\r\n   ";
//			strToken = (CString)extensions.Tokenize(" ", str_pos );
//		}
//	}
//	else {
//		gGraphicsCardInfo = "Could not detect graphics hardware?\r\n";
//		gGraphicsCardInfo += "  You may need to create or open a model first.\r\n";
//		gGraphicsCardInfo += "  You may need to update your graphics card driver.\r\n";
//	}
//}

bool CGraphicView::WGLisExtensionSupported(const char *extension)
{
	const size_t extlen = strlen(extension);
	const char *supported = NULL;

	// Try to use wglGetExtensionStringARB on current DC, if possible
	PROC wglGetExtString = wglGetProcAddress("wglGetExtensionsStringARB");

	HDC hDC = wglGetCurrentDC();
	
	if (wglGetExtString)
		supported = ((char*(__stdcall*)(HDC))wglGetExtString)(hDC);

	// if that failed, try standard opengl extensions string 
	if (supported == NULL)
		supported = (char*)glGetString(GL_EXTENSIONS);

	// if that fails then this card does not support it
	if (supported == NULL)
		return false;

	// we have extensions so search the string for the specified extension
	for (const char* p = supported; ; p++)
	{
		p = strstr(p, extension);

		if (p == NULL)
			return false;						// No Match

		// Make Sure That Match Is At The Start Of The String Or That
		// The Previous Char Is A Space, Or Else We Could Accidentally
		// Match "wglFunkywglExtension" With "wglExtension"

		// Also, Make Sure That The Following Character Is Space Or NULL
		// Or Else "wglExtensionTwo" Might Match "wglExtension"
		if ((p==supported || p[-1]==' ') && (p[extlen]=='\0' || p[extlen]==' '))
			return true;						// Match
	}
}

BOOL CGraphicView::SetupPixelFormat( )
{
	DWORD dwFlags =	PFD_DRAW_TO_WINDOW |    // support window
		PFD_SUPPORT_OPENGL |				// support OpenGL
		PFD_DOUBLEBUFFER |					// double buffered
		//PFD_SWAP_COPY |
		0x00008000;							//PFD_SUPPORT_COMPOSITION; 

	static PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),    // size of this pfd
		1,                                // version number
		dwFlags,						  // drawing flags
		PFD_TYPE_RGBA,                    // RGB type
		24,                               // 24-bit color depth, 32 for transparency
		0, 0, 0, 0, 0, 0,                 // color bits ignored
		0,                                // no alpha buffer
		0,                                // shift bit ignored
		0,                                // no accumulation buffer
		0, 0, 0, 0,                       // accumulation bits ignored
		16,                               // 16-bit z-buffer, 32 = more accurate but slower
		0,                                // no stencil buffer
		0,                                // no auxiliary buffer
		PFD_MAIN_PLANE,                   // main layer
		0,                                // reserved - allow 0 overlays 
		0, 0, 0                           // layer masks 
	};

	int nPixelFormat = 0;
	//for testing...
	//gArbMultisampleSupported = false;
	if( gArbMultisampleSupported && gArbMultisampleFormat > 0 ){
		TRACE( "Multisample Initialized!\n" );
		nPixelFormat = gArbMultisampleFormat;
	}else{
		TRACE( "Multisample Not Initialized!\n" );
		nPixelFormat = ::ChoosePixelFormat( m_pDC->GetSafeHdc(), &pfd );
	}

	if ( nPixelFormat == 0 )
		return FALSE;

	BOOL bPixelFormatOK = ::SetPixelFormat( m_pDC->GetSafeHdc(), nPixelFormat, &pfd );
	if( !bPixelFormatOK ){
		// the requested video mode is not available so get a default one (the first)
		int pixelformat = 1;	
		bPixelFormatOK = DescribePixelFormat(m_pDC->GetSafeHdc(), pixelformat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
		if( bPixelFormatOK ){
			bPixelFormatOK = SetPixelFormat(m_pDC->GetSafeHdc(), pixelformat, &pfd);
			TRACE( "Default video mode forced in GraphicView!" );
		}
	}
	return bPixelFormatOK;
}

BOOL CGraphicView::InitOpenGL( )
{
	//Get a DC for the Client Area
	m_pDC = new CClientDC(this);

	//Failure to Get DC
	if( m_pDC == NULL )
		return FALSE;

	if( !SetupPixelFormat() )
		return FALSE;

	//Create Rendering Context
	m_hRC = ::wglCreateContext( m_pDC->GetSafeHdc() );

	//Failure to Create Rendering Context
	if( m_hRC == 0 )
		return FALSE;

	//Make the RC Current
	if( ::wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC ) == FALSE )
		return FALSE;

	//check for antialiasing support after we have created a valid window
	CheckForMultisampleSupport();

	// Usual OpenGL stuff
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClearDepth(1.0f);
	glEnable( GL_DEPTH_TEST );
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	//JL Note 10/5/2009 - moved to GraphicsHelpers.cpp
	bool bOpenGLSupported = QueryGraphicsCard();
	if( !bOpenGLSupported )
	{
		ini.graphics().rubberBand = false;
		ini.graphics().lighting = false;
		//should already be set in CheckForMultisampleSupport() but just in case...
		gArbMultisampleSupported = false;
	}
	static bool bOpenGLWarn = !bOpenGLSupported;
	if( bOpenGLWarn )
	{
		//JL Note 10/5/2009 - we need to set a flag here to indicate that we should reduce the graphics
		// quality/features
		CString warningString = "We could not find a suitable graphics driver.";
		warningString += " Simply updating your graphics card driver may solve this problem.";
		warningString += " Otherwise, it might be time for a new graphics card.";
		warningString += " \r\n\r\nIn the meantime, VisualAnalysis will run in limited rendering mode.";
		warningString += " We will disable lighting and rubberbanding. These items can be";
		warningString += " re-enabled from the Main Menu under: Edit | Preferences | Graphics.";
		AfxMessageBox( warningString );
		bOpenGLWarn = false;
	}

	//JL note 10/1/2009 - setup the view volume based on the model size
	CAABB box;
	theModel.boundingBox( box );
	m_Camera.FitTo( box );

	return TRUE;
}

bool CGraphicView::CheckForMultisampleSupport( )
{  
	// see if the string exists in WGL
	if (!WGLisExtensionSupported("WGL_ARB_multisample"))
	{
		gArbMultisampleSupported=false;
		return false;
	}

	// get our pixel format
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
		(PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

	if (!wglChoosePixelFormatARB)
	{
		//no support
		gArbMultisampleSupported=false;
		return false;
	}

	// we need the current device context to query the opengl window for attributes
	HDC hDC = wglGetCurrentDC( );

	int pixelFormat;
	bool valid;
	UINT numFormats;
	float fAttributes[] = {0,0};

	// these are the attributes that are required for multisampling.
	// of particular interest are WGL_SAMPLE_BUFFERS_ARB and WGL_SAMPLES_ARB
	int iAttributes[] = { WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
		WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB,24,
		WGL_ALPHA_BITS_ARB,8,
		WGL_DEPTH_BITS_ARB,16,
		WGL_STENCIL_BITS_ARB,0,
		WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
		WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
		WGL_SAMPLES_ARB, 4 ,						// Check For 4x Multisampling
		0,0};

	// see if we can get a valid pixel format
	valid = (wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&pixelFormat,&numFormats) != NULL);
 
	if (valid && numFormats >= 1)
	{
		gArbMultisampleSupported	= true;
		gArbMultisampleFormat	= pixelFormat;	
		return true;
	}

	// pixel format failed, try for 2x multisampling
	iAttributes[19] = 2;
	valid = (wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&pixelFormat,&numFormats) != NULL);
	if (valid && numFormats >= 1)
	{
		gArbMultisampleSupported	= true;
		gArbMultisampleFormat	= pixelFormat;	 
		return true;
	}
	  
	// 2x failed...
	return  false;
}

void CGraphicView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	CScrollView::OnActivateView(bActivate, pActivateView, pDeactiveView);
	//if this is the first time and multisample is supported we need to kill the window
	//and recreate it with multisample enabled

	if( !::IsWindow( m_hWnd ) )
		return;

	// TeK Change 6/11/2008: WER Crash report, perhaps MF is "gone"?
	CMainFrame* pMF = (CMainFrame*)AfxGetMainWnd();
	if( bActivate && pActivateView == this /*&& !m_bActive*/ ) {
		activate();
		// 6/11/2008: was VAWindow()->SendMes...
		CHECK_IF( pMF && ::IsWindow( pMF->GetSafeHwnd() ) ) {
			pMF->SendMessage( ACTIVATED_GRAPHIC_VIEW, 0, (LPARAM)&m_Filter );
		}
	}
	else if( !bActivate && (pDeactiveView == this) && 
		pMF && pMF->IsWindowEnabled() && pActivateView != this ) {
		deactivate();
	}

	return;
}

// VA Messages sent through theController to MainFrame then to this window
void CGraphicView::OnUpdate( CView* pSender, LPARAM l, CObject* pObject )
{
	if( pSender != this ) {
		if( dynamic_cast<CVAMessage*>(pObject) && (CHANGE_MESSAGE == l) ) {
			CVAMessage* msg = (CVAMessage*)pObject; 
			ProjectChange( msg->wp, msg->lp );
		}
		else if( SELCHANGE_MESSAGE == l ) {
			Invalidate();
		}
		else CScrollView::OnUpdate( pSender, l, pObject );
	}
	return;
}

// IES Messages
LRESULT CGraphicView::ProjectChange( WPARAM change, LPARAM /*pData*/ )
{
	//when we get a group change message we will recompute every thing that is selected
	switch( ETChange( change ) )
	{
	case CHANGE_MODEL_ADD:
		{
			CCoordinate maxC, minC;
			maxC = theModel.maximumCoordinate();
			minC = theModel.minimumCoordinate();
#pragma message( TEK "This grid-manager code does not belong to CGraphicWindow, perhap mainframe should do this?" )
			// see if any of the grids need resized
			for( int iGrid = 1; iGrid <= theGridManager.gridCount(); iGrid++ )
			{
				if( theGridManager.grid( iGrid )->isExpandable() ) {
					theGridManager.grid( iGrid )->expandExtents( minC, maxC, 1.25, 2 );  // 25% beyond the current extents and don't exceed beyond two grid lines
				}
			}
		}
		break;
	case CHANGE_MODEL_MODIFY:
	{
		// undo does not send any data and it also does not necessarily update member UI times so we have to 
		// just nuke all graphics extrusions in this case
		//if( !pData )
		m_AnnotationManager.verifyAllAnnotationObjects();
		break;
	}
	case CHANGE_MODEL_REMOVE:
		// see if we've got any annotations which are attached to this object
		m_AnnotationManager.verifyAllAnnotationObjects();
		break;
	case CHANGE_SERVICELOAD_ADD:
	case CHANGE_SERVICELOAD_MODIFY:
	case CHANGE_SERVICELOAD_REMOVE:
	case CHANGE_INI_SETTINGS:
		break;
	case CHANGE_LOADCASE_ADD:
	case CHANGE_LOADCASE_REMOVE:
	case CHANGE_LOADCASE_MODIFY:
		SetAServiceCase();
		break;

	case CHANGE_NEW_ANALYSIS:
		if( m_Filter.loadCase() && (m_Filter.resultCase() == NULL) && theProject.haveAnyResults() ) {
				SetAResultCase( false );
				if( !theEngine.backgroundProcessing() ) {
#pragma message( TEK "This code does not belong here, GraphicView shouldn't know or care about the frame..." )
					// make sure active window is a post window
					if( m_Filter.windowType != POST_WINDOW && m_Filter.windowType != DESIGN_WINDOW ) {
						CGraphicWindowChildFrame* parent = dynamic_cast<CGraphicWindowChildFrame*>(GetParent()->GetParent()) ;
						if( parent )
							parent->SetTab( 1 );
					}
				}
		}
		break;

	case CHANGE_DESTROY_RESULTS:
		// check this windows result case, we may need to update
		if( m_Filter.loadCase() ) {
			if( !m_Filter.loadCase()->areAnyResultsValid() ) {
				m_Filter.resultCase( NULL );
			}
		}
		// switch back to a model window if the engine is now working
		if( m_Filter.windowType == POST_WINDOW && !theEngine.engineIsActive() && !theEngine.backgroundProcessing()  ) {
			CGraphicWindowChildFrame* parent = dynamic_cast<CGraphicWindowChildFrame*>(GetParent()->GetParent()) ;
			if( parent )
				parent->SetTab( 0 );
		}
		break;
	default:
		break;  // ignore other changes

	}	// end switch( reason for change )
	mCheckExtentChange = false;	// reset
	Invalidate();
	return (LRESULT)0;
}  // end ProjectChange

void CGraphicView::SetWindowTitle() {
	SetWindowCaption();
	ViewManagerChangeResults();
	return;
}

void CGraphicView::SetWindowCaption()
{
	ETWindowType wt = m_Filter.windowType;
	m_windowTitle = WindowTypeName[wt];
	switch( wt ) {
	case MODEL_WINDOW:
		if( m_Filter.loadCase() ) {
			m_windowTitle += ": ";
			m_windowTitle += m_Filter.loadCase()->name();
		}
		break;
	case POST_WINDOW:
	case MEMBER_GRAPHIC_WINDOW:
	case MULTIMEMBER_GRAPHIC_WINDOW:
		if( m_Filter.resultCase() ) {
			m_windowTitle += ": ";
			m_windowTitle += m_Filter.resultCase()->name();
		}
		break;
	}
	if( m_nViewCountID > 0 )  {
		CString sN;
		sN.Format( "(%d)", m_nViewCountID );
		m_windowTitle += sN;
	}
	CWnd* pPW = GetParent();
	CHECK_IF( pPW ) {
		CWnd* pPPW = pPW->GetParent();
		CHECK_IF( pPPW ) {
			pPPW->SetWindowText( m_windowTitle );
		}
	}

	// refresh MDI Window menu - so that window list displays correct window title
	VAWindow()->SendMessage( WM_MDIREFRESHMENU, 0, 0);
	return;
}	// end COldGraphicView::SetWindowCaption

extern char BUILD_NUMBER_STR[10]; // main.cpp

void CGraphicView::AddTitle( ) 
{
	// place title information in the view
	if( !m_Filter.windowTitles && !theProject.isDemo() )
		return;
			
	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	m_TitleText.ClearText();
	m_TitleText.SetControlPoint( m_Filter.titleOffset );

	CString sProject = SmartProjectTitle();

	m_TitleText.AddLine( sProject );

	// Company & Engineer (shows free-trial stuff too)
	CString sLicensee;
	sLicensee.Format( "%s, %s", theProject.company(), theProject.engineer() );
	m_TitleText.AddLine( sLicensee );

	// Billing Information (if present)
	if( lstrlen( theProject.billing() ) > 0 ) {
		m_TitleText.AddLine( theProject.billing() );
	}

	// Date & Time
	CTime t = CTime::GetCurrentTime();
	m_TitleText.AddLine( t.Format("%b %d, %Y; %I:%M %p") );

	// Load Case or Result Case Name 
	// place load case title
	CString sWindow;
	CString sResults;
	switch( m_Filter.windowType ) {
	case MODEL_WINDOW:
		if( m_Filter.loadCase() && m_Filter.loads ) {
			sWindow = "Load Case: ";
			sWindow += m_Filter.loadCase()->name();
		}
		break;
	case POST_WINDOW:
		if( m_Filter.resultCase() ) {
			sWindow = "Result Case: ";
			sWindow += m_Filter.resultCase()->name();

			// TeK Add 6/23/2008: Show filtered result type(s) too
			if( (MEMBER_NO_RESULT != m_Filter.member.resultType) &&
				m_Filter.members && (theModel.elements(MEMBER_ELEMENT) > 0) ) {
				sResults = "Member ";
				sResults += MemberResultsLongName[m_Filter.member.resultType];
			}
			if( (PLATE_NO_RESULT != m_Filter.plate.resultType) &&
				m_Filter.plates && (theModel.elements(PLANAR_ELEMENT) > 0) ) {
				if( sResults.GetLength() > 0 ) sResults += "; ";
				sResults += "Plate ";
				sResults += PlateResultsLongName[m_Filter.plate.resultType];
			}
			if( (CABLE_NO_RESULT != m_Filter.cable.resultType) &&
				m_Filter.cables && (theModel.elements(CABLE_ELEMENT) > 0) ) {
				if( sResults.GetLength() > 0 ) sResults += "; ";
				sResults += "Cable ";
				sResults += PlateResultsLongName[m_Filter.cable.resultType];
			}
		}
		break;
	case DESIGN_WINDOW:
		sWindow = "Design View, Unity Checks";
		break;
	default:
		// nothing!
		break;
	}
	if( sWindow.GetLength() > 0 ) m_TitleText.AddLine( sWindow );
	if( sResults.GetLength() > 0 ) m_TitleText.AddLine( sResults );

	// VisualAnalysis Full Build Version
	CString sVersion;
	sVersion.Format( "IES VisualAnalysis %s", BUILD_NUMBER_STR );
	m_TitleText.AddLine( sVersion );


	RECT r;
	r.left = 0;
	r.top = 0;
	r.bottom = viewport[3];
	r.right = viewport[2];

	m_TitleText.UpdateBoundingBox( true, m_pDC/*m_pTextDC*/, 
		ini.font().graphWindow.size, 
		ini.font().graphWindow.name );
	m_TitleText.DrawText2D( m_pDC /*m_pTextDC*/, ini.font().graphWindow.name, 
					ini.font().graphWindow.size/m_printScale, 
					ini.color().graphTitles, m_TitleText.ControlPoint(), 0., r  ); 

	return;
}


void CGraphicView::SaveWindowInformation( obfstream& out ) {
	if( FILE_SUCCESS != m_Filter.write( out ) ) {
		ASSERT(FALSE);
	}
	//if( FILE_SUCCESS != m_View.write( out ) ) {
	//	ASSERT(FALSE);
	//}
	return;
} // end SaveWindowInformation()


void CGraphicView::WriteAnnotations( obfstream& out )
{
	m_AnnotationManager.write( out );
	return;
}

void CGraphicView::ReadAnnotations( ibfstream& in )
{
	m_AnnotationManager.read( in, this );
	return;
}

void CGraphicView::WriteCoordinatesToStatusBar( const CPoint3D& p )
{
	//JL Note 9/30/2009 - remove m_View
	if( pStatbar && m_bActive )
		pStatbar->SetCoordinates( p.x, p.y, p.z );
	return;

}

// Sizing & Miscellaneous

void CGraphicView::OnSize( UINT type, int cx, int cy )
{
	CScrollView::OnSize( type, cx, cy );

	if( cx <= 0 || cy <= 0 || type == SIZE_MINIMIZED )
	{
		return;
	}
	// set up the scroll bar sizes - here is where we want to hide the scroll bars if not zoomed
	// in to were they are needed - to hide them just set s.cx and s.cy = 0
	SIZE s;
	s.cx = cx;
	s.cy = cy;
	SetScrollSizes(MM_TEXT, s);

	wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC );

	m_Camera.Resize( cx, cy );

	Invalidate();

	// TeK Commented 9/18/2009: Is this REALLY neccessary??
	//if( (SIZE_RESTORED == type) || (SIZE_MAXIMIZED == type) ) {
	//	VAWindow()->SendMessage( ACTIVATED_GRAPHIC_VIEW, 0, (LPARAM)&m_Filter );
	//}

	return;
}

void CGraphicView::OnVScroll( unsigned int nSBCode, unsigned int nPos, CScrollBar* pScrollBar )
{
	CScrollView::OnVScroll( nSBCode, nPos, pScrollBar );
	return;
}

void CGraphicView::OnHScroll( unsigned int nSBCode, unsigned int nPos, CScrollBar* pScrollBar )
{
	CScrollView::OnHScroll( nSBCode, nPos, pScrollBar );
	return;
}

BOOL CGraphicView::OnEraseBkgnd( CDC* pDC )
{
	return CScrollView::OnEraseBkgnd( pDC );
}

void CGraphicView::OnPrepareDC( CDC* pDC, CPrintInfo* pInfo  )
{
	CScrollView::OnPrepareDC( pDC, pInfo );
	return;
}

void CGraphicView::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	// if tip is up, shut it down
	if( ::IsWindow( m_MyTip ) )
		m_MyTip.Hide();
	switch( nChar )
	{
	case VK_ESCAPE:
		//break out of any drawing mode
		Mouse( MOUSE_NO_ACTION );
		break;
	case VK_CONTROL:
	case VK_SHIFT:
		if( Mouse( MOUSE_MODEL_DRAGGING ) )
			SetCursor( theApp.LoadCursor( ROTATE_CURSOR ) );
	}
	CScrollView::OnKeyDown( nChar, nRepCnt, nFlags );
	return;
}

void CGraphicView::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	switch( nChar )
	{
	case VK_ESCAPE:
		//break out of any drawing mode
		Mouse( MOUSE_NO_ACTION );
		break;
	case VK_CONTROL:
	case VK_SHIFT:
		if( Mouse( MOUSE_MODEL_DRAGGING ) )
			SetCursor( theApp.LoadCursor( PAN_CURSOR ) );
	}
	CScrollView::OnKeyUp( nChar, nRepCnt, nFlags );
	return;
}
/**********************************************************
* Function Name: ViewManagerChange( )
* 
* Description: Respond to filter changes.
*
***********************************************************/
void CGraphicView::ViewManagerChangeResults() {
	if( CurrentResultCase() && m_Filter.windowType == POST_WINDOW ) {
		m_graphicsModel.SetupResults( m_Filter );
	}
	Invalidate( );
}


// Somebody wants me to change my filter...
void CGraphicView::SetWindowFilter( const CWindowFilter& _f, int flags ) {
	m_Filter.copyWindowFilter( _f, flags );
	if( MODEL_WINDOW == m_Filter.windowType ) {
		SetAServiceCase();
	}
	else if( POST_WINDOW == m_Filter.windowType ) {
		SetAResultCase( true ); // make sure we have a good pointer!
		//m_Filter.SetInternalScaleFactor();
		m_graphicsModel.SetupResults( m_Filter );
	}
	if( CWindowFilter::COPY_ROTATION & flags ) {
		Camera().SetPositionRotationAndScale( m_Filter.camPos, m_Filter.camRotation, m_Filter.camViewVolume, m_Filter.camZoomScale );
	} 
	else {
		ZoomToExtents(false);
	}
	SetWindowCaption();
	Invalidate();
}


//TViewingData& CGraphicView::view()
//{
//	//get the camera settings...
//	return m_View;
//}

//void CGraphicView::GetWorldSize( double * xmax, double * xmin, double * ymax,
//								double * ymin, double * zmax, double * zmin )
//{
//	// determine the extreme "world" values displayed on the present view
//	CRect rc;
//	GetClientRect( rc );
//	POINT p;
//	p.x = rc.left;
//	p.y = rc.top;
//	*xmax = *ymax = *zmax = -1.E100;
//	*xmin = *ymin = *zmin = 1.E100;
//	CPoint3D w;
//	GetSnapped3DPoint( p, w );
//	*xmax = max( *xmax, w.x );	*ymax = max( *ymax, w.y );	*zmax = max( *zmax, w.z );
//	*xmin = min( *xmin, w.x );	*ymin = min( *ymin, w.y );	*zmin = min( *zmin, w.z );
//	p.x = rc.left;
//	p.y = rc.bottom;
//	GetSnapped3DPoint( p, w );
//	*xmax = max( *xmax, w.x );	*ymax = max( *ymax, w.y );	*zmax = max( *zmax, w.z );
//	*xmin = min( *xmin, w.x );	*ymin = min( *ymin, w.y );	*zmin = min( *zmin, w.z );
//	p.x = rc.right;
//	p.y = rc.bottom;
//	GetSnapped3DPoint( p, w );
//	*xmax = max( *xmax, w.x );	*ymax = max( *ymax, w.y );	*zmax = max( *zmax, w.z );
//	*xmin = min( *xmin, w.x );	*ymin = min( *ymin, w.y );	*zmin = min( *zmin, w.z );
//	p.x = rc.right;
//	p.y = rc.top;
//	GetSnapped3DPoint( p, w );
//	*xmax = max( *xmax, w.x );	*ymax = max( *ymax, w.y );	*zmax = max( *zmax, w.z );
//	*xmin = min( *xmin, w.x );	*ymin = min( *ymin, w.y );	*zmin = min( *zmin, w.z );
//	return;
//}	// end GetWorldSize

void CGraphicView::EscapeKey()
{
	// Escape key was pressed - handle appropriately
	// Right now we just check to see if drawing an element and if so, we get out
	// gracefully
	m_bForceRotate = false;
	if( Mouse( MOUSE_PENCIL_DRAGGING ) && 
		( m_Filter.drawMode != DRAW_NOTHING ) )
	{
		EndInvertedPolygon( MOUSE_PENCIL_DRAGGING );
		EndInvertedLine( MOUSE_PENCIL_DRAGGING );
		ReleaseMouse( MOUSE_PENCIL_DRAGGING );
		Invalidate();		// force a repaint of entire window
	}

	if( Mouse( MOUSE_ZOOMBOX_DRAGGING ) ) {  // cancel zoom box
		EndInvertedRectangle( );
		ReleaseMouse( MOUSE_ZOOMBOX_DRAGGING );
	}

	if( Mouse( MOUSE_MODEL_DRAGGING ) )
		ReleaseMouse( MOUSE_MODEL_DRAGGING );

	SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );  //restore cursor
	ReleaseCapture();
	return;
}	// end	CModelView::EscapeKey()


bool CGraphicView::cursorInWindow( CPoint point, bool isClientPoint ) {
	RECT r;
	if( isClientPoint )
		GetClientRect( &r );
	else
		GetWindowRect( &r );
	if( point.x < r.left || point.y < r.top || point.x > r.right || point.y > r.bottom )
		return false;
	return true;
}

void CGraphicView::ZoomView()
{
	// here we temporarily set the cursor over the center of this window
	// then call the setcursor function, then move the cursor back to its orig. position
	//CRect r;
	//GetWindowRect( r );
	SetActiveWindow();
	SetMouse( MOUSE_ZOOMBOX_WAITING );
	SetCapture();  // make sure this window gets ALL windows mouse messages
	SetCursor( theApp.LoadCursor( "ZOOMBOX_CURSOR"  ) );
	return;
}	// end ZoomView

LRESULT CGraphicView::OnHelpHitTest(WPARAM, LPARAM)
{
	switch( m_Filter.windowType ) {
	case MODEL_WINDOW:
		gContextHelpID = IDH_WINDOW_MODELEDIT;
		break;
	case POST_WINDOW:
		gContextHelpID = IDH_WINDOW_RESULT;
		break;
	case DESIGN_WINDOW:
		gContextHelpID = IDH_WINDOW_DESIGN;
		break;
	}	 
	return gContextHelpID;
}

void CGraphicView::activate() {
	VAHelpPane.forceActiveWindow( m_Filter.windowType );
	//JL Note 9/30/2009 - removed m_View
	//if( m_View.ViewScale == 0. ) {
	//	m_View.SetViewingMatrix();
	//}
	//else
	//	m_View.SetViewingMatrix();   // update the viewing matrix
	m_bInActivate = true;
	m_bFirstActivate = false;
	m_bInActivate = false;
	m_bActive = true;
	m_bForceDraw = false;
	m_MouseSinceActivate = 0;
	time( &m_time );
	// set active rendering context as this window's context
	ReleaseMouse( MOUSE_ZOOMBOX_WAITING );
	ReleaseMouse( MOUSE_ZOOMBOX_DRAGGING );
	ReleaseMouse( MOUSE_SELECTIONBOX_WAITING );
	ReleaseMouse( MOUSE_SELECTIONBOX_DRAGGING );
	ReleaseMouse( MOUSE_PENCIL_WAITING );
	ReleaseMouse( MOUSE_PENCIL_DRAGGING );
	Invalidate();
	return;
}

void CGraphicView::deactivate()
{
	if( ::IsWindow( m_MyTip.m_hWnd ) )
		m_MyTip.Hide();
	m_bActive = false;
	ReleaseMouse( MOUSE_ZOOMBOX_WAITING );
	ReleaseMouse( MOUSE_ZOOMBOX_DRAGGING );
	ReleaseMouse( MOUSE_SELECTIONBOX_WAITING );
	ReleaseMouse( MOUSE_SELECTIONBOX_DRAGGING );
	ReleaseMouse( MOUSE_PENCIL_WAITING );
	ReleaseMouse( MOUSE_PENCIL_DRAGGING );
	return;
}

//void CGraphicView::FullView() {
//	// respond to popup full view
//	//JL Note 9/30/2009 - removed m_View
//	//m_View.SetViewingMatrix();
//	ZoomToExtents();		// set the properties
//	Invalidate();		// for now force a repaint of entire window
//	return;
//}	// end COldGraphicView::FullView()

void CGraphicView::GetRotation( double rotMatrix[][4] )
{
	double matrix[16] = {0.};
	glGetDoublev( GL_MODELVIEW_MATRIX, matrix );
	int count = 0;
	for( int i = 0; i < 4; i++ )
		for( int j = 0; j < 4; j++ )
			rotMatrix[i][j] = matrix[count++];
}

void CGraphicView::SetRotation( double rotMatrix[][4] )
{
	float matrix[16];
	int count = 0;
	for( int i = 0; i < 4; i++ )
		for( int j = 0; j < 4; j++ )
			matrix[count++] = (float)rotMatrix[i][j];
	glLoadMatrixf( matrix );
}

void CGraphicView::Rotate( )
{
	CPoint3D eulers( m_Filter.rotation[0], m_Filter.rotation[1], m_Filter.rotation[2] );
	m_Camera.Rotate( eulers );
	Invalidate();
}

void CGraphicView::RotateX( double inc )
{
	m_Camera.RotateX( inc );
	Invalidate();
}

void CGraphicView::RotateY( double inc )
{
	m_Camera.RotateY( inc );
	Invalidate();
}

void CGraphicView::RotateZ( double inc )
{
	m_Camera.RotateZ( inc );
	Invalidate();
}

CPoint3D CGraphicView::GetRotationAngles()
{
	CPoint3D pt3D = m_Camera.GetRotations();
	return pt3D;
}

void CGraphicView::ZoomDisplayToRectangle( CRect r )
{
	m_Camera.ZoomArea( r );
	Invalidate();
	return;

}

void CGraphicView::ZoomToExtents( bool bSelected )
{
// scan list of what's visible and get extreme coordinates, then set up camera to show
// so that everything is displayed

	CAABB box;
	theModel.boundingBox( box, bSelected );
	//new camera //
	//JL note 10/1/2009 - try to fit the model a little better
	//m_Camera.FitTo( box );
	m_Camera.ZoomAll( box );
	Invalidate();

	return;
}


void CGraphicView::ViewZoomInAtPoint( CPoint pt ) 
{
	double scale = ini.filter().zoomFactor;
	if( scale > 0.) 
	{
		m_Camera.Zoom( 1/scale, pt ); 
		SetCapture();
		Invalidate();
		//ViewZoomAtPoint( pt, scale );
	}
	return;
}


void CGraphicView::ViewZoomOutAtPoint( CPoint pt ) 
{
	double scale = ini.filter().zoomFactor;
	if( scale > 0.) 
	{
		m_Camera.Zoom( scale, pt );
		SetCapture();
		Invalidate();
		//ViewZoomAtPoint( pt, -scale );
	}
	return;
}

void CGraphicView::ViewZoomAtPoint( CPoint /*pt*/, double /*scale*/ )
{
	HGLRC currentRC = m_hRC;
	if( !currentRC && !wglMakeCurrent( m_hDC,this->m_hRC ) )
	{
		currentRC = wglGetCurrentContext();
		if( !currentRC && !wglMakeCurrent( m_hDC,this->m_hRC ) )
			ASSERT(false);
	}
	Invalidate();	 // now force a repaint of entire window
}

void CGraphicView::currentDrawMode( ETDrawMode what )
{ 
	m_Filter.drawMode = what; 
	// reset what we are doing
	ReleaseMouse( MOUSE_ZOOMBOX_WAITING );
	ReleaseMouse( MOUSE_ZOOMBOX_DRAGGING );
	ReleaseMouse( MOUSE_SELECTIONBOX_WAITING );
	ReleaseMouse( MOUSE_SELECTIONBOX_DRAGGING );
	ReleaseMouse( MOUSE_PENCIL_WAITING );
	ReleaseMouse( MOUSE_PENCIL_DRAGGING );
	ReleaseMouse( MOUSE_NODE_DRAGGING );
	return; 
}

void CGraphicView::OnAddTextAnnotation( void )
{
	int index = m_AnnotationManager.annotationObjects() + 1;
	unsigned int id = SetGraphicsName( GRAPHIC_ANNOTATION_TEXT, index );
	CGraphicsAnnotationObject* pNewAnnotation = NULL;
	bool bLeader = false;
	if( theModel.nodes( true ) == 1 && m_Filter.windowType == MODEL_WINDOW ) {
		bLeader = true;
		const CNode* pN = theModel.node( 1, true );
		pNewAnnotation =
			new CGraphicsAnnotationObject( CGraphicsAnnotationObject::ANNOTATION_WITH_ARROW_LEADER, this, 
			"Arial", id , pN );
		pNewAnnotation->setLeaderTo( pN );
	}
	else if( theModel.elements( MEMBER_ELEMENT, true ) == 1 && m_Filter.windowType == MODEL_WINDOW ) {
		bLeader = true;
		const CMember* pM = (const CMember*)theModel.element( MEMBER_ELEMENT, 1, true );
		pNewAnnotation =
			new CGraphicsAnnotationObject( CGraphicsAnnotationObject::ANNOTATION_WITH_ARROW_LEADER, this, 
			"Arial", id , pM );
		pNewAnnotation->setLeaderTo( pM );
	}
	else if( theModel.elements( PLANAR_ELEMENT, true ) == 1 && m_Filter.windowType == MODEL_WINDOW ) {
		const CPlanar* pP = (const CPlanar*)theModel.element( PLANAR_ELEMENT, 1, true );
		bLeader = true;
		pNewAnnotation =
			new CGraphicsAnnotationObject( CGraphicsAnnotationObject::ANNOTATION_WITH_ARROW_LEADER, this, 
			"Arial", id , pP );
		pNewAnnotation->setLeaderTo( pP );
	}
	else { 
		pNewAnnotation =
			new CGraphicsAnnotationObject( CGraphicsAnnotationObject::ANNOTATION_TEXT_ONLY, this, 
			"Arial", id );
	}

	if( pNewAnnotation ) {
		pNewAnnotation->AddLine("Enter your text" );
		if( pNewAnnotation->type() ==  CGraphicsAnnotationObject::ANNOTATION_TEXT_ONLY )
			pNewAnnotation->setFixedPosition( true );
		else
			pNewAnnotation->setFixedPosition( false );

		CAnnotationDialog dlg( *pNewAnnotation, bLeader, this );
		if( dlg.DoModal() == IDOK ) {
			pNewAnnotation->SetControlPoint( m_LastMouseButton );
			pNewAnnotation->set3DLocation( m_Current3DPt );
			pNewAnnotation->updateBoundingBox(true);
			m_AnnotationManager.addAnnotationObject( *pNewAnnotation );
		}
		else
			delete pNewAnnotation;
	}
	Invalidate();
	return;
}

void CGraphicView::OnAddNodeDimensionAnnotation( void )
{
	ASSERT( theModel.nodes( true ) == 2 );
	const CNode* pN1 = theModel.node( 1, true );
	const CNode* pN2 = theModel.node( 2, true );
	ASSERT( pN1 ); ASSERT( pN2 );
	if( pN1 && pN2 ) {
		CPoint3D p1, p2;
		p1.x = pN1->x();
		p1.y = pN1->y();
		p1.z = pN1->z();
		p2.x = pN2->x();
		p2.y = pN2->y();
		p2.z = pN2->z();
		int index = m_AnnotationManager.annotationObjects() + 1;
		unsigned int id = SetGraphicsName( GRAPHIC_ANNOTATION_TEXT, index );
		CGraphicsAnnotationObject* pNewAnnotation =
			new CGraphicsAnnotationObject( CGraphicsAnnotationObject::ANNOTATION_DIMENSION, this, "Arial", id,
			pN1, pN2 );
		pNewAnnotation->setLeaderTo( p1, 0 );
		pNewAnnotation->setLeaderTo( p2, 1 );
		pNewAnnotation->SetControlPoint( m_LastMouseButton );
		m_AnnotationManager.addAnnotationObject( *pNewAnnotation );
		m_AnnotationManager.Draw( m_pDC, CRect( 0, 0, 0, 0 ) );  // get the fonts set up for this annotation

	}
	Invalidate();
	return;
}

void CGraphicView::OnDeleteAnnotation( void )
{
	/*ASSERT( m_pEditAnnotation );
	if( m_pEditAnnotation ) {
		m_AnnotationManager.removeAnnotationObject( *m_pEditAnnotation );
		m_pEditAnnotation = NULL;
		Invalidate();
	}*/
}

void CGraphicView::OnEditAnnotation( void )
{
	/*ASSERT( m_pEditAnnotation );
	if( m_pEditAnnotation ) {
		CAnnotationDialog dlg( *m_pEditAnnotation, false, this, false );
		if( dlg.DoModal() == IDOK ) {
			Invalidate();
		}
	}*/
}

void CGraphicView::OnSnapGridToNode( )
{
	CGraphicGrid* g = theGridManager.grid( 1 );
	ASSERT_RETURN( g );
	//get first selected node
	bool bFound = false;
	CPoint3D pt = g->origin();
	for( CNodeIterator ni( m_Filter.node.filter ); !ni; ++ni )
	{
		if( ni()->isSelected() )
		{
			pt.x = ni()->x();
			pt.y = ni()->y();
			pt.z = ni()->z();
			bFound =true;
			break;
		}
	}
	if( bFound )
		g->setOrigin( (const CPoint3D&)pt );
}

void CGraphicView::OnAddAreaVertex( )
{
	for( int i = 1; i <= theModel.areas(); i++ ){
		//check the boundary
		for( int j = 1; j <= theModel.area( i )->boundary().points(); j++ ){
			CArea* pOrig = (CArea*)theModel.area(i);
			CChain chain = pOrig->boundary();
			for( int k = 1; k <= chain.points(); k++ )
			{
				if( chain.point(k)->isSelected() ){
					CArea* pNew = new CArea( *pOrig );
					if( !chain.insertPoint( k ) ){
						ASSERT( false );
						delete pNew;
						return;
					}
					pNew->boundary( chain );
					resetCopyFlags();
					setCopyFlag( CArea::COPY_BOUNDARY );
					// TeK Fix 5/26/2009: controller is expecting a writable string buffer or null.
					//theController.modify( pOrig, pNew, "Insert area vertex." );
					theController.modify( pOrig, pNew ); // controller will report error message
					return;
				}
			}
		}
		//check holes
		for( int j = 1; j <= theModel.area( i )->holes(); j++ ){
			CArea* pOrig = (CArea*)theModel.area(i);
			CChain chain = *pOrig->holeArray()[j];
			for( int k = 1; k <= chain.points(); k++ )
			{
				if( chain.point(k)->isSelected() ){
					CArea* pNew = new CArea( *pOrig );
					if( !chain.insertPoint( k ) ){
						ASSERT( false );
						delete pNew;
						return;
					}
					pNew->removeHole( j );
					pNew->addHole( chain );
					resetCopyFlags();
					setCopyFlag( CArea::COPY_HOLES );
					// TeK Fix 5/26/2009: controller is expecting a writable string buffer or null.
					//theController.modify( pOrig, pNew, "Insert hole vertex." );
					theController.modify( pOrig, pNew );  // controller will report error message
					return;
				}
			}
		}
		//check corridors
		for( int j = 1; j <= theModel.area( i )->corridors(); j++ ){
			CArea* pOrig = (CArea*)theModel.area(i);
			CChain chain = *pOrig->corridorArray()[j];
			for( int k = 1; k <= chain.points(); k++ )
			{
				if( chain.point(k)->isSelected() ){
					CArea* pNew = new CArea( *pOrig );
					if( !chain.insertPoint( k ) ){
						ASSERT( false );
						delete pNew;
						return;
					}
					pNew->removeCorridor( j );
					pNew->addCorridor( chain );
					resetCopyFlags();
					setCopyFlag( CArea::COPY_CORRIDORS );
					// TeK Fix 5/26/2009: controller is expecting a writable string buffer or null.
					//theController.modify( pOrig, pNew, "Insert corridor vertex." );
					theController.modify( pOrig, pNew );  // controller will report error message
					return;
				}
			}
		}

	}

}

//void CGraphicView::SetServiceCase( CServiceCase* pL )
//{
//	// Set the active load case to pL and redraw the loads
//	if( pL != m_Filter.loadCase() )
//	{
//		m_Filter.loadCase( pL );
//		Invalidate();		// force a repaint of entire window
//		return;
//	}
//}

void CGraphicView::SetAServiceCase()
{
	// Set up a good service case, but only if we don't already have one
	if( NULL == serviceCase() ) {
		// Attempt to get the service from the current result case
		const CResultCase* pRC = m_Filter.resultCase();
		if( pRC ) {
			const CLoadCase& lc = pRC->owner();
			if( SERVICE_CASE == lc.type() ) {
				m_Filter.loadCase( const_cast<CLoadCase*>(&lc) );
				return;
			}
		}
		// find the first available service case
		if( theProject.loadCases( SERVICE_CASE ) >= 1 ) {
			m_Filter.loadCase( theProject.loadCase( SERVICE_CASE, 1 ) );
		}
	}
	return;
}


void CGraphicView::SetAResultCase( bool recalcFactor )
{
	// Attempt to set up results from the active load case, otherwise get the first results available
	const CResultCase* pRC = m_Filter.resultCase();

	// First, do we already have a result case that matches our load case?
	if( pRC && (m_Filter.loadCase() == (CLoadCase*)&pRC->owner()) ) {
		if( recalcFactor ) {
			m_Filter.SetInternalScaleFactor();
		}
		return; // we are done!
	}
	else pRC = NULL;

	if( m_Filter.loadCase() ) {
		const CLoadCase* pLC = m_Filter.loadCase();
		if( pLC->areResultsValid( STATIC_P_DELTA_TIME ) ) {
			pRC = &pLC->resultCase( STATIC_P_DELTA_TIME );
		}
		else if( pLC->areResultsValid( FIRST_ORDER_TIME ) ) {
			pRC = &pLC->resultCase( FIRST_ORDER_TIME );
		}
		else if( pLC->areResultsValid( ENVELOPE_MAXIMUM_TIME ) ) {
			pRC = &pLC->resultCase( ENVELOPE_MAXIMUM_TIME );
		}
		else if( pLC->areAnyResultsValid() ) {
			pRC = &pLC->resultCase( 1 );
		}
	}
	if( NULL == pRC ) {
		if( theProject.haveAnyResults() ) {
			// Try to find some results to show!
			pRC = theProject.resultCase( 1 );
			if( pRC ) {
				m_Filter.loadCase( (CLoadCase*)&pRC->owner() );
			}
		}
	}
	else {
		m_Filter.resultCase( pRC );
	}

	// TeK Removed 10/1/2009: Keep functions single-purposed, 
	// most callers will cause invalidate anyway!
	//if( CurrentResultCase() ) {
	//	Invalidate();
	//}
	return;
}


void CGraphicView::UpdateResults( )
{
	SetAResultCase( false );
	//SetupResults( );
	m_graphicsModel.SetupResults( m_Filter );
	Invalidate( );
}

// Printing support
BOOL CGraphicView::OnPreparePrinting( CPrintInfo* pInfo )
{
	pInfo->SetMinPage( 1 );
	pInfo->SetMaxPage( 1 );  // only one page in a graphic window
	return DoPreparePrinting( pInfo );
}

void CGraphicView::OnFilePrintPreview()
{
	CScrollView::OnFilePrintPreview();
	return;
}
void CGraphicView::OnFilePrint()
{
	CScrollView::OnFilePrint();
	return;
}

void CGraphicView::OnPrint( CDC* pDC, CPrintInfo* pInfo )
{
	// Determine the DIB size for either printing or print preview.
	CRect rcClient;
	GetClientRect(&rcClient);
	
	//JL Added 9/1/2009 - Was causing CreateDIBSection to fail after several repeated prints
	//JL Note 10/8/2009 - The real problem was that we were calling OnBeginPrint in the OnPrint 
	// function so OnPrint was getting called twice (first by MFC CView and then again in this function)!
	//Make sure everything is properly cleaned up first
	if( m_hDib )
		DeleteObject( m_hDib );
	if( m_hMemDC )
		DeleteObject( m_hMemDC );
	m_hMemDC = NULL;
	m_hDib = NULL;
	m_pBitmapBits = NULL;

	// Get client window size and aspect ratio
	int windowHeight = rcClient.Height();
	int windowWidth  = rcClient.Width();
	if( (windowWidth <= 0) || (windowHeight <= 0) ) {
		// TeK Change 8/27/2008: Fake a typical window, if we have no valid size?
		windowWidth = 800;
		windowHeight = 600;
	}
	float fClientRatio = float(windowHeight)/float(windowWidth);

	// Get print page size and aspect ratio
	int leftMarginPixels = (int)(ini.report().leftMargin * pDC->GetDeviceCaps( LOGPIXELSX ));
	int rightMarginPixels = (int)(ini.report().rightMargin * pDC->GetDeviceCaps( LOGPIXELSX ));
	int topMarginPixels = (int)(ini.report().topMargin * pDC->GetDeviceCaps( LOGPIXELSY ));
	int bottomMarginPixels = (int)(ini.report().bottomMargin * pDC->GetDeviceCaps( LOGPIXELSY ));

	m_szPage.cx = pDC->GetDeviceCaps(HORZRES) - (leftMarginPixels + rightMarginPixels);
	m_szPage.cy = pDC->GetDeviceCaps(VERTRES) - (topMarginPixels + bottomMarginPixels);
	if( (0 == m_szPage.cy) || (0 == m_szPage.cx) ) {
		AfxMessageBox( "Printing failed: no page size?" );
		return;
	}

	float fPageRatio = float( m_szPage.cy )/float( m_szPage.cx );

	m_printScale = 1;
	CSize szDIB;
	if (pInfo->m_bPreview)
	{
		szDIB.cx = windowWidth;
		szDIB.cy = windowHeight;
		//use larger of two to fit the image to the page
		if( windowWidth > windowHeight )
			szDIB.cy = LONG(windowWidth * fPageRatio);
		else 
			szDIB.cx = LONG(windowWidth / fPageRatio);
	}
	else  // Printing
	{
		// Use higher resolution for printing.
		// Adjust size according screen's ratio.
		if (m_szPage.cy > fClientRatio*m_szPage.cx)
		{
			// View area is wider than Printer area
			szDIB.cx = m_szPage.cx;
			szDIB.cy = long(fClientRatio*m_szPage.cx);
		}
		else
		{
			// View area is narrower than Printer area
			szDIB.cx = long(float(m_szPage.cy)/fClientRatio);
			szDIB.cy = m_szPage.cy;
		}

		//reset print scale
		m_printScale = 1;

		// Reduce the Resolution if the Bitmap size is too big.
		// Adjust the maximum value, which depends on printer's resolution.
		//JL Note 10/9/2009 - anything above 3000 is too big for standard page size
		//opengl lines become too thin when we go too far above 3000.
		while( szDIB.cx > 3000 || szDIB.cy > 3000 )
		{
		//while (szDIB.cx*szDIB.cy > 10e6)
		//{
			szDIB.cx = szDIB.cx/2;
			szDIB.cy = szDIB.cy/2;
			m_printScale *= 2;
		}
		
		//now fit it to the print page
		if( windowWidth > windowHeight )
			szDIB.cy = LONG(szDIB.cx * fPageRatio);
		else 
			szDIB.cx = LONG(szDIB.cx / fPageRatio);
	}

	if( (szDIB.cx <= 0) || (szDIB.cy <= 0) ) {
		AfxMessageBox( "Printing failed, invalid DIB size calculated." );
		return;
	}

	// 2. Create DIB Section
	memset(&m_bmi, 0, sizeof(BITMAPINFO));
	m_bmi.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	m_bmi.bmiHeader.biWidth			= szDIB.cx;
	m_bmi.bmiHeader.biHeight		= szDIB.cy;
	m_bmi.bmiHeader.biPlanes		= 1;
	m_bmi.bmiHeader.biBitCount		= 24;
	m_bmi.bmiHeader.biCompression	= BI_RGB;
	m_bmi.bmiHeader.biSizeImage		= 0;// Use 0 for BI_RGB, image buffer size for BI_JPEG or BI_PNG
										// This is used for compressed bitmaps where the image size is not a direct function of 
										// the width and height of the image itself. Our bitmaps aren't compressed, so we can set it to 0

	// TeK Added 8/27/2008: See documentation for CreateCompatibleDC
	if( !(pDC->GetDeviceCaps(RASTERCAPS) & RC_BITBLT) ) {
		AfxMessageBox( "Printing failed RC_BITBLT test! Printer driver cannot support raster (bitmap) copy function." );
		return;
	}

	m_Camera.Resize( szDIB.cx, szDIB.cy );

	// Create memory DC, and associate it with the DIB.
	m_hMemDC = CreateCompatibleDC(NULL);

	if (!m_hMemDC) {
		AfxMessageBox( "Printing failed in CreateCompatibleDC." );
		return;
	}

	m_hDib = CreateDIBSection(m_hMemDC, &m_bmi, DIB_RGB_COLORS, &m_pBitmapBits, NULL, 0);
	if( NULL == m_hDib ) {
		AfxMessageBox( "Printing failed in CreateDIBSection." );
		return;
	}
	
	HGDIOBJ hSO = SelectObject(m_hMemDC, m_hDib);
	if( (NULL == hSO) || (HGDI_ERROR == hSO) ) {
		AfxMessageBox( "Printing failed in SelectObject." );
		return;
	}

	// Setup memory DC's pixel format.
	if (!SetupPrintingPixelFormat(m_hMemDC))
	{
		DeleteObject(m_hDib);
		m_hDib = NULL;
		DeleteDC(m_hMemDC);
		m_hMemDC = NULL;
		AfxMessageBox( "Printing failed in SetupPrintintPixelFormat. Try reducing the printer's dpi (dots per inch), or quality setting." );
		return;
	}

	// Create memory RC
	m_hMemRC = wglCreateContext(m_hMemDC);
	if (!m_hMemRC)
	{
		DeleteObject(m_hDib);
		m_hDib = NULL;
		DeleteDC(m_hMemDC);
		m_hMemDC = NULL;
		AfxMessageBox( "Printing failed in wglCreateContext. Try reducing the printing dpi (dots per inch)." );
		return;
	}

	// Make the memory RC current
	if(!wglMakeCurrent(m_hMemDC, m_hMemRC) ){
		AfxMessageBox( "Printing failed in wglMakeCurrent()! Try reducing the printing dpi (dots per inch)." );
		ASSERT( FALSE );
	}

	DrawForPrinter( pDC, pInfo );
	
	////GDI Test...
	//CPen pen(PS_SOLID, 50, RGB (0, 0, 255 ) );
	//CBrush brush(RGB(255,255,0));
	//pDC->SetMapMode(MM_LOMETRIC);
	//CPen* pOldPen = pDC->SelectObject(&pen);
	//CBrush* pOldBrush = pDC->SelectObject(&brush);
	//pDC->Ellipse(100,-100,1100,-1100);
	//pDC->SelectObject(pOldBrush);
	//pDC->SelectObject(pOldPen);
	//DeleteObject(pen);
	//DeleteObject(brush);

	// We'll copy the image to the printer's DC for printing or print preview.
	// The image is now in the DIB so we don't need the memory RC anymore. 
	wglMakeCurrent(NULL, NULL);	
	wglDeleteContext(m_hMemRC);

	// Restore last DC and RC
	m_hDC = ::GetDC(m_hWnd);
	wglMakeCurrent(m_hDC, m_hRC);	

	// 2. Calculate the target size according to the image size, and orientation of the paper.
	float fBmiRatio = float(m_bmi.bmiHeader.biHeight) / m_bmi.bmiHeader.biWidth;
	ASSERT( !zero(fBmiRatio));
	CSize szTarget;  
	szTarget.cx = m_szPage.cx;
	szTarget.cy = m_szPage.cy;
	if (m_szPage.cx > m_szPage.cy)	 // Landscape page
	{
		if(fBmiRatio<1)	  // Landscape image
		{
			szTarget.cx = m_szPage.cx;
			szTarget.cy = long(fBmiRatio * m_szPage.cx);
		}
		else			  // Portrait image
		{
			szTarget.cx = long(m_szPage.cy/fBmiRatio);
			szTarget.cy = m_szPage.cy;
		}
	}
	else		    // Portrait page
	{
		if(fBmiRatio<1)	  // Landscape image
		{
			szTarget.cx = m_szPage.cx;
			szTarget.cy = long(fBmiRatio * m_szPage.cx);
		}
		else			  // Portrait image
		{
			szTarget.cx = long(m_szPage.cy/fBmiRatio);
			szTarget.cy = m_szPage.cy;
		}
	}

	//int leftMarginPixels = (int)(ini.report().leftMargin * pDC->GetDeviceCaps( LOGPIXELSX ));
	//int topMarginPixels = (int)(ini.report().topMargin * pDC->GetDeviceCaps( LOGPIXELSY ));
	CSize szOffset((m_szPage.cx - szTarget.cx) / 2 + leftMarginPixels, (m_szPage.cy - szTarget.cy) / 2 + topMarginPixels);

	// Stretch image to fit in the target size.
	int nRet = StretchDIBits(pDC->GetSafeHdc(),
		szOffset.cx, szOffset.cy,
		szTarget.cx, szTarget.cy,
		0, 0,
		m_bmi.bmiHeader.biWidth, m_bmi.bmiHeader.biHeight,
		(GLubyte*) m_pBitmapBits,
		&m_bmi, DIB_RGB_COLORS, SRCCOPY);

	if(nRet == GDI_ERROR)
	{
		TRACE0("Failed in StretchDIBits()");
	}

	// release memory
	if( m_hDib )
		DeleteObject( m_hDib );
	if( m_hMemDC )
		DeleteDC( m_hMemDC );
	m_hMemDC = NULL;
	m_hDib = NULL;
	m_pBitmapBits = NULL;

	m_printScale = 1;
}

//void CGraphicView::OnBeginPrinting( CDC* /*pDC*/, CPrintInfo* /*pInfo*/ )
//{
//}

void CGraphicView::OnEndPrinting( CDC* pDC, CPrintInfo* pInfo )
{
	// Restore last DC and RC
	m_hDC = ::GetDC(m_hWnd);
	wglMakeCurrent(m_hDC, m_hRC);	

	//reset camera
	CRect rcClient;
	GetClientRect(&rcClient);
	//JL Note 8/11/2009 - reset hDC
	m_hDC = m_pDC->GetSafeHdc();
	//JL Added 5/22/2009 - resize the window back to the original client size so that
	//selection works properly
	m_Camera.Resize( rcClient.Width(), rcClient.Height() );
	CScrollView::OnEndPrinting( pDC, pInfo );
}

bool CGraphicView::SetupPrintingPixelFormat( HDC hDC )
{
	PIXELFORMATDESCRIPTOR pixelDesc;

	DWORD dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_STEREO_DONTCARE;

	pixelDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixelDesc.nVersion = 1;

	pixelDesc.dwFlags = dwFlags;
	pixelDesc.iPixelType = PFD_TYPE_RGBA;
	pixelDesc.cColorBits = 24;
	pixelDesc.cRedBits = 8;
	pixelDesc.cRedShift = 16;
	pixelDesc.cGreenBits = 8;
	pixelDesc.cGreenShift = 8;
	pixelDesc.cBlueBits = 8;
	pixelDesc.cBlueShift = 0;
	pixelDesc.cAlphaBits = 0;
	pixelDesc.cAlphaShift = 0;
	pixelDesc.cAccumBits = 64;
	pixelDesc.cAccumRedBits = 16;
	pixelDesc.cAccumGreenBits = 16;
	pixelDesc.cAccumBlueBits = 16;
	pixelDesc.cAccumAlphaBits = 0;
	pixelDesc.cDepthBits = 32;
	pixelDesc.cStencilBits = 8;
	pixelDesc.cAuxBuffers = 0;
	pixelDesc.iLayerType = PFD_MAIN_PLANE;
	pixelDesc.bReserved = 0;
	pixelDesc.dwLayerMask = 0;
	pixelDesc.dwVisibleMask = 0;
	pixelDesc.dwDamageMask = 0;

	int nPixelIndex = ::ChoosePixelFormat(hDC, &pixelDesc);
	if (nPixelIndex == 0) // Choose default
	{
		nPixelIndex = 1;
		if (::DescribePixelFormat(hDC, nPixelIndex, 
			sizeof(PIXELFORMATDESCRIPTOR), &pixelDesc) == 0)
		{
			AfxMessageBox( "Default video mode forced in GraphicView!" );
			return false;
		}
	}

	if (!::SetPixelFormat(hDC, nPixelIndex, &pixelDesc)) {
		return false;
	}

	return true;
}

void CGraphicView::OnEditCopy( void )
{
	if( ::OpenClipboard( m_hWnd ) ) {
		::EmptyClipboard();

		// Get client geometry
		CRect rect;
		GetClientRect(&rect);
		CSize size(rect.Width(),rect.Height());
		TRACE("  client zone : (%d;%d)\n",size.cx,size.cy);
		// Lines have to be 32 bytes aligned, suppose 24 bits per pixel
		// I just cropped it
		size.cx -= size.cx % 4;
		TRACE("  final client zone : (%d;%d)\n",size.cx,size.cy);

		// Alloc pixel bytes
		int NbBytes = 3 * size.cx * size.cy;
		unsigned char *pPixelData = new unsigned char[NbBytes];

		// Copy from OpenGL
		glReadBuffer(GL_BACK);
		OnDraw( m_pDC );
		wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC );
		glReadPixels(0,0,size.cx,size.cy,GL_BGR_EXT,GL_UNSIGNED_BYTE,pPixelData); 

		// Create a bitmap and select it in the device context
		// Note that this will never be used ;-) but no matter
		CBitmap bitmap;
		CDC *pDC = GetDC();
		CDC MemDC;
		ASSERT(MemDC.CreateCompatibleDC(NULL));
		ASSERT(bitmap.CreateCompatibleBitmap(pDC,size.cx,size.cy));
		MemDC.SelectObject(&bitmap);

		// Fill header
		BITMAPINFOHEADER header;
		header.biWidth = size.cx;
		header.biHeight = size.cy;
		header.biSizeImage = NbBytes;
		header.biSize = 40;
		header.biPlanes = 1;
		header.biBitCount =  3 * 8; // RGB
		header.biCompression = 0;
		header.biXPelsPerMeter = 0;
		header.biYPelsPerMeter = 0;
		header.biClrUsed = 0;
		header.biClrImportant = 0;

		// Generate handle
		HANDLE handle = (HANDLE)::GlobalAlloc (GHND,sizeof(BITMAPINFOHEADER) + NbBytes);
		if(handle != NULL)
		{
			// Lock handle
			char *pData = (char *) ::GlobalLock((HGLOBAL)handle);
			// Copy header and data
			memcpy(pData,&header,sizeof(BITMAPINFOHEADER));
			memcpy(pData+sizeof(BITMAPINFOHEADER),pPixelData,NbBytes);
			// Unlock
			::GlobalUnlock((HGLOBAL)handle);

			// Push DIB in clipboard
			OpenClipboard();
			EmptyClipboard();
			SetClipboardData(CF_DIB,handle);
			CloseClipboard();
		}

		// Cleanup
		MemDC.DeleteDC();
		bitmap.DeleteObject();
		delete [] pPixelData;
	}  // if OpenClipboard
}	// end CGraphicView::CmEditCopy

void CGraphicView::RotatePresentView() {
	// had to override the CGraphicViewOld function so that if we have a zero size project the
	// grid gets set up properly!
	bool wasSet = false;
	//JL Note 9/30/1009 - remove m_View
	//if( ! equal( m_View.Rotations[0], m_Filter.rotation[0] ) ||
	//	! equal( m_View.Rotations[1], m_Filter.rotation[1] ) ||
	//	! equal( m_View.Rotations[2], m_Filter.rotation[2] )) {
	//		m_View.Rotations[0] = m_Filter.rotation[0];	// copy the new angles
	//		m_View.Rotations[1] = m_Filter.rotation[1];
	//		m_View.Rotations[2] = m_Filter.rotation[2];
	//		m_View.SetViewingMatrix();  			// see that the viewing matrix gets updated
	//		if( m_Filter.zoomAll )
	//			SetWindowToExtents();

	//		wasSet = true;
	//}
	// TeK Change 5/21/96: Move this out of conditional, because we get
	// this message when the View Manager Filters change and we want to make
	// sure that we repaint.  We might want to optimize the messages
	// to minimize the repainting.
	if( !wasSet && m_Filter.zoomAll )
		ZoomToExtents(false);
	Invalidate();		// for now force a repaint of entire window

	// lastly update the statusbar rotation combo
	VAWindow()->SendMessage( VIEW_MANAGER_CHANGE_MESSAGE, 1, 0 );
	return;
} // RotatePresentView();


void CGraphicView::ZoomOutView() 
{

	double scale = ini.filter().zoomFactor;
	if( scale > 0.) 
	{
		m_Camera.Zoom( scale );
		Invalidate();
	}
	return;
}	// end COldGraphicView::ZoomView


void CGraphicView::ZoomInView() 
{
	double scale = ini.filter().zoomFactor;
	if( scale > 0.) 
	{
		m_Camera.Zoom( 1/scale );
		//SetCapture();
		Invalidate();
	}
	return;
}	// end COldGraphicView::ZoomView

CServiceCase* CGraphicView::serviceCase( void )
{
	CServiceCase* pSC = NULL;
	if( m_Filter.loadCase() ){
		if( m_Filter.loadCase()->type() == SERVICE_CASE ) {
			pSC = (CServiceCase*)(m_Filter.loadCase());
		}
	}
	return pSC;
}

//CServiceCase* CGraphicView::projectCurrentLoadCase()
//{
//	// return the first service load case in the project if one exists
//	// otherwise return null
//	if( theProject.loadCases( SERVICE_CASE ) > 0 )
//		return ((CServiceCase*)theProject.loadCase( SERVICE_CASE, 1 ));
//	else
//		return NULL;
//}	// end projectCurrentLoadCase()

void CGraphicView::SetLoadCase( CLoadCase* pL )
{
	ASSERT( pL );
	m_Filter.loadCase( pL ); // also calls SetInternalScaleFactor()
	Invalidate();		// force a repaint of entire window
	return;
}	// end	CModelView::SetLoadCase()


const CResultCase* CGraphicView::CurrentResultCase( void )
{
	return m_Filter.resultCase();
}

void CGraphicView::SetResultCase( const CResultCase* pRC )
{
	m_Filter.resultCase( pRC );
}


// animation support
bool CGraphicView::AnimateView( int nFrames, int nFramesPerSecond, const char* aviFile ) {

	CAVIGenerator AviGen;	// generator
	BYTE* bmBits = NULL;	// image buffer
	HRESULT hr;

	const CLoadCase* pLoadCase = m_Filter.loadCase();
	int stepCount = 1;
	if( pLoadCase->isTimeStep() ) {
		nFrames = pLoadCase->timeSteps();
	}

	ASSERT( nFrames > 0 );
	ASSERT( strlen( aviFile ) > 0 );

	bool bSuccess = false;

	// determine the size of the window to animate
	CRect r;
	GetClientRect( &r );
	if( r.Width() <= 0 || r.Height() <= 0 )
		return bSuccess;
	
	AviGen.SetRate(nFramesPerSecond);	// set 20fps
	AviGen.SetBitmapHeader(this);	// get bitmap info out of the view
	AviGen.SetFileName( aviFile );
	hr=AviGen.InitEngine();	// start engine
	if (FAILED(hr) )
	{
		AfxMessageBox(AviGen.GetLastErrorMessage());
		goto Cleaning;
	}
	else if( hr == S_FALSE ) //animation canceled...
		goto Cleaning;

	LPBITMAPINFOHEADER lpbih=AviGen.GetBitmapHeader(); // getting bitmap info

	if( !lpbih )
	{
		AfxMessageBox( AviGen.GetLastErrorMessage() );
		goto Cleaning;
	}

	bmBits=new BYTE[lpbih->biSizeImage];

	StartProgressDialog("Generating Animation Frames");
	double inc;
	inc = 1./(double)nFrames;
	SetProgressDialogIncrement( inc );

	double origFactor = m_Filter.displacementFactor;//GetDistortionFactor();//mDisplacementScaleFactor;
	const CResultCase* pRc;

	// for time step analyses we loop on all the time steps
	if( pLoadCase->isTimeStep() ) {

		const CResultCase* origCase = m_Filter.resultCase();

		for( int istep = 1; istep <= pLoadCase->timeSteps(); istep = istep + stepCount ) {

			char buf[80];
			sprintf( buf, "Frame %d of %d.", istep, nFrames );
			if( UpdateProgressDialog( double(istep)*inc, buf ) )
			{
				m_Filter.resultCase( origCase );
				goto Cleaning;
			}
			// now do the images
			m_Filter.resultCase( &(pLoadCase->resultCase( istep )) );
			glReadBuffer(GL_BACK);
			OnDraw( m_pDC );
			wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC );
			glReadPixels(0,0,lpbih->biWidth,lpbih->biHeight, GL_BGR_EXT,GL_UNSIGNED_BYTE,bmBits);  
			// adding frame and continue if OK
			hr=AviGen.AddFrame(bmBits);
			if (FAILED(hr))
			{
				AfxMessageBox(AviGen.GetLastErrorMessage());
				m_Filter.resultCase( origCase );
				goto Cleaning;
			}	
		}
		m_Filter.resultCase( origCase );
	}
	else {  // no time steps - use sine function to simulate!

		pRc =  CurrentResultCase();

		double add = 1./double(nFrames)*M_PI;
		double angle = -add/2.;
		char buf[80];
		for ( int i = 0; i < nFrames; i++) {
			sprintf( buf, "Frame %d of %d.", i+1, nFrames );
			if( UpdateProgressDialog( double(i+1)*inc, buf ) )
			{
				m_Filter.displacementFactor = origFactor;
				goto Cleaning;
			}
			// now do the images
			angle = angle + add;
			double fact = sin( angle );
			m_Filter.SetUserScaleFactor( origFactor*fact );
			glReadBuffer(GL_BACK);
			OnDraw( m_pDC );
			wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC );
			glReadPixels(0,0,lpbih->biWidth,lpbih->biHeight, GL_BGR_EXT,GL_UNSIGNED_BYTE,bmBits);  

			// adding frame and continue if OK
			hr=AviGen.AddFrame(bmBits);

			if (FAILED(hr))
			{
				AfxMessageBox(AviGen.GetLastErrorMessage());
				m_Filter.SetUserScaleFactor( origFactor );
				goto Cleaning;
			}
		}  // for i
		m_Filter.SetUserScaleFactor( origFactor );
	}

	bSuccess = true;
	// cleaning memory	
	Cleaning:

	//AVIFileExit();

	KillProgressDialog();
	AviGen.ReleaseEngine(); // releasing resources
	delete[] bmBits;	// release resources

	VAWindow()->SendMessage( VIEW_MANAGER_CHANGE_MESSAGE, 0, 0 ); //ending AnimateView()
	Invalidate();
	return bSuccess;
}  // animate view

// CAnnotationDialog dialog

IMPLEMENT_DYNAMIC(CAnnotationDialog, CDialog)
CAnnotationDialog::CAnnotationDialog(CGraphicsAnnotationObject& rAnn, 
									 bool bAllowLeader, CWnd* pParent /*=NULL*/, bool bIsNewAnnotation )
									 : CDialog(CAnnotationDialog::IDD, pParent), m_Annotation( rAnn ), 
									 m_bAllowLeader( bAllowLeader), m_bIsNewAnnotation( bIsNewAnnotation )
{
}

CAnnotationDialog::~CAnnotationDialog()
{
}

void CAnnotationDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ANNOTATION_TEXT, mTextControl);
	DDX_Control(pDX, IDC_ANNOTATION_ARROW_RADIO, mArrowLeader);
	DDX_Control(pDX, IDC_ANNOTATION_LINE_RADIO, mLineLeader);
}


BEGIN_MESSAGE_MAP(CAnnotationDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CAnnotationDialog message handlers

void CAnnotationDialog::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
	UpdateData();
	char buf[512];
	m_Annotation.ClearText();
	for( int i = 0; i < mTextControl.GetLineCount(); i++ ) {
		int count = mTextControl.GetLine( i, buf, 511 );
		if( count > 0 ) {
			buf[count] = '\0';
			m_Annotation.AddLine( buf );
		}
	}
	if( m_bIsNewAnnotation ) {
		if( m_bAllowLeader ) {
			if( mArrowLeader.GetCheck() > 0 )
				m_Annotation.setType( CGraphicsAnnotationObject::ANNOTATION_WITH_ARROW_LEADER );
			else
				m_Annotation.setType( CGraphicsAnnotationObject::ANNOTATION_WITH_LINE_LEADER );
		}
		else
			m_Annotation.setType( CGraphicsAnnotationObject::ANNOTATION_TEXT_ONLY );
	}

	if( m_Annotation.type() != CGraphicsAnnotationObject::ANNOTATION_DIMENSION ) {
		if( IS_CHECKED( IDC_NO_MOVE_RADIO ) )
			m_Annotation.setFixedPosition( true );
		else
			m_Annotation.setFixedPosition( false );
	}

	return;
}

BOOL CAnnotationDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	if( m_Annotation.LineCount() > 0 ) {
		CString s;
		int len;
		CString s1 = m_Annotation.FirstLine(&len);
		s = s1;
		for( int i = 0; i < m_Annotation.LineCount()-1; i++ ) {
			s1 = m_Annotation.NextLine(&len);
			s = s + "\r\n" + s1;
		}
		mTextControl.SetWindowText( s );
	}
	mTextControl.SetSel(-1, 0);

	if( m_bIsNewAnnotation ) {
		if( m_bAllowLeader ) {
			if( m_Annotation.type() == CGraphicsAnnotationObject::ANNOTATION_WITH_ARROW_LEADER )
				mArrowLeader.SetCheck( TRUE );
			else
				mLineLeader.SetCheck( TRUE );
		}
		else {
			mLineLeader.EnableWindow( FALSE );
			mArrowLeader.EnableWindow( FALSE );
		}
	}
	else {
			mLineLeader.ShowWindow( SW_HIDE );
			mArrowLeader.ShowWindow( SW_HIDE );
			HIDE( IDC_LEADER_TEXT );
	}

	if( m_Annotation.type() == CGraphicsAnnotationObject::ANNOTATION_DIMENSION ) {
		HIDE( IDC_MOVE_TEXT );
		HIDE( IDC_YES_MOVE_RADIO );
		HIDE( IDC_NO_MOVE_RADIO );
	}
	else {
		if( m_Annotation.hasFixedPosition() )
			SET_CHECK( IDC_NO_MOVE_RADIO );
		else
			SET_CHECK( IDC_YES_MOVE_RADIO );
			
	}


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

// TeK Add 10/17/2009: In free-trial mode we should have some watermark
// that cannot be turned off, and that also prints.  It can be fairly subtle
// so-as not to ruin the great graphics!
#pragma message( JL "Please hook-up some kind of watermark for free-trial" )

// Code from VA 6.0...the IDB_TRIAL_WATERMARK is in VA 7.0 resources
//void CGraphicView::DrawWatermark( float topColor[3], float bottomColor[3] )
//{
//	if( theProject.isDemo() ) 
//	{
//		static bool bLoaded = false;
//		static CBitmap myBitmap;
//		if( !bLoaded ) {
//			AfxSetResourceHandle( AfxGetInstanceHandle() );
//			bLoaded = myBitmap.LoadBitmap( IDB_TRIAL_WATERMARK );
//		}
//		ASSERT( bLoaded );
//		StartGL2DDrawing();
//		EnableGLTransparency();
//		glMatrixMode(GL_PROJECTION);
//		glLoadIdentity();
//		glOrtho(-1.0, 1.0, -1.0, 1.0, 1.0, -1.0);
//		glMatrixMode(GL_MODELVIEW);
//		glLoadIdentity();
//		//m_Text.DrawString( m_pDC, ini.font().graphWindow.name, ini.font().graphWindow.size, ini.color().members, "Trial Version" ); 					
//		glDisable(GL_DEPTH_TEST); // Color buffer only rendering
//		if( bLoaded )
//			StartGLWatermarkTexture( gnWatermarkTexture, myBitmap );
//		glBegin(GL_QUADS);
//		glColor3fv(bottomColor);
//		glTexCoord2f(0.0f, 0.0f);
//		glVertex2f(-1.0f, -1.0f);
//		glTexCoord2f(3.0f, 0.0f);
//		glVertex2f( 1.0f, -1.0f);
//		glColor3fv(topColor);
//		glTexCoord2f(3.0f, 3.0f);
//		glVertex2f( 1.0f, 1.0f);
//		glTexCoord2f(0.0f, 3.0f);
//		glVertex2f(-1.0f, 1.0f);
//		glEnd();
//		if( bLoaded )
//			EndGLTextureMode();
//		DisableGLTransparency();
//		EndGL2DDrawing();
//	}
//}
