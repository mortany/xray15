/*

Copyright 2015 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/
#include "utilityMethods.h"
#include "ClusterClass.h"

void UnwrapUtilityMethods::UnwrapMatrixFromNormal(Point3& normal, Matrix3& mat)
{
	Point3 vx;
	vx.z = .0f;
	vx.x = -normal.y;
	vx.y = normal.x;
	if (vx.x == .0f && vx.y == .0f) {
		vx.x = 1.0f;
	}
	mat.SetRow(0, vx);
	mat.SetRow(1, normal^vx);
	mat.SetRow(2, normal);
	mat.SetTrans(Point3(0, 0, 0));
	mat.NoScale();
}

void UnwrapUtilityMethods::UpdatePrompt(Interface *ip, const TCHAR *status, int skip)	 	 
{	 	 
    static unsigned long lSkip = 0;	 	 
    ++lSkip;	 	 
    if ((ip) && (status) && ((skip == 0) || ((lSkip%skip) == 0)))
    {
        ip->ReplacePrompt(status);	 	 
    }
}

int UnwrapUtilityMethods::CompTableDiam(const void *elem1, const void *elem2) 
{
	ClusterClass **ta = (ClusterClass **)elem1;
	ClusterClass **tb = (ClusterClass **)elem2;

	ClusterClass *a = *ta;
	ClusterClass *b = *tb;

	float aDiam, bDiam;
	aDiam = a->w > a->h ? a->w : a->h;
	bDiam = b->w > b->h ? b->w : b->h;

	if (aDiam == bDiam) return 0;
	else if (aDiam > bDiam) return -1;
	else return 1;
}

int UnwrapUtilityMethods::CompTableArea(const void *elem1, const void *elem2) 
{
	ClusterClass **ta = (ClusterClass **)elem1;
	ClusterClass **tb = (ClusterClass **)elem2;

	ClusterClass *a = *ta;
	ClusterClass *b = *tb;

	float aH, bH;
	aH = a->w * a->h;
	bH = b->w * b->h;

	if (aH == bH) return 0;
	else if (aH > bH) return -1;
	else return 1;

}
