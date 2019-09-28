//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_GeomObject_to_RTIGeometry.h"

// local includes
#include "../Util.h"
#include "../resource.h"
// max sdk
#include <dllutilities.h>
#include <RenderingAPI/Translator/TranslatorStatistics.h>
#include <RenderingAPI/Translator/Helpers/IMeshFlattener.h>
#include <RenderingAPI/Renderer/ICameraContainer.h>

namespace Max
{;
namespace RapidRTTranslator
{;

namespace
{
    // Returns the name for the attribute that stores UVW coordinates for the given channel on the RapidRT mesh
    CStr get_uvw_attribute_name(const unsigned int channel_id)
    {
        CStr attribute_name;
        attribute_name.printf("uvw_%u", channel_id);
        return attribute_name;
    }
    CStr get_UTangent_attribute_name(const unsigned int channel_id)
    {
        CStr attribute_name;
        attribute_name.printf("u_tangent_%u", channel_id);
        return attribute_name;
    }
    CStr get_VTangent_attribute_name(const unsigned int channel_id)
    {
        CStr attribute_name;
        attribute_name.printf("v_tangent_%u", channel_id);
        return attribute_name;
    }
}

Translator_GeomObject_to_RTIGeometry::Translator_GeomObject_to_RTIGeometry(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Object(key, translator_graph_node),
    BaseTranslator_to_RTIGeometry(translator_graph_node),
    Translator(translator_graph_node),
    m_num_triangles_translated(0)
{

}

Translator_GeomObject_to_RTIGeometry::~Translator_GeomObject_to_RTIGeometry()
{

}

MSTR Translator_GeomObject_to_RTIGeometry::GetTimingCategory() const
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMER_MESH_TRANSLATION);
}

TranslationResult Translator_GeomObject_to_RTIGeometry::Translate(const TimeValue t, Interval& validity, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
    m_num_triangles_translated = 0;

    INode* const node = GetNode();
    if(node != NULL)
    {
        // Use the mesh flattener to create one mesh per material ID
        //!! We pass the current view to the mesh flattener to enable view-dependent objects. HOWEVER, we DO NOT listen for change notifications on the
        //!! the camera: we can't do that because there's no way to tell whether the object actually depends on the view at all, and we don't want to start
        //!! re-evaluating the entire scene whenever the view changes. 
        std::unique_ptr<IMeshFlattener> mesh_flattener = TranslationHelpers::IMeshFlattener::AllocateInstance(*node, GetRenderSessionContext().GetCamera().GetView(t, validity), t, validity);
        DbgAssert(mesh_flattener != nullptr);
        if(mesh_flattener != nullptr)
        {
            const size_t num_sub_meshes = mesh_flattener->GetNumSubMeshes();
            allocate_geometry_outputs(num_sub_meshes);

            // Allocate these outside the loop to re-use the allocation when possible
            MtlID material_id = 0;
            std::vector<IPoint3> face_indices;
            std::vector<Point3> vertices;
            std::vector<Point3> normals;
            std::vector<IMeshFlattener::TextureCoordChannel> texture_coords_channels;

            // Remember previously existing UV channels, to delete them if they become unused
            const std::vector<unsigned int> previously_translated_uv_channels = m_uv_channels_translated;
            m_uv_channels_translated.clear();
            m_material_ids_translated.clear();
            m_material_ids_translated.reserve(num_sub_meshes);

            // Translate each sub-mesh individually
            for(size_t sub_mesh_index = 0; sub_mesh_index < num_sub_meshes; ++sub_mesh_index)
            {
                mesh_flattener->GetSubMesh(sub_mesh_index, material_id, face_indices, vertices, normals, texture_coords_channels);

                // Keep track of material IDs used by the object
                m_material_ids_translated.push_back(material_id);

                // Initialize the output for this sub-mesh
                const rti::GeometryHandle geometry_handle = initialize_geometry_output(sub_mesh_index, material_id);

                // Setup the sub-mesh in Rapid
                {
                    rti::TEditPtr<rti::Geometry> geometry(geometry_handle);
                    
                    RRTUtil::check_rti_result(geometry->init(rti::GEOM_TRIANGLE_MESH));

                    DbgAssert(normals.size() == vertices.size());
                    RRTUtil::check_rti_result(geometry->setAttribute3fv(rti::GEOM_ATTRIB_COORD_ARRAY, static_cast<int>(vertices.size()), reinterpret_cast<const rti::Vec3f*>(vertices.data())));
                    RRTUtil::check_rti_result(geometry->setAttribute3fv(rti::GEOM_ATTRIB_NORMAL_ARRAY, static_cast<int>(normals.size()), reinterpret_cast<const rti::Vec3f*>(normals.data())));
                    RRTUtil::check_rti_result(geometry->setAttribute1iv(rti::GEOM_ATTRIB_INDEX_ARRAY, static_cast<int>(face_indices.size()) * 3, reinterpret_cast<const int*>(face_indices.data())));

                    // Translate the UV channels
                    {
                        // Keep track of UV channels present on the mesh. We do this on the first sub-mesh only, assuming that all sub-meshes use the same set of UV channels.
                        if(sub_mesh_index == 0)
                        {
                            m_uv_channels_translated.reserve(texture_coords_channels.size());
                        }

                        // Translate each UV channel as a separate attribute
                        for(size_t i = 0; i < texture_coords_channels.size(); ++i)
                        {
                            const IMeshFlattener::TextureCoordChannel& texture_channel = texture_coords_channels[i];

                            // Setup this texture channel
                            if(DbgVerify((texture_channel.coords.size() == vertices.size())))
                            {
                                const CStr uvw_attribute_name = get_uvw_attribute_name(texture_channel.channel_id);
                                geometry->setAttribute3fv(uvw_attribute_name, static_cast<int>(texture_channel.coords.size()), reinterpret_cast<const rti::Vec3f*>(texture_channel.coords.data()));
                                const CStr u_tangent_attribute_name = get_UTangent_attribute_name(texture_channel.channel_id);
                                geometry->setAttribute3fv(u_tangent_attribute_name, static_cast<int>(texture_channel.tangentsU.size()), reinterpret_cast<const rti::Vec3f*>(texture_channel.tangentsU.data()));
                                const CStr v_tangent_attribute_name = get_VTangent_attribute_name(texture_channel.channel_id);
                                geometry->setAttribute3fv(v_tangent_attribute_name, static_cast<int>(texture_channel.tangentsV.size()), reinterpret_cast<const rti::Vec3f*>(texture_channel.tangentsV.data()));

                                // Keep track of UV channels present on the mesh. We do this on the first sub-mesh only, assuming that all sub-meshes use the same set of UV channels.
                                if(sub_mesh_index == 0)
                                {
                                    m_uv_channels_translated.push_back(texture_channel.channel_id);
                                }
                                else
                                {
                                    DbgAssert(std::find(m_uv_channels_translated.begin(), m_uv_channels_translated.end(), texture_channel.channel_id) != m_uv_channels_translated.end());
                                }
                            }
                        }

                        // Remove UV channels which are no longer present
                        for(const unsigned int previously_translated_channel_id : previously_translated_uv_channels)
                        {
                            if(std::find(m_uv_channels_translated.begin(), m_uv_channels_translated.end(), previously_translated_channel_id) == m_uv_channels_translated.end())
                            {
                                // Remove this attribute as it's no longer used (its contents are no longer valid)
                                const CStr uvw_attribute_name = get_uvw_attribute_name(previously_translated_channel_id);
                                geometry->removeAttribute(uvw_attribute_name);
                                const CStr u_tangent_attribute_name = get_UTangent_attribute_name(previously_translated_channel_id);
                                geometry->removeAttribute(u_tangent_attribute_name);
                                const CStr v_tangent_attribute_name = get_VTangent_attribute_name(previously_translated_channel_id);
                                geometry->removeAttribute(v_tangent_attribute_name);
                            }
                        }
                    }
                }

                m_num_triangles_translated += face_indices.size();
            }
            
            return TranslationResult::Success;
        }
        else
        {
            return TranslationResult::Failure;
        }
    }
    else
    {
        // No mesh: no object to translate
        allocate_geometry_outputs(0);
        return TranslationResult::Success;
    }
}

void Translator_GeomObject_to_RTIGeometry::AccumulateStatistics(TranslatorStatistics& stats) const
{
    stats.AddFaces(m_num_triangles_translated);
}

const std::vector<unsigned int>& Translator_GeomObject_to_RTIGeometry::get_uv_channels_translated() const
{
    return m_uv_channels_translated;
}

const std::vector<MtlID>& Translator_GeomObject_to_RTIGeometry::get_material_ids_translated() const
{
    return m_material_ids_translated;
}

bool Translator_GeomObject_to_RTIGeometry::CareAboutMaterialDisplacement() const
{
    return true;
}

}}	// namespace 
