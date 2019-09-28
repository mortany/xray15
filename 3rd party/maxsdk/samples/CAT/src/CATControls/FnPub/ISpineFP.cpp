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

#include "ISpineFP.h"
#include "iparamb2.h"

FPInterfaceDesc* ISpineFP::GetDesc()
{
	return GetFnPubDesc();
}

FPInterfaceDesc* ISpineFP::GetFnPubDesc()
{
	// Descriptor
	static FPInterfaceDesc iSpine_FPinterface(
		CatAPI::I_SPINE_FP, _T("SpineFPInterface"), 0, NULL, FP_MIXIN,

		ISpineFP::fnSetAbsRel, _T("SetAbsRel"), 0, TYPE_VOID, 0, 2,
			_T("time"), 0, TYPE_TIMEVALUE,
			_T("value"), 0, TYPE_FLOAT,
		ISpineFP::fnGetAbsRel, _T("GetAbsRel"), 0, TYPE_FLOAT, 0, 2,
			_T("time"), 0, TYPE_TIMEVALUE,
			_T("value"), 0, TYPE_INTERVAL_BR,

		properties,

		ISpineFP::propGetBaseHub, FP_NO_FUNCTION, _T("BaseHub"), 0, TYPE_INTERFACE,
		ISpineFP::propGetTipHub, FP_NO_FUNCTION, _T("TipHub"), 0, TYPE_INTERFACE,
		ISpineFP::propGetSpineFK, ISpineFP::propSetSpineFK, _T("SpineFK"), 0, TYPE_BOOL,

		p_end
	);

	return &iSpine_FPinterface;
}
