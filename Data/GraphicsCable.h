#pragma once

#include "Cable.h"
#include "Data.h"
#include "rsltcase.h"
#include "Graphics/GraphicsExtrusion.h"

#include <GL/gl.h>

#include "legacy/PointerArray-T.h"
#include "math/analytic3D.h"
#include "GraphicsObjects.h"
#include "WinFiltr.h"

#include "Graphics/GLText.h"

class CGraphicsCable;
class CMemberExtrusion;

// maps this graphics object to the model object
typedef std::map<const CData*, CGraphicsCable*> GraphicsCableMap;
typedef std::map<const CResultCase*, CGraphicsCable*> DisplacedGraphicsCableMap;


class DATA_LINK CGraphicsCable
{
public:
	// the only way a cable should be setup
	CGraphicsCable( const CCable* pC, const CResultCase* pRC = NULL, double scale = 0., bool bSetupExtrusion = true );
	// create a new graphics member but copy pSimilarGraphicsMember's extrusions 
	CGraphicsCable( const CCable* pC, CGraphicsCable* pSimilarGraphicsCable );
	// deletes extrusions
	~CGraphicsCable();
	CGraphicsCable& operator = ( const CGraphicsCable& m );
	// draw it!
	void					Draw( const CWindowFilter& rFilter );
	// draw with text
	void					Draw( CDC* pDC, CWindowFilter& rFilter, CGLText* text );
	// draw text only
	void					DrawText( CDC* pDC, CWindowFilter& rFilter, CGLText* text  );
	// draw the result diagram on the member
	void					DrawResultDiagram( const CWindowFilter& rFilter );
	//get the distortion factor
	double					distortionFactor();
	// set up texture coordinates for result gradients
	void					SetupTextureCoords( double min, double max, int theGraphType, const CResult* pR, bool bDiagram );
	// set up the texture coordinates for low quality drawing
	void					SetSpineTextureCoords( double min, double max, int GraphType, const CResult* pR, bool bDiagram );
	// have threads been setup yet?
	bool					bHaveExtrusions( ){return m_bHaveExtrusions;};
	// called by graphics thread to build extrusions 
	bool					SetupExtrusions( );
	// graphic member's time stamp
	clock_t					TimeCreated( void ) const { return m_timeCreated; };
	// check to see if m_pM's time stamp has changed
	bool					isSynchedWithDataCable();
	// if m_pM has changed update the graphics member
	void					SynchWithDataCable();
	// return the medium and hi quality extrusions
	CMemberExtrusion*		MedLOD(){ return m_MedLOD; };
	CMemberExtrusion*		HiLOD(){ return m_HiLOD; };
	//get the top extent of the cross-section's bounding box
	double					GetYMax();

private:

	// setup the cable spine and set the orientation and start location of the cable element.
	void					SetupOrientation( );
	// creates a new hi detail extrusion
	bool					SetupHiQualityExtrusion( );
	// creates a new medium detail extrusion
	bool					SetupMedQualityExtrusion( );
	// draw the low detail cable - draws the cables spine
	void					DrawCableLow( );
	// draw the medium detail cable
	void					DrawCableMed( );
	// draw the hi detail cable
	void					DrawCableHi( );
	// gets the lowest point in the cable - for text and local axes processing
	double					GetLocalMinY( );
	// for the text labels
	void					GetCablePropertyBuffers( char* buf1, char* buf2, CWindowFilter& filter );
	// copy of the cable
	const CCable*			m_pC;
	// copy of this result case if one exists
	const CResultCase*		m_pRC;
	// hi detail extrusion
	CMemberExtrusion*		m_HiLOD;
	// low detail extrusion
	CMemberExtrusion*		m_MedLOD;
	// an array of points defining the spine
	std::vector<CPoint3D>	m_spine;
	// node 1 location
	CPoint3D				m_Start;
	// deflection scale
	double					m_scale;
	// extrusions setup yet?
	bool					m_bHaveExtrusions;
	// time this guy was created
	clock_t						m_timeCreated;
	
	std::vector<double>		m_spineTexCoords;
};