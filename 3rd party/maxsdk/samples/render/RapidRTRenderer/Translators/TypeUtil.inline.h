//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include <lslights.h>

namespace Max
{;
namespace RapidRTTranslator
{;

inline GeomObject* TypeUtil::to_GeomObject(Object* obj)
{
    if((obj != nullptr) 
        && ((obj->SuperClassID() == GEOMOBJECT_CLASS_ID) || (obj->SuperClassID() == SHAPE_CLASS_ID)))
    {
        return static_cast<GeomObject*>(obj);
    }
    else
    {
        return nullptr;
    }
}

inline LightObject* TypeUtil::to_LightObject(Object* obj)
{
    if((obj != nullptr) && (obj->SuperClassID() == LIGHT_CLASS_ID))
    {
        return static_cast<LightObject*>(obj);
    }
    else
    {
        return nullptr;
    }
}

inline LightscapeLight2* TypeUtil::to_LightscapeLight(Object* obj)
{
    LightObject* light = to_LightObject(obj);
    if((light != nullptr) && light->IsSubClassOf(LIGHTSCAPE_LIGHT_CLASS))
    {
        return static_cast<LightscapeLight2*>(light);
    }
    else
    {
        return nullptr;
    }
}

inline CameraObject* TypeUtil::to_CameraObject(Object* obj)
{
    if((obj != nullptr) && (obj->SuperClassID() == CAMERA_CLASS_ID))
    {
        return static_cast<CameraObject*>(obj);
    }
    else
    {
        return nullptr;
    }
}

}}	// namespace 
