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

#include "BaseTranslator_to_RTI.h"

#include <RenderingAPI/Translator/ITranslationManager.h>
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
// rti
#include <rti/scene/scene.h>
#include <rti/scene/Group.h>

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK;

// Translates: rti::Scene
class Translator_RTIScene :
	public BaseTranslator_to_RTI,
    protected MaxSDK::NotificationAPI::INotificationCallback
{
public:

	typedef GenericTranslatorKey_Empty<Translator_RTIScene> Key;

	// Constructs this translator with the key passed to the translator factory
	Translator_RTIScene(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_RTIScene();

    // Returns the translated scene
    rti::SceneHandle get_output_scene() const;

    // Used to forcibly invalidate the scene to trigger a re-start of the rendering
    void force_invalidate();

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    virtual Interval CheckValidity(const TimeValue t, const Interval& previous_validity) const override;
    virtual void PreTranslate(const TimeValue translationTime, Interval& validity) override;
    virtual void PostTranslate(const TimeValue translationTime, Interval& validity) override;
    virtual MSTR GetTimingCategory() const override;

protected:

    // Replaces the environment being monitored with the given one
    void monitor_environment(IEnvironmentContainer* const environment);

    // -- inherited NotificationAPI::INotificationCallback
    virtual void NotificationCallback_NotifyEvent(const MaxSDK::NotificationAPI::IGenericEvent& genericEvent, void* userData) override; 

private:

    // The environment being monitored
    IEnvironmentContainer* m_monitored_environment;
};

}}	// namespace 
