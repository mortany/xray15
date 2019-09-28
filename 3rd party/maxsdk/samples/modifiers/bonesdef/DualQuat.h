/**********************************************************************
 *<
	FILE: DualQuat.h

	DESCRIPTION:  Declaration of the DualQuat class

	CREATED BY: Michael Zyracki

 *>	Copyright (c) 2009, All Rights Reserved.
 **********************************************************************/

#ifndef __DUALQUAT_H_
#define __DUALQUAT_H_

#include "quat.h"

class DualQuat: public MaxHeapOperators {
public:
	Quat mReal;
    Quat mDual;

	// Constructors
	/*! \remarks Constructor. No initialization is performed. */
	DualQuat(){}

	DualQuat(const DualQuat &val){mReal = val.mReal; mDual = val.mDual;}
	/*! \remarks Constructor. The data members are initialized to the values
	passed. */
	DualQuat(const Quat &real,const Quat &dual)  { mReal = real; mDual = dual; }
	/*! \remarks Constructor. Convert the specified 3x3 rotation matrix to a
	unit quaternion. */
	DualQuat(const Matrix3& mat);

	DualQuat(Quat &quat,Point3 &pos);
	/*! \remarks Constructor. The Quat is initialized to the AngAxis passed. */


	// Access operators
	Quat Real(){return mReal;}
	Quat Dual(){return mDual;}

	float RealScalar() { return mReal.w; }
	Point3 RealVector() { return Point3(mReal.x, mReal.y, mReal.z); }

	float DualScalar() { return mDual.w; }
	Point3 DualVector() { return Point3(mDual.x, mDual.y, mDual.z); }


	// Unary operators
	/*! \remarks Unary negation. */
	DualQuat operator-() const { return DualQuat(-mReal,-mDual); } 
	/*! \remarks Unary +. Returns the DualQuat unaltered.\n\n
	Assignment operators */
	DualQuat operator+() const { return *this; }

	// Math functions
	float Norm() const;
	/*! \remarks Returns the inverse of this dual quaternion (1/q). */
	DualQuat Inverse() const;
	/*! \remarks Returns the conjugate of a dual quaternion. */
	DualQuat Conjugate() const;
	/*! \remarks Returns the natural logarithm of a UNIT dual quaternion. */
	DualQuat LogN() const;
	/*! \remarks Returns the exponentiate quaternion (where <b>q.w</b>==0).
	\par Operators:
	*/
	DualQuat Exp() const;

	// Assignment operators
	/*! \remarks Adds this dual quaternion by a dual quaternion. */
	DualQuat& operator+=(const DualQuat&);
	/*! \remarks Multiplies this dual quaternion by a dual quaternion. */
	DualQuat& operator*=(const DualQuat&);
	/*! \remarks Multiplies this dual quaternion by a floating point value. */
	DualQuat& operator*=(float);
	/*! \remarks Divides this dual quaternion by a floating point value. */
	DualQuat& operator/=(float);

	DualQuat Plus(const DualQuat & val) const;

//	Quat& Set(const Matrix3& mat);
//	Quat& Set(const AngAxis& aa);
//	Quat& SetEuler(float X, float Y, float Z);
	Quat& Invert();                 // in place

	/*! \remarks Modifies <b>q</b> so it is on same side of hypersphere as
	<b>qto</b>. */
	DualQuat& MakeClosest(const DualQuat& qto);

	// Comparison
	/*! \remarks Returns nonzero if the quaternions are equal; otherwise 0. */
	int operator==(const DualQuat& a)const {return (mReal==a.mReal&&mDual==a.mDual);};
	int Equals(const DualQuat& a, float epsilon = 1E-6f)const{return (mReal.Equals(a.mReal,epsilon)&& mDual.Equals(a.mDual,epsilon));};

	/*! \remarks Sets this quaternion to the identity quaternion (<b>x=y=z=0.0;
	w=1.0</b>). */
	void Identity() { mReal.Identity(); mDual.Identity();}
	/*! \remarks Returns nonzero if the quaternion is the identity; otherwise
	0. */
	int IsIdentity()const {return (mReal.IsIdentity()&&mDual.IsIdentity());};
	/*! \remarks Normalizes this quaternion, dividing each term by a scale
	factor such that the resulting sum or the squares of all parts equals unity.
	*/
	void Normalize();  // normalize


	void MakeMatrix(Matrix3 &mat)const;
	Matrix3 MakeMatrix3()const;
	void MakeQuatPos(Quat &quat, Point3 &pos)const;

	void SetFromMatrix(const Matrix3& mat);
	void SetFromQuatPos(Quat &quat,Point3 &pos);

	// Binary operators
	/*! \remarks Returns the product of two dual quaternions. */
	DualQuat operator*(const DualQuat&) const;  // product of two dual quaternions
	/*! \remarks Returns the ratio of two quaternions: This creates a result
	quaternion r = p/q, such that q*r = p. (Order of multiplication is
	important) */
	DualQuat operator/(const DualQuat&) const;  // ratio of two dual quaternions
	float operator%(const DualQuat&) const;   // dot product
};

#endif //__DUALQUAT_H_
