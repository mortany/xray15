//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2009 Autodesk, Inc.  All rights reserved.
//  Copyright 2003 Character Animation Technologies.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "cat.h"
#include "math.h"
#include <math.h>

/////////////////////////////////////////////////////
//
// Is used for Bezier interp
class CATKey {
public:
	//Parameters for the bezier interpolation
	float x, y, Tangent, OutTanLength, InTanLength;

	inline CATKey(float xval, float yval, float Tangent, float outTL, float inTL);
	inline CATKey() : x(0.0f), y(0.0f), Tangent(0.0f), OutTanLength(0.0f), InTanLength(0.0f) {}
	inline void maxMin();

};

inline CATKey::CATKey(float xval, float yval, float Tan, float outTL, float inTL)
{

	x = xval;
	y = yval;
	Tangent = Tan;
	OutTanLength = outTL;
	InTanLength = inTL;
	maxMin();
};

inline void CATKey::maxMin()
{
	// Values MUST be between 0 and 1
	if (OutTanLength > 1) OutTanLength = 1;
	if (OutTanLength < 0) OutTanLength = 0;
	if (InTanLength > 1) InTanLength = 1;
	if (InTanLength < 0) InTanLength = 0;
};

extern void	computeControlKeys(CATKey knot1,
	CATKey knot2,
	Point2 &outVec,
	Point2 &inVec);
extern float InterpValue(CATKey knot1, CATKey knot2, TimeValue t);//, float *val);

extern void	DrawSegment(Bitmap &pix, const int *const p0, const int *const p1, BMM_Color_fl &color);
extern void	ComputeRescaleCoeffs(const double oldMin, const double	oldMax, const double newMin, const double newMax, double &alpha, double &beta);
