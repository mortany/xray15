//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license
//  agreement provided at the time of installation or download, or which
//  otherwise accompanies this software in either electronic or hard copy form.
//
//////////////////////////////////////////////////////////////////////////////

/**
* @file    Shader.h
* @brief   Public interface for shaders.
*
* @author  Henrik Edstrom
* @date    2008-01-27
*
*/

#ifndef __RTI_SHADER_H__
#define __RTI_SHADER_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Class.h"
#include "rti/core/Common.h"
#include "rti/core/IMessageCallback.h"
#include "rti/math/Math.h"
#include "rti/scene/Texture.h"


///////////////////////////////////////////////////////////////////////////////
// rti::Shader ////////////////////////////////////////////////////////////////

BEGIN_RTI

/**
* @class   Shader
* @brief   Public interface for shaders.
*
* @author  Henrik Edstrom
* @date    2008-01-27
*/
class RTAPI Shader : public Class {
public:
  DECL_INTERFACE_CLASS(CTYPE_SHADER)

  //---------- Methods ----------------------------------------

  RTIResult compile(const char* shaderName, const char* shaderSource,
                    IMessageCallback* errorHandler);

  RTIResult instantiate(const char* shaderName, const wchar_t* libraryName,
                        bool explicitName = false);
  int getVersion() const;

  RTIResult setLayerList(int numShaders, const int* shaders);

  RTIResult setUniform1b(const char* name, bool value);
  RTIResult setUniform2b(const char* name, Vec2b value);
  RTIResult setUniform3b(const char* name, Vec3b value);
  RTIResult setUniform4b(const char* name, Vec4b value);

  RTIResult setUniform1i(const char* name, int value);
  RTIResult setUniform2i(const char* name, Vec2i value);
  RTIResult setUniform3i(const char* name, Vec3i value);
  RTIResult setUniform4i(const char* name, Vec4i value);

  RTIResult setUniform1f(const char* name, float value);
  RTIResult setUniform2f(const char* name, Vec2f value);
  RTIResult setUniform3f(const char* name, Vec3f value);
  RTIResult setUniform4f(const char* name, Vec4f value);

  RTIResult setUniform2x2f(const char* name, bool transpose, const Mat2f& value);
  RTIResult setUniform3x3f(const char* name, bool transpose, const Mat3f& value);
  RTIResult setUniform4x4f(const char* name, bool transpose, const Mat4f& value);

  RTIResult setUniformTexture1D(const char* name, TextureHandle texture);
  RTIResult setUniformTexture2D(const char* name, TextureHandle texture);
  RTIResult setUniformTexture3D(const char* name, TextureHandle texture);
  RTIResult setUniformTextureCube(const char* name, TextureHandle texture);

  RTIResult setUniform1bv(const char* name, int size, const bool* values);
  RTIResult setUniform2bv(const char* name, int size, const Vec2b* values);
  RTIResult setUniform3bv(const char* name, int size, const Vec3b* values);
  RTIResult setUniform4bv(const char* name, int size, const Vec4b* values);

  RTIResult setUniform1iv(const char* name, int size, const int* values);
  RTIResult setUniform2iv(const char* name, int size, const Vec2i* values);
  RTIResult setUniform3iv(const char* name, int size, const Vec3i* values);
  RTIResult setUniform4iv(const char* name, int size, const Vec4i* values);

  RTIResult setUniform1fv(const char* name, int size, const float* values);
  RTIResult setUniform2fv(const char* name, int size, const Vec2f* values);
  RTIResult setUniform3fv(const char* name, int size, const Vec3f* values);
  RTIResult setUniform4fv(const char* name, int size, const Vec4f* values);

  RTIResult setUniform2x2fv(const char* name, bool transpose, int size, const Mat2f* values);
  RTIResult setUniform3x3fv(const char* name, bool transpose, int size, const Mat3f* values);
  RTIResult setUniform4x4fv(const char* name, bool transpose, int size, const Mat4f* values);

  RTIResult setUniformTexture1Dv(const char* name, int size, const TextureHandle* values);
  RTIResult setUniformTexture2Dv(const char* name, int size, const TextureHandle* values);
  RTIResult setUniformTexture3Dv(const char* name, int size, const TextureHandle* values);
  RTIResult setUniformTextureCubev(const char* name, int size, const TextureHandle* values);

};  // Shader


//---------- Handle type --------------------------------------

typedef THandle<Shader, ClassHandle> ShaderHandle;


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_SHADER_H__
