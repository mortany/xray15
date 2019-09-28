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

#include "BaseTranslator_to_RTIShader.h"

namespace Max
{;

namespace RapidRTTranslator
{;

// Translates AMG-supporting MtlBase ==> rti::Shader
class BaseTranslator_AMG_to_RTIShader :
	public BaseTranslator_to_RTIShader
{
public:

	BaseTranslator_AMG_to_RTIShader(TranslatorGraphNode& tanslator_graph_node);

protected:

	static bool AMG_IsCompatible(MtlBase *mtlbase);

	TranslationResult AMG_Translate(MtlBase *mtlbase, const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, 
		                           int dirlights=0, std::vector<rti::Vec4f> *direction_spread = nullptr, std::vector<rti::Vec4f> *color_visi = nullptr);


    virtual void report_shader_param_error(const char* param_name) const;

private:

    size_t m_lastGraphHash;
};

}}	// namespace 

#include "BaseTranslator_to_RTIShader.inline.h"
