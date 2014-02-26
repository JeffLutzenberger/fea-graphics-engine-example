#pragma once

//#include "Planar.h"
#include "Data.h"
#include "rsltcase.h"

#include <GL/gl.h>

#include "legacy/PointerArray-T.h"
#include "math/analytic3D.h"

class CPlanar;
class DATA_LINK CDataGraphicsPlate
{
public:
	CDataGraphicsPlate();
	CDataGraphicsPlate( const CPlanar* pP, const CResultCase* pRC, double scale );
	~CDataGraphicsPlate(void);

	//void					Draw( const WindowFilter& filter, double scale );
	void					Draw( bool pictureView, 
		                          bool bDeformed = false, 
								  bool bShrink = false,
								  bool bShoowLocalAxes = false,
								  bool bShowColorGradient = false,
								  double scale = 0.0,
								  const CResultCase* pRC = NULL);

	void					SetupTextureCoordinates(double min, 
									double max, 
									int GraphType, 				
									bool isGlobal, 
									bool isPrincipal, 
									ETPlanarPlane plane, 
									const CResultCase* pRC );

	void Initialize( const CPlanar* pP );

private:
	//bool					GenerateSpringCoordinates( bool bDeformed = false );
	const CPlanar*			m_pP;
	const CResultCase*		m_pRC;

	/*int						m_nSections;
	int						m_nPts_x_3;
	float*					m_springPts;
	CPoint3D				m_start;
	CVector3D				m_RotationAxis;
	double					m_RotationAngle;*/
	double					m_resultMagAtNode[4];
	double					m_scale;
	CPoint3D				m_texCoords[4];
	CPoint3D				m_points[4];
	CPoint3D				m_dispPoints[4];
	bool					m_bTriangle;
	//std::vector<CPoint3D>	m_texCoords;
	//double					m_min; 
	//double					m_max;
	int						m_GraphType; 
	bool					m_isGlobal; 
	bool					m_isPrincipal; 
	ETPlanarPlane			m_plane; 
	bool					m_bDisplacementsChanged;
};
