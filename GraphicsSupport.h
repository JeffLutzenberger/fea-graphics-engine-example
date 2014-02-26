#pragma once

#include "Node.h"

class CGraphicsSupport
{
public:
	CGraphicsSupport( const CNode* pN );
	~CGraphicsSupport(void);

	void	Draw( ETGraphicDetail detail, double length );

private:
	const CNode*			m_pN;
};
