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

// This base class adds the ability to distribute SetValues to the CATNodeControl
// The classes that derive from this base clase are tails, spines and digits.

#include "CATNodeControl.h"
class CATNodeControlDistributed : public CATNodeControl
{
public:
	CATNodeControlDistributed() {}
	~CATNodeControlDistributed() {}

	// This implementation of SetValue detects if we should
	// distribute this SetValue over multiple instances of this class
	// before modifying the SetXFormPacket data appropriately and passing
	// through to CATNodeControl
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	// Returns if every segment needs to recieve a SetValue.
	// Returns TRUE only if the ancestors of the tail (hub & lower)
	// is selected currently
	BOOL InheritsParentTransform();

	// Use this method to tell if an ancestor of this bone is
	// currently selected.  We use this to tell if this
	// bone should continue on with a setvalue or not...
	// \param iBoneID - The index of the bone to begin testing ancestors from
	// \return true if any ancestors of iBoneID are selected
	bool IsAncestorSelected(int iBoneID);

	// Return the number of bones in this body part
	// that are selected, up to (& including) iBoneID
	// \param iBoneID - The max index to test.  If -1, test all bones
	// \return The number of bones currently selected
	int NumSelectedBones(int iBoneID = -1);
};