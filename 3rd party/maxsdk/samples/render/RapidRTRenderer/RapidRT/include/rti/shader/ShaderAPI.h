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
* @file    ShaderAPI.h
* @brief   Header for the shader API.
*
* @author  Henrik Edstrom
* @date    2008-01-27
*
*/

#ifndef __RTI_SHADER_API_H__
#define __RTI_SHADER_API_H__

#ifndef DOXYGEN_SHOULD_SKIP_THIS

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// C++ / STL
#include <cstddef>

// Math
#include "rti/math/Math.h"

///////////////////////////////////////////////////////////////////////////////
// Warnings ///////////////////////////////////////////////////////////////////

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4100)  // unreferenced formal parameter
#endif

///////////////////////////////////////////////////////////////////////////////
// Macros /////////////////////////////////////////////////////////////////////

#ifndef DLLEXPORT
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __attribute__((visibility("default")))
#endif
#endif

// shader helper macros

#define IMPL_SHADER(className)                                                          \
  extern "C" DLLEXPORT IShaderBase* className##_create(IShaderManager* shaderManager) { \
    return new className;                                                               \
  }                                                                                     \
  extern "C" DLLEXPORT int className##_getAPIVersion() { return SHADER_API_VERSION; }


///////////////////////////////////////////////////////////////////////////////
// rti Shader API /////////////////////////////////////////////////////////////

BEGIN_RTI

//---------- API Version --------------------------------------

const int SHADER_API_VERSION = 2724;


//---------- Shader Types -------------------------------------

enum EShaderType {
  SHADER_MATERIAL    = 0,
  SHADER_ENVIRONMENT = 1,
  SHADER_CAMERA      = 2,

  NUM_SHADER_TYPES
};


//---------- Shader Functions ---------------------------------

enum EShaderFunc {
  SHADER_FUNC_EMISSION     = 0x1 << 0,
  SHADER_FUNC_SCATTERING   = 0x1 << 1,
  SHADER_FUNC_MEDIUM       = 0x1 << 2,
  SHADER_FUNC_CULL         = 0x1 << 3,
  SHADER_FUNC_MATTE        = 0x1 << 4,
  SHADER_FUNC_ENVIRONMENT  = 0x1 << 5,
  SHADER_FUNC_GENERATE_RAY = 0x1 << 6,
};


//---------- Shader Parameters  -------------------------------

enum EShaderParam {
  SHADER_PARAM_VARYING_1F,
  SHADER_PARAM_VARYING_2F,
  SHADER_PARAM_VARYING_3F,
  SHADER_PARAM_VARYING_4F,

  SHADER_PARAM_VARYING_2X2F,
  SHADER_PARAM_VARYING_3X3F,
  SHADER_PARAM_VARYING_4X4F
};


//---------- State Types --------------------------------------

struct BSDFRef {
  BSDFRef() : m_p(nullptr) {}
  BSDFRef(void* p) : m_p(p) {}

  void* m_p;
};


struct ShadeState {
  // common
  Vec3f     rt_RayOrigin;
  Vec3f     rt_RayDirection;
  Vec3f     rt_SurfacePosition;
  Vec3f     rt_ShadingNormal;
  Vec3f     rt_GeometryNormal;
  Vec3f     rt_SurfacePositionLocal;
  Vec3f     rt_ShadingNormalLocal;
  Vec3f     rt_GeometryNormalLocal;
  float     rt_OutsideIndexOfRefraction;
  
  bool      rt_FrontFacing;
  int       rt_PrimitiveIndex;

  Vec2f     rt_RasterPosition;
  Vec2i     rt_PixelPosition;
  Vec2i     rt_ImageResolution;
  float     rt_Time;

  // emission
  Vec3f     rt_Emission;
  float     rt_SurfaceArea;

  // scattering
  BSDFRef   rt_Scattering;

  // medium
  Vec3f     rt_AbsorptionCoefficient;
  float     rt_IndexOfRefraction;

  // cull
  bool      rt_Cull;

  // matte
  Vec3f     rt_MatteColor;
  Vec3f     rt_MatteMask;

  // environment
  Vec3f     rt_Environment;

  // camera
  float     rt_LensRadius;
  Vec2f     rt_LensSample;
  float     rt_TimeSample;
  float     rt_ShutterOpen;
  float     rt_ShutterClose;
  Vec3f     rt_RayOriginDX;
  Vec3f     rt_RayOriginDY;
  Vec3f     rt_RayDirectionDX;
  Vec3f     rt_RayDirectionDY;
  Vec3f     rt_ColorFilter;

  Mat4f     rt_WorldToLocalMatrix;
  Mat4f     rt_LocalToWorldMatrix;
  Mat3f     rt_WorldToLocalNormalMatrix;
  Mat3f     rt_LocalToWorldNormalMatrix;

  // Padding - don't use this memory
  float     pad[2];

  // internal parameters
  bool      m_discard;
  void*     m_shaderAPI;
  void*     m_uniformData;
  void*     m_varyingData;
  int       m_vertexIndices[3];
};


//---------- Array Type ---------------------------------------

const int RTI_SL_ARRAY_MAX_INITIAL_SIZE = 256;

template <class T>
struct SLArray {

  //---------- Types ------------------------------------------

  enum {
    BUFFER_SIZE = RTI_SL_ARRAY_MAX_INITIAL_SIZE
  };

  typedef T ElemType;


  //---------- Constructors -----------------------------------

  SLArray() {
    init();
  }


  SLArray(int size) {
    init();
    reserveBuffer(size);
    m_size = size;
  }


  SLArray(int size, const T* elements) {
    init();
    reserveBuffer(size);
    for (int i = 0; i < size; ++i) {
      m_data[i] = elements[i];
    }
    m_size = size;
  }


  SLArray(const SLArray &rhs) {
    init();
    reserveBuffer(rhs.m_size);
    for (int i = 0; i < rhs.m_size; ++i) {
      m_data[i] = rhs[i];
    }
    m_size = rhs.m_size;
  }


  //---------- Destructor -------------------------------------

  ~SLArray() {
    if (m_allocatedCapacity > 0) {
      delete[] m_data;
    }
  }


  //---------- Methods ----------------------------------------

  int length() const { return m_size; }


  //---------- Operators --------------------------------------

  SLArray& operator=(const SLArray<T>& rhs) {
    reserveBuffer(rhs.m_size);
    for (int i = 0; i < rhs.m_size; ++i) {
      m_data[i] = rhs[i];
    }
    m_size = rhs.m_size;
    return *this;
  }


  const T& operator[](int i) const { return m_data[i]; }


  T& operator[](int i) { return m_data[i]; }


  //---------- Attributes (public) ----------------------------

  int   m_size;
  T*    m_data;

  int   m_allocatedCapacity;
  T     m_buffer[BUFFER_SIZE];


private:

  //---------- Methods ----------------------------------------

  void init() {
    m_size              = 0;
    m_allocatedCapacity = -BUFFER_SIZE;
    m_data              = m_buffer;
  }


  void reserveBuffer(int size) {

    // Positive capacity means 'external buffer', negative means 'internal buffer'.
    if (size <= m_allocatedCapacity || size <= -m_allocatedCapacity) {
      return;
    }

    bool oldExternalBuffer = m_allocatedCapacity > 0;

    // Allocate new buffer.
    T* newBuffer = new T[size];

    // Copy old contents.
    for (int i = 0; i < m_size; ++i) {
      newBuffer[i] = m_data[i];
    }

    // Free old buffer, if present.
    if (oldExternalBuffer) {
      delete[] m_data;
    }

    m_allocatedCapacity = size;
    m_data              = newBuffer;
  }
};


//---------- Forward declarations -----------------------------

class IShaderBase;
class MaterialShader;
class TextureSampler1D;
class TextureSampler2D;
class TextureSampler3D;
class TextureSamplerCube;


//---------- Registration Interface ---------------------------

class IShaderManager {
public:
  virtual ~IShaderManager() {}

  virtual void registerShaderFunctions(IShaderBase* shader, int funcMask) = 0;

  virtual void registerUniform1b(IShaderBase* shader, const char* name, bool& variable) = 0;
  virtual void registerUniform2b(IShaderBase* shader, const char* name, Vec2b& variable) = 0;
  virtual void registerUniform3b(IShaderBase* shader, const char* name, Vec3b& variable) = 0;
  virtual void registerUniform4b(IShaderBase* shader, const char* name, Vec4b& variable) = 0;

  virtual void registerUniform1i(IShaderBase* shader, const char* name, int& variable)   = 0;
  virtual void registerUniform2i(IShaderBase* shader, const char* name, Vec2i& variable) = 0;
  virtual void registerUniform3i(IShaderBase* shader, const char* name, Vec3i& variable) = 0;
  virtual void registerUniform4i(IShaderBase* shader, const char* name, Vec4i& variable) = 0;

  virtual void registerUniform1f(IShaderBase* shader, const char* name, float& variable) = 0;
  virtual void registerUniform2f(IShaderBase* shader, const char* name, Vec2f& variable) = 0;
  virtual void registerUniform3f(IShaderBase* shader, const char* name, Vec3f& variable) = 0;
  virtual void registerUniform4f(IShaderBase* shader, const char* name, Vec4f& variable) = 0;

  virtual void registerUniform2x2f(IShaderBase* shader, const char* name, Mat2f& variable) = 0;
  virtual void registerUniform3x3f(IShaderBase* shader, const char* name, Mat3f& variable) = 0;
  virtual void registerUniform4x4f(IShaderBase* shader, const char* name, Mat4f& variable) = 0;

  virtual void registerUniformTexture1D(IShaderBase* shader, const char* name,
                                        TextureSampler1D& sampler) = 0;
  virtual void registerUniformTexture2D(IShaderBase* shader, const char* name,
                                        TextureSampler2D& sampler) = 0;
  virtual void registerUniformTexture3D(IShaderBase* shader, const char* name,
                                        TextureSampler3D& sampler) = 0;
  virtual void registerUniformTextureCube(IShaderBase* shader, const char* name,
                                          TextureSamplerCube& sampler) = 0;

  virtual void registerUniform1bv(IShaderBase* shader, const char* name,
                                  SLArray<bool>& variable) = 0;
  virtual void registerUniform2bv(IShaderBase* shader, const char* name,
                                  SLArray<Vec2b>& variable) = 0;
  virtual void registerUniform3bv(IShaderBase* shader, const char* name,
                                  SLArray<Vec3b>& variable) = 0;
  virtual void registerUniform4bv(IShaderBase* shader, const char* name,
                                  SLArray<Vec4b>& variable) = 0;

  virtual void registerUniform1iv(IShaderBase* shader, const char* name,
                                  SLArray<int>& variable) = 0;
  virtual void registerUniform2iv(IShaderBase* shader, const char* name,
                                  SLArray<Vec2i>& variable) = 0;
  virtual void registerUniform3iv(IShaderBase* shader, const char* name,
                                  SLArray<Vec3i>& variable) = 0;
  virtual void registerUniform4iv(IShaderBase* shader, const char* name,
                                  SLArray<Vec4i>& variable) = 0;

  virtual void registerUniform1fv(IShaderBase* shader, const char* name,
                                  SLArray<float>& variable) = 0;
  virtual void registerUniform2fv(IShaderBase* shader, const char* name,
                                  SLArray<Vec2f>& variable) = 0;
  virtual void registerUniform3fv(IShaderBase* shader, const char* name,
                                  SLArray<Vec3f>& variable) = 0;
  virtual void registerUniform4fv(IShaderBase* shader, const char* name,
                                  SLArray<Vec4f>& variable) = 0;

  virtual void registerUniform2x2fv(IShaderBase* shader, const char* name,
                                    SLArray<Mat2f>& variable) = 0;
  virtual void registerUniform3x3fv(IShaderBase* shader, const char* name,
                                    SLArray<Mat3f>& variable) = 0;
  virtual void registerUniform4x4fv(IShaderBase* shader, const char* name,
                                    SLArray<Mat4f>& variable) = 0;

  virtual void registerUniformTexture1Dv(IShaderBase* shader, const char* name,
                                         SLArray<TextureSampler1D>& variable) = 0;
  virtual void registerUniformTexture2Dv(IShaderBase* shader, const char* name,
                                         SLArray<TextureSampler2D>& variable) = 0;
  virtual void registerUniformTexture3Dv(IShaderBase* shader, const char* name,
                                         SLArray<TextureSampler3D>& variable) = 0;
  virtual void registerUniformTextureCubev(IShaderBase* shader, const char* name,
                                           SLArray<TextureSamplerCube>& variable) = 0;

  virtual void registerParameter(IShaderBase* shader, EShaderParam eType, const char* name,
                                 int offset) = 0;

};  // IShaderManager


//---------- API Interface ------------------------------------

class IShaderAPI {
public:
  virtual ~IShaderAPI() {}

  virtual bool    raytype(int rayType) = 0;
  virtual int     raycount(int rayType) = 0;

  virtual float   sample1() = 0;
  virtual Vec2f   sample2() = 0;

  virtual BSDFRef lambert(Vec3f color, Vec3f normal) = 0;
  virtual BSDFRef lambert(Vec3f color, Vec3f  normal, bool clampAlbedo) = 0;
  virtual BSDFRef oren_nayar(Vec3f color, Vec3f normal, float sigma) = 0;
  virtual BSDFRef ward(Vec3f color, Vec3f normal, float roughness) = 0;
  virtual BSDFRef ward(Vec3f color, Vec3f normal, Vec3f tangent, float roughnessU, 
                       float roughnessV) = 0;
  virtual BSDFRef reflection(Vec3f color, Vec3f normal) = 0;
  virtual BSDFRef refraction(Vec3f color, Vec3f normal, float ior) = 0;
  virtual BSDFRef dielectric(Vec3f color, Vec3f normal, float ior) = 0;

  virtual BSDFRef microfacet(Vec3f color, Vec3f normal, float roughness, float ior) = 0;
  virtual BSDFRef microfacet(Vec3f color, Vec3f normal, float roughness, float ior, int ndfType) = 0;
  virtual BSDFRef microfacet(Vec3f color, Vec3f normal, Vec3f tangent, float roughnessU, 
                             float roughnessV, float ior, int ndfType) = 0;

  virtual BSDFRef microfacet_reflection(Vec3f color, Vec3f normal, float roughness) = 0;
  virtual BSDFRef microfacet_reflection(Vec3f color, Vec3f normal, Vec3f f0, float roughness, 
                                        int ndfType) = 0;
  virtual BSDFRef microfacet_reflection(Vec3f color, Vec3f normal, Vec3f tangent, Vec3f f0,
                                        float roughnessU, float roughnessV, int ndfType) = 0;
  virtual BSDFRef microfacet_reflection(Vec3f color, Vec3f normal, Vec3f f0, Vec3f bottomf0,
                                        float roughness, int ndfType) = 0;
  virtual BSDFRef microfacet_reflection(Vec3f color, Vec3f normal, Vec3f tangent, Vec3f f0,
                                        Vec3f bottomf0, float roughnessU, float roughnessV, 
                                        int ndfType) = 0;
  virtual BSDFRef microfacet_refraction(Vec3f color, Vec3f normal, float roughness, float ior) = 0;
  virtual BSDFRef microfacet_transmission(Vec3f color, Vec3f normal, float roughness,
                                          Vec3f reflectionF0, float reflectionRoughness,
                                          int ndfType)                                         = 0;
  virtual BSDFRef microfacet_transmission(Vec3f color, Vec3f normal, Vec3f tangent,
                                          float roughnessU, float roughnessV, Vec3f reflectionF0,
                                          float reflectionRoughnessU, float reflectionRoughnessV,
                                          int ndfType)                                         = 0;
  virtual BSDFRef transparent(Vec3f color) = 0;
  virtual BSDFRef translucence(Vec3f color, Vec3f normal) = 0;
  virtual BSDFRef diffuse_fresnel(Vec3f color, Vec3f normal, Vec3f f0, float roughness, 
                                  int ndfType) = 0;
  virtual BSDFRef diffuse_fresnel(Vec3f color, Vec3f normal, Vec3f f0, float roughness, 
                                  int ndfType, bool clampAlbedo) = 0;
  virtual BSDFRef wood_fiber(Vec3f color, Vec3f normal, float roughness, Vec3f fiberDirection) = 0;
  virtual BSDFRef bssrdf_gaussian(Vec3f color, Vec3f normal, Vec3f radius) = 0;
  virtual BSDFRef bssrdf_diffuse_fresnel(Vec3f color, Vec3f normal, Vec3f f0, float roughness, 
                                         int ndfType, Vec3f mfp) = 0;
  virtual BSDFRef mix(float alpha, BSDFRef bsdf0, BSDFRef bsdf1) = 0;
  virtual BSDFRef mix(Vec3f alpha, BSDFRef bsdf0, BSDFRef bsdf1) = 0;
  virtual BSDFRef add(BSDFRef bsdf0, BSDFRef bsdf1) = 0;

  virtual float noise1(float v) const = 0;
  virtual float noise1(Vec2f v) const = 0;
  virtual float noise1(Vec3f v) const = 0;
  virtual float noise1(Vec4f v) const = 0;

  virtual Vec2f noise2(float v) const = 0;
  virtual Vec2f noise2(Vec2f v) const = 0;
  virtual Vec2f noise2(Vec3f v) const = 0;
  virtual Vec2f noise2(Vec4f v) const = 0;

  virtual Vec3f noise3(float v) const = 0;
  virtual Vec3f noise3(Vec2f v) const = 0;
  virtual Vec3f noise3(Vec3f v) const = 0;
  virtual Vec3f noise3(Vec4f v) const = 0;

  virtual Vec4f noise4(float v) const = 0;
  virtual Vec4f noise4(Vec2f v) const = 0;
  virtual Vec4f noise4(Vec3f v) const = 0;
  virtual Vec4f noise4(Vec4f v) const = 0;

  virtual float pnoise1(float v, int p) const = 0;
  virtual float pnoise1(Vec2f v, Vec2i p) const = 0;
  virtual float pnoise1(Vec3f v, Vec3i p) const = 0;
  virtual float pnoise1(Vec4f v, Vec4i p) const = 0;

  virtual Vec2f pnoise2(float v, int p) const = 0;
  virtual Vec2f pnoise2(Vec2f v, Vec2i p) const = 0;
  virtual Vec2f pnoise2(Vec3f v, Vec3i p) const = 0;
  virtual Vec2f pnoise2(Vec4f v, Vec4i p) const = 0;

  virtual Vec3f pnoise3(float v, int p) const = 0;
  virtual Vec3f pnoise3(Vec2f v, Vec2i p) const = 0;
  virtual Vec3f pnoise3(Vec3f v, Vec3i p) const = 0;
  virtual Vec3f pnoise3(Vec4f v, Vec4i p) const = 0;

  virtual Vec4f pnoise4(float v, int p) const = 0;
  virtual Vec4f pnoise4(Vec2f v, Vec2i p) const = 0;
  virtual Vec4f pnoise4(Vec3f v, Vec3i p) const = 0;
  virtual Vec4f pnoise4(Vec4f v, Vec4i p) const = 0;

  virtual float cellnoise1(float v) const = 0;
  virtual float cellnoise1(Vec2f v) const = 0;
  virtual float cellnoise1(Vec3f v) const = 0;
  virtual float cellnoise1(Vec4f v) const = 0;

  virtual Vec2f cellnoise2(float v) const = 0;
  virtual Vec2f cellnoise2(Vec2f v) const = 0;
  virtual Vec2f cellnoise2(Vec3f v) const = 0;
  virtual Vec2f cellnoise2(Vec4f v) const = 0;

  virtual Vec3f cellnoise3(float v) const = 0;
  virtual Vec3f cellnoise3(Vec2f v) const = 0;
  virtual Vec3f cellnoise3(Vec3f v) const = 0;
  virtual Vec3f cellnoise3(Vec4f v) const = 0;

  virtual Vec4f cellnoise4(float v) const = 0;
  virtual Vec4f cellnoise4(Vec2f v) const = 0;
  virtual Vec4f cellnoise4(Vec3f v) const = 0;
  virtual Vec4f cellnoise4(Vec4f v) const = 0;

  virtual float woodnoise1(float v) const = 0;
  virtual float woodnoise1(Vec3f v) const = 0;

  virtual float burtlenoise1(int v) const = 0;
  virtual Vec2f burtlenoise2(Vec2i v) const = 0;

};  // IShaderAPI


//---------- IShaderBase --------------------------------------

class IShaderBase {
public:
  virtual ~IShaderBase() {}
  virtual void  registerShader(IShaderManager* shaderAPI) = 0;
  virtual void  copy(const IShaderBase* shader) = 0;
  virtual int   getVersion() const = 0;
  virtual int   getType() const = 0;

};  // IShaderBase


//---------- MaterialShader -----------------------------------

class MaterialShader : public IShaderBase {
public:
  virtual void emission(IShaderAPI* api, ShadeState& rt_state) const {}
  virtual void scattering(IShaderAPI* api, ShadeState& rt_state) const {};

  virtual void medium(IShaderAPI* api, ShadeState& rt_state) const {};
  virtual void cull(IShaderAPI* api, ShadeState& rt_state) const {}
  virtual void matte(IShaderAPI* api, ShadeState& rt_state) const {}

  virtual int getVersion() const { return 1; }
  virtual int getType() const { return SHADER_MATERIAL; }

};  // MaterialShader


//---------- EnvironmentShader --------------------------------

class EnvironmentShader : public IShaderBase {
public:
  virtual void shade(IShaderAPI* api, ShadeState& rt_state) const = 0;
  virtual int getVersion() const { return 1; }
  virtual int getType() const { return SHADER_ENVIRONMENT; }

};  // LightShader


//---------- CameraShader -------------------------------------

class CameraShader : public IShaderBase {
public:
  virtual void generateRay(IShaderAPI* api, ShadeState& rt_state) const = 0;
  virtual int getVersion() const { return 1; }
  virtual int getType() const { return SHADER_CAMERA; }

};  // LightShader


//---------- Texture Interfaces -------------------------------

class ITextureSampler1D {
public:
  virtual ~ITextureSampler1D() {}
  virtual Vec4f lookup(float coord) const = 0;
  virtual int width() const = 0;

};  // ITextureSampler1D


class ITextureSampler2D {
public:
  virtual ~ITextureSampler2D() {}
  virtual Vec4f lookup(Vec2f coord) const = 0;
  virtual int width() const  = 0;
  virtual int height() const = 0;

};  // ITextureSampler2D


class ITextureSampler3D {
public:
  virtual ~ITextureSampler3D() {}
  virtual Vec4f lookup(Vec3f coord) const = 0;
  virtual int width() const  = 0;
  virtual int height() const = 0;
  virtual int depth() const  = 0;

};  // ITextureSampler3D


class ITextureSamplerCube {
public:
  virtual ~ITextureSamplerCube() {}
  virtual Vec4f lookup(Vec3f dir) const = 0;
  virtual int width() const  = 0;
  virtual int height() const = 0;

};  // ITextureSamplerCube


class TextureSampler1D {
public:
  const ITextureSampler1D* m_sampler;
  int                      m_id;

  TextureSampler1D() : m_sampler(nullptr), m_id(-1) {}

  RTI_INLINE
  Vec4f lookup(float coord) const {
    if (m_sampler) {
      return m_sampler->lookup(coord);
    }
    return Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
  }
};


class TextureSampler2D {
public:
  const ITextureSampler2D* m_sampler;
  int                      m_id;

  TextureSampler2D() : m_sampler(nullptr), m_id(-1) {}

  RTI_INLINE
  Vec4f lookup(Vec2f coord) const {
    if (m_sampler) {
      return m_sampler->lookup(coord);
    }
    return Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
  }
};


class TextureSampler3D {
public:
  const ITextureSampler3D* m_sampler;
  int                      m_id;

  TextureSampler3D() : m_sampler(nullptr), m_id(-1) {}

  RTI_INLINE
  Vec4f lookup(Vec3f coord) const {
    if (m_sampler) {
      return m_sampler->lookup(coord);
    }
    return Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
  }
};


class TextureSamplerCube {
public:
  const ITextureSamplerCube* m_sampler;
  int                        m_id;

  TextureSamplerCube() : m_sampler(0), m_id(-1) {}

  RTI_INLINE
  Vec4f lookup(Vec3f dir) const {
    if (m_sampler) {
      return m_sampler->lookup(dir);
    }
    return Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
  }
};


//---------- API Function Prototypes --------------------------

typedef IShaderBase* (*CreateShaderFunc)(void);
typedef int (*GetAPIVersionFunc)(void);

END_RTI

#ifndef RTI_EXCLUDE_SHADER_STDLIB
#include "rti/shader/ShaderStdLib.h"
#endif

#ifdef _WIN32
#pragma warning(pop)
#endif

///////////////////////////////////////////////////////////////////////////////

#endif  // DOXYGEN_SHOULD_SKIP_THIS

#endif  // __RTI_SHADER_API_H__
