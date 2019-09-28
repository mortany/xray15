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

#include "TranslatorGraphNode.h"

//==================================================================================================
// Implementation of dll-exported methods from class Translator
//==================================================================================================

#include "3dsmax_banned.h"

namespace MaxSDK
{;
namespace RenderingAPI
{;


Translator::Translator(TranslatorGraphNode& graphNode)
    : mGraphNode(graphNode)
{

}

Translator::~Translator()
{

}

void Translator::AccumulateGraphStatistics(TranslatorStatistics& statistics) const
{
    return mGraphNode.AccumulateGraphStatistics(statistics);
}

TranslatorGraphNode& Translator::GetGraphNode() const
{
    return mGraphNode;
}

void Translator::Invalidate(const bool defer_invalidation_check)
{
    return mGraphNode.Invalidate(defer_invalidation_check);
}

void Translator::InvalidateParents()
{
    return mGraphNode.invalidate_parents();
}

void Translator::TranslatedObjectDeleted()
{
    return mGraphNode.TranslatedObjectDeleted();
}

size_t Translator::GetNumOutputs() const
{
    return mGraphNode.GetNumOutputs();
}

void Translator::SetNumOutputs(const size_t num)
{
    return mGraphNode.SetNumOutputs(num);
}

void Translator::SetOutput(const size_t index, std::shared_ptr<const ITranslatorOutput> output)
{
    return mGraphNode.SetOutput(index, output);
}

const Translator* Translator::AcquireChildTranslatorInternal(const TranslatorKey& key, const TimeValue t, ITranslationProgress& translationProgress, TranslationResult& result)
{
    return mGraphNode.AcquireChildTranslatorInternal(key, t, translationProgress, result);
}

std::shared_ptr<const ITranslatorOutput> Translator::GetOutputInternal(const size_t index) const
{
    return mGraphNode.GetOutputInternal(index);
}

inline IRenderSessionContext& Translator::GetRenderSessionContext() 
{
    return mGraphNode.GetRenderSessionContext();
}

inline const IRenderSessionContext& Translator::GetRenderSessionContext() const
{
    return mGraphNode.GetRenderSessionContext();
}


}}  // namespace