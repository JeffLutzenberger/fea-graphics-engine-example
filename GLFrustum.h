#pragma once

#include "node.h"
#include "member.h"

//Frustum class set's up the planes described by our current view frustum.
//Should work for any camera. Allows us to test if points are visible before
//we attempt to draw them.
//http://www.crownandcutlass.com/features/technicaldetails/frustum.html
class CGLFrustum
{
public:
	CGLFrustum(void);
	~CGLFrustum(void);

	void ExtractFrustum();

	bool PointInFrustum( float x, float y, float z );

	bool NodeInFrustum( const CNode* pN );

	bool MemberInFrustum( const CMember* pM );

private:
	float frustum[6][4];
};
