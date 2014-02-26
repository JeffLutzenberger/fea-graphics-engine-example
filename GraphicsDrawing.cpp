// GraphicsDrawing.cpp : implementation file
// Drawing support

#include "stdafx.h"
#pragma hdrstop

#include "GraphicView.h"
#include "GraphChildFrame.h"

#include "main/GraphicsCamera.h"

#include "Graphics/GraphicsHelpers.h"
#include "Graphics/GraphicsHelperClasses.h"
#include "GraphicsContourBar.h"

#include "Design/GroupManager.h"	// theGroupManager
#include "Design/Mesh.h"			// theMeshManager
#include "Model.h"
#include "Project.h"
#include "ServCase.h"
#include "movload.h"
#include "IniSize.h"
#include "IniColor.h"
#include "IniFilter.h"
#include "IniFont.h"
#include "IniDesk.h"
#include "user/change.h"
#include "graphics/GridManager.h"
#include "VAStatus.h"

#include "OldDataUnits.h" // PercentageQuantity -- May 2006, this should go away soon...

#include "user/control.h"

#include "engineering\FEM Engine\Engine.h" // the Engine!
#include "VisualDesign/VDInterface.h" // VDUnitySuccessLimit()

#include <gl/gl.h>
#include <gl/glu.h>

#define ZOOM_TOLERANCE 0.01

#define TEMPORARY_SCALE_FACTOR 10.

#define TEMPORARY_NUM_MEMBER_DIV	4

#ifdef _DEBUG
	#undef THIS_FILE
	static char THIS_FILE[]=__FILE__;
	#define new DEBUG_NEW
#endif

//#define CRLF "\x0D\x0A"
// Actual Drawing support

void drawRects()
{
   glLoadName(1);
   glBegin(GL_QUADS);
   glColor3f(1.0, 1.0, 0.0);
   glVertex3i(2, 0, 0);
   glColor3f(1.0, 0.0, 0.0);
   glVertex3i(2, 6, 0);
   glColor3f(0.0, 1.0, 0.0);
   glVertex3i(6, 6, 0);
   glVertex3i(6, 0, 0);
   glEnd();
   glLoadName(2);
   glBegin(GL_QUADS);
   glColor3f(0.0, 1.0, 1.0);
   glVertex3i(3, 2, -1);
   glVertex3i(3, 8, -1);
   glVertex3i(8, 8, -1);
   glVertex3i(8, 2, -1);
   glEnd();
   glLoadName(3);
   glBegin(GL_QUADS);
   glColor3f(1.0, 0.0, 1.0);
   glVertex3i(0, 2, -2);
   glVertex3i(0, 7, -2);
   glVertex3i(5, 7, -2);
   glVertex3i(5, 2, -2);
   glEnd();
}

void CGraphicView::OnDraw(CDC* /*pDC*/)
{
	//CTimeThisFunction t( "OnDraw" );
	if( ::IsIconic( GetParent()->m_hWnd ) ) 
		return;  // nothing to do

	if( !m_bForceDraw &&
		(Mouse( MOUSE_TEXT_DRAGGING ) ||
		 Mouse( MOUSE_ANNOTATION_DRAGGING ) ||
		 Mouse( MOUSE_ZOOMBOX_DRAGGING ) ||
		 Mouse( MOUSE_SELECTIONBOX_DRAGGING ) ||
		 Mouse( MOUSE_PENCIL_DRAGGING ) ||
		 Mouse( MOUSE_NODE_DRAGGING ) ) ) 
		 return;

	ASSERT_RETURN( m_pDC && m_pDC->GetSafeHdc() );

	// set the first result case if one exists
	if( !m_bHasBeenDrawnOnce && m_Filter.resultCase() == NULL )
	{
      if( theProject.haveAnyResults() )
            m_Filter.resultCase( theProject.resultCase( 1 ) );
      else
            m_Filter.resultCase( NULL );
	}

	if( ::IsWindow(m_analysisTextWindow.GetSafeHwnd()) ) {
		if( m_Filter.windowType == POST_WINDOW && m_Filter.resultCase() == NULL ) {
			if( theEngine.engineIsActive() || // engine is working put up the progress messages
				( !theProject.haveAnyResults() && theEngine.backgroundProcessing() ) ) {  // analysis errors with background engine on
				RECT r;
				GetClientRect( &r );
				m_analysisTextWindow.SetWindowPos( NULL, 0, 0, r.right, r.bottom, SWP_NOZORDER|SWP_SHOWWINDOW );
				return;
			}
			if( theProject.haveAnyResults() ) {
				m_Filter.resultCase( theProject.resultCase( 1 ) );
				if( m_Filter.resultCase() != NULL )
					m_analysisTextWindow.ShowWindow( SW_HIDE );
			}
			else {  // we've got a results view with no results and the engine is not working, only choice is to switch 
					// back to a model view
				CGraphicWindowChildFrame* parent = dynamic_cast<CGraphicWindowChildFrame*>(GetParent()->GetParent()) ;
				if( parent )
					parent->SetTab( 0 );
			}
		}else {
			m_analysisTextWindow.ShowWindow( SW_HIDE );
		}
	}

	if( !wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC ) )
		return;
	
	COLORREF c = ini.color().background;
	float color[3] = { float( double(GetRValue(c))/255. ), 
		float( double(GetGValue(c))/255. ),
		float( double(GetBValue(c))/255. )};
	ClearGLBG( color );

	if( m_Filter.drawDetail == ETGraphicDetail( DETAIL_LOW ) || !ini.graphics().lighting )
		DisableGLLighting();
	else
		ApplyDefaultGLLight();
	
	bool disableText = false;
	if( m_rotationCube.Animating() )
	{
		//update the camera's angles
		disableText = true;
		CPoint3D rotations;
		m_rotationCube.GetAnimatingRotation( rotations );
		m_Camera.SetRotations( rotations );
	}
	//move the camera and rotate the model
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glDepthRange(0.0, 1.0);  /* The default z mapping */
	
	m_Camera.ResetViewport();
	m_Camera.SetVerticalAxis( theProject.designCriteria().getVerticalDirection() );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CPoint snappedPoint( m_LastMouseButton );  // this one gets "snapped"
	GetSnappedScreenPt( snappedPoint );
	m_CurSnapped2DPt = snappedPoint;

	//drawRects();
	if( m_rotationCube.Animating() )
		m_graphicsModel.StartReducedDrawing();

	DrawModel();
	DrawBoundingBox( m_CurrentLayerBox );
	DrawGrid( true );
	DrawOriginAxes( );

	AddTitle( );

	m_rotationCube.Draw( m_Camera.GetRotations(), m_pDC );
	m_Camera.ResetViewport();

	SwapBuffers( m_pDC->GetSafeHdc() );

	//always reset this - just to be safe
	m_graphicsModel.ResumeCompleteDrawing();

	if( m_rotationCube.Animating() )
	{
		Invalidate();
	}


	if( m_bForceDraw ){
		//the user may have tried to zoom while drawing a plate or member which means
		//we need to update the inverted polygon's or inverted line's position
		if( Mouse( MOUSE_PENCIL_DRAGGING ) && m_Filter.windowType == MODEL_WINDOW )
		{
			ETDrawMode mode = m_Filter.drawMode;
			switch( mode )
			{
//			case DRAW_STRESSPATH:
			case DRAW_MEMBER:
			case DRAW_CABLE:			
				RedrawInvertedLine();
				break;
			case DRAW_PLANAR:
			case DRAW_AREA:
				RedrawInvertedPolygon();
				break;
			default:
				//ASSERT( false );
				break;
			}
		}
	}
	m_bForceDraw = false;


	//check for errors...
	GLenum GLError = glGetError();
	ASSERT( glGetError() == GL_NO_ERROR );

	if( GLError != GL_NO_ERROR )
	{
		TRACE( "Warning - Error in OpenGL: %i\n", GLError ); 
	}

	wglMakeCurrent( NULL, NULL );
 
	m_bHasBeenDrawnOnce = true;

	return;
}

void CGraphicView::DrawForPrinter( CDC* pDC, CPrintInfo* pInfo )
{
	CWaitCursor wc; 
	if( ::IsIconic( GetParent()->m_hWnd ) ) 
		return;  // nothing to do

	ASSERT_RETURN( pDC );

	//////////////////////////////////////////////////////
	m_TitleText.ClearFonts();
	m_graphicsModel.ClearFonts();

	CDC* pDCTemp = m_pDC;
	if( pInfo && !pInfo->m_bPreview )
	{
		//m_pTextDC = pDC;
		m_pDC = pDC;
	}

	COLORREF c = ini.color().background;
	float color[3] = { float( double(GetRValue(c))/255. ), 
		float( double(GetGValue(c))/255. ),
		float( double(GetBValue(c))/255. )};
	ClearGLBG( color );

	if( m_Filter.drawDetail == ETGraphicDetail( DETAIL_LOW ) || !ini.graphics().lighting )
		DisableGLLighting();
	else
		ApplyDefaultGLLight();

	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glDepthRange(0.0, 1.0);  /* The default z mapping */
	
	m_Camera.SetView();
	m_Camera.SetVerticalAxis( theProject.designCriteria().getVerticalDirection() );
	m_Camera.SetPrintScale( m_printScale );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	EnableGLAA();
	m_graphicsModel.SetCamera( m_Camera );
	m_graphicsModel.SetWindowFilter( m_Filter );
	m_graphicsModel.DrawModelForPrinter( m_pDC, m_printScale );
	DrawGrid( true );
	DrawOriginAxes();
	AddTitle( );

	// all done!
	glFinish();

	if( pInfo && !pInfo->m_bPreview )
	{
		m_pDC = pDCTemp;
		//m_pTextDC = pDCTemp;
	}
	m_TitleText.ClearFonts();
	m_graphicsModel.ClearFonts();

	m_Camera.SetPrintScale( 1 );

	//check for errors...
	GLenum GLError = glGetError();
	ASSERT( glGetError() == GL_NO_ERROR );

	if( GLError != GL_NO_ERROR )
	{
		TRACE( "Warning - Error in OpenGL: %i\n", GLError );
	}

	wglMakeCurrent( NULL, NULL );

	return;
}

void CGraphicView::DrawModel( )
{
	//EnableGLAA();
	m_graphicsModel.SetCamera( m_Camera );
	m_graphicsModel.SetWindowFilter( m_Filter );
	m_graphicsModel.DrawModel( m_pDC );
}

void CGraphicView::DrawBoundingBox( const COrientedBox& _box )
{
	if( !m_bDrawCurrentLayerBox ) return;
	float alpha = 1.0;
	/*clock_t deltaT = clock() - m_timeLayerBoxReset;
	if( deltaT > 1000 )
		alpha = (float)((3000-deltaT) / 1000);
	if( deltaT > 3000 ){
		m_bDrawCurrentLayerBox = false;
		return;
	}*/
	StartSolidDrawing(0);
	EnableGLTransparency( );
	COLORREF c = InverseColor( ini.color().members );
	float color[4] = { (float)(GetRValue( c )/255.), 
		(float)(GetGValue( c )/255.), 
		(float)(GetBValue( c )/255.), 
		alpha };
	SetGLColor( c );
	ApplyAmbientGLMaterial( color );
	glPushMatrix();
	DrawBox( _box );
	StartWireframeDrawing();
	glLineWidth(float( ini.size().member ) );
	c = ini.color().members;
	float color2[4] = { (float)(GetRValue( c )/255.), 
		(float)(GetGValue( c )/255.), 
		(float)(GetBValue( c )/255.), 
		alpha };
	SetGLColor( c );
	ApplyAmbientGLMaterial( color2 );
	DrawBox( _box );
	EndWireframeDrawing();
	glPopMatrix();
	DisableGLTransparency();
	EndSolidDrawing();
	//Invalidate();
}

void CGraphicView::SetCurrentLayerBox( const COrientedBox& _box )
{
	m_CurrentLayerBox = _box; 
	m_timeLayerBoxReset = clock();
	//reset the box timer -- leave it on for x milliseconds and slowly fade it out

}
void CGraphicView::DrawLayerBoundingBox( )
{
	StartSolidDrawing(0);
	EnableGLTransparency();
	COLORREF c = InverseColor( ini.color().members );
	float color[4] = { (float)(GetRValue( c )/255.), 
		(float)(GetGValue( c )/255.), 
		(float)(GetBValue( c )/255.), 
		ALPHA };
	SetGLColor( c );
	ApplyAmbientGLMaterial( color );
	// for each layer draw its bounding box
	glPushMatrix();
	glTranslatef( (float)m_CurrentLayerBox.Center().x, 
		          (float)m_CurrentLayerBox.Center().y, 
				  (float)m_CurrentLayerBox.Center().z );
	DrawBox( m_CurrentLayerBox );
	glPopMatrix();
	DisableGLTransparency();
	EndSolidDrawing();
}

void CGraphicView::DrawGrid( bool /*bPointsOnly*/ )
{
	//http://www.opengl.org/resources/code/samples/mjktips/grid/index.html
	//render ground plane grid with openGL evaluator...
	//int gridSize = 20;
	//int uSize = 4;
	//int vSize = 4;

	//float gridExtents = 500;
	//GLfloat grid2x2[2][2][3] = {
 //   {{-gridExtents, -gridExtents, 0.0}, {gridExtents, -gridExtents, 0.0}},
 //   {{-gridExtents, gridExtents, 0.0}, {gridExtents, gridExtents, 0.0}}
	//};

	//GLfloat *grid = &grid2x2[0][0][0];

	//glEnable(GL_MAP2_VERTEX_3);
	//
	//glMap2f(GL_MAP2_VERTEX_3,
 //   0.0, 1.0,  /* U ranges 0..1 */
 //   3,         /* U stride, 3 floats per coord */
 //   2,         /* U is 2nd order, ie. linear */
 //   0.0, 1.0,  /* V ranges 0..1 */
 //   2 * 3,     /* V stride, row is 2 coords, 3 floats per coord */
 //   2,         /* V is 2nd order, ie linear */
 //   grid);  /* control points */

	//glMapGrid2f( 50, 0.0, 1.0, 50, 0.0, 1.0);
	//glColor3f( 0.4f, 0.4f, 0.7f );
	//glEvalMesh2(GL_LINE,
 //   0, 50,   /* Starting at 0 mesh 5 steps (rows). */
 //   0, 50);  /* Starting at 0 mesh 6 steps (columns). */

	//glMapGrid2f( 10, 0.0, 1.0, 10, 0.0, 1.0);
	//glColor3f( 0.7f, 0.7f, 0.9f );
	//glLineWidth( 2 );
	//glEvalMesh2(GL_LINE,
 //   0, 10,   /* Starting at 0 mesh 5 steps (rows). */
 //   0, 10);  /* Starting at 0 mesh 6 steps (columns). */

	//push the grid back a bit so it does not have the same z value as our model objects
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glEnable( GL_POLYGON_OFFSET_LINE );
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glPolygonOffset( 10000000, 100000000 );
	
	if( m_Filter.windowType != MODEL_WINDOW ) return;
	bool bNeeded = false;
	for( int iGrid = 1; iGrid <= theGridManager.gridCount(); iGrid++ ) {
		CGraphicGrid* pG = theGridManager.grid( iGrid );
		if( pG && pG->isVisible() ) {
			bNeeded = true;
			break;
		}
	}
	if( !bNeeded ) return;

	DisableGLLighting();
	//push the grid back a bit so that model objects draw over it (glPolygonOffset won't work for lines)
	glDepthRange( 0.0001, 1 );
	COLORREF c = ini.color().grid;
	glColor3ub( GetRValue( c ), GetGValue( c ), GetBValue( c ) );
	glLineWidth( float(1./*pointSize*/) );
	for( int iGrid = 1; iGrid <= theGridManager.gridCount(); iGrid++ ) {
		CGraphicGrid* pG = theGridManager.grid( iGrid );
		if( pG && pG->isVisible() ) {
			float pointSize = (pG->isBigDots() ? 5.f : 2.f );
			glPointSize( pointSize );
			//bool bPoints = bPointsOnly;
			//if( dynamic_cast<CIrregularGrid*>(pG) ) {
			//	bPoints = false;
			//}

			//glColor3f( 0.8f, 0.8f, 0.9f );
			pG->DrawLines( );
			pG->DrawPoints( );
		}
	}
	glDepthRange( 0, 1 );
	return;
}

void CGraphicView::DrawGridPoints( void )
{
	COLORREF c = ini.color().grid;
	glColor3ub( GetRValue( c ), GetGValue( c ), GetBValue( c ) );
	glColor3f( 0.f, 0.f, 0.f );
	for( int iGrid = 1; iGrid <= theGridManager.gridCount(); iGrid++ ) {
		if( theGridManager.grid( iGrid )->isVisible() ) {
			theGridManager.grid( iGrid )->DrawPoints( );
		}
	}

	return;
}

void CGraphicView::DrawOriginAxes( )
{
	DrawOriginAxes( false, CPoint(), 0 );
}

void CGraphicView::DrawOriginAxes( bool /*bForSelection*/, CPoint /*p*/, int /*selSize*/ )
{
	
	if( !m_Filter.axes )
		return;
	//coordinate axes at origin
	glDisable( GL_DEPTH_TEST );
	DisableGLLighting();
	glLineWidth( (float)ini.size().axisWidth );
	glPushMatrix();
	float axisSize = (float)m_Camera.ScalePixelSize( ini.size().axis )*m_printScale;
	DrawOriginArrows( axisSize, false, true );
	glPopMatrix();
	glEnable( GL_DEPTH_TEST );
}

void CGraphicView::DrawInvertedLine( CPoint start, CPoint end )
{
	if( ini.graphics().rubberBand )
		m_invertedLine.MoveLineTo( m_hWnd, m_pDC, start, end, ini.size().member );
	//TRACE( "m_hWnd = %i\n", m_hWnd );
	return;
}

void CGraphicView::DrawInvertedAnnotation( CGraphicsAnnotationObject* /*pAnn*/ )
{
	return;
}

void CGraphicView::DrawInvertedLegend( CGraphicsContourBar* pLegend )
{
	if( !ini.graphics().rubberBand )
		return;
	ASSERT_RETURN( m_pDC && pLegend );
	if( !m_pDC || !pLegend )
		return;
	CRect wndRect;
	GetClientRect( wndRect );

	//copy front buffer to back buffer...
	glReadBuffer( GL_FRONT );
	glDrawBuffer( GL_BACK );
	glCopyPixels(wndRect.left, wndRect.top, wndRect.Width(), wndRect.Height(), GL_COLOR);

	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);

	CPoint controlPt( 0, 0 );
	pLegend->SetWindowRect( wndRect );
	pLegend->Draw( m_pDC, 0.f, 1.f, true);

	glDisable(GL_COLOR_LOGIC_OP);
	
	//swap back buffer for front buffer
	SwapBuffers( m_pDC->GetSafeHdc() );
}
void CGraphicView::DrawInvertedRectangle( CRect r )
{
	if( ini.graphics().rubberBand )
	{
		std::vector<CPoint> pts;
		pts.push_back( CPoint( r.left, r.top ));
		pts.push_back( CPoint( r.left, r.bottom ));
		pts.push_back( CPoint( r.right, r.bottom ));
		pts.push_back( CPoint( r.right, r.top ));
		m_invertedPoly.SetFill( false );
		m_invertedPoly.Draw( m_hWnd, m_pDC, pts, ini.size().member ); 
		m_invertedPoly.SetFill( true );
	}
	return;
}

void CGraphicView::DrawInvertedPolygon( CPoint /*end*/ )
{
	ASSERT_RETURN( mElement2D.size() >= mSideBeingDrawn );
	if( ini.graphics().rubberBand )
	{
		if( mSideBeingDrawn == 1 && ini.graphics().rubberBand )
			m_invertedLine.MoveLineTo( m_hWnd, m_pDC, mElement2D[0], mElement2D[1], ini.size().member );
		if( mSideBeingDrawn > 0 ) {
			std::vector<CPoint> pts;
			for( int i = 0; i < mSideBeingDrawn; i++ )
				pts.push_back( mElement2D[i] );
			pts.push_back( m_DragEnd2D );
			for( int i = 0; i < pts.size(); i++ )
			{
				CString str = "";
				str.Format( "Side %i, Point %i: (%i, %i)\n",mSideBeingDrawn, i,mElement2D[i].x, mElement2D[i].y );
				TRACE( str );
			}
			m_invertedPoly.Draw( m_hWnd, m_pDC, pts, ini.size().node ); 
		}
	}
	return;
}


bool CGraphicView::DrawLine(CPoint3D p1, CPoint3D p2, COLORREF c )
{
	//StartDrawing();
	SetGLColor( c );
	glBegin( GL_LINES );
		glVertex3d( p1.x, p1.y, p1.z );
		glVertex3d( p2.x, p2.y, p2.z );
	glEnd();
	//EndDrawing();
	return true;
}

double CGraphicView::GetDisplacementFactor() {
	// TeK Change 2/21/2008: The PROJECT holds the displacement factor based on ALL 
	// results available.  This insures that displacements scale consistently regardless
	// of which result case is visible in the window.  The engineer can scale them up or down
	// using the filter scale factor
	// How should displacements be scaled graphically?
	if( m_Filter.resultCase() ) {
		const CResultCase* pRC = m_Filter.resultCase(); // TeK Add 8/20/2008??? Need to treat mode shapes differently???
		if( m_Filter.displacements ) {
			if( zero(m_Filter.displacementFactor) ) {
			//if( zero(m_Filter.displacementFactor) || (MODE_SHAPE_RESULTS == pRC->type()) ) { // TeK Add 8/20/2008???
				return 1.0; // show "true" displacements
			}
			else {
				double pdf = theProject.displacementFactor( pRC );
				return pdf*m_Filter.displacementFactor;
			}
		}
	}
	return 0.0; // No displacements shown!
}

const CNameFilter& CGraphicView::nameFilterForGraphicObject( ETGraphicObject o )
{
	//TeK 8/4/2007: Don't set "*" on filter, it is less efficient and not necessary!
	// also make default filter static for efficiency, return type made reference (don't make a copy)
	static CNameFilter f;
	//f.set( "*" );
	switch( o ) 
	{
	case GRAPHIC_NODE:
		return m_Filter.node.filter;
	case GRAPHIC_SPRING:
		return m_Filter.spring.filter;
	case GRAPHIC_MEMBER:
		return m_Filter.member.filter;
	case GRAPHIC_PLATE:
	case GRAPHIC_AUTO_PLATE:
		return m_Filter.plate.filter;
	case GRAPHIC_CABLE:
		return m_Filter.cable.filter;
	case GRAPHIC_ALL_OBJECTS:
	default:
		return f;
	}
}

void CGraphicView::GetAttachedMemberAngles( const CNode* pN )
{
	m_MemberVectors.flush();
	int nElem = theModel.elementsAt( pN );
	for( int i = 1; i <= nElem; i++ )
	{
		const CElement* pE = theModel.elementAt( pN, i );
		if( pE )
		{
			CPoint p1, p2;
			CPoint3D p3D1, p3D2;
			p3D1.x = pE->node(1)->x();
			p3D1.y = pE->node(1)->y();
			p3D1.z = pE->node(1)->z();
			p3D2.x = pE->node(2)->x();
			p3D2.y = pE->node(2)->y();
			p3D2.z = pE->node(2)->z();
			
			GetScreenPtFrom3DPoint( p3D1, p1 );
			GetScreenPtFrom3DPoint( p3D2, p2 );
			p3D1.x = p1.x;
			p3D1.y = p1.y;
			p3D1.z = 0.;
			p3D2.x = p2.x;
			p3D2.y = p2.y;
			p3D2.z = 0.;

			if( pN == pE->node(1) )
				m_MemberVectors.add( CVector3D( p3D1, p3D2 ).normalize() );
			else
				m_MemberVectors.add( CVector3D( p3D2, p3D1 ).normalize() );
		}
	}
}

const CMember* CGraphicView::GetMemberWithThis2DOrientation( const CVector3D& v, const CNode* pN )
{
	m_MemberVectors.flush();
	int nElem = theModel.elementsAt( pN );
	for( int i = 1; i <= nElem; i++ )
	{
		const CElement* pE = theModel.elementAt( pN, i );
		if( pE )
		{
			CPoint p1, p2;
			CPoint3D p3D1, p3D2;
			p3D1.x = pE->node(1)->x();
			p3D1.y = pE->node(1)->y();
			p3D1.z = pE->node(1)->z();
			p3D2.x = pE->node(2)->x();
			p3D2.y = pE->node(2)->y();
			p3D2.z = pE->node(2)->z();
			
			GetScreenPtFrom3DPoint( p3D1, p1 );
			GetScreenPtFrom3DPoint( p3D2, p2 );
			p3D1.x = p1.x;
			p3D1.y = p1.y;
			p3D1.z = 0.;
			p3D2.x = p2.x;
			p3D2.y = p2.y;
			p3D2.z = 0.;

			if( pN == pE->node(1) && CVector3D( p3D1, p3D2 ).normalize() == v )
				return (const CMember*)pE;
			else if( pN == pE->node(2) && CVector3D( p3D2, p3D1 ).normalize() == v )
				return (const CMember*)pE;
			else
				return NULL;
		}
	}
	return NULL;

}

void CGraphicView::InsertAnnotation( CPoint3D loc, CString str, double rotation )
{
	// we are coming from a dxf file
	int index = m_AnnotationManager.annotationObjects() + 1;
	unsigned int id = SetGraphicsName( GRAPHIC_ANNOTATION_TEXT, index );
	CGraphicsAnnotationObject* pNewAnnotation = NULL;
	pNewAnnotation =
		new CGraphicsAnnotationObject( CGraphicsAnnotationObject::ANNOTATION_TEXT_ONLY, this, 
		"Arial", id );;
	pNewAnnotation->AddLine( str );
	CPoint p2;
	GetScreenPtFrom3DPoint( loc, p2 );
	pNewAnnotation->SetControlPoint( p2 );
	pNewAnnotation->SetRotationAngle( rad_to_deg(rotation) );
	m_AnnotationManager.addAnnotationObject( *pNewAnnotation );
	return;
}

void CGraphicView::GetAnnotation( int index, CPoint3D* loc, CString* str, double* rotation, double * height )
{
	// we are going to a dxf file
	ASSERT( index > 0 && index <= m_AnnotationManager.annotationObjects() );
	if( index > 0 && index <= m_AnnotationManager.annotationObjects() ) {
		CGraphicsAnnotationObject* pAnnotation = m_AnnotationManager.annotationObject( index );
		if( pAnnotation ) {			 
			CPoint off = m_AnnotationManager.offset( pAnnotation );
			if( !wglGetCurrentContext() )
			if( !wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC ) )
					return;
			CPoint pt/* = pAnnotation->ControlPoint()*/;
			CRect r = pAnnotation->boundingRectangle();
			pt.x = r.left + off.x;
			pt.y = r.bottom + off.y;
			//Get3DPointFromScreen( pt, *loc, 0.f );
			GetXYPointForDXFTextExport( pt, *loc );
			*str = pAnnotation->text();
			*rotation = pAnnotation->RotationAngle();
			pt.y = r.top;
			pt.x = 0;
			CPoint3D top;
			//Get3DPointFromScreen( pt, top, 0.f );
			GetXYPointForDXFTextExport( pt, top );
			pt.y = r.bottom;
			pt.x = 0;
			CPoint3D bottom;
			//Get3DPointFromScreen( pt, bottom, 0.f );
			GetXYPointForDXFTextExport( pt, bottom );
			*height = top.distance_to( bottom )/1.5;  // reduce to get "better height"
		}
	}
	return;
}

void CGraphicView::AddAnalysisMessage( LPCSTR text )
{
	if( strcmp( text, "SOLUTION:" ) == 0 || strcmp( text, "PROBLEM:" ) == 0 )
		return;
	CString str;
	m_analysisTextWindow.GetWindowText(str);
	if( str.GetLength() > 0 )
		str = str + "\r\n";
	str = str + text;
	m_analysisTextWindow.SetWindowText( str );
	int lc = m_analysisTextWindow.GetLineCount();
	m_analysisTextWindow.LineScroll( lc, 0 );
	return;
}

void CGraphicView::ClearAnalysisMessages()
{
	m_analysisTextWindow.SetWindowText("Analysis is proceeding as follows: \r\n");
	return;
}

void CGraphicView::UpdateAnalysisMessage( double fraction, LPCSTR text )
{
	CString str;
	m_analysisTextWindow.GetWindowText(str);
	int i;
	for( i = str.GetLength(); i > 1; i-- ) {
		char c = str[i-1];
		char c1 = str[i-2];
		c;
		c1;
		if( str[i-1] == '\n' && str[i-2] == '\r' ) {
			break;
		}
	}
	if( i == 1 )
		i = 0;
	CString newLine;
	newLine.Format( "%s. %.1lf%% Done.", text, fraction*100. );
	m_analysisTextWindow.SetSel( i, str.GetLength() );
	m_analysisTextWindow.ReplaceSel( newLine, FALSE );
	Invalidate();
	return;
}


void CGraphicView::ShowAnalysisTextWindow( int show ) 
{
	if( show == SW_SHOW )  {
		RECT r;
		GetClientRect( &r );
		m_analysisTextWindow.ShowWindow( show );
		m_analysisTextWindow.SetFocus();
		m_analysisTextWindow.SetWindowPos( NULL, 0, 0, r.right, r.bottom, SWP_NOZORDER|SWP_SHOWWINDOW );
		// blank the display
		if( m_pDC ) {
			CBrush br( GetSysColor(COLOR_BTNFACE) );
			m_pDC->FillRect( &r, &br );
			m_analysisTextWindow.InvalidateRect( &r );
		}
	}
	else
		m_analysisTextWindow.ShowWindow( show );
	return;
}