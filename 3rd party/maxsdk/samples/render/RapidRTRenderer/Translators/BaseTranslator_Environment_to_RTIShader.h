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

// Rendering API
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Renderer/ISceneContainer.h>
#include <RenderingAPI/Renderer/ICameraContainer.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Environment.h>

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK;

// Translates: Environment ==> rti::Shader
class BaseTranslator_Environment_to_RTIShader :
    public BaseTranslator_Environment,      // Translate from Environment
	public BaseTranslator_AMG_to_RTIShader         // Translate to rti::Shader, using AMG
{
public:

    class Key;

	// Constructs this translator with the key passed to the translator factory
	BaseTranslator_Environment_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node);
	~BaseTranslator_Environment_to_RTIShader();

    // -- inherited from Translator
    virtual void PreTranslate(const TimeValue translationTime, Interval& valid) override;
    virtual void PostTranslate(const TimeValue translationTime, Interval& valid) override;

protected:

    // -- from BaseTranslator_to_RTIShader
    virtual MSTR get_shader_name() const;

};

class BaseTranslator_Environment_to_RTIShader::Key :
    public TranslatorKey
{
public:
    Key(IEnvironmentContainer* environment);

    IEnvironmentContainer* get_environment() const;

    // -- inherited from TranslatorKey
    virtual bool operator==(const TranslatorKey& rhs) const override;
    virtual std::unique_ptr<const TranslatorKey> CreateClone() const override;
    virtual std::unique_ptr<Translator> AllocateNewTranslator(TranslatorGraphNode& translator_graph_node, const IRenderSessionContext& render_session_context, const TimeValue initial_time) const override;

private:

    Key(const Key& from);
    static size_t compute_hash(IEnvironmentContainer* environment);

private:

    IEnvironmentContainer* m_environment;
    // Determines that this key corresponds to a sun&sky environment translator. This enables the key to become invalid
    // if the environment map type changes, forcing the creating of a new (non sun&sky) translator.
    bool m_is_sun_sky;
};


}}	// namespace 
