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
#include "BaseTranslator_Environment_to_RTIShader.h"

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: Physical Sun/Sky Env. ==> rti::Shader
class Translator_PhysSunSkyEnv_to_RTIShader :
	public BaseTranslator_Environment_to_RTIShader
{
public:

    // Returns whether this translator can translate the given key
    static bool can_translate(IEnvironmentContainer* const environment);

	// Constructs this translator with the key passed to the translator factory
	Translator_PhysSunSkyEnv_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_PhysSunSkyEnv_to_RTIShader();

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
};

}}	// namespace 
