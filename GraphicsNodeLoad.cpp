#include "StdAfx.h"

#include "GraphicsNodeLoad.h"

#include "Graphics/GraphicsHelpers.h"

#include "Model.h"
#include "Node.h"
#include "Units/Units.h"
#include "datautil.h"

CGraphicsNodeLoad::CGraphicsNodeLoad( const CNodalLoad* pNL ):
m_etDir( DY ),
m_loc( CPoint3D() ),
m_textPoint ( CPoint() ),
m_axis( CVector3D() ),
m_rot( 0.f ),
m_pNL( pNL )
{
	SetupNodeLoad( );
}
CGraphicsNodeLoad::~CGraphicsNodeLoad()
{
}

//void CGraphicsNodeLoad::Draw( )
//{
//	ASSERT_RETURN(m_pNL);
//
//	float w = (float)ini.size().nodalLoadWidth;
//	float l = (float)ini.size().nodalLoadLength;
//	glMatrixMode( GL_MODELVIEW );
//	glPushMatrix();
//	glTranslatef( (float)m_loc.x, (float)m_loc.y, (float)m_loc.z );
//	glRotatef( float(m_rot*180/M_PI), (float)(m_axis.x), (float)(m_axis.y), (float)(m_axis.z) );
//	if( isTranslation( m_etDir ) )
//		DrawLineArrow( l, w );
//	else
//		DrawSolidMomentArrow( l, w );
//	glPopMatrix();
//}

void CGraphicsNodeLoad::Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, double size, double offset  )
{
	ASSERT_RETURN( m_pNL && pDC );

	float w = (float)ini.size().nodalLoadWidth;
	float l = (float)size;
	if( size <= 0 )
		l = ini.size().nodalLoadLength;
	//get the label
	CString label;
	ETUnit quantity;
	text->ClearText();
	if( filter.loadValues )
	{
		if( m_pNL->type() == NODAL_FORCE )
		{
			if( isTranslation( m_pNL->direction() ) )
				quantity = FORCE_UNIT;
			else
				quantity = MOMENT_UNIT;
			label = Units::show(  quantity , m_pNL->magnitude() );
		}
		else if( m_pNL->type() == NODAL_SETTLEMENT )
		{
			if( isTranslation( m_pNL->direction() ) )
				quantity = LENGTH_SMALL_UNIT;
			else
				quantity = ANGLE_UNIT;
			label = Units::show(  quantity , m_pNL->magnitude() );
		}
		text->AddLine( label );
	}
	
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
		glTranslatef( (float)m_loc.x, (float)m_loc.y, (float)m_loc.z );
		glRotatef( float(m_rot*180/M_PI), (float)(m_axis.x), (float)(m_axis.y), (float)(m_axis.z) );
		if( isTranslation( m_etDir ) )
		{   glTranslatef( -offset, 0, 0 );
			DrawLineArrow( l, w, pDC, text,
						ini.font().graphWindow.name, 
						ini.font().graphWindow.size, 
						ini.color().nodalLoads); 
		}
		else
			DrawSolidMomentArrow( l, w, pDC, text,
						ini.font().graphWindow.name, 
						ini.font().graphWindow.size, 
						ini.color().nodalLoads); 
	glPopMatrix();
}

void CGraphicsNodeLoad::SetupNodeLoad( )
{

	double dLoc[3];
	dLoc[0] = m_pNL->node().x();
	dLoc[1] = m_pNL->node().y();
	dLoc[2] = m_pNL->node().z();
	m_etDir = m_pNL->direction();
	m_loc.x = dLoc[0];
	m_loc.y = dLoc[1];
	m_loc.z = dLoc[2];
	bool bNeg = false;
	if( m_pNL->magnitude() < 0 )
		bNeg = true;

	if( m_pNL->direction() == DX || m_pNL->direction() == RZ )
	{
		m_rot = (float)(0.);
		if( bNeg )
			m_rot += (float)M_PI;
		m_axis = CVector3D( 0., 1., 0. );
	}
	else if( m_pNL->direction() == DY  )
	{
		m_rot = (float)(M_PI*0.5f);
		if( bNeg )
			m_rot += (float)M_PI;
		m_axis = CVector3D( 0., 0., 1. );
	}
	else if( m_pNL->direction() == DZ )
	{
		m_rot = (float)(-M_PI*0.5f);
		if( bNeg )
			m_rot += (float)M_PI;
		m_axis = CVector3D( 0., 1., 0. );
	}
	else if( m_pNL->direction() == RX )
	{
		m_rot = (float)(M_PI*0.5f);
		if( bNeg )
			m_rot += (float)M_PI;
		m_axis = CVector3D( 0., 1., 0. );
	}
	else if( m_pNL->direction() == RY )
	{
		m_rot= (float)(-M_PI*0.5f);
		if( bNeg )
			m_rot += (float)M_PI;
		m_axis = CVector3D( 1., 0., 0. );
	}
	CString label;
	ETUnit quantity;
	if( m_pNL->type() == NODAL_FORCE )
	{
		if( isTranslation( m_etDir ) )
			quantity = FORCE_UNIT;
		else
			quantity = MOMENT_UNIT;
		label = Units::show( quantity, m_pNL->magnitude() );
	}
	else if( m_pNL->type() == NODAL_SETTLEMENT )
	{
		if( isTranslation( m_etDir ) )
			quantity = LENGTH_SMALL_UNIT;
		else
			quantity = ANGLE_UNIT;
		label = Units::show( quantity, m_pNL->magnitude() );
	}

	return;
}

void CGraphicsNodeLoad::GetTextPoint( CPoint& p2D, double arrowLength )
{
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glTranslatef( (float)m_loc.x, (float)m_loc.y, (float)m_loc.z );
	glRotatef( float(m_rot*180/M_PI), (float)(m_axis.x), (float)(m_axis.y), (float)(m_axis.z) );
	if( isTranslation( m_etDir ) )
	{
		CPoint3D p3D( -arrowLength*1.1, 0., 0. );
		GetScreenPtFrom3DPoint( p3D, p2D );
	}
	else
	{
		CPoint3D p3D( 0., arrowLength*0.5, 0. );
		GetScreenPtFrom3DPoint( p3D, p2D );
	}
	glPopMatrix();
}

void CGraphicsNodeLoad::TranslateAndRotate( double arrowLength, float font_len, float scale )
{
	glTranslatef( (float)m_loc.x, (float)m_loc.y, (float)m_loc.z );
	glRotatef( float(m_rot*180/M_PI), (float)(m_axis.x), (float)(m_axis.y), (float)(m_axis.z) );
	if( isTranslation( m_etDir ) ){
		glTranslatef( (float)-arrowLength*scale, 0.f, 0.f );
		//if( m_etDir == DX )
			glTranslatef( -font_len*scale, 0.f, 0.f );
		//else if( m_etDir == DY )
		//	glTranslatef( 0.f, -font_len, 0.f );
		//else if( m_etDir == DZ )
		//	glTranslatef( 0.f, 0.f, -font_len);
	}
	else
		glTranslatef( 0.f, float(arrowLength*0.75)*scale, 0.f );
}

CGraphicsRigidDiaphragmLoad::CGraphicsRigidDiaphragmLoad( const CRigidDiaphragmLoad* pRDL ):
m_etDir( DY ),
m_loc( CPoint3D() ),
m_textPoint ( CPoint() ),
m_axis( CVector3D() ),
m_rot( 0.f ),
m_pRDL( pRDL )
{
}
CGraphicsRigidDiaphragmLoad::~CGraphicsRigidDiaphragmLoad()
{
}

void CGraphicsRigidDiaphragmLoad::Draw( double arrowLength )
{
	ASSERT_RETURN( m_pRDL );  // Normal Precaution for pointer function parameters
	float length = (float)arrowLength;
	if( length <= 0 )
		length = ini.size().nodalLoadLength;
	float width = (ini.size().nodalLoadWidth+2);
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glTranslatef( (float)m_loc.x, (float)m_loc.y, (float)m_loc.z );
	//can have up to 3 diaphragm loads...
	///ETCoordinate dir = Z;
	//if( m_pRDL->diaphragm() )  // plane's normal
	//	dir = m_pRDL->diaphragm()->normalDirection();  
	// up to three magnitudes may be entered for each plane possiblity,
	if( !zero( m_pRDL->magnitude( DX ) ) ){
		//draw x dir arrow
		DrawLineArrow( float( length ), float( width ) );
	}
	if( !zero( m_pRDL->magnitude( DY ) ) ){
		//draw y dir arrow
		glPushMatrix();
		glRotatef( 90.f, 0.f, 0.f, 1.f );
		DrawLineArrow( float( length ), float( width ) );
		glPopMatrix();
	}
	if( !zero( m_pRDL->magnitude( DZ ) ) ){
		//draw z dir arrow
		glPushMatrix();
		glRotatef( -90.f, 0.f, 1.f, 0.f );
		DrawLineArrow( float( length ), float( width ) );
		glPopMatrix();
	}
	if( !zero( m_pRDL->magnitude( RX ) ) ){
		//draw moment arrow about x-axis
		glPushMatrix();
		glRotatef( 90.f, 0.f, 1.f, 0.f );
		DrawLineMomentArrow( float( length ), float( width ) );
		glPopMatrix();
	}
	if( !zero( m_pRDL->magnitude( RY ) ) ){
		//draw moment arrow about y-axis
		glPushMatrix();
		glRotatef( -90.f, 1.f, 0.f, 0.f );
		DrawLineMomentArrow( float( length ), float( width ) );
		glPopMatrix();
	}
	if( !zero( m_pRDL->magnitude( RZ ) ) ){
		//draw moment arrow about z-axis
		DrawLineMomentArrow( float( length ), float( width ) );
	}
	glGetDoublev( GL_MODELVIEW_MATRIX, m_mv );
	glPopMatrix();
}

void CGraphicsRigidDiaphragmLoad::Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, double size )
{
	Draw( size );
	ASSERT_RETURN( m_pRDL && pDC && text );  // Normal Precaution for pointer function parameters
	float length = (float)size;
	if( size <= 0 )
		length = ini.size().nodalLoadLength;
	float width = (ini.size().nodalLoadWidth+2);
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glTranslatef( (float)m_loc.x, (float)m_loc.y, (float)m_loc.z );
	//can have up to 3 diaphragm loads...
	///ETCoordinate dir = Z;
	//if( m_pRDL->diaphragm() )  // plane's normal
	//	dir = m_pRDL->diaphragm()->normalDirection();  
	// up to three magnitudes may be entered for each plane possiblity,
	if( !filter.loadValues )
		return;
	CPoint tip2D;
	CPoint tail2D;
	double l = length;
	ETUnit quantity;
	CString label;
	if( !zero( m_pRDL->magnitude( DX ) ) ){
		//draw x dir arrow
		DrawLineArrow( float( length ), float( width ) );
		//now text...
		if( GetScreenPtFrom3DPoint( CPoint3D( 0, 0, 0 ), tip2D ) &&
				GetScreenPtFrom3DPoint( CPoint3D( -l, 0, 0), tail2D ))
		{
			quantity = FORCE_UNIT;
			label = Units::show(  quantity , m_pRDL->magnitude(DX) );
			label = "FX: " + label;
			text->ClearText();
			text->AddLine( label );
			double angle = atan2( -(float)(tip2D.y - tail2D.y), (float)(tip2D.x - tail2D.x) );
			//text->SetVAlignment( CGLOutlineText::VMIDDLE );
			text->SetHAlignment( CGLOutlineText::HRIGHT );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
			ini.font().graphWindow.size/*m_printScale*/, 
			ini.color().nodalLoads, tail2D, angle  ); 
		}

	}
	if( !zero( m_pRDL->magnitude( DY ) ) ){
		//draw y dir arrow
		glPushMatrix();
		glRotatef( 90.f, 0.f, 0.f, 1.f );
		DrawLineArrow( float( length ), float( width ) );
		if( GetScreenPtFrom3DPoint( CPoint3D( 0, 0, 0 ), tip2D ) &&
				GetScreenPtFrom3DPoint( CPoint3D( -l, 0, 0), tail2D ))
		{
			quantity = FORCE_UNIT;
			label = Units::show(  quantity , m_pRDL->magnitude(DY) );
			label = "FY: " + label;
			text->ClearText();
			text->AddLine( label );
			double angle = atan2( -(float)(tip2D.y - tail2D.y), (float)(tip2D.x - tail2D.x) );
			//text->SetVAlignment( CGLOutlineText::VMIDDLE );
			text->SetHAlignment( CGLOutlineText::HRIGHT );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
			ini.font().graphWindow.size/*m_printScale*/, 
			ini.color().nodalLoads, tail2D, angle  ); 
		}
		glPopMatrix();
	}
	if( !zero( m_pRDL->magnitude( DZ ) ) ){
		//draw z dir arrow
		glPushMatrix();
		glRotatef( -90.f, 0.f, 1.f, 0.f );
		DrawLineArrow( float( length ), float( width ) );
		if( GetScreenPtFrom3DPoint( CPoint3D( 0, 0, 0 ), tip2D ) &&
				GetScreenPtFrom3DPoint( CPoint3D( -l, 0, 0), tail2D ))
		{
			quantity = FORCE_UNIT;
			label = Units::show(  quantity , m_pRDL->magnitude(DZ) );
			label = "FZ: " + label;
			text->ClearText();
			text->AddLine( label );
			double angle = atan2( -(float)(tip2D.y - tail2D.y), (float)(tip2D.x - tail2D.x) );
			//text->SetVAlignment( CGLOutlineText::VMIDDLE );
			text->SetHAlignment( CGLOutlineText::HRIGHT );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
			ini.font().graphWindow.size/*m_printScale*/, 
			ini.color().nodalLoads, tail2D, angle  ); 
		}
		glPopMatrix();
	}
	if( !zero( m_pRDL->magnitude( RX ) ) ){
		//draw moment arrow about x-axis
		glPushMatrix();
		glRotatef( 90.f, 0.f, 1.f, 0.f );
		DrawLineMomentArrow( float( length ), float( width ) );
		if( GetScreenPtFrom3DPoint( CPoint3D( 0, 0, 0 ), tip2D ) &&
				GetScreenPtFrom3DPoint( CPoint3D( -l*0.5f, -l*0.5f, 0), tail2D ))
		{
			quantity = FORCE_UNIT;
			label = Units::show(  quantity , m_pRDL->magnitude(RX) );
			label = "RX: " + label;
			text->ClearText();
			text->AddLine( label );
			double angle = atan2( -(float)(tip2D.y - tail2D.y), (float)(tip2D.x - tail2D.x) );
			//text->SetVAlignment( CGLOutlineText::VMIDDLE );
			text->SetHAlignment( CGLOutlineText::HRIGHT );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
			ini.font().graphWindow.size/*m_printScale*/, 
			ini.color().nodalLoads, tail2D, angle  ); 
		}
		glPopMatrix();
	}
	if( !zero( m_pRDL->magnitude( RY ) ) ){
		//draw moment arrow about y-axis
		glPushMatrix();
		glRotatef( -90.f, 1.f, 0.f, 0.f );
		DrawLineMomentArrow( float( length ), float( width ) );
		if( GetScreenPtFrom3DPoint( CPoint3D( 0, 0, 0 ), tip2D ) &&
				GetScreenPtFrom3DPoint( CPoint3D( -l*0.5f, -l*0.5f, 0), tail2D ))
		{
			quantity = FORCE_UNIT;
			label = Units::show(  quantity , m_pRDL->magnitude(RY) );
			label = "RY: " + label;
			text->ClearText();
			text->AddLine( label );
			double angle = atan2( -(float)(tip2D.y - tail2D.y), (float)(tip2D.x - tail2D.x) );
			//text->SetVAlignment( CGLOutlineText::VMIDDLE );
			text->SetHAlignment( CGLOutlineText::HRIGHT );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
			ini.font().graphWindow.size/*m_printScale*/, 
			ini.color().nodalLoads, tail2D, angle  ); 
		}
		glPopMatrix();
	}
	if( !zero( m_pRDL->magnitude( RZ ) ) ){
		//draw moment arrow about z-axis
		DrawLineMomentArrow( float( length ), float( width ) );
		if( GetScreenPtFrom3DPoint( CPoint3D( 0, 0, 0 ), tip2D ) &&
				GetScreenPtFrom3DPoint( CPoint3D( -l*0.5f, -l*0.5f, 0), tail2D ))
		{
			quantity = FORCE_UNIT;
			label = Units::show(  quantity , m_pRDL->magnitude(RZ) );
			label = "RZ: " + label;
			text->ClearText();
			text->AddLine( label );
			double angle = atan2( -(float)(tip2D.y - tail2D.y), (float)(tip2D.x - tail2D.x) );
			//text->SetVAlignment( CGLOutlineText::VMIDDLE );
			text->SetHAlignment( CGLOutlineText::HRIGHT );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
			ini.font().graphWindow.size/*m_printScale*/, 
			ini.color().nodalLoads, tail2D, angle  ); 
		}
	}
	glGetDoublev( GL_MODELVIEW_MATRIX, m_mv );
	glPopMatrix();
}

void CGraphicsRigidDiaphragmLoad::SetupRigidDiaphragmLoad( const CRigidDiaphragmLoad* pRDL, float length )
{

	double dLoc[3];
	const CNode* pN = pRDL->location();
	ASSERT_RETURN( pN );
	
	dLoc[0] = m_loc[0] = pN->x();
	dLoc[1] = m_loc[1] = pN->y();
	dLoc[2] = m_loc[2] = pN->z();

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glTranslatef( (float)m_loc.x, (float)m_loc.y, (float)m_loc.z );

	if( !zero( pRDL->magnitude( DX ) ) ){
		//draw x dir arrow
	}
	else if( !zero( pRDL->magnitude( DY ) ) ){
		//draw y dir arrow
		glRotatef( 90.f, 0.f, 0.f, 1.f );
	}
	else if( !zero( pRDL->magnitude( DZ ) ) ){
		//draw z dir arrow
		glRotatef( -90.f, 0.f, 1.f, 0.f );
	}
	glTranslatef( length, 0.f, 0.f );
	glGetDoublev( GL_MODELVIEW_MATRIX, m_mv );
	glPopMatrix();

	return;
}

void CGraphicsRigidDiaphragmLoad::translate( const CRigidDiaphragmLoad* pRDL, float length )
{
	double dLoc[3];
	const CNode* pN = pRDL->location();
	ASSERT_RETURN( pN );
	
	dLoc[0] = m_loc[0] = pN->x();
	dLoc[1] = m_loc[1] = pN->y();
	dLoc[2] = m_loc[2] = pN->z();

	glMatrixMode( GL_MODELVIEW );
	//glPushMatrix();
	glTranslatef( (float)m_loc.x, (float)m_loc.y, (float)m_loc.z );

	if( !zero( pRDL->magnitude( DX ) ) ){
		//draw x dir arrow
	}
	else if( !zero( pRDL->magnitude( DY ) ) ){
		//draw y dir arrow
		glRotatef( 90.f, 0.f, 0.f, 1.f );
	}
	else if( !zero( pRDL->magnitude( DZ ) ) ){
		//draw z dir arrow
		glRotatef( -90.f, 0.f, 1.f, 0.f );
	}
	glTranslatef( length, 0.f, 0.f );
	//glGetDoublev( GL_MODELVIEW_MATRIX, m_mv );
	//glPopMatrix();

	return;
}

void CGraphicsRigidDiaphragmLoad::GetTextPoint( CPoint& p2D, double arrowLength )
{
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glTranslatef( (float)m_loc.x, (float)m_loc.y, (float)m_loc.z );
	glRotatef( float(m_rot*180/M_PI), (float)(m_axis.x), (float)(m_axis.y), (float)(m_axis.z) );
	if( isTranslation( m_etDir ) )
	{
		CPoint3D p3D( -arrowLength*1.1, 0., 0. );
		GetScreenPtFrom3DPoint( p3D, p2D );
	}
	else
	{
		CPoint3D p3D( 0., arrowLength, 0. );
		GetScreenPtFrom3DPoint( p3D, p2D );
	}
	glPopMatrix();
}

