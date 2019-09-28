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

#include "BaseTranslator_to_RTIShader.h"

// local

// max sdk
#include <RenderingAPI/Renderer/ICameraContainer.h>
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Camera.h>

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK;
using namespace RenderingAPI;

// Base class for translation of camera shaders (rti::Shader)
class BaseTranslator_Camera_to_RTIShader : 
    public BaseTranslator_Camera,           // Translate from Camera
    public BaseTranslator_to_RTIShader     // Translate to rti::Shader
{
public:

    // This translator's key stores the camera INode. The camera node is needed to ensure that a new translator gets created whenever the camera node is changed,
    // even though we don't actually use the node for anything.
    struct Allocator;
    typedef GenericTranslatorKey_SinglePointer<BaseTranslator_Camera_to_RTIShader, INode, Allocator> Key;

    BaseTranslator_Camera_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node);
    ~BaseTranslator_Camera_to_RTIShader();

    // -- inherited from Translator
    virtual void PostTranslate(const TimeValue translationTime, Interval& validity) override;
};

struct BaseTranslator_Camera_to_RTIShader::Allocator
{
    std::unique_ptr<Translator> operator()(const Key& key, TranslatorGraphNode& translator_graph_node, const IRenderSessionContext& render_session_context, const TimeValue initial_time) const;    
};

}}	// namespace 
