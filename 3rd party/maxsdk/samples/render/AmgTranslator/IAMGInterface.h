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

#ifdef AMG_TRANSLATOR_MODULE
#	define AmgTranslatorAPI __declspec(dllexport)
#else
#	define AmgTranslatorAPI __declspec(dllimport)
#endif

namespace MaxSDK
{
    namespace RenderingAPI
    {
        class IRenderingLogger;
    }
}

class IAMGInterface : public MaxHeapOperators
{
public:
	AmgTranslatorAPI static IAMGInterface *Create();
	AmgTranslatorAPI static bool Initialize(const TSTR& amgDataPath);
	AmgTranslatorAPI static bool IsSupported(const MCHAR *client, SClass_ID SID, Class_ID CID);

	virtual bool  BuildGraph(MtlBase* pNode, TimeValue t, int context, MaxSDK::RenderingAPI::IRenderingLogger *logger) = 0;
	virtual size_t               GetGraphHash() = 0;
	virtual bool                 GenerateRapidSLShader(const char *shaderName, const char *output, std::string &targetString) = 0;
	virtual size_t               GetParameterCount() = 0;
	virtual FPValue              GetParameterValue(int i, TimeValue t, CStr &parameter_name, MSTR &metaInfo) = 0;
	virtual bool                 GetEmissiveFlag() = 0;
	virtual bool                 GetMatteFlag() = 0;
};
