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

namespace Max
{;
namespace RapidRTTranslator
{;

//==================================================================================================
// struct Translator_RTIRenderAttributes::Attributes
//==================================================================================================

inline Translator_RTIRenderAttributes::Attributes::Attributes()
{

}

inline Translator_RTIRenderAttributes::Attributes::Attributes(
    const bool is_matte_p, 
    const bool is_emissive_p, 
    const bool generates_caustics_p, 
    const bool visible_to_primary_rays_p)
    : is_matte(is_matte_p),
    is_emissive(is_emissive_p),
    generates_caustics(generates_caustics_p),
    visible_to_primary_rays(visible_to_primary_rays_p)
{

}

inline bool Translator_RTIRenderAttributes::Attributes::operator==(const Attributes& rhs) const
{
    return (is_matte == rhs.is_matte)
        && (is_emissive == rhs.is_emissive)
        && (generates_caustics == rhs.generates_caustics)
        && (visible_to_primary_rays == rhs.visible_to_primary_rays);
}

//==================================================================================================
// struct Translator_RTIRenderAttributes::AttributesHash
//==================================================================================================

inline size_t Translator_RTIRenderAttributes::AttributesHash::operator()(const Attributes& attributes) const
{
    return std::hash<bool>()(attributes.is_matte)
        ^ (std::hash<bool>()(attributes.is_emissive) << 1)
        ^ (std::hash<bool>()(attributes.generates_caustics) << 2)
        ^ (std::hash<bool>()(attributes.visible_to_primary_rays) << 3);
}

}}	// namespace 
