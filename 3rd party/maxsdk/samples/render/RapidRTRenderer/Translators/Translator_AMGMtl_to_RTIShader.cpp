//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_AMGMtl_to_RTIShader.h"

// local
#include "../resource.h"
// max sdk
#include <maxstring.h>
#include <dllutilities.h>
// IAmgInterface.h is located in sdk samples folder, such that it may be shared with sdk samples plugins, without actually being exposed in the SDK.
#include <../samples/render/AmgTranslator/IAmgInterface.h>
#include "PBUtil.h"
#include <inode.h>
#include <Graphics/IShaderManager.h>
#include <iparamb2.h>


namespace Max
{;
namespace RapidRTTranslator
{;

Translator_AMGMtl_to_RTIShader::Translator_AMGMtl_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Mtl_to_RTIShader(key, translator_graph_node),
    Translator(translator_graph_node),
    m_material(key)
{
	DbgAssert(can_translate(key));
}

Translator_AMGMtl_to_RTIShader::~Translator_AMGMtl_to_RTIShader()
{

}

bool Translator_AMGMtl_to_RTIShader::can_translate(const Key& key)
{
    Mtl* material = key;
    if (material == nullptr) 
		return false;

	return IAMGInterface::IsSupported(_T("RapidRT"), material->SuperClassID(), material->ClassID());
}

MSTR Translator_AMGMtl_to_RTIShader::GetTimingCategory() const
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMER_AMG_MATERIAL);
}

TranslationResult Translator_AMGMtl_to_RTIShader::Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& /*keyframesNeeded*/)
{
	if(m_material != NULL)
	{
		return AMG_Translate(m_material, t, validity, translation_progress);
	}

	return TranslationResult::Failure;
}

}}	// namespace 
