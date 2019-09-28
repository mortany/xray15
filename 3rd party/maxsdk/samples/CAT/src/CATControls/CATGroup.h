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

#include "CATRigPresets.h"
#include "ICATParent.h"

class CATClipValue;
class CATClipWeights;
class CATGroup;

/////////////////////////////////////////////////////
// CATGroup Interface Class

#define I_CATGROUP		Interface_ID(0x697e25c7, 0x5a3f5dd0)
//#define I_CATGROUP		0x697e25c7

class CATGroup : public BaseInterface
{
protected:

	CATClipWeights*	weights;

public:
	virtual class CATControl* AsCATControl() = 0;
	virtual CATClipWeights* GetWeights() { return weights; };

	virtual Color	GetGroupColour() = 0;
	virtual void	SetGroupColour(Color clr) = 0;

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);
};
