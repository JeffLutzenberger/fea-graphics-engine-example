#include "StdAfx.h"
#include "GraphicsSupport.h"

#include "Project.h"
#include "IniSize.h"
#include "Graphics/GraphicsHelpers.h"

CGraphicsSupport::CGraphicsSupport( const CNode* pN ):
m_pN( pN )
{
}

CGraphicsSupport::~CGraphicsSupport(void)
{
}

void CGraphicsSupport::Draw( ETGraphicDetail detail, double length )
{
	ASSERT( m_pN );

	if( !m_pN || m_pN->isFree() )
		return;

	
	//double len = 1.0;
	//if( m_Filter.perspective )
	//{
	//	//CRect r;
	//	//GetClientRect( &r );
	//	//CPoint3D pt3D( pN->x(), pN->y(), pN->z() );
	//	//CVector3D v1( m_Camera.GetPosition(), pt3D );
	//	//len = abs(v1.dot( m_Camera.GetForwardVector() )/r.Width()); 
	//}

	double xi = m_pN->x();
	double yi = m_pN->y();
	double zi = m_pN->z();
	//fixed
	float l = (float)length;
	//float grey[4] = { .5f, .5f, .5f, 1.f };
	if( m_pN->isFixed() && detail == DETAIL_HI )
	{
		//draw a block showing a fixed support
		COLORREF c = ini.color().supports;
		if( m_pN->isSelected() )
			c = InverseColor( ini.color().supports ); // TeK Fix 7/15/2009: was .nodes?
		ApplyAmbientGLMaterialiv( c );
		glPushMatrix();
			ETCoordinate vertical = theProject.designCriteria().getVerticalDirection();
			glTranslatef( float(xi), float(yi), float(zi) );
			if( vertical == X )
				glRotatef( 90.f, 0.f, 0.f, -1.f );
			else if( vertical == Z )
				glRotatef( 90.f, 1.f, 0.f, 0.f );
			DrawBox( float(l), float(l/4), float(l), true );
		glPopMatrix();
	}
	//pinned
	else if( m_pN->isPinned() && detail == DETAIL_HI )
	{
		//draw pinned support
		COLORREF c = ini.color().supports;
		if( m_pN->isSelected() )
			c = InverseColor( ini.color().supports );  // TeK Followup fix 8/11/2009
		ApplyAmbientGLMaterialiv( c );
		glPushMatrix();
			//ApplyAmbientGLMaterial( grey );
			glTranslatef( float(xi), float(yi), float(zi) );
			ETCoordinate vertical = theProject.designCriteria().getVerticalDirection();
			glPushMatrix();
				if( vertical == X )
					glRotatef( 90.f, 0.f, 0.f, -1.f );
				else if( vertical == Z )
					glRotatef( 90.f, 1.f, 0.f, 0.f );
				glPushMatrix();
					DrawPyramid( float(l/2),float(l/2), true );
					glTranslatef( float(0.f), float(-l/2.f), float(0.f ));
					DrawBox( float(l), float(l/4), float(l), true );
				glPopMatrix();
			glPopMatrix();
			//solid boxes for fixed rotations
			if( m_pN->isFixed( RY ) )
				DrawBox( float(l/2), float(l/8), float(l/2), true );
			if( m_pN->isFixed( RZ ) )
				DrawBox( float(l/8), float(l/2), float(l/2), true );
			if( m_pN->isFixed( RX ) )
				DrawBox( float(l/2), float(l/2), float(l/8), true );
		glPopMatrix();
	}
	//other
	else
	{
		COLORREF c = ini.color().supports;
		if( m_pN->isSelected() )
			c = InverseColor( ini.color().supports );  // TeK Followup fix 8/11/2009
		SetGLColor( ini.color().supports );
		glPushMatrix();
			glTranslatef( float(xi), float(yi), float(zi) );
			//glScalef( float(len), float(len), float(len) );
			//draw all supports separately
			float w = (float)ini.size().supportWidth;
			//float l = (float)ini.size().supportLength;
			if( m_pN->isFixed( DX ) )
				DrawLineArrow( l, w, true, true );
			if( m_pN->isFixed( DY ) )
			{
				glPushMatrix();
				glRotatef( 90.f, 0.f, 0.f, 1.f );
				DrawLineArrow( l, w, true, true  );
				glPopMatrix();
			}
			if( m_pN->isFixed( DZ ) )
			{
				glPushMatrix();
				glRotatef( -90.f, 0.f, 1.f, 0.f );
				DrawLineArrow( l, w, true, true );
				glPopMatrix();
			}
			if( m_pN->isFixed( RZ ) )
				DrawLineMomentArrow( l, w, true );
			if( m_pN->isFixed( RX ) )
			{
				glPushMatrix();
				glRotatef( 90.f, 0.f, 1.f, 0.f );
				DrawLineMomentArrow( l, w, true );
				glPopMatrix();
			}
			if( m_pN->isFixed( RY ) )
			{
				glPushMatrix();
				glRotatef( 90.f, 1.f, 0.f, 0.f );
				DrawLineMomentArrow( l, w, true );
				glPopMatrix();
			}
		glPopMatrix();
	}
}