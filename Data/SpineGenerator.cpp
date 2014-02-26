#include "IESDataStdAfx.h"
#pragma hdrstop

#include "SpineGenerator.h"
#include "data/coord.h"
#include "Node.h"
#include "structur.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSpineGenerator::CSpineGenerator( const CMember* pM ):
m_pM( pM ),
m_pC( NULL ),
m_pRC( NULL )
{
}

CSpineGenerator::CSpineGenerator( const CMember* pM, const CResultCase* pRC ):
m_pM( pM ),
m_pC( NULL ),
m_pRC( pRC )
{
}

CSpineGenerator::CSpineGenerator( const CCable* pC ):
m_pC( pC ),
m_pM( NULL ),
m_pRC( NULL )
{
}

CSpineGenerator::CSpineGenerator( const CCable* pC, const CResultCase* pRC ):
m_pC( pC ),
m_pM( NULL ),
m_pRC( pRC )
{
}

CSpineGenerator::~CSpineGenerator(void)
{
}

bool CSpineGenerator::GetSpine( bool deformed, int nPoints, double setback, double scale, std::vector<CPoint3D>& spine )
{
	ASSERT_RETURN_ITEM( m_pM || m_pC, false );
	if( m_pM )
		return GetMemberSpine( deformed, nPoints, setback, scale, spine );
	else if( m_pC )
		return GetCableSpine( deformed, nPoints, setback, scale, spine );
	else 
		ASSERT_RETURN_ITEM( false, false );
}

bool CSpineGenerator::GetMemberSpine( bool deformed, int nPoints, double setback, double scale, std::vector<CPoint3D>& spine )
{
	ASSERT_RETURN_ITEM( m_pM, false );

	CNode* pN1;
	pN1 = (CNode*)m_pM->node(1);
	if( !pN1 )
		return false;

	CNode* pN2;
  	pN2 = (CNode*)m_pM->node(2);
	if( !pN2 )
		return false;

	double offsetx = 0.0;
	double offsety = 0.0;
	double offsetz = 0.0;

	offsetx = setback;
	double xLength = m_pM->length() - 2.*offsetx;
	if( xLength <= 0.0 ) xLength = 1.0;
	if( m_pM->hasCenterlineOffset() ) 
	{
		offsety = m_pM->centerlineOffset( DY );
		offsetz = m_pM->centerlineOffset( DZ );
	}
	
	//now make the spine.
	//spine.clear();
	bool haveDisplacedShape = false;
	if( m_pM /*&& !zero( scale )*/ && m_pRC && deformed )
		haveDisplacedShape = true;
	int nSections = nPoints;
	if( ( !haveDisplacedShape && m_pM->isTapered() ) ||
		(  haveDisplacedShape && m_pM->isTapered() && nSections < 10 ) )
	{
		nSections = 10;
	}
	else if( !haveDisplacedShape )
		nSections = 1;
	if( nSections <= 0 )
		nSections = 1;
	//double dLoc[3] = {0.0};
	double len = 0.;
	double lenInc = xLength/nSections;
	// get global member orientation matrix to be used for response cases
	double Xdir[3] = {1.0,0.0,0.0};
	double Ydir[3] = {0.0,1.0,0.0};
	double Zdir[3] = {0.0,0.0,1.0};
	//double tempZDir[3] = {0., 0., 1.};
	bool bGlobal = true;
	if( haveDisplacedShape )
	{ 
		m_pM->localToGlobal( Xdir, true );
		m_pM->localToGlobal( Ydir, true );
		m_pM->localToGlobal( Zdir, true );
		/*if(m_pRC->type() == RESPONSE_CASE_RESULTS )
		{
			m_pM->localToGlobal( Xdir, true );
			m_pM->localToGlobal( Ydir, true );
			m_pM->localToGlobal( Zdir, true );
		}
		else if( m_pRC->type() == ENVELOPE_RESULT_CASE )
		{
			m_pM->localToGlobal( Xdir, true );
			m_pM->localToGlobal( Ydir, true );
			m_pM->localToGlobal( Zdir, true );
			bGlobal = true;
		}*/
	}
	for( int iLoc = 0; iLoc <= nSections; iLoc++ ) {
		double xi = len + offsetx; 
		double yi = offsety;
		double zi = offsetz;
		double elongation = 0.0;
		double lateral = 0.0;
		double vertical = 0.0;
		if( haveDisplacedShape ){ 
			const CResult* cR = m_pRC->result( *m_pM );
			CHECK_IF( cR != NULL ){
				double dx = scale*cR->interpolatedDisplacement( DX, xi, bGlobal );
				double dy = scale*cR->interpolatedDisplacement( DY, xi, bGlobal );
				double dz = scale*cR->interpolatedDisplacement( DZ, xi, bGlobal );
				if( abs(dx) > 1e15 ) dx = 0.;
				if( abs(dy) > 1e15 ) dy = 0.;
				if( abs(dz) > 1e15 ) dz = 0.;
				// use the transpose to go from global to local
				elongation = Xdir[0]*dx + Xdir[1]*dy + Xdir[2]*dz;
				vertical   = Ydir[0]*dx + Ydir[1]*dy + Ydir[2]*dz;
				lateral    = Zdir[0]*dx + Zdir[1]*dy + Zdir[2]*dz;
				xi += elongation;
				yi += vertical;
				zi += lateral;
			}
		}
		len += lenInc;
		spine.push_back( CPoint3D( xi, yi, zi ) );
	}
	
	return true;
}

double cableDisplacement( int showPoint, int maxPoints, ETDirection d,
								   bool isGlobal, const CResult* cR )
{
	if( showPoint < 0 )
		return 0;
	if( maxPoints <= 1 )
		return 0;
	if( showPoint >= maxPoints )
		return 0;

	double displacement = 0.0;
	ASSERT_RETURN_ITEM( cR, 0.0 );

	int memberSections = cR->locations();
	if( memberSections <= 1 )
		return 0;
	// First take care of end points (easy case!)
	if( showPoint == 0 )
		displacement = cR->displacement( d, 1, isGlobal );
	else if( showPoint == maxPoints-1 )
		displacement = cR->displacement( d, memberSections, isGlobal );
	else
	{ // interpolate
		double ratio = (double)showPoint/(double)(maxPoints-1);
		int iPoint1 = (int)(ratio * (memberSections-1));
		int iPoint2 = iPoint1 + 1;
		double ratio1 = (double)(iPoint1)/(double)(memberSections-1);
		double ratio2 = (double)(iPoint2)/(double)(memberSections-1);
		double top = ratio - ratio1;
		double bot = ratio2 - ratio1;

		double displacement1 = cR->displacement( d, iPoint1+1, isGlobal );
		double displacement2 = cR->displacement( d, iPoint2+1, isGlobal );

		// Just give a linear interpolation
		// Use double arithmetic for accuracy
		if( bot == 0.0 )
			return 0;

		displacement = displacement1 + top/bot*(displacement2 - displacement1);
	}  // end if interpolating

	return displacement;
}	// end CPostView::showDisplacement()

bool CSpineGenerator::GetCableSpine( bool deformed, int nPoints, double /*setback*/, double scale, std::vector<CPoint3D>& spine )
{
	ASSERT_RETURN_ITEM( m_pC, false );
	const CNode* pN1;
	const CNode* pN2;
	pN1 = m_pC->node(1);
	pN2 = m_pC->node(2);
	ASSERT_RETURN_ITEM( pN1 && pN2, false );
	const CResult* cR = NULL;
	bool haveDisplacedShape = false;
	bool isStraight = false;
	if( m_pC && !zero( scale ) && m_pRC && deformed ){
		cR = m_pRC->result( *m_pC );
		haveDisplacedShape = true;
		// check for special case where all cable displacements 
		// are zero (engine does this if cable is straight)
		isStraight = true;
		for( int i = 1; i <= cR->locations(); i ++ ) {
			if( !zero(cR->displacement( DY, i, false ) ) ) {
				isStraight = false;
				break;
			}
		}
	}

	double dLoc1[3] = { pN1->x(), pN1->y(), pN1->z() };

	// set the subdivisions for result drawing
	int CABLE_DRAW_POINTS = nPoints;

	const CResult* cR1 = NULL;
	const CResult* cR2 = NULL;

	CCoordinate disp1;
	double dispDelta[3];

	if( haveDisplacedShape && m_pRC ) {
		cR1 = m_pRC->result( *pN1 );
		if( !cR1 ) return false;
		cR2 = m_pRC->result( *pN2 );
		if( !cR2 ) return false;

		disp1.x(  pN1->x() + scale*cR1->displacement( DX ) );
		disp1.y(  pN1->y() + scale*cR1->displacement( DY ) );
		disp1.z(  pN1->z() + scale*cR1->displacement( DZ ) );
		CCoordinate disp2;
		disp2.x(  pN2->x() + scale*cR2->displacement( DX ) );
		disp2.y(  pN2->y() + scale*cR2->displacement( DY ) );
		disp2.z(  pN2->z() + scale*cR2->displacement( DZ ) );
		dispDelta[0] = disp2.x() - disp1.x();
		dispDelta[1] = disp2.y() - disp1.y();
		dispDelta[2] = disp2.z() - disp1.z();
	} 

	for( int i = 1; i <= CABLE_DRAW_POINTS; i++ )	{
		double factor;
		factor = double(i-1)/double(CABLE_DRAW_POINTS-1);
		dLoc1[0] = dLoc1[1] = dLoc1[2] = 0.;
		if( isStraight ) {
			dLoc1[0] = disp1.x() + dispDelta[0]*factor;
			dLoc1[1] = disp1.y() + dispDelta[1]*factor;
			dLoc1[2] = disp1.z() + dispDelta[2]*factor;
		}
		else {
			CCoordinate c;
			c = m_pC->catenaryLocation( factor );
			dLoc1[0] = c.x();
			dLoc1[1] = c.y();
			dLoc1[2] = c.z();

			if( haveDisplacedShape && cR1 && cR ) {
				if( theStructure.canNodesDisplace( DX ) )
					dLoc1[0] += scale * cableDisplacement( i-1, CABLE_DRAW_POINTS, DX, true, cR );
				if( theStructure.canNodesDisplace( DY ) )
					dLoc1[1] += scale * cableDisplacement( i-1, CABLE_DRAW_POINTS, DY, true, cR );
				if( theStructure.canNodesDisplace( DZ ) )
					dLoc1[2] += scale * cableDisplacement( i-1, CABLE_DRAW_POINTS, DZ, true, cR );
				dLoc1[0] += scale*cR1->displacement( DX );
				dLoc1[1] += scale*cR1->displacement( DY );
				dLoc1[2] += scale*cR1->displacement( DZ );

			}
		}
		spine.push_back( CPoint3D( dLoc1[0], dLoc1[1], dLoc1[2] ) );
		
	}  // end for each pCable segment to draw

	return true;
}

bool CSpineGenerator::GetRotations( int nPoints, double setback, double rotations[] )
{
	ASSERT_RETURN_ITEM( m_pM, false );

	double offsetx = 0.0;
	
	offsetx = setback;
	double xLength = m_pM->length() - 2.*offsetx;

	const CResult* cR = NULL;
	bool haveDisplacedShape = false;
	if( m_pM && m_pRC )
	{
		haveDisplacedShape = true;
		cR = m_pRC->result( *m_pM );
	}
	int nSections = nPoints;
	if( haveDisplacedShape == false )
	{
		if( m_pM->isTapered() )
			nSections = 10;
		else
			nSections = 1;
	}
	if( nSections <= 0 )
		nSections = 1;
	double len = 0.;
	double lenInc = xLength/nSections;
	for( int iLoc = 0; iLoc <= nSections; iLoc++ ) {
		double xi = len + offsetx; 
		double rx = 0;
		if( haveDisplacedShape ){ 
			CHECK_IF( cR != NULL ){
				if( m_pRC->type() == RESPONSE_CASE_RESULTS || m_pRC->type() == ENVELOPE_RESULT_CASE )
				{
					// transform global (all positive) to local member orientation
					// this is not the same as getting the local displacements using the 
					// interpolatedDisplacment function since positive local is not neccesarily
					// positive in the global system.
					rx = cR->interpolatedDisplacement( RX, xi, true );
					//double dx = scale*cR->interpolatedDisplacement( DX, xi, true );
					/*double dy = scale*cR->interpolatedDisplacement( DY, xi, true );
					double dz = scale*cR->interpolatedDisplacement( DZ, xi, true );*/
					// use the transpose (inverse for an orthogonal matrix) to go from global to local
					//elongation = Xdir[0]*dx + Xdir[1]*dy + Xdir[2]*dz;
					//vertical   = Ydir[0]*dx + Ydir[1]*dy + Ydir[2]*dz;
					//lateral    = Zdir[0]*dx + Zdir[1]*dy + Zdir[2]*dz;
				}
				else
				{
					rx = cR->interpolatedDisplacement( RX, xi, true );
					//double dx = scale*cR->interpolatedDisplacement( DX, xi, true );
					//double dy = scale*cR->interpolatedDisplacement( DY, xi, true );
					//double dz = scale*cR->interpolatedDisplacement( DZ, xi, true );
					//// use the transpose (inverse for an orthogonal matrix) to go from global to local
					//elongation = Xdir[0]*dx + Xdir[1]*dy + Xdir[2]*dz;
					//vertical   = Ydir[0]*dx + Ydir[1]*dy + Ydir[2]*dz;
					//lateral    = Zdir[0]*dx + Zdir[1]*dy + Zdir[2]*dz;

				}
				//xi += elongation;
				//yi += vertical;
				//zi += lateral;
			}
		}
		rotations[iLoc] = rx;
		len += lenInc;
	}
	
	return true;
}