// Copyright (c) 2014 Autodesk, Inc.
// All rights reserved.
// 
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.

#pragma once

#include "GeomExport.h"
#include "maxheap.h"
#include "point3.h"
#include "assert1.h"
#include <iosfwd>

/*! class DPoint3
 Description:
This class describes a 3D point using double precision x, y and z coordinates.
Methods are provided to add and subtract points, multiply and divide by
scalars, and element by element multiply and divide two points. All methods are
implemented by the system.
 Data Members:
double x,y,z;  */
class DPoint3: public MaxHeapOperators {
public:
	double x,y,z;

    //! Initializes all vector components to zero.
	DPoint3() : x(0.0), y(0.0), z(0.0) {}
	/*!  Constructor. x, y, and z are initialized to the values
	specified. */
	DPoint3(double X, double Y, double Z)  { x = X; y = Y; z = Z;  }
	/*!  Constructor. x, y, and z are initialized to the DPoint3
	specified. */
	DPoint3(const DPoint3& a) { x = a.x; y = a.y; z = a.z; } 
	DPoint3(const Point3& a) { x = a.x; y = a.y; z = a.z; } 
	/*!  Constructor. x, y, and z are initialized to. af[0], af[1],
	and af[2] respectively.
	 Operators:
	*/
	DPoint3(const double af[3]) { x = af[0]; y = af[1]; z = af[2]; }

	// Access operators
	/*!  Allows access to x, y and z using the subscript operator.
	\return  An index of 0 will return x, 1 will return y, 2 will return z. */
	double& operator[](int i) { DbgAssert((i >= 0) && (i <= 2)); return (&x)[i]; }
	/*!  Allows access to x, y and z using the subscript operator.
	\return  An index of 0 will return x, 1 will return y, 2 will return z. */
	const double& operator[](int i) const { DbgAssert((i >= 0) && (i <= 2)); return (&x)[i]; }

 	// Conversion function
	/*!  Conversion function. Returns the address of the DPoint3.x */
	operator double*() { return(&x); }
	/*! Convert DPoint3 to Point3 */
	operator Point3() { return Point3(x, y, z); }

 	// Unary operators
	/*!  Unary - operator. Negates both x, y and z. */
	DPoint3 operator-() const { return(DPoint3(-x,-y,-z)); } 
	/*!  Unary +. Returns the point unaltered. */
	DPoint3 operator+() const { return *this; } 

	// Assignment operators
	GEOMEXPORT DPoint3& operator=(const Point3& a) {	x = a.x; y = a.y; z = a.z;	return *this; }
	/*!  Subtracts a DPoint3 from this DPoint3. */
	DPoint3& operator-=(const DPoint3&);
	/*!  Adds a DPoint3 to this DPoint3. */
	DPoint3& operator+=(const DPoint3&);
    //! Member-wise multiplication of two vectors: (x*x, y*y, z*z)
    DPoint3& operator*=(const DPoint3&);
    /*!  Each element of this DPoint3 is multiplied by the specified
	double. */
	DPoint3& operator*=(double);
	/*!  Each element of this DPoint3 is divided by the specified
	double. */
	DPoint3& operator/=(double);

	// Binary operators
	/*!  Subtracts a DPoint3 from a DPoint3. */
	DPoint3 operator-(const DPoint3&) const;
	/*!  Adds a DPoint3 to a DPoint3. */
	DPoint3 operator+(const DPoint3&) const;
    //! Member-wise multiplication of two vectors: (x*x, y*y, z*z)
	DPoint3 operator*(const DPoint3&) const;
	/*!  Computes the cross product of this DPoint3 and the specified
	DPoint3. */
	GEOMEXPORT DPoint3 operator^(const DPoint3&) const;

	    
	/*!  Returns the 'Length' of this point (vector). This is:\n\n
	sqrt(v.x*v.x+v.y*v.y+v.z*v.z) */
    GEOMEXPORT double Length() const;

	/*!  The 'Length' squared of this point. This is
	 v.x*v.x+v.y*v.y+v.z*v.z. */
	GEOMEXPORT double LengthSquared() const;

	/*!  in place normalize */
	GEOMEXPORT DPoint3& Unify();

	/*! \return The largest axis */
	GEOMEXPORT int MaxComponent() const;
	/*! \return The smallest axis */
	GEOMEXPORT int MinComponent() const;


	// Test for equality
   /*!  Equality operator. Test for equality between two Point3's.
   \return  Nonzero if the Point3's are equal; otherwise 0. */
   GEOMEXPORT int operator==(const DPoint3& p) const { 
		 return ((p.x==x)&&(p.y==y)&&(p.z==z)); 
	 }
   /*!  Equality operator. Test for nonequality between two Point3's.
   \return  Nonzero if the Point3's are not equal; otherwise 0. */
   GEOMEXPORT int operator!=(const DPoint3& p) const { 
		 return ((p.x!=x)||(p.y!=y)||(p.z!=z)); 
	 }

	// Property functions
	// Data members
   /*!  const for (0,0,0) */
	GEOMEXPORT static const DPoint3 Origin;
	/*!  const for (1,0,0) */
	GEOMEXPORT static const DPoint3 XAxis;
	/*!  const for (0,1,0) */
	GEOMEXPORT static const DPoint3 YAxis;
	/*!  const for (0,0,1) */
	GEOMEXPORT static const DPoint3 ZAxis;


	};

/*!  Returns the 'Length' of the point. This is
sqrt(v.x*v.x+v.y*v.y+v.z*v.z) */
GEOMEXPORT double Length(const DPoint3&); 
/*!  Returns the component with the maximum absolute value. 0=x, 1=y,
2=z. */
GEOMEXPORT int MaxComponent(const DPoint3&);  // the component with the maximum abs value
/*!  Returns the component with the minimum absolute value. 0=x, 1=y,
2=z. */
GEOMEXPORT int MinComponent(const DPoint3&);  // the component with the minimum abs value
/*!  Returns a unit vector. This is a DPoint3 with each component
divided by the point Length(). */
GEOMEXPORT DPoint3 Normalize(const DPoint3&); // Return a unit vector.

GEOMEXPORT DPoint3 operator*(double, const DPoint3&);	// multiply by scalar
GEOMEXPORT DPoint3 operator*(const DPoint3&, double);	// multiply by scalar
GEOMEXPORT DPoint3 operator/(const DPoint3&, double);	// divide by scalar


GEOMEXPORT std::ostream &operator<<(std::ostream&, const DPoint3&); 
	 
// Inlines:

inline double Length(const DPoint3& v) {	
	return (double)sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
	}

inline DPoint3& DPoint3::operator-=(const DPoint3& a) {	
	x -= a.x;	y -= a.y;	z -= a.z;
	return *this;
	}

inline DPoint3& DPoint3::operator+=(const DPoint3& a) {
	x += a.x;	y += a.y;	z += a.z;
	return *this;
	}

inline DPoint3& DPoint3::operator*=(const DPoint3& rhs) 
{
    x *= rhs.x;
    y *= rhs.y;
    z *= rhs.z;
    return *this;
}

inline DPoint3& DPoint3::operator*=(double f) {
	x *= f;   y *= f;	z *= f;
	return *this;
	}

inline DPoint3& DPoint3::operator/=(double f) { 
	DbgAssert(f != 0.0);
	double invF = 1.0 / f;	// Mimic 2019 behavior
	x *= invF;	y *= invF;	z *= invF;
	return *this; 
	}

inline DPoint3 DPoint3::operator-(const DPoint3& b) const {
	return(DPoint3(x-b.x,y-b.y,z-b.z));
	}

inline DPoint3 DPoint3::operator+(const DPoint3& b) const {
	return(DPoint3(x+b.x,y+b.y,z+b.z));
	}

inline DPoint3 DPoint3::operator*(const DPoint3& b) const {  
	return DPoint3(x * b.x, y * b.y, z * b.z);	
	}

inline DPoint3 operator*(double f, const DPoint3& a) {
	return(DPoint3(a.x*f, a.y*f, a.z*f));
	}

inline DPoint3 operator*(const DPoint3& a, double f) {
	return(DPoint3(a.x*f, a.y*f, a.z*f));
	}

inline DPoint3 operator/(const DPoint3& a, double f) {
	DbgAssert(f != 0.0);
	double invF = 1.0 / f;	// Mimic 2019 behavior
	return(DPoint3(a.x * invF, a.y * invF, a.z * invF));
	}

// Helper for converting DPoint3 to Point3
inline Point3 Point3FromDPoint3(const DPoint3& from)
{
	return Point3(from.x, from.y, from.z);
}

/*!  Returns the cross product of two DPoint3s. */
GEOMEXPORT DPoint3 CrossProd(const DPoint3& a, const DPoint3& b);	// CROSS PRODUCT
	
/*!  Returns the dot product of two DPoint3s. */
GEOMEXPORT double DotProd(const DPoint3& a, const DPoint3& b) ;		// DOT PRODUCT


/*! \sa  Class DRay.\n\n
 Description:
This class describes a vector in space using an origin point p, and a
unit direction vector in double precisiondir.
 Data Members:
DPoint3 p;\n\n
Point of origin.\n\n
DPoint3 dir;\n\n
Unit direction vector. */
class DRay: public MaxHeapOperators {
	// Warning - instances of this class are saved as a binary blob to scene file
	// Adding/removing members will break file i/o
public:
	DPoint3 p;   // point of origin
	DPoint3 dir; // unit vector
};

