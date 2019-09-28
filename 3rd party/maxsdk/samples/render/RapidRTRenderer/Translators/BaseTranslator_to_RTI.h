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

#include <RenderingAPI/Translator/Translator.h>
#include <RenderingAPI/Renderer/IRenderSessionContext.h>

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK::RenderingAPI;

// Base class for translating: anything ==> RTI
// Mainly manages the creation of RTI handles as translation outputs.
class BaseTranslator_to_RTI :
	public virtual Translator
{
public:

    BaseTranslator_to_RTI(TranslatorGraphNode& translator_graph_node);
	virtual ~BaseTranslator_to_RTI();

    // -- inherited from Translator
    virtual TranslationResult TranslateKeyframe(const TimeValue frame_time, const TimeValue keyframe_time, ITranslationProgress& translationProgress) override;
    virtual void AccumulateStatistics(TranslatorStatistics& stats) const;

protected:

    // Helper method which fetches a handle from an output, returning an empty handle if the output is not initialized.
    template<typename RTIHandleType> RTIHandleType get_output_handle(const size_t output_index) const;
    // Helper method which initializes the given output to the templated handle type, iff the output doesn't already point to a handle of this type.
    template<typename RTIHandleType> RTIHandleType initialize_output_handle(const size_t output_index);

    // Shortcut method for acquiring a child. Assumes the translator key is a member type named "Key" - a design artifact which is part of the Rapid
    // translator, but not enforced by the translator API (hence this method isn't part of the translator API).
    template<typename TranslatorType> 
    const TranslatorType* AcquireChildTranslator(const typename TranslatorType::Key& key, const TimeValue t, ITranslationProgress& translation_progress, TranslationResult& result);

    // Avoid hiding the inherited method
    using Translator::AcquireChildTranslator;
};

}}	// namespace 

#include "BaseTranslator_to_RTI.inline.h"