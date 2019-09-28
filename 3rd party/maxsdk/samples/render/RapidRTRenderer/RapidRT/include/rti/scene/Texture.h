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
* @file    Texture.h
* @brief   Public interface for textures.
*
* @author  Henrik Edstrom
* @date    2008-05-30
*
*/

#ifndef __RTI_TEXTURE_H__
#define __RTI_TEXTURE_H__

///////////////////////////////////////////////////////////////////////////////
// Includes ///////////////////////////////////////////////////////////////////

// rti
#include "rti/core/Class.h"
#include "rti/core/Common.h"
#include "rti/math/Math.h"


///////////////////////////////////////////////////////////////////////////////
// rti::Texture ///////////////////////////////////////////////////////////////

BEGIN_RTI

//---------- Types --------------------------------------------

enum ETextureType { 
  TEXTURE_1D, 
  TEXTURE_2D, 
  TEXTURE_3D, 
  TEXTURE_CUBE 
};


enum ETexturePixelDataType { 
  TEXTURE_UNSIGNED_BYTE, 
  TEXTURE_FLOAT32, 
  TEXTURE_SRGB 
};


enum ETextureFilter { 
  TEXTURE_NEAREST, 
  TEXTURE_LINEAR 
};


enum ETextureRepeat {
  TEXTURE_REPEAT,
  TEXTURE_CLAMP,
  TEXTURE_CLAMP_TO_BORDER,
  TEXTURE_MIRROR_REPEAT,
  TEXTURE_MIRROR_CLAMP,
  TEXTURE_MIRROR_CLAMP_TO_BORDER
};


/**
* @class   TextureDescriptor
* @brief   Describes the parameters of a texture.
*
* @author  Henrik Edstrom
* @date    2008-06-02
*/
struct TextureDescriptor {

  //---------- Constructor ------------------------------------

  TextureDescriptor();


  //---------- Attributes -------------------------------------

  ETextureType          m_type;
  ETexturePixelDataType m_pixelDataType;
  int                   m_numComponents;
  ETextureFilter        m_filter;
  ETextureRepeat        m_repeatU;
  ETextureRepeat        m_repeatV;
  ETextureRepeat        m_repeatW;
  int                   m_width;
  int                   m_height;
  int                   m_depth;
  const unsigned char*  m_pixelData;
  const unsigned char*  m_cubePixelData[6];
  Vec4f                 m_borderColor;
  bool                  m_invertRows;
};


/**
* @class   Texture
* @brief   Public interface for textures.
*
* @author  Henrik Edstrom
* @date    2008-05-30
*/
class RTAPI Texture : public Class {
public:
  DECL_INTERFACE_CLASS(CTYPE_TEXTURE)

  //---------- Methods ----------------------------------------

  RTIResult create(const TextureDescriptor& desc);

};  // Texture


//---------- Handle type --------------------------------------

typedef THandle<Texture, ClassHandle> TextureHandle;


//---------- TextureDescriptor inline section -----------------

inline TextureDescriptor::TextureDescriptor()
    : m_type(TEXTURE_2D),
      m_pixelDataType(TEXTURE_UNSIGNED_BYTE),
      m_numComponents(3),
      m_filter(TEXTURE_LINEAR),
      m_repeatU(TEXTURE_REPEAT),
      m_repeatV(TEXTURE_REPEAT),
      m_repeatW(TEXTURE_REPEAT),
      m_width(0),
      m_height(0),
      m_depth(0),
      m_pixelData(nullptr),
      m_borderColor(Vec4f(0.0f)),
      m_invertRows(false) {
  
  for (int i = 0; i < 6; ++i) {
    m_cubePixelData[i] = nullptr;
  }
}


END_RTI


///////////////////////////////////////////////////////////////////////////////

#endif  // __RTI_TEXTURE_H__
