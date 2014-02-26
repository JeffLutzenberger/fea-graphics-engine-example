#if !defined( GRAPHICS_OBJECTS_H )
#define GRAPHICS_OBJECTS_H

#include "data.h"

#include <GL/gl.h>
#include <map>

#include "legacy/PointerArray-T.h"
#include "math/analytic3D.h"
#include "Units\Units.h"

class CNode;
class CMember;
class CPlanar;

class DATA_LINK CGraphicsObjects
{
public:
	CGraphicsObjects(CWnd* pParent = NULL, unsigned int id = 0 );
	virtual ~CGraphicsObjects(void);

	virtual ETFileError	read( ibfstream& in );
	virtual ETFileError	write( obfstream& out, int options = 0 ) const;

	void SetParentWindow( CWnd* pParent ){ m_pParentWnd = pParent; };
	CWnd* ParentWindow(){ return m_pParentWnd; };
	const unsigned int GraphicsID() { return m_graphicsID; };
	void SetGraphicsID( unsigned int i ) { m_graphicsID = i; };
	void SetRotationAngle( double angle ) { m_rotationAngle = angle; };
	double RotationAngle() { return m_rotationAngle; };
	void SetColor( COLORREF c ) { m_Color = c; };
	COLORREF Color( void ) { return m_Color; };

	// pure virtuals - must be defined in derived classes
	virtual const CRect boundingRectangle( void ) = 0;
	virtual void Draw( CDC* pDC, const CPoint& offset ) = 0;

protected:

	CRect m_boundingRect;
	CWnd* m_pParentWnd;
	unsigned int m_graphicsID;
	double m_rotationAngle;
	COLORREF m_Color;
	
	bool m_bSelectable;

private:

};

class DATA_LINK COutlineFont {
public:
    COutlineFont( CDC* pDC, const CString& typeFace, int pointSize );
    virtual ~COutlineFont();
	
    void DrawString(const CString& s); 
	const CString& typeFace( void ) { return m_typeFace; }
	const int basePointSize( void ) { return m_InitialPointSize; }

private:
    GLuint m_listbase;

 	CString m_typeFace;
	int m_InitialPointSize;

};

class DATA_LINK CBitmapFont {
public:
    CBitmapFont( CDC* dc, const CString& fontname, int pointSize );
    virtual ~CBitmapFont();
	
    void  DrawString( const CString& s );
	const CString& typeFace( void ) { return m_typeFace; }
	const int pointSize( void ) { return m_fontSize; }

private:
    GLuint m_listbase;

	CString m_typeFace;
	int m_fontSize;
};


class DATA_LINK COutlineFontManager {
public:
	COutlineFontManager();
	~COutlineFontManager();
	
	void			addFont( COutlineFont& aFont );
	void			removeFont( const COutlineFont& aFont, ETShouldDelete sd = DEFAULT_DELETE );
	COutlineFont*	getFont( CDC* pDC, const CString& typeFace, int pointSize );
	int				fonts() const;
	COutlineFont*	font( int index ) const;
	void			clearAll();
	void			removeAll();
	
private:
	
	TPointerArray<COutlineFont>	mFonts;
};	// end class COutlineFontManager

class DATA_LINK CBitmapFontManager {
public:
	CBitmapFontManager();
	~CBitmapFontManager();
	
	void			addFont( CBitmapFont& aFont );
	void			removeFont( const CBitmapFont& aFont, ETShouldDelete sd = DEFAULT_DELETE );
	CBitmapFont*	getFont( CDC* pDC, const CString& typeFace, int pointSize );
	int				fonts() const;
	CBitmapFont*	font( int index ) const;
	void			clearAll();
	void			removeAll();
	
private:
	
	TPointerArray<CBitmapFont>	mFonts;
};

class DATA_LINK CGraphicsTextObject;
class DATA_LINK CGraphicsTextObject : public CGraphicsObjects
{
public:
	//CGraphicsTextObject(){};
	CGraphicsTextObject(CWnd* pParent = NULL, const char* fontName = "Arial", unsigned int id = 0 );
	CGraphicsTextObject( const CGraphicsTextObject* pObj );
	~CGraphicsTextObject(void);

	virtual ETFileError	read( ibfstream& in );
	virtual ETFileError	write( obfstream& out, int options = 0 ) const;

	const CRect boundingRectangle( void );
	void Draw( CDC* pDC, int printScale = 1 ) ;
	void Draw( CDC* pDC, const CPoint& offset );
	void Draw( CDC* pDC, const CPoint& offset, const RECT WndRect, int printScale = 1 );

	const CString& text( void ) { return m_Text; };

	void SetPointSize( int s ) { m_PointSize = s; /*updateBoundingBox( true );*/ };
	int PointSize( void ) { return m_PointSize; };
	void SetFontName(const CString& s ) { m_FontName = s; /*updateBoundingBox( true );*/ };
	const CString& FontName ( void ) { return m_FontName; };
	void SetControlPoint( const CPoint& p ) { m_ControlPoint = p; /*updateBoundingBox( false );*/ };
	const CPoint& ControlPoint( void ) { return m_ControlPoint; };

	void setSelectable( bool bSelectable ){ m_bSelectable = bSelectable; };

enum m_ETTextControl {
	TEXT_UPPER_LEFT,
	TEXT_CENTERED_ON,
	TEXT_LOWER_RIGHT,
	TEXT_UPPER_RIGHT,
	TEXT_LOWER_LEFT
}; 
	void SetControlType( m_ETTextControl c ) { m_ControlType = c; /*updateBoundingBox( false );*/ };
	m_ETTextControl ControlType( void ) { return m_ControlType; };

	bool pointIsOver( const CPoint& p );

	void AddLine( const char* line );
	int  LineCount( void );
	void ClearText( void );
	const CString& FirstLine( int *length );
	const CString& NextLine( int* length );
	void SetOutlineFont( COutlineFont* pF ) { m_outlineFont = pF; }
	void SetBitmapFont( CBitmapFont* pF ) { m_bitmapFont = pF; }
	int  FontPixelHeight( void ) const { return m_FontPixelHeight; };

	void updateBoundingBox( bool newFont );

protected:

private:
	CPoint m_ControlPoint;
	m_ETTextControl  m_ControlType;
	int m_PointSize;
	CString m_FontName;
	int m_FontPixelHeight;

	CString m_Text;
	CString m_Lengths;
	CString m_Line;

	int m_End;
	int m_LengthEnd;
	int m_overallHeight;
	int m_overallWidth;

	COutlineFont* m_outlineFont;
	CBitmapFont*  m_bitmapFont;

	bool m_bSelectable;
};

class DATA_LINK CGraphicsTextOffset
{
public:
	CGraphicsTextOffset( const CPoint& p, unsigned int id ) { m_offset = p, m_id = id; };
	CGraphicsTextOffset( void ) { m_offset = CPoint( 0, 0 ); m_id = 0; };
	~CGraphicsTextOffset() { };

	virtual ETFileError	read( ibfstream& in );
	virtual ETFileError	write( obfstream& out, int options = 0 ) const;

	const CPoint& offset( void ) const { return m_offset; };
	unsigned int id( void ) const { return m_id; };

	void setOffset( const CPoint& p ) { m_offset = p; };

private:
	CPoint m_offset;
	unsigned int m_id;
};

// manage and draw a collection of simple text objects
class DATA_LINK CGraphicsTextManager
{
public:
	CGraphicsTextManager();
	~CGraphicsTextManager();
	
	void			addTextObject( CGraphicsTextObject& text );
	void			removeTextObject( const CGraphicsTextObject& text, ETShouldDelete sd = DEFAULT_DELETE );
	int				textObjects() const;
	CGraphicsTextObject* textObject( int index ) const;
	void			Draw( CDC* pDC, const RECT r, int printScale = 1 );
	void			clearAll( );
	void			removeAll( void );
	CGraphicsTextObject*	pointIsOver( const CPoint& p );
	// support for offsets of text objects
	void			setOffset( const CPoint& offset, unsigned int id );
	const CPoint&	offset( CGraphicsTextObject* pText ) const;
protected:
	int				offsets() const;
	CGraphicsTextOffset* offset( int index );
	
private:
	
	TPointerArray<CGraphicsTextObject>	mTextObjects;
	TPointerArray<CGraphicsTextOffset>	mOffsets;
	int				m_nObjectPointIsOver;
	COutlineFontManager m_outlineFontManager; 
	CBitmapFontManager m_bitmapFontManager; 

};



class DATA_LINK CGraphicsAnnotationObject : public CGraphicsTextObject
{
public:

enum m_ETAnnotationType {
	ANNOTATION_TEXT_ONLY,
	ANNOTATION_WITH_LINE_LEADER,
	ANNOTATION_WITH_ARROW_LEADER,
	ANNOTATION_DIMENSION
}; 
	CGraphicsAnnotationObject( m_ETAnnotationType type = ANNOTATION_TEXT_ONLY, CWnd* pParent = NULL,
		const char* fontName = "Arial", unsigned int id = 0, const CData* pDataObject1 = NULL, 
		const CData* pDataObject2 = NULL, bool bFixedPosition = true );
	~CGraphicsAnnotationObject(void);

	virtual ETFileError	read( ibfstream& in );
	virtual ETFileError	write( obfstream& out, int options = 0 ) const;

	void Draw( CDC* pDC, const CPoint& offset, const CRect& r );
	m_ETAnnotationType type( void ) const { return m_type; };
	void setType( m_ETAnnotationType t ) { m_type = t; };
	void setUnits( ETUnit q ) { m_units = q; };
	void setLeaderTo( CPoint3D p, int iEnd = 0 );
	void setLeaderTo( const CNode* pN );
	void setLeaderTo( const CMember* pM );
	void setLeaderTo( const CPlanar* pP );
	const CData* dataObject1( void ) { return m_dataObject1; };
	const CData* dataObject2( void ) { return m_dataObject2; };
	bool hasFixedPosition( void ) const { return m_bFixedPosition; };
	void setFixedPosition( bool bIsFixed = true ) { m_bFixedPosition = bIsFixed; };
	void set3DLocation( CPoint3D pt ) { m_3DPosition = pt; };

protected:
	void updateDimension();

private:
	m_ETAnnotationType m_type;
	CPoint3D m_leaderToPoint3D[2];
	ETUnit m_units;
	double m_dimensionLength;
	const CData* m_dataObject1;    // this gets set if we are dependent on a data object in the model
	const CData* m_dataObject2;    // this gets set if we are dependent on a data object in the model
	bool m_bFixedPosition;
	CPoint3D m_3DPosition;  // when we do not have a fixed position this is the real-world location (control)

};

// manage and draw a collection of annotation objects
class DATA_LINK CGraphicsAnnotationManager
{
public:
	CGraphicsAnnotationManager();
	~CGraphicsAnnotationManager();

	virtual ETFileError	read( ibfstream& in, CWnd* parent );
	virtual ETFileError	write( obfstream& out, int options = 0 ) const;

	void			addAnnotationObject( CGraphicsAnnotationObject& text );
	void			removeAnnotationObject( const CGraphicsAnnotationObject& text, ETShouldDelete sd = DEFAULT_DELETE );
	int				annotationObjects() const;
	CGraphicsAnnotationObject* annotationObject( int index ) const;
	void			Draw( CDC* pDC, const CRect& r );
	void			removeAll( void );
	CGraphicsAnnotationObject*	pointIsOver( const CPoint& p );
	// support for offsets of text objects
	void			setOffset( const CPoint& offset, unsigned int id );
	const CPoint&	offset( CGraphicsAnnotationObject* pAnn ) const;
	// support for dependencies to data objects
	void			removeAnnotationsAttachedTo( const CData* pData );
	void			verifyAnnotationsAttachedTo( const CData* pData );
	void			verifyAllAnnotationObjects();

protected:
	int				offsets() const;
	CGraphicsTextOffset* offset( int index );
	
private:
	
	TPointerArray<CGraphicsAnnotationObject>	mAnnotationObjects;
	TPointerArray<CGraphicsTextOffset>	mOffsets;
	int				m_nObjectPointIsOver;
	COutlineFontManager m_outlineFontManager; 
	CBitmapFontManager m_bitmapFontManager; 

};


class DATA_LINK CGraphicsPixelArrow : public CGraphicsObjects
{
public:
	CGraphicsPixelArrow(CWnd* pParent = NULL, unsigned int id = 0 );
	~CGraphicsPixelArrow(){};

	// virtuals
	virtual const CRect boundingRectangle( void );
	virtual void Draw( CDC* pDC, const CPoint& offset );
	void Draw( CDC* pDC, const CPoint& offset, const CRect& r );

	void SetControlPoint( const CPoint& p ) { m_ControlPoint = p; updateBoundingBox( ); };
	CPoint ControlPoint( void ) { return m_ControlPoint; };

	void SetSize( double headLength, double headWidth, double shaftLength, double shaftWidth );

	bool pointIsOver( const CPoint& point );

private:

	void		updateBoundingBox( );
	double		m_headL;
	double		m_headW;
	double		m_shaftL;
	double		m_shaftW;
	CPoint3D	m_pos;
	CPoint m_ControlPoint;
};

struct fnt_struct{
	CString typeFace;
	int pointSize;
	GLYPHMETRICSFLOAT gmf[256];
	int listbase;
};

struct ltPtSize
{
	bool operator()( fnt_struct f1, fnt_struct f2 ) const 
	{
		return f1.pointSize < f2.pointSize;
	}
};


typedef std::map<fnt_struct, fnt_struct /*int*/, ltPtSize> GraphicsFontMap;
typedef std::map<fnt_struct, GLYPHMETRICSFLOAT*, ltPtSize> GraphicsGlyphMap; 

class DATA_LINK CGraphicsText {
public:
    CGraphicsText( );
    virtual ~CGraphicsText();
	
	void InitializeNewFont( CDC* pDC, const CString& typeFace, int pointSize, bool bDoBitmap = false );
	void InitializeNewOLFont( CDC* pDC, const CString& typeFace, int pointSize, fnt_struct& _fnt );
	void InitializeNewBMFont( CDC* pDC, const CString& typeFace, int pointSize, fnt_struct& _fnt );
	void DestroyFont( const CString& typeFace, int pointSize );
	void ClearFonts();
	
    void DrawString3D( CDC* pDC, const CString& typeFace, int pointSize, COLORREF color, const CString& s);
	void DrawString2D( CDC* pDC, const CString& typeFace, int pointSize, COLORREF c, const CString& s, CPoint pt, double theta, const CRect& r );

	void DrawText3D( CDC* pDC, const CString& typeFace, int pointSize, COLORREF c );
	void DrawText2D( const CString& typeFace, int pointSize, COLORREF c, CPoint pt, double theta, const CRect& r );
	void DrawText2D( CDC* pDC, const CString& typeFace, int pointSize, COLORREF c, CPoint pt, double theta, const CRect& r );
	void DrawInvertedText2D( CDC* pDC, const CString& typeFace, int pointSize, COLORREF c, CPoint pt, double theta, const CRect& r );
	
	void AddLine( const char* line ){ m_text.push_back( CString(line) ); };
	void ClearText( ){ m_text.clear(); };
	bool HaveText(){ return m_text.size() > 0; };

	void Center( bool bDoCenter = true ){ m_bCenter = bDoCenter; m_bRight = false; };
	void Right( bool bDoRight = true ){ m_bRight = bDoRight; m_bCenter = false; };
	void VerticalCenter( bool bDoVerticalCenter = true ){ m_bVerticalCenter = bDoVerticalCenter; };

	void UpdateBoundingBox( bool newFont, CDC* pDC, int pointSize, CString fontName  );
	void SetControlPoint( CPoint pt ){m_controlPoint = pt;};
	const CPoint& ControlPoint( ){return m_controlPoint;};
	bool PointIsOver( const CPoint& point );
	void SetDragging( bool bDrag = true ){ m_bDragging = bDrag; }; 
	bool Dragging( ){return m_bDragging;};

private:
	
	GraphicsFontMap	m_fontMap;
	GraphicsGlyphMap m_glyphMap;
	GraphicsFontMap m_bmFontMap;

	CDC*	m_pDC;

	std::vector<CString>	m_text;
	bool	m_bCenter;
	bool	m_bRight;
	bool	m_bVerticalCenter;
	bool	m_bDragging;
	int		m_overallHeight;
	int		m_overallWidth;
	CRect	m_boundingRect;
	CPoint	m_controlPoint;

};

extern CGraphicsText DATA_LINK theGraphicsText;

#endif  // sentry
