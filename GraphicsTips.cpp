// GraphicsTips.cpp
// CGraphicView functions dedicated to "fly by" tips
// TeK Split out from GraphicsMouse.cpp

#include "stdafx.h"
#pragma hdrstop

#include "GraphicView.h"
#include "Model.h"
#include "ServCase.h"
#include "Results.h"
#include "MovLoad.h"

#include "Design/GroupManager.h"
#include "Design/Mesh.h"

#include "Dialog/VAHelpPane.h"

#include "User/Control.h"
#include "VisualDesign/VDInterface.h"

#include "WindowTips.h"

LRESULT CGraphicView::MsgFlyoverTipText( WPARAM /*ignored*/, LPARAM str )
{
	if( !isActive() || !m_bHasBeenDrawnOnce ) return TRUE;  // don't put up flyover if we're not active
	if( NULL != str )
	{
		CString& s = *((CString*)str);
		if( m_Filter.windowType == MODEL_WINDOW )
			return FlyOverModelWindow( s );
		if( m_Filter.windowType == DESIGN_WINDOW )
			return FlyOverDesignWindow( s );
		if( m_Filter.windowType == POST_WINDOW )
			return FlyOverResultWindow( s );
	}
	return FALSE;
}


BOOL CGraphicView::FlyOverModelWindow( CString& s )
{
	if( !m_bHasBeenDrawnOnce )
		FALSE;

	//CTimeThisFunction t( "OnDraw" );
	if( ::IsIconic( GetParent()->m_hWnd ) ) 
		FALSE;  // nothing to do

	CPoint point;
	m_MyTip.GetPoint( point );

	if( Mouse( MOUSE_ZOOMBOX_DRAGGING ) || Mouse( MOUSE_MODEL_DRAGGING ) )  
		return FALSE;

	if( !wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC ) )
		return FALSE;

	CHitRecordCollection hitCollection;
	GetSelectionHitList( point, ini.size().node, ini.size().node, hitCollection );

	BOOL bSet = FALSE;
	const CNode* pNode = m_graphicsModel.OverNode( hitCollection );
	if( pNode )
	{
		s = CWindowTips::GetModelWindowTip( pNode );
		bSet = TRUE;
	}
	if( FALSE == bSet ) {
		const CMember* pMember = m_graphicsModel.OverMember( hitCollection );
		if( pMember ){
			s = CWindowTips::GetModelWindowTip( pMember );
			bSet = TRUE;
		}
	}
	if( FALSE == bSet ) {
		const CCable* pCable = m_graphicsModel.OverCable( hitCollection );
		if( pCable )
		{
			s = CWindowTips::GetModelWindowTip( pCable );
			bSet = TRUE;
		}
	}
	if( FALSE == bSet ) {
		const CPlanar* pPlate = m_graphicsModel.OverPlate( hitCollection );
		if( pPlate )
		{
			s = CWindowTips::GetModelWindowTip( pPlate );
			bSet = TRUE;
		}
	}
	if( FALSE == bSet ) {
		const CSpring* pSpring = m_graphicsModel.OverSpring( hitCollection );
		if( pSpring )
		{
			s = CWindowTips::GetModelWindowTip( pSpring );
			bSet = TRUE;
		}
	}

	// TeK Note: We would like more tooltip information
#pragma message( JL "Add tool tip support for areas, loads, grid points, etc, if EASY to do..." )

	if( bSet && (s.GetLength() > 0) ) {
		VAHelpPane.showTempTip( s );
	}
	if( !m_Filter.flyoverData ) {
		s.Empty();
		return TRUE;
	}
	return FALSE;
}

BOOL CGraphicView::FlyOverResultWindow( CString& s )
{
	CPoint point;
	m_MyTip.GetPoint( point );

	if( Mouse( MOUSE_ZOOMBOX_DRAGGING ) || Mouse( MOUSE_MODEL_DRAGGING ) )  
		return FALSE;

	if( !wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC ) )
		return FALSE;

	CHitRecordCollection hitCollection;
	GetSelectionHitList( point, ini.size().node, ini.size().node, hitCollection );

	BOOL bSet = FALSE;
	const CNode* pNode = m_graphicsModel.OverNode( hitCollection );
	if( pNode && CurrentResultCase() )
	{
		s = CWindowTips::GetResultWindowTip( pNode, CurrentResultCase(), m_Filter );
		bSet = TRUE;
	}
	if( FALSE == bSet ) 
	{
		const CMember* pMember = m_graphicsModel.OverMember( hitCollection );
		if( pMember && CurrentResultCase() ) 
		{
			s = CWindowTips::GetResultWindowTip( pMember, CurrentResultCase(), m_Filter );
			bSet = TRUE;
		}
	}
	if( FALSE == bSet ) 
	{
		const CCable* pCable = m_graphicsModel.OverCable( hitCollection );
		if( pCable && CurrentResultCase() )
		{
			s = CWindowTips::GetResultWindowTip( pCable, CurrentResultCase(), m_Filter );
			bSet = TRUE;
		}		
	}
	if( FALSE == bSet ) 
	{
		const CPlanar* pPlate = m_graphicsModel.OverPlate( hitCollection );
		if( pPlate == NULL )
			pPlate =  m_graphicsModel.OverAutoPlate( hitCollection );
		if( pPlate && CurrentResultCase() )
		{
			s = CWindowTips::GetResultWindowTip( pPlate, CurrentResultCase(), m_Filter );
			bSet = TRUE;
		}	
	}
	if( FALSE == bSet ) 
	{
		const CSpring* pSpring = m_graphicsModel.OverSpring( hitCollection );
		if( pSpring && CurrentResultCase() )
		{
			s = CWindowTips::GetResultWindowTip( pSpring, CurrentResultCase(), m_Filter );
			bSet = TRUE;
		}
	}

	if( bSet && (s.GetLength() > 0) ) {
		VAHelpPane.showTempTip( s );
	}
	if( !m_Filter.flyoverData ) {
		s.Empty();
		return TRUE;
	}
	else return FALSE;
}

BOOL CGraphicView::FlyOverDesignWindow( CString& s )
{
	CPoint point;
	m_MyTip.GetPoint( point );

	if( Mouse( MOUSE_ZOOMBOX_DRAGGING ) || Mouse( MOUSE_MODEL_DRAGGING ) )  // this should not happen
		return FALSE;

	if( !wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC ) )
		return FALSE;

	CHitRecordCollection hitCollection;
	GetSelectionHitList( point, ini.size().node, ini.size().node, hitCollection );

	BOOL bSet = FALSE;
	bool bWarning = false;
	const CMember* pMember = m_graphicsModel.OverMember( hitCollection );
	if( pMember )
	{
		s = CWindowTips::GetDesignWindowTip( pMember, bWarning );
		bSet = TRUE;
	}

	if( FALSE == bSet )
	{
		// check the planar list
		const CPlanar* pPlate = m_graphicsModel.OverPlate( hitCollection );
		if( pPlate )
		{
			s = CWindowTips::GetDesignWindowTip( pPlate, bWarning );
			bSet = TRUE;
		}
	}
	if( bSet && (s.GetLength() > 0) ) {
		VAHelpPane.showTempTip( s, bWarning );
	}
	if( !m_Filter.flyoverData ) {
		s.Empty();
		return TRUE;
	}
	return FALSE;
}


