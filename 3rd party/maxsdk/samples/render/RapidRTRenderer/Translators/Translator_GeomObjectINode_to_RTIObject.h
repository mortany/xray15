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
#include "BaseTranslator_INode_to_RTIObject.h"
// max sdk
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Translator/Helpers/INodeInstancingPool.h>
// std
#include <vector>

class INode;

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK::RenderingAPI::TranslationHelpers;

class BaseTranslator_to_RTIShader;

// Translates: Geometric INode ==> rti::Object
class Translator_GeomObjectINode_to_RTIObject :
	public BaseTranslator_INode_to_RTIObject
{
public:

    // Returns whether this translator can translate the given key
    static bool can_translate(const Key& key, const TimeValue t);

	// Constructs this translator with the key passed to the translator factory
	Translator_GeomObjectINode_to_RTIObject(const Key& key,
        TranslatorGraphNode& translator_graph_node);
	~Translator_GeomObjectINode_to_RTIObject();

    // -- inherited from Translator
    virtual void AccumulateStatistics(TranslatorStatistics& stats) const override;
    virtual MSTR GetTimingCategory() const override;

protected:

    // -- inherited from BaseTranslator_to_RTIObject
    virtual const BaseTranslator_to_RTIGeometry* acquire_geometry_translator(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, TranslationResult& result);
    virtual const BaseTranslator_to_RTIShader* acquire_shader_translator(const MtlID material_id, const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, TranslationResult& result);

    // -- inherited from BaseTranslator_INode
    virtual bool CareAboutMissingUVWChannels() const override;
    virtual std::vector<unsigned int> GetMeshUVWChannelIDs() const override;
    virtual std::vector<MtlID> GetMeshMaterialIDs() const override;

private:

    // Set of UVW channels and material IDs present on the mesh as it was translated. Used to report missing UVW channels.
    std::vector<unsigned int> m_uv_channels_present;
    std::vector<MtlID> m_material_ids_present;

};

}}	// namespace 
