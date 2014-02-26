///////////////////////////////////////////////////////////////////////////
// GraphicsGrid.h      		
// version 1.0                                               26 August 2005
// 
//
// class CVAGrid - this is our grid manager. It manages a list of CGraphicGrid
//                 objects.
//
///////////////////////////////////////////////////////////////////////////
#if !defined( GRAPHICSGRID_H )
#define GRAPHICSGRID_H

#include "Graphics/GraphicGrid.h"

#include "Legacy\PointerArray-T.h"
#include "Core/Graphics/GraphicsDisplayList.h"

typedef TPointerArray<CGraphicGrid> CGridArray;
typedef TPointerArray<CPoint3D> CGridPointArray;
typedef TPointerArray<CGraphicsDisplayList> CDisplayListArray;

class CVAGrid {
public:
	// Constructors and Destructors
	CVAGrid();
	~CVAGrid();

	// UI Settings
	bool isActive( void ) { return true; }
	bool isSnap( void ) { return true; }

	// manipulation functions
	void addGrid( CGraphicGrid& pGrid );
	void removeGrid( int iGrid );
	void modifyGrid( int iGrid, CGraphicGrid& pGrid );
    CGraphicGrid* grid( int iGrid ) const;
	int gridCount( void ) const { return m_Grids.getItemsInContainer(); };

	// show in OPENGL window
	void drawGrid( double pointSize );

	// calculation functions
	CPoint3D closestGridPoint( const CPoint3D& pt, double* dist = NULL );
	const CGridPointArray& getGridPoints( void ) const;
	const CGridLineArray& getGridLines( void ) const;
	const CDisplayListArray& getDisplayLists( void ) const;
	void snapToGrid( CPoint3D& c );

protected:
	void UpdateGridData( void );
	void CompileDisplayList( int iGrid, CDC* pDC, HGLRC hGLRC );

private:
	CGridArray	m_Grids;   // we own these guys
	CGridPointArray m_GridPoints; // each grid owns the points, this is a copy
	CGridLineArray m_GridLines;
	CDisplayListArray m_DisplayLists;
};  // end class CVAGrid


#endif   // sentry