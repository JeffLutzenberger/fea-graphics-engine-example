#pragma once

#include "Include/UseData.h"
#include "legacy/PointerArray-T.h"
#include "Layers.h"

#include "GraphicsCamera.h"
#include "OrthoCamera.h"
#include "GraphicsGrid.h"
#include "Graphics/GraphicsObjects.h"
#include "Graphics/GLTextureText.h"
#include "Graphics/GraphicsDisplayList.h"
#include "GraphicsEngine.h"
#include "GraphicsModel.h"

#include "GraphicsMemberLoad.h"
#include "Graphics/GraphicsHelpers.h"
#include "Graphics/GraphicsHelperClasses.h"
#include "Graphics/HitRecord.h"
#include "Graphics/GraphicsExtrusion.h"
#include "Graphics/GraphicsMember.h"
#include "Graphics/GraphicsCable.h"
#include "GraphicsContourBar.h"
#include "GraphicsRotationCube.h"
#include "VertexArrayManager.h"
#include "dialog/WindowTip.h"
#include "Math/Analytic3d.h"

#include "ViewData.h"

#include "dialog/WindowTip.h"

#include "WinFiltr.h"
#include "IniColor.h"  // OBJECT_COLOR_SIZE
//#include "dialog/RotatDlg.h"  // ETOrientation

enum ETOrientation {
	O_2D_FRONT,
	DEFAULT_ORIENTATION = O_2D_FRONT,
	O_2D_RIGHT,
	O_2D_TOP,
	O_2D_BOTTOM,
	O_2D_LEFT,
	O_2D_BACK,
	O_ISO_TOP_AB,
	O_ISO_ZUP_ABC,
	O_CUSTOM
};

const int viewOrientations = O_CUSTOM;

#include "VAMain_Resource.h" // for annotation class

#define ZOOM_TOLERANCE 0.01

#define RGB_BLACK RGB( 0, 0, 0 )

#include <map>

#include "GraphicView.h"  // base class

// legacy includes
#include "ViewData.h"
#include "WinFiltr.h"
// CGraphicView view

class CRigidDiaphragm;
class CNode;
class CMember;
class CPlanar;
class CSpring;
class CCable;
class CElement;
class CFoundation;
class CNodalLoad;
class CMemberLoad;
class CMovingLoad;
class CPlanarLoad;
class CAreaLoad;
class CRigidDiaphragmLoad;
class CServiceCase;
class CData;
class CReport;

class CGraphicView : public CScrollView
{
	DECLARE_DYNCREATE(CGraphicView)

public:
	// CMainFrame access here
	CGraphicView();           // constructor used by dynamic creation
	CGraphicView( LPSTR aTitle, CWindowFilter& theFilter);

	virtual ~CGraphicView();

	//basic window functions
	ETWindowType	isA() { return m_Filter.windowType; }
	void			activate();
	void			deactivate();
	bool			isActive() { return m_bActive; };
	bool			isClosing() { return m_bClosing; };
	void			WriteCoordinatesToStatusBar( const CPoint3D& p );
	void			EscapeKey();  
	void			ViewManagerChangeResults();
	void			SetWindowTitle();
	void			SaveWindowInformation( obfstream& out );
	void			WriteAnnotations( obfstream& out );
	void			ReadAnnotations( ibfstream& in );
	void			ShowAnalysisTextWindow( int show );

	CWindowFilter&  windowFilter() { return m_Filter; }
	// flags are define in WinFiltr.h
	// use COPY_FILTER | COPY_WINDOW | COPY_ROTATION | COPY_LAYERS | COPY_CASES | COPY_ALL
	void            SetWindowFilter( const CWindowFilter& _f, int flags );  

	//reports
	const char* QuickReportName();
	void QuickReport( CReport& report );
	void QuickResultViewReport( CReport& report );
	void QuickModelViewReport( CReport& report );

	//public selection functions
	int  TotalSelected();

	// window status information
	void SetExtentRedraw() { mCheckExtentChange = true; };
	bool cursorInWindow( CPoint point, bool isClientPoint );

	// view data functions
	void RotatePresentView();	// update the rotation angles and redraw

	// public zoom, pan, rotate
	void	ZoomToExtents( bool bSelected );
	void	ZoomView();
	void	ZoomOutView();
	void	ZoomInView();
	void	Rotate();
	void	GetRotation( double rotMatrix[][4] );
	void	SetRotation( double rotMatrix[][4] );
	void	RotateModel( const CPoint& delta );
	void	DragModel( const CPoint& delta );
	void	RotateX( double inc );
	void	RotateY( double inc );
	void	RotateZ( double inc );
	void	ZoomDisplayToRectangle( CRect r );
	void	ViewZoomInAtPoint( CPoint _pt );
	void	ViewZoomOutAtPoint( CPoint _pt );
	void	ViewZoomAtPoint( CPoint _pt, double scale );
	COrthoCamera&	Camera(){return m_Camera;}
	CPoint3D GetRotationAngles();

	afx_msg BOOL	OnMouseWheel( UINT nFlags, short zDelta, CPoint pt );

	//current service cases - are all of these necessary???????
	CServiceCase*		serviceCase();
	void			    SetAResultCase( bool recalcFactor );
	void			    SetAServiceCase();
	const CResultCase*	CurrentResultCase( );
	void				SetResultCase( const CResultCase* pRC ); 
	void				SetLoadCase( CLoadCase* pL );

	//what are we drawing
	ETDrawMode		currentDrawMode(){ return m_Filter.drawMode; };
	void			currentDrawMode( ETDrawMode what );

	// public drawing support
	void			OnDraw(CDC* pDC);      // overridden to draw this view
	void			DrawForPrinter( CDC* pDC, CPrintInfo* pInfo );

	//public printing support
	//afx_msg void	OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	afx_msg void	OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	afx_msg void	OnPrint(CDC* pDC, CPrintInfo* pInfo );
	afx_msg void	OnFilePrint();
	afx_msg void	OnFilePrintPreview();
	bool			Printing(){ return m_bPrinting; };
	void			Printing( bool print ){ m_bPrinting = print; };
	void			OnEditCopy();

	//animation
	bool			AnimateView( int nFrames, int nFramesPerSecond, const char* aviFile );

	//get the 
	CVAGrid&		windowGrid() { return mGrid; }

	// for layer support - is there a better way?
	void			DrawBoundingBox( const COrientedBox& _box );
	void			DrawLayerBoundingBox( );
	bool			m_bDrawCurrentLayerBox;
	long			m_LayerBoxTime;
	clock_t			m_timeLayerBoxReset;
	void			SetCurrentLayerBox( const COrientedBox& _box );

	// IES Project and VA messages will come in here
	virtual void	OnUpdate( CView *pSender, LPARAM lHint, CObject *pHint );
	LRESULT			ProjectChange( WPARAM, LPARAM );
	void			UpdateResults( );

	void	IncrementMouseSinceActivate( ){ m_MouseSinceActivate++; };

	void			MeshArea(){};
	// add or get a text object  (dxf file support)
	void			InsertAnnotation( CPoint3D p, CString s, double rotation );
	void			GetAnnotation( int index, CPoint3D* loc, CString* str, double* rotation, double* height );
	int				AnnotationCount() { return m_AnnotationManager.annotationObjects(); }

	// engine message support
	void AddAnalysisMessage( LPCSTR text );
	void ClearAnalysisMessages();
	void UpdateAnalysisMessage( double fraction, LPCSTR text );

private:

	// Window creation/startup/activation
	BOOL			PreCreateWindow( CREATESTRUCT& cs );
	BOOL			PreTranslateMessage( MSG* pMsg );
	void			OnInitialUpdate();     // first time after construct
	virtual void	OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	void			SetWindowCaption();
	LRESULT			OnHelpHitTest(WPARAM, LPARAM);


	// OpenGL initialization
	BOOL			InitOpenGL( );
	BOOL			SetupPixelFormat( );

	//Query for a particular OpenGL extension. We need a valid window to 
	//determine if an extensions is supported.
	bool			WGLisExtensionSupported(const char *extension);
	//once we have a window we can check to see if multisample is supported.
	// - sets the gArbMultisampleSupported flag 
	// - sets (but does not initialize) a multisample pixel format (if supported)
	// - return success or failure
	bool			CheckForMultisampleSupport( );

	// Creation and destruction actions
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnDestroy();

	// Sizing & other border actions
	afx_msg void	OnSize( UINT type, int cx, int cy );
	afx_msg void	OnVScroll( unsigned int, unsigned int, CScrollBar* );
	afx_msg void	OnHScroll( unsigned int, unsigned int, CScrollBar* );
	//void			GetWorldSize( double * xmax, double * xmin, double * ymax,
	//double * ymin, double * zmax, double * zmin );

	// Annotation Creation
	afx_msg void	OnAddTextAnnotation();
	afx_msg void	OnAddNodeDimensionAnnotation();
	// Annotation Editing and Removal
	afx_msg void	OnEditAnnotation();
	afx_msg void	OnDeleteAnnotation();

	// printing support
	bool			SetupPrintingPixelFormat( HDC hDC );
	BOOL			OnPreparePrinting( CPrintInfo* pInfo );

	// Windows Message Response Functions
	// Mouse Actions
	afx_msg void	OnMButtonUp( UINT nFlags, CPoint point );
	afx_msg void	OnMButtonDown( UINT nFlags, CPoint point );
	afx_msg void	OnMButtonDblClk( UINT nFlags, CPoint point );
	afx_msg void	OnLButtonUp( UINT type, CPoint point );
	afx_msg void	OnLButtonDown( UINT type, CPoint point );
	afx_msg void	OnLButtonDblClk( UINT type, CPoint point );
	afx_msg void	OnRButtonUp( UINT type, CPoint point );
	afx_msg void	OnRButtonDown( UINT type, CPoint point );
	afx_msg void	OnRButtonDblClk( UINT type, CPoint point );
	afx_msg void	OnMouseMove( UINT type, CPoint point );
	afx_msg void	OnNcLButtonDown( UINT u, CPoint p );
	afx_msg void	OnKillFocus(CWnd* pNewWnd) ;

	void CmUpArrowPress( void );
	void CmDownArrowPress( void );



	enum m_ETMouseState {
		MOUSE_NO_ACTION				= 0x000,	// used as a code to do nothing
		MOUSE_TEXT_DRAGGING			= 0x001,	// a text object is being dragged
		MOUSE_ZOOMBOX_WAITING		= 0x002,	// zoom box has not been initialized
		MOUSE_ZOOMBOX_DRAGGING		= 0x004,	// zoom box is being dragged
		MOUSE_SELECTIONBOX_WAITING	= 0x008,	// selection box has not been initialized
		MOUSE_SELECTIONBOX_DRAGGING = 0x010,	// selection box is being dragged
		MOUSE_PENCIL_WAITING		= 0x020,	// pencil has not been initialized
		MOUSE_PENCIL_DRAGGING		= 0x040,	// pencil is dragging
		MOUSE_NODE_DRAGGING			= 0x080,	// node is dragging
		MOUSE_MODEL_DRAGGING		= 0x100,	// dragging the model
		MOUSE_ANNOTATION_DRAGGING	= 0x200,	// dragging an annotation object
		MOUSE_LEGEND_DRAGGING		= 0x400		// dragging a result legend
	}; 

	bool	handleMouseSelection( ETGraphicObject go, CPoint point, UINT type, const CHitRecordCollection& rHitCollection  );
	bool	handleMouseBoxSelection( CHitRecordCollection& rHitCollection );
	bool	handleMouseDblClickSelection( ETGraphicObject go, CPoint point, UINT type, const CHitRecordCollection& rHitCollection );
	bool	handleMouseNodeDragButtonUp( bool* bBadDrop, bool* bCross, bool* bBadArea );
	bool	handleMouseVertexDragButtonUp( );
	bool	Mouse( m_ETMouseState m ) { return ((m_mouseState & m) != 0); };
	void	SetMouse( m_ETMouseState m ) { m_mouseState |= m; };
	void	ReleaseMouse( m_ETMouseState m ) { m_mouseState &= ~m; };
	bool	InSelectionBoxMode( const CPoint& deltaMouse, UINT type );
	bool	InNodeDragMode( const CPoint& deltaMouse, UINT type );
	bool	InDrawMode( const CPoint& deltaMouse, UINT type );
	LRESULT MsgFlyoverTipText( WPARAM /*ignored*/, LPARAM str );  // mouse over something worth mentioning
	BOOL	FlyOverModelWindow( CString& s );
	BOOL	FlyOverDesignWindow( CString& s );
	BOOL	FlyOverResultWindow( CString& s );

	//selection functions
	enum m_ETSelectType {  // the graphic objects we draw (CData derivatives)
		SELECT_ON,        
		SELECT_OFF, 
		SELECT_TOGGLE    
	};
	int  GetSelectionHitList( CPoint p, int selAreaX, int selAreaY, CHitRecordCollection& rRecordCollection );
	bool GetSnapped3DPoint( const CPoint& screenPt, CPoint3D& worldPt );
	bool Get3DPointFromScreen( const CPoint& screenPt, CPoint3D& worldPt, float depth = -1. );
	bool GetScreenPtFrom3DPoint( const CPoint3D& modelSpacePt, CPoint& screenPt );
	bool GetSnappedScreenPt( CPoint& point );
	bool GetXYPointForDXFTextExport( const CPoint& screenPt, CPoint3D& worldPt );

	void SelectAllObjects( ETGraphicObject t, CNameFilter filter, m_ETSelectType s, bool bCheckVisibility = true  );
	void SelectToggle( ETGraphicObject t, CData* object );
	void SelectAllObjectsInHitList( const CHitRecordCollection& rHitCollection );
	void UnselectAll( ETGraphicObject t = GRAPHIC_ALL_OBJECTS );
	void SetSelectedState( CData* pD, m_ETSelectType s );

	bool RightClickModelWindow( CMenu* pOtherMenu = NULL );
	bool RightClickResultWindow( CMenu* pOtherMenu = NULL );
	bool RightClickDesignWindow( CMenu* pOtherMenu = NULL );

	void DoubleClickModelWindow( ETGraphicObject go );
	void DoubleClickResultWindow( ETGraphicObject go, const CData* object );
	void DoubleClickDesignWindow( ETGraphicObject go, const CData* object );
	void QuickResultReport( CReport& report );

	// Keyboard Action
	afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags );
	// grid interaction
	afx_msg void OnSnapGridToNode();

	// add vertex
	afx_msg void OnAddAreaVertex();

	// Actual Drawing support
	BOOL OnEraseBkgnd( CDC* pDC );  // asked to erase the background
	void OnPrepareDC( CDC* pDC, CPrintInfo* pInfo  );
	
	void AddTitle( );  // put title information in the text object list  
	
	//JL added 1/16/2008
	CGraphicsModel m_graphicsModel;
	
	void DrawModel();
	void DrawGrid( bool bPointsOnly );
	void DrawGridPoints();
	void DrawOriginAxes();
	void DrawOriginAxes(bool bForSelection, CPoint p, int selSize);
	
	double GetDisplacementFactor();

	// miscellaneous drawing support
	void StartDragNode( const CPoint& delta );
	void StartDrawMode( const CPoint& p );
	void StartSelectionBox( const CPoint& p );
	void StartInvertedRectangle( CRect r, m_ETMouseState m = MOUSE_NO_ACTION );
	void StartInvertedLine( CPoint start, CPoint end, m_ETMouseState m = MOUSE_NO_ACTION );
	void RedrawInvertedLine();
	void StartInvertedPolygon( CPoint start, CPoint end, m_ETMouseState m = MOUSE_NO_ACTION );
	void RedrawInvertedPolygon();
	void StartZoomBox( const CPoint& p );
	void EndSelectionBox( const CPoint& p );
	void EndZoomBox( );
	void EndMemberDrawing( const CPoint& p );
	void EndCableDrawing( const CPoint& p );
	void EndPlateSideDrawing( const CPoint& p );
	void EndAreaSideDrawing( const CPoint& p );
	void EndFoundationSideDrawing( const CPoint& p );
	void MoveInvertedRectangle( CPoint lowerRight );
	void MoveInvertedLine( CPoint end );
	void MoveInvertedPolygonEndPoint( CPoint end );
	void EndInvertedRectangle( );
	void EndInvertedLine( m_ETMouseState m = MOUSE_NO_ACTION );
	void EndInvertedPolygon( m_ETMouseState m = MOUSE_NO_ACTION );
	void DrawInvertedPoint( const CPoint& p );
	void DrawInvertedLine( CPoint start, CPoint end );  // workhorse for rubber banding
	void DrawInvertedRectangle( CRect r ); 
	void DrawInvertedPolygon( CPoint end );
	void DrawInvertedText( CGraphicsText& text );
	void DrawInvertedAnnotation( CGraphicsAnnotationObject* pText );
	void DrawInvertedLegend( CGraphicsContourBar* pLegend );
	void GetAttachedMemberAngles( const CNode* pN );
	const CMember* GetMemberWithThis2DOrientation( const CVector3D& v, const CNode* pN );
	
	// primitive drawing support
	bool DrawLine( CPoint3D p1, CPoint3D p2, COLORREF c ); 
	ETGraphicObject elementToGraphicObject( ETElement e );
	const CNameFilter& nameFilterForGraphicObject( ETGraphicObject o );	

	//dragging functions
	void DragText( const CPoint& delta );
	void DragLegend( const CPoint& delta );

	void SetViewOnNode( const CNode* /*pN*/, ETOrientation /*or*/, double /*scale*/ ) { ASSERT( FALSE ); };  // work needed
	void SetViewOnMember( const CMember* /*pM*/, ETOrientation /*or*/, double /*scale*/ ) { ASSERT( FALSE ); };

	void ShowOnlyLoadCaseTitle() { ASSERT( FALSE ); };  // work needed

	// engine data timing support - may have to expand to several data types like FEM engine in the future
	void  UpdateDataTime( ) { m_engineDataTime = clock(); };
	clock_t  DataTime( ) { return m_engineDataTime; };

private:  // our own data

	//basic window functionality
	bool			m_bActive;
	bool			m_bClosing;
	bool			m_bPrinting; 
	bool			m_bInActivate;
	bool			m_bFirstActivate;
	CString			m_windowTitle;
	time_t			m_time;
	
	// used to override any standard logic in OnDraw that might cause the 
	// window to not be draw (currently used to force a repaint when the
	// user is drawing a plate or memeber but also needs to zoom or pan)
	bool			m_bForceDraw;

	//use for when the user clicks in the little axis box
	bool			m_bForceRotate;

	//our window filter
	CWindowFilter		m_Filter;
	
	//mouse data
	int		m_MouseSinceActivate;

	// Our OpenGL window variables
	HDC		m_hDC;
	BOOL	m_doubleBuffered;
	HGLRC	m_hRC; //Rendering Context
	CDC*	m_pDC;  //Device Context
	bool    m_bHaveRenderingContext;
	
	//printing
	HDC			m_hMemDC;
	HGLRC		m_hMemRC;
	BITMAPINFO	m_bmi;
	LPVOID		m_pBitmapBits;
	HBITMAP		m_hDib;
	CSize		m_szPage;	
	int			m_printScale;

	//our camera
	COrthoCamera m_Camera;

	//the little rotation box in the lower left of the screen
	CGraphicsRotationCube m_rotationCube;

	//our context pop-up menu
	CMenu		m_PopupMenu;

	//everybody's favorite, the tool-tips
	CXInfoTip	m_MyTip;

	CGraphicsContourBar*		m_pDraggingLegend;

	// Data objects needed for functionality
	CVAGrid						mGrid;
	CGraphicsAnnotationManager	m_AnnotationManager;
	
	CGraphicsText	m_TitleText;

	// the following variables are used for keeping track of the mouse actions
	// and states
	int			  m_mouseState;
	bool		  m_bHasBeenDrawnOnce;
	bool		  mDrawGeneration;
	CRect         m_DraggingRect;
	CPoint3D	  m_DragStart3D;
	CPoint3D	  m_DragEnd3D;
	CPoint3D      m_Current3DPt;
	CPoint3D	  m_CurSnapped3DPt;	
	CPoint		  m_CurSnapped2DPt;	
	CPoint		  m_DragStart2D;
	CPoint		  m_DragEnd2D;
	CPoint		  m_LastMouseButton;
	CPoint		  m_LastMouseMove;
	
	int						mSideBeingDrawn;	// the side of the element currently being drawn
	std::vector<CPoint>		mElement2D;			// the element being drawn client coordinates
	std::vector<CPoint3D>	mElement3D;			// model coordinates for plates, areas, and foundations
	
	CGLInvertedLine			m_invertedLine;
	CGLInvertedPolygon		m_invertedPoly;
	
	int				mMouseSinceActivate;
	CNode*			mNodeBeingDragged;		// when dragging a node, this is the guy
	CCoordinate*	m_vertexBeingDragged;	// when dragging an area vertex....
	bool			mCheckExtentChange;		// gets set by outside world when model size has changed

	bool					m_bFlyoverStatusSaved;	// status of flyover display before we shut it off in a mouse-down
	//CGraphicsAnnotationObject*	m_pEditAnnotation;		// the text object being modified

	//display lists - just one per model entity for now.
	CGraphicsDisplayList m_nodeDispList;
	CGraphicsDisplayList m_memberDispList;
	CGraphicsDisplayList m_planarDispList;

	//temporary member angles for member extension drawing
	CVector3DArray	m_MemberVectors;

	//optional items to draw
	COrientedBox	m_CurrentLayerBox;

	// time reference for data which engine is sensitive to
	clock_t			m_engineDataTime;

	int m_nViewCountID;

	CEdit			m_analysisTextWindow;

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	friend class CReportView;
	friend class CGraphicsEngine;
	friend class CFindWnd;
protected:
	DECLARE_MESSAGE_MAP()
};

extern int gGraphicWindowCount;

// CAnnotationDialog dialog

class CAnnotationDialog : public CDialog
{
	DECLARE_DYNAMIC(CAnnotationDialog)

public:
	CAnnotationDialog( CGraphicsAnnotationObject& rAnn, bool bAllowLeader, CWnd* pParent = NULL, bool bIsNewAnnotation = true );   // standard constructor
	virtual ~CAnnotationDialog();

	// Dialog Data
	enum { IDD = IDD_ANNOTATION_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	CEdit mTextControl;
	CGraphicsAnnotationObject& m_Annotation;
	virtual BOOL OnInitDialog();
	bool m_bAllowLeader;
	bool m_bIsNewAnnotation;
	CButton mArrowLeader;
	CButton mLineLeader;
};