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

#include "Translator_RTIRenderAttributes.h"

#include "../Util.h"

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_RTIRenderAttributes::Translator_RTIRenderAttributes(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_to_RTI(translator_graph_node),
    Translator(translator_graph_node),
    m_attributes(key.GetData())
{

}

Translator_RTIRenderAttributes::~Translator_RTIRenderAttributes()
{

}

rti::RenderAttributesHandle Translator_RTIRenderAttributes::get_output_attributes() const
{
    return get_output_handle<rti::RenderAttributesHandle>(0);
}

TranslationResult Translator_RTIRenderAttributes::Translate(const TimeValue /*t*/, Interval& /*validity*/, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
    // Setup the render attributes
    const rti::RenderAttributesHandle attributes_handle = initialize_output_handle<rti::RenderAttributesHandle>(0);
    if(attributes_handle.isValid())
    {
        rti::TEditPtr<rti::RenderAttributes> render_attributes(attributes_handle);
        
        RRTUtil::check_rti_result(render_attributes->setAttribute1b(rti::ATTRIB_VISIBILITY_PRIMARY, m_attributes.visible_to_primary_rays));
        RRTUtil::check_rti_result(render_attributes->setAttribute1b(rti::ATTRIB_MATTE_OBJECT, m_attributes.is_matte));
        RRTUtil::check_rti_result(render_attributes->setAttribute1b(rti::ATTRIB_LIGHT, m_attributes.is_emissive));
        RRTUtil::check_rti_result(render_attributes->setAttribute1b(rti::ATTRIB_CAUSTICS, m_attributes.generates_caustics));

        return TranslationResult::Success;
    }
    else
    {
        return TranslationResult::Failure;
    }
}

Interval Translator_RTIRenderAttributes::CheckValidity(const TimeValue /*t*/, const Interval& previous_validity) const
{
    return previous_validity;
}

void Translator_RTIRenderAttributes::PreTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Do nothing
}

void Translator_RTIRenderAttributes::PostTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Do nothing
}

MSTR Translator_RTIRenderAttributes::GetTimingCategory() const 
{
    return MSTR();
}

}}	// namespace 
