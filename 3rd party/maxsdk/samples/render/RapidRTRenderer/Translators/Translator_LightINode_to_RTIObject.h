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

#include "BaseTranslator_INode_to_RTIObject.h"

namespace Max
{;
namespace RapidRTTranslator
{;

class BaseTranslator_to_RTIShader;

// Translates: Light INode to rti::Object
class Translator_LightINode_to_RTIObject :
	public BaseTranslator_INode_to_RTIObject        // Transltes from INode to rti::Object
{
public:

	// Constructs this translator with the key passed to the translator factory
	Translator_LightINode_to_RTIObject(
        const Key& key, 
        TranslatorGraphNode& translator_graph_node);
	~Translator_LightINode_to_RTIObject();

    // -- inherited from BaseTranslator_to_RTIObject
    virtual bool is_visible_to_primary_rays(const TimeValue t, Interval& validity) override;

    // -- inherited from Translator
    virtual void AccumulateStatistics(TranslatorStatistics& stats) const override;
    virtual MSTR GetTimingCategory() const override;

protected:

    // Returns whether the light is visible
    virtual bool is_light_visible(const TimeValue t, Interval& validity) const = 0;

    // -- inherited from BaseTranslator_INode
    virtual bool CareAboutNotificationEvent(const MaxSDK::NotificationAPI::NodeEventType eventType) const override;
    virtual bool CareAboutMissingUVWChannels() const override;
    virtual std::vector<unsigned int> GetMeshUVWChannelIDs() const override;
    virtual std::vector<MtlID> GetMeshMaterialIDs() const override;

};

}}	// namespace 
