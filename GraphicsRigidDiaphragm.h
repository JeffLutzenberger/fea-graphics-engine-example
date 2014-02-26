#pragma once

#include "Node.h"
#include "RigidDiaphragm.h"
#include "Data.h"
#include "rsltcase.h"
#include "Graphics/GraphicsHelpers.h"

#include <GL/gl.h>

#include "legacy/PointerArray-T.h"
#include "math/analytic3D.h"

class CGraphicsRigidDiaphragm
{
public:
	CGraphicsRigidDiaphragm( const CRigidDiaphragm* pRD, const CResultCase* pRC = NULL, double scale = 0. );
	~CGraphicsRigidDiaphragm();

	void					Draw( );

private:

	void					DrawDiaphragm( const CCoordinate& maxC, const CCoordinate& minC );
	void					SetupOrientation( );

	const CRigidDiaphragm*	m_pRD;
	const CResultCase*		m_pRC;

	CPoint3D				m_Start;

	double					m_scale;

	
};