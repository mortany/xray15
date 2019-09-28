//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_to_RTIFrameBuffer.h"

// Local includes

// Max sdk
#include <RenderingAPI/Renderer/IRenderSession.h>

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_to_RTIFrameBuffer::Translator_to_RTIFrameBuffer(const Key& /*key*/, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_to_RTI(translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_to_RTIFrameBuffer::~Translator_to_RTIFrameBuffer()
{

}

rti::FramebufferHandle Translator_to_RTIFrameBuffer::get_output_frame_buffer() const
{
    return get_output_handle<rti::FramebufferHandle>(0);
}

TranslationResult Translator_to_RTIFrameBuffer::Translate(const TimeValue /*t*/, Interval& /*validity*/, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
    // Initialize the frame buffer
    const rti::FramebufferHandle framebuffer_handle = initialize_output_handle<rti::FramebufferHandle>(0);
    if(framebuffer_handle.isValid())
    {
        rti::TEditPtr<rti::Framebuffer> frame_buffer(framebuffer_handle);
        
        frame_buffer->setFormat(rti::FB_FORMAT_RGBA_F32);
        frame_buffer->setMode(rti::FB_MODE_FINAL);
        // The Rapid render session implements IFramebufferCallback
        rti::IFramebufferCallback* const frame_buffer_callback = dynamic_cast<rti::IFramebufferCallback*>(GetRenderSessionContext().GetRenderSession());
        DbgAssert(frame_buffer_callback != nullptr);
        frame_buffer->setSwapCallback(frame_buffer_callback);

        return TranslationResult::Success;
    }
    else
    {
        return TranslationResult::Failure;
    }
}

Interval Translator_to_RTIFrameBuffer::CheckValidity(const TimeValue /*t*/, const Interval& previous_validity) const
{
    return previous_validity;
}

void Translator_to_RTIFrameBuffer::PreTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Do nothing
}

void Translator_to_RTIFrameBuffer::PostTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Do nothing
}

MSTR Translator_to_RTIFrameBuffer::GetTimingCategory() const 
{
    return MSTR();
}


}}	// namespace 
