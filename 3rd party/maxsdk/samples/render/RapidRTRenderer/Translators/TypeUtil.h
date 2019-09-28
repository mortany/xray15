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

class Object;
class GeomObject;
class LightObject;
class LightscapeLight2;
class CameraObject;

namespace Max
{;
namespace RapidRTTranslator
{;

// Various functions to type casts for the 3ds Max SDK
class TypeUtil
{
public:

    static GeomObject* to_GeomObject(Object* obj);
    static LightObject* to_LightObject(Object* obj);
    static LightscapeLight2* to_LightscapeLight(Object* obj);
    static CameraObject* to_CameraObject(Object* obj);
};

}}	// namespace 

#include "TypeUtil.inline.h"