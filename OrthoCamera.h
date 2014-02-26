#pragma once

#include "Math/Analytic3D.h"
#include "Math/Matrix44.h"
#include "Math/Box.h"

#include "Data/Enumdata.h"

#include "Graphics/HitRecord.h"

#define SELECT_BUFFER_SIZE 8192

class COrthoCamera
{
public:
	COrthoCamera(void);
	~COrthoCamera(void);

	//set the acpect ratio based on the width and height of the CWnd area
	void SetWindowAspect( int cx, int cy );

	//set the current projection and orientation
	void SetView( );

	//get and set camera settings for named views
	void GetPositionRotationAndScale( CPoint3D& position, CPoint3D& rotations, CPoint3D& viewVolume, double& zoomScale );
	void SetPositionRotationAndScale( const CPoint3D& position, const CPoint3D& rotations, const CPoint3D& viewVolume, double zoomScale );

	void Resize( int cx, int cy );

	void ResetViewport( );

	void SetPrintScale( int scale );

	void SetupDefaultCamera( const CAABB& box );

	void Zoom( double inc );

	void Zoom( double inc, const CPoint& pt );

	void ZoomArea( const CRect& r );

	//sets the zoom scale to fit the contents of this box to the view
	//accounts for rotation
	void ZoomAll( const CAABB& box );

	//initialize the view volume to fit the current model
	void FitTo( const CAABB& box );

	void MoveBy( const CPoint& delta );

	void RotateX( double inc );
	void RotateY( double inc );
	void RotateZ( double inc );
	//mouse rotation 
	void RotateXY( double deltaX, double deltaY );
	void Rotate( const CPoint3D& eulers );
	const CPoint3D& GetRotations();
	void SetRotations( const CPoint3D& rotations );

	double ZoomFactor();

	//returns world object size for n number of pixels where n=pixelSize
	double ScalePixelSize( int pixelSize );

	bool PointInView( const CPoint3D& pt );

	void SetVerticalAxis( ETCoordinate direction );

	void StartMousePicking( const CPoint& p, const CSize& selSize, int nBufSize, UINT selectBuf[] );

	int EndMousePicking( CHitRecordCollection& rRecordCollection, UINT selectBuf[] );

	bool WriteSettings( obfstream& out );

	bool ReadSettings( ibfstream& in );

private:

	void SetOrientation( );

	void GetViewVolumePointFromScreen( const CPoint& pt, double& x, double& y );

	double m_viewVolumeWidth;
	double m_viewVolumeHeight;
	double m_viewVolumeDepth;

	double m_viewVolumeX;
	double m_viewVolumeY;
	double m_viewVolumeZ;

	double m_xloc;
	double m_yloc;
	double m_zloc;

	CPoint3D m_rotations;

	int m_viewportX;
	int m_viewportY;

	double zoomFactor;

	double m_aspectRatio;

	CMatrix44 m_rotationMatrix;

	ETCoordinate	m_upAxis;
	
	bool m_bMousePick;

	CPoint m_screenPt;
	CSize m_selSize;

	int m_printScale;
	
};
