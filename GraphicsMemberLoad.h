#pragma once

#include "Loads.h"
#include "Data.h"
#include "MemLoad.h"
#include "WinFiltr.h"

#include <GL/gl.h>

#include "legacy/PointerArray-T.h"
#include "math/analytic3D.h"
#include "Math/Matrix44.h"
#include "Math/Quaternion.h"


#include "Graphics/GraphicsObjects.h"
#include "Graphics/GLOutlineText.h"

class CGraphicsMemberLoad
{
public:
	CGraphicsMemberLoad();
	CGraphicsMemberLoad( const CMemberLoad* pML );
	~CGraphicsMemberLoad();
		
	void Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, double size = 0, double offsset = 0 );
	//void GetMagnitude( int i );
	//void GetModelViewMatrix( double mv[16], double loadLength, ETGraphicDetail detail );

	//double		m_yoffset;

private:

	void SetupMemberLoad( const CMemberLoad* pML );

	void DrawForceLoad( double length, double width, double offset );
	void DrawTemperatureLoad( double length, double width, ETGraphicDetail detail );
	void GetArrowRotation( double& angle, CVector3D& axis );
	
	const CMemberLoad *m_pML;

	float m_memberOrientation[16];

	ETDirection m_etDir;
	ETMemberLoad m_etType;


	CPoint3D	m_loc;
	CPoint3D	m_textStart;
	CPoint3D	m_textEnd;
	CVector3D	m_dir;
};