#include "StdAfx.h"
#include "GraphicsMovingLoad.h"
#include "Member.h"
#include "Node.h"
#include "Graphics/SpineGenerator.h"
#include "Graphics/GraphicsHelpers.h"
#include "ShapeDB/CrossSection.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CGraphicsMovingLoad::CGraphicsMovingLoad( const CMovingLoad* pML )
{
	for( int i = 0; i < 16; i++ )
	{
		if( i%5 == 0 )
			m_mv[i] = 1;
		else
			m_mv[i] = 0;
	}
	SetupMovingLoad( pML );
}

CGraphicsMovingLoad::~CGraphicsMovingLoad(void)
{
	for( int i = 0; i < 16; i++ )
	{
		if( i%5 == 0 )
			m_mv[i] = 1;
		else
			m_mv[i] = 0;
	}
}

void CGraphicsMovingLoad::SetupMovingLoad( const CMovingLoad* pML )
{
	ASSERT_RETURN( pML );
	//get the local spine
	CPoint3DArray spine;
	m_pML = pML;
	const CMember* pM = pML->member();
	ASSERT_RETURN( pM );

	float w;
	glGetFloatv( GL_LINE_WIDTH, &w );
	// get global member orientation matrix
	// TeK Change 7/10/2007: Member Loads are NOT affected by a 'theta' angle! (changed true to false below)
	double Xdir[3] = {1.0,0.0,0.0};
	pM->localToGlobal( Xdir, false );
	double Ydir[3] = {0.0,1.0,0.0};
	pM->localToGlobal( Ydir, false );
	double Zdir[3] = {0.0, 0.0, 1.0};
	pM->localToGlobal( Zdir, false );

	//member orientation matrix
	double m[4][4] = {
		{Xdir[0], Xdir[1], Xdir[2],    0.0f},
		{Ydir[0], Ydir[1], Ydir[2],    0.0f},
		{Zdir[0], Zdir[1], Zdir[2],    0.0f},
		{0.0f,      0.0f,     0.0f,		1.0f}};
	CQuaternion q( m );
	CVector3D axis;
	double angle;
	//member location
	q.Disassemble( axis, angle );
	m_loc.x = pM->node(1)->x();
	m_loc.y = pM->node(1)->y() + pM->centerlineOffset( DY );
	m_loc.z = pM->node(1)->z() + pM->centerlineOffset( DZ );
	m_rot = (float)angle;
	m_axis = axis;
	//member orientation
	m_dir.x = pM->node(2)->x() - pM->node(1)->x();
	m_dir.y = pM->node(2)->y() - pM->node(1)->y();
	m_dir.z = pM->node(2)->z() - pM->node(1)->z();
	m_dir.normalize();
}

void CGraphicsMovingLoad::Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, float loadLength )
{
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	//translate and rotate coordinate system into local member coordinate system
	glTranslatef( (float)m_loc.x, (float)m_loc.y, (float)m_loc.z );
	glRotatef( float(m_rot*180/M_PI), (float)(m_axis.x), (float)(m_axis.y), (float)(m_axis.z) );
	DrawMovingLoad( filter, loadLength );
	//get the modelview matrix
	glGetDoublev( GL_MODELVIEW_MATRIX, m_mv );
	glPopMatrix();

	//draw text...
	if( filter.loadValues )
	{
		CString label = "";
		ETUnit quantity;
		for( int i = 1; i <= m_pML->truck().axleLoads(); i++ )
		{
			const CAxleLoad* aL = m_pML->truck().getAxleLoad(i);
			quantity = FORCE_UNIT;
			label += "Axle ";
			CString buf;
			buf.Format( "%i", i );
			label += buf;
			if( aL )
				label += Units::show(  quantity , aL->magnitude() );
			label += " ";
		}
		if( m_pML->isReversible() )
			label += "Reversible ";
		else
			label += "One-way ";
		if( m_pML->hasUniformLoad() ){
			label += "Lane Load ";
			quantity = LINEAR_FORCE_UNIT;
			label += Units::show(  quantity , m_pML->uniformLoad() );
			label += " ";
		}
		text->ClearText();
		if( label.GetLength() > 0 )
			text->AddLine( label );
		//CGraphicsMovingLoad ml( pML );
		const CNode* pN1 = m_pML->member()->node(1);
		const CNode* pN2 = m_pML->member()->node(2);
		ASSERT_RETURN( pN1 && pN2 );	
		CPoint3D pt3D1( pN1->x(), pN1->y(), pN1->z());
		CPoint3D pt3D2( pN2->x(), pN2->y(), pN2->z());
		CPoint3D center = (pt3D1 + pt3D2) * 0.5;
		CPoint tip2D;
		CPoint tail2D;
		if( GetScreenPtFrom3DPoint( pt3D1, tip2D )	&&
			GetScreenPtFrom3DPoint( pt3D2, tail2D ))
		{
			double angle = atan2( -(float)(tip2D.y - tail2D.y), (float)(tip2D.x - tail2D.x) );
			//text->SetVAlignment( CGLOutlineText::VMIDDLE );
			text->SetHAlignment( CGLOutlineText::HCENTER );
			text->DrawText2D( pDC,
							ini.font().graphWindow.name, 
							ini.font().graphWindow.size, 
							ini.color().nodalLoads, 
							tail2D, angle  ); 
		}
	} //load values
}

void CGraphicsMovingLoad::DrawMovingLoad( const CWindowFilter& filter, float loadLength )
{
	ASSERT_RETURN( m_pML );

	double line_length = loadLength;//ini.size().memberLoadLength;
	double line_width = ini.size().memberLoadWidth;

	//in local member coordinates now...

	//determine arrow angle and axis of rotation
	double arrowAngle = 0.;
	CVector3D arrowRotAxis;
	GetArrowRotation( arrowAngle, arrowRotAxis );

	float radius = (float)(line_length/4.f);
	double mlength = m_pML->member()->length();

	if( mlength <= 0 )
		return;

	CCrossSection sec( m_pML->member() );
	CPoint3DArray dummy;
	sec.GetCrossSection( dummy, true );
	float transW = sec.GetYMax();
	if( filter.drawDetail == DETAIL_LOW ) 
		transW = 0.f;

	//represent truck as a translucent quad
	//double tlength = m_pML->truck().loadLength();
	double mid = (mlength)/2.;  // a fractional offset
	//EnableGLTransparency();
	//glPushMatrix();
	//glTranslatef( (float)(mid-tlength/2.), 2*transW, 0. );
	//glBegin(GL_QUADS);
	//glVertex3f( 0.f, (float)line_length, 0.f );
	//glVertex3f( 0.f, 0.f, 0.f );
	//glVertex3f( (float)tlength, 0.f, 0.f );
	//glVertex3f( (float)tlength, (float)line_length, 0.f );
	//glEnd();
	//StartWireframeDrawing();
	//glBegin(GL_QUADS);
	//glVertex3f( 0.f, (float)line_length, 0.f );
	//glVertex3f( 0.f, 0.f, 0.f );
	//glVertex3f( (float)tlength, 0.f, 0.f );
	//glVertex3f( (float)tlength, (float)line_length, 0.f );
	//glEnd();
	//EndWireframeDrawing();
	//StartSolidDrawingNoLightingNoAA();
	//glPopMatrix();
	//DisableGLTransparency();
	
	if( m_pML->truck().axleLoads() > 0 ) 
	{
		//show direction arrows above the truck quad
		if( m_pML->isReversible() ){
			glPushMatrix( );
			glTranslatef( (float)(mid-2*line_length), 4*radius, 0.f );
			DrawLineArrow( float( -line_length ), float( line_width ) );
			glTranslatef( (float)(4*line_length), 0.f, 0.f );
			DrawLineArrow( float( line_length ), float( line_width ) );
			glPopMatrix();
		}
		else{
			glPushMatrix( );
			glTranslatef( (float)(mid-line_length/2), 4*radius, 0.f );
			DrawLineArrow( float( line_length ), float( line_width ) );
			glPopMatrix();
		}

		//now show wheel and load arrow
		double axel_offset = /*aL->offset() +*/ mid /*- tlength/2.*/;
		glPushMatrix();
		glTranslatef( (float)axel_offset, transW+radius, 0.f );
		glRotatef( -float(m_rot*180/M_PI), (float)(m_axis.x), (float)(m_axis.y), (float)(m_axis.z) );
		//direction of the member determines if global y is up or down, we want it to always be up
		if( m_dir.cross( CVector3D( 0., 1., 0.) ).length() < 0 )
		{
			glRotatef( float(arrowAngle*180/M_PI), (float)arrowRotAxis.x, 
						(float)arrowRotAxis.y, 
						(float)arrowRotAxis.z );
		}
		else
		{
			glRotatef( float(-arrowAngle*180/M_PI), (float)arrowRotAxis.x, 
						(float)arrowRotAxis.y, 
						(float)arrowRotAxis.z );
		}
		CCrossSection sec( m_pML->member() );
		CPoint3DArray dummy;
		switch( filter.drawDetail )
		{
				case DETAIL_LOW:
					DrawSolidCircle( radius, float( line_width ) ); 
					glTranslatef( -radius, 0.f, 0.f );
					DrawLineArrow( float( line_length ), float( line_width ) );
					break;
				case DETAIL_MED:
					DrawSolidCircle( radius, float( line_width ) ); 
					glTranslatef( -radius, 0.f, 0.f );
					DrawLineArrow( float( line_length ), float( line_width ) );
					break;
				case DETAIL_HI:
					DrawSolidCircle( radius, float( line_width ) ); 
					glTranslatef( -radius, 0.f, 0.f );
					DrawLineArrow( float( line_length ), float( line_width ) );
					break;
				default:
					break;
		}
		glPopMatrix();
	}
	// show uniform lane load
	if( m_pML->hasUniformLoad() )
	{
		double mag = m_pML->uniformLoad();
		double start = 0.;
		double end = mlength;
		double loadLength = abs( end-start );
		int nArrows = int(abs(loadLength/line_length));
		if( nArrows <= 2 )
			nArrows = 3;
		float delta = float(loadLength/(nArrows));
		//set up arrow scaling for linear distributed load
		for( int i = 0; i <= nArrows; i++ )
		{
			glPushMatrix();
			if( mag < 0 )
				glTranslatef( 0.f, transW, 0.f );
			else glTranslatef( 0.f, -transW, 0.f );
			glRotatef( -float(m_rot*180/M_PI), (float)(m_axis.x), (float)(m_axis.y), (float)(m_axis.z) );
			//direction of the member determines if global y is up or down, we want it to always be up
			if( m_dir.cross( CVector3D( 0., 1., 0.) ).length() < 0 )
			{
				glRotatef( float(-arrowAngle*180/M_PI), (float)arrowRotAxis.x, 
					(float)arrowRotAxis.y, 
					(float)arrowRotAxis.z );
			}
			else
			{
				glRotatef( float(arrowAngle*180/M_PI), (float)arrowRotAxis.x, 
					(float)arrowRotAxis.y, 
					(float)arrowRotAxis.z );
			}
			DrawLineArrow( float( line_length ), float( line_width ) );
			glPopMatrix();
			glTranslatef( delta, 0.f, 0.f );
		}
	}
}

void CGraphicsMovingLoad::GetArrowRotation( double& angle, CVector3D& axis )
{
	if( m_pML->direction() == DY  )
	{
		angle = M_PI*0.5f;
		axis = CVector3D( 0., 0., 1. );
	}
	else 
	{
		ASSERT_RETURN( false );
	}
}

void CGraphicsMovingLoad::GetTextPoint( CPoint& p2D, double arrowLength, 
									   ETGraphicMoveLoadText etTextType, int axel_num )
{
	ASSERT_RETURN( m_pML );

	//in local member coordinates now...

	//determine arrow angle and axis of rotation
	double arrowAngle = 0.;
	CVector3D arrowRotAxis;
	GetArrowRotation( arrowAngle, arrowRotAxis );

	float radius = (float)(arrowLength/2.f);
	double mlength = m_pML->member()->length();

	if( mlength > 0 ) {
		CCrossSection sec( m_pML->member() );
		CPoint3DArray dummy;
		sec.GetCrossSection( dummy, true );
		float transW = sec.GetYMax();
		//if( detail == DETAIL_LOW ) transW = 0.f;

		//represent truck as a translucent quad
		double tlength = m_pML->truck().loadLength();
		double mid = (mlength)/2.;  // a fractional offset

		// load direction text
		if( m_pML->truck().axleLoads() > 0 && etTextType == MOVE_DIR) {
			//show direction arrows above the truck quad
			glPushMatrix( );
			glTranslatef( (float)(mid), 5*radius, 0.f );
			CPoint3D p3D( 0., 0., 0. );
			GetScreenPtFrom3DPoint( p3D, p2D );
			glPopMatrix();
		}
		// axle load text
		else if( m_pML->truck().axleLoads() > 0 && etTextType == WHEEL_LOAD && 
			     m_pML->truck().axleLoads() <= axel_num ) {
			//now show wheels and load arrows
			const CAxleLoad* aL = m_pML->truck().getAxleLoad(axel_num);
			double axel_offset = aL->offset() + mid - tlength/2.;
			glPushMatrix();
			glTranslatef( (float)axel_offset, transW+radius, 0.f );
			glRotatef( -float(m_rot*180/M_PI), (float)(m_axis.x), (float)(m_axis.y), (float)(m_axis.z) );
			//direction of the member determines if global y is up or down, we want it to always be up
			if( m_dir.cross( CVector3D( 0., 1., 0.) ).length() < 0 )
			{
				glRotatef( float(arrowAngle*180/M_PI), (float)arrowRotAxis.x, 
						(float)arrowRotAxis.y, 
						(float)arrowRotAxis.z );
			}
			else
			{
				glRotatef( float(-arrowAngle*180/M_PI), (float)arrowRotAxis.x, 
						(float)arrowRotAxis.y, 
						(float)arrowRotAxis.z );
			}
			CCrossSection sec( m_pML->member() );
			CPoint3DArray dummy;
			glTranslatef( -float(radius+1.5*arrowLength), 0.f, 0.f );
			CPoint3D p3D( 0., 0., 0. );
			GetScreenPtFrom3DPoint( p3D, p2D );
			glPopMatrix();
		}
		// uniform lane load text
		else if( m_pML->truck().axleLoads() > 0 && etTextType == LANE_LOAD ){
			double mag = m_pML->uniformLoad();
			//double start = 0.;
			//double end = mlength;
			glPushMatrix();
			if( mag < 0 ) glTranslatef( 0.f, float(transW+1.5f*arrowLength), 0.f );
			else glTranslatef( 0.f, float(-transW-1.5f*arrowLength), 0.f );
			glRotatef( -float(m_rot*180/M_PI), (float)(m_axis.x), (float)(m_axis.y), (float)(m_axis.z) );
			//direction of the member determines if global y is up or down, we want it to always be up
			if( m_dir.cross( CVector3D( 0., 1., 0.) ).length() < 0 )
			{
				glRotatef( float(-arrowAngle*180/M_PI), (float)arrowRotAxis.x, 
						(float)arrowRotAxis.y, 
						(float)arrowRotAxis.z );
			}
			else
			{
				glRotatef( float(arrowAngle*180/M_PI), (float)arrowRotAxis.x, 
						(float)arrowRotAxis.y, 
						(float)arrowRotAxis.z );
			}
			glTranslatef( (float)(tlength/2.), 0.f, 0.f );
			CPoint3D p3D( 0., 0., 0. );
			GetScreenPtFrom3DPoint( p3D, p2D );
			glPopMatrix();
		}

	}

}

int CGraphicsMovingLoad::NumAxles( )
{
	if( m_pML )
		return m_pML->truck().axleLoads();
	else return 0;
}

