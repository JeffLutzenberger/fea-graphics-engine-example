// GraphicsDrawing.cpp : implementation file
// Drawing support

#include "stdafx.h"
#pragma hdrstop

#include "GraphicsModel.h"
#include "GraphChildFrame.h"

#include "main/GraphicsCamera.h"

#include "Graphics/GraphicsMember.h"
#include "Graphics/GraphicsCable.h"
#include "Graphics/GraphicsHelpers.h"
#include "Graphics/GraphicsHelperClasses.h"
#include "Graphics/SpineGenerator.h"
#include "ShapeDB/CrossSection.h"
#include "Graphics/GraphicsExtrusion.h"
#include "GraphicsNodeLoad.h"
#include "GraphicsMemberLoad.h"
#include "GraphicsMovingLoad.h"
#include "GraphicsPlateLoad.h"
#include "GraphicsAreaLoad.h"

//new graphics objects
#include "GraphicsNode.h"
#include "GraphicsReaction.h"
#include "GraphicsRigidDiaphragm.h"
#include "GraphicsSpring.h"
#include "GraphicsSupport.h"
#include "GraphicsFoundation.h"
#include "GraphicsArea.h"
#include "GraphicsPlate.h"

#include "GraphicsContourBar.h"

#include "Design/GroupManager.h"	// theGroupManager
#include "Design/Mesh.h"			// theMeshManager
#include "Model.h"
#include "Project.h"
#include "Node.h"
#include "Member.h"
#include "Spring.h"
#include "Planar.h"
#include "MemLoad.h"
#include "PlnrLoad.h"
#include "ServCase.h"
#include "movload.h"
#include "IniSize.h"
#include "IniColor.h"
#include "IniFilter.h"
#include "IniFont.h"
#include "IniDesk.h"
#include "user/change.h"
#include "graphics/GridManager.h"
#include "VAStatus.h"
#include "AreaManager.h"

#include "OldDataUnits.h" // PercentageQuantity -- May 2006, this should go away soon...

#include "user/control.h"

#include "engineering\FEM Engine\Engine.h" // the Engine!
#include "VisualDesign/VDInterface.h" // VDUnitySuccessLimit()

#include <gl/gl.h>
#include <gl/glu.h>

#define ZOOM_TOLERANCE 0.01

#define TEMPORARY_SCALE_FACTOR 10.

#define TEMPORARY_NUM_MEMBER_DIV	4

#ifdef _DEBUG
	#undef THIS_FILE
	static char THIS_FILE[]=__FILE__;
	#define new DEBUG_NEW
#endif

CGraphicsModel::CGraphicsModel():
m_text( NULL ),
m_textureText(),
m_outlineText(),
m_Filter( ),
m_pDC(NULL),
m_nGradientTexture( 0 ),
m_memberMax( 0. ),
m_memberMin( 0. ),
m_cableMax( 0. ),
m_cableMin( 0. ),
m_plateMin( 0. ),
m_plateMax( 0. ),
m_memberLegend( CGraphicsContourBar( NULL, 1 ) ),
m_cableLegend( CGraphicsContourBar( NULL, 2, CPoint( 0, -45 ) ) ),
m_plateLegend( CGraphicsContourBar( NULL, 3, CPoint( 0, -45) ) ),
m_memberDesignLegend( CGraphicsDesignContourBar( NULL, 4 ) ),
m_bReducedDrawing( false )
{
	//// TeK Add 12/21/2006: Don't use 'this' in initializer list
	//m_memberLegend.SetParentWindow( this );
	//m_cableLegend.SetParentWindow( this );
	//m_plateLegend.SetParentWindow( this );
	//m_memberDesignLegend.SetParentWindow( this );
	if( ini.graphics().useTextureFonts )
		m_text = &m_textureText;
	else
		m_text = &m_outlineText;

	theGraphicsEngine.CreateExtrusions( m_Filter );
}

void CGraphicsModel::ClearFonts()
{
	if( m_text )
		m_text->ClearFonts();
	m_contourText.ClearFonts();
}

void CGraphicsModel::EnableText( )
{
	if( m_text )
		m_text->Enable();
}

void CGraphicsModel::DisableText( )
{
	if( m_text )
		m_text->Disable();
}

void CGraphicsModel::SetWindowFilter( const CWindowFilter& windowFilter )
{
	m_Filter = windowFilter;
}

void CGraphicsModel::SetCamera( const COrthoCamera& camera )
{
	m_Camera = camera;
}

void CGraphicsModel::StartReducedDrawing()
{
	m_bReducedDrawing = true;
	DisableText();
}

void CGraphicsModel::ResumeCompleteDrawing()
{
	m_bReducedDrawing = false;
	EnableText();
}
double CGraphicsModel::GetDisplacementFactor() 
{
	if( !m_Filter.displacements )
		return 0;
	return m_Filter.internalDisplacementFactor;
	
	/*
	//JL Commented out 8/17/2009 - Time history and mode shape scale factors 
	//do not work properly with this method

	// TeK Change 2/21/2008: The PROJECT holds the displacement factor based on ALL 
	// results available. This insures that displacements scale consistently regardless
	// of which result case is visible in the window.  The engineer can scale them up or down
	// using the filter scale factor
	// How should displacements be scaled graphically?
	const CResultCase* pRC = m_Filter.resultCase(); // TeK Add 8/20/2008??? Need to treat mode shapes differently???
	if( pRC ) {
		if( m_Filter.displacements ) {
			if( zero(m_Filter.displacementFactor) ) {
			//if( zero(m_Filter.displacementFactor) || (MODE_SHAPE_RESULTS == pRC->type()) ) { // TeK Add 8/20/2008???
				return 1.0; // show "true" displacements
			}
			else {
				double pdf = theProject.displacementFactor( pRC );
				return pdf*m_Filter.displacementFactor;
			}
		}
	}
	return 0.0; // No displacements shown!*/
}

void CGraphicsModel::DrawModel( CDC* pDC )
{
	ASSERT( pDC );
	if( !pDC )
		return;

	m_pDC = pDC;

	//m_GLFrustum.ExtractFrustum( );
	//m_text3D.AddLine( "TEST" );
	//m_text3D.DrawText3D( pDC, "Arial", 12, ini.color().grid, CPoint(), 0 );
	DrawNodes( );
	DrawReactions( );
	DrawSupports( );
	DrawFoundations( );
	DrawSprings( );
	DrawRigidDiaphragms( );
	DrawAreas( );
	DrawAreaVertices( );
	DrawPlates( );
	DrawAutoMeshedPlates( );
	DrawMembers( );
	DrawCables( );
	DrawNodalLoads( );
	DrawMemberLoads( );
	DrawPlateLoads( );
	DrawMovingLoads( );
	DrawAreaLoads( );
	DrawRigidDiaphragmLoads( );

	DrawLegends( );
}

void CGraphicsModel::DrawModelForPrinter( CDC* pDC, int fontPrintScale )
{
	ASSERT( pDC );
	if( !pDC )
		return;

	m_pDC = pDC;

	//reset legend textures...
	m_memberLegend.recreateContourTexture();
	m_cableLegend.recreateContourTexture();
	m_plateLegend.recreateContourTexture();

	//make sure our text knows how big to draw itself
	if( m_text )
		m_text->SetPrintScale( fontPrintScale );

	DrawNodes( );
	DrawReactions( );
	DrawSupports( );
	DrawFoundations( );
	DrawSprings( );
	DrawRigidDiaphragms( );
	DrawAreas( );
	DrawAreaVertices( );
	DrawPlates( );
	DrawAutoMeshedPlates( );
	DrawMembers( );
	DrawCables( );
	DrawNodalLoads( );
	DrawMemberLoads( );
	DrawPlateLoads( );
	DrawMovingLoads( );
	DrawAreaLoads( );
	DrawRigidDiaphragmLoads( );

	DrawLegends( fontPrintScale );

	if( m_text )
		m_text->SetPrintScale( 1 );

}
void CGraphicsModel::DrawModelForSelection( CDC* pDC, bool bBoxSelecting )
{
	ASSERT( pDC );
	if( !pDC )
		return;

	//ignore wireframe drawing to speed up selection
	//bool overlay = m_Filter.wireFrameOverlay;
	//m_Filter.wireFrameOverlay = false;

	//selection buffer does not work when text is drawn (temporary fix)
	if( m_text )
		m_text->Disable();

	DrawNodes( );
	DrawMembers( );
	DrawReactions( );
	DrawSupports( );
	DrawFoundations( );
	DrawSprings( );
	DrawRigidDiaphragms( );
	DrawAreas( );
	DrawAreaVertices( );
	DrawPlates( );
	DrawCables( );
	DrawNodalLoads( );
	DrawMemberLoads( );
	DrawPlateLoads( );
	DrawMovingLoads( );
	DrawAreaLoads( );
	DrawRigidDiaphragmLoads( );

	if( !bBoxSelecting )
		DrawLegends( );

	if( m_text )
		m_text->Enable();

	//m_Filter.wireFrameOverlay = overlay;

}

void CGraphicsModel::DrawMembersForSnapping( CDC* pDC )
{
	ASSERT( pDC );
	if( !pDC )
		return;

	//ignore wireframe drawing to speed up selection
	bool overlay = m_Filter.wireFrameOverlay;
	m_Filter.wireFrameOverlay = false;

	//selection buffer does not work when text is drawn (temporary fix)
	if( m_text )
		m_text->Disable();

	DrawMembers( );

	if( m_text )
		m_text->Enable();

	m_Filter.wireFrameOverlay = overlay;
}

void CGraphicsModel::DrawNodes( )
{
	if( !m_Filter.CanShowNodes() || !theModel.nodes() )
		return;

	//make the nodes points draw ontop of grid lines
	//glDepthRange( 0, 0.999 );

	StartWireframeDrawing( );
	double scale = GetDisplacementFactor();
	for( CNodeIterator ni( m_Filter.node.filter ); !ni; ++ni )
	{
		const CNode* pN = (const CNode*)ni();
		if( !pN || !m_Filter.isVisible( *pN ) )
			continue;
		//if( !m_GLFrustum.NodeInFrustum( pN ) )
		//	continue;
		int index = ni.index();
		ASSERT( index < MAX_GRAPHICS_INDEX );
		unsigned int glID = SetGraphicsName( GRAPHIC_NODE, index );
		glLoadName( glID );
		CGraphicsNode gn( pN );
		gn.Draw( m_pDC, m_Filter, scale, m_text ); 
		//gn.Draw( m_pDC, m_Filter, scale, m_text ); 
	}

	//make the area points draw ontop of grid lines
	//glDepthRange( 0, 1.0 );

	return;
}

void CGraphicsModel::DrawReactions()
{
	if( !m_Filter.CanShowReactions() || ! theModel.nodes() || m_bReducedDrawing )
		return;

	const CResultCase* pRC = m_Filter.resultCase();
	if( !pRC )
		return;

	StartSolidDrawing( gLoadPolygonOffset );
	DisableGLLighting();
	double distFactor = GetDisplacementFactor();
	double length = m_Camera.ScalePixelSize( ini.size().reactionLength );
	for( CNodeIterator ni( m_Filter.node.filter ); !ni; ++ni )
	{
		if( !m_Filter.isVisible( *(CNode*)ni() ) )
			continue;
		int index = ni.index();
		ASSERT( index < MAX_GRAPHICS_INDEX );
		unsigned int glID = SetGraphicsName( GRAPHIC_NODE, index );
		glLoadName( glID );
		CGraphicsReaction gr( (CNode*)ni(), pRC );
		gr.Draw( m_pDC, m_Filter, distFactor, m_text, length );
	}
	return;
}

void CGraphicsModel::DrawSupports()
{
	if( !m_Filter.CanShowSupports() || theModel.nodes() <= 0 || m_bReducedDrawing )
		return;

	double length = m_Camera.ScalePixelSize( ini.size().supportLength );
	StartWireframeDrawing( );
	for( CNodeIterator ni( m_Filter.node.filter ); !ni; ++ni )
	{
		if( !m_Filter.isVisible( *(CNode*)ni() ) )
			continue;
		ASSERT( ni.index() < pow( 2., 24 ));
		int index = ni.index();
		unsigned int glID = SetGraphicsName( GRAPHIC_NODE, index );
		glLoadName( glID );
		CGraphicsSupport gs( (CNode*)ni() );
		gs.Draw( m_Filter.drawDetail, length );
	}
	
	if( m_Filter.drawDetail == DETAIL_HI )
		EnableGLLighting();

	StartSolidDrawing( gMemberPolygonOffset );
	for( CNodeIterator ni( m_Filter.node.filter ); !ni; ++ni )
	{
		if( !m_Filter.isVisible( *(CNode*)ni() ) )
			continue;
		ASSERT( ni.index() < pow( 2., 24 ));
		int index = ni.index();
		unsigned int glID = SetGraphicsName( GRAPHIC_NODE, index );
		glLoadName( glID );
		CGraphicsSupport gs( (CNode*)ni() );
		gs.Draw( m_Filter.drawDetail, length );
	}

	if( m_Filter.drawDetail == DETAIL_HI )
		DisableGLLighting();

	return;
}

void CGraphicsModel::DrawFoundations( )
{
	if( !m_Filter.CanShowFoundations( ) )
		return;

	//wireframe - do wireframe first so it will draw on top of the solid
	StartWireframeDrawing();
	for( int i = 1; i <= theModel.foundations(); i++ ) {
		const CFoundation* pFoundation = theModel.foundation( i );
		if( !m_Filter.isVisible( *pFoundation ) )
			continue;
		unsigned int glID = SetGraphicsName( GRAPHIC_FOUNDATION, i );
		glLoadName( glID );
		CGraphicsFoundation gf( pFoundation );
		gf.Draw( m_Filter );
	}

	//set drawing state here
	if( m_Filter.drawDetail == DETAIL_HI )
		EnableGLLighting();

	StartSolidDrawing( gMemberPolygonOffset );
	for( int i = 1; i <= theModel.foundations(); i++ ) {
		const CFoundation* pFoundation = theModel.foundation( i );
		if( !m_Filter.isVisible( *pFoundation ) )
			continue;
		unsigned int glID = SetGraphicsName( GRAPHIC_FOUNDATION, i );
		glLoadName( glID );
		CGraphicsFoundation gf( pFoundation );
		gf.Draw( m_Filter );
	}

	if( m_Filter.drawDetail == DETAIL_HI )
		DisableGLLighting();

	return;

}
void CGraphicsModel::DrawRigidDiaphragms( ) 
{
	if( !m_Filter.CanShowRigidDiaphragms() || theModel.rigidDiaphragms() <= 0 )
		return;

	//wireframe drawing
	StartWireframeDrawing();
	for( int i = 1; i <= theModel.rigidDiaphragms(); i++ ) {
		CRigidDiaphragm* pD = (CRigidDiaphragm*)theModel.rigidDiaphragm(i);
		ASSERT( pD );
		if( !pD || !m_Filter.isVisible( *pD ) )
			continue;
		unsigned int glID = SetGraphicsName( GRAPHIC_RIGID_DIAPHRAGM, i );
		glLoadName( glID );
		COLORREF c = InverseColor( ini.color().rigidDiaphragms );
		if( pD->isSelected() )
			c = InverseColor( c );
		ApplyAmbientGLMaterialiv( c );
		CGraphicsRigidDiaphragm rd( pD );
		rd.Draw( );
	}

	//set drawing state
	if( m_Filter.drawDetail == DETAIL_HI )
		EnableGLLighting();

	StartSolidDrawing( gPlatePolygonOffset );
	for( int i = 1; i <= theModel.rigidDiaphragms(); i++ ) {
		CRigidDiaphragm* pD = (CRigidDiaphragm*)theModel.rigidDiaphragm(i);
		ASSERT( pD );
		if( !pD || !m_Filter.isVisible( *pD ) )
			continue;
		unsigned int glID = SetGraphicsName( GRAPHIC_RIGID_DIAPHRAGM, i );
		glLoadName( glID );
		COLORREF c = ini.color().rigidDiaphragms;
		if( pD->isSelected() )
			c = InverseColor( c );
		ApplyAmbientGLMaterialiv( c );
		CGraphicsRigidDiaphragm rd( pD );
		rd.Draw( );
	}

	if( m_Filter.drawDetail == DETAIL_HI )
		DisableGLLighting();

}

void CGraphicsModel::DrawAreas()
{
	if( !m_Filter.CanShowAreas() || theModel.areas() <= 0 )
		return;

	//draw with no lighting and no material 

	//offset areas alot...
	//glEnable( GL_POLYGON_OFFSET_FILL );
	//glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	//glPolygonOffset( 0, 5 );

	StartWireframeDrawing();
	double length = m_Camera.ScalePixelSize( ini.size().planarLoadLength );
	for( int i = 1; i <= theModel.areas(); i++ ) {
		const CArea* pArea = theModel.area( i );
		if( !m_Filter.isVisible( *pArea ) )
			continue;
		unsigned int glID = SetGraphicsName( GRAPHIC_AREA, i );
		glLoadName( glID );
		CGraphicsArea ga( pArea );
		ga.Draw( GRAPHIC_LINE, length );
	}

	//glDisable( GL_POLYGON_OFFSET_FILL );

	glEnable( GL_POLYGON_OFFSET_FILL );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	//glPolygonOffset( 0, 5 );

	StartSolidDrawing( gAreaPolygonOffset );
	for( int i = 1; i <= theModel.areas(); i++ ) {
		const CArea* pArea = theModel.area( i );
		if( !m_Filter.isVisible( *pArea ) )
			continue;
		unsigned int glID = SetGraphicsName( GRAPHIC_AREA, i );
		glLoadName( glID );
		CGraphicsArea ga( pArea );
		ga.Draw( GRAPHIC_SOLID, length );
	}

	glDisable( GL_POLYGON_OFFSET_FILL );

	//now draw the holes
	StartWireframeDrawing();
	for( int i = 1; i <= theModel.areas(); i++ ) {
		const CArea* pArea = theModel.area( i );
		if( !m_Filter.isVisible( *pArea ) )
			continue;
		for( int j = 1; j <= pArea->holes(); j++ )
		{
			unsigned int glID = SetAreaItemName( GRAPHIC_AREA_HOLE, i, j );
			glLoadName( glID );
			CGraphicsArea ga( pArea );
			ga.DrawHole( j, GRAPHIC_LINE );
		}
	}
	//glDisable( GL_POLYGON_OFFSET_FILL );

	glEnable( GL_POLYGON_OFFSET_FILL );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	//glPolygonOffset( 0, 5 );

	StartSolidDrawing( gPlatePolygonOffset );
	for( int i = 1; i <= theModel.areas(); i++ ) {
		const CArea* pArea = theModel.area( i );
		if( !m_Filter.isVisible( *pArea ) )
			continue;
		for( int j = 1; j <= pArea->holes(); j++ )
		{
			unsigned int glID = SetAreaItemName( GRAPHIC_AREA_HOLE, i, j );
			glLoadName( glID );
			CGraphicsArea ga( pArea );
			ga.DrawHole( j, GRAPHIC_SOLID );
		}
	}

	glDisable( GL_POLYGON_OFFSET_FILL );

	//offset areas alot...
	//glEnable( GL_POLYGON_OFFSET_FILL );
	//glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	//glPolygonOffset( 0, 5 );

	//now draw the corridors
	StartWireframeDrawing();
	for( int i = 1; i <= theModel.areas(); i++ ) {
		const CArea* pArea = theModel.area( i );
		if( !m_Filter.isVisible( *pArea ) )
			continue;
		for( int j = 1; j <= pArea->corridors(); j++ )
		{
			unsigned int glID = SetAreaItemName( GRAPHIC_AREA_CORRIDOR, i, j );
			glLoadName( glID );
			CGraphicsArea ga( pArea );
			ga.DrawCorridor( j, GRAPHIC_LINE );
		}
	}
	//glDisable( GL_POLYGON_OFFSET_FILL );

	glEnable( GL_POLYGON_OFFSET_FILL );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	//glPolygonOffset( 0, 5 );

	StartSolidDrawing( gPlatePolygonOffset );
	for( int i = 1; i <= theModel.areas(); i++ ) {
		const CArea* pArea = theModel.area( i );
		if( !m_Filter.isVisible( *pArea ) )
			continue;
		for( int j = 1; j <= pArea->corridors(); j++ )
		{
			unsigned int glID = SetAreaItemName( GRAPHIC_AREA_CORRIDOR, i, j );
			glLoadName( glID );
			CGraphicsArea ga( pArea );
			ga.DrawCorridor( j, GRAPHIC_SOLID );
		}
	}

	glDisable( GL_POLYGON_OFFSET_FILL );
}

void CGraphicsModel::DrawAreaVertices()
{
	if( !m_Filter.CanShowAreas() || theModel.areas() <= 0 || m_bReducedDrawing)
		return;

	//make the area points draw ontop of grid lines
	//glDepthRange( 0, 0.9 );

	StartWireframeDrawing();
	for( int i = 1; i <= theModel.areas( ); i++ ) {
		const CArea* pArea = theModel.area( i );
		if( !pArea || !m_Filter.isVisible( *pArea ) )
			continue;
		//area boundary
		for( int j = 1; j <= pArea->vertices(); j++ ){
			unsigned int glID = SetAreaItemName( GRAPHIC_AREA_VERTEX, i, j );
			glLoadName( glID );
			const CCoordinate* pC = pArea->vertex(j);
			ASSERT( pC );
			if( !pC ) continue;

			if( pC->isSelected() )
			{
				glPointSize( 2*float( ini.size().areaVertex ) );
				COLORREF c = InverseColor( ini.color().members );
				SetGLColor( c );
			}
			else
			{
				glPointSize( float( ini.size().areaVertex ) );
				glColor3f( 0.f, 0.f, 1.f );
			}
			glBegin( GL_POINTS );
				glVertex3f( float(pC->x()), float(pC->y()), float(pC->z()) );
			glEnd();
		}
	}
	//glDepthRange( 0, 1 );
	return;
}

void CGraphicsModel::DrawPlates()
{
	if( !m_Filter.CanShowPlates() || theModel.elements( PLANAR_ELEMENT ) <= 0 )
		return;

	//when drawing plates we have few options:
	//1. Model view 
	//		- solid
	//		- wireframe overlay
	//2. Result view -
	//		- undisplaced (with or without gradient)
	//		- displaced only (with or without gradient)
	//		- displaced and undisplaced (with or without gradient on displaced shape)
	//3. Design view
	//		- undisplaced (with or without design colors)
	//		- undisplaced ungrouped (translucent)

	glLineWidth( float( ini.size().planar ) );

	double distFactor = GetDisplacementFactor();

	//int nSelected = theModel.elements( PLANAR_ELEMENT, true );
	//bool showLocalAxes = m_Filter.plate.localAxes;
	double localAxisLength = m_Camera.ScalePixelSize( ini.size().planarAxisLength );

	//wireframe drawing - note: do this first so the wireframe will draw on top of the solid model
	StartWireframeDrawing();
	for( CElementIterator ne( PLANAR_ELEMENT, m_Filter.plate.filter ); !ne; ++ne )
	{
		const CPlanar* pP = (const CPlanar*)ne();
		ASSERT( pP );
		if( !pP || !m_Filter.isVisible( *pP ) )
			continue;

		COLORREF c = InverseColor( ini.color().background );
		if( pP->isSelected() )
			c = InverseColor( ini.color().planarFill );
		ApplyAmbientGLMaterialiv( c );

		ASSERT( ne.index() < MAX_GRAPHICS_INDEX );
		int index = ne.index();
		unsigned int glID = SetGraphicsName( GRAPHIC_PLATE, index );
		glLoadName( glID );
		CGraphicsPlate gp( pP );
		gp.Draw( m_pDC, m_Filter, distFactor, GRAPHIC_LINE, m_text, localAxisLength );
	}

	//undisplaced shape in a result view
	if( m_Filter.windowType == POST_WINDOW && m_Filter.undeformed && m_Filter.displacements ){
		StartDashedLineDrawing();
		for( CElementIterator ne( PLANAR_ELEMENT, m_Filter.plate.filter ); !ne; ++ne )
		{
			const CPlanar* pP = (const CPlanar*)ne();
			ASSERT( pP );
			if( !pP || !m_Filter.isVisible( *pP ) )
				continue;

			COLORREF c = ini.color().planarFill;
			c = InverseColor( ini.color().background );
			ApplyAmbientGLMaterialiv( c );

			ASSERT( ne.index() < MAX_GRAPHICS_INDEX );
			int index = ne.index();
			unsigned int glID = SetGraphicsName( GRAPHIC_PLATE, index );
			glLoadName( glID );
			CGraphicsPlate gp( pP );
			gp.Draw( m_pDC, m_Filter, 0, GRAPHIC_DASHED_LINE, localAxisLength );
		}
		EndDashedLineDrawing();
	}

	//draw solid part
	//setup texture mapping for results
	bool bDoTexture = false;
	if( m_Filter.windowType == POST_WINDOW && m_Filter.plate.GetResultType() != PLATE_NO_RESULT && 
		m_Filter.resultCase()->type() != MODE_SHAPE_RESULTS )
	{
		bDoTexture = true;
		m_plateLegend.StartGLGradientTexture();
		DisableGLLighting();
	}

	//set drawing state - turn off lighting if we are showing textured results
	if( m_Filter.drawDetail == DETAIL_HI && !bDoTexture )
		EnableGLLighting();
	
	//if( m_Filter.drawDetail == DETAIL_HI )
	StartSolidDrawing( gPlatePolygonOffset );

	for( CElementIterator ne( PLANAR_ELEMENT, m_Filter.plate.filter ); !ne; ++ne )
	{
		const CPlanar* pP = (const CPlanar*)ne();
		ASSERT( pP );

		if( !pP || !m_Filter.isVisible( *pP ) )
			continue;

		//if( pP->isSelected() && nSelected == 1 )
		//	m_Filter.plate.localAxes = true;

		ASSERT( ne.index() < MAX_GRAPHICS_INDEX );
		int index = ne.index();
		unsigned int glID = SetGraphicsName( GRAPHIC_PLATE, index );
		glLoadName( glID );

		//COLORREF c = ini.color().planarFill;
		//c = InverseColor( ini.color().background );
		//if( m_Filter.drawDetail == DETAIL_HI )
		//{
		COLORREF c = GetElementColor( pP );
		//}
		//if( m_Filter.windowType == DESIGN_WINDOW )
		//{
		//	float color[4] = { float(GetRValue( c )/255.), 
		//               float(GetGValue( c )/255.),
		//			   float(GetBValue( c )/255.), 1.0f/*0.2f*/ };
		//	SetUnityValuesAndColors( pP, color );
		//	ApplyAmbientGLMaterial( color );
		//}
		//else
		//{
		if( bDoTexture )
			c = RGB(255,255,255);
		if( pP->isSelected() )
			c = InverseColor( c );
		ApplyAmbientGLMaterialiv( c );
		//}	
		CGraphicsPlate gp( pP );
		if( bDoTexture )
			gp.SetupTextureCoordinates( m_plateMin, m_plateMax, m_Filter );
		gp.Draw( m_pDC, m_Filter, distFactor, GRAPHIC_SOLID, m_text, localAxisLength );

		//if( pP->isSelected() && nSelected == 1 )
		//	m_Filter.plate.localAxes = showLocalAxes;

	}
	EndSolidDrawing();

	if( bDoTexture )
		EndGLTextureMode();

	if( m_Filter.drawDetail == DETAIL_HI  )
		DisableGLLighting();
}

void CGraphicsModel::DrawAutoMeshedPlates()
{
	if( theAreaManager.meshingInProgress() || theModel.meshed_planars() <= 0 || theModel.meshed_dataIsLocked() || m_bReducedDrawing)
		return;

#pragma message( JL "Need meshed-plates filtering work" )
	if( (theModel.areas() <= 0 ) || !m_Filter.areas )
	{
		return;
	}
	//when drawing plates we have few options:
	//1. Model view 
	//		- solid
	//		- wireframe overlay
	//2. Result view -
	//		- undisplaced (with or without gradient)
	//		- displaced only (with or without gradient)
	//		- displaced and undisplaced (with or without gradient on displaced shape)
	//3. Design view
	//		- undisplaced (with or without design colors )
	//		- undisplaced ungrouped (translucent)

	// the following line keeps the mesher from messing with the mesh while we are drawing it
	theModel.meshed_lockData();

	glLineWidth( float( ini.size().planar ) );

	double distFactor = GetDisplacementFactor();

	double localAxisLength = m_Camera.ScalePixelSize( ini.size().planarAxisLength );

	//wireframe drawing - note: do this first so the wireframe will draw on top of the solid model
	StartWireframeDrawing();
	for( int i = 0; i < theModel.meshed_planars(); i++ )
	{
		const CPlanar* pP = theModel.meshed_planar(i);//(const CPlanar*)ne();
		ASSERT( pP );
		if( !pP || !m_Filter.isVisible( *pP ) )
			continue;

		ASSERT( i < MAX_GRAPHICS_INDEX );
		int index = i;
		unsigned int glID = SetGraphicsName( GRAPHIC_AUTO_PLATE, index );
		glLoadName( glID );

		COLORREF c = GetElementColor( pP );
		if( pP->isSelected() )
			c = InverseColor( c );
		ApplyAmbientGLMaterialiv( c );
		CGraphicsPlate gp( pP );
		//gp.Draw( m_pDC, m_Filter, distFactor, GRAPHIC_LINE, m_text );
		gp.Draw( m_pDC, m_Filter, distFactor, GRAPHIC_LINE, localAxisLength );
	}

	//undisplaced shape in a result view
	if( m_Filter.windowType == POST_WINDOW && m_Filter.undeformed && m_Filter.displacements ){
		StartDashedLineDrawing();
		for( int i = 0; i < theModel.meshed_planars(); i++ )
		{
			const CPlanar* pP = theModel.meshed_planar(i);//(const CPlanar*)ne();
			ASSERT( pP );
			if( !pP || !m_Filter.isVisible( *pP ) )
				continue;

			CGraphicsPlate gp( pP );
			gp.Draw( m_pDC, m_Filter, 0, GRAPHIC_DASHED_LINE, localAxisLength );
		}
		EndDashedLineDrawing();
	}

	//draw solid part
	//setup texture mapping for results
	bool bDoTexture = false;
	if( m_Filter.windowType == POST_WINDOW && m_Filter.plate.GetResultType() != PLATE_NO_RESULT && 
		m_Filter.resultCase()->type() != MODE_SHAPE_RESULTS )
	{
		bDoTexture = true;
		m_plateLegend.StartGLGradientTexture();
		//StartGLGradientTexture( m_plateLegend.m_nContourTexture );
		DisableGLLighting();
	}

	//set drawing state - turn off lighting if we are showing textured results
	if( m_Filter.drawDetail == DETAIL_HI && !bDoTexture )
		EnableGLLighting();
	else 
		DisableGLLighting();
	
	//we show the outline of plates in a model view and design view
	StartSolidDrawing( gPlatePolygonOffset );
	if( m_Filter.windowType == POST_WINDOW )
	{
		for( int i = 0; i < theModel.meshed_planars(); i++ )
		{
			const CPlanar* pP = theModel.meshed_planar(i);//(const CPlanar*)ne();
			ASSERT( pP );

			//don't draw the solid part if the owner's plate is selected
			if( !pP || !m_Filter.isVisible( *pP ) ||
				(pP->areaOwner() && pP->areaOwner()->isSelected()) )
				continue;

			ASSERT( i < MAX_GRAPHICS_INDEX );
			int index = i;
			unsigned int glID = SetGraphicsName( GRAPHIC_AUTO_PLATE, index );

			glLoadName( glID );
			COLORREF c = GetElementColor( pP );
			if( bDoTexture )
				c = RGB(255,255,255);
			if( pP->isSelected() )
				c = InverseColor( c );

			ApplyAmbientGLMaterialiv( c );
			CGraphicsPlate gp( pP );
			if( bDoTexture )
				gp.SetupTextureCoordinates( m_plateMin, m_plateMax, m_Filter );
			gp.Draw( m_pDC, m_Filter, distFactor, GRAPHIC_SOLID, localAxisLength );
		}
	}

	if( m_Filter.drawDetail == DETAIL_HI )
		DisableGLLighting();

	if( bDoTexture )
		EndGLTextureMode();

	// unlock the data so the mesher can modify if necessary
	theModel.meshed_unlockData();
}

void CGraphicsModel::DrawMembers( )
{
	//CTimeThisFunction t("DrawMembers From GraphicsMembers");
	
	if( !m_Filter.CanShowMembers() )
		return;

	//when drawing members we have few options:
	//1. Model view 
	//      - a line
	//		- solid with wireframe overlay
	//2. Result view -
	//		- undisplaced (with or without gradient)
	//		- displaced only (with or without gradient)
	//		- displaced and undisplaced (with or without gradient on displaced shape)
	//3. Design view
	//		- undisplaced (with or without design colors )
	//		- undisplaced ungrouped (translucent)

	glLineWidth( float( ini.size().member ) );

	//int nSelected = theModel.elements( MEMBER_ELEMENT, true );
	//bool showLocalAxes = m_Filter.member.localAxes;
		
	//wireframe overylay drawing
	if( m_Filter.drawDetail != DETAIL_LOW && m_Filter.wireFrameOverlay )
	{
		bool bDeformed = m_Filter.windowType == POST_WINDOW;
		for( CElementIterator ne( MEMBER_ELEMENT, m_Filter.member.filter ); !ne; ++ne ) 
		{
			StartWireframeDrawing();
			CMember* pM = (CMember*)ne();
			ASSERT(pM);
			if( !pM || !m_Filter.isVisible( *pM ) )
				continue;

			//if( pM->isSelected() && nSelected == 1 && m_Filter.windowType == MODEL_WINDOW )
			//	m_Filter.member.localAxes = true;
				
			//is member in camera volume
			//get the frustum, see if both nodes are outside it if so, continue...
			//if( !m_GLFrustum.MemberInFrustum( pM ) )
			//	continue;

			ASSERT( ne.index() < MAX_GRAPHICS_INDEX );
			int index = ne.index();
			unsigned int glID = SetGraphicsName( GRAPHIC_MEMBER, index );
			glLoadName( glID );

			int lineWidth = ini.size().member;
			COLORREF c = GetElementColor( pM );
			//COLORREF c = InverseColor( ini.color().background );
			c = DarkenColor( DarkenColor( c ) );

			if( pM->isARigidLink() )
				c = RGB(255,0,0);
			if( pM->isSelected() )
			{
				c = InverseColor( c );
				float color[4];
				color[0] = (float)(GetRValue( c )/255.); 
				color[1] = (float)(GetGValue( c )/255.); 
				color[2] = (float)(GetBValue( c )/255.);
				color[3] = ALPHA;
				//if it's close to grey we won't see much difference when we invert the color
				if( equal( color[0], 0.5, 0.05 ) && equal( color[1], 0.5, 0.05 ) && equal( color[2], 0.5, 0.05 ) ){
					color[0] = 1.0f;
					color[1] = 1.0f;
					color[2] = 0.0f;
				}
				ApplyAmbientGLMaterial( color );
				lineWidth *= 2;
			}
			else
				ApplyAmbientGLMaterialiv( c );
		
			/*if( pM->isARigidLink() )
			{
				EndSolidDrawing();
				StartWireframeDrawing();
				StartDashedLineDrawing();
			}*/
			//JL note 9/24/2009: use a nice thin line for wireframe overlay
			//glLineWidth( 1 );
			DrawMember( pM, bDeformed, false, 0 );
			//if( pM->isARigidLink() )
			//{
			//	//EndDashedLineDrawing();
			//	/*if( m_Filter.drawDetail == DETAIL_HI )
			//	{
			//		EndWireframeDrawing();
			//		StartSolidDrawing( gMemberPolygonOffset );
			//	}*/
			//}
		}
		EndWireframeDrawing();
	}

	//undisplaced shape in a result view
	if( m_Filter.windowType == POST_WINDOW && m_Filter.undeformed && m_Filter.displacements )
	{
		StartDashedLineDrawing();
		ETGraphicDetail savedDetail = m_Filter.drawDetail;
		m_Filter.drawDetail = DETAIL_LOW;
		glLineWidth( float( ini.size().member ) );
		for( CElementIterator ne( MEMBER_ELEMENT, m_Filter.member.filter ); !ne; ++ne ) 
		{
			CMember* pM = (CMember*)ne();
			ASSERT(pM);
			if( !pM || !m_Filter.isVisible( *pM ) )
				continue;

			//if( pM->isSelected() && nSelected == 1 && m_Filter.windowType == MODEL_WINDOW )
			//	m_Filter.member.localAxes = true;

			//is member in camera volume
			//get the frustum, see if both nodes are outside it if so, continue...
			//if( !m_GLFrustum.MemberInFrustum( pM ) )
			//	continue;

			ASSERT( ne.index() < MAX_GRAPHICS_INDEX );
			int index = ne.index();
			unsigned int glID = SetGraphicsName( GRAPHIC_MEMBER, index );
			glLoadName( glID );
			COLORREF c = GetElementColor( pM/*, style*/ );
			ApplyAmbientGLMaterialiv( c );
			DrawMember( pM, false, false, 0 );

			//if( pM->isSelected() && nSelected == 1 )
			//	m_Filter.member.localAxes = showLocalAxes;
		}
		EndDashedLineDrawing();
		m_Filter.drawDetail = savedDetail;
	}

	//draw solid part
	//setup texture mapping for results
	if( bDoTextureMember() )
	{
		m_memberLegend.StartGLGradientTexture();
		//StartGLGradientTexture( m_plateLegend.m_nContourTexture );
		DisableGLLighting();
	}
	
	//set drawing state - turn off lighting if we are showing textured results
	if( m_Filter.drawDetail == DETAIL_HI && !bDoTextureMember() )
		EnableGLLighting();

	if( m_Filter.drawDetail == DETAIL_HI )
		StartSolidDrawing( gPlatePolygonOffset /*gMemberPolygonOffset*/ );

	//make the member lines draw ontop of grid lines
	//if( m_Filter.drawDetail == DETAIL_LOW )
		//glDepthRange( 0, gMemberLineDisplayDepth );
		
	bool bDeformed = m_Filter.windowType == POST_WINDOW;
	double scale = GetDisplacementFactor();

	for( CElementIterator ne( MEMBER_ELEMENT, m_Filter.member.filter ); !ne; ++ne ) 
	{
		CMember* pM = (CMember*)ne();
		ASSERT(pM);
		
		if( !pM || !m_Filter.isVisible( *pM ) )
			continue;

		//if( pM->isSelected() && nSelected == 1 && m_Filter.windowType == MODEL_WINDOW )
		//	m_Filter.member.localAxes = true;

		//if( !m_GLFrustum.MemberInFrustum( pM ) )
		//		continue;

		ASSERT( ne.index() < MAX_GRAPHICS_INDEX );
		int index = ne.index();
		unsigned int glID = SetGraphicsName( GRAPHIC_MEMBER, index );
		glLoadName( glID );

		COLORREF c = GetElementColor( pM );
		if( pM->isARigidLink() )
			c = RGB(255,0,0);
		//SetGLColor( c );
		//ApplyAmbientGLMaterialiv( c );
		if( m_Filter.windowType == DESIGN_WINDOW )
		{
			float color[4] = { float(GetRValue( c )/255.), 
		               float(GetGValue( c )/255.),
					   float(GetBValue( c )/255.), 1.0f/*0.2f*/ };
			SetUnityValuesAndColors( pM, color );
			ApplyAmbientGLMaterial( color );
		}
		else
		{
			if( bDoTextureMember() )
				c = RGB(255,255,255);
			ApplyAmbientGLMaterialiv( c );
		}	
		int lineWidth = ini.size().member;
		//TeK Change 4/24/2009: if( pM->isSuper() || pM->isARigidLink() )
		// Only members with 'exposed' sub-nodes are really "user-combined" members, we shouldn't
		// show all 'super' members bold, just ones that the user could "fix" or control.
		// Also we want to minimize the display of rigid-link members
		if( pM->hasExposedSubNodes() )
		{
			lineWidth *= 2;
		}
		if( (!m_Filter.wireFrameOverlay || m_Filter.drawDetail == DETAIL_LOW || pM->isARigidLink()) && pM->isSelected() )
		{
			c = InverseColor( ini.color().members );
			//SetGLColor( c );
			ApplyAmbientGLMaterialiv( c );
			lineWidth *= 2;
		}
		glLineWidth( float( lineWidth ) );

		if( m_Filter.drawDetail == DETAIL_LOW )
		{
			DisableGLLighting();
			StartWireframeDrawing();
		}
		if( pM->isARigidLink() )
		{
			EndSolidDrawing();
			StartWireframeDrawing();
			//StartDashedLineDrawing();
		}
		DrawMember( pM, bDeformed, true, scale );
		if( pM->isARigidLink() )
		{
			//EndDashedLineDrawing();
			if( m_Filter.drawDetail == DETAIL_HI )
			{
				EndWireframeDrawing();
				StartSolidDrawing( gMemberPolygonOffset );
			}
		}

		if( m_Filter.windowType == DESIGN_WINDOW )
			EndDashedLineDrawing();

		//if( pM->isSelected() && nSelected == 1 )
		//	m_Filter.member.localAxes = showLocalAxes;
	}

	//if( m_Filter.drawDetail == DETAIL_LOW )
	//	glDepthRange( 0, 1 );
	
	if( m_Filter.drawDetail == DETAIL_HI )
		EndSolidDrawing();

	if( bDoTextureMember() )
		EndGLTextureMode();

	//glDisable( GL_POLYGON_OFFSET_LINE );

	glLineWidth( float( ini.size().member ) );

	return;
}

void CGraphicsModel::DrawMember( const CMember* pM, bool bDisplaced, bool bLabel, double scale )
{
	ASSERT_RETURN( pM );
	const CResultCase* pRC = m_Filter.resultCase();
	if( bDisplaced )
	{
		ASSERT_RETURN( pRC );
		const CResult* cR = pRC->result( *pM );
		if( (pM->oneWayAction() != BOTH_TENSION_AND_COMPRESSION) && cR && cR->isRemoved() )
			return;
	}
	double memberAxisLength = m_Camera.ScalePixelSize( ini.size().memberAxisLength );
	//add and draw the member
	if( bDisplaced ) //displaced shape
	{
		//add it if its not cached
		CGraphicsMember* pMInMap = ((CMember*)(pM))->m_ResultGraphicsMembers[ pRC ];
		if( !pMInMap )
		{
			//m_MyTip.Hide();
			theGraphicsEngine.ClearDisplacedExtrusions( pRC );
			theGraphicsEngine.CreateDisplacedExtrusions( pRC, 
				m_Filter, 
				GetDisplacementFactor(), 
				m_memberMin, 
				m_memberMax,
				m_cableMin,
				m_cableMax );
			pMInMap = ((CMember*)(pM))->m_ResultGraphicsMembers[ pRC ];
		}
		else if( pMInMap && !equal(pMInMap->distortionFactor(), GetDisplacementFactor()) ) {
			//distortion factor has changed so update the extrusion
			//this member is not cached so its likely that none of them are for this result case
			//m_MyTip.Hide();
			theGraphicsEngine.ClearDisplacedExtrusions( pRC );
			theGraphicsEngine.CreateDisplacedExtrusions( pRC, 
				m_Filter, 
				GetDisplacementFactor(), 
				m_memberMin, 
				m_memberMax,
				m_cableMin,
				m_cableMax);
			pMInMap = ((CMember*)(pM))->m_ResultGraphicsMembers[ pRC ];
		}
		// now we can draw it
		double scaledNodeSize = m_Camera.ScalePixelSize( ini.size().node );
		double scaledReleaseLength = m_Camera.ScalePixelSize( ini.size().releaseLength );
		if( pMInMap ){
			//JL note 9/24/2009 - label drawing down so that it draws on top of everything else.
			//if( bLabel )
			//	pMInMap->Draw( m_pDC, m_Filter, 0, m_text, scale, (float)memberAxisLength, scaledNodeSize  );
			//else
				pMInMap->Draw( m_Filter, 0, (float)memberAxisLength, scaledNodeSize, scaledReleaseLength );
			//show results diagram
			if( m_Filter.member.diagramResults && m_Filter.member.resultType != MEMBER_NO_RESULT )
			{
				bool bDoTexture = false;
				if( m_Filter.windowType == POST_WINDOW && m_Filter.member.GetResultType() != MEMBER_NO_RESULT && 
					m_Filter.resultCase()->type() != MODE_SHAPE_RESULTS )
				{
					bDoTexture = true;
					EndGLTextureMode();
				}
				glPushAttrib( GL_LINE_WIDTH );
					glLineWidth( (float)ini.size().axisWidth );
					StartWireframeDrawing();
					//glLineWidth( 1.0f );
					COLORREF c = ini.color().memberForceDisplay;
					SetGLColor( c );
					pMInMap->DrawResultDiagram( m_Filter );
					EndWireframeDrawing();
					StartSolidDrawing( gLoadPolygonOffset );
					DisableGLLighting();
					c = ini.color().memberForceFill;
					SetGLColor( c );
					pMInMap->DrawResultDiagram( m_Filter );
				glPopAttrib();

				if( bDoTextureMember() )
					m_memberLegend.StartGLGradientTexture();

		
			}
			if( bLabel )
				pMInMap->DrawMemberText( m_pDC, m_Filter, m_text, scale ); 
		}
	}
	else //undisplaced shape
	{
		if( !pM->mpGraphicsMember ){
			bool bFoundMatch = false;
			//we need to create a graphics member. check to see if there is one in the model
			//that is similar to the one we need to create.
			// TeK Performance test 7/13/2009, don't check for matches, just create!
			// Performance is faster with the GRAEF huge model, when initially setting up a model
			// without looping over all the members to find similar ones.  I think the isGeometrySimilar()
			// method may be slow, but maybe it is only faster if DETAIL_HI is not ON??
#pragma message( JL "TeK Note: Uncomment this code again, if you want to! Seems faster without it?" )
			//for( CElementIterator ne( MEMBER_ELEMENT ); !ne; ++ne ) 
			//{
			//	if( (CMember*)pM == (CMember*)ne() )
			//		continue;
			//	CGraphicsMember* pGM = ((CMember*)ne())->mpGraphicsMember;
			//	if( pGM && ((CMember*)pM)->isGeometrySimilar( *(CMember*)ne() ) )
			//	{
			//		((CMember*)pM)->mpGraphicsMember = new CGraphicsMember( pM, pGM );
			//		bFoundMatch = true;
			//		break;
			//	}
			//}
			if( !bFoundMatch ){ //create a brand new one
				((CMember*)pM)->mpGraphicsMember = new CGraphicsMember( pM );
			}

		}
		if( pM->mpGraphicsMember )
		{
			//pull back from nodes
			double nodeSizePullBack = m_Camera.ScalePixelSize( ini.size().node );
			double scaledReleaseLength = m_Camera.ScalePixelSize( ini.size().releaseLength );
			if( bLabel )
				pM->mpGraphicsMember->Draw( m_pDC, m_Filter, 0, m_text, 0, (float)memberAxisLength, nodeSizePullBack, scaledReleaseLength );
			else
				pM->mpGraphicsMember->Draw( m_Filter, 0, (float)memberAxisLength, nodeSizePullBack, scaledReleaseLength );
		}
	}
	//if( m_Filter.windowType == DESIGN_WINDOW )
	//{
	//	glDisable( GL_POLYGON_STIPPLE );
	//	glDisable( GL_LINE_STIPPLE );	
	//}
}

void CGraphicsModel::DrawCables( )
{
	if( !m_Filter.CanShowCables() || theModel.elements( CABLE_ELEMENT ) <= 0 )
		return;

	//when drawing cables we have few options:
	//1. Model view 
	//      - a line
	//		- solid with wireframe overlay
	//2. Result view -
	//		- undisplaced (with or without gradient)
	//		- displaced only (with or without gradient)
	//		- displaced and undisplaced (with or without gradient on displaced shape)
	//3. Design view
	//		- same as model view

	//double distFactor = GetDisplacementFactor();

	glLineWidth( float( ini.size().cableWidth ) );

	//int nSelected = theModel.elements( CABLE_ELEMENT, true );
	//bool showLocalAxes = m_Filter.cable.localAxes;
	
	//wireframe overylay drawing - note: do this first so the wireframe will draw on top of the solid model
	if( m_Filter.drawDetail != DETAIL_LOW && m_Filter.wireFrameOverlay)
	{
		bool bDeformed = m_Filter.windowType == POST_WINDOW;
		StartWireframeDrawing();
		for( CElementIterator ne( CABLE_ELEMENT, m_Filter.cable.filter ); !ne; ++ne ) 
		{
			CCable* pC = (CCable*)ne();
			ASSERT(pC);
			if( !pC || !m_Filter.isVisible( *pC ) )
				continue;

			//if( pC->isSelected() && nSelected == 1 )
			//	m_Filter.cable.localAxes = true;

			ASSERT( ne.index() < MAX_GRAPHICS_INDEX );
			int index = ne.index();
			unsigned int glID = SetGraphicsName( GRAPHIC_CABLE, index );
			glLoadName( glID );

			int lineWidth = ini.size().cableWidth;
			COLORREF c = GetElementColor( pC );
			if( pC->isSelected() )
			{
				c = InverseColor( c );
				float color[4];
				color[0] = (float)(GetRValue( c )/255.); 
				color[1] = (float)(GetGValue( c )/255.); 
				color[2] = (float)(GetBValue( c )/255.);
				color[3] = ALPHA;
				//if it's close to grey we won't see much difference when we invert the color
				if( equal( color[0], 0.5, 0.05 ) && equal( color[1], 0.5, 0.05 ) && equal( color[2], 0.5, 0.05 ) ){
					color[0] = 1.0f;
					color[1] = 1.0f;
					color[2] = 0.0f;
				}
				ApplyAmbientGLMaterial( color );
				lineWidth *= 2;
			}
			else
				ApplyAmbientGLMaterialiv( c );

			glLineWidth( float( lineWidth ) );
			DrawCable( pC, bDeformed, false, 0 );

			//if( pC->isSelected() && nSelected == 1 )
			//	m_Filter.cable.localAxes = showLocalAxes;
		}
		EndWireframeDrawing();
	}

	//undisplaced shape in a result view
	if( m_Filter.windowType == POST_WINDOW && m_Filter.undeformed && m_Filter.displacements )
	{
		StartDashedLineDrawing();
		ETGraphicDetail savedDetail = m_Filter.drawDetail;
		m_Filter.drawDetail = DETAIL_LOW;
		for( CElementIterator ne( CABLE_ELEMENT, m_Filter.cable.filter ); !ne; ++ne ) 
		{
			CCable* pC = (CCable*)ne();
			ASSERT(pC);
			if( !pC || !m_Filter.isVisible( *pC ) )
				continue;

			//if( pC->isSelected() && nSelected == 1 )
			//	m_Filter.cable.localAxes = true;

			ASSERT( ne.index() < MAX_GRAPHICS_INDEX );
			int index = ne.index();
			unsigned int glID = SetGraphicsName( GRAPHIC_CABLE, index );
			glLoadName( glID );

			COLORREF c = GetElementColor( pC/*, style*/ );
			ApplyAmbientGLMaterialiv( c );
			DrawCable( pC, false, false, 0 );

			//if( pC->isSelected() && nSelected == 1 )
			//	m_Filter.member.localAxes = showLocalAxes;
		}
		EndDashedLineDrawing();
		m_Filter.drawDetail = savedDetail;
	}

	//draw solid part
	//setup texture mapping for results
	if( bDoTextureCable() )
		m_cableLegend.StartGLGradientTexture();
		
	//set drawing state - turn off lighting if we are showing textured results
	if( m_Filter.drawDetail == DETAIL_HI && !bDoTextureMember() )
		EnableGLLighting();

	if( m_Filter.drawDetail == DETAIL_HI )
		StartSolidDrawing( gMemberPolygonOffset );

	//make the member lines draw ontop of grid lines
	//if( m_Filter.drawDetail == DETAIL_LOW )
	//	glDepthRange( 0, gMemberLineDisplayDepth );

	bool bDeformed = m_Filter.windowType == POST_WINDOW;
	double scale = GetDisplacementFactor();

	for( CElementIterator ne( CABLE_ELEMENT, m_Filter.cable.filter ); !ne; ++ne ) 
	{
		CCable* pC = (CCable*)ne();
		ASSERT(pC);
		if( !pC || !m_Filter.isVisible( *pC ) )
			continue;

		//if( pC->isSelected() && nSelected == 1 )
		//	m_Filter.cable.localAxes = true;

		ASSERT( ne.index() < MAX_GRAPHICS_INDEX );
		int index = ne.index();
		unsigned int glID = SetGraphicsName( GRAPHIC_CABLE, index );
		glLoadName( glID );

		COLORREF c = GetElementColor( pC );
		if( bDoTextureCable() )
			c = RGB(255,255,255);
		ApplyAmbientGLMaterialiv( c );
		int lineWidth = ini.size().cableWidth;
		if( (!m_Filter.wireFrameOverlay || m_Filter.drawDetail == DETAIL_LOW) && pC->isSelected() )
		{
			c = InverseColor( ini.color().cables );
			ApplyAmbientGLMaterialiv( c );
			lineWidth *= 2;
		}
		glLineWidth( float( lineWidth ) );
		if( m_Filter.drawDetail == DETAIL_LOW )
			DisableGLLighting();
		DrawCable( pC, bDeformed, true, scale );

		//if( pC->isSelected() && nSelected == 1 )
		//	m_Filter.cable.localAxes = showLocalAxes;
	}

	if( m_Filter.drawDetail == DETAIL_HI )
		EndSolidDrawing();

	if( bDoTextureCable() )
		EndGLTextureMode();

	return;
}

void CGraphicsModel::DrawCable( const CCable* pC, bool bDisplaced, bool bLabel, double /*scale*/ )
{
	ASSERT_RETURN( pC );
	const CResultCase* pRC = m_Filter.resultCase();

	double cableAxisLength = m_Camera.ScalePixelSize( ini.size().memberAxisLength );
	
	//add and draw the member
	if( bDisplaced ) //displaced shape
	{
		ASSERT_RETURN( pRC );
		//add it if its not cached
		CGraphicsCable* pCInMap = ((CCable*)(pC))->m_ResultGraphicsCables[ pRC ];
		if( !pCInMap )
		{
			//m_MyTip.Hide();
			theGraphicsEngine.ClearDisplacedExtrusions( pRC );
			theGraphicsEngine.CreateDisplacedExtrusions( pRC, 
				m_Filter, 
				GetDisplacementFactor(), 
				m_memberMin, 
				m_memberMax,
				m_cableMin,
				m_cableMax );
			pCInMap = ((CCable*)(pC))->m_ResultGraphicsCables[ pRC ];
		}
		else if( pCInMap && !equal(pCInMap->distortionFactor(), GetDisplacementFactor()) ) {
			//distortion factor has changed so update the extrusion
			//this member is not cached so its likely that none of them are for this result case
			//m_MyTip.Hide();
			theGraphicsEngine.ClearDisplacedExtrusions( pRC );
			theGraphicsEngine.CreateDisplacedExtrusions( pRC, 
				m_Filter, 
				GetDisplacementFactor(), 
				m_memberMin, 
				m_memberMax,
				m_cableMin,
				m_cableMax );
			pCInMap = ((CCable*)(pC))->m_ResultGraphicsCables[ pRC ];
		}
		// now we can draw it
		if( pCInMap ){
			//if( bLabel )
			//	pCInMap->Draw( m_pDC, m_Filter, m_text, cableAxisLength );
			//else
				pCInMap->Draw( m_Filter, cableAxisLength );
			//show results diagram
			if( m_Filter.cable.diagramResults && m_Filter.cable.resultType != CABLE_NO_RESULT )
			{
				bool bDoTexture = false;
				if( m_Filter.windowType == POST_WINDOW && m_Filter.cable.GetResultType() != CABLE_NO_RESULT && 
					m_Filter.resultCase()->type() != MODE_SHAPE_RESULTS )
				{
					bDoTexture = true;
					EndGLTextureMode();
				}

				glPushAttrib( GL_LINE_WIDTH );
					glLineWidth( (float)ini.size().axisWidth );
					StartWireframeDrawing();
					//glLineWidth( 1.0f );
					COLORREF c = ini.color().memberForceDisplay;
					SetGLColor( c );
					pCInMap->DrawResultDiagram( m_Filter );
					StartSolidDrawing( gLoadPolygonOffset );
					DisableGLLighting();
					c = ini.color().memberForceFill;
					SetGLColor( c );
					pCInMap->DrawResultDiagram( m_Filter );
				glPopAttrib();

				if( bDoTexture )
					m_cableLegend.StartGLGradientTexture();
			}
			if( bLabel )
				pCInMap->DrawText( m_pDC, m_Filter, m_text ); 
		}
	}
	else //undisplaced shape
	{
		if( !pC->mpGraphicsCable )
		{
			bool bFoundMatch = false;
			//don't bother checking to see if there is one in the model
			//that is similar to the one we need to create because cables won't 
			//get drawn correctly with this check anyway. JL 6/15/2007
			/*for( CElementIterator ne( CABLE_ELEMENT ); !ne; ++ne ) 
			{
				CGraphicsCable* pGC = ((CCable*)ne())->mpGraphicsCable;
				if( ((CCable*)pC)->isGeometrySimilar( *(CCable*)ne() ) && pGC )
				{
					((CCable*)pC)->mpGraphicsCable = new CGraphicsCable( pC, pGC );
					bFoundMatch = true;
					break;
				}
			}*/
			if( !bFoundMatch ){ //create a brand new one
				((CCable*)pC)->mpGraphicsCable = new CGraphicsCable( pC );
			}
		}
		if( pC && pC->mpGraphicsCable )
		{
			if( bLabel )
				pC->mpGraphicsCable->Draw( m_pDC, m_Filter, m_text/*, scale*/, cableAxisLength );
			else
				pC->mpGraphicsCable->Draw( m_Filter, cableAxisLength );
		}
	}
	if( m_Filter.windowType == DESIGN_WINDOW )
	{
		glDisable( GL_POLYGON_STIPPLE );
		glDisable( GL_LINE_STIPPLE );	
	}
}

void CGraphicsModel::DrawSprings()
{
	if( !m_Filter.CanShowSprings() || !theModel.elements( SPRING_ELEMENT ) )
		return;

	StartWireframeDrawing();
	double distFactor = GetDisplacementFactor();
	double springLength = m_Camera.ScalePixelSize( ini.size().springLength );
	for( CElementIterator ne( SPRING_ELEMENT, m_Filter.spring.filter ); !ne; ++ne )
	{
		if( !m_Filter.isVisible( *(CSpring*)ne() ) )
			continue;
		ASSERT( ne.index() < pow( 2., 24 ));
		unsigned int glID = SetGraphicsName( GRAPHIC_SPRING, ne.index() );
		glLoadName( glID );
		const CSpring* pS = (const CSpring*)ne();
		CGraphicsSpring s( pS );
		s.Draw( m_pDC, m_Filter, distFactor, m_text, springLength );
	}
	return;
}

void CGraphicsModel::DrawNodalLoads( )
{
	if( !m_Filter.CanShowNodalLoads() )
		return;

	CServiceCase* pSC = serviceCase();
	if( pSC ) 
	{
		StartSolidDrawing( gLoadPolygonOffset );
		DisableGLLighting();
		glLineWidth(float( ini.size().nodalLoadWidth ) );
		//loop over nodal loads in this load case
		for( int i = 1; i <= pSC->nodalLoads(); i++ ) 
		{
			CNodalLoad* pNL = ((CNodalLoad*)pSC->nodalLoad( i ));

			if( pNL )
			{
				if( !m_Filter.isVisible( *pNL ) )
					continue;
				ASSERT( i < pow( 2., 24 ));
				unsigned int glID = SetGraphicsName( GRAPHIC_NODAL_LOAD, i );
				glLoadName( glID );
				if( pNL->isSelected() )
				{
					COLORREF c = InverseColor( ini.color().nodalLoads );
					SetGLColor( c );
					ApplyAmbientGLMaterialiv( c );
				}
				else
				{
					SetGLColor( ini.color().nodalLoads );
					COLORREF c = ini.color().nodalLoads;
					ApplyAmbientGLMaterialiv( c );
				}
				if( (pNL->type() == NODAL_SETTLEMENT && m_Filter.node.settlements) || 
					( pNL->type() == NODAL_FORCE && m_Filter.node.loads ) )
				{
					double loadLength = m_Camera.ScalePixelSize( ini.size().nodalLoadLength );
					double loadOffset = m_Camera.ScalePixelSize( 12 );
				
					CGraphicsNodeLoad nl( pNL );
					nl.Draw( m_pDC, m_Filter, m_text, loadLength, loadOffset );
				}
			}
		}
		EndSolidDrawing();
		if( ini.graphics().lighting )
			EnableGLLighting();
	}
	return;
}

void CGraphicsModel::DrawMemberLoads()
{
	if( !m_Filter.CanShowMemberLoads() )
		return;

	CServiceCase* pSC = serviceCase();
	if( pSC ) 
	{
		//setup color
		StartSolidDrawing( gLoadPolygonOffset );
		DisableGLLighting();
		//glLineWidth(float( ini.size().nodalLoadWidth ) );
		//loop over nodal loads in this load case
		for( int i = 1; i <= pSC->elementLoads( MEMBER_ELEMENT ); i++ ) 
		{
			CMemberLoad* pML = ((CMemberLoad*)pSC->elementLoad( MEMBER_ELEMENT, i ));
			if( pML )
			{
				if( !m_Filter.isVisible( *pML ) )
					continue;
				ASSERT( i < pow( 2., 24 ));
				unsigned int glID = SetGraphicsName( GRAPHIC_MEMBER_LOAD, i );
				glLoadName( glID );
				if( pML->isSelected() )
				{
					COLORREF c = InverseColor( ini.color().memberLoads );
					SetGLColor( c );
					ApplyAmbientGLMaterialiv( c );
				}
				else
				{
					SetGLColor( ini.color().memberLoads );
					COLORREF c = ini.color().memberLoads;
					ApplyAmbientGLMaterialiv( c );
				}
				double loadLength = m_Camera.ScalePixelSize( ini.size().memberLoadLength );
				double loadOffset = m_Camera.ScalePixelSize( 8 ); //offset by 8 pixels
				CGraphicsMemberLoad ml( pML );
				ml.Draw(m_pDC, m_Filter, m_text, loadLength, loadOffset );
			}
		}
		EndSolidDrawing();
		if( ini.graphics().lighting )
			EnableGLLighting();
	}
	return;
}

void CGraphicsModel::DrawMovingLoads()
{ 
	CServiceCase* pSC = serviceCase();
	if( pSC ) 
	{
		//setup color
		StartSolidDrawing( gLoadPolygonOffset );
		DisableGLLighting();
		glLineWidth(float( ini.size().nodalLoadWidth ) );
		//loop over nodal loads in this load case
		for( int i = 1; i <= pSC->movingLoads(); i++ ) 
		{
			CMovingLoad* pML = (CMovingLoad*)pSC->movingLoad( i );
			if( pML )
			{
				if( !m_Filter.isVisible( *pML ) )
					continue;
				ASSERT( i < pow( 2., 24 ));
				unsigned int glID = SetGraphicsName( GRAPHIC_MOVING_LOAD, i );
				glLoadName( glID );
				if( pML->isSelected() )
				{
					COLORREF c = InverseColor( ini.color().movingLoads );
					SetGLColor( c );
					ApplyAmbientGLMaterialiv( c );
				}
				else
				{
					SetGLColor( ini.color().movingLoads );
					COLORREF c = ini.color().movingLoads;
					ApplyAmbientGLMaterialiv( c );
				}
				double loadLength = m_Camera.ScalePixelSize( ini.size().memberLoadLength );
				CGraphicsMovingLoad ml( pML );
				ml.Draw( m_pDC, m_Filter, m_text, loadLength );
			}
		}
		EndSolidDrawing();
		if( ini.graphics().lighting )
			EnableGLLighting();
	}
	return;
}

void CGraphicsModel::DrawPlateLoads( )
{
	CServiceCase* pSC = serviceCase();
	if( !pSC || !m_Filter.CanShowPlateLoads()) {
		return;
	}

	StartSolidDrawing( gLoadPolygonOffset );
	DisableGLLighting();
	glLineWidth(float( ini.size().nodalLoadWidth ) );
	for( int i = 1; i <= pSC->elementLoads( PLANAR_ELEMENT ); i++ ) 
	{
		CPlanarLoad* pPL = ((CPlanarLoad*)pSC->elementLoad( PLANAR_ELEMENT, i ));
		if( pPL )
		{
			if( !m_Filter.isVisible( *pPL ) )
				continue;
			ASSERT( i < pow( 2., 24 ));
			unsigned int glID = SetGraphicsName( GRAPHIC_PLATE_LOAD, i );
			glLoadName( glID );
			if( pPL->isSelected() )
			{
				COLORREF c = InverseColor( ini.color().planarLoads );
				SetGLColor( c );
				ApplyAmbientGLMaterialiv( c );
			}
			else
			{
				SetGLColor( ini.color().planarLoads );
				COLORREF c = ini.color().planarLoads;
				ApplyAmbientGLMaterialiv( c );
			}
			double loadLength = m_Camera.ScalePixelSize( ini.size().planarLoadLength );
			CGraphicsPlateLoad pl( pPL );
			pl.Draw( m_pDC, m_Filter, m_text, loadLength );
		}
	}
	EndSolidDrawing();
	if( ini.graphics().lighting )
		EnableGLLighting();

	return;
}

void CGraphicsModel::DrawAreaLoads( )
{
	CServiceCase* pSC = serviceCase();
	if( !pSC || !m_Filter.CanShowAreaLoads() || m_bReducedDrawing) 
		return;

	//wireframe drawing
	DisableGLLighting();
	StartWireframeDrawing();
	for( int i = 1; i <= pSC->areaLoads( ); i++ ) 
	{
		CAreaLoad* pAL = ((CAreaLoad*)pSC->areaLoad( i ));
		if( pAL )
		{
			if( !m_Filter.isVisible( *pAL ) )
				continue;
			ASSERT( i < pow( 2., 24 ));
			unsigned int glID = SetGraphicsName( GRAPHIC_AREA_LOAD, i );
			glLoadName( glID );
			if( pAL->isSelected() )
			{
				COLORREF c = InverseColor( ini.color().planarLoads );
				float color[4] = { GetRValue( c )/255.f, GetGValue( c )/255.f, GetBValue( c )/255.f, ALPHA };
				SetGLColor( c );
				ApplyAmbientGLMaterial( color );
			}
			else
			{
				SetGLColor( ini.color().planarLoads );
				COLORREF c = ini.color().planarLoads;
				float color[4] = { GetRValue( c )/255.f, GetGValue( c )/255.f, GetBValue( c )/255.f, ALPHA };
				ApplyAmbientGLMaterial( color );
			}
			double arrowLength = m_Camera.ScalePixelSize( ini.size().planarLoadLength );
			CGraphicsAreaLoad al( pAL );
			al.Draw( m_pDC, m_Filter, m_text, false, arrowLength );
		}
	}
	EndWireframeDrawing();

	////set drawing state
	//if( m_Filter.drawDetail == DETAIL_HI )
	//	EnableGLLighting();
	//else 
	//	DisableGLLighting();

	//StartSolidDrawing( gLoadPolygonOffset );
	//for( int i = 1; i <= pSC->areaLoads( ); i++ ) {
	//	CAreaLoad* pAL = ((CAreaLoad*)pSC->areaLoad( i ));
	//	if( pAL )
	//	{
	//		if( !m_Filter.isVisible( *pAL ) )
	//			continue;
	//		ASSERT( i < pow( 2., 24 ));
	//		unsigned int glID = SetGraphicsName( GRAPHIC_AREA_LOAD, i );
	//		glLoadName( glID );
	//		if( pAL->isSelected() )
	//		{
	//			COLORREF c = InverseColor( ini.color().planarLoads );
	//			float color[4] = { GetRValue( c )/255.f, GetGValue( c )/255.f, GetBValue( c )/255.f, ALPHA };
	//			SetGLColor( c );
	//			ApplyAmbientGLMaterial( color );
	//		}
	//		else
	//		{
	//			SetGLColor( ini.color().planarLoads );
	//			COLORREF c = ini.color().planarLoads;
	//			float color[4] = { GetRValue( c )/255.f, GetGValue( c )/255.f, GetBValue( c )/255.f, ALPHA };
	//			ApplyAmbientGLMaterial( color );
	//		}
	//		CGraphicsAreaLoad al( pAL );
	//		al.Draw( m_pDC, m_Filter, m_text, true );
	//		//al.Draw( ini.size().planarLoadLength/*scale*/, ini.size().planarLoadWidth, m_Filter.drawDetail, GRAPHIC_LINE );
	//	}
	//}
	//EndSolidDrawing();
}

void CGraphicsModel::DrawRigidDiaphragmLoads( )
{
	CServiceCase* pSC = serviceCase();
	if( !pSC || !m_Filter.CanShowRigidDiaphragmLoads()) {
		return;
	}

	StartSolidDrawing( gLoadPolygonOffset );
	DisableGLLighting();
	glLineWidth(float( ini.size().nodalLoadWidth ) );
	//loop over nodal loads in this load case
	for( int i = 1; i <=pSC->rigidDiaphragmLoads(); i++ ) 
	{
		CRigidDiaphragmLoad* pRDL = ((CRigidDiaphragmLoad*)pSC->rigidDiaphragmLoad( i ));

		if( pRDL )
		{
			if( !m_Filter.isVisible( *pRDL ) )
				continue;
			ASSERT( i < pow( 2., 24 ));
			unsigned int glID = SetGraphicsName( GRAPHIC_RIGID_DIAPHRAGM_LOAD, i );
			glLoadName( glID );
			if( pRDL->isSelected() )
			{
				COLORREF c = InverseColor( ini.color().nodalLoads );
				float color[4] = { GetRValue( c )/255.f, GetGValue( c )/255.f, GetBValue( c )/255.f, ALPHA };
				SetGLColor( c );
				ApplyAmbientGLMaterial( color );
			}
			else
			{
				SetGLColor( ini.color().nodalLoads );
				COLORREF c = ini.color().nodalLoads;
				float color[4] = { GetRValue( c )/255.f, GetGValue( c )/255.f, GetBValue( c )/255.f, ALPHA };
				ApplyAmbientGLMaterial( color );
			}
			double loadLength = m_Camera.ScalePixelSize( ini.size().nodalLoadLength );
			CGraphicsRigidDiaphragmLoad rdl( pRDL );
			rdl.Draw( m_pDC, m_Filter, m_text, loadLength );
		}
	}
	EndSolidDrawing();
	if( ini.graphics().lighting )
		EnableGLLighting();
}

void CGraphicsModel::DrawGroundPlane()
{
	float color[] = {0.25f, 0.25f, 0.25f, 0.5f };
	glColor3fv( color );
	//COLORREF c = ini.color().grid;
	//glColor3ub( GetRValue( c ), GetGValue( c ), GetBValue( c ) );
	
	StartSolidDrawing( 0 );
	if( ini.graphics().lighting )
		EnableGLLighting();
	EnableGLTransparency();
	//glEnable( GL_POLYGON_STIPPLE );
	//glPolygonStipple(stippleMask[8]);
	ApplyAmbientGLMaterial( color );
	glBegin( GL_QUADS );
	float groundSize = 1000.;
	glNormal3f( 0.f, 1.f, 0.f );
	glVertex3f( groundSize, 0.f, groundSize );
	glVertex3f( groundSize, 0.f, -groundSize );
	glVertex3f( -groundSize, 0.f, -groundSize );
	glVertex3f( -groundSize, 0.f, groundSize );
	glEnd();
	//glDisable( GL_POLYGON_STIPPLE );
	DisableGLTransparency();
	EndSolidDrawing();
	
	/*StartWireframeDrawing();
	glNormal3f( 0.f, 1.f, 0.f );
	glVertex3f( groundSize, 0.f, groundSize );
	glVertex3f( groundSize, 0.f, -groundSize );
	glVertex3f( -groundSize, 0.f, -groundSize );
	glVertex3f( -groundSize, 0.f, groundSize );
	glEnd();
	EndWireframeDrawing();*/
	
}

bool CGraphicsModel::SetupResults( const CWindowFilter& windowFilter )
{
	SetWindowFilter( windowFilter );
	SetupMemberResults();
	SetupCableResults();
	SetupPlateResults();
	return true;
}

bool CGraphicsModel::SetupMemberResults( )
{
	// TeK add 1-18-2000, allow NO graph even when result type is specified
	// so we can label result extremes independently
	if( !m_Filter.resultCase() )
		return false;

	if( POST_WINDOW == m_Filter.windowType )
	{
		if( m_Filter.resultCase()->type() == MODE_SHAPE_RESULTS )
			m_Filter.member.resultType = MEMBER_NO_RESULT;
	}

	CResultCase* resultCase = const_cast<CResultCase*>(m_Filter.resultCase());
	if( resultCase )
		m_Filter.member.SetResultRange( resultCase );

	double max = m_Filter.member.maxResult;
	double min = m_Filter.member.minResult;

	if( abs(min) > abs(max) ){
		m_memberMax = min;
		m_memberMin = max;
	}
	else{
		m_memberMax = max;
		m_memberMin = min;
	}

	for( CElementIterator ne( MEMBER_ELEMENT, m_Filter.member.filter ); !ne; ++ne ) 
	{
		const CResultCase* pRC = m_Filter.resultCase();
		ASSERT_RETURN_ITEM( pRC, false );
		CGraphicsMember* pGM = ((CMember*)ne())->m_ResultGraphicsMembers[ pRC ];
		if( !pGM )
		{
			//this member is not cached so its likely that none of them are for this result case
			theGraphicsEngine.CreateDisplacedExtrusions( pRC, 
				m_Filter, 
				GetDisplacementFactor(), 
				m_memberMin, 
				m_memberMax,
				m_cableMin,
				m_cableMax );
		}
		if( pGM )
		{
			const CResult* pR = pRC->result( *ne() );
			pGM->SetupTextureCoords( 
				min, 
				max, 
				m_Filter.member.GetGraphType(), 
				pR, 
				m_Filter.member.diagramResults );
		}
	}
	
	return true;
}

bool CGraphicsModel::SetupCableResults( )
{
	// TeK add 1-18-2000, allow NO graph even when result type is specified
	// so we can label result extremes independently
	if( !m_Filter.resultCase() )
		return false;
	if( POST_WINDOW == m_Filter.windowType )
	{
		if( m_Filter.resultCase()->type() == MODE_SHAPE_RESULTS )
			m_Filter.cable.resultType = CABLE_NO_RESULT;
	}

	CResultCase* resultCase = const_cast<CResultCase*>(m_Filter.resultCase());
	if( resultCase )
		m_Filter.cable.SetResultRange( resultCase );

	double max = m_Filter.cable.maxResult;
	double min = m_Filter.cable.minResult;

	if( abs(min) > abs(max) ){
		m_cableMax = min;
		m_cableMin = max;
	}
	else{
		m_cableMax = max;
		m_cableMin = min;
	}

	for( CElementIterator ne( CABLE_ELEMENT, m_Filter.member.filter ); !ne; ++ne ) 
	{
		const CResultCase* pRC = m_Filter.resultCase();
		ASSERT_RETURN_ITEM( pRC, false );
		CGraphicsCable* pGC = ((CCable*)ne())->m_ResultGraphicsCables[ pRC ];
		if( !pGC )
		{
			//this member is not cached so its likely that none of them are for this result case
			theGraphicsEngine.CreateDisplacedExtrusions( pRC, 
				m_Filter, 
				GetDisplacementFactor(), 
				m_memberMin, 
				m_memberMax,
				m_cableMin,
				m_cableMax );
		}
		if( pGC )
		{
			const CResult* pR = pRC->result( *ne() );
			pGC->SetupTextureCoords( 
				min, 
				max, 
				m_Filter.member.GetGraphType(), 
				pR, 
				m_Filter.cable.diagramResults );
		}
	}

	return true;
}

bool CGraphicsModel::SetupPlateResults( )
{
	if( POST_WINDOW == m_Filter.windowType )
	{
		if( m_Filter.resultCase()->type() == MODE_SHAPE_RESULTS )
			m_Filter.plate.resultType = PLATE_NO_RESULT;
	}

	CResultCase* resultCase = const_cast<CResultCase*>(m_Filter.resultCase());
	if( resultCase )
		m_Filter.plate.SetResultRange( resultCase );

	double max = m_Filter.plate.maxResult;
	double min = m_Filter.plate.minResult;

	if( abs(min) > abs(max) )
	{
		m_plateMax = min;
		m_plateMin = max;
	}
	else
	{
		m_plateMax = max;
		m_plateMin = min;
	}

	return true;
}

void CGraphicsModel::SetUnityValuesAndColors( const CMember* pM, float color[4] )
{
	int group = theGroupManager.isMemberGrouped( pM );
	if( group > 0 ) {
		glDisable( GL_POLYGON_STIPPLE );
		glDisable( GL_LINE_STIPPLE );
		CDesignGroup* pGroup = theGroupManager.group( group );
		ASSERT_RETURN( pGroup );

		bool hasUnityMessage = false;
		double unityValue = -1;
		if( pGroup && pGroup->areUnityChecksValid() )
			unityValue = pGroup->unityCheck( pM, &hasUnityMessage );
		if( unityValue > 0 ) {
			if( hasUnityMessage ){
				glEnable( GL_POLYGON_STIPPLE );
				glPolygonStipple(stippleMask[14]);
				glEnable( GL_LINE_STIPPLE );
				glLineStipple( 4, 0xAAAA );
			}
			// TeK Change 11/19/2007 limit may be slightly larger than 1.0!
			if( unityValue <= VDUnitySuccessLimit() ) { 
				//go from blue to green
				//JL Change 3.17.2008 - normalize unity value if limit is bigger than 1.0
				if( VDUnitySuccessLimit() > 1.0 )
					unityValue = unityValue/VDUnitySuccessLimit();
				color[0] = 0.f;
				color[1] = float(unityValue);
				float v = float(1.0 - unityValue);//float(VDUnitySuccessLimit() - unityValue);
				color[2] = v;
				//color[2] = max(v, 1.0f);
				color[3] = 1.f;
			}
			else {
				// failed
				color[0] = 1.f;
				color[1] = 0.f;
				color[2] = 0.f;
				color[3] = 1.f;
			}
		}
		else {
			// Not Checked! (Dark Grey)
			color[0] = 0.5f;
			color[1] = 0.5f;
			color[2] = 0.5f;
			color[3] = 1.f; // TeK Fix 5/7/2008, my bad, don't use 0 here
		}
	}
	else {  // not grouped - we use a dashed line
		glEnable( GL_POLYGON_STIPPLE );
		glPolygonStipple(stippleMask[1]);
		glEnable( GL_LINE_STIPPLE );
		glLineStipple( 4, 0xAAAA );
		// TeK Changed 1/24/2008: Intention: use a "light gray" instead of BLUE for not grouped
		// Color must match CGraphicsDesignContourBar::Draw()
		color[0] = 0.25f;
		color[1] = 0.25f;
		color[2] = 0.25f;
		color[3] = 1.f;
	}
}

//void CGraphicsModel::SetUnityValuesAndColors( const CPlanar* pP, float color[4] )
//{
//	// unity check values are required - set the pen type and color accordingly
//	int mesh = theMeshManager.isPlateMeshed( pP );
//	if( mesh > 0 ) {
//		CDesignMesh* pMesh = theMeshManager.mesh( mesh );
//		ASSERT_RETURN( pMesh );
//		glDisable( GL_POLYGON_STIPPLE );
//		glDisable( GL_LINE_STIPPLE );
//		double unityValue = -1;
//		if( mesh > 0 ) {
//			bool hasUnityMessage = false;
//			if( pMesh && pMesh->areUnityChecksValid() ) {
//				unityValue = pMesh->unityCheck( pP, &hasUnityMessage );
//			}
//			if( unityValue > 0 ) {
//				if( hasUnityMessage ){
//					glEnable( GL_POLYGON_STIPPLE );
//					glPolygonStipple(stippleMask[14]);
//					glEnable( GL_LINE_STIPPLE );
//					glLineStipple( 4, 0xAAAA );
//				}
//				// TeK Change 11/19/2007 limit may be slightly larger than 1.0!
//				if( unityValue <= VDUnitySuccessLimit() ) { 
//					//go from blue to green
//					color[0] = 0.f;
//					color[1] = float(unityValue);
//					float v = float(VDUnitySuccessLimit() - unityValue);
//					color[2] = max(v, 1.0f);
//					color[3] = 1.f;
//				}else{
//					// failed
//					color[0] = 1.f;
//					color[1] = 0.f;
//					color[2] = 0.f;
//					color[3] = 1.f;
//				}
//			
//			}
//			else {
//				// No Unity Checks? (black)
//				color[0] = 0.f;
//				color[1] = 0.f;
//				color[2] = 0.f;
//				color[3] = 0.f;
//			}
//		}
//		else {  // not grouped - we use a dashed line
//			glEnable( GL_POLYGON_STIPPLE );
//			glPolygonStipple(stippleMask[1]);
//			glEnable( GL_LINE_STIPPLE );
//			glLineStipple( 4, 0xAAAA );
//			color[0] = 0.f;
//			color[1] = 0.f;
//			color[2] = 1.f;
//			color[3] = 1.f;
//		}
//	}
//}

bool CGraphicsModel::bDoTextureMember()
{
	return( m_Filter.windowType == POST_WINDOW && 
		m_Filter.member.GetResultType() != MEMBER_NO_RESULT && 
		m_Filter.resultCase()->type() != MODE_SHAPE_RESULTS && 
		m_Filter.member.coloredResults );
}

bool CGraphicsModel::bDoTextureCable()
{
	return( m_Filter.windowType == POST_WINDOW && 
		m_Filter.cable.GetResultType() != CABLE_NO_RESULT && 
		m_Filter.resultCase()->type() != MODE_SHAPE_RESULTS && 
		m_Filter.cable.coloredResults );
}

//get object color if one exists
COLORREF CGraphicsModel::GetElementColor( const CElement* pE/*, ETGraphicsDrawStyle style*/ )
{
	COLORREF color = RGB(0,0,0);
	ASSERT_RETURN_ITEM( pE, color );

	// Use colorMode except for "Picture View" (High Quality) mode
//#pragma message( TEK "Change 'picture view' color options here--currently 'material' only!" )
	ETColorMode cm = m_Filter.colorMode;
	if( m_Filter.drawDetail != DETAIL_LOW ) {
		cm = MATERIAL_COLORS;
	}

	switch( cm ) {
	default:
		ASSERT(FALSE); // but fall through for a default color!

	case DEFAULT_COLORS:
	case NAMED_COLORS: // Get a default color first, then override, if possible!
		switch( pE->type() ) {
		default:
		ASSERT(FALSE); // new element type?
		case MEMBER_ELEMENT:
			color = ini.color().members;
			break;
		case PLANAR_ELEMENT:
			color = ini.color().planarFill;
			//if( GRAPHIC_SOLID == style ) color = ini.color().planarFill;
			//else color = ini.color().planars;
			break;
		case SPRING_ELEMENT:
			color = ini.color().springs;
			break;
		case CABLE_ELEMENT:
			color = ini.color().cables;
			break;
		}
		if( NAMED_COLORS == m_Filter.colorMode ) {
			// loop through the ini named object colors and see if we match the object name
			for( int i = 0; i < OBJECT_COLOR_SIZE; i++ ) {
				int l = strlen( ini.objectColors().objectNames[i] );
				if( l > 0 ) {
					if( strncmp( pE->name(), ini.objectColors().objectNames[i], l ) == 0 ) {
						color = ini.objectColors().colors[i];
						break; // done!
					}
				}
			}
		} 
		break;

	case MATERIAL_COLORS:
		if( pE->material().GetDBMaterial() ) {
			color = pE->material().GetDBMaterial()->GetColor();
		} else {
			// its a legacy material
			color = pE->material().color( );
		}
		break;
	}
	return color;
}

CServiceCase* CGraphicsModel::serviceCase( void )
{
	CServiceCase* pSC = NULL;
	if( m_Filter.loadCase() ){
		if( m_Filter.loadCase()->type() == SERVICE_CASE ) {
			pSC = dynamic_cast<CServiceCase*>(m_Filter.loadCase());
		}
	}
	return pSC;
}

void CGraphicsModel::DrawLegends( unsigned int printFontScale )
{
	//result legends
	int index = 1;
	CPoint controlPt( 0, 0 );
	CRect r;
	
	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	r.left = 0;
	r.top = 0;
	r.bottom = viewport[3];
	r.right = viewport[2];

	bool memberLegendShown = false;
	bool cableLegendShown = false;
	int y_offset = 0;
	if( m_Filter.windowType == POST_WINDOW && m_Filter.member.resultType != MEMBER_NO_RESULT )
	{
		if( m_Filter.resultCase() && 
			m_Filter.resultCase()->type() != MODE_SHAPE_RESULTS /*&&
			m_Filter.resultCase()->type() != RESPONSE_CASE_RESULTS*/ )
		{
			//draw the contour bar
			if( m_Filter.member.legends && 
				m_Filter.member.resultType != MEMBER_NO_RESULT &&
				theModel.elements( MEMBER_ELEMENT ) > 0 )
			{
				memberLegendShown = true;
				unsigned int glID = SetGraphicsName( GRAPHIC_LEGEND, index++ );
				glLoadName( glID );
				CString result_text = MemberResultsName[ m_Filter.member.resultType ];
				result_text = "Member: " + result_text;
				ETUnit unit = UNITLESS_UNIT;
				int type = int( m_Filter.member.resultType );
				if( type <= MEMBER_DZ )
					unit = LENGTH_SMALL_UNIT;
				else if( type > MEMBER_DZ  && type <= MEMBER_VZ )
					unit = FORCE_UNIT;
				else if( type > MEMBER_VZ && type <= MEMBER_MZ )
					unit = MOMENT_UNIT;
				else if( type > MEMBER_MZ && type <= MEMBER_NORMAL )
					unit = STRESS_UNIT;
				m_memberLegend.SetWindowRect( r );
				m_memberLegend.UpdateLabels( result_text, m_memberMin, m_memberMax );
				m_memberLegend.Draw( m_pDC, 0.f, 1.f, false );
				m_memberLegend.DrawContourBarText( m_contourText, m_pDC /*m_pTextDC*/, /*wndRect*/ r, unit, printFontScale );
				y_offset -= m_memberLegend.boundingRectangle().Height();
			}
		}
	}
	// cable legend
	if( m_Filter.windowType == POST_WINDOW && m_Filter.cable.resultType != CABLE_NO_RESULT )
	{
		if( m_Filter.resultCase() && 
			m_Filter.resultCase()->type() != MODE_SHAPE_RESULTS &&
			m_Filter.resultCase()->type() != RESPONSE_CASE_RESULTS )
		{
			//draw the contour bar
			if( m_Filter.cable.legends && 
				m_Filter.cable.coloredResults &&
				theModel.elements( CABLE_ELEMENT ) > 0 )
			{
				cableLegendShown = true;
				unsigned int glID = SetGraphicsName( GRAPHIC_LEGEND, index++ );
				glLoadName( glID );
				CString result_text = CableResultsName[ m_Filter.cable.resultType ];
				result_text = "Cable: " + result_text;
				ETUnit unit = UNITLESS_UNIT;
				int type = int( m_Filter.cable.resultType );
				if( type == CABLE_DX_RESULT || type == CABLE_DY_RESULT || type == CABLE_DZ_RESULT )
					unit = LENGTH_SMALL_UNIT;
				else if( type == CABLE_FORCE_RESULT )
					unit = FORCE_UNIT;
				else if( type == CABLE_STRESS_RESULT )
					unit = STRESS_UNIT;
				m_cableLegend.SetWindowRect( r );
				m_cableLegend.UpdateLabels( result_text, m_cableMin, m_cableMax );
				m_cableLegend.SetStartOffset( CPoint( 0, y_offset ) );
				m_cableLegend.Draw( m_pDC, 0.f, 1.f, false );
				m_cableLegend.DrawContourBarText( m_contourText, m_pDC /*m_pTextDC*/, r /*r*/, unit, printFontScale );
				y_offset -= m_cableLegend.boundingRectangle().Height();
			}
		}
	}
	// plate legend
	if( m_Filter.windowType == POST_WINDOW && 
		m_Filter.plate.GetResultType() != PLATE_NO_RESULT &&
		theModel.elements( PLANAR_ELEMENT ) > 0 )
	{
		if( m_Filter.resultCase() && 
			m_Filter.resultCase()->type() != MODE_SHAPE_RESULTS &&
			m_Filter.resultCase()->type() != RESPONSE_CASE_RESULTS )
		{
			//draw the contour bar
			if( m_Filter.plate.legends ){
				int index = 1;
				unsigned int glID = SetGraphicsName( GRAPHIC_LEGEND, index );
				glLoadName( glID );
				
				CString result_text = "Plate: ";
				result_text += PlateResultsName[ m_Filter.plate.GetResultType() ];
				
				ETUnit unit = UNITLESS_UNIT;
				
				int graphType = m_Filter.plate.GetGraphType();

				bool isGlobal = m_Filter.plate.isGlobal();
				bool isPrincipal = m_Filter.plate.isPrincipal();
				if( graphType == 13 )
					unit = LENGTH_SMALL_UNIT;
				else if( graphType == 14 )  // bearing pressures
					unit = STRESS_UNIT;
				else if( !isPrincipal ) {
					if( graphType <= TAU_XY || ( isGlobal && graphType >= 9 ) ) {
						unit = STRESS_UNIT;
					}else {
						//forces
						switch( graphType ){
						case MOMENT_X:
						case MOMENT_Y:
						case MOMENT_XY:
						case MOMENT_Z:
							unit = LINEAR_MOMENT_UNIT;
							break;
						case SHEAR_X:
						case SHEAR_Y:
							unit = LINEAR_FORCE_UNIT; 
							break;
						default:
							break;
						}
					}
				}
				else {  // principal stresses requested
					unit = STRESS_UNIT;
				}
				m_plateLegend.SetWindowRect( r );
				m_plateLegend.UpdateLabels( result_text, m_plateMin, m_plateMax );
				m_plateLegend.SetStartOffset( CPoint( 0, y_offset ) );
				m_plateLegend.Draw( m_pDC, 0.f, 1.f, false );
				m_plateLegend.DrawContourBarText( m_contourText, m_pDC /*m_pTextDC*/, r, unit, printFontScale );
				y_offset -= m_plateLegend.boundingRectangle().Height();
			}
		}
	}
	//design legend
	if( m_Filter.windowType == DESIGN_WINDOW && (m_Filter.designLegend) )
	{
		//draw the contour bar
		int index = 1;
		unsigned int glID = SetGraphicsName( GRAPHIC_LEGEND, index );
		glLoadName( glID );
		//CRect wndRect;
		//GetClientRect( wndRect );
		CString result_text = "Unity Check Values";
		ETUnit unit = UNITLESS_UNIT;
		//go from blue to green (0...1)
		m_memberDesignLegend.SetWindowRect( r );
		// TeK Changed 3/17/2008: Use preference limit rather than hard-coded 1.0
		m_memberDesignLegend.UpdateLabels( result_text, 0., VDUnitySuccessLimit() );
		m_memberDesignLegend.Draw( m_pDC, 0.25f, 0.5f  );
		m_memberDesignLegend.DrawContourBarText( m_contourText, m_pDC /*m_pTextDC*/, r, unit, printFontScale );
	}
}

CGraphicsContourBar* CGraphicsModel::HitContourBar( CPoint pt )
{
	CGraphicsContourBar* contourBar = NULL;
	if( m_Filter.windowType == POST_WINDOW )
	{
		if( m_Filter.member.legends )
			contourBar = (CGraphicsContourBar*)m_memberLegend.pointIsOver( pt );
		if( !contourBar && m_Filter.cable.legends )
			contourBar = (CGraphicsContourBar*)m_cableLegend.pointIsOver( pt );
		if( !contourBar && m_Filter.plate.legends )
			contourBar = (CGraphicsContourBar*)m_plateLegend.pointIsOver( pt );
	}
	else if( m_Filter.windowType == DESIGN_WINDOW )
	{
		if( m_Filter.designLegend )
			contourBar = (CGraphicsContourBar*)m_memberDesignLegend.pointIsOver( pt );
	}
	return contourBar;
}

const CNode* CGraphicsModel::OverNode( const CHitRecordCollection& rHitCollection )
{
	// OpengGL selection implementation:
	// 1. We get the hit record collection once per event (ie. mouse move, button down, etc ).
	// 2. The hit record collection contains depth sorted hits on all graphic entities in the model 
	//    e.g. anything with a glLoadName(  )
	// 3. We currently just loop over all hit records and check the ones we're interested in
	// 4. we can choose which entities (display lists we draw) when performing selection so for
	//    example, we may only draw nodes and then check for hits. The selection buffer will only
	//    contain nodes and therefore only report hit nodes. This might be useful later to speed things
	//    up but for now just picking everything that is currently displayed seems to work fine.
	// 5. If you see a way to speed or clean this up feel free to monkey with it...
	//
 
	////CTimeThisFunction t("mouseOverNode");
	////last hit on stack is the top most in the z-buffer
	//const CNode* pN = NULL;
	//int nHits = rHitCollection.m_arrayHits.GetSize();
	//if( nHits <= 0 )
	//	return NULL;
	//for( int i = nHits-1; i >= 0 ; i-- )
	//{
	//	CHitRecord* pHR = rHitCollection.m_arrayHits[i];
	//	//ASSERT( pHR->m_arrayNames.GetSize() == 1 );
	//	if( pHR->m_arrayNames.GetSize() != 1 ) continue;
	//	if( GetModelItemType( pHR->m_arrayNames[0] ) == GRAPHIC_NODE )
	//	{
	//		//TRACE( "Graphics node found, index: %i\n", GetModelItemIndex( pHR->m_arrayNames[0] ) );
	//		pN = theModel.node( GetModelItemIndex( pHR->m_arrayNames[0] ) );
	//		ASSERT( pN );
	//		return pN;
	//	}
	//}

	//CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	const CNode* pN = NULL;
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		//ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( pHR->m_arrayNames.GetSize() != 1 ) continue;
		if( GetModelItemType( pHR->m_arrayNames[0] ) == GRAPHIC_NODE )
		{
			//TRACE( "Graphics node found, index: %i\n", GetModelItemIndex( pHR->m_arrayNames[0] ) );
			pN = theModel.node( GetModelItemIndex( pHR->m_arrayNames[0] ) );
			ASSERT( pN );
			return pN;
		}
	}


	//CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	//for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	//{
	//	if( i == 0 ) 
	//		pClosestHR = rHitCollection.m_arrayHits[i];
	//	CHitRecord* pHR = rHitCollection.m_arrayHits[i];
	//	ASSERT( pHR->m_arrayNames.GetSize() == 1 );
	//	if( pClosestHR->m_nMinZDepth > pHR->m_nMinZDepth )
	//		pClosestHR = pHR;
	//}
	//if( pClosestHR ){
	//	if( GetModelItemType( pClosestHR->m_arrayNames[0] ) == GRAPHIC_NODE )
	//	{
	//		//TRACE( "Graphics node found, index: %i\n", GetModelItemIndex( pHR->m_arrayNames[0] ) );
	//		const CNode* pN = theModel.node( GetModelItemIndex( pClosestHR->m_arrayNames[0] ) );
	//		ASSERT( pN );
	//		return pN;
	//	}
	//}
	return NULL;
}

// this is alot faster than using OpenGL hit test method.
// and it keeps the cursor from snapping to only grid points
// JL 5/1/2007
bool CGraphicsModel::OverNodeOrGridPoint( CPoint screenPt, CPoint3D& _pt3D, const CWindowFilter& filter )
{
	const CPoint3D* pP = NULL;
	const CPoint3D* pPTemp = NULL;
	bool bHit = false;
	//get the closest grid point using the grid's function
	//1. get the ray for the mouse location going from the front of the view frustum
	//   to the back
	CPoint3D pt1, pt2;
	if( !Get3DPointFromScreen( screenPt, pt1, 0 ) || 
		!Get3DPointFromScreen( screenPt, pt2, 1 ) ||
		theGridManager.gridCount() < 1 )
		return pP != NULL;
	//we should now have a ray from the front plane to the back plane
	CLine3D ray( pt1, pt2 );
	double smallestDistance = 1e9;
	double distance = 1e-9;
	//2. now loop over visible grids and get the closest grid point
	for( int i = 1; i <= theGridManager.gridCount(); i++ )
	{
		if( theGridManager.grid( i )->isVisible( ) )
		{
			pPTemp = (const CPoint3D*)theGridManager.grid( i )->closestGridPoint( ray, &distance );
			if( distance < smallestDistance )
			{
				smallestDistance = distance;
				_pt3D.x = pPTemp->x;
				_pt3D.y = pPTemp->y;
				_pt3D.z = pPTemp->z;
				bHit = true;
			}
		}
	}
	//double tolerance = 5;
	//3. now loop over visible nodes and get the closest node point
	//const CNode* pClosestNode = NULL;
	for( CNodeIterator ni( filter.node.filter ); !ni; ++ni )
	{
		if( !filter.isVisible( *(CNode*)ni() ) )
			continue;
		distance = ray.distance_to( *(CNode*)ni() );
		if( distance < smallestDistance )
		{
			smallestDistance = distance;
			_pt3D.x = ni()->x();
			_pt3D.y = ni()->y();
			_pt3D.z = ni()->z();
			bHit = true;
		}
	}

	//4. loop over visible area vertices and get the closest area vertex
	for( int i = 1; i <= theModel.areas( ); i++ ) {
		const CArea* pArea = theModel.area( i );
		if( !pArea || !filter.isVisible( *pArea ) )
			continue;
		//area boundary
		for( int j = 1; j <= pArea->vertices(); j++ ){
			const CCoordinate* pC = pArea->vertex(j);
			ASSERT( pC );
			if( !pC ) 
				continue;
			distance = ray.distance_to( *pC );
			if( distance < smallestDistance )
			{
				smallestDistance = distance;
				_pt3D.x = pC->x();
				_pt3D.y = pC->y();
				_pt3D.z = pC->z();
				bHit = true;
			}
		}
	}
	return bHit;
}

// for certain graphic objects we can use either point distance test (usually faster) 
// or an opengl hit record test. set bUseHitRecordTest to use the opengl method
const CData* CGraphicsModel::OverGraphicObject( CPoint /*p*/, ETGraphicObject go, const CHitRecordCollection& rHitCollection, const CServiceCase* pSC  )
{
	//CTimeThisFunction t("OverGraphicObject");
	switch( go ) {
		case GRAPHIC_AREA_VERTEX:
			return OverAreaVertex( rHitCollection );
		case GRAPHIC_NODE:
			return OverNode( rHitCollection );
		case GRAPHIC_NODAL_LOAD:
			return OverNodalLoad( rHitCollection, pSC );
		case GRAPHIC_SPRING:
			return OverSpring( rHitCollection );
		case GRAPHIC_MEMBER:
			return OverMember( rHitCollection );
		case GRAPHIC_CABLE:
			return OverCable( rHitCollection );
		case GRAPHIC_PLATE:
			return OverPlate( rHitCollection );
		case GRAPHIC_AUTO_PLATE:
			return OverAutoPlate( rHitCollection );
		case GRAPHIC_AREA:
			return OverArea( rHitCollection );
		case GRAPHIC_AREA_HOLE:
			return OverAreaHole( rHitCollection );
		case GRAPHIC_AREA_CORRIDOR:
			return OverAreaCorridor( rHitCollection );
		case GRAPHIC_FOUNDATION:
			return OverFoundation( rHitCollection );
		case GRAPHIC_MEMBER_LOAD:
			return OverMemberLoad( rHitCollection, pSC );
		case GRAPHIC_PLATE_LOAD:
			return (const CData*)OverPlateLoad( rHitCollection, pSC );
		case GRAPHIC_AREA_LOAD:
			return OverAreaLoad( rHitCollection, pSC );
		case GRAPHIC_MOVING_LOAD:
			return (const CData*)OverMovingLoad( rHitCollection, pSC );
		case GRAPHIC_RIGID_DIAPHRAGM:
			return OverRigidDiaphragm( rHitCollection );
		case GRAPHIC_RIGID_DIAPHRAGM_LOAD:
			return OverRigidDiaphragmLoad( rHitCollection, pSC );
		default:
			return NULL;
	};
}

const CMember* CGraphicsModel::OverMember( const CHitRecordCollection& rHitCollection )
{
	//CTimeThisFunction t("mouseOverMember");
	CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		if( i == 0 ) 
			pClosestHR = rHitCollection.m_arrayHits[i];
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( pClosestHR->m_nMinZDepth > pHR->m_nMinZDepth )
			pClosestHR = pHR;
	}
	if( pClosestHR ){
		if( GetModelItemType( pClosestHR->m_arrayNames[0] ) == GRAPHIC_MEMBER )
		{
			const CMember* pM = (const CMember*)theModel.element( MEMBER_ELEMENT, GetModelItemIndex( pClosestHR->m_arrayNames[0] ) );
			ASSERT( pM );
			return pM;
		}
	}
	return NULL;
}

const CPlanar* CGraphicsModel::OverPlate( const CHitRecordCollection& rHitCollection )
{
	CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		if( i == 0 ) 
			pClosestHR = rHitCollection.m_arrayHits[i];
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( pClosestHR->m_nMinZDepth > pHR->m_nMinZDepth )
			pClosestHR = pHR;
	}
	if( pClosestHR ){
		if( GetModelItemType( pClosestHR->m_arrayNames[0] ) == GRAPHIC_PLATE )
		{
			const CPlanar* pP = (const CPlanar*)theModel.element( PLANAR_ELEMENT, GetModelItemIndex( pClosestHR->m_arrayNames[0] ) );
			ASSERT( pP );
			return pP;
		}
	}
	return NULL;
}

const CPlanar* CGraphicsModel::OverAutoPlate( const CHitRecordCollection& rHitCollection )
{
	CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		if( i == 0 ) 
			pClosestHR = rHitCollection.m_arrayHits[i];
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( pClosestHR->m_nMinZDepth > pHR->m_nMinZDepth )
			pClosestHR = pHR;
	}
	if( pClosestHR ){
		if( GetModelItemType( pClosestHR->m_arrayNames[0] ) == GRAPHIC_AUTO_PLATE )
		{
			const CPlanar* pP = (const CPlanar*)theModel.meshed_planar( GetModelItemIndex( pClosestHR->m_arrayNames[0] ) );
			ASSERT( pP );
			return pP;
		}
	}
	return NULL;
}

const CSpring* CGraphicsModel::OverSpring( const CHitRecordCollection& rHitCollection )
{
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( GetModelItemType( pHR->m_arrayNames[0] ) == GRAPHIC_SPRING )
		{
			//TRACE( "Graphics plate found, index: %i\n", GetModelItemIndex( pHR->m_arrayNames[0] ) );
			const CSpring* pS = (const CSpring*)theModel.element( SPRING_ELEMENT, GetModelItemIndex( pHR->m_arrayNames[0] ) );
			ASSERT( pS );
			return pS;
			break;
		}
	}
	return NULL;
}

const CCable* CGraphicsModel::OverCable( const CHitRecordCollection& rHitCollection )
{
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( GetModelItemType( pHR->m_arrayNames[0] ) == GRAPHIC_CABLE )
		{
			//TRACE( "Graphics member found, index: %i\n", GetModelItemIndex( pHR->m_arrayNames[0] ) );
			const CCable* pC = (const CCable*)theModel.element( CABLE_ELEMENT, GetModelItemIndex( pHR->m_arrayNames[0] ) );
			ASSERT( pC );
			return pC;
			break;
		}
	}
	return NULL;
}

const CArea* CGraphicsModel::OverArea( const CHitRecordCollection& rHitCollection )
{
	CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		if( i == 0 ) 
			pClosestHR = rHitCollection.m_arrayHits[i];
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( pClosestHR->m_nMinZDepth > pHR->m_nMinZDepth )
			pClosestHR = pHR;
	}
	if( pClosestHR ){
		if( GetModelItemType( pClosestHR->m_arrayNames[0] ) == GRAPHIC_AREA )
		{
			const CArea* pA = (const CArea*)theModel.area( GetModelItemIndex( pClosestHR->m_arrayNames[0] ) );
			ASSERT( pA );
			return pA;
		}
	}
	return NULL;
}

const CChain* CGraphicsModel::OverAreaHole( const CHitRecordCollection& rHitCollection )
{
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( GetModelItemType( pHR->m_arrayNames[0] ) == GRAPHIC_AREA_HOLE )
		{
			int packed_index = GetModelItemIndex( pHR->m_arrayNames[0] );
			int area_index = GetAreaIndex( packed_index );
			ASSERT_RETURN_ITEM( area_index <= theModel.areas(), NULL );
			const CArea* pA = theModel.area( area_index );
			ASSERT_RETURN_ITEM( pA, NULL );
			int hole_index = GetAreaItemIndex( packed_index );
			ASSERT_RETURN_ITEM( hole_index <= pA->holes(), NULL );
			return pA->hole(hole_index);
			//const CCoordinate* pC = pA->vertex(vertex_index);
			//ASSERT_RETURN_ITEM( pC, NULL );
			//	return pC;
		}
	}
	return NULL;
}

const CChain* CGraphicsModel::OverAreaCorridor( const CHitRecordCollection& rHitCollection )
{
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( GetModelItemType( pHR->m_arrayNames[0] ) == GRAPHIC_AREA_CORRIDOR )
		{
			int packed_index = GetModelItemIndex( pHR->m_arrayNames[0] );
			int area_index = GetAreaIndex( packed_index );
			ASSERT_RETURN_ITEM( area_index <= theModel.areas(), NULL );
			const CArea* pA = theModel.area( area_index );
			ASSERT_RETURN_ITEM( pA, NULL );
			int corridor_index = GetAreaItemIndex( packed_index );
			ASSERT_RETURN_ITEM( corridor_index <= pA->corridors(), NULL );
			return pA->corridor(corridor_index);
			//const CCoordinate* pC = pA->vertex(vertex_index);
			//ASSERT_RETURN_ITEM( pC, NULL );
			//	return pC;
		}
	}
	return NULL;
}

const CCoordinate* CGraphicsModel::OverAreaVertex( const CHitRecordCollection& rHitCollection )
{
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( GetModelItemType( pHR->m_arrayNames[0] ) == GRAPHIC_AREA_VERTEX )
		{
			int packed_index = GetModelItemIndex( pHR->m_arrayNames[0] );
			int area_index = GetAreaIndex( packed_index );
			ASSERT_RETURN_ITEM( area_index <= theModel.areas(), NULL );
			const CArea* pA = theModel.area( area_index );
			ASSERT_RETURN_ITEM( pA, NULL );
			int vertex_index = GetAreaItemIndex( packed_index );
			ASSERT_RETURN_ITEM( vertex_index <= pA->vertices(), NULL );
			// vertex_index should be okay, we just need to have the area class resolve the index
			// for us since it can be a boundary, hole or corridor index.
			const CCoordinate* pC = pA->vertex(vertex_index);
			ASSERT_RETURN_ITEM( pC, NULL );
				return pC;
		}
	}
	return NULL;
}

const CFoundation* CGraphicsModel::OverFoundation( const CHitRecordCollection& rHitCollection )
{
	CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		if( i == 0 ) 
			pClosestHR = rHitCollection.m_arrayHits[i];
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( pClosestHR->m_nMinZDepth > pHR->m_nMinZDepth )
			pClosestHR = pHR;
	}
	if( pClosestHR ){
		if( GetModelItemType( pClosestHR->m_arrayNames[0] ) == GRAPHIC_FOUNDATION )
		{
			const CFoundation* pF = (const CFoundation*)theModel.foundation( GetModelItemIndex( pClosestHR->m_arrayNames[0] ) );
			ASSERT( pF );
			return pF;
		}
	}
	return NULL;
}

const CNodalLoad* CGraphicsModel::OverNodalLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC )
{
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( GetModelItemType( pHR->m_arrayNames[0] ) == GRAPHIC_NODAL_LOAD )
		{
			if( pSC ) 
			{
				int index = GetModelItemIndex( pHR->m_arrayNames[0] );
				CNodalLoad* pNL = ((CNodalLoad*)pSC->nodalLoad( index ));
				if( pNL )
					return pNL;		
			}
		}
	}
	return NULL;
}

const CMemberLoad* CGraphicsModel::OverMemberLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC )
{
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( GetModelItemType( pHR->m_arrayNames[0] ) == GRAPHIC_MEMBER_LOAD )
		{
			if( pSC ) 
			{
				int index = GetModelItemIndex( pHR->m_arrayNames[0] );
				CMemberLoad* pML = ((CMemberLoad*)pSC->elementLoad( MEMBER_ELEMENT, index ));
				if( pML )
					return pML;		
			}
		}
	}
	return NULL;
}

const CPlanarLoad* CGraphicsModel::OverPlateLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC )
{
	CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		if( i == 0 ) 
			pClosestHR = rHitCollection.m_arrayHits[i];
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( pClosestHR->m_nMinZDepth > pHR->m_nMinZDepth )
			pClosestHR = pHR;
	}
	if( pClosestHR ){
		if( GetModelItemType( pClosestHR->m_arrayNames[0] ) == GRAPHIC_PLATE_LOAD )
		{
			if( pSC ) 
			{
				int index = GetModelItemIndex( pClosestHR->m_arrayNames[0] );
				CPlanarLoad* pPL = ((CPlanarLoad*)pSC->elementLoad( PLANAR_ELEMENT, index ));
				if( pPL )
					return pPL;		
			}
		}
	}
	return NULL;

}

const CMovingLoad* CGraphicsModel::OverMovingLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC )
{
	CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		if( i == 0 ) 
			pClosestHR = rHitCollection.m_arrayHits[i];
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( pClosestHR->m_nMinZDepth > pHR->m_nMinZDepth )
			pClosestHR = pHR;
	}
	if( pClosestHR ){
		if( GetModelItemType( pClosestHR->m_arrayNames[0] ) == GRAPHIC_MOVING_LOAD )
		{
			if( pSC ) 
			{
				int index = GetModelItemIndex( pClosestHR->m_arrayNames[0] );
				CMovingLoad* pML = ((CMovingLoad*)pSC->movingLoad( index ));
				if( pML )
					return pML;		
			}
		}
	}
	return NULL;
}

const CAreaLoad* CGraphicsModel::OverAreaLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC )
{
	CHitRecord* pClosestHR = NULL; //so that we know which object is closest to viewer
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		if( i == 0 ) 
			pClosestHR = rHitCollection.m_arrayHits[i];
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( pClosestHR->m_nMinZDepth > pHR->m_nMinZDepth )
			pClosestHR = pHR;
	}
	if( pClosestHR ){
		if( GetModelItemType( pClosestHR->m_arrayNames[0] ) == GRAPHIC_AREA_LOAD )
		{
			if( pSC ) 
			{
				int index = GetModelItemIndex( pClosestHR->m_arrayNames[0] );
				CAreaLoad* pAL = ((CAreaLoad*)pSC->areaLoad( index ));
				if( pAL )
					return pAL;		
			}
		}
	}
	return NULL;

}

const CRigidDiaphragmLoad* CGraphicsModel::OverRigidDiaphragmLoad( const CHitRecordCollection& rHitCollection, const CServiceCase* pSC )
{
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( GetModelItemType( pHR->m_arrayNames[0] ) == GRAPHIC_RIGID_DIAPHRAGM_LOAD )
		{
			if( pSC ) 
			{
				int index = GetModelItemIndex( pHR->m_arrayNames[0] );
				CRigidDiaphragmLoad* pRDL = ((CRigidDiaphragmLoad*)pSC->rigidDiaphragmLoad( index ));
				if( pRDL )
					return pRDL;		
			}
		}
	}
	return NULL;
}

const CRigidDiaphragm* CGraphicsModel::OverRigidDiaphragm( const CHitRecordCollection& rHitCollection )
{
	for( int i = 0; i < rHitCollection.m_arrayHits.GetSize(); i++ )
	{
		CHitRecord* pHR = rHitCollection.m_arrayHits[i];
		ASSERT( pHR->m_arrayNames.GetSize() == 1 );
		if( GetModelItemType( pHR->m_arrayNames[0] ) == GRAPHIC_RIGID_DIAPHRAGM )
		{
			const CRigidDiaphragm* pRD = (const CRigidDiaphragm*)theModel.rigidDiaphragm( GetModelItemIndex( pHR->m_arrayNames[0] ) );
			ASSERT( pRD );
			return pRD;
			break;
		}
	}
	return NULL;
}