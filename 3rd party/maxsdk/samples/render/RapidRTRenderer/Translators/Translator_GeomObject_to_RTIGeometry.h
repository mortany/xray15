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

// local includes
#include "TranslatorOutputs.h"
#include "BaseTranslator_to_RTIGeometry.h"
// Translator API
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Translator/Helpers/INodeInstancingPool.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Object.h>
// std includes
#include <set>

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK::RenderingAPI::TranslationHelpers;

// Translates: GeomObject ==> rti::Geometry
// Translates a 3ds Max GeomObject into one or more rti::Geometry (Rapid needs objects to be split based on materials)
class Translator_GeomObject_to_RTIGeometry : 
    public BaseTranslator_Object,       // Translate from Object
	public BaseTranslator_to_RTIGeometry    // Translate to rti::Geometry
{
public:

    // The key contains the node pool from which the mesh is queried
	typedef GenericTranslatorKey_Struct<Translator_GeomObject_to_RTIGeometry, std::shared_ptr<const INodeInstancingPool>> Key;

	// Constructs this translator with the key passed to the translator factory
	Translator_GeomObject_to_RTIGeometry(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_GeomObject_to_RTIGeometry();

    const std::vector<unsigned int>& get_uv_channels_translated() const;
    const std::vector<MtlID>& get_material_ids_translated() const;

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    virtual void AccumulateStatistics(TranslatorStatistics& stats) const override;
    virtual MSTR GetTimingCategory() const override;

protected:

    // -- inherited from BaseTranslator_Object
    virtual bool CareAboutMaterialDisplacement() const override;

private:

    // Number of triangles translated for this object
    size_t m_num_triangles_translated;

    // The UV channels and material IDs which were translated last. This is used to keep track of channels which may need to be removed if they disappear.
    std::vector<unsigned int> m_uv_channels_translated;
    std::vector<MtlID> m_material_ids_translated;
};

}}	// namespace 
