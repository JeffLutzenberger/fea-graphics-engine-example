#pragma once

#include "Data.h"
#include "rsltcase.h"
//#include "Grahics/GraphicsExtrusion.h"
#include "Data/Graphics/GraphicsObjects.h"
#include "Units/Units.h"

#include <GL/gl.h>

#include "legacy/PointerArray-T.h"
#include "math/analytic3D.h"

enum ETContourHitObject{
		CONTOUR_BAR_MIN_SLIDER,
		CONTOUR_BAR_MAX_SLIDER,
		CONTOUR_BAR,
		CONTOUR_BAR_NOTHING
	};

//class CGraphicsContourBar;
class CGraphicsContourBar : public CGraphicsObjects
{
public:
	CGraphicsContourBar( CWnd* pParent = NULL, unsigned int id = 0 );
	CGraphicsContourBar( CWnd* pParent, unsigned int id, CPoint offset );
	CGraphicsContourBar( CGraphicsContourBar* pObj );
	virtual ~CGraphicsContourBar(void);

	void Draw( CDC* pDC, const CPoint& offset );
	void Draw( CDC* pDC, float start_tex_coord, float end_tex_coord, bool dragging );
	void DrawSliders( );
	virtual const CRect boundingRectangle( void );

	void UpdateLabels( const CString& label, double min, double max );
	void DrawContourBarText( CGraphicsText& text, CDC* pDC, CRect r, ETUnit unit, float printScale );
	const CGraphicsContourBar* pointIsOver( const CPoint& pt );
	void dragSlider( const CPoint& pt );
	void resetSliders();
	void incrementOffset( const CPoint& delta );
	const CPoint&	offset(){return m_offset;};
	void SetStartOffset( const CPoint startOffset ){ m_startingOffset = startOffset; };
	void SetWindowRect( const CRect& wndRect ){ m_wndRect = wndRect; };

	void OnLButtonDown();
	void OnLButtonUp();
	void OnMouseMove( const CPoint& delta );

	void	StartGLGradientTexture();
	void	recreateContourTexture();

	//our gradient texture for results
	GLuint	m_nContourTexture;

protected:

	//override this for design contour bars since they will not have sliders
	virtual ETContourHitObject hitWhat( const CPoint& pt );
	CRect	m_wndRect;
	CString	m_label;
	double m_min;
	double m_max;
	CPoint m_offset;
	CPoint m_startingOffset;
	CRect m_barRect;
	unsigned int m_id;
	double m_minSlider; //range 0..1 on slider scale
	double m_maxSlider; //range 0..1 on slider scale
	GLubyte* m_contourTexture;
	bool m_bBlendedGradient;
	ETContourHitObject m_hitObject;
};

class CGraphicsDesignContourBar : public CGraphicsContourBar
{
public:
	CGraphicsDesignContourBar( CWnd* pParent = NULL, unsigned int id = 0 );
	CGraphicsDesignContourBar( CGraphicsDesignContourBar* pObj );
	~CGraphicsDesignContourBar(void);

	virtual void DrawContourBarText( CGraphicsText& text, CDC* pDC, CRect r, ETUnit unit, float printScale );
	void Draw( CDC* pDC, const CPoint& offset );
	void Draw( CDC* pDC, float start_tex_coord, float end_tex_coord );

};