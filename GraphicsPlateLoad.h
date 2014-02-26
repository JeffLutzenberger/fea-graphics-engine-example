#pragma once

#include "PlnrLoad.h"
#include "Data.h"
#include "WinFiltr.h"

#include <GL/gl.h>

#include "legacy/PointerArray-T.h"
#include "math/analytic3D.h"
#include "Math/Matrix44.h"
#include "Math/Quaternion.h"
#include "Graphics/GraphicsHelpers.h"

#include "Graphics/GraphicsObjects.h"
#include "Graphics/GLOutlineText.h"

class CGraphicsPlateLoad
{
public:
	CGraphicsPlateLoad( const CPlanarLoad* pNL );
	~CGraphicsPlateLoad();

	void Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, double loadLength );
	
private:
	void SetupPlateLoad( const CPlanarLoad* pPL );
	void DrawPressure( double loadLength );

	const CPlanarLoad *m_pPL;

	CVector3D m_textOffsetVector;
};