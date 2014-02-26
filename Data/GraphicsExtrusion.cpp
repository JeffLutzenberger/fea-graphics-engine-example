#include "IESDataStdAfx.h"
#include "Core/Graphics/GraphicsHelpers.h"
#include "GraphicsExtrusion.h"
#include "Core/Graphics/GraphicsTesselator.h"
#include "ShapeDB/CrossSection.h"
#include <GL/gl.h>
#include "datautil.h"
#include "iniGraphics.h"

#include "Project.h"

#ifdef _DEBUG
	#undef THIS_FILE
	static char THIS_FILE[]=__FILE__;
	#define new DEBUG_NEW
#endif

	CGraphicsExtrusion::CGraphicsExtrusion( std::vector<CPoint3D> spine, 
									    const CPoint3DArrayArray& crossSections,
										const CDoubleArray& crossSectionLocs,
									    const CDoubleArray& rotations ):
m_spine( spine ),
m_crossSections( crossSections ),
m_crossSectionLocs( crossSectionLocs ),
m_rotations( rotations ),
m_nSections( spine.size() ),
m_YMax( -1.e6 ),
m_pEdgeFlags( NULL ),
m_pVerts( NULL ),
m_pNorms( NULL ),
m_pTexCoords( NULL ),
m_pEndVerts( NULL ),
m_pEndNorms( NULL ),
m_pSpineVerts( NULL ),
////CTArrays
m_vertices(),
m_vertexIndex(/*1,1000,200*/),
m_normals(/*1, 1000, 200*/),
m_texCoords(/*1,1000, 200*/),
//end caps
m_endVertices(/*1,100,100*/),
m_endVertexIndex(/*1,100,100*/),
m_endNormals(/*1,100,100*/)
{
	generateWallVertices();
	//generateEndCapVertices( );
	generateNormals();
	loadVertexArrays();
}

CGraphicsExtrusion::CGraphicsExtrusion( const CGraphicsExtrusion& ext ) :
m_spine( ext.m_spine ),
m_normals( ext.m_normals ),
m_crossSections( ext.m_crossSections ),
m_rotations( ext.m_rotations ),
m_crossSectionLocs( ext.m_crossSectionLocs ),
m_vertices( ext.m_vertices ),
m_edgeFlags( ext.m_edgeFlags ),
m_texCoords( ext.m_texCoords ),
m_nSections( ext.m_nSections ),
m_YMax( ext.m_YMax ),
m_vertexIndex( ext.m_vertexIndex ),
m_endVertices( ext.m_endVertices ),
m_endVertexIndex( ext.m_endVertexIndex ),
m_endNormals( ext.m_endNormals ),
m_pEdgeFlags( NULL ),
m_pVerts( NULL ),
m_pNorms( NULL ),
m_pTexCoords( NULL ),
m_pEndVerts( NULL ),
m_pEndNorms( NULL ),
m_pSpineVerts( NULL )
{
	loadVertexArrays();
}

CGraphicsExtrusion::~CGraphicsExtrusion(void)
{
	deleteVertexArrays( );
	//m_endVertexIndex.flush();
	//m_endVertices.flush();
	//m_vertices.flush();
	//m_endNormals.flush();
	//m_normals.flush();
}

const float* CGraphicsExtrusion::GetVerts( int& size )
{
	size = m_vertexIndex.size();
	if( m_pVerts )
		return (const float*)m_pVerts;
	else return NULL;
}

const float* CGraphicsExtrusion::GetNorms( int& size )
{
	size = m_vertexIndex.size();
	if( m_pNorms )
		return (const float*)m_pNorms;
	else return NULL;
}

std::vector<CPoint3D>& CGraphicsExtrusion::GetOrderedTexCoords( )
{
	return m_texCoords;
}

const CPoint3DArrayArray& CGraphicsExtrusion::GetCrossSections()
{
	return m_crossSections;
}

int CGraphicsExtrusion::GetNumPtsPerCrossSection( )
{
	return m_crossSections[1].getItemsInContainer();
}

void CGraphicsExtrusion::Draw()
{
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	//glEnableClientState( GL_EDGE_FLAG_ARRAY );

	glVertexPointer( 3, GL_FLOAT, 0, m_pVerts );
	glNormalPointer( GL_FLOAT, 0, m_pNorms );
	glTexCoordPointer( 2, GL_FLOAT, 0, m_pTexCoords );
	//glEdgeFlagPointer( 0, m_pEdgeFlags );

	glDrawArrays( GL_QUADS, 0, m_vertexIndex.size() );

	//glDisableClientState( GL_EDGE_FLAG_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_VERTEX_ARRAY );

	//DrawEndCaps();
	//if( !g_bDrawWireframe && ini.graphics().graphicsLevel > 2 )
	//{
	//	glEnableClientState( GL_VERTEX_ARRAY );
	//	//glEnableClientState( GL_NORMAL_ARRAY );

	//	glVertexPointer( 3, GL_FLOAT, 0, m_pEndVerts );
	//	// number of end normals not getting set properly
	//	//glNormalPointer( GL_FLOAT, 0, m_pEndNorms );

	//	glDrawArrays( GL_TRIANGLES, 0, m_endVertexIndex.size() );

	//	//glDisableClientState( GL_NORMAL_ARRAY );
	//	glDisableClientState( GL_VERTEX_ARRAY );
	//}
}

void CGraphicsExtrusion::DrawSpine( )
{
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, m_pVerts );
	glDrawArrays( GL_LINES, 0, m_spine.size() );
	glDisableClientState( GL_VERTEX_ARRAY );
}

/**********************************************************
* Function Name: generateWallVertices( )
* 
* Description: Generates the vertices for the walls of an extrusion. 
*              Extrusion walls are formed from a series of QUADS built
*              around a spine defining the path of the extrusion
*
***********************************************************/
void CGraphicsExtrusion::generateWallVertices( )
{
	m_edgeFlags.clear();
	//m_vertices.flush();
	//m_vertexIndex.flush();
	//m_texCoords.flush();
	
	if( m_spine.size() == 0 || m_crossSections.getItemsInContainer() == 0)
	  return;

	ASSERT( (int)m_crossSections.getItemsInContainer() == m_crossSectionLocs.getItemsInContainer() );

	//setup cross section coordinates. We taper the extrusion if there is more than 1 cross section
	// in crossSections. Tapering is done by linear interpolation between the two cross sections at 
	// the locations defined by crossSectionLocs
	int nCrossSections = m_crossSections.getItemsInContainer();
	ASSERT( nCrossSections > 0 );
	int nSpinePts = m_spine.size();
	// calculate the length of the spine and cross1 section. Needed for
	// texture coordinates.
	double spinelen = 0.0f;
	double crosslen = 0.0f;

	for (int i = 0; i < nSpinePts-1; i++) 
	{
		spinelen += m_spine[i+1].distance_to( m_spine[i] );
	}
	if ( spinelen == 0.0f ) spinelen = 1.0f;

	int nCrossPts = m_crossSections[1].getItemsInContainer();
	ASSERT( nCrossPts > 0 );
	CPoint3DArray cross1 = m_crossSections[1];
	CPoint3DArray cross2 = m_crossSections[1];

	const CPoint3D cross1p1 = cross1[1];
	const CPoint3D cross1p2 = cross1[nCrossPts];
	bool bClosed = false;   // is cross1 section closed
	if ( zero( cross1p1.distance_to( cross1p2 ) ) ) 
	{
		bClosed = true;
	}
	    
	for (int i = 1; i < nCrossPts; i++) 
	{
		crosslen += cross1[i+1].distance_to( cross1[i] );
	}
	if ( crosslen == 0.0f ) crosslen = 1.0f;

	CVector3D prevX( 0., 0., 0. );
	CVector3D prevY( 0., 0., 0. );

	const CVector3D empty( 0.0, 0.0, 0.0 );
	bool colinear = false;
	CVector3D X, Y, Z;

	// find first non-collinear spine segments and calculate the first
	// valid X and Y axis
	for (int i = 1; i <= nSpinePts && (prevX == empty || prevY == empty); i++) 
	{
		if (prevX.is_zero() ) 
		{
			X = calculate_x_axis( m_spine, i );
			if (!X.is_zero()) prevX = X;
		}
		if (prevY.is_zero()) 
		{
			Y = calculate_y_axis( m_spine, i );
			if (!Y.is_zero()) prevY = Y;
		}
	}

	if (prevX.is_zero()) 
		prevX = CVector3D(1.0f, 0.0f, 0.0f);
	if (prevY.is_zero()) 
	{ 
		// all spine segments are colinear, calculate constant Y axis
		prevY = CVector3D(0.0f, 1.0f, 0.0f);
		if (prevX != CVector3D(1.0f, 0.0f, 0.0f)) 
		{
			//JL Note 12/2/2009 - this can happen for cables since the spine points
			// are in the global coordinate system. 
			//TRACE( "GraphicsExtrusion - Verify this function\n" );
			ETCoordinate upAxis = theProject.designCriteria().getVerticalDirection();
			if( (int)upAxis == 2 /*Y*/  )
				prevY = CVector3D( 0, 1, 0 );
			else if( (int)upAxis == 1 /*X*/ )
				prevY = CVector3D( 1, 0, 0 );
			else if( (int)upAxis == 3 /*Z*/ )
				prevY = CVector3D( 0, 0, 1 );
			if( prevX == prevY )
			{	
				ASSERT( false );
				Z = CVector3D( 0., 0., 1. );
				prevY = Z.cross( prevX );
			}
		}
		colinear = true;
	}

	double xLoc = 0.0f; // for texcoords

	// loop through all spine points
	int cur = 1;
	int next = 2;
	for (int i = 1; i <= nSpinePts; i++) 
	{
		if (colinear) 
		{
			X = prevX;
			Y = prevY;
		}
		else 
		{
			X = calculate_x_axis( m_spine, i );
			if (X.is_zero() /*== empty*/) X = prevX;
			Y = calculate_y_axis( m_spine, i );
			if (Y.is_zero() /*== empty*/) Y = prevY;
			if (Y.dot(prevY) < 0) Y = -Y;
		}

		X.normalize();
		Y.normalize();
		Z = X.cross(Y);
		Z.normalize();

		prevX = X;
		prevY = Y;

		//setup the transformation matrix for this spine point
		CMatrix44 matrix;

		matrix.m[0][0] = X.x;
		matrix.m[1][0] = X.y;
		matrix.m[2][0] = X.z;
		matrix.m[3][0] = 0.0f;

		matrix.m[0][1] = Y.x;
		matrix.m[1][1] = Y.y;
		matrix.m[2][1] = Y.z;
		matrix.m[3][1] = 0.0f;

		matrix.m[0][2] = Z.x;
		matrix.m[1][2] = Z.y;
		matrix.m[2][2] = Z.z;
		matrix.m[3][2] = 0.0f;

		matrix.m[0][3] = m_spine[i-1].x;
		matrix.m[1][3] = m_spine[i-1].y;
		matrix.m[2][3] = m_spine[i-1].z;

		CMatrix44 mat2;
		
		if( xLoc >= m_crossSectionLocs[next] && next+1 <= m_crossSectionLocs.getItemsInContainer() )
		{
			cur++;
			next++;
		}
		
		double len1 = m_crossSectionLocs[ cur ];
		double len2 = m_crossSectionLocs[ next ];

		cross1 = m_crossSections[ cur ];
		cross2 = m_crossSections[ next ];

		////now loop on cross section points
		double currentcrosslen = 0.0f; 
		for ( int j = 1; j <= nCrossPts; j++) 
		{
			double y1 = cross1[j].y;//[0];
			double y2 = cross2[j].y;//[0];
			double z1 = cross1[j].z;//[1];
			double z2 = cross2[j].z;//[1];

			//get the maximum y value for the first cross-section
			if( j == 1 && y1 > m_YMax )
			{
				if( zero(y1) )
					m_YMax = z1;
				else
					m_YMax = y1;
			}

			CVector3D v;
			v.x = 0.;
			v.y = interpolate_coordinate( len1, len2, xLoc, y1, y2 );
			v.z = interpolate_coordinate( len1, len2, xLoc, z1, z2 );
			v.w = 1.;

			CVector3D vtemp;
			matrix.RightMultiply4( v, vtemp );

			//mat2.RightMultiply4( v, vtemp );

			//m_vertices.add( CPoint3D( vtemp.x, vtemp.y, vtemp.z ) );
			m_vertices.push_back( CPoint3D( vtemp.x, vtemp.y, vtemp.z ) );
			
			// edge flag trickery follows - basically we want to ensure that the edges (caps) of the extrusion are 
			// always drawn when in wireframe mode. So we preset some of the edge flags here and then set the rest
			// below in the loadVertexArrays() function
			if( i == 1 || i == nSpinePts ) //left or right end
				m_edgeFlags.push_back( GL_TRUE );
			else 
				m_edgeFlags.push_back( GL_FALSE );

			//texture coordinates
			//CPoint3D tc( currentcrosslen / crosslen, 0.0, 0.0 );
			CPoint3D tc( 0., 1.0, 0.0 );
			m_texCoords.push_back( tc );
			if( j < nCrossPts ) 
				currentcrosslen += cross1[j].distance_to( cross1[j+1] );
		}
		if (i < nSpinePts) 
			//xLoc += m_spine[i+1].x - m_spine[i].x;
			xLoc += m_spine[i-1].distance_to( m_spine[i] );
	} // for spine points

#define ADD_QUAD(i0, j0, i1, j1, i2, j2, i3, j3)   \
  do { \
    m_vertexIndex.push_back((i0)*nCrossPts+(j0)); \
    m_vertexIndex.push_back((i3)*nCrossPts+(j3)); \
    m_vertexIndex.push_back((i2)*nCrossPts+(j2)); \
    m_vertexIndex.push_back((i1)*nCrossPts+(j1)); \
  } while (0)

	// create walls
	for (int i = 0; i < nSpinePts-1; i++)
		for (int j = 0; j < nCrossPts-1; j++)
			ADD_QUAD(i, j, i+1, j, i+1, j+1, i, j+1);

#undef ADD_QUAD

}

void CGraphicsExtrusion::generateNormals( )
{
	//assume quads for side walls
	//m_normals.flush();
	m_normals.clear();
	int inc = 4;
	for( unsigned int i = 0; i < m_vertexIndex.size(); i+=inc )
	{
		CPoint3D p1 = m_vertices[m_vertexIndex[i]];
		CPoint3D p2 = m_vertices[m_vertexIndex[i+1]];
		CPoint3D p3 = m_vertices[m_vertexIndex[i+2]];
		CVector3D v1 = CVector3D( p2, p1 );
		CVector3D v2 = CVector3D( p3, p1 );
		CVector3D nv( 1., 0., 0. );
		if( !zero( v1.length() ) && !zero( v2.length() ) )
			nv = v1.cross( v2 ).normalize();
		m_normals.push_back( CPoint3D( nv.x, nv.y, nv.z ) );
		m_normals.push_back( CPoint3D( nv.x, nv.y, nv.z ) );
		m_normals.push_back( CPoint3D( nv.x, nv.y, nv.z ) );
		m_normals.push_back( CPoint3D( nv.x, nv.y, nv.z ) );
	}
	//assume triangles for end caps
	m_endNormals.clear();
	inc = 3;
	for( unsigned int i = 0; i < m_endVertexIndex.size(); i++ )
	{
		//CPoint3D p1 = m_endVertices[i];
		//CPoint3D p2 = m_endVertices[i+1];
		//CPoint3D p3 = m_endVertices[i+2];
		//CVector3D v1 = CVector3D( p2, p1 );
		//CVector3D v2 = CVector3D( p3, p1 );
		//CVector3D nv = v1.cross( v2 ).normalize();
		//m_endNormals.add( CPoint3D( nv.x, nv.y, nv.z ) );
		//m_endNormals.add( CPoint3D( nv.x, nv.y, nv.z ) );
		//m_endNormals.add( CPoint3D( nv.x, nv.y, nv.z ) );
		m_endNormals.push_back( CPoint3D( 1., 0., 0. ) );
		m_endNormals.push_back( CPoint3D( 1., 0., 0. ) );
		m_endNormals.push_back( CPoint3D( 1., 0., 0. ) );
	}
}

/********************************************************************
*
* Function loadVertexArrays( )
*
* Description: Loads the vertices and normals for each quad in the extrusion
*              into vertex buffers for use in OpenGL. We also store the edge flag 
*              for each vertex to tell OpenGL whether or not to draw the edge 
*              when drawing in line mode.
**********************************************************************/
void CGraphicsExtrusion::loadVertexArrays( )
{
	if( m_vertexIndex.size() <= 0 )
		return;
	int count = 0;
	int tex_count = 0;
	m_pEdgeFlags = new GLboolean[m_vertexIndex.size()];
	m_pVerts = new float[m_vertexIndex.size()*3];
	m_pNorms = new float[m_normals.size()*3];
	m_pTexCoords = new float[m_normals.size()*2];
	m_pSpineVerts = new float[m_spine.size()*3];

	for( unsigned int i = 0; i < m_spine.size(); i++ )
	{
		m_pSpineVerts[count]   = (float)m_spine[i].x;
		m_pSpineVerts[count+1] = (float)m_spine[i].y;
		m_pSpineVerts[count+2] = (float)m_spine[i].z;
	}

	count = 0;
	for( unsigned int i = 0; i < m_vertexIndex.size(); i++ )
	{
		//we have already set the end cap edge flags to true so we make sure not to override those here.
		//any other even edge flag on the quad should also be set to true.
		if( (i & 1) || m_edgeFlags[m_vertexIndex[i]] == GL_TRUE ) //even or already true
			m_pEdgeFlags[i] = GL_TRUE;
		else //odd and not already true
			m_pEdgeFlags[i] = m_edgeFlags[m_vertexIndex[i]];
		//vertices and normals
		m_pVerts[count] = (float)m_vertices[m_vertexIndex[i]].x;
		m_pVerts[count+1] = (float)m_vertices[m_vertexIndex[i]].y;
		m_pVerts[count+2] = (float)m_vertices[m_vertexIndex[i]].z;
		m_pNorms[count] = (float)m_normals[i].x;
		m_pNorms[count+1] = (float)m_normals[i].y;
		m_pNorms[count+2] = (float)m_normals[i].z;
		count += 3;
		//texture coordinates
		m_pTexCoords[tex_count] = (float)m_texCoords[m_vertexIndex[i]].x;
		m_pTexCoords[tex_count+1] = (float)m_texCoords[m_vertexIndex[i]].y;
		tex_count += 2;
	}
	if( m_endVertexIndex.size() <= 0 )
		return;
	count = 0;
	m_pEndVerts = new float[m_endVertices.size()*3];
	m_pEndNorms = new float[m_endNormals.size()*3];
	for( unsigned int i = 0; i < m_endVertices.size(); i++ )
	{
		m_pEndVerts[count] = (float)m_endVertices[i].x;
		m_pEndVerts[count+1] = (float)m_endVertices[i].y;
		m_pEndVerts[count+2] = (float)m_endVertices[i].z;
		m_pEndNorms[count] = (float)m_endNormals[i].x;
		m_pEndNorms[count+1] = (float)m_endNormals[i].y;
		m_pEndNorms[count+2] = (float)m_endNormals[i].z;
		count += 3;
	}
}

void CGraphicsExtrusion::UpdateTexCoords( )
{
	//m_pTexCoords = new float[m_normals.getItemsInContainer()*2];

	if( !m_pTexCoords )
		m_pTexCoords = new float[m_normals.size()*2];
		
	int tex_count = 0;
	for( unsigned int i = 0; i < m_vertexIndex.size(); i++ )
	{
		//texture coordinates
		m_pTexCoords[tex_count] = (float)m_texCoords[m_vertexIndex[i]].x;
		m_pTexCoords[tex_count+1] = (float)m_texCoords[m_vertexIndex[i]].y;
		tex_count += 2;
	}
}

void CGraphicsExtrusion::generateEndCapVertices( )
{
	
	//first endcap
	int nCrossPts = m_crossSections[1].getItemsInContainer();
	double (*v)[3] = new double[nCrossPts+1][3];
	m_EndCapTess1.SetWindingRule(GLU_TESS_WINDING_ODD);
	m_EndCapTess1.SetFilling( TRUE );
	//m_EndCap1.StartDef(); //display list
	m_EndCapTess1.StartDef();
	CPoint3D p = m_spine[1] - m_spine[0];
	CVector3D norm = CVector3D( p.x, p.y, p.z ).normalize();
	//glNormal3f( (float)norm.x, (float)norm.y, (float)norm.z ); 
	//start adding endcap vertices to tessellator
	for( int i = 1; i <= nCrossPts; i++ ) 
	{
		v[i-1][0] = m_vertices[i-1].x;
		v[i-1][1] = m_vertices[i-1].y;
		v[i-1][2] = m_vertices[i-1].z;
		m_EndCapTess1.AddVertex( v[i-1] );
	}
	m_EndCapTess1.EndDef( );
	//m_EndCap1.EndDef();
	int count = 1;
	for( unsigned int i = 0; i < m_EndCapTess1.GetVerts().size(); i++ )
	{
		m_endVertices.push_back( m_EndCapTess1.GetVerts()[i] );
		m_endVertexIndex.push_back( count );
		count += 1;
	}
	
	//second endcap
	m_EndCapTess2.SetWindingRule(GLU_TESS_WINDING_ODD);
	m_EndCapTess2.SetFilling( TRUE );
	//m_EndCap2.StartDef();//display list
	m_EndCapTess2.StartDef();
	int nSpinePts = m_spine.size();
	p = m_spine[nSpinePts-1] - m_spine[nSpinePts-2];
	norm = CVector3D( p.x, p.y, p.z ).normalize();
	glVertex3f( (float)norm.x, (float)norm.y, (float)norm.z ); 
	int iEndEndCap = m_vertices.size();
	int iStartEndCap = iEndEndCap - nCrossPts;
	int j = 0;
	for( int i = iStartEndCap+1; i <= iEndEndCap; i++ ) 
	{
		v[j][0] = m_vertices[i-1].x;
		v[j][1] = m_vertices[i-1].y;
		v[j][2] = m_vertices[i-1].z;
		m_EndCapTess2.AddVertex( v[j] );
		j++;
	}
	m_EndCapTess2.EndDef();
	//m_EndCap2.EndDef();
	//m_EndCap1.EndDef();
	for( unsigned int i = 0; i < m_EndCapTess2.GetVerts().size(); i++ )
	{
		m_endVertices.push_back( m_EndCapTess2.GetVerts()[i] );
		m_endVertexIndex.push_back( count );
		count += 1;
	}

	delete[] v;
}
void CGraphicsExtrusion::deleteVertexArrays( )
{
	if( m_pEdgeFlags )
		delete[] m_pEdgeFlags;
	if( m_pVerts )
		delete[] m_pVerts;
	if( m_pNorms )
		delete[] m_pNorms;
	if( m_pTexCoords )
		delete[] m_pTexCoords;
	if( m_pEndVerts )
		delete[] m_pEndVerts;
	if( m_pEndNorms )
		delete[] m_pEndNorms;
	if( m_pSpineVerts )
		delete[] m_pSpineVerts;
}

CVector3D CGraphicsExtrusion::calculate_x_axis( std::vector<CPoint3D>& spine, int iPos )
{
	// doh! 1-based arrays
	CVector3D X;
	int numspine = spine.size();
	if (iPos == 1) X = CVector3D( spine[1], spine[0] );
	else if (iPos == numspine) X = CVector3D( spine[numspine-1], spine[numspine-2] );
	else X = CVector3D( spine[iPos], spine[iPos-1] );
	X.normalize();
	return X;
}

CVector3D CGraphicsExtrusion::calculate_y_axis( std::vector<CPoint3D>& spine, int iPos )
{
	CVector3D Z( 0.0, 0.0, 1.0 );
	CVector3D X( 1.0, 0.0, 0.0 );

	int numspine = spine.size( );
	if( iPos == numspine )
		X = CVector3D( spine[numspine-1], spine[numspine-2] );
	else
		X = CVector3D( spine[iPos], spine[iPos-1] );
	X.normalize( );
	return Z.cross( X ).normalize();
}

double CGraphicsExtrusion::interpolate_coordinate( double cross1Loc, double cross2Loc, double pos,
		                                           double startLoc, double endLoc )
{
	if( pos < cross1Loc )		
		return startLoc;
	if( pos > cross2Loc )
		return endLoc;

	double dx = cross2Loc - cross1Loc; //distance between cross sections
	double dy = endLoc - startLoc; //distance between start coordinate and end coordinate
	double local_pos = pos - cross1Loc; //local pos wrt distance between cross sections
	if( dx == 0.f )
		return startLoc;
	double d_coord = dy/dx*local_pos;

	return  startLoc + d_coord;

}

//operators
const CGraphicsExtrusion& CGraphicsExtrusion::
operator = ( const CGraphicsExtrusion& ext )
{
	ASSERT( false );
    if ( this != &ext )
    {
		/*m_spine = ext.m_spine;
		m_crossSections = ext.m_crossSections;
		m_rotations = ext.m_rotations;
		m_crossSectionLocs = ext.m_crossSectionLocs;
		m_vertices = ext.m_vertices;
		m_nSections = ext.m_nSections;
		m_vertexIndex = ext.m_vertexIndex;
		m_normals = ext.m_normals;*/
    }
    return *this;
}

///////////////////////////////////////////////////////////////////
//
// CMemberExtrusion implementation
//
///////////////////////////////////////////////////////////////////
CMemberExtrusion::CMemberExtrusion( const CMember* pM, std::vector<CPoint3D> spine, double setback, bool bFullDetail ) :
m_pM( pM ),
m_pC( NULL ),
m_spine( spine ),
m_setback( setback ),
m_bFullDetail( bFullDetail )
{
	//for( int i = 1; i <= spine.getItemsInContainer(); i++ )
	//	m_spine.push_back( spine[i] );
	//AfxBeginThread( GraphicsExtrusionThread, this );
	BuildExtrusions( );
}

CMemberExtrusion::CMemberExtrusion( const CCable* pC, std::vector<CPoint3D> spine, bool bFullDetail ) :
m_pM( NULL ),
m_pC( pC ),
m_spine( spine ),
m_setback( 0. ),
m_bFullDetail( bFullDetail )
{
	//for( int i = 1; i <= spine.getItemsInContainer(); i++ )
	//	m_spine.push_back( spine[i] );
	//AfxBeginThread( GraphicsExtrusionThread, this );
	BuildExtrusions( );
}

CMemberExtrusion::CMemberExtrusion( const CMember* pM, CMemberExtrusion* pMemExt ) :
m_pM( pM ),
m_pC( NULL ),
m_spine( ),
m_setback(  ),
m_bFullDetail( true )
{
	if( pMemExt )
		for( int i = 1; i <= pMemExt->extrusions().getItemsInContainer(); i++ )
			m_extrusions.add( new CGraphicsExtrusion( *pMemExt->extrusions()[i] ) );

}

CMemberExtrusion::CMemberExtrusion( const CCable* pC, CMemberExtrusion* pMemExt ) :
m_pM( NULL ),
m_pC( pC ),
m_spine( ),
m_setback(  ),
m_bFullDetail( true )
{
	if( pMemExt )
		for( int i = 1; i <= pMemExt->extrusions().getItemsInContainer(); i++ )
			m_extrusions.add( new CGraphicsExtrusion( *pMemExt->extrusions()[i] ) );

}

CMemberExtrusion::~CMemberExtrusion()
{
	m_extrusions.flush( YES_DELETE );
	//m_spineTexCoord.flush();
}

double CMemberExtrusion::GetYMax()
{
	if( m_extrusions.getItemsInContainer() > 0 )
		return m_extrusions[1]->GetYMax();
	else
		return 0.f;
}
void CMemberExtrusion::Draw()
{
	if( m_extrusions.getItemsInContainer() <= 0 )
		BuildExtrusions();
	for( int i = 1; i <= m_extrusions.getItemsInContainer(); i++ )
	{
		m_extrusions[i]->Draw();
	}
}

void CMemberExtrusion::BuildExtrusions( )
{
	CPoint3DArrayArray xsecs;
	CDoubleArray crossLocs;
	CDoubleArray rotations;//not implemented - torsional rotation
	int nsections = 1;
	if( m_pM )
	{
		SetupMemberCrossSections( m_pM, xsecs, crossLocs, 0 );
		CCrossSection sec( m_pM );
		nsections = sec.NumCrossSections();
	}
	else if( m_pC )
	{
		SetupCableCrossSections( m_pC, xsecs, crossLocs, 0 );
		CCrossSection sec( m_pC );
		nsections = sec.NumCrossSections();
	}
	else
		ASSERT( false );
	CGraphicsExtrusion* ext1 = new CGraphicsExtrusion( m_spine, xsecs, crossLocs, rotations );
	ASSERT_RETURN( ext1 );
	m_extrusions.add( ext1 );
	//double angles - pesky little bastards
	if( nsections == 2 )
	{
		xsecs.flush();
		crossLocs.flush();
		rotations.flush();
		if( m_pM )
			SetupMemberCrossSections( m_pM, xsecs, crossLocs, 1 );
		else if( m_pC )
			SetupCableCrossSections( m_pC, xsecs, crossLocs, 1 );
		else
			ASSERT( false );
		CGraphicsExtrusion* ext2 = new CGraphicsExtrusion( m_spine, xsecs, crossLocs, rotations );
		ASSERT_RETURN( ext2 );
		m_extrusions.add( ext2 );
	}
}

void CMemberExtrusion::SetupMemberCrossSections( const CMember* pM, 
											  CPoint3DArrayArray& crossSections, 
											  CDoubleArray& crossSectionLocs,
											  int nCrossSection )
{
	ASSERT_RETURN( pM );

	///////////////////////////////////////////////////////////////////////////////
	// SetupMemberCrossSections
	//
	// This function sets up the cross-sections for a given member extrusion.
	// Extrusions must have at least two cross sections (beginning and end) 
	// and an array specifying the location of each cross-section. An extrusion 
	// can have as many cross sections as needed. For the tapered members we do  
	// tapering by specifying the cross sections at each taper point and let the  
	// extrusion class linearly interpolate polygon vertices between each cross section. 
	//
	// Note: The CCrossSection class is responsible for determining the appropriate  
	// cross-section based on the taper type.
	//
	// JL 04/05/2005
	///////////////////////////////////////////////////////////////////////////////
	double start_taper = pM->taperStartOffset();
	if( start_taper < m_setback )
		start_taper = m_setback;
	double end_taper = pM->taperEndOffset();
	if( end_taper <= 0. || end_taper <= start_taper ) {
		if( pM->taperType() == DOUBLE_TAPER  ||  pM->taperType() == DOUBLE_TOP_TAPER )
			end_taper = pM->length()/2.;
		else
			end_taper = pM->length() - m_setback;
	}

	//JL Note 3/23/2009 - dealt with in the CGraphicsModel
//#pragma message(JL "Jeff - drawing a rigid link here?")
//	if( m_pM->isARigidLink() ) {
//		;
//	}

	bool getBB = false;// we want to draw the actual cross-section
	switch( pM->taperType() ) {
		case NO_TAPER:
		{
			CCrossSection fullsection( pM );
			CPoint3DArray xsection_left;
			CPoint3DArray xsection_right;
			fullsection.GetCrossSection( xsection_left, getBB, NO_TAPER, nCrossSection );
			fullsection.GetCrossSection( xsection_right, getBB, NO_TAPER, nCrossSection );
			crossSections.add( xsection_left );
			crossSections.add( xsection_right );
			crossSectionLocs.add( m_setback );
			crossSectionLocs.add( pM->length() - m_setback );
			break;
		}
		case SINGLE_TAPER:
		case SINGLE_TOP_TAPER:
		{
			CCrossSection fullsection( pM ); // full cross-section 
			CCrossSection tapersection( pM ); // taper cross-section
			
			CPoint3DArray xsection_left;
			fullsection.GetCrossSection( xsection_left, getBB, NO_TAPER, nCrossSection ); // full cross-section at left
			crossSections.add( xsection_left );
			crossSectionLocs.add( m_setback );
			if( start_taper > 0.f ){
				CPoint3DArray xsection_start;
				fullsection.GetCrossSection( xsection_start, getBB, NO_TAPER, nCrossSection ); 
				crossSections.add( xsection_start );
				crossSectionLocs.add( start_taper );
			}
			if( end_taper < pM->length() ){
				CPoint3DArray xsection_end; //end of taper
				tapersection.GetCrossSection( xsection_end, getBB, pM->taperType(), nCrossSection );
				crossSections.add( xsection_end );
				crossSectionLocs.add( end_taper );
			}
			CPoint3DArray xsection_right; // right end
			tapersection.GetCrossSection( xsection_right, getBB, pM->taperType(), nCrossSection );
			crossSections.add( xsection_right );
			crossSectionLocs.add( pM->length() - m_setback );
			break;
		}
		case DOUBLE_TAPER:	// Centerline
		case DOUBLE_TOP_TAPER:
		{
			CCrossSection fullsection( pM ); // full cross-section 
			CCrossSection tapersection( pM ); // taper cross-section
			
			CPoint3DArray xsection_left;
			fullsection.GetCrossSection( xsection_left, getBB, NO_TAPER, nCrossSection ); // full cross-section always at left
			crossSections.add( xsection_left );
			crossSectionLocs.add( m_setback );
			if( start_taper > 0.f ){
				CPoint3DArray xsection_start_left;
				fullsection.GetCrossSection( xsection_start_left, getBB, NO_TAPER, nCrossSection );
				crossSections.add( xsection_start_left );
				crossSectionLocs.add( start_taper );
			}
			if( end_taper < pM->length() ){
				CPoint3DArray xsection_end_left;
				tapersection.GetCrossSection( xsection_end_left, getBB, pM->taperType(), nCrossSection );
				crossSections.add( xsection_end_left );
				crossSectionLocs.add( end_taper );
				if( !zero(end_taper - pM->length()/2 ) ){
					CPoint3DArray xsection_end_right;
					tapersection.GetCrossSection( xsection_end_right, getBB, pM->taperType(), nCrossSection );
					crossSections.add( xsection_end_right );
					crossSectionLocs.add( (pM->length() - end_taper) );
				}
			}
			if( start_taper > 0.f ){
				CPoint3DArray xsection_start_right;
				fullsection.GetCrossSection( xsection_start_right, getBB, NO_TAPER, nCrossSection );
				crossSections.add( xsection_start_right );
				crossSectionLocs.add( (pM->length() - start_taper) );
			}
			CPoint3DArray xsection_right;
			fullsection.GetCrossSection( xsection_right, getBB, NO_TAPER, nCrossSection );
			crossSections.add( xsection_right );
			crossSectionLocs.add( pM->length() - m_setback );
			break;
		}
	}  // switch
}

void CMemberExtrusion::SetupCableCrossSections( const CCable* pC, 
											  CPoint3DArrayArray& crossSections, 
											  CDoubleArray& crossSectionLocs,
											  int nCrossSection )
{
	ASSERT_RETURN( pC );

	///////////////////////////////////////////////////////////////////////////////
	// SetupCableCrossSections
	//
	// This function sets up the cross-sections for a given cable extrusion.
	// Extrusions must have at least two cross sections (beginning and end) 
	// and an array specifying the location of each cross-section. An extrusion 
	// can have as many cross sections as needed. For the tapered members we do  
	// tapering by specifying the cross sections at each taper point and let the  
	// extrusion class linearly interpolate polygon vertices between each cross section. 
	//
	// Note: The CCrossSection class is responsible for determining the appropriate  
	// cross-section based on the taper type.
	//
	// JL 04/05/2005
	///////////////////////////////////////////////////////////////////////////////
	CCrossSection fullsection( pC );
	CPoint3DArray xsection_left;
	CPoint3DArray xsection_right;
	bool getBB = false;// we want to draw the actual cross-section 
	fullsection.GetCrossSection( xsection_left, getBB, NO_TAPER, nCrossSection );
	fullsection.GetCrossSection( xsection_right, getBB, NO_TAPER, nCrossSection );
	crossSections.add( xsection_left );
	crossSections.add( xsection_right );
	crossSectionLocs.add( 0. );
	crossSectionLocs.add( pC->length() );
}

void CMemberExtrusion::SetExtrusionTextureCoords(double min, double max, int GraphType, const CResult* pR, bool bDiagram )
{
	ASSERT_RETURN( pR );
	//m_spineTexCoord.flush();
	//loop over all cross-sections defined by the spine
	int nCrossSections = m_spine.size();
	//m_spineTexCoord.clear();
	//loop over all cross-sections that define this member (currently, at most, double angles)
	for( int iExtrusion = 1; iExtrusion <= m_extrusions.getItemsInContainer(); iExtrusion++ )
	{
		std::vector<CPoint3D>& tcoords = m_extrusions[iExtrusion]->GetOrderedTexCoords( );
		int nCrossSectionPts = m_extrusions[iExtrusion]->GetNumPtsPerCrossSection( );
		//get the cross-section points for this extrusion - used for normal stress tex coords
		const CPoint3DArrayArray& crossSectionPtsArray = m_extrusions[iExtrusion]->GetCrossSections();
		ASSERT_RETURN( nCrossSections*nCrossSectionPts == (int)tcoords.size() );
		int texCoordCount = 1;

		int memberSections = pR->locations();
		double maxValue = 0.0; //max value for this extrusion, we use this to plot the max value of cables 
		for( int i = 0; i < nCrossSections; i++ ) 
		{ 
			int iPoint1 = 0;
			int iPoint2 = 0;
			double top = 0;
			double bot = 0;
			// First take care of end points (easy case!)
			if( i == 0 ){
				iPoint1 = 0;
				iPoint2 = 0;
			}
			else if( i == nCrossSections ){
				iPoint1 = memberSections;
				iPoint2 = memberSections;
			}
			else
			{ //interpolate
				double ratio = (double)i/(double)(nCrossSections-1);
				iPoint1 = (int)(ratio * (memberSections-1));
				iPoint2 = iPoint1 + 1;
				double ratio1 = (double)(iPoint1)/(double)(memberSections-1);
				double ratio2 = (double)(iPoint2)/(double)(memberSections-1);
				top = ratio - ratio1;
				bot = ratio2 - ratio1;
				if( zero( bot ) ) 
					bot = 1.;
			}

			if( iPoint1 >= memberSections -1 )
				iPoint1 = memberSections - 1;
			if( iPoint2 >= memberSections -1 )
				iPoint2 = memberSections - 1;

			// now determine the texture coordinate value mapped from min:max -> 0:1
			//double xLoc = m_spine[i].x;
			double xValue1 = 0.;
			double xValue2 = 0.;
			double xValue = 0.;
			//double fact = (double)(i-1)/(nCrossSections);
			//ASSERT( fact <= 1.0 );
			//int ivalue1 = (int)(fact*(double)(pR->locations()-1));
			if( GraphType == -999 ){
				xValue1 = 0.;
				xValue2 = 0.;
			}
			else if( GraphType > 0 && GraphType < 100 ) 
			{
				if( GraphType != 6 ){ 
					xValue1 = pR->force( IntToETDir( GraphType ), iPoint1+1 );
					xValue2 = pR->force( IntToETDir( GraphType ), iPoint2+1 );
				}else{ 
					xValue1 = pR->force( DX, iPoint1+1 );
					xValue2 = pR->force( DX, iPoint2+1 );
				}
			}
			else if( GraphType >= 100 ){// deflections
				xValue1 = pR->displacement( IntToETDir(GraphType-100), iPoint1+1 );
				xValue2 = pR->displacement( IntToETDir(GraphType-100), iPoint2+1 );
			}
			else {  // stress
				if( m_pC ){
					xValue1 = pR->stress( ETCableStress(-GraphType), iPoint1+1 );
					xValue2 = pR->stress( ETCableStress(-GraphType), iPoint2+1 );
				}else{
					xValue1 = pR->stress( ETMemberStress(-GraphType), iPoint1+1 );
					xValue2 = pR->stress( ETMemberStress(-GraphType), iPoint2+1 );
				}
			}

			if( zero( bot ) )
				bot = 1.;
			xValue = xValue1 + top/bot*(xValue2 - xValue1);

			double normalizedXValue = 0.;
			if( !zero(max - min) )
			{
				//if this is a diagram view we need to allow negative numbers
				if( bDiagram )
					normalizedXValue = (xValue)/abs(max-min);
				else
				{
					normalizedXValue = ( xValue - min )/( max - min );
					if( abs(max) < abs(min) )
						normalizedXValue = abs(( xValue - max )/( min - max ));
				}
			}
			if( abs( normalizedXValue ) > maxValue ) maxValue = normalizedXValue;
			if( iExtrusion == 1 && !m_pC )
			{
				//normalizedXValue = 0.1 + normalizedXValue*0.8;
				//m_spineTexCoord.push_back( normalizedXValue );
			}
			if( !m_pC ){
				//current cross-section - just use the first one for now
				CPoint3DArray& crossSection = crossSectionPtsArray[1];
				for( int j = 0; j < nCrossSectionPts; j++ )
				{
					//ASSERT( normalizedXValue <= 1.0 );
					//if normal stress type  and not a diagram
					//normal stress texture array using pR->interpolatedNormalStress( ... )
					if( GraphType == -11 && !bDiagram)
					{
						double y_crossSec = crossSection[j+1].y;
						double z_crossSec = crossSection[j+1].z;
						double normalStress = pR->interpolatedNormalStress( i+1, nCrossSections, y_crossSec, z_crossSec );
						normalizedXValue = ( normalStress - min )/( max - min );
						if( abs(max) < abs(min) )
							normalizedXValue = abs(( normalStress - max )/( min - max ));
						normalizedXValue = 0.1 + normalizedXValue*0.8;
						tcoords[texCoordCount-1].x = normalizedXValue;
					}
					else{
						tcoords[texCoordCount-1].x = normalizedXValue;
					}
					texCoordCount++;
				}
			}
		} // for i loop - cross sections
		//if its a cable we just use the max value for the cable like we did in VA 5.5
		if( m_pC ){
			for( int i = 0; i < nCrossSections; i++ ) {
				if( iExtrusion == 1 )
				{
					//m_spineTexCoord.push_back( maxValue );
				}
				for( int j = 0; j < nCrossSectionPts; j++ )
				{
					//ASSERT( normalizedXValue <= 1.0 );
					tcoords[texCoordCount-1].x = maxValue;
					texCoordCount++;
				}
			}
		}
		m_extrusions[iExtrusion]->UpdateTexCoords();
	} // for i extrusion
}
	
UINT GraphicsExtrusionThread( LPVOID pParam )
{
	CMemberExtrusion* pMem = (CMemberExtrusion*)pParam;
	if( !pMem )
		return 1;
	pMem->BuildExtrusions( );
	return 0;
}
///////////////////////////////////////////////////////////////////
//
// CExtrusionMap implementation
//
///////////////////////////////////////////////////////////////////
void CExtrusionMap::Draw( const CString& name )
{
	m_map[name]->Draw();
}
CMemberExtrusion* CExtrusionMap::get( const CString& name )
{
	return m_map[name];
}

void CExtrusionMap::set( const CString& name, CMemberExtrusion* ext )
{
	m_map[name] = ext;
}

void CExtrusionMap::remove( const CString& name )
{
		m_map.erase(name);
}

void CExtrusionMap::clear( )
{
	m_map.clear();
}

int CExtrusionMap::size( )
{
	return m_map.size( );
}