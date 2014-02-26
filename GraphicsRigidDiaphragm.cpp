#include "StdAfx.h"

#include "GraphicsRigidDiaphragm.h"

#include "Graphics/GraphicsHelpers.h"
#include "IniSize.h"
#include "Model.h"
//#include "Units\Units.h"
//#include "datautil.h"


CGraphicsRigidDiaphragm::CGraphicsRigidDiaphragm( const CRigidDiaphragm* pRD, const CResultCase* pRC, double scale ):
m_pRD( pRD ),
m_pRC( pRC ),
m_scale( scale ),
m_Start( CPoint3D( 0., 0., 0. ) )
{
}

CGraphicsRigidDiaphragm::~CGraphicsRigidDiaphragm(){};

void CGraphicsRigidDiaphragm::Draw( )
{
	ASSERT_RETURN( m_pRD );

	if( m_pRD->simple() ) {  // simple diaphragm contains all nodes in this plane!
		// use the overall model extents for this definition if we can not determine node extremes
		CCoordinate maxC = theModel.maximumCoordinate();
		CCoordinate minC = theModel.minimumCoordinate();
		CCoordinate mi; CCoordinate mx;
		mi.x(1.E10); mi.y(1.E10); mi.z(1.E10);
		mx.x(-1.E10); mx.y(-1.E10); mx.z(-1.E10);
		for( int i = 1; i <= m_pRD->nodes(); i++ ) {
			 const CNode* pN = m_pRD->node( i );
			 if( pN->x() > mx.x() ) mx.x( pN->x() );
			 if( pN->y() > mx.y() ) mx.y( pN->y() );
			 if( pN->z() > mx.z() ) mx.z( pN->z() );
			 if( pN->x() < mi.x() ) mi.x( pN->x() );
			 if( pN->y() < mi.y() ) mi.y( pN->y() );
			 if( pN->z() < mi.z() ) mi.z( pN->z() );
		}
		if( mx.x() > -1.E10 ) maxC.x( mx.x() );
		if( mx.y() > -1.E10 ) maxC.y( mx.y() );
		if( mx.z() > -1.E10 ) maxC.z( mx.z() );
		if( mi.x() < 1.E10 ) minC.x( mi.x() );
		if( mi.y() < 1.E10 ) minC.y( mi.y() );
		if( mi.z() < 1.E10 ) minC.z( mi.z() );
		DrawDiaphragm( maxC, minC );
		}   // simple()
	else {  // not a simple diaphragm - we draw a "diaphragm" around each node
		for( int inode = 1; inode <= m_pRD->nodes(); inode++ ) {
			const CNode* pN = m_pRD->node( inode );
			CCoordinate maxC = *pN;
			double mIniSizeFactor = 1;
			maxC.x( maxC.x() + mIniSizeFactor*ini.size().rigidDiaphragm );
			maxC.y( maxC.y() + mIniSizeFactor*ini.size().rigidDiaphragm );
			maxC.z( maxC.z() + mIniSizeFactor*ini.size().rigidDiaphragm );
			CCoordinate minC = *pN;
			minC.x( minC.x() - mIniSizeFactor*ini.size().rigidDiaphragm );
			minC.y( minC.y() - mIniSizeFactor*ini.size().rigidDiaphragm );
			minC.z( minC.z() - mIniSizeFactor*ini.size().rigidDiaphragm );
			DrawDiaphragm( maxC, minC );
		}   // inode
	}


	return;
}

void CGraphicsRigidDiaphragm::DrawDiaphragm( const CCoordinate& maxC, const CCoordinate& minC )
{
	//double xw[4], yw[4], zw[4];
	CPoint3D points[4];
	switch( m_pRD->normalDirection() ) {
		case X:
		default:
			points[0].x = points[1].x = points[2].x = points[3].x = m_pRD->planeCoordinate();
			points[0].y = maxC.y(); points[1].y = minC.y(); points[2].y = minC.y(); points[3].y = maxC.y();
			points[0].z = maxC.z(); points[1].z = maxC.z(); points[2].z = minC.z(); points[3].z = minC.z();

			/*xw[0] = xw[1] = xw[2] = xw[3] = m_pRD->planeCoordinate();
			yw[0] = maxC.y(); yw[1] = minC.y(); yw[2] = minC.y(); yw[3] = maxC.y();
			zw[0] = maxC.z(); zw[1] = maxC.z(); zw[2] = minC.z(); zw[3] = minC.z();*/
			break;
		case Y:
			points[0].y = points[1].y = points[2].y = points[3].y = m_pRD->planeCoordinate();
			points[0].z = maxC.z(); points[1].z = minC.z(); points[2].z = minC.z(); points[3].z = maxC.z();
			points[0].x = maxC.x(); points[1].x = maxC.x(); points[2].x = minC.x(); points[3].x = minC.x();
			break;
		case Z:
			points[0].z = points[1].z = points[2].z = points[3].z = m_pRD->planeCoordinate();
			points[0].x = maxC.x(); points[1].x = minC.x(); points[2].x = minC.x(); points[3].x = maxC.x();
			points[0].y = maxC.y(); points[1].y = maxC.y(); points[2].y = minC.y(); points[3].y = minC.y();
			break;
	}
	DrawQuadPlate( points[0], points[1], points[2], points[3] );
	/*glBegin( GL_QUADS );
	glVertex3f( float( xw[0] ), float( yw[0] ), float( zw[0] ) );
	glVertex3f( float( xw[1] ), float( yw[1] ), float( zw[1] ) );
	glVertex3f( float( xw[2] ), float( yw[2] ), float( zw[2] ) );
	glVertex3f( float( xw[3] ), float( yw[3] ), float( zw[3] ) );
	glEnd();*/
}
void CGraphicsRigidDiaphragm::SetupOrientation( )
{
}
