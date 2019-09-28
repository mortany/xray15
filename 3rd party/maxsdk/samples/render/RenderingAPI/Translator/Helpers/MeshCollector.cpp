//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "MeshCollector.h"

// rendering api
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_MtlBase.h>
// max sdk
#include <mesh.h>
#include <object.h>
#include <MeshNormalSpec.h>

#include "3dsmax_banned.h"

using namespace MaxSDK;
using namespace MaxSDK::RenderingAPI;

namespace
{
    // (taken from mental ray integration)
    // This is a local implementation of isnan(). It's much faster than calling
    // the MSVCRT version because:
    // a. it's using float rather than double (less bits to compare, no shift operation needed)
    // b. it's inlinable
    //
    // I took this code sample out of the Cephes math library,
    // and verified its correctness against the IEEE754 standard.
    // Here's how the standard goes:
    // NaN's are represented by a bit pattern with an exponent of all 1s and a non-zero fraction. 
    // The exponent is bits 30 to 23, while the fraction is bits 22 to 0.
    // This algorithm simply verifies that bits 30 to 23 are all 1, while bits 22 to 0
    // are not all 0.
    inline int IsNaN_local(float x)
    {
        union
        {
            float f;
            unsigned int i;
        } u;

        u.f = x;

        return (((u.i & 0x7f800000) == 0x7f800000) && ((u.i & 0x007fffff) != 0));
    }

    // Checks the given float for NaN, returning 0.0 if it's a NaN
    inline float eliminate_NaN(const float input)
    {
        return !IsNaN_local(input) ? input : 0.0f;
    }
    inline Point3 eliminate_NaN(const Point3& input)
    {
        return Point3(eliminate_NaN(input.x), eliminate_NaN(input.y), eliminate_NaN(input.z));
    }

    // Fixes the given mesh normal by verifying that it's normalized and has valid values
    inline Point3 fix_normal(const Point3& input)
    {
        const Point3 no_NaN = eliminate_NaN(input);

        // Check that the normal is valid
        const float normal_length = no_NaN.FLength();
        if(fabsf(normal_length - 1.0f) < 1.0e-4f)   // already correctly normalized?
        {
            return no_NaN;
        }
        else if(normal_length > 1.0e-3f)
        {
            // Not properly normalized: normalize it
            return no_NaN.Normalize();
        }
        else
        {
            // Normal is degenerate... nothing we can do, so null it out for later handling
            return Point3(0.0f, 0.0f, 0.0f);
        }
    }
}

//==================================================================================================
// class IMeshCollector
//==================================================================================================

namespace MaxSDK
{;
namespace RenderingAPI
{;
namespace TranslationHelpers
{;

using namespace Max::RenderingAPI::TranslationHelpers;

std::unique_ptr<IMeshCollector> IMeshCollector::AllocateInstance(INode& node, const View& render_view, const TimeValue t, Interval& validity)
{
    return std::unique_ptr<IMeshCollector>(new MeshCollector(node, render_view, t, validity));
}


}}} // namespace

namespace Max
{;
namespace RenderingAPI
{;
namespace TranslationHelpers
{;

//==================================================================================================
// class MeshCollector
//==================================================================================================

MeshCollector::MeshCollector(INode& node, const View& render_view, const TimeValue t, Interval& validity)
{
    // Initialize the mesh for rendering
    initialize_mesh(node, render_view, t, validity);
}

MeshCollector::~MeshCollector()
{
    // Delete the temporary Mesh, if needed
    if(m_mesh_to_delete != nullptr)
    {
        delete m_mesh_to_delete;
        m_mesh_to_delete = nullptr;
    }
}

void MeshCollector::initialize_mesh(INode& node, const View& render_view, const TimeValue t, Interval& validity)
{
    // Should never be re-initialized
    DbgAssert(m_mesh == nullptr);
    if(m_mesh == nullptr)
    {
        const ObjectState& object_state = node.EvalWorldState(t);
        GeomObject* geom_object = dynamic_cast<GeomObject*>(object_state.obj);
        if(geom_object != nullptr)
        {
            // Get object validity
            validity &= object_state.obj->ObjectValidity(t);

            // If there's displacement from the material, then we need to update it before retrieving the mesh.
            const bool care_about_displacement = ((render_view.flags & RENDER_MESH_DISPLACEMENT_MAP) != 0);
            if(care_about_displacement)
            {
                Mtl* const mtl = node.GetMtl();
                if((mtl != nullptr) && (mtl->Requirements(-1) & MTLREQ_DISPLACEMAP))
                {
                    // We ignore the validity interval provided by Update(), as we don't care about the general animation in the material. We only 
                    // consider the animation of displacement properties, provided by DisplacementValidity().
                    Interval dummy_interval = FOREVER;
                    BaseTranslator_MtlBase::UpdateMaterialHierarchy(*mtl, t, dummy_interval);
                    validity &= mtl->DisplacementValidity(t);
                }
            }

            // Note: We assume that View::CheckForRenderAbort() alwasy returns false, as we make no provisions to handle GetRenderMesh() aborting.
            BOOL need_delete = false;
            m_mesh = geom_object->GetRenderMesh(t, &node, const_cast<View&>(render_view), need_delete);
            if(need_delete)
            {
                m_mesh_to_delete = m_mesh;
            }

            // Failed to create a TriObj? is this bad? Should we have a fall back plan? Not sure... let's see if/when we encounter the problem.
            DbgAssert(m_mesh != nullptr);
        }
        else
        {
            // Sanity check: there's likely a bug if we have an object which isn't a GeomObject
            DbgAssert(object_state.obj == nullptr);
        }

        if(m_mesh != nullptr)
        {
            m_mesh->InvalidateGeomCache();
            m_mesh->buildRenderNormals();

            // Access the mesh's specified normals interface
            m_mesh_normal_spec = m_mesh->GetSpecifiedNormals();
            if(m_mesh_normal_spec != nullptr)
            {
                // Are any normals specified?
                if((m_mesh_normal_spec->GetNumFaces() <= 0) || (m_mesh_normal_spec->GetNumNormals() <= 0))
                {
                    // Don't use specified normals
                    m_mesh_normal_spec = nullptr;
                }
            }
        }
    }
}

void MeshCollector::collect_vertices()
{
    if(!m_vertices_collected)
    {
        m_vertices.clear();
        m_vertex_faces.clear();
        m_face_material_ids.clear();

        if(m_mesh != nullptr)
        {
            // Process all vertices
            m_vertices.reserve(m_mesh->numVerts);
            for(int vert_index = 0; vert_index < m_mesh->numVerts; ++vert_index)
            {
                m_vertices.push_back(eliminate_NaN(m_mesh->verts[vert_index]));
            }

            // Process all vertex indices and faces
            m_vertex_faces.reserve(m_mesh->numFaces);
            m_face_material_ids.reserve(m_mesh->numFaces);
            for(int face_index = 0; face_index < m_mesh->numFaces; ++face_index)
            {
                Face& face = m_mesh->faces[face_index];
                m_vertex_faces.push_back(FaceDefinition(face.v[0], face.v[1], face.v[2]));
                m_face_material_ids.push_back(face.getMatID());
            }
        }

        m_vertices_collected = true;
    }
}

void MeshCollector::collect_normals()
{
    if(!m_normals_collected)
    {
        m_normals.clear();
        m_normal_faces.clear();

        if(m_mesh != nullptr)
        {
            // Determine if we should use MeshNormalSpec to get the normals
            const bool use_mesh_normal_spec = (m_mesh_normal_spec != nullptr)
                && DbgVerify(m_mesh_normal_spec->GetNumFaces() == m_mesh->numFaces)
                && DbgVerify(m_mesh_normal_spec->GetNumNormals() > 0)
                && DbgVerify(m_mesh_normal_spec->GetNormalArray() != nullptr);

            // This array is used, in the case we don't use MeshNormalSpec, to remember which is the index
            // of the first normal referred to by a RVertex, as each RVertex may store multiple normals.
            std::vector<unsigned int> first_normal_index_for_each_vertex;

            // Build the array of normal vectors
            if(!use_mesh_normal_spec)
            {
                // Get the normals from the mesh... but there can be multiple normals for each vertex, so we
                // first need to count them.
                first_normal_index_for_each_vertex.reserve(m_mesh->numVerts);
                unsigned int total_num_normals = 0;
                for(int vert_index = 0; vert_index < m_mesh->numVerts; ++vert_index)
                {
                    const RVertex& rVert = m_mesh->getRVert(vert_index);
                    const int num_normals_for_vert =
                        (rVert.rFlags & SPECIFIED_NORMAL)
                        ? 1
                        : (rVert.rFlags & NORCT_MASK);
                    first_normal_index_for_each_vertex.push_back(total_num_normals);
                    total_num_normals += num_normals_for_vert;
                }

                // Build the array of normals
                m_normals.reserve(total_num_normals);
                for(int vert_index = 0; vert_index < m_mesh->numVerts; ++vert_index)
                {
                    const RVertex& rVert = m_mesh->getRVert(vert_index);
                    const int num_normals_for_vert =
                        (rVert.rFlags & SPECIFIED_NORMAL)
                        ? 1
                        : (rVert.rFlags & NORCT_MASK);
                    if(num_normals_for_vert == 1)
                    {
                        m_normals.push_back(fix_normal(rVert.rn.getNormal()));
                    }
                    else
                    {
                        for(int i = 0; i < num_normals_for_vert; ++i)
                        {
                            const RNormal& rNormal = DbgVerify(rVert.ern != nullptr) ? rVert.ern[i] : rVert.rn;
                            m_normals.push_back(fix_normal(rNormal.getNormal()));
                        }
                    }
                }
                DbgAssert(m_normals.size() == total_num_normals);
            }
            else
            {
                // Get the array of normals from MeshNormalSpec
                const Point3* normals = m_mesh_normal_spec->GetNormalArray();
                const int num_normals = m_mesh_normal_spec->GetNumNormals();
                m_normals.reserve(num_normals);
                for(int normal_index = 0; normal_index < num_normals; ++normal_index)
                {
                    m_normals.push_back(fix_normal(normals[normal_index]));
                }
            }

            // We use this array to denote those faces for which we have computed the geometric normal
            // (lazily, as needed). We store the index of each face's geometric normal in m_normals, with 
            // ~0u for those that haven't been added yet.
            std::vector<unsigned int> face_geometric_normal_indices;

            const size_t num_vertex_normals = m_normals.size();

            // Build the array of normal indices
            m_normal_faces.reserve(m_mesh->numFaces);
            for(int face_index = 0; face_index < m_mesh->numFaces; ++face_index)
            {
                Face& face = m_mesh->faces[face_index];
                unsigned int new_normal_face_definition[3];
                for(int face_vert_index = 0; face_vert_index < 3; ++face_vert_index)
                {
                    // Find which normal (in m_normal) is to be used for this face vertex
                    unsigned int use_normal_index = ~0u;
                    if(!use_mesh_normal_spec)
                    {
                        const int vert_index = face.v[face_vert_index];
                        RVertex& rVert = m_mesh->getRVert(vert_index);
                        const int num_normals_for_vert =
                            (rVert.rFlags & SPECIFIED_NORMAL)
                            ? 1
                            : (rVert.rFlags & NORCT_MASK);
                    
                        const unsigned int first_normal_index = first_normal_index_for_each_vertex[vert_index];
                        if(rVert.rFlags & SPECIFIED_NORMAL)
                        {
                            // A single normal, specified
                            use_normal_index = first_normal_index;
                        }
                        else
                        {
                            // Multiple normals... find the one that matches this face's smoothing group
                            const int num_normals = (rVert.rFlags & NORCT_MASK);
                            for(int normal_index = 0; normal_index < num_normals; ++normal_index)
                            {
                                RNormal& rNormal = (num_normals == 1) ? rVert.rn : rVert.ern[normal_index];
                                if(face.getSmGroup() & rNormal.getSmGroup())
                                {
                                    // This normal matches the face's smoothing group: use it
                                    use_normal_index = (first_normal_index + normal_index);
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        // Get the normal index from MeshNormalSpec
                        use_normal_index = m_mesh_normal_spec->GetNormalIndex(face_index, face_vert_index);
                    }
                    DbgAssert((use_normal_index < num_vertex_normals) || (use_normal_index == ~0u));

                    // Use the geometric normal, if no normal was found or if the found normal turns out
                    // to be invalid (fix_normal() sets bad normals to (0,0,0))
                    if((use_normal_index >= num_vertex_normals)
                        || (m_normals[use_normal_index] == Point3(0.0f, 0.0f, 0.0f)))
                    {
                        // Resize stuff on first encountering a need for a geometric normal
                        if(face_geometric_normal_indices.empty())
                        {
                            face_geometric_normal_indices.resize(m_mesh->numFaces, ~0u);
                            m_normals.reserve(num_vertex_normals + m_mesh->numFaces);
                        }

                        // Compute the face's geometric normal and add it to the list of normals,
                        // if and only if it wasn't already added.
                        if(face_geometric_normal_indices[face_index] == ~0u)
                        {
                            face_geometric_normal_indices[face_index] = static_cast<unsigned int>(m_normals.size());
                            m_normals.push_back(fix_normal(m_mesh->getFaceNormal(face_index)));
                        }

                        use_normal_index = face_geometric_normal_indices[face_index];
                    }

                    DbgAssert(use_normal_index < m_normals.size());
                    new_normal_face_definition[face_vert_index] = use_normal_index;
                }

                m_normal_faces.emplace_back(new_normal_face_definition[0], new_normal_face_definition[1], new_normal_face_definition[2]);
            }
        }

        m_normals_collected = true;
    }
}

void MeshCollector::collect_UVWs()
{
    if(!m_UVWs_collected)
    {
        m_UVW_channel_ids.clear();
        m_UVWs.clear();
        m_UVW_faces.clear();

        if(m_mesh != nullptr)
        {
            // Determine what UVW channels are present on the mesh
            {
                const int num_maps = m_mesh->getNumMaps();
                m_UVW_channel_ids.reserve((num_maps > 0) ? num_maps : 0);
                for(int i = 0; i < num_maps; ++i)
                {
                    if(m_mesh->mapSupport(i) 
                        && DbgVerify(m_mesh->mapVerts(i) != nullptr)
                        && DbgVerify(m_mesh->getNumMapVerts(i) > 0)
                        && DbgVerify(m_mesh->mapFaces(i) != nullptr))
                    {
                        m_UVW_channel_ids.push_back(i);
                    }
                }
            }

            // Collect the UVWs for each channel
            m_UVWs.resize(m_UVW_channel_ids.size());
            m_UVW_faces.resize(m_UVW_channel_ids.size());
            for(size_t i = 0; i < m_UVW_channel_ids.size(); ++i)
            {
                const int current_channel_id = m_UVW_channel_ids[i];

                // Collect the UVW vertices
                std::vector<Point3>& to_UVWs = m_UVWs[i];
                const UVVert* const from_UVW = m_mesh->mapVerts(current_channel_id);
                const int num_map_verts = m_mesh->getNumMapVerts(current_channel_id);
                to_UVWs.reserve(num_map_verts);
                for(int uvw_vert_index = 0; uvw_vert_index < num_map_verts; ++uvw_vert_index)
                {
                    to_UVWs.push_back(eliminate_NaN(from_UVW[uvw_vert_index]));
                }

                // Collect the UVW faces
                std::vector<FaceDefinition>& to_UVW_faces = m_UVW_faces[i];
                const TVFace* const from_UVW_faces = m_mesh->mapFaces(current_channel_id);
                to_UVW_faces.reserve(m_mesh->numFaces);
                for(int face_index = 0; face_index < m_mesh->numFaces; ++face_index)
                {
                    const TVFace& from_face = from_UVW_faces[face_index];
                    to_UVW_faces.push_back(FaceDefinition(from_face.t[0], from_face.t[1], from_face.t[2]));
                }                
            }
        }

        m_UVWs_collected = true;
    }
}

void MeshCollector::collect_UV_tangents()
{
    if(!m_UV_tangents_collected)
    {
        m_UV_tangents.clear();

        // Need to collect the vertices and normals and UVWs first
        collect_vertices();
        collect_normals();
        collect_UVWs();

        m_UV_tangent_faces.resize(m_UVW_channel_ids.size());
        m_UV_tangents.resize(m_UVW_channel_ids.size());
        for(size_t uvw_index = 0; uvw_index < m_UVW_channel_ids.size(); ++uvw_index)
        {
            const std::vector<Point3>& uvws = m_UVWs[uvw_index];
            const std::vector<FaceDefinition>& uvw_faces = m_UVW_faces[uvw_index];

            // Calculate and accumulate the tangents for this channel, using class TangentNode to automatically
            // split tangents where the normals are discontinuous.
            std::vector<TangentNode> tangent_nodes(uvws.size());
            unsigned int total_num_tangents = 0;
            for(unsigned int face_index = 0; face_index < uvw_faces.size(); ++face_index)
            {
                const FaceDefinition& vertex_face = m_vertex_faces[face_index];
                const FaceDefinition& normal_face = m_normal_faces[face_index];
                const FaceDefinition& uvw_face = uvw_faces[face_index];

                // Calculate the tangents for this face
                const UVTangentVectors face_tangents = ComputeUVTangents(m_vertices[vertex_face.indices[0]], m_vertices[vertex_face.indices[1]], m_vertices[vertex_face.indices[2]], uvws[uvw_face.indices[0]].XY(), uvws[uvw_face.indices[1]].XY(), uvws[uvw_face.indices[2]].XY());

                // Add the face's tangent to each vertex is refers to
                total_num_tangents += tangent_nodes[uvw_face.indices[0]].add_face_tangents(face_tangents, m_normals[normal_face.indices[0]]);
                total_num_tangents += tangent_nodes[uvw_face.indices[1]].add_face_tangents(face_tangents, m_normals[normal_face.indices[1]]);
                total_num_tangents += tangent_nodes[uvw_face.indices[2]].add_face_tangents(face_tangents, m_normals[normal_face.indices[2]]);
            }

            // Now flatten the list of tangent vectors (as each entry in tangent_nodes is a linked list,
            // we need to flatten the whole thing and re-index the faces accordingly).
            // We also count the tangents, such that we know the index at which the first tangent
            // in each linked list will start once flattened.
            std::vector<unsigned int> tangent_remapped_indices;
            tangent_remapped_indices.reserve(uvws.size());
            std::vector<UVTangentVectors>& out_tangents = m_UV_tangents[uvw_index];
            out_tangents.reserve(total_num_tangents);
            for(const TangentNode& tangent_node : tangent_nodes)
            {
                // Remember the index of the first tangent to which this UVW vertex maps
                tangent_remapped_indices.push_back(static_cast<unsigned int>(out_tangents.size()));
                // Add each of the tangents in the linked list to the final list
                tangent_node.push_back_tangents(out_tangents);
            }
            DbgVerify(out_tangents.size() == total_num_tangents);

            // Build the face definitions for the tangents. The topology is, to start with, identical to that of the UVW
            // vertices, except that some tangents may have been duplicated because of differing normals and therefore have
            // to be remapped.
            std::vector<FaceDefinition>& out_tangent_faces = m_UV_tangent_faces[uvw_index];
            out_tangent_faces.reserve(uvw_faces.size());
            for(unsigned int face_index = 0; face_index < uvw_faces.size(); ++face_index)
            {
                const FaceDefinition& normal_face = m_normal_faces[face_index];
                const FaceDefinition& uvw_face = uvw_faces[face_index];
                unsigned int tangent_face_verts[3];
                for(int face_vert_index = 0; face_vert_index < 3; ++face_vert_index)
                {
                    // Get the tangents linked list that corresponds to this vertex
                    const unsigned int uvw_index = uvw_face.indices[face_vert_index];
                    const TangentNode& tangent_node = tangent_nodes[uvw_index];
                    const unsigned int tangent_index = 
                        tangent_remapped_indices[uvw_index]
                        + tangent_node.get_matching_tangent_index(m_normals[normal_face.indices[face_vert_index]]);
                    DbgAssert(tangent_index < out_tangents.size());
                    tangent_face_verts[face_vert_index] = tangent_index;
                }
                out_tangent_faces.emplace_back(tangent_face_verts[0], tangent_face_verts[1], tangent_face_verts[2]);
            }
        }
        
        m_UV_tangents_collected = true;
    }
}

const std::vector<Point3>& MeshCollector::get_vertices()  
{
    collect_vertices();
    return m_vertices;
}

const std::vector<MeshCollector::FaceDefinition>& MeshCollector::get_vertex_faces()
{
    collect_vertices();
    return m_vertex_faces;
}

const std::vector<Point3>& MeshCollector::get_normals()  
{
    collect_normals();
    return m_normals;
}

const std::vector<MeshCollector::FaceDefinition>& MeshCollector::get_normal_faces()
{
    collect_normals();
    return m_normal_faces;
}

const std::vector<MtlID>& MeshCollector::get_face_material_ids()  
{
    collect_vertices();
    return m_face_material_ids;
}

const std::vector<int>& MeshCollector::get_UVW_channel_ids() 
{
    collect_UVWs();
    return m_UVW_channel_ids;
}

const std::vector<std::vector<Point3>>& MeshCollector::get_UVWs()
{
    collect_UVWs();
    return m_UVWs;
}

const std::vector<std::vector<MeshCollector::FaceDefinition>>& MeshCollector::get_UVW_faces()
{
    collect_UVWs();
    return m_UVW_faces;
}

const std::vector<std::vector<UVTangentVectors>>& MeshCollector::get_UV_tangents()
{
    collect_UV_tangents();
    return m_UV_tangents;
}

const std::vector<std::vector<MeshCollector::FaceDefinition>>& MeshCollector::get_UV_tangent_faces()
{
    collect_UV_tangents();
    return m_UV_tangent_faces;
}


//==================================================================================================
// class MeshCollector::TangentNode
//==================================================================================================

unsigned int MeshCollector::TangentNode::add_face_tangents(const UVTangentVectors& face_tangents, const Point3& face_vertex_normal)
{
    if(!m_initialized)
    {
        m_initialized = true;
        m_normal = face_vertex_normal;
        m_tangents = face_tangents;
        return 1;
    }
    else if(m_normal == face_vertex_normal)
    {
        // Add in the tangent (will be normalized later)
        m_tangents.tangentU += face_tangents.tangentU;
        m_tangents.tangentV += face_tangents.tangentV;
        return 0;
    }
    else
    {
        // Can't add tangents to this node, because normals don't match, so add further down the linked list
        // (extending the list as needed)
        if(m_next == nullptr)
        {
            m_next = std::make_unique<TangentNode>();
        }

        return m_next->add_face_tangents(face_tangents, face_vertex_normal);
    }
}

void MeshCollector::TangentNode::push_back_tangents(std::vector<UVTangentVectors>& tangents) const
{
    // Ignore uninitialized nodes (meaning they're not used by any faces)
    if(m_initialized)
    {
        tangents.push_back(UVTangentVectors(Normalize(m_tangents.tangentU), Normalize(m_tangents.tangentV)));
        if(m_next != nullptr)
        {
            m_next->push_back_tangents(tangents);
        }
    }
}

// Returns the index of the tangent node, in the linked list, which's normal matches the given one.
unsigned int MeshCollector::TangentNode::get_matching_tangent_index(const Point3& normal) const
{
    if(normal == m_normal)
    {
        return 0;
    }
    else if(m_next != nullptr)
    {
        return 1 + m_next->get_matching_tangent_index(normal);
    }
    else
    {
        // Normal not matched... not supposed to happen
        DbgAssert(false);
        return 0;
    }
}

}}}      // namespace