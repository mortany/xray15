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

#include "BaseTranslator_to_RTIGeometry.h"

#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Object.h>

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK::RenderingAPI;
using namespace MaxSDK::RenderingAPI::TranslationHelpers;

// Translates: LightscapeLight ==> rti::Geometry
class Translator_LightscapeLight_to_RTIGeometry :
    public BaseTranslator_Object,       // from Object 
	public BaseTranslator_to_RTIGeometry        // to rti::Geometry
{
public:

    typedef GenericTranslatorKey_Struct<Translator_LightscapeLight_to_RTIGeometry, std::shared_ptr<const INodeInstancingPool>> Key;

	// Constructs this translator with the key passed to the translator factory
	Translator_LightscapeLight_to_RTIGeometry(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_LightscapeLight_to_RTIGeometry();

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    virtual MSTR GetTimingCategory() const override;

protected:

    // -- inherited from BaseTranslator_Object
    virtual bool CareAboutMaterialDisplacement() const override;
};

}}	// namespace 
