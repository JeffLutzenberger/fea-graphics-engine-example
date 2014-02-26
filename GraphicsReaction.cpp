#include "StdAfx.h"
#include "GraphicsReaction.h"

#include "Graphics/GraphicsHelpers.h"

CGraphicsReaction::CGraphicsReaction( const CNode* pN, const CResultCase* pRC ):
m_pN( pN ),
m_pRC( pRC )
{
}

CGraphicsReaction::~CGraphicsReaction(void)
{
}

void CGraphicsReaction::Draw( CDC* pDC, const CWindowFilter& filter, double scale, CGLText* text, double length )
{
	ASSERT( m_pN && m_pRC );

	if( !m_pN || !m_pRC || m_pN->isFree() )
		return;

	float w = (float)ini.size().reactionWidth;
	float l = length;
	
	const CResult* cR = m_pRC->result( *m_pN );
	double xi = m_pN->x();
	double yi = m_pN->y();
	double zi = m_pN->z();
	if( cR )
	{
		xi += scale*cR->displacement( DX );
		yi += scale*cR->displacement( DY );
		zi += scale*cR->displacement( DZ );
		
	}
	COLORREF c = ini.color().nodes;
	ApplyAmbientGLMaterialiv( c );
	glPushMatrix();
	glTranslatef( float( xi ), float( yi ), float( zi ) );
	double mv[16];
	glGetDoublev( GL_MODELVIEW_MATRIX, mv );
	for( int i=0; i<3; i++ ){ 
		for( int j=0; j<3; j++ ) {
			if ( i==j )
				mv[i*4+j] = 1.0;
			else
				mv[i*4+j] = 0.0;
		}
	}
	CPoint p2D;
	CVector3D mvz( mv[8], mv[9], mv[10] );
	CPoint3D p3D( 0., -l, 0. );
	if( m_pN->isFixed( DX ) && !zero(cR->force( DX )) && filter.node.fx )
	{
		CString label = "FX = ";
		label += Units::show( FORCE_UNIT, cR->force( DX ));
		text->ClearText();
		text->AddLine( label );
		glPushMatrix();
			if( cR->force( DX ) < 0. )
				glRotatef( 180.f, 0.f, 0.f, 1.f );
			DrawLineArrow( l, w, pDC, text,
						ini.font().graphWindow.name, 
						ini.font().graphWindow.size, 
						ini.color().nodalLoads); 
		glPopMatrix();
	}
	if( m_pN->isFixed( DY ) && !zero(cR->force( DY )) && filter.node.fy )
	{
		CString label = "FY = ";
		label += Units::show( FORCE_UNIT, cR->force( DY ));
		text->ClearText();
		text->AddLine( label );
		glPushMatrix();
		glRotatef( 90.f, 0.f, 0.f, 1.f );
			if( cR->force( DY ) < 0. )
				glRotatef( 180.f, 0.f, 0.f, 1.f );
			DrawLineArrow( l, w, pDC, text,
						ini.font().graphWindow.name, 
						ini.font().graphWindow.size, 
						ini.color().nodalLoads); 
		glPopMatrix();
	}
	if( m_pN->isFixed( DZ ) && !zero(cR->force( DZ )) && filter.node.fz )
	{
		CString label = "FZ = ";
		label += Units::show( FORCE_UNIT, cR->force( DZ ));
		text->ClearText();
		text->AddLine( label );
		glPushMatrix();
			glRotatef( 90.f, 0.f, 1.f, 0.f );
			if( cR->force( DZ ) < 0. )
				glRotatef( 180.f, 0.f, 0.f, 1.f );
			DrawLineArrow( l, w, pDC, text,
						ini.font().graphWindow.name, 
						ini.font().graphWindow.size, 
						ini.color().nodalLoads); 
		glPopMatrix();
	}
	if( m_pN->isFixed( RX ) && !zero(cR->force( RX )) && filter.node.mx )
	{
		CString label = "MX = ";
		label += Units::show( MOMENT_UNIT, cR->force( RX ));
		text->ClearText();
		text->AddLine( label );
		glPushMatrix();
			glRotatef( 90.f, 0.f, 1.f, 0.f );
			if( cR->force( RX ) < 0. )
				glRotatef( 180.f, 0.f, 0.f, 1.f );
			DrawSolidMomentArrow( l, w, pDC, text,
						ini.font().graphWindow.name, 
						ini.font().graphWindow.size, 
						ini.color().nodalLoads); 
		glPopMatrix();	
		text->AddLine( label );
	}
	if( m_pN->isFixed( RY ) && !zero(cR->force( RY )) && filter.node.my )
	{
		CString label = "MY = ";
		label += Units::show( MOMENT_UNIT, cR->force( RY ));
		text->ClearText();
		text->AddLine( label );
		glPushMatrix();
			glRotatef( 90.f, 1.f, 0.f, 0.f );
			if( cR->force( RY ) > 0. )
				glRotatef( 180.f, 0.f, 0.f, 1.f );
			DrawSolidMomentArrow( l, w, pDC, text,
						ini.font().graphWindow.name, 
						ini.font().graphWindow.size, 
						ini.color().nodalLoads); 
		glPopMatrix();	
	}
	if( m_pN->isFixed( RZ ) && !zero(cR->force( RZ )) && filter.node.mz )
	{
		CString label = "MZ = ";
		label += Units::show( MOMENT_UNIT, cR->force( RZ ));
		text->ClearText();
		text->AddLine( label );
		glPushMatrix();
			if( cR->force( RZ ) < 0. )
				glRotatef( 180.f, 0.f, 1.f, 0.f );
			DrawSolidMomentArrow( l, w, pDC, text,
						ini.font().graphWindow.name, 
						ini.font().graphWindow.size, 
						ini.color().nodalLoads); 
		glPopMatrix();
	}
	glPopMatrix();
}