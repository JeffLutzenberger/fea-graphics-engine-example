#pragma once

#include "Core/Graphics/GLOutlineText.h"
#include "Core/Graphics/GLTextureText.h"
#include "Core/Graphics/GraphicsHelpers.h"
#include "Core/Graphics/GraphicsHelperClasses.h"
#include "OrthoCamera.h"

enum ETCubeOrientation{
	//faces...
	FRONT_FACE,
	BACK_FACE,
	TOP_FACE,
	BOTTOM_FACE,
	LEFT_FACE,
	RIGHT_FACE,
	//corners...
	TOP_FRONT_LEFT,
	TOP_FRONT_RIGHT,
	BOTTOM_FRONT_RIGHT,
	BOTTOM_FRONT_LEFT,
	TOP_BACK_LEFT,
	TOP_BACK_RIGHT,
	BOTTOM_BACK_LEFT,
	BOTTOM_BACK_RIGHT,
	//edges...
	TOP_FRONT,
	BOTTOM_FRONT,
	LEFT_FRONT,
	RIGHT_FRONT,
	TOP_BACK,
	BOTTOM_BACK,
	LEFT_BACK,
	RIGHT_BACK,
	TOP_LEFT,
	TOP_RIGHT,
	BOTTOM_LEFT,
	BOTTOM_RIGHT,
	LAST_ORIENTATION,
	NO_ORIENTATION_HIT = LAST_ORIENTATION
};

class CGraphicsRotationCube
{
public:
	CGraphicsRotationCube(void);
	~CGraphicsRotationCube(void);

	//draw without labels
	void Draw( );

	//draw with labels
	void Draw( const CPoint3D& rotations, CDC* pDC );

	bool PointIsInViewport( const CPoint& pt, const CRect& r );

	bool OnMouseClick( CPoint3D& rotations, const CPoint& p, const CRect& r, int selSize );

	bool OnMouseMove( const CPoint3D& rotations, const CPoint& p, const CRect& r, int selSize );

	bool Animating();

	bool GetAnimatingRotation( CPoint3D& rotations );

private:

	void DrawCorners( );
	void DrawEdges( );
	void DrawFaces( );
	void DrawLabels( CDC* pDC );

	void SetupCamera( const CPoint3D& rotations );

	double CubeOrientationAngle( int cube_orientation, int xyz );

	COrthoCamera	m_camera;
	int m_viewportSize;
	float m_cubeSize;

	ETCubeOrientation m_hoverItem;

	CGLText*		m_text;
	CGLOutlineText	m_outlineText;
	CGLTextureText	m_textureText;

	CPoint3D m_startRotation;
	CPoint3D m_endRotation;

	bool m_bAnimating;

	int m_nFrame;

	long m_startTime; //in ms
};
