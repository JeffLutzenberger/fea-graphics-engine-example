#pragma once

#include "planar.h"
#include "IniFilter.h"

#include "Core/Graphics/GLOutlineText.h"
#include "Core/Graphics/GraphicsHelpers.h"

class CGraphicsPlate
{
public:
	CGraphicsPlate(const CPlanar* pP);
	~CGraphicsPlate(void);

	void	Draw( CDC* pDC, const CWindowFilter& filter, double scale, ETGraphicsDrawStyle style, double plateAxisLength );
	void	Draw( CDC* pDC, const CWindowFilter& filter, double scale, ETGraphicsDrawStyle style, CGLText* text, double plateAxisLength  );
	
	void	SetupTextureCoordinates(double min, double max, CWindowFilter& filter );

private:

	bool m_bHaveTexCoords;
	CPoint3D m_texCoords[4];

	//location of our text...
	CPoint3D m_center;

	const CPlanar* m_pP;

	void GetPlatePropertyBuffers(char* buf1, char* buf2, const CPlanar* pP, const CWindowFilter& filter );

	void SetUnityValuesAndColors( float color[4] );

	void addToBuffers( char* buf1, char* buf2, int size, int* fillNumber, const char* str );
};
