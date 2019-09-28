//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Environment.h>

// local
#include "../../resource.h"

// Max SDK
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <RenderingAPI/Renderer/IEnvironmentContainer.h>
#include <dllutilities.h>

#include "3dsmax_banned.h"

namespace MaxSDK
{;
namespace RenderingAPI
{;
using namespace NotificationAPI;

BaseTranslator_Environment::BaseTranslator_Environment(IEnvironmentContainer* const environment, TranslatorGraphNode& graphNode)
    : Translator(graphNode),
    BaseTranslator_Texmap(environment, graphNode),
    m_environment(environment)
{

}

BaseTranslator_Environment::~BaseTranslator_Environment()
{

}

IEnvironmentContainer* BaseTranslator_Environment::GetEnvironment() const
{
    return m_environment;
}

MSTR BaseTranslator_Environment::GetTimingCategory() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_ENVIRONMENT);
}

}}	// namespace 


