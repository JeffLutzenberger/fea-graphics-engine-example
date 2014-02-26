#include "IESDataStdAfx.h"
#include "DataGraphicsPlate.h"
#include "Graphics/GraphicsHelpers.h"
#include "ini.h"
#include "IniSize.h"
#include "IniColor.h"
#include "IniFont.h"
#include "Node.h"
#include "Model.h"
#include "project.h"
#include "math/Matrix44.h"


CDataGraphicsPlate::CDataGraphicsPlate( ) :
m_pP( NULL ),
m_pRC( NULL ),
m_scale( 0.0 ),
m_bTriangle( false ),
//m_min(1e6),
//m_max(-1e6),
m_GraphType( -10000 ),
m_isGlobal( false ),
m_isPrincipal( false ),
m_plane( MID_PLANE ),
m_bDisplacementsChanged( true )
{

}

CDataGraphicsPlate::CDataGraphicsPlate( const CPlanar* pP, const CResultCase* pRC = NULL, double scale = 0.0 ) :
m_pP( pP ),
m_pRC( pRC ),
m_scale( scale ),
m_bTriangle( false ),
//m_min(1e6),
//m_max(-1e6),
m_GraphType( -10000 ),
m_isGlobal( false ),
m_isPrincipal( false ),
m_plane( MID_PLANE ),
m_bDisplacementsChanged( true )
{
	Initialize( pP );
}

CDataGraphicsPlate::~CDataGraphicsPlate(void)
{
	for( int i = 0; i < 4; i++ ){
		m_resultMagAtNode[i] = 0.;
		m_texCoords[i] = CPoint3D();
	}
	m_pP = NULL;
	m_pRC = NULL;
}

void CDataGraphicsPlate::Initialize( const CPlanar* pP )
{
	m_pP = pP;
	for( int i = 0; i < 4; i++ ){
		m_resultMagAtNode[i] = 0.;
		m_texCoords[i] = CPoint3D();
	}
	CHECK_IF( pP )
	{
		m_bTriangle = (m_pP->node(4) == m_pP->node(1)) || (m_pP->node(4) == NULL);
		int nPts = m_bTriangle ? 3 : 4;
		for( int i = 0; i < nPts; i++ ) {
			const CNode* pN = m_pP->node(i+1);
			if( pN ) { // TeK Added Error checking, crashed with a corrupted file...
				m_points[i] = *pN;
			}
		}
	}
}

//void CDataGraphicsPlate::Draw( const WindowFilter& filter, double scale );
void CDataGraphicsPlate::Draw( bool pictureView, 
						   bool bDeformed, 
						   bool bShrink,
						   bool bShowLocalAxes,
						   bool bShowColorGradient,
						   double scale,
						   const CResultCase* pRC )
{
	ASSERT_RETURN( m_pP );

	//we can't simply cache the m_points because when plate nodes are modified they never
	//notify the plate so we have no way of knowing when to update the corners of the 
	//graphic plate.
	//
	//we can, however, cache the displaced positions because nodes will definitely NOT
	//be modified when we're in a result view.
	m_bTriangle = (m_pP->node(4) == m_pP->node(1)) || (m_pP->node(4) == NULL);
	int nPts = m_bTriangle ? 3 : 4;
	for( int i = 0; i < nPts; i++ ) {
		const CNode* pN = m_pP->node(i+1);
		CHECK_IF( pN ) { // TeK Added Error checking, crashed with a corrupted file...
			m_points[i] = *pN;
		}
	}

	const CResult* cR = NULL;
	//bool bDisplacementsChanged = false;
	if( bDeformed && theProject.isValid( pRC ) ) {
		if( m_bDisplacementsChanged || m_pRC != pRC || scale != m_scale ){
			m_bDisplacementsChanged = true;
			m_pRC = pRC;
			m_scale = scale;
			cR = m_pRC->result( *m_pP );
		}
	}

	CPoint3D points[4];
	
	for( int i = 0; i < nPts; i++ ) {
		points[i] = m_points[i];
		if( m_bDisplacementsChanged && cR )
		{
			m_dispPoints[i].x = points[i].x + m_scale*cR->displacement( DX, (ETPlanarLocation)i+1 );
			m_dispPoints[i].y = points[i].y + m_scale*cR->displacement( DY, (ETPlanarLocation)i+1 );
			m_dispPoints[i].z = points[i].z + m_scale*cR->displacement( DZ, (ETPlanarLocation)i+1 );
		}
		if( bDeformed )
			points[i] = m_dispPoints[i];
	}
	m_bDisplacementsChanged = false;

	CPoint3D center;
	CPoint3D unshrunkCenter;

	if( m_bTriangle )
	{
		center = points[0] + points[1] + points[2];
		center = center*0.333333;
		unshrunkCenter = center;

		if( bShrink )
		{
			center = center*0.2;
			points[0] = points[0]*0.8 + center;
			points[1] = points[1]*0.8 + center;
			points[2] = points[2]*0.8 + center;
		}
		double t = 0.;
		if( pictureView )
			t = m_pP->thickness();
		if( bShowColorGradient )
			DrawTexturedTriPlate( points, m_texCoords, t );
		else
			DrawTriPlate( points[0], points[1], points[2], t );
	}
	else
	{
		center = points[0] + points[1] + points[2] + points[3];
		center = center*0.25;
		unshrunkCenter = center;

		if( bShrink )
		{
			center = center*0.2;
			points[0] = points[0]*0.8 + center;
			points[1] = points[1]*0.8 + center;
			points[2] = points[2]*0.8 + center;
			points[3] = points[3]*0.8 + center;
		}
		double t = 0.;
		if( pictureView )
			t = m_pP->thickness();
		if( bShowColorGradient )
			DrawTexturedQuadPlate( points, m_texCoords, t );
		else
			DrawQuadPlate( points[0], points[1], points[2], points[3], t );
	}
	if( bShowLocalAxes )
	{
		double trans[3][3];
		m_pP->transformationMatrix( trans );
		transpose33( trans );
		CQuaternion q( trans );
		CVector3D axis;
		double angle;
		q.Disassemble( axis, angle );
		glPushMatrix();
		glTranslatef( (float)unshrunkCenter.x, (float)unshrunkCenter.y, (float)unshrunkCenter.z );
		glRotatef( float(angle*180/M_PI), float(axis.x), float(axis.y), float(axis.z) );
		glTranslatef( 0.f, 0.f, (float)m_pP->thickness() );
		// TeK Change 1/28/2008: Drawing full arrows on plate mesh is very crowded, changed to show
		// just the colored X,Y arrows without text.
		DrawOriginArrows( ini.graphics().fNarrowWidth, true, false, false );
		glPopMatrix();
	}
}

void CDataGraphicsPlate::SetupTextureCoordinates(double min, 
											 double max, 
											 int GraphType, 
											 bool isGlobal, 
											 bool isPrincipal, 
											 ETPlanarPlane plane, 
											 const CResultCase* pRC )
{
	ASSERT_RETURN( pRC );
	if( //m_min != min ||
		//m_max != max ||
		m_GraphType != GraphType ||
		m_isGlobal != isGlobal ||
		m_isPrincipal != isPrincipal ||
		m_plane != plane ||
		m_pRC != pRC )
	{
		//m_min = min;
		//m_max = max;
		m_GraphType = GraphType;
		m_isGlobal = isGlobal;
		m_isPrincipal = isPrincipal;
		m_plane = plane;
		m_pRC = pRC;
		m_bDisplacementsChanged = true;
	}
	else 
		return;

	const CResult* pR = m_pRC->result( *m_pP );
	ASSERT_RETURN( pR );

	CPoint3D texCoords[4];
	int nPts = m_bTriangle ? 3 : 4;

	ETPlanarPlane p = plane;

	for( int i = 0; i < 4; i++ ){
		m_resultMagAtNode[i] = 0.;
		m_texCoords[i] = CPoint3D();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	if( nPts < 3 ) return;
	double m_resultMagAtNode[4];
	//GraphType = 13;
	if( GraphType == 13 ) {  // deflections
		m_resultMagAtNode[0] = pR->displacement( NODE1 );
		m_resultMagAtNode[1] = pR->displacement( NODE2 );
		m_resultMagAtNode[2] = pR->displacement( NODE3 );
		if( nPts == 4 ) m_resultMagAtNode[3] = pR->displacement( NODE4 );
	}
	else if( !isPrincipal ) {
		if( GraphType <= TAU_XY || ( isGlobal && GraphType >= 9 ) ) {
			int si = GraphType;
			if( si == 9 )  // special cases for GraphType
				si = SIGMA_Z;
			else if( si == 10 )
				si = TAU_XY;
			else if( si == 11 )
				si = TAU_YZ;
			else if( si == 12 )
				si = TAU_ZX;
			m_resultMagAtNode[0] = pR->stress( (ETPlanarStress)si, NODE1, p, isGlobal );
			m_resultMagAtNode[1] = pR->stress( (ETPlanarStress)si, NODE2, p, isGlobal );
			m_resultMagAtNode[2] = pR->stress( (ETPlanarStress)si, NODE3, p, isGlobal );
			if( nPts == 4 ) m_resultMagAtNode[3] = pR->stress( (ETPlanarStress)si, NODE4, p, isGlobal );
		}
		else {
			m_resultMagAtNode[0] = pR->force( (ETPlanarForce)GraphType, NODE1, isGlobal );
			m_resultMagAtNode[1] = pR->force( (ETPlanarForce)GraphType, NODE2, isGlobal );
			m_resultMagAtNode[2] = pR->force( (ETPlanarForce)GraphType, NODE3, isGlobal );
			if( nPts == 4 ) m_resultMagAtNode[3] = pR->force( (ETPlanarForce)GraphType, NODE4, isGlobal );
		}
	}
	else {  // principal stresses requested
		m_resultMagAtNode[0] = pR->principalStress( (ETPrincipalStress)GraphType, NODE1, p );
		m_resultMagAtNode[1] = pR->principalStress( (ETPrincipalStress)GraphType, NODE2, p );
		m_resultMagAtNode[2] = pR->principalStress( (ETPrincipalStress)GraphType, NODE3, p );
		if( nPts == 4 ) m_resultMagAtNode[3] = pR->principalStress( (ETPrincipalStress)GraphType, NODE4, p );
	}
	if( nPts != 4 ) {
		m_resultMagAtNode[3] = m_resultMagAtNode[0];
	}

	//normalize the texture coordinate and add it to the plates member texture coordinates
	double normalizedMag = 0.;
	if( !zero(max - min) )
	{
		for( int i = 0; i < nPts; i++ ){
			normalizedMag = ( m_resultMagAtNode[i] - min )/( max - min );
			if( abs(max) < abs(min) )
				normalizedMag = abs(( m_resultMagAtNode[i] - max )/( min - max ));
			m_texCoords[i] = CPoint3D( normalizedMag, 0., 0. );
		}
	}
	return;
}