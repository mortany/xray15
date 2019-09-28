//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "BaseTranslator_AMG_to_RTIShader.h"

// local
#include "Translator_BakedTexmap_to_RTITexture.h"
#include "Translator_BitmapFile_to_RTITexture.h"
#include "Translator_TextureOutput_to_RTITexture.h"
// AMG

// IAmgInterface.h is located in sdk samples folder, such that it may be shared with sdk samples plugins, without actually being exposed in the SDK.
#include <../samples/render/AmgTranslator/IAmgInterface.h>

// Max SDK
#include <RenderingAPI/Translator/ITranslationProgress.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>
#include <iparamb2.h>

#ifdef _DEBUG
#define DUMP_AMG_FILE 1
// Set to 1 to be able to edit the file externally
#define READ_BACK_AMG_FILE 0
#endif

#ifdef DUMP_AMG_FILE
#include <maxtextfile.h>
#endif

namespace Max
{;
namespace RapidRTTranslator
{;

//==================================================================================================
// class BaseTranslator_AMG_to_RTIShader
//==================================================================================================
BaseTranslator_AMG_to_RTIShader::BaseTranslator_AMG_to_RTIShader(TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_to_RTIShader(translator_graph_node),
    Translator(translator_graph_node),
	m_lastGraphHash(0)
{
}

TranslationResult BaseTranslator_AMG_to_RTIShader::AMG_Translate(MtlBase *mtlbase, const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, 
													   int dirlights, std::vector<rti::Vec4f> *direction_spread, std::vector<rti::Vec4f> *color_visi)
{
	if(mtlbase != NULL)
	{
		// If we are passing directional lights, this means we are translating an 
		// environment so the "context" sent to the translation system should be 2
		// otherwise it should be 0 (= material translation)
		int call_context = (direction_spread!=nullptr)?2:0;

		// Real initialization only happens once, so this is safe to do here
		const TCHAR* maxRootDir = GetCOREInterface()->GetDir(APP_MAX_SYS_ROOT_DIR);
		TSTR amgDataPath = TSTR(maxRootDir) + TSTR(_T("AMG\\Data\\"));
		IAMGInterface::Initialize(amgDataPath);


		// Create a translation context
		std::unique_ptr<IAMGInterface> pTranslator = std::unique_ptr<IAMGInterface>(IAMGInterface::Create());
		if (pTranslator == nullptr)
		{
			return TranslationResult::Failure;
		}

		
        GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Debug, _T("Construct AMG Graph"));

		// Construct the graph using the scriptlets from the AmgTranslator
		
		bool buildResult = pTranslator->BuildGraph(mtlbase, t, call_context, &GetRenderSessionContext().GetLogger());
		if (!buildResult)
		{
			// Something went wrong, we were unable to create the graph. Error message should have already been output by the
			// above function, so we just exit....
			return TranslationResult::Failure;
		}


		// GENERATE THE RapidSL shader
		// This is done only when the graph changes... We generate a hash, and compare.... 

		rti::ShaderHandle shader_handle;

		// Begin assuming the graph has changed
		bool graph_has_changed = true;  

		// Check if it has not:
		size_t graphHash = pTranslator->GetGraphHash();
		if (m_lastGraphHash == graphHash) 
		{
			// No change - re-use last shader handle
			shader_handle = get_output_shader(); // Re-use the old one

			// Safety - check if it is valid
			if(shader_handle.isValid())
				graph_has_changed = false; // ...if so, we know we can re-use it
		}
		
		if (graph_has_changed)
		{
			// Remember the graph hash for next time
			m_lastGraphHash = graphHash;

            GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Debug, _T("Generate RapidSL shader"));

			std::string targetString;
			if (!pTranslator->GenerateRapidSLShader("AMGShader", (call_context==2?"environment":"material"), targetString))
			{
				return TranslationResult::Failure;
			}


#ifdef DUMP_AMG_FILE
			// Uncomment this section to dump the shader to exe/temp/AMG_Comipiled_RapidRT.rt

			MaxSDK::Util::MaxString effectShaderA = MaxSDK::Util::MaxString::FromUTF8(targetString.data());
			TSTR effectShader = effectShaderA;
			TSTR fileName(_T("AMG_Comipiled_"));
			fileName += (call_context==2?_T("environment"):_T("material")); // Easier to debug
			TSTR pMaxSystemRootDir = GetCOREInterface()->GetDir(APP_MAX_SYS_ROOT_DIR);
			TSTR pathName = pMaxSystemRootDir + _T("\\temp\\");
			TSTR pureFileName;
			SplitFilename(fileName, NULL, &pureFileName, NULL);
			pathName += pureFileName + _T(".rt");

			// Scope it so it gets closed....
			{
				MaxSDK::Util::TextFile::Writer fileWriter;
				if (fileWriter.Open(pathName)) 
				{
					fileWriter.Write(effectShader.data());
				}
			}

#ifdef READ_BACK_AMG_FILE
			// Put a breakpoint here to be able to edit the shader while it's on disk
			// For debug/quick editing purpouses

			int readBack = READ_BACK_AMG_FILE;
			if (readBack == 1)
			{
				MaxSDK::Util::TextFile::Reader fileReader;
				if (fileReader.Open(pathName)) 
				{
					size_t x = fileReader.NumberOfChars();
					MaxSDK::Util::MaxString str = fileReader.ReadFull();
					size_t y = str.Length(0);
					targetString = str.ToUTF8();
					size_t z = targetString.length();
					size_t q = x+y+z;
					q=q;
				}
			}
#endif
#endif

            GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Debug, _T("Compile RapidSL shader"));

			shader_handle = initialize_output_shader("AMGShader", targetString.data());
		}

		if(!shader_handle.isValid())
		{
			return TranslationResult::Failure;
		}


		// Now do the editing
		{
			for (int i=0; i <pTranslator->GetParameterCount(); i++)
			{
				CStr parameter_name;
				MSTR metaInfo;
				FPValue value    = pTranslator->GetParameterValue(i, t, parameter_name, metaInfo);

				switch ((int) root_value_type(value.type)) 
				{
				case TYPE_BITMAP:
					{
					}
                    break;
                case TYPE_REFTARG:
				case TYPE_TEXMAP:
					{
                        ReferenceTarget* const ref_targ = 
                            (value.type == TYPE_REFTARG) ? value.r
                            : (value.type == TYPE_TEXMAP) ? value.tex
                            : nullptr;
						if((ref_targ != nullptr) && IsTex(ref_targ))
						{
							Texmap *texmap = (Texmap *)ref_targ;

							if (!metaInfo.isNull()) // There's a real bitmap - use it
							{
								StdTexoutGen *ptr = Translator_TextureOutput_to_RTITexture::CreateFromOutputParameter(*texmap, metaInfo, t, validity);

								if (ptr)
								{
									// Bake the texture
									TranslationResult result;
									const Translator_TextureOutput_to_RTITexture* texture_translator = AcquireChildTranslator<Translator_TextureOutput_to_RTITexture>(ptr, t, translation_progress, result);
									if(result != TranslationResult::Aborted)
									{
										rti::TEditPtr<rti::Shader> shader(shader_handle);
										set_shader_texture2d(*shader, parameter_name, (texture_translator != nullptr) ? texture_translator->get_texture_handle() : rti::TextureHandle());

										// Set implicit enable parameter base on the nonzero size
										if (texture_translator != nullptr)
										{
											IPoint2 res = texture_translator->get_texture_resolution();
											set_shader_bool(*shader, parameter_name + "_on", (res.x > 1));
										}
									}
									else
									{
										return TranslationResult::Aborted;
									}
								}
								else
								{
									// Translate the bitmap
									TranslationResult result;
									const Translator_BitmapFile_to_RTITexture::KeyStruct key = Translator_BitmapFile_to_RTITexture::KeyStruct::CreateFromBitmapParameter(*texmap, metaInfo, t, validity);
									const Translator_BitmapFile_to_RTITexture* texture_translator = AcquireChildTranslator<Translator_BitmapFile_to_RTITexture>(key, t, translation_progress, result);
									if(result != TranslationResult::Aborted)
									{
										rti::TEditPtr<rti::Shader> shader(shader_handle);
										set_shader_texture2d(*shader, parameter_name, (texture_translator != nullptr) ? texture_translator->get_texture_handle() : rti::TextureHandle());

										// Set implicit texture size parameters to shader - if needed
										if (texture_translator != nullptr)
										{
											IPoint2 res = texture_translator->get_texture_resolution();
											set_shader_int(*shader, parameter_name + "_x", res.x);
											set_shader_int(*shader, parameter_name + "_y", res.y);												
											set_shader_float(*shader, parameter_name + "_1ox", 1.0f/res.x);
											set_shader_float(*shader, parameter_name + "_1oy", 1.0f/res.y);												
										}
									}
									else
									{
										return TranslationResult::Aborted;
									}
								}
							}
							else // Bake it
							{
								{
									MSTR message;
									message.printf(_T("Baking texmap %s to RTI::Texture.\n"), texmap->GetName().data()); 
									GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Debug, message);
								}

								// Bake the texture
								TranslationResult result;
								const Translator_BakedTexmap_to_RTITexture* texture_translator = AcquireChildTranslator<Translator_BakedTexmap_to_RTITexture>(texmap, t, translation_progress, result);
								if(result != TranslationResult::Aborted)
								{
									rti::TEditPtr<rti::Shader> shader(shader_handle);
									set_shader_texture2d(*shader, parameter_name, (texture_translator != nullptr) ? texture_translator->get_texture_handle() : rti::TextureHandle());

									// Set implicit texture size paramters to shader - if needed
									if (texture_translator != nullptr)
									{
										IPoint2 res = texture_translator->get_texture_resolution();
										set_shader_int(*shader, parameter_name + "_x", res.x);
										set_shader_int(*shader, parameter_name + "_y", res.y);												
										set_shader_float(*shader, parameter_name + "_1ox", 1.0f/res.x);
										set_shader_float(*shader, parameter_name + "_1oy", 1.0f/res.y);												
									}
								}
								else
								{
									return TranslationResult::Aborted;
								}
							}
						}
					}
					break;
				case TYPE_BOOL:
					{
						rti::TEditPtr<rti::Shader> shader(shader_handle);
						set_shader_bool(*shader, parameter_name, value.b);
					}
					break;
				case TYPE_INT:
					{
						rti::TEditPtr<rti::Shader> shader(shader_handle);
						set_shader_int(*shader, parameter_name, value.i);
					}
					break;
				case TYPE_FLOAT:
					{
						rti::TEditPtr<rti::Shader> shader(shader_handle);
						set_shader_float(*shader, parameter_name, value.f);
					}
					break;
				case TYPE_COLOR:
				case TYPE_POINT3:
				case TYPE_FRGBA:
				case TYPE_RGBA:
					{
						rti::TEditPtr<rti::Shader> shader(shader_handle);
						set_shader_float3(*shader, parameter_name, *value.aclr);
					}
					break;
				case TYPE_MATRIX3:
					{
						rti::TEditPtr<rti::Shader> shader(shader_handle);
						Matrix3 mat = *value.m;

						// Wake up Neo..... follow the white rabbit.....
						Point3  row0 = value.m->GetRow(0);
						Point3  row1 = value.m->GetRow(1);
						Point3  row2 = value.m->GetRow(2);
						Point3  row3 = value.m->GetRow(3);

						rti::Mat4f temp(row0.x, row0.y, row0.z, 0.0,
									    row1.x, row1.y, row1.z, 0.0,
										row2.x, row2.y, row2.z, 0.0,
										row3.x, row3.y, row3.z, 1.0);

						set_shader_mat4f(*shader, parameter_name, temp);
					}
					break;

				case TYPE_UNSPECIFIED: // Just silently ignore these, it's varying parameters attached to the scene already
					break;

				default:
					{
						TSTR error;

 						error.printf(_T("Unsupported parameter type %d for parameter %s"), value.type, MaxSDK::Util::MaxString::FromUTF8(parameter_name).ToMCHAR().data()); // TODO LOCALIZE
						GetRenderSessionContext().GetLogger().LogMessage(MaxSDK::RenderingAPI::IRenderingLogger::MessageType::Error, error);
						break;
					}
				}
			}

			if (direction_spread != nullptr)  // Environment mode call, we will find three specially named parameters in injected code
			{
				rti::TEditPtr<rti::Shader> shader(shader_handle);
				set_shader_int(*shader,     "amg_num_dir_lights",            dirlights);
				set_shader_float4v(*shader, "amg_light_directional_spread",  *direction_spread);
				set_shader_float4v(*shader, "amg_light_color_primvisb",      *color_visi);
			}

			// Set the actual flags
			set_output_is_emitter(pTranslator->GetEmissiveFlag());
			set_output_is_matte(pTranslator->GetMatteFlag());
		}

	    GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Debug, _T("RapidSL shader complete"));

        return TranslationResult::Success;
	}
    else
    {
        // null material: nothing to be translated
        SetNumOutputs(0);
        return TranslationResult::Success;
    }
}

bool BaseTranslator_AMG_to_RTIShader::AMG_IsCompatible(MtlBase *mtlbase)
{
	return IAMGInterface::IsSupported(_T("RapidRT"), mtlbase->SuperClassID(), mtlbase->ClassID());
}


// Overriden error reporter
void BaseTranslator_AMG_to_RTIShader::report_shader_param_error(const char* param_name) const
{
// Debug message only
#ifdef _DEBUG
    MSTR param_name_mstr = MSTR::FromACP(param_name);
    MSTR shader_name = get_shader_name();
    MSTR error_message;
    error_message.printf(_T("Parameter '%s' not present on shader '%s' (optimized out by compiler?)"), static_cast<const MCHAR*>(param_name_mstr), shader_name.data());
	GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Progress, error_message);
#endif
	// Avoid warning, but do nothing
	param_name = param_name;
}

}}	// namespace 
