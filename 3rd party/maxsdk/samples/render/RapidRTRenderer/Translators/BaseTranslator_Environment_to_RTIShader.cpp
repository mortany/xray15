//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "BaseTranslator_Environment_to_RTIShader.h"

// local
#include "../resource.h"
#include "Translator_PhysSunSkyEnv_to_RTIShader.h"
#include "Translator_StandardEnv_to_RTIShader.h"
// max sdk
#include <dllutilities.h>
#include <RenderingAPI/Renderer/IRenderingProcess.h>

using namespace MaxSDK;

namespace Max
{;
namespace RapidRTTranslator
{;

//==================================================================================================
// class BaseTranslator_Environment_to_RTIShader
//==================================================================================================

BaseTranslator_Environment_to_RTIShader::BaseTranslator_Environment_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Environment(key.get_environment(), translator_graph_node),
    BaseTranslator_AMG_to_RTIShader(translator_graph_node),
    Translator(translator_graph_node)
{

}

BaseTranslator_Environment_to_RTIShader::~BaseTranslator_Environment_to_RTIShader()
{

}

MSTR BaseTranslator_Environment_to_RTIShader::get_shader_name() const
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_ENVIRONMENT_SHADER_NAME);
}

void BaseTranslator_Environment_to_RTIShader::PreTranslate(const TimeValue translationTime, Interval& validity) 
{
    // The environment can take a little while to translate, so we provide a progress report message
    GetRenderSessionContext().GetRenderingProcess().SetRenderingProgressTitle(MaxSDK::GetResourceStringAsMSTR(IDS_PROGRESS_TRANSLATING_ENVIRONMENT));

    BaseTranslator_Environment::PreTranslate(translationTime, validity);
}

void BaseTranslator_Environment_to_RTIShader::PostTranslate(const TimeValue translationTime, Interval& validity) 
{
    BaseTranslator_Environment::PostTranslate(translationTime, validity);

    // Remove the environment translation progress title
    GetRenderSessionContext().GetRenderingProcess().SetRenderingProgressTitle(_T(""));
}

//==================================================================================================
// class BaseTranslator_Environment_to_RTIShader::Key
//==================================================================================================

BaseTranslator_Environment_to_RTIShader::Key::Key(IEnvironmentContainer* environment)
    : TranslatorKey(compute_hash(environment)),
    m_environment(environment),
    m_is_sun_sky(Translator_PhysSunSkyEnv_to_RTIShader::can_translate(environment))
{

}

size_t BaseTranslator_Environment_to_RTIShader::Key::compute_hash(IEnvironmentContainer* environment)
{
    const bool is_sun_sky = Translator_PhysSunSkyEnv_to_RTIShader::can_translate(environment);
    return std::hash<IEnvironmentContainer*>()(environment)
        ^ (std::hash<bool>()(is_sun_sky) << 1);
}

BaseTranslator_Environment_to_RTIShader::Key::Key(const Key& from)
    : TranslatorKey(from.get_hash()),
    m_environment(from.m_environment),
    m_is_sun_sky(from.m_is_sun_sky)
{

}

IEnvironmentContainer* BaseTranslator_Environment_to_RTIShader::Key::get_environment() const
{
    return m_environment;
}

bool BaseTranslator_Environment_to_RTIShader::Key::operator==(const TranslatorKey& rhs) const 
{
    const Key* cast_key = dynamic_cast<const Key*>(&rhs);

    return (cast_key != nullptr)
        && (m_environment == cast_key->m_environment)
        && (m_is_sun_sky == cast_key->m_is_sun_sky);
}

std::unique_ptr<const TranslatorKey> BaseTranslator_Environment_to_RTIShader::Key::CreateClone() const 
{
    return std::unique_ptr<const TranslatorKey>(new Key(*this));
}

std::unique_ptr<Translator> BaseTranslator_Environment_to_RTIShader::Key::AllocateNewTranslator(TranslatorGraphNode& translator_graph_node, const IRenderSessionContext& /*render_session_context*/, const TimeValue /*initial_time*/) const 
{
    if(m_is_sun_sky)
        return std::unique_ptr<Translator>(new Translator_PhysSunSkyEnv_to_RTIShader(*this, translator_graph_node));
    else
        return std::unique_ptr<Translator>(new Translator_StandardEnv_to_RTIShader(*this, translator_graph_node));
}

}}	// namespace 
