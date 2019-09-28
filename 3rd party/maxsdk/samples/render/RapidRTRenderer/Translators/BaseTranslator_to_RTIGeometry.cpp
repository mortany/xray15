//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "BaseTranslator_to_RTIGeometry.h"

namespace Max
{;
namespace RapidRTTranslator
{;

// ITranslatorOutput which adds a MtlID to a regular handle output
class BaseTranslator_to_RTIGeometry::GeometryOutput : public RTIHandleTranslatorOutput<rti::GeometryHandle>
{
public:
    explicit GeometryOutput(const MtlID material_id)
        : m_material_id(material_id)
    {
    }

    MtlID get_material_id() const
    {
        return m_material_id;
    }

private:

    const MtlID m_material_id;
};

BaseTranslator_to_RTIGeometry::BaseTranslator_to_RTIGeometry(TranslatorGraphNode& translator_graph_node)
	: BaseTranslator_to_RTI(translator_graph_node),
    Translator(translator_graph_node)
{

}

BaseTranslator_to_RTIGeometry::~BaseTranslator_to_RTIGeometry()
{

}

size_t BaseTranslator_to_RTIGeometry::get_num_output_geometries() const
{
    return GetNumOutputs();
}

std::pair<rti::GeometryHandle, MtlID> BaseTranslator_to_RTIGeometry::get_output_geometry(const size_t index) const
{
    DbgAssert(index < get_num_output_geometries());

    const auto output = GetOutput<GeometryOutput>(index);
    std::pair<rti::GeometryHandle, MtlID> output_geometry;
    output_geometry.first = (output != nullptr) ? output->get_handle() : rti::GeometryHandle();
    output_geometry.second = (output != nullptr) ? output->get_material_id() : 0;
    return output_geometry;
}

void BaseTranslator_to_RTIGeometry::allocate_geometry_outputs(const size_t num_geometries)
{
    SetNumOutputs(num_geometries);
}

rti::GeometryHandle BaseTranslator_to_RTIGeometry::initialize_geometry_output(const size_t index, const MtlID material_id)
{
    // Get the existing output
    const auto output = GetOutput<GeometryOutput>(index);
    // See if the output wasn't initialized, or if it needs to be re-initialized with a new material ID
    if((output == nullptr) || (output->get_material_id() != material_id))
    {
        std::shared_ptr<GeometryOutput> new_output(new GeometryOutput(material_id));
        SetOutput(index, new_output);
        return new_output->get_handle();
    }
    else
    {
        return output->get_handle();
    }
}


}}	// namespace 
