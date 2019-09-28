//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "PBUtil.h"

#include <iparamb2.h>

namespace Max
{;
namespace RapidRTTranslator
{;

IParamBlock2* PBUtil::get_param_block_by_name(Animatable& anim, const MCHAR* name)
{
	const int count = anim.NumParamBlocks();
	for(int i = 0; i < count; ++i)
	{
		IParamBlock2* param_block = anim.GetParamBlock(i);
		if(param_block != nullptr)
		{
			ParamBlockDesc2* desc = param_block->GetDesc();
			if((desc != nullptr) && (_tcscmp(name, desc->int_name) == 0))
			{
				return param_block;
			}
		}
	}

	return nullptr;
}

AColor PBUtil::GetAColor(IParamBlock2& pb, ParamID id, TimeValue t, Interval& valid, int tabIndex)
{
	AColor val;
	const BOOL success = pb.GetValue(id, t, val, valid, tabIndex);
	DbgAssert(success);
	return success ? val : AColor(0.0f, 0.0f, 0.0f);
}

int PBUtil::GetInt(IParamBlock2& pb, ParamID id, TimeValue t, Interval& valid, int tabIndex)
{
	int val;
	const BOOL success = pb.GetValue(id, t, val, valid, tabIndex);
	DbgAssert(success);
	return success ? val : 0;
}

float PBUtil::GetFloat(IParamBlock2& pb, ParamID id, TimeValue t, Interval& valid, int tabIndex)
{
	float val;
	const BOOL success = pb.GetValue(id, t, val, valid, tabIndex);
	DbgAssert(success);
	return success ? val : 0.0f;
}

Texmap* PBUtil::GetTexmap(IParamBlock2& pb, ParamID id, TimeValue t, Interval& valid, int tabIndex)
{
	Texmap* val;
	const BOOL success = pb.GetValue(id, t, val, valid, tabIndex);
	DbgAssert(success);
	return success ? val : nullptr;
}

}}	// namespace 
