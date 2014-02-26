#pragma once

#include "Loads.h"
#include "Data.h"
#include "IniFilter.h"

#include <GL/gl.h>

#include "legacy/PointerArray-T.h"
#include "math/analytic3D.h"

#include "Data/Graphics/GraphicsObjects.h"


class CGraphicsNodeLoad
{
public:
	CGraphicsNodeLoad( const CNodalLoad* pNL );
	~CGraphicsNodeLoad();

	//void Draw( );
	void Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, double size = 0, double offset = 0 );
	void GetTextPoint( CPoint& p2D, double arrowLength );
	void TranslateAndRotate( double arrowLength, float font_len = 0.f, float scale = 1.f );
	void Translate( double arrowLength );

private:
	void SetupNodeLoad( );

	const CNodalLoad*	m_pNL;
	ETDirection			m_etDir;
	CPoint3D			m_loc;
	CPoint				m_textPoint;
	CVector3D			m_axis;
	float				m_rot;

};


class CGraphicsRigidDiaphragmLoad
{
public:
	CGraphicsRigidDiaphragmLoad( const CRigidDiaphragmLoad* pRDL );
	~CGraphicsRigidDiaphragmLoad();

	void Draw( double arrowSize );
	void Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, double size );
	void GetTextPoint( CPoint& p2D, double arrowLength );

	double m_mv[16];

	void SetupRigidDiaphragmLoad( const CRigidDiaphragmLoad* pNL, float length = 1.f );

	void translate( const CRigidDiaphragmLoad* pRDL, float length );

private:
	const CRigidDiaphragmLoad* m_pRDL;
	ETDirection			m_etDir;
	CPoint3D			m_loc;
	CPoint				m_textPoint;
	CVector3D			m_axis;
	float				m_rot;

};