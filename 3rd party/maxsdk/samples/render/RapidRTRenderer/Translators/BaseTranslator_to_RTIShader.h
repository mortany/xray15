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

// max sdk
#include <strclass.h>
#include <imtl.h>

#include <rti/scene/shader.h>
#include <rti/scene/Texture.h>

#include <vector>

class Mtl;
class Texmap;
class Color;
class AColor;
class Point2;
class IPoint2;

namespace Max
{;

namespace RapidRTTranslator
{;

// Translates antyhing ==> rti::Shader
class BaseTranslator_to_RTIShader :
	public BaseTranslator_to_RTI    
{
public:

	BaseTranslator_to_RTIShader(TranslatorGraphNode& tanslator_graph_node);

    // Accesses the outputs generated by this translator
    rti::ShaderHandle get_output_shader() const;
    bool get_output_is_matte() const;
    bool get_output_is_emissive() const;

protected:

    // Creates the rti::Shader from the source passed string translated by this class
	// Initializes the output on the ITranslatorGraphnode.
    // Also instantiates and compiles the shader.
    const rti::ShaderHandle initialize_output_shader(const char *shader_name, const char *shader_string);
    // Creates the rti::Shader (if needed - if it doesn't already exist) translated by this class, 
	// loaded from the shader directory, 
	// Initializes the output on the ITranslatorGraphnode.
    // Also instantiates and compiles the shader.
    const rti::ShaderHandle initialize_output_shader(const char* shader_name); 

    // Sets the translated material as being matte and/or emissive
    void set_output_is_matte(const bool is_matte);
    void set_output_is_emitter(const bool is_emitter);

    // Sets shader parameters, including internal error checking and asserts.
    // All errors are handled internally, thus these methods return false; a failure to set a parameter value is not considered a fatal error, and
    // thus isn't meant to be propagated.
    void set_shader_int(rti::Shader& shader, const char* param_name, const int val);
    void set_shader_int2(rti::Shader& shader, const char* param_name, const IPoint2& val);
    void set_shader_bool(rti::Shader& shader, const char* param_name, const bool val);
    void set_shader_float(rti::Shader& shader, const char* param_name, const float val);
    void set_shader_float2(rti::Shader& shader, const char* param_name, const Point2& val);
    void set_shader_float3(rti::Shader& shader, const char* param_name, const Color& val);
    void set_shader_float3(rti::Shader& shader, const char* param_name, const AColor& val);
    void set_shader_float4(rti::Shader& shader, const char* param_name, const AColor& val);
    void set_shader_floatv(rti::Shader& shader, const char* param_name, const std::vector<float>& val);
    void set_shader_float3v(rti::Shader& shader, const char* param_name, const std::vector<rti::Vec3f>& val);
    void set_shader_float4v(rti::Shader& shader, const char* param_name, const std::vector<rti::Vec4f>& val);
    void set_shader_mat3f(rti::Shader& shader, const char* param_name, const rti::Mat3f& val);
    void set_shader_mat4f(rti::Shader& shader, const char* param_name, const rti::Mat4f& val);
    void set_shader_texture1d(rti::Shader& shader, const char* param_name, const rti::TextureHandle val);
    void set_shader_texture2d(rti::Shader& shader, const char* param_name, const rti::TextureHandle val);

    // Returns a name for the shader which is used in reporting error/logging information. This name should uniquely identify the shader/material,
    // in a way which is relevant for the user.
    virtual MSTR get_shader_name() const = 0;

	// We may want to override this
    virtual void report_shader_param_error(const char* param_name) const;

private:

    class CompilationMessageCallback;

    // Indices in which ITranslatorOutput's are stored
    enum TranslatorOutputIndices
    {
        kOutputIndex_Shader,
        kOutputIndex_IsMatte,
        kOutputIndex_IsEmitter,

        kOutputIndex_Count
    };
};

}}	// namespace 

#include "BaseTranslator_to_RTIShader.inline.h"
