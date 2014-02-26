#pragma once

#include "Include/UseData.h"
#include "legacy/PointerArray-T.h"
#include "Math/Analytic3d.h"
#include "Layers.h"

#include "GraphicsCamera.h"
#include "GraphicsGrid.h"
#include "Graphics/GraphicsObjects.h"
#include "Graphics/GraphicsDisplayList.h"
#include "GraphicsEngine.h"
#include "GLFrustum.h"
#include "OrthoCamera.h"

#include "Core/Graphics/GLOutlineText.h"
#include "Core/Graphics/GLTextureText.h"
//#include "Core/Graphics/GLText3D.h"

#include "GraphicsMemberLoad.h"
#include "Graphics/GraphicsHelpers.h"
#include "Graphics/GraphicsHelperClasses.h"
#include "Graphics/HitRecord.h"
#include "Graphics/GraphicsExtrusion.h"
#include "Graphics/GraphicsMember.h"
#include "Graphics/GraphicsCable.h"
#include "GraphicsContourBar.h"
#include "GraphicsEngine.h"
#include "VertexArrayManager.h"
#include "dialog/WindowTip.h"
#include "Math/Analytic3d.h"

#include "Servcase.h"

#include "ViewData.h"

#include "dialog/WindowTip.h"

#include "WinFiltr.h"
#include "IniColor.h"  // OBJECT_COLOR_SIZE
#include "VAMain_Resource.h" // for annotation class

#define ZOOM_TOLERANCE 0.01

#define RGB_BLACK RGB( 0, 0, 0 )

#include <map>

#include "ViewData.h"
#include "WinFiltr.h"

// CGraphicView view

class CRigidDiaphragm;
class CNode;
class CMember;
class CPlanar;
class CSpring;
class CCable;
class CElement;
class CFoundation;
class CNodalLoad;
class CMemberLoad;
class CMovingLoad;
class CPlanarLoad;
class CAreaLoad;
class CRigidDiaphragmLoad;
class CServiceCase;
class CData;
class CReport;

////////////////////////////////////////////////////////////////////////////////
///
/// CGraphicsModel Class - Responsible for drawing our model.
///
/// The purpose of this class is to draw the model independent of the window/camera type.
/// In the future we could have one window that is an orthographic view for modeling with 
/// standard CAD functionality and another window that has perspective, lighting, etc. 
/// that can be used as a high-quality view. In theory we could then use this class to draw 
/// in both window types and simply change the window behavior without altering much of the 
/// drawing code.
/// 
/// Drawing "modes": 
///		Model View
///		Result View
///		Design View
///
/// Requirements:
///		-CWindowFilter (high dependency)
///		-access to the global Model (high dependency)
///		-access to ini settings (high dependency)
///		-pointer to the current device context for text rendering and access 
///			to window information (loose dependency)
///		-OpenGL text class (CGLOutlineText) (loose dependency)
///		-glNames for selection (high dependency)
///
////////////////////////////////////////////////////////////////////////////////
class CGraphicsModel
{
public:
	
	CGraphicsModel();

	void ClearFonts();

	void EnableText( );

	void DisableText( );

	void SetWindowFilter( const CWindowFilter& windowFilter );

	void DrawModel( CDC* pDC );

	void DrawModelForPrinter( CDC* pDC, int fontPrintScale );

	void DrawModelForSelection( CDC* pDC, bool bBoxSelecting );

	void DrawMembersForSnapping( CDC* pDC );

	// Limit the number of things we draw when we rotate the model
	void StartReducedDrawing();
	
	// Set the state back to draw the entire model
	void ResumeCompleteDrawing();

	//camera info
	void SetCamera( const COrthoCamera& camera );

	//sets max and min values for the current result case
	bool SetupResults( const CWindowFilter& windowFilter );

	CGraphicsContourBar* HitContourBar( CPoint pt );

	//mouse over
	const CNode*			OverNode( const CHitRecordCollection& rHitCollection );
	const CMember*			OverMember( const CHitRecordCollection& rHitCollection );
	const CPlanar*			OverPlate( const CHitRecordCollection& rHitCollection );
	const CPlanar*			OverAutoPlate( const CHitRecordCollection& rHitCollection );
	const CSpring*			OverSpring( const CHitRecordCollection& rHitCollection );
	const CCable*			OverCable( const CHitRecordCollection& rHitCollection );
	const CArea*			OverArea( const CHitRecordCollection& rHitCollection );
	const CChain*			OverAreaHole( const CHitRecordCollection& rHitCollection );
	const CChain*			OverAreaCorridor( const CHitRecordCollection& rHitCollection );
	const CFoundation*		OverFoundation( const CHitRecordCollection& rHitCollection );
	const CCoordinate*		OverAreaVertex( const CHitRecordCollection& rHitCollection );
	const CRigidDiaphragm*	OverRigidDiaphragm( const CHitRecordCollection& rHitCollection );
	const CNodalLoad*		OverNodalLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC );
	const CMemberLoad*		OverMemberLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC );
	const CPlanarLoad*		OverPlateLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC );
	const CMovingLoad*		OverMovingLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC );
	const CAreaLoad*		OverAreaLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC );
	const CRigidDiaphragmLoad*	OverRigidDiaphragmLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC  );
	const CData*	OverGraphicObject( CPoint p, ETGraphicObject go, const CHitRecordCollection& rHitCollection, const CServiceCase* pSC  );	
	bool			OverNodeOrGridPoint( CPoint screenPt, CPoint3D& _pt3D, const CWindowFilter& filter );

private:

	//our device context
	CDC*	m_pDC;

	//our gradient texture for results
	GLuint	m_nGradientTexture;

	//a copy of the window filter
	CWindowFilter	m_Filter;

	//our camera info
	COrthoCamera	m_Camera;

	//JL Note 10/22/2009 - using textured fonts now...
	//our text object(s)
	CGLText*			m_text;
	CGLTextureText	m_textureText;
	CGLOutlineText	m_outlineText;

	CGraphicsText m_contourText;

	//our view frustum to test if objects are visible before drawing them
	CGLFrustum m_GLFrustum;

	//max and min values for the current result case so that we can normalize the 
	//texture coordinates for the member, cable and plate color gradients
	double m_memberMax;
	double m_memberMin;
	double m_cableMax;
	double m_cableMin;
	double m_plateMax;
	double m_plateMin;

	//our legends for results
	CGraphicsContourBar			m_memberLegend;
	CGraphicsContourBar			m_cableLegend;
	CGraphicsContourBar			m_plateLegend;
	CGraphicsDesignContourBar	m_memberDesignLegend;

	// to speed up rotating and animation
	bool m_bReducedDrawing;

	//uses m_Filter and theProject to calculatoe the current displacement factor for result viwes
	double GetDisplacementFactor();

	//use m_Filter to get a valid service case, return NULL on failure
	CServiceCase* serviceCase( void );

	COLORREF GetElementColor( const CElement* pE/*, ETGraphicsDrawStyle style*/ );

	//model objects
	
	void DrawNodes( );

	void DrawReactions( );

	void DrawSupports( );

	void DrawFoundations( );

	void DrawRigidDiaphragms( );

	void DrawAreas( );

	void DrawArea( );

	void DrawAreaVertices( );

	void DrawPlates( );

	void DrawAutoMeshedPlates( );

	void DrawSprings( );

	void DrawMembers( );

	void DrawMember( const CMember* pM, bool bDisplaced, bool bLabel, double scale );

	void DrawCables( );

	void DrawCable( const CCable* pC, bool bDisplaced, bool bLabel, double scale  );

	void DrawGroundPlane( );

	void DrawLegends( unsigned int printFontScale = 1 );

	//model object loads
	
	void DrawNodalLoads( );

	void DrawMemberLoads( );

	void DrawMovingLoads( );

	void DrawPlateLoads( );

	void DrawAreaLoads( );

	void DrawRigidDiaphragmLoads( );

	//results settings for plates, members and cables

	bool SetupMemberResults( );

	bool SetupCableResults( );

	bool SetupPlateResults( );

	void SetUnityValuesAndColors( const CMember* pM, float color[4] );

	//void SetUnityValuesAndColors( const CPlanar* pP, float color[4] );

	bool bDoTextureMember();

	bool bDoTextureCable();
};

