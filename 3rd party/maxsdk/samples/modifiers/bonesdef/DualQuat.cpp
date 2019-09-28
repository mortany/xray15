/**********************************************************************
 *<
	FILE: DualQuat.cpp

	DESCRIPTION:  Implementation of the DualQuat class

	CREATED BY: Michael Zyracki

 *>	Copyright (c) 2009, All Rights Reserved.
 **********************************************************************/

#include <math.h>
#define IMPORTING
#include "geom.h"
#include "DualQuat.h"

DualQuat::DualQuat(Quat &quat,Point3 &pos)
{
	SetFromQuatPos(quat,pos);
}

DualQuat::DualQuat(const Matrix3& mat)
{
	SetFromMatrix(mat);	
}


void DualQuat::SetFromMatrix(const Matrix3 &mat)
{
	Matrix3 temp = mat;
	temp.NoScale();
	Point3 pos = mat.GetTrans();
	Quat quat(mat);
	SetFromQuatPos(quat,pos);
}

void DualQuat::SetFromQuatPos(Quat &quat, Point3 &pos) 
{
	mReal = quat;
	mDual.x = pos.x;  mDual.y = pos.y;  mDual.z = pos.z;
	mDual.w = 0.0f;
	mDual = mReal*mDual;
	mDual *= 0.5f;
}

void DualQuat::MakeMatrix(Matrix3 &mat)const
{
	Quat quat;
	Point3 pos;
	MakeQuatPos(quat,pos);
	mat.SetRotate(quat);
	mat.SetTrans(pos);
}

Matrix3 DualQuat::MakeMatrix3()const
{
	Matrix3 mat;
	MakeMatrix(mat);
	return mat;
}

void DualQuat::MakeQuatPos(Quat &quat, Point3 &pos)const
{
	quat = mReal;
	Quat posQuat = mDual;
	posQuat = mReal.Conjugate() *posQuat;
	pos.x = posQuat.x*2.0f;
	pos.y = posQuat.y*2.0f;
	pos.z = posQuat.z*2.0f;
}

float DualQuat::Norm() const
{
	float realNorm = mReal.x*mReal.x + mReal.y*mReal.y + mReal.z*mReal.z + mReal.w*mReal.w;
	if(realNorm<1e-4f&&realNorm>-1e-4)
		realNorm = 1.0f; //otherwise we would crash possibly.

	float dotProd = mReal % mDual;
	float dualNorm = dotProd/realNorm;
	return realNorm+dualNorm;

}

void DualQuat::Normalize()
{
	float len = mReal % mReal;
	if ( len > 1.0e-9f )
	{
		len = 1.0f/sqrtf( len );
		mReal *= len;
		mDual *= len;
	}
}

DualQuat DualQuat::Inverse() const
{
	float norm = Norm();
	norm *=norm; //square the norm;
	DualQuat sol;

	sol.mReal = mReal.Conjugate();
	sol.mDual = mDual.Conjugate();
	sol.mReal /= norm;
	sol.mDual /= norm;
	return sol;
}
DualQuat DualQuat::Conjugate() const
{
	DualQuat sol;
	sol.mReal = mReal.Conjugate();
	sol.mDual = mDual.Conjugate();
	return sol;
}
DualQuat DualQuat::LogN() const
{
//todo
	return DualQuat();
}
DualQuat DualQuat::Exp() const
{
//todo
	return DualQuat();
}

DualQuat& DualQuat::operator+=(const DualQuat& val)
{
	DualQuat sol = val;
	mReal = sol.mReal.Plus(mReal);
	mDual = sol.mDual.Plus(mDual);
	return *this;
}

DualQuat DualQuat::Plus(const DualQuat & val) const
{
	DualQuat sol(val);
	sol.mReal = sol.mReal.Plus(mReal);
	sol.mDual = sol.mDual.Plus(mDual);
	return sol;
}
DualQuat& DualQuat::operator*=(const DualQuat& val)
{
	DualQuat sol = (*this) * val;
	*this = sol;
	return *this;
}
DualQuat& DualQuat::operator*=(float val)
{
	mReal *=val;
	mDual *=val;
	return *this;

}
DualQuat& DualQuat::operator/=(float val)
{
	mReal /=val;
	mDual /=val;
	return *this;

}

DualQuat DualQuat::operator*(const DualQuat &val) const  // product of two dual quaternions
{
	DualQuat sol;
	sol.mReal = mReal  * val.mReal;
	sol.mDual = mReal  * val.mDual;
	sol.mDual.Plus(mDual* val.mReal);
	return sol;
}
//DualQuat DualQuat::operator/(const DualQuat&) const;  // ratio of two dual quaternions
//float DualQuat::operator%(const DualQuat::Quat&) const;   // dot product
