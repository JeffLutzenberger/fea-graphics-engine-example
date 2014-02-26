#pragma once

#include "Data.h"
#include "Foundation.h"
#include "rsltcase.h"

#include "IniSize.h"
#include "IniColor.h"
#include "IniFilter.h"

class CGraphicsFoundation
{
public:
	CGraphicsFoundation( const CFoundation* pF );
	~CGraphicsFoundation(void);

	void	Draw( const CWindowFilter& filter );

private:

	const CFoundation*		m_pF;
	const CResultCase*		m_pRC;
};
