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

#include "TranslatorOutputs.h"

namespace Max
{;
namespace RapidRTTranslator
{;

// Helper method which fetches a handle from an output, returning an empty handle if the output is not initialized.
template<typename RTIHandleType>
RTIHandleType BaseTranslator_to_RTI::get_output_handle(const size_t output_index) const
{
    const auto handle_output = GetOutput<RTIHandleTranslatorOutput<RTIHandleType>>(output_index);
    return (handle_output != NULL) ? handle_output->get_handle() : RTIHandleType();
}

// Helper method which initializes the given output to the templated handle type, iff the output doesn't already point to a handle of this type.
template<typename RTIHandleType>
RTIHandleType BaseTranslator_to_RTI::initialize_output_handle(const size_t output_index)
{
    const RTIHandleType handle = get_output_handle<RTIHandleType>(output_index);
    if(!handle.isValid())
    {
        std::shared_ptr<RTIHandleTranslatorOutput<RTIHandleType>> new_output(new RTIHandleTranslatorOutput<RTIHandleType>());
        SetOutput(output_index, new_output);
        return new_output->get_handle();
    }
    else
    {
        return handle;
    }
}

template<typename TranslatorType> 
const TranslatorType* BaseTranslator_to_RTI::AcquireChildTranslator(const typename TranslatorType::Key& key, const TimeValue t, ITranslationProgress& translation_progress, TranslationResult& result)
{
    const TranslatorType* child_translator = Translator::AcquireChildTranslator<TranslatorType>(key, t, translation_progress, result);
    return child_translator;
}

}}	// namespace 
