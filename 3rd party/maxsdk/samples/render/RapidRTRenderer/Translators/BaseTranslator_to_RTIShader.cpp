//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "BaseTranslator_to_RTIShader.h"
// Local includes
#include "../resource.h"
// Translator API
#include <RenderingAPI/Translator/ITranslationManager.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>

// max sdk
#include <IPathConfigMgr.h>
#include <dllutilities.h>

// std includes
#include <vector>

using namespace MaxSDK;

namespace Max
{;
namespace RapidRTTranslator
{;

//==================================================================================================
// class BaseTranslator_to_RTIShader::CompilationMessageCallback
//
// Message callback for RTI compilation
//==================================================================================================
class BaseTranslator_to_RTIShader::CompilationMessageCallback : public rti::IMessageCallback
{
public:

    CompilationMessageCallback(IRenderingLogger& logger, const char* shader_name)
        : m_logger(logger),
        m_shader_name(shader_name)
    {
    }

    void message(int nSeverity, const char* szText)
    {
        // Enum from RapidRT team
        enum {
            MSG_IMPORTANT  = 0,
            MSG_DEFAULT    = 1,
            MSG_MINOR      = 2,
            MSG_DEBUG_1    = 3,
            MSG_DEBUG_2    = 4,
            MSG_DEBUG_3    = 5
        };

        handle_message((nSeverity <= MSG_MINOR) ? IRenderingLogger::MessageType::Info : IRenderingLogger::MessageType::Debug, szText);
    }

    void warning(const char* szText)
    {
        handle_message(IRenderingLogger::MessageType::Warning, szText);
    }

    void error(const char* szText)
    {
        handle_message(IRenderingLogger::MessageType::Error, szText);
    }

    void fatal(const char* szText)
    {
        handle_message(IRenderingLogger::MessageType::Error, szText);
    }
private:

    void handle_message(const IRenderingLogger::MessageType type, const char* szText)
    {
        const MSTR shader_name = MSTR::FromACP(m_shader_name);
        const MSTR rti_message = MSTR::FromACP(szText);
        MSTR formatted_message;
        formatted_message.printf(MaxSDK::GetResourceStringAsMSTR(IDS_COMPILATION_MESSAGE_FORMAT), shader_name.data(), rti_message.data());
        m_logger.LogMessage(type, formatted_message);
    }

    void operator=(const CompilationMessageCallback&);

private:

    IRenderingLogger& m_logger;
    const char* m_shader_name;
};

//==================================================================================================
// class BaseTranslator_to_RTIShader
//==================================================================================================
BaseTranslator_to_RTIShader::BaseTranslator_to_RTIShader(TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_to_RTI(translator_graph_node),
    Translator(translator_graph_node)
{
}

const rti::ShaderHandle BaseTranslator_to_RTIShader::initialize_output_shader(const char *shader_name, const char* shader_string)
{
    bool success = false;
    rti::ShaderHandle shader_handle = initialize_output_handle<rti::ShaderHandle>(kOutputIndex_Shader);
    if(shader_handle.isValid())
    {
        rti::TEditPtr<rti::Shader> shader(shader_handle);

        CompilationMessageCallback message_callback(GetRenderSessionContext().GetLogger(), shader_name);
        success = RRTUtil::check_rti_result(shader->compile(shader_name, shader_string, &message_callback));
        if(success)
        {
            return shader_handle;
        }
        else
        {
            // Compilation failed: report error
            MSTR error_message;
            error_message.printf(MaxSDK::GetResourceStringAsMSTR(IDS_SHADER_COMPILE_FAILED), MSTR::FromACP(shader_name).data(), _T(""));
            GetRenderSessionContext().GetLogger().LogMessageOnce(IRenderingLogger::MessageType::Error, error_message);
        }
	}

    // Failed: delete the output and return a null handle
    ResetOutput(kOutputIndex_Shader);
    return rti::ShaderHandle();
}


const rti::ShaderHandle BaseTranslator_to_RTIShader::initialize_output_shader(const char* shader_name)
{
    rti::ShaderHandle shader_handle = initialize_output_handle<rti::ShaderHandle>(kOutputIndex_Shader);
    if(shader_handle.isValid())
    {
		const wchar_t* shadersStandardLocation = L"RapidRT\\Shaders\\Standard";
		MSTR shaderStandardDirPath(IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_MAX_SYS_ROOT_DIR));
		shaderStandardDirPath += shadersStandardLocation;

		MSTR shaderFilePath(shaderStandardDirPath + L"\\");
		shaderFilePath += MSTR::FromCStr(CStr(shader_name)) + L".rtsl";

		// Open the shader for reading 
		FILE* shader_file = fopen(shaderFilePath.ToACP().data(), "r");
		if(shader_file != nullptr)
		{
			// Determine file size
			fseek(shader_file, 0, SEEK_END);
			const size_t file_size = ftell(shader_file);
			fseek(shader_file, 0, SEEK_SET);

			// Allocate buffer for reading file
			std::vector<char> shader_file_contents;
			shader_file_contents.resize(file_size + 1); // 1 extra for null character

			// Read the file and close it
			const size_t bytes_read = fread(shader_file_contents.data(), sizeof(char), file_size, shader_file);
			DbgAssert(bytes_read <= file_size);
			fclose(shader_file);
			shader_file = nullptr;

			// Resize the buffer, as it could be smaller than the file contents (thanks to eol translation and such), and insert terminating null character
			shader_file_contents.resize(bytes_read + 1);
			shader_file_contents[bytes_read] = '\0';

			return initialize_output_shader(shader_name, shader_file_contents.data());
		} 
		else
		{
			// Missing file: report error
			MSTR error_message;
			error_message.printf(MaxSDK::GetResourceStringAsMSTR(IDS_MISSING_SHADER_FILE), shaderFilePath.data());
			GetRenderSessionContext().GetLogger().LogMessageOnce(IRenderingLogger::MessageType::Error, error_message);
		}
    }

    // Failed: delete the output and return a null handle
    ResetOutput(kOutputIndex_Shader);
    return rti::ShaderHandle();
}

void BaseTranslator_to_RTIShader::set_output_is_matte(const bool is_matte)
{
    if(get_output_is_matte() != is_matte)
    {
        SetOutput_SimpleValue<bool>(kOutputIndex_IsMatte, is_matte);
    }
}

void BaseTranslator_to_RTIShader::set_output_is_emitter(const bool is_emitter)
{
    if(get_output_is_emissive() != is_emitter)
    {
        SetOutput_SimpleValue<bool>(kOutputIndex_IsEmitter, is_emitter);
    }
}

rti::ShaderHandle BaseTranslator_to_RTIShader::get_output_shader() const
{
    return get_output_handle<rti::ShaderHandle>(kOutputIndex_Shader);
}

bool BaseTranslator_to_RTIShader::get_output_is_matte() const
{
    return GetOutput_SimpleValue<bool>(kOutputIndex_IsMatte, false);
}

bool BaseTranslator_to_RTIShader::get_output_is_emissive() const
{
    return GetOutput_SimpleValue<bool>(kOutputIndex_IsEmitter, false);
}

void BaseTranslator_to_RTIShader::report_shader_param_error(const char* param_name) const
{
    MSTR param_name_mstr = MSTR::FromACP(param_name);
    MSTR shader_name = get_shader_name();
    MSTR error_message;
    error_message.printf(MaxSDK::GetResourceStringAsMSTR(IDS_SHADER_PARAM_ERROR_FORMAT), static_cast<const MCHAR*>(param_name_mstr), shader_name.data());
	GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Error, error_message);
	DbgAssert(false);
}


}}	// namespace 
