//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <strbasic.h>
#include <iparamb2Typedefs.h>
#include <maxtypes.h>

class IParamBlock2;
class Animatable;
class AColor;
class Texmap;
class Interval;

namespace Max
{;
namespace RapidRTTranslator
{;

// Various functions to facilitate access to ParamBlocks
class PBUtil
{
public:

	// Searches the given Animatable for a IParamBlock2 with the given name.
	// Will return null if not found.
	static IParamBlock2* get_param_block_by_name(Animatable& anim, const MCHAR* name);

	// Fetch a parameter value, with support for validity interval (which IParamBlock2::GetAColor() and friends don't support)
	static AColor GetAColor(IParamBlock2& pb, ParamID id, TimeValue t, Interval& valid, int tabIndex=0);
	static int GetInt(IParamBlock2& pb, ParamID id, TimeValue t, Interval& valid, int tabIndex=0);
	static float GetFloat(IParamBlock2& pb, ParamID id, TimeValue t, Interval& valid, int tabIndex=0);
	static Texmap* GetTexmap(IParamBlock2& pb, ParamID id, TimeValue t, Interval& valid, int tabIndex=0);
};


}}	// namespace 
