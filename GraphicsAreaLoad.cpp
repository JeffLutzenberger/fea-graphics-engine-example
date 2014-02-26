#include "StdAfx.h"
#include "GraphicsAreaLoad.h"

#include "Graphics/GraphicsHelpers.h"
#include "Graphics/GraphicsHelperClasses.h"

#include "GraphicsMemberLoad.h"
#include "GraphicsPlateLoad.h"

#include "Model.h"
#include "Area.h"
#include "Units\Units.h"
#include "datautil.h"
#include "ini.h"
#include "IniSize.h"

//#pragma message( JL "Area Loads now own member and planar loads and can be drawn here" )
//#pragma message( JL "Call the areaLoad's generatedLoadsAreUpToDate() function to see if the loads have been created" )
//#pragma message( JL "Call the areaLoad's generatedMemberLoadCount() and generatedPlanarLoadCount() functions to get counts" )
//#pragma message( JL "Call the areaLoad's generatedMemberLoad() and generatedPlanarLoad() accessors to get the loads" )
//#pragma message( JL "Somewhere in the filter we need to be able to turn these on and off" )
//#pragma message( JL "These loads should not be selectable graphically" )
//#pragma message( JL "These loads should appear different from regular loads (transparent or something cool)" )

CGraphicsAreaLoad::CGraphicsAreaLoad( const CAreaLoad* pAL )
{
	m_pAL = pAL;
}

CGraphicsAreaLoad::~CGraphicsAreaLoad(void)
{
}

void CGraphicsAreaLoad::Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, bool setFilling, double arrowLength )
{
	if( filter.areaFEALoads )
		DrawFEALoads( pDC, filter, text, arrowLength );
	else
		DrawAreaLoad( pDC, filter, text, setFilling, arrowLength );
}
void CGraphicsAreaLoad::DrawAreaLoad( CDC* pDC, const CWindowFilter& filter, CGLText* text, bool setFilling, double arrowLength )
{
	//get the associated area vertices
	ASSERT_RETURN( m_pAL );
	const CArea* pA = (const CArea*)(&m_pAL->area());
	ASSERT_RETURN( pA && pA->formsAPlane() );
	//draw sides first
	CDoubleArray mag;
	CPoint3DArray top;
	double max_mag = 0.;
	float length = (float)arrowLength;//ini.size().planarLoadLength;
	int nPoints = pA->boundary().points();
	for( int i = 1; i <= nPoints; i++ ){
		double magnitude = m_pAL->nodeChainMagnitude(i);
		if( abs( magnitude ) > abs( max_mag ) ) max_mag = magnitude;
		mag.add( magnitude*length );
	}
	if( zero( max_mag ) ) max_mag = 1.;
	max_mag = abs( max_mag );
	double avg_mag = 0;
	for( int i = 1; i <= mag.size(); i++ )
	{
		mag[i] /= max_mag;
		avg_mag += mag[i];
	}
	avg_mag /= nPoints;

	//start at the first vertex and work around from there
	CVector3D load_dir = -CVector3D(m_pAL->loadDirection()).normalize();
	CVector3D x_dir = load_dir;

	CVector3D textOffsetVector = load_dir;
	textOffsetVector*=avg_mag;

	//draw arrows
	int j = 1;
	DisableGLTransparency();

	CPoint3D angles;

	j = 1;
	CPoint3D centroid( 0., 0., 0. );
	for( int i = 1; i <= nPoints; i++ )
	{
		/*double magnitude = m_pAL->nodeChainMagnitude(i);
		CPoint3D p1( *pA->boundary().point( i ) );
		CPoint3D p2( p1.x + load_dir.x*mag[i], p1.y + load_dir.y*mag[i], p1.z + load_dir.z*mag[i] );
		DrawLineArrow( p1, p2, abs( mag[i] ), 1. );*/

		j++;
		if( j > pA->boundary().points() ) j = 1;
		CPoint3D p1( *pA->boundary().point( i ) );
		CPoint3D p2( *pA->boundary().point( j ) );
		centroid+=p1;
		int nPts = 4;
		//float l = p1.distance_to( p2 );
		//float dl = l/nPts;
		for( int k = 0; k < nPts; k++ )
		{
			double kMag = ( mag[j] - mag[i] )*k/nPts + mag[i];
			//don't draw zero length arrows
			if( zero( kMag ) ) continue;
			CLine3D lin( p1, p2 );
			CVector3D kV(load_dir.x*kMag, load_dir.y*kMag, load_dir.z*kMag);
			CPoint3D kP1 = lin.offset( double(k)/double(nPts) );
			CPoint3D kP2( kP1.x + kV.x, kP1.y + kV.y, kP1.z + kV.z );  
			if( kMag < 0. )
				kP2 = CPoint3D( kP1.x - kV.x, kP1.y - kV.y, kP1.z - kV.z );
			DrawLineArrow( kP1, kP2, float(kMag), ini.size().planarLoadWidth );
		}
	}
	centroid.x /= nPoints;
	centroid.y /= nPoints;
	centroid.z /= nPoints;

	if( !g_bDrawWireframe )
		EnableGLTransparency();
	glBegin( GL_QUADS );
	j = 1;
	for( int i = 1; i <= nPoints; i++ ){
		j++;
		if( j > pA->boundary().points() ) j = 1;
		CPoint3D p1( *pA->boundary().point( i ) );
		CPoint3D p2( *pA->boundary().point( j ) );
		CVector3D v1(load_dir.x*mag[j], load_dir.y*mag[j], load_dir.z*mag[j]);
		CPoint3D p3( p2.x + v1.x, p2.y + v1.y, p2.z + v1.z );
		CVector3D v2(load_dir.x*mag[i], load_dir.y*mag[i], load_dir.z*mag[i]);
		CPoint3D p4( p1.x + v2.x, p1.y + v2.y, p1.z + v2.z );
		if( i == 1 )
			top.add( p4 );
		top.add( p3 );
		if( sign( mag[i] ) != sign( mag[j] ) ){
			glEnd();
			glBegin( GL_TRIANGLES );
			//draw two triangles
			double a = mag[i];
			double d = mag[j];
			double len = p1.distance_to( p2 );
			ASSERT( !zero( len ) );
			double denom = 1;
			if( !zero( a ) && !zero( d ) )
				denom = 1 + abs(d/a);
			ASSERT_RETURN( !zero( denom ) );
			double ratio = 1 / denom;
			CPoint3D intPt = CLine3D( p1, p2 ).offset( ratio );
			CGLFace( p1, intPt, p4 ).Draw();
			CGLFace( intPt, p2, p3 ).Draw();
			glEnd();
			glBegin( GL_QUADS );
		}
		else
			CGLFace( p1, p2, p3, p4 ).Draw();
	}
	glEnd();

	if( !g_bDrawWireframe )
	{
		CGLTessellatedPolygon poly( top );
		if( !setFilling )
			poly.SetFilling( FALSE );
		else 
			poly.SetFilling( TRUE );
		for( int iHole = 1; iHole <= pA->holes(); iHole++ )
		{
			CChain* hole_chain = pA->holeArray()[iHole];
			if( !hole_chain ){
				ASSERT(FALSE);
				continue;
			}
			CPoint3DArray hole_pts;
			for( int i = 1; i <= hole_chain->points(); i++ ){
				double magnitude = m_pAL->magnitudeAtPoint( *(hole_chain->point(i) ) );
				double hole_mag = abs(magnitude*length/max_mag);
				CVector3D v1(load_dir.x*hole_mag, load_dir.y*hole_mag, load_dir.z*hole_mag);
				CPoint3D p1( *(hole_chain->point(i) ) );
				CPoint3D p2( p1.x + v1.x, p1.y + v1.y, p1.z + v1.z );
				hole_pts.add( p2 ); 
			}
			poly.addHole( hole_pts );
		}
		poly.Draw();
		DisableGLTransparency();
	}

	//text...
	if( filter.loadValues && text )
	{
		CString label;
		ETUnit quantity;
		quantity = PRESSURE_UNIT;
		label = Units::show( quantity, m_pAL->magnitude( AT_START ) );
		label += " -> ";
		label += Units::show( quantity, m_pAL->magnitude( AT_END ) );
		
		text->ClearText();
		if( label.GetLength() > 0 )
			text->AddLine( label );

		//location of our text
		CPoint3D center3D = centroid;
		//raise the text off the plate, m_textOffsetVector gets set when we draw the arrows
		//center3D.x += textOffsetVector.x;
		//center3D.y += textOffsetVector.y;
		//center3D.z += textOffsetVector.z;

		CPoint center2D;
		CVector3D v1 = m_pAL->area().spanDirection();
		CPoint3D startPt( centroid.x, centroid.y, centroid.z );
		CPoint3D endPt( startPt.x + v1.x*100, startPt.y + v1.y*100, startPt.z + v1.z*100 );
		//CPoint3D startPt( *pA->boundary().point( 1 ) );
		//CPoint3D endPt( *pA->boundary().point( 2 ) );
		CPoint p1;
		CPoint p2;
		if( GetScreenPtFrom3DPoint( startPt, p1 ) && 
			GetScreenPtFrom3DPoint( endPt, p2 ) &&
			GetScreenPtFrom3DPoint( center3D, center2D ) )
		{
			double angle = atan2( -(float)(p2.y - p1.y), (float)(p2.x - p1.x) );
			text->SetHAlignment( CGLOutlineText::HCENTER );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
					ini.font().graphWindow.size/*m_printScale*/, 
					ini.color().graphTitles, center2D, angle  ); 
			text->SetHAlignment( CGLOutlineText::HLEFT );
		}
	}
	//if( m_pAL )
	//{
	//	//get average size of area to help size the span direction arrow
	//	int nItems = top.getItemsInContainer();
	//	double dist = 50.;
	//	if( nItems > 0 )
	//	{
	//		dist = 0;
	//		CPoint3D p1 = top[1];
	//		for( int i = 2; i <= nItems; i++ )
	//			dist += p1.distance_to( top[i] );
	//		dist /= nItems;
	//	}
	//	CVector3D v1 = m_pAL->area().spanDirection( );
	//	/*v1.x *= dist*0.25;
	//	v1.y *= dist*0.25;
	//	v1.z *= dist*0.25;*/
	//	CPoint3D startPt( centroid.x, centroid.y, centroid.z );
	//	CPoint3D endPt( startPt.x + v1.x*dist*0.25, startPt.y + v1.y*dist*0.25, startPt.z + v1.z*dist*0.25 );
	//	CPoint3D endPt2( startPt.x - v1.x*dist*0.25, startPt.y - v1.y*dist*0.25, startPt.z - v1.z*dist*0.25 );
	//	//DrawLineSpanArrow( endPt, startPt, float(dist*0.5), line_width );
	//	DrawLineSpanArrow( endPt2, startPt, float(dist*0.5), (float)(line_width+2) );
	//	if( !m_pAL->area().isOneWay() )
	//	{
	//		CVector3D up = m_pAL->area().plane().n();
	//		CVector3D v2 = v1.cross( up );
	//		startPt = CPoint3D( centroid.x, centroid.y, centroid.z );
	//		endPt = CPoint3D( startPt.x + v2.x*dist*0.25, startPt.y + v2.y*dist*0.25, startPt.z + v2.z*dist*0.25 );
	//		endPt2 = CPoint3D( startPt.x - v2.x*dist*0.25, startPt.y - v2.y*dist*0.25, startPt.z - v2.z*dist*0.25 );
	//		DrawLineSpanArrow( endPt, startPt, float(dist*0.5), (float)(line_width+2) );
	//		//DrawLineSpanArrow( endPt2, startPt, float(dist*0.5), line_width );
	//	}
	//}
}

void CGraphicsAreaLoad::DrawFEALoads( CDC* pDC, const CWindowFilter& filter, CGLText* text, double arrowLength )
{
	ASSERT_RETURN( m_pAL );
	if( !m_pAL->generatedLoadsAreUpToDate() )
		return;

	int memberLoadCount = m_pAL->generatedMemberLoadCount();
	for( int i = 0; i < memberLoadCount; i++ )
	{
		CMemberLoad* pML = m_pAL->generatedMemberLoad(i);
		CGraphicsMemberLoad ml( pML );
		ml.Draw(pDC, filter, text );
	}

	int planarLoadCount = m_pAL->generatedPlanarLoadCount();
	for( int i = 0; i < planarLoadCount; i++ )
	{
		CPlanarLoad* pPL = m_pAL->generatedPlanarLoad(i);
		//double loadLength = ini.size().planarLoadLength;
		CGraphicsPlateLoad pl( pPL );
		pl.Draw( pDC, filter, text, arrowLength );
	}
}

