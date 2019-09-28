//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_GeomObjectINode_to_RTIObject.h"

// local includes
#include "Translator_GeomObject_to_RTIGeometry.h"
#include "Translator_NodeWireColor_to_RTIShader.h"
#include "BaseTranslator_Mtl_to_RTIShader.h"
#include "Translator_RTIRenderAttributes.h"
#include "TypeUtil.h"
#include "../resource.h"
// Max SDK
#include <dllutilities.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <RenderingAPI/Translator/ITranslationManager.h>
#include <RenderingAPI/Translator/TranslatorStatistics.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_GeomObjectINode_to_RTIObject::Translator_GeomObjectINode_to_RTIObject(
    const Key& key, 
    TranslatorGraphNode& translator_graph_node)

    : BaseTranslator_INode_to_RTIObject(key, NotifierType_Node_Geom, translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_GeomObjectINode_to_RTIObject::~Translator_GeomObjectINode_to_RTIObject()
{

}

bool Translator_GeomObjectINode_to_RTIObject::can_translate(const Key& key, const TimeValue t)
{
    INode& node = key;
    const ObjectState& object_state = node.EvalWorldState(t);
    GeomObject* geom_object = TypeUtil::to_GeomObject(object_state.obj);
    return (geom_object != nullptr);
}

const BaseTranslator_to_RTIGeometry* Translator_GeomObjectINode_to_RTIObject::acquire_geometry_translator(const TimeValue t, Interval& /*validity*/, ITranslationProgress& translation_progress, TranslationResult& result)
{
    // Retrieve the node pool to be used with this node
    const std::shared_ptr<const INodeInstancingPool> node_pool = GetNodePool(t);
    if(DbgVerify(node_pool != nullptr))
    {
        const Translator_GeomObject_to_RTIGeometry* const geom_translator = AcquireChildTranslator<Translator_GeomObject_to_RTIGeometry>(Translator_GeomObject_to_RTIGeometry::Key(node_pool), t, translation_progress, result);

        // Save the set of UVW and material IDs present on the mesh. Note that these aren't updated when the mesh is re-translated, but that's OK
        // because we only report the missing UVW channels on the first frame of an offline render.
        if(geom_translator != nullptr)
        {
            m_uv_channels_present = geom_translator->get_uv_channels_translated();
            m_material_ids_present = geom_translator->get_material_ids_translated();
        }
        else
        {
            m_uv_channels_present.clear();
            m_material_ids_present.clear();
        }

        return geom_translator;
    }
    else
    {
        return nullptr;
    }
}

const BaseTranslator_to_RTIShader* Translator_GeomObjectINode_to_RTIObject::acquire_shader_translator(const MtlID material_id, const TimeValue t, Interval& /*validity*/, ITranslationProgress& translation_progress, TranslationResult& result)
{
    INode& node = GetNode();
    Mtl* material = GetMaterial();

    // Resolve multi-material
    if((material != nullptr) && material->IsMultiMtl())
    {
        const int num_sub_materials = material->NumSubMtls();
        if(num_sub_materials > 0)
        {
            material = material->GetSubMtl(material_id % num_sub_materials);
        }
        else
        {
            material = NULL;
        }
    }

    // Translate the material
    const BaseTranslator_to_RTIShader* const material_translator =
        (material != NULL)
        ? AcquireChildTranslator<BaseTranslator_to_RTIShader>(BaseTranslator_Mtl_to_RTIShader::Key(material), t, translation_progress, result)
        // If no material, translate 'wire color' material
        : AcquireChildTranslator<Translator_NodeWireColor_to_RTIShader>(Translator_NodeWireColor_to_RTIShader::Key(node), t, translation_progress, result);

    if(result == TranslationResult::Failure)
    {
        // Output error
        TSTR msg;
        if(material != nullptr)
        {
            msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_MTL_TRANSLATION_FAILURE), material->GetName().data(), material->ClassName().data(), node.GetName());
        }
        else
        {
            msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_WIREMTL_TRANSLATION_FAILURE), node.GetName());
        }
        GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Error, msg);
    }

    return material_translator;
}

void Translator_GeomObjectINode_to_RTIObject::AccumulateStatistics(TranslatorStatistics& stats) const
{
    stats.AddGeomObject(get_num_output_objects());
}

MSTR Translator_GeomObjectINode_to_RTIObject::GetTimingCategory() const
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMER_MESH_TRANSLATION);
}

bool Translator_GeomObjectINode_to_RTIObject::CareAboutMissingUVWChannels() const
{
    // Yes, we want to report missing UVW channels for this translator
    return true;
}

std::vector<unsigned int> Translator_GeomObjectINode_to_RTIObject::GetMeshUVWChannelIDs() const
{
    return m_uv_channels_present;
}

std::vector<MtlID> Translator_GeomObjectINode_to_RTIObject::GetMeshMaterialIDs() const
{
    return m_material_ids_present;
}

}}	// namespace 
