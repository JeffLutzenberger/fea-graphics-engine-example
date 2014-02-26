#include "StdAfx.h"
#include "GraphicsContourBar.h"
#include "Graphics/GraphicsHelpers.h"
#include "IniColor.h"
#include "IniFont.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const double gMinContourSliderDelta = 0.05;

CGraphicsContourBar::CGraphicsContourBar( CWnd* pParent, unsigned int id ):
CGraphicsObjects( pParent, id ),
m_label( "" ),
m_min( 0. ),
m_max( 0. ),
m_offset( CPoint( 0, 0 ) ),
m_startingOffset( CPoint( 0, 0 ) ),
m_barRect( CRect() ),
m_id( id ),
m_minSlider( 0 ),
m_maxSlider( 1 ),
m_hitObject( CONTOUR_BAR_NOTHING ),
m_contourTexture( NULL ),
m_nContourTexture( 0 ),
m_bBlendedGradient( true )
{
}

CGraphicsContourBar::CGraphicsContourBar( CWnd* pParent, unsigned int id, CPoint offset ):
CGraphicsObjects( pParent, id ),
m_label( "" ),
m_min( 0. ),
m_max( 0. ),
m_offset( offset ),
m_startingOffset( CPoint( 0, 0 ) ),
m_barRect( CRect() ),
m_id( id ),
m_minSlider( 0 ),
m_maxSlider( 1 ),
m_hitObject( CONTOUR_BAR_NOTHING ),
m_contourTexture( NULL ),
m_nContourTexture( 0 ),
m_bBlendedGradient( true )
{
}

CGraphicsContourBar::CGraphicsContourBar( CGraphicsContourBar* pObj ):
CGraphicsObjects( pObj->m_pParentWnd, pObj->m_graphicsID ),
m_label( pObj->m_label ),
m_min( pObj->m_min ),
m_max( pObj->m_max ),
m_offset( pObj->m_offset ),
m_startingOffset( pObj->m_startingOffset ),
m_barRect( pObj->m_barRect ),
m_id( pObj->m_id ),
m_minSlider( pObj->m_minSlider ),
m_maxSlider( pObj->m_maxSlider ),
m_hitObject( pObj->m_hitObject ),
m_contourTexture( NULL ),
m_nContourTexture( 0 ),
m_bBlendedGradient( pObj->m_bBlendedGradient )
{
}

CGraphicsContourBar::~CGraphicsContourBar(void)
{
	if( m_contourTexture ) delete[] m_contourTexture;
}

void CGraphicsContourBar::UpdateLabels(const CString& label, double min, double max )
{

	m_min = min;
	m_max = max;
	m_label = label;
}

void CGraphicsContourBar::Draw( CDC* pDC, const CPoint& /*offset*/ )
{
	Draw( pDC, 0., 1., false);
}

void CGraphicsContourBar::Draw( CDC* pDC, float start_tex_coord, float end_tex_coord, bool dragging )
{
	if( dragging )
	{
		if( m_hitObject == CONTOUR_BAR_MIN_SLIDER || m_hitObject == CONTOUR_BAR_MAX_SLIDER )
		{
			DrawSliders( );
			return;
		}	
	}

	if( ini.graphics().blendResultColors != m_bBlendedGradient )
	{
		m_bBlendedGradient = ini.graphics().blendResultColors;
		recreateContourTexture();
	}

	CRect wndRect;
	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	wndRect.left = 0;
	wndRect.top = 0;
	wndRect.bottom = viewport[3];
	wndRect.right = viewport[2];

	StartGL2DDrawing();
	StartSolidDrawing( 0 );
	glDisable( GL_DEPTH_TEST );
	glOrtho(double(wndRect.left), double(wndRect.right), double(wndRect.bottom), double(wndRect.top), -1., 1. );
	
	short startx = short( wndRect.right - wndRect.Width()*0.35 + m_offset.x + m_startingOffset.x );//short( p.x );
	short endx   = short( wndRect.right - wndRect.Width()*0.05 + m_offset.x + m_startingOffset.x );//short( p.y );
	short starty = short( wndRect.bottom - wndRect.Width()*0.075 + m_offset.y + m_startingOffset.y );
	short endy   = short( wndRect.bottom - wndRect.Width()*0.06 + m_offset.y + + m_startingOffset.y );

	m_barRect = CRect( startx, starty, endx, endy );
	
	COLORREF c = ini.color().background;//ini.color().background;
	COLORREF cinv = InverseColor( c );
	// set the outline to the inverse window color
	float invColor[4] = { float( double(GetRValue(cinv))/255. ), 
		float( double(GetGValue(cinv))/255. ),
		float( double(GetBValue(cinv))/255. ), 
		ALPHA };
	
	m_boundingRect = m_barRect;
	double width = (double)m_barRect.Width();
	int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
	int fontPixelHeight = MulDiv( ini.font().graphLegend.size, pixely, 72);
	m_boundingRect.InflateRect( (int)(width*0.10), fontPixelHeight*3 );

	//draw background box
	glColor4fv( invColor );
	glLineWidth( 2.f );
	glBegin( GL_LINE_LOOP );
	glVertex2s( m_boundingRect.left, m_boundingRect.top );
	glVertex2f( m_boundingRect.right, m_boundingRect.top );
	glVertex2f( m_boundingRect.right, m_boundingRect.bottom );
	glVertex2f( m_boundingRect.left, m_boundingRect.bottom );
	glEnd();

	invColor[3] = 0.25;
	glEnable( GL_BLEND );
	glDisable( GL_DEPTH_TEST );
		glColor4fv( invColor );
		glBegin( GL_QUADS );
		glVertex2s( m_boundingRect.left, m_boundingRect.top );
		glVertex2f( m_boundingRect.right, m_boundingRect.top );
		glVertex2f( m_boundingRect.right, m_boundingRect.bottom );
		glVertex2f( m_boundingRect.left, m_boundingRect.bottom );
		glEnd();
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	//end background box

	glColor3f( 1.f, 1.f, 1.f );
	if( m_nContourTexture == 0 )
		recreateContourTexture();
	//failsafe...
	CHECK_IF( m_nContourTexture > 0 )
	{
		bool bDoBlending = ini.graphics().blendResultColors;
		::StartGLGradientTexture( m_nContourTexture, bDoBlending );
	}
	glBegin( GL_QUADS );
	glTexCoord2f( start_tex_coord, start_tex_coord );
	glVertex2s( startx, starty );
	glTexCoord2f( end_tex_coord, end_tex_coord );
	glVertex2f( endx, starty );
	glTexCoord2f( end_tex_coord, end_tex_coord );
	glVertex2f( endx, endy );
	glTexCoord2f( start_tex_coord, start_tex_coord );
	glVertex2f( startx, endy );
	glEnd();
	EndGLTextureMode();
	
	invColor[3] = 1;
	glColor4fv( invColor );
	glLineWidth( 2.f );
	glBegin( GL_LINE_LOOP );
	glTexCoord2f( 0., 0. );
	glVertex2s( startx, starty );
	glTexCoord2f( 1., 1. );
	glVertex2f( endx, starty );
	glTexCoord2f( 1., 1. );
	glVertex2f( endx, endy );
	glTexCoord2f( 0., 0. );
	glVertex2f( startx, endy );
	glEnd();

	//draw tick marks
	float y1 = starty;
	float y2 = endy;
	float dx = float(endx - startx)/4;
	glBegin( GL_LINES );
	for( int i = 0; i < 5; i++ ){
		float x = startx + dx*i;
		glVertex2f( x, y1 );
		glVertex2f( x, y2 );
	}
	glEnd();

	//draw range pointers - these can slide along the legend bar
	if( m_minSlider < 0 ) m_minSlider = 0;
	if( m_maxSlider > 1 ) m_maxSlider = 1;
	if( m_minSlider > m_maxSlider ) m_minSlider = m_maxSlider-0.01;
	if( m_maxSlider < m_minSlider ) m_maxSlider = m_minSlider+0.01;

	double min_loc = startx+m_minSlider*(endx-startx);
	double max_loc = startx+m_maxSlider*(endx-startx);
	double triangle_size = m_barRect.Height()*0.75;
	//min slider
	glColor3f( 0.5f, 0.5f, 0.5f );
	glBegin(GL_TRIANGLES);
	glVertex2f( min_loc, endy );
	glVertex2f( min_loc - triangle_size, endy + triangle_size );
	glVertex2f( min_loc + triangle_size, endy + triangle_size );
	glEnd();
	glColor4fv( invColor );
	glBegin(GL_LINE_LOOP);
	glVertex2f( min_loc, endy );
	glVertex2f( min_loc - triangle_size, endy + triangle_size );
	glVertex2f( min_loc + triangle_size, endy + triangle_size );
	glEnd();
	//max slider
	glColor3f( 0.5f, 0.5f, 0.5f );
	glBegin(GL_TRIANGLES);
	glVertex2f( max_loc, endy );
	glVertex2f( max_loc - triangle_size, endy + triangle_size );
	glVertex2f( max_loc + triangle_size, endy + triangle_size );
	glEnd();
	glColor4fv( invColor );
	glBegin(GL_LINE_LOOP);
	glVertex2f( max_loc, endy );
	glVertex2f( max_loc - triangle_size, endy + triangle_size );
	glVertex2f( max_loc + triangle_size, endy + triangle_size );
	glEnd();
	EndGL2DDrawing();
	glEnable( GL_DEPTH_TEST );

	return;
}

void CGraphicsContourBar::DrawSliders( )
{
	CRect wndRect;
	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	wndRect.left = 0;
	wndRect.top = 0;
	wndRect.bottom = viewport[3];
	wndRect.right = viewport[2];

	StartGL2DDrawing();
	StartSolidDrawing( 0 );
	//glDisable( GL_DEPTH_TEST );
	//DisableGLTransparency();
	glOrtho(double(wndRect.left), double(wndRect.right), double(wndRect.bottom), double(wndRect.top), -1., 1. );
	
	COLORREF c = ini.color().background;//ini.color().background;
	COLORREF cinv = InverseColor( c );
	
	// set the outline to the inverse window color
	float invColor[4] = { float( double(GetRValue(cinv))/255. ), 
		float( double(GetGValue(cinv))/255. ),
		float( double(GetBValue(cinv))/255. ), 
		ALPHA };

	short startx = short( wndRect.right - wndRect.Width()*0.35 + m_offset.x + m_startingOffset.x );//short( p.x );
	short endx   = short( wndRect.right - wndRect.Width()*0.05 + m_offset.x + m_startingOffset.x );//short( p.y );
	//short starty = short( wndRect.bottom - wndRect.Width()*0.075 + m_offset.y + m_startingOffset.y );
	short endy   = short( wndRect.bottom - wndRect.Width()*0.06 + m_offset.y + + m_startingOffset.y );

	//draw range pointers - these can slide along the legend bar
	if( m_minSlider < 0 ) m_minSlider = 0;
	if( m_maxSlider > 1 ) m_maxSlider = 1;
	if( m_minSlider > m_maxSlider ) m_minSlider = m_maxSlider-0.01;
	if( m_maxSlider < m_minSlider ) m_maxSlider = m_minSlider+0.01;

	double min_loc = startx+m_minSlider*(endx-startx);
	double max_loc = startx+m_maxSlider*(endx-startx);
	double triangle_size = m_barRect.Height()*0.75;
	//min slider
	glColor3f( 0.5f, 0.5f, 0.5f );
	glBegin(GL_TRIANGLES);
	glVertex2f( min_loc, endy );
	glVertex2f( min_loc - triangle_size, endy + triangle_size );
	glVertex2f( min_loc + triangle_size, endy + triangle_size );
	glEnd();
	glColor4fv( invColor );
	glBegin(GL_LINE_LOOP);
	glVertex2f( min_loc, endy );
	glVertex2f( min_loc - triangle_size, endy + triangle_size );
	glVertex2f( min_loc + triangle_size, endy + triangle_size );
	glEnd();
	//max slider
	glColor3f( 0.5f, 0.5f, 0.5f );
	glBegin(GL_TRIANGLES);
	glVertex2f( max_loc, endy );
	glVertex2f( max_loc - triangle_size, endy + triangle_size );
	glVertex2f( max_loc + triangle_size, endy + triangle_size );
	glEnd();
	glColor4fv( invColor );
	glBegin(GL_LINE_LOOP);
	glVertex2f( max_loc, endy );
	glVertex2f( max_loc - triangle_size, endy + triangle_size );
	glVertex2f( max_loc + triangle_size, endy + triangle_size );
	glEnd();
	EndGL2DDrawing();
	glEnable( GL_DEPTH_TEST );

	return;

}
void CGraphicsContourBar::DrawContourBarText(CGraphicsText& text, CDC* pDC, CRect r, ETUnit unit, float printScale )
{
	if( zero(printScale) ) printScale = 1.0;
	COLORREF c = InverseColor( ini.color().background );
	int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
	int fontPixelHeight = MulDiv( ini.font().graphLegend.size/printScale, pixely, 72);
	CPoint control_pt = m_barRect.CenterPoint();
	control_pt.y = m_barRect.top;
	control_pt.y -= (int)(double)(fontPixelHeight)*1.5;
	
	CString unit_text = "";
	unit_text.Format( " (%s)", Units::showUnit( unit ) );
	m_label += unit_text;
	text.AddLine( m_label );
	
	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	r.top = 0;
	r.left = 0;
	r.right = viewport[2];
	r.bottom = viewport[3];

	text.ClearText();
	text.AddLine( m_label );
	text.Center();
	text.DrawText2D( pDC, ini.font().graphLegend.name, 
					ini.font().graphLegend.size/*printScale*/, 
					c, control_pt, 0., r  ); 

	CString str;
	float dx = float(m_barRect.Width())/4.f;
	double val = m_min;
	for( int i = 0; i < 5; i++ ){
		val = m_min + (m_max - m_min)/4*i;
		text.ClearText();
		text.Center();
		str.Format( "%s", Units::report( unit, val, 6 ) );
		text.AddLine( str );
		control_pt = m_barRect.CenterPoint();
		control_pt.y = m_barRect.bottom + (int)((double)(fontPixelHeight)*1.5);
		control_pt.x -= int(m_barRect.Width()/2);
		control_pt.x += int(i*dx);
		text.DrawText2D( pDC, ini.font().graphLegend.name, 
					ini.font().graphLegend.size/*printScale*/, 
					c, control_pt, 0., r  ); 
	}

}

const CRect CGraphicsContourBar::boundingRectangle()
{
	return m_boundingRect;
}

const CGraphicsContourBar* CGraphicsContourBar::pointIsOver( const CPoint& point )
{
	CRect rect = m_boundingRect;
	rect.InflateRect( int(m_boundingRect.Width()*0.1), int(m_boundingRect.Width() * 0.1) );
	if( point.x >= rect.left && 
		point.x <= rect.right && point.y >= rect.top && 
		point.y <= rect.bottom  )
	{
		//find out what we hit, contour bar or sliders
		m_hitObject = hitWhat( point );
		return this;
	}
	else
		return NULL;
}

void CGraphicsContourBar::OnLButtonDown()
{
	//if we hit the contour bar figure out what we hit and set the enum type
}

void CGraphicsContourBar::OnLButtonUp()
{
	if( CONTOUR_BAR_MIN_SLIDER || CONTOUR_BAR_MAX_SLIDER )
	{
		//sliders have moved
		recreateContourTexture();
	}

	//release everything
	m_hitObject = CONTOUR_BAR_NOTHING;
}

void CGraphicsContourBar::OnMouseMove( const CPoint& delta )
{
	//move the contour bar or the sliders...
	switch( m_hitObject )
	{
	case CONTOUR_BAR_MIN_SLIDER:
	case CONTOUR_BAR_MAX_SLIDER:
		dragSlider( delta );
		break;
	case CONTOUR_BAR:
		incrementOffset( delta );
		break;
	}
}

ETContourHitObject CGraphicsContourBar::hitWhat( const CPoint& pt )
{
	double hit_size = m_barRect.Height()*0.75;
	//sliders are just under our m_barRect. 
	int minSliderLoc = m_barRect.left + (int)(m_minSlider*(double)m_barRect.Width());
	int maxSliderLoc = m_barRect.left + (int)(m_maxSlider*(double)m_barRect.Width());
	CRect minSliderRect = CRect(minSliderLoc - (int)hit_size, //left
								m_barRect.top, //top
								minSliderLoc + (int)hit_size,
								m_barRect.bottom + (int)hit_size );
	CRect maxSliderRect = CRect(maxSliderLoc - (int)hit_size, //left
								m_barRect.top, //top
								maxSliderLoc + (int)hit_size,
								m_barRect.bottom + (int)hit_size );
	minSliderRect.InflateRect( minSliderRect.Width()*0.2, minSliderRect.Height()*0.2 );
	maxSliderRect.InflateRect( maxSliderRect.Width()*0.2, maxSliderRect.Height()*0.2 );
	if( minSliderRect.PtInRect( pt ) )
	{
		//TRACE( "Hit min slider\n" );
		return CONTOUR_BAR_MIN_SLIDER;
	}
	if( maxSliderRect.PtInRect( pt ) )
	{
		//TRACE( "Hit max slider\n" );
		return CONTOUR_BAR_MAX_SLIDER;
	}
	//otherwise we hit the contour bar...
	return CONTOUR_BAR;
}

void CGraphicsContourBar::StartGLGradientTexture()
{
	
	if( m_nContourTexture <= 0 )
		recreateContourTexture();
	bool bDoBlending = ini.graphics().blendResultColors;
	::StartGLGradientTexture( m_nContourTexture, bDoBlending );
}

void CGraphicsContourBar::recreateContourTexture()
{
	//rebuild our color gradient and reload it into texture memory
	//color gradient is 8 colors (colors are 3 bytes each) 
	//we pad each the area outside of the sliders with grey
	//int n_gradient = 8;
	double slider_delta = m_maxSlider - m_minSlider;
	if( slider_delta <= 0 ) slider_delta = gMinContourSliderDelta;
	if( slider_delta <= gMinContourSliderDelta ) 
		slider_delta = gMinContourSliderDelta; //make sure the texture is not too big
	int n_colors = gGradientTextureSize;//(int)((double)n_gradient/(double)slider_delta);
	//if( n_colors % 2 != 0 ) n_colors++;
	int n_min_slider_loc = (int)(m_minSlider * (double)n_colors);
	if( n_min_slider_loc < 0 )
		n_min_slider_loc = 0;
	if( n_min_slider_loc >= n_colors )
		n_min_slider_loc = n_colors-1;
	int n_max_slider_loc = (int)(m_maxSlider * (double)n_colors);
	if( n_max_slider_loc < 0 )
		n_max_slider_loc = 1;
	if( n_max_slider_loc > n_colors )
		n_max_slider_loc = n_colors;
	if( n_min_slider_loc >= n_max_slider_loc )
	{
		if( n_min_slider_loc > 0 )
			n_min_slider_loc--;
		else if( n_max_slider_loc < n_colors )
			n_max_slider_loc++;
		else
		{
			n_min_slider_loc = 0;
			n_max_slider_loc = n_colors;
			ASSERT( false );
		}
	}

	//unbind the glTexture with glDeleteTextures(...)
	//GLuint textures[] = {m_nContourTexture};
	glDeleteTextures( 1, &m_nContourTexture );
	m_nContourTexture = 0;
	
	//delete the contour texture
	if( m_contourTexture ) 
		delete[] m_contourTexture;
	
	//create a new one
	m_contourTexture = new GLubyte[n_colors*3];
	GLubyte padding_byte = (GLubyte)0xaa; //grey
	//pad the entire array with grey first...
	for( int i = 0; i < n_colors*3; i++ )
	{
		m_contourTexture[i] = (GLubyte)padding_byte;
	}
	//now fill in the colors between the sliders...
	for( int i = n_min_slider_loc*3; i < n_max_slider_loc*3; i++ )
	{
		m_contourTexture[i] = (GLubyte)gGradientTexture[i];
	}
	bool bDoBlending = ini.graphics().blendResultColors;
	SetupGradientTexture( m_nContourTexture, m_contourTexture, n_colors /*3 bytes per color*/, bDoBlending );
}

void CGraphicsContourBar::resetSliders()
{
	m_minSlider = 0;
	m_maxSlider = 1;
	recreateContourTexture();
}
void CGraphicsContourBar::dragSlider( const CPoint& pt )
{
	//find out which one we're moving and drag it
	//clamp the movement so that the slider can only move left and right and between:
	//0 and m_maxSlider if we're dragging the m_minSlider
	//m_minSlider and 1 if we're dragging the m_maxSlider
	double barWidth = (double)m_barRect.Width();
	double sliderDelta = double(pt.x)/barWidth;
	double sliderLoc = 0;
	if( m_hitObject == CONTOUR_BAR_MIN_SLIDER )
		sliderLoc = sliderDelta + m_minSlider;
	else if( m_hitObject == CONTOUR_BAR_MAX_SLIDER )
		sliderLoc = sliderDelta + m_maxSlider;
	if( sliderLoc < 0 ) sliderLoc = 0;
	if( sliderLoc > 1 ) sliderLoc = 1;
	if( m_hitObject == CONTOUR_BAR_MIN_SLIDER )
	{
		if( sliderLoc >= m_maxSlider - gMinContourSliderDelta )
			m_minSlider = m_maxSlider - gMinContourSliderDelta;
		else
			m_minSlider = sliderLoc;
	}
	else if( m_hitObject == CONTOUR_BAR_MAX_SLIDER )
	{
		if( sliderLoc <= m_minSlider + gMinContourSliderDelta )
			m_maxSlider = m_minSlider + gMinContourSliderDelta;
		else
			m_maxSlider = sliderLoc;
	}
}

void CGraphicsContourBar::incrementOffset(const CPoint& delta )
{
	m_offset += delta;
}

//////////////////////////////////////////////////////////////////////////
//
// Design Contour bar
//
//////////////////////////////////////////////////////////////////////////
CGraphicsDesignContourBar::CGraphicsDesignContourBar( CWnd* pParent, unsigned int id ):
CGraphicsContourBar( pParent, id )
{
}

CGraphicsDesignContourBar::CGraphicsDesignContourBar( CGraphicsDesignContourBar* pObj ):
CGraphicsContourBar( pObj->m_pParentWnd, pObj->m_graphicsID )
{
}

CGraphicsDesignContourBar::~CGraphicsDesignContourBar(void)
{
}

void CGraphicsDesignContourBar::Draw( CDC* pDC, const CPoint& /*offset*/ )
{
	Draw( pDC, 0., 1.);
}

void CGraphicsDesignContourBar::DrawContourBarText(CGraphicsText& text, CDC* pDC, CRect r, ETUnit unit, float printScale )
{
	if( zero(printScale) ) printScale = 1.0;
	COLORREF c = InverseColor( ini.color().background );
	int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
	int fontPixelHeight = MulDiv( ini.font().graphLegend.size, pixely, 72);
	CPoint control_pt = m_barRect.CenterPoint();
	CString unit_text = "";
	
	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	r.top = 0;
	r.left = 0;
	r.right = viewport[2];
	r.bottom = viewport[3];

	CString str;
	float dx = float(m_barRect.Width())/4.f;
	double val = m_min;
	for( int i = 0; i < 5; i++ ){
		val = (m_max - m_min)/4*i;
		text.ClearText();
		text.Center();
		str.Format( "%s", Units::report( unit, val, 6 ) );
		text.AddLine( str );
		control_pt = m_barRect.CenterPoint();
		control_pt.y += int(m_barRect.Height());
		control_pt.x -= int(m_barRect.Width()/2);
		control_pt.x += int(i*dx);
		text.DrawText2D( pDC, ini.font().graphLegend.name, 
					ini.font().graphLegend.size/printScale, 
					c, control_pt, 0., r  ); 
	}

	//add sub legend text "not grouped", "failed", and "warning message"
	CRect wndRect = m_wndRect;
	double h = fontPixelHeight;
	double w = wndRect.Width()*0.05;

	control_pt.x = m_barRect.left;
	control_pt.y = m_barRect.top - 4*h;
	
	text.ClearText();
	text.Center( false );
	text.VerticalCenter( false );
	CString sub_legend = "Ungrouped";
	text.AddLine( sub_legend );
	text.DrawText2D( pDC, ini.font().graphLegend.name, 
					ini.font().graphLegend.size/printScale, 
					c, control_pt, 0., r  ); 

	text.ClearText();
	text.Center();
	text.VerticalCenter( false );
	sub_legend = "Failed";
	text.AddLine( sub_legend );
	control_pt.x = m_barRect.CenterPoint().x;
	text.DrawText2D( pDC, ini.font().graphLegend.name, 
					ini.font().graphLegend.size/printScale, 
					c, control_pt, 0., r  ); 

	text.ClearText();
	text.Center( false );
	text.VerticalCenter( false );
	sub_legend = "Warning";
	text.AddLine( sub_legend );
	control_pt.x = m_barRect.right - w;
	text.DrawText2D( pDC, ini.font().graphLegend.name, 
					ini.font().graphLegend.size/printScale, 
					c, control_pt, 0., r  ); 

}
void CGraphicsDesignContourBar::Draw( CDC* pDC, float /*start_tex_coord*/, float /*end_tex_coord*/ )
{
	CRect wndRect = m_wndRect;

	StartGL2DDrawing();
	StartSolidDrawing( 0 );
	glDisable( GL_DEPTH_TEST );
	glOrtho(double(wndRect.left), double(wndRect.right), double(wndRect.bottom), double(wndRect.top), -1., 1. );
	
	short startx = short( wndRect.right - wndRect.Width()*0.35 + m_offset.x + m_startingOffset.x );//short( p.x );
	short endx   = short( wndRect.right - wndRect.Width()*0.05 + m_offset.x + m_startingOffset.x );//short( p.y );
	short starty = short( wndRect.bottom - wndRect.Width()*0.075 + m_offset.y + m_startingOffset.y );
	short endy   = short( wndRect.bottom - wndRect.Width()*0.06 + m_offset.y + + m_startingOffset.y );

	m_barRect = CRect( startx, starty, endx, endy );
	
	COLORREF c = ini.color().background;//ini.color().background;
	COLORREF cinv = InverseColor( c );
	// set the outline to the inverse window color
	float invColor[4] = { float( double(GetRValue(cinv))/255. ), 
		float( double(GetGValue(cinv))/255. ),
		float( double(GetBValue(cinv))/255. ), 
		ALPHA };
	
	m_boundingRect = m_barRect;
	double width = (double)m_barRect.Width();
	int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
	int fontPixelHeight = MulDiv( ini.font().graphLegend.size, pixely, 72);
	m_boundingRect.InflateRect( (int)(width*0.10), fontPixelHeight*4 );

	//draw background box
	glColor4fv( invColor );
	glLineWidth( 2.f );
	glBegin( GL_LINE_LOOP );
	glVertex2s( m_boundingRect.left, m_boundingRect.top );
	glVertex2f( m_boundingRect.right, m_boundingRect.top );
	glVertex2f( m_boundingRect.right, m_boundingRect.bottom );
	glVertex2f( m_boundingRect.left, m_boundingRect.bottom );
	glEnd();

	invColor[3] = 0.25;
	glEnable( GL_BLEND );
	glDisable( GL_DEPTH_TEST );
		glColor4fv( invColor );
		glBegin( GL_QUADS );
		glVertex2s( m_boundingRect.left, m_boundingRect.top );
		glVertex2f( m_boundingRect.right, m_boundingRect.top );
		glVertex2f( m_boundingRect.right, m_boundingRect.bottom );
		glVertex2f( m_boundingRect.left, m_boundingRect.bottom );
		glEnd();
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	////end background box

	//draw color bar
	float blue[4] = {0,0,1,1};
	float green[4] = {0,1,0,1};
	glBegin( GL_QUADS );
	glColor4fv( blue );
	glVertex2s( startx, starty );
	glColor4fv( green );
	glVertex2s( endx, starty );
	glColor4fv( green );
	glVertex2s( endx, endy );
	glColor4fv( blue );
	glVertex2s( startx, endy );
	glEnd();

	invColor[3] = 1;
	glColor4fv( invColor );
	glLineWidth( 2 );
	glBegin( GL_LINE_LOOP );
	glVertex2s( startx, starty );
	glVertex2s( endx, starty );
	glVertex2s( endx, endy );
	glVertex2s( startx, endy );
	glEnd();
	//end color bar

	//draw tick marks
	float y1 = starty;
	float y2 = endy;
	float dx = float(endx - startx)/4;
	glBegin( GL_LINES );
	for( int i = 0; i < 5; i++ ){
		float x = startx + dx*i;
		glVertex2f( x, y1 );
		glVertex2f( x, y2 );
	}
	glEnd();
	
	//////////////////////////////////////////////////////////////////////////
	// draw the sub-legend:
	// "failed", "not grouped", "warning message"
	//////////////////////////////////////////////////////////////////////////

	short h = fontPixelHeight;
	short w = (short)(wndRect.Width()*0.05);
	short wover2 = (short)(w/2.);
	// draw the "not grouped" box on the left side of the bar
	glColor3f( 0.25f, 0.25f, 0.25f ); // TeK Change 1/24/2008: Also set in CGraphicView::::SetUnityValuesAndColors()
	glEnable( GL_POLYGON_STIPPLE );
	glPolygonStipple(stippleMask[1]);
	glEnable( GL_LINE_STIPPLE );
	glLineStipple( 4, 0xAAAA );
	glBegin( GL_QUADS );
	glVertex2s( startx, starty - h);
	glVertex2s( startx + w, starty - h );
	glVertex2s( startx + w, starty - 2*h );
	glVertex2s( startx, starty - 2*h );
	glEnd();
	glBegin( GL_LINES );
	glVertex2s( startx, starty - h);
	glVertex2s( startx + w, starty - h );
	glVertex2s( startx + w, starty - 2*h );
	glVertex2s( startx, starty - 2*h );
	glEnd();

	// draw the "failed" box in the center
	glColor3f( 1.f, 0.f, 0.f );
	glDisable( GL_POLYGON_STIPPLE );
	glDisable( GL_LINE_STIPPLE );
	short center_x = short( 0.5*(endx + startx) );
	glBegin( GL_QUADS );
	glVertex2s( center_x - wover2, starty - h );
	glVertex2s( center_x + wover2, starty - h );
	glVertex2s( center_x + wover2, starty - 2*h );
	glVertex2s( center_x - wover2, starty - 2*h );
	glEnd();
	glBegin( GL_LINES );
	glVertex2s( center_x - wover2, starty - h );
	glVertex2s( center_x + wover2, starty - h );
	glVertex2s( center_x + wover2, starty - 2*h );
	glVertex2s( center_x - wover2, starty - 2*h );
	glEnd();

	// draw the "warning message" box on the left side of the bar
	glColor3f( 0.f, .5f, .5f );
	glEnable( GL_POLYGON_STIPPLE );
	glPolygonStipple(stippleMask[10]);
	glEnable( GL_LINE_STIPPLE );
	glLineStipple( 4, 0xAAAA );
	glBegin( GL_QUADS );
	glVertex2s( endx - w, starty - h );
	glVertex2s( endx, starty - h );
	glVertex2s( endx, starty - 2*h );
	glVertex2s( endx - w, starty - 2*h );
	glEnd();
	glBegin( GL_LINES );
	glVertex2s( endx - w, starty - h );
	glVertex2s( endx, starty - h );
	glVertex2s( endx, starty - 2*h );
	glVertex2s( endx - w, starty - 2*h );
	glEnd();

	glDisable( GL_POLYGON_STIPPLE );
	glDisable( GL_LINE_STIPPLE );

	EndGL2DDrawing();
	glEnable( GL_DEPTH_TEST );

	return;

}