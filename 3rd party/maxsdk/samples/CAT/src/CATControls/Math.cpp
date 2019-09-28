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

#include "math.h"

#include "CatPlugins.h"
#include "simpobj.h"
#include <math.h>
#include "decomp.h"

// Max Script
#include <maxscript/maxscript.h>
#include <maxscript/util/listener.h>
#include <maxscript/maxwrapper/mxsobjects.h>

#include <maxscript/foundation/strings.h>
#include <maxscript/foundation/numbers.h>

#include <maxscript/compiler/parser.h>

#include <CATAPI/CATClassID.h>

// Hip re-calc function. Stripped down DetectCol. Will align hip to imaginary sphere
// with center spherecenter, and radius of sphereradius, from outside and inside sphere in
// hipTM z direction.
void RaySphereCollision(Ray &ray, Point3 spherecenter, float radius)
{
	// Put the ray into sphere space
	ray.p -= spherecenter;
	ray.dir *= -1.0f;

	float at;
	float a, b, c, ac4, b2, at1, at2;
	float root;
	BOOL neg1, neg2;

	a = DotProd(ray.dir, ray.dir);
	b = DotProd(ray.dir, ray.p) * 2.0f;
	c = DotProd(ray.p, ray.p) - radius*radius;

	ac4 = 4.0f * a * c;
	b2 = b*b;

	if (ac4 > b2) { ray.p += spherecenter; return; }

	// We want the smallest positive root
	root = Sqrt(b2 - ac4);
	at1 = (-b + root) / (2.0f * a);
	at2 = (-b - root) / (2.0f * a);
	neg1 = at1 < 0.0f;
	neg2 = at2 < 0.0f;
	// no intersections
/*	if (neg1 && neg2){ ray.p += spherecenter; return; }
	// 1 intersection
	else if (neg1 && !neg2) at = at2;
	else if (!neg1 && neg2) at = at1;
	// 2 intersections
	else*/ if (at1 < at2) at = at1;
	else at = at2;

	ray.p = spherecenter + ray.p + (at*ray.dir);
}

/*
 * ST - Found this on the Internet at http://www.acm.org/jgt, Adapted it
 *		to replace our BlendVect function. dont ask
 *		how it works...
 *
 * GB - Edited so it will compile.  Probably won't work - see comment
 *      at end of function (16-Jul-03).
 *
 * A function for creating a rotation matrix that rotates a vector called
 * "from" into another vector called "to".
 * Input : from[3], to[3] which both must be *normalized* non-zero vectors
 * Output: mtx[3][3] -- a 3x3 matrix in colum-major form
 * Authors: Tomas Moller, John Hughes
 *          "Efficiently Building a Matrix to Rotate One Vector to Another"
 *          Journal of Graphics Tools, 4(4):1-4, 1999
 */
#define EPSILON 0.000001

Matrix3 fromToRotation(const Point3& from, const Point3& to)
{
	Point3 v;
	float mtx[3][3];
	float e, h, f;

	e = DotProd(from, to);
	v = CrossProd(from, to);

	f = (e < 0) ? -e : e;

	if (f > 1.0 - EPSILON)     /* "from" and "to"-vector almost parallel */
	{
		float u[3], v[3]; /* temporary storage vectors */
		float x[3];       /* vector most nearly orthogonal to "from" */
		float c1, c2, c3; /* coefficients for later use */
		int i, j;

		x[0] = (from[0] > 0.0f) ? from[0] : -from[0];
		x[1] = (from[1] > 0.0f) ? from[1] : -from[1];
		x[2] = (from[2] > 0.0f) ? from[2] : -from[2];

		if (x[0] < x[1])
		{
			if (x[0] < x[2])
			{
				x[0] = 1.0; x[1] = x[2] = 0.0;
			}
			else
			{
				x[2] = 1.0; x[0] = x[1] = 0.0;
			}
		}
		else
		{
			if (x[1] < x[2])
			{
				x[1] = 1.0; x[0] = x[2] = 0.0;
			}
			else
			{
				x[2] = 1.0; x[0] = x[1] = 0.0;
			}
		}

		u[0] = x[0] - from[0]; u[1] = x[1] - from[1]; u[2] = x[2] - from[2];
		v[0] = x[0] - to[0];   v[1] = x[1] - to[1];   v[2] = x[2] - to[2];

		c1 = 2.0f / (u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
		c2 = 2.0f / (v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		c3 = c1 * c2  *(u[0] * v[0] + u[1] * v[1] + u[2] * v[2]);

		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				mtx[i][j] = -c1 * u[i] * u[j]
					- c2 * v[i] * v[j]
					+ c3 * v[i] * u[j];
			}
			mtx[i][i] += 1.0;
		}
	}
	else  /* the most common case, unless "from"="to", or "from"=-"to" */
	{
#if 0
		/* unoptimized version - a good compiler will optimize this. */
		/* h = (1.0 - e)/DOT(v, v); old code */
		h = 1.0 / (1.0 + e);      /* optimization by Gottfried Chen */
		mtx[0][0] = e + h * v[0] * v[0];
		mtx[0][1] = h * v[0] * v[1] - v[2];
		mtx[0][2] = h * v[0] * v[2] + v[1];

		mtx[1][0] = h * v[0] * v[1] + v[2];
		mtx[1][1] = e + h * v[1] * v[1];
		mtx[1][2] = h * v[1] * v[2] - v[0];

		mtx[2][0] = h * v[0] * v[2] - v[1];
		mtx[2][1] = h * v[1] * v[2] + v[0];
		mtx[2][2] = e + h * v[2] * v[2];
#else
		/* ...otherwise use this hand optimized version (9 mults less) */
		float hvx, hvz, hvxy, hvxz, hvyz;
		/* h = (1.0 - e)/DOT(v, v); old code */
		h = 1.0f / (1.0f + e);      /* optimization by Gottfried Chen */
		hvx = h * v[0];
		hvz = h * v[2];
		hvxy = hvx * v[1];
		hvxz = hvx * v[2];
		hvyz = hvz * v[1];
		mtx[0][0] = e + hvx * v[0];
		mtx[0][1] = hvxy - v[2];
		mtx[0][2] = hvxz + v[1];

		mtx[1][0] = hvxy + v[2];
		mtx[1][1] = e + h * v[1] * v[1];
		mtx[1][2] = hvyz - v[0];

		mtx[2][0] = hvxz - v[1];
		mtx[2][1] = hvyz + v[0];
		mtx[2][2] = e + hvz * v[2];
#endif
	}

	// I think Matrix3 is row-major, whereas mtx is column-major.  In that
	// case, swap all indices into mtx, and still call the following...
	return Matrix3(mtx);
}

/*
// This function takes two vectors and performs a linear blend between them
// from Vec1 at ratio 0 to Vec2 at ratio 1.
 */
float BlendFloat(const float &val1, const float &val2, const float &ratio)
{
	return val1 + ((val2 - val1) * ratio);
}

Point3 BlendVector(Point3 Vec1, Point3 Vec2, float ratio)
{
	if (ratio == 1.0f) return Vec2;
	if (ratio == 0.0f) return Vec1;
	/*	if(Vec1 == Vec2) return Vec1;

		Vec1 = Vec1.Normalize();
		Vec2 = Vec2.Normalize();

		if(Vec1 == Vec2)
			return Vec1;

		Point3 RotAxis = Vec1^Vec2;
		double angle = acos(DotProd(Vec1, Vec2));
		double RotAngle = angle * ratio;

		RotAxis = RotAxis.Normalize(); // This line was the cause of so much pain....
		Matrix3 Rot;

		Rot.SetAngleAxis(RotAxis, (float)RotAngle);
		Vec1 = Rot.VectorTransform(Vec1);

		// Added by phil
	//	Vec1 = Vec1.Normalize();

		return Vec1;
	*/
	return (Vec1 += ((Vec2 - Vec1) * ratio));
}

void BlendRot(Matrix3 &tm1, const Matrix3 &tm2, float ratio, BOOL useQuat)
{
	if (ratio == 1.0f)
	{
		// Copy rotation info only
		for (int i = 0; i < 3; i++)
			tm1.SetRow(i, tm2.GetRow(i));
		return;
	}
	else if (ratio == 0.0f) return;

	Point3 pos = tm1.GetTrans();

	if (!(tm1.GetIdentFlags()&ROT_IDENT && tm2.GetIdentFlags()&ROT_IDENT))
	{
		if (useQuat)
		{
			Quat rot1(tm1), rot2(tm2);
			rot1.MakeClosest(rot2);
			rot1 = Slerp(rot1, rot2, ratio);
			rot1.Normalize();
			tm1.SetRotate(rot1);
		}
		else
		{
			Point3 xVect = BlendVector(tm1.GetRow(0), tm2.GetRow(0), ratio); // If result is not orthoganal
			Point3 yVect = BlendVector(tm1.GetRow(1), tm2.GetRow(1), ratio); // xVect can be removed...
			Point3 zVect = BlendVector(tm1.GetRow(2), tm2.GetRow(2), ratio);

			tm1.SetRow(0, xVect);
			tm1.SetRow(1, yVect);
			tm1.SetRow(2, zVect);

			tm1.Orthogonalize();
		}
	}
	tm1.SetTrans(pos);
}

//
// Added Scale to the mix, where scale is applied LOCALLY throughout the blend
//
void BlendMat(Matrix3 &tm1, const Matrix3 &tm2, float ratio)
{
	/*
//	Matrix3 RotFrom = tm1;
//	Matrix3 RotTo = tm2;

	if(ratio == 1.0f) { tm1 = tm2; return; }
	else if(ratio == 0.0f) return;

	Point3 trans1, trans2, diff;
	trans1 = tm1.GetTrans();
	trans2 = tm2.GetTrans();

	if(trans1 != trans2)
	{
		diff = trans2 - trans1;
		trans1 += (diff * ratio);
	}

	Point3 scale1, scale2;

	// Extract the scale from each matrix
	if(tm1.GetIdentFlags()&SCL_IDENT) scale1 = P3_IDENTITY_SCALE;
	else
	{
//		const MRow *rows = tm1.GetAddr();
		scale1[X] = Length(tm1.GetRow(X));
		scale1[Y] = Length(tm1.GetRow(Y));
		scale1[Z] = Length(tm1.GetRow(Z));
	}

	// Extract the scale from each matrix
	if(tm2.GetIdentFlags()&SCL_IDENT) scale1 = P3_IDENTITY_SCALE;
	else
	{
//		const MRow *rows = tm2.GetAddr(); // Does scaled matrix make affect quat?
		scale2[X] = Length(tm2.GetRow(X));
		scale2[Y] = Length(tm2.GetRow(Y));
		scale2[Z] = Length(tm2.GetRow(Z));
	}

	if(scale1 != scale2)
	{
		scale1 += ((scale2 - scale1) * ratio);
	}

	if(!(tm1.GetIdentFlags()&ROT_IDENT && tm2.GetIdentFlags()&ROT_IDENT))
	{
			Quat rot1(tm1), rot2(tm2);
			rot1.MakeClosest(rot2);
			rot1 = Slerp(rot1, rot2, ratio);
			rot1.Normalize();
			tm1.SetRotate(rot1);

	}
	else
		tm1.IdentityMatrix();

	// Apply all the changes
	tm1.PreScale(scale1, FALSE);
	tm1.SetTrans(trans1);
*/
	if (ratio == 1.0f) { tm1 = tm2; return; }
	else if (ratio == 0.0f) return;

	AffineParts parts1;
	decomp_affine(tm1, &parts1);

	AffineParts parts2;
	decomp_affine(tm2, &parts2);

	parts1.t += ((parts2.t - parts1.t) * ratio);

	parts1.q.MakeClosest(parts2.q);
	parts1.q = Slerp(parts1.q, parts2.q, ratio);

	if (parts1.k != parts2.k) {
		parts1.k += ((parts2.k - parts1.k) * ratio);
	}

	tm1.IdentityMatrix();
	tm1.SetRotate(parts1.q);

	// Apply all the changes
	tm1.PreScale(parts1.k, FALSE);

	tm1.SetTrans(parts1.t);
}

void MirrorQuat(Quat &qt, Axis mirrorplane)
{
	Matrix3 tm(1);
	qt.MakeMatrix(tm);

	MirrorMatrix(tm, mirrorplane);
	qt = tm;
	/*	switch(mirrorplane){
		case X:
			qt.x *= -1.0f;
			qt.z *= -1.0f;
			return;
		case Y:
			qt.y *= -1.0f;
			qt.z *= -1.0f;
			return;
		case Z:
			DbgAssert(0);// TODO
			return;
		}
	*/
}

void MirrorAngAxis(AngAxis &ax, int mirrorplane)
{
	//	Matrix3 tm(1);
	//	tm.SetRotate(ax);
	///	MirrorMatrix(tm, mirrorplane, FALSE);
	//	ax = tm;
	switch (mirrorplane) {
	case X:
		ax.axis.z *= -1.0f;
		ax.axis.y *= -1.0f;
		return;
	case Y:
		ax.axis.y *= -1.0f;
		ax.axis.z *= -1.0f;
		return;
	case Z:
		DbgAssert(0);// TODO
		return;
	}

}

extern void MirrorPoint(Point3 &pt, int mirrorplane, BOOL eulerrot)
{
	if (eulerrot) {
		switch (mirrorplane) {
		case X:
			pt.y *= -1.0f;
			pt.z *= -1.0f;
			return;
		case Y:
			pt.x *= -1.0f;
			pt.z *= -1.0f;
			return;
		case Z:
			pt.y *= -1.0f;
			pt.x *= -1.0f;
			return;
		}
	}
	else {
		switch (mirrorplane) {
		case X:
			pt.x *= -1.0f;
			return;
		case Y:
			pt.y *= -1.0f;
			return;
		case Z:
			pt.z *= -1.0f;
			return;
		}
	}
}

//#define worldZ Point3(0,0,1);

void BlendRotByAA(Matrix3 &tm1, const Matrix3 &tm2, float ratio)
{
	// Experimental BlendRot. The idea is to take an normal vector (sum of
	// all vectors) from each matrix, and blend between them. Then, using zVect as
	// a base, measure the angle from the origin (0,0,1) to each matrix, and
	// blend these, then using the orient vector as axis, rotate to final blended orient

//	ratio = max(min(ratio, 1.0f), 0.0f);
	if (ratio < 0) ratio = 0;
	if (ratio > 1) ratio = 1;

	if (ratio == 0) return;
	else if (ratio == 1) { tm1 = tm2; return; }

	// BTW - I Know its not a normal, but it fits and I made it up, so it is now
	Point3 mat1zVect = tm1.GetRow(Z);
	Point3 mat2zVect = tm2.GetRow(Z);
	Point3 mat1Normal = Normalize(tm1.GetRow(X) + tm1.GetRow(Y) + mat1zVect);
	Point3 mat2Normal = Normalize(tm2.GetRow(X) + tm2.GetRow(Y) + mat2zVect);

	Point3 normal1ToZvect = tm1.GetRow(Z) - mat1Normal;

	Point3 normal2ToZvect = tm2.GetRow(Z) - mat2Normal;

	double mat1toMat2angle = acos(DotProd(mat1Normal, mat2Normal)) * ratio;
	AngAxis mat1Rot(Normalize(CrossProd(mat1Normal, mat2Normal)), (float)mat1toMat2angle);

	//	BlendVector(normal1ToZvect, normal2ToZvect, ratio);
	//	double mat1RotateAngle = (normal1Angle - normal2Angle) * ratio;
	//	AngAxis mat1NormalRot(normal1ToZvect, mat1RotateAngle);

	RotateMatrix(tm1, mat1Rot);
	//	PreRotateMatrix(tm1, mat1NormalRot);
}

// given the matrix and an Angle Axis by which to rotate it
// we rotate the matrix will keeping its position in the same place
void RotMat(Matrix3 &tm, const AngAxis &ax) {
	Point3 pos = tm.GetTrans();
	RotateMatrix(tm, ax);
	tm.SetTrans(pos);	// reset the position
}

// This method takes an existing matrix at rotates it until the
// specified axis is looking at the specified point is space.
void RotMatToLookAtPoint(Matrix3 &tm, int axis, BOOL neg, const Point3 &target) {
	Point3 mat_to_target = Normalize(target - tm.GetTrans());
	AngAxis ax;
	if (neg) {
		ax.angle = acos(DotProd(mat_to_target, -tm.GetRow(axis)));
		ax.axis = Normalize(CrossProd(mat_to_target, -tm.GetRow(axis)));
	}
	else {
		ax.angle = acos(DotProd(mat_to_target, tm.GetRow(axis)));
		ax.axis = Normalize(CrossProd(mat_to_target, tm.GetRow(axis)));
	}
	RotMat(tm, ax);
}

// this method takes an existing matrix at rotates it untill its Z-Axis is
// looking at the specified point is space.
void RotateMatrixToAlignWithVector(Matrix3 &tm, const Point3 &vector, int axis) {
	Point3 pos = tm.GetTrans();

	Point3 matrix_vector = tm.GetRow(axis);
	AngAxis ax(Normalize(CrossProd(vector, matrix_vector)), acos(clamp((float)DotProd(vector, matrix_vector), -1.0f, 1.0f)));
	if (ax.angle > 0.0f) {
		RotateMatrix(tm, ax);
	}

	tm.SetTrans(pos);
}

BOOL RemoveTableNode(Tab<INode*> *nodes, INode* node) {
	for (int i = 0; i < nodes->Count(); i++)
	{
		INode* currNode = (*nodes)[i];
		if (currNode == node)
		{
			int j;
			for (j = i; j < (nodes->Count() - 1); j++)
				(*nodes)[j] = (*nodes)[j + 1];
			nodes->SetCount(j);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL MirrorCtrl(Control* ctrl, int mirrorplane)
{
	DbgAssert(ctrl);
	if (ctrl->ClassID().PartA() == PRS_CONTROL_CLASS_ID) {
		MirrorCtrl((Control*)ctrl->GetPositionController(), mirrorplane);
		MirrorCtrl((Control*)ctrl->GetRotationController(), mirrorplane);
		return TRUE;
	}

	if (ctrl->ClassID().PartA() == EULER_CONTROL_CLASS_ID) {
		switch (mirrorplane) {
		case X:
			MirrorCtrl((Control*)ctrl->SubAnim(Y), mirrorplane);
			MirrorCtrl((Control*)ctrl->SubAnim(Z), mirrorplane);
			break;
		case Y:
			MirrorCtrl((Control*)ctrl->SubAnim(X), mirrorplane);
			MirrorCtrl((Control*)ctrl->SubAnim(Z), mirrorplane);
			break;
		case Z:
			MirrorCtrl((Control*)ctrl->SubAnim(Y), mirrorplane);
			MirrorCtrl((Control*)ctrl->SubAnim(X), mirrorplane);
			break;
		}
		return TRUE;
	}
	if (ctrl->ClassID() == IPOS_CONTROL_CLASS_ID) {
		switch (mirrorplane) {
		case X: 	MirrorCtrl((Control*)ctrl->SubAnim(X), mirrorplane);	break;
		case Y: 	MirrorCtrl((Control*)ctrl->SubAnim(Y), mirrorplane);	break;
		case Z:		MirrorCtrl((Control*)ctrl->SubAnim(X), mirrorplane);	break;
		}
		return TRUE;
	}

	IKeyControl *ikeys = GetKeyControlInterface(ctrl);
	if (!ikeys)	return FALSE;
	int numKeys = ikeys->GetNumKeys();
	for (int i = 0; i < numKeys; i++) {
		switch (ctrl->ClassID().PartA()) {
		case TCBINTERP_POSITION_CLASS_ID: {
			ITCBPoint3Key *key = NULL;
			ikeys->GetKey(i, key);
			if (key)	MirrorPoint(key->val, mirrorplane, FALSE);
			break;
		}
		case LININTERP_POSITION_CLASS_ID: {
			ILinPoint3Key *key = NULL;
			ikeys->GetKey(i, key);
			if (key)	MirrorPoint(key->val, mirrorplane, FALSE);
			break;
		}
		case HYBRIDINTERP_POSITION_CLASS_ID: {
			IBezPoint3Key *key = NULL;
			ikeys->GetKey(i, key);
			if (key) {
				MirrorPoint(key->val, mirrorplane, FALSE);
				MirrorPoint(key->intan, mirrorplane, FALSE);
				MirrorPoint(key->outtan, mirrorplane, FALSE);
			}
			break;
		}
		case TCBINTERP_ROTATION_CLASS_ID: {
			ITCBRotKey *key = NULL;
			ikeys->GetKey(i, key);
			if (key) MirrorAngAxis(key->val, mirrorplane);
		}
		case LININTERP_ROTATION_CLASS_ID: {
			ILinRotKey *key = NULL;
			ikeys->GetKey(i, key);
			if (key) {
				AngAxis ax(key->val);
				MirrorAngAxis(ax, mirrorplane);
				key->val = Quat(ax);
			}
		}
		}
	}
	return TRUE;
}

void WriteControlToUserProps(INode* node, Control* ctrl, TSTR keyname)
{
	if (!ctrl) return;

	TSTR key, val;
	Tab<TimeValue> times;
	float keyvalue;
	Interval range = GetCOREInterface()->GetAnimRange();
	DWORD flags = KEYAT_POSITION | KEYAT_ROTATION | KEYAT_SCALE;
	ctrl->GetKeyTimes(times, range, flags);

	Interval iv = FOREVER;
	key.printf(_T("CATProp_Num%sKeys"), keyname);
	if (times.Count() > 0) {
		// write the number of keys
		node->SetUserPropInt(key, times.Count());

		int tps = GetTicksPerFrame() * GetFrameRate();
		key.printf(_T("CATProp_%sStartTime"), keyname);	node->SetUserPropFloat(key, (float)range.Start() / (float)tps);
		key.printf(_T("CATProp_%sEndTime"), keyname);	node->SetUserPropFloat(key, (float)range.End() / (float)tps);

		for (int i = 0; i < times.Count(); i++) {
			key.printf(_T("CATProp_%sKey%i"), keyname, i);
			iv = FOREVER;
			ctrl->GetValue(times[i], (void*)&keyvalue, iv, CTRL_ABSOLUTE);
			val.printf(_T("%f %f"), (float)times[i] / (float)tps, keyvalue);
			node->SetUserPropString(key, val);
		}
	}
	else {// a minimum of 1 key
		node->SetUserPropInt(key, 1);
		key.printf(_T("CATProp_%sKey%i"), keyname, 0);
		iv = FOREVER;
		ctrl->GetValue(0, (void*)&keyvalue, iv, CTRL_ABSOLUTE);
		val.printf(_T("%f %f"), 0.0f, keyvalue);
		node->SetUserPropString(key, val);
	}
}

BOOL MAXScriptEvaluate(Interface* ip, TCHAR *s, ReferenceTarget *this_ref, int i)
{
	ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
	MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
	BOOL result=TRUE; 

	six_typed_value_locals_tls(Parser* parser,
		Value* this_val,
		Value* i_val,
		Value* code,
		Value* result,
		StringStream* source);

	try
	{
		ScopedSaveCurrentFrames scopedSaveCurrentFrames(_tls);
		vl.source	= new StringStream (s); 
		vl.source->log_to(_tls->current_stdout); 
		vl.parser	= new Parser (_tls->current_stdout); 
		vl.this_val = MAXClass::make_wrapper_for(this_ref);
		vl.i_val = Integer::intern(i);
		ip->RedrawViews(ip->GetTime(), REDRAW_BEGIN);

		vl.source->flush_whitespace();

		vl.code = vl.parser->compile_all(vl.source);
		vl.result = vl.code->eval();

		ip->RedrawViews(ip->GetTime(), REDRAW_END);
	}
	catch (MAXScriptException& e)
	{
		result = FALSE;
		ProcessMAXScriptException(e, _T("MAXScriptEvaluate Script"), false, false, true);
	}
	catch (...)
	{
		result = FALSE;
		ProcessMAXScriptException(UnknownSystemException(), _T("MAXScriptEvaluate Script"), false, false, true);
	}
	vl.source->close();
	return result;
}

// Methods for the subobject gizmo
extern inline void dLine2Pts(GraphicsWindow *gw, Point3& r, Point3& q) {
	Point3 s[3];
	s[0] = r; s[1] = q;
	gw->polyline(2, s, NULL, NULL, FALSE, NULL);
}

void DrawCross(GraphicsWindow *gw, Matrix3 tm, const float &size, Box3 &bbox)
{
	Point3 p1, p2;

	Matrix3 tmX = tm;
	tmX.PreTranslate(Point3(-size / 2.0f, 0.0f, 0.0f));	p1 = tmX.GetTrans();
	tmX.PreTranslate(Point3(size, 0.0f, 0.0f));			p2 = tmX.GetTrans();
	dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

	Matrix3 tmY = tm;
	tmY.PreTranslate(Point3(0.0f, -size / 2.0f, 0.0f));	p1 = tmY.GetTrans();
	tmY.PreTranslate(Point3(0.0f, size, 0.0f));			p2 = tmY.GetTrans();
	dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

	Matrix3 tmZ = tm;
	tmZ.PreTranslate(Point3(0.0f, 0.0f, -size / 2.0f));	p1 = tmZ.GetTrans();
	tmZ.PreTranslate(Point3(0.0f, 0.0f, size));			p2 = tmZ.GetTrans();
	dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;
}

extern inline void DrawArc10Segs(GraphicsWindow *gw, Matrix3 tm, const Point3 &axis, const float &degstart, const float &degend, const Point3 &spoke, Box3 &bbox, bool markers_at_ends, MarkerType marker_type)
{
	const int numsegs = 10;
	Point3 s[numsegs + 1];
	Point3 pos = tm.GetTrans();

	AngAxis ax;

	// I am getting the drawing flipped in the viewport. I think this might be a difference
	// between right handed and left handed representions in rotations.
	ax.axis = -axis;
	ax.angle = DegToRad(degstart);
	// Rotate the matrix bacwards to point at the start of our arc.
	RotateMatrix(tm, ax);
	ax.angle = DegToRad(degend - degstart) / (float)numsegs;

	s[0] = (pos + tm.VectorTransform(spoke));
	// rotate the matrix forwards through our arc.
	for (int i = 1; i <= numsegs; i++) {
		RotateMatrix(tm, ax);
		s[i] = (pos + tm.VectorTransform(spoke));
		bbox += s[i];
	}
	gw->polyline(numsegs + 1, s, NULL, NULL, FALSE, NULL);

	if (markers_at_ends) {
		gw->marker(&s[0], marker_type);
		gw->marker(&s[numsegs], marker_type);
	}
}

// blr = bottom left rear
// trf = top right front
extern inline void DrawBox(GraphicsWindow *gw, Matrix3 tm, const Point3 &blr, const Point3 &trf, Box3 &bbox) {
	const int numlines = 16;
	Point3 s[numlines + 1];
	int es[numlines + 1];

	tm.PreTranslate(blr);
	s[0] = tm.GetTrans();	bbox += s[0];

	// Draw the bottom square
	tm.PreTranslate(Point3(trf.x - blr.x, 0.0f, 0.0f));		s[1] = tm.GetTrans();	es[0] = GW_EDGE_VIS;
	tm.PreTranslate(Point3(0.0f, trf.y - blr.y, 0.0f));		s[2] = tm.GetTrans();	es[1] = GW_EDGE_VIS;
	tm.PreTranslate(Point3(blr.x - trf.x, 0.0f, 0.0f));		s[3] = tm.GetTrans();	es[2] = GW_EDGE_VIS;
	tm.PreTranslate(Point3(0.0f, blr.y - trf.y, 0.0f));		s[4] = tm.GetTrans();	es[3] = GW_EDGE_VIS;

	// Move to the top
	tm.PreTranslate(Point3(0.0f, 0.0f, trf.z - blr.z));		s[5] = tm.GetTrans();	es[4] = GW_EDGE_VIS;	//Vertical

	// Draw the top square
	tm.PreTranslate(Point3(trf.x - blr.x, 0.0f, 0.0f));		s[6] = tm.GetTrans();	es[5] = GW_EDGE_VIS;
	tm.PreTranslate(Point3(0.0f, trf.y - blr.y, 0.0f));		s[7] = tm.GetTrans();	es[6] = GW_EDGE_VIS;	bbox += s[7];
	tm.PreTranslate(Point3(blr.x - trf.x, 0.0f, 0.0f));		s[8] = tm.GetTrans();	es[7] = GW_EDGE_VIS;
	tm.PreTranslate(Point3(0.0f, blr.y - trf.y, 0.0f));		s[9] = tm.GetTrans();	es[8] = GW_EDGE_VIS;

	tm.PreTranslate(Point3(trf.x - blr.x, 0.0f, 0.0f));		s[10] = tm.GetTrans();	es[9] = GW_EDGE_SKIP;
	tm.PreTranslate(Point3(0.0f, 0.0f, blr.z - trf.z));		s[11] = tm.GetTrans();	es[10] = GW_EDGE_VIS;	//Vertical
	tm.PreTranslate(Point3(0.0f, trf.y - blr.y, 0.0f));		s[12] = tm.GetTrans();	es[11] = GW_EDGE_SKIP;
	tm.PreTranslate(Point3(0.0f, 0.0f, trf.z - blr.z));		s[13] = tm.GetTrans();	es[12] = GW_EDGE_VIS;	//Vertical
	tm.PreTranslate(Point3(blr.x - trf.x, 0.0f, 0.0f));		s[14] = tm.GetTrans();	es[13] = GW_EDGE_SKIP;
	tm.PreTranslate(Point3(0.0f, 0.0f, blr.z - trf.z));		s[15] = tm.GetTrans();	es[14] = GW_EDGE_VIS;	//Vertical

	gw->polyline(numlines, s, NULL, NULL, 0, es);
}

//////////////////////////////////////////////////////////////////////////
// The following functions are used by CATMuscle

Point3 DetectCol(Point3 p3Up, INode *node, Interval &valid, Point3 point, TimeValue t, float distortion, float hardness, int flags)
{
	if (node == NULL)
		return point;

	Object* obj = node->EvalWorldState(t, TRUE).obj;
	Point3 hitpoint = point;

	Matrix3 tm = node->GetObjectTM(t, &valid);
	Matrix3 tmInv = Inverse(tm);
	Point3 point_in_node_space = point * tmInv;
	tmInv.NoTrans();
	p3Up = p3Up * tmInv;
	Point3 hitpoint_Vec = point_in_node_space, hitpoint_Distorted = point_in_node_space;

	// collisions with spheres
	if (obj->ClassID() == Class_ID(SPHERE_CLASS_ID, 0)) {
		IParamBlock2* sphereParams = obj->GetParamBlockByID(SPHERE_PARAMBLOCK_ID);
		DbgAssert(sphereParams);
		if (sphereParams == nullptr)
			return point;
		float radius;
		sphereParams->GetValue(SPHERE_RADIUS, t, radius, valid);

		if (distortion > 0.0f)
		{
			// Test for collision
			float dDistPointFromSphere = Length(point_in_node_space);
			if (dDistPointFromSphere > radius) hitpoint_Distorted = point_in_node_space; // No Collision
			else {
				// this is one method of collision detection that simply moves
				// the point away from the spheres center.
				hitpoint_Distorted = point_in_node_space * (radius / dDistPointFromSphere);
			}
		}
		if (distortion < 1.0f)
		{
			float at;
			Ray ray;
			ray.p = point_in_node_space;
			ray.dir = p3Up;
			if (flags&COLLISIONFLAG_INVERT) ray.dir *= -1.0f;

			float a, b, c, ac4, b2, at1, at2;
			float root;
			BOOL neg1, neg2;

			a = DotProd(ray.dir, ray.dir);
			b = DotProd(ray.dir, ray.p) * 2.0f;
			c = DotProd(ray.p, ray.p) - radius*radius;

			ac4 = 4.0f * a * c;
			b2 = b*b;

			if (ac4 > b2) return point;

			// We want the smallest positive root
			root = Sqrt(b2 - ac4);
			at1 = (-b + root) / (2.0f * a);
			at2 = (-b - root) / (2.0f * a);

			neg1 = at1 < 0.0f;
			neg2 = at2 < 0.0f;
			// no intersections
			if (neg1 && neg2) return point;
			// 1 intersection
			else if (neg1 && !neg2) at = at2;
			else if (!neg1 && neg2) at = at1;
			// 2 intersections
			// choose the furtherest away point
			else if (at1 < at2) at = at2;
			else at = at1;

			Point3 norm = Normalize(ray.p + at*ray.dir);
			hitpoint_Vec = ray.p + at*ray.dir;

			if (flags&COLLISIONFLAG_SMOOTH)
				hitpoint_Vec = ray.p + ((hitpoint_Vec - ray.p) * (float)fabs(DotProd(norm, Normalize(ray.dir))));
		}

		hitpoint = BlendVector(hitpoint_Vec, hitpoint_Distorted, distortion);
	}
	// use the generic intersect ray method.
	// No doubt extremely expensive, but who cares...
	else {
		Ray ray;
		ray.p = point_in_node_space;
		float at;
		Point3 norm;

		Box3 bbox;
		obj->GetDeformBBox(t, bbox, &tm);

		if (distortion > 0.0f) {
			//	ray.dir = (ray.p - tm.GetTrans());// * tmInv;
			// fire a ray from the objects pivot towards the current vertexes pos
			ray.dir = ray.p;
			if (flags&COLLISIONFLAG_INVERT) ray.dir *= -1.0f;

			if (obj->IntersectRay(t, ray, at, norm))
				hitpoint_Distorted = ray.p + at*ray.dir;
		}
		if (distortion < 1.0f) {
			ray.dir = p3Up;
			//	if(ray.dir.x < 0.0f && ray.p.x < bbox) // going in the -x dir
			//		ray.p.x = fmax(ray.p.x, bbox);
			if (flags&COLLISIONFLAG_INVERT) ray.dir *= -1.0f;

			if (obj->IntersectRay(t, ray, at, norm)) {
				hitpoint_Vec = ray.p + at*ray.dir;

				if (flags&COLLISIONFLAG_SMOOTH)
					hitpoint_Vec = ray.p + ((hitpoint_Vec - ray.p) * (float)fabs(DotProd(norm, Normalize(ray.dir))));
			}
		}
		hitpoint = BlendVector(hitpoint_Vec, hitpoint_Distorted, distortion);
	}
	// Reverse the process, moving the new point out into world space again
	hitpoint = hitpoint * tm;

	hitpoint = point + ((hitpoint - point) * hardness);
	return hitpoint;
}

Point3 CalcBezier(const Point3 &p0, const Point3 &p1, const Point3 &p2, const Point3 &p3, const float &mu, const Point3 *p4) {

	if (mu <= 0.0f)return p0;
	else if (mu >= 1.0f)return p3;

	float mum1, mum12, mum13, mu3, mu2;
	Point3 p;

	mu2 = mu * mu;
	mu3 = mu * mu * mu;

	mum1 = 1 - mu;
	mum12 = mum1 * mum1;
	mum13 = mum1 * mum1 * mum1;

	if (p4) {
		// if we were to have 5 control points
		float mu4 = mu * mu * mu * mu;
		float mum14 = mum12 * mum12;

		// p is the position based on the interpolation of 5 other positions
		p.x = (p0.x * mum14) + (3 * p1.x * mum13 * mu) + (3 * p2.x * mum12 * mu2) + (3 * p3.x * mum1 * mu3) + (p4->x * mu4);
		p.y = (p0.y * mum14) + (3 * p1.y * mum13 * mu) + (3 * p2.y * mum12 * mu2) + (3 * p3.y * mum1 * mu3) + (p4->y * mu4);
		p.z = (p0.z * mum14) + (3 * p1.z * mum13 * mu) + (3 * p2.z * mum12 * mu2) + (3 * p3.z * mum1 * mu3) + (p4->z * mu4);
	}
	else {
		// p is the position based on the interpolation of 4 other positions
		p.x = (p0.x * mum13) + (3 * p1.x * mum12 * mu) + (3 * p2.x * mum1 * mu2) + (p3.x * mu3);
		p.y = (p0.y * mum13) + (3 * p1.y * mum12 * mu) + (3 * p2.y * mum1 * mu2) + (p3.y * mu3);
		p.z = (p0.z * mum13) + (3 * p1.z * mum12 * mu) + (3 * p2.z * mum1 * mu2) + (p3.z * mu3);
	}
	return p;
}

#define DELTA 0.01f
INode* FindNodeAtPos(INode* node, Point3 p, TimeValue t) {
	if (!node->IsRootNode()) {
		if (Length(node->GetNodeTM(t).GetTrans() - p) < DELTA)
			return node;
	}
	INode* foundnode = NULL;
	for (int i = 0; i < node->NumberOfChildren(); i++) {
		foundnode = FindNodeAtPos(node->GetChildNode(i), p, t);
		if (foundnode) return foundnode;
	}
	return NULL;
}

// Get the old nodes parent, and find the equivalent node on the other side of the scene
void ReparentNode(INode *oldnode, INode* newnode, Axis mirroraxis) {
	if (!oldnode) return;
	INode* parentNode = oldnode->GetParentNode();
	TimeValue t = GetCOREInterface()->GetTime();
	if (!parentNode->IsRootNode()) {
		Matrix3 partm = parentNode->GetNodeTM(t);
		MirrorMatrix(partm, mirroraxis);
		INode* newnodeparent = FindNodeAtPos(GetCOREInterface()->GetRootNode(), partm.GetTrans(), t);
		if (newnodeparent) {
			newnodeparent->AttachChild(newnode, FALSE);
		}
	}
}
