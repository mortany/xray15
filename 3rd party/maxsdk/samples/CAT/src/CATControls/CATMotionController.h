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

//////////////////////////////////////////////////////////////////////////
// Class CATMotionController - A small header class to help ensure correct
// CATMotion hierarchy destruction.

#pragma once

#include <control.h>

class CATControl;
class CATClipValue;
class LimbData2;

// This small utility class allows us to define a common destruction mechanism for CATMotion Controllers.

class CATMotionController : public Control
{
	// This implementation of AutoDelete cleans the pointers
	// from our parent to us.
	RefResult AutoDelete();

	// Return a pointer to the class that has a pointer to this class
	// Implemented in CATMotionLimb/Hub/Tail
	virtual Control* GetOwningCATMotionController() { return NULL; };

public:

	// Remove class from the CATMotion hierarchy
	// This is overridden in CATMotionLimb to allow
	// it to un-weld leaves (something we can't do
	// auto-magically)
	virtual void DestructCATMotionHierarchy(LimbData2* pLimb);

	// Null a pointer in this classes paramblock
	//void NullPointerInParamBlock(CATMotionController* pCtrl);

	// Null all this classes P_NO_REF params
	void NullNoRefInParamBlock();
};