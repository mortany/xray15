//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "MeshFlattener.h"

// Max SDK
#include <mesh.h>
#include <MeshNormalSpec.h>
#include <object.h>
#include <triobj.h>

#include "3dsmax_banned.h"

using namespace MaxSDK::RenderingAPI;

namespace
{
    bool operator==(const UVTangentVectors& lhs, const UVTangentVectors& rhs)
    {
        return (lhs.tangentU == rhs.tangentU)
            && (lhs.tangentV == rhs.tangentV);
    }
}

//==================================================================================================
// class IMeshFlattener
//==================================================================================================

namespace MaxSDK
{;
namespace RenderingAPI
{;
namespace TranslationHelpers
{;

using namespace Max::RenderingAPI::TranslationHelpers;

std::unique_ptr<IMeshFlattener> IMeshFlattener::AllocateInstance(INode& node, const View& render_view, const TimeValue t, Interval& validity)
{
    return std::unique_ptr<IMeshFlattener>(new MeshFlattener(node, render_view, t, validity));
}


}}} // namespace

namespace Max
{;
namespace RenderingAPI
{;
namespace TranslationHelpers
{;

//==================================================================================================
// class MeshFlattener::SubMeshInfo
//==================================================================================================

MeshFlattener::SubMeshInfo::SubMeshInfo(const MtlID m, const int nf, const int nv)
    : material_id(m),
    num_faces(nf),
    num_vertices(nv)
{

}

//==================================================================================================
// class MeshFlattener
//==================================================================================================

MeshFlattener::MeshFlattener(INode& node, const View& render_view, const TimeValue t, Interval& validity)
    : MeshCollector(node, render_view, t, validity)
{
    // Flatten the mesh
    flatten_mesh();
}

MeshFlattener::~MeshFlattener()
{

}

size_t MeshFlattener::GetNumSubMeshes() 
{
    return m_sub_meshes_info.size();
}

void MeshFlattener::get_face_uvw_info(std::vector<FaceTextureCoords>& face_uvw, const int face_index)
{
    const std::vector<std::vector<Point3>>& uvws = get_UVWs();
    const std::vector<std::vector<FaceDefinition>>& uvw_faces = get_UVW_faces();
    const std::vector<std::vector<UVTangentVectors>>& tangents = get_UV_tangents();
    const std::vector<std::vector<FaceDefinition>>& tangent_faces = get_UV_tangent_faces();

    // Process each UVW channel
    face_uvw.resize(uvws.size());
    for(int i = 0; i < uvws.size(); ++i)
    {
        // Get the face's UVWs
        const std::vector<Point3>& channel_uvws = uvws[i];
        const FaceDefinition& uvw_face = uvw_faces[i][face_index];
        face_uvw[i].uvw[0] = channel_uvws[uvw_face.indices[0]];
        face_uvw[i].uvw[1] = channel_uvws[uvw_face.indices[1]];
        face_uvw[i].uvw[2] = channel_uvws[uvw_face.indices[2]];

        // Get the face's tangents
        const std::vector<UVTangentVectors>& channel_tangents = tangents[i];
        const FaceDefinition& tangent_face = tangent_faces[i][face_index];
        face_uvw[i].uv_tangents[0] = channel_tangents[tangent_face.indices[0]];
        face_uvw[i].uv_tangents[1] = channel_tangents[tangent_face.indices[1]];
        face_uvw[i].uv_tangents[2] = channel_tangents[tangent_face.indices[2]];
    }
}

void MeshFlattener::GetSubMesh(
    const size_t sub_mesh_index, 
    MtlID& material_id, 
    std::vector<IPoint3>& out_face_indices, 
    std::vector<Point3>& out_vertices,
    std::vector<Point3>& out_normals,
    std::vector<TextureCoordChannel>& out_textureCoordChannels) 
{
    const std::vector<int>& mesh_uvw_channels_present = get_UVW_channel_ids();
    const std::vector<Point3>& collected_vertices = get_vertices();
    const std::vector<FaceDefinition>& collected_vertex_faces = get_vertex_faces();
    const std::vector<Point3>& collected_normals = get_normals();
    const std::vector<FaceDefinition>& collected_normal_faces = get_normal_faces();
    const std::vector<MtlID>& collected_face_material_ids = get_face_material_ids();
    DbgAssert((collected_vertex_faces.size() == collected_normal_faces.size())
        && (collected_face_material_ids.size() == collected_vertex_faces.size()));

    DbgAssert(sub_mesh_index < m_sub_meshes_info.size());
    if(sub_mesh_index < m_sub_meshes_info.size())
    {
        const SubMeshInfo& sub_mesh_info = m_sub_meshes_info[sub_mesh_index];
        material_id = sub_mesh_info.material_id;

        // Allocate vertex and face data up-front, to re-use dynamically allocated storage and avoid re-allocating it for every face or vertex
        FlattenedVertexAttributes vertex_attributes[3];
        std::vector<FaceTextureCoords> face_uvw(mesh_uvw_channels_present.size());

        // Get the faces information
        out_face_indices.resize(sub_mesh_info.num_faces);
        for(int mesh_face_index = 0, sub_mesh_face_index = 0; mesh_face_index < collected_vertex_faces.size(); ++mesh_face_index)
        {
            if(collected_face_material_ids[mesh_face_index] == material_id)
            {
                // Get the vertex indices for this face
                const FaceDefinition& vertex_face = collected_vertex_faces[mesh_face_index];
                const FaceDefinition& normal_face = collected_normal_faces[mesh_face_index];
                DbgAssert((vertex_face.indices[0] < m_flattened_vertices.size()) && (vertex_face.indices[1] < m_flattened_vertices.size()) && (vertex_face.indices[2] < m_flattened_vertices.size()));
                DbgAssert(sub_mesh_face_index < out_face_indices.size());

                // Pre-compute the UVW information for this face
                get_face_uvw_info(face_uvw, mesh_face_index);

                // Initialize the vertex attributes before we try to find a match in the list
                vertex_attributes[0].initialize(collected_vertices[vertex_face.indices[0]], collected_normals[normal_face.indices[0]], material_id, face_uvw, 0);
                vertex_attributes[1].initialize(collected_vertices[vertex_face.indices[1]], collected_normals[normal_face.indices[1]], material_id, face_uvw, 1);
                vertex_attributes[2].initialize(collected_vertices[vertex_face.indices[2]], collected_normals[normal_face.indices[2]], material_id, face_uvw, 2);

                // Find each vertex in the list and fetch its index in the flattened mesh
                out_face_indices[sub_mesh_face_index] = IPoint3(
                    m_flattened_vertices[vertex_face.indices[0]].get_flattened_vertex_index(vertex_attributes[0]),
                    m_flattened_vertices[vertex_face.indices[1]].get_flattened_vertex_index(vertex_attributes[1]),
                    m_flattened_vertices[vertex_face.indices[2]].get_flattened_vertex_index(vertex_attributes[2]));
                DbgAssert((out_face_indices[sub_mesh_face_index].x < sub_mesh_info.num_vertices) && (out_face_indices[sub_mesh_face_index].y < sub_mesh_info.num_vertices) && (out_face_indices[sub_mesh_face_index].z < sub_mesh_info.num_vertices));
                ++sub_mesh_face_index;
            }
        }

        out_vertices.resize(sub_mesh_info.num_vertices);
        out_normals.resize(sub_mesh_info.num_vertices);

        // Initialize the array of UV vertices (based on what UV channels are present on the mesh)
        {
            out_textureCoordChannels.resize(0);
            out_textureCoordChannels.reserve(mesh_uvw_channels_present.size());
            for(int i = 0; i < mesh_uvw_channels_present.size(); ++i)
            {
                out_textureCoordChannels.emplace_back(mesh_uvw_channels_present[i], sub_mesh_info.num_vertices);
            }   
        }

        // Accumulate the vertices of the flattened mesh
        for(const FlattenedVertexList& flattened_vertex : m_flattened_vertices)
        {
            flattened_vertex.accumulate_vertex_data(material_id, out_vertices, out_normals, out_textureCoordChannels);
        }
    }
    else
    {
        material_id = 0;
        out_face_indices.clear();
        out_vertices.clear();
        out_normals.clear();
        out_textureCoordChannels.clear();
    }
}

void MeshFlattener::flatten_mesh()
{
    const std::vector<Point3>& vertices = get_vertices();
    const std::vector<FaceDefinition>& vertex_faces = get_vertex_faces();
    const std::vector<Point3>& normals = get_normals();
    const std::vector<FaceDefinition>& normal_faces = get_normal_faces();
    const std::vector<MtlID>& face_material_ids = get_face_material_ids();
    const std::vector<int>& uvw_channel_ids = get_UVW_channel_ids();
    DbgAssert((vertex_faces.size() == normal_faces.size())
        && (face_material_ids.size() == vertex_faces.size()));

    // Allocate storage for the flattened vertices
    m_flattened_vertices.resize(vertices.size());

    // This counts the number of faces, for each material ID present in the mesh
    std::vector<int> face_count_per_material_id(20, 0);      // generous initial size to avoid resizing in most cases

    // Phase 1: flatten the vertices
    {
        // Allocate vertex and face data up-front, to re-use dynamically allocated storage and avoid re-allocating it for every face or vertex
        FlattenedVertexAttributes vertex_attributes[3];
        std::vector<FaceTextureCoords> face_uvw(uvw_channel_ids.size());

        for(int face_index = 0; face_index < vertex_faces.size(); ++face_index)
        {
            const MtlID material_id = face_material_ids[face_index];

            // Count faces for each material ID
            if(face_count_per_material_id.size() <= material_id)
            {
                face_count_per_material_id.resize(material_id + 1, 0);
            }
            ++(face_count_per_material_id[material_id]);

            // Pre-compute the UVW information for this face
            get_face_uvw_info(face_uvw, face_index);

            // Flatten this face's vertices
            const FaceDefinition& vertex_face = vertex_faces[face_index];
            const FaceDefinition& normal_face = normal_faces[face_index];
            vertex_attributes[0].initialize(vertices[vertex_face.indices[0]], normals[normal_face.indices[0]], material_id, face_uvw, 0);
            vertex_attributes[1].initialize(vertices[vertex_face.indices[1]], normals[normal_face.indices[1]], material_id, face_uvw, 1);
            vertex_attributes[2].initialize(vertices[vertex_face.indices[2]], normals[normal_face.indices[2]], material_id, face_uvw, 2);
            m_flattened_vertices[vertex_face.indices[0]].add_vertex_attributes(vertex_attributes[0]);
            m_flattened_vertices[vertex_face.indices[1]].add_vertex_attributes(vertex_attributes[1]);
            m_flattened_vertices[vertex_face.indices[2]].add_vertex_attributes(vertex_attributes[2]);
        }
    }

    // Phase 2: assign new indices to the flattened vertices, all the while subdividing the mesh into sub-meshes for each material ID.
    std::vector<int> vertex_count_per_material_id(face_count_per_material_id.size(), 0);      // Counts the number of vertices for each material ID present in the mesh
    for(FlattenedVertexList& flattened_vertex : m_flattened_vertices)
    {
        flattened_vertex.assign_flattened_vertex_indices(vertex_count_per_material_id);
    }

    // Phase 3: setup the sub-meshes (discarding any material ID which wasn't actually in use by the mesh)
    DbgAssert(face_count_per_material_id.size() == vertex_count_per_material_id.size());
    const size_t num_material_ids = face_count_per_material_id.size();
    m_sub_meshes_info.reserve(num_material_ids);
    for(size_t material_id = 0; material_id < num_material_ids; ++material_id)
    {
        DbgAssert((face_count_per_material_id[material_id] == 0) == (vertex_count_per_material_id[material_id] == 0)); 
        if(face_count_per_material_id[material_id] > 0)      //  ignore unused material IDs
        {
            m_sub_meshes_info.emplace_back(static_cast<MtlID>(material_id), face_count_per_material_id[material_id], vertex_count_per_material_id[material_id]);
        }
    }
}

//==================================================================================================
// class MeshFlattener::FlattenedVertexAttributes
//==================================================================================================

MeshFlattener::FlattenedVertexAttributes::FlattenedVertexAttributes()
{

}

void MeshFlattener::FlattenedVertexAttributes::initialize(
    const Point3& vertex_position,
    const Point3& vertex_normal,
    const MtlID material_id,
    const std::vector<FaceTextureCoords>& face_uvw,
    const int face_vertex_index)
{
    m_vertex_position = vertex_position;
    m_vertex_normal = vertex_normal;
    m_material_id = material_id;

    // Initialize the UV channels.
    m_vertex_UVs.resize(0);
    m_vertex_UVs.reserve(face_uvw.size());
    for(int i = 0; i < face_uvw.size(); ++i)
    {
        m_vertex_UVs.emplace_back(face_uvw[i].uvw[face_vertex_index], face_uvw[i].uv_tangents[face_vertex_index]);
    }
}

const Point3& MeshFlattener::FlattenedVertexAttributes::get_vertex_position() const
{
    return m_vertex_position;
}

const Point3& MeshFlattener::FlattenedVertexAttributes::get_vertex_normal() const
{
    return m_vertex_normal;
}

const std::vector<MeshFlattener::FlattenedVertexAttributes::VertexTextureCoords>& MeshFlattener::FlattenedVertexAttributes::get_vertex_UVs() const
{
    return m_vertex_UVs;
}

MtlID MeshFlattener::FlattenedVertexAttributes::get_material_id() const
{
    return m_material_id;
}

bool MeshFlattener::FlattenedVertexAttributes::operator==(const FlattenedVertexAttributes& rhs) const
{
    return (m_vertex_position == rhs.m_vertex_position)
        && (m_vertex_normal == rhs.m_vertex_normal)
        && (m_vertex_UVs == rhs.m_vertex_UVs)
        && (m_material_id == rhs.m_material_id);
}

//==================================================================================================
// class MeshFlattener::FlattenedVertexList
//==================================================================================================

MeshFlattener::FlattenedVertexList::FlattenedVertexList()
    : m_initialized(false),
    m_next(nullptr),
    m_flattened_vertex_index(0)
{

}

MeshFlattener::FlattenedVertexList::FlattenedVertexList(const FlattenedVertexList& from)
    : m_initialized(from.m_initialized),
    m_vertex_attributes(from.m_vertex_attributes),
    m_flattened_vertex_index(from.m_flattened_vertex_index),
    m_next((from.m_next == nullptr) ? nullptr : new FlattenedVertexList(*(from.m_next)))
{

}

MeshFlattener::FlattenedVertexList::FlattenedVertexList(const FlattenedVertexAttributes& new_vertex_attributes)
    : m_initialized(true),
    m_vertex_attributes(new_vertex_attributes),
    m_next(nullptr),
    m_flattened_vertex_index(0)
{

}

MeshFlattener::FlattenedVertexList::~FlattenedVertexList()
{

}

MeshFlattener::FlattenedVertexList& MeshFlattener::FlattenedVertexList::operator=(const FlattenedVertexList& from)
{
    m_initialized = from.m_initialized;
    m_vertex_attributes = from.m_vertex_attributes;
    m_flattened_vertex_index = from.m_flattened_vertex_index;
    m_next.reset((from.m_next == nullptr) ? nullptr : new FlattenedVertexList(*(from.m_next)));

    return *this;
}

void MeshFlattener::FlattenedVertexList::add_vertex_attributes(const FlattenedVertexAttributes& new_vertex_attributes)
{
    if(!m_initialized)
    {
        // Uninitialized: adopt the new attributes
        m_vertex_attributes = new_vertex_attributes;
        m_initialized = true;
    }
    else
    {
        // See if we can merge the attributes with any attributes in the list
        if(m_vertex_attributes == new_vertex_attributes)
        {
            // Vertex attributes merged successfully: no need to create a new flattened vertex
        }
        else if(m_next == nullptr)
        {
            // Reached the end of the list: a new flattened vertex must be created.
            m_next.reset(new FlattenedVertexList(new_vertex_attributes));
        }
        else
        {
            // Continue traversing the list
            m_next->add_vertex_attributes(new_vertex_attributes);
        }
    }
}

void MeshFlattener::FlattenedVertexList::assign_flattened_vertex_indices(std::vector<int>& vertex_count_per_material_id)
{
    if(m_initialized)
    {
        DbgAssert(m_vertex_attributes.get_material_id() < vertex_count_per_material_id.size());
        m_flattened_vertex_index = (vertex_count_per_material_id[m_vertex_attributes.get_material_id()])++;

        // Traverse the list of flattened vertices
        if(m_next != nullptr)
        {
            m_next->assign_flattened_vertex_indices(vertex_count_per_material_id);
        }
    }
}

void MeshFlattener::FlattenedVertexList::accumulate_vertex_data(
    const MtlID material_id, 
    std::vector<Point3>& vertices, 
    std::vector<Point3>& normals, 
    std::vector<TextureCoordChannel>& texture_coords_channels) const
{
    if(m_initialized && (m_vertex_attributes.get_material_id() == material_id))
    {
        DbgAssert((m_flattened_vertex_index < vertices.size()) && (m_flattened_vertex_index < normals.size()));
        vertices[m_flattened_vertex_index] = m_vertex_attributes.get_vertex_position();
        normals[m_flattened_vertex_index] = m_vertex_attributes.get_vertex_normal();

        // Accumulate the UV coordinates for each channel present in the mesh
        const std::vector<FlattenedVertexAttributes::VertexTextureCoords>& vertex_UVs = m_vertex_attributes.get_vertex_UVs();

        if(DbgVerify(vertex_UVs.size() == texture_coords_channels.size()))
        {
            for(int i = 0; i < vertex_UVs.size(); ++i)
            {
                TextureCoordChannel& texture_channel = texture_coords_channels[i];
                const FlattenedVertexAttributes::VertexTextureCoords& vertex_texture_coord = vertex_UVs[i];
                texture_channel.coords[m_flattened_vertex_index] = vertex_texture_coord.uvw;
                texture_channel.tangentsU[m_flattened_vertex_index] = vertex_texture_coord.uv_tangents.tangentU;
                texture_channel.tangentsV[m_flattened_vertex_index] = vertex_texture_coord.uv_tangents.tangentV;
            }
        }
    }

    // Process next entry in the list
    if(m_next != nullptr)
    {
        m_next->accumulate_vertex_data(material_id, vertices, normals, texture_coords_channels);
    }
}

int MeshFlattener::FlattenedVertexList::get_flattened_vertex_index(const FlattenedVertexAttributes& for_vertex_attributes) const
{
    // Check if the given vertex was/would have been merged into this one.
    if(m_vertex_attributes == for_vertex_attributes)
    {
        return m_flattened_vertex_index;
    }
    else if(m_next != nullptr)
    {
        // Continue searching the list
        return m_next->get_flattened_vertex_index(for_vertex_attributes);
    }
    else
    {
        // Reached the end of the list without a match: that's a bug
        DbgAssert(false);
        return 0;   // return a (supposedly) valid value to avoid crashes and such
    }
}

//==================================================================================================
// class MeshFlattener::FlattenedVertexAttributes::VertexTextureCoords
//==================================================================================================

MeshFlattener::FlattenedVertexAttributes::VertexTextureCoords::VertexTextureCoords()
{
}

MeshFlattener::FlattenedVertexAttributes::VertexTextureCoords::VertexTextureCoords(const Point3& p_uvw, const UVTangentVectors& p_uv_tangents)
    : uvw(p_uvw), 
    uv_tangents(p_uv_tangents) 
{
}

bool MeshFlattener::FlattenedVertexAttributes::VertexTextureCoords::operator==(const VertexTextureCoords& rhs) const
{
    return (uvw == rhs.uvw)
        && (uv_tangents == rhs.uv_tangents);
}


}}}      // namespace