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

#include <RenderingAPI/Translator/ITranslatorOutput.h>

#include <assert1.h>

#include <memory>

namespace Max
{;
namespace RapidRTTranslator
{;

//==================================================================================================
// class RTIHandleTranslatorOutput
//
// Implementation of ITranslatorOutput which contains a RapidRT Handle.
//==================================================================================================
template<typename RTIHandleType>
class RTIHandleTranslatorOutput :
    public ITranslatorOutput
{
public:

    // The constructor creates a brand new handle.
	RTIHandleTranslatorOutput();
    ~RTIHandleTranslatorOutput();

    const RTIHandleType& get_handle() const;

private:

	RTIHandleType m_handle;
};

}}	// namespace 

#include "TranslatorOutputs.inline.h"
