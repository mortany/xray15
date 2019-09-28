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

#include "BaseTranslator_to_RTI.h"

// rti
#include <rti/scene/entity.h>
// std
#include <vector>

class Matrix3;

namespace Max
{;
namespace RapidRTTranslator
{;

class BaseTranslator_to_RTIGeometry;
class BaseTranslator_to_RTIShader;

// Translates anything ==> one/more rti::Object
// Initially, we only translate INodes into rti::Object (hence there is only one subclass of this), but eventually we may want to translator
// other types of objects, hence the rti::Object creation functionality is encapsualted into its own class.
class BaseTranslator_to_RTIObject :
    public BaseTranslator_to_RTI
{
public:

    BaseTranslator_to_RTIObject(TranslatorGraphNode& translator_graph_node);
    ~BaseTranslator_to_RTIObject();

    // Appends this translator's outputs to the given list
    void append_output_objects(std::vector<rti::EntityHandle>& entities_list) const;
    size_t get_num_output_objects() const;

    // -- inherited from Translator
    virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    
protected:

    // Acquires the translator which handles the translation of the rti::Geometry
    virtual const BaseTranslator_to_RTIGeometry* acquire_geometry_translator(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, TranslationResult& result) = 0;

    // Acquires the shader which handles the translation of the rti::Shader associated with the given material ID.
    virtual const BaseTranslator_to_RTIShader* acquire_shader_translator(const MtlID material_id, const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, TranslationResult& result) = 0;

    // Returns the transforms to be used with this rti::Object
    virtual void get_transform_matrices(const TimeValue t, Interval& validity, Matrix3& transform, Matrix3& motion_transform) const = 0;

    // Returns whether the object has the "caustic generator" attribute
    virtual bool is_caustic_generator(const TimeValue t, Interval& validity) = 0;

    // Returns whether the object is visible to primary rays
    virtual bool is_visible_to_primary_rays(const TimeValue t, Interval& validity) = 0;
};

}}	// namespace 
