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

// local
#include "BaseTranslator_to_RTITexture.h"
// Max sdk
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_ReferenceTarget.h>
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
// std
#include <vector>

class BitmapInfo;

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: TYPE_BITMAP parameter of any ReferenceTarget ==> rti::Texture
class Translator_BitmapFile_to_RTITexture :
    public BaseTranslator_to_RTITexture    // Translate to rti::Shader
{
public:

    // The key stores basic information on a bitmap file: file name, frame number, and gamma. This ensures perfect de-duplication when a texture file
    // is referenced multiple times in a scene.
    struct KeyStruct
    {
        // Initializes to empty key, that doesn't translate to any bitmap.
        KeyStruct();
        // Initializes from a BitmapInfo
        KeyStruct(const BitmapInfo& bi);
        bool operator==(const KeyStruct& rhs) const;

        // Creates a key based on a TYPE_BITMAP parameter. Requires a TimeValue and Interval when evaluating the parameter block value.
        static KeyStruct CreateFromBitmapParameter(ReferenceTarget& ref_targ, const MSTR param_name, const TimeValue t, Interval& validity);

        MSTR file_name;
        int frame_number;
        bool custom_file_gamma_flag;     // == BMM_CUSTOM_FILEGAMMA custom flag
        bool custom_gamma_flag;          // == BMM_CUSTOM_GAMMA custom flag
        // Custom gamma. Set iff BMM_CUSTOM_GAMMA, otherwise 0.
        float custom_gamma;
        std::vector<unsigned char> pi_data;
    };
    struct KeyStructHash
    {
        size_t operator()(const KeyStruct& data) const;
    };
    typedef GenericTranslatorKey_Struct<Translator_BitmapFile_to_RTITexture, KeyStruct, KeyStructHash> Key;

    // Constructs this translator with the key passed to the translator factory
	Translator_BitmapFile_to_RTITexture(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_BitmapFile_to_RTITexture();

    // Returns the texture handle for the baked texture
    rti::TextureHandle get_texture_handle() const;
    IPoint2 get_texture_resolution() const;

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    virtual void PreTranslate(const TimeValue translationTime, Interval& validity) override;
    virtual void PostTranslate(const TimeValue translationTime, Interval& validity) override;
    virtual Interval CheckValidity(const TimeValue t, const Interval& previous_validity) const override;
    virtual MSTR GetTimingCategory() const override;

private:

    const KeyStruct m_key;
};

}}	// namespace 
