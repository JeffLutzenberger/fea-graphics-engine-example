#include "StdAfx.h"
#pragma hdrstop

#include "GraphicsGrid.h"
#include "Graphics/GraphicsHelpers.h"

#include "coord.h"
#include "Units\Units.h"
#include "coord.h"
#include "model.h"

#include "DataUtil.h"
#include "IniColor.h"
#include "Project.h"
#include "WinFiltr.h"

#include <gl/gl.h>
#include <gl/glu.h>

#define	MAXGRIDSHOW		100				// maximum number of grid lines to show before we increase spacing

const short gridVersion = 1;

#ifdef _DEBUG
	#undef THIS_FILE
	static char THIS_FILE[]=__FILE__;
	#define new DEBUG_NEW
#endif


CVAGrid::CVAGrid() : 
	m_Grids( ARRAY_SIZE, 1, ARRAY_DELTA, YES_DELETE ),  // we own these guys
	m_GridPoints( ARRAY_SIZE, 1, ARRAY_DELTA, NO_DELETE ),  // each grid owns the points, this is a copy
	m_GridLines( ARRAY_SIZE, 1, ARRAY_DELTA, NO_DELETE ), // each grid owns the lines, this is a copy
	m_DisplayLists( ARRAY_SIZE, 1, ARRAY_DELTA, YES_DELETE ) //we own these too
{

	// JL Removed 12/11/2007 - not sure this class is necessary anymore
	//CRectangularGrid* pGrid = new CRectangularGrid();
	//addGrid( *pGrid );
	//return ;
}   // end CVAGrid()


CVAGrid::~CVAGrid()
{
   m_Grids.flush();
   m_GridPoints.flush();
   m_GridLines.flush();
   return;
}   // end ~CVAGrid()


void CVAGrid::addGrid( CGraphicGrid& pGrid )
{
	m_Grids.add( pGrid );
	CGraphicsDisplayList* pDispList = new CGraphicsDisplayList;
	m_DisplayLists.add( pDispList );

	UpdateGridData();
	return;
}

void CVAGrid::removeGrid( int iGrid )
{
	ASSERT( iGrid > 0 && iGrid <= gridCount() );
	if( iGrid > 0 && iGrid <= gridCount() )
	{
		m_Grids.remove( iGrid );
		m_DisplayLists.remove( iGrid );
		UpdateGridData();
	}
	return;
}

void CVAGrid::modifyGrid( int iGrid, CGraphicGrid& pGrid )
{
	ASSERT( iGrid > 0 && iGrid <= gridCount() );
	if( iGrid > 0 && iGrid <= gridCount() )
	{
		m_Grids.replace( iGrid, pGrid );
		UpdateGridData();
	}
	return;
}

CGraphicGrid* CVAGrid::grid( int iGrid ) const
{
	ASSERT( iGrid > 0 && iGrid <= gridCount() );
	if( iGrid > 0 && iGrid <= gridCount() )
	{
		return m_Grids[iGrid];
	}
	else
		return NULL;
}

void CVAGrid::drawGrid( double pointSize )
{
	COLORREF c = ini.color().grid;
	glColor3ub( GetRValue( c ), GetGValue( c ), GetBValue( c ) );
	glPointSize( float(pointSize) );
	glBegin( GL_POINTS );

	for( int iGrid = 1; iGrid <= gridCount(); iGrid++ ) {
		CGridPointArray pts = m_Grids[iGrid]->getGridPoints();
		for( int j = 1; j <= pts.getItemsInContainer(); j++ ) {
			glVertex3f( (float)pts[j]->x, (float)pts[j]->y, (float)pts[j]->z );
		}
	}

	glEnd();
	//try drawing lines
	glBegin( GL_LINES );
	glColor3f( 0.7f, 0.7f, 0.7f );
	for( int iGrid = 1; iGrid <= gridCount(); iGrid++ )
	{
		CGridLineArray lines = m_Grids[iGrid]->getGridLines();
		for( int j = 1; j <= lines.getItemsInContainer(); j++ )
		{
			if( lines[j]->length() > 0. )
			{
				const CPoint3D& p1 = lines[j]->point(1);
				const CPoint3D& p2 = lines[j]->point(2);
				glVertex3f( (float)p1.x, (float)p1.y, (float)p1.z );
				glVertex3f( (float)p2.x, (float)p2.y, (float)p2.z );
			}
		}
	}
	glEnd();

	return;
}


// calculation functions
CPoint3D CVAGrid::closestGridPoint( const CPoint3D& pt,  double* dist )
{
	ASSERT( gridCount() > 0. );
	CPoint3D p(0., 0., 0.);
	double d = 1.E20;
	for( int iGrid = 1; iGrid <= gridCount(); iGrid++ ) {
		double d1;
		CPoint3D p1 = grid( iGrid )->closestGridPoint( pt, &d1 );
		if( d1 < d ) {
			d = d1;
			p = p1;
		}
	}
	if( dist != NULL )
		*dist = d;
	return p;
}

const CGridPointArray& CVAGrid::getGridPoints( void ) const
{
	return m_GridPoints;
}

const CGridLineArray& CVAGrid::getGridLines( void ) const
{
	return m_GridLines;
}

const CDisplayListArray& CVAGrid::getDisplayLists( void ) const
{
	return m_DisplayLists;
}


void CVAGrid::UpdateGridData( void )
{
	m_GridPoints.flush();
	m_GridLines.flush();
	for( int i = 1; i <= gridCount(); i++ ) {
		CGridPointArray pts = m_Grids[i]->getGridPoints();
		for( int j = 1; j <= pts.getItemsInContainer(); j++ )
			m_GridPoints.add( pts[j] );
		//CGridLineArray lines = m_Grids[i]->getGridLines();
		//for(int j = 1; j <= lines.getItemsInContainer(); j++ )
		//	m_GridLines.add( lines[j] );
		CGridLineArray lines = m_Grids[i]->getXGridLines();
		for(int j = 1; j <= lines.getItemsInContainer(); j++ )
			m_GridLines.add( lines[j] );
		CGridLineArray ylines = m_Grids[i]->getYGridLines();
		for(int j = 1; j <= ylines.getItemsInContainer(); j++ )
			m_GridLines.add( ylines[j] );
		CGridLineArray zlines = m_Grids[i]->getZGridLines();
		for(int j = 1; j <= zlines.getItemsInContainer(); j++ )
			m_GridLines.add( zlines[j] );
		//now clear and load the display list for this grid
		//CompileDisplayList( i );
	}
}

void  CVAGrid::snapToGrid( CPoint3D& c )
{
	CPoint3D p( c );
	c = closestGridPoint( p );
	return;
}

void CVAGrid::CompileDisplayList( int iGrid, CDC* /*pDC*/, HGLRC /*hGLRC*/ )
{

	m_DisplayLists[iGrid]->StartDef( );

	COLORREF c = ini.color().grid;
	glColor3ub( GetRValue( c ), GetGValue( c ), GetBValue( c ) );
	glPointSize( 3.0f/*float(pointSize)*/ );
	
	glBegin( GL_POINTS );
	CGridPointArray pts = m_Grids[iGrid]->getGridPoints();
	for( int j = 1; j <= pts.getItemsInContainer(); j++ ) {
		glVertex3f( (float)pts[j]->x, (float)pts[j]->y, (float)pts[j]->z );
	}
	glEnd();
	
	//try drawing lines
	glBegin( GL_LINES );
	glColor3f( 0.7f, 0.7f, 0.7f );
	CGridLineArray lines = m_Grids[iGrid]->getGridLines();
	for( int j = 1; j <= lines.getItemsInContainer(); j++ )
	{
		if( lines[j]->length() > 0. )
		{
			const CPoint3D& p1 = lines[j]->point(1);
			const CPoint3D& p2 = lines[j]->point(2);
			glVertex3f( (float)p1.x, (float)p1.y, (float)p1.z );
			glVertex3f( (float)p2.x, (float)p2.y, (float)p2.z );
		}
	}
	glEnd();

	m_DisplayLists[iGrid]->EndDef( );

	//EndGLDrawing( pDC );
}