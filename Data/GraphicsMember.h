#pragma once

#include "Member.h"
#include "Data.h"
#include "rsltcase.h"
#include "GraphicsExtrusion.h"
#include "GraphicsObjects.h"

#include <GL/gl.h>

#include "legacy/PointerArray-T.h"
#include "math/analytic3D.h"
#include "GraphicsObjects.h"
#include "WinFiltr.h"

#include "Graphics/GLOutlineText.h"

class CGraphicsMember;
class CMemberExtrusion;

// maps this graphics object to the model object
typedef std::map<const CData*, CGraphicsMember*> GraphicsMemberMap;
typedef std::map<const CResultCase*, CGraphicsMember*> DisplacedGraphicsMemberMap;

class DATA_LINK CGraphicsMember
{
public:
	// create a brand new graphics member
	CGraphicsMember( const CMember* pM, const CResultCase* pRC = NULL, double scale = 0., bool bSetupExtrusion = true );
	// create a new graphics member but copy pSimilarGraphicsMember's extrusions 
	CGraphicsMember( const CMember* pM, CGraphicsMember* pSimilarGraphicsMember );
	// deletes extrusions
	~CGraphicsMember();
	CGraphicsMember& operator = ( const CGraphicsMember& m );
	// draw it!
	void					Draw( const CWindowFilter& rFilter, int nDivPts );
	// draw with text
	void					Draw( CDC* pDC, CWindowFilter& rFilter, int nDivPts, CGLText* text, double displacementScale );
	// draw just the text
	void					DrawMemberText( CDC* pDC, CWindowFilter& rFilter, CGLText* text, double displacementScale );
	// draw the result diagram on the member
	void					DrawResultDiagram( CWindowFilter& rFilter );
	// draw the selection box around this member
	void					DrawSelectionBox();
	//get the distortion factor
	double					distortionFactor();
	// set up texture coordinates for result gradients
	void					SetupTextureCoords( double min, double max, int theGraphType, const CResult* pR, bool bDiagram );
	// have threads been setup yet?
	bool					bHaveExtrusions( ){return m_bHaveExtrusions;};
	// called by graphics thread to build extrusions 
	bool					SetupExtrusions( );
	// graphic member's time stamp
	clock_t					TimeCreated( void ) const { return m_timeCreated; };
	// check to see if m_pM's time stamp has changed
	bool					isSynchedWithDataMember();
	// if m_pM has changed update the graphics member
	void					SynchWithDataMember();
	// return the medium and hi quality extrusions
	CMemberExtrusion*		MedLOD(){ return m_MedLOD; };
	CMemberExtrusion*		HiLOD(){ return m_HiLOD; };
	//get the top extent of the cross-section's bounding box
	double					GetYMax();
	// this function should translate and rotate the cross-section of the extruded shape into the proper orientation
	// I have no idea if it works! It was too easy to code up so it probably doesn't! JL 10/2/2008
	void					GetTranslatedAndRotatedCrossSection( CPoint3DArray& crossSection );

	
private:

	// setup the member spine and set the orientation and start location of the member element.
	void					SetupSpine( );
	// creates a new hi detail extrusion
	bool					SetupHiQualityExtrusion( );
	// creates a new medium detail extrusion
	bool					SetupMedQualityExtrusion( );
	// set up the texture coordinates for low quality drawing
	void					SetSpineTextureCoords( double min, double max, int GraphType, const CResult* pR, bool bDiagram );
	// draw the low detail member - draws the member's spine
	void					DrawMemberLow( );
	// draw the medium detail member
	void					DrawMemberMed( );
	// draw the hi detail member
	void					DrawMemberHi( );
	// draw division points for snapping 
	void					DrawDivPts( int nDivPts );
	//for our text labels
	void					GetMemberPropertyBuffers( char* buf1, char* buf2, CWindowFilter& rFilter );
	// copy of the member element
	const CMember*			m_pM;
	// copy of the result case if one exists
	const CResultCase*		m_pRC;
	// the hi detail extrusion
	CMemberExtrusion*		m_HiLOD;
	// the medium detail extrusion
	CMemberExtrusion*		m_MedLOD;
	// an array of points defining the spine
	std::vector<CPoint3D>	m_spine;
	// node 1 location
	CPoint3D				m_Start;
	// rotation axis for member orientation
	CVector3D				m_RotationAxis;
	// rotation angle for rotation axis
	double					m_RotationAngle;
	// rotation matrix
	float					m_RotationMatrix[16];
	// deflection scale
	double					m_scale;
	// extrusions setup yet?
	bool					m_bHaveExtrusions;
	// time this guy was created
	clock_t					m_timeCreated;
	// our texture coords for the low quality mode
	std::vector<double>		m_spineTexCoords;
	//direction of the load for diagram drawing...
	ETDirection				m_loadDir;
	
};

