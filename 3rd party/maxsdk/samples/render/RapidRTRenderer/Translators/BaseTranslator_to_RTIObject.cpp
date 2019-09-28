//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "BaseTranslator_to_RTIObject.h"

#include "BaseTranslator_to_RTIGeometry.h"
#include "BaseTranslator_to_RTIShader.h"
#include "Translator_RTIRenderAttributes.h"
// rti
#include <rti/scene/object.h>

namespace Max
{;
namespace RapidRTTranslator
{;

BaseTranslator_to_RTIObject::BaseTranslator_to_RTIObject(TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_to_RTI(translator_graph_node),
    Translator(translator_graph_node)
{

}

BaseTranslator_to_RTIObject::~BaseTranslator_to_RTIObject()
{

}

TranslationResult BaseTranslator_to_RTIObject::Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& /*keyframesNeeded*/)
{
    // Translate the rti::Geometry. We need to perform this first in order to know how many sub-meshes we'll have to instantiate.
	TranslationResult result;
    const BaseTranslator_to_RTIGeometry* geometry_translator = acquire_geometry_translator(t, validity, translation_progress, result);
	if(geometry_translator != NULL)
	{
        // Initialize the translator outputs: one for each rti::Geometry that we'll be instantiating.
        const size_t num_geometries = geometry_translator->get_num_output_geometries();
        SetNumOutputs(num_geometries);

        // Fetch the transform matrices
        Matrix3 transform, motion_transform;
        get_transform_matrices(t, validity, transform, motion_transform);
        const rti::Mat4f rti_transform = RRTUtil::convertMatrix(transform);
        const rti::Mat4f rti_motion_transform = RRTUtil::convertMatrix(motion_transform);

        // Create an rti::Object for each rti::Geometry.
		for(size_t geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
		{
            const std::pair<rti::GeometryHandle, MtlID> geometry = geometry_translator->get_output_geometry(geometry_index);

			// Translate the rti::Shader for this geometry
            const BaseTranslator_to_RTIShader* shader_translator = acquire_shader_translator(geometry.second, t, validity, translation_progress, result);
            if(shader_translator != nullptr)
            {
                // Translate the rti::RenderAttributes
                const Translator_RTIRenderAttributes::Attributes attributes(
                    shader_translator->get_output_is_matte(), 
                    shader_translator->get_output_is_emissive(), 
                    is_caustic_generator(t, validity),
                    is_visible_to_primary_rays(t, validity));

                const Translator_RTIRenderAttributes* attributes_translator = 
                    AcquireChildTranslator<Translator_RTIRenderAttributes>(Translator_RTIRenderAttributes::Key(attributes), t, translation_progress, result);
                if(attributes_translator != nullptr)
                {							
				    // Setup the rti::Object
                    rti::ObjectHandle object_handle = initialize_output_handle<rti::ObjectHandle>(geometry_index);
				    if(object_handle.isValid())
                    {
					    rti::TEditPtr<rti::Object> object(object_handle);
                        
					    RRTUtil::check_rti_result(object->setGeometry(geometry.first));
					    RRTUtil::check_rti_result(object->setShader(shader_translator->get_output_shader()));
                        object->setRenderAttributes(attributes_translator->get_output_attributes());

                        // Set the transform matrices
                        object->setTransform(rti_transform);
                        object->setMotionTransform(rti_motion_transform);
				    }
                }
            }

            // Return immediately on abort
            if(result == TranslationResult::Aborted)
            {
                return result;
            }
            else if(result == TranslationResult::Failure)
            {
                // Continue - failure to translate an object's material shouldn't result in that object being exluded from rendering.
            }
		}

		return TranslationResult::Success;
	}
	else
	{
		// Couldn't translate geometry: that's a failure to translate this node
		return result;
	}
}

void BaseTranslator_to_RTIObject::append_output_objects(std::vector<rti::EntityHandle>& entities_list) const
{
    const size_t num_outputs = GetNumOutputs();
    entities_list.reserve(entities_list.size() + num_outputs);
    for(size_t i = 0; i < num_outputs; ++i)
    {
        const rti::ObjectHandle handle = get_output_handle<rti::ObjectHandle>(i);
        if(handle.isValid())
        {
            entities_list.push_back(handle);
        }
    }
}

size_t BaseTranslator_to_RTIObject::get_num_output_objects() const
{
    return GetNumOutputs();
}

}}	// namespace 
