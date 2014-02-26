#pragma once

#include "AreaLoad.h"
#include "Data.h"

#include <GL/gl.h>

#include "legacy/PointerArray-T.h"
#include "math/analytic3D.h"
#include "Math/Matrix44.h"
#include "Math/Quaternion.h"
#include "Core/Graphics/GraphicsHelpers.h"
#include "WinFiltr.h"

#include "Graphics/GraphicsObjects.h"
#include "Graphics/GLOutlineText.h"

class CGraphicsAreaLoad
{
public:
	CGraphicsAreaLoad(void);

	CGraphicsAreaLoad( const CAreaLoad* pAL );
	~CGraphicsAreaLoad(void);

	void Draw( CDC* pDC, const CWindowFilter& filter, CGLText* text, bool setFilling, double arrowLength );

private:

	void DrawAreaLoad( CDC* pDC, const CWindowFilter& filter, CGLText* text, bool setFilling, double arrowLength );
	void DrawFEALoads( CDC* pDC, const CWindowFilter& filter, CGLText* text, double arrowLength );
	
	const CAreaLoad *m_pAL;
	
};
