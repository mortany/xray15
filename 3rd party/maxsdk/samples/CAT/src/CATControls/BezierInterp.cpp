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

/* epsilon surrounding for near zero values */

#include "BezierInterp.h"
#include "bmmlib.h"

#define     EQN_EPS     1e-9
#define	    IsZero(x)	((x) > -EQN_EPS && (x) < EQN_EPS)

#define     cbrt(x)     ((x) > 0.0 ? pow((double)(x), 1.0/3.0) : \
                          ((x) < 0.0 ? -pow((double)-(x), 1.0/3.0) : 0.0))

float DT(CATKey n0, CATKey n1)
{
	return float(n1.x - n0.x);
};

/* ************************************************************************** **
** DESCRIPTION:																  **
** ************************************************************************** */
float	getAngle(const CATKey	&knot)
{
	return (float)atan(knot.Tangent / (GetFrameRate()));
}

/* ************************************************************************** **
** DESCRIPTION:															      **
** ************************************************************************** */
float	GetOutTanHypDist(const CATKey	&knot1,
	const CATKey	&knot2,
	const float	angle)
{
	return (knot2.x - knot1.x) / (float)cos(angle) * knot1.OutTanLength;
}

/* ************************************************************************** **
** DESCRIPTION:																  **
** ************************************************************************** */
float	GetInTanHypDist(const CATKey	&knot1,
	const CATKey	&knot2,
	const float		angle)
{
	return (knot2.x - knot1.x) / (float)cos(angle) * knot2.InTanLength;
};

int SolveCubic(double c[4], double s[3])
{
	int     i, num;
	double  sub;
	double  A, B, C;
	double  sq_A, p, q;
	double  cb_p, D;

	/* normal form: x^3 + Ax^2 + Bx + C = 0 */

	A = c[2] / c[3];
	B = c[1] / c[3];
	C = c[0] / c[3];

	/*  substitute x = y - A/3 to eliminate quadric term:
  x^3 +px + q = 0 */

	sq_A = A * A;
	p = 1.0 / 3 * (-1.0 / 3 * sq_A + B);
	q = 1.0 / 2 * (2.0 / 27 * A * sq_A - 1.0 / 3 * A * B + C);

	/* use Cardano's formula */

	cb_p = p * p * p;
	D = q * q + cb_p;

	if (IsZero(D))
	{
		if (IsZero(q)) /* one triple solution */
		{
			s[0] = 0;
			num = 1;
		}
		else /* one single and one double solution */
		{
			double u = cbrt(-q);
			s[0] = 2 * u;
			s[1] = -u;
			num = 2;
		}
	}
	else if (D < 0) /* Casus irreducibilis: three real solutions */
	{
		double phi = 1.0 / 3 * acos(-q / sqrt(-cb_p));
		double t = 2 * sqrt(-p);

		s[0] = t * cos(phi);
		s[1] = -t * cos(phi + PI / 3);
		s[2] = -t * cos(phi - PI / 3);
		num = 3;
	}
	else /* one real solution */
	{
		double sqrt_D = sqrt(D);
		double u = cbrt(sqrt_D - q);
		double v = -cbrt(sqrt_D + q);

		s[0] = u + v;
		num = 1;
	}

	/* resubstitute */

	sub = 1.0 / 3 * A;

	for (i = 0; i < num; ++i)
		s[i] -= sub;

	return num;
};

/******************************************************************************\
 ******************************************************************************
\******************************************************************************/

/* ************************************************************************** **
** DESCRIPTION: Given two knots, computes the control points controling the   **
**   tangent of the Bezier segments, in 'world position'.                     **
** PARAMETERS:																  **
**   knot1 knot2.... two knots. The Bezier segment is in [knot1.x, knot2.x]   **
**                   if knot1.x > knot2.x, then BASE is added to knot2,x,     **
**                   and substracted to knot1's tangents x position.		  **
**   outVec, inVec.. return values.											  **
** NOTES:                                                                     **
**   outVec, inVec...... 'world' positions of the tangent endpoints           **
**   angle.............. angle of the handle.                                 **
**   base............... length of the base of the triangle formed by the     **
**                       handle and timeline.                                 **
**   height............. height of the triangle formed by the handle and      **
**                       timeline.                                            **
**   hyp................ hypotenuse of the triangle formed by the handle and  **
**                       timeline.                                            **
** ************************************************************************** */
void	computeControlKeys(CATKey			knot1,
	CATKey			knot2,
	Point2			&outVec,
	Point2			&inVec)
{
	float			angle, hyp;
	bool			isShifted(false);

	if (knot2.x < knot1.x)
		knot2.x += STEPTIME100, isShifted = true;
	//	knot1.Tangent = -knot1.Tangent;
	knot2.Tangent = -knot2.Tangent;

	angle = getAngle(knot1);
	hyp = GetOutTanHypDist(knot1, knot2, angle);
	outVec.x = knot1.x + (float)cos(angle) * hyp;
	outVec.y = knot1.y + (float)sin(angle) * hyp;

	angle = getAngle(knot2);
	hyp = GetInTanHypDist(knot1, knot2, angle);
	if (hyp < 0.0f)
	{
		// We have a zero length handle, which we cant put up with
		// so we set back to defaults.
		hyp = DT(knot1, knot2) / 3.0f / (float)cos(angle);
		knot2.InTanLength = 0.3333f;
	}
	inVec.x = knot2.x - (float)cos(angle) * hyp;
	inVec.y = knot2.y + (float)sin(angle) * hyp;
	if (isShifted)
		inVec.x -= STEPTIME100;
}

/* ************************************************************************** **
** DESCRIPTION:																  **
**  My function will pass in the 2 points as point2							  **
**  Plus the EaseIn and EaseOut as point2 									  **
**  plus u , t, val															  **
**  t must still be passed in												  **
**  My ease in out handle lengths will be normalised to 0..1				  **
**  a structure will be defined to handle all key data.						  **
**  CATKey 																	  **
** 		.x																	  **
** 		.y																	  **
** 		.OutTan			// gradient of handle								  **
** 		.OutTanLength														  **
** 		.InTan			// gradient											  **
** 		.InTanLength														  **
** void HybridInterpolator::InterpValue(int n0, int n1, float u, float *val)  **
** ************************************************************************** */
float InterpValue(CATKey knot1, CATKey knot2, TimeValue t)//, float *val)
{
	//tpf = GetFrameRate();
	//knot1.Tangent /= tpf;
	//knot2.Tangent /= tpf;

	// Phil u must be 0..1 and s = 1 - u
	float u = (t - knot1.x) / (knot2.x - knot1.x);
	// u is just a linear blend between no time and n1 time

	// 'world' positions of the tangent endpoints

	Point2	outVec, inVec;

	//computeControlKeys(knot1, knot2, outVec, inVec);
	//knot1.Tangent /= tpf;
	//knot2.Tangent /= tpf;

	float angle;
	// angle of the handle
	float base;
	// length of the base of the triangle formed by the handle and timeline
	float height;
	// height of the triangle formed by the handle and timeline
	float hyp;
	// hypotenuse of the triangle formed by the handle and timeline

	// OutTan is dy/dx
	// the UI will show OutTan * FrameRate,
	// so this function will be passed OutTan/FrameRate
	angle = getAngle(knot1);

	//watje2
		// length of the hypetenuse of the triangle formed with the tangent handle and x
	hyp = GetOutTanHypDist(knot1, knot2, angle);

	//if we have a negative length set it to 1/3 the distance between neighbor
		// Phil which means 0.33 length tangent. I.E. Default tangent length
		// hyp = handle length
	//	if (hyp < 0.0f)
	//	{
			// We have a zero length handle, which we cant put up with
			// so we set back to defaults
	//		base = DT(knot1,knot2)/3.0f;
	//		hyp = base/cos(angle)/GetTicksPerFrame();
	//		knot1.OutTanLength = 0.3333f;
	//	}
		// then base needs to get written regardless
	base = (float)cos(angle) * hyp;// * GetTicksPerFrame();

	// Phil: height of the tangent handle end.
	height = (float)sin(angle) * hyp;// * GetTicksPerFrame();

	outVec.x = knot1.x;
	outVec.y = knot1.y;
	outVec.x += base;
	outVec.y += height;

	angle = getAngle(knot2);
	hyp = GetInTanHypDist(knot1, knot2, angle);
	if (hyp < 0.0f)
	{
		// We have a zero length handle, which we cant put up with
		// so we set back to defaults
		base = DT(knot1, knot2) / 3.0f;
		hyp = base / (float)cos(angle);///GetFrameRate();
		knot2.InTanLength = 0.3333f;
	}
	base = (float)cos(angle) * hyp;// * GetTicksPerFrame();
	height = (float)sin(angle) * hyp;// * GetTicksPerFrame();

	inVec.x = knot2.x;
	inVec.y = knot2.y;
	inVec.x -= base;
	inVec.y += height;

	//new quadratic solver
	//	float location;

		// Phil: as far as i can see this just returns t
		// u was evaluated by linearly interpolating between the two keys
		// u was calculated in GetInterpParam()
	//	location = knot1.x+(knot2.x-knot1.x) * u;

		// Phil: ax3+bx2+cx+d=0
	double Ax, Bx, Cx, Dx;
	Ax = knot2.x - knot1.x - 3.0f*inVec.x + 3.0f*outVec.x;
	Bx = knot1.x - 2.0f * outVec.x + inVec.x;
	Cx = -knot1.x + outVec.x;
	Dx = knot1.x - t;

	double Ay, By, Cy, Dy;
	Ay = knot2.y - knot1.y - 3.0f*inVec.y + 3.0f*outVec.y;
	By = knot1.y - 2.0f * outVec.y + inVec.y;
	Cy = -knot1.y + outVec.y;
	Dy = knot1.y;

	//Dx -= location;
	//Dx -= t
	double c[4], sol[3];
	c[0] = Dx;
	c[1] = 3.0f*Cx;
	c[2] = 3.0f*Bx;
	c[3] = Ax;

	float newU;
	// Phil: Why is this??
	if ((Ax == 0.0f) && (Bx == 0.0f))
	{
		newU = u;
	}
	// Phil: if t is on key1
	//else if (location == knot1.x)
	else if (t == knot1.x)
	{
		newU = 0.0f;
	}
	// Phil: if t is on key2
	//else if (location == knot2.x)
	else if (t == knot2.x)
	{
		newU = 1.0f;
	}
	else
	{
		int count = SolveCubic(c, sol);
		// Phil: Count is th nuber of solutions to the cubic equation
		if (count == 1)
			newU = (float)sol[0]; // One solution, just return it
		else
		{
			newU = -1.0f;
			for (int i = 0; i < count; i++)
			{
				// Phil: make sure the solution is within 0..1
				if ((sol[i] >= 0.0f) && (sol[i] <= 1.0f))
				{
					// Phil: newU was initialised to -1 so grab the first solution you come across.
					if (newU < 0.0f)
						newU = (float)sol[i];
					// Get the largest solution
					else if (sol[i] > newU)
						newU = (float)sol[i];
				}

			}
		}
	}

	float newU2 = newU*newU;
	float newU3 = newU2;
	newU3 *= newU;
	// Phil: Wow, The cubic equation!!!
	//			ax3 + bx2 + cx + d
	float Yvalue = float(Ay*newU3 + 3.0f*By*newU2 + 3.0f*Cy*newU + Dy);

	//	*val = Yvalue;
	return Yvalue;

};

/* ************************************************************************** **
** Description: writes pixel 'color' at [x,y] in picture pix.				  **
** ************************************************************************** */
void	mySetPixel(Bitmap				&pix,
	const int			x,
	const int			y,
	BMM_Color_fl	&color)
{
	pix.PutPixels(x, y, 1, &color);
};

/* ************************************************************************** **
** Description: draw segment s0, s1 in bitmap pix.							  **
**   Uses Bresenham algorithm, considering directions					      **
**   dx > 0, i.e. the four right sided octants.                               **
** ************************************************************************** */

void DrawSegment(Bitmap &pix, const int *const p0, const int *const p1, BMM_Color_fl &color)
{
	const int	dx(p1[0] - p0[0]);
	const int	dy(p1[1] - p0[1]);
	int			x, y, eps;

	if (dy == 0)
		for (x = p0[0]; x < p1[0]; ++x)
			mySetPixel(pix, x, p0[1], color);
	else if ((dy <= dx) && (0 <= dx) && (0 <= dy))
	{
		y = p0[1];
		eps = 0;
		for (x = p0[0]; x < p1[0]; ++x)
		{
			mySetPixel(pix, x, y, color);
			eps += dy;
			if (2 * eps >= dx)
			{
				y = y + 1;
				eps -= dx;
			}
		}
	}
	else if ((dx <= dy) && (0 <= dx) && (0 < dy))
	{
		x = p0[0];
		eps = 0;

		for (y = p0[1]; y < p1[1]; ++y)
		{
			mySetPixel(pix, x, y, color);
			eps += dx;
			if (2 * eps >= dy)
			{
				x = x + 1;
				eps -= dy;
			}
		}
	}
	else if ((-dy < dx) && (0 <= dx) && (dy < 0))
	{
		y = p0[1];
		eps = 0;
		for (x = p0[0]; x < p1[0]; ++x)
		{
			mySetPixel(pix, x, y, color);
			eps += -dy;
			if (2 * eps >= dx)
			{
				y = y - 1;
				eps -= dx;
			}
		}
	}
	else if ((dx <= -dy) && (0 <= dx) && (dy < 0))
	{
		x = p0[0];
		eps = 0;
		for (y = -p0[1]; y < -p1[1]; y++)
		{
			mySetPixel(pix, x, -y, color);
			eps += dx;
			if (2 * eps >= -dy)
			{
				x = x + 1;
				eps -= -dy;
			}
		}
	}
};

/* ************************************************************************** **
** Description: computes two coeffs alpha and beta							  **
**   to rescale     coeff in [oldMin, oldMax] to							  **
**   alpha + beta * coeff in [newMin, newMax]								  **
** ************************************************************************** */
void	ComputeRescaleCoeffs(const double	oldMin,
	const double	oldMax,
	const double	newMin,
	const double	newMax,
	double			&alpha,
	double			&beta)
{
	beta = (newMax - newMin) / (oldMax - oldMin);
	alpha = newMin - beta * oldMin;
};
