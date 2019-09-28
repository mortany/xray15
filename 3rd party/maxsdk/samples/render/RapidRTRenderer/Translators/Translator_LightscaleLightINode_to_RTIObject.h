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

#include "Translator_LightINode_to_RTIObject.h"

#include <RenderingAPI/Translator/GenericTranslatorKeys.h>

#include <vector>

class INode;

namespace Max
{;
namespace RapidRTTranslator
{;

class BaseTranslator_to_RTIShader;

// Translates: LightscapeLight INode to rti::Object
class Translator_LightscaleLightINode_to_RTIObject :
	public Translator_LightINode_to_RTIObject       // Translates from Light INode to rti::Object
{
public:

    // Returns whether this translator can translate the given key
    static bool can_translate(const Key& key, const TimeValue t);

	// Constructs this translator with the key passed to the translator factory
	Translator_LightscaleLightINode_to_RTIObject(
        const Key& key, 
        TranslatorGraphNode& translator_graph_node);
	~Translator_LightscaleLightINode_to_RTIObject();

protected:

    // -- inherited from Translator_LightINode_to_RTIObject
    virtual bool is_light_visible(const TimeValue t, Interval& validity) const;

    // -- inherited from BaseTranslator_to_RTIObject
    virtual const BaseTranslator_to_RTIGeometry* acquire_geometry_translator(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, TranslationResult& result);
    virtual const BaseTranslator_to_RTIShader* acquire_shader_translator(const MtlID material_id, const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, TranslationResult& result);
};

}}	// namespace 
