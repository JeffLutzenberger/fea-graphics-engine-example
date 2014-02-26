#include "StdAfx.h"
#include "GraphicsFoundation.h"
#include "Graphics/GraphicsHelpers.h"
#include "Graphics/GraphicsHelperClasses.h"
#include "ini.h"
#include "IniSize.h"
#include "IniColor.h"
#include "IniFont.h"
#include "Node.h"
#include "Model.h"
#include "project.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

CGraphicsFoundation::CGraphicsFoundation( const CFoundation* pF ):
m_pF( pF )
{
}

CGraphicsFoundation::~CGraphicsFoundation(void)
{
}

void CGraphicsFoundation::Draw(const CWindowFilter &/*filter*/)
{
	ASSERT_RETURN( m_pF );

	CPoint3DArray topPts;
	CPoint3DArray bottomPts;
	CVector3D norm( m_pF->boundary().normal().x(), m_pF->boundary().normal().y(), m_pF->boundary().normal().z() );
	double t = m_pF->thickness();
	for( int j = 1; j <= m_pF->boundary().points(); j++ ) {
		topPts.add( *m_pF->boundary().point(j) );
	}

	for( int j = 1; j <= m_pF->boundary().points(); j++ ) {
		CPoint3D pt = *m_pF->boundary().point(j);
		pt.x += -norm.x*t;
		pt.y += -norm.y*t;
		pt.z += -norm.z*t;
		bottomPts.add( pt );
	}

	CGLTessellatedPolygon topPoly( topPts );
	CGLTessellatedPolygon bottomPoly( bottomPts );

	if( m_pF->isSelected() /*&& style != GRAPHIC_LINE*/ )
	{
		float color[4] = { 9.f, 9.f, 0.0f, ALPHA };
		ApplyAmbientGLMaterial( color );
	}
	else
	{
		glLineWidth( 1.f );
		float color[4] = { 0.f, 0.f, 75.f, ALPHA };
		ApplyAmbientGLMaterial( color );
	}
	topPoly.Draw();
	bottomPoly.Draw();

	//now draw sides
	glBegin( GL_QUADS );
	int j = 1;
	for( int i = 1; i <= topPts.getItemsInContainer(); i++ ){
		j++;
		if( j > topPts.getItemsInContainer() ) 
			j = 1;
		CGLFace( bottomPts[i], bottomPts[j], topPts[j], topPts[i] ).Draw();
	}
	glEnd();

}
