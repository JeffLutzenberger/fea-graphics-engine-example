#include "StdAfx.h"
#include "GraphicsMemberLoad.h"
#include "Member.h"
#include "Node.h"
#include "Graphics/SpineGenerator.h"
#include "Graphics/GraphicsHelpers.h"
#include "ShapeDB/CrossSection.h"

#include "ini.h"
#include "IniSize.h"
#include "IniColor.h"
#include "IniFont.h"

CGraphicsMemberLoad::CGraphicsMemberLoad(void):
m_pML( NULL ),
m_etDir( DX ),
m_etType( MEMBER_UNIFORM ),
m_loc( CPoint3D() ),
m_dir( CVector3D() ),
m_textStart( CPoint3D() ),
m_textEnd( CPoint3D() )
{
}

CGraphicsMemberLoad::CGraphicsMemberLoad( const CMemberLoad *pML ):
m_pML( NULL ),
m_etDir( DX ),
m_etType( MEMBER_UNIFORM ),
m_loc( CPoint3D() ),
m_dir( CVector3D() ),
m_textStart( CPoint3D() ),
m_textEnd( CPoint3D() )
{
	SetupMemberLoad( pML );
}

CGraphicsMemberLoad::~CGraphicsMemberLoad(void)
{
}

void CGraphicsMemberLoad::SetupMemberLoad( const CMemberLoad *pML )
{
	ASSERT_RETURN( pML );
	//get the local spine
	CPoint3DArray spine;
	m_pML = pML;
	const CMember* pM = (const CMember*)(&pML->member());
	ASSERT( pM );
	m_etType = pML->type();
	m_etDir = pML->direction();
	
	pM->OrientationMatrix( m_memberOrientation );

	//member orientation matrix
	m_loc.x = pM->node(1)->x();
	m_loc.y = pM->node(1)->y() + pM->centerlineOffset( DY );
	m_loc.z = pM->node(1)->z() + pM->centerlineOffset( DZ );
	m_dir.x = pM->node(2)->x() - pM->node(1)->x();
	m_dir.y = pM->node(2)->y() - pM->node(1)->y();
	m_dir.z = pM->node(2)->z() - pM->node(1)->z();
	m_dir.normalize();
	
}
void CGraphicsMemberLoad::Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, double size, double offset )
{
	//double length = ini.size().memberLoadLength;
	double width = ini.size().memberLoadWidth;
	float length = (float)size;
	if( size <= 0 )
		length = ini.size().memberLoadLength;

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	//translate to the start of the member
	switch( m_etType )
	{
	case MEMBER_UNIFORM:
		if( filter.member.uniformLoads )
			DrawForceLoad( length, width, offset );
		break;
	case MEMBER_LINEAR:
		if( filter.member.linearLoads )
			DrawForceLoad( length, width, offset );
		break;
	case MEMBER_CONCENTRATED:
		if( filter.member.concentratedLoads )
			DrawForceLoad( length, width, offset );
		break;
	case MEMBER_TEMPERATURE:
	case MEMBER_TEMPERATURE_GRADIENT:
		if( filter.member.temperatureLoads )
			DrawTemperatureLoad( length, width, filter.drawDetail );
		break;
	default:
		break;
	}
	//get the modelview matrix
	glPopMatrix();

	ASSERT_RETURN(m_pML);
	if( filter.loadValues && m_pML )
	{
		CString label;
		ETUnit quantity;
		switch( m_pML->type() )
		{
		case MEMBER_CONCENTRATED:
			if( filter.member.concentratedLoads ){
				if( isTranslation( m_pML->direction() ) )
					quantity = FORCE_UNIT;
				else
					quantity = MOMENT_UNIT;
				label = Units::show( quantity, m_pML->magnitude( AT_START ) );
			}
			break;
		case MEMBER_UNIFORM:
			if( filter.member.uniformLoads ){
				if( isTranslation( m_pML->direction() ) )
					quantity = LINEAR_FORCE_UNIT;
				else
					quantity = LINEAR_MOMENT_UNIT;
				label = Units::show( quantity, m_pML->magnitude( AT_START ) );
			}
			break;
		case MEMBER_LINEAR:
			if( filter.member.linearLoads ){
				if( isTranslation( m_pML->direction() ) )
					quantity = LINEAR_FORCE_UNIT;
				else
					quantity = LINEAR_MOMENT_UNIT;
				label = Units::show( quantity, m_pML->magnitude( AT_START ) );
				label += " -> ";
				label += Units::show( quantity, m_pML->magnitude( AT_END ) );
			}
			break;
		case MEMBER_TEMPERATURE:
			if( filter.member.temperatureLoads ){
				quantity = TEMPERATURE_UNIT;
				label = Units::show( quantity, m_pML->magnitude( AT_START ) );
			}
			break;
		case MEMBER_TEMPERATURE_GRADIENT:
			if( filter.member.temperatureLoads ){
				quantity = TEMPERATURE_GRADIENT_UNIT;
				label = Units::show( quantity, m_pML->magnitude( AT_START ) );
			}
			break;
		default:
			break;
		}

		if( label != "" && m_pML->onProjectedLength() )
			label += " (proj.)";

		text->ClearText();
		if( label.GetLength() > 0 )
		{
			text->AddLine( label );
		}

		CPoint p1;
		CPoint p2;
		ASSERT_RETURN( m_pML->member().length() > 0. );
		
		if( GetScreenPtFrom3DPoint( m_textStart, p1 ) && 
			GetScreenPtFrom3DPoint( m_textEnd, p2 ) )
		{
			CPoint3D center3D = (m_textStart + m_textEnd)*0.5;
			CPoint center;
			GetScreenPtFrom3DPoint( center3D, center );
			
			double angle = atan2( -(float)(p2.y - p1.y), (float)(p2.x - p1.x) );
			text->SetHAlignment( CGLOutlineText::HCENTER );
			text->DrawText2D( pDC,
							ini.font().graphWindow.name, 
							ini.font().graphWindow.size, 
							ini.color().memberLoads, 
							center, angle  ); 
		}
	}	
}

void CGraphicsMemberLoad::DrawForceLoad( double length, double width, double offset )
{
	ASSERT_RETURN( m_pML );
	
	//start/end point, load length, number of arrows
	double start = m_pML->offset( AT_START );
	double end = m_pML->offset( AT_END );
	const CMember* pM = (const CMember*)(&m_pML->member());
	ASSERT( pM );

	//local starting offset
	CVector3D dist(m_dir.x*start, m_dir.y*start, m_dir.z*start) ;

	//global starting postion
	CPoint3D startPt3D = m_loc + CPoint3D(dist.x, dist.y, dist.z);

	double loadLength = abs( end-start );

	int nArrows = int(abs(loadLength/length)*1.5);
	if( nArrows < 3 ) nArrows = 3;
	if( nArrows > 10 ) nArrows = 10;
	float delta = float(loadLength/(nArrows));
	if( m_etType == MEMBER_CONCENTRATED )
	{
		if( m_pML->offset( AT_START ) <= 0. )
		{
			delta = 0.;
			nArrows = 1;
		}
		else
		{
			//check for fractional point load and set delta to fraction * pM->length()
			//and narrows to 1/fraction
			delta = 0;
			nArrows = 1;
		}
	}
	
	//set up arrow scaling for linear distributed load
	double startMag = m_pML->magnitude( AT_START );
	double endMag = m_pML->magnitude( AT_END );
	if( m_etType == MEMBER_UNIFORM || m_etType == MEMBER_CONCENTRATED )
		endMag = startMag;
	double max = abs( startMag );
	if( abs( endMag ) > max ) max = abs( endMag );
	
	//so that concentrated loads are visible above uniform loads
	if( nArrows == 1 )
		length *= 1.5;
	double startArrowLength = length;
	double endArrowLength = length;
	
	if( equal( startMag, 0. ) ) startArrowLength = 0.;
	else startArrowLength = length*startMag/max;
	
	if( equal( endMag, 0. ) ) endArrowLength = 0.;
	else endArrowLength = length*endMag/max;

	//determine arrow angle and axis of rotation
	double arrowAngle = 0.;
	CVector3D arrowRotAxis;
	GetArrowRotation( arrowAngle, arrowRotAxis );

	CVector3D xvec( m_memberOrientation[0], m_memberOrientation[1], m_memberOrientation[2] );
	CVector3D yvec( m_memberOrientation[4], m_memberOrientation[5], m_memberOrientation[6] );
	CVector3D zvec( m_memberOrientation[8], m_memberOrientation[9], m_memberOrientation[10] );

	float y_offset = offset;
	
	glLineWidth( (float)ini.size().memberLoadWidth );
				
	if( m_pML->isGlobal() )
	{
		//if load direction is parallel to member then offset the load in the local y direction
		if( ( m_pML->direction() == DX && xvec.is_parallel( CVector3D::X_AXIS ) ) ||
			( m_pML->direction() == DY && xvec.is_parallel( CVector3D::Y_AXIS ) ) ||
			( m_pML->direction() == DZ && xvec.is_parallel( CVector3D::Z_AXIS ) ) )
		{
			startPt3D.x += yvec.x * y_offset;
			startPt3D.y += yvec.y * y_offset;
			startPt3D.z += yvec.z * y_offset;
		}
		for( int i = 0; i <= nArrows; i++ )
		{
			// don't draw an extra arrow
			if( m_etType == MEMBER_CONCENTRATED && i == 0 )
				continue;
			//get arrow location on member
			dist = CVector3D( m_dir.x*delta*i, m_dir.y*delta*i, m_dir.z*delta*i );
			CPoint3D arrowPt = startPt3D + CPoint3D(dist.x, dist.y, dist.z);
			//scale factor for arrow size
			float sf = (float)(startArrowLength/length);
			if( loadLength > 0. )
				sf = float(((endArrowLength - startArrowLength)/loadLength*delta*i + startArrowLength )/length);
			glPushMatrix();
				glTranslatef( (float)arrowPt.x, (float)arrowPt.y, (float)arrowPt.z );
				//now rotate the arrow and draw it
				if( m_pML->direction() == DY  )
					glRotatef( 90.f, 0.f, 0.f, 1.f );
				else if( m_pML->direction() == DZ )
					glRotatef( -90.f, 0.f, 1.f, 0.f );
				else if( m_pML->direction() == RX )
					glRotatef( 90.f, 0.f, 1.f, 0.f );
				else if( m_pML->direction() == RY )
					glRotatef( -90.f, 1.f, 0.f, 0.f );
				glTranslatef( -sign(sf)*y_offset, 0, 0 );
				if( isRotation( m_pML->direction() ) )
					DrawLineMomentArrow( float( length*sf ), float( width ) );
				else
					DrawLineArrow( float( length*sf-sign(sf)*y_offset ), float( width ) );
			glPopMatrix();
		}
		//now do the line across the top...
		CPoint3D p1(0., 0., 0.);
		CPoint3D p2( xvec.x*loadLength, xvec.y*loadLength, xvec.z*loadLength );	
		glPushMatrix();
		glTranslatef( (float)startPt3D.x, (float)startPt3D.y, (float)startPt3D.z );
		//now rotate
		if( m_pML->direction() == DX || m_pML->direction() == RX ){
			p1.x -= startArrowLength;
			p2.x -= endArrowLength;
			m_textStart = p1 + startPt3D;
			m_textEnd = p2 + startPt3D;
			m_textStart.x += length*0.05;
			m_textEnd.x += length*0.05;
		}else if( m_pML->direction() == DY  || m_pML->direction() == RY ){
			p1.y -= startArrowLength;
			p2.y -= endArrowLength;
			m_textStart = p1 + startPt3D;
			m_textEnd = p2 + startPt3D;
			m_textStart.y += length*0.05;
			m_textEnd.y += length*0.05;
		}
		else if( m_pML->direction() == DZ || m_pML->direction() == RZ ){
			p1.z -= startArrowLength;
			p2.z -= endArrowLength;
			m_textStart = p1 + startPt3D;
			m_textEnd = p2 + startPt3D;
			m_textStart.z += length*0.05;
			m_textEnd.z += length*0.05;
		}
		if( !isRotation( m_pML->direction() ) )
		{
			glBegin( GL_LINES );
			glVertex3f( (float)p1.x, (float)p1.y, (float)p1.z );
			glVertex3f( (float)p2.x, (float)p2.y, (float)p2.z );
			glEnd();
		}
		glPopMatrix();
	}
	else
	{
		glPushMatrix();
			glTranslatef( (float)startPt3D.x, (float)startPt3D.y, (float)startPt3D.z );
			glMultMatrixf( m_memberOrientation );
			for( int i = 0; i <= nArrows; i++ )
			{
				// don't draw an extra arrow
				if( m_etType == MEMBER_CONCENTRATED && i == 0 )
					continue;
				//get arrow location on member
				dist = CVector3D( m_dir.x*delta*i, m_dir.y*delta*i, m_dir.z*delta*i );
				CPoint3D arrowPt = CPoint3D(dist.x, dist.y, dist.z);
				//scale factor for arrow size
				float sf = (float)(startArrowLength/length);
				if( loadLength > 0. )
					sf = float(((endArrowLength - startArrowLength)/loadLength*delta*i + startArrowLength )/length);
				glPushMatrix();
					glTranslatef( (float)(delta*i), 0.f, 0.f );
					//now rotate the arrow and draw it
					if( m_pML->direction() == DY  )
						glRotatef( 90.f, 0.f, 0.f, 1.f );
					else if( m_pML->direction() == DZ )
						glRotatef( -90.f, 0.f, 1.f, 0.f );
					else if( m_pML->direction() == RX )
						glRotatef( 90.f, 0.f, 1.f, 0.f );
					else if( m_pML->direction() == RY )
						glRotatef( -90.f, 1.f, 0.f, 0.f );
					glTranslatef( -sign(sf)*y_offset, 0, 0 );
					//if it's a traction load move the load so it's not 
					//right on the member to make selection easier
					if( m_pML->direction() == DX )
						glTranslatef( 0, y_offset, 0 );
					if( isRotation( m_pML->direction() ) )
						DrawLineMomentArrow( float( length*sf ), float( width ) );
					else
						DrawLineArrow( float( length*sf-sign(sf)*y_offset ), float( width ) );
				glPopMatrix();
			}
			//now do the line across the top...
			if( !isRotation( m_pML->direction() ) && m_pML->direction() != DX ){
				glPushMatrix();
					if( m_pML->direction() == DZ )
						glRotatef( 90.f, 1., 0., 0. );
					CPoint3D p1(0., -(float)startArrowLength, 0.);
					CPoint3D p2( (float)loadLength, -(float)endArrowLength, 0. );
					//glTranslatef( 0, -y_offset, 0 );
					glBegin( GL_LINES );
					glVertex3f( (float)p1.x, (float)p1.y, (float)p1.z );
					glVertex3f( (float)p2.x, (float)p2.y, (float)p2.z );
					glEnd();
				glPopMatrix();
			}
		glPopMatrix();
		//get text point
		glPushMatrix();
			glLoadIdentity();
			glTranslatef( (float)startPt3D.x, (float)startPt3D.y, (float)startPt3D.z );
			glMultMatrixf( m_memberOrientation );
			if( m_pML->direction() == DZ )
				glRotatef( 90.f, 1., 0., 0. );
			double mv[16] = {0};
			glGetDoublev( GL_MODELVIEW_MATRIX, mv );
			CPoint3D p1(0., -(float)(startArrowLength-length*0.05), 0.);
			CPoint3D p2( (float)loadLength, -(float)(endArrowLength-length*0.05), 0. );
			glTranslatef( 0, -y_offset, 0 );
			CMatrix44 matrix( mv );
			CMatrix44 inv( mv );
			matrix.Inverse( inv );
			inv.RightMultiply( p1, m_textStart );
			inv.RightMultiply( p2, m_textEnd );
			m_textStart = m_textStart + startPt3D;
			m_textEnd = m_textEnd + startPt3D;
		glPopMatrix();
	}
}

void CGraphicsMemberLoad::DrawTemperatureLoad( double length, double width, ETGraphicDetail detail )
{
	ASSERT_RETURN( m_pML );
	
	const CMember* pM = (const CMember*)(&m_pML->member());
	ASSERT( pM );

	CPoint3D pt3D1( pM->node(1)->x(), 
		pM->node(1)->y() + pM->centerlineOffset( DY ), 
		pM->node(1)->z() + pM->centerlineOffset( DZ ) );
	
	CPoint3D pt3D2( pM->node(2)->x(), 
		pM->node(2)->y() + pM->centerlineOffset( DY ), 
		pM->node(2)->z() + pM->centerlineOffset( DZ ) );

	CPoint3D center = pt3D1 + pt3D2;
	center.x *= 0.5;
	center.y *= 0.5;
	center.z *= 0.5;
	
	glTranslatef( (float)center.x, (float)center.y, (float)center.z );

	glMultMatrixf( m_memberOrientation );

	//set up arrow scaling, just +1 or -1 for temperature loads
	double startMag = m_pML->magnitude( AT_START );
	double max = abs( startMag );
	ASSERT( max > 0. );
	float sf = (float)(startMag/max);
	CPoint3DArray translations;

	int n = 1;
	if( m_etType == MEMBER_TEMPERATURE_GRADIENT )
		n = 2;

	for( int i = 0; i < n; i++ )
	{
		glPushMatrix();

		if( m_etType == MEMBER_TEMPERATURE_GRADIENT )
		{
			double arrowAngle = 0.;
			CVector3D arrowRotAxis;
			GetArrowRotation( arrowAngle, arrowRotAxis );
			glRotatef( float(arrowAngle*180/M_PI), (float)arrowRotAxis.x, 
				                                   (float)arrowRotAxis.y, 
				                                   (float)arrowRotAxis.z );
		}
		if( i == 1 )
		{
			glRotatef( 180.f, 1.f, 0.f, 0.f );
			sf*=-1;
		}

		float transW = (float)(2*width);
		float transL = (float)length;
		if( sf > 0. )
			transL = float(2*length);

		switch( detail )
		{
		case DETAIL_LOW:
			glTranslatef( transL, transW, 0.f );
			DrawLineArrow( float( length*sf ), float( width ) );
			glTranslatef( -transL, 0.f, 0.f );
			glRotatef( 180.f, 0.f, 1.f, 0.f );
			glTranslatef( transL, 0.f, 0.f );
			DrawLineArrow( float( length*sf ), float( width ) );
			break;
		case DETAIL_MED:
		case DETAIL_HI:
			{
			CCrossSection sec( &(m_pML->member()) );
			CPoint3DArray dummy;
			sec.GetCrossSection( dummy, true ); 
			transW = 2.f*sec.GetYMax();
			glTranslatef( transL, transW, 0.f );
			DrawLineArrow( float( length*sf ), float( width ) );
			glTranslatef( -transL, 0.f, 0.f );
			glRotatef( 180.f, 0.f, 1.f, 0.f );
			glTranslatef( transL, 0.f, 0.f );
			DrawLineArrow( float( length*sf ), float( width ) );
			break;
			}
		default:
			break;
		}
		CPoint p2D;
		GetScreenPtFrom3DPoint( CPoint3D( 0., -2*transW, 0. ), p2D );
		m_textStart = pt3D1;
		m_textEnd = pt3D2;
		glPopMatrix();
	}
}

void CGraphicsMemberLoad::GetArrowRotation( double& angle, CVector3D& rot_axis )
{
	if( m_pML->direction() == DX || m_pML->direction() == RZ )
	{
		angle = 0.;
		rot_axis = CVector3D( 0., 1., 0. );
	}
	else if( m_pML->direction() == DY  )
	{
		angle = M_PI*0.5f;
		rot_axis = CVector3D( 0., 0., 1. );
	}
	else if( m_pML->direction() == DZ )
	{
		angle = -M_PI*0.5f;
		rot_axis = CVector3D( 0., 1., 0. );
	}
	else if( m_pML->direction() == RX )
	{
		angle = M_PI*0.5f;
		rot_axis = CVector3D( 0., 1., 0. );
	}
	else if( m_pML->direction() == RY )
	{
		angle= -M_PI*0.5f;
		rot_axis = CVector3D( 1., 0., 0. );
	}
	else
	{
		ASSERT( false );
		angle = 0.;
		rot_axis = CVector3D( 1., 0., 0. );
	}

}
