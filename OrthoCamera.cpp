#include "StdAfx.h"
#include "OrthoCamera.h"

#include "Graphics/GraphicsHelpers.h"
#include "Graphics/GridManager.h"

#include "datautil.h"

#include <gl/gl.h>
#include <gl/glu.h>


COrthoCamera::COrthoCamera(void):
m_viewVolumeWidth( 50 ),
m_viewVolumeHeight( 50 ),
m_viewVolumeDepth( 50 ),
m_viewVolumeX( 0 ),
m_viewVolumeY( 0 ),
m_viewVolumeZ( 0 ),
m_xloc( 0 ),
m_yloc( 0 ),
m_zloc( 0 ),
m_aspectRatio( 1 ),
m_viewportX( 10 ),
m_viewportY( 10 ),
zoomFactor( 1 ),
m_rotations( CPoint3D() ),
m_upAxis( Y ),
m_bMousePick( false ),
m_printScale( 1 )
{
	m_rotationMatrix.SetEulerAngles( 0, 0, 0 );
}

COrthoCamera::~COrthoCamera(void)
{
}

void COrthoCamera::SetVerticalAxis( ETCoordinate dir )
{
	m_upAxis = dir;
}

void COrthoCamera::SetWindowAspect( int cx, int cy )
{
	ASSERT( cy > 0 );
	if( cy <= 0 )
		m_aspectRatio = 1.0;
	else
		m_aspectRatio = double(cx)/double(cy);
}

void COrthoCamera::SetPrintScale( int scale )
{
	m_printScale = scale;
}

void COrthoCamera::GetPositionRotationAndScale( CPoint3D& position, CPoint3D& rotations, CPoint3D& viewVolume, double& zoomScale )
{
	position.x = m_viewVolumeX;
	position.y = m_viewVolumeY;
	position.z = m_viewVolumeZ;
	viewVolume.x = m_viewVolumeWidth;
	viewVolume.y = m_viewVolumeHeight;
	viewVolume.z = m_viewVolumeDepth;
	rotations = m_rotations;
	zoomScale = zoomFactor;
}

void COrthoCamera::SetPositionRotationAndScale( const CPoint3D& position, const CPoint3D& rotations, const CPoint3D& viewVolume, double zoomScale )
{
	m_xloc = m_viewVolumeX = position.x;
	m_yloc = m_viewVolumeY = position.y;
	//m_zloc = m_viewVolumeZ = position.z;
	m_viewVolumeWidth = viewVolume.x;
	m_viewVolumeHeight = viewVolume.y;
	m_viewVolumeDepth = viewVolume.z;
	m_rotations = rotations;
	zoomFactor = zoomScale;
	SetView();
}

void COrthoCamera::StartMousePicking( const CPoint& p, const CSize& selSize, int nBufSize, UINT selectBuf[]  )
{
	m_bMousePick = true;
	m_screenPt = p;
	m_selSize = selSize;

	glSelectBuffer( nBufSize, selectBuf );

	// Enter selection mode
	glRenderMode( GL_SELECT );
	
	glInitNames();
	glPushName(0);

	SetView();
}

void processHits(GLint hits, GLuint buffer[])
{
   unsigned int i, j;
   GLuint names, *ptr;

   TRACE("hits = %d\n", hits);
   ptr = (GLuint *) buffer;
   for (i = 0; i < hits; i++) {  /* for each hit  */
      names = *ptr;
      TRACE(" number of names for hit = %d\n", names); ptr++;
      TRACE("  z1 is %g;", (float) *ptr/0x7fffffff); ptr++;
      TRACE(" z2 is %g\n", (float) *ptr/0x7fffffff); ptr++;
      TRACE("   the name is ");
      for (j = 0; j < names; j++) {  /* for each name */
         TRACE("%d ", *ptr); ptr++;
      }
      TRACE("\n");
   }
}

int COrthoCamera::EndMousePicking( CHitRecordCollection& rRecordCollection, UINT selectBuf[] )
{
	m_bMousePick = false;
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );

	glFlush();
	//glFinish();

	int nHits = glRenderMode( GL_RENDER );
	ASSERT( nHits >= 0 );
	//if( nHits > 0 )
	//	processHits( nHits, selectBuf );
	LoadHitRecordCollection( selectBuf, nHits, rRecordCollection );

	return nHits;
}

void COrthoCamera::SetView( )
{  
	double h = m_viewVolumeHeight;
	double w = m_viewVolumeWidth;
	double d = m_viewVolumeDepth;

	if ( m_aspectRatio < 1.0 ) 
		h /= m_aspectRatio;
	else 
		w *= m_aspectRatio; 

	w /= 2;
	h /= 2;
	d *= 10;
	
	if( m_bMousePick )
	{
		//preserve the current projection by pushing it onto the stack
		glMatrixMode( GL_PROJECTION );
		glPushMatrix();
		glLoadIdentity();
		GLint viewport[4];
		CPoint point = m_screenPt;
		glGetIntegerv( GL_VIEWPORT, viewport );
		point.y = viewport[3] - point.y;
		gluPickMatrix( (double)point.x, (double)point.y, (double)m_selSize.cx, (double)m_selSize.cy, viewport );	
	}
	else
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
	}
	//glOrtho(0.0, 8.0, 0.0, 8.0, -0.5, 2.5);
	glOrtho( m_viewVolumeX - w*zoomFactor, 
		     m_viewVolumeX + w*zoomFactor, 
			 m_viewVolumeY - h*zoomFactor, 
			 m_viewVolumeY + h*zoomFactor, 
			 m_viewVolumeZ - d/*zoomFactor*/,
			 m_viewVolumeZ + d/*zoomFactor*/);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	SetOrientation( );
}

void COrthoCamera::SetOrientation( )
{
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glTranslated( m_xloc, m_yloc, m_zloc );
	CVector3D pitchVector( 1, 0, 0 );
	CVector3D yawVector( 0, 1, 0 );
	if( m_upAxis == Z )
	{
		glRotatef( -90, 1, 0, 0 );
		glRotatef( -90, 0, 0, 1 );
		pitchVector = CVector3D( 0, 1, 0 );
		yawVector = CVector3D( 0, 0, 1 );
	}
	else if( m_upAxis == X )
	{
		glRotatef( 90, 0, 0, 1 );
		glRotatef( 90, 1, 0, 0 );
		pitchVector = CVector3D( 0, 0, 1 );
		yawVector = CVector3D( 1, 0, 0 );
	}
	glRotatef(m_rotations.x*RAD2DEG, (float)pitchVector.x, (float)pitchVector.y, (float)pitchVector.z );
	glRotatef(m_rotations.y*RAD2DEG, (float)yawVector.x, (float)yawVector.y, (float)yawVector.z );
	glTranslated( -m_xloc, -m_yloc, -m_zloc );
}
 
void COrthoCamera::Resize( int cx, int cy )
{
	if( cx <= 0 )
		cx = 10;
	if( cy <= 0 )
		cx = 10;
	m_viewportX = cx;
	m_viewportY = cy;
	glViewport( 0, 0, cx, cy);
	SetWindowAspect( cx, cy );
	SetView( );
}

void COrthoCamera::ResetViewport( )
{
	Resize( m_viewportX, m_viewportY );
}
void COrthoCamera::SetupDefaultCamera( const CAABB& box )
{
	//the model should always return a valid size
	double emptyViewVolumeSize = 50;
	zoomFactor = 1.0;
	m_viewVolumeWidth = box.Width()*1.2;
	m_viewVolumeHeight = box.Height()*1.2;
	m_viewVolumeDepth = box.Depth()*1.2;
	//just in case
	if( m_viewVolumeWidth >= 1e20 || m_viewVolumeHeight >= 1e20 )
	{
		//if we have a default grid (grid 1) size the view to it...
		if( theGridManager.gridCount() > 0 )
		{
			CGraphicGrid* pG = theGridManager.grid( 1 );
			if( pG )
			{
				m_viewVolumeWidth = pG->extents().x*1.05;
				m_viewVolumeHeight = pG->extents().y*1.05;
				m_viewVolumeDepth = max( 2*m_viewVolumeWidth, 2*m_viewVolumeHeight );
			}
		}
		else
		{
			m_viewVolumeWidth = emptyViewVolumeSize;
			m_viewVolumeHeight = emptyViewVolumeSize;
			m_viewVolumeDepth = 2*emptyViewVolumeSize;
		}
	}
	if( m_viewVolumeWidth <= 0.0 )
		m_viewVolumeWidth = emptyViewVolumeSize;
	if( m_viewVolumeHeight <= 0.0 )
		m_viewVolumeHeight = emptyViewVolumeSize;
	if( m_viewVolumeDepth <= 0.0 )
		m_viewVolumeDepth = 2*emptyViewVolumeSize;
	double maxSize = max ( max( m_viewVolumeWidth, m_viewVolumeHeight ), m_viewVolumeDepth );
	maxSize *= 2;
	m_viewVolumeWidth = maxSize;
	m_viewVolumeHeight = maxSize;
	m_viewVolumeDepth = maxSize;

	SetView( );
}

void COrthoCamera::ZoomAll( const CAABB& box )
{
	//rotate the box points and get the x and y extents...
	CPoint3D extents = box.Extents();
	CPoint3D pts[8];
	pts[0] = CPoint3D( -extents.x, -extents.y, extents.z );
	pts[1] = CPoint3D( extents.x, -extents.y, extents.z );
	pts[2] = CPoint3D( extents.x, extents.y, extents.z );
	pts[3] = CPoint3D( -extents.x, extents.y, extents.z );
	pts[4] = CPoint3D( -extents.x, -extents.y, -extents.z );
	pts[5] = CPoint3D( extents.x, -extents.y, -extents.z );
	pts[6] = CPoint3D( extents.x, extents.y, -extents.z );
	pts[7] = CPoint3D( -extents.x, extents.y, -extents.z );
	CMatrix44 mat;
	mat.SetEulerAngles( m_rotations.x, m_rotations.y, m_rotations.z );
	CPoint3D result;
	double minX=1e9; double minY=1e9; double minZ=1e9;
	double maxX=-1e9; double maxY=-1e9; double maxZ=-1e9;
	for( int i = 0; i < 8; i++ )
	{
		mat.RightMultiply( pts[i], result );pts[i] = result;
		minX = min( pts[i].x, minX );
		minY = min( pts[i].y, minY );
		minZ = min( pts[i].z, minZ );
		maxX = max( pts[i].x, maxX );
		maxY = max( pts[i].y, maxY );
		maxZ = max( pts[i].z, maxZ );
	}
	CAABB newBox( minX/2, maxX/2, minY/2, maxY/2, minZ/2, maxZ/2 );
	double w = newBox.Width();
	double h = newBox.Height();
	if( m_upAxis == Z )
	{
		h = newBox.Depth();
		w = newBox.Height();
	}
	else if( m_upAxis == X )
	{
		h = newBox.Width();
		w = newBox.Depth();
	}
	//give the model a little wiggle room by making the extents 10% larger...
	w *= 1.15;
	h *= 1.15;
	if( w > h )
	{
		if( w > 0 && !zero(m_viewVolumeWidth) )
			zoomFactor = w/m_viewVolumeWidth;
		else zoomFactor = 1;
	}
	else
	{
		if( h > 0 && !zero(m_viewVolumeHeight) )
			zoomFactor = h/m_viewVolumeHeight;
		else zoomFactor = 1;
	}
	//always center on the original box
	m_xloc = m_viewVolumeX = box.Center().x;
	m_yloc = m_viewVolumeY = box.Center().y;
	m_zloc = m_viewVolumeZ = box.Center().z;
}
void COrthoCamera::FitTo( const CAABB& box )
{
	//set the zoom factor to result in scaled width and height that matches the 
	//width and height of the box
	//use the larger of the two to ensure we fit the entire model into the view
	SetupDefaultCamera( box );
	double w = box.Width();
	double h = box.Height();
	if( m_upAxis == Z )
	{
		h = box.Depth();
		w = box.Height();
	}
	else if( m_upAxis == X )
	{
		h = box.Width();
		w = box.Depth();
	}
	if( w > h )
	{
		if( w > 0 && !zero(m_viewVolumeWidth) )
			zoomFactor = w/m_viewVolumeWidth;
		else zoomFactor = 1;
	}
	else
	{
		if( h > 0 && !zero(m_viewVolumeHeight) )
			zoomFactor = h/m_viewVolumeHeight;
		else zoomFactor = 1;
	}

	m_xloc = m_viewVolumeX = box.Center().x;
	m_yloc = m_viewVolumeY = box.Center().y;
	m_zloc = m_viewVolumeZ = box.Center().z;	
}

void COrthoCamera::GetViewVolumePointFromScreen( const CPoint& pt, double& x, double& y )
{
	int screenX = pt.x;
	int screenY = m_viewportY - pt.y;
	int screenDX = screenX - (int)((double)m_viewportX/2);
	int screenDY = screenY - (int)((double)m_viewportY/2);

	double h = m_viewVolumeHeight;
	double w = m_viewVolumeWidth;

	if ( m_aspectRatio < 1.0 ) 
		h /= m_aspectRatio;
	else 
		w *= m_aspectRatio; 

	x = m_viewVolumeX + zoomFactor * w * screenDX / m_viewportX;
	y = m_viewVolumeY + zoomFactor * h * screenDY / m_viewportY;
}

double COrthoCamera::ScalePixelSize( int pixelSize )
{
	if( m_viewportX <= 0 ) 
		return m_viewVolumeWidth*zoomFactor/50 * pixelSize * m_printScale;
	else
		return m_viewVolumeWidth*zoomFactor/m_viewportX * pixelSize * m_printScale;
}

void COrthoCamera::Zoom( double inc )
{
	zoomFactor *= inc;
	SetView( );
}

void COrthoCamera::Zoom( double inc, const CPoint& pt )
{
	double x1 = 0;
	double y1 = 0;
	GetViewVolumePointFromScreen( pt, x1, y1 );
	zoomFactor *= inc;
	SetView( );
	double x2 = 0;
	double y2 = 0;
	GetViewVolumePointFromScreen( pt, x2, y2 );
	m_viewVolumeX -= (x2 - x1 );
	m_viewVolumeY -= (y2 - y1 );
	SetView( );
}

void COrthoCamera::ZoomArea( const CRect& r )
{
	double x = 0;
	double y = 0;
	CPoint screenCenterPt = r.CenterPoint();
	GetViewVolumePointFromScreen( screenCenterPt, x, y );
	m_viewVolumeX = x;
	m_viewVolumeY = y;
	double widthScale = abs((double)r.Width()/(double)m_viewportX);
	double heightScale = abs((double)r.Height()/(double)m_viewportY);
	//use larger of the two
	if( widthScale > heightScale )
		zoomFactor *= widthScale;
	else
		zoomFactor *= heightScale;
	SetView( );
}

double COrthoCamera::ZoomFactor()
{
	return zoomFactor;
}
void COrthoCamera::MoveBy( const CPoint& delta )
{
	double h = m_viewVolumeHeight*zoomFactor;
	double w = m_viewVolumeWidth*zoomFactor;

	if ( m_aspectRatio < 1.0 ) 
		h /= m_aspectRatio;
	else 
		w *= m_aspectRatio; 

	ASSERT_RETURN( m_viewportY > 0 );
	double xScale = w/(double)m_viewportX;
	double yScale = h/(double)m_viewportY;
	m_viewVolumeX -= xScale*(double)delta.x;
	m_viewVolumeY += yScale*(double)delta.y;
	SetView( );
}

void COrthoCamera::RotateX( double inc )
{
	m_rotations.x += inc;
}

void COrthoCamera::RotateY( double inc )
{
	m_rotations.y += inc;
}

void COrthoCamera::RotateZ( double inc )
{
	m_rotations.z += inc;
}

void COrthoCamera::RotateXY( double incX, double incY )
{
	//always rotate by x and y, we map these x and y rotations to the 
	//"up" axis orientation in the SetOrientation() function
	RotateX( incY/75 );
	RotateY( incX/50 );
}

void COrthoCamera::Rotate( const CPoint3D& eulers )
{
	m_rotations = eulers;
}

const CPoint3D& COrthoCamera::GetRotations()
{
	return m_rotations;
}

void COrthoCamera::SetRotations( const CPoint3D& rotations )
{
	m_rotations = rotations;
}

bool COrthoCamera::PointInView( const CPoint3D& pt )
{
	int viewport[4];
	double modelview[16];
	double projection[16];
	
	glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );
	double winX = 0.;
	double winY = 0.;
	double winZ = 0.;
	if( !gluProject( pt.x, pt.y, pt.z, 
		    modelview, projection, viewport, 
			&winX, &winY, &winZ ) || winZ > 1.0 )
			return false;
	return true;
}


short cameraVersion = 1;
//version 1: Added 9/10/2009 - this is a new camera model for VA 7.0

bool COrthoCamera::WriteSettings( obfstream& out )
{
	out << cameraVersion; // TeK Add 9/11/2009, must get written to be useful

	out << m_viewVolumeWidth;
	out << m_viewVolumeHeight;
	out << m_viewVolumeDepth;

	out << m_viewVolumeX;
	out << m_viewVolumeY;
	out << m_viewVolumeZ;

	out << m_xloc;
	out << m_yloc;
	out << m_zloc;

	out << m_rotations.x;
	out << m_rotations.y;
	out << m_rotations.z;

	out << (long)m_viewportX;
	out << (long)m_viewportY;

	out << zoomFactor;

	out << m_aspectRatio;

	return true;
}

bool COrthoCamera::ReadSettings( ibfstream& in )
{
	short v;
	in >> v; // TeK Add 9/11/2009, now we have the ability to deal with future format changes
	CHECK_IF( (v >= 1) && (v <= cameraVersion) ) {
		in >> m_viewVolumeWidth;
		in >> m_viewVolumeHeight;
		in >> m_viewVolumeDepth;

		in >> m_viewVolumeX;
		in >> m_viewVolumeY;
		in >> m_viewVolumeZ;

		in >> m_xloc;
		in >> m_yloc;
		in >> m_zloc;

		in >> m_rotations.x;
		in >> m_rotations.y;
		in >> m_rotations.z;

		//JL Note 9/22/2009 - the viewport needs to be sized dynamically when the project
		//is loaded and OnSize is called. we really shouldn't even be writing these values out.
		long viewportX = 0;
		long viewportY = 0;
		in >> viewportX;//m_viewportX;
		in >> viewportY;//m_viewportY;
		//m_viewportX = (int)viewportX;
		//m_viewportY = (int)viewportY;

		in >> zoomFactor;
		
		//JL Note 9/22/2009 - the aspect ratio needs to be set dynamically when the project
		//is loaded and OnSize is called. we really shouldn't even be writing this value out.
		double aspectRatio = 0;
		in >> aspectRatio;//m_aspectRatio;
		return true;
	}

	return false;
}