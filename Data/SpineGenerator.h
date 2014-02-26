///////////////////////////////////////////////////////////////////////////
// CSpineGenerator.h      		
// version 1.0                                               04 October 2005
// 
//
// class CSpineGenerator - this class returns the local spine of a member
//                         or cable element. If it's a member element it 
//                         can optionally return the rotation of the spine
//                         due to torsion about the x axis of the member.
//
///////////////////////////////////////////////////////////////////////////
#pragma once

#include "Math/Analytic3d.h"
#include "member.h"
#include "cable.h"
#include "rsltcase.h"

class DATA_LINK CSpineGenerator
{
public:
	CSpineGenerator();
	CSpineGenerator( const CMember* m );
	CSpineGenerator( const CMember* m, const CResultCase* pRC );
	CSpineGenerator( const CCable* c );
	CSpineGenerator( const CCable* c, const CResultCase* pRC );
	~CSpineGenerator(void);

	bool GetSpine( bool deformed, int nPoints, double setback, double scale, std::vector<CPoint3D>& spine );
	bool GetRotations( int nPoints, double setback, double rotations[] );

private:
	bool GetCableSpine( bool deformed, int nPoints, double setback, double scale, std::vector<CPoint3D>& spine );
	bool GetMemberSpine( bool deformed, int nPoints, double setback, double scale, std::vector<CPoint3D>& spine );

	int nPoints;
	const CCable* m_pC;
	const CMember* m_pM;
	const CResultCase* m_pRC;
};
