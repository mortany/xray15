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

#include "MeshCollector.h"

// Max SDK
#include <RenderingAPI/Translator/Helpers/IMeshFlattener.h>
#include <maxtypes.h>
#include <point2.h>
#include <point3.h>
#include <ipoint3.h>
#include <Noncopyable.h>
#include <gutil.h>

// std
#include <vector>

class Mesh;
class MeshNormalSpec;
class INode;
class Interval;
class TriObject;
class View;

namespace Max
{;
namespace RenderingAPI
{;
namespace TranslationHelpers
{;

// 3ds Max meshes are slightly complicated beasts, optimized for storage efficiency. Vertices, normals, and UVs are stored in different, mostly
// distinct arrays in order to save memory. But renderers typically require a flattened representation with exactly one array entry for every vertex
// (including position, UV, normal).
//
// The resulting flattened mesh is divided into sub-meshes, one for each material ID present in the mesh. These sub-meshes may be stitched together
// or used separately by the renderer - depending on whether or not it supports multiple materials per mesh.
//
// Furthermore, this class encapsulates the logic for evaluating 3ds Max meshes (e.g. the different ways in which normals, uvs, etc. may be specified 
// or handled).
class MeshFlattener : 
    public MaxSDK::RenderingAPI::TranslationHelpers::IMeshFlattener,
    public MaxSDK::Util::Noncopyable,
    // Inherit some base functionality from MeshCollector, but privately to avoid external users
    // casting to MeshCollector (because this inheritance is an internal implementation detail).
    private MeshCollector
{
public:

    // This constructor fetches the mesh from the node and performs the flattening process. 
    // The validity interval is updated but not retained beyond the call to this constructor.
    MeshFlattener(INode& node, const View& render_view, const TimeValue t, Interval& validity);
    ~MeshFlattener();

    // -- inherited from IMeshFlattener
    virtual size_t GetNumSubMeshes() override;
    virtual void GetSubMesh(const size_t sub_mesh_index, MtlID& material_id, std::vector<IPoint3>& face_indices, std::vector<Point3>& vertices, std::vector<Point3>& normals, std::vector<TextureCoordChannel>& textureCoordChannels) override;

private:

    // UVW information attached to a face
    struct FaceTextureCoords
    {
        Point3 uvw[3];
        UVTangentVectors uv_tangents[3];
    };

    // Performs the flattening of the mesh
    void flatten_mesh();

    // Retrieves the UVW coords and tangents for the given face, for every texture channel present on the mesh
    void get_face_uvw_info(std::vector<FaceTextureCoords>& face_uvw, const int face_index);

private:
    
    class FlattenedVertexAttributes;
    class FlattenedVertexList;

    // Stores information about each sub-mesh found in this mesh
    // (The mesh are split into sub-meshes based on material ID)
    struct SubMeshInfo
    {
        SubMeshInfo(const MtlID material_id, const int num_faces, const int num_vertices);
        MtlID material_id;
        int num_faces;
        int num_vertices;
    };

    // The list of sub-meshes
    std::vector<SubMeshInfo> m_sub_meshes_info;

    // The flattened mesh, expressed as a list of flattened vertices
    std::vector<FlattenedVertexList> m_flattened_vertices;
};

// Attributes of a flattened vertex
class MeshFlattener::FlattenedVertexAttributes
{
public:

    // UVW information stored on a per-vertex basis
    struct VertexTextureCoords
    {
        VertexTextureCoords();
        VertexTextureCoords(const Point3& p_uvw, const UVTangentVectors& p_uv_tangents);
        bool operator==(const VertexTextureCoords& rhs) const;
        Point3 uvw;
        UVTangentVectors uv_tangents;
    };

    // Creates uninitialized attributes
    FlattenedVertexAttributes();

    // Initializes the attributes from the given face vertex
    void initialize(
        const Point3& vertex_position,
        const Point3& vertex_normal,
        const MtlID material_id,
        // The UVW information for this face, pre-compuated for efficiency (as every vertex of a face shares this information). One array element for
        // each UVW channel
        const std::vector<FaceTextureCoords>& face_uvw,
        const int face_vertex_index);

    const Point3& get_vertex_position() const;
    const Point3& get_vertex_normal() const;
    
    // Return the array of UV coordiantes and tangent basis vectors present on the mesh, one entry for each channel where Mesh::mapSupport() returns true.
    const std::vector<VertexTextureCoords>& get_vertex_UVs() const;

    MtlID get_material_id() const;

    bool operator==(const FlattenedVertexAttributes& rhs) const;

private:

    Point3 m_vertex_position;
    Point3 m_vertex_normal;
    // Stores the vertex's UV coordinates (and associated tangent vectors), one for each map channel present in the mesh. 
    // We don't store the channel IDs as it's "well understood" that we have one entry for each channel where Mesh::mapSupport() returns true.
    std::vector<VertexTextureCoords> m_vertex_UVs;
    MtlID m_material_id = 0;
};

// Maps a single unflattened mesh (from the 3ds max mesh) to one or more flattened vertices.
class MeshFlattener::FlattenedVertexList
{
public:

    FlattenedVertexList();
    explicit FlattenedVertexList(const FlattenedVertexList& from);
    explicit FlattenedVertexList(const FlattenedVertexAttributes& new_vertex_attributes);
    ~FlattenedVertexList();

    FlattenedVertexList& operator=(const FlattenedVertexList& from);

    // Adds the given vertex to the flattened mesh. A new flattened vertex is created as needed (if the attributes differ from
    // any vertex already created).
    void add_vertex_attributes(const FlattenedVertexAttributes& new_vertex_attributes);
    // Assigns new indices the the flattened mesh, while separating the mesh into sub-meshes based on material IDs.
    // Accepts a vector that stores a vertex counter for each material ID present in the mesh - this counter is incremented
    // as flattened vertices are iterated.
    void assign_flattened_vertex_indices(std::vector<int>& vertex_count_per_material_id);

    // Stores the vertex data in the given buffers, for any vertex that matches the given material ID
    void accumulate_vertex_data(const MtlID material_id, std::vector<Point3>& vertices, std::vector<Point3>& normals, std::vector<TextureCoordChannel>& texture_coords_channels) const;

    // Returns the flattened vertex index of the vertex that matches the given one
    int get_flattened_vertex_index(const FlattenedVertexAttributes& for_vertex_attributes) const;

private:

    // Whether this vertex was initialized - i.e. used so far in the mesh traversal.
    bool m_initialized;
    // The attributes of this vertex
    FlattenedVertexAttributes m_vertex_attributes;
    // The index of the vertex in the flattened mesh (which may differ from the index in the 3ds max mesh whenever)
    int m_flattened_vertex_index;
    // Linked list pointer to next flattened vertex (in case the 3ds max vertex maps to multiple flattened vertices)
    std::unique_ptr<FlattenedVertexList> m_next;
};

}}}      // namespace