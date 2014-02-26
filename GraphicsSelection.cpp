#include "stdafx.h"
#pragma hdrstop

#include "GraphicView.h"

#include "structur.h"
#include "model.h"
#include "project.h"

#include "graphics/GridManager.h"

#include "User/message.h"
#include "main.h"  // theApp
#include "MainFrm.h"  // CMainFrame

#include "Units\Units.h"
 
#include "Dialog/DlgUtil.h"  // IsContextHelpUp()
#include "Dialog/Dialogs.h"

#include "User/UserUtil.h"


#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

bool CGraphicView::Get3DPointFromScreen( const CPoint& screenPt, CPoint3D& worldPt, float depth )
{
	ASSERT_RETURN_ITEM( m_pDC != NULL, false );
	// this function works for any screen point. If no depth value is specified then we return
	// a world pt corresponding to the depth value of the pixel under the current mouse position
	// otherwise a depth value between 0.0 and 1.0 can be specified where 0.0 corresponds to the near 
	// clip plane and 1.0 corresponds to the far clip plane.
	//if( !wglGetCurrentContext() )
	if( !wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC ) )
			return false;
	int viewport[4];
	double modelview[16];
	double projection[16];
	float winX, winY, winZ;
	double posX, posY, posZ;

	glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );

	winX = (float)screenPt.x;
	winY = (float)viewport[3] - (float)screenPt.y;
	if( depth < 0. )
		glReadPixels( screenPt.x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
	else
	{
		ASSERT( depth >= 0. && depth <= 1.0 );
		winZ = depth;
	}

	if( GL_FALSE == gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ) )
		return false;

	worldPt.x = posX;
	worldPt.y = posY;
	worldPt.z = posZ;

	return true;
}

bool CGraphicView::GetXYPointForDXFTextExport( const CPoint& screenPt, CPoint3D& worldPt )
{
	//get a ray that goes through screenPt from the front of the view frustum to the back of it
	CPoint3D nearPt, farPt;
	if( !Get3DPointFromScreen( screenPt, nearPt, 0.f ) )
		return false;
	if( !Get3DPointFromScreen( screenPt, farPt, 1.f ) )
		return false;
	CLine3D ray( nearPt, farPt );

	//now find out where in 3D space the ray passes through the z=0 plane
	CVector3D xyNormal( 0., 0., 1. );
	CPoint3D ptOnXYPlane( 0., 0., 0. );
	CPlane3D xyPlane( xyNormal, ptOnXYPlane );
	double denom = xyNormal.dot( CVector3D( farPt, nearPt ) );
	if( zero( denom ) ) return false;
	double num = xyNormal.dot( CVector3D( ptOnXYPlane, nearPt  ) );
	double s = num/denom;
	worldPt = ray.offset( s );
	
	if( true/*is world point valid*/ )
		return true;
	else return false;
}

bool CGraphicView::GetSnapped3DPoint( const CPoint& screenPt, CPoint3D& worldPt )
{
	// JL Note 2/15/2008 - not sure why we were return false when dragging a zoombox...
	// zoombox will not draw if the code below is uncommented
	if( /*Mouse( MOUSE_ZOOMBOX_DRAGGING ) ||*/ Mouse( MOUSE_MODEL_DRAGGING ) )  
		return FALSE;

	//this function returns true and sets worldPt to the 3D coordinates of the closest grid point, node
	//point or member point if a grid, node point or member is found in the selection area.
	//If nothing is found then it does nothing to worldPt and returns false
	
	//	CTimeThisFunction timer( "GetSnapped3DPoint" );
	
	CPoint point = screenPt;
	bool bSuccess = false;
	
	if( !m_hRC || !m_pDC )
	{
		TRACE( "Failed in CGraphicView::GetSnapped3DPoint\n" );
		return bSuccess;
	}

	if( !wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC ) )
	{
		TRACE( "Failed in CGraphicView::GetSnapped3DPoint\n" );
		return bSuccess;
	}

	// Create buffer to hold selection hits 
	//(must be done before entering selection mode)
	// TeK Change 1/19/2009: We shouldn't be dynamically allocating memory on EVERY mouse-move!
	// Changed to use a static buffer (twice as big!)
	const int nBufSize = 4096*2;
	static UINT selectBuf[nBufSize];
	memset( selectBuf,0, sizeof(selectBuf));

	double pickSize = double(ini.size().node*5);
	m_Camera.StartMousePicking( point, CSize(pickSize, pickSize), nBufSize, selectBuf );

	m_graphicsModel.SetWindowFilter( m_Filter );
	m_graphicsModel.DrawMembersForSnapping( m_pDC );

	CHitRecordCollection rRecCol;
	m_Camera.EndMousePicking( rRecCol, selectBuf );

	//first check for node and grid hits
	if( m_graphicsModel.OverNodeOrGridPoint( screenPt, worldPt, m_Filter ) )
	{
		bSuccess = true;
	}

	CPoint screenSnapPt;
	int screenDistanceToSnapPoint = 0;
	if( GetScreenPtFrom3DPoint( worldPt, screenSnapPt ) )
	{
		screenDistanceToSnapPoint = max( abs( screenPt.x - screenSnapPt.x ), abs( screenPt.y - screenSnapPt.y ) );
	}

	//if we have snapped to a node or grid point and the distance between
	//this point and the node or grid point is less than x pixels then we
	//stay snapped to the node or grid point
	//SHORT altState = GetAsyncKeyState( VK_MENU );
	if( /*!(altState & 0x8000) && nHits > 0 &&*/ screenDistanceToSnapPoint > 8 )
	{
		if( const CMember* pM = m_graphicsModel.OverMember( rRecCol ) )
		{
			CPoint3D nearPt, farPt;
			if( Get3DPointFromScreen( screenPt, nearPt, 0.f ) &&
				Get3DPointFromScreen( screenPt, farPt, 1.f ) )
			{
				CLine3D ray( nearPt, farPt );
				CPoint3D p1( pM->node(1)->x(), pM->node(1)->y(), pM->node(1)->z() );
				CPoint3D p2( pM->node(2)->x(), pM->node(2)->y(), pM->node(2)->z() );
				CLine3D memLine( p1, p2 );
				CPoint3D pt3D = memLine.nearest_point( ray );
				if( pt3D != CPoint3D::undefined_point && memLine.is_on_segment( pt3D ) )
				{	
					worldPt = pt3D;
					bSuccess = true;
				}
				else
					bSuccess = false;
			}
		}
	}
	//delete[] selectBuf;

	return bSuccess;
}

bool CGraphicView::GetScreenPtFrom3DPoint( const CPoint3D& modelSpacePt, CPoint& screenPt )
{
	// this function works for any xyz point but usually we will just be returning the 
	// screen point for a grid point or node.
	GLint viewport[4];
	GLdouble mvMatrix[16], projMatrix[16];
	double winX = 0.0;
	double winY = 0.0;
	double winZ = 0.0; //depth value between 0 and 1 - for depth sorting
	bool bSuccess = false;

	HGLRC currentRC = wglGetCurrentContext();
	if( !currentRC )
	{
		TRACE( "CGraphicView::GetScreenPtFrom3DPoint Failed in GetCurrentContext\n" );
		return bSuccess;
	}
	HDC hDC = wglGetCurrentDC( );
	if( !hDC )
	{
		TRACE( "CGraphicView::GetScreenPtFrom3DPoint Failed in GetCurrentDC\n" );
		return bSuccess;
	}

	//if( !wglMakeCurrent( m_pDC->GetSafeHdc(), m_hDC ) )
	if( !wglMakeCurrent( hDC, currentRC ) )
	{
		TRACE( "CGraphicView::GetScreenPtFrom3DPoint Failed in MakeCurrent\n" );
		return bSuccess;
	}

	glGetIntegerv( GL_VIEWPORT, viewport );
	glGetDoublev( GL_MODELVIEW_MATRIX, mvMatrix );
	glGetDoublev( GL_PROJECTION_MATRIX, projMatrix );

	if( !gluProject( modelSpacePt.x, modelSpacePt.y, modelSpacePt.z, 
		             mvMatrix, projMatrix, viewport, 
				     &winX, &winY, &winZ ) )
	{
		//TRACE( "Failed in GetScreenPt #2\n" );
		//wglMakeCurrent( NULL, NULL );
		return bSuccess;
	}

	//openGL convention (origin at lower left)
	screenPt.x = int(winX);
	screenPt.y = int(winY);


	//windows convention (origin at upper left)
	screenPt.y = viewport[3] - viewport[1] - screenPt.y;
	bSuccess = true;
	return bSuccess;
}
      
bool CGraphicView::GetSnappedScreenPt( CPoint& point )
{
	//this function gets the snapped screen point based on the current 3D point which
	// will be snapped to the nearest grid point or node.
	CPoint3D model3D;
	if( !GetSnapped3DPoint( point, model3D ) )
		return false;
	
	GetScreenPtFrom3DPoint( model3D, point );
	return true;
}

int CGraphicView::GetSelectionHitList( CPoint p, int selSizeX, int selSizeY, CHitRecordCollection& rRecordCollection )
{
	if( !wglMakeCurrent(m_pDC->GetSafeHdc(), m_hRC) )
		return 0;

	// Create buffer to hold selection hits 
	//(must be done before entering selection mode)
	// TeK Change 1/19/2009: We shouldn't be dynamically allocating memory on EVERY mouse-move!
	// Changed to use a static buffer (twice as big!)
	const int nBufSize = 4096*2;
	static UINT selectBuf[nBufSize];
	memset( selectBuf,0, sizeof(selectBuf));
	
	m_Camera.StartMousePicking( p, CSize(selSizeX, selSizeY), nBufSize, selectBuf );

	m_graphicsModel.DrawModelForSelection( GetDC(), Mouse( MOUSE_SELECTIONBOX_DRAGGING ) );
	/*glLoadName(1);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 0.0);
	glVertex3i(2, 0, 0);
	glVertex3i(2, 6, 0);
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
	glEnd();*/

	int nHits = m_Camera.EndMousePicking( rRecordCollection, selectBuf );

	wglMakeCurrent( NULL, NULL );

	return nHits;

}
