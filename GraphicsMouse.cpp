// GraphicsMouse.cpp : implementation file
// Drawing support

#include "stdafx.h"
#pragma hdrstop

#include "GraphicView.h"

#include "model.h"
#include "project.h"
#include "Design/GroupManager.h"
#include "Design/mesh.h"
#include "servcase.h"

#include "graphics/GridManager.h"

#include "User/message.h"
#include "main.h"  // theApp
#include "MainFrm.h"  // CMainFrame
#include "User/control.h"

#include "Dialog/DlgUtil.h"  // IsContextHelpUp()
#include "User/UserUtil.h"
#include "User/CombinedMembers.h"

#include "UI/ChoiceDlg.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

// local menu id's
const int CM_VIEW_CREATE_TEXT_ANNOTATION = 13301;
const int CM_VIEW_CREATE_DIMENSION_ANNOTATION = 13302;
const int CM_VIEW_SNAP_GRID_TO_NODE = 13303;
const int CM_MODEL_ADD_AREA_VERTEX = 13304;
const int CM_VIEW_EDIT_ANNOTATION = 13305;
const int CM_VIEW_DELETE_ANNOTATION = 13306;

BEGIN_MESSAGE_MAP(CGraphicView, CScrollView)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
	ON_COMMAND(CM_VIEW_CREATE_TEXT_ANNOTATION, OnAddTextAnnotation) 
	ON_COMMAND(CM_VIEW_CREATE_DIMENSION_ANNOTATION, OnAddNodeDimensionAnnotation) 
	ON_COMMAND(CM_VIEW_EDIT_ANNOTATION, OnEditAnnotation) 
	ON_COMMAND(CM_VIEW_DELETE_ANNOTATION, OnDeleteAnnotation) 
	ON_COMMAND( CM_VIEW_SNAP_GRID_TO_NODE, OnSnapGridToNode )
	ON_COMMAND( CM_MODEL_ADD_AREA_VERTEX, OnAddAreaVertex )
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_CREATE()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEWHEEL()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_MESSAGE( FLYOVER_WINDOW_TEXT_REQUEST_MESSAGE, MsgFlyoverTipText )
	ON_MESSAGE( CHANGE_MESSAGE, ProjectChange )
	ON_MESSAGE( WM_HELPHITTEST, OnHelpHitTest )

	//ON_MESSAGE( SELCHANGE_MESSAGE, OnSelChange )
END_MESSAGE_MAP()


// Mouse support
void CGraphicView::OnMouseMove( UINT type, CPoint _point )
{
	if( !m_bHasBeenDrawnOnce )  // we do nothing until some of the graphics has initialized otherwise things like the grid fail
		return;

	VAHelpPane.updateActiveWindow( m_Filter.windowType );

	if( IsContextHelpUp() )
		m_MyTip.Hide();

	// how far has the mouse moved since the button operation
	CPoint mouseButtonDelta = _point - m_LastMouseButton;
	// how far since the last mouse move
	CPoint mouseDelta = _point - m_LastMouseMove;

	m_LastMouseMove = _point;


	//first do anything that does not require hit testing on the model
	if( Mouse( MOUSE_MODEL_DRAGGING ) )
	{
		m_graphicsModel.StartReducedDrawing();
		if( ( type & MK_CONTROL ) || ( type & MK_SHIFT ) || m_bForceRotate )
		{
			// rotate model
			RotateModel( mouseDelta );
			SetCursor( theApp.LoadCursor( ROTATE_CURSOR ) ); 
			if( Mouse( MOUSE_PENCIL_DRAGGING ) ) m_bForceDraw = true;
			return;
		}
		else
		{
			DragModel( mouseDelta );
			SetCursor( theApp.LoadCursor( PAN_CURSOR ) ); 
			if( Mouse( MOUSE_PENCIL_DRAGGING ) ) m_bForceDraw = true;
			return;
		}
	}
	// convert the screen coordinates (point) to a 3-d model coordinate including jumping to nearest node or grid point
	CPoint snappedPoint( _point );  // this one gets "snapped"
	GetSnappedScreenPt( snappedPoint );
	CPoint3D model3D = CPoint3D::undefined_point;

	if( GetSnapped3DPoint( _point, model3D ) && m_Current3DPt != model3D )
	{
		if( Mouse( MOUSE_PENCIL_DRAGGING ) ) {  // show offset points
			if( m_Filter.drawMode == DRAW_MEMBER ) {
				CPoint3D offset = model3D - m_DragStart3D;
				WriteCoordinatesToStatusBar( offset );
			}
			else if( mSideBeingDrawn > 0 && mSideBeingDrawn < mElement3D.size() ) {
				CPoint3D offset = model3D;
				model3D.x -= mElement3D[mSideBeingDrawn].x;
				model3D.y -= mElement3D[mSideBeingDrawn].y;
				model3D.z -= mElement3D[mSideBeingDrawn].z;
				WriteCoordinatesToStatusBar( offset );
			}
			else
				WriteCoordinatesToStatusBar( model3D );
		}
		else
			WriteCoordinatesToStatusBar( model3D );
		m_CurSnapped3DPt = model3D;
		m_CurSnapped2DPt = snappedPoint;
	}

	m_Current3DPt = model3D;

	CPoint safeLastMouseMove = m_LastMouseMove;
	
	if( Mouse( MOUSE_TEXT_DRAGGING ) && m_TitleText.Dragging() )
	{
		DragText( mouseDelta );
	}
	else if( Mouse( MOUSE_LEGEND_DRAGGING ) && m_pDraggingLegend )
	{
		DragLegend( mouseDelta );
	}
	
	//the zoom box gets started in the OnLButtonDown function
	else if( Mouse( MOUSE_ZOOMBOX_DRAGGING ) )
	{
		MoveInvertedRectangle( _point );
	}
	else if( InSelectionBoxMode( mouseButtonDelta, type ) ) 
	{
		StartSelectionBox( _point );
	}
	else if( Mouse( MOUSE_SELECTIONBOX_DRAGGING ) )
	{
		MoveInvertedRectangle( _point );
	}
	else if( InNodeDragMode( mouseButtonDelta, type ) )
	{
		//TRACE("Start Extending Member\n");
		StartDragNode( mouseButtonDelta );
	}
	else if( InDrawMode( mouseButtonDelta, type ) )
	{
		//TRACE("StartDrawMode\n");
		StartDrawMode( _point );
	}
	else if( Mouse( MOUSE_PENCIL_DRAGGING ) )
	{
		// only drag if we've move a little bit, say 2 pixels
		//if( abs(mouseDelta.x) + abs(mouseDelta.y) > 2 )
		//{
			ETDrawMode mode = m_Filter.drawMode;
//			if( m_Filter.windowType == POST_WINDOW && theModel.elements( PLANAR_ELEMENT ) > 0 )
//				mode = DRAW_STRESSPATH;
			switch( mode )
			{
//			case DRAW_STRESSPATH:
			case DRAW_MEMBER:
			case DRAW_CABLE:
				MoveInvertedLine( snappedPoint );
				break;
			case DRAW_PLANAR:
			case DRAW_AREA:
				MoveInvertedPolygonEndPoint( snappedPoint );
				break;
			default:
				//ASSERT( false );
				break;
			}
		//}
		//else
		//	m_LastMouseMove = safeLastMouseMove;
	}
	else if( Mouse( MOUSE_NODE_DRAGGING ) ) 
	{
		//TRACE( "Mouse node dragging\n" );
		//snap point to angles
		CVector3D minVector( 0.0, 0.0, 0.0 );
		//double maxDot = -1.0;
		CPoint3D p1( snappedPoint.x, snappedPoint.y, 0. );
		CPoint3D p2( m_LastMouseButton.x, m_LastMouseButton.y, 0. );
		CVector3D dragVector = CVector3D( p1, p2 );
		//for( int i = 1; i <= m_MemberVectors.size(); i++ )
		//{
		//	//get min angle between inverted line and member vector
		//	CVector3D memberVector = m_MemberVectors[i];
		//	double dotprod = memberVector.dot( dragVector.normalize() );
		//	if( dotprod > maxDot )
		//	{
		//		maxDot = dotprod;
		//		minVector = m_MemberVectors[i];
		//	}
		//}
		//double scale = CVector3D( p1, p2 ).dot( minVector );
		//CVector3D delta = minVector*scale;
		//snappedPoint = m_LastMouseButton + CPoint( int(delta.x), int(delta.y) );
		
		MoveInvertedLine( snappedPoint );
	}
	else if( Mouse( MOUSE_LEGEND_DRAGGING ) )
	{
		ASSERT(FALSE);
	}
	//JL Note 8/31/2009 - this method does not look very good and causes some problems on some machines...
	else if( m_Filter.CanShowAreas() )
	{
		//// RDV Note - this might be really stupid to do this here for performance reasons
		//// the other place we might try is the flyover
		//// not doing anything important, lets see if we might hightlight an area "line"
		//CArea::g_lastHitType = OUTSIDE_AREA;
		//for( int i = 1; i <= theModel.areas(); i++ ) {
		//	const CArea* pA = theModel.area(i);
		//	if( pA ) 
		//	{
		//		double tol = m_Camera.ScalePixelSize( 5 );		
		//		CPoint3D nearPt, farPt;
		//		if( Get3DPointFromScreen( _point, nearPt, 0.f ) &&
		//			Get3DPointFromScreen( _point, farPt, 1.f ) )
		//		{
		//			CLine3D ray( nearPt, farPt );
		//			ETAdvancedAreaHit h = pA->segmentHitTest( ray, tol );
		//			if( h == ON_BOUNDARY_HIT || h == ON_HOLE_BOUNDARY_HIT || h == ON_CORRIDOR_BOUNDARY_HIT )
		//			{
		//			//	CChain* pCh = (CChain*)CArea::g_lastHitChain;
		//			//	if( !pCh ) 
		//			//		continue;
		//			//	int index = CArea::g_lastHitChainIndex;
		//			//	if( index <= 0 || index > pCh->points() )
		//			//		continue;
		//			//	//int item = CArea::g_lastHitItem;
		//			//	int iPt1 = index;
		//			//	int iPt2 = index+1 > pCh->points() ? 1 : index+1;
		//			//	CPoint3D pt1 = (CPoint3D)(*pCh->point( iPt1 ));
		//			//	CPoint3D pt2 = (CPoint3D)(*pCh->point( iPt2 ));
		//			//	CPoint screenPt1;
		//			//	CPoint screenPt2;
		//			//	if( GetScreenPtFrom3DPoint( pt1, screenPt1 ) &&
		//			//		GetScreenPtFrom3DPoint( pt2, screenPt2 ))
		//			//	{
		//			//		m_areaEdgeLine.StartLine( m_hWnd, m_pDC, screenPt1, screenPt2, ini.size().member );
		//			//		break;
		//			//	}
		//			}
		//		}
		//	}  // pA
		//}  // for i
	}

	//see if we're inside the rotation cube...
	CRect r;
	GetClientRect( &r );
	static bool bOrientationCubeHover = false;
	if( m_rotationCube.PointIsInViewport( _point, r ) || bOrientationCubeHover )
	{
		//we're inside the rotation viewport, now check to see if we hit anything on the cube
		//if( wglMakeCurrent(m_pDC->GetSafeHdc(), m_hRC) )
		//{
		CPoint3D rotations = m_Camera.GetRotations();
		bOrientationCubeHover = m_rotationCube.OnMouseMove( rotations, _point, r, ini.size().node );
		m_Camera.ResetViewport();
		Invalidate();
		//}
	}

	return;
}

void CGraphicView::OnLButtonUp( UINT /*type*/, CPoint _point )
{
	ReleaseCapture();

	// reset flyover!
	m_Filter.flyoverData = m_bFlyoverStatusSaved;

	//the text is disabled when we drag the model. 
	//just to be safe here, always turn the text back on
	m_graphicsModel.ResumeCompleteDrawing();

	// how far has the mouse moved since the button operation
	CPoint mouseDelta = _point - m_LastMouseButton;
	
	ReleaseMouse( MOUSE_ZOOMBOX_WAITING );
	ReleaseMouse( MOUSE_SELECTIONBOX_WAITING );
	ReleaseMouse( MOUSE_PENCIL_WAITING );
	ReleaseMouse( MOUSE_MODEL_DRAGGING );

	if( m_bForceRotate ) 
		m_bForceRotate = false;

	// if text or annotations dragging, invert old location, update new location and invert it and exit
	if( Mouse( MOUSE_TEXT_DRAGGING ) ) 
	{
		ReleaseMouse( MOUSE_TEXT_DRAGGING );
		if( m_TitleText.Dragging() )
			m_TitleText.SetDragging( false );
		Invalidate();  // a redraw shows the moved text properly
		return;
	}
	else if( Mouse( MOUSE_LEGEND_DRAGGING ) && m_pDraggingLegend )
	{
		ReleaseMouse( MOUSE_LEGEND_DRAGGING );
		m_pDraggingLegend->OnLButtonUp();
		m_pDraggingLegend = NULL;
		Invalidate();
		return;
	}

	// convert the screen coordinates (snappedPoint) to a 3-d model coordinate including jumping to nearest node or grid snappedPoint
	CPoint snappedPoint( _point );
	GetSnappedScreenPt( snappedPoint );

	if( Mouse( MOUSE_ZOOMBOX_DRAGGING ) )	// are we waiting to end the zoom box?
	{
		// TeK 10/1/2009/Late, added for fat-fingered, slow-witted people, like me, I was going to put up a nice message
		// about how to use the program, but then decided, we could just do something smart...
		// Hey look mom, I'm a graphics programmer!!!
		if( (m_DraggingRect.Height() < 25) || (m_DraggingRect.Width() < 25) ) {
			EndInvertedRectangle();
			ReleaseMouse( MOUSE_ZOOMBOX_DRAGGING );
			SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );
			RECT r;
			CRect windowRect;
			GetWindowRect( windowRect );
			GetClientRect( &r );
			CPoint pt( _point.x - windowRect.left, _point.y - windowRect.top );
			ViewZoomInAtPoint( _point );
		}
		else {
			EndZoomBox( );
		}
		return;
	}

	// these flags are set during generation 
	bool bCross = false;
	bool bBadArea = false;
	bool bBadDrop = false;

	if( Mouse( MOUSE_SELECTIONBOX_DRAGGING ) )		// are we dragging a selection set?
	{
		EndSelectionBox( _point );
	}
	else if( Mouse( MOUSE_PENCIL_DRAGGING ) )
	{
		ETDrawMode mode = m_Filter.drawMode;
//		if( m_Filter.windowType == POST_WINDOW && theModel.elements( PLANAR_ELEMENT ) > 0 )
//			mode = DRAW_STRESSPATH;
		switch( mode )
		{
		case DRAW_MEMBER:
			EndMemberDrawing( snappedPoint );
			break;
		case DRAW_CABLE:
			EndCableDrawing( snappedPoint );
			break;
		case DRAW_PLANAR:
			EndPlateSideDrawing( snappedPoint );
			break;
		case DRAW_AREA:
			EndAreaSideDrawing( snappedPoint );
			break;
//		case DRAW_STRESSPATH:
//			EndStressPathDrawing( snappedPoint );
//			break;
		case DRAW_NOTHING:
			break;
		default:
			ASSERT( false );
		}
		return;
	}
	else if( Mouse( MOUSE_NODE_DRAGGING ) )	// are we dragging a node?
	{
		//we can be moving a node or an area vertex
		ReleaseCapture();
		SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );
		ReleaseMouse( MOUSE_NODE_DRAGGING );
		EndInvertedLine( MOUSE_NODE_DRAGGING );
		if( mNodeBeingDragged )
			handleMouseNodeDragButtonUp( &bBadDrop, &bCross, &bBadArea );
		else if( m_vertexBeingDragged )
			handleMouseVertexDragButtonUp( );
	}  // mouse Node Drag
	else if( m_Filter.drawMode == DRAW_NOTHING ) // TeK 10/1/2007: Lint 'bad type' fixed
	{  // draw nothing mode - reset
		ReleaseMouse( MOUSE_ZOOMBOX_WAITING );
		ReleaseMouse( MOUSE_ZOOMBOX_DRAGGING );
		ReleaseMouse( MOUSE_SELECTIONBOX_WAITING );
		ReleaseMouse( MOUSE_SELECTIONBOX_DRAGGING );
		ReleaseMouse( MOUSE_PENCIL_WAITING );
		ReleaseMouse( MOUSE_PENCIL_DRAGGING );
		ReleaseMouse( MOUSE_NODE_DRAGGING );
	}
	else {
		// TeK Moved from MouseDown, Just a "click"!
		VAWindow()->SendMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
	}

	if( bBadDrop )
		AfxMessageBox( "You have moved the node so that it lies on another node or member.  This is not allowed.",
			MB_OK | MB_ICONHAND );
	if( bCross )
		AfxMessageBox( "You have moved the node to a snappedPoint where connected members cross other members or nodes in the model.  This is not allowed.",
			MB_OK | MB_ICONHAND );
	if( bBadArea )
		AfxMessageBox( "You have moved the node to a snappedPoint where connected plates have poorly defined areas.  This is not allowed.",
			MB_OK | MB_ICONHAND );


	return;
}

void CGraphicView::OnLButtonDown( UINT type, CPoint _point )
{
	if( (CWnd*)this != GetFocus() ) { // make sure we've got the focus
		SetFocus();	
	}

	// force flyover off!
	m_MyTip.Hide();
	m_bFlyoverStatusSaved = m_Filter.flyoverData;
	//turn off the layer box
	m_bDrawCurrentLayerBox = false;

	if( Mouse( MOUSE_ZOOMBOX_DRAGGING ) || Mouse( MOUSE_MODEL_DRAGGING ) )  // this should not happen
		return;

	if( Mouse( MOUSE_PENCIL_DRAGGING ) && m_Filter.drawMode == DRAW_PLANAR ) 
		return;

	if( Mouse( MOUSE_PENCIL_DRAGGING ) && m_Filter.drawMode == DRAW_AREA ) 
		return;

	mMouseSinceActivate++;

	CPoint3D snapped3D;
	GetSnapped3DPoint( _point, snapped3D );
	CPoint point( _point ); 
	GetSnappedScreenPt( point );

	m_LastMouseButton = _point;  // save this location for things like double clicking
	m_LastMouseMove = _point;

	//see if we're inside the rotation cube...
	CRect r;
	GetClientRect( &r );
	if( m_rotationCube.PointIsInViewport( _point, r ) )
	{
		//we're inside the rotation viewport, now check to see if we hit anything on the cube
		if( wglMakeCurrent(m_pDC->GetSafeHdc(), m_hRC) )
		{
			CPoint3D rotations = m_Camera.GetRotations();
			if( m_rotationCube.OnMouseClick( rotations, _point, r, ini.size().node ) )
			{
				//disable to text for faster rotation...
				m_graphicsModel.StartReducedDrawing();
				m_Camera.ResetViewport();
				Invalidate();
				return;
			}
		}
		return;
	}

	if( m_TitleText.PointIsOver( _point ) )
	{
		m_TitleText.SetDragging( true );
		SetMouse( MOUSE_TEXT_DRAGGING );	
		return;
	}

	// if we are over a legend, start dragging it
	if( m_Filter.windowType == POST_WINDOW || m_Filter.windowType == DESIGN_WINDOW ){
		//check to see if we hit a countour bar, if so we return the contour bar and 
		//ALSO set the contour bar object that we wish to drag (either a slider or the entire bar)
		m_pDraggingLegend = m_graphicsModel.HitContourBar( _point );
		if( m_pDraggingLegend )
		{
			//m_pDraggingLegend->OnLButtonDown();
			SetMouse( MOUSE_LEGEND_DRAGGING );
			return;
		}
	}

	if( Mouse( MOUSE_ZOOMBOX_WAITING ) ) 
	{
		StartZoomBox( _point );
		return;
	}

	//// make sure we are set up to allow drawing since the user has clicked his left mouse button
	SetMouse( MOUSE_PENCIL_WAITING );

	// check the list of graphic objects and see if selection state 
	// needs processed
	CHitRecordCollection hitCollection;
	GetSelectionHitList( _point, ini.size().node, ini.size().node, hitCollection );
	for( int i = 0; i < GRAPHIC_ALL_OBJECTS; i++ ) 
	{
		ETGraphicObject go = (ETGraphicObject)i;
		if( handleMouseSelection( go, _point, type, hitCollection ) )
		{
			return;
		}
	}

	SetMouse( MOUSE_SELECTIONBOX_WAITING );

	if( !(type & MK_CONTROL) && TotalSelected() > 0 && mMouseSinceActivate > 1 )		// unselect everything
	{
		UnselectAll();
		//Don't tell anybody until the mouse comes UP!
		// VAWindow()->PostMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
		Invalidate(); // repaint to show the change!
	}

	return;
}

void CGraphicView::OnLButtonDblClk( UINT type, CPoint _point )
{
	m_MyTip.Hide();
	//CGraphicView::OnLButtonDblClk( type, _point );
	m_LastMouseButton = _point;
	mMouseSinceActivate++;

	if( Mouse( MOUSE_ZOOMBOX_DRAGGING ) || Mouse( MOUSE_MODEL_DRAGGING ) || Mouse( MOUSE_MODEL_DRAGGING ))  // this should not happen
		return;


	// TeK Add 10/3/2009, double-click legend and change color-blend mode
	if( m_Filter.windowType == POST_WINDOW || m_Filter.windowType == DESIGN_WINDOW ){
		m_pDraggingLegend = m_graphicsModel.HitContourBar( _point );
		if( m_graphicsModel.HitContourBar( _point ) )
		{
			ini.graphics().blendResultColors = !ini.graphics().blendResultColors;
			m_pDraggingLegend->resetSliders();
			Invalidate();
			return;
		}
	}

	if( (CWnd*)this != GetFocus() ) 
		SetFocus();

	if( Mouse( MOUSE_PENCIL_DRAGGING ) )
	{
		ETDrawMode mode = m_Filter.drawMode;
		switch( mode )
		{
		case DRAW_MEMBER:
		case DRAW_CABLE:
		case DRAW_PLANAR:
			return;
			break;
		case DRAW_AREA:
			//get area start point
			if( mSideBeingDrawn > 2 && mElement3D.size() > 0 ){
				CPoint finishPt;
				GetScreenPtFrom3DPoint( mElement3D[0], finishPt );
				EndAreaSideDrawing( finishPt );
			}
			return;
			break;
		default:
			break;
		}
	}
	
	
	if( theProject.mode() == READ_ONLY_MODE )  // ignore if we are in a readonly mode
		return;

	CHitRecordCollection hitCollection;
	GetSelectionHitList( _point, ini.size().node, ini.size().node, hitCollection );

	// check the list of graphic objects and see if selection state 
	// needs processed
	for( int i = 0; i < GRAPHIC_ALL_OBJECTS; i++ ) {
		ETGraphicObject go = (ETGraphicObject)i;
		if( handleMouseDblClickSelection( go, _point, type, hitCollection ) )
			return;
	}

	// TeK Commented this UnselectAll, not sure why it is here after a double-click 
	// that did didn't click on anything?
	// if we are on a graphic object, edit it...
	// first unselect everything and redraw as such
	//UnselectAll();


	return;
}

void CGraphicView::OnMButtonUp( UINT /*type*/, CPoint _point )
{
	ReleaseCapture();

	//the text is disabled when we drag the model. 
	//just to be safe here, always turn the text back on
	m_graphicsModel.ResumeCompleteDrawing();

	if( Mouse( MOUSE_MODEL_DRAGGING ) )
	{
		//text was disabled but is now back on, redraw the scene to show the
		//model with the text on
		ReleaseMouse( MOUSE_MODEL_DRAGGING );
		Invalidate();
	}

	// reset flyover!
	m_Filter.flyoverData = m_bFlyoverStatusSaved;

	mMouseSinceActivate++;
	SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );
	ReleaseCapture();
	m_LastMouseButton = _point;
	return;
}

void CGraphicView::OnMButtonDown( UINT /*type*/, CPoint _point )
{
	// force flyover off!
	m_MyTip.Hide();
	m_bFlyoverStatusSaved = m_Filter.flyoverData;

	m_LastMouseButton = _point;
	m_LastMouseMove = _point;

	mMouseSinceActivate++;
	SetMouse( MOUSE_MODEL_DRAGGING );
	SetCursor( theApp.LoadCursor( PAN_CURSOR ) ); 
	SetCapture();
	return;
}

void CGraphicView::OnMButtonDblClk( UINT /*type*/, CPoint _point )
{
	m_MyTip.Hide();
	m_LastMouseButton = _point;
	mMouseSinceActivate++;
	if( cursorInWindow( _point, true ) )
		ZoomToExtents(false);
	return;
}

BOOL CGraphicView::OnMouseWheel( UINT nFlags, short zDelta, CPoint _pt )
{
	if( !m_bActive ) { // TeK Added 10/26/2009, weird messaging?
		return FALSE; // IGNORE!
	}

	VAHelpPane.updateActiveWindow( m_Filter.windowType );
	//SetCursor( theApp.LoadCursor( DOLLY_CURSOR ) ); 
	if( cursorInWindow( _pt, false ) ) {
		bool up = zDelta > 0;
		if( nFlags & MK_CONTROL ) {  // do a scroll left/right
			if( up ) GetParent()->SendMessage( WM_HSCROLL, SB_LINELEFT, NULL );
			else  GetParent()->SendMessage( WM_HSCROLL, SB_LINERIGHT, NULL );
		}
		else if( nFlags & MK_SHIFT ) {  // do a scroll up/down
			if( up ) GetParent()->SendMessage( WM_VSCROLL, SB_LINEUP, NULL );
			else  GetParent()->SendMessage( WM_VSCROLL, SB_LINEDOWN, NULL );
		}
		else {  // do a zoom in/out
			//get window point 
			RECT r;
			CRect windowRect;
			GetWindowRect( windowRect );
			GetClientRect( &r );
			CPoint pt( _pt.x - windowRect.left, _pt.y - windowRect.top );
			if( up ) ViewZoomInAtPoint( pt );
			else ViewZoomOutAtPoint( pt );
			if( Mouse( MOUSE_PENCIL_DRAGGING ) ) m_bForceDraw = true;
		}
		return TRUE;
	}
	else
		return CView::OnMouseWheel( nFlags, zDelta, _pt );
}

void CGraphicView::OnRButtonUp( UINT /*type*/, CPoint _point )
{
	ReleaseMouse( MOUSE_SELECTIONBOX_WAITING );
	ReleaseMouse( MOUSE_PENCIL_WAITING );
	ReleaseMouse( MOUSE_MODEL_DRAGGING );
	ReleaseCapture();

	// reset flyover!
	m_Filter.flyoverData = m_bFlyoverStatusSaved;
	m_LastMouseButton = _point;
	mMouseSinceActivate++;
	return;
}

extern int fnSplitWhat; // defined in modlmenu.cpp!
void CGraphicView::OnRButtonDown( UINT /*type*/, CPoint _point )
{
	ReleaseMouse( MOUSE_SELECTIONBOX_WAITING );
	ReleaseMouse( MOUSE_PENCIL_WAITING );
	ReleaseMouse( MOUSE_MODEL_DRAGGING );
	m_LastMouseButton = _point;
	m_LastMouseMove = _point;
	mMouseSinceActivate++;

	// force flyover off!
	m_MyTip.Hide();
	m_bFlyoverStatusSaved = m_Filter.flyoverData;
	//m_Filter.flyoverData = false;

	ClientToScreen( &_point );
	for( int i = m_PopupMenu.GetMenuItemCount()-1; i >= 0; --i ) 
	{
		m_PopupMenu.RemoveMenu( i, MF_BYPOSITION );
	}
	// Commands should be sent directly to the client window!
	CMainFrame* pMain = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
	if( pMain ) 
	{
		bool bSuccess = false;
		switch( m_Filter.windowType ){
			case MODEL_WINDOW:
				bSuccess = RightClickModelWindow( );
				break;
			case DESIGN_WINDOW:
				bSuccess = RightClickDesignWindow( );
				break;
			case POST_WINDOW:
				bSuccess = RightClickResultWindow( );
				break;
			default:
				break;
		}
		if( bSuccess )
		{
			m_PopupMenu.TrackPopupMenu( TPM_LEFTBUTTON, _point.x, _point.y, pMain );
		}
	}
 	return;
}

bool CGraphicView::RightClickModelWindow( CMenu* pOtherMenu )
{
	// Commands should be sent directly to the client window!
	CMainFrame* pMain = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
	CMenu* theMenu = &m_PopupMenu;
	bool useOtherMenu = pOtherMenu != NULL;
	if( useOtherMenu )
		theMenu = pOtherMenu;
	if( pMain ) 
	{
		// before we check all the model objects for selection, lets see if we are hovering over an
		// area's edge segment and if so our context menu just contains splitting the side
		bool bNeedSeparator = false;
		if( useOtherMenu == false ) 
		{
			if( m_Filter.CanShowAreas() ) 
			{
				for( int i = 1; i <= theModel.areas(); i++ ) 
				{
					const CArea* pA = theModel.area(i);
					if( pA ) 
					{
						double tol = m_Camera.ScalePixelSize( 5 )*m_printScale;		
						CPoint3D nearPt, farPt;
						if( Get3DPointFromScreen( m_LastMouseMove, nearPt, 0.f ) &&
							Get3DPointFromScreen( m_LastMouseMove, farPt, 1.f ) )
						{
							CLine3D ray( nearPt, farPt );
							ETAdvancedAreaHit h = pA->segmentHitTest( ray, tol );
							if( h == ON_BOUNDARY_HIT || h == ON_HOLE_BOUNDARY_HIT || h == ON_CORRIDOR_BOUNDARY_HIT && CArea::g_lastHitChain != NULL) 
							{
								theMenu->AppendMenu( MF_STRING, CM_MODEL_SPLIT_CHAIN_SEGMENT, "&Split Boundary" );
								bNeedSeparator = true;
								break;
							}
						}
					}  // pA
				}  // for i
			}
		}

		if( bNeedSeparator ) 
		{
			theMenu->AppendMenu( MF_SEPARATOR );
			bNeedSeparator = false;
		}

		char text[256];
		SelectionCountData sel;
		CServiceCase* pSC = dynamic_cast<CServiceCase*>(m_Filter.loadCase());
		bool bIgnoreDesign = (DESIGN_WINDOW != m_Filter.windowType);
		getCurrentSelectionCount( sel, pSC, bIgnoreDesign );
		int selModel = sel.Members + sel.Planars + sel.Springs + sel.Nodes + sel.Cables + sel.RigidDiaphragms + sel.Areas;
		int loads = 0;
		if( pSC ) loads = pSC->loads();
		// EDIT
		if( useOtherMenu == false ) {
			if( (sel.Total > 0) && (sel.Total == sel.Nodes || sel.Total == sel.Members || sel.Total == sel.Planars ||
				sel.Total == sel.Springs || sel.Total == sel.Cables|| sel.Total == sel.RigidDiaphragms || sel.Total == sel.NodalLoads ||
				sel.Total == sel.MemberLoads || sel.Total == sel.PlanarLoads || sel.Total == sel.MovingLoads || sel.Total == sel.AreaLoads ||
				sel.Total == sel.RigidDiaphragmLoads || sel.Total == sel.Foundations ) ) 
			{
				// Edit | Selected is no longer available: use Project Manager!
				if( !pMain->IsProjectManagerShown() ) {
					theMenu->AppendMenu( MF_STRING, CM_VIEW_PROJECTMANAGER, "Show Project Manager" );
				}
			}
			else if( !theProject.isLowProduct() )
			{
				if( !pMain->IsFindToolShown() ) theMenu->AppendMenu( MF_STRING, CM_EDITFIND, "Show Find Tool" );
			}
		}

		if( sel.Total ) {
			theMenu->AppendMenu( MF_STRING, CM_EDITCOPY, "Copy" );
			bNeedSeparator = true;
		}
		if( sel.Total ) 
		{
			// TeK Add 10/1/2009, added 'Generate' tip for VA 4, 5, and 6 customers...to be removed in VA 8?!
			theMenu->AppendMenu( MF_STRING, CM_EDITPASTE, "Paste (Generate Copies)" );
			bNeedSeparator = true; 
		}
		// TeK 10/1/2009: Removed, unusual, rarely used, clutters menu...
		// also happens to duplicate above 'Paste' menu..
		//if( theController.clipboardHasData() ) {
		//	theMenu->AppendMenu( MF_STRING, CM_EDITPASTE, "&Generate Clipboard Copies..." );
		//	bNeedSeparator = true;
		//}
		if( sel.Members && theController.clipboardHasOneMember() ) {
			theMenu->AppendMenu( MF_STRING, CM_EDITPASTEMEMBERPROPERTIES, "&Format Painter...");
			bNeedSeparator = true;
		}
		if( sel.Total ) {
			theMenu->AppendMenu( MF_STRING, CM_EDITDELETE, "Delete" );
			bNeedSeparator = true;
		}

		if( bNeedSeparator ) {
			theMenu->AppendMenu( MF_SEPARATOR );
			bNeedSeparator = false;
		}

		bNeedSeparator = false;
		if( useOtherMenu == false ) {
			// VIEW
			theMenu->AppendMenu( MF_STRING, CM_VIEW_ZOOM_FULL, "Zoom Normal" );
			theMenu->AppendMenu( MF_STRING, CM_VIEW_ZOOM_BOX, "Zoom Area..." );
			if( !theProject.isLowProduct() )
			{
				theMenu->AppendMenu( MF_STRING, CM_VIEW_CREATENAMEDVIEW, "Save Current View..." );
				// TeK Remove 10/1/2009: Not needed, Save can also update now...
				//theMenu->AppendMenu( MF_STRING, CM_VIEW_MODIFYNAMEDVIEW, "Update Current View" );
			}
			if( !theStructure.isPlane() && TotalSelected() > 0 && theModel.SelectedDefineAPlane() )
			{
				theMenu->AppendMenu( MF_STRING, CM_VIEW_CLIP_TO_SELECTED, "Clip to Selected" );
			}
		}
		if( sel.AreaLoads || sel.Areas || sel.Cables || sel.MemberLoads || sel.Members || 
			sel.NodalLoads || sel.Nodes || sel.PlanarLoads || sel.Planars || sel.RigidDiaphragmLoads ||
			sel.RigidDiaphragms || sel.Springs )
			theMenu->AppendMenu( MF_STRING, CM_MODEL_INVERT_SELECTION, "Invert Selection" );
		if( bNeedSeparator ) {
			theMenu->AppendMenu( MF_SEPARATOR );
			bNeedSeparator = false;
		}


		// MODEL
		bNeedSeparator = false;
		// TeK Remove 10/1/2009: See Paste above...
		//if( sel.Total ) 
		//{
		//	bNeedSeparator = true; 
		//	theMenu->AppendMenu( MF_STRING, ID_MODEL_GENERATECOPIES, "Generate Copies..." );
		//}
		if( (selModel > 1) && !theProject.isLowProduct() )
		{
			bNeedSeparator = true; 
			theMenu->AppendMenu( MF_STRING, CM_MODEL_RENAME, "Rename..." );
		}

		if( sel.Nodes && (sel.Nodes == sel.Total) ) 
		{
			bNeedSeparator = true; 
			theMenu->AppendMenu( MF_STRING, CM_MODEL_CREATE_SPRING, "Create Spring Support(s)..." );
		}

		if( useOtherMenu == false ) {

			if( !theProject.isLowProduct() ) 
			{
				bNeedSeparator = true; 
				theMenu->AppendMenu( MF_STRING, CM_MODEL_MOVE, "Move..." );
				theMenu->AppendMenu( MF_STRING, CM_MODEL_ROTATE, "Rotate..." );
			}

			CCombineMembers cm(false);
			if( OK_TO_COMBINE == cm.CanCombine() && !theProject.isLowProduct() )
			//if( areSelectedMembersAChain() && !theProject.isLowProduct() )
			{
				theMenu->AppendMenu( MF_STRING, CM_MODEL_CREATE_SUPERMEMBER, "Combine Members" );
				bNeedSeparator = true; 
			}

			//split combined members
			if( sel.Members && (sel.Members == sel.Total) ) 
			{
				pMain->SetupSplitMemberCode();
				if( 2 == fnSplitWhat ) {
					strcpy( text, "Split Combined Member(s)..." );
					theMenu->AppendMenu( MF_STRING, CM_MODEL_SPLIT_MEMBER, text );
					bNeedSeparator = true; 
				}
				else if( 1 == fnSplitWhat ) {
					strcpy( text, "Split Member(s)..." );
					theMenu->AppendMenu( MF_STRING, CM_MODEL_SPLIT_MEMBER, text );
					bNeedSeparator = true; 
				}
			}

			//extend members
			if( sel.Members > 0 ) 
			{
				strcpy( text, "Extend Member(s)..." );
				theMenu->AppendMenu(  MF_STRING, CM_MODEL_EXTEND_MEMBER, text );
				bNeedSeparator = true; 
				
			}

			//split cables
			if( sel.Cables > 0 )
			{
				strcpy( text, "Split Cable(s)..." );
				theMenu->AppendMenu( MF_STRING, CM_MODEL_SPLIT_CABLE, text );
				bNeedSeparator = true; 
			}

			//split plates
			if( !theProject.isLowProduct() ) 
			{
				if( sel.Planars && (sel.Planars == sel.Total) ) 
				{
					if( sel.Planars > 1 )
						strcpy( text, "Split Plates..." );
					else
						strcpy( text, "Split Plate..." );
					theMenu->AppendMenu( MF_STRING, CM_MODEL_SPLIT_PLATE, text );
					bNeedSeparator = true; 
				}
			}
			if( sel.Planars || sel.Members ) {
				theMenu->AppendMenu( MF_STRING, CM_MODEL_REVERSE_NODES, "Reverse Local Axes" );
				bNeedSeparator = true; 
			}
			if( bNeedSeparator ) {
				theMenu->AppendMenu( MF_SEPARATOR );
				bNeedSeparator = false; 
			}

			// Areas
			if( sel.AreaVertices ){
				theMenu->AppendMenu( MF_STRING, CM_MODEL_ADD_AREA_VERTEX, "Insert Vertex" );
				bNeedSeparator = true; 
			}
			if( sel.AreaVertices >= 1 ) {
				theMenu->AppendMenu( MF_STRING, CM_MODEL_CREATE_NODE_AT_VERTEX, "Create Node(s) at Vertex" );
				bNeedSeparator = true; 
			}
			if( selectionLiesInPlane() ) {
				theMenu->AppendMenu( MF_STRING, CM_MODEL_CREATE_SELECTED_AREAS, "Create Area from Selection" );
				bNeedSeparator = true; 
			}
			if( sel.Areas == 1 )  // see if we are over a hole in an area 
			{
				CArea* pA = (CArea*)theModel.area( 1, true );
				CCoordinate c( m_Current3DPt.x, m_Current3DPt.y, m_Current3DPt.z );
				int itemHit = -1;
				if( pA && pA->hitTest( c, &itemHit ) == HOLE_HIT ) {
					theMenu->AppendMenu( MF_STRING, CM_MODEL_REMOVE_AREA_HOLE, "Remove Area Hole" );
					pA->selectedItem( itemHit );
					bNeedSeparator = true; 
				}
				else if( pA && pA->hitTest( c, &itemHit ) == CORRIDOR_HIT ) {
					theMenu->AppendMenu( MF_STRING, CM_MODEL_REMOVE_AREA_CORRIDOR, "Remove Area Corridor" );
					bNeedSeparator = true; 
					pA->selectedItem( itemHit );
				}

			}

			if( bNeedSeparator ) {
				theMenu->AppendMenu( MF_SEPARATOR );
				bNeedSeparator = false; 
			}
			// LOAD
			// RDV Note 7/2/09 - When nodes lie over area vertices we must make sure and allow nodal loads
			if( theProject.isValid( serviceCase() ) && sel.Nodes && (sel.Nodes + sel.AreaVertices) == sel.Total
				&& !serviceCase()->haveMovingLoads() ) 
			{
				theMenu->AppendMenu( MF_STRING, CM_MODEL_CREATE_NODALLOAD, "Apply Nodal Load(s)..." );
				bNeedSeparator = true; 
			}
			if( theProject.isValid( serviceCase() ) && sel.Members && 
				(sel.Members == sel.Total) && !serviceCase()->haveMovingLoads() ) 
			{
				strcpy( text, "Apply Member Load(s)..." );
				theMenu->AppendMenu( MF_STRING, CM_MODEL_CREATE_MEMBERLOAD, text );
				bNeedSeparator = true; 
			}
			if( !theProject.isLowProduct() )
			{
				if( theProject.isValid( serviceCase() ) && sel.Planars && (sel.Planars == sel.Total) &&
					( theStructure.canPlanarsBend() || theStructure.canPlanarsStretch() ) 
					&& !serviceCase()->haveMovingLoads() ) 
				{
					strcpy( text, "Apply Plate Load(s)..." );
					theMenu->AppendMenu( MF_STRING, CM_MODEL_CREATE_PLANARLOAD, text );
					bNeedSeparator = true; 
				}
				if( theProject.isValid( serviceCase() ) && sel.Areas && (sel.Areas == sel.Total)  
					&& !serviceCase()->haveMovingLoads() ) 
				{
					strcpy( text, "Apply &Area Load(s)..." );
					theMenu->AppendMenu( MF_STRING, CM_MODEL_CREATE_AREALOAD, text );
					bNeedSeparator = true; 
				}
				if( sel.Areas && sel.Areas == sel.Total ) 
				{
					bool canConvert = false;
					for( int i = 1; i <= theModel.areas(theModel.areas( true )>0); i++ ) {
						const CArea* pA = theModel.area(i, true);
						if( pA && pA->autoMesh() && pA->needsMeshing() == false ) {
							canConvert = true;
							break;
						}
					}
					if( canConvert ) {
						strcpy( text, "Convert Auto-Mesh to Plates..." );
						theMenu->AppendMenu( MF_STRING, CM_MODEL_MESH2PLATES, text );
						bNeedSeparator = true; 
					}
				}

				if( theProject.isValid( serviceCase() ) && sel.RigidDiaphragms && (sel.RigidDiaphragms == sel.Total) 
					&& !serviceCase()->haveMovingLoads() ) 
				{
					theMenu->AppendMenu( MF_STRING, CM_MODEL_CREATE_RIGIDDIAPHRAGMLOAD, "Apply Rigid Diaphragm Load(s)..." );
					bNeedSeparator = true; 
				}

			}
			if( sel.NodalLoads > 0 || sel.PlanarLoads > 0 || sel.MemberLoads > 0 )
			{
				theMenu->AppendMenu( MF_STRING, CM_MODEL_SCALE_SELECTED_LOAD, "Scale Load(s)..." );
				bNeedSeparator = true; 
			}
		}  // use other menu = false
		if( useOtherMenu == false ) {
			if( sel.Nodes == 1 )
			{
				theMenu->AppendMenu( MF_STRING, CM_VIEW_SNAP_GRID_TO_NODE, "Snap Grid Origin to Node" );
			}

			bNeedSeparator = false;
			if( pMain->ValidFoundationSelection(true) )
			{
				if( bNeedSeparator ) {
					theMenu->AppendMenu( MF_SEPARATOR );

				}
				if( theModel.foundations(OnlySelected) == 1 ) 
					theMenu->AppendMenu( MF_STRING, CM_MODEL_FOUNDATION, "Update Foundation..." );
				else
					theMenu->AppendMenu( MF_STRING, CM_MODEL_FOUNDATION, "Create Foundation..." );
				bNeedSeparator = true;
			}

		} // use other menu = false

		if( bNeedSeparator ) {
			theMenu->AppendMenu( MF_SEPARATOR );
			bNeedSeparator = false; 
		}

		// REPORT
		theMenu->AppendMenu( MF_STRING, CM_VIEW_QUICK_REPORT, QuickReportName() );
		theMenu->AppendMenu( MF_STRING, CM_VIEW_REPORT, "R&eport Wizard..." );


		return true;
	}
	return false;
}

bool CGraphicView::RightClickDesignWindow( CMenu* pOtherMenu )
{
	CMainFrame* pMain = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
	CMenu* theMenu = &m_PopupMenu;
	bool useOtherMenu = pOtherMenu != NULL;
	if( useOtherMenu )
		theMenu = pOtherMenu;
	if( pMain ) 
	{
		// Determine what kinds of things are selected at the moment.
		int selMembers = theModel.elements( MEMBER_ELEMENT, OnlySelected );
		int selCables = theModel.elements( CABLE_ELEMENT, OnlySelected );
		int selPlates = theModel.elements( PLANAR_ELEMENT, OnlySelected );
		int selSprings = theModel.elements( SPRING_ELEMENT, OnlySelected );
		int selTotal = selMembers + selPlates + selCables + selSprings;
		int selNodes = theModel.nodes( OnlySelected );
		int selGroups = theGroupManager.groups(true);
		int selMeshes = theMeshManager.meshes(true);
		int selDesign = selGroups + selMeshes;
		int selFoundations = theModel.foundations(OnlySelected);

		bool bSeparator = false;

		// TeK Removed: I want my help here...
		//if( useOtherMenu == false ) {
			// Now populate the menu with the commands we want
			theMenu->AppendMenu( MF_STRING, CM_HELP_WHATSTHIS, "&What's This?" );
			theMenu->AppendMenu( MF_SEPARATOR );
		//}


		if( theController.designSoftwareIsPresent() ) 
		{
			if( useOtherMenu == false ) {
				// Design View Popup
				if( selDesign && !pMain->IsProjectManagerShown() )
				{
					theMenu->AppendMenu( MF_STRING, CM_VIEW_PROJECTMANAGER, "&Show Project Manager" );
					bSeparator = true;
				}
				// 
				//else if( selDesign )
				//{
				//	theMenu->AppendMenu( MF_STRING, CM_EDITDELETE, "&Delete" );
				//	bSeparator = true;
				//}
				if( bSeparator ) {
					theMenu->AppendMenu( MF_SEPARATOR );
					bSeparator = false;
				}
			}

			bSeparator = false;
			if( pMain->CreateDesignGroupsEnable(true) )
			{
				theMenu->AppendMenu( MF_STRING, CM_CREATE_DESIGN_GROUPS, "&Create Design Group..." );
				bSeparator = true;
			}
			if( pMain->DesignGroupsEnable(true) || pMain->DesignMeshesEnable(true) )
			{
				theMenu->AppendMenu( MF_STRING, CM_DESIGN_MEMBERS, "D&esign..." );
				bSeparator = true;
			}
			if( pMain->AddMemberToGroupEnable(true) || pMain->AddPlateToMeshEnable(true) )
			{
				theMenu->AppendMenu( MF_STRING, CM_ADD_MEMBER_TO_GROUP, "&Add Member(s) To Group..." );
				bSeparator = true;
			}
			if( pMain->RemoveMemberFromGroupEnable(true) || pMain->RemovePlateFromMeshEnable(true) )
			{
				theMenu->AppendMenu( MF_STRING, CM_REMOVE_MEMBER_FROM_GROUP, "&Remove Member(s) From Group..." );
				bSeparator = true;
			}
			if( useOtherMenu == false ) {
				if( theGroupManager.hasDesignChanged() ||	theMeshManager.hasDesignChanged() )
				{
					theMenu->AppendMenu( MF_STRING, CM_DESIGN_SYNCHRONIZE, "&Synchronize Design Changes" );
					bSeparator = true;
				}
				if( pMain->ValidVisualFoundation(true) && selFoundations == 1 )
				{
					theMenu->AppendMenu( MF_STRING, CM_TOOLS_RUNVISUALFOUNDATION, "Des&ign Foundation..." );
					bSeparator = true;
				}
			}

			if( bSeparator ) {
				theMenu->AppendMenu( MF_SEPARATOR );
				bSeparator = false;
			}

		}

		if( useOtherMenu == false ) {

			// Common to both Design and Result Views
			// View Menu
			if( !pMain->IsFindToolShown() )
			{
				theMenu->AppendMenu( MF_STRING, CM_EDITFIND, "&Show Find Tool" );
			}
			theMenu->AppendMenu( MF_STRING, CM_VIEW_ZOOM_FULL, "&Zoom Normal" );
			theMenu->AppendMenu( MF_STRING, CM_VIEW_ZOOM_BOX, "Zoom &Area..." );
			theMenu->AppendMenu( MF_STRING, CM_VIEW_CREATENAMEDVIEW, "&Name Current View..." );
			theMenu->AppendMenu( MF_STRING, CM_VIEW_MODIFYNAMEDVIEW, "&Update Current View" );

			if( !theStructure.isPlane() && (TotalSelected() > 0) && theModel.SelectedDefineAPlane() )
				theMenu->AppendMenu( MF_STRING, CM_VIEW_CLIP_TO_SELECTED, "&Clip to Selected" );

			if( (selNodes > 0) || (selTotal > 0) || (selSprings > 0) )
				theMenu->AppendMenu( MF_STRING, CM_MODEL_INVERT_SELECTION, "&Invert Selection" );

			// VisualDesign 5.0 Feature
			if( theController.beamDesignSoftwareIsPresent() &&
				theController.designCasesHaveValidResults() && (theController.designVersion() > 4) ) 
			{
				if( selGroups > 1 || selMembers > 1 ) { // TeK Add 10/19/2009
					theMenu->AppendMenu( MF_STRING, CM_DESIGN_SUMMARY, "&Member Unity Summary" );
				}
				theMenu->AppendMenu( MF_STRING, CM_VIEW_QUICK_REPORT, QuickReportName() );
			}

		}

		theMenu->AppendMenu( MF_STRING, CM_VIEW_REPORT, "R&eport Wizard..." );

		return true;
	}
	return false;
}

bool CGraphicView::RightClickResultWindow( CMenu* pOtherMenu )
{
	CMainFrame* pMain = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
	CMenu* theMenu = &m_PopupMenu;
	bool useOtherMenu = pOtherMenu != NULL;
	if( useOtherMenu )
		theMenu = pOtherMenu;
	if( pMain ) 
	{
		// Determine what kinds of things are selected at the moment.
		int selMembers = theModel.elements( MEMBER_ELEMENT, OnlySelected );
		int selCables = theModel.elements( CABLE_ELEMENT, OnlySelected );
		int selPlates = theModel.elements( PLANAR_ELEMENT, OnlySelected );
		int selSprings = theModel.elements( SPRING_ELEMENT, OnlySelected );
		int selTotal = selMembers + selPlates + selCables + selSprings;
		int selNodes = theModel.nodes( OnlySelected );
		//int selGroups = theGroupManager.groups(true);
		//int selMeshes = theMeshManager.meshes(true);
		//int selDesign = selGroups + selMeshes;

		bool bSeparator = false;
		if( useOtherMenu == false ) {
			// Now populate the menu with the commands we want
			theMenu->AppendMenu( MF_STRING, CM_HELP_WHATSTHIS, "&What's This?" );
			theMenu->AppendMenu( MF_SEPARATOR );

			// Result View Popup
			if( !pMain->IsProjectManagerShown() )
			{
				theMenu->AppendMenu( MF_STRING, CM_VIEW_PROJECTMANAGER, "&Show Project Manager" );
				bSeparator = true;
			}

			if( theModel.nodes() > 0 ) {
				theMenu->AppendMenu( MF_STRING, CM_VIEW_TOGGLE_NODES, "Toggle &Nodes" );
				bSeparator = true;
			}
			if( theModel.elements( MEMBER_ELEMENT ) > 0 ) {
				theMenu->AppendMenu( MF_STRING, CM_VIEW_TOGGLE_MEMBERS, "Toggle &Member Elements" );
				bSeparator = true;
			}
			if( theModel.elements( CABLE_ELEMENT ) > 0 ) {
				theMenu->AppendMenu( MF_STRING, CM_VIEW_TOGGLE_CABLES, "Toggle &Cable Elements" );
				bSeparator = true;
			}
			if( theModel.elements( PLANAR_ELEMENT ) > 0 ) {
				theMenu->AppendMenu( MF_STRING, CM_VIEW_TOGGLE_PLANARS, "Toggle &Plate Elements" );
				bSeparator = true;
			}
			if( theModel.elements( SPRING_ELEMENT ) > 0 ) {
				theMenu->AppendMenu( MF_STRING, CM_VIEW_TOGGLE_SPRINGS, "Toggle &Spring Supports" );
				bSeparator = true;
			}
			if( bSeparator ) {
				theMenu->AppendMenu( MF_SEPARATOR );
				bSeparator = false;
			}
		
			if( selMembers && m_Filter.loadCase()->resultCaseCount() >= 1 ) 
			{
				bSeparator = true;
				if( 1 == selMembers ) {
					if( m_Filter.loadCase()->haveMovingLoads() )
						theMenu->AppendMenu( MF_STRING, CM_VIEW_INFLUENCE_DIAGRAM, "&Influence Diagram" );
					theMenu->AppendMenu( MF_STRING, CM_VIEW_MEMBER_GRAPH, "Member &Graph" );
				}
				else {
					theMenu->AppendMenu( MF_STRING, CM_VIEW_MEMBER_GRAPH, "Members &Graph" );
				}
			}
			if( selNodes && (m_Filter.loadCase()->isTimeStep() || m_Filter.loadCase()->haveMovingLoads()) ) 
			{
				if( 1 == selNodes ) {
					bSeparator = true;
					if( m_Filter.loadCase()->haveMovingLoads() )
						theMenu->AppendMenu( MF_STRING, CM_VIEW_INFLUENCE_DIAGRAM, "&Influence Diagram" );
					else 
						theMenu->AppendMenu( MF_STRING, CM_VIEW_NODE_GRAPH, "Graph &Node Results" );
				}
			}

			// TeK Remove 10/22/2009: Plate graphs work well in AG, but numbers are just WRONG in VA?
			// I don't want to deal with it, are they really needed anymore?
			//double normalVector[3];
			//if( selPlates >= 1 && areSelectedPlatesInAPlane( normalVector ) )
			//{
			//	theMenu->AppendMenu( MF_STRING, CM_VIEW_PLATE_GRAPH, "G&raph Plates" );
			//	bSeparator = true;
			//}
			if( bSeparator ) {
				theMenu->AppendMenu( MF_SEPARATOR );
				bSeparator = false;
			}
			// Common to both Design and Result Views
			// View Menu
			if( !pMain->IsFindToolShown() )
			{
				theMenu->AppendMenu( MF_STRING, CM_EDITFIND, "&Show Find Tool" );
			}
			theMenu->AppendMenu( MF_STRING, CM_VIEW_ZOOM_FULL, "&Zoom Normal" );
			theMenu->AppendMenu( MF_STRING, CM_VIEW_ZOOM_BOX, "Zoom &Area..." );
			theMenu->AppendMenu( MF_STRING, CM_VIEW_CREATENAMEDVIEW, "&Name Current View..." );
			theMenu->AppendMenu( MF_STRING, CM_VIEW_MODIFYNAMEDVIEW, "&Update Current View" );

			if( !theStructure.isPlane() && (TotalSelected() > 0) && theModel.SelectedDefineAPlane() )
				theMenu->AppendMenu( MF_STRING, CM_VIEW_CLIP_TO_SELECTED, "&Clip to Selected" );

			if( (selNodes > 0) || (selTotal > 0) || (selSprings > 0) ) {
				theMenu->AppendMenu( MF_STRING, CM_MODEL_INVERT_SELECTION, "&Invert Selection" );
				bSeparator = true;
			}
			// Animator
			if( m_Filter.loadCase() && m_Filter.resultCase() && m_Filter.loadCase()->areResultsValid( m_Filter.resultCase()->time() ) ) {
				theMenu->AppendMenu( MF_SEPARATOR );
				theMenu->AppendMenu( MF_STRING, CM_VIEW_ANIMATE, "&Animate Results View..." );
				bSeparator = true;
			}

		}  // not other menu

		if( bSeparator ) {
			theMenu->AppendMenu( MF_SEPARATOR );
			bSeparator = false;
		}
		const char* pch = QuickReportName();
		if( strlen( pch ) > 0 ) {
			// Report Menu
			theMenu->AppendMenu( MF_STRING, CM_VIEW_QUICK_REPORT, pch );
		}
		theMenu->AppendMenu( MF_STRING, CM_VIEW_REPORT, "R&eport Wizard..." );

		return true;
	}
	return false;
}

void CGraphicView::OnRButtonDblClk( UINT /*type*/, CPoint /*_point*/ )
{
	m_MyTip.Hide();
	mMouseSinceActivate++;
	return;
}

void CGraphicView::OnNcLButtonDown( UINT /*type*/, CPoint _point )
{
	m_LastMouseButton = _point;
	mMouseSinceActivate++;
	return;
}

void CGraphicView::OnKillFocus(CWnd* /*pNewWnd*/)
{
	// if tip is up, shut it down
	if( ::IsWindow( m_MyTip ) )
		m_MyTip.Hide();
	return;
}

void CGraphicView::SelectAllObjects( ETGraphicObject t, CNameFilter filter, m_ETSelectType s, bool bCheckVisibility )
{
	if( SELECT_OFF == s ) { // TeK Change 8/4/2007, don't take changes for unselect all!
		unSelectEverything();
		return;
	}
	// we are to set the selection state to all of the objects of type t
	// we need to update the CData information and draw selected state
	// this is all based on the name filter specified and if none specified, its the main filter's name filter
	//CTimeThisFunction timer( "Select all graphic objects: CGraphicView::SelectAllObjects" );
	switch( t )
	{
	case GRAPHIC_ALL_OBJECTS:
	case GRAPHIC_NODE:
		{
			//CTimeThisFunction t( "Unselect all nodes" );
			for( CNodeIterator ni( filter ); !ni; ++ni ) {
				if( m_Filter.isVisible( *(CNode*)ni() ) || !bCheckVisibility ){
					CNode* pN = (CNode*)ni();
					SetSelectedState( pN, s );
				}
			}
		}
		if( t != GRAPHIC_ALL_OBJECTS )
			break;
	case GRAPHIC_MEMBER:
		for( CElementIterator ne1( MEMBER_ELEMENT, filter ); !ne1; ++ne1 ) {
			if( m_Filter.isVisible( *(CMember*)ne1() ) || !bCheckVisibility){
				CMember* pM = (CMember*)ne1();
				SetSelectedState( pM, s );
			}
		}
		if( t != GRAPHIC_ALL_OBJECTS )
			break;
	case GRAPHIC_PLATE:
		for( CElementIterator ne2( PLANAR_ELEMENT, filter ); !ne2; ++ne2 ) {
			if( m_Filter.isVisible( *(CPlanar*)ne2() ) || !bCheckVisibility){
				CPlanar* pP = (CPlanar*)ne2();
				SetSelectedState( pP, s );
			}
		}
		if( t != GRAPHIC_ALL_OBJECTS )
			break;
	case GRAPHIC_AUTO_PLATE:
		for( CMeshedPlateIterator ne2( filter ); !ne2; ++ne2 ) {
			if( m_Filter.isVisible( *(CPlanar*)ne2() ) || !bCheckVisibility){
				CPlanar* pP = (CPlanar*)ne2();
				SetSelectedState( pP, s );
			}
		}
		if( t != GRAPHIC_ALL_OBJECTS )
			break;
	case GRAPHIC_SPRING:
		for( CElementIterator ne3( SPRING_ELEMENT, filter ); !ne3; ++ne3 ) {
			if( m_Filter.isVisible( *(CSpring*)ne3() ) || !bCheckVisibility){
				CSpring* pS = (CSpring*)ne3();
				SetSelectedState( pS, s );
			}
		}
		if( t != GRAPHIC_ALL_OBJECTS )
			break;
	case GRAPHIC_CABLE:
		for( CElementIterator ne4( CABLE_ELEMENT, filter ); !ne4; ++ne4 ) {
			if( m_Filter.isVisible( *(CCable*)ne4() ) || !bCheckVisibility ){
				CCable* pC = (CCable*)ne4();
				SetSelectedState( pC, s );
			}
		}
		if( t != GRAPHIC_ALL_OBJECTS )
			break;
	case GRAPHIC_RIGID_DIAPHRAGM:
		{
			for( int i = 1; i <= theModel.rigidDiaphragms(); i++ ) 
			{
				CRigidDiaphragm* pRD = (CRigidDiaphragm*)theModel.rigidDiaphragm( i );
				if( pRD && (m_Filter.isVisible( *pRD ) || !bCheckVisibility ))
				{
					SetSelectedState( pRD, s );
				}
			}
			if( t != GRAPHIC_ALL_OBJECTS )
				break;
		}
	case GRAPHIC_AREA:
		for( CAreaIterator aiter( filter ); !aiter; ++aiter ){
			if( m_Filter.isVisible( *(CArea*)aiter() ) || !bCheckVisibility){
				CArea* pA = (CArea*)aiter();
				SetSelectedState( pA, s );
			}
		}
		if( t!= GRAPHIC_ALL_OBJECTS )
			break;
	case GRAPHIC_AREA_HOLE:
		for( CAreaIterator aiter( filter ); !aiter; ++aiter ){
			if( m_Filter.isVisible( *(CArea*)aiter() ) || !bCheckVisibility){
				CArea* pA = (CArea*)aiter();
				for( int i = 1; i <= pA->holes(); i++ )
					SetSelectedState( (CChain*)pA->hole( i ), s );
			}
		}
		if( t!= GRAPHIC_ALL_OBJECTS )
			break;
	case GRAPHIC_AREA_CORRIDOR:
		for( CAreaIterator aiter( filter ); !aiter; ++aiter ){
			if( m_Filter.isVisible( *(CArea*)aiter() ) || !bCheckVisibility){
				CArea* pA = (CArea*)aiter();
				for( int i = 1; i <= pA->corridors(); i++ )
					SetSelectedState( (CChain*)pA->corridor( i ), s );
			}
		}
		if( t!= GRAPHIC_ALL_OBJECTS )
			break;
	case GRAPHIC_AREA_VERTEX:
		for( CAreaIterator aiter( filter ); !aiter; ++aiter ){
			if( m_Filter.isVisible( *(CArea*)aiter() ) || !bCheckVisibility){
				CArea* pA = (CArea*)aiter();
				for( int i = 1; i <= pA->vertices(); i++ )
					SetSelectedState( (CCoordinate*)pA->vertex( i ), s );
			}
		}
		if( t!= GRAPHIC_ALL_OBJECTS )
			break;
	case GRAPHIC_FOUNDATION:
		for( CFoundationIterator fiter( filter ); !fiter; ++fiter ){
			if( m_Filter.isVisible( *(CFoundation*)fiter() ) || !bCheckVisibility){
				CFoundation* pF = (CFoundation*)fiter();
				SetSelectedState( pF, s );
			}
		}
		if( t!= GRAPHIC_ALL_OBJECTS )
			break;
	case GRAPHIC_NODAL_LOAD:
	{
		CServiceCase* pSC = serviceCase();
		if( pSC ) 
		{
			//loop over nodal loads in this load case
			for( int i = 1; i <=pSC->nodalLoads(); i++ ) 
			{
				CNodalLoad* pNL = ((CNodalLoad*)pSC->nodalLoad( i ));
				if( pNL && m_Filter.isVisible( *pNL ) || !bCheckVisibility)
				{
					SetSelectedState( pNL, s );
				}
			}
		}
		if( t != GRAPHIC_ALL_OBJECTS )
			break;
	}
	case GRAPHIC_MEMBER_LOAD:
	{
		CServiceCase* pSC = serviceCase();
		if( pSC ) {
			//loop over nodal loads in this load case
			for( int i = 1; i <=pSC->elementLoads( MEMBER_ELEMENT ); i++ ) 
			{
				CMemberLoad* pML = ((CMemberLoad*)pSC->elementLoad( MEMBER_ELEMENT, i ));
				if( pML && (m_Filter.isVisible( *pML ) || !bCheckVisibility ) )
				{
					SetSelectedState( pML, s );
				}
			}
		}
		if( t != GRAPHIC_ALL_OBJECTS )
			break;
	}
	case GRAPHIC_PLATE_LOAD:
		{
			CServiceCase* pSC = serviceCase();
			if( pSC ) {
				//loop over nodal loads in this load case
				for( int i = 1; i <=pSC->elementLoads( PLANAR_ELEMENT ); i++ ) 
				{
					CPlanarLoad* pPL = ((CPlanarLoad*)pSC->elementLoad( PLANAR_ELEMENT, i ));
					if( pPL && (m_Filter.isVisible( *pPL ) || !bCheckVisibility ))
					{
						SetSelectedState( pPL, s );
					}
				}
			}
			if( t != GRAPHIC_ALL_OBJECTS )
				break;
		}
	case GRAPHIC_AREA_LOAD:
	{
		CServiceCase* pSC = serviceCase();
		if( pSC ) {
			//loop over nodal loads in this load case
			for( int i = 1; i <=pSC->areaLoads( ); i++ ) 
			{
				CAreaLoad* pAL = ((CAreaLoad*)pSC->areaLoad( i ));
				if( pAL && (m_Filter.isVisible( *pAL ) || !bCheckVisibility))
				{
					SetSelectedState( pAL, s );
				}
			}
		}
		if( t != GRAPHIC_ALL_OBJECTS )
			break;
	}
	case GRAPHIC_RIGID_DIAPHRAGM_LOAD:
		{
			CServiceCase* pSC = serviceCase();
			if( pSC ) 
			{
				//loop over nodal loads in this load case
				for( int i = 1; i <=pSC->rigidDiaphragmLoads(); i++ ) 
				{
					CRigidDiaphragmLoad* pRDL = ((CRigidDiaphragmLoad*)pSC->rigidDiaphragmLoad( i ));
					if( pRDL && (m_Filter.isVisible( *pRDL ) || !bCheckVisibility))
					{
						SetSelectedState( pRDL, s );
					}
				}
			}
			if( t != GRAPHIC_ALL_OBJECTS )
				break;
		}
	case GRAPHIC_MOVING_LOAD:
	{
		CServiceCase* pSC = serviceCase();
		if( pSC ) {
			//loop over nodal loads in this load case
			for( int i = 1; i <=pSC->movingLoads( ); i++ ) 
			{
				CMovingLoad* pML = ((CMovingLoad*)pSC->movingLoad( i ));
				if( pML && (m_Filter.isVisible( *pML ) || !bCheckVisibility ))
				{
					SetSelectedState( pML, s );
				}
			}
		}
		if( t != GRAPHIC_ALL_OBJECTS )
			break;
	}
	default:
		break;
	};
	return;
}

void CGraphicView::SelectAllObjectsInHitList( const CHitRecordCollection& rHitCollection )
{
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		ETGraphicObject objType = GetModelItemType( pHR->m_arrayNames[0] );
		CHitRecordCollection hrc;
		hrc.m_arrayHits.Add( pHR );
		if( const CData* pD = m_graphicsModel.OverGraphicObject( CPoint(), objType, hrc, serviceCase() ) ) 
		{
			CData* object = (CData*) pD;
			// single  selection
			SelectToggle( objType, object );
		}
		//we call remove all here so that the hits don't get deleted in the destructor
		hrc.m_arrayHits.RemoveAll();
	}
	
	return;
}
void CGraphicView::SelectToggle( ETGraphicObject /*t*/, CData* object )
{
	object->toggleSelect();
	if( DESIGN_WINDOW == m_Filter.windowType )
	{
		const CMember* pMember = dynamic_cast<const CMember*>(object);
		if( pMember )
		{
			theGroupManager.toggleSelect( pMember );
			int theGroupNumber = theGroupManager.isMemberGrouped( pMember );
      		if( m_Filter.toggleDesignGroups && theGroupNumber > 0 ) {
				const CDesignGroup* pG = theGroupManager.group( theGroupNumber );
				int memberCount = pG->members();
				for( int i = 1; i <= memberCount; i++ ) {
            		const CMember* pM = pG->member( i );
					if( pM != object ) {
               			pM->toggleSelect();
					}
				}
               	
            }
		}

		const CPlanar* pPlate = dynamic_cast<const CPlanar*>(object);
		if( pPlate ) 
		{
			theMeshManager.toggleSelect( pPlate );
			int theMeshNumber = theMeshManager.isPlateMeshed( pPlate );
			if( m_Filter.toggleDesignGroups && theMeshNumber > 0 ) {
				const CDesignMesh* pM = theMeshManager.mesh( theMeshNumber );
				int planarCount = pM->plates();
				for( int i = 1; i <= planarCount; i++ ) {
					const CPlanar* pP = pM->plate( i );
					if( pP != pPlate ) {
						pP->toggleSelect();
					}
				}
			} 
		}
	}
	return;
}

void CGraphicView::UnselectAll( ETGraphicObject t  )
{
	//CTimeThisFunction timer( "Unselect all graphic objects: CGraphicView::UnselectAll" );
	CNameFilter filter;
	filter.set("*");  // everything
	//TRACE("Calling UnselectALL\n");
	SelectAllObjects(  t, filter, SELECT_OFF, false );
	return;
}

void CGraphicView::SetSelectedState( CData* pD, m_ETSelectType s )
{
	if( s == SELECT_ON ){
		if( DESIGN_WINDOW == m_Filter.windowType ){
			const CMember* pMember = dynamic_cast<const CMember*>(pD);
			if( pMember ){
				theGroupManager.unSelectAll();
				theGroupManager.toggleSelect( pMember );
			}
			const CPlanar* pPlate = dynamic_cast<const CPlanar*>(pD);
			if( pPlate ){
				theMeshManager.unSelectAll();
				theMeshManager.toggleSelect( pPlate );
			}
		}
		pD->select();
	}
	else if( s == SELECT_OFF ){
		if( DESIGN_WINDOW == m_Filter.windowType ){
			const CMember* pMember = dynamic_cast<const CMember*>(pD);
			if( pMember ){
				theGroupManager.unSelectAll();
			}
			const CPlanar* pPlate = dynamic_cast<const CPlanar*>(pD);
			if( pPlate ){
				theMeshManager.unSelectAll();
			}
		}
		pD->unSelect();
	}else{
		if( DESIGN_WINDOW == m_Filter.windowType ){
			const CMember* pMember = (const CMember*)pD;
			if( pMember ){
				theGroupManager.toggleSelect( pMember );
			}
			const CPlanar* pPlate = (const CPlanar*)pD;
			if( pPlate ){
				theMeshManager.toggleSelect( pPlate );
			}
		}
		pD->toggleSelect();
	}
	return;
}

int CGraphicView::TotalSelected( void )
{
	int total;
	total = theModel.nodes( OnlySelected ) +
	theModel.elements( MEMBER_ELEMENT, OnlySelected ) +
	theModel.elements( CABLE_ELEMENT, OnlySelected ) +
	theModel.elements( SPRING_ELEMENT, OnlySelected ) +
	theModel.elements( PLANAR_ELEMENT, OnlySelected ) +
	theModel.areas( OnlySelected ) +
	theModel.foundations( OnlySelected ) +
	theModel.rigidDiaphragms( OnlySelected );
	for( int i = 1; i <= theModel.areas(); i++ ){
		const CChain& aChain = theModel.area(i)->boundary();
		total += aChain.points();
	}
	CServiceCase* pSC = dynamic_cast<CServiceCase*>(m_Filter.loadCase());
	if( pSC ) {
		total += pSC->nodalLoads( true ) +
			pSC->elementLoads( MEMBER_ELEMENT, true ) +
			pSC->elementLoads( PLANAR_ELEMENT, true ) +
			pSC->movingLoads( true ) +
			pSC->rigidDiaphragmLoads( true );
	}
	return total;
}


bool CGraphicView::handleMouseSelection( ETGraphicObject go, CPoint point, UINT type, const CHitRecordCollection& rHitCollection  )
{
	// we have a mouse action (button down) at "point"
	// keyboard is coded in "type"
	// see if we are over a graphic object of type "go" and if so select/toggle select
	// accordingly
	//CTimeThisFunction t("handleMouseSelection");

	//JL Added 9/23/2009 - when we box select with the shift key we respond to the down event 
	//                     first. If we are over a graphics object then we "select all objects"
	//                     before entering box selection mode. To solve this problem we allow 
	//                     the space bar to initiate box selection. In this case we ignore the 
	//                     "mouse down" event and go straight to box selection.
	SHORT spaceBarState = GetAsyncKeyState( VK_SPACE );
	bool bSpaceBarDown = (spaceBarState & 0x8000) != 0;
	if( bSpaceBarDown )
		return false;

	//we're on something...
	if( const CData* pD = m_graphicsModel.OverGraphicObject( point, go, rHitCollection, serviceCase() ) ) 
	{
		CData* object = (CData*) pD;
		if( !(type & MK_CONTROL) && !object->isSelected() )	
		{ 
			// unselect everything
			UnselectAll();
		}
		if( type & MK_SHIFT ) 
		{ 
			// select all VISIBLE objects of this type in the model
			if( !(type & MK_CONTROL ) ) 
			{
				SelectAllObjects( go, nameFilterForGraphicObject( go ), SELECT_ON, true );
			}
			else 
			{  
				// ctrl - shift only select ones with same prefix
				CNameFilter filter;
				filter.set( LPCSTR(object->namePrefix()));
				SelectAllObjects( go, filter, SELECT_ON, true );
			}
		}
		else 
		{  
			// single  selection
			//CTimeThisFunction t1("handleMouseSelection : SelectToggle");
			// RDV Note 6/16/09 - If we have a vertex and a node at the same place, toggle select
			// the vertex as well
			const CNode* pN = dynamic_cast<CNode*>(object);
			if( go == GRAPHIC_NODE &&  m_Filter.windowType == MODEL_WINDOW && pN ) {
				// if we've got a vertex at this location, select it as well
				for( int i = 1; i <= theModel.areas(); i++ ) {
					CArea* pA = (CArea*)theModel.area(i);
					if( pA ) { 
						for( int j = 1; j <= pA->vertices(); j++ ) {
							CCoordinate* pV = (CCoordinate*)pA->vertex(j);
							if( pV && zero( pV->distanceTo( *pN ) ) )
								pV->toggleSelect();
						}
					}
				}
			}
			SelectToggle( go, object );
		}
		// this is where we invalidate the view if a selection is made. in order to use the faster selection
		// method we still need to send this message to update the modify filter but we need to suppress the
		// invalidate command.
		VAWindow()->PostMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
		Invalidate();
		return true;
	}
	return false;
}

bool CGraphicView::handleMouseBoxSelection( CHitRecordCollection& rHitCollection )
{
	//only select unique items
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		for( int j = rHitCollection.m_arrayHits.GetSize()-1; j > i; j-- )
		{
			CHitRecord* pAnotherHR = rHitCollection.m_arrayHits[j];
			if( pHR->m_arrayNames[0] == pAnotherHR->m_arrayNames[0] )
			{
				//remove the other hit record
				delete rHitCollection.m_arrayHits[j];
				rHitCollection.m_arrayHits.RemoveAt(j);
			}
		}
	}
	SelectAllObjectsInHitList( rHitCollection );
	return true;
}

bool CGraphicView::handleMouseDblClickSelection( ETGraphicObject go, CPoint point, UINT /*type*/, const CHitRecordCollection& rHitCollection )
{
	// we have a mouse action (button double click) at "point"
	// keyboard is coded in "type"
	// see if we are over a graphic object of type "go" and if so select/toggle select
	// accordingly
	if( const CData* pD = m_graphicsModel.OverGraphicObject( point, go, rHitCollection, serviceCase() ) ) {
		CData* object = (CData*) pD;
		if( !object->isSelected() ) {  
			SelectToggle( go, object ); // toggle the selected status
			VAWindow()->SendMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
		}
		CWaitCursor wc;
		if( m_Filter.windowType == MODEL_WINDOW ) {
			DoubleClickModelWindow( go );
		}
		else if( m_Filter.windowType == POST_WINDOW ) {
			DoubleClickResultWindow( go, object );
		}
		else if( m_Filter.windowType == DESIGN_WINDOW ) {
			DoubleClickDesignWindow( go, object );
		}
		SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );
		return true;
		}
	return false;
}

void CGraphicView::DoubleClickModelWindow( ETGraphicObject /*go*/ )
{
	//JL 4/27/2009 - open the project manager if it is not open...
	CMainFrame* pMain = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
	if( pMain ) 
	{
		if( !pMain->IsProjectManagerShown() )
			pMain->CmViewProjectManager();
	}
}


void CGraphicView::DoubleClickResultWindow( ETGraphicObject go, const CData* /*object*/ )
{
	CReport* pReport = NULL;
	switch( go ) {
		case GRAPHIC_NODE: 
		case GRAPHIC_MEMBER:
		case GRAPHIC_SPRING:
		case GRAPHIC_CABLE:
		case GRAPHIC_PLATE:
		case GRAPHIC_AUTO_PLATE:
			//VAWindow()->PostMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
			Invalidate();
			pReport = new CReport( "Quick Analysis Results", QUICK_REPORT );
			QuickResultReport( *pReport );
			VAWindow()->SendMessage( OPEN_REPORT_MESSAGE, (WPARAM)0, (LPARAM)pReport );
			break;
		default:
			break;
	}
}


void CGraphicView::DoubleClickDesignWindow( ETGraphicObject go, const CData* object )
{
	const CMember* pMember = NULL;
	const CPlanar* pPlate = NULL;
	bool bReport = false;
	switch( go ) {
		case GRAPHIC_MEMBER:
			pMember = (const CMember*)object;
			if( theGroupManager.isMemberGrouped( pMember ) ) {
				theGroupManager.toggleSelect( pMember );
				bReport = true;
			}
			break;
		case GRAPHIC_PLATE:
			pPlate = (const CPlanar*)object;
			if( theMeshManager.isPlateMeshed( pPlate ) ) {
				theMeshManager.toggleSelect( pPlate );
				bReport = true;
			}
			break;
		default:
			break;
	}
	if( bReport ) {
		//VAWindow()->PostMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
		Invalidate();
		CReport* pReport = new CReport( "Quick Design Results", QUICK_REPORT );
		QuickResultReport( *pReport );
		VAWindow()->SendMessage( OPEN_REPORT_MESSAGE, (WPARAM)0, (LPARAM)pReport );
	}
}


bool CGraphicView::handleMouseNodeDragButtonUp( bool* bBadDrop, bool* bCross, bool* bBadArea )
{
	bool success = false;
	bool bNodesOverlap = false;
	const CNode* mergeNode = NULL;
	if( ( m_DragStart3D != m_DragEnd3D ) ) { // node moved!
		// see if the moved node falls on another node in the model
		CNode rN( NULL, m_DragEnd3D.x, m_DragEnd3D.y, m_DragEnd3D.z );
		for( CNodeIterator ni; !ni; ++ni ) {
			const CNode* pN1 = ni();
			if( pN1 != mNodeBeingDragged ) {
				if( pN1->distanceTo( rN ) < theProject.designCriteria().nodeTolerance() ) {
					//*bBadDrop = true;
					bNodesOverlap = true;
					mergeNode = pN1;
					break;
				}
			}
		}
		// see if the moved node falls on another member in the model
		if( !bNodesOverlap )
		{
			for( CElementIterator ne2( MEMBER_ELEMENT ); !ne2; ++ne2 ) {
				const CMember* pM = (const CMember*)ne2();
				const CNode* pN1 = pM->node(1);
				const CNode* pN2 = pM->node(2);
				if( pN1 != mNodeBeingDragged && pN2 != mNodeBeingDragged ) {
					if( rN.isOnLine( *pN1, *pN2 ) )
					{
						*bBadDrop = true;
						break;
					}
				}
			}
		}

		if( !(*bBadDrop) && !bNodesOverlap ) {
			// first see if we would create crossing members or ill fated planars
			for( CElementIterator ne( MEMBER_ELEMENT ); !ne; ++ne )
			{
				const CMember* pM = (const CMember*)ne();
				if( pM->node(1) == mNodeBeingDragged || pM->node(2) == mNodeBeingDragged ) {
					CNode n1 = *pM->node(1);
					CNode n2 = *pM->node(2);
					if( pM->node(1) == mNodeBeingDragged ) {
						n1.x( m_DragEnd3D.x );
						n1.y( m_DragEnd3D.y );
						n1.z( m_DragEnd3D.z );
					}
					else {
						n2.x( m_DragEnd3D.x );
						n2.y( m_DragEnd3D.y );
						n2.z( m_DragEnd3D.z );
					}
					for( CNodeIterator ni; !ni; ++ni ) {
						const CNode* pN = ni();
						if( pN != mNodeBeingDragged && pN != pM->node(1) && pN != pM->node(2) ) {
							if( pN->isOnLine( n1, n2 ) ) {
								*bCross = true;
								break;
							}
						}
					}
					if( !(*bCross) )  {
						for( CElementIterator ne1( MEMBER_ELEMENT ); !ne1; ++ne1 ) {
							CCoordinate intersectPoint;
							const CMember* pM1 = (const CMember*)ne1();
							if( pM1->node(1) != mNodeBeingDragged && pM1->node(2) != mNodeBeingDragged ) {
								if ( MembersCross(&n1, &n2, pM1, &intersectPoint) ) 
								{
									if( fabs( intersectPoint.distanceTo( n1 ) ) > theProject.designCriteria().nodeTolerance() &&
										fabs( intersectPoint.distanceTo( n2 ) ) > theProject.designCriteria().nodeTolerance() ) {
										*bCross = true;
										break;
									}
								}
							}
						}
					}
				}
				if( *bCross ) break;
			}
		}
		if( !(*bCross)  && !(*bBadDrop) && !bNodesOverlap ) {
			for( CElementIterator pe( PLANAR_ELEMENT ); !pe; ++pe ) {
				const CPlanar* pP = (const CPlanar*)pe();
				if( pP->node(1) == mNodeBeingDragged || pP->node(2) == mNodeBeingDragged ||
					pP->node(3) == mNodeBeingDragged || pP->node(4) == mNodeBeingDragged ) {
					CPlanar p;
					CNode n;
					n.x( m_DragEnd3D.x );
					n.y( m_DragEnd3D.y );
					n.z( m_DragEnd3D.z );
					for( int i = 1; i <= 4; i++ ) {
						if( pP->node( i ) == mNodeBeingDragged )
							p.node( i, &n );
						else
							p.node( i, pP->node(i) );
					}
					if( p.area() <= 0 ) {
						*bBadArea = true;
						break;
					}
				}
			}
		}
		success = (!(*bBadArea) && !(*bCross) && !(*bBadDrop) && !bNodesOverlap );
		if( success ) {
			CHECK_IF( mNodeBeingDragged ) {
				// move the node
				CNode* pN = (CNode*)mNodeBeingDragged->clone();
				pN->x( m_DragEnd3D.x );
				pN->y( m_DragEnd3D.y );
				pN->z( m_DragEnd3D.z );
				resetCopyFlags();
				setCopyFlag( CNode::COPY_LOCATION );
				theApp.recordCommand( DRAG_NODE_CMD );
				theController.startGroupChange( "Drag a Node" );
				theController.modify( mNodeBeingDragged, pN );
				theController.endGroupChange();
				delete pN;
			}
		}
		else if( bNodesOverlap && mergeNode ) {
			// lets see if the user wants to merge these two nodes
			if( AfxMessageBox( "The two overlapping nodes can be merged into one, would you like to do that?", MB_YESNO ) == IDYES ) {
				theApp.recordCommand( DRAG_NODE_CMD );
				theController.merge( mNodeBeingDragged, mergeNode );
			}
		}
	}
	mNodeBeingDragged = NULL;
	return success;
}  // handleMouse
	
bool CGraphicView::handleMouseVertexDragButtonUp( )
{
	bool success = false;
	if( ( m_DragStart3D != m_DragEnd3D ) )	// node moved!
	{
		// see if the moved vertex falls on another area vertex in the model
		CHECK_IF( m_vertexBeingDragged ) {
			CCoordinate* pNewC = new CCoordinate( *m_vertexBeingDragged );
			pNewC->x( m_DragEnd3D.x );
			pNewC->y( m_DragEnd3D.y );
			pNewC->z( m_DragEnd3D.z );
			theController.startGroupChange( "Drag an Area Vertex" );
			success = theController.modify( m_vertexBeingDragged, pNewC  );
			theController.endGroupChange();
			delete pNewC;
		}
	}
							
	m_vertexBeingDragged = NULL;
	return success;
}  // handleMouse

bool CGraphicView::InSelectionBoxMode( const CPoint& deltaMouse, UINT type )
{
	bool bIsAcceptableMouseDelta = ( abs(deltaMouse.x) > ini.size().node || abs(deltaMouse.y) > ini.size().node );
	bool bShiftDown = ((type & MK_SHIFT ) != 0);//GetKeyState( VK_SHIFT );
	//JL Added 9/23/2009 - space bar or shift will work...
	SHORT spaceBarState = GetAsyncKeyState( VK_SPACE );
	bool bSpaceBarDown = (spaceBarState & 0x8000) != 0;
	//if( bShiftDown )
	//	TRACE("Shift is down\n" );
	bool bSelBoxWaiting = Mouse( MOUSE_SELECTIONBOX_WAITING );
	bool bPencilDragging = Mouse( MOUSE_PENCIL_DRAGGING );
	return( bIsAcceptableMouseDelta && (bShiftDown || bSpaceBarDown) && bSelBoxWaiting && !bPencilDragging );
}

bool CGraphicView::InNodeDragMode( const CPoint& deltaMouse, UINT type )
{
	bool bIsAcceptableMouseDelta = ( abs(deltaMouse.x) > ini.size().node || abs(deltaMouse.y) > ini.size().node );
	bool bControlDown = ((type & MK_CONTROL ) != 0);//GetKeyState( VK_CONTROL );
	bool bPencilWaiting = Mouse( MOUSE_PENCIL_WAITING );
	bool bSelBoxDragging = Mouse( MOUSE_SELECTIONBOX_DRAGGING );
	bool bReadOnly = (theProject.mode() == READ_ONLY_MODE);
	return( m_Filter.windowType == MODEL_WINDOW && bControlDown && bIsAcceptableMouseDelta && bPencilWaiting && !bReadOnly && !bSelBoxDragging );
}

bool CGraphicView::InDrawMode( const CPoint& deltaMouse, UINT type )
{
	bool bIsAcceptableMouseDelta = ( abs(deltaMouse.x) > ini.size().node || abs(deltaMouse.y) > ini.size().node );
	//JL Added 9/23/2009 - space bar or shift will work for box selection
	SHORT spaceBarState = GetAsyncKeyState( VK_SPACE );
	bool bSpaceBarDown = (spaceBarState & 0x8000) != 0;
	return ( Mouse( MOUSE_PENCIL_WAITING ) 
		&& !Mouse( MOUSE_SELECTIONBOX_DRAGGING )
		&& !(theProject.mode() == READ_ONLY_MODE)
		//JL Note 9/30/2009 - remove m_View
		&& ( ( /*(m_View.type != PERSPECTIVE_VIEW) && */
		mGrid.isActive() 
		&& mGrid.isSnap() 
		&& (theModel.nodes( true ) != 1) ) || ( theModel.nodes( true ) == 1 ) )
		&& bIsAcceptableMouseDelta 
		&&  ( !( type & MK_SHIFT ) && !( type & MK_CONTROL ) && !bSpaceBarDown )
		&&  ( (m_Filter.windowType == MODEL_WINDOW) /*||
				//JL Note 7/27/2009 - removed support for stress path plots
		      (m_Filter.windowType == POST_WINDOW && theModel.elements( PLANAR_ELEMENT ) > 0 )*/ )
		);
}

void CGraphicView::DrawInvertedText( CGraphicsText& text )
{
	ASSERT_RETURN( m_pDC );
	if( !m_pDC )
		return;
	CRect wndRect;
	GetClientRect( wndRect );

	text.DrawInvertedText2D( m_pDC/*m_pTextDC*/, ini.font().graphWindow.name, 
					ini.font().graphWindow.size, 
					ini.color().graphTitles, text.ControlPoint(), 0., wndRect  );

	//JL note 3/24/2009 - try wglGetCurrentDC()...
	SwapBuffers( wglGetCurrentDC() /*pDC->GetSafeHdc()*/ );
}

void CGraphicView::DragText( const CPoint& delta )
{
	// off with the old
	DrawInvertedText( m_TitleText );
	m_Filter.titleOffset = delta + m_Filter.titleOffset;
	m_TitleText.SetControlPoint( m_Filter.titleOffset );
	// and on with the new
	DrawInvertedText( m_TitleText );
}

void CGraphicView::DragLegend( const CPoint& delta )
{
	if( delta.x == 0 && delta.y == 0 )
		return;
	if( !m_pDraggingLegend )
		return;
	// off with the old
	//DrawInvertedLegend( m_pDraggingLegend );
	m_pDraggingLegend->OnMouseMove( delta );
	// and on with the new
	//DrawInvertedLegend( m_pDraggingLegend );
	//JL Note 9/22/2009 - this may be a performance problem...
	Invalidate();
}
void CGraphicView::RotateModel( const CPoint& delta )
{
	//CTimeThisFunction t( "RotateModel" );
	double increment = ini.filter().nudgeAngle;
	//m_Camera.RotateY( delta.x*increment );
	//m_Camera.RotateX( delta.y*increment );
	m_Camera.RotateXY( delta.x*increment, delta.y*increment );
	//m_Camera.RotateXY( delta.y*0.01, delta.x*0.01, m_Filter );
	Invalidate();
}
void CGraphicView::DragModel( const CPoint& delta )
{
	//////////////// orthographic move ///////////////////////
	m_Camera.MoveBy( delta );
	Invalidate();

	/////////////// perspective move /////////////////////////
	//HGLRC currentRC = m_hRC;
	//if( !currentRC && !wglMakeCurrent( m_hDC,this->m_hRC ) )
	//{
	//	currentRC = wglGetCurrentContext();
	//	if( !currentRC && !wglMakeCurrent( m_hDC,this->m_hRC ) )
	//		ASSERT(false);
	//}

	//m_MyTip.Hide();
	//
	//double d1 = Z_FAR; //front.distance_to(back);
	//ASSERT( d1 > 0 );
	//if( d1 <= 0 ) 
	//	return;
	////get the distance between the front plane and the focal point
	//double focalLength = 1;//m_Camera.GetFocalLength();
	////now get the normalized distance (0..1) into the frustum for the focal point
	//double norm = focalLength/d1;
	//if( norm > 1.0 ) norm = 1.0;
	//if( norm < 0.0 ) norm = 0.0;
	////now get the old and new points based on the focal plane
	//CPoint3D prev, cur;
	//CPoint screenPt = m_LastMouseMove;
	//CPoint prevScreenPt = screenPt - delta;
	//if( !Get3DPointFromScreen( screenPt, cur, (float)norm ) || 
	//	!Get3DPointFromScreen( prevScreenPt, prev, (float)norm ) )
	//	return;
	//we should have good points
	//now we can move the camera by the difference between cur and prev...
	//CPoint3D delta3D = prev - cur;
	//if( m_Camera.isPerspectiveOn() )
	//{
	//	delta3D.x *= focalLength;
	//	delta3D.y *= focalLength;
	//	delta3D.z *= focalLength;
	//}
	//m_Camera.MoveBy( delta3D );

	Invalidate();
}

void CGraphicView::StartDragNode( const CPoint& /*delta*/ )
{
	int selNodes = theModel.nodes( true );
	int selVertices = 0;
	for( int i = 1; i <= theModel.areas(); i++ ){
		for( int j = 1; j <= theModel.area(i)->boundary().points(); j++ ){
			if( theModel.area(i)->boundary().point(j)->isSelected() ){
				selVertices++;
			}
		}
	}

	if( selNodes > 1 || selVertices > 1 ) return;

	const CNode* pN = NULL;
	if( selNodes > 0 )
		pN = theModel.node( 1, true );

	//CNodeIterator it( OnlySelected );
	//CNode* pN = (CNode*)it();
	const CCoordinate* pC = NULL;
	for( int i = 1; i <= theModel.areas(); i++ ){
		for( int j = 1; j <= theModel.area(i)->vertices(); j++ ){
			if( theModel.area(i)->vertex(j)->isSelected() ){
				pC = theModel.area(i)->vertex(j);
				break;
			}
		}
	}

	if( pN )
		mNodeBeingDragged = (CNode*)pN;
	else if( pC ){
		m_vertexBeingDragged = (CCoordinate*)pC;
	}
	else return;

	UnselectAll();
	//VAWindow()->PostMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
	//Invalidate();
	SetMouse( MOUSE_NODE_DRAGGING );
	ReleaseMouse( MOUSE_PENCIL_WAITING );
	SetCapture();
	SetCursor( theApp.LoadCursor( "DRAGNODE_CURSOR" ) );  //dragging hand cursor
	//if any members are attached find the 2-D orientation so we can snap the rubber band to these lines
	GetAttachedMemberAngles( pN );
	CPoint end = m_LastMouseButton;
	end.x += ini.size().node;
	end.y += ini.size().node;
	CPoint start = m_LastMouseButton;
	GetSnappedScreenPt( start );
	StartInvertedLine( start, end, MOUSE_NODE_DRAGGING/*MOUSE_PENCIL_DRAGGING*/ );
}


void CGraphicView::StartDrawMode( const CPoint& p )
{
	UnselectAll();
	//VAWindow()->SendMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
	ETDrawMode mode = m_Filter.drawMode;
//	if( m_Filter.windowType == POST_WINDOW && theModel.elements( PLANAR_ELEMENT ) > 0 )
//		mode = DRAW_STRESSPATH;
	if( mode == DRAW_MEMBER ||
		mode == DRAW_CABLE ||
		mode == DRAW_PLANAR ||
		mode == DRAW_AREA /*||
		mode == DRAW_STRESSPATH*/ )
	{
		CPoint end = p;
		CPoint start = m_LastMouseButton;
		GetSnappedScreenPt( start );
		if( m_Filter.drawMode == DRAW_PLANAR || m_Filter.drawMode == DRAW_AREA )
			StartInvertedPolygon( start, end, MOUSE_PENCIL_DRAGGING );
		else
			StartInvertedLine( start, end, MOUSE_PENCIL_DRAGGING );
		SetCapture();

		HCURSOR hc = NULL;
		if( mode == DRAW_MEMBER ) {
			hc = theApp.LoadCursor( "MEMBER_CURSOR"  );
		}
		else if( mode == DRAW_CABLE ) {
			hc = theApp.LoadCursor( "CABLE_CURSOR"  );
		}
		else if( mode == DRAW_PLANAR ) {
			hc = theApp.LoadCursor( "ELEMENT_CURSOR"  );
			mSideBeingDrawn = 1;
		}
		//else if( mode == DRAW_STRESSPATH ) {
		//	hc = theApp.LoadCursor( "STRESSPATH_CURSOR"  );
		//}
		else if( mode == DRAW_AREA ) {
			hc = theApp.LoadCursor( "AREA_CURSOR"  );
		}
		if( hc ) {
			SetCursor( hc );
		}

	}
	ReleaseMouse( MOUSE_PENCIL_WAITING );
	SetMouse( MOUSE_PENCIL_DRAGGING );
} 

void CGraphicView::StartSelectionBox( const CPoint& p )
{
	 // lets start up a selection box
	ReleaseMouse( MOUSE_SELECTIONBOX_WAITING );
	CRect r( m_LastMouseButton.x, m_LastMouseButton.y, p.x, p.y );
	StartInvertedRectangle( r, MOUSE_SELECTIONBOX_DRAGGING );
	SetCapture();  // make sure this window gets ALL windows mouse messages
	SetCursor( theApp.LoadCursor( "SELECTBOX_CURSOR"  ) );
}

void CGraphicView::StartZoomBox( const CPoint& p )
{
	// lets start up a zoom box
	ReleaseMouse( MOUSE_ZOOMBOX_WAITING );
	//CRect r( m_LastMouseButton.x, m_LastMouseButton.y, p.x, p.y );
	CRect r( p.x, p.y, p.x+20, p.y+20 );
	StartInvertedRectangle( r, MOUSE_ZOOMBOX_DRAGGING );
	//SetCapture();  // make sure this window gets ALL windows mouse messages
	//SetCursor( theApp.LoadCursor( "ZOOMBOX_CURSOR"  ) );
	return;
}

void CGraphicView::EndSelectionBox( const CPoint& /*p*/ )
{
	EndInvertedRectangle( );
	ReleaseCapture();
	SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );

	////////////////////////////////////////////////////////////////////////////////////
	////unproject all visible points and determine if they are inside the dragging rectangle...
	//http://www.opengl.org/documentation/specs/man_pages/hardcopy/GL/html/glu/project.html
	//GLint viewport[4];
	//GLdouble mvMatrix[16], projMatrix[16];
	//double winX = 0.0;
	//double winY = 0.0;
	//double winZ = 0.0; //depth value between 0 and 1 - for depth sorting
	//bool bSuccess = false;

	//HGLRC currentRC = wglGetCurrentContext();
	//if( !currentRC )
	//	return bSuccess;
	//
	//HDC hDC = wglGetCurrentDC( );
	//if( !hDC )
	//	return bSuccess;

	//if( !wglMakeCurrent( hDC, currentRC ) )
	//	return bSuccess;

	//glGetIntegerv( GL_VIEWPORT, viewport );
	//glGetDoublev( GL_MODELVIEW_MATRIX, mvMatrix );
	//glGetDoublev( GL_PROJECTION_MATRIX, projMatrix );

	////multiply the projection matrix and the modelview matrix
	//CMatrix44 P( projMatrix );
	//CMatrix44 M( mvMatrix );
	//CMatrix44 PxM;
	//multMatrix44AndMatrix44( P, M, PxM );
	//
	////now multiply each model object by PxM
	//
	//if( !gluProject( modelSpacePt.x, modelSpacePt.y, modelSpacePt.z, 
	//	             mvMatrix, projMatrix, viewport, 
	//			     &winX, &winY, &winZ ) )
	//{
	//	//TRACE( "Failed in GetScreenPt #2\n" );
	//	//wglMakeCurrent( NULL, NULL );
	//	return bSuccess;
	//}
	/////////////////////////////////////////////////////////////////////////////////////////

	//new method, loop over model objects, find objects in the selection box...
	//nodes
	//members
	//cables
	//plates
	//areas
	//rigid diaphragms
	//node loads
	//member loads
	//plate loads
	//area loads
	//moving loads


	//divide selection box into 4x4
	CHitRecordCollection uniqueHitCollection;
	for( int i = 0; i < 5; i++ )
	{
		for( int j = 0; j < 5; j++ )
		{
			CHitRecordCollection hitCollection;
			CRect rect( m_DraggingRect.left+m_DraggingRect.Width()*(i)/5,	//left
						m_DraggingRect.top+m_DraggingRect.Height()*(j)/5,	//top
						m_DraggingRect.left+m_DraggingRect.Width()*(1+i)/5,	//right
						m_DraggingRect.top+m_DraggingRect.Height()*(1+j)/5 );//bottom
			CPoint center = rect.CenterPoint();
			GetSelectionHitList( center, abs(rect.Width()), abs(rect.Height()), hitCollection );
			for( int k = 0; k < hitCollection.m_arrayHits.GetCount(); k++ )
				uniqueHitCollection.m_arrayHits.Add( hitCollection.m_arrayHits[k] );
			hitCollection.m_arrayHits.RemoveAll();
			//handleMouseBoxSelection( hitCollection );
		}
	}
	handleMouseBoxSelection( uniqueHitCollection );
	// update mouse status
	ReleaseMouse( MOUSE_SELECTIONBOX_DRAGGING );
	VAWindow()->SendMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
	Invalidate();
}
void CGraphicView::EndZoomBox( )
{
	EndInvertedRectangle(  );  // clean up the last boz
	ReleaseMouse( MOUSE_ZOOMBOX_DRAGGING );
	//ReleaseCapture();
	SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );
	ZoomDisplayToRectangle( m_DraggingRect);	// do the zooming stuff
	Invalidate();
	return;
}

void CGraphicView::EndCableDrawing( const CPoint&/* p*/ )
{
	EndInvertedLine( MOUSE_PENCIL_DRAGGING );
	ReleaseCapture();
	SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );
	bool bIsAcceptableLength = ( m_DragStart3D.distance_to( m_DragEnd3D ) > 1.E-5 );
	if( bIsAcceptableLength ) 
	{
        mDrawGeneration = true;  // this is done so that messaging that results from the 				
		// controller's generation does not mess up other things in this window
		resetGenerateMessageCounts();
		// TeK Change 12/16/2002: Select the element after drawing it, makes editing faster and easier!
		theApp.recordCommand( DRAG_CABLE_CMD );

		const CCable* pM = generateCable( 
			m_DragStart3D.x, m_DragStart3D.y, m_DragStart3D.z, 
			m_DragEnd3D.x, m_DragEnd3D.y, m_DragEnd3D.z, false,
            NULL, true, true, false );		// generate the member  // RDV: Note
		if( pM ) 
		{
			pM->select();
			VAWindow()->PostMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
			Invalidate();
		}
		mDrawGeneration = false;
	} 
}

void CGraphicView::StartInvertedRectangle( CRect r, m_ETMouseState m )
{
		m_DraggingRect = r;   // set for future use
		m_DragStart2D = r.TopLeft();  // set for future use
		m_DragEnd2D = r.BottomRight();
		DrawInvertedRectangle( m_DraggingRect );
	// update mouse status if needed
	if( m != MOUSE_NO_ACTION )
		SetMouse( m );
	return;
}

void CGraphicView::StartInvertedLine( CPoint start, CPoint end, m_ETMouseState m )
{
	CPoint3D tempP;
	//TRACE( "Get 3D point call should be changed to get snapped point." ); 
	if( GetSnapped3DPoint( start, tempP ) )
		m_DragStart3D = tempP;
	else  // RDV Note - Had to add the following because we sometimes failed the GetSnapped3DPoint so we need somewhere to begin
		m_DragStart3D = m_CurSnapped3DPt;

	if( GetSnapped3DPoint( end, tempP ) )
		m_DragEnd3D = tempP;
	else
		m_DragEnd3D = m_CurSnapped3DPt;

	// we want the line to start at the snapped location
	CPoint snappedStartPoint( start );  
	GetScreenPtFrom3DPoint( m_DragStart3D, snappedStartPoint );
	CPoint snappedEndPoint( end );  
	GetScreenPtFrom3DPoint( m_DragEnd3D, snappedEndPoint );

	if( ini.graphics().rubberBand )
		m_invertedLine.StartLine( m_hWnd, m_pDC, snappedStartPoint, snappedEndPoint, ini.size().member );

	m_DragStart2D = snappedStartPoint;  // set for future use
	m_DragEnd2D = snappedEndPoint;

	// update mouse status if needed
	if( m != MOUSE_NO_ACTION )
		SetMouse( m );
	return;
}

void CGraphicView::RedrawInvertedLine()
{
	// we want the line to start at the snapped location
	CPoint snappedStartPoint;  
	GetScreenPtFrom3DPoint( m_DragStart3D, snappedStartPoint );
	CPoint snappedEndPoint;  
	GetScreenPtFrom3DPoint( m_DragEnd3D, snappedEndPoint );
	m_DragStart2D = snappedStartPoint;  // set for future use
	m_DragEnd2D = snappedEndPoint;
	if( ini.graphics().rubberBand )
	{
		m_invertedLine.MoveLineTo( m_hWnd, m_pDC, snappedStartPoint, snappedEndPoint, ini.size().member );
		//m_invertedLine.StartLine( m_hWnd, m_pDC, snappedStartPoint, snappedEndPoint, ini.size().member );
	}
}

void CGraphicView::StartInvertedPolygon( CPoint start, CPoint end, m_ETMouseState m )
{
	//clear the points and the inverted line array
	mElement2D.clear();
	mElement3D.clear();
	
	mSideBeingDrawn = 1;

	m_DragStart2D = start;  // set for future use
	m_DragEnd2D = end;
	
	//add the 2D start and end point 
	mElement2D.push_back( CPoint( start ) );  // set for future use
	mElement2D.push_back( CPoint( end ) );

	//now add the 3D start and end points
	CPoint3D pt3D_start;
	CPoint3D pt3D_end;

	if( !GetSnapped3DPoint( start, pt3D_start ) )
	{
		TRACE( "Get snapped polygon start point failed!");
		pt3D_start = m_CurSnapped3DPt;
	}

	if( !GetSnapped3DPoint( end, pt3D_end ) )
	{
		TRACE( "Get snapped polygon end point failed!");
		pt3D_end = m_CurSnapped3DPt;
	}

	m_DragStart3D = pt3D_start;
	m_DragEnd3D = pt3D_end;

	//draw the first side of the polygon
	if( ini.graphics().rubberBand )
		m_invertedLine.StartLine( m_hWnd, m_pDC, mElement2D[0], mElement2D[1], ini.size().member );

	// 3d start point is the first point in the 3d element array
	mElement3D.push_back( CPoint3D( pt3D_start.x, pt3D_start.y, pt3D_start.z ) );

	// update mouse status if needed
	if( m != MOUSE_NO_ACTION )
		SetMouse( m );

	return;
}

void CGraphicView::RedrawInvertedPolygon( )
{
	CPoint snappedPoint( m_LastMouseMove ); 
	GetSnappedScreenPt( snappedPoint );
	mElement2D.clear();
	for( int i = 0; i < mElement3D.size(); i++ )
	{
		CPoint pt;
		GetScreenPtFrom3DPoint( mElement3D[i], pt );
		if( i == 0 )
			m_DragStart2D = pt;
		mElement2D.push_back( pt );
	}
	mElement2D.push_back( snappedPoint );
	ASSERT_RETURN( mElement2D.size() > mSideBeingDrawn  );
	if( snappedPoint == m_DragEnd2D )
		return;
	m_DragEnd2D = snappedPoint;
	mElement2D[mSideBeingDrawn] = m_DragEnd2D; // update
	DrawInvertedPolygon( m_DragEnd2D );
}

void CGraphicView::MoveInvertedRectangle( CPoint mousePt )
{
	// now update the location
	DrawInvertedRectangle( m_DraggingRect );
	m_DraggingRect.right = mousePt.x;
	m_DraggingRect.bottom = mousePt.y;  
	DrawInvertedRectangle( m_DraggingRect );  // turn on current location
	m_DragEnd2D = mousePt;
	return;
}

void CGraphicView::MoveInvertedPolygonEndPoint( CPoint end )
{
	ASSERT_RETURN( mElement2D.size() > mSideBeingDrawn  );
	if( end == m_DragEnd2D )
		return;
	if( m_invertedPoly.m_bIsOn )
		DrawInvertedPolygon( m_DragEnd2D );
	m_DragEnd2D = end;
	mElement2D[mSideBeingDrawn] = m_DragEnd2D; // update
	DrawInvertedPolygon( m_DragEnd2D );
}

void CGraphicView::MoveInvertedLine( CPoint end )
{
	// now update end point	
	if( end == m_DragEnd2D )
		return;
	if( GetSnapped3DPoint( end, m_DragEnd3D ) )
	{
		DrawInvertedLine( m_DragStart2D, m_DragEnd2D ); 
		m_DragEnd2D = end;
		DrawInvertedLine( m_DragStart2D, m_DragEnd2D );  // on with the new
	}
	return;
}

void CGraphicView::EndInvertedRectangle( )
{
	return;
}

void CGraphicView::EndInvertedLine( m_ETMouseState m )
{
	// update mouse status if needed
	if( m != MOUSE_NO_ACTION )
		ReleaseMouse( m );
	if( ini.graphics().rubberBand )
		m_invertedLine.EndLine( m_hWnd, m_pDC, ini.size().member );
	return;
}

void CGraphicView::EndInvertedPolygon( m_ETMouseState m )
{
	// update mouse status if needed
	if( m != MOUSE_NO_ACTION )
		ReleaseMouse( m );
	return;
}

void CGraphicView::EndMemberDrawing( const CPoint& /*p*/ )
{
	EndInvertedLine( MOUSE_PENCIL_DRAGGING );
	ReleaseCapture();
	SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );
	bool bIsAcceptableLength = ( m_DragStart3D.distance_to( m_DragEnd3D ) > 1.E-5 );
	if( bIsAcceptableLength ) 
	{
		mDrawGeneration = true;  // this is done so that messaging that results from the 
		 // controller's generation does not mess up other things in this window
		resetGenerateMessageCounts();
		theApp.recordCommand( DRAG_MEMBER_CMD );
		CCoordinate c1( m_DragStart3D.x, m_DragStart3D.y, m_DragStart3D.z );
		CCoordinate c2( m_DragEnd3D.x, m_DragEnd3D.y, m_DragEnd3D.z );

		if( generateDrawnMember( c1, c2 ) ) 
		{
			Invalidate();
		}
		mDrawGeneration = false;
	} 
}

void CGraphicView::EndPlateSideDrawing( const CPoint& p )
{
	CPoint snappedPoint( p );
	CPoint3D model3D;
	bool bValidPoint = GetSnapped3DPoint( snappedPoint, model3D );
	GetSnappedScreenPt( snappedPoint );
	
	bool matchesFirst = false;
	if( bValidPoint  )   // we are close to a node
	{
		CPoint3D point3D( model3D );
		bool matchesOther = false;
		for( int s = 0; s < mSideBeingDrawn; s++ ) 
		{
			if( mSideBeingDrawn <= mElement3D.size()+1 ){
				if( equal( mElement3D[s].x, point3D.x ) &&
					equal( mElement3D[s].y, point3D.y ) &&
					equal( mElement3D[s].z, point3D.z ) ) 
				{
					matchesOther = true;
					if( s == 0 ) matchesFirst = true;
				}
			}
		}
		if( !matchesOther || ( matchesFirst && mSideBeingDrawn == 3 ) ) 
		{
			TRACE("Plate side being draw = %i\n", mSideBeingDrawn );
			mSideBeingDrawn++;
			mElement3D.push_back( point3D );
			mElement2D.push_back( snappedPoint );
			ASSERT_RETURN( (int)mElement2D.size() == mSideBeingDrawn+1 );
			MoveInvertedPolygonEndPoint( snappedPoint );	// make sure we get rid of the "last" box
		}
		else	// too close to the first snappedPoint, just ignore - maybe user double clicked!
		{
			MoveInvertedPolygonEndPoint( snappedPoint );	// make sure we get rid of the "last" box
			SetCursor( theApp.LoadCursor( "ELEMENT_CURSOR"  ) );  //element cursor
			return;
		}
	}
	if( mSideBeingDrawn == 4 )
	{
		EndInvertedPolygon( MOUSE_PENCIL_DRAGGING );
		EndInvertedLine( MOUSE_PENCIL_DRAGGING );
		ReleaseMouse( MOUSE_PENCIL_DRAGGING );
		ReleaseCapture();
 		SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );
		// generate the element
      	mDrawGeneration = true;
		resetGenerateMessageCounts();

		theApp.recordCommand( DRAG_PLATE_CMD );

		//lint:  double elementX[5], elementY[5], elementZ[5];
		//for( int s = 0; s < 4; s++ ) 
		//{
		//	elementX[s] = mElement3D[s].x;
		//	elementY[s] = mElement3D[s].y;
		//	elementZ[s] = mElement3D[s].z;
		//}
		// How should we treat member intersections?
		ETSplitMethod method = (theStructure.isATruss() ? SPLIT_SIMPLE : SPLIT_COMBINE);
		SHORT altState = GetKeyState( VK_MENU );
		if( (altState & 0x8000) ) 
		{
			method = SPLIT_NOT;
		}
		if( ini.desktop().alwaysAskToSplitMembers ) 
		{
			method = SPLIT_ASK_USER;
		}
		CCoordinate c1( mElement3D[0] );
		CCoordinate c2( mElement3D[1] );
		CCoordinate c3( mElement3D[2] );
		CCoordinate c4( mElement3D[3] );
		bool bIsTriangle = ( equal( mElement3D[2].x, mElement3D[3].x ) &&
			                 equal( mElement3D[2].y, mElement3D[3].y ) &&
							 equal( mElement3D[2].z, mElement3D[3].z ) );
		if( generateDrawnPlanar( c1, c2, c3, c4, bIsTriangle, method ) )   
		{
			Invalidate();
		}
		//VAWindow()->SendMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
		mDrawGeneration = false;
		mElement3D.clear();
		mElement2D.clear();
		Invalidate();
	}  // side 4 drawn
}

void CGraphicView::EndAreaSideDrawing(const CPoint& p )
{
	CPoint snappedPoint( p );
	CPoint3D model3D;
	bool bValidPoint = GetSnapped3DPoint( snappedPoint, model3D );
	GetSnappedScreenPt( snappedPoint );

	bool matchesFirst = false;
	if( bValidPoint  )   // we are close to a node or grid point
	{
		CPoint3D point3D( model3D );
		bool matchesOther = false;
		for( int s = 0; s < mSideBeingDrawn; s++ ) 
		{
			if( mSideBeingDrawn <= mElement3D.size() ){
				if( equal( mElement3D[s].x, point3D.x ) &&
					equal( mElement3D[s].y, point3D.y ) &&
					equal( mElement3D[s].z, point3D.z ) ) 
				{
					matchesOther = true;
					if( s == 0 ) matchesFirst = true;
				}
			}
		}
		if( !matchesOther || ( matchesFirst ) ) 
		{
			TRACE("Area side being draw = %i\n", mSideBeingDrawn );
			mSideBeingDrawn++;
			mElement3D.push_back( point3D );
			mElement2D.push_back( snappedPoint );
			ASSERT_RETURN( (int)mElement2D.size() == mSideBeingDrawn+1 );
			//MoveInvertedPolygonEndPoint( snappedPoint );	// make sure we get rid of the "last" box
		}
		else	// too close to the first snappedPoint, just ignore - maybe user double clicked!
		{
			//MoveInvertedPolygonEndPoint( snappedPoint );	// make sure we get rid of the "last" box
			SetCursor( theApp.LoadCursor( "ELEMENT_CURSOR"  ) );  //element cursor
			return;
		}
	}
	if( matchesFirst )
	{
		EndInvertedPolygon( MOUSE_PENCIL_DRAGGING );
		EndInvertedLine( MOUSE_PENCIL_DRAGGING );
		ReleaseMouse( MOUSE_PENCIL_DRAGGING );
		ReleaseCapture();
		SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );
		// generate the element
		mDrawGeneration = true;
		resetGenerateMessageCounts();

		theApp.recordCommand( DRAG_AREA_CMD );

		CChain chain;
		for( int i = 0; i < mSideBeingDrawn-1; i++ ) {
			CPoint3D pt;
			pt.x = mElement3D[i].x;
			pt.y = mElement3D[i].y;
			pt.z = mElement3D[i].z;
			chain.addPoint( pt.x, pt.y, pt.z );
		}

		//check to see if this plane is parallel to another plane
		//if it isn't then we should allow this to be a new area otherwise
		//we need to check that it is not an overlapping area

		//the (potentially slow) overlap algorithm:
		// JL Note: This algorithm should probably go in the chain or area class
		bool bItsAnArea = false;
		bool bItsAHole = false;
		bool bItsACorridor = false;
		bool bOverlaps = false;
		bool bHoleOrCorridor = false;
		bool bItFormsAPlane = chain.formsAPlane();
		const CArea* pExistingArea = NULL;
		if( bItFormsAPlane )
		{
			for( int iArea = 1; iArea <= theModel.areas(); iArea++ ){
				int pts_inside = 0;
				pExistingArea = theModel.area( iArea );
				pts_inside = 0;
				ASSERT_RETURN( pExistingArea->formsAPlane() );
				ASSERT_RETURN( chain.formsAPlane() );
				//only test if the areas are in the same plane
				if( pExistingArea->plane() == chain.plane() ){
					// 4 tests should catch most of the cases - modify at your own risk
					// here we go....
					// 1: chain perfectly fits over area - warning: not checking order
					int coincident_pts = 0;
					for( int iChainPt = 1; iChainPt <= chain.points(); iChainPt++ ){
						const CCoordinate* chain_pt = chain.point( iChainPt );
						for( int iAreaPt = 1; iAreaPt <= pExistingArea->boundary().points(); iAreaPt++ ){
							if( zero(CPoint3D(*pExistingArea->boundary().point(iAreaPt)).distance_to( *chain_pt ) ) )
								coincident_pts++;
						}
					}
					if( coincident_pts == chain.points() ){
						// before we sell out here, see if one of the lines in the chain is crossing inside the area
						bool insideHit = false;
						for( int iChainPt = 1; iChainPt <= chain.points(); iChainPt++ ){
							const CCoordinate* chain_pt = chain.point( iChainPt );
							int iNext = iChainPt + 1;
							if( iChainPt == chain.points() ) iNext = 1;
							const CCoordinate* chain_pt1 = chain.point( iNext );
							const CCoordinate midpoint( (chain_pt->x() + chain_pt1->x())/2.,(chain_pt->y() + chain_pt1->y())/2.,
								(chain_pt->z() + chain_pt1->z())/2. );
							ETAdvancedAreaHit h = pExistingArea->advancedHitTest( midpoint );
							if( h == INSIDE_BOUNDARY_HIT ) {
								insideHit = true;
								break;
							}
						}  // for iChainPt							
						if( insideHit ) 
							bHoleOrCorridor = true;
						else
							bOverlaps = true;
						break;
					}
					// 2: chain is completely inside an area
					pts_inside = 0;
					for( int iChainPt = 1; iChainPt <= chain.points(); iChainPt++ ){
						const CCoordinate& chain_pt = *chain.point( iChainPt );
						bool bOnBoundary = false;
						bool bOnBoundaryCorner = false;
						if( pExistingArea->isPointInArea( chain_pt, &bOnBoundary, &bOnBoundaryCorner ) ){
							if( bOnBoundary == false && bOnBoundaryCorner == false )
								pts_inside++;
						}
					}
					if( pts_inside == chain.points() ){
						bHoleOrCorridor = true;
						break;
					}
					//3: area is completely inside the chain
					pts_inside = 0;
					for( int iAreaPt = 1; iAreaPt <= pExistingArea->boundary().points(); iAreaPt++ ){
						const CCoordinate* area_pt = pExistingArea->boundary().point( iAreaPt );
						if( chain.isPointContained( *area_pt, true ) ){
							pts_inside++;
						}
					}
					if( pts_inside == pExistingArea->boundary().points() ){
						bOverlaps = true;
						break;
					}
					
					//4: line intersection 
					//WARNING! possible O(n*m) per area
					for( int iChainPt = 1; iChainPt <= chain.points(); iChainPt++){
						int iNextChainPt = iChainPt + 1;
						if( iChainPt == chain.points() ) iNextChainPt = 1;
						CLine3D chain_line( *chain.point( iChainPt ), *chain.point( iNextChainPt ) );
						for( int iAreaPt = 1; iAreaPt <= pExistingArea->boundary().points(); iAreaPt++ ){
							int iNextAreaPt = iAreaPt + 1;
							if( iAreaPt == pExistingArea->boundary().points() ) iNextAreaPt = 1;
							CLine3D area_line( *pExistingArea->boundary().point( iAreaPt ), 
											   *pExistingArea->boundary().point( iNextAreaPt ) );
							//touching lines are ignored, as in a tee-intersection or co-linear lines
							if( chain_line.JL_lines_cross( area_line ) ){
								bOverlaps = true;
								break;
							}
							// check for lines parallel and touching
							if( chain_line.is_parallel( area_line ) )
							{
								int touchCount = 0;
								if( chain_line.is_on_segment( *pExistingArea->boundary().point( iAreaPt ) ) )
									touchCount++;
								if( chain_line.is_on_segment( *pExistingArea->boundary().point( iNextAreaPt ) ) )
									touchCount++;
								if( area_line.is_on_segment( *chain.point( iChainPt ) ) )
									touchCount++;
								if( area_line.is_on_segment( *chain.point( iNextChainPt ) ) )
									touchCount++;
								if( touchCount >= 2 ) {
									bOverlaps = true;
									break;
								}
							}
						}
						if( bOverlaps ) break;
					}
					if( bOverlaps ) 
						break;
				}
			}
			if( bOverlaps ){
				//overlapping area
				bItsAnArea = false;
				bItsACorridor = false;
				bItsAHole = false;
				int insideCount = 0;
				int outsideCount = 0;
				int onBoundaryCount = 0;
				for( int iChainPt = 1; iChainPt <= chain.points(); iChainPt++ ){
					const CCoordinate& chain_pt = *chain.point( iChainPt );
					bool bOnBoundary = false;
					bool bOnBoundaryCorner = false;
					if( pExistingArea->isPointInArea( chain_pt, &bOnBoundary, &bOnBoundaryCorner ) ){
						if( bOnBoundary == false && bOnBoundaryCorner == false )
							insideCount++;
						else
							onBoundaryCount++;
					}
					else
						outsideCount++;
				}
				// can we make a boundary change by subtracting this area
				bool subtract = false;
				if( insideCount > 0 && outsideCount == 0 && onBoundaryCount >= 2 )
					subtract = true;

				bool doTheMerge = false;

				if( subtract == false ) {
					static int lastChoice = 1;
					CChoiceDlg dlg( this, 
						"What would you like to do with the area you drew?", 
						"&Merge it with the area it touches.", 
						"Make it a &new area.", 
						"&Oops! That's not what I had in mind.",
						lastChoice );
					lastChoice = dlg.DoModal();
					if( lastChoice == 1 ) 
						doTheMerge = true;
					else if( lastChoice == 2 ) {  // new area
						bItsAnArea = true;
					}
				}  // subtract == false
				else {
					static int lastChoice = 1;
					CChoiceDlg dlg( this, 
						"What would you like to do with the area you drew?", 
						"Subtact it from the area it touches creating a &hole.", 
						"Add it to the area it touches creating a &corridor.",
						"&Oops! That's not what I had in mind.",
						lastChoice );
					lastChoice = dlg.DoModal();
					if( lastChoice == 1 ) 
						doTheMerge = true;
					else if( lastChoice == 2 )
						bItsACorridor = true;
				}

				if( doTheMerge )
				{
					// do a merge
					CChain newChain;
					if( mergeChains( chain, pExistingArea->boundary(), *pExistingArea, newChain, subtract ) )
					{
						// modify the area's boundary
						CArea* pNewArea = new CArea( *pExistingArea );
						pNewArea->boundary( newChain );
						resetCopyFlags();
						setCopyFlag( CArea::COPY_BOUNDARY );
						theController.modify( pExistingArea, pNewArea );
					}
				}
			}
			else if( bHoleOrCorridor ) {
				//hole or corridor
				static int lastChoice = 1;
				CChoiceDlg dlg( this, 
					"Would you like to make this area a Hole or a Corridor?", 
					"Make it a &Hole.", 
					"Make it a &Corridor.", 
					"&Oops! That's not what I had in mind.",
					lastChoice );
				lastChoice = dlg.DoModal();
				if( lastChoice == 1 ) bItsAHole = true;
				else if( lastChoice == 2 ) bItsACorridor = true;
			}
			else{
				bItsAnArea = true;
			}

			if( bItsAnArea ){
				// RDV Note - Added the following to be consistent with area's normal in the model
				bool needReverse = false;
				bool isAPlane = chain.formsAPlane(); // make sure normal is setup
				ASSERT_RETURN( isAPlane );
				CVector3D n( chain.normal().x(), chain.normal().y(), chain.normal().z() );
				if( n.direction() == CVector3D::MINUS_X || n.direction() == CVector3D::MINUS_Y ||
					n.direction() == CVector3D::MINUS_Z )
					needReverse = true;  // always make sure normal points in positive global direction
				else if( n.direction() != CVector3D::PLUS_X && n.direction() != CVector3D::PLUS_Y &&
					n.direction() != CVector3D::PLUS_Z ) {  // make sure sloped area has + coordinate in building vertical
						if( theProject.designCriteria().getVerticalDirection() == X && n.x < 0. ) 
							needReverse = true;
						else if( theProject.designCriteria().getVerticalDirection() == Y && n.y < 0. )
							needReverse = true;
						else if( theProject.designCriteria().getVerticalDirection() == Z && n.z < 0. )
							needReverse = true;
				}
				if( needReverse ) 
					chain.reversePoints();
				CArea* pArea = new CArea(theController.lastArea());
				pArea->boundary( chain );
				pArea->select();
				pArea->name( NULL );
				// make sure this new area is free of holes and corridors
				pArea->removeAllHoles();
				pArea->removeAllCorridors();
				// TeK Add 10/31/2007: If areas are not shown, turn them on!
				if( !m_Filter.areas ) {
					m_Filter.areas = true;
				}
				theController.add( pArea );

			}
			else if( bItsAHole && pExistingArea ){
				CArea* pNewArea = new CArea( *pExistingArea );
				pNewArea->addHole( chain );
				resetCopyFlags();
				setCopyFlag( CArea::COPY_HOLES );
				theController.modify( pExistingArea, pNewArea );
			}
			else if( bItsACorridor && pExistingArea ){
				CArea* pNewArea = new CArea( *pExistingArea );
				pNewArea->addCorridor( chain );
				resetCopyFlags();
				setCopyFlag( CArea::COPY_CORRIDORS );
				theController.modify( pExistingArea, pNewArea );
			}
		}
		else{
			AfxMessageBox( "This area does not form a plane.  Please check the coordinates of points to insure they are planar." );
		}
		VAWindow()->PostMessage( SELCHANGE_MESSAGE, (WPARAM)0, (LPARAM)this );
		mDrawGeneration = false;
		mElement3D.clear();
		mElement2D.clear();
		Invalidate();
	}  // side 4 drawn
}


//void CGraphicView::EndStressPathDrawing( const CPoint& /*_point*/ )
//{
//	AfxMessageBox( "The Stress Path Graph feature has been removed for repair.  IES apologizes for the inconvenience.", MB_ICONSTOP );
//	EndInvertedLine( MOUSE_PENCIL_DRAGGING );
//	ReleaseCapture();
//	SetCursor( theApp.LoadStandardCursor( IDC_ARROW ) );
//	//CCoordinate c1( m_DragStart3D.x, m_DragStart3D.y, m_DragStart3D.z );
//	//CCoordinate c2( m_DragEnd3D.x, m_DragEnd3D.y, m_DragEnd3D.z );
//	//CString msg; msg.Format( "(%lf,%lf,%lf) to (%lf,%lf,%lf)", c1.x(), c1.y(), c1.z(), 
//	//	c2.x(), c2.y(), c2.z() );
//	//bool bIsAcceptableLength = ( c1.distanceTo( c2 ) > 1.E-5 );
//	//if( bIsAcceptableLength && m_Filter.resultCase() ) 
//	//{
//	//	CStressPath stressPath( c1, c2, *(m_Filter.resultCase()) );
//	//	if( stressPath.isLineValidStressPath() ) {
//	//		// We may want to ask if performance becomes an issue?
//	//		//int plotStressPath = AfxMessageBox( "Would you like to create a stress path plot?", MB_YESNO );
//	//		//if( IDYES == plotStressPath ) {
//	//		CDoubleArray lengthValues; 
//	//		stressPath.getLengthArray( lengthValues );
//
//	//		CDoubleArray totalValues;
//	//		CDoubleArray averageValues;
//	//		CDoubleArray maxValues;
//	//		double p1Coords[3];
//	//		double p2Coords[3];
//	//		CString plateNames;
//	//		CString p1PlateName;
//	//		CString p2PlateName;
//	//		stressPath.returnStressPathSummaryData( averageValues, totalValues, maxValues,
//	//			p1Coords, p2Coords, plateNames, p1PlateName, p2PlateName );
//
//	//		CString startLocation[2];
//	//		CString endLocation[2];
//	//		startLocation[0] = p1PlateName;
//	//		endLocation[0] = p2PlateName;
//	//		if( theStructure.usesCoordinate(Z) ) {
//	//			startLocation[1].Format( "(%s, %s, %s)", Units::show(LENGTH_LARGE_UNIT , p1Coords[0] ), 
//	//				Units::show(LENGTH_LARGE_UNIT , p1Coords[1] ),
//	//				Units::show(LENGTH_LARGE_UNIT , p1Coords[2] ) );
//	//			endLocation[1].Format( "(%s, %s, %s)", Units::show(LENGTH_LARGE_UNIT , p2Coords[0] ), 
//	//				Units::show(LENGTH_LARGE_UNIT , p2Coords[1] ),
//	//				Units::show(LENGTH_LARGE_UNIT , p2Coords[2] ) );
//	//		}
//	//		else {
//	//			startLocation[1].Format( "(%s, %s)", Units::show(LENGTH_LARGE_UNIT , p1Coords[0] ), 
//	//				Units::show(LENGTH_LARGE_UNIT , p1Coords[1] ) );
//	//			endLocation[1].Format( "(%s, %s)", Units::show(LENGTH_LARGE_UNIT , p2Coords[0] ), 
//	//				Units::show(LENGTH_LARGE_UNIT , p2Coords[1] ) );
//	//		}
//
//
//	//		TPropertySheet ps( "Stress Path Graphs", AfxGetMainWnd(), 0, TRUE );
//	//		ps.m_psh.dwSize = sizeof( ps.m_psh );
//	//		ps.m_psh.dwFlags |= PSH_NOAPPLYNOW;
//	//		ps.m_psh.dwFlags |= (PSH_PROPSHEETPAGE | PSH_HASHELP); 
//	//
//	//		for( int i = 0; i < StressPathTypes; i++ ) {
//	//			ETStressPathType type = ETStressPathType(i);
//	//			if( !zero(maxValues[i+1]) ) {
//	//				CStressPathGraph* pGraph = new CStressPathGraph( &ps, type );
//	//				pGraph->SetUpStressPathData( stressPath.getStressArray( type, 0 ), 
//	//					stressPath.getStressArray( type, 1 ), stressPath.getStressArray( type, 2 ),
//	//					lengthValues, m_Filter.resultCase(), plateNames, startLocation, endLocation );
//	//				ps.AddPage( pGraph );
//	//			}
//	//		}
//
//	//		CStressPathSummary summaryPage( &ps );
//	//		summaryPage.SetUpStressPathSummaryData( averageValues, totalValues, p1Coords, p2Coords, plateNames );
//	//		ps.AddPage( &summaryPage );
//
//	//		/*int dlgReturn =*/ ps.DoModal();
//	//		for( int i = ps.GetPageCount()-2; i >= 0; i-- ) {
//	//		CPropertyPage* pg = ps.GetPage(i);
//	//		ps.RemovePage(i);
//	//		delete pg;
//	//		}
//	//	}
//	//	else {
//	//		AfxMessageBox( "Not a valid stress path.  Make sure your line lies close to the plate boundary nodes.   Please try drawing path again.", MB_OK );
//	//	}
//	//} 
//	return;
//}
//

void CGraphicView::CmUpArrowPress( void )
{
	VAWindow()->mModelCommandBar.ScrollLoadCaseCombo( -1 );
	VAWindow()->mResultCommandBar.ScrollLoadCaseCombo( -1 );
	SetFocus();
	return;
}


void CGraphicView::CmDownArrowPress( void )
{
	VAWindow()->mModelCommandBar.ScrollLoadCaseCombo( 1 );
	VAWindow()->mResultCommandBar.ScrollLoadCaseCombo( 1 );
	SetFocus();
	return;
}

