#pragma once

#include "Node.h"
#include "rsltcase.h"

#include "IniSize.h"
#include "IniColor.h"
#include "IniFilter.h"

#include "Core/Graphics/GLOutlineText.h"

class CGraphicsReaction
{
public:
	CGraphicsReaction( const CNode* pN, const CResultCase* pRC );
	~CGraphicsReaction(void);

	void	Draw( CDC* pDC, const CWindowFilter& filter, double scale, CGLText* text, double length );

private:
	const CNode*			m_pN;
	const CResultCase*		m_pRC;
};
