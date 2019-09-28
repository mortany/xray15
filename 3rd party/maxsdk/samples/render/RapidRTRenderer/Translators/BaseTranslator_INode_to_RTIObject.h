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

// Internal includes
#include "BaseTranslator_to_RTIObject.h"
// Translator API
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_INode.h>
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Renderer/ICameraContainer.h>
// std
#include <vector>
// Max SDK
#include <NotificationAPI/NotificationAPI_Subscription.h>

class INode;

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK::RenderingAPI;
using namespace MaxSDK::NotificationAPI;
using namespace TranslationHelpers;

class BaseTranslator_to_RTIShader;

// Translates INode -> one/more rti::Object
// Essentially a bridge between classes BaseTranslator_to_RTIObject and INodeBaseTranslator
class BaseTranslator_INode_to_RTIObject :
    public BaseTranslator_INode,        // Translates from INode
    public BaseTranslator_to_RTIObject // Translates to rti::Object
{
public:

    // The key holds the INode& as well as the scene's physical scale, which light sources
    struct Allocator;
    typedef GenericTranslatorKey_SingleReference<BaseTranslator_INode_to_RTIObject, INode, Allocator> Key;

	// Constructs this translator with the key passed to the translator factory
	BaseTranslator_INode_to_RTIObject(
        const Key& key,
        const NotifierType notifier_node_type,
        TranslatorGraphNode& translator_graph_node);
	~BaseTranslator_INode_to_RTIObject();

    // -- inherited from Translator
    virtual Interval CheckValidity(const TimeValue t, const Interval& previous_validity) const override;

    // -- inherited from BaseTranslator_INode
    virtual Mtl* ResolveMaterial(Mtl* const mtl) const override;
    virtual bool CareAboutRenderProperty(const PartID render_property_id) const;
    virtual bool CareAboutDisplayProperty(const PartID display_property_id) const;
    virtual bool CareAboutGIProperty(const PartID gi_property_id) const;

protected:

    // -- inherited from BaseTranslator_to_RTIObject
    virtual void get_transform_matrices(const TimeValue t, Interval& validity, Matrix3& transform, Matrix3& motion_transform) const override;
    virtual bool is_caustic_generator(const TimeValue t, Interval& validity);
    virtual bool is_visible_to_primary_rays(const TimeValue t, Interval& validity);

};

struct BaseTranslator_INode_to_RTIObject::Allocator
{
    std::unique_ptr<Translator> operator()(const Key& key, TranslatorGraphNode& translator_graph_node, const IRenderSessionContext& render_session_context, const TimeValue initial_time) const;    
};

}}	// namespace 
