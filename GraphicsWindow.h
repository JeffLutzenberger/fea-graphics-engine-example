#pragma once

#include "GraphicsCamera.h"
#include "GraphicsGrid.h"
#include "Graphics/GraphicsObjects.h"
#include "Core/Graphics/GraphicsDisplayList.h"
#include "GraphicsEngine.h"

#include "Core/Graphics/GraphicsHelpers.h"
#include "Core/Graphics/GraphicsHelperClasses.h"
#include "Graphics/HitRecord.h"
#include "Data/Graphics/GraphicsExtrusion.h"
#include "Data/Graphics/GraphicsMember.h"
#include "Data/Graphics/GraphicsCable.h"
#include "VertexArrayManager.h"
#include "dialog/WindowTip.h"
#include "Math/Analytic3d.h"

#include <map>

//#include "Data/Enumdata.h" //graphic detail ET

#include "GraphicView.h"  // base class

#define MAX_INVERTED_LINES 20			// number of lines that can be managed and inverted during drawing polygons (and rectangles)
// legacy includes
#include "ViewData.h"
#include "WinFiltr.h"
//typedef TPointerArray<TViewingData> TViewDataArray;		// "Stack"
//typedef TPointerArray<CWindowFilter> TViewFilterArray;		// "Stack"

// CGraphicView view

class CRigidDiaphragm;
class CNode;
class CMember;
class CPlanar;
class CSpring;
class CCable;
class CElement;
class CNodalLoad;
class CMemberLoad;
class CMovingLoad;
class CPlanarLoad;
class CServiceCase;
class CData;

//class CGraphicView : public CScrollView
//{
//	DECLARE_DYNCREATE(CGraphicView)
//
//public:
//	// CMainFrame access here
//	CGraphicView();           // constructor used by dynamic creation
//	virtual ~CGraphicView();
//
//	//basic window functions
//	void			activate( void );
//	void			deactivate( void );
//	bool			isActive() { return m_bActive; };
//	bool			isClosing() { return m_bClosing; };
//	void			WriteCoorindatesToStatusBar( const CPoint3D& p );
//	ETWindowType	isA() { return m_Filter.windowType; }
//	void			EscapeKey();  
//	void			ViewManagerChange();
//
//	//current service cases - are all of these necessary???????
//	CServiceCase*	serviceCase( void );
//	CServiceCase*	projectCurrentLoadCase( void );
//	const CResultCase* CurrentResultCase( );
//	void CurrentResultCase( const CResultCase* pRC ); 
//	void SetLoadCase( CServiceCase* pL );
//
//	//what are we drawing
//	ETElement		currentDrawMode(){ return m_Filter.drawMode; };
//	void			currentDrawMode( ETElement what );
//
//	//public view support
//	void			ZoomView();
//	void			SetPresetView( double rotations[3] );
//
//	//public visibility queries
//	bool			isVisible( const CNode& node );  
//	bool			isVisible( const CMember& member );
//	bool			isVisible( const CPlanar& planar );
//	bool			isVisible( const CSpring& spring );
//
//	//get the 
//	CVAGrid&		windowGrid() { return mGrid; }
//
//	// for layer support - is there a better way?
//	void			DrawBoundingBox( const COrientedBox& _box );
//	void			DrawLayerBoundingBox( );
//	bool			m_bDrawCurrentLayerBox;
//	void			SetCurrentLayerBox( const COrientedBox& _box ){ m_CurrentLayerBox = _box; };
//
//protected:
//
//	// Window creation/startup/activation
//	BOOL			PreCreateWindow( CREATESTRUCT& cs );
//	BOOL			PreTranslateMessage( MSG* pMsg );
//	void			OnInitialUpdate();     // first time after construct
//	virtual void	OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
//	void			SetWindowCaption( LPCSTR = NULL );
//
//	// OpenGL initialization
//	BOOL			InitOpenGL( );
//	BOOL			SetupPixelFormat( );
//	void			QueryGraphicsCard( );
//	
//	// Creation and destruction actions
//	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
//	afx_msg void	OnDestroy();
//
//	// Sizing & other border actions
//	afx_msg void	OnSize( UINT type, int cx, int cy );
//	afx_msg void	OnVScroll( unsigned int, unsigned int, CScrollBar* );
//	afx_msg void	OnHScroll( unsigned int, unsigned int, CScrollBar* );
//	CScrollBar*		GetScrollBarCtrl(int nBar) const;
//	void			GetWorldSize( double * xmax, double * xmin, double * ymax,
//								double * ymin, double * zmax, double * zmin );
//
//	// Printing support
//	bool			SetupPrintingPixelFormat( HDC hDC );
//	BOOL			OnPreparePrinting( CPrintInfo* pInfo );
//	afx_msg void	OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
//	afx_msg void	OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
//	afx_msg void	OnPrint(CDC* pDC, CPrintInfo* pInfo );
//	afx_msg void	OnFilePrint();
//	afx_msg void	OnFilePrintPreview();
//
//	// Annotation Creation
//	afx_msg void	OnAddTextAnnotation();
//	afx_msg void	OnAddNodeDimensionAnnotation();
//
//	// IES Project and VA messages will come in here
//	virtual void			OnUpdate( CView *pSender, LPARAM lHint, CObject *pHint );
//	LRESULT					ProjectChange( WPARAM, LPARAM );
//	LRESULT					StatusBarResultCaseChange( WPARAM, LPARAM lp ); 
//	void					SetServiceCase( CServiceCase* pSc );
//	void					SetResultCase( CLoadCase* pLc );
//	void					UpdateResults( );
//	LRESULT					MsgSaveView( WPARAM /*ignored*/, LPARAM /*ignored*/ );
//
//	// Windows Message Response Functions
//	// Mouse Actions
//	afx_msg BOOL	OnMouseWheel( UINT nFlags, short zDelta, CPoint pt );
//	afx_msg void	OnMButtonUp( UINT nFlags, CPoint point );
//	afx_msg void	OnMButtonDown( UINT nFlags, CPoint point );
//	afx_msg void	OnMButtonDblClk( UINT nFlags, CPoint point );
//	afx_msg void	OnLButtonUp( UINT type, CPoint point );
//	afx_msg void	OnLButtonDown( UINT type, CPoint point );
//	afx_msg void	OnLButtonDblClk( UINT type, CPoint point );
//	afx_msg void	OnRButtonUp( UINT type, CPoint point );
//	afx_msg void	OnRButtonDown( UINT type, CPoint point );
//	afx_msg void	OnRButtonDblClk( UINT type, CPoint point );
//	afx_msg void	OnMouseMove( UINT type, CPoint point );
//	afx_msg void	OnNcLButtonDown( UINT u, CPoint p );
//	afx_msg void	OnKillFocus(CWnd* pNewWnd) ;
//
//	enum m_ETMouseState {
//		MOUSE_NO_ACTION				= 0x000,	// used as a code to do nothing
//		MOUSE_TEXT_DRAGGING			= 0x001,	// a text object is being dragged
//		MOUSE_ZOOMBOX_WAITING		= 0x002,	// zoom box has not been initialized
//		MOUSE_ZOOMBOX_DRAGGING		= 0x004,	// zoom box is being dragged
//		MOUSE_SELECTIONBOX_WAITING	= 0x008,	// selection box has not been initialized
//		MOUSE_SELECTIONBOX_DRAGGING = 0x010,	// selection box is being dragged
//		MOUSE_PENCIL_WAITING		= 0x020,	// pencil has not been initialized
//		MOUSE_PENCIL_DRAGGING		= 0x040,	// pencil is dragging
//		MOUSE_NODE_DRAGGING			= 0x080,	// node is dragging
//		MOUSE_MODEL_DRAGGING		= 0x100,	// dragging the model
//		MOUSE_ANNOTATION_DRAGGING	= 0x200		// dragging an annotation object
//	}; 
//
//	bool	handleMouseSelection( ETGraphicObject go, CPoint point, UINT type, const CHitRecordCollection& rHitCollection  );
//	bool	handleMouseBoxSelection( const CHitRecordCollection& rHitCollection );
//	bool	handleMouseDblClickSelection( ETGraphicObject go, CPoint point, UINT type, const CHitRecordCollection& rHitCollection );
//	bool	handleMouseNodeDragButtonUp( bool* bBadDrop, bool* bCross, bool* bBadArea );
//	bool	Mouse( m_ETMouseState m ) { return (m_mouseState & m); };
//	void	SetMouse( m_ETMouseState m ) { m_mouseState |= m; };
//	void	ReleaseMouse( m_ETMouseState m ) { m_mouseState &= ~m; };
//	bool	InSelectionBoxMode( const CPoint& deltaMouse, UINT type );
//	bool	InNodeDragMode( const CPoint& deltaMouse, UINT type );
//	bool	InDrawMode( const CPoint& deltaMouse, UINT type );
//	LRESULT MsgFlyoverTipText( WPARAM /*ignored*/, LPARAM str );  // mouse over something worth mentioning
//
//	const CNode*			mouseOverNode( const CHitRecordCollection& rHitCollection );
//	const CMember*			mouseOverMember( const CHitRecordCollection& rHitCollection );
//	const CPlanar*			mouseOverPlate( const CHitRecordCollection& rHitCollection );
//	const CSpring*			mouseOverSpring( const CHitRecordCollection& rHitCollection );
//	const CCable*			mouseOverCable( const CHitRecordCollection& rHitCollection );
//	const CNodalLoad*		mouseOverNodalLoad( const CHitRecordCollection& rHitCollection );
//	const CMemberLoad*		mouseOverMemberLoad( const CHitRecordCollection& rHitCollection );
//	const CPlanarLoad*		mouseOverPlateLoad( CPoint p );
//	const CMovingLoad*		mouseOverMovingLoad( CPoint p );
//	const CRigidDiaphragm*	mouseOverRigidDiaphragm( const CHitRecordCollection& rHitCollection  );
//	const CPoint3D*			mouseOverGridPoint( const CHitRecordCollection& rHitCollection );
//
//	//selection functions
//	enum m_ETSelectType {  // the graphic objects we draw (CData derivatives)
//		SELECT_ON,        
//		SELECT_OFF, 
//		SELECT_TOGGLE    
//	};
//	int  GetSelectionHitList( CPoint p, int selAreaX, int selAreaY, CHitRecordCollection& rRecordCollection );
//	bool GetSnapped3DPoint( const CPoint& screenPt, CPoint3D& worldPt );
//	bool Get3DPointFromScreen( const CPoint& screenPt, CPoint3D& worldPt, float depth = -1. );
//	bool GetScreenPtFrom3DPoint( const CPoint3D& modelSpacePt, CPoint& screenPt );
//	bool GetSnappedScreenPt( CPoint& point );
//	void SelectAllObjects( ETGraphicObject t, CNameFilter filter, m_ETSelectType s  );
//	void SelectToggle( ETGraphicObject t, CData* object );
//	void SelectAllObjectsInHitList( const CHitRecordCollection& rHitCollection );
//	void UnselectAll( ETGraphicObject t = GRAPHIC_ALL_OBJECTS );
//	void SetSelectedState( CData* pD, m_ETSelectType s );
//	int  TotalSelected( void );
//
//	//mouse point functions
//	const CElement* mouseOverElement( CPoint p, ETElement e );
//	const CData*	mouseOverGraphicObject( CPoint p, ETGraphicObject go, const CHitRecordCollection& rHitCollection  );
//
//	// Keyboard Action
//	afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
//
//	// grid interaction
//	afx_msg void OnSnapGridToNode( void );
//
//	// Actual Drawing support
//	void OnDraw(CDC* pDC);      // overridden to draw this view
//	void DrawForPrinter( CDC* pDC );
//	BOOL OnEraseBkgnd( CDC* pDC );  // asked to erase the background
//	void OnPrepareDC( CDC* pDC, CPrintInfo* pInfo  );
//	void AddTitle();  // put title information in the text object list  
//	void DrawSupports( void );
//	void DrawNodes( void );
//	void DrawMembers( void );
//	void SetupAndRenderMembers( ETGraphicsDrawStyle style = GRAPHIC_SOLID, int nDivPts = 0, bool bDisplaced = false );
//	void DrawCables( void );
//	void SetupAndRenderCables( ETGraphicsDrawStyle style = GRAPHIC_SOLID, int nDivPts = 0, bool bDisplaced = false );
//	void DrawPlates( void );
//	void SetupAndRenderPlates( ETGraphicsDrawStyle style = GRAPHIC_SOLID, int nDivPts = 0, bool bDisplaced = false );
//	void DrawRigidDiaphragms( void );
//	void SetupAndRenderRigidDiaphragms( ETGraphicsDrawStyle style = GRAPHIC_SOLID, int nDivPts = 0, bool bDisplaced = false );
//	void DrawAreas( void );
//	void SetupAndRenderAreas( ETGraphicsDrawStyle style = GRAPHIC_SOLID, int nDivPts = 0, bool bDisplaced = false );
//	void DrawSprings( void );
//	void SetupAndRenderSprings( ETGraphicsDrawStyle style = GRAPHIC_LINE, int nDivPts = 0, bool bDisplaced = false );
//	void DrawNodalLoads( void );
//	void DrawMemberLoads( void );
//	void DrawMovingLoads( void );
//	void DrawPlateLoads( void );
//	void DrawGrid( const CPoint3D& selPt );
//	void DrawGridPoints( void );
//	double GetDistortionFactor( void );
//	
//	// individual entity drawing
//	//void DrawSupport( const CNode* pN, double distortionFactor = 0. );
//	void DrawNode( const CNode* pN, double distortionFactor = 0. );
//	void DrawMember( const CMember* pM, int nDivPts = 0, bool bDisplaced = false, double distortionFactor = 0. );
//	void DrawPlate( const CPlanar* pP, bool bDisplaced = false, double distortionFactor = 0. );
//	void DrawSpring( const CSpring* pS, bool bDisplaced = false, double distortionFactor = 0. );
//	void DrawCable( const CCable* pC, bool bDisplaced = false, double distortionFactor = 0. );
//	void DrawRigidDiaphragm( const CRigidDiaphragm* pR, bool bDisplaced = false );
//	void DrawArea( const CArea* pA, bool bDisplaced = false );
//	void DrawNodalLoad( const CNodalLoad* pNL, bool bShowSelectedState = false );
//	void DrawMemberLoad( const CMemberLoad* pML, bool bShowSelectedState = false );
//	void DrawMovingLoad( const CMovingLoad* pMV, bool bShowSelectedState = false );
//	void DrawPlateLoad( const CPlanarLoad* pPL, bool bShowSelectedState = false );
//	void DrawObjectSelectedState( ETGraphicObject t, const CData* pD );
//	void DrawSupport( const CNode* pN, bool bShowSelectedState = false );
//	void DrawGroundPlane( );
//
//	//text functions
//	void CreateText();
//	void createNodeText();
//	void createMemberText();
//	void createCableText();
//	void createPlateText();
//	void createNodeLoadText();
//	void createMemberLoadText();
//	void createMovingLoadText();
//	void GetMemberPropertyBuffers( char* buf1, char* buf2, const CMember* pM );
//	void GetCablePropertyBuffers( char* buf1, char* buf2, const CCable* pC );
//	void GetPlatePropertyBuffers( char* buf1, char* buf2, const CPlanar* pP );
//	void addToBuffers( char* buf1, char* buf2, int size, int* fillNumber, const char* str );
//	
//	// miscellaneous drawing support
//	void StartDragNode( const CPoint& delta );
//	void StartDrawMode( const CPoint& p );
//	void StartSelectionBox( const CPoint& p );
//	void StartInvertedRectangle( CRect r, m_ETMouseState m = MOUSE_NO_ACTION );
//	void StartInvertedLine( CPoint start, CPoint end, m_ETMouseState m = MOUSE_NO_ACTION );
//	void StartInvertedPolygon( CPoint start, CPoint end, m_ETMouseState m = MOUSE_NO_ACTION );
//	void StartZoomBox( const CPoint& p );
//	void EndSelectionBox( const CPoint& p );
//	void EndZoomBox( const CPoint& p );
//	void EndMemberDrawing( const CPoint& p );
//	void EndCableDrawing( const CPoint& p );
//	void EndPlateDrawing( const CPoint& p );
//	void EndPlateSideDrawing( const CPoint& p );
//	void MoveInvertedRectangle( CPoint lowerRight );
//	void MoveInvertedLine( CPoint end );
//	void MoveInvertedPolygonEndPoint( CPoint end );
//	void EndInvertedRectangle( m_ETMouseState m = MOUSE_NO_ACTION );
//	void EndInvertedLine( m_ETMouseState m = MOUSE_NO_ACTION );
//	void EndInvertedPolygon( m_ETMouseState m = MOUSE_NO_ACTION );
//	void DrawInvertedPoint( const CPoint& p );
//	void DrawInvertedLine( CPoint start, CPoint end );  // workhorse for rubber banding
//	void DrawInvertedRectangle( CRect r ); 
//	void DrawInvertedPolygon( CPoint end );
//	void DrawInvertedText( CGraphicsTextObject* pText );
//	void DrawInvertedAnnotation( CGraphicsAnnotationObject* pText );
//	void GetAttachedMemberAngles( const CNode* pN );
//	const CMember* GetMemberWithThis2DOrientation( const CVector3D& v, const CNode* pN );
//	void StartDrawing( void );   // makes a device context if there is not one currently
//	void EndDrawing( void );  // disables device context created by StartDrawing if needed
//	void SetColor( COLORREF c );  // setup the current drawing color
//	COLORREF InverseColor( COLORREF c );
//	
//	// primitive drawing support
//	bool DrawLine( CPoint3D p1, CPoint3D p2, COLORREF c ); 
//	ETGraphicObject elementToGraphicObject( ETElement e );
//	CNameFilter nameFilterForGraphicObject( ETGraphicObject o );	
//	
//	//dragging functions
//	void DragText( const CPoint& delta );
//	void DragAnnotation( const CPoint& delta );
//
//	//pan, zoom and rotate
//	void DragModel( const CPoint& delta );
//	void RotateModel( const CPoint& delta );
//	void Rotate();
//	void RotateX( double inc );
//	void RotateY( double inc );
//	void RotateZ( double inc );
//	void ZoomDisplayToRectangle( CRect r );
//	void SetWindowToExtents( void );
//	void ViewZoomInAtPoint( CPoint _pt );
//	void ViewZoomOutAtPoint( CPoint _pt );
//	void ViewZoomAtPoint( CPoint _pt, double scale );
//	void UpdateCamera( ){ m_Camera.UpdateCamera(); };
//
//	void SetViewOnNode( const CNode* /*pN*/, ETOrientation /*or*/, double /*scale*/ ) { ASSERT( FALSE ); };  // work needed
//	void SetViewOnMember( const CMember* /*pM*/, ETOrientation /*or*/, double /*scale*/ ) { ASSERT( FALSE ); };
//	
//	void ShowOnlyLoadCaseTitle() { ASSERT( FALSE ); };  // work needed
//
//	// window status information
//	void SetExtentRedraw( void ) { mCheckExtentChange = true; };
//	bool cursorInWindow( CPoint point, bool isClientPoint );
//
//	//member extrusion functions
//	void RemoveExtrusion( const CData* pData );
//	void RemoveAllExtrusions( );
//	void RemoveDisplacedExtrusions( );
//
//	//results
//	bool SetupResults( void );
//	bool UpdateMemberTextureCoords( double min, double max, int theGraphType );
//
//	// engine data timing support - may have to expand to several data types like FEM engine in the future
//	void  UpdateDataTime( ) { m_engineDataTime = clock(); };
//	clock_t  DataTime( ) { return m_engineDataTime; };
//
//private:  // our own data
//
//	//basic window functionality
//	bool			m_bActive;
//	bool			m_bClosing;
//	bool			m_bPrinting; 
//	bool			m_bInActivate;
//	bool			m_bFirstActivate;
//	CString			m_windowTitle;
//	time_t			m_time;
//
//
//	//our window filter
//	CWindowFilter		m_Filter;
//	TViewFilterArray	m_FilterStack;
//
//	//our viewing data - necessary????
//	TViewingData		m_View;
//	TViewDataArray		m_ViewStack;
//
//	//mouse data
//	int		m_MouseSinceActivate;
//
//	// Our OpenGL window variables
//	HDC		m_hDC;
//	BOOL	m_doubleBuffered;
//	HGLRC	m_hRC; //Rendering Context
//	CDC*	m_pDC;  //Device Context
//	bool    m_bHaveRenderingContext;
//
//	//printing
//	HDC			m_hOldDC;
//	HDC			m_hMemDC;
//	HGLRC		m_hOldRC;
//	HGLRC		m_hMemRC;
//	BITMAPINFO	m_bmi;
//	LPVOID		m_pBitmapBits;
//	HBITMAP		m_hDib;
//	CSize		m_szPage;	
//
//	//our camera
//	CGraphicsCamera m_Camera;
//
//	//our context pop-up menu
//	CMenu		m_PopupMenu;
//
//	//everybody's favorite, the tool-tips
//	CXInfoTip	m_MyTip;
//
//	// Data objects needed for functionality
//	CVAGrid  mGrid;
//	CGraphicsTextManager m_TextManager;
//	CGraphicsAnnotationManager m_AnnotationManager;
//	CVertexArrayManager m_VertexArrayManager;
//
//	// the following variables are used for keeping track of the mouse actions
//	// and states
//	int			  m_mouseState;
//	bool		  m_bHasBeenDrawnOnce;
//	bool		  mDrawGeneration;
//	CRect         m_DraggingRect;
//	CPoint3D	  m_DragStart3D;
//	CPoint3D	  m_DragEnd3D;
//	CPoint3D      m_Current3DPt;
//	CPoint		  m_CurSnapped2DPt;	
//	CPoint		  m_DragStart2D;
//	CPoint		  m_DragEnd2D;
//	CPoint		  m_LastMouseButton;
//	CPoint		  m_LastMouseMove;
//	int			  mSideBeingDrawn;	// the side of the element currently being drawn
//	CPoint		  mElement2D[5];	// the element being drawn client coordinates
//	CPoint3D	  mElement3D[5];	// the element being drawn model coordinates
//	int			  mMouseSinceActivate;
//	CNode*		  mNodeBeingDragged;   // when dragging a node, this is the guy
//	bool		  mCheckExtentChange;  // gets set by outside world when model size has changed
//
//	bool			m_bUseBlending;
//	bool			m_bFlyoverStatusSaved;		// status of flyover display before we shut it off in a mouse-down
//	CGraphicsTextObject* m_pDraggingText;  // the text object being dragged
//	CGraphicsAnnotationObject* m_pDraggingAnnotation;  // the text object being dragged
//
//	//CGLInvertedPoint	m_selGridPoint;
//	CGLInvertedLine		m_invertedLine;
//	CGLInvertedLine		m_invertedLineArray[MAX_INVERTED_LINES];
//	CGLInvertedPolygon	m_invertedPoly;
//
//	//display lists - just one per model entity for now.
//	CGraphicsDisplayList m_nodeDispList;
//	CGraphicsDisplayList m_memberDispList;
//	CGraphicsDisplayList m_planarDispList;
//
//	//map of extrusion - huge speed-up!
//	//maps a member's index to the extrusion
//	//ExtrusionMap m_MemberExtrusionMap;
//	GraphicsMemberMap m_GraphicsMemberMap;
//	GraphicsMemberMap m_GraphicsDispMemberMap;
//	GraphicsCableMap m_GraphicsCableMap;
//	GraphicsCableMap m_GraphicsDispCableMap;
//
//	//temporary member angles for member extension drawing
//	CVector3DArray	m_MemberVectors;
//
//	//optional items to draw
//	COrientedBox			m_CurrentLayerBox;
//
//	// background thread processor
//	CGraphicsEngine	m_backgroundEngine;
//	// time reference for data which engine is sensitive to
//	clock_t			m_engineDataTime;
//
//#ifdef _DEBUG
//	virtual void AssertValid() const;
//	virtual void Dump(CDumpContext& dc) const;
//#endif
//
//	friend class CReportView;
//	friend class CGraphicsEngine;
//protected:
//	DECLARE_MESSAGE_MAP()
//};
