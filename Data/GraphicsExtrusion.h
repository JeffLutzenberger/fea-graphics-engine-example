///////////////////////////////////////////////////////////////////////////
// GraphicsExtrusion.h      		
// version 1.0                                               05 October 2005
// 
//
// class CGraphicsExtrusion - this class is an extrusion class, it extrudes.
//                            a little more detail: this class does generic
//                            extrusion. It is used to create one extruded shape
//							  along the length of a line defined by the spine 
//							  array. If you need to extrude a member or cable
//							  please use the CMemberExtrusion class below.
//
// Purpose - to calculate and draw an extruded cross-section with tapering if
//           necessary. This class is responsible for calling OpenGL funcitons to draw
//           the extruded shape. It is not necessary to store this object as the opengl
//           commands are passed down the graphics pipeline once the Draw() function
//           gets called. 
//
// Attributes:
//
//				m_spine				set of points defining the line that the extrusion should follow.
//				m_crossSections		set of cross-sections for each extrusion segment.
//				m_crossSectionLocs	set of points where the cross section changes or ends
//				m_rotations			set of doubles defining the torsional rotation of the member
//				m_vertices			the resulting extrusion vertices
//
///////////////////////////////////////////////////////////////////////////
#pragma once

#include "Math/Analytic3D.h"
#include "Math/Matrix44.h"
#include "Math/Quaternion.h"
#include "Legacy/Array-T.h"
#include "Member.h"
#include "Cable.h"
#include "winfiltr.h"
#include "Graphics/GraphicsTesselator.h"
#include <GL/gl.h>
#include "results.h"


class DATA_LINK CGraphicsExtrusion
{
public:
	CGraphicsExtrusion( std::vector<CPoint3D> spine,
						const CPoint3DArrayArray& crossSections,
						const CDoubleArray& crossSectionLocs,
						const CDoubleArray& rotations );
	CGraphicsExtrusion( const CGraphicsExtrusion& ext );

	~CGraphicsExtrusion(void);

	void	Draw( );
	void	DrawSpine( );
	void	UdateData( );
	void	UpdateTexCoords( );
	double	GetYMax( ){ return m_YMax; };

	const float*	GetVerts( int& size );
	const float*	GetNorms( int& size );
	//returns the ordered array of texture coordinates. 
	//note: if these are changed the texture coordinate vertex buffer must be updated
	std::vector<CPoint3D>&	GetOrderedTexCoords( );
	//returns number of points in each cross-section
	int				GetNumPtsPerCrossSection( );
	const CPoint3DArrayArray& GetCrossSections( );

private:
	void generateWallVertices( );
	void generateEndCapVertices( );
	void generateNormals( );
	void DrawEndCaps( );
	void loadVertexArrays( );
	void deleteVertexArrays( );


	CVector3D calculate_x_axis( std::vector<CPoint3D>& spine, int iPos );
	CVector3D calculate_y_axis( std::vector<CPoint3D>& spine, int iPos );
	double interpolate_coordinate( double cross1Loc, double cross2Loc, double pos,
		                           double startLoc, double endLoc );

private:
	std::vector<CPoint3D>		m_spine;
	const CPoint3DArrayArray	m_crossSections;
	const CDoubleArray&			m_rotations;
	const CDoubleArray&			m_crossSectionLocs;
	int							m_nSections;
	double						m_YMax;
	//side walls
	std::vector<bool>			m_edgeFlags;
	std::vector<CPoint3D>		m_vertices;
	std::vector<int>			m_vertexIndex;
	std::vector<CPoint3D>		m_normals;
	std::vector<CPoint3D>		m_texCoords;
	//end caps
	std::vector<CPoint3D>		m_endVertices;
	std::vector<int>			m_endVertexIndex;
	std::vector<CPoint3D>		m_endNormals;

	GLboolean*					m_pEdgeFlags;		// array of edge flags for vertex arrays - see OpenGL Red Book
	float*						m_pVerts;			// array of vertices for vertex arrays
	float*						m_pNorms;			// array of normals for vertex arrays
	float*						m_pTexCoords;		// array of texture coordinates for vertex arrays
	float*						m_pEndVerts;		// array of vertices for endcap vertex arrays
	float*						m_pEndNorms;		// array of normals for endcap vertex arrays
	float*						m_pSpineVerts;		// array of spine points for vertex array
	
	//end cap tesselations and display lists
	CGraphicsTessellator		m_EndCapTess1;
	CGraphicsTessellator		m_EndCapTess2;

public:

	const CGraphicsExtrusion&
	   	operator=( const CGraphicsExtrusion& ext );
};

///////////////////////////////////////////////////////////////////////////////////////
// class CMemberExtrusion - This is the guy to use to draw members. it can handle
//							multiple extrusions for things like double angles and 
//							composite shapes.
//
// Purpose - to calculate and draw an extruded member's cross-section with tapering if
//           necessary. This class is responsible for calling all of the extrusions draw
//           functions.
//
// Attributes:
//
//				m_pM				pointer to the member we're drawing
//				m_spine				a copy of the member spine
//              m_ID				the member's encoded opengl index
//				m_setback			member setback (defined in CWindowFilter)
//
///////////////////////////////////////////////////////////////////////////
//typedef std::vector<CGraphicsExtrusion> ExtrusionArray;
typedef TPointerArray<CGraphicsExtrusion> ExtrusionArray;
class DATA_LINK CMemberExtrusion
{
public:
	CMemberExtrusion( const CMember* pM, std::vector<CPoint3D> spine, double setback, bool bFullDetail );
	CMemberExtrusion( const CCable* pC, std::vector<CPoint3D> spine, bool bFullDetail );
	CMemberExtrusion( const CMember* pM, CMemberExtrusion* pMemExt );
	CMemberExtrusion( const CCable* pC, CMemberExtrusion* pMemExt );
	~CMemberExtrusion();

	void					Draw();
	void					BuildExtrusions( );

	double					GetYMax( );
	void					SetExtrusionTextureCoords( double min, double max, int GraphType, const CResult* pR, bool bDiagram = true );
	//std::vector<double>&	GetSpineTexCoords( ){ return m_spineTexCoord; };

	std::vector<CPoint3D>& spine(){return m_spine;};
	double setback(){return m_setback;};
	//ETGraphicDetail detail(){return m_detail;};
	ExtrusionArray extrusions(){return m_extrusions;};

private:
	// setup cross sections given an array of crossSections, locations of each crossSection and the number of 
	// crossSections - 0 for the first and 1 for the second (double angles)
	void CMemberExtrusion::SetupMemberCrossSections( const CMember* pM, 
											  CPoint3DArrayArray& crossSections, 
											  CDoubleArray& crossSectionLocs,
											  int nCrossSection );
	void CMemberExtrusion::SetupCableCrossSections( const CCable* pC, 
											  CPoint3DArrayArray& crossSections, 
											  CDoubleArray& crossSectionLocs,
											  int nCrossSection );

	ExtrusionArray		m_extrusions;
	const CMember*		m_pM;
	const CCable*		m_pC;

	std::vector<CPoint3D>	m_spine;
	//std::vector<double>		m_spineTexCoord;
	double					m_setback;
	bool					m_bFullDetail;
};
DATA_LINK typedef std::map<CString, CMemberExtrusion*> ExtrusionMap;
DATA_LINK typedef std::vector<CGraphicsExtrusion> MemberExtrusionArray;
class DATA_LINK CExtrusionMap
{
public:
	CExtrusionMap(){};
	~CExtrusionMap(){};

	ExtrusionMap				m_map;

	void						Draw( const CString& name );
	CMemberExtrusion*			get( const CString& name );
	void						set( const CString& name, CMemberExtrusion* ext );
	void						remove( const CString& name );
	void						clear( );
	int							size( );
};

UINT GraphicsExtrusionThread( LPVOID pParam );
