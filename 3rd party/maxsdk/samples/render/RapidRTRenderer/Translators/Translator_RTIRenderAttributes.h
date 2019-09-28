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

#include "BaseTranslator_to_RTI.h"

#include <RenderingAPI/Translator/GenericTranslatorKeys.h>

#include <rti/scene/RenderAttributes.h>

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: Struct ==> rti::RenderAttributes
class Translator_RTIRenderAttributes :
	public BaseTranslator_to_RTI
{
public:

    struct Attributes
    {
        Attributes();
        Attributes(
            const bool is_matte, 
            const bool is_emissive, 
            const bool generates_caustics,
            const bool visible_to_primary_rays);
        bool operator==(const Attributes& rhs) const;

        bool is_matte = false;
        bool is_emissive = false;
        bool generates_caustics = false;
        bool visible_to_primary_rays = false;
    };
    struct AttributesHash
    {
        size_t operator()(const Attributes& attributes) const;
    };

    typedef GenericTranslatorKey_Struct<Translator_RTIRenderAttributes, Attributes, AttributesHash> Key;

	// Constructs this translator with the key passed to the translator factory
	Translator_RTIRenderAttributes(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_RTIRenderAttributes();

    // Returns the rti::RenderAttributes created by this class
    rti::RenderAttributesHandle get_output_attributes() const;

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    virtual Interval CheckValidity(const TimeValue t, const Interval& previous_validity) const override;
    virtual void PreTranslate(const TimeValue translationTime, Interval& validity) override;
    virtual void PostTranslate(const TimeValue translationTime, Interval& validity) override;
    virtual MSTR GetTimingCategory() const override;

private:

    const Attributes m_attributes;
};

}}	// namespace 

#include "Translator_RTIRenderAttributes.inline.h"