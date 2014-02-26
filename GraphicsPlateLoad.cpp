#include "StdAfx.h"

#include "GraphicsPlateLoad.h"

#include "Graphics/GraphicsHelpers.h"
#include "Graphics/GraphicsHelperClasses.h"

#include "Model.h"
#include "Planar.h"
#include "Units\Units.h"
#include "datautil.h"

#include "ini.h"
#include "IniSize.h"
#include "IniColor.h"
#include "IniFont.h"

CGraphicsPlateLoad::CGraphicsPlateLoad( const CPlanarLoad* pPL ) :
m_textOffsetVector( CVector3D() )
{
	SetupPlateLoad( pPL );
}
CGraphicsPlateLoad::~CGraphicsPlateLoad()
{
}

void CGraphicsPlateLoad::SetupPlateLoad( const CPlanarLoad* pPL )
{
	ASSERT_RETURN( pPL );
	m_pPL = pPL;
}

void CGraphicsPlateLoad::Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, double loadLength )
{
	//double line_length, double line_width, ETGraphicDetail detail, ETGraphicsDrawStyle style
	//PLANAR_UNIFORM_PRESSURE,
	//PLANAR_LINEAR_PRESSURE,
	//PLANAR_HYDROSTATIC_PRESSURE,
	//PLANAR_LINEARVARYING_PRESSURE,  // new in va40
	//PLANAR_TEMPERATURE_INCREASE,  // new in va40
	//PLANAR_TEMPERATURE_GRADIENT   // new in va40
	DrawPressure( loadLength );

	//text...
	ASSERT_RETURN(m_pPL);
	if( filter.loadValues && m_pPL )
	{
		CString label;
		ETUnit quantity;
		switch( m_pPL->type() )
		{
		case PLANAR_UNIFORM_PRESSURE:
			if( filter.plate.pressureLoads ){
				quantity = PRESSURE_UNIT;
				label = Units::show( quantity, m_pPL->magnitude( AT_START ) );
			}
			break;
		case PLANAR_LINEAR_PRESSURE:
			if( filter.plate.pressureLoads ){
				quantity = PRESSURE_UNIT;
				label = Units::show( quantity, m_pPL->magnitude( AT_START ) );
				label += " -> ";
				label += Units::show( quantity, m_pPL->magnitude( AT_END ) );
			}
			break;
		case PLANAR_HYDROSTATIC_PRESSURE:
			if( filter.plate.pressureLoads ){
				quantity = PRESSURE_UNIT;
				label = Units::show( quantity, m_pPL->magnitude( AT_START ) );
				label += " -> ";
				label += Units::show( quantity, m_pPL->magnitude( AT_END ) );
			}
			break;
		case PLANAR_TEMPERATURE_INCREASE:
			if( filter.plate.thermalLoads ){
				quantity = TEMPERATURE_UNIT;
				label = Units::show( quantity, m_pPL->magnitude( AT_START ) );
			}
			break;
		case PLANAR_TEMPERATURE_GRADIENT:
			if( filter.plate.thermalLoads ){
				quantity = TEMPERATURE_GRADIENT_UNIT;
				label = Units::show( quantity, m_pPL->magnitude( AT_START ) );
			}
			break;
		default:
			break;
		}

		text->ClearText();
		if( label.GetLength() > 0 )
			text->AddLine( label );

		//location of our text
		CPoint3D center3D = m_pPL->planar().centroid();
		//raise the text off the plate, m_textOffsetVector gets set when we draw the arrows
		center3D.x += m_textOffsetVector.x;
		center3D.y += m_textOffsetVector.y;
		center3D.z += m_textOffsetVector.z;

		CPoint center2D;
		double trans[3][3];
		m_pPL->planar().transformationMatrix( trans );
		double angle = atan2( trans[0][1], trans[0][0] );
		if( GetScreenPtFrom3DPoint( center3D, center2D ) )
		{
			text->SetHAlignment( CGLOutlineText::HCENTER );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
					ini.font().graphWindow.size/*m_printScale*/, 
					ini.color().graphTitles, center2D, angle  ); 
			text->SetHAlignment( CGLOutlineText::HLEFT );
		}
	}
}

void CGraphicsPlateLoad::DrawPressure( double loadLength )
{

	const CPlanar* pP = (const CPlanar*)(&m_pPL->planar());
	ASSERT_RETURN( pP );
	//CPoint3D points[4];
	bool isTriangle = (pP->node(4) == pP->node(1)) || (pP->node(4) == NULL);
	int nPts = isTriangle ? 3 : 4;

	double mag[4] = {0};
	double max_mag = 0.;
	float length = loadLength;//(float)ini.size().planarLoadLength;
	for( int i = 0; i < nPts; i++ ){
		double magnitude = m_pPL->magnitude( ETPosition( i ) );
		if( abs( magnitude ) > abs( max_mag ) ) max_mag = magnitude;
		mag[i] = magnitude*length;
	}
	if( zero( max_mag ) ) max_mag = 1.;
	max_mag = abs( max_mag );
	double avg_mag = 0;
	for( int i = 0; i < nPts; i++ )
	{
		mag[i] /= max_mag;
		avg_mag += mag[i];
	}
	avg_mag /= nPts;

	//CPoint3DArray points;
	CPoint3D points[4] = {CPoint3D()};
	for( int i = 0; i < nPts; i++ )
		points[i] = CPoint3D( pP->node(i+1)->x(), pP->node(i+1)->y(), pP->node(i+1)->z() );
	if( nPts == 3 ) points[3] = points[2];

	//start at the first vertex and work around from there
	CVector3D nn = CVector3D( points[1], points[0] ).cross( CVector3D( points[2], points[0] ) );
	CVector3D nv( 1., 0., 0. );
	if( zero(nn.length()) )
		nn = nv;
	nn.normalize();

	CVector3D load_dir = -nn;
	CVector3D x_dir = load_dir;
	m_textOffsetVector = load_dir;
	m_textOffsetVector*=avg_mag;

	//draw arrows
	int j = 0;
	float saveWidth = 1;
	glGetFloatv( GL_LINE_WIDTH, &saveWidth );
	glLineWidth( ini.size().planarLoadWidth );
	for( int i = 0; i < nPts; i++ )
	{
		j++;
		if( j >= nPts ) j = 0;
		CPoint3D p1( points[i] );
		CPoint3D p2( points[j] );
		double loadLength = p1.distance_to(p2);
		int nArrows = int(abs(loadLength/length)*1.5);
		if( nArrows < 3 ) nArrows = 3;
		if( nArrows > 10 ) nArrows = 10;
		CPoint3D startTop = p1;
		CPoint3D endTop = p2;
		for( int k = 0; k <= nArrows; k++ )
		{
			double kMag = ( mag[j] - mag[i] )*k/nArrows + mag[i];
			//don't draw zero length arrows
			if( zero( kMag ) ) continue;
			CLine3D lin( p1, p2 );
			CVector3D kV(load_dir.x*kMag, load_dir.y*kMag, load_dir.z*kMag);
			CPoint3D kP1 = lin.offset( double(k)/double(nArrows) );
			CPoint3D kP2( kP1.x + kV.x, kP1.y + kV.y, kP1.z + kV.z );
			if( k == 0 )
				startTop = kP2;
			if( k == nArrows )
				endTop = kP2;
			if( kMag < 0. )
				kP2 = CPoint3D( kP1.x - kV.x, kP1.y - kV.y, kP1.z - kV.z );
			DrawLineArrow( kP1, kP2, (float)kMag, ini.size().planarLoadWidth );
		}
		//draw the line across the top...
		glBegin( GL_LINES );
			glVertex3f( (float)p1.x, (float)p1.y, (float)p1.z );
			glVertex3f( (float)p2.x, (float)p2.y, (float)p2.z );
			glVertex3f( (float)startTop.x, (float)startTop.y, (float)startTop.z );
			glVertex3f( (float)endTop.x, (float)endTop.y, (float)endTop.z );
		glEnd();
	}
	glLineWidth( saveWidth );

	////now draw load volume
	////do the sides first
	//if( !g_bDrawWireframe )
	//	EnableGLTransparency();
	//glBegin( GL_QUADS );
	//j = 0;
	//for( int i = 0; i < nPts; i++ ){
	//	j++;
	//	if( j >= nPts ) j = 0;
	//	CPoint3D p1( points[i] );
	//	CPoint3D p2( points[j] );
	//	CVector3D v1(load_dir.x*mag[j], load_dir.y*mag[j], load_dir.z*mag[j]);
	//	CPoint3D p3( p2.x + v1.x, p2.y + v1.y, p2.z + v1.z );
	//	CVector3D v2(load_dir.x*mag[i], load_dir.y*mag[i], load_dir.z*mag[i]);
	//	CPoint3D p4( p1.x + v2.x, p1.y + v2.y, p1.z + v2.z );
	//	top[i] = p4;
	//	if( sign( mag[i] ) != sign( mag[j] ) ){
	//		glEnd();
	//		glBegin( GL_TRIANGLES );
	//		//draw two triangles
	//		double a = mag[i];
	//		double d = mag[j];
	//		double len = p1.distance_to( p2 );
	//		ASSERT( !zero( len ) );
	//		double denom = 1;
	//		if( !zero( a ) && !zero( d ) )
	//			denom = 1 + abs(d/a);
	//		ASSERT_RETURN( !zero( denom ) );
	//		double ratio = 1 / denom;
	//		CPoint3D intPt = CLine3D( p1, p2 ).offset( ratio );
	//		CGLFace( p1, intPt, p4 ).Draw();
	//		CGLFace( intPt, p2, p3 ).Draw();
	//		glEnd();
	//		glBegin( GL_QUADS );
	//	}
	//	else
	//		CGLFace( p1, p2, p3, p4 ).Draw();
	//}
	//glEnd();

	//// now the top
	//if( !g_bDrawWireframe )
	//{
	//	if( nPts == 3 )
	//	{
	//		glBegin( GL_TRIANGLES );
	//		glVertex3f( (float)top[0].x, (float)top[0].y, (float)top[0].z );
	//		glVertex3f( (float)top[1].x, (float)top[1].y, (float)top[1].z );
	//		glVertex3f( (float)top[2].x, (float)top[2].y, (float)top[2].z );
	//		glEnd();
	//	}else{
	//		glBegin( GL_QUADS );
	//		glVertex3f( (float)top[0].x, (float)top[0].y, (float)top[0].z );
	//		glVertex3f( (float)top[1].x, (float)top[1].y, (float)top[1].z );
	//		glVertex3f( (float)top[2].x, (float)top[2].y, (float)top[2].z );
	//		glVertex3f( (float)top[3].x, (float)top[3].y, (float)top[3].z );		
	//		glEnd();
	//	}
	//	/*CGLTessellatedPolygon poly( top );
	//	if( style == GRAPHIC_LINE || style == GRAPHIC_DASHED_LINE )
	//		poly.SetFilling( FALSE );
	//	poly.Draw();*/
	//	DisableGLTransparency();
	//}

}