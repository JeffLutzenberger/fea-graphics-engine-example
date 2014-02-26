#include "IESDataStdAfx.h"
#include "Graphics/GraphicsHelpers.h"
#include "GraphicsCable.h"
#include "ShapeDB/CrossSection.h"
#include "Graphics/SpineGenerator.h"
#include "iniGraphics.h"
#include "Node.h"

#ifdef _DEBUG
	#undef THIS_FILE
	static char THIS_FILE[]=__FILE__;
	#define new DEBUG_NEW
#endif

void getCableExtremeLabel( CWindowFilter& filter, CString& extremeLabel );

CGraphicsCable::CGraphicsCable( const CCable* pC, const CResultCase* pRC, double scale, bool bSetupExtrusion ) :
m_pC( pC ),
m_pRC( pRC ),
m_HiLOD( NULL ),
m_MedLOD( NULL ),
m_scale( scale ),
m_Start( CPoint3D( 0., 0., 0. ) ),
m_bHaveExtrusions( false )
{
	ASSERT( pC );
	SetupOrientation();
	if( bSetupExtrusion )
		SetupHiQualityExtrusion( );
	// save the time stamp of this object so we know when to redo it
	m_timeCreated = pC->UITime();
	// create the extruded shape
}

CGraphicsCable::CGraphicsCable( const CCable* pC, CGraphicsCable* pSimilarGraphicsCable ) :
m_pC( pC ),
m_pRC( NULL ),
m_HiLOD( NULL ),
m_MedLOD( NULL ),
m_scale( 0. ),
m_Start( CPoint3D( 0., 0., 0. ) ),
m_bHaveExtrusions( false )
{
	ASSERT( pC );
	SetupOrientation();
	m_HiLOD = new CMemberExtrusion( pC, pSimilarGraphicsCable->HiLOD() );
	// save the time stamp of this object so we know when to redo it
	m_timeCreated = pC->UITime();
}

CGraphicsCable::~CGraphicsCable()
{
	if( m_HiLOD )
		delete m_HiLOD;
	if( m_MedLOD )
		delete m_MedLOD;
}

CGraphicsCable & CGraphicsCable:: operator = ( const CGraphicsCable &c )
{
	m_pC = c.m_pC;
	m_pRC = c.m_pRC;
	m_HiLOD = c.m_HiLOD;
	m_MedLOD = c.m_MedLOD;
	m_spine = c.m_spine;
	m_Start = c.m_Start;
	m_scale = c.m_scale;
	m_bHaveExtrusions = c.m_bHaveExtrusions;
	m_timeCreated = c.m_timeCreated;
	return *this;
}

void CGraphicsCable::Draw( const CWindowFilter& rFilter )
{
	ASSERT_RETURN( m_pC ); 
	if( m_pC->length() <= 0. )
		return;

	float local_axes_offsety = ini.graphics().fNarrowWidth + float(GetLocalMinY());

	glPushMatrix();
		if( rFilter.pictureView )
		{
			DrawCableHi( );
			if( m_HiLOD )
				local_axes_offsety = -(float)(1.25*m_HiLOD->GetYMax()) + float(GetLocalMinY());
		}
		else
		{
			DrawCableLow( );
		}
		/////////////////////////////////////////////////
		// local axes
		/////////////////////////////////////////////////
		if( rFilter.cable.localAxes ){
			//glDepthRange( 0, 0.8 );
			float lineWidth = 0;
			glGetFloatv(GL_LINE_WIDTH, &lineWidth );
			glLineWidth( ini.graphics().fNarrowWidth );
			glPushAttrib( GL_LIGHTING );
			glPushMatrix();
			glTranslatef( float(m_Start.x), float(m_Start.y), float(m_Start.z) );
			glTranslatef( float(m_pC->length()*0.5), local_axes_offsety, 0.f);
			DrawOriginArrows( ini.graphics().fShortLength );
			glPopMatrix();
			glPopAttrib();
			glLineWidth( lineWidth );
			//glDepthRange( 0, 1 );
		}
	glPopMatrix();
}

void CGraphicsCable::DrawText( CDC* pDC, CWindowFilter& rFilter, CGLText* text  )
{
	ASSERT_RETURN( m_pC );
	if( !pDC )
		return;

	//now draw text...
    //check for text...return
	char buf1[512] = "";
	char buf2[512] = "";
	GetCablePropertyBuffers( buf1, buf2, rFilter );	
	if( lstrlen( buf1 ) < 0 &&  lstrlen( buf2 ) < 0 )
		return;

	//we've got text...
	const CNode* pN1 = m_pC->node(1);
	const CNode* pN2 = m_pC->node(2);
	
	ASSERT_RETURN( pN1 && pN2 );

	CPoint3D pt3D1( pN1->x(), pN1->y(), pN1->z());
	CPoint3D pt3D2( pN2->x(), pN2->y(), pN2->z());
	CLine3D l( pt3D1, pt3D2 );
	CPoint3D center = (pt3D1 + pt3D2)*0.5;
	
	if( lstrlen( buf2 ) > 0 ) {  // display bottom buffer
		text->AddLine( buf2 );
	}

	CPoint p1;
	CPoint p2;
	CPoint centerPt;
	if( GetScreenPtFrom3DPoint( pt3D1, p1 ) &&
		GetScreenPtFrom3DPoint( pt3D2, p2 ) &&
		GetScreenPtFrom3DPoint( center, centerPt ) )
	{
		double angle = atan2( -(float)(p1.y - p2.y), (float)(p1.x - p2.x) );
		text->SetHAlignment( CGLText::HCENTER );
		//top buffer
		if( lstrlen( buf1 ) > 0 )
		{
			text->ClearText();
			if( rFilter.pictureView )
			{
				//offset buffer 1 up away from the member
				float ymax = fabs((float)(m_pC->mpGraphicsCable->GetYMax()));
				if( zero(ymax) ) 
					ymax = 1.;
				float pt3D_dist = (float)pt3D1.distance_to(pt3D2);
				if( pt3D_dist > 0 )
				{
					float pixelScale = sqrt((float)((p1.x-p2.x)*(p1.x-p2.x))+(float)((p1.y-p2.y)*(p1.y-p2.y)))/pt3D_dist;
					float pixelYMax = pixelScale*ymax;
					//now offset p1 and p2
					CVector2D v( CPoint2D( p1.x, p1.y), CPoint2D( p2.x, p2.y ) );
					v = v.GetNormalized();
					v = v*pixelYMax;
					v = v.GetRotated( M_PI/4 );
					centerPt.x += (int)v.x;
					centerPt.y += (int)v.y;
				}
			}		
			text->AddLine( buf1 );
			text->SetVAlignment( CGLOutlineText::VBOTTOM );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
				ini.font().graphWindow.size/*m_printScale*/, 
				ini.color().nodes, centerPt, angle  );
		}

		//bottom buffer
		if( lstrlen( buf2 ) > 0 )
		{
			text->ClearText();
			if( rFilter.pictureView )
			{
				//offset buffer 1 up away from the member
				float ymax = fabs((float)(m_pC->mpGraphicsCable->GetYMax()));
				if( zero(ymax) ) 
					ymax = 1.;
				float pt3D_dist = (float)pt3D1.distance_to(pt3D2);
				if( pt3D_dist > 0 )
				{
					float pixelScale = sqrt((float)((p1.x-p2.x)*(p1.x-p2.x))+(float)((p1.y-p2.y)*(p1.y-p2.y)))/pt3D_dist;
					float pixelYMax = pixelScale*ymax;
					//now offset p1 and p2
					CVector2D v( CPoint2D( p1.x, p1.y), CPoint2D( p2.x, p2.y ) );
					v = v.GetNormalized();
					v = v*pixelYMax;
					v = v.GetRotated( M_PI/4 );
					centerPt.x -= 2*(int)v.x;
					centerPt.y -= 2*(int)v.y;
				}
			}		
			text->AddLine( buf2 );
			text->SetVAlignment( CGLOutlineText::VTOP );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
				ini.font().graphWindow.size/*m_printScale*/, 
				ini.color().nodes, centerPt, angle  );
		}
		text->SetHAlignment( CGLText::HLEFT );
		text->SetVAlignment( CGLText::VTOP );
	}
}
void CGraphicsCable::Draw(CDC* pDC, CWindowFilter& rFilter, CGLText* text )
{
	ASSERT_RETURN( m_pC );

	Draw( rFilter );
	DrawText( pDC, rFilter, text );
}

void CGraphicsCable::DrawResultDiagram( const CWindowFilter& /*rFilter*/ )
{
	if( m_pC->length() <= 0. )
		return;
	glPushMatrix();
	//glTranslatef( float(m_Start.x), float(m_Start.y), float(m_Start.z) );
	//glRotatef(	(float)m_RotationAngle*(float)(180./M_PI), 
	//	(float)m_RotationAxis.x, (float)m_RotationAxis.y, (float)m_RotationAxis.z );
	float local_axes_offsety = 0.f;
	//if( rFilter.drawDetail == DETAIL_HI || rFilter.drawDetail == DETAIL_MED )
	//	local_axes_offsety = (float)m_MedLOD->GetYMax();
	glTranslatef( 0.f, -(float)(1.25*local_axes_offsety), 0.f);
	int nPoints = m_spine.size();
	std::vector<double> texCoords;
	//if( m_HiLOD )
	//	texCoords = m_HiLOD->GetSpineTexCoords();
	int texPts = m_spineTexCoords.size();
	glBegin( GL_QUADS );
	for( int i = 0; i < nPoints-1; i++ )
	{
		if( i < texPts ){
			CPoint3D p1 = m_spine[i];
			CPoint3D p2 = m_spine[i];
			CPoint3D p3 = m_spine[i+1];
			CPoint3D p4 = m_spine[i+1];
			p1.y += (float)(ini.graphics().fLongLength*m_spineTexCoords[i]);
			p4.y += (float)(ini.graphics().fLongLength*m_spineTexCoords[i+1]);
			glVertex3f( (float)p1.x, (float)p1.y, (float)p1.z );
			glVertex3f( (float)p2.x, (float)p2.y, (float)p2.z );
			glVertex3f( (float)p3.x, (float)p3.y, (float)p3.z );
			glVertex3f( (float)p4.x, (float)p4.y, (float)p4.z );
		}
	}
	glEnd( );
	glPopMatrix( );
}
void CGraphicsCable::DrawCableLow( )
{
	glBegin( GL_LINES );
	int nPoints = m_spine.size();
	std::vector<double> texCoords;
	//if( m_MedLOD )
	//	texCoords = m_MedLOD->GetSpineTexCoords();
	int texPts = m_spineTexCoords.size();
	for( int i = 0; i < nPoints-1; i++ )
	{
		CPoint3D p1 = m_spine[i];
		CPoint3D p2 = m_spine[i+1];
		if( i < texPts )
			glTexCoord2f( (float)m_spineTexCoords[i], 0.f );
		glVertex3f( (float)p1.x, (float)p1.y, (float)p1.z );
		if( i < texPts )
			glTexCoord2f( (float)m_spineTexCoords[i+1], 0.f );
		glVertex3f( (float)p2.x, (float)p2.y, (float)p2.z );
	}
	glEnd( );
}

void CGraphicsCable::DrawCableMed( )
{
	if( m_MedLOD )
		m_MedLOD->Draw();
	else
		DrawCableLow();
}

void CGraphicsCable::DrawCableHi( )
{
	if( m_HiLOD )
		m_HiLOD->Draw();
	else{
		SetupHiQualityExtrusion( );
		if( m_HiLOD ) 
			m_HiLOD->Draw();
		else DrawCableLow();
	}
}

bool CGraphicsCable::SetupExtrusions( )
{
	if( m_bHaveExtrusions )
		m_bHaveExtrusions = false;
	if( m_HiLOD ){
		delete m_HiLOD;
		m_HiLOD = NULL;
	}
	if( m_MedLOD ){
		delete m_MedLOD;
		m_MedLOD = NULL;
	}
	bool bHiSuccess = false;
	bool bMedSuccess = false;
	bHiSuccess = SetupHiQualityExtrusion( );
	bMedSuccess = SetupMedQualityExtrusion( );
	if( bHiSuccess && bMedSuccess )
		m_bHaveExtrusions = true;
	return m_bHaveExtrusions;
}

bool CGraphicsCable::SetupHiQualityExtrusion( )
{
	ASSERT( m_pC && m_spine.size() > 0 );
	m_HiLOD = new CMemberExtrusion( m_pC, m_spine, true );
	if( m_HiLOD )
		return true;
	else return false;
}
bool CGraphicsCable::SetupMedQualityExtrusion( )
{
	ASSERT( m_pC && m_spine.size() > 0 );
	m_MedLOD = new CMemberExtrusion( m_pC, m_spine, true );
	if( m_MedLOD )
		return true;
	else return false;
}

void CGraphicsCable::SetupOrientation( )
{

	CNode* pN1;
	pN1 = (CNode*)m_pC->node(1);
	if( !pN1 )
		return;

	CNode* pN2;
  	pN2 = (CNode*)m_pC->node(2);
	if( !pN2 )
		return;

	bool deformed = false;
	int nPoints = 10; 
	if( m_pRC )
	{
		deformed = true;
		nPoints = 10;//mFilter.Cable.resultPoints;
	}
	//get the local spine
	m_spine.clear( );
	CSpineGenerator sg( m_pC, m_pRC );
	if( !sg.GetSpine( deformed, nPoints, 0., m_scale, m_spine ) )
	{
		ASSERT( false );
		return;
	}

	m_Start.x = (float)pN1->x();
	m_Start.y = (float)pN1->y();
	m_Start.z = (float)pN1->z();

}

void CGraphicsCable::SetSpineTextureCoords( double min, double max, int GraphType, const CResult* pR, bool bDiagram )
{
	ASSERT_RETURN( pR );
	int nCrossSections = m_spine.size();
	m_spineTexCoords.clear();
	
	//int texCoordCount = 1;

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
			xValue1 = pR->stress( ETCableStress(-GraphType), iPoint1+1 );
			xValue2 = pR->stress( ETCableStress(-GraphType), iPoint2+1 );
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
	}
	//use the max value for the cable like we did in VA 5.5
	for( int i = 0; i < nCrossSections; i++ ) 
	{ 
		m_spineTexCoords.push_back( maxValue );
	}

}

double CGraphicsCable::GetLocalMinY( )
{
	double minY = 0.;
	for( unsigned int i = 0; i < m_spine.size(); i++ )
		if( m_spine[i].y < minY ) minY = m_spine[i].y;
	return minY;
}

double CGraphicsCable::distortionFactor()
{
	return m_scale;
}

void CGraphicsCable::SetupTextureCoords( double min, double max, int theGraphType, const CResult* pR, bool bDiagram )
{
	SetSpineTextureCoords( min, max, theGraphType, pR, bDiagram );
	if( m_HiLOD )
		m_HiLOD->SetExtrusionTextureCoords( min, max, theGraphType, pR, bDiagram );
	//if( m_MedLOD )
	//	m_MedLOD->SetExtrusionTextureCoords( min, max, theGraphType, pR, bDiagram );
	//set colors for the low detail model
}

bool CGraphicsCable::isSynchedWithDataCable()
{
	if( !m_pC ) return false;
	else return ( m_pC->UITime() == m_timeCreated );
}

void CGraphicsCable::SynchWithDataCable()
{
	if( !isSynchedWithDataCable() ){
		ASSERT_RETURN( m_pC );
		SetupOrientation();
		// save the time stamp of this object so we know when to redo it
		m_timeCreated = m_pC->UITime();
		//CTime
		SetupExtrusions();
	}
}

double CGraphicsCable::GetYMax()
{
	if( m_HiLOD )
		return m_HiLOD->GetYMax();
	else return 0.f;
}

void CGraphicsCable::GetCablePropertyBuffers( char* buf1, char* buf2, CWindowFilter& filter )
{
	// set up text for cable property display (two buffers)
	ASSERT_RETURN( m_pC );

	int fillBufNumber = 1;
	ETShowUnit units = filter.showUnits ? SHOW_UNIT : NO_UNIT;
	
	bool bHasText = false;
	bool bHasExtremeLabel = false;
	if( MODEL_WINDOW == filter.windowType ){
		bHasText = ( filter.cable.name || filter.cable.properties ||
			filter.cable.material || filter.cable.catenaryLength ||
			filter.cable.straightLength );
	}
	else if( POST_WINDOW == filter.windowType ){
		bHasText = filter.cable.name; 
		bHasExtremeLabel = filter.cable.extremeLabel && 
			filter.cable.GetResultType() != CABLE_NO_RESULT &&
			(filter.cable.coloredResults || filter.cable.diagramResults);
	}

	if( bHasExtremeLabel && filter.resultCase())
	{
		//const CResult* pResult = filter.resultCase()->result( *m_pC );
		CString extremeLabel;
		getCableExtremeLabel( filter, extremeLabel );
		addToBuffers( buf1, buf2, 256, &fillBufNumber, extremeLabel );
	}
	if( filter.cable.name )  // label the cable
	{   
		addToBuffers( buf1, buf2, 256, &fillBufNumber, m_pC->name() );
	}

	if( filter.cable.properties )
	{
		addToBuffers( buf1, buf2, 256, &fillBufNumber,
			m_pC->property().list( CProperty::REPORT_SECTION, 0, 0, 0, units ) );
	}
	if( filter.cable.weight ) {
		char buf[132] = {0};
		sprintf( buf, "W=%s", Units::show( FORCE_UNIT, m_pC->weight(), units) );
		addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
	}
	if( filter.cable.material  )
	{
		addToBuffers( buf1, buf2, 256, &fillBufNumber,
			m_pC->material().list( CMaterial::REPORT_TYPE, 0, 0, 0, units ) );
	}
	if( filter.cable.catenaryLength  )
	{
		char buf[132] = {0};
		sprintf( buf, "Cat. L=%s", Units::show( LENGTH_LARGE_UNIT, m_pC->catenaryLength(), units) );
		addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
	}
	if( filter.cable.straightLength  )
	{
		char buf[132] = {0};
		sprintf( buf, "Str. L=%s", Units::show( LENGTH_LARGE_UNIT, m_pC->length(), units) );
		addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
	}

	//if( lstrlen( buf1 ) > 0 ) {  // display top buffer
	//	int ix, iy;
	//	ix = (int)( x + double(mFontPixelHeight+2*ini.graphics().fNarrowWidth)*sin( angle )/2. );
	//	iy = (int)( y - double(mFontPixelHeight+2*ini.graphics().fNarrowWidth)*cos( angle )/2. );
	//	AngledTextOut( mpDC, ix, iy, buf1, lstrlen( buf1 ), angle );
	//}
	//if( lstrlen( buf2 ) > 0 ) {  // display bottom buffer
	//	int ix, iy;
	//	ix = (int)( x - double(mFontPixelHeight+2*ini.graphics().fNarrowWidth/3)*sin( angle )*3./2. );
	//	iy = (int)( y + double(mFontPixelHeight+2*ini.graphics().fNarrowWidth/3)*cos( angle )*3./2. );
	//	AngledTextOut( mpDC, ix, iy, buf2, lstrlen( buf2 ), angle );
	//}
	//if( !theStructure.isATruss() && ( filter.cable.localAxes || toggleSelect ) )
	//{
	//	COLORREF color = ini.color().memberAxes;
	//	if( toggleSelect )
	//		color = mpDC->GetTextColor();
	//	double dLoc[3], dCos[3];
	//	dLoc[0] = pN1->x() + xo + ( pN2->x() - pN1->x() )/3.;
	//	dLoc[1] = pN1->y() + yo + ( pN2->y() - pN1->y() )/3.;
	//	dLoc[2] = pN1->z() + zo + ( pN2->z() - pN1->z() )/3.;
	//	// local member X-axis
	//	dCos[0] = 1.;
	//	dCos[1] = 0.;
	//	dCos[2] = 0.;
	//	if( pM->localToGlobal( dCos ) ) Arrow( dLoc, dCos, (int)(mIniSizeFactor*ini.graphics().fShortLength),
	//		mIniSizeFactor*ini.graphics().fNarrowWidth, color, "x" );
	//	// local member Y-axis
	//	dCos[0] = 0.;
	//	dCos[1] = 1.;
	//	dCos[2] = 0.;
	//	if( pM->localToGlobal( dCos ) ) Arrow( dLoc, dCos, (int)(mIniSizeFactor*ini.graphics().fShortLength),
	//		mIniSizeFactor*ini.graphics().fNarrowWidth, color, "y" );
	//	// local member Z-axis
	//	if( theStructure.type() != PLANE_FRAME ) {
	//		dCos[0] = 0.;
	//		dCos[1] = 0.;
	//		dCos[2] = 1.;
	//		if( pM->localToGlobal( dCos ) ) Arrow( dLoc, dCos, (int)(mIniSizeFactor*ini.graphics().fShortLength),
	//			mIniSizeFactor*ini.graphics().fNarrowWidth, color, "z" );
	//	}
	//}

	return;
}	// end CModelView::showCableProperties()

void getCableExtremeLabel( CWindowFilter& filter, CString& extremeLabel )
{
	int theGraphType = filter.cable.GetGraphType();
	extremeLabel = "N/A";
	//CHECK_IF( pResult )
	//{
		CString temp;
		// filter results
		//if( (m_Filter.member.coloredResults || m_Filter.member.diagramResults) && m_Filter.member.GetResultType() != MEMBER_NO_RESULT )
		// axial force
		double max = 0;
		double min = 0;
		if( theGraphType == 6 ) 
		{ 
			min = filter.resultCase()->maximumCableForce();//maxForce( DX, -1 );
			max = filter.resultCase()->minimumCableForce();//maxForce( DX, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( FORCE_UNIT, max );
			else extremeLabel = Units::show( FORCE_UNIT, min );
		}
		// axial stress
		else if( theGraphType == 0 ) 
		{ 
			min = filter.resultCase()->maximumCableStress();//maxForce( DY, -1 );
			max = filter.resultCase()->minimumCableStress();//maxForce( DY, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( FORCE_UNIT, max );
			else extremeLabel = Units::show( FORCE_UNIT, min );
		}
		// displacements
		else if( theGraphType == 100 )
		{
			min = filter.resultCase()->maximumCableDisplacement( DX );//maxDisplacement( DX,-1 );
			max = filter.resultCase()->minimumCableDisplacement( DX );//maxDisplacement( DX, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( LENGTH_SMALL_UNIT, max );
			else extremeLabel = Units::show( LENGTH_SMALL_UNIT, min );
		}
		else if( theGraphType == 101 )
		{
			min = filter.resultCase()->maximumCableDisplacement( DY );//maxDisplacement( DX,-1 );
			max = filter.resultCase()->minimumCableDisplacement( DY );//maxDisplacement( DX, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( LENGTH_SMALL_UNIT, max );
			else extremeLabel = Units::show( LENGTH_SMALL_UNIT, min );
		}
		else if( theGraphType == 102 )
		{
			min = filter.resultCase()->maximumCableDisplacement( DZ );//maxDisplacement( DX,-1 );
			max = filter.resultCase()->minimumCableDisplacement( DZ );//maxDisplacement( DX, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( LENGTH_SMALL_UNIT, max );
			else extremeLabel = Units::show( LENGTH_SMALL_UNIT, min );
		}
	//} //CHECK_IF
}