#include "IESDataStdAfx.h"
#pragma hdrstop

#include "GraphicsObjects.h"
#include "Core/Graphics/GraphicsHelpers.h"

#include "Data/Model.h"
#include "Data/Node.h"
#include "Data/Member.h"
#include "Data/Planar.h"
#include "Units/Units.h"
#include "Data/datautil.h"

#define PI	3.1415926535

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

CGraphicsObjects::CGraphicsObjects( CWnd* pParent, unsigned int id ) :
m_pParentWnd( pParent ), m_boundingRect( 0, 0, 0, 0 ), m_graphicsID( id ),
m_rotationAngle( 0. ), m_Color( RGB( 0, 0, 0 ) )
{
}

CGraphicsObjects::~CGraphicsObjects(void)
{
}

const short graphicObjectVersion = 1;

ETFileError CGraphicsObjects::read( ibfstream& in )
{
   short  version;
   in >> version;
 
   if( ( version <= graphicObjectVersion ) && ( version > 0 ) )
   {
	in >> m_boundingRect.left; in >> m_boundingRect.right; in >> m_boundingRect.top; in >> m_boundingRect.bottom;
	long l; in >> l; m_graphicsID = l;
	in >> m_rotationAngle;
	in >> l; m_Color =  l;
	
	in >> m_bSelectable;
   }
   
   if( !in ) return FILE_READ_ERROR;
   else return FILE_SUCCESS;
} // read()

ETFileError CGraphicsObjects::write( obfstream& out, int /*options*/ ) const
{

    out << graphicObjectVersion;

	out << m_boundingRect.left; out << m_boundingRect.right; out << m_boundingRect.top; out << m_boundingRect.bottom;
	long l = m_graphicsID; out << l;
	out << m_rotationAngle;
	l = m_Color; out << l;
	
	out << m_bSelectable;

	return FILE_SUCCESS;
} // write()




//  Text Objects
CGraphicsTextObject::CGraphicsTextObject( CWnd* pParent, const char* fontName, unsigned int id ) 
  : CGraphicsObjects( pParent, id ),
	m_ControlPoint( 0, 0 ),	m_ControlType( TEXT_UPPER_LEFT ), 
	m_PointSize( 12 ), m_FontName( fontName ), m_Text( "" ), m_Lengths( "" ),
	m_overallWidth( 0 ), m_overallHeight( 0 ), m_outlineFont( NULL ), m_bitmapFont( NULL ),
	m_bSelectable( true )
{
}

CGraphicsTextObject::CGraphicsTextObject( const CGraphicsTextObject* pObj ) : 
CGraphicsObjects( pObj->m_pParentWnd, pObj->m_graphicsID ),
m_ControlPoint( pObj->m_ControlPoint ),	
m_ControlType( pObj->m_ControlType ), 
m_PointSize( pObj->m_PointSize ), 
m_FontName( pObj->m_FontName ), 
m_Text( pObj->m_Text ), 
m_Lengths( pObj->m_Lengths ),
m_overallWidth( pObj->m_overallWidth ), 
m_overallHeight( pObj->m_overallHeight ), 
m_outlineFont( pObj->m_outlineFont ), 
m_bitmapFont( pObj->m_bitmapFont ),
m_bSelectable( pObj->m_bSelectable )
{
}

CGraphicsTextObject::~CGraphicsTextObject(void)
{
}

const short graphicTextObjectVersion = 1;

ETFileError CGraphicsTextObject::read( ibfstream& in )
{
   CGraphicsObjects::read( in );
   short  version;
   in >> version;
   if( ( version <= graphicTextObjectVersion ) && ( version > 0 ) )
   {
 	in >> m_ControlPoint.x; in >> m_ControlPoint.y;
	long l; in >> l; m_ControlType = (m_ETTextControl)l; 
	in >> l; m_PointSize = l;
	in >> m_FontName;
	in >> l; m_FontPixelHeight = l;

	in >> m_Text;
	in >> m_Lengths;
	in >> m_Line;

	in >> l; m_End = l;
	in >> l; m_LengthEnd = l;
	in >> l; m_overallHeight = l;
	in >> l; m_overallWidth = l;

	m_outlineFont = NULL;
	m_bitmapFont = NULL;

	in >> m_bSelectable;

   }
   
   if( !in ) return FILE_READ_ERROR;
   else return FILE_SUCCESS;
} // read()



ETFileError CGraphicsTextObject::write( obfstream& out, int options ) const
{
	CGraphicsObjects::write( out, options );
    out << graphicTextObjectVersion;

	out << m_ControlPoint.x; out << m_ControlPoint.y;
	long l = m_ControlType; out << l;
	l = m_PointSize; out << l;
	out << m_FontName;
	l = m_FontPixelHeight; out << l;

	out << m_Text;
	out << m_Lengths;
	out << m_Line;

	l = m_End; out << l;
	l = m_LengthEnd; out << l;
	l = m_overallHeight; out << l;
	l = m_overallWidth; out << l;

	out << m_bSelectable;

	return FILE_SUCCESS;
} // write()



const CRect CGraphicsTextObject::boundingRectangle( void )
{
	return m_boundingRect;
}


bool CGraphicsTextObject::pointIsOver( const CPoint& point )
{
	if( !m_bSelectable ) return false; 
	if( point.x >= m_boundingRect.left && 
		point.x <= m_boundingRect.right && point.y >= m_boundingRect.top && 
		point.y <= m_boundingRect.bottom  )
		return true;
	else
		return false;
}

void CGraphicsTextObject::Draw( CDC* pDC, const CPoint& offset )
{
	//use the Draw function below...it works for printing as well as normal drawing
	ASSERT( FALSE );
	RECT r;
	m_pParentWnd->GetClientRect( &r );
	Draw( pDC, offset, r );
}
void CGraphicsTextObject::Draw( CDC* pDC, const CPoint& offset, const RECT WndRect, int printScale ) 
{
	// render the text object
	//ASSERT( m_pParentWnd );
	if( !m_pParentWnd || !IsWindow( m_pParentWnd->m_hWnd ) )
		return;

	//when printing we reduce the image size because the print resolution is usually very high and takes too long to 
	//render in OpenGL. We apply a correction to the text here so that it sizes properly with the printed version
	//See CGraphicView::OnBeginPrinting for more details
	
    int pixely = (int)((float)pDC->GetDeviceCaps( LOGPIXELSY )/(float)printScale);
	m_FontPixelHeight = MulDiv( int((float)m_PointSize/*/(float)printScale*/), pixely, 72);
	
	CBitmapFont* pFont = NULL;
	COutlineFont* pFont1 = NULL;
	// select the proper font, use outlines for rotated text, bitmap fonts otherwise
	//if( equal( RotationAngle(), 0. ) )
	//	pFont = m_bitmapFont;
	//else {
		pFont1 = m_outlineFont;
	//}

	if( pFont || pFont1 ) {
		// start off by setting up OPENGL so that the window client size is the viewport and
		// scaled so that 1 model space unit is 1 pixel
		RECT r;
		r = WndRect;
		//m_pParentWnd->GetClientRect( &r );
		int width = r.right - r.left;
		int height = r.bottom - r.top;

		// TeK Changed 11/20/2007: from ASSERT( width > 0 && height > 0 );
		if( (width <= 0) || (height <=0) ) {
			m_pParentWnd->GetClientRect( &r );
			width = r.right - r.left;
			height = r.bottom - r.top;
		}
		
		StartGL2DDrawing();

		// Make the world and window coordinates coincide so that 1.0 in 
		// model space equals one pixel in window space.   
		// note that client coordinates are not quite model space because the origin in 
		// model space is in the center of the window with y up whereas in client 
		// coordinates, origin is upper left with y down.

		GLdouble size = (GLdouble)((width >= height) ? width : height) / 2.0;
		//size*=(GLdouble)printScale;
		GLdouble aspect;
		if (width <= height) {
			aspect = (GLdouble)height/(GLdouble)width;
			glOrtho(-size, size, -size*aspect, size*aspect, -1000000.0, 1000000.0);
		}
		else {
			aspect = (GLdouble)width/(GLdouble)height;
			glOrtho(-size*aspect, size*aspect, -size, size, -1000000.0, 1000000.0);
		}

		// set up the color
		glColor3ub( GetRValue( m_Color ), GetGValue( m_Color ), GetBValue( m_Color ) );

		int iy = 0;
		if( m_ControlType == TEXT_UPPER_LEFT || m_ControlType == TEXT_UPPER_RIGHT ) {
				iy = m_boundingRect.top;
		}
		else if( m_ControlType == TEXT_LOWER_LEFT || m_ControlType == TEXT_LOWER_RIGHT ) {
			iy = m_boundingRect.bottom - m_overallHeight;
		}
		else if( m_ControlType == TEXT_CENTERED_ON ) {
			iy = m_boundingRect.CenterPoint().y - m_overallHeight/2;
		}

		// now loop on all of the lines in this object
		int length;
		CString line = FirstLine( &length );
		iy += m_FontPixelHeight;
		int iLine = 0;
		while( line.GetLength() > 0 ) {
			iLine++;
			int ix = 0;
			if( m_ControlType == TEXT_UPPER_LEFT || m_ControlType == TEXT_LOWER_LEFT ) {
				ix = m_boundingRect.left;
			}
			else if( m_ControlType == TEXT_UPPER_RIGHT || m_ControlType == TEXT_LOWER_RIGHT ) {
				ix = m_boundingRect.right - length;
			}
			else if( m_ControlType == TEXT_CENTERED_ON ) {
				ix = m_boundingRect.CenterPoint().x - length/2;
			}

			if( pFont1 ) {  // we've got rotated text, handle it differently to take into acount the rotation angle
				double offsety = ((double(LineCount())-1.)/2. - (iLine-1.))*m_FontPixelHeight;
				double theta = RotationAngle();
				int cx = m_boundingRect.CenterPoint().x - int(offsety*sin(theta));
				int cy = m_boundingRect.CenterPoint().y - int(offsety*cos(theta));
				ix = cx - int( double(length)/2.*cos(theta));
				iy = cy + int( double(length)/2.*sin(theta));
			}

			GLfloat x = (GLfloat)(ix - width/2 + offset.x);
			GLfloat y = (GLfloat)(height/2 - iy - offset.y);  // note we are converting mouse point offsets here so +y is down
			//render to origin then offset to avoid clipping problems
			//http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
			glPushMatrix();
			if( pFont ) {  // bitmap fonts
				glScaled(aspect, aspect, 1.0);
				glRasterPos2i( 0, 0 );
				glBitmap( 0, 0, 0, 0, x, y, NULL );
				pFont->DrawString( line );
			}
			else if( pFont1 ) { // outline fonts
				glRasterPos2i( 0, 0 );
				glTranslated(x*aspect, y*aspect, 0.0);
				glScaled(m_FontPixelHeight*aspect, m_FontPixelHeight*aspect, 
					m_FontPixelHeight*aspect);
				glRotated(RotationAngle()*180./PI, 0.0, 0.0, 1.0);
				glColor3ub( GetRValue( m_Color ), GetGValue( m_Color ), GetBValue( m_Color ) );
				pFont1->DrawString( line );
			}
			glPopMatrix();
			iy += m_FontPixelHeight;
			line = NextLine( &length );
		}
		// restore settings
		EndGL2DDrawing();
		
	}

	return;
}

void CGraphicsTextObject::Draw( CDC* pDC, int printScale ) 
{
	COutlineFont* pFont1 = m_outlineFont;
	if( pFont1 ) 
	{
		int iy = 0;
		int pixely = (int)((float)pDC->GetDeviceCaps( LOGPIXELSY )/(float)printScale);
		m_FontPixelHeight = MulDiv( int((float)m_PointSize), pixely, 72);
	
		// now loop on all of the lines in this object
		int length;
		CString line = FirstLine( &length );
		iy += m_FontPixelHeight;
		//int iLine = 0;
		while( line.GetLength() > 0 ) {
			glPushMatrix();
			if( pFont1 ) { // outline fonts
				//glRasterPos2i( 0, 0 );
				glTranslated(0.0, iy, 0.0);
				glScaled(m_FontPixelHeight, m_FontPixelHeight, 
					m_FontPixelHeight);
				glColor3ub( GetRValue( m_Color ), GetGValue( m_Color ), GetBValue( m_Color ) );
				pFont1->DrawString( line );
			}
			glPopMatrix();
			iy += m_FontPixelHeight;
			line = NextLine( &length );
		}
	}
}

void CGraphicsTextObject::updateBoundingBox( bool newFont )
{
	// located the boundary of the text to be displayed
	ASSERT( m_pParentWnd );
	if( !m_pParentWnd || !IsWindow( m_pParentWnd->m_hWnd ) )
		return;
	if( newFont ) {
		m_Lengths = "";
		HDC dc = ::GetDC( m_pParentWnd->m_hWnd );
		int pixely = GetDeviceCaps( dc, LOGPIXELSY );
		m_FontPixelHeight = MulDiv( m_PointSize, pixely, 72);
		m_overallHeight = 0;
		m_overallWidth = 0;

		PLOGFONT plCurrentFont = (PLOGFONT)LocalAlloc( GPTR, sizeof(LOGFONT));		// allocate memory for the font
		lstrcpy( plCurrentFont->lfFaceName, m_FontName );
		plCurrentFont->lfHeight = -m_FontPixelHeight;
		plCurrentFont->lfWidth = 0;
		plCurrentFont->lfWeight = FW_REGULAR;
		plCurrentFont->lfItalic = false;						// no italics
		plCurrentFont->lfUnderline = 0;					// dont underline
		plCurrentFont->lfStrikeOut = 0;					// dont strikeout
		int orientation = int( (RotationAngle()*180./PI)*10. );
		plCurrentFont->lfOrientation = orientation;
		plCurrentFont->lfEscapement = orientation;
		plCurrentFont->lfClipPrecision = CLIP_LH_ANGLES;
		HFONT font = CreateFontIndirect( plCurrentFont );
		HGDIOBJ oldObject = SelectObject( dc, font );
		if( m_ControlType == TEXT_UPPER_LEFT )
			SetTextAlign( dc, TA_LEFT | TA_TOP );
		else if( m_ControlType == TEXT_LOWER_LEFT )
			SetTextAlign( dc, TA_LEFT | TA_BOTTOM );
		else if( m_ControlType == TEXT_UPPER_RIGHT )
			SetTextAlign( dc, TA_RIGHT | TA_TOP );
		else if( m_ControlType == TEXT_LOWER_RIGHT )
			SetTextAlign( dc, TA_RIGHT | TA_BOTTOM );
		else if( m_ControlType == TEXT_CENTERED_ON )
			SetTextAlign( dc, TA_CENTER | TA_BASELINE );
		
		int length;
		CString line = FirstLine( &length );
		while( line.GetLength() > 0 ) {
			RECT r;
			r.top = 0;
			r.bottom = 0;
			r.left = 0;
			r.right = 0;
			DrawText( dc, line, line.GetLength(), &r, DT_CALCRECT | DT_SINGLELINE );
			int width = r.right - r.left;
			m_overallWidth = max( m_overallWidth, width );
			m_overallHeight += (r.bottom - r.top);
			CString s;
			s.Format( "%d\r", width );
			m_Lengths += s;
			line = NextLine( &length );
		}
		SelectObject( dc, oldObject );
		DeleteObject( font );
		LocalFree( (LOCALHANDLE)plCurrentFont );
		::ReleaseDC( m_pParentWnd->m_hWnd, dc );
	}

	if( m_ControlType == TEXT_UPPER_LEFT ) {
		m_boundingRect.left = m_ControlPoint.x;
		m_boundingRect.right = m_ControlPoint.x + m_overallWidth;
		m_boundingRect.top = m_ControlPoint.y;
		m_boundingRect.bottom = m_ControlPoint.y + m_overallHeight;
	}
	else if( m_ControlType == TEXT_LOWER_LEFT ) {
		m_boundingRect.left = m_ControlPoint.x;
		m_boundingRect.right = m_ControlPoint.x + m_overallWidth;
		m_boundingRect.top = m_ControlPoint.y - m_overallHeight;
		m_boundingRect.bottom = m_ControlPoint.y;
	}
	else if( m_ControlType == TEXT_UPPER_RIGHT ) {
		m_boundingRect.left = m_ControlPoint.x - m_overallWidth;
		m_boundingRect.right = m_ControlPoint.x;
		m_boundingRect.top = m_ControlPoint.y;
		m_boundingRect.bottom = m_ControlPoint.y + m_overallHeight;
	}
	else if( m_ControlType == TEXT_LOWER_RIGHT ) {
		m_boundingRect.left = m_ControlPoint.x - m_overallWidth;
		m_boundingRect.right = m_ControlPoint.x;
		m_boundingRect.top = m_ControlPoint.y - m_overallHeight;
		m_boundingRect.bottom = m_ControlPoint.y;
	}
	else if( m_ControlType == TEXT_CENTERED_ON ) {
		m_boundingRect.left = m_ControlPoint.x - m_overallWidth/2;
		m_boundingRect.right = m_ControlPoint.x + m_overallWidth/2;
		m_boundingRect.top = m_ControlPoint.y - m_overallHeight/2;
		m_boundingRect.bottom = m_ControlPoint.y + m_overallHeight/2;
	}

	return;
}

void CGraphicsTextObject::ClearText( void )
{
	m_Text = "";
	m_Lengths = "";
	m_boundingRect.SetRectEmpty();
	return;
}

void CGraphicsTextObject::AddLine( const char* line )
{
	m_Text += line;
	m_Text += "\r";   // new line delimiter
	updateBoundingBox( true );
	return;
}

int CGraphicsTextObject::LineCount( void )
{
	int count = 0;
	int i = 0;
	while( i < m_Text.GetLength() ) {
		if( m_Text[i] == '\r' )
			count++;
		i++;
	}
	return count;
}

const CString& CGraphicsTextObject::FirstLine( int* length ) {
	// return the first line of text to be displayed
	m_LengthEnd = m_Lengths.Find( "\r" );
	int l;
	if( sscanf( m_Lengths.Mid( 0, m_LengthEnd ), "%d", &l ) == 1 )
		*length = l;
	else
		*length = 0;
	m_End = m_Text.Find( "\r" );
	m_Line = m_Text.Mid( 0, m_End );
	return m_Line;
}

const CString& CGraphicsTextObject::NextLine( int* length ) {
	int startLength = m_LengthEnd + 1;
	if( startLength > m_Lengths.GetLength() )
		*length = 0;
	m_LengthEnd = m_Lengths.Find( "\r", startLength );
	if( m_LengthEnd < 1 )
		*length = 0;
	else {
	int l;
		if( sscanf( m_Lengths.Mid( startLength, m_LengthEnd-startLength ), "%d", &l ) == 1 )
			*length = l;
		else
			*length = 0;
	}
	int start = m_End+1;
	m_Line = "";
	if( start > m_Text.GetLength() ) 
		return m_Line;
	m_End = m_Text.Find( "\r", start );
	if( m_End < 1 )
		return m_Line;
	else  {
		m_Line = m_Text.Mid( start, m_End-start );
		return m_Line;
	}
}



COutlineFont::COutlineFont( CDC* pDC, const CString& fontname, int pointSize ) 
	: m_typeFace( fontname ), m_InitialPointSize( pointSize ) 
{
	TRACE( "COutlineFont::COutlineFont( %s, %d)\n", fontname, pointSize );
    // Class constructor.
    // Stores each character in its own display list
    // for later drawing via the wglUseFontOutlines() call.

    if (pDC && fontname && strlen(fontname) > 0 && m_InitialPointSize > 0 ) {

        m_listbase = glGenLists(256);

		int oldMode = pDC->SetMapMode( MM_TEXT );
		int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
		int fontPixelHeight = MulDiv( m_InitialPointSize, pixely, 72);
//		int orientation = int( (angle*180./PI)*10. );

        // Setup the Font characteristics
        LOGFONT logfont;
        GLYPHMETRICSFLOAT gmf[256];

        logfont.lfHeight        = -fontPixelHeight;
        logfont.lfWidth         = 0;
        logfont.lfEscapement    = 0;
        logfont.lfOrientation   = 0;
        logfont.lfWeight        = FW_NORMAL;
        logfont.lfItalic        = FALSE;
        logfont.lfUnderline     = FALSE;
        logfont.lfStrikeOut     = FALSE;
        logfont.lfCharSet       = ANSI_CHARSET;
        logfont.lfOutPrecision  = OUT_DEFAULT_PRECIS;
        logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        logfont.lfQuality       = DEFAULT_QUALITY;
        logfont.lfPitchAndFamily = FF_DONTCARE|DEFAULT_PITCH;
        strcpy(logfont.lfFaceName, fontname);

        CFont font;
        CFont* oldfont;
        BOOL success = font.CreateFontIndirect(&logfont);
        oldfont = pDC->SelectObject(&font);
        if (!success || 
            FALSE == wglUseFontOutlines(
                pDC->m_hDC, 
                0, 
                256, 
                m_listbase,
                0.0, 
                0.0, 
                WGL_FONT_POLYGONS,
                gmf)) {
            glDeleteLists(m_listbase, 256);
            m_listbase = 0;
        }
        else {
            pDC->SelectObject(oldfont);
        }
		pDC->SetMapMode( oldMode );
    }
}

COutlineFont::~COutlineFont()
{
    // Class destructor.

    glDeleteLists(m_listbase, 256);
    m_listbase = 0;
}

void COutlineFont::DrawString(const CString& s)
{
    // Draws the given text string.
	GLsizei len = GLsizei(s.GetLength());
    if (s && len > 0) {
		// Must save/restore the list base.
		glPushAttrib(GL_LIST_BIT);{
			glListBase(m_listbase);
			glCallLists(len, GL_UNSIGNED_BYTE, (const GLvoid*)s);
		} glPopAttrib();
	}
}



COutlineFontManager::COutlineFontManager()
	: mFonts( ARRAY_SIZE, 1, ARRAY_DELTA, YES_DELETE )
{
	return;
} // end COutlineFontManager()


COutlineFontManager::~COutlineFontManager() {
	mFonts.flush();
	return;
} // end ~COutlineFontManager()


void COutlineFontManager::addFont( COutlineFont& aFont ) {
	mFonts.add( &aFont );
	return;
} // end addFont()


void COutlineFontManager::removeFont( const COutlineFont& aFont, ETShouldDelete sd ) {
	mFonts.detach( (COutlineFont*)&aFont, sd );	// cast away const
	return;
} // end removeFont()

void COutlineFontManager::clearAll()
{

}
void COutlineFontManager::removeAll()
{
	mFonts.flush();
	return;
}

int COutlineFontManager::fonts() const {
	return mFonts.getItemsInContainer();
} // end fonts()

COutlineFont* COutlineFontManager::font( int i ) const {
	if( i > 0 && i <= fonts() ) 
	{
		return mFonts[i];
	}
	else
	{
		ASSERT( FALSE ); 
		return NULL;
	}
} // end font()

COutlineFont* COutlineFontManager::getFont( CDC* pDC, const CString& typeFace, int pointSize )
{
	// shearch the current set and look for this font and if it does not exist,
	// create it
	ASSERT( pDC );
	for( int i = 1; i <= fonts(); i++ ) {
		COutlineFont* pF = mFonts[i];
		if( pF->typeFace() == typeFace && pF->basePointSize() == pointSize )
			return pF;
	}
	// we've got to create one
	COutlineFont* pF = new COutlineFont( pDC, typeFace, pointSize );
	addFont( *pF );
	return pF;
}

CBitmapFont::CBitmapFont( CDC* dc, const CString& fontname, int fontSize)
   : m_typeFace( fontname ), m_fontSize( fontSize )
{
    // Class constructor.
    // Stores the bitmaps for each character in its own display list
    // for later drawing via the wglUseFontBitmaps() call.

    if (dc && fontname && strlen(fontname) > 0) {

        m_listbase = glGenLists(256);
		if( !glIsList( m_listbase ) ){
			ASSERT(false);
			AfxMessageBox("bad list");
		}

		int oldMode = dc->SetMapMode( MM_TEXT );
		int pixely = dc->GetDeviceCaps( LOGPIXELSY );
		int fontPixelHeight = MulDiv( fontSize, pixely, 72);

		LOGFONT logfont;
		logfont.lfHeight = -fontPixelHeight;
		logfont.lfWidth = 0;
		logfont.lfEscapement = 0;
		logfont.lfOrientation = logfont.lfEscapement;
		logfont.lfWeight = FW_NORMAL;
		logfont.lfItalic = FALSE;
		logfont.lfUnderline = FALSE;
		logfont.lfStrikeOut = FALSE;
		logfont.lfCharSet = ANSI_CHARSET;
		logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
		logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		logfont.lfQuality = DEFAULT_QUALITY;
		logfont.lfPitchAndFamily = FF_DONTCARE|DEFAULT_PITCH;
		lstrcpy(logfont.lfFaceName, fontname);

		CFont font;
		CFont* oldfont;
		BOOL success = font.CreateFontIndirect(&logfont);
		oldfont = dc->SelectObject(&font);
		if ( !success ) {
			glDeleteLists(m_listbase, 256);
			m_listbase = 0;
		} 
		else if( FALSE == wglUseFontBitmapsW(dc->m_hDC, 0, 256, m_listbase) &&
			     FALSE == wglUseFontBitmapsW(dc->m_hDC, 0, 256, m_listbase) )  {
			glDeleteLists(m_listbase, 256);
			m_listbase = 0;
		}
		else {
			dc->SelectObject(oldfont);
		}
		dc->SetMapMode( oldMode );
    }
}

CBitmapFont::~CBitmapFont()
{
    // Class destructor.

    glDeleteLists(m_listbase, 256);
    m_listbase = 0;
}


void
CBitmapFont::DrawString( const CString& s )
{
    // Draws the given text string at the given location.
    GLsizei len = GLsizei(s.GetLength());
    if (s && len > 0) {
		// Must save/restore the list base.
		glPushAttrib(GL_LIST_BIT);{
			glListBase(m_listbase);
			glCallLists(len, GL_UNSIGNED_BYTE, (const GLvoid*)s);
		} glPopAttrib();
		glEnd();
	}
}


CBitmapFontManager::CBitmapFontManager()
	: mFonts( ARRAY_SIZE, 1, ARRAY_DELTA, YES_DELETE )
{
	return;
} // end CBitmapFontManager()


CBitmapFontManager::~CBitmapFontManager() {
	mFonts.flush();
	return;
} // end ~CBitmapFontManager()


void CBitmapFontManager::addFont( CBitmapFont& aFont ) {
	mFonts.add( &aFont );
	return;
} // end addFont()


void CBitmapFontManager::removeFont( const CBitmapFont& aFont, ETShouldDelete sd ) {
	mFonts.detach( (CBitmapFont*)&aFont, sd );	// cast away const
	return;
} // end removeFont()

void CBitmapFontManager::clearAll()
{
	//for( int i = 0; i < mFonts.getItemsInContainer(); i++ ){
}

void CBitmapFontManager::removeAll( ) {
	mFonts.flush();	// cast away const
	return;
} // end removeFont()

int CBitmapFontManager::fonts() const {
	return mFonts.getItemsInContainer();
} // end fonts()

CBitmapFont* CBitmapFontManager::font( int i ) const {
	if( i > 0 && i <= fonts() ) 
	{
		return mFonts[i];
	}
	else
	{
		ASSERT( FALSE ); 
		return NULL;
	}
} // end font()

CBitmapFont* CBitmapFontManager::getFont( CDC* pDC, const CString& typeFace, int pointSize )
{
	// search the current set and look for this font and if it does not exist,
	// create it
	ASSERT( pDC );
	for( int i = 1; i <= fonts(); i++ ) {
		CBitmapFont* pF = mFonts[i];
		if( pF->typeFace() == typeFace &&
			pF->pointSize() == pointSize )
			return pF;
	}
	// we've got to create one
	CBitmapFont* pF = new CBitmapFont( pDC, typeFace, pointSize );
	addFont( *pF );
	return pF;
}



CGraphicsTextManager::CGraphicsTextManager()
	: mTextObjects( ARRAY_SIZE, 1, ARRAY_DELTA, YES_DELETE ),
	  mOffsets( ARRAY_SIZE, 1, ARRAY_DELTA, YES_DELETE )
{
	m_nObjectPointIsOver = 0;
	return;
} // end CGraphicsTextManager()


CGraphicsTextManager::~CGraphicsTextManager() {
	mTextObjects.flush();
	mOffsets.flush();
	return;
} // end ~CGraphicsTextManager()


void CGraphicsTextManager::addTextObject( CGraphicsTextObject& text ) {
	mTextObjects.add( &text );
	return;
} // end addTextObject()


void CGraphicsTextManager::removeTextObject( const CGraphicsTextObject& text, ETShouldDelete sd ) {
	mTextObjects.detach( (CGraphicsTextObject*)&text, sd );	// cast away const
	return;
} // end removeFont()

int CGraphicsTextManager::textObjects() const {
	return mTextObjects.getItemsInContainer();
} // end fonts()

CGraphicsTextObject* CGraphicsTextManager::textObject( int i ) const {
	if( i > 0 && i <= textObjects() ) 
	{
		return mTextObjects[i];
	}
	else
	{
		ASSERT( FALSE ); 
		return NULL;
	}
} // end textObject()

void CGraphicsTextManager::Draw( CDC* pDC, const RECT r, int printScale )
{
	// eventually this needs to get smart to keep objects from being draw over the top
	// of other objects
	for( int i = 1; i <= textObjects(); i++ ) {
		if( mTextObjects[i] ) {
			// set up the proper drawing font
			if( !equal( mTextObjects[i]->RotationAngle(), 0. ) )
				mTextObjects[i]->SetOutlineFont( m_outlineFontManager.getFont( pDC, 
				mTextObjects[i]->FontName(), (int)((float)( mTextObjects[i]->PointSize())/(float)printScale ) ) );
			else
				mTextObjects[i]->SetBitmapFont( m_bitmapFontManager.getFont( pDC, 
				mTextObjects[i]->FontName(), (int)((float)( mTextObjects[i]->PointSize())/(float)printScale ) ) );
			mTextObjects[i]->Draw( pDC, offset( mTextObjects[i] ), r, printScale );
		}
	}

}

void CGraphicsTextManager::clearAll( )
{
	for( int i = 1; i <= mTextObjects.getItemsInContainer(); i++ )
		mTextObjects[i]->ClearText();
	return;
}

void CGraphicsTextManager::removeAll( void )
{
	m_bitmapFontManager.removeAll();
	m_outlineFontManager.removeAll();
	mTextObjects.flush();
	//mOffsets.flush();
	return;
}

CGraphicsTextObject* CGraphicsTextManager::pointIsOver( const CPoint& p )
{
	m_nObjectPointIsOver = 0;
	for( int i = 1; i <= textObjects(); i++ ) {
		if( mTextObjects[i] ) {
			CPoint pt = p - offset( mTextObjects[i] );
			if( mTextObjects[i]->pointIsOver( pt ) ) {
				m_nObjectPointIsOver = i;
				return mTextObjects[i];
			}
		}
	}
	return NULL;
}


void CGraphicsTextManager::setOffset( const CPoint& offset_increment, unsigned int id )
{
	// first see if this "id" already has an offset and if so, just change it
	for( int i = 1; i <= offsets(); i++ ) {
		if( mOffsets[i] ) {
			if( mOffsets[i]->id() == id ) {
				CPoint newOffset;
				newOffset = mOffsets[i]->offset();
				newOffset += offset_increment;
				mOffsets[i]->setOffset( newOffset );
				return;
			}
		}
	}
	// we've got to add this one
	CGraphicsTextOffset* pOffset = new CGraphicsTextOffset( offset_increment, id );
	mOffsets.add( pOffset );
	return;
}

int	CGraphicsTextManager::offsets() const
{
	return mOffsets.getItemsInContainer();
}

CGraphicsTextOffset* CGraphicsTextManager::offset( int i )
{
	if( i > 0 && i <= offsets() ) 
	{
		return mOffsets[i];
	}
	else
	{
		ASSERT( FALSE ); 
		return NULL;
	}
}

const CPoint& CGraphicsTextManager::offset( CGraphicsTextObject* pText ) const
{
	for( int i = 1; i <= offsets(); i++ ) {
		if( mOffsets[i] ) {
			if( mOffsets[i]->id() == pText->GraphicsID() )
				return mOffsets[i]->offset();
		}
	}
	// no offset, just return null
	static CPoint p( 0, 0 );
	return p;
}



//  Annotation Objects and their management

CGraphicsAnnotationObject::CGraphicsAnnotationObject( m_ETAnnotationType type, CWnd* pParent, 
													 const char* fontName, unsigned int id, 
													 const CData* pObject1, const CData* pObject2,
													 bool bFixedPosition ) 
  : CGraphicsTextObject( pParent, fontName,id ), m_type( type ), 
  m_dataObject1( pObject1 ), m_dataObject2( pObject2 ),
  m_units(  LENGTH_LARGE_UNIT  ), m_bFixedPosition( bFixedPosition )
{
	if( type == ANNOTATION_DIMENSION )
		SetControlType( TEXT_CENTERED_ON );
	else
		SetControlType( TEXT_UPPER_RIGHT );
}


CGraphicsAnnotationObject::~CGraphicsAnnotationObject(void)
{
}

const short graphicAnnotationObjectVersion = 1;

ETFileError CGraphicsAnnotationObject::read( ibfstream& in )
{
   CGraphicsTextObject::read( in );
   short  version;
   in >> version;
   if( ( version <= graphicAnnotationObjectVersion ) && ( version > 0 ) )
   {
	long l; in >> l; m_type = (m_ETAnnotationType)l;
	in >> m_leaderToPoint3D[0].x;	in >> m_leaderToPoint3D[0].y;	in >> m_leaderToPoint3D[0].z;
	in >> m_leaderToPoint3D[1].x;	in >> m_leaderToPoint3D[1].y;	in >> m_leaderToPoint3D[1].z;
	in >> l;  m_units = (ETUnit)l;
	in >> m_dimensionLength;
	in >> m_bFixedPosition;
	// read in the CData Objects, currently only allow nodes and members to be saved
	short count; in >> count;
	m_dataObject1 = m_dataObject2 = NULL;
	if( count == 1 ) {
		CString type;
		in >> type;
		long index;
		in >> index;
		ASSERT( index >= 1 );
		if( type == "Node" )
			m_dataObject1 = theModel.node( int(index) );
		else if( type == "Member" )
			m_dataObject1 = theModel.element( MEMBER_ELEMENT, int(index) );
		else if( type == "Planar" ) {  // note negative index for meshed planars may exist but that's OK
			m_dataObject1 = theModel.element( PLANAR_ELEMENT, int(index) );
		}
		else
			ASSERT( FALSE );
	}
	else if( count == 2 ) {
		CString type;
		in >> type;
		long index1, index2;
		in >> index1;
		in >> index2;
		ASSERT( index2 >= 1 );
		ASSERT( index2 >= 1 );
		if( type == "Node" ) {
			m_dataObject1 = theModel.node( index1 );
			m_dataObject2 = theModel.node( index2 );
		}
		else
			ASSERT( FALSE );
	}
	in >> m_3DPosition.x; in >> m_3DPosition.y; in >> m_3DPosition.z;
   
   }  // version OK
   
   if( !in ) return FILE_READ_ERROR;
   else return FILE_SUCCESS;
} // read()



ETFileError CGraphicsAnnotationObject::write( obfstream& out, int options ) const
{
	CGraphicsTextObject::write( out, options );
    out << graphicAnnotationObjectVersion;

	long l = m_type; out << l;
	out << m_leaderToPoint3D[0].x;	out << m_leaderToPoint3D[0].y;	out << m_leaderToPoint3D[0].z;
	out << m_leaderToPoint3D[1].x;	out << m_leaderToPoint3D[1].y;	out << m_leaderToPoint3D[1].z;
	l = m_units; out << l;
	out << m_dimensionLength;
	out << m_bFixedPosition;
	// write out the CData Objects, currently only allow nodes and members to be saved
	short count = 0;
	if( m_dataObject1 != NULL ) 
		count = 1;
	if( m_dataObject1 != NULL && m_dataObject2 != NULL )
		count = 2;
	if( count == 1 ) {
		const CNode* pN = dynamic_cast<const CNode*>(m_dataObject1);
		const CMember* pM = dynamic_cast<const CMember*>(m_dataObject1);
		const CPlanar* pP = dynamic_cast<const CPlanar*>(m_dataObject1);
		if( pN ) {
			out << count;
			CString type = "Node";
			out << type;
			long n1 = theModel.index( pN );
			out << n1;
		}
		else if( pM ) {
			out << count;
			CString type = "Member";
			out << type;
			long n1 = theModel.index( pM );
			out << n1;
		}
		else if( pP ) {
			out << count;
			CString type = "Planar";
			out << type;
			long n1 = theModel.index( pP );  // may be negative for meshed planars
			out << n1;
		}
		else {
			ASSERT( FALSE );
			count = 0;
			out << count;
		}
	}
	else if( count == 2 ) {
		const CNode* pN1 = dynamic_cast<const CNode*>(m_dataObject1);
		const CNode* pN2 = dynamic_cast<const CNode*>(m_dataObject2);
		ASSERT( pN1 && pN2 );
		if( pN1 && pN2 ) {
			out << count;
			CString type = "Node";
			out << type;
			long n1 = theModel.index( pN1 );
			long n2 = theModel.index( pN2 );
			out << n1;
			out << n2;
		}
		else {
			count = 0;
			out << count;
		}
	}

	out << m_3DPosition.x; out << m_3DPosition.y; out << m_3DPosition.z;
 
	return FILE_SUCCESS;
} // write()


void CGraphicsAnnotationObject::Draw( CDC* pDC, const CPoint& offset, const CRect& r ) 
{
	// render the text object
	ASSERT( m_pParentWnd );
	if( !m_pParentWnd || !IsWindow( m_pParentWnd->m_hWnd ) )
		return;

	// draw the text first
	if( m_type != ANNOTATION_DIMENSION ) {
		// see if control point needs modified
		if( !hasFixedPosition() ) {  // yes we are attached to the structure!
			CPoint screenPt;
			if( GetScreenPtFrom3DPoint( m_3DPosition, screenPt ) ) 
				SetControlPoint( screenPt );
		}
		CGraphicsTextObject::Draw( pDC, offset, r );
	}
	if( m_type == ANNOTATION_WITH_LINE_LEADER || m_type == ANNOTATION_WITH_ARROW_LEADER ) {
		// now handle the leaders 
		CPoint screenPt;
		//CRect r;
		//m_pParentWnd->GetClientRect( &r );
		if( GetScreenPtFrom3DPoint( m_leaderToPoint3D[0], screenPt ) ) {
			//HGLRC hglrc = wglGetCurrentContext();
			//if( !hglrc )
			//{
			//	hglrc = wglCreateContext( pDC->GetSafeHdc() );
			//	wglMakeCurrent( pDC->GetSafeHdc(), hglrc );
			//}
			StartGL2DDrawing();
			glOrtho(double(r.left), double(r.right), double(r.bottom), double(r.top), -1., 1. );
			glDisable( GL_DEPTH_TEST );
			glColor3ub( GetRValue( m_Color ), GetGValue( m_Color ), GetBValue( m_Color ) );
			glPushMatrix();
			glBegin( GL_LINES );
			glLineWidth( float(1.) );
			int x, y;
			if( screenPt.x < m_boundingRect.CenterPoint().x+offset.x ) x = m_boundingRect.left;
			else x = m_boundingRect.right;
			if( screenPt.y < m_boundingRect.CenterPoint().y+offset.y ) y = m_boundingRect.top;
			else y = m_boundingRect.bottom;
			x += offset.x;
			y += offset.y;
			double dx = screenPt.x-x;
			double dy = screenPt.y-y;
			glVertex2i( screenPt.x, screenPt.y);
			glVertex2i( x, y );
			glEnd();
			if( m_type == ANNOTATION_WITH_ARROW_LEADER ) {
				float arrowlength;
				double arrowAngle;
				arrowlength = sqrt( float(dx)*float(dx) + float(dy)*float(dy) );
				if( arrowlength > 40. ) arrowlength = 40.;
				arrowAngle = atan2( dy, dx );
				glTranslatef( (float)screenPt.x, (float)screenPt.y, 0.f );	
				glRotatef( float(arrowAngle*180/M_PI), 0., 0.,	1. );
				DrawLineArrow( arrowlength, 1., false );
			}
			glPopMatrix();	
			glEnable( GL_DEPTH_TEST );
			EndGL2DDrawing();
		}
	}
	else if( m_type == ANNOTATION_DIMENSION ) {
		// set up the quantity
		CPoint screenPt1,  screenPt2;
		//CRect r;
		//m_pParentWnd->GetClientRect( &r );
		if( GetScreenPtFrom3DPoint( m_leaderToPoint3D[0], screenPt1 )  &&
			GetScreenPtFrom3DPoint( m_leaderToPoint3D[1], screenPt2 ) ) {
			double dx = screenPt2.x - screenPt1.x;
			double dy = screenPt2.y - screenPt1.y;
			double len = sqrt( dx*dx + dy*dy );
			if( len > 0. ) {
				CPoint pCenter( screenPt1.x + int(dx/2), screenPt1.y + int(dy/2) );
				double a = atan2( -dy, dx );
				bool bSwap = false;
				if( a > M_PI/2 ) {
					a -= M_PI;
					bSwap = true;
				}
				if( a < -M_PI/2 ) {
					a += M_PI;
					bSwap = true;
				}
				if( bSwap ) {
					dx = -dx;
					dy = -dy;
					CPoint pTemp = screenPt1;
					screenPt1 = screenPt2;
					screenPt2 = pTemp;
				}
				// by default, move the text above the center by 50 units
				CPoint pControl;
				pControl.x = pCenter.x - int(50*sin(a));
				pControl.y = pCenter.y - int(50*cos(a));
				SetControlPoint( pControl );
				// how far we offset our dimension lines from the base line
				int off = int(cos(a)*offset.y + sin(a)*offset.x) - 50 + FontPixelHeight()/2;
				int offx = int(-sin(a)*offset.y + cos(a)*offset.x);
				int iLoc = 0;  // centered
				double extensionLeader = 0.;
				if( offx < -len/2 ) {
					iLoc = -1;  // text is on the left end
					extensionLeader = -(offx+len/2);
				}
				else if( offx > len/2 ) {
					iLoc = 1;  // text is on the right end
					extensionLeader = offx-len/2;
				}
				// draw text
				SetRotationAngle( a );
				CGraphicsTextObject::Draw( pDC, offset, r );
				// draw the leader line
				//HGLRC hglrc = wglGetCurrentContext();
				//if( !hglrc ) {
				//	hglrc = wglCreateContext( pDC->GetSafeHdc() );
				//	wglMakeCurrent( pDC->GetSafeHdc(), hglrc );
				//}
				// draw dimension leaders
				StartGL2DDrawing();
				glOrtho(double(r.left), double(r.right), double(r.bottom), double(r.top), -1., 1. );
				glDisable( GL_DEPTH_TEST );
				glColor3ub( GetRValue( m_Color ), GetGValue( m_Color ), GetBValue( m_Color ) );
				glPushMatrix();
				glBegin( GL_LINES );
				glLineWidth( float(1.) );
				glVertex2i( screenPt1.x, screenPt1.y);
				int x = screenPt1.x - int(off*cos(a+M_PI/2));
				int y = screenPt1.y + int(off*sin(a+M_PI/2));
				glVertex2i( x, y );
				if( iLoc != 0 ) {
					if( iLoc < 0 ) {
						x -= int(extensionLeader*cos(a));
						y += int(extensionLeader*sin(a));
					}
					else {
						x -= int(30*cos(a));
						y += int(30*sin(a));
					}
				}
				glVertex2i( x, y );
				x = screenPt2.x - int(off*cos(a+M_PI/2));
				y = screenPt2.y + int(off*sin(a+M_PI/2));
				if( iLoc != 0 ) {
					if( iLoc > 0 ) {
						x += int(extensionLeader*cos(a));
						y -= int(extensionLeader*sin(a));
					}
					else {
						x += int(30*cos(a));
						y -= int(30*sin(a));
					}
				}
				glVertex2i( x, y );
				x = screenPt2.x - int(off*cos(a+M_PI/2));
				y = screenPt2.y + int(off*sin(a+M_PI/2));
				glVertex2i( x, y );
				glVertex2i( screenPt2.x, screenPt2.y);
				glEnd();
				glPopMatrix();	
				glEnable( GL_DEPTH_TEST );
				// now the arrows
				x = screenPt1.x - int(off*cos(a+M_PI/2));
				y = screenPt1.y + int(off*sin(a+M_PI/2));
				glTranslatef( (float)x, (float)y, 0.f );	
				double aa = -(a - M_PI);
				if( iLoc != 0 )
					aa = -a;
				glRotatef( float(aa*180/M_PI), 0., 0., 1. );
				DrawLineArrow( 40., 1., false );
				glRotatef( float(aa*180/M_PI), 0., 0., -1. );
				glTranslatef( (float)-x, (float)-y, 0.f );	
				x = screenPt2.x - int(off*cos(a+M_PI/2));
				y = screenPt2.y + int(off*sin(a+M_PI/2));
				glTranslatef( (float)x, (float)y, 0.f );	
				aa = -a;
				if( iLoc != 0 )
					aa = -(a - M_PI);
				glRotatef( float(aa*180/M_PI), 0., 0., 1. );
				DrawLineArrow( 40., 1., false );
				glRotatef( float(aa*180/M_PI), 0., 0., -1. );
				glTranslatef( (float)-x, (float)-y, 0.f );	

				EndGL2DDrawing();
			} // len > 0
		}
	}
	return;
}

void CGraphicsAnnotationObject::setLeaderTo( CPoint3D p, int iEnd ){ 
	m_leaderToPoint3D[iEnd] = p; 
	if( m_type == ANNOTATION_DIMENSION ) 
		updateDimension(); 
}

void CGraphicsAnnotationObject::setLeaderTo( const CNode* pN )
{
	CPoint3D leaderTo;
	leaderTo.x = pN->x();
	leaderTo.y = pN->y();
	leaderTo.z = pN->z();
	setLeaderTo( leaderTo, 0 );
}

void CGraphicsAnnotationObject::setLeaderTo( const CMember* pM )
{
	CPoint3D leaderTo;
	leaderTo.x = (pM->node(1)->x() + pM->node(2)->x() )/2.;
	leaderTo.y = (pM->node(1)->y() + pM->node(2)->y() )/2.;
	leaderTo.z = (pM->node(1)->z() + pM->node(2)->z() )/2.;
	setLeaderTo( leaderTo, 0 );
}

void CGraphicsAnnotationObject::setLeaderTo( const CPlanar* pP )
{
	CPoint3D leaderTo;
	leaderTo.x = pP->node(1)->x() + pP->node(2)->x()+ pP->node(3)->x();
	leaderTo.y = pP->node(1)->y() + pP->node(2)->y()+ pP->node(3)->y();
	leaderTo.z = pP->node(1)->z() + pP->node(2)->z()+ pP->node(3)->z();
	if( pP->nodes() == 3 ) {
		leaderTo.x /= 3;
		leaderTo.y /= 3;
		leaderTo.z /= 3;
	}
	else {
		leaderTo.x += pP->node(3)->x();
		leaderTo.y += pP->node(3)->y();
		leaderTo.z += pP->node(3)->z();
		leaderTo.x /= 4;
		leaderTo.y /= 4;
		leaderTo.z /= 4;
	}
	setLeaderTo( leaderTo, 0 );
}


void CGraphicsAnnotationObject::updateDimension( void ) {
	m_dimensionLength = m_leaderToPoint3D[0].distance_to( m_leaderToPoint3D[1] );
	ClearText();
	AddLine( Units::report( m_units, m_dimensionLength, 0, 0, SHOW_UNIT ) );
	return;
}


CGraphicsAnnotationManager::CGraphicsAnnotationManager()
	: mAnnotationObjects( ARRAY_SIZE, 1, ARRAY_DELTA, YES_DELETE ),
	  mOffsets( ARRAY_SIZE, 1, ARRAY_DELTA, YES_DELETE )
{
	m_nObjectPointIsOver = 0;
	return;
} 


CGraphicsAnnotationManager::~CGraphicsAnnotationManager() {
	mAnnotationObjects.flush();
	mOffsets.flush();
	return;
} 

const short graphicAnnotationManagerVersion = 1;

ETFileError CGraphicsAnnotationManager::read( ibfstream& in, CWnd* parent )
{
   ETFileError status;
   short  version;
   in >> version;
   if( ( version <= graphicAnnotationManagerVersion ) && ( version > 0 ) )
   {
	   long count;
	   in >> count;
	   ASSERT( count >= 0 );
	   if( count >= 0 ) {
		   for( int i = 0; i < count; i++ ) {
			   CGraphicsAnnotationObject* pG =  
				   new CGraphicsAnnotationObject( CGraphicsAnnotationObject::ANNOTATION_TEXT_ONLY, parent );
			   status = pG->read( in );
			   if( status != FILE_SUCCESS ) {
				   ASSERT(FALSE);
				   return status;
			   }
			   mAnnotationObjects.add( pG );
		   }
	   }
	   else {
		   ASSERT(FALSE);
		   return FILE_READ_ERROR;
	   }

	   in >> count;
	   ASSERT( count >= 0 );
	   if( count >= 0 ) {
		   for( int i = 0; i < count; i++ ) {
			   CGraphicsTextOffset* pG =  new CGraphicsTextOffset();
			   status = pG->read( in );
			   if( status != FILE_SUCCESS ) {
				   ASSERT(FALSE);
				   return status;
			   }
			   mOffsets.add( pG );
		   }
	   }
	   else {
		   ASSERT(FALSE);
		   return FILE_READ_ERROR;
	   }
   }
   else {
	   ASSERT(FALSE);
	   return FILE_READ_ERROR;
   }
   
   if( !in ) {
	   ASSERT(FALSE);
	   return FILE_READ_ERROR;
   }
   else return FILE_SUCCESS;
} // read()



ETFileError CGraphicsAnnotationManager::write( obfstream& out, int options ) const
{
	ETFileError status;
    out << graphicAnnotationManagerVersion;
	long count = mAnnotationObjects.getItemsInContainer();
	out << count;
	for( int i = 1; i <= count; i++ ) {
		CGraphicsAnnotationObject* pG = mAnnotationObjects[i];
		status = pG->write( out, options );
		if( status != FILE_SUCCESS )
			return status;
	}

	count = mOffsets.getItemsInContainer();
	out << count;
	for( int i = 1; i <= count; i++ ) {
		CGraphicsTextOffset* pG = mOffsets[i];
		status = pG->write( out, options );
		if( status != FILE_SUCCESS )
			return status;
	}

	return FILE_SUCCESS;
} // write()


void CGraphicsAnnotationManager::addAnnotationObject( CGraphicsAnnotationObject& ann ) {
	mAnnotationObjects.add( &ann );
	return;
} 


void CGraphicsAnnotationManager::removeAnnotationObject( const CGraphicsAnnotationObject& ann, ETShouldDelete sd ) {
	mAnnotationObjects.detach( (CGraphicsAnnotationObject*)&ann, sd );	// cast away const
	return;
} 

int CGraphicsAnnotationManager::annotationObjects() const {
	return mAnnotationObjects.getItemsInContainer();
} 

CGraphicsAnnotationObject* CGraphicsAnnotationManager::annotationObject( int i ) const {
	if( i > 0 && i <= annotationObjects() ) 
	{
		return mAnnotationObjects[i];
	}
	else
	{
		ASSERT( FALSE ); 
		return NULL;
	}
} // end textObject()

void CGraphicsAnnotationManager::Draw( CDC* pDC, const CRect& r )
{
	// eventually this needs to get smart to keep objects from being draw over the top
	// of other objects
	for( int i = 1; i <= annotationObjects(); i++ ) {
		if( mAnnotationObjects[i] ) {
			// set up the proper drawing font
			if( !equal( mAnnotationObjects[i]->RotationAngle(), 0. ) )
				mAnnotationObjects[i]->SetOutlineFont( m_outlineFontManager.getFont( pDC, 
				mAnnotationObjects[i]->FontName(), mAnnotationObjects[i]->PointSize() ) );
			else
				mAnnotationObjects[i]->SetBitmapFont( m_bitmapFontManager.getFont( pDC, 
				mAnnotationObjects[i]->FontName(), mAnnotationObjects[i]->PointSize() ) );
			mAnnotationObjects[i]->Draw( pDC, offset( mAnnotationObjects[i] ), r );
		}
	}
}

void CGraphicsAnnotationManager::removeAll( void )
{
	mAnnotationObjects.flush();
	return;
}

CGraphicsAnnotationObject* CGraphicsAnnotationManager::pointIsOver( const CPoint& p )
{
	m_nObjectPointIsOver = 0;
	for( int i = 1; i <= annotationObjects(); i++ ) {
		if( mAnnotationObjects[i] ) {
			CPoint pt = p - offset( mAnnotationObjects[i] );
			if( mAnnotationObjects[i]->pointIsOver( pt ) ) {
				m_nObjectPointIsOver = i;
				return mAnnotationObjects[i];
			}
		}
	}
	return NULL;
}

void CGraphicsAnnotationManager::setOffset( const CPoint &offset_increment, unsigned int id )
{
	// first see if this "id" already has an offset and if so, just change it
	for( int i = 1; i <= offsets(); i++ ) {
		if( mOffsets[i] ) {
			if( mOffsets[i]->id() == id ) {
				CPoint newOffset;
				newOffset = mOffsets[i]->offset();
				newOffset += offset_increment;
				mOffsets[i]->setOffset( newOffset );
				return;
			}
		}
	}
	// we've got to add this one
	CGraphicsTextOffset* pOffset = new CGraphicsTextOffset( offset_increment, id );
	mOffsets.add( pOffset );
	return;
}

int	CGraphicsAnnotationManager::offsets() const
{
	return mOffsets.getItemsInContainer();
}

CGraphicsTextOffset* CGraphicsAnnotationManager::offset( int i )
{
	if( i > 0 && i <= offsets() ) 
	{
		return mOffsets[i];
	}
	else
	{
		ASSERT( FALSE ); 
		return NULL;
	}
}

const CPoint& CGraphicsAnnotationManager::offset( CGraphicsAnnotationObject* pAnn ) const
{
	for( int i = 1; i <= offsets(); i++ ) {
		if( mOffsets[i] ) {
			if( mOffsets[i]->id() == pAnn->GraphicsID() )
				return mOffsets[i]->offset();
		}
	}
	// no offset, just return null
	static CPoint p( 0, 0 );
	return p;
}


void CGraphicsAnnotationManager::removeAnnotationsAttachedTo( const CData* pData )
{
	ASSERT( pData );
	if( pData ) {
		for( int i = annotationObjects(); i >= 1; i-- ) {
			if( mAnnotationObjects[i] ) {
				if( mAnnotationObjects[i]->dataObject1() == pData  ||
					mAnnotationObjects[i]->dataObject2() == pData ) 
						mAnnotationObjects.remove( i );
			}
		}
	}
	return;
}

void CGraphicsAnnotationManager::verifyAnnotationsAttachedTo( const CData* pData )
{
	ASSERT( pData );
	if( pData ) {
		const CNode* pN = dynamic_cast<const CNode*>(pData);
		const CMember* pM = dynamic_cast<const CMember*>(pData);
		const CPlanar* pP = dynamic_cast<const CPlanar*>(pData);
		CPoint3D leaderTo;
		if( pN ) {
		}
		else
		for( int i = annotationObjects(); i >= 1; i-- ) {
			if( mAnnotationObjects[i] ) {
				if( mAnnotationObjects[i]->dataObject1() == pData ) {
					switch( mAnnotationObjects[i]->type() ) {
						case CGraphicsAnnotationObject::ANNOTATION_TEXT_ONLY:
							break;
						case CGraphicsAnnotationObject::ANNOTATION_WITH_LINE_LEADER:
						case CGraphicsAnnotationObject::ANNOTATION_WITH_ARROW_LEADER:
							if( pN )
								mAnnotationObjects[i]->setLeaderTo( pN );
							else if( pM )
								mAnnotationObjects[i]->setLeaderTo( pM );
							else if( pP )
								mAnnotationObjects[i]->setLeaderTo( pP );
							break;
						case CGraphicsAnnotationObject::ANNOTATION_DIMENSION:
							ASSERT( pN );
							if( pN ) {
								leaderTo.x = pN->x();
								leaderTo.y = pN->y();
								leaderTo.z = pN->z();
								mAnnotationObjects[i]->setLeaderTo( leaderTo, 0 );
							}
							break;
						default:
							ASSERT( FALSE );
							break;
					}
				}
				else if( mAnnotationObjects[i]->dataObject2() == pData ) {
					switch( mAnnotationObjects[i]->type() ) {
						case CGraphicsAnnotationObject::ANNOTATION_TEXT_ONLY:
						case CGraphicsAnnotationObject::ANNOTATION_WITH_LINE_LEADER:
						case CGraphicsAnnotationObject::ANNOTATION_WITH_ARROW_LEADER:
							ASSERT( FALSE );
							break;
						case CGraphicsAnnotationObject::ANNOTATION_DIMENSION:
							ASSERT( pN );
							if( pN ) {
								leaderTo.x = pN->x();
								leaderTo.y = pN->y();
								leaderTo.z = pN->z();
								mAnnotationObjects[i]->setLeaderTo( leaderTo, 1 );
							}
							break;
						default:
							ASSERT( FALSE );
							break;
					}
				}
			}
		}
	}
	return;
}

void CGraphicsAnnotationManager::verifyAllAnnotationObjects()
{
	for( int i = annotationObjects(); i >= 1; i-- ) {
		if( mAnnotationObjects[i] ) {
			bool needsRemoved = false;
			if( mAnnotationObjects[i]->dataObject1() ) {
				const CNode* pN = dynamic_cast<const CNode*>(mAnnotationObjects[i]->dataObject1());
				const CMember* pM = dynamic_cast<const CMember*>(mAnnotationObjects[i]->dataObject1());
				const CPlanar* pP = dynamic_cast<const CPlanar*>(mAnnotationObjects[i]->dataObject1());
				if( pN ) {
					if( theModel.node( pN->name() ) )
						mAnnotationObjects[i]->setLeaderTo( pN );
					else
						needsRemoved = true;
				}
				else if( pM ) {
					if( theModel.element( MEMBER_ELEMENT, pM->name() ) )
						mAnnotationObjects[i]->setLeaderTo( pM );
					else
						needsRemoved = true;
				}
				else if( pP ) {
					if( theModel.element( PLANAR_ELEMENT, pP->name() ) )
						mAnnotationObjects[i]->setLeaderTo( pP );
					else
						needsRemoved = true;
				}

			}
			if( mAnnotationObjects[i]->dataObject2()  ) {
				const CNode* pN = dynamic_cast<const CNode*>(mAnnotationObjects[i]->dataObject2());
				if( pN ) {
					if( theModel.node( pN->name() ) ) {
						CPoint3D leaderTo;
						leaderTo.x = pN->x();
						leaderTo.y = pN->y();
						leaderTo.z = pN->z();
						mAnnotationObjects[i]->setLeaderTo( leaderTo, 1 );
					}
					else
						needsRemoved = true;
				}
			}
			if( needsRemoved )
				mAnnotationObjects.remove( i );
		}
	}
	return;
}


CGraphicsPixelArrow::CGraphicsPixelArrow(CWnd* pParent, unsigned int id ) :
CGraphicsObjects( pParent, id )
{
	
}

const CRect CGraphicsPixelArrow::boundingRectangle()
{
	ASSERT(false );
	return CRect();
}

void CGraphicsPixelArrow::Draw( CDC* /*pDC*/, const CPoint& /*offset*/ )
{
	//use the one below...
	ASSERT(FALSE);
}

void CGraphicsPixelArrow::Draw( CDC* pDC, const CPoint& offset, const CRect& r )
{
	ASSERT( m_pParentWnd );
	if( !m_pParentWnd || !IsWindow( m_pParentWnd->m_hWnd ) )
		return;

	int width = r.right - r.left;
	int height = r.bottom - r.top;
	CRect wndRect = r;
	// TeK Changed 11/20/2007: from ASSERT( width > 0 && height > 0 );
	if( (width <= 0) || (height <=0) ) {
		m_pParentWnd->GetClientRect( &wndRect );
		width = wndRect.right - wndRect.left;
		height = wndRect.bottom - wndRect.top;
	}
	

	//HGLRC hglrc = wglGetCurrentContext();
	//if( !hglrc )
	//{
	//	hglrc = wglCreateContext( pDC->GetSafeHdc() );
	//	wglMakeCurrent( pDC->GetSafeHdc(), hglrc );
	//}

	glFinish();
	StartGL2DDrawing();
	glOrtho(double(wndRect.left), double(wndRect.right), double(wndRect.bottom), double(wndRect.top), -1., 1. );
	glDisable( GL_DEPTH_TEST );
	glColor3f(1.f, 0.f, 0.f); 
	//glPointSize( float(ini.graphics().fWideWidth * 2) ); 

	//glEnable(GL_COLOR_LOGIC_OP);
	//glLogicOp(GL_XOR);

	glPushMatrix();
		glTranslatef( float(offset.x), float(offset.y), 0. );
		Draw2DLineArrow( (float)m_headL, (float)m_headW, (float)m_shaftL, (float)m_shaftW );
	glPopMatrix();

	glEnd();
	
	glEnable( GL_DEPTH_TEST );
	EndGL2DDrawing();
	SwapBuffers( pDC->GetSafeHdc() );

	//glDrawBuffer( GL_BACK );

		
}

void CGraphicsPixelArrow::SetSize( double headLength, double headWidth, double shaftLength, double shaftWidth )
{
	m_headL = headLength;
	m_headW = headWidth;
	m_shaftL = shaftLength;
	m_shaftW = shaftWidth;
	m_boundingRect = CRect( -int(m_headW/2), int((m_shaftL+m_headL)), int( m_headW/2 ), 0 );
}
void CGraphicsPixelArrow::updateBoundingBox( )
{
	m_boundingRect.left = LONG(m_ControlPoint.x - m_headW/2);
	m_boundingRect.right = LONG(m_ControlPoint.x + m_headW/2);
	m_boundingRect.top = LONG(m_ControlPoint.y + m_shaftL);
	m_boundingRect.bottom = LONG(m_ControlPoint.y);
}

bool CGraphicsPixelArrow::pointIsOver( const CPoint& point )
{
	if( point.x >= m_boundingRect.left && 
		point.x <= m_boundingRect.right && point.y >= m_boundingRect.top && 
		point.y <= m_boundingRect.bottom  )
		return true;
	else
		return false;
}



const short graphicTextOffsetVersion = 1;

ETFileError CGraphicsTextOffset::read( ibfstream& in )
{
   short  version;
   in >> version;
 
   if( ( version <= graphicTextOffsetVersion ) && ( version > 0 ) )
   {
 	in >> m_offset.x;
 	in >> m_offset.y;
	long l;
	in >> l;
	m_id = l;
   }
   
   if( !in ) return FILE_READ_ERROR;
   else return FILE_SUCCESS;
} // read()

ETFileError CGraphicsTextOffset::write( obfstream& out, int /*options*/ ) const
{

   out << graphicTextOffsetVersion;

 	out << m_offset.x;
 	out << m_offset.y;
	long l;
	l = m_id;
	out << l;

	return FILE_SUCCESS;
} // write()


/////////////////////////////////////////////////////////////////////////////
// New Text Class
////////////////////////////////////////////////////////////////////////////
// our global graphics text
CGraphicsText DATA_LINK theGraphicsText;

CGraphicsText::CGraphicsText():
m_bCenter( false ),
m_bRight( false ),
m_bDragging( false ),
m_overallHeight( 0 ),
m_overallWidth( 0 ),
m_boundingRect( CRect(0,0,0,0) ),
m_controlPoint( CPoint(0,0) ),
m_pDC( NULL )
{
}

CGraphicsText::~CGraphicsText()
{
}

void CGraphicsText::DestroyFont( const CString& /*typeFace*/, int /*pointSize*/ )
{
}

void CGraphicsText::ClearFonts()
{
	m_fontMap.clear();
}

void CGraphicsText::DrawString3D( CDC* pDC, const CString& typeFace, int pointSize, COLORREF c, const CString& s)
{
	//glScaled(m_FontPixelHeight*aspect, m_FontPixelHeight*aspect, m_FontPixelHeight*aspect);
	//glRotated(RotationAngle()*180./PI, 0.0, 0.0, 1.0);
	fnt_struct fnt;
	fnt.typeFace = typeFace;
	fnt.pointSize = pointSize;
	if( m_fontMap.find( fnt ) == m_fontMap.end() )
		InitializeNewFont( pDC, typeFace, pointSize );
	if( m_fontMap.find( fnt ) != m_fontMap.end() )
	{
		int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
		int fontPixelHeight = MulDiv( pointSize, pixely, 72);
		float color[4] = {	float(GetRValue( c )/255.), 
			float(GetGValue( c )/255.), 
			float(GetBValue( c )/255.), ALPHA };
		glColor3f( color[0], color[1], color[2] );
		GLsizei len = GLsizei(s.GetLength());
		// Must save/restore the list base.
		glPushMatrix();
		float l2 = (m_bCenter) ? (float)len*0.5f : 0.f;
		glScaled(fontPixelHeight, fontPixelHeight, fontPixelHeight);
		glTranslatef( -l2, 0.f, 0.f );
		glPushAttrib(GL_LIST_BIT);{
			glListBase(m_fontMap[ fnt ].listbase);
			glCallLists(len, GL_UNSIGNED_BYTE, (const GLvoid*)s);
		} glPopAttrib();
		glPopMatrix();
	}
}

void CGraphicsText::DrawText3D( CDC* pDC, const CString& typeFace, int pointSize, COLORREF c )
{
	//glScaled(m_FontPixelHeight*aspect, m_FontPixelHeight*aspect, m_FontPixelHeight*aspect);
	//glRotated(RotationAngle()*180./PI, 0.0, 0.0, 1.0);
	fnt_struct fnt;
	fnt.typeFace = typeFace;
	fnt.pointSize = pointSize;
	if( m_fontMap.find( fnt ) == m_fontMap.end() )
		InitializeNewFont( pDC, typeFace, pointSize );
	if( m_fontMap.find( fnt ) != m_fontMap.end() )
	{
		int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
		int fontPixelHeight = MulDiv( pointSize, pixely, 72);
		float color[4] = {	float(GetRValue( c )/255.), 
			float(GetGValue( c )/255.), 
			float(GetBValue( c )/255.), ALPHA };
		glColor3f( color[0], color[1], color[2] );
		glPushMatrix();
		for( unsigned int i = 0; i < m_text.size(); i++ )
		{
			GLsizei len = GLsizei(m_text[i].GetLength());
			float text_length = 0.;
			if( m_bCenter ){
				for( int j = 0; j < len; j++ )
					text_length += m_fontMap[fnt].gmf[m_text[i][j]].gmfCellIncX;
				text_length *= 0.5;
			}
			// Must save/restore the list base.
			glPushMatrix();
			//do centering here
			glTranslatef( (float)(-text_length*fontPixelHeight), 0.f, 0.f );
			glScaled(fontPixelHeight, fontPixelHeight, fontPixelHeight);
			glPushAttrib(GL_LIST_BIT);{
			glListBase(m_fontMap[ fnt ].listbase);
			glCallLists(len, GL_UNSIGNED_BYTE, (const GLvoid*)m_text[i]);
			} glPopAttrib();
			glPopMatrix();
			glTranslatef( 0., (float)(-fontPixelHeight), 0. );
		}
		glPopMatrix();
	}
}
void CGraphicsText::DrawString2D( CDC* pDC, 
								 const CString& typeFace, 
								 int pointSize, 
								 COLORREF c, 
								 const CString& s, 
								 CPoint pt, 
								 double theta, 
								 const CRect& r )
{

	fnt_struct fnt;
	fnt.typeFace = typeFace;
	fnt.pointSize = pointSize;
	if( m_fontMap.find( fnt ) == m_fontMap.end() )
		InitializeNewFont( pDC, typeFace, pointSize );
	if( m_fontMap.find( fnt ) != m_fontMap.end() )
	{
		int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
		int fontPixelHeight = MulDiv( pointSize, pixely, 72);
		float color[4] = {	float(GetRValue( c )/255.), 
			float(GetGValue( c )/255.), 
			float(GetBValue( c )/255.), ALPHA };
		glColor3f( color[0], color[1], color[2] );
		GLsizei len = GLsizei(s.GetLength());
		// start off by setting up OPENGL so that the window client size is the viewport and
		// scaled so that 1 model space unit is 1 pixel
		int width = r.right - r.left;
		int height = r.bottom - r.top;
		// TeK Changed 11/21/2007: from ASSERT( width > 0 && height > 0 );
		if( (width <= 0) || (height <=0) ) {
			CRect fixR;
			GetClientRect( pDC->GetWindow()->m_hWnd, &fixR );
			width = fixR.right - fixR.left;
			height = fixR.bottom - fixR.top;
		}
		
		StartGL2DDrawing();

		// Make the world and window coordinates coincide so that 1.0 in 
		// model space equals one pixel in window space.   
		// note that client coordinates are not quite model space because the origin in 
		// model space is in the center of the window with y up whereas in client 
		// coordinates, origin is upper left with y down.

		GLdouble size = (GLdouble)((width >= height) ? width : height) / 2.0;
		//size*=(GLdouble)printScale;
		GLdouble aspect;
		if (width <= height) {
			aspect = (GLdouble)height/(GLdouble)width;
			glOrtho(-size, size, -size*aspect, size*aspect, -1000000.0, 1000000.0);
		}
		else {
			aspect = (GLdouble)width/(GLdouble)height;
			glOrtho(-size*aspect, size*aspect, -size, size, -1000000.0, 1000000.0);
		}


		double offsety = 0.;//((double(LineCount())-1.)/2. - (iLine-1.))*fontPixelHeight;
		int cx = /*m_boundingRect.CenterPoint()*/pt.x - int(offsety*sin(theta));
		int cy = /*m_boundingRect.CenterPoint()*/pt.y - int(offsety*cos(theta));
		int ix = cx - int( double(len)/2.*cos(theta));
		int iy = cy + int( double(len)/2.*sin(theta));

		GLfloat x = (GLfloat)(ix - width/2/* + offset.x*/);
		GLfloat y = (GLfloat)(height/2 - iy /*- offset.y*/);  // note we are converting mouse point offsets here so +y is down
		//render to origin then offset to avoid clipping problems
		//http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
		glPushMatrix();
			glRasterPos2i( 0, 0 );
			glTranslated(x*aspect, y*aspect, 0.0);
			float l2 = (m_bCenter) ? (float)len*0.5f : 0.f;
			glScaled(fontPixelHeight*aspect, fontPixelHeight*aspect, fontPixelHeight*aspect);
			glRotated(theta*180./PI, 0.0, 0.0, 1.0);
			glTranslatef( -l2, 0.f, 0.f );
			glPushAttrib(GL_LIST_BIT);{
				glListBase(m_fontMap[ fnt ].listbase);
				glCallLists(len, GL_UNSIGNED_BYTE, (const GLvoid*)s);
			} glPopAttrib();

		glPopMatrix();
		// restore settings
		EndGL2DDrawing();
	}

	return;
}

void CGraphicsText::DrawText2D( CDC* pDC, 
							   const CString& typeFace, 
							   int pointSize, 
							   COLORREF c, 
							   CPoint pt, 
							   double theta, 
							   const CRect& r )
{
	fnt_struct fnt;
	fnt.typeFace = typeFace;
	fnt.pointSize = pointSize;
	if( zero( theta ) )
	{
		//use a bitmap font...
	}
	if( m_fontMap.find( fnt ) == m_fontMap.end() )
		InitializeNewFont( pDC, typeFace, pointSize, zero( theta ) );
	if( m_fontMap.find( fnt ) != m_fontMap.end() )
	{
		int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
		int fontPixelHeight = MulDiv( pointSize, pixely, 72);
		float color[4] = {	float(GetRValue( c )/255.), 
			float(GetGValue( c )/255.), 
			float(GetBValue( c )/255.), ALPHA };
		glColor3f( color[0], color[1], color[2] );
		//GLsizei len = m_overallWidth;//GLsizei(m_text[0].GetLength());
		// start off by setting up OPENGL so that the window client size is the viewport and
		// scaled so that 1 model space unit is 1 pixel
		int width = r.right - r.left;
		int height = r.bottom - r.top;
		// TeK Changed 11/20/2007: from ASSERT( width > 0 && height > 0 );
		if( (width <= 0) || (height <=0) ) {
			CRect fixR;
			GetClientRect( pDC->GetWindow()->m_hWnd, &fixR );
			width = fixR.right - fixR.left;
			height = fixR.bottom - fixR.top;
		}
		StartGL2DDrawing();

		// Make the world and window coordinates coincide so that 1.0 in 
		// model space equals one pixel in window space.   
		// note that client coordinates are not quite model space because the origin in 
		// model space is in the center of the window with y up whereas in client 
		// coordinates, origin is upper left with y down.

		GLdouble size = (GLdouble)((width >= height) ? width : height) / 2.0;
		GLdouble aspect;
		if (width <= height) {
			aspect = (GLdouble)height/(GLdouble)width;
			glOrtho(-size, size, -size*aspect, size*aspect, -1.0, 1.0);
		}
		else {
			aspect = (GLdouble)width/(GLdouble)height;
			glOrtho(-size*aspect, size*aspect, -size, size, -1.0, 1.0);
		}

		//double offsety = 0.;//((double(LineCount())-1.)/2. - (iLine-1.))*fontPixelHeight;
		int cx = pt.x;//m_boundingRect.CenterPoint().x/*pt.x*/ - int(offsety*sin(theta));
		int cy = pt.y;//m_boundingRect.CenterPoint().y/*pt.y*/ - int(offsety*cos(theta));
		int ix = cx;// - int( double(len)/2.*cos(theta));
		int iy = cy;// + int( double(len)/2.*sin(theta));

		GLfloat x = (GLfloat)(ix - width/2/* + offset.x*/);
		GLfloat y = (GLfloat)(height/2 - iy /*- offset.y*/);  // note we are converting mouse point offsets here so +y is down
		//render to origin then offset to avoid clipping problems
		//http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
		glPushMatrix();
		if( zero( theta ) /*&& !bPrinting*/ ){
			//...
			glScaled(aspect, aspect, 1.0);	
			for( unsigned int i = 0; i < m_text.size(); i++ )
			{
				iy += (int)(1.1*fontPixelHeight);
				y = (GLfloat)(height/2 - iy /*- offset.y*/);
				GLsizei len = GLsizei(m_text[i].GetLength());
				glRasterPos2i( 0, 0 );
				float text_length = 0.;
				if( m_bCenter ){
					for( int j = 0; j < len; j++ )
						text_length += m_bmFontMap[fnt].gmf[m_text[i][j]].gmfCellIncX;
					text_length *= 0.5;
				}else if( m_bRight ){
					for( int j = 0; j < len; j++ )
						text_length += m_fontMap[fnt].gmf[m_text[i][j]].gmfCellIncX;
				}
				float y_pos = 0;
				if( m_bVerticalCenter )
					y_pos = (float)(1.1*fontPixelHeight)/2.0f;
				glBitmap( 0, 0, 0, 0, x-(float)(fontPixelHeight)*text_length, y + y_pos, NULL );
				//glBitmap( 0, 0, 0, 0, 0, 0, NULL );
				glPushAttrib(GL_LIST_BIT);{
					glListBase(m_bmFontMap[ fnt ].listbase);
					glCallLists(len, GL_UNSIGNED_BYTE, (const GLvoid*)m_text[i]);
				} glPopAttrib();
			}	
		}else{
			glTranslated(x*aspect, y*aspect, 0.0);
			glRotated(theta*180./PI, 0.0, 0.0, 1.0);
			for( unsigned int i = 0; i < m_text.size(); i++ )
			{
				glTranslatef( 0.f, (float)(-fontPixelHeight*aspect), 0.f );
				glPushMatrix();
				glScaled(fontPixelHeight*aspect, fontPixelHeight*aspect, fontPixelHeight*aspect);
				GLsizei len = GLsizei(m_text[i].GetLength());
				float text_length = 0.;
				if( m_bCenter ){
					for( int j = 0; j < len; j++ )
						text_length += m_fontMap[fnt].gmf[m_text[i][j]].gmfCellIncX;
					text_length *= 0.5;
				}else if( m_bRight ){
					for( int j = 0; j < len; j++ )
						text_length += m_fontMap[fnt].gmf[m_text[i][j]].gmfCellIncX;
				}
				float y_pos = 0;
				if( m_bVerticalCenter )
					y_pos = (float)(1.1*fontPixelHeight)/2.0f;
				glTranslatef( -text_length, y_pos, 0.f );
				glPushAttrib(GL_LIST_BIT);{
					glListBase(m_fontMap[ fnt ].listbase);
					glCallLists(len, GL_UNSIGNED_BYTE, (const GLvoid*)m_text[i]);
				} glPopAttrib();
				glPopMatrix();
			}
		}
		glPopMatrix();
		// restore settings
		EndGL2DDrawing();
	}

	return;
}

void CGraphicsText::DrawInvertedText2D( CDC* pDC, const CString& typeFace, int pointSize, COLORREF /*c*/, CPoint pt, double theta, const CRect& r )
{
	fnt_struct fnt;
	fnt.typeFace = typeFace;
	fnt.pointSize = pointSize;
	if( zero( theta ) )
	{
		//use a bitmap font...
	}
	if( m_fontMap.find( fnt ) == m_fontMap.end() )
		InitializeNewFont( pDC, typeFace, pointSize, zero( theta ) );
	if( m_fontMap.find( fnt ) != m_fontMap.end() )
	{
		int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
		int fontPixelHeight = MulDiv( pointSize, pixely, 72);
		// start off by setting up OPENGL so that the window client size is the viewport and
		// scaled so that 1 model space unit is 1 pixel
		int width = r.right - r.left;
		int height = r.bottom - r.top;
		// TeK Changed 11/20/2007: from ASSERT( width > 0 && height > 0 );
		if( (width <= 0) || (height <=0) ) {
			CRect fixR;
			GetClientRect( pDC->GetWindow()->m_hWnd, &fixR );
			width = fixR.right - fixR.left;
			height = fixR.bottom - fixR.top;
		}
		//copy front buffer to back buffer...
		glReadBuffer( GL_FRONT );
		glDrawBuffer( GL_BACK );
		glCopyPixels(r.left, r.top, r.Width(), r.Height(), GL_COLOR);
		
		StartGL2DDrawing();
		//DisableGLTransparency();
		//glOrtho(double(r.left), double(r.right), double(r.bottom), double(r.top), -1., 1. );
		glDisable( GL_DEPTH_TEST );
		glColor3f(1, 1, 1); 

		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp(GL_XOR);
		// Make the world and window coordinates coincide so that 1.0 in 
		// model space equals one pixel in window space.   
		// note that client coordinates are not quite model space because the origin in 
		// model space is in the center of the window with y up whereas in client 
		// coordinates, origin is upper left with y down.

		GLdouble size = (GLdouble)((width >= height) ? width : height) / 2.0;
		GLdouble aspect;
		if (width <= height) {
			aspect = (GLdouble)height/(GLdouble)width;
			glOrtho(-size, size, -size*aspect, size*aspect, -1.0, 1.0);
		}
		else {
			aspect = (GLdouble)width/(GLdouble)height;
			glOrtho(-size*aspect, size*aspect, -size, size, -1.0, 1.0);
		}

		int cx = pt.x;
		int cy = pt.y;
		int ix = cx;
		int iy = cy;

		GLfloat x = (GLfloat)(ix - width/2);
		GLfloat y = (GLfloat)(height/2 - iy);  // note we are converting mouse point offsets here so +y is down
		//render to origin then offset to avoid clipping problems
		//http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
		glPushMatrix();
		if( zero( theta ) /*&& !bPrinting*/ ){
			//...
			glScaled(aspect, aspect, 1.0);	
			for( unsigned int i = 0; i < m_text.size(); i++ )
			{
				iy += (int)(1.1*fontPixelHeight);
				y = (GLfloat)(height/2 - iy /*- offset.y*/);
				GLsizei len = GLsizei(m_text[i].GetLength());
				glRasterPos2i( 0, 0 );
				float text_length = 0.;
				if( m_bCenter ){
					for( int j = 0; j < len; j++ )
						text_length += m_bmFontMap[fnt].gmf[m_text[i][j]].gmfCellIncX;
					text_length *= 0.5;
				}else if( m_bRight ){
					for( int j = 0; j < len; j++ )
						text_length += m_fontMap[fnt].gmf[m_text[i][j]].gmfCellIncX;
				}
				float y_pos = 0;
				if( m_bVerticalCenter )
					y_pos = (float)(1.1*fontPixelHeight)/2.0f;
				glBitmap( 0, 0, 0, 0, x-(float)(fontPixelHeight)*text_length, y + y_pos, NULL );
				glPushAttrib(GL_LIST_BIT);{
					glListBase(m_bmFontMap[ fnt ].listbase);
					glCallLists(len, GL_UNSIGNED_BYTE, (const GLvoid*)m_text[i]);
				} glPopAttrib();
			}	
		}else{
			glTranslated(x*aspect, y*aspect, 0.0);
			glRotated(theta*180./PI, 0.0, 0.0, 1.0);
			for( unsigned int i = 0; i < m_text.size(); i++ )
			{
				glTranslatef( 0.f, (float)(-fontPixelHeight*aspect), 0.f );
				glPushMatrix();
				glScaled(fontPixelHeight*aspect, fontPixelHeight*aspect, fontPixelHeight*aspect);
				GLsizei len = GLsizei(m_text[i].GetLength());
				float text_length = 0.;
				if( m_bCenter ){
					for( int j = 0; j < len; j++ )
						text_length += m_fontMap[fnt].gmf[m_text[i][j]].gmfCellIncX;
					text_length *= 0.5;
				}else if( m_bRight ){
					for( int j = 0; j < len; j++ )
						text_length += m_fontMap[fnt].gmf[m_text[i][j]].gmfCellIncX;
				}
				float y_pos = 0;
				if( m_bVerticalCenter )
					y_pos = (float)(1.1*fontPixelHeight)/2.0f;
				glTranslatef( -text_length, y_pos, 0.f );
				glPushAttrib(GL_LIST_BIT);{
					glListBase(m_fontMap[ fnt ].listbase);
					glCallLists(len, GL_UNSIGNED_BYTE, (const GLvoid*)m_text[i]);
				} glPopAttrib();
				glPopMatrix();
			}
		}
		glPopMatrix();

		glDisable(GL_COLOR_LOGIC_OP);
		// finish - restore state
		glEnable( GL_DEPTH_TEST );

		// restore settings
		EndGL2DDrawing();
	}

	return;
}

void CGraphicsText::DrawText2D(	const CString& typeFace, int pointSize, COLORREF c, CPoint pt, double theta, const CRect& r )
{
	if( !m_pDC )
		return;
	else
		DrawText2D( m_pDC, typeFace, pointSize, c, pt, theta, r );
}
void CGraphicsText::UpdateBoundingBox( bool newFont, CDC* pDC, int pointSize, CString fontName  )
{
	// located the boundary of the text to be displayed
	if( !pDC ) return;
	if( m_text.size() <= 0 ) return;
	if( m_text[0] == "" ) return;
	int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
	int fontPixelHeight = MulDiv( pointSize, pixely, 72);
	if( newFont ) {
		//m_Lengths = "";
		m_overallHeight = 0;
		m_overallWidth = 0;
		PLOGFONT plCurrentFont = (PLOGFONT)LocalAlloc( GPTR, sizeof(LOGFONT));		// allocate memory for the font
		lstrcpy( plCurrentFont->lfFaceName, fontName );
		plCurrentFont->lfHeight = -fontPixelHeight;
		plCurrentFont->lfWidth = 0;
		plCurrentFont->lfWeight = FW_REGULAR;
		plCurrentFont->lfItalic = false;						// no italics
		plCurrentFont->lfUnderline = 0;					// dont underline
		plCurrentFont->lfStrikeOut = 0;					// dont strikeout
		//int orientation = int( (RotationAngle()*180./PI)*10. );
		plCurrentFont->lfOrientation = 0;//orientation;
		plCurrentFont->lfEscapement = 0;//orientation;
		plCurrentFont->lfClipPrecision = CLIP_LH_ANGLES;
		HFONT font = CreateFontIndirect( plCurrentFont );
		HGDIOBJ oldObject = SelectObject( pDC->GetSafeHdc(), font );
		if( m_bCenter )
			SetTextAlign( pDC->GetSafeHdc(), TA_CENTER | TA_BASELINE );
		else //upper left
			SetTextAlign( pDC->GetSafeHdc(), TA_LEFT | TA_TOP );
		/*if( m_ControlType == TEXT_UPPER_LEFT )
			SetTextAlign( dc, TA_LEFT | TA_TOP );
		else if( m_ControlType == TEXT_LOWER_LEFT )
			SetTextAlign( dc, TA_LEFT | TA_BOTTOM );
		else if( m_ControlType == TEXT_UPPER_RIGHT )
			SetTextAlign( dc, TA_RIGHT | TA_TOP );
		else if( m_ControlType == TEXT_LOWER_RIGHT )
			SetTextAlign( dc, TA_RIGHT | TA_BOTTOM );
		else if( m_ControlType == TEXT_CENTERED_ON )
			SetTextAlign( dc, TA_CENTER | TA_BASELINE );*/
		
		//int length;
		for( unsigned int i = 0; i < m_text.size(); i++ ){
			CString line = m_text[i];
			RECT r;
			r.top = 0;
			r.bottom = 0;
			r.left = 0;
			r.right = 0;
			DrawText( pDC->GetSafeHdc(), line, line.GetLength(), &r, DT_CALCRECT | DT_SINGLELINE );
			int width = r.right - r.left;
			m_overallWidth = max( m_overallWidth, width );
			m_overallHeight += (r.bottom - r.top);
		}
		SelectObject( pDC->GetSafeHdc(), oldObject );
		DeleteObject( font );
		LocalFree( (LOCALHANDLE)plCurrentFont );
	}


	m_boundingRect.left = m_controlPoint.x;
	m_boundingRect.right = m_controlPoint.x + m_overallWidth;
	m_boundingRect.top = m_controlPoint.y;
	m_boundingRect.bottom = m_controlPoint.y + m_overallHeight;


	return;
}

bool CGraphicsText::PointIsOver( const CPoint& point )
{
	if( point.x >= m_boundingRect.left && 
		point.x <= m_boundingRect.right && point.y >= m_boundingRect.top && 
		point.y <= m_boundingRect.bottom  )
		return true;
	else
		return false;
}

void CGraphicsText::InitializeNewFont( CDC* pDC, const CString& typeFace, int pointSize, bool /*bDoBitmap*/ )
{
	fnt_struct fnt;
	InitializeNewOLFont( pDC, typeFace, pointSize, fnt );
	InitializeNewBMFont( pDC, typeFace, pointSize, fnt );
}

void CGraphicsText::InitializeNewOLFont( CDC* pDC, const CString& typeFace, int pointSize, fnt_struct& _fnt )
{
	TRACE( "InitializeNewOLFont( %s, %d )\n", typeFace, pointSize );
	if(pDC && pDC->GetSafeHdc() && strlen( typeFace ) > 0 && pointSize > 0 ) 
	{
		int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
		int fontPixelHeight = MulDiv( pointSize, pixely, 72);

		int listbase = glGenLists(128);
		int oldMode = pDC->SetMapMode( MM_TEXT );

		// Setup the Font characteristics
		LOGFONT logfont;
		
		logfont.lfHeight        = -fontPixelHeight;
		logfont.lfWidth         = 0;
		logfont.lfEscapement    = 0;
		logfont.lfOrientation   = 0;
		logfont.lfWeight        = FW_MEDIUM;
		logfont.lfItalic        = FALSE;
		logfont.lfUnderline     = FALSE;
		logfont.lfStrikeOut     = FALSE;
		logfont.lfCharSet       = ANSI_CHARSET;
		logfont.lfOutPrecision  = OUT_TT_PRECIS;
		logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		logfont.lfQuality       = ANTIALIASED_QUALITY;//CLEARTYPE_QUALITY;

		logfont.lfPitchAndFamily = FF_DONTCARE|DEFAULT_PITCH;
		strcpy(logfont.lfFaceName, typeFace);

        CFont font;
        BOOL success = font.CreateFontIndirect(&logfont);

		pDC->SelectObject(&font);

		if( !success ||
			FALSE == wglUseFontOutlines( pDC->m_hDC,			
						0,								// Starting Character
						128,							// Number Of Display Lists To Build
						listbase,						// Starting Display Lists
						0.0f,							// Deviation From The True Outlines
						0.0f,							// Font Thickness In The Z Direction
						WGL_FONT_POLYGONS,				// Use Polygons, Not Lines
						_fnt.gmf) ){						// Address Of Buffer To Recieve Data
							glDeleteLists(listbase, 128);
							listbase = 0;
		}else{
			_fnt.pointSize = pointSize;
			_fnt.typeFace = typeFace;
			_fnt.listbase = listbase;
			m_fontMap[ _fnt ] = _fnt;
		}
		pDC->SetMapMode( oldMode );
    }

}

void CGraphicsText::InitializeNewBMFont( CDC* pDC, const CString& typeFace, int pointSize, fnt_struct& _fnt )
{
	if(pDC && pDC->GetSafeHdc() && strlen( typeFace ) > 0 && pointSize > 0 ) 
	{
		int pixely = pDC->GetDeviceCaps( LOGPIXELSY );
		int fontPixelHeight = MulDiv( pointSize, pixely, 72);
	
		int listbase = glGenLists(256);

		HFONT font = CreateFont(	-fontPixelHeight,//pointSize,			// height
						0,								// width
						0,								// angle of escapement
						0,								// orientation angle
						FW_MEDIUM,						// weight
						FALSE,							// Italic
						FALSE,							// Underline
						FALSE,							// Strikeout
						ANSI_CHARSET,					// Character Set Identifier
						OUT_TT_PRECIS,					// Output Precision
						CLIP_DEFAULT_PRECIS,			// Clipping Precision
						ANTIALIASED_QUALITY,			// Output Quality
						FF_DONTCARE|DEFAULT_PITCH,		// Family And Pitch
						typeFace);						// Font Name

		HFONT oldfont = (HFONT)pDC->SelectObject( font );		// Selects The Font We Want
		fnt_struct fnt;

		if( FALSE == wglUseFontBitmaps(pDC->m_hDC, 0, 256, listbase) )  {
			glDeleteLists(listbase, 256);
		}else{
			fnt.pointSize = pointSize;
			fnt.typeFace = typeFace;
			//this is from our outline font. we use it to determine the height and width of the text
			for( int i = 0; i < 256; i++ ) fnt.gmf[i] = _fnt.gmf[i];
			fnt.listbase = listbase;
			m_bmFontMap[ fnt ] = fnt;
		}
		pDC->SelectObject(oldfont);							// Selects The Font We Want
		DeleteObject(font);									// Delete The Font
	}
}
