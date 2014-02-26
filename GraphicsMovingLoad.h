#pragma once

#include "Loads.h"
#include "Data.h"
#include "MovLoad.h"
#include "IniFilter.h"

#include <GL/gl.h>

#include "legacy/PointerArray-T.h"
#include "math/analytic3D.h"
#include "Math/Matrix44.h"
#include "Math/Quaternion.h"

#include "Graphics/GLOutlineText.h"

#include "Data/Graphics/GraphicsObjects.h"

enum ETGraphicMoveLoadText{
	WHEEL_LOAD,
	LANE_LOAD,
	MOVE_DIR
};

class CGraphicsMovingLoad
{
public:
	CGraphicsMovingLoad( const CMovingLoad* pML );
	~CGraphicsMovingLoad(void);
	
	//void Draw( const CWindowFilter& filter );
	void Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, float loadLength );
	void GetTextPoint( CPoint& p2D, double arrowLength, ETGraphicMoveLoadText etTextType, int axel_num = 0 );
	int NumAxles( );

	double m_mv[16];

private:
	void SetupMovingLoad( const CMovingLoad* pML );
	void DrawMovingLoad( const CWindowFilter& filter, float loadLength );

	//void DrawForceLoad( double length, double width, ETGraphicDetail detail );
	//void DrawTemperatureLoad( double length, double width, ETGraphicDetail detail );
	void GetArrowRotation( double& angle, CVector3D& axis );
	void GetPolyRotation( double& angle, CVector3D& axis );

	const CMovingLoad *m_pML;

	ETDirection m_etDir;
	ETMemberLoad m_etType;


	CPoint3D	m_loc;
	CPoint3D	m_start;
	CPoint3D	m_end;
	CVector3D	m_axis;

	double		m_rot;
	CVector3D	m_dir;

	double		m_arrowRot;
	CVector3D	m_arrowAxis;
};
