#pragma once

#include "Area.h"

#include "Graphics/GraphicsHelpers.h"
#include "Graphics/GraphicsHelperClasses.h"

class CGraphicsArea
{
public:
	CGraphicsArea( const CArea* pA );
	~CGraphicsArea(void);

	void Draw( ETGraphicsDrawStyle style, double spanArrowLength );

	void DrawCorridor( int index, ETGraphicsDrawStyle style );

	void DrawHole( int index, ETGraphicsDrawStyle style );

private:
	const CArea*	m_pA;
};
