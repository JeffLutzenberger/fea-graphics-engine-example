#if !defined( GRAPHICS_CAMERA_H )
#define GRAPHICS_CAMERA_H

#include "Math/Analytic2D.h"
#include "Math/Analytic3D.h"
#include "Math/Matrix44.h"
#include "Math/Quaternion.h"
#include "Math/Box.h"
#include "WinFiltr.h"

#define Z_NEAR						1//0.1  //anything less results in artifacts on my Radeon card
#define Z_FAR						1e5
#define FOV							M_PI*0.25
#define EMPTY_VIEW_VOLUME_WIDTH		50
#define EMPTY_VIEW_VOLUME_HEIGHT	50
#define EMPTY_VIEW_VOLUME_ZPOS		500

class CGraphicsCamera
{
public:
	CGraphicsCamera(void);
	CGraphicsCamera( HWND hWnd, HDC hDC, HGLRC hGLRC );
	~CGraphicsCamera(void);

	// camera setup and sizing
	bool InitCamera( HWND hWnd, HDC hDC, HGLRC hGLRC );
	void SetWindowAspect( int cx, int cy );
	void SetupViewVolume( double width, double height );
	void SetupViewVolume( );
	void SetProjection( );
	void SetupDefaultCamera( );
	void Resize( int cx, int cy );
	void ResetViewport( );
	void SetupWorldSizeAndCenter( double width, double height,
											  double depth, double xCenter, 
											  double yCenter, double zCenter );
	// camera positioning funcitons
	void ViewAll( );
	bool MoveTo( CPoint3D p );
	bool MoveTo( double _x, double _y, double _z );
	bool MoveBy( CPoint3D delta );
	bool MoveBy( double dx, double dy, double dz );
	bool MouseMove( const CPoint& delta );
	void LookAt( double _x, double _y, double _z );
	void Dolly( double inc );
	void Zoom( double inc, CPoint pt );
	void ZoomToRectangle( CRect r );
	void OrthoZoom( const CPoint& pt, double scale );
	void InstantPan( CPoint start, CPoint end );
	const CPoint3D& Rotation(){ return m_rot; };
	void RotateX( double angleInc, CWindowFilter& filter );
	void RotateY( double angleInc, CWindowFilter& filter );
	void RotateZ( double angleInc, CWindowFilter& filter );
	void RotateXY( double xInc, double yInc, CWindowFilter& filter );
	void Rotate( const CPoint3D& euler_angles );
	void Rotate( CVector3D axis, double angleInc );
	void FitTo( const CAABB& box );
	void UpdateCamera();
	void SetFocalPt( CPoint3D pt ){ m_focalPt = pt; };

	//camera query functions
	CVector3D	GetForwardVector();
	CVector3D	GetUpVector();
	CVector3D	GetRightVector();
	double		GetViewVolumeWidth();
	double		GetViewVolumeHeight();
	CPoint3D	GetPosition();
	CPoint3D	GetFocalPoint();
	double		GetFocalLength();
	CPlane3D	GetFocalPlane();
	CRect		GetViewportRect( );
	bool		isPerspectiveOn( ){return m_bPerspective;};
	void		Perspective( bool bDoPerspective = true ){ m_bPerspective = bDoPerspective; SetupViewVolume(); };

	// model to screen space translation
	CPoint GetScreenPtFrom3DPoint( const CPoint3D& modelSpacePt );

	CPoint3D m_pos;
	CPoint3D  m_focalPt;

	//camera settings for file I/O
	void SetCameraSettings( const CPoint3D position, const CPoint3D focalPoint, const CVector3D upVector );
	void GetCameraSettings( CPoint3D& position, CPoint3D& focalPoint, CVector3D& upVector );

protected:

	// scale a screen (pixel) size to a model space size
	void CalculateModelSpaceScaleFactor( void );

private:
	//Window handle and Device and Rendering contexts
	HWND	m_hWnd;
	HDC		m_hDC;
	HGLRC	m_hGLRC;

	//Our camera variables
	double m_Aspect; // x/y - mfc window aspect ratio
	CVector3D m_forward;
	CVector3D m_right;
	CVector3D m_up;
	bool		m_bPerspective;

	//for animation
	bool m_bBusy;
	
	//position and rotation

	CVector3D m_dir;

	CPoint3D m_rot;

	//view volume parameters
	double m_ViewVolumeWidth;
	double m_ViewVolumeHeight;
	double m_Near;
	double m_Far;
	
	//viewport parameters
	int m_cx;
	int m_cy;

	CRect m_wndRect; //hold on to m_hWnd instead and just ::GetClientRect( ... )
	CPlane3D m_gridPlane;


};

#endif // sentry
