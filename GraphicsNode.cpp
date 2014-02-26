#include "StdAfx.h"
#include "GraphicsNode.h"

#include "Graphics/GraphicsHelpers.h"

#include "Model.h"

CGraphicsNode::CGraphicsNode(const CNode* m_pN ):
m_pN( m_pN )
{
}

CGraphicsNode::~CGraphicsNode(void)
{
}

void CGraphicsNode::Draw( CDC* pDC, const CWindowFilter& filter, double scale, CGLText* text )
{
	ASSERT( m_pN );

	if( !m_pN )
		return;

	const CResultCase* pRC = filter.resultCase();
	double xi = m_pN->x();
	double yi = m_pN->y();
	double zi = m_pN->z();
	if( !pRC && filter.windowType == POST_WINDOW )
		return;
	if( pRC && filter.windowType == POST_WINDOW )
	{
		const CResult* cNR = pRC->result( *m_pN );
		CHECK_IF( cNR != NULL ){
			xi += scale*cNR->displacement( DX );
			yi += scale*cNR->displacement( DY );
			zi += scale*cNR->displacement( DZ );
		}
	}

	////glDisable( GL_DEPTH_TEST );
	//COLORREF c = RGB(0, 0, 0);
	//if( m_pN->isSelected()  )
	//	c = RGB(110,110,100);//InverseColor( c );
	//ApplyAmbientGLMaterialiv( c );
	//glColor3ub( GetRValue( c ), GetGValue( c ), GetBValue( c ) );
	//glPointSize( 2*float( ini.size().node ) );
	///*if( m_pN->isSelected() ){
	//	glPointSize( 4*float( ini.size().node ) );
	//	glDisable( GL_DEPTH_TEST );
	//}*/
	//glBegin( GL_POINTS );
	//	glVertex3f( float(xi), float(yi), float(zi) );
	//glEnd();

	COLORREF c = ini.color().nodes;
	if( m_pN->isSelected()  )
		c = InverseColor( c );
	SetGLColor( c );
	glPointSize( float( ini.size().node ) );
	if( m_pN->isSelected() ){
		glPointSize( 2*float( ini.size().node ) );
		//glDisable( GL_DEPTH_TEST );
	}
	glBegin( GL_POINTS );
		glVertex3f( float(xi), float(yi), float(zi) );
	glEnd();

	//draw circle around it if it's a scissor...
	//start billboard...
	if( m_pN->isScissor() )
	{
		glPushMatrix();
		glTranslatef( float(xi), float(yi), float(zi) );
		StartBillboardView();
		DrawCircle( ini.size().node*2, ini.size().member );
		glPopMatrix();
	}

	if( m_pN->isSelected() ){
		glPointSize( float( ini.size().node ) );
		//glEnable( GL_DEPTH_TEST );
	}

	//draw text
	ETShowUnit show = (filter.showUnits ? SHOW_UNIT : NO_UNIT);
	if( filter.windowType != DESIGN_WINDOW && filter.nodes ) 
	{
		text->ClearText();
		CString nodeText = "";
		if( filter.node.name )
		{
			nodeText += m_pN->name();
			text->AddLine( nodeText );
			nodeText = "";
		}
		if( filter.node.location )
		{
			CString loc;
			if( theStructure.isPlane() ) 
			{
				loc.Format( "(%s,%s)", 
					Units::show( LENGTH_LARGE_UNIT, m_pN->x(), show ),
					Units::show( LENGTH_LARGE_UNIT,	m_pN->y(), show ) );
			} else {
				loc.Format("(%s,%s,%s)", 
					Units::show( LENGTH_LARGE_UNIT, m_pN->x(), show), 
					Units::show( LENGTH_LARGE_UNIT, m_pN->y(), show),
					Units::show( LENGTH_LARGE_UNIT, m_pN->z(), show) );
			}
			if( !loc.IsEmpty() )
			nodeText = loc;
			text->AddLine( nodeText );
			nodeText = "";
		}
		if( filter.node.mass )
		{
			CString tmass;
			CString rmass;
			nodeText = "";
			if( m_pN->mass( TRANSLATION ) > 0. )
			{
				tmass.Format( "dM=%s", 
					Units::show( MASS_UNIT, m_pN->mass(TRANSLATION), show ) );
				nodeText = tmass;
				text->AddLine( nodeText );
				nodeText = "";
			}
			if( m_pN->mass( ROTATION ) > 0. ) 
			{
				rmass.Format( " rM=%s", 
					Units::show( ROTATIONAL_MASS_UNIT, m_pN->mass(ROTATION), show) );
				nodeText = rmass;
				text->AddLine( nodeText );
				nodeText = "";
			}
		}
		CPoint tip2D;
		if( text && !text->isEmpty() && pDC && GetScreenPtFrom3DPoint( CPoint3D( xi, yi, zi ), tip2D ) )
		{
			double angle = 0;
			tip2D.x += 2*ini.size().node;
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
			ini.font().graphWindow.size/*m_printScale*/, 
			ini.color().nodes, tip2D, angle  ); 
		}
	}
	return;
}