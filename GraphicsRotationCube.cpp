#include "StdAfx.h"
#include "GraphicsRotationCube.h"
#include "IniColor.h"

double rad180 = 180*M_PI/180;
double rad90 = 90*M_PI/180;
double rad45 = 45*M_PI/180;
double rad135 = 135*M_PI/180;
double rad225 = 225*M_PI/180;
double rad270 = 270*M_PI/180;
double rad315 = 315*M_PI/180;

double cubeOrientation[LAST_ORIENTATION][3] = {
	{ 0,		0,		0 },		// Front
	{ 0,		rad180,	0 },		// Back
	{ rad90,	0,		0 },		// Top
	{ rad270,	0,		0 },		// Bottom
	{ 0,		rad90,	0 },		// Left
	{ 0,		rad270,	0 },		// Right
	{ rad45,	rad45,	0 },		// Top Front Left
	{ rad45,	rad315,	0 },		// Top Front Right
	{ rad315,	rad315,	0 },		// Bottom Front Left
	{ rad315,	rad45,	0 },		// Bottom Front Right
	{ rad45,	rad135,	0 },		// Top Back Left
	{ rad45,	rad225,	0 },		// Top Back Right
	{ rad315,	rad135,	0 },		// Bottom Back Left
	{ rad315,	rad225,	0 },		// Bottom Back Right
	{ rad45,	0,		0 },		// Top Front
	{ rad315,	0,		0 },		// Bottom Front
	{ 0,		rad45,	0 },		// Left Front
	{ 0,		rad315,	0 },		// Right Front
	{ rad45,	rad180,	0 },		// Top Back
	{ rad315,	rad180, 0 },		// Bottom Back
	{ 0,		rad135, 0 },		// Left Back
	{ 0,		rad225, 0 },		// Right Back
	{ rad45,	rad90,	0 },		// Top Left
	{ rad45,	rad270,	0 },		// Top Right
	{ rad315,	rad90,	0 },		// Bottom Left
	{ rad315,	rad270,	0 }			// Bottom Right
};

double CGraphicsRotationCube::CubeOrientationAngle( int cube_orientation, int xyz ) {
	ASSERT( cube_orientation >= 0 && cube_orientation < LAST_ORIENTATION );
	ASSERT( xyz >= 0 && xyz < 3 );
	return cubeOrientation[cube_orientation][xyz];
}

CGraphicsRotationCube::CGraphicsRotationCube(void) :
m_text( NULL )
{
	if( ini.graphics().useTextureFonts )
		m_text = &m_textureText;
	else
		m_text = &m_outlineText;

	m_viewportSize = 100;
	m_cubeSize = 100;
	m_hoverItem = NO_ORIENTATION_HIT;
	m_bAnimating = false;
}

CGraphicsRotationCube::~CGraphicsRotationCube(void)
{
}

void CGraphicsRotationCube::SetupCamera( const CPoint3D& rotations )
{
	m_camera.SetupDefaultCamera( CAABB( (double)m_cubeSize, (double)m_cubeSize, (double)m_cubeSize ) );
	m_camera.Resize( m_viewportSize, m_viewportSize );
	m_camera.FitTo( CAABB( CPoint3D(0,0,0), CPoint3D( (double)m_cubeSize, (double)m_cubeSize, (double)m_cubeSize ) ) );
	m_camera.SetRotations( rotations );
}

bool CGraphicsRotationCube::Animating()
{
	static int count = 0;
	if( m_bAnimating && count++ > 1000 )
	{
		count = 0;
		m_bAnimating = false;
	}
	if( !m_bAnimating )
		count = 0;
	return m_bAnimating;
}

bool CGraphicsRotationCube::PointIsInViewport( const CPoint& pt, const CRect& r )
{
	return ( pt.x <= m_viewportSize && pt.y >= r.Height()-m_viewportSize );
}

double constrain_angle( double angle )
{
	if( angle < -M_PI )
		angle = 2*M_PI + angle;
	else if( angle > M_PI )
		angle = -2*M_PI + angle;
	return angle;
}

bool CGraphicsRotationCube::GetAnimatingRotation( CPoint3D& rotations )
{
	//we rotate through nFrames 
	//JL Note - should switch this to time-dependent
	//int nFrames = 20;
	//animate over 2 seconds
	long animationLength = 1000;
	double delta = (double)(GetTickCount()-m_startTime)/(double)animationLength;
	if( delta > 1 )
	{
		delta = 1;
		//return true;
	}

	//double delta = (double)m_nFrame/(double)nFrames;
	double deltax = m_endRotation.x - m_startRotation.x;
	double deltay = m_endRotation.y - m_startRotation.y;
	deltax = constrain_angle( deltax );
	deltay = constrain_angle( deltay );
	rotations.x = m_startRotation.x + deltax*delta;
	rotations.y = m_startRotation.y + deltay*delta;

	if( delta >= 1 )
	{
		m_nFrame = 0;
		m_bAnimating = false;
	}

	return true;
}

bool CGraphicsRotationCube::OnMouseClick( CPoint3D& rotations, const CPoint& p, const CRect& r, int selSize )
{
	const int nBufSize = 4096*2;
	static UINT selectBuf[nBufSize];
	memset( selectBuf,0, sizeof(selectBuf));
	CHitRecordCollection rHitCollection;

	COrthoCamera littleCam;
	littleCam.SetupDefaultCamera( CAABB( m_cubeSize, m_cubeSize, m_cubeSize ) );
	littleCam.Resize( m_viewportSize, m_viewportSize );
	littleCam.FitTo( CAABB( CPoint3D(0,0,0), CPoint3D(m_cubeSize*2, m_cubeSize*2, m_cubeSize*2) ) );
	littleCam.SetRotations( rotations );
	CPoint littleCamPt = p;
	littleCamPt.y = p.y - (r.Height() - m_viewportSize);
	littleCam.StartMousePicking( littleCamPt, CSize(selSize, selSize), nBufSize, selectBuf );
	
	Draw( );

	int nHits = littleCam.EndMousePicking( rHitCollection, selectBuf );

	if( nHits <= 0 )
	{
		return false;
	}

	CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		if( i == 0 ) 
			pClosestHR = rHitCollection.m_arrayHits[i];
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( pClosestHR->m_nMinZDepth > pHR->m_nMinZDepth )
			pClosestHR = pHR;
	}
	if( pClosestHR && GetModelItemType( pClosestHR->m_arrayNames[0] ) == GRAPHIC_ROTATION_CUBE )
	{
		m_startRotation = rotations;
		m_startRotation.x = constrain_angle( m_startRotation.x );
		m_startRotation.y = constrain_angle( m_startRotation.y );
		int packed_face_index = GetModelItemIndex( pClosestHR->m_arrayNames[0] );
		int graphicType = (int)packed_face_index;
		rotations[0] = CubeOrientationAngle( graphicType, 0 ); //-stdOrientationAngles(graphicType, 0);
		rotations[1] = CubeOrientationAngle( graphicType, 1 ); //-stdOrientationAngles(graphicType, 1);
		rotations[2] = CubeOrientationAngle( graphicType, 2 ); //-stdOrientationAngles(graphicType, 2);
		m_endRotation = rotations;
		m_endRotation.x = constrain_angle( m_endRotation.x );
		m_endRotation.y = constrain_angle( m_endRotation.y );
		if( m_startRotation == m_endRotation )
		{
			return true;
		}
		else
		{
			m_startTime = GetTickCount();
			//m_nFrame = 0;
			m_bAnimating = true;
		}
		return true;
	}
	return false;
}

bool CGraphicsRotationCube::OnMouseMove( const CPoint3D& rotations, const CPoint& p, const CRect& r, int selSize )
{
	m_hoverItem = NO_ORIENTATION_HIT;

	const int nBufSize = 4096*2;
	static UINT selectBuf[nBufSize];
	memset( selectBuf,0, sizeof(selectBuf));
	CHitRecordCollection rHitCollection;

	COrthoCamera littleCam;
	littleCam.SetupDefaultCamera( CAABB( m_cubeSize, m_cubeSize, m_cubeSize ) );
	littleCam.Resize( m_viewportSize, m_viewportSize );
	littleCam.FitTo( CAABB( CPoint3D(0,0,0), CPoint3D(m_cubeSize*2, m_cubeSize*2, m_cubeSize*2) ) );
	littleCam.SetRotations( rotations );
	littleCam.SetView();
	CPoint littleCamPt = p;
	littleCamPt.y = p.y - (r.Height() - m_viewportSize);
	littleCam.StartMousePicking( littleCamPt, CSize(selSize, selSize), nBufSize, selectBuf );
	
	Draw( );
	
	int nHits = littleCam.EndMousePicking( rHitCollection, selectBuf );

	if( nHits <= 0 )
	{
		return false;
	}

	CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		if( i == 0 ) 
			pClosestHR = rHitCollection.m_arrayHits[i];
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( pClosestHR->m_nMinZDepth > pHR->m_nMinZDepth )
			pClosestHR = pHR;
	}
	if( pClosestHR && GetModelItemType( pClosestHR->m_arrayNames[0] ) == GRAPHIC_ROTATION_CUBE )
	{
		int packed_face_index = GetModelItemIndex( pClosestHR->m_arrayNames[0] );
		m_hoverItem = (ETCubeOrientation)packed_face_index;
		return true;
	}
	return false;
}

void CGraphicsRotationCube::Draw( )
{
	glDepthRange( 0,0.5 );
	DisableGLLighting();
	DrawCorners( );
	DrawEdges( );
	DrawFaces( );
	glDepthRange( 0, 1 );
}

void CGraphicsRotationCube::Draw( const CPoint3D& rotations, CDC* pDC )
{
	//make a new camera and set the viewport to a little box in the lower left hand corner...
	COrthoCamera littleCam;
	littleCam.SetupDefaultCamera( CAABB( m_cubeSize, m_cubeSize, m_cubeSize ) );
	littleCam.Resize( m_viewportSize, m_viewportSize );
	littleCam.FitTo( CAABB( CPoint3D(0,0,0), CPoint3D(m_cubeSize*2, m_cubeSize*2, m_cubeSize*2) ) );
	littleCam.SetRotations( rotations );
	littleCam.SetView();

	Draw( );
	DrawLabels( pDC );
}

void CGraphicsRotationCube::DrawCorners( )
{
	float w = m_cubeSize/2;
	if( zero( w ) ) w = 0.1f;

	float pointColor[] = {0.3f, 0.3f, 0.3f};
	float selectColor[] = { 0.2f, 0.2f, 1.0f };

	int unselectedSize = 6;
	int selectedSize = 8;

	CPoint3D p0(-w, -w, -w );
	CPoint3D p1( w, -w, -w );
	CPoint3D p2( w,  w, -w );
	CPoint3D p3(-w,  w, -w );
	CPoint3D p4(-w, -w,  w );
	CPoint3D p5( w, -w,  w );
	CPoint3D p6( w,  w,  w );
	CPoint3D p7(-w,  w,  w );

	glPushAttrib( GL_POINT_BIT | GL_CURRENT_BIT );
	glColor3fv( pointColor );

	//bottom back left
	glPointSize( unselectedSize );
	glColor3fv( pointColor );
	if( m_hoverItem == BOTTOM_BACK_LEFT )
	{
		glColor3fv( selectColor );
		glPointSize( selectedSize );
	}
	unsigned int glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, BOTTOM_BACK_LEFT );
	glLoadName( glID );
	glBegin( GL_POINTS );
	glVertex3f( p0.x, p0.y, p0.z );
	glEnd();

	//bottom back right
	glPointSize( unselectedSize );
	glColor3fv( pointColor );
	if( m_hoverItem == BOTTOM_BACK_RIGHT )
	{
		glColor3fv( selectColor );
		glPointSize( selectedSize );
	}
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, BOTTOM_BACK_RIGHT );
	glLoadName( glID );
	glBegin( GL_POINTS );
	glVertex3f( p1.x, p1.y, p1.z );
	glEnd();

	//top back right
	glPointSize( unselectedSize );
	glColor3fv( pointColor );
	if( m_hoverItem == TOP_BACK_RIGHT )
	{
		glColor3fv( selectColor );
		glPointSize( selectedSize );
	}
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, TOP_BACK_RIGHT );
	glLoadName( glID );
	glBegin( GL_POINTS );
	glVertex3f( p2.x, p2.y, p2.z );
	glEnd();

	//top back left
	glPointSize( unselectedSize );
	glColor3fv( pointColor );
	if( m_hoverItem == TOP_BACK_LEFT )
	{
		glColor3fv( selectColor );
		glPointSize( selectedSize );
	}
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, TOP_BACK_LEFT );
	glLoadName( glID );
	glBegin( GL_POINTS );
	glVertex3f( p3.x, p3.y, p3.z );
	glEnd();

	//bottom front left
	glPointSize( unselectedSize );
	glColor3fv( pointColor );
	if( m_hoverItem == BOTTOM_FRONT_LEFT )
	{
		glColor3fv( selectColor );
		glPointSize( selectedSize );
	}
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, BOTTOM_FRONT_LEFT );
	glLoadName( glID );
	glBegin( GL_POINTS );
	glVertex3f( p4.x, p4.y, p4.z );
	glEnd();

	//bottom front right
	glPointSize( unselectedSize );
	glColor3fv( pointColor );
	if( m_hoverItem == BOTTOM_FRONT_RIGHT )
	{
		glColor3fv( selectColor );
		glPointSize( selectedSize );
	}
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, BOTTOM_FRONT_RIGHT );
	glLoadName( glID );
	glBegin( GL_POINTS );
	glVertex3f( p5.x, p5.y, p5.z );
	glEnd();

	//top front right
	glPointSize( unselectedSize );
	glColor3fv( pointColor );
	if( m_hoverItem == TOP_FRONT_RIGHT )
	{
		glColor3fv( selectColor );
		glPointSize( selectedSize );
	}
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, TOP_FRONT_RIGHT );
	glLoadName( glID );
	glBegin( GL_POINTS );
	glVertex3f( p6.x, p6.y, p6.z );
	glEnd();

	//top front left
	glPointSize( unselectedSize );
	glColor3fv( pointColor );
	if( m_hoverItem == TOP_FRONT_LEFT )
	{
		glColor3fv( selectColor );
		glPointSize( selectedSize );
	}
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, TOP_FRONT_LEFT );
	glLoadName( glID );
	glBegin( GL_POINTS );
	glVertex3f( p7.x, p7.y, p7.z );
	glEnd();

	glPopAttrib();

}

void CGraphicsRotationCube::DrawEdges( )
{
	float w = m_cubeSize/2;
	if( zero( w ) ) w = 0.1f;

	float edgeColor[3] = {0.5f, 0.5f, 0.5f};
	float selectColor[] = { 0.4f, 0.4f, 0.8f };

	CPoint3D p0(-w, -w, -w );
	CPoint3D p1( w, -w, -w );
	CPoint3D p2( w,  w, -w );
	CPoint3D p3(-w,  w, -w );
	CPoint3D p4(-w, -w,  w );
	CPoint3D p5( w, -w,  w );
	CPoint3D p6( w,  w,  w );
	CPoint3D p7(-w,  w,  w );

	////////////////// edges ////////////////////////////////////////////////////////
	glPushAttrib( GL_LINE_BIT | GL_CURRENT_BIT);
	glLineWidth( 4 );
	glColor3fv( edgeColor );

	//top front
	glColor3fv( edgeColor );
	if( m_hoverItem == TOP_FRONT )
		glColor3fv( selectColor );
	unsigned int glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, TOP_FRONT );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p7.x, p7.y, p7.z );
	glVertex3f( p6.x, p6.y, p6.z );
	glEnd();

	//bottom front
	glColor3fv( edgeColor );
	if( m_hoverItem == BOTTOM_FRONT )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, BOTTOM_FRONT );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p4.x, p4.y, p4.z );
	glVertex3f( p5.x, p5.y, p5.z );
	glEnd();	

	//left front
	glColor3fv( edgeColor );
	if( m_hoverItem == LEFT_FRONT )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, LEFT_FRONT );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p7.x, p7.y, p7.z );
	glVertex3f( p4.x, p4.y, p4.z );
	glEnd();	

	//right front
	glColor3fv( edgeColor );
	if( m_hoverItem == RIGHT_FRONT )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, RIGHT_FRONT );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p6.x, p6.y, p6.z );
	glVertex3f( p5.x, p5.y, p5.z );
	glEnd();	

	//top back
	glColor3fv( edgeColor );
	if( m_hoverItem == TOP_BACK )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, TOP_BACK );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p2.x, p2.y, p2.z );
	glVertex3f( p3.x, p3.y, p3.z );
	glEnd();

	//bottom back
	glColor3fv( edgeColor );
	if( m_hoverItem == BOTTOM_BACK )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, BOTTOM_BACK );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p0.x, p0.y, p0.z );
	glVertex3f( p1.x, p1.y, p1.z );
	glEnd();	

	//left back
	glColor3fv( edgeColor );
	if( m_hoverItem == LEFT_BACK )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, LEFT_BACK );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p3.x, p3.y, p3.z );
	glVertex3f( p0.x, p0.y, p0.z );
	glEnd();	

	//right back
	glColor3fv( edgeColor );
	if( m_hoverItem == RIGHT_BACK )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, RIGHT_BACK );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p2.x, p2.y, p2.z );
	glVertex3f( p1.x, p1.y, p1.z );
	glEnd();	

	//top left
	glColor3fv( edgeColor );
	if( m_hoverItem == TOP_LEFT )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, TOP_LEFT );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p3.x, p3.y, p3.z );
	glVertex3f( p7.x, p7.y, p7.z );
	glEnd();

	//bottom left
	glColor3fv( edgeColor );
	if( m_hoverItem == BOTTOM_LEFT )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, BOTTOM_LEFT );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p0.x, p0.y, p0.z );
	glVertex3f( p4.x, p4.y, p4.z );
	glEnd();	

	//top right
	glColor3fv( edgeColor );
	if( m_hoverItem == TOP_RIGHT )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, TOP_RIGHT );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p6.x, p6.y, p6.z );
	glVertex3f( p2.x, p2.y, p2.z );
	glEnd();	

	//bottom right
	glColor3fv( edgeColor );
	if( m_hoverItem == BOTTOM_RIGHT )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, BOTTOM_RIGHT );
	glLoadName( glID );
	glBegin( GL_LINES );
	glVertex3f( p5.x, p5.y, p5.z );
	glVertex3f( p1.x, p1.y, p1.z );
	glEnd();	

	glPopAttrib();
}

void CGraphicsRotationCube::DrawFaces( )
{
	float w = m_cubeSize/2;
	if( zero( w ) ) w = 0.1f;
	
	CPoint3D p0(-w, -w, -w );
	CPoint3D p1( w, -w, -w );
	CPoint3D p2( w,  w, -w );
	CPoint3D p3(-w,  w, -w );
	CPoint3D p4(-w, -w,  w );
	CPoint3D p5( w, -w,  w );
	CPoint3D p6( w,  w,  w );
	CPoint3D p7(-w,  w,  w );

	CGLFace bottom( p0, p1, p5, p4 );
	CGLFace right( p1, p2, p6, p5 );
	CGLFace front( p4, p5, p6, p7 );
	CGLFace left( p3, p0, p4, p7 );
	CGLFace back( p1, p0, p3, p2 );
	CGLFace top( p2, p3, p7, p6 );

	glPushAttrib( GL_POLYGON_BIT | GL_CURRENT_BIT);
	
	StartSolidDrawing(1);
	float faceColor[] = {0.8f, 0.8f, 0.8f};
	float selectColor[] = { 0.7f, 0.7f, 0.8f };

	glColor3fv( faceColor );
	if( m_hoverItem == FRONT_FACE )
		glColor3fv( selectColor );
	unsigned int glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, FRONT_FACE );
	glLoadName( glID );	
	glBegin( GL_QUADS );
		front.Draw();
	glEnd();

	glColor3fv( faceColor );
	if( m_hoverItem == BACK_FACE )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, BACK_FACE );
	glLoadName( glID );
	glBegin( GL_QUADS );
		back.Draw();
	glEnd();

	glColor3fv( faceColor );
	if( m_hoverItem == LEFT_FACE )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, LEFT_FACE );
	glLoadName( glID );
	glBegin( GL_QUADS );
		left.Draw();
	glEnd();

	glColor3fv( faceColor );
	if( m_hoverItem == RIGHT_FACE )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, RIGHT_FACE );
	glLoadName( glID );
	glBegin( GL_QUADS );
		right.Draw();
	glEnd();

	glColor3fv( faceColor );
	if( m_hoverItem == TOP_FACE )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, TOP_FACE );
	glLoadName( glID );
	glBegin( GL_QUADS );
		top.Draw();
	glEnd();

	glColor3fv( faceColor );
	if( m_hoverItem == BOTTOM_FACE )
		glColor3fv( selectColor );
	glID = SetGraphicsName( GRAPHIC_ROTATION_CUBE, BOTTOM_FACE );
	glLoadName( glID );
	glBegin( GL_QUADS );
		bottom.Draw();
	glEnd();

	glPopAttrib();

}

void CGraphicsRotationCube::DrawLabels( CDC* pDC )
{
	if( !m_text )
		return;

	COLORREF black = 0x333333;//0x555555

	float w = m_cubeSize/2;
	//push the text out a bit...
	w*=1.1f;

	CString fontType = "Arial";
	m_text->ClearText();
	m_text->SetHAlignment( CGLText::HCENTER );
	m_text->SetVAlignment( CGLText::VMIDDLE );

	glDepthRange( 0, 0.5 );
	
	glPushMatrix();
	glTranslatef( 0, 0, w );
	m_text->AddLine( "Front" );
	m_text->DrawText3D( pDC, fontType, 20, black );
	m_text->ClearText();
	glPopMatrix();

	glPushMatrix();
	glRotatef( 180, 0, 1, 0 );
	glTranslatef( 0, 0, w );
	m_text->AddLine( "Back" );
	m_text->DrawText3D( pDC, fontType, 20, black );
	m_text->ClearText();
	glPopMatrix();

	glPushMatrix();
	glRotatef( -90, 0, 1, 0 );
	glTranslatef( 0, 0, w );
	m_text->AddLine( "Left" );
	m_text->DrawText3D( pDC, fontType, 20, black );
	m_text->ClearText();
	glPopMatrix();

	glPushMatrix();
	glRotatef( 90, 0, 1, 0 );
	glTranslatef( 0, 0, w );
	m_text->AddLine( "Right" );
	m_text->DrawText3D( pDC, fontType, 20, black );
	m_text->ClearText();
	glPopMatrix();

	glPushMatrix();
	glRotatef( -90, 1, 0, 0 );
	glTranslatef( 0, 0, w );
	m_text->AddLine( "Top" );
	m_text->DrawText3D( pDC, fontType, 20, black );
	m_text->ClearText();
	glPopMatrix();

	glPushMatrix();
	glRotatef( 90, 1, 0, 0 );
	glTranslatef( 0, 0, w );
	m_text->AddLine( "Bottom" );
	m_text->DrawText3D( pDC, fontType, 20, black );
	m_text->ClearText();
	glPopMatrix();

	glDepthRange( 0, 1 );
}

