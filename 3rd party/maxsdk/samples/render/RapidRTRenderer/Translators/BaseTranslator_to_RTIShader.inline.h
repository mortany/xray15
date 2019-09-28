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

#include "../Util.h"

namespace Max
{;
namespace RapidRTTranslator
{;


inline bool check_rti_result_noassert(const rti::RTIResult rti_result)
{
    return (rti_result == rti::RTI_SUCCESS);
}


inline void BaseTranslator_to_RTIShader::set_shader_float(rti::Shader& shader, const char* param_name, float val)
{
    if(!check_rti_result_noassert(shader.setUniform1f(param_name, val)))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_bool(rti::Shader& shader, const char* param_name, const bool val)
{
    if(!check_rti_result_noassert(shader.setUniform1b(param_name, val)))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_float2(rti::Shader& shader, const char* param_name, const Point2& val)
{
    if(!check_rti_result_noassert(shader.setUniform2f(param_name, rti::Vec2f(val.x, val.y))))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_float3(rti::Shader& shader, const char* param_name, const Color& val)
{
    if(!check_rti_result_noassert(shader.setUniform3f(param_name, RRTUtil::convertColor(val))))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_float3(rti::Shader& shader, const char* param_name, const AColor& val)
{
    if(!check_rti_result_noassert(shader.setUniform3f(param_name, RRTUtil::convertColor(Color(val)))))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_float4(rti::Shader& shader, const char* param_name, const AColor& val)
{
    if(!check_rti_result_noassert(shader.setUniform4f(param_name, RRTUtil::convertColor(val))))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_texture1d(rti::Shader& shader, const char* param_name, const rti::TextureHandle val)
{
    if(!check_rti_result_noassert(shader.setUniformTexture1D(param_name, val)))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_texture2d(rti::Shader& shader, const char* param_name, const rti::TextureHandle val)
{
    if(!check_rti_result_noassert(shader.setUniformTexture2D(param_name, val)))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_int(rti::Shader& shader, const char* param_name, const int val)
{
    if(!check_rti_result_noassert(shader.setUniform1i(param_name, val)))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_int2(rti::Shader& shader, const char* param_name, const IPoint2& val)
{
    if(!check_rti_result_noassert(shader.setUniform2i(param_name, reinterpret_cast<const rti::Vec2i&>(val))))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_floatv(rti::Shader& shader, const char* param_name, const std::vector<float>& val)
{
    if(!check_rti_result_noassert(shader.setUniform1fv(param_name, static_cast<int>(val.size()), val.data())))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_float3v(rti::Shader& shader, const char* param_name, const std::vector<rti::Vec3f>& val)
{
    if(!check_rti_result_noassert(shader.setUniform3fv(param_name, static_cast<int>(val.size()), val.data())))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_float4v(rti::Shader& shader, const char* param_name, const std::vector<rti::Vec4f>& val)
{
    if(!check_rti_result_noassert(shader.setUniform4fv(param_name, static_cast<int>(val.size()), val.data())))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_mat3f(rti::Shader& shader, const char* param_name, const rti::Mat3f& val)
{
    if(!check_rti_result_noassert(shader.setUniform3x3f(param_name, false, val)))
    {
        report_shader_param_error(param_name);
    }
}

inline void BaseTranslator_to_RTIShader::set_shader_mat4f(rti::Shader& shader, const char* param_name, const rti::Mat4f& val)
{
    if(!check_rti_result_noassert(shader.setUniform4x4f(param_name, false, val)))
    {
        report_shader_param_error(param_name);
    }
}


}}	// namespace 
