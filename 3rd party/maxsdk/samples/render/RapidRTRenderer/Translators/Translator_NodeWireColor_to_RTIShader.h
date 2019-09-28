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
// Max SDK
#include <NotificationAPI/NotificationAPI_Subscription.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_INode.h>
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>

class INode;

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: Node wire color ==> rti::Shader
class Translator_NodeWireColor_to_RTIShader :
    public BaseTranslator_INode,            // Translate from node wire color
	public BaseTranslator_to_RTIShader     // Translate to rti::Shader
{
public:

	typedef GenericTranslatorKey_SingleReference<Translator_NodeWireColor_to_RTIShader, INode> Key;

	// Constructs this translator with the key passed to the translator factory
	Translator_NodeWireColor_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_NodeWireColor_to_RTIShader();

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    virtual MSTR GetTimingCategory() const override;

protected:

    // -- inherited from BaseTranslator_INode
    virtual bool CareAboutMissingUVWChannels() const override;
    virtual std::vector<unsigned int> GetMeshUVWChannelIDs() const override;
    virtual std::vector<MtlID> GetMeshMaterialIDs() const override;

    // -- inherited from BaseTranslator_to_RTIShader
    virtual MSTR get_shader_name() const override;

    // -- inherited from BaseTranslator_INode
    virtual bool CareAboutNotificationEvent(const MaxSDK::NotificationAPI::NodeEventType eventType) const override;

};

}}	// namespace 
