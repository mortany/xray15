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

#include "../CATAPI.h"

#define I_LAYERINFO		0x78d3df5a

class INLAInfoClassDesc : public ClassDesc2 {
};

class ILayerInfo : public StdControl, public ILayerInfoFP
{
public:

	// This method will be called directly after creating a new layer Info controller
	// New layer types may wish to over ride this method to initialise the creation of thier own
	virtual		void PostLayerCreateCallback() = 0;
	virtual		void PreLayerRemoveCallback() = 0;

	virtual		INLAInfoClassDesc* GetClassDesc() = 0;

	void* GetInterface(ULONG id) {
		if (id == I_LAYERINFO)		return (void*)this;
		return StdControl::GetInterface(id);
	}
	virtual BaseInterface* GetInterface(Interface_ID id)
	{
		if (id == I_LAYERINFO_FP) return (ILayerInfoFP*)(this);
		else return Control::GetInterface(id);
	}
};
