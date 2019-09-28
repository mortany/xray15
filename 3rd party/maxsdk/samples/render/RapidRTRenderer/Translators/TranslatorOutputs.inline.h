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

namespace Max
{;
namespace RapidRTTranslator
{;

//==================================================================================================
// class RTIHandleTranslatorOutput
//==================================================================================================

template<typename RTIHandleType>
RTIHandleTranslatorOutput<RTIHandleType>::RTIHandleTranslatorOutput()
	: m_handle(RTIHandleType::create())
{
}

template<typename RTIHandleType>
RTIHandleTranslatorOutput<RTIHandleType>::~RTIHandleTranslatorOutput()
{
    // Destroy the handle
    m_handle.destroy();
}

template<typename RTIHandleType>
const RTIHandleType& RTIHandleTranslatorOutput<RTIHandleType>::get_handle() const
{
    return m_handle;
}

}}	// namespace 

