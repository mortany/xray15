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

#include "BaseTranslator_Mtl_to_RTIShader.h"

#include <stdmat.h>

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: Unsupported Mtl ==> rti::Shader
// Uses viewport properties to approximate unsupported materials
class Translator_UnsupportedMtl_to_RTIShader :
	public BaseTranslator_Mtl_to_RTIShader
{
public:

	// Constructs this translator with the key passed to the translator factory
	Translator_UnsupportedMtl_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_UnsupportedMtl_to_RTIShader();

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;

private:

	Mtl* const m_material;
};

}}	// namespace 
