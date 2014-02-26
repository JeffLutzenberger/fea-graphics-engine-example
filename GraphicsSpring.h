#pragma once

#include "Spring.h"
#include "Data.h"
#include "rsltcase.h"

#include "IniSize.h"
#include "IniColor.h"
#include "IniFilter.h"

#include "Core/Graphics/GLOutlineText.h"

class CGraphicsSpring
{
public:
	CGraphicsSpring( const CSpring* pS );
	~CGraphicsSpring(void);

	void	Draw( CDC* pDC, const CWindowFilter& filter, double scale, CGLText* text, float springLength );

	double	m_mv[16];

private:

	const CSpring*			m_pS;
	const CResultCase*		m_pRC;
};
