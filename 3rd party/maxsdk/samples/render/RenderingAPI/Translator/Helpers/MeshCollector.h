//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <RenderingAPI/Translator/Helpers/IMeshCollector.h>

// std
#include <vector>

class Mesh;
class MeshNormalSpec;

namespace Max
{;
namespace RenderingAPI
{;
namespace TranslationHelpers
{;

class MeshCollector : 
    public MaxSDK::RenderingAPI::TranslationHelpers::IMeshCollector
{
public:

    MeshCollector(INode& node, const View& render_view, const TimeValue t, Interval& validity);
    ~MeshCollector();

    // -- inherited from IMeshCollector
    virtual const std::vector<Point3>& get_vertices() override;
    virtual const std::vector<FaceDefinition>& get_vertex_faces() override;
    virtual const std::vector<Point3>& get_normals() override;
    virtual const std::vector<FaceDefinition>& get_normal_faces() override;
    virtual const std::vector<MtlID>& get_face_material_ids() override;
    virtual const std::vector<int>& get_UVW_channel_ids() override;
    virtual const std::vector<std::vector<Point3>>& get_UVWs() override;
    virtual const std::vector<std::vector<FaceDefinition>>& get_UVW_faces() override;
    virtual const std::vector<std::vector<UVTangentVectors>>& get_UV_tangents() override;
    virtual const std::vector<std::vector<FaceDefinition>>& get_UV_tangent_faces() override;

private:

    // Returns the mesh as evaluated from the given node
    void initialize_mesh(INode& node, const View& render_view, const TimeValue t, Interval& validity);

    // Methods that collect/cache mesh data on demand
    void collect_vertices();
    void collect_normals();
    void collect_UVWs();
    void collect_UV_tangents();

private:

    class TangentNode;
    
    // The mesh to be flattened
    Mesh* m_mesh = nullptr;
    MeshNormalSpec* m_mesh_normal_spec = nullptr;

    // Saves pointers to Mesh which needs to be deleted once translation is finished.
    Mesh* m_mesh_to_delete = nullptr;

    // The cached, generated/collected mesh data
    bool m_vertices_collected = false;
    std::vector<Point3> m_vertices;
    std::vector<FaceDefinition> m_vertex_faces;
    std::vector<MtlID> m_face_material_ids;
    bool m_normals_collected = false;
    std::vector<Point3> m_normals;
    std::vector<FaceDefinition> m_normal_faces;
    bool m_UVWs_collected = false;
    std::vector<int> m_UVW_channel_ids;
    std::vector<std::vector<Point3>> m_UVWs;
    std::vector<std::vector<FaceDefinition>> m_UVW_faces;
    bool m_UV_tangents_collected = false;
    std::vector<std::vector<FaceDefinition>> m_UV_tangent_faces;
    std::vector<std::vector<UVTangentVectors>> m_UV_tangents;
};

//==================================================================================================
// class MeshCollector::TangentNode
//==================================================================================================

// Tangents can't be shared across normal discontinuities, so we have to generate a linked list
// of tangents, for each UVW vertex, and later flatten that into a tangent-specific topology.
class MeshCollector::TangentNode
{
public:
    // Adds a face's tangent to this node, either adding it here in place if the normal of this face's
    // vertex matches that which is already stored here, or appends a new node to the linked list, if
    // the normal is different (thus introducing a discontinuity in the tangent topology, forced by the
    // discontinuity in the normals).
    // The method returns the number of nodes added to the linked list during this call, if any
    // (including this node, if it was initialized during this call)
    unsigned int add_face_tangents(const UVTangentVectors& face_tangents, const Point3& face_vertex_normal);

    // Recursively pushes back all the tangents to the given vector
    void push_back_tangents(std::vector<UVTangentVectors>& tangents) const;

    // Returns the index of the tangent node, in the linked list, which's normal matches the given one.
    unsigned int get_matching_tangent_index(const Point3& normal) const;

private:
    // Whether this node has been initialized (i.e. stores a tangent yet)
    bool m_initialized = false;
    Point3 m_normal;
    UVTangentVectors m_tangents;
    // Next entry in the linked list
    std::unique_ptr<TangentNode> m_next;
};

}}}      // namespace