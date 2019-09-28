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

#include <CATAPI/ISpine.h>

class ISpineFP : public CatAPI::ISpine
{
public:
	enum FPDataProps {
		propGetBaseHub,
		propGetTipHub,
		propGetSpineFK,
		propSetSpineFK,
		fnGetAbsRel,
		fnSetAbsRel,

	};

	BEGIN_FUNCTION_MAP
		RO_PROP_FN(propGetBaseHub, GetBaseIHub, TYPE_INTERFACE);
		RO_PROP_FN(propGetTipHub, GetTipIHub, TYPE_INTERFACE);
		PROP_FNS(propGetSpineFK, GetSpineFK, propSetSpineFK, SetSpineFK, TYPE_BOOL);
		VFN_2(fnSetAbsRel, SetAbsRel, TYPE_TIMEVALUE, TYPE_FLOAT);
		FN_2(fnGetAbsRel, TYPE_FLOAT, GetAbsRel, TYPE_TIMEVALUE, TYPE_INTERVAL_BR);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc();

	static FPInterfaceDesc* GetFnPubDesc();
};