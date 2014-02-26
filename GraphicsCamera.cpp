#include "StdAfx.h"
#pragma hdrstop

#include "GraphicsCamera.h"
#include "Graphics/GraphicsHelpers.h"
#include "Graphics/GridManager.h"

#include "datautil.h"

#include "Model.h"

#include <gl/gl.h>
#include <gl/glu.h>

#ifdef _DEBUG
	#undef THIS_FILE
	static char THIS_FILE[]=__FILE__;
	#define new DEBUG_NEW
#endif


CGraphicsCamera::CGraphicsCamera(void) :
m_hDC( NULL ),
m_hGLRC( NULL ),
m_hWnd( NULL ),
m_Aspect( 1.0 ),
m_up( CVector3D( 0., 1., 0. ) ),
m_pos( CPoint3D( 0.0, 0.0, -10.0 ) ),
m_dir( CVector3D( 0.0, 0.0, -1.0 ) ),
m_rot( CPoint3D( 0.0, 0.0, 0.0 ) ),
m_ViewVolumeWidth( EMPTY_VIEW_VOLUME_WIDTH ),
m_ViewVolumeHeight( EMPTY_VIEW_VOLUME_HEIGHT ),
m_Near( Z_NEAR ),
m_Far( Z_FAR ),
m_focalPt( CPoint3D::ORIGIN ),
m_bBusy( false ),
m_gridPlane(),
m_bPerspective( true ),
m_cx( 100 ),
m_cy( 100 )
{
}

CGraphicsCamera::CGraphicsCamera( HWND hWnd, HDC hDC, HGLRC hGLRC ) :
m_hDC( hDC ),
m_hGLRC( hGLRC ),
m_hWnd( hWnd ),
m_Aspect( 1.0 ),
m_up( CVector3D( 0., 1., 0. ) ),
m_pos( CPoint3D( 0.0, 0.0, -10.0 ) ),
m_dir( CVector3D( 0.0, 0.0, -1.0 ) ),
m_rot( CPoint3D( 0.0, 0.0, 0.0 ) ),
m_ViewVolumeWidth( EMPTY_VIEW_VOLUME_WIDTH ),
m_ViewVolumeHeight( EMPTY_VIEW_VOLUME_HEIGHT ),
m_Near( Z_NEAR ),
m_Far( Z_FAR ),
m_focalPt( CPoint3D::ORIGIN ),
m_bBusy( false ),
m_gridPlane(),
m_bPerspective( true ),
m_cx( 100 ),
m_cy( 100 )
{
}
CGraphicsCamera::~CGraphicsCamera(void)
{
}

//file I/O helpers 
void CGraphicsCamera::SetCameraSettings( const CPoint3D position, const CPoint3D focalPoint, const CVector3D upVector )
{
	m_pos = position;
	m_focalPt = focalPoint;
	m_up = upVector;
	UpdateCamera();
}
void CGraphicsCamera::GetCameraSettings( CPoint3D& position, CPoint3D& focalPoint, CVector3D& upVector )
{
	position = m_pos;
	focalPoint = m_focalPt;
	upVector = m_up;
}

// Camera initialization functions

/*****************************************************************************************
* Function Name: InitCamera( HDC hDC, HGLRC hGLRC )
*
* Description: This function sets up the camera giving it a device context and a rendering
*              context. We check that the DC and GLRC contexts are valid by attempting to
*			   get the rendering thread associated with them. If we succeed then we
*			   assume that these contexts are valid for the life of the camera.
*
******************************************************************************************/
bool CGraphicsCamera::InitCamera( HWND hWnd, HDC hDC, HGLRC hGLRC )
{
	m_hWnd = hWnd;
	m_hDC = hDC;
	m_hGLRC = hGLRC;

	if( wglMakeCurrent( hDC, hGLRC ) )
	{
		wglMakeCurrent( NULL, NULL );
		return true;
	}
	else return false;
}

/*****************************************************************************************
* Function Name: SetWindowAspect( CRect rect )
*
* Description: Sets the aspect ratio of the openGL window. Should always be called from
*			   OnSize( ... )
*
******************************************************************************************/
void CGraphicsCamera::SetWindowAspect( int cx, int cy )
{
	if( cy <= 0 )
		m_Aspect = 1.0;
	else
		m_Aspect = double(cx)/double(cy);
}


/*****************************************************************************************
* Function Name: SetupViewVolume( double width, double height )
*
* Description: Sets up the view volume with proper aspect ratio. We always set the view
*              volume at the origin with a default size of the model extents then use
*              gluLookAt for zooming, translation and rotation. 
*
******************************************************************************************/
void CGraphicsCamera::SetupViewVolume( double width, double height )
{
	m_ViewVolumeWidth = width;
	m_ViewVolumeHeight = height;

	if( wglMakeCurrent( m_hDC, m_hGLRC ) )
	{
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity( );
		if( m_bPerspective )
			SetProjection();
		else{
			width = height*m_Aspect;
			glOrtho( -width, width, -height, height, Z_NEAR, Z_FAR );
		}
		glMatrixMode( GL_MODELVIEW );
		wglMakeCurrent( NULL, NULL );
	}

}

void CGraphicsCamera::OrthoZoom( const CPoint& /*pt*/, double scale )
{
	if( equal( scale, 1. ) )
		return;
	m_ViewVolumeWidth *= (1-scale);
	m_ViewVolumeHeight *= (1-scale);
	if( wglMakeCurrent( m_hDC, m_hGLRC ) )
	{
		double width = m_ViewVolumeWidth;
		double height = m_ViewVolumeHeight;
		TRACE("Ortho Zoom width = %f, height = %f\n", width, height );
	
		width = height*m_Aspect;
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity( );
		glOrtho( -width, 
			width, 
			-height, 
			height, 
			Z_NEAR, Z_FAR );
		glMatrixMode( GL_MODELVIEW );
		wglMakeCurrent( NULL, NULL );
	}
}
void CGraphicsCamera::SetupViewVolume( )
{
	if( wglMakeCurrent( m_hDC, m_hGLRC ) )
	{
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity( );
		SetProjection();
		glMatrixMode( GL_MODELVIEW );
		wglMakeCurrent( NULL, NULL );
	}

}

/*****************************************************************************************
* Function Name: SetProjection( )
*
* Description: Sets the projection angle for the camera. 45 to 60 is a reasonable value.
*              Z_NEAR and Z_FAR specify the near and far clipping plane distances. If these
*              are too big we start to get funny depth sorting behavior.
*
******************************************************************************************/
void CGraphicsCamera::SetProjection( )
{
	if( m_bPerspective )
		gluPerspective( 40.0, m_Aspect, Z_NEAR, Z_FAR );
	else{
		double width = m_ViewVolumeWidth;
		double height = m_ViewVolumeHeight;
		width = height*m_Aspect;
		glOrtho( -width, 
			width, 
			-height, 
			height, 
			Z_NEAR, Z_FAR );
	}
}

/*****************************************************************************************
* Function Name: Resize( )
*
* Description: This function sets the OpenGL viewport. Normally we set this to the size of the
*              window that we're rendering to. In the case of, for example, the rotation cube
*              we set the view port to something smaller and put it in the lower left hand corner.
*
******************************************************************************************/
void CGraphicsCamera::Resize( int cx, int cy )
{
	m_cx = cx;
	m_cy = cy;
	glViewport( 0, 0, cx, cy);
	SetWindowAspect( cx, cy );
	SetupViewVolume( m_ViewVolumeWidth, m_ViewVolumeHeight );
	glEnable( GL_NORMALIZE );
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

void CGraphicsCamera::ResetViewport()
{
	if( m_cx <= 0 ) 
		m_cx = 10;
	if( m_cy <= 0 )
		m_cy = 10;
	Resize( m_cx, m_cy );
}

/*****************************************************************************************
* Function Name: SetupDefaultCamera( )
*
* Description: This function sets up a default view for the current camera. It assumes we
*              we have a valid DC and GLRC which should be set by InitCamera(...). 
*              We also set some default view bounds based on the current model size.
*              Usually we will want to set up the camera with a known position and rotation,
*              however, this function works well for our debugging and initial design phase.
*
******************************************************************************************/
void CGraphicsCamera::SetupDefaultCamera( )
{
	m_pos.z = float(EMPTY_VIEW_VOLUME_ZPOS);
	//the model should always return a valid size
	m_ViewVolumeWidth = theModel.size( X )*1.2;
	m_ViewVolumeHeight = theModel.size( Y )*1.2;
	m_pos.z = float(1.2 * theModel.size( Z ));
	//just in case
	if( m_ViewVolumeWidth >= 1e20 || m_ViewVolumeHeight >= 1e20 )
	{
		//if we have a default grid (grid 1) size the view to it...
		if( theGridManager.gridCount() > 0 )
		{
			CGraphicGrid* pG = theGridManager.grid( 1 );
			if( pG )
			{
				m_ViewVolumeWidth = pG->extents().x*1.05;
				m_ViewVolumeHeight = pG->extents().y*1.05;
				m_pos.z = (float)(max( m_ViewVolumeWidth, m_ViewVolumeHeight ));
			}
		}
		else
		{
			m_ViewVolumeWidth = EMPTY_VIEW_VOLUME_WIDTH;
			m_ViewVolumeHeight = EMPTY_VIEW_VOLUME_HEIGHT;
		}
	}
	if( m_ViewVolumeWidth <= 0.0 )
		m_ViewVolumeWidth = EMPTY_VIEW_VOLUME_WIDTH;
	if( m_ViewVolumeHeight <= 0.0 )
		m_ViewVolumeHeight = EMPTY_VIEW_VOLUME_HEIGHT;

	m_pos.x = 0.0f;
	m_pos.y = 0.0f;
	if( fabs( m_pos.z ) < 1.0 )
		m_pos.z = sign(m_pos.z)*1.0f;
	if( zero( m_pos.z ) )
		m_pos.z = 1.0;
	SetupViewVolume( m_ViewVolumeWidth, m_ViewVolumeHeight );
}


bool CGraphicsCamera::MoveTo( CPoint3D p )
{
	m_pos = p;
	return true;
}

bool CGraphicsCamera::MoveTo( double _x, double _y, double _z )
{
	m_pos.x = _x;
	m_pos.y = _y;
	m_pos.z = _z;
	return true;
}

bool CGraphicsCamera::MoveBy( double dx, double dy, double dz )
{
	m_pos.x += dx;
	m_pos.y += dy;
	m_pos.z += dz;
	m_focalPt.x += dx;
	m_focalPt.y += dy;
	m_focalPt.z += dz;
	return true;
}

bool CGraphicsCamera::MoveBy( CPoint3D delta )
{
	//TRACE( "CCamera::MoveBy: (%f, %f, %f)\n", delta.x, delta.y, delta.z ); 
	m_pos += delta;
	m_focalPt += delta;
	return true;
}

bool CGraphicsCamera::MouseMove( const CPoint& delta )
{
	float dx = float( delta.x );
	float dy = float( delta.y );
	CVector3D v1( m_pos, m_focalPt );
	CVector3D right = v1.normalize().cross( m_up );
	CVector3D up = m_up;
	right *= dx;
	up *= dy;
	m_pos = m_pos + up.asPoint3D() + right.asPoint3D();
	m_focalPt = m_focalPt + up.asPoint3D() + right.asPoint3D();

	return true;
}
void CGraphicsCamera::Dolly( double inc )
{
	CVector3D dir = GetForwardVector();
	CVector3D temp = dir*inc;
	CPoint3D delta( temp.x, temp.y, temp.z );
	MoveBy( delta );
}

void CGraphicsCamera::Zoom( double inc, CPoint /*pt*/ )
{
	if( wglMakeCurrent( m_hDC, m_hGLRC ) )
	{
		CVector3D forward = CVector3D( m_focalPt, m_pos );
		forward *= inc;
		m_pos.x += forward.x;
		m_pos.y += forward.y;
		m_pos.z += forward.z;
		glMatrixMode( GL_MODELVIEW );
		glPushMatrix();
		UpdateCamera();
		glPopMatrix();
	}

}

void CGraphicsCamera::ZoomToRectangle( CRect r )
{
	int w1 = r.Width();
	int h1 = r.Height();
	int w2 = GetViewportRect().Width();
	int h2 = GetViewportRect().Height();

	double widthZoomInc = (double)w1/(double)w2;
	double heightZoomInc = (double)h1/(double)h2;
	double zoomInc = max( widthZoomInc, heightZoomInc );
	Zoom( 1-zoomInc, r.CenterPoint() );

}
void CGraphicsCamera::InstantPan( CPoint /*startPt*/, CPoint /*endPt*/ )
{
}

void CGraphicsCamera::RotateX( double angleInc, CWindowFilter& filter )
{
	//keep track of our rotation - might need this later?
	m_rot.x += angleInc;
	if( m_rot.x > M_PI*2 )
		m_rot.x = 0.0;
	else if( m_rot.x < -M_PI*2 )
		m_rot.x = 0.0;
	filter.SetRotations( m_rot );

	//JL Note 2/11/2009 - let's try rotating about the global x axis...
	CVector3D right( 1, 0, 0);
	//get the quaternion representation of this rotation
	//CVector3D right = m_up.cross( CVector3D( m_pos, m_focalPt ).normalize() );
	CQuaternion quat( angleInc, CVector3D( right ).normalize() );
	double m[4][4] = {0.};
	//convert to the 4x4 rotation matrix form
	quat.QuatToMat( m );
	CVector3D v1( m_pos, m_focalPt );
	//multiply our view vector by the rotation matrix
	v1 = v1*m;
	//update our position
	m_pos = CPoint3D( v1.x + m_focalPt.x, v1.y + m_focalPt.y, v1.z + m_focalPt.z );
	//update our camera's up vector
	m_up = m_up*m;
}

void CGraphicsCamera::RotateY( double angleInc, CWindowFilter& filter )
{
	m_rot.y += angleInc;
	if( m_rot.y > M_PI*2 )
		m_rot.y = 0.0;
	else if( m_rot.y < -M_PI*2 )
		m_rot.y = 0.0;
	filter.SetRotations( m_rot );

	//JL Note 2/11/2009 - let's try rotating about the global y axis...
	CQuaternion quat( angleInc, CVector3D(0,1,0) );
	//CQuaternion quat( angleInc, m_up.normalize() );
	double m[4][4] = {0.};
	quat.QuatToMat( m );
	CVector3D v1( m_pos, m_focalPt );
	//multiply our view vector by the rotation matrix
	v1 = v1*m;
	//update our position
	m_pos = CPoint3D( v1.x + m_focalPt.x, v1.y + m_focalPt.y, v1.z + m_focalPt.z );
	//update our camera's up vector
	m_up = m_up*m;
}

void CGraphicsCamera::RotateXY( double xInc, double yInc, CWindowFilter& filter )
{
	m_rot.x += xInc;
	if( m_rot.x > M_PI*2 )
		m_rot.x = 0.0;
	else if( m_rot.x < -M_PI*2 )
		m_rot.x = 0.0;
	m_rot.y += yInc;
	if( m_rot.y > M_PI*2 )
		m_rot.y = 0.0;
	else if( m_rot.y < -M_PI*2 )
		m_rot.y = 0.0;
	filter.SetRotations( m_rot );

	//get the quaternion representation of this rotation
	CVector3D right = m_up.cross( CVector3D( m_pos, m_focalPt ).normalize() );
	CQuaternion xquat( xInc, CVector3D( right ).normalize() );
	CQuaternion yquat( yInc, m_up.normalize() );
	CQuaternion total_quat = xquat + yquat;
	total_quat.normalize();

	double m[4][4] = {0.};
	total_quat.QuatToMat( m );
	CVector3D v1( m_pos, m_focalPt );
	//multiply our view vector by the rotation matrix
	v1 = v1*m;
	//update our position
	m_pos = CPoint3D( v1.x + m_focalPt.x, v1.y + m_focalPt.y, v1.z + m_focalPt.z );
	//update our camera's up vector
	m_up = m_up*m;

}
void CGraphicsCamera::RotateZ( double angleInc, CWindowFilter& filter )
{
	m_rot.z += angleInc;
	if( m_rot.z > M_PI*2 )
		m_rot.z = 0.0;
	else if( m_rot.z < -M_PI*2 )
		m_rot.z = 0.0;
	filter.SetRotations( m_rot );

	//JL note 2/11/2009 - let's try rotating around the global z axis...
	CQuaternion quat( angleInc, CVector3D( 0, 0, 1 ) );
	//CQuaternion quat( angleInc, CVector3D( m_pos, m_focalPt ).normalize() );
	double m[4][4] = {0.};
	quat.QuatToMat( m );
	CVector3D v1( m_pos, m_focalPt );
	//multiply our view vector by the rotation matrix
	v1 = v1*m;
	//update our position
	m_pos = CPoint3D( v1.x + m_focalPt.x, v1.y + m_focalPt.y, v1.z + m_focalPt.z );
	//update our camera's up vector
	m_up = m_up*m;
}

//void CGraphicsCamera::Rotate
void CGraphicsCamera::Rotate( const CPoint3D& euler_angles )
{
	CQuaternion q( euler_angles.x, euler_angles.y, euler_angles.z );
	double m[4][4] = {0. };
	q.QuatToMat( m );
	CVector3D v1( m_pos, m_focalPt );
	double dist = v1.length();
	v1 = CVector3D( 0., 0., 1. );
	v1*dist;
	
	m_pos = CPoint3D( v1.x + m_focalPt.x, v1.y + m_focalPt.y, v1.z + m_focalPt.z );
	m_up = CVector3D( 0., 1., 0. );
	Rotate( CVector3D::X_AXIS, euler_angles.x );
	Rotate( CVector3D::Y_AXIS, euler_angles.y );
	Rotate( CVector3D::Z_AXIS, euler_angles.z );
	//m_up = m_up*m;
}
//rotate the camera angleInc radians around a unit vector
void CGraphicsCamera::Rotate( CVector3D axis, double angleInc )
{
	CQuaternion quat( angleInc, axis.normalize() );
	double m[4][4] = {0.};
	quat.QuatToMat( m );
	CVector3D v1( m_pos, m_focalPt );
	//multiply our view vector by the rotation matrix
	v1 = v1*m;
	//update our position
	m_pos = CPoint3D( v1.x + m_focalPt.x, v1.y + m_focalPt.y, v1.z + m_focalPt.z );
	//update our camera's up vector
	m_up = m_up*m;
}

void CGraphicsCamera::UpdateCamera()
{
	//glMatrixMode( GL_MODELVIEW );
	//glLoadIdentity();
	//gluLookAt( m_pos.x, m_pos.y, m_pos.z,				// position 
	//	       m_focalPt.x, m_focalPt.y, m_focalPt.z,	// focal point
	//		   m_up.x, m_up.y, m_up.z );				// up vector
}

//Camera query functions
CVector3D CGraphicsCamera::GetForwardVector()
{
	CVector3D forward = CVector3D( m_focalPt, m_pos );
	return forward.normalize();
}

CVector3D CGraphicsCamera::GetRightVector()
{
	CVector3D forward = GetForwardVector();
	return forward.cross( m_up ).normalize();
}

CVector3D CGraphicsCamera::GetUpVector()
{
	return m_up.normalize();
}

double CGraphicsCamera::GetViewVolumeWidth()
{ 
	return m_ViewVolumeWidth; 
}

double CGraphicsCamera::GetViewVolumeHeight()
{ 
	return m_ViewVolumeHeight; 
}

CPoint3D CGraphicsCamera::GetPosition()
{ 
	return m_pos; 
}
	
CPoint3D CGraphicsCamera::GetFocalPoint()
{
	return m_focalPt;
}

double CGraphicsCamera::GetFocalLength()
{
	double focalLength = m_pos.distance_to( m_focalPt );
	ASSERT( focalLength > 0 );
	if( focalLength <= 0 )
		focalLength = EMPTY_VIEW_VOLUME_ZPOS;
	return focalLength;
}

//Gets a plane at the focal point perpendicular to the forward vector
CPlane3D CGraphicsCamera::GetFocalPlane()
{
	return CPlane3D( GetForwardVector(), m_focalPt );
}

CPoint CGraphicsCamera::GetScreenPtFrom3DPoint( const CPoint3D& modelSpacePt )
{

	GLint viewport[4];
	GLdouble mvMatrix[16], projMatrix[16];
	double winX = 0.0;
	double winY = 0.0;
	double winZ = 0.0; //depth value between 0 and 1 - for depth sorting

	CPoint winPt( 0, 0 );
	HGLRC currentRC = wglGetCurrentContext();
	if( currentRC || wglMakeCurrent( m_hDC, m_hGLRC ) )
	{
		glGetIntegerv( GL_VIEWPORT, viewport );
		glGetDoublev( GL_MODELVIEW_MATRIX, mvMatrix );
		glGetDoublev( GL_PROJECTION_MATRIX, projMatrix );
		if(	mvMatrix[0] + mvMatrix[1] + mvMatrix[2] + 
			mvMatrix[4] + mvMatrix[5] + mvMatrix[6] +
			mvMatrix[8] + mvMatrix[9] + mvMatrix[10] <= 1.0 )
		{
			ASSERT(FALSE);
			glMatrixMode( GL_MODELVIEW );
			glLoadIdentity();
			glGetDoublev(GL_MODELVIEW_MATRIX, mvMatrix);
			return winPt;
		}

		gluProject( modelSpacePt.x, modelSpacePt.y, modelSpacePt.z, 
							mvMatrix, projMatrix, viewport, 
							&winX, &winY, &winZ );
	}	

	//openGL convention (origin at lower left)
	winPt = CPoint( int(winX), int(winY) );

	//windows convention (origin at upper left)
	CRect wndRect;
	::GetClientRect( m_hWnd, wndRect );
	winPt.y = wndRect.Height() - winPt.y;
	//TRACE("screen Y = %i\n", winPt.y );
	
	return winPt;
}

CRect CGraphicsCamera::GetViewportRect( )
{
	GLint viewport[4];
	CRect r;
	HGLRC currentRC = wglGetCurrentContext();
	if( currentRC || wglMakeCurrent( m_hDC, m_hGLRC ) )
	{
		glGetIntegerv( GL_VIEWPORT, viewport );
		r = CRect( CPoint( viewport[0], viewport[1] ), CPoint( viewport[2], viewport[3] ) );
	}
	return r;
}

// The following function is called to indicate how much of the 
// real world to use and also the center of the view
void CGraphicsCamera::SetupWorldSizeAndCenter( double width, double height,
											  double /*depth*/, double xCenter, 
											  double yCenter, double zCenter )
{
	// code below assumes an XY View (I think) and needs to be modified in the future
	m_ViewVolumeWidth = width;
	m_ViewVolumeHeight = height;
	m_pos.z = zCenter;
	//make sure we have good view of the origin
	if( equal( m_pos.z, 0.0 ) )
		m_pos.z = EMPTY_VIEW_VOLUME_ZPOS;

	m_pos.x = xCenter;
	m_pos.y = yCenter;

	m_focalPt = CPoint3D( xCenter, yCenter, zCenter );

	SetupViewVolume( width*2, height*2 );
	m_pos.z *= m_pos.z;
	MoveTo( m_pos );
}

void CGraphicsCamera::FitTo( const CAABB& box )
{
	m_ViewVolumeWidth = box.Width();
	m_ViewVolumeHeight = box.Height();
	//JL Added 2.15.2008 - Fixes zoom all in ortho mode when we have a single member 
	if( m_ViewVolumeWidth <= 1 )
		m_ViewVolumeWidth = m_ViewVolumeHeight*m_Aspect;
	else if( m_ViewVolumeHeight <= 1 )
		m_ViewVolumeHeight = m_ViewVolumeWidth/m_Aspect;
	m_focalPt = box.Center();
	m_pos = box.Center();
	m_pos.z += box.Depth()*0.5;
	double tanTheta = tan( FOV/2 );
	ASSERT( tanTheta > 0. );
	double stand_off_w = box.Width()/2/tanTheta;
	double stand_off_h = box.Height()/2/tanTheta;
	if( stand_off_h > stand_off_w )
		m_pos.z += stand_off_h;
	else
		m_pos.z += stand_off_w;
	m_up = CVector3D( 0.f, 1.f, 0.f );
	SetupViewVolume();
}




