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
#include "BaseTranslator_to_RTI.h"
// Rendering API
#include <RenderingAPI/Translator/Translator.h>
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Renderer/ISceneContainer.h>
// RTI
#include <rti/scene/Entity.h>
#include <rti/scene/Group.h>
// std
#include <vector>

class INode;

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: Scene Root ==> rti::Group
class Translator_SceneRoot_to_RTIGroup :
	public BaseTranslator_to_RTI,
    private ISceneContainer::IChangeNotifier
{
public:

	typedef GenericTranslatorKey_Empty<Translator_SceneRoot_to_RTIGroup> Key;

	// Constructs this translator with the key passed to the translator factory
	Translator_SceneRoot_to_RTIGroup(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_SceneRoot_to_RTIGroup();

    // Returns the rti::Group that gets translated by this class
    rti::GroupHandle get_output_group() const;

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    virtual Interval CheckValidity(const TimeValue t, const Interval& previous_validity) const override;
    virtual void PreTranslate(const TimeValue translationTime, Interval& validity) override;
    virtual void PostTranslate(const TimeValue translationTime, Interval& validity) override;
    virtual MSTR GetTimingCategory() const override;

private:

    // -- inherited private ISceneContainer::IChangeNotifier
    virtual void NotifySceneNodesChanged();
    virtual void NotifySceneBoundingBoxChanged();
};

}}	// namespace 
