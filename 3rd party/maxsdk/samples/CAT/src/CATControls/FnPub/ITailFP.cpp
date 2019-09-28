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

#include "ITailFP.h"
#include "iparamb2.h"

FPInterfaceDesc* ITailFP::GetDesc()
{
	return GetFnPubDesc();
}

FPInterfaceDesc* ITailFP::GetFnPubDesc()
{
	// Descriptor
	static FPInterfaceDesc iTail_FPinterface(
		CatAPI::TAIL_INTERFACE_FP, _T("TailFPInterface"), 0, NULL, FP_MIXIN,

		properties,

		ITailFP::propGetHub, FP_NO_FUNCTION, _T("Hub"), 0, TYPE_INTERFACE,

		p_end
	);

	return &iTail_FPinterface;
}
