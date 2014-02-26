#pragma once

#include "Node.h"
#include "Data.h"
#include "rsltcase.h"

#include "IniSize.h"
#include "IniColor.h"
#include "IniFilter.h"

#include "Core/Graphics/GLOutlineText.h"

class CGraphicsNode
{
public:
	CGraphicsNode(const  CNode* pN );
	~CGraphicsNode(void);

	void	Draw( CDC* pDC, const CWindowFilter& filter, double scale, CGLText* text );

private:

	const CNode*			m_pN;
};
