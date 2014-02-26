#include "IESDataStdAfx.h"
#include "Core/Graphics/GraphicsHelpers.h"
#include "GraphicsMember.h"
#include "ShapeDB/CrossSection.h"
#include "SpineGenerator.h"
#include "iniGraphics.h"
#include "IniColor.h"
#include "IniFont.h"
#include "Node.h"
#include "Model.h"

//required for text drawing
#include "Data/Design/DesignUtils.h"
#include "Data/Design/GroupManager.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

void getMemberExtremeLabel( const CResult* pResult, int theGraphType, CString& extremeLabel );

CGraphicsMember::CGraphicsMember( const CMember* pM, const CResultCase* pRC, double scale, bool bSetupExtrusion ) :
m_pM( pM ),
m_pRC( pRC ),
m_HiLOD( NULL ),
m_MedLOD( NULL ),
m_scale( scale ),
m_Start( CPoint3D( 0., 0., 0. ) ),
m_RotationAxis( CVector3D( 0.0, 0.0, 1.0 ) ),
m_RotationAngle( 0. ),
m_bHaveExtrusions( false ),
m_loadDir( DY )
{
	ASSERT( pM );
	SetupSpine();
	if( bSetupExtrusion )
		SetupHiQualityExtrusion( );
	// save the time stamp of this object so we know when to redo it
	m_timeCreated = pM->UITime();
	// create the extruded shape
}

CGraphicsMember::CGraphicsMember( const CMember* pM, CGraphicsMember* pSimilarGraphicsMember ) :
m_pM( pM ),
m_pRC( NULL ),
m_HiLOD( NULL ),
m_MedLOD( NULL ),
m_scale( 0. ),
m_Start( CPoint3D( 0., 0., 0. ) ),
m_RotationAxis( CVector3D( 0.0, 0.0, 1.0 ) ),
m_RotationAngle( 0. ),
m_bHaveExtrusions( false ),
m_loadDir( DY )
{
	ASSERT( pM );
	SetupSpine();
	// in case we get here but the similar member does not have an extrusion.
	// this will happen when the user is in low quality mode
	if( pSimilarGraphicsMember->HiLOD() )
		m_HiLOD = new CMemberExtrusion( pM, pSimilarGraphicsMember->HiLOD() );
	// save the time stamp of this object so we know when to redo it
	m_timeCreated = pM->UITime();
}

CGraphicsMember::~CGraphicsMember()
{
	if( m_HiLOD )
		delete m_HiLOD;
	if( m_MedLOD )
		delete m_MedLOD;
}

CGraphicsMember & CGraphicsMember:: operator = ( const CGraphicsMember &m )
{
	m_pM = m.m_pM;
	m_pRC = m.m_pRC;
	m_HiLOD = m.m_HiLOD;
	m_MedLOD = m.m_MedLOD;
	m_spine = m.m_spine;
	m_spineTexCoords = m.m_spineTexCoords;
	m_Start = m.m_Start;
	m_RotationAxis = m.m_RotationAxis;
	m_RotationAngle = m.m_RotationAngle;
	m_scale = m.m_scale;
	m_bHaveExtrusions = m.m_bHaveExtrusions;
	m_timeCreated = m.m_timeCreated;
	for( int i = 0; i < 16; i++ )
		m_RotationMatrix[i] = m.m_RotationMatrix[i];
	m_loadDir = m.m_loadDir;
	return *this;
}
void CGraphicsMember::Draw(const CWindowFilter& rFilter, int nDivPts )
{
	ASSERT_RETURN( m_pM );

	if( m_pM->length() <= 0. )
		return;

	double memberAxisLength = ini.graphics().fShortLength;
	double nodePullBack = ini.graphics().fWideOffset;
	double endReleaseLength = ini.graphics().fWideOffset; // scales better than fShortLength
	if( rFilter.pictureView ) {
		// but this works better in picture view mode
		endReleaseLength = ini.graphics().fShortLength; 
	}

	//pull back
	float node_pull_back_scale2 = float(1.-2*nodePullBack/m_pM->length());
	bool bShrinkMember = rFilter.member.shrink && rFilter.windowType == MODEL_WINDOW;
	float pull_back = min( float(2.*ini.graphics().fWideWidth), float( 0.1*m_pM->length() ) );
	float pull_back_scale = float(1.-pull_back/m_pM->length());
	float pull_back_scale2 = float(1.-2*pull_back/m_pM->length());

	float local_axes_offsety = float(ini.graphics().fNarrowWidth);

	glPushMatrix();
		glTranslatef( float(m_Start.x), float(m_Start.y), float(m_Start.z) );
		glMultMatrixf( m_RotationMatrix );
		DrawDivPts( nDivPts );

		//////////////////////////////////////////////////
		// extruded member - includes results if available
		//////////////////////////////////////////////////
		if( !rFilter.pictureView || m_pM->isARigidLink() )
		{
			glTranslatef( (float)nodePullBack, 0.0f, 0.0f );
			glScalef( node_pull_back_scale2, 1.0, 1.0 );
			DrawMemberLow( );
		}
		else
		{
			glPushMatrix();
			if( m_pM->node(1)->isSelected() || m_pM->node(2)->isSelected() || bShrinkMember ) //we only do setback for undisplaced members
			{
				if( (m_pM->node(1)->isSelected() && m_pM->node(2)->isSelected()) || bShrinkMember )
				{
					glTranslatef( pull_back, 0.0f, 0.0f );
					glScalef( pull_back_scale2, 1.0, 1.0 );
				}
				else if( m_pM->node(1)->isSelected() )
				{
					glTranslatef( pull_back, 0.0f, 0.0f );
					glScalef( pull_back_scale, 1.0, 1.0 );
				}
				else if( m_pM->node(2)->isSelected() )
				{
					glScalef( pull_back_scale, 1.0, 1.0 );
				}
			}
			DrawMemberHi( );
			if( m_HiLOD )
				local_axes_offsety = (float)m_HiLOD->GetYMax();
			glPopMatrix();
			if( m_pM->node(1)->isSelected() || m_pM->node(2)->isSelected() || bShrinkMember )
			{
				glTranslatef( (float)nodePullBack, 0.0f, 0.0f );
				glScalef( node_pull_back_scale2, 1.0, 1.0 );
				DrawMemberLow( );
			}
		}
		
		/////////////////////////////////////////////////
		// end zones
		/////////////////////////////////////////////////
		if( rFilter.member.endZone && rFilter.windowType == MODEL_WINDOW)
		{
			glPushMatrix();
			if( m_pM->hasEndZone(1) )
			{
				//Draw left end zone
				if( m_pM->node(1)->isSelected() )
				{
					glPushMatrix();
					glTranslatef( float(m_pM->length()*.1), 0.f, 0.f );
				}
				float endzone_offset = (float)(m_pM->endZoneDistance( 1 ));
				//now draw a cube that encompasses the endzone of the member
				DrawBox( float(endReleaseLength), float(endReleaseLength), endzone_offset );
				if( m_pM->node(1)->isSelected() )
					glPopMatrix();
			}
			if( m_pM->hasEndZone(2) )
			{
				
				float endzone_offset = (float)(m_pM->endZoneDistance( 2 ));
				glTranslatef( float( m_pM->length()-endzone_offset), 0.f, 0.f );
				if( m_pM->node(2)->isSelected() )
				{
					glPushMatrix();
					glTranslatef( -float(m_pM->length()*.1), 0.f, 0.f );
				}
				//Draw right end zone
				DrawBox( float(endReleaseLength), float(endReleaseLength), endzone_offset );
				if( m_pM->node(2)->isSelected() )
					glPopMatrix();
			}
			glPopMatrix();
		}

		/////////////////////////////////////////////////
		// end releases
		/////////////////////////////////////////////////
		if( rFilter.member.releases && rFilter.windowType == MODEL_WINDOW )
		{
			COLORREF c = ini.color().releases;
			float color[4] = {float(GetRValue(c))/255.f, float(GetGValue(c))/255.f, float(GetBValue(c))/255.f, ALPHA};
			ApplyAmbientGLMaterial( color );
			//start with node 1
			if( !m_pM->isReleased( 1, NO_DIRECTION ) )
			{
				float r = float( endReleaseLength );//float(ini.graphics().fShortLength);
				glPushMatrix();
				glTranslatef( float(m_pM->length()*.1), 0.f, 0.f );
				if( m_pM->isReleased( 1, RZ ) )
				{
					DrawCircle( r, float(ini.graphics().fNarrowWidth) );
				}
				if( m_pM->isReleased( 1, RY ) )
				{
					glPushMatrix();
					glRotatef( 90.f, 1.f, 0.f, 0.f );
					DrawCircle( r, float(ini.graphics().fNarrowWidth) );
					glPopMatrix();
				}
				if( m_pM->isReleased( 1, RX ) )
				{
					glPushMatrix();
					glRotatef( 90.f, 0.f, 1.f, 0.f );
					DrawCircle( r, float(ini.graphics().fNarrowWidth) );
					glPopMatrix();
				}
				if( m_pM->isReleased( 1, DX ) )
				{
					DrawElongatedDiamond( float(endReleaseLength), 
						     float(endReleaseLength), 
							 float(endReleaseLength), true );
				}
				if( m_pM->isReleased( 1, DY ) )
				{
					glPushMatrix();
					glRotatef( 90.f, 0.f, 0.f, 1.f );
					DrawElongatedDiamond( float(endReleaseLength), 
						float(endReleaseLength), 
						float(endReleaseLength), true );
					glPopMatrix();
				}
				if( m_pM->isReleased( 1, DZ ) )
				{
					glPushMatrix();
					glRotatef( 90.f, 0.f, 1.f, 0.f );
					DrawElongatedDiamond( float(endReleaseLength), 
						float(endReleaseLength), 
						float(endReleaseLength), true );
					glPopMatrix();
				}		
				glPopMatrix();
			}
			//now node 2
			if( !m_pM->isReleased( 2, NO_DIRECTION ) )
			{
				float r = float( endReleaseLength );//float(ini.graphics().fShortLength);
				glPushMatrix();
				
				glTranslatef( float( m_pM->length()*.9), 0.f, 0.f );
				if( m_pM->isReleased( 2, RZ ) )
				{
					DrawCircle( r, float(ini.graphics().fNarrowWidth) );
				}
				if( m_pM->isReleased( 2, RY ) )
				{
					glPushMatrix();
					glRotatef( 90.f, 1.f, 0.f, 0.f );
					DrawCircle( r, float(ini.graphics().fNarrowWidth) );
					glPopMatrix();
				}
				if( m_pM->isReleased( 2, RX ) )
				{
					glPushMatrix();
					glRotatef( 90.f, 0.f, 1.f, 0.f );
					DrawCircle( r, float(ini.graphics().fNarrowWidth) );
					glPopMatrix();
				}
				if( m_pM->isReleased( 2, DX ) )
				{
					DrawElongatedDiamond( float(endReleaseLength), 
						float(endReleaseLength), 
						float(endReleaseLength), true );
				}
				if( m_pM->isReleased( 2, DY ) )
				{
					glPushMatrix();
					glRotatef( 90.f, 0.f, 0.f, 1.f );
					DrawElongatedDiamond( float(endReleaseLength), 
						float(endReleaseLength), 
						float(endReleaseLength), true );
					glPopMatrix();
				}
				if( m_pM->isReleased( 2, DZ ) )
				{
					glPushMatrix();
					glRotatef( 90.f, 0.f, 1.f, 0.f );
					DrawElongatedDiamond( float(endReleaseLength), 
						float(endReleaseLength), 
						float(endReleaseLength), true );
					glPopMatrix();
				}		
				glPopMatrix();
			}
		}

		/////////////////////////////////////////////////
		// local axes
		/////////////////////////////////////////////////
		if( rFilter.member.localAxes ){
			//glDepthRange( 0, gAxesLineDisplayDepth );
			float lineWidth = 0;
			glGetFloatv(GL_LINE_WIDTH, &lineWidth );
			glLineWidth( ini.graphics().fNarrowWidth );
			glPushAttrib( GL_LIGHTING );
			glPushMatrix();
			if( m_HiLOD )
				local_axes_offsety *= -1;
			glTranslatef( float(m_pM->length()*0.5), (float)(1.25*local_axes_offsety), 0.f);
			StartSolidDrawing( 0 );
			DisableGLLighting();
			if( memberAxisLength <= 0 )
				memberAxisLength = double(ini.graphics().fShortLength);
			DrawOriginArrows( (float)memberAxisLength );
			glPopMatrix();
			glPopAttrib();
			glLineWidth( lineWidth );
			//glDepthRange( 0, 1 );
		}
	glPopMatrix();
}

void CGraphicsMember::DrawMemberText( CDC* pDC, CWindowFilter& rFilter, CGLText* text, double scale )
{
	ASSERT_RETURN( m_pM );
	if( !pDC )
		return;

	if( m_pM->length() <= 0. )
		return;

	char buf1[512] = "";
	char buf2[512] = "";
	GetMemberPropertyBuffers( buf1, buf2, rFilter );	
	if( lstrlen( buf1 ) < 0 &&  lstrlen( buf2 ) < 0 )
		return;

	//we've got text...
	ETFrameMember frame_type = getFrameMemberType( *m_pM );
	const CNode* pN1 = m_pM->node(1);
	const CNode* pN2 = m_pM->node(2);
	
	ASSERT_RETURN( pN1 && pN2 );

	//int nPoints = m_spine.size();
	//int texPts = m_spineTexCoords.size();
	//CPoint3D pt3D1 = m_spine[0];
	//CPoint3D pt3D2 = m_spine[nPoints-1];
	CPoint3D pt3D1( pN1->x(), pN1->y(), pN1->z());
	CPoint3D pt3D2( pN2->x(), pN2->y(), pN2->z());

	/*const CResultCase* pRC = rFilter.resultCase();
	if( pRC && rFilter.windowType == POST_WINDOW )
	{
		const CResult* cNR = pRC->result( *pN1 );
		CHECK_IF( cNR != NULL ){
			pt3D1.x += scale*cNR->displacement( DX );
			pt3D1.y += scale*cNR->displacement( DY );
			pt3D1.z += scale*cNR->displacement( DZ );
		}
		cNR = pRC->result( *pN2 );
		CHECK_IF( cNR != NULL ){
			pt3D2.x += scale*cNR->displacement( DX );
			pt3D2.y += scale*cNR->displacement( DY );
			pt3D2.z += scale*cNR->displacement( DZ );
		}
	}*/

	CLine3D l( pt3D1, pt3D2 );
	CPoint3D center = (pt3D1 + pt3D2)*0.5;
	if( frame_type >Z_ROOF_FM )
		center = l.offset( 0.33 );
	
	//if( lstrlen( buf2 ) > 0 ) {  // display bottom buffer
	//	text->AddLine( buf2 );
	//}

	CPoint p1;
	CPoint p2;
	CPoint centerPt;
	if( (lstrlen( buf1 ) > 0 || lstrlen( buf2 ) > 0) &&
		GetScreenPtFrom3DPoint( pt3D1, p1 ) &&
		GetScreenPtFrom3DPoint( pt3D2, p2 ) &&
		GetScreenPtFrom3DPoint( center, centerPt ) )
	{
		COLORREF text_color = InverseColor( ini.color().background );
		double angle = atan2( (float)(p1.y - p2.y), (float)(p2.x - p1.x) );
		text->SetHAlignment( CGLText::HCENTER );
		//top buffer
		if( lstrlen( buf1 ) > 0 )
		{
			text->ClearText();
			if( rFilter.pictureView )
			{
				//offset buffer 1 up away from the member
				float ymax = fabs((float)(m_pM->mpGraphicsMember->GetYMax()));
				if( zero(ymax) ) 
					ymax = 1.;
				if( scale <= 0 )
					scale = 1;
				float pixelYMax = ymax/(float)scale;
				//now offset p1 and p2
				CVector2D v( CPoint2D( p2.x, p2.y), CPoint2D( p1.x, p1.y ) );
				v = v.GetNormalized();
				v = v*pixelYMax;
				v = v.GetRotated( M_PI/2 );
				centerPt.x += (int)v.x;
				centerPt.y += (int)v.y;
			}		
			text->AddLine( buf1 );
			text->SetVAlignment( CGLOutlineText::VBOTTOM );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
				ini.font().graphWindow.size/*m_printScale*/, 
				text_color, centerPt, angle  );
		}

		//bottom buffer
		if( lstrlen( buf2 ) > 0 )
		{
			text->ClearText();
			if( rFilter.pictureView )
			{
				//offset buffer 1 up away from the member
				float ymax = fabs((float)(m_pM->mpGraphicsMember->GetYMax()));
				if( zero(ymax) ) 
					ymax = 1.;
				if( scale <= 0 )
					scale = 1;
				float pixelYMax = ymax/(float)scale;
				//now offset p1 and p2
				CVector2D v( CPoint2D( p2.x, p2.y), CPoint2D( p1.x, p1.y ) );
				v = v.GetNormalized();
				v = v*pixelYMax;
				v = v.GetRotated( M_PI/2 );
				centerPt.x -= 2*(int)v.x;
				centerPt.y -= 2*(int)v.y;
			}		
			text->AddLine( buf2 );
			text->SetVAlignment( CGLOutlineText::VTOP );
			text->DrawText2D( pDC, ini.font().graphWindow.name, 
				ini.font().graphWindow.size/*m_printScale*/, 
				text_color, centerPt, angle  );
		}
		text->ClearText();
		text->SetHAlignment( CGLText::HLEFT );
		text->SetVAlignment( CGLText::VTOP );
	}
}
void CGraphicsMember::Draw(CDC* pDC, CWindowFilter& rFilter, int nDivPts, CGLText* text, double scale )
{
	ASSERT_RETURN( m_pM );
	Draw( rFilter, nDivPts );
	DrawMemberText( pDC, rFilter, text, scale );
}

void CGraphicsMember::DrawSelectionBox()
{
	glPushMatrix();
	glTranslatef( float(m_Start.x), float(m_Start.y), float(m_Start.z) );
	//JL Fix - 5/6/2008
	//don't include the beta angle!
	ASSERT_RETURN( m_pM->length() > 0. );
	// get global member orientation matrix
	double Xdir[3] = {1.0,0.0,0.0};
	m_pM->localToGlobal( Xdir, false );
	double Ydir[3] = {0.0,1.0,0.0};
	m_pM->localToGlobal( Ydir, false );
	double Zdir[3] = {0.0, 0.0, 1.0};
	m_pM->localToGlobal( Zdir, false );

	//member orientation matrix
	float rotationMatrix[] = 
	{
		(float)Xdir[0],
		(float)Xdir[1],
		(float)Xdir[2],
		0.f,
		(float)Ydir[0],
		(float)Ydir[1],
		(float)Ydir[2],
		0.f,
		(float)Zdir[0],
		(float)Zdir[1],
		(float)Zdir[2],
		0.f,
		0.f,
		0.f,
		0.f,
		1.f
	};
	glMultMatrixf( rotationMatrix );
	int nPoints = m_spine.size();
	if( nPoints <= 0 ) return;
	float depth = 1;
	if( m_MedLOD ) depth = float(m_MedLOD->GetYMax());
	CPoint3D p1 = m_spine[0] - CPoint3D( 0.f, depth, 0.f );
	CPoint3D p2 = m_spine[0] + CPoint3D( 0.f, depth, 0.f );
	CPoint3D p3 = m_spine[0] + m_spine[nPoints-1] + CPoint3D( 0.f, depth, 0.f );
	CPoint3D p4 = m_spine[0] + m_spine[nPoints-1] - CPoint3D( 0.f, depth, 0.f );

	glBegin( GL_QUADS );
	glVertex3f( (float)p1.x, (float)p1.y, (float)p1.z );
	glVertex3f( (float)p2.x, (float)p2.y, (float)p2.z );
	glVertex3f( (float)p3.x, (float)p3.y, (float)p3.z );
	glVertex3f( (float)p4.x, (float)p4.y, (float)p4.z );
	glEnd();
	glPopMatrix();
}

void CGraphicsMember::DrawDivPts( int nDivPts )
{
	int nPoints = m_spine.size();
	double  len = abs( m_spine[0].distance_to( m_spine[ nPoints-1 ] ) );
	CVector3D v( m_spine[ nPoints-1 ], m_spine[0] );
	v.normalize();
	double delta = len/(nDivPts + 1);
	glBegin( GL_POINTS );
	for( int i = 0; i < nDivPts; i++ )
	{	
		CVector3D v1 = CVector3D(v.x*(delta*(i+1)), v.y*(delta*(i+1)), v.z*(delta*(i+1)));
		CPoint3D p2( v1.x, v1.y, v1.z );
		CPoint3D p3 = m_spine[0] + p2; 
		glVertex3f( (float)(p3.x), (float)(p3.y), (float)(p3.z) );
	}
	glEnd();
}

void CGraphicsMember::DrawResultDiagram( CWindowFilter& rFilter )
{
	if( !m_pM || m_pM->length() <= 0. )
		return;
	glPushMatrix();
	glTranslatef( float(m_Start.x), float(m_Start.y), float(m_Start.z) );

	//JL Fix - 5/6/2008
	//don't include the beta angle!
	ASSERT_RETURN( m_pM->length() > 0. );
	// get global member orientation matrix
	double Xdir[3] = {1.0,0.0,0.0};
	m_pM->localToGlobal( Xdir, false );
	double Ydir[3] = {0.0,1.0,0.0};
	m_pM->localToGlobal( Ydir, false );
	double Zdir[3] = {0.0, 0.0, 1.0};
	m_pM->localToGlobal( Zdir, false );

	//member orientation matrix
	float rotationMatrix[] = 
	{
		(float)Xdir[0],
		(float)Xdir[1],
		(float)Xdir[2],
		0.f,
		(float)Ydir[0],
		(float)Ydir[1],
		(float)Ydir[2],
		0.f,
		(float)Zdir[0],
		(float)Zdir[1],
		(float)Zdir[2],
		0.f,
		0.f,
		0.f,
		0.f,
		1.f
	};
	glMultMatrixf( rotationMatrix );

	//glMultMatrixf( m_RotationMatrix );

	//if( m_loadDir == DZ || m_loadDir == RZ )
	if( rFilter.member.resultType == MEMBER_VZ ||
		rFilter.member.resultType == MEMBER_MY ||
		rFilter.member.resultType == MEMBER_DZ )
		glRotatef( 90.f, 1.f, 0.f, 0.f );
	int nPoints = m_spine.size();
	std::vector<double> texCoords;
	int texPts = m_spineTexCoords.size();

	glBegin( GL_QUADS );
		for( int i = 0; i < nPoints-1; i++ )
		{
			if( i < texPts ){
				CPoint3D p1 = m_spine[i];
				CPoint3D p2 = m_spine[i];
				CPoint3D p3 = m_spine[i+1];
				CPoint3D p4 = m_spine[i+1];
				p1.y += (float)(ini.graphics().fLongLength*m_spineTexCoords[i]);
				p4.y += (float)(ini.graphics().fLongLength*m_spineTexCoords[i+1]);
				glVertex3f( (float)p1.x, (float)p1.y, (float)p1.z );
				glVertex3f( (float)p2.x, (float)p2.y, (float)p2.z );
				glVertex3f( (float)p3.x, (float)p3.y, (float)p3.z );
				glVertex3f( (float)p4.x, (float)p4.y, (float)p4.z );
			}
		}
	glEnd( );
	glPopMatrix( );
}

void CGraphicsMember::DrawMemberLow( )
{
//#pragma message(JL "Jeff - drawing a rigid link here?")
//	if( m_pM->isARigidLink() ) {
//		;
//	}
	glBegin( GL_LINES );
	int nPoints = m_spine.size();
	int texPts = m_spineTexCoords.size();
	for( int i = 0; i < nPoints-1; i++ )
	{
		CPoint3D p1 = m_spine[i];
		CPoint3D p2 = m_spine[i+1];
		if( i < texPts )
			glTexCoord2f( (float)m_spineTexCoords[i], 0.f );
		glVertex3f( (float)p1.x, (float)p1.y, (float)p1.z );
		if( i < texPts )
			glTexCoord2f( (float)m_spineTexCoords[i+1], 0.f );
		glVertex3f( (float)p2.x, (float)p2.y, (float)p2.z );
	}
	glEnd( );
}

void CGraphicsMember::DrawMemberMed( )
{
	if( m_bHaveExtrusions && m_MedLOD )
		m_MedLOD->Draw();
	else
		DrawMemberLow();
	
}

void CGraphicsMember::DrawMemberHi( )
{
	if( m_HiLOD )
		m_HiLOD->Draw();
	else{
		SetupHiQualityExtrusion( );
		if( m_HiLOD ) m_HiLOD->Draw();
		else DrawMemberLow();
	}
}

bool CGraphicsMember::SetupExtrusions( )
{
	if( m_bHaveExtrusions )
		m_bHaveExtrusions = false;
	if( m_HiLOD ){
		delete m_HiLOD;
		m_HiLOD = NULL;
	}
	if( m_MedLOD ){
		delete m_MedLOD;
		m_MedLOD = NULL;
	}
	bool bHiSuccess = false;
	bool bMedSuccess = false;
	bHiSuccess = SetupHiQualityExtrusion( );
	bMedSuccess = SetupMedQualityExtrusion( );
	if( bHiSuccess && bMedSuccess )
		m_bHaveExtrusions = true;
	return m_bHaveExtrusions;
}

bool CGraphicsMember::SetupHiQualityExtrusion( )
{
	ASSERT( m_pM && m_spine.size() > 0 );
	if( !m_HiLOD )
		m_HiLOD = new CMemberExtrusion( m_pM, m_spine, 0./*mFilter.member.setback*/, true );
	if( m_HiLOD )
		return true;
	else return false;
}
bool CGraphicsMember::SetupMedQualityExtrusion( )
{
	ASSERT( m_pM && m_spine.size() > 0 );
	m_MedLOD = new CMemberExtrusion( m_pM, m_spine, 0./*mFilter.member.setback*/, true );
	if( m_MedLOD )
		return true;
	else return false;
}

void CGraphicsMember::SetupSpine( )
{
	ASSERT_RETURN( m_pM );
	CNode* pN1;
	pN1 = (CNode*)m_pM->node(1);
	if( !pN1 )
		return;

	CNode* pN2;
  	pN2 = (CNode*)m_pM->node(2);
	if( !pN2 )
		return;

	bool deformed = false;
	int nPoints = 1; 
	if( m_pRC )
	{
		deformed = true;
		// TeK Change 3/26/2008: For performance & quality, vary based on the model!
		//nPoints = 8;
		int nMembers = theModel.elements(MEMBER_ELEMENT);
		if( nMembers < 50 ) {
			nPoints = 25;
		}
		else if( nMembers < 300 ) {
			nPoints = 11;
		}
		else nPoints = 5;
	}
	//get the local spine
	m_spine.clear( );
	CSpineGenerator sg( m_pM, m_pRC );
	if( !sg.GetSpine( deformed, nPoints, 0., m_scale, m_spine ) )
	{
		ASSERT( false );
		return;
	}

	m_Start.x = (float)pN1->x();
	m_Start.y = (float)pN1->y();
	m_Start.z = (float)pN1->z();

	ASSERT_RETURN( m_pM->length() > 0. );
	// get global member orientation matrix
	double Xdir[3] = {1.0,0.0,0.0};
	m_pM->localToGlobal( Xdir, true );
	double Ydir[3] = {0.0,1.0,0.0};
	m_pM->localToGlobal( Ydir, true );
	double Zdir[3] = {0.0, 0.0, 1.0};
	m_pM->localToGlobal( Zdir, true );

	//member orientation matrix
	double m[4][4] = {
		{Xdir[0], Xdir[1], Xdir[2],	0.0f},
		{Ydir[0], Ydir[1], Ydir[2],	0.0f},
		{Zdir[0], Zdir[1], Zdir[2],	0.0f},
		{0.0f,      0.0f,     0.0f,	1.0f}};
	m_RotationMatrix[0] = (float)Xdir[0];
	m_RotationMatrix[1] = (float)Xdir[1];
	m_RotationMatrix[2] = (float)Xdir[2];
	m_RotationMatrix[3] = 0.f;
	m_RotationMatrix[4] = (float)Ydir[0];
	m_RotationMatrix[5] = (float)Ydir[1];
	m_RotationMatrix[6] = (float)Ydir[2];
	m_RotationMatrix[7] = 0.f;
	m_RotationMatrix[8] = (float)Zdir[0];
	m_RotationMatrix[9] = (float)Zdir[1];
	m_RotationMatrix[10] = (float)Zdir[2];
	m_RotationMatrix[11] = 0.f;
	m_RotationMatrix[12] = 0.f;
	m_RotationMatrix[13] = 0.f;
	m_RotationMatrix[14] = 0.f;
	m_RotationMatrix[15] = 1.f;
	
	CQuaternion q( m );
	q.Disassemble( m_RotationAxis, m_RotationAngle );

}

double CGraphicsMember::distortionFactor()
{
	return m_scale;
}

void CGraphicsMember::SetupTextureCoords( double min, double max, int theGraphType, const CResult* pR, bool bDiagram )
{
	SetSpineTextureCoords( min, max, theGraphType, pR, bDiagram );
	if( m_HiLOD )
		m_HiLOD->SetExtrusionTextureCoords( min, max, theGraphType, pR, bDiagram );
	if( m_MedLOD )
		m_MedLOD->SetExtrusionTextureCoords( min, max, theGraphType, pR, bDiagram );
}

void CGraphicsMember::SetSpineTextureCoords( double min, double max, int GraphType, const CResult* pR, bool bDiagram )
{
	ASSERT_RETURN( pR );
	int nCrossSections = m_spine.size();
	m_spineTexCoords.clear();
	
	int memberSections = pR->locations();
	double maxValue = 0.0; //max value for this extrusion, we use this to plot the max value of cables 
	for( int i = 0; i < nCrossSections; i++ ) 
	{ 
		int iPoint1 = 0;
		int iPoint2 = 0;
		double top = 0;
		double bot = 0;
		// First take care of end points (easy case!)
		if( i == 0 ){
			iPoint1 = 0;
			iPoint2 = 0;
		}
		else if( i == nCrossSections ){
			iPoint1 = memberSections;
			iPoint2 = memberSections;
		}
		else
		{ //interpolate
			double ratio = (double)i/(double)(nCrossSections-1);
			iPoint1 = (int)(ratio * (memberSections-1));
			iPoint2 = iPoint1 + 1;
			double ratio1 = (double)(iPoint1)/(double)(memberSections-1);
			double ratio2 = (double)(iPoint2)/(double)(memberSections-1);
			top = ratio - ratio1;
			bot = ratio2 - ratio1;
			if( zero( bot ) ) 
				bot = 1.;
		}

		if( iPoint1 >= memberSections -1 )
			iPoint1 = memberSections - 1;
		if( iPoint2 >= memberSections -1 )
			iPoint2 = memberSections - 1;

		// now determine the texture coordinate value mapped from min:max -> 0:1
		double xValue1 = 0.;
		double xValue2 = 0.;
		double xValue = 0.;
		if( GraphType == -999 ){
			xValue1 = 0.;
			xValue2 = 0.;
		}
		else if( GraphType > 0 && GraphType < 100 ) 
		{
			// TeK Change 3/20/2008: IntToETDir doesn't work for GraphType = 6
			if( GraphType != 6 ) { 
				m_loadDir = IntToETDir( GraphType );
				xValue1 = pR->force( IntToETDir( GraphType ), iPoint1+1 );
				xValue2 = pR->force( IntToETDir( GraphType ), iPoint2+1 );
			} else { 
				m_loadDir = DX;
				xValue1 = pR->force( DX, iPoint1+1 );
				xValue2 = pR->force( DX, iPoint2+1 );
			}
		}
		else if( GraphType >= 100 ){// deflections
			m_loadDir = IntToETDir( GraphType-100 );
			xValue1 = pR->displacement( IntToETDir(GraphType-100), iPoint1+1 );
			xValue2 = pR->displacement( IntToETDir(GraphType-100), iPoint2+1 );
		}
		else {  // stress
			xValue1 = pR->stress( ETMemberStress(-GraphType), iPoint1+1 );
			xValue2 = pR->stress( ETMemberStress(-GraphType), iPoint2+1 );
		}

		if( zero( bot ) )
			bot = 1.;
		xValue = xValue1 + top/bot*(xValue2 - xValue1);

		double normalizedXValue = 0.;
		if( !zero(max - min) )
		{
			//if this is a diagram view we need to allow negative numbers
			if( bDiagram )
				normalizedXValue = (xValue)/abs(max-min);
			else
			{
				normalizedXValue = ( xValue - min )/( max - min );
				if( abs(max) < abs(min) )
					normalizedXValue = abs(( xValue - max )/( min - max ));
			}
		}
		if( abs( normalizedXValue ) > maxValue ) maxValue = normalizedXValue;
		m_spineTexCoords.push_back( normalizedXValue );
	}
}

bool CGraphicsMember::isSynchedWithDataMember()
{
	if( !m_pM ) return false;
	else return ( m_pM->UITime() == m_timeCreated );
}

void CGraphicsMember::SynchWithDataMember()
{
	if( !isSynchedWithDataMember() ){
		ASSERT_RETURN( m_pM );
		SetupSpine();
		// save the time stamp of this object so we know when to redo it
		m_timeCreated = m_pM->UITime();
		SetupExtrusions();
	}
}

double CGraphicsMember::GetYMax()
{
	if( m_HiLOD )
		return m_HiLOD->GetYMax();
	else return 0.f;
}

void CGraphicsMember::GetTranslatedAndRotatedCrossSection( CPoint3DArray& crossSection )
{
	//get the cross-section, translate it and rotate the points...
	CPoint3DArray localCrossSection;
	if( m_HiLOD )
	{
/*		glPushMatrix();
			glLoadIdentity();
			glTranslatef( float(m_Start.x), float(m_Start.y), float(m_Start.z) );
			glMultMatrixf( m_RotationMatrix );
			GLfloat matrix[16];
			glGetFloatv(GL_MODELVIEW_MATRIX,matrix);
		glPopMatrix(); */
		// RDV Note 3/26/09 - I'm actually braving it here and just placing the translations
		// in the 4th column - OLD SCHOOL!!! But then again my kids tell me I'm OLD.

		m_RotationMatrix[12+0]= (float)m_Start.x;
		m_RotationMatrix[12+1] = (float)m_Start.y;
		m_RotationMatrix[12+2] = (float)m_Start.z;

		CCrossSection c( m_pM );
		CPoint3DArray xsection;
		c.GetCrossSection( xsection );
	
		localCrossSection = m_HiLOD->extrusions()[1]->GetCrossSections()[1];
		//rotate and translate the points
		//these bloody things are 1-based (to parapharse charlton heston: damn dirty 1-based arrays)
		//JL Added 8/11/2009 - included offsets in local cross-section
		double ody = m_pM->centerlineOffset(DY);
		double odz = m_pM->centerlineOffset(DZ);
		bool bHaveOffset = (!zero(ody) || !zero(odz));
		for( int i = 1; i <= xsection./*localCrossSection.*/getItemsInContainer(); i++ )
		{
			CPoint3D pt;
			CPoint3D localCrossSectionPt = localCrossSection[i];
			if( bHaveOffset )
			{
				localCrossSectionPt.y += ody;
				localCrossSectionPt.z += odz;
			}
			multMatrix44AndPoint4( m_RotationMatrix, localCrossSectionPt, pt );
			crossSection.add( pt );
		}
	}
}

void CGraphicsMember::GetMemberPropertyBuffers( char* buf1, char* buf2, CWindowFilter& rFilter )
{
	// set up text for member property display (two buffers)
	ASSERT_RETURN( m_pM );

	int fillBufNumber = 1;
	ETShowUnit units = rFilter.showUnits ? SHOW_UNIT : NO_UNIT;
	bool bDesignView = (DESIGN_WINDOW == rFilter.windowType);
	bool bModelView = (MODEL_WINDOW == rFilter.windowType);
	char buf[512] = "";

	bool bHasMemberText = false;
	bool bHasExtremeLabel = false;
	if( MODEL_WINDOW == rFilter.windowType ) {
		bHasMemberText = (rFilter.member.name || rFilter.member.properties || 
			rFilter.member.material || rFilter.member.orientation || 
			rFilter.member.localAxes || rFilter.member.length	|| 
			rFilter.member.weight || rFilter.member.centerlineOffset ||
			rFilter.member.oneWay || rFilter.member.taper ||
			rFilter.member.endZone );
	}
	else if( POST_WINDOW == rFilter.windowType ) {
		bHasMemberText = rFilter.member.name;
		bHasExtremeLabel =  rFilter.member.extremeLabel && 
			rFilter.member.GetResultType() != MEMBER_NO_RESULT &&
			(rFilter.member.coloredResults || rFilter.member.diagramResults);
		
	}
	else if( DESIGN_WINDOW == rFilter.windowType ) {
		// TeK Change 10/16/2007: Added three flags after unity...
		// TeK Change 2/18/2008: Added another one!
		bHasMemberText = (rFilter.unityValue || rFilter.member.name || rFilter.member.properties || 
			rFilter.member.material || rFilter.designSize );
	}
	
	//if( !bHasMemberText && !bHasExtremeLabel )
	//	return;

	//load up the member text...
	if( bHasExtremeLabel && rFilter.resultCase())
	{
		int theGraphType = rFilter.member.GetGraphType();
		const CResult* pResult = rFilter.resultCase()->result( *m_pM );
		CString extremeLabel;
		getMemberExtremeLabel( pResult, theGraphType, extremeLabel );
		//text->AddLine( extremeLabel );
		addToBuffers( buf1, buf2, 256, &fillBufNumber, extremeLabel );
	}
	if( rFilter.member.name ) {    // label the member
		CString str = m_pM->name();
		if( rFilter.windowType == DESIGN_WINDOW ){
			int idg = theGroupManager.isMemberGrouped( m_pM );
			if( idg > 0 ){ 
				CDesignGroup* pDG = theGroupManager.group( idg );
				if( pDG ){
					str += " (";
					str += pDG->name();
					str += ")";
				}
			}else{
				str += " (Not Grouped)";
			}
		}
		//text->AddLine( str );
		addToBuffers( buf1, buf2, 256, &fillBufNumber, str );
	}

	if( rFilter.member.properties && (bModelView || bDesignView) )
	{
		addToBuffers( buf1, buf2, 256, &fillBufNumber,
			m_pM->property().list( CProperty::REPORT_SECTION, 0, 0, 0, units ) );
	}
	if( rFilter.member.material && (bModelView || bDesignView) )
	{
		addToBuffers( buf1, buf2, 256, &fillBufNumber,
			m_pM->material().list( CMaterial::REPORT_TYPE, 0, 0, 0, units ) );
	}
	if( bModelView ) {
		if( rFilter.member.weight ) {
			sprintf( buf, "W=%s", Units::show( FORCE_UNIT, m_pM->weight(), units) );
			addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
		}
		if( rFilter.member.length )
		{
			char buf[132] = {0};
			sprintf( buf, "L=%s", Units::show( LENGTH_LARGE_UNIT, m_pM->length(), units) );
			addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
		}
		if( !theStructure.isATruss() && ( rFilter.member.orientation )
			// TeK Add 6/14/97: User defined shapes don't use theta:
			&& (USER_PROPERTY != m_pM->property().type()) ) {
				char buf[132] = {0};
				if( theStructure.type() != SPACE_FRAME )
					sprintf( buf, "theta=%s", Units::show( ANGLE_UNIT, m_pM->property().theta(), units) );
				else
					sprintf( buf, "beta=%s", Units::show( ANGLE_UNIT, m_pM->property().beta(), units) );
				addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
		}
		if( rFilter.member.oneWay && m_pM->oneWayAction() != BOTH_TENSION_AND_COMPRESSION ) {
			char buf[132] = {0};
			if( m_pM->oneWayAction() == TENSION_ONLY ) sprintf( buf, "<=T=>" );
			else if( m_pM->oneWayAction() == COMPRESSION_ONLY ) sprintf( buf, "=>C<=" );
			else sprintf( buf, "2-Way" );
			addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
		}
		if( rFilter.member.taper && m_pM->taperType() != NO_TAPER ) {
			addToBuffers( buf1, buf2, 256, &fillBufNumber, m_pM->list( CMember::REPORT_TAPER_TYPE, 0, 0, 0, units) );
		}
		if( rFilter.member.endZone && ((m_pM->endZoneType(1) != NO_ENDZONE) || (m_pM->endZoneType(2) != NO_ENDZONE)) ) 
		{
			// report both end zones in one string
			addToBuffers( buf1, buf2, 256, &fillBufNumber, m_pM->list( CMember::REPORT_ENDZONE_TYPE, 0, 0, 0, units) );
		}
		double ody = m_pM->centerlineOffset(DY);
		double odz = m_pM->centerlineOffset(DZ);
		if( rFilter.member.centerlineOffset && (!zero(ody) || !zero(odz)) ) {
			char buf[132] = {0};
			sprintf( buf, "No Offset" );
			if( !zero(ody) && !zero(odz) ) {
				sprintf( buf, "Off.y=%s, Off.z=%s",
					Units::show( LENGTH_SMALL_UNIT, ody, units), 
					Units::show( LENGTH_SMALL_UNIT, odz, units)	); 
			}
			else if( !zero(ody) ) {
				sprintf( buf, "Off.y=%s", Units::show( LENGTH_SMALL_UNIT, ody, units)); 
			}
			else if( !zero(odz) ) {
				sprintf( buf, "Off.z=%s", Units::show( LENGTH_SMALL_UNIT, odz, units)); 
			}
			addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
		}
		if( rFilter.member.iFactors && 
			( !zero( 1. - m_pM->property().stiffnessFactor( X ) ) || 
			!zero( 1. - m_pM->property().stiffnessFactor( Y ) ) ||
			!zero( 1. - m_pM->property().stiffnessFactor( Z ) ) ) ) {
				char buf[132] = {0};
				if( theStructure.canMembersDisplace( RX ) && (m_pM->property().stiffnessFactor( X ) < 1.0) ) {
					sprintf( buf, "J %s%%", Units::show( UNITLESS_UNIT, m_pM->property().stiffnessFactor( X )*100.0 ) );
					addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
				}
				if( theStructure.canMembersDisplace( RY ) && (m_pM->property().stiffnessFactor( Y ) < 1.0) ) {
					sprintf( buf, "Iy %s%%", Units::show( UNITLESS_UNIT, m_pM->property().stiffnessFactor( Y )*100.0 ) );
					addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
				}
				if( theStructure.canMembersDisplace( RZ ) && (m_pM->property().stiffnessFactor( Z ) < 1.0) ) {
					sprintf( buf, "Iz %s%%", Units::show( UNITLESS_UNIT, m_pM->property().stiffnessFactor( Z )*100.0 ) );
					addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
				}
		}
	}

	// unity values
	if( bDesignView ) {
		if( rFilter.unityValue ) {
			// unity check values are required - set the pen type and color accordingly
			int group = theGroupManager.isMemberGrouped( m_pM );
			if( group > 0 ) {
				CDesignGroup* pGroup = theGroupManager.group( group );
				// TeK Added 11/19/2007: areUnityChecksValid call..
				if( pGroup && pGroup->areUnityChecksValid() ) {
					bool hasUnityMessage = false;
					double unityValue = pGroup->unityCheck( m_pM, &hasUnityMessage );
					char errorMarker[3] = "!";
					if( hasUnityMessage ) {
						if( pGroup->unityCheckMessageID( m_pM ) < 0 ) { // errors are -, warnings are +
							strcpy( errorMarker, "!!" ); 
						}
					}

					// TeK Added 10/1/2007: Missing VA5.5 feature, added per customer request
					bool bShowAsWorst = true;
					if( rFilter.worstUnityValueOnly ) {
						if( !equal( unityValue, pGroup->worstUnityCheck()) ) {
							bShowAsWorst = false;
						}
					}
					if( bShowAsWorst ) {
						// TeK Add 8/13/97: Mark unity values as APPROXIMATE if the design
						// has changed (we haven't synchronized it so analysis results
						// are for different member stiffnesses)
						if( theGroupManager.hasDesignChanged() ) {
							sprintf( buf, "~%s%s", (const char*)nice(unityValue), (hasUnityMessage? errorMarker : "") );
						}
						else {
							sprintf( buf, "%s%s", (const char*)nice(unityValue), (hasUnityMessage? errorMarker : "") );
						}
						addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
					}
				}
			}
		}
	
		// TeK Change 9/1/2007: We have inspector settings to show group name and member name independently!
		// Also we have different Name Prefix filters on these, so we must keep them independent...
		// obsolete???? JL 11/8/2006 (just use member name to filter this)
		// handle display of group names if necessary
		if( rFilter.member.groupName ) {
			int group = theGroupManager.isMemberGrouped( m_pM );
			if( group > 0 && rFilter.member.groupName ) {
				CDesignGroup* pGroup = theGroupManager.group( group );
				strcpy( buf, pGroup->name() );
				if( rFilter.member.name ) {
					strcat( buf, " - " );
				}
				addToBuffers( buf1, buf2, 256, &fillBufNumber, buf );
			}
			/*if( rFilter.member.name )
			strcat( buf2, m_pM->name() );*/
		}

		// handle display of design section or member name if necessary
		if( rFilter.designSize )  {
			int group = theGroupManager.isMemberGrouped( m_pM );
			if( group > 0 ) 
			{
				// TeK Change 9/16/98: Only display designed section name if design is valid,
				// otherwise show current
				CDesignGroup* pGroup = theGroupManager.group( group );
				char pSec[256] = "-NA-";
				if( pGroup )
				{
					strcpy( pSec, " " );
					strcat( pSec,
						pGroup->list( CDesignGroup::REPORT_DESIGNSHAPE, 0, 0, 0, NO_UNIT  ) );
				}
				//if( strlen( buf2 ) > strlen( buf1 ) ) strcat( buf1, pSec );
				//else 	strcat( buf2, pSec );
				addToBuffers( buf1, buf2, 256, &fillBufNumber, pSec );
			}
		}
	}
	return;
}

void getMemberExtremeLabel( const CResult* pResult, int theGraphType, CString& extremeLabel )
{
	extremeLabel = "N/A";
	CHECK_IF( pResult )
	{
		CString temp;
		// filter results
		//if( (m_Filter.member.coloredResults || m_Filter.member.diagramResults) && m_Filter.member.GetResultType() != MEMBER_NO_RESULT )
		// axial force
		double max = 0;
		double min = 0;
		if( theGraphType == 6 ) { 
			min = pResult->maxForce( DX, -1 );
			max = pResult->maxForce( DX, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( FORCE_UNIT, max );
			else extremeLabel = Units::show( FORCE_UNIT, min );
			//if( pResult->haveTime() && m_Filter.loadCase()->haveMovingLoads() ) {
			//	timeInformation* pt = pResult->maxForceTimeInformation( DX );
			//	if( pt ) {
			//		double t = pt->time;
			//		s = s + theMovingLoadManager.truckLocationDescription( (const CServiceCase*)m_Filter.loadCase(), t, true );
			//	}
		}
		//VY
		else if( theGraphType == 1 ) { 
			min = pResult->maxForce( DY, -1 );
			max = pResult->maxForce( DY, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( FORCE_UNIT, max );
			else extremeLabel = Units::show( FORCE_UNIT, min );
			//if( pResult->haveTime() && m_Filter.loadCase()->haveMovingLoads() ) {
			//	timeInformation* pt = pResult->maxForceTimeInformation( DY );
			//	if( pt ) {
			//		double t = pt->time;
			//		s = s + theMovingLoadManager.truckLocationDescription( (const CServiceCase*)m_Filter.loadCase(), t, true );
			//	}
			//}
		}
		//VZ
		else if( theGraphType == 2 ) { 
			min = pResult->maxForce( DZ, -1 );
			max = pResult->maxForce( DZ, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( FORCE_UNIT, max );
			else extremeLabel = Units::show( FORCE_UNIT, min );
			//if( pResult->haveTime() && m_Filter.loadCase()->haveMovingLoads() ) {
			//	timeInformation* pt = pResult->maxForceTimeInformation( DZ );
			//	if( pt ) {
			//		double t = pt->time;
			//		s = s + theMovingLoadManager.truckLocationDescription( (const CServiceCase*)m_Filter.loadCase(), t, true );
			//	}
			//}
		}
		// moments
		// Mx
		else if( theGraphType == 3 )
		{
			min = pResult->maxForce( RX, -1 );
			max = pResult->maxForce( RX, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( MOMENT_UNIT, max );
			else extremeLabel = Units::show( MOMENT_UNIT, min );
			//if( pResult->haveTime() && m_Filter.loadCase()->haveMovingLoads() ) {
			//	timeInformation* pt = pResult->maxForceTimeInformation( RX );
			//	if( pt ) {
			//		double t = pt->time;
			//		s = s + theMovingLoadManager.truckLocationDescription( (const CServiceCase*)m_Filter.loadCase(), t, true );
			//	}
			//}
		}
		// My
		else if( theGraphType == 4 )
		{
			min = pResult->maxForce( RY, -1 );
			max = pResult->maxForce( RY, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( MOMENT_UNIT, max );
			else extremeLabel = Units::show( MOMENT_UNIT, min );
			//if( pResult->haveTime() && m_Filter.loadCase()->haveMovingLoads() ) {
			//	timeInformation* pt = pResult->maxForceTimeInformation( RY );
			//	if( pt ) {
			//		double t = pt->time;
			//		s = s + theMovingLoadManager.truckLocationDescription( (const CServiceCase*)m_Filter.loadCase(), t, true );
			//	}
			//}
		}
		// Mz
		else if( theGraphType == 5 )
		{
			min = pResult->maxForce( RZ, -1 );
			max = pResult->maxForce( RZ, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( MOMENT_UNIT, max );
			else extremeLabel = Units::show( MOMENT_UNIT, min );
			//if( pResult->haveTime() && m_Filter.loadCase()->haveMovingLoads() ) {
			//	timeInformation* pt = pResult->maxForceTimeInformation( RZ );
			//	if( pt ) {
			//		double t = pt->time;
			//		s = s + theMovingLoadManager.truckLocationDescription( (const CServiceCase*)m_Filter.loadCase(), t, true );
			//	}
			//}
		}
		// displacements
		else if( theGraphType == 100 ){
			min = pResult->maxDisplacement( DX,-1 );
			max = pResult->maxDisplacement( DX, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( LENGTH_SMALL_UNIT, max );
			else extremeLabel = Units::show( LENGTH_SMALL_UNIT, min );
			//if( pResult->haveTime() && m_Filter.loadCase()->haveMovingLoads() ) {
			//	timeInformation* pt = pResult->maxDisplacementTimeInformation( DX );
			//	if( pt ) {
			//		double t = pt->time;
			//		s = s + theMovingLoadManager.truckLocationDescription( (const CServiceCase*)m_Filter.loadCase(), t, true );
			//	}
			//}
		}
		else if( theGraphType == 101 ){
			min = pResult->maxDisplacement( DY,-1 );
			max = pResult->maxDisplacement( DY, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( LENGTH_SMALL_UNIT, max );
			else extremeLabel = Units::show( LENGTH_SMALL_UNIT, min );
			//if( pResult->haveTime() && m_Filter.loadCase()->haveMovingLoads() ) {
			//	timeInformation* pt = pResult->maxDisplacementTimeInformation( DX );
			//	if( pt ) {
			//		double t = pt->time;
			//		s = s + theMovingLoadManager.truckLocationDescription( (const CServiceCase*)m_Filter.loadCase(), t, true );
			//	}
			//}
		}else if( theGraphType == 102 ){
			min = pResult->maxDisplacement( DZ,-1 );
			max = pResult->maxDisplacement( DZ, 1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( LENGTH_SMALL_UNIT, max );
			else extremeLabel = Units::show( LENGTH_SMALL_UNIT, min );
			//if( pResult->haveTime() && m_Filter.loadCase()->haveMovingLoads() ) {
			//	timeInformation* pt = pResult->maxDisplacementTimeInformation( DX );
			//	if( pt ) {
			//		double t = pt->time;
			//		s = s + theMovingLoadManager.truckLocationDescription( (const CServiceCase*)m_Filter.loadCase(), t, true );
			//	}
			//}
		}
		// stress
		else {  
			min = pResult->maxStress( (ETMemberStress)(-theGraphType), -1 );
			max = pResult->maxStress( (ETMemberStress)(-theGraphType),  1 );
			if( abs(max) > abs(min) )
				extremeLabel = Units::show( STRESS_UNIT, max );
			else extremeLabel = Units::show( STRESS_UNIT, min );
		}
	}
}