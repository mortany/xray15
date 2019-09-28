//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "BaseTranslator_to_RTI.h"

namespace Max
{;
namespace RapidRTTranslator
{;

BaseTranslator_to_RTI::BaseTranslator_to_RTI(TranslatorGraphNode& translator_graph_node)
    : Translator(translator_graph_node)
{

}

BaseTranslator_to_RTI::~BaseTranslator_to_RTI()
{
	// Do nothing
}

TranslationResult BaseTranslator_to_RTI::TranslateKeyframe(const TimeValue /*frame_time*/, const TimeValue /*keyframe_time*/, ITranslationProgress& /*translationProgress*/)
{
    // Default implementation as most translators don't need this method
    DbgAssert(false);
    return  TranslationResult::Failure;
}

void BaseTranslator_to_RTI::AccumulateStatistics(TranslatorStatistics& /*stats*/) const
{

}

}}	// namespace 
