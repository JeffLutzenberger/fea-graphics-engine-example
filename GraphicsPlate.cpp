#include "StdAfx.h"
#include "GraphicsPlate.h"
#include "Node.h"

#include "Graphics/GraphicsHelpers.h"

//#include "Design/GroupManager.h"	// theGroupManager
#include "Design/Mesh.h"			// theMeshManager
#include "VisualDesign/VDInterface.h" // VDUnitySuccessLimit()

CGraphicsPlate::CGraphicsPlate(const CPlanar* pP):
m_pP(pP),
m_bHaveTexCoords( false ),
m_center( CPoint3D() )
{
}

CGraphicsPlate::~CGraphicsPlate(void)
{
}

void CGraphicsPlate::Draw( CDC* pDC, const CWindowFilter& filter, double scale, ETGraphicsDrawStyle style, double localAxisLength )
{
	ASSERT_RETURN( m_pP && pDC );

	//COLORREF c = ini.color().planarFill;
	//if( style == GRAPHIC_LINE || style == GRAPHIC_DASHED_LINE )
	//	c = InverseColor( ini.color().background );
	//else
	//{
	//	if( m_pP->isSelected() )
	//		c = InverseColor( ini.color().planarFill );
	//	else
	//	{
	//		c = ini.color().planarFill;
	//		if( filter.windowType == DESIGN_WINDOW )
	//		{
	//			float color[4] = { float(GetRValue( c )/255.), 
	//	               float(GetGValue( c )/255.),
	//				   float(GetBValue( c )/255.), 1.0f/*0.2f*/ };
	//			SetUnityValuesAndColors( color );
	//			ApplyAmbientGLMaterial( color );
	//		}
	//		else if( m_bHaveTexCoords )
	//		{
	//			c = RGB(255,255,255);
	//			ApplyAmbientGLMaterialiv( c );
	//		}
	//	}
	//}
	//	
	//ApplyAmbientGLMaterialiv( c );
	bool bDeformed = filter.windowType == POST_WINDOW;
	bool bShrink = filter.plate.shrink || m_pP->isSelected();

	bool bTriangle = (m_pP->node(4) == m_pP->node(1)) || (m_pP->node(4) == NULL);
	int nPts = bTriangle ? 3 : 4;
	CPoint3D points[4];
	for( int i = 0; i < nPts; i++ ) 
	{
		const CNode* pN = m_pP->node(i+1);
		CHECK_IF( pN ) { // TeK Added Error checking, crashed with a corrupted file...
			points[i] = *pN;
		}
	}

	const CResultCase* pRC = filter.resultCase();
	const CResult* cR = NULL;
	
	if( !pRC && filter.windowType == POST_WINDOW )
		return;

	if( pRC && filter.windowType == POST_WINDOW )
	{
		cR = pRC->result( *m_pP );
	}
	
	CPoint3D dispPoints[4];
	
	for( int i = 0; i < nPts; i++ ) {
		if( cR )
		{
			dispPoints[i].x = points[i].x + scale*cR->displacement( DX, (ETPlanarLocation)i+1 );
			dispPoints[i].y = points[i].y + scale*cR->displacement( DY, (ETPlanarLocation)i+1 );
			dispPoints[i].z = points[i].z + scale*cR->displacement( DZ, (ETPlanarLocation)i+1 );
		}
		if( bDeformed )
			points[i] = dispPoints[i];
	}
	
	CPoint3D center;
	CPoint3D unshrunkCenter;

	if( bTriangle )
	{
		center = points[0] + points[1] + points[2];
		center = center*0.333333;
		unshrunkCenter = center;

		if( bShrink )
		{
			center = center*0.2;
			points[0] = points[0]*0.8 + center;
			points[1] = points[1]*0.8 + center;
			points[2] = points[2]*0.8 + center;
		}
		double t = 0.;
		if( filter.drawDetail == DETAIL_HI && style != GRAPHIC_DASHED_LINE )
			t = m_pP->thickness();
		if( ini.graphics().blending && filter.windowType == MODEL_WINDOW && style == GRAPHIC_SOLID )
			EnableGLTransparency();
		if( m_bHaveTexCoords )
			DrawTexturedTriPlate( points, m_texCoords, t );
		else
			DrawTriPlate( points[0], points[1], points[2], t );
		if( ini.graphics().blending && filter.windowType == MODEL_WINDOW && style == GRAPHIC_SOLID )
			DisableGLTransparency( );
	}
	else
	{
		center = points[0] + points[1] + points[2] + points[3];
		center = center*0.25;
		unshrunkCenter = center;

		if( bShrink )
		{
			center = center*0.2;
			points[0] = points[0]*0.8 + center;
			points[1] = points[1]*0.8 + center;
			points[2] = points[2]*0.8 + center;
			points[3] = points[3]*0.8 + center;
		}
		double t = 0.;
		if( filter.drawDetail == DETAIL_HI && style != GRAPHIC_DASHED_LINE)
			t = m_pP->thickness();
		if( ini.graphics().blending && filter.windowType == MODEL_WINDOW && style == GRAPHIC_SOLID )
			EnableGLTransparency( );
		if( m_bHaveTexCoords )
			DrawTexturedQuadPlate( points, m_texCoords, t );
		else
			DrawQuadPlate( points[0], points[1], points[2], points[3], t );
		if( ini.graphics().blending && filter.windowType == MODEL_WINDOW && style == GRAPHIC_SOLID )
			DisableGLTransparency( );
	}
	if( filter.plate.localAxes )
	{
		//glDepthRange( 0, 0.9 );
		float lineWidth = 1;
		glGetFloatv(GL_LINE_WIDTH, &lineWidth );
		glLineWidth( (float)ini.size().axisWidth );
		glPushAttrib( GL_LIGHTING );
		double trans[3][3];
		m_pP->transformationMatrix( trans );
		transpose33( trans );
		CQuaternion q( trans );
		CVector3D axis;
		double angle;
		q.Disassemble( axis, angle );
		glPushMatrix();
		glTranslatef( (float)unshrunkCenter.x, (float)unshrunkCenter.y, (float)unshrunkCenter.z );
		glRotatef( float(angle*180/M_PI), float(axis.x), float(axis.y), float(axis.z) );
		glTranslatef( 0.f, 0.f, (float)m_pP->thickness() );
		//StartSolidDrawingNoLightingNoAA();
		// TeK Change 1/28/2008: Drawing full arrows on plate mesh is very crowded, changed to show
		// just the colored X,Y arrows without text.
		if( localAxisLength <= 0 )
			localAxisLength = (double)ini.size().planarAxisLength;
		DrawOriginArrows( float(localAxisLength), true, false, false );
		//EndSolidDrawingNoLightingNoAA();
		glPopMatrix();
		glPopAttrib();
		glLineWidth( lineWidth );
		//glDepthRange( 0, 1 );
	}

	//location of our text
	if( bTriangle )
	{
		center = points[0] + points[1] + points[2];
		m_center = center*0.333333;
	}
	else
	{
		center = points[0] + points[1] + points[2] + points[3];
		m_center = center*0.25;
	}

	if( filter.windowType == DESIGN_WINDOW )
	{
		glDisable( GL_POLYGON_STIPPLE );
		glDisable( GL_LINE_STIPPLE );
	}
}

void CGraphicsPlate::Draw( CDC* pDC, const CWindowFilter& filter, double scale, ETGraphicsDrawStyle style, CGLText* text, double localAxisLength )
{
	ASSERT_RETURN( m_pP && pDC );

	Draw( pDC, filter, scale, style, localAxisLength );

	//now draw our text...
	// loop over the CGraphicsMembers and add their text objects to the text manager
	bool bHasPlateText = false;
	if( MODEL_WINDOW == filter.windowType ) {
		bHasPlateText = ( filter.plate.name || filter.plate.properties || 
			filter.plate.material || filter.plate.area || filter.plate.weight );
	}
	else if( POST_WINDOW == filter.windowType ) {
		bHasPlateText = filter.plate.name;
	}
	else if( DESIGN_WINDOW == filter.windowType ) {
		bHasPlateText = (filter.unityValue );
	}
	if( !bHasPlateText )
		return;

	//we've got text...
	text->ClearText();
	if( filter.plate.name ) {    // label the member
		text->AddLine( m_pP->name() );
	}
	char buf1[256];
	strcpy( buf1, "" );
	char buf2[256];
	strcpy( buf2, "" );
	GetPlatePropertyBuffers( buf1, buf2, m_pP, filter );
	if( lstrlen( buf1 ) > 0 )  // display top buffer
		text->AddLine( buf1 );
	
	if( lstrlen( buf2 ) > 0 )   // display bottom buffer
		text->AddLine( buf2 );
	
	CPoint center2D;
	double trans[3][3];
	m_pP->transformationMatrix( trans );
	double angle = atan2( trans[0][1], trans[0][0] );
	//no need for m_center here, can just use m_pP->centroid()...
	if( GetScreenPtFrom3DPoint( m_center, center2D ) )
	{
		text->SetHAlignment( CGLOutlineText::HCENTER );
		text->DrawText2D( pDC, ini.font().graphWindow.name, 
				ini.font().graphWindow.size/*m_printScale*/, 
				ini.color().graphTitles, center2D, angle  ); 
		text->SetHAlignment( CGLOutlineText::HLEFT );
	}
}

void CGraphicsPlate::SetupTextureCoordinates(double min, double max, CWindowFilter& filter )
{
	ASSERT_RETURN( m_pP );
	
	const CResult* pR = filter.resultCase()->result( *m_pP );
	
	ASSERT_RETURN( pR );

	bool bTriangle = (m_pP->node(4) == m_pP->node(1)) || (m_pP->node(4) == NULL);
	int nPts = bTriangle ? 3 : 4;

	ETPlanarPlane plane = MID_PLANE;
	if( filter.plate.botSurface ) plane = BOTTOM_PLANE;
	else if( filter.plate.topSurface ) plane = TOP_PLANE;

	int GraphType = filter.plate.GetGraphType();
	bool isGlobal = filter.plate.isGlobal();
	bool isPrincipal = filter.plate.isPrincipal();

	double resultMagAtNode[4]; 

	for( int i = 0; i < 4; i++ ){
		resultMagAtNode[i] = 0.;
	}

	if( nPts < 3 ) return;
	if( GraphType == 13 ) 
	{  // deflections
		resultMagAtNode[0] = pR->displacement( NODE1 );
		resultMagAtNode[1] = pR->displacement( NODE2 );
		resultMagAtNode[2] = pR->displacement( NODE3 );
		if( nPts == 4 ) 
			resultMagAtNode[3] = pR->displacement( NODE4 );
	}
	else if( GraphType == 14 )
	{  // bearing pressures
		resultMagAtNode[0] = pR->bearingPressure( NODE1 );
		resultMagAtNode[1] = pR->bearingPressure( NODE2 );
		resultMagAtNode[2] = pR->bearingPressure( NODE3 );
		if( nPts == 4 ) 
			resultMagAtNode[3] = pR->bearingPressure( NODE4 );
	}
	else if( !isPrincipal ) 
	{
		if( GraphType <= TAU_XY || ( isGlobal && GraphType >= 9 ) ) 
		{
			int si = GraphType;
			if( si == 9 )  // special cases for GraphType
				si = SIGMA_Z;
			else if( si == 10 )
				si = TAU_XY;
			else if( si == 11 )
				si = TAU_YZ;
			else if( si == 12 )
				si = TAU_ZX;
			resultMagAtNode[0] = pR->stress( (ETPlanarStress)si, NODE1, plane, isGlobal );
			resultMagAtNode[1] = pR->stress( (ETPlanarStress)si, NODE2, plane, isGlobal );
			resultMagAtNode[2] = pR->stress( (ETPlanarStress)si, NODE3, plane, isGlobal );
			if( nPts == 4 ) 
				resultMagAtNode[3] = pR->stress( (ETPlanarStress)si, NODE4, plane, isGlobal );
		}
		else 
		{
			resultMagAtNode[0] = pR->force( (ETPlanarForce)GraphType, NODE1, isGlobal );
			resultMagAtNode[1] = pR->force( (ETPlanarForce)GraphType, NODE2, isGlobal );
			resultMagAtNode[2] = pR->force( (ETPlanarForce)GraphType, NODE3, isGlobal );
			if( nPts == 4 ) 
				resultMagAtNode[3] = pR->force( (ETPlanarForce)GraphType, NODE4, isGlobal );
		}
	}
	else 
	{  // principal stresses requested
		resultMagAtNode[0] = pR->principalStress( (ETPrincipalStress)GraphType, NODE1, plane );
		resultMagAtNode[1] = pR->principalStress( (ETPrincipalStress)GraphType, NODE2, plane );
		resultMagAtNode[2] = pR->principalStress( (ETPrincipalStress)GraphType, NODE3, plane );
		if( nPts == 4 ) 
			resultMagAtNode[3] = pR->principalStress( (ETPrincipalStress)GraphType, NODE4, plane );
	}
	if( nPts != 4 ) 
	{
		resultMagAtNode[3] = resultMagAtNode[0];
	}

	//normalize the texture coordinate and add it to the plates texture coordinates
	double normalizedMag = 0.;
	if( !zero(max - min) )
	{
		for( int i = 0; i < nPts; i++ ){
			normalizedMag = ( resultMagAtNode[i] - min )/( max - min );
			if( abs(max) < abs(min) )
				normalizedMag = abs(( resultMagAtNode[i] - max )/( min - max ));
			m_texCoords[i] = CPoint3D( normalizedMag, 0., 0. );
		}
	}
	m_bHaveTexCoords = true;
	return;
}

void CGraphicsPlate::SetUnityValuesAndColors( float color[4] )
{
	ASSERT_RETURN( m_pP );
	
	// unity check values are required - set the pen type and color accordingly
	int mesh = theMeshManager.isPlateMeshed( m_pP );
	if( mesh > 0 ) {
		CDesignMesh* pMesh = theMeshManager.mesh( mesh );
		ASSERT_RETURN( pMesh );
		glDisable( GL_POLYGON_STIPPLE );
		glDisable( GL_LINE_STIPPLE );
		double unityValue = -1;
		if( mesh > 0 ) {
			bool hasUnityMessage = false;
			if( pMesh && pMesh->areUnityChecksValid() ) {
				unityValue = pMesh->unityCheck( m_pP, &hasUnityMessage );
			}
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
					color[0] = 0.f;
					color[1] = float(unityValue);
					float v = float(VDUnitySuccessLimit() - unityValue);
					color[2] = max(v, 1.0f);
					color[3] = 1.f;
				}else{
					// failed
					color[0] = 1.f;
					color[1] = 0.f;
					color[2] = 0.f;
					color[3] = 1.f;
				}
			
			}
			else {
				// No Unity Checks? (black)
				color[0] = 0.f;
				color[1] = 0.f;
				color[2] = 0.f;
				color[3] = 0.f;
			}
		}
		else {  // not grouped - we use a dashed line
			glEnable( GL_POLYGON_STIPPLE );
			glPolygonStipple(stippleMask[1]);
			glEnable( GL_LINE_STIPPLE );
			glLineStipple( 4, 0xAAAA );
			color[0] = 0.f;
			color[1] = 0.f;
			color[2] = 1.f;
			color[3] = 1.f;
		}
	}
}

void CGraphicsPlate::GetPlatePropertyBuffers(char* buf1, char* buf2, const CPlanar* pP, const CWindowFilter& filter )
{
	ETShowUnit show = (filter.showUnits ? SHOW_UNIT : NO_UNIT);
	int fillBufNumber = 1;
	if( filter.plate.properties ) {
		char buf[132];
		sprintf( buf, "t=%s", Units::show( LENGTH_SMALL_UNIT, pP->thickness(), show) );
		addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
	}
	if( filter.plate.weight ) {
		char buf[132];
		sprintf( buf, "W=%s", Units::show( FORCE_UNIT, pP->weight(), show) );
		addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
	}
	if( filter.plate.material ) {
		addToBuffers( buf1, buf2, 256, &fillBufNumber,
			pP->material().list( CMaterial::REPORT_TYPE, 0, 0, 0, NO_UNIT ) );
	}
	if( filter.plate.area ) {
		char buf[132];
		sprintf( buf, "A=%s", Units::show( AREA_UNIT, pP->area(), show) );
		addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
	}

	// unity values
	if( DESIGN_WINDOW == filter.windowType && filter.unityValue ) {
		// unity check values are required - set the pen type and color accordingly
		int mesh = theMeshManager.isPlateMeshed( pP );
		if( mesh > 0 ) {
			CDesignMesh* pMesh = theMeshManager.mesh( mesh );
			double unityValue = -1;
			bool hasUnityMessage = false;
			// TeK Add 4/2008: are unity checks valid?
			if( pMesh  && pMesh->areUnityChecksValid() )
				unityValue = pMesh->unityCheck( pP, &hasUnityMessage );
			if( hasUnityMessage && 
				pMesh && (pMesh->unityCheckMessageID( pP ) < 0) ) {
					unityValue = 1.05;  // force red pen
			}

			// TeK Added 12/11/2007: Missing VA5.5 feature, added per customer request
			bool bShowAsWorst = true;
			if( filter.worstUnityValueOnly ) {
				if( !equal( unityValue, pMesh->worstUnityCheck()) ) {
					bShowAsWorst = false;
				}
			}

			if( pMesh && bShowAsWorst ) {
				// TeK Add 8/13/97: Mark unity values as APPROXIMATE if the design
				// has changed (we haven't synchronized it so analysis results
				// are for different member stiffnesses)
				bool negativeMessage = false;
				if( hasUnityMessage ) {
					if( pMesh->unityCheckMessageID( pP ) < 0 )	negativeMessage = true;
				}
				if( negativeMessage ) {
					sprintf( buf1, "N.A." );
				}
				else if( theMeshManager.hasDesignChanged() ) {
					sprintf( buf1, "~%s%s", (const char*)nice(unityValue), (hasUnityMessage? "!" : "") );
				}
				else sprintf( buf1, "%s%s", (const char*)nice(unityValue), (hasUnityMessage? "!" : "") );
			}
		}
	}

	// design groups
	if( (DESIGN_WINDOW == filter.windowType) && filter.plate.groupName ) {
		int mesh = theMeshManager.isPlateMeshed( pP );
		if( mesh > 0 ) {
			CDesignMesh* pMesh = theMeshManager.mesh( mesh );
			strcpy( buf1, pMesh->name() );
		}
	}

	// handle display of design section if necessary
	if( filter.designSize && (DESIGN_WINDOW == filter.windowType) ) {
		int mesh = theMeshManager.isPlateMeshed( pP );
		if( mesh > 0 ) {
			CDesignMesh* pMesh = theMeshManager.mesh( mesh );
			if( strlen( buf2 ) > strlen( buf1 ) ) {
				strcat( buf1, " " );
				strcat( buf1, Units::show( LENGTH_SMALL_UNIT, pMesh->thickness(), show ) );
			}
			else {
				strcat( buf2, " " );
				strcat( buf2,Units::show( LENGTH_SMALL_UNIT, pMesh->thickness(), show ) );
			}
		}
	}

	return;

}

void CGraphicsPlate::addToBuffers( char* buf1, char* buf2, int size, int* fillNumber, const char* str ) 
{
   // toggle back and forth filling the two buffers with "str"
   if( *fillNumber == 1 ) {  // try to fill buffer 1
   	if( lstrlen( buf1 ) + lstrlen( str ) + 2 <= size ) {  // can do!
      	strcat( buf1, " " );	// put in a space
         strcat( buf1, str );
         *fillNumber = 2;
         return;
      	}
      else {  // not enough room in 1 try 2
      	if( lstrlen( buf2 ) + lstrlen( str ) + 2 <= size ) {  // can do!
         	strcat( buf2, " " );
            strcat( buf2, str );  // note we don't switch the swap number
            return;
         	}
      	}
   	}
   else { // try to fill buffer 2
   	if( lstrlen( buf2 ) + lstrlen( str ) + 2 <= size ) {  // can do!
      	strcat( buf2, " " );	// put in a space
         strcat( buf2, str );
         *fillNumber = 1;
         return;
      	}
      else {  // not enough room in 2 try 1
      	if( lstrlen( buf1 ) + lstrlen( str ) + 2 <= size ) {  // can do!
         	strcat( buf1, " " );
            strcat( buf1, str );  // note we don't switch the swap number
            return;
         	}
      	}
   	}
   return;
}

