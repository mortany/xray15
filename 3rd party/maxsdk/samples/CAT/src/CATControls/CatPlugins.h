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

//	This contains everything that is shared
//	between all CAT plugins.
//
//	eg,
//	- Global Objects
//	- Global Function Prototypes.
//	- Anything the DLLEntry functions need from the
//	  plugins.    ( eg ClassDescription	access function )

#pragma once

//
//	Just a few global Max Headers.
//	Plugin specific headers should go directly
//	into the plugin .cpp file.
//
#include <Max.h>
#include "resource.h"
#include <istdplug.h>
#include <iparamb2.h>

#include "cat.h"

// The base catunits, measuring time in footsteps
#define BASE	192			// 1 frame at 25 fps

// I shifted these in here, because too many classes
// need them to just include everywhere
#define IPOINT3_CONTROL_CLASS_ID			Class_ID(0x118f7e02,0xfeee238b)
#define CATHIERARCHYBRANCH_CLASS_ID			Class_ID(0x94e2d9c, 0x2c702b5)
#define CATHIERARCHYBRANCH_INTERFACE_ID	Interface_ID(0x22fa7a80, 0x5990646a)

// Get strings from the String Table
extern TCHAR *GetString(int id);

// Get interval from StepEase
//extern Interval GetSegInterval();
//	Our global HInstance object.
//	(instantiated in DLLEntry.cpp)
extern HINSTANCE hInstance;

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

/////////////////////////////////////////////////////
// We set up an accessor to be used for any param
// block that wants to limit its values between 0 and 1
class ZeroToOneParamsAccessor : public PBAccessor
{
	void Set(PB2Value& v, ReferenceMaker*, ParamID, int tabIndex, TimeValue)
	{
		UNREFERENCED_PARAMETER(tabIndex);
		v.f = min(max(v.f, 0.0f), 1.0f);
	}
};
static ZeroToOneParamsAccessor ZeroToOneParamAccessor;

// This little helper class prolly should be in the SDK
// Basically, its turns on Undo's for a given scope.
// RAII is a wonderful thing, huh?
// TODO: Do I need to check begin depth?
class HoldActions {
	int mAcceptStringID;
public:
	// Constructor initializes the
	// hold. iAccept should be the ID
	// of a string resource to be passed
	// to theHold.Accept
	HoldActions(int iAccept)
		: mAcceptStringID(iAccept) {
		theHold.Begin();
	}

	~HoldActions()
	{
		if (std::uncaught_exception())
			theHold.Cancel();
		else
			theHold.Accept(GetString(mAcceptStringID));
	}
};

// Lots of general CAT functions.

inline DPoint3 AsDPoint3(const Point3& pt) { return DPoint3((double)pt.x, (double)pt.y, (double)pt.z); }
inline Point3 AsPoint3(const DPoint3& pt) { return Point3((float)pt.x, (float)pt.y, (float)pt.z); }

////////////////////////////////////////////////////////////////////
// the rollout that is used to vonfigure clips loading and saving
// now any of the class can pop up a dialogue to save or laod clips and poses
class CATClipRoot;
extern BOOL MakeClipPoseDlg(ReferenceTarget* savingref, CATClipRoot* cliproot, const TCHAR* filename);
extern BOOL MakeLoadClipPoseDlg(ReferenceTarget* loadingref, CATClipRoot* cliproot, BOOL bIsClip, const TCHAR *pStr, BOOL bIsLimb = FALSE);
