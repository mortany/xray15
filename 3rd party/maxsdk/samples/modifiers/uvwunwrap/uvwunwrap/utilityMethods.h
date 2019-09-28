/*

Copyright 2015 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/

#pragma once
#include "point3.h"
#include "matrix3.h"
#include "maxapi.h"

class UnwrapUtilityMethods
{
public:
	static void UnwrapMatrixFromNormal(Point3& normal, Matrix3& mat);
    static void UpdatePrompt(Interface *ip, const TCHAR *status, int skip = 10);
	static int CompTableDiam(const void *elem1, const void *elem2);
	static int CompTableArea(const void *elem1, const void *elem2);
};