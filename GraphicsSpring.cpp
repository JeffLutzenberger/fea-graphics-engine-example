#include "StdAfx.h"
#include "GraphicsSpring.h"
#include "Graphics/GraphicsHelpers.h"
#include "ini.h"
#include "IniSize.h"
#include "IniColor.h"
#include "IniFont.h"
#include "Node.h"
#include "Model.h"
#include "project.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif


CGraphicsSpring::CGraphicsSpring( const CSpring* pS ) :
m_pS( pS )
{
	for( int i = 0; i < 16; i++ )
	{
		if( i%5 == 0 )
			m_mv[i] = 1;
		else
			m_mv[i] = 0;
	}
}

CGraphicsSpring::~CGraphicsSpring(void)
{
	for( int i = 0; i < 16; i++ )
	{
		if( i%5 == 0 )
			m_mv[i] = 1;
		else
			m_mv[i] = 0;
	}
}

void CGraphicsSpring::Draw( CDC* pDC, const CWindowFilter& filter, double scale, CGLText* text, float springLength )
{
	ASSERT_RETURN( m_pS );

	COLORREF c = ini.color().springs;
	if( m_pS->isSelected() )
		c = InverseColor( ini.color().springs );
	ApplyAmbientGLMaterialiv( c );

	float saveWidth;
	glGetFloatv( GL_LINE_WIDTH, &saveWidth );
	glLineWidth( float(ini.size().springWidth) );

	glPushMatrix();
	float cx = (float)(m_pS->cosine( X ));
	float cy = (float)(m_pS->cosine( Y ));
	float cz = (float)(m_pS->cosine( Z ));

	double x1 = m_pS->node(1)->x();
	double y1 = m_pS->node(1)->y();
	double z1 = m_pS->node(1)->z();
	const CResult* cR = NULL;
	bool bDeformed = (filter.displacements && filter.resultCase() && filter.windowType == POST_WINDOW);
	if( bDeformed && theProject.isValid( filter.resultCase() ) ) 
	{
		//get displacements from the node that this spring is attached to
		cR = filter.resultCase()->result( *(m_pS->node(1)) );
		if( cR )
		{
			x1 += scale*cR->displacement( DX );
			y1 += scale*cR->displacement( DY );
			z1 += scale*cR->displacement( DZ );
		}
	}
	glTranslatef( float(x1), float(y1), float(z1) );

	CVector3D dir_x( cx, cy, cz );
	CVector3D dir_z = dir_x.cross( CVector3D( 1, 0, 0 ) );

	if( !dir_z.is_zero() )
	{
		//apply transformation
		CVector3D dir_y = dir_z.cross( dir_x ); 
		double m[3][3] = { dir_x.x, dir_x.y, dir_x.z,
			               dir_y.x, dir_y.y, dir_y.z,
						   dir_z.x, dir_z.y, dir_z.z};
		CQuaternion q( m );
		CVector3D axis;
		double angle;
		q.Disassemble( axis, angle );
		glRotatef( float(angle*180/M_PI), float(axis.x), float(axis.y), float(axis.z) );
	}
	else if( equal( cx, -1.0 ) )
	{
		glRotatef( 180.f, 0.f, 0.f, 1.f );
	}

	if( m_pS->direction() == DX || 
		m_pS->direction() == DY || 
		m_pS->direction() == DZ || 
		m_pS->direction() == NO_DIRECTION )
	{
		DrawHelix( springLength, 4, springLength/10 );
	}
	else if( m_pS->direction() == RX || 
		m_pS->direction() == RY || 
		m_pS->direction() == RZ ||
		m_pS->direction() == ALL_DIRECTIONS )
	{
		DrawSpiral( 3, springLength/5 );
	}

	//now text
	text->ClearText();
	CString label = "";
	if( filter.spring.name && filter.windowType == MODEL_WINDOW )
	{
		label = m_pS->name();
		text->AddLine( label );
	}
	if( filter.spring.stiffness && filter.windowType == MODEL_WINDOW )
	{
		if( isRotation(m_pS->direction()) ) {
			label.Format( "%s", Units::show( ROTATIONAL_STIFFNESS_UNIT, m_pS->stiffness() ));
		}
		else {
			label.Format( "%s", Units::show( LINEAR_FORCE_UNIT, m_pS->stiffness() ));
		}
		text->AddLine( label );
	}
	if( filter.windowType == POST_WINDOW )
	{
		//get results for the spring
		cR = NULL;
		cR = filter.resultCase()->result( *m_pS );
		if( filter.spring.force && cR ) 
		{
			if( isRotation(m_pS->direction()) ) 
				label = Units::show( MOMENT_UNIT, cR->force() );			
			else 
				label = Units::show( FORCE_UNIT, cR->force() );
			text->AddLine( label );
		}
		else if( filter.spring.displacement ) 
		{
			if( isRotation(m_pS->direction()) && cR) 
				label = Units::show( ANGLE_UNIT, cR->displacement() );
			else 
				label = Units::show( LENGTH_SMALL_UNIT, cR->displacement() );
			text->AddLine( label );
		}
	}

	CPoint tip2D;
	CPoint tail2D;
	double len = (double)springLength;
	if( pDC && 
		GetScreenPtFrom3DPoint( CPoint3D( 0, 0, 0 ), tip2D ) &&
		GetScreenPtFrom3DPoint( CPoint3D( len, 0, 0), tail2D ))
	{
		double angle = atan2( -(float)(tail2D.y - tip2D.y), (float)(tail2D.x - tip2D.x) );
		text->DrawText2D( pDC, ini.font().graphWindow.name, 
		ini.font().graphWindow.size/*m_printScale*/, 
		ini.color().nodes, tail2D, angle  ); 
	}
	glLineWidth( saveWidth );

	glPopMatrix();

}
