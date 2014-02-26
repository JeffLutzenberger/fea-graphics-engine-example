#include "StdAfx.h"
#include "GraphicsArea.h"
#include "IniSize.h"

CGraphicsArea::CGraphicsArea( const CArea* pA ) :
m_pA( pA )
{
}

CGraphicsArea::~CGraphicsArea(void)
{
}

void CGraphicsArea::Draw( ETGraphicsDrawStyle style, double spanArrowLength )
{
	ASSERT( m_pA );
	if( !m_pA )
		return;

	COLORREF c = ini.color().areas;
	if( m_pA->isSelected() || style == GRAPHIC_LINE )
		c = InverseColor( ini.color().areas );
	ApplyAmbientGLMaterialiv( c );
	
	CPoint3DArray pts;
	for( int j = 1; j <= m_pA->boundary().points(); j++ ) {
		pts.add( *m_pA->boundary().point(j) );
	}

	CGLTessellatedPolygon poly( pts );
	//do the holes, draw them transparent...
	for( int i = 1; i <= m_pA->holes(); i++ )
	{
		CPoint3DArray hole_pts;
		CChain* hole_chain = m_pA->holeArray()[i];
		if( !hole_chain ){
			ASSERT(FALSE);
			continue;
		}
		for( int j = 1; j <= hole_chain->points(); j++ )
		{
			hole_pts.add( *hole_chain->point( j ) ); 
		}
		poly.addHole( hole_pts );
	}
	if( style == GRAPHIC_LINE )
		poly.SetFilling( FALSE );
	////now draw corridors
	if( m_pA->corridors() > 0 )
	{
		for( int i = 1; i <= m_pA->corridors(); i++ )
		{
			CPoint3DArray corridor_pts;
			CChain* corridor_chain = m_pA->corridorArray()[i];
			if( !corridor_chain ){
				ASSERT(FALSE);
				continue;
			}
			for( int j = 1; j <= corridor_chain->points(); j++ )
			{
				corridor_pts.add( *corridor_chain->point( j ) );
			}
			poly.addHole( corridor_pts );
		}
	}
	
	if( ini.graphics().blending && style == GRAPHIC_SOLID )
		EnableGLTransparency( );
	poly.Draw();
	if( ini.graphics().blending && style == GRAPHIC_SOLID )
		DisableGLTransparency( );

	//draw the span direction arrow(s)
	if( m_pA )
	{
		//get average size of area to help size the span direction arrow
		double dist = ( m_pA->boundary().longestSide() + m_pA->boundary().shortestSide() )/2.;
		double length = min( dist/10, spanArrowLength );
		CVector3D v1 = m_pA->spanDirection();
		CPoint3D startPt( m_pA->boundary().centroid().x(), m_pA->boundary().centroid().y(), 
			m_pA->boundary().centroid().z() );
		CPoint3D endPt( startPt.x + v1.x*dist*0.25, startPt.y + v1.y*dist*0.25, startPt.z + v1.z*dist*0.25 );
		CPoint3D endPt2( startPt.x - v1.x*dist*0.25, startPt.y - v1.y*dist*0.25, startPt.z - v1.z*dist*0.25 );
		//int width = ini.size().planarLoadWidth;
		if( style == GRAPHIC_LINE )
		{
			COLORREF c = InverseColor( ini.color().background );
			glColor3ub( GetRValue( c ), GetGValue( c ), GetBValue( c ) );
			ApplyAmbientGLMaterialiv( c );
		}
		else
		{
			COLORREF c = ini.color().background;
			ApplyAmbientGLMaterialiv( c );
		}
		//rotate modelview matrix into plane of the area...
		CPlane3D plane = m_pA->plane();
		CVector3D n = plane.n();
		//convert plane to modelview matrix...
		CVector3D x_dir = m_pA->spanDirection();
		CVector3D z_dir = n;
		CVector3D y_dir = z_dir.cross( x_dir );
		float m[16] = {	(float)x_dir.x, (float)x_dir.y, (float)x_dir.z, 0.f,
					(float)y_dir.x, (float)y_dir.y, (float)y_dir.z, 0.f,
					(float)z_dir.x, (float)z_dir.y, (float)z_dir.z, 0.f,
					0.f, 0.f, 0.f, 1.f };
		glPushMatrix();
			glTranslatef( (float)startPt.x+n.x, (float)startPt.y+n.y, (float)startPt.z+n.z );
			glMultMatrixf( m );
			if( m_pA->isOneWay() )
				DrawSpanArrow( (float)length, style != GRAPHIC_LINE  );
			else
				DrawTwoWaySpanArrow( (float)length, style != GRAPHIC_LINE );
			glTranslatef( 0, 0, (float)(-2));
			if( m_pA->isOneWay() )
				DrawSpanArrow( (float)length, style != GRAPHIC_LINE  );
			else
				DrawTwoWaySpanArrow( (float)length, style != GRAPHIC_LINE );
		glPopMatrix();
	}

}

void CGraphicsArea::DrawCorridor( int index, ETGraphicsDrawStyle style )
{
	ASSERT( m_pA );
	if( !m_pA )
		return;

	COLORREF c = ini.color().areas;
	if( m_pA->isSelected() || style == GRAPHIC_LINE )
		c = InverseColor( ini.color().areas );
	float color[4] = { GetRValue( c )/400.f, GetGValue( c )/400.f, GetBValue( c )/400.f, ALPHA };
	ApplyAmbientGLMaterial( color );

	if( m_pA->corridors() > 0 && index <= m_pA->corridors() )
	{
		CPoint3DArray corridor_pts;
		CChain* corridor_chain = m_pA->corridorArray()[index];
		if( !corridor_chain ){
			ASSERT(FALSE);
			return;
		}
		for( int j = 1; j <= corridor_chain->points(); j++ )
		{
			corridor_pts.add( *corridor_chain->point( j ) );
		}
		CGLTessellatedPolygon corridor( corridor_pts );
		if( style == GRAPHIC_LINE || style == GRAPHIC_DASHED_LINE )
			corridor.SetFilling( FALSE );
		
		if( ini.graphics().blending && style == GRAPHIC_SOLID )
			EnableGLTransparency( );
		corridor.Draw();
		if( ini.graphics().blending && style == GRAPHIC_SOLID )
			DisableGLTransparency( );
	}
}

void CGraphicsArea::DrawHole( int index, ETGraphicsDrawStyle style )
{
	ASSERT( m_pA );
	if( !m_pA )
		return;

	COLORREF c = ini.color().areas;
	if( m_pA->isSelected() || style == GRAPHIC_LINE )
		c = InverseColor( ini.color().areas );
	float color[4] = { GetRValue( c )/400.f, GetGValue( c )/400.f, GetBValue( c )/400.f, 0.25 };
	ApplyAmbientGLMaterial( color );

	if( m_pA->holes() > 0 && index <= m_pA->holes() )
	{
		CPoint3DArray hole_pts;
		CChain* hole_chain = m_pA->holeArray()[index];
	if( !hole_chain ){
			ASSERT(FALSE);
			return;
		}
		for( int j = 1; j <= hole_chain->points(); j++ )
		{
			hole_pts.add( *hole_chain->point( j ) ); 
		}
		CGLTessellatedPolygon hole( hole_pts );
		if( style == GRAPHIC_LINE || style == GRAPHIC_DASHED_LINE )
			hole.SetFilling( FALSE );
		hole.Draw();
	}
}