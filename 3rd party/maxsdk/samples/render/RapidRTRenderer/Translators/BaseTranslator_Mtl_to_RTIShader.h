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

// Local includes
#include "BaseTranslator_to_RTIShader.h"
#include "BaseTranslator_AMG_to_RTIShader.h"

// RTI
#include <rti/scene/shader.h>

// Max SDK
#include <NotificationAPI/NotificationAPI_Subscription.h>
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Mtl.h>

class Mtl;
class Texmap;

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: Mtl ==> rti::Shader
class BaseTranslator_Mtl_to_RTIShader :
    public BaseTranslator_Mtl,          // Translate from Mtl
	public BaseTranslator_AMG_to_RTIShader     // Translate, using AMG, to rti::Shader
{
public:

    struct Allocator;
	typedef GenericTranslatorKey_SinglePointer<BaseTranslator_Mtl_to_RTIShader, Mtl, Allocator> Key;

	BaseTranslator_Mtl_to_RTIShader(const Key& key, TranslatorGraphNode& tanslator_graph_node);
    ~BaseTranslator_Mtl_to_RTIShader();

protected:

    // -- inherited from BaseTranslator_to_RTIShader
    virtual MSTR get_shader_name() const;

};

struct BaseTranslator_Mtl_to_RTIShader::Allocator
{
public:
    std::unique_ptr<Translator> operator()(const Key& key, TranslatorGraphNode& translator_graph_node, const IRenderSessionContext& render_session_context, const TimeValue initial_time) const;    
};


}}	// namespace 
