//_____________________________________________________________________________
//
// File: MetalBump.cpp
// 
//
//_____________________________________________________________________________


//_____________________________________________________________________________
//
// Include files  
//
//_____________________________________________________________________________

#include "MetalBump.h"
#include "ShaderConst.h"
#include "ShaderMat.h"
#include "MatMgr.h"
#include "IMtlEdit.h"
#include "MaxIcon.h"
#include "Color.h"
#include "shaders.h"
#include "AssetManagement\IAssetAccessor.h"
#include "assetmanagement\AssetType.h"
#include "IPathConfigMgr.h"
#include "ICustAttribContainer.h"
#include "CustAttrib.h"
#include "IViewportManager.h"
#include "Notify.h"
#include <Dxerr.h>
#include "AssetManagement/iassetmanager.h"

#include "3dsmaxport.h"

using namespace MaxSDK::AssetManagement;

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                               DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                               DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )


#ifndef ReleasePpo
   #define ReleasePpo(ppo) \
      if (*(ppo) != NULL) \
      { \
         (*(ppo))->Release(); \
         *(ppo) = NULL; \
      } \
      else (VOID)0
#endif

//_____________________________________________________________________________
//
// Globals
//
//_____________________________________________________________________________

class MaxShader;

static      DefaultClassDesc     gDefaultShaderDesc;
static      PSCM_Accessor        gPSCMAccessor;
//static    MaxShader            *gMaxShader;
static bool gUpdate = true;

ParamDlg*   MaxShader::m_UVGenDlg;

ClassDesc2* GetDefaultShaderDesc() 
{ 
   return(&gDefaultShaderDesc); 
}




//_____________________________________
//
// ParamBlock
//
//_____________________________________

static ParamBlockDesc2 MaterialParamBlk(0,_T("Params"),0,&gDefaultShaderDesc, 
   P_AUTO_CONSTRUCT + P_AUTO_UI, 1, 
   //rollout
   IDD_PANEL, IDS_PARAMS, 0, 0, NULL,

   DIFFUSE_FILE,  _T("DIFFUSE"), TYPE_BITMAP,   P_SHORT_LABELS,   IDS_MAPFILENAME,
   p_ui,       TYPE_BITMAPBUTTON, IDC_DIFFUSE,
   p_accessor,    &gPSCMAccessor,
   p_file_types,  IDS_ALL_DX_TYPES,
   p_end,

   NORMAL_FILE,   _T("NORMAL"),  TYPE_BITMAP,   P_SHORT_LABELS,   IDS_MAPFILENAME,
   p_ui,       TYPE_BITMAPBUTTON, IDC_NORMAL,
   p_accessor,    &gPSCMAccessor,
   p_file_types,  IDS_ALL_DX_TYPES,
   p_end,

   DETAIL_FILE,   _T("DETAIL"),  TYPE_BITMAP,   P_SHORT_LABELS, IDS_MAPFILENAME,
   p_ui,       TYPE_BITMAPBUTTON, IDC_DETAIL,
   p_accessor,    &gPSCMAccessor,
   p_file_types,  IDS_ALL_DX_TYPES,
   p_end,

   MASK_FILE,     _T("MASK"), TYPE_BITMAP,   P_SHORT_LABELS, IDS_MAPFILENAME,
   p_ui,       TYPE_BITMAPBUTTON, IDC_MASK,
   p_accessor,    &gPSCMAccessor,
   p_file_types,  IDS_ALL_DX_TYPES,
   p_end,

   REFLECTION_FILE,_T("REFLECTION"),TYPE_BITMAP,   P_SHORT_LABELS, IDS_MAPFILENAME,
   p_ui,       TYPE_BITMAPBUTTON, IDC_REFLECTION,
   p_accessor,    &gPSCMAccessor,
   p_file_types,  IDS_CM_FILE_TYPES,
   p_end,

   BUMP_FILE,     _T("BUMP"), TYPE_BITMAP,      P_SHORT_LABELS, IDS_MAPFILENAME,
   p_ui,       TYPE_BITMAPBUTTON, IDC_BUMP,
   p_accessor,    &gPSCMAccessor,
   p_file_types,  IDS_ALL_DX_TYPES,
   p_end,

   DIFFUSE_ON,    _T("DIFFUSE_ON"), TYPE_BOOL, 0,  IDS_MAT_STATE,
   p_default,     FALSE,
   p_ui,       TYPE_SINGLECHECKBOX, IDC_DIFFUSE_ON,
   p_accessor,    &gPSCMAccessor,
   p_end,

   NORMAL_ON,     _T("NORMAL_ON"), TYPE_BOOL, 0,   IDS_MAT_STATE,
   p_default,     FALSE,
   p_ui,       TYPE_SINGLECHECKBOX, IDC_NORMAL_ON,
   p_accessor,    &gPSCMAccessor,
   p_end,

   SPECULAR_ON,   _T("SPECULAR_ON"), TYPE_BOOL, 0, IDS_MAT_STATE,
   p_default,     FALSE,
   p_ui,       TYPE_SINGLECHECKBOX, IDC_SPECULAR_ON,
   p_accessor,    &gPSCMAccessor,
   p_end,

   DETAIL_ON,     _T("DETAIL_ON"), TYPE_BOOL, 0, IDS_MAT_STATE,
   p_default,     FALSE,
   p_ui,       TYPE_SINGLECHECKBOX, IDC_DETAIL_ON,
   p_accessor,    &gPSCMAccessor,
   p_end,

   MASK_ON,    _T("MASK_ON"), TYPE_BOOL, 0, IDS_MAT_STATE,
   p_default,     FALSE,
   p_ui,       TYPE_SINGLECHECKBOX, IDC_MASK_ON,
   p_accessor,    &gPSCMAccessor,
   p_end,

   REFLECTION_ON, _T("REFLECTION_ON"), TYPE_BOOL, 0,  IDS_MAT_STATE,
   p_default,     FALSE,
   p_ui,       TYPE_SINGLECHECKBOX, IDC_REFLECTION_ON,
   p_accessor,    &gPSCMAccessor,
   p_end,

   BUMP_ON,    _T("BUMP_ON"), TYPE_BOOL, 0, IDS_MAT_STATE,
   p_default,     FALSE,
   p_ui,       TYPE_SINGLECHECKBOX, IDC_BUMP_ON,
   p_accessor,    &gPSCMAccessor,
   p_end,

   SPIN_BUMPSCALE,   _T("SPIN_BUMPSCALE"), TYPE_INT, P_ANIMATABLE, IDS_BUMP_SCALE, 
   p_default,     1000, 
   p_ui,          TYPE_SLIDER, EDITTYPE_INT, IDC_EDIT_BUMPSCALE, IDC_SPIN_BUMPSCALE, 10, 
   p_range,       1000,4000, 
   p_accessor,    &gPSCMAccessor,
   p_end,

   // need to be the original name as it is used in RTT macroscript
   SPIN_MIXSCALE, _T("SPIN_TEXSCALE"), TYPE_INT, P_ANIMATABLE, IDS_MIX_SCALE, 
   p_default,     0, 
   p_range,       0,1000, 
   p_ui,          TYPE_SLIDER, EDITTYPE_INT, IDC_EDIT_MIXSCALE, IDC_SPIN_MIXSCALE, 10, 
   p_accessor,    &gPSCMAccessor,
   p_end,

   SPIN_ALPHA,    _T("SPIN_ALPHA"), TYPE_FLOAT, P_ANIMATABLE,  IDS_ALPHA, 
   p_default,     1.0f, 
   p_range,       0.0f,1.0f, 
   p_ui,          TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_ALPHA, IDC_SPIN_ALPHA, 0.01f, 
   p_accessor,    &gPSCMAccessor,
   p_end,

   COLOR_AMBIENT, _T("COLOR_AMBIENT"), TYPE_RGBA, P_ANIMATABLE,   IDS_COLOR_AMB, 
   p_default,     Color(0.0f,0.0f,0.0f), 
   p_ui,       TYPE_COLORSWATCH, IDC_COLOR_AMBIENT, 
   p_accessor,    &gPSCMAccessor,
   p_end,

   COLOR_DIFFUSE, _T("COLOR_DIFFUSE"), TYPE_RGBA, P_ANIMATABLE,   IDS_COLOR_DIFF,   
   p_default,     Color(1.0f,1.0f,1.0f), 
   p_ui,       TYPE_COLORSWATCH, IDC_COLOR_DIFFUSE, 
   p_accessor,    &gPSCMAccessor,
   p_end,

   COLOR_SPECULAR,   _T("COLOR_SPECULAR"), TYPE_RGBA, P_ANIMATABLE,  IDS_COLOR_SPEC,   
   p_default,     Color(1.0f,1.0f,1.0f), 
   p_ui,       TYPE_COLORSWATCH, IDC_COLOR_SPECULAR, 
   p_accessor,    &gPSCMAccessor,
   p_end,

   // to support flexible mapping...
   MAPPING_DIFFUSE1, _T("MAP_DIFFUSE1"), TYPE_INT, 0, IDS_MAP_DIFFUSE1, 
   p_default,     1, 
   p_ui,          TYPE_SPINNER, EDITTYPE_INT, IDC_EDIT_MAPDF1, IDC_SPIN_MAPDF1, 1.0f, 
   p_range,       1,100, 
   p_accessor,    &gPSCMAccessor,
   p_end,

   MAPPING_DIFFUSE2, _T("MAP_DIFFUSE2"), TYPE_INT, 0, IDS_MAP_DIFFUSE2, 
   p_default,     1, 
   p_ui,          TYPE_SPINNER, EDITTYPE_INT, IDC_EDIT_MAPDF2, IDC_SPIN_MAPDF2, 1.0f, 
   p_range,       1,100, 
   p_accessor,    &gPSCMAccessor,
   p_end,
   MAPPING_SPECULAR, _T("MAP_SPECULAR"), TYPE_INT, 0, IDS_MAP_SPECULAR, 
   p_default,     1, 
   p_ui,          TYPE_SPINNER, EDITTYPE_INT, IDC_EDIT_MAPSP, IDC_SPIN_MAPSP, 1.0f, 
   p_range,       1,100, 
   p_accessor,    &gPSCMAccessor,
   p_end,
   MAPPING_BUMP,  _T("MAP_BUMP"), TYPE_INT, 0, IDS_MAP_BUMP, 
   p_default,     1, 
   p_ui,          TYPE_SPINNER, EDITTYPE_INT, IDC_EDIT_MAPB, IDC_SPIN_MAPB, 1.0f, 
   p_range,       1,100, 
   p_accessor,    &gPSCMAccessor,
   p_end,
   SYNC_MATERIAL, _T("SYNC"), TYPE_BOOL,  0, IDS_SYNC,
   p_default,     FALSE,
   p_ui,       TYPE_SINGLECHECKBOX,  IDC_SYNC,
   p_accessor,    &gPSCMAccessor,
   p_end,
   SPIN_REFLECTSCALE, _T("SPIN_REFLECTSCALE"), TYPE_INT, P_ANIMATABLE, IDS_REFLECT_SCALE, 
   p_default,     0, 
   p_range,       0,1000, 
   p_ui,          TYPE_SLIDER, EDITTYPE_INT, IDC_EDIT_REFLECTSCALE, IDC_SPIN_REFLECTSCALE, 10, 
   p_accessor,    &gPSCMAccessor,
   p_end,

   ALPHA_ON,      _T("ALPHA_ON"), TYPE_BOOL, 0, IDS_MAT_STATE,
   p_default,     FALSE,
   p_ui,       TYPE_SINGLECHECKBOX, IDC_ALPHA_ON,
   p_accessor,    &gPSCMAccessor,
   p_end,


   p_end

   );


//////////////////////////////////////////////////////////////////////
// NH 22|05|2002  
// Added this BitmapNotify so that the DX texture 
// can be reloaded.  The reason is because only the name is used from
// from the bitmap and the actual loading is implemented using DX calls
// this forces them to be reloaded and makes them behave like true Max 
// bitmaps
//////////////////////////////////////////////////////////////////////
class ShaderUpdate : public BitmapNotify  
{
   public:
      MaxVertexShader * sh;
      ShaderMat         *Mat;
      RMatChannel channel;
      TCHAR * bitmapName;

      void SetMaterialData(MaxVertexShader * ms, TCHAR * name, RMatChannel ch){
         sh = ms;
         bitmapName = name;
         channel = ch; 
      }

      int Changed(ULONG flags) { 
         //we don't care about the flags - lets just reload !!
         Mat = sh->m_Material;
         Mat->SetChannelName(TSTR(bitmapName) ,true,channel);
         Mat->UnLoadTexture(channel);
         sh->m_BuildTexture = true;
         sh->m_ChannelName[channel] = bitmapName;
         sh->m_BuildTexture = true;
         GetCOREInterface()->ForceCompleteRedraw();
         return 1;
      }
};

void PSCM_Accessor::Set(PB2Value &v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
{

	MaxShader         *Map;
	MaxVertexShader   *VS;
	ShaderMat         *Mat;
	BOOL           On;
	bool        Update = false;

	Map = (MaxShader*)owner;

	if(!Map)
	{
		return;
	}

	VS = Map->m_VS;

	if(!VS)
	{
		return;
	}

	Mat = VS->m_Material;

	if(!Mat)
	{
		return;
	}  


	switch(id)
	{
	case DIFFUSE_FILE:   

		if(v.bm != NULL && v.bm->bi.Name() !=NULL)
		{
			const TCHAR* bitmapName = v.bm->bi.Name();
			Mat->SetChannelName(TSTR(bitmapName),true,CHANNEL_DIFFUSE);
			Mat->UnLoadTexture(CHANNEL_DIFFUSE);
			VS->m_BuildTexture = true;
			VS->m_ChannelName[CHANNEL_DIFFUSE] = bitmapName;

			if(_tcsicmp(v.s,GetString(IDS_BITMAP_NONE)))
			{
				Map->m_PBlock->SetValue(DIFFUSE_ON,t,true);
			}
			else
			{
				Map->m_PBlock->SetValue(DIFFUSE_ON,t,false);
			}
			//the following is to support the auto update  of bitmaps
			PBBitmap * bm = v.bm;
			bm->Load();
			Bitmap * b = v.bm->bm;
			if(b){
				Map->m_ShaderUpdateArray[CHANNEL_DIFFUSE]->SetMaterialData(
					VS,
					const_cast<TCHAR*>(bitmapName),
					CHANNEL_DIFFUSE);
				b->SetNotify(Map->m_ShaderUpdateArray[CHANNEL_DIFFUSE]);
			}
		}

		break;

	case NORMAL_FILE: 
		if(v.bm != NULL && v.bm->bi.Name() !=NULL)
		{
			const TCHAR* bitmapName = v.bm->bi.Name();
			Mat->SetChannelName(TSTR(bitmapName),true,CHANNEL_NORMAL);
			Mat->UnLoadTexture(CHANNEL_NORMAL);
			VS->m_BuildTexture = true;
			VS->m_ChannelName[CHANNEL_NORMAL] = bitmapName;

			if(_tcsicmp(v.s,GetString(IDS_BITMAP_NONE)))
			{
				Map->m_PBlock->SetValue(NORMAL_ON,t,true);
			}
			else
			{
				Map->m_PBlock->SetValue(NORMAL_ON,t,false);
			}
			//the follwoing is to support the auto update  of bitmaps
			PBBitmap * bm = v.bm;
			bm->Load();
			Bitmap * b = v.bm->bm;
			if(b){
				Map->m_ShaderUpdateArray[CHANNEL_NORMAL]->SetMaterialData(
					VS,
					const_cast<TCHAR*>(bitmapName),
					CHANNEL_NORMAL);
				b->SetNotify(Map->m_ShaderUpdateArray[CHANNEL_NORMAL]);
			}

		}

		break;


	case DETAIL_FILE: 
		if(v.bm != NULL && v.bm->bi.Name() !=NULL)
		{
			const TCHAR* bitmapName = v.bm->bi.Name();
			Mat->SetChannelName(TSTR(bitmapName),true,CHANNEL_DETAIL);
			Mat->UnLoadTexture(CHANNEL_DETAIL);
			VS->m_BuildTexture = true;
			VS->m_ChannelName[CHANNEL_DETAIL] = bitmapName;

			if(_tcsicmp(v.s,GetString(IDS_BITMAP_NONE)))
			{
				Map->m_PBlock->SetValue(DETAIL_ON,t,true);
			}
			else
			{
				Map->m_PBlock->SetValue(DETAIL_ON,t,false);
			}
			//the following is to support the auto update  of bitmaps
			PBBitmap * bm = v.bm;
			bm->Load();
			Bitmap * b = v.bm->bm;
			if(b){
				Map->m_ShaderUpdateArray[CHANNEL_DETAIL]->SetMaterialData(
					VS,
					const_cast<TCHAR*>(bitmapName),
					CHANNEL_DETAIL);
				b->SetNotify(Map->m_ShaderUpdateArray[CHANNEL_DETAIL]);
			}

		}

		break;

	case MASK_FILE:   
		if(v.bm != NULL && v.bm->bi.Name() !=NULL)
		{
			const TCHAR* bitmapName = v.bm->bi.Name();
			Mat->SetChannelName(TSTR(bitmapName),true,CHANNEL_MASK);
			Mat->UnLoadTexture(CHANNEL_MASK);
			VS->m_BuildTexture = true;
			VS->m_ChannelName[CHANNEL_MASK] = bitmapName;

			if(_tcsicmp(v.s,GetString(IDS_BITMAP_NONE)))
			{
				Map->m_PBlock->SetValue(MASK_ON,t,true);
			}
			else
			{
				Map->m_PBlock->SetValue(MASK_ON,t,false);
			}  
			PBBitmap * bm = v.bm;
			bm->Load();
			Bitmap * b = v.bm->bm;
			if(b){
				Map->m_ShaderUpdateArray[CHANNEL_MASK]->SetMaterialData(
					VS,
					const_cast<TCHAR*>(bitmapName),
					CHANNEL_MASK);
				b->SetNotify(Map->m_ShaderUpdateArray[CHANNEL_MASK]);
			}
		}

		break;


	case REFLECTION_FILE:   
		if(v.bm != NULL && v.bm->bi.Name() !=NULL)
		{
			const TCHAR* bitmapName = v.bm->bi.Name();
			Mat->SetChannelName(TSTR(bitmapName),true,CHANNEL_REFLECTION);
			Mat->UnLoadTexture(CHANNEL_REFLECTION);
			VS->m_BuildTexture = true;
			VS->m_ChannelName[CHANNEL_REFLECTION] = bitmapName;

			if(_tcsicmp(v.s,GetString(IDS_BITMAP_NONE)))
			{
				Map->m_PBlock->SetValue(REFLECTION_ON,t,true);
			}
			else
			{
				Map->m_PBlock->SetValue(REFLECTION_ON,t,false);
			}
			PBBitmap * bm = v.bm;
			bm->Load();
			Bitmap * b = v.bm->bm;
			if(b){
				Map->m_ShaderUpdateArray[CHANNEL_REFLECTION]->SetMaterialData(
					VS,
					const_cast<TCHAR*>(bitmapName),
					CHANNEL_REFLECTION);
				b->SetNotify(Map->m_ShaderUpdateArray[CHANNEL_REFLECTION]);
			}     
		}
		break;

	case BUMP_FILE:   
		if(v.bm != NULL && v.bm->bi.Name() !=NULL)
		{
			const TCHAR* bitmapName = v.bm->bi.Name();
			Mat->SetChannelName(TSTR(bitmapName),true,CHANNEL_BUMP);
			Mat->UnLoadTexture(CHANNEL_BUMP);
			VS->m_BuildTexture = true;
			VS->m_ChannelName[CHANNEL_BUMP] = bitmapName;

			if(_tcsicmp(v.s,GetString(IDS_BITMAP_NONE)))
			{
				Map->m_PBlock->SetValue(BUMP_ON,t,true);
			}
			else
			{
				Map->m_PBlock->SetValue(BUMP_ON,t,false);
			}
			PBBitmap * bm = v.bm;
			bm->Load();
			Bitmap * b = v.bm->bm;
			if(b){
				Map->m_ShaderUpdateArray[CHANNEL_BUMP]->SetMaterialData(
					VS,
					const_cast<TCHAR*>(bitmapName),
					CHANNEL_BUMP);
				b->SetNotify(Map->m_ShaderUpdateArray[CHANNEL_BUMP]);
			}              
		}
		break;




	case DIFFUSE_ON:
		if(!v.i)
		{
			Mat->m_Shader[CHANNEL_DIFFUSE].m_On = false;
			VS->m_ChannelUse &= (~MAT_DIFFUSE_ON);

		}
		else
		{
			if(Mat->IsNameSet(CHANNEL_DIFFUSE))
			{
				Mat->m_Shader[CHANNEL_DIFFUSE].m_On = true;
				VS->m_ChannelUse |= MAT_DIFFUSE_ON;
			}  
		}                 

		VS->m_BuildTexture = true;
		break;


	case NORMAL_ON:
		if(!v.i)
		{
			Mat->m_Shader[CHANNEL_NORMAL].m_On  = false;
			VS->m_ChannelUse &= (~MAT_NORMAL_ON);
		}
		else
		{
			if(Mat->IsNameSet(CHANNEL_NORMAL))
			{
				Mat->m_Shader[CHANNEL_NORMAL].m_On  = true;
				VS->m_ChannelUse |= MAT_NORMAL_ON;
			}
			Update = true;

		}                 

		if(Update)
		{
			Map->m_PBlock->GetValue(BUMP_ON,t,On,FOREVER);

			if(On)
			{
				Map->m_PBlock->SetValue(BUMP_ON,t,false);
			}
		}

		VS->m_BuildTexture = true;
		break;


	case SPECULAR_ON:

		if(!v.i)
		{
			Mat->m_Shader[CHANNEL_SPECULAR].m_On = false;
			VS->m_ChannelName[CHANNEL_SPECULAR] = GetString(IDS_BITMAP_NONE);
			VS->m_ChannelUse &= (~MAT_SPECULAR_ON);

			Map->m_PBlock->GetValue(MASK_ON,t,On,FOREVER);
			if(On)
			{
				Map->m_PBlock->SetValue(MASK_ON,t,false);

			}

		}
		else
		{
			VS->m_ChannelName[CHANNEL_SPECULAR] = _T("GSM.tga");
			TSTR channelName = _T("GSM.tga");
			Mat->SetChannelName(channelName,true,CHANNEL_SPECULAR);
			Mat->m_Shader[CHANNEL_SPECULAR].m_On = true;
			VS->m_ChannelUse |= MAT_SPECULAR_ON;
		}                 

		VS->m_BuildTexture = true;
		break;


	case DETAIL_ON:

		if(!v.i)
		{
			Mat->m_Shader[CHANNEL_DETAIL].m_On = false;
			VS->m_ChannelUse &= (~MAT_DETAIL_ON);
		}
		else
		{
			// Fixed a copy and paste here... The following was specified as CHANNEL_BUMP
			if(Mat->IsNameSet(CHANNEL_DETAIL))
			{
				Mat->m_Shader[CHANNEL_DETAIL].m_On = true;
				VS->m_ChannelUse |= MAT_DETAIL_ON;
			}
		}                 

		VS->m_BuildTexture = true;
		break;


	case MASK_ON:

		if(!v.i)
		{
			Mat->m_Shader[CHANNEL_MASK].m_On = false;
			VS->m_ChannelUse &= (~MAT_MASK_ON);
		}
		else
		{
			if(Mat->IsNameSet(CHANNEL_MASK))
			{
				Mat->m_Shader[CHANNEL_MASK].m_On = true;
				VS->m_ChannelUse |= MAT_MASK_ON;

			}

			Map->m_PBlock->SetValue(SPECULAR_ON,t,true);
		}                 

		VS->m_BuildTexture = true;
		break;


	case REFLECTION_ON:

		if(!v.i)
		{
			Mat->m_Shader[CHANNEL_REFLECTION].m_On = false;
			VS->m_ChannelUse &= (~MAT_REFLECTION_ON);

		}
		else
		{
			if(Mat->IsNameSet(CHANNEL_REFLECTION))
			{
				Mat->m_Shader[CHANNEL_REFLECTION].m_On = true;
				VS->m_ChannelUse |= MAT_REFLECTION_ON;
			}
		}                 
		VS->m_BuildTexture = true;
		break;

	case BUMP_ON:
		if(!v.i)
		{
			Mat->m_Shader[CHANNEL_BUMP].m_On = false;
			VS->m_ChannelUse &= (~MAT_BUMP_ON);
		}
		else
		{
			if(Mat->IsNameSet(CHANNEL_BUMP))
			{
				Mat->m_Shader[CHANNEL_BUMP].m_On = true;
				VS->m_ChannelUse |= MAT_BUMP_ON;
			}
			Update = true;

		}                 

		if(Update)
		{
			Map->m_PBlock->GetValue(NORMAL_ON,t,On,FOREVER);

			if(On)
			{  
				Map->m_PBlock->SetValue(NORMAL_ON,t,false);
			}
		}

		VS->m_BuildTexture = true;
		break;



	case SPIN_BUMPSCALE:

		Mat->m_BumpScale = ((float)v.i / SLIDER_SCALE);
		VS->m_BumpScale  = ((float)v.i / SLIDER_SCALE);
		break;


	case SPIN_MIXSCALE:

		Mat->m_MixScale  = ((float)v.i / SLIDER_SCALE);
		VS->m_MixScale   = ((float)v.i / SLIDER_SCALE);
		break;


	case SPIN_ALPHA:

		VS->m_Alpha = 1.0f;
		break;


	case SPIN_REFLECTSCALE:

		Mat->m_ReflectScale  = ((float)v.i / SLIDER_SCALE);
		VS->m_ReflectScale  = ((float)v.i / SLIDER_SCALE);
		break;


	case COLOR_AMBIENT:

		VS->m_Ambient = *v.p;
		break;


	case COLOR_DIFFUSE:
		VS->m_Diffuse = *v.p;
		break;

	case COLOR_SPECULAR:

		VS->m_Specular = *v.p;
		break;


	case ALPHA_ON:
		if(!v.i)
		{
			VS->m_Alpha = 1.0f;
		}
		else
		{
			VS->m_Alpha = 0.0f;
		}

		Mat->m_Alpha = VS->m_Alpha;
		Mat->SetAlpha(VS->m_Alpha);

		break;


	case MAPPING_DIFFUSE1:
		Map->MappingCh[0]=v.i;
		VS->m_RMesh[VS->m_CurCache].Invalidate();
		break;
	case MAPPING_DIFFUSE2:
		Map->MappingCh[1]=v.i;
		VS->m_RMesh[VS->m_CurCache].Invalidate();
		break;
	case MAPPING_SPECULAR:
		Map->MappingCh[2]=v.i;
		VS->m_RMesh[VS->m_CurCache].Invalidate();
		break;
	case MAPPING_BUMP:
		Map->MappingCh[3]=v.i;
		VS->m_RMesh[VS->m_CurCache].Invalidate();
		break;


	default: 
		break;

	}

	BOOL Sync;
	Map->m_PBlock->GetValue(SYNC_MATERIAL,0,Sync,FOREVER);
	if(Sync)
		Map->OverideMaterial();

	GetCOREInterface()->ForceCompleteRedraw();

}

void NotifyHandler(void *param, NotifyInfo *info) 
{
// Node can be added from an undo/redo etc.. At this time not all the node
// data can be accessed, such as Object state and TM etc...So in this case we have to tell the 
// lighting manager to reload the light list.   
   MaxVertexShader * vs = (MaxVertexShader*)param;
   TimeValue Time = GetCOREInterface()->GetTime();

   INode * newNode = (INode*)info->callParam;
   if(newNode)
   {
      ObjectState Os   = newNode->EvalWorldState(Time);


      if(Os.obj==NULL){
         vs->m_Lighting.m_forceUpdate = true;
         return;
      }
      if(Os.obj->SuperClassID() != LIGHT_CLASS_ID)
         return;

      switch(info->intcode)
      {
         case NOTIFY_SCENE_ADDED_NODE:
            vs->m_Lighting.AddLight(newNode);
            break;
         case NOTIFY_SCENE_PRE_DELETED_NODE:
            vs->m_Lighting.DeleteLight(newNode);
            break;
      }
   }
}
//_____________________________________
//
// Constructor
//
//_____________________________________

MaxVertexShader::MaxVertexShader(ReferenceTarget *rt) 
{
   int i;

   m_StdDualVS = NULL;
   m_StdDualVS = (IStdDualVS *)CreateInstance(REF_MAKER_CLASS_ID, 
                                    STD_DUAL_VERTEX_SHADER);

   if(m_StdDualVS)
   {
      m_StdDualVS->SetCallback((IStdDualVSCallback*)this);
   }

   m_RTarget      = rt;
   m_Device    = NULL;
   m_BuildTexture = true;
   m_ChannelUse   = MAT_NONE_ON;
   m_Material     = new ShaderMat();
   m_Draw         = true;  //this is for a better default for code checks by DX9 driver
   m_Ambient      = Color(0.0f,0.0f,0.0f);
   m_Diffuse      = Color(1.0f,1.0f,1.0f);
   m_Specular     = Color(1.0f,1.0f,1.0f);
   m_BumpScale    = 1.0f;
   m_MixScale     = 0.5f;
   m_ReflectScale = 0.5f;
   m_Alpha        = 1.0f;
   m_Ready        = false;
   m_Mtl       = NULL;
   m_GWindow      = NULL;
   m_MaxCache     = 0;
   m_CurCache     = 0;
   m_T            = GetCOREInterface()->GetTime();
   badDevice = false;

   MaxShader *Shader = (MaxShader *)GetRefTarg();

   if(Shader)
   {
      m_Device = Shader->GetDevice();
   }

   //lets check for compatibility here, we do it here as ConfirmDevice is 
   //in the draw thread, and you can get continuous error messages

   if(InternalCheck()!=S_OK)
      badDevice = true;

   if(m_Device && !badDevice)
   {
      MatMgr::GetIPtr()->LoadDefaults(m_Device,true);
   }

   for(i=0; i < MAX_MESH_CACHE; i++)
   {
      m_Mesh[i] = NULL;
      m_Node[i] = NULL;
   }
   RegisterNotification(NotifyHandler,this,NOTIFY_SCENE_ADDED_NODE);
   RegisterNotification(NotifyHandler,this,NOTIFY_SCENE_PRE_DELETED_NODE);

   GetCOREInterface()->RedrawViews(m_T);

}

//_____________________________________
//
// Destructor
//
//_____________________________________

MaxVertexShader::~MaxVertexShader()
{
   UnRegisterNotification(NotifyHandler,this,NOTIFY_SCENE_ADDED_NODE);
   UnRegisterNotification(NotifyHandler,this,NOTIFY_SCENE_PRE_DELETED_NODE);

   if(m_Material)
   {
      delete m_Material;
   }

   if(m_StdDualVS)
   {
      m_StdDualVS->DeleteThis();
   }

   m_RTarget = NULL;

}


//_____________________________________
//
// SetMeshCache 
//
//_____________________________________

void MaxVertexShader::SetMeshCache(Mesh *mesh, INode *node, TimeValue T)
{
   int i;

   //
   // Search for node
   //
   for(i=0; i < m_MaxCache; i++)
   {
      if(node == m_Node[i])
      {
         m_CurCache = i;
         break;
      }
   }
   //
   // Not found
   // 
   if(i == m_MaxCache)
   {
      //
      // Check for a deleted node slot
      //
      for(i=0; i < m_MaxCache; i++)
      {
         if(m_Node[i] == NULL)
         {
            m_CurCache        = i;
            m_Mesh[m_CurCache] = mesh;
            m_Node[m_CurCache] = node;
            m_RMesh[m_CurCache].Invalidate();

            break;
         }
      }
      //
      // Not found
      //
      if(i == m_MaxCache)
      {
         m_CurCache = m_MaxCache; 

         //
         // Full
         //    
         if(m_MaxCache + 1 >= MAX_MESH_CACHE)
         {
            m_Mesh[m_CurCache] = mesh;
            m_Node[m_CurCache] = node;
            m_RMesh[m_CurCache].Invalidate();

         }
         else
         {  //
            // Add
            // 
            m_Mesh[m_CurCache] = mesh;
            m_Node[m_CurCache] = node;
            m_RMesh[m_CurCache].Invalidate();

            m_MaxCache++;
         
         }
      }
      
   }

   if(mesh != m_Mesh[m_CurCache])
   {
      m_RMesh[m_CurCache].Invalidate();
      m_Mesh[m_CurCache] = mesh;
   }

// Not sure we really need to be so drastic....
// if(T != m_T)
// {
//    m_RMesh[m_CurCache].Invalidate();
// }

   m_T = T;
      
}




HRESULT MaxVertexShader::Initialize(Mesh *mesh, INode *node)
{

   SetMeshCache(mesh,node,GetCOREInterface()->GetTime());

   if(m_StdDualVS)
   {
      m_StdDualVS->Initialize(mesh, node);
   }

   Draw();

   return S_OK;

}

HRESULT MaxVertexShader::Initialize(MNMesh *mnmesh, INode *node)
{
   Mesh msh;
   mnmesh->OutToTri(msh);
   SetMeshCache(&msh,node,GetCOREInterface()->GetTime());
   if(m_StdDualVS)
   {
      m_StdDualVS->Initialize(mnmesh, node);
   }
   Draw();
   return S_OK;
}


//______________________________________
//
// SetColorStates
//
//______________________________________

void MaxVertexShader::SetColorStates(TimeValue T)
{
   int Val;

   MaxShader   *Shader;
      
   Shader = (MaxShader *)GetRefTarg();

   if(Shader)
   {
      Shader->m_PBlock->GetValue(SPIN_BUMPSCALE,T,Val,FOREVER);
      m_BumpScale = ((float)Val / SLIDER_SCALE);

      Shader->m_PBlock->GetValue(SPIN_MIXSCALE,T,Val,FOREVER);
      m_MixScale = ((float)Val / SLIDER_SCALE);

      Shader->m_PBlock->GetValue(SPIN_REFLECTSCALE,T,Val,FOREVER);
      m_ReflectScale = ((float)Val / SLIDER_SCALE);

      Shader->m_PBlock->GetValue(COLOR_AMBIENT,T,m_Ambient,FOREVER);
      Shader->m_PBlock->GetValue(COLOR_DIFFUSE,T,m_Diffuse,FOREVER);
      Shader->m_PBlock->GetValue(COLOR_SPECULAR,T,m_Specular,FOREVER);

      m_Material->m_BumpScale    = m_BumpScale;
      m_Material->m_MixScale     = m_MixScale;
      m_Material->m_ReflectScale  = m_ReflectScale;

      m_Material->SetAlpha(m_Alpha);

      m_Material->m_Ambient.r = m_Ambient.r;
      m_Material->m_Ambient.g = m_Ambient.g;
      m_Material->m_Ambient.b = m_Ambient.b;
      
      m_Material->m_Diffuse.r = m_Diffuse.r;
      m_Material->m_Diffuse.g = m_Diffuse.g;
      m_Material->m_Diffuse.b = m_Diffuse.b;

      m_Material->m_Specular.r = m_Specular.r;
      m_Material->m_Specular.g = m_Specular.g;
      m_Material->m_Specular.b = m_Specular.b;


   }


}

//_____________________________________
//
// GetMatIndex 
//
//_____________________________________

int MaxVertexShader::GetMatIndex()
{
	Mtl*			Std    = NULL;
	MaxShader*	s      = NULL;

	if(m_Mtl->IsMultiMtl())
	{
		MaxShader* Shader = (MaxShader*)GetRefTarg();

		// I use the SubAnims here, as I do not want any NULL material introduced that are not visible to the user
		// this can happen when you have N materials and you select N+1 mat ID - the material will add a NULL material 
		// to the list to compensate - this screws with the index into the paramblock
		for(int i=0; i < m_Mtl->NumSubs(); i++)
		{
			Animatable* anim = m_Mtl->SubAnim(i);

			if (anim && IsMtl(anim))
			{
				Std = (Mtl*)anim;
				ICustAttribContainer* cc = Std->GetCustAttribContainer();

				if(cc!=NULL)   //might happen if the material has never been loaded....
				{
					for (int kk = 0; kk < cc->GetNumCustAttribs(); kk++)
					{
						CustAttrib *ca = cc->GetCustAttrib(kk);
						IViewportShaderManager *manager = (IViewportShaderManager*)ca->GetInterface(VIEWPORT_SHADER_MANAGER_INTERFACE);
						if (manager) {
							s = (MaxShader*)manager->GetActiveEffect();
						}
					}


					if(s == Shader)
					{
						int id=0;
						// this gets interesting - the MatID are editable !! must get them from the PAramblock
						IParamBlock2 * pblock = m_Mtl->GetParamBlock(0);   // there is only one
						if(pblock)
						{
							ParamBlockDesc2* pd = pblock->GetDesc();

							for(int j = 0; j < pd->count; j++)
							{
								if(_tcscmp(_T("materialIDList"),pd->paramdefs[j].int_name)==0)  //not localised
								{
									ParamID pid = pblock->IndextoID(j);
									pblock->GetValue(pid,0,id,FOREVER,i); 
									id = id+1;  //for some reason this is stored as a zero index, when a 1's based index is used
								}
							}
						}
						return(id);
					}
				}
			}
		}

	}

	return(0);

}

//_____________________________________
//
// Draw 
//
//_____________________________________

void MaxVertexShader::Draw()
{
   bool        NegScale;
   MaxShader   *Shader;
   
   Shader = (MaxShader *)GetRefTarg();

   if(!m_Ready || badDevice)
   {
      m_Draw = false;
      return;
   }

   m_Draw    = true;

   m_Mtl  = m_Node[m_CurCache]->GetMtl();
   NegScale = TMNegParity(m_Node[m_CurCache]->GetObjectTM(m_T));
   m_RMesh[m_CurCache].SetMappingData(Shader->MappingCh);

   //
   // Create or reload textures
   //
   CreateTextures();
   //
   // No textures let max render it
   //
   // if(m_ChannelUse == MAT_NONE_ON)

   //NH - changed this, so that even specular is not displayed, unless textures are active
   // MAT_NONE_ON did not account for specular)
   // numloaded = 1 What = 2 specifies that the specular has been enabled with no textures loaded...
   int numLoaded,what;
   numLoaded = m_Material->NumLoaded(what);

   if(numLoaded==0 || (numLoaded == 1 && what == 2))
   {     
      m_Draw = false;
      return;
   }

   //
   // Prepare
   //


   SetCubemapMatrices();
   SetMatrices();
   SetRenderStates(true);
   SetColorStates(m_T);
   m_Material->UpdateMaterial(m_T / GetTicksPerFrame());
   //
   // Rebuild mesh if needed
   //
   if(!m_RMesh[m_CurCache].Evaluate(m_Device,m_Mesh[m_CurCache],GetMatIndex(),NegScale))
   {
      m_Draw = false;
      return;
   }
   //
   // Set light group based on maps loaded
   //
   m_Material->SetLightGroup();

   //
   // Ambient pass
   //
   if(!m_Lighting.EvaluateAmbient(m_Device,&m_RMesh[m_CurCache],m_Material))
   {
      m_Draw = false;
      return;
   }

   //
   // Lighting pass
   //
   if(!m_Lighting.EvaluateLighting(m_Device,&m_RMesh[m_CurCache],m_Material))
   {
      m_Draw = false;
      return;
   }
   //
   // Material pass
   //
   if(!m_Material->Evaluate((m_T / GetTicksPerFrame()),m_Device,&m_RMesh[m_CurCache]))
   {
      m_Draw = false;
      return;
   }
   //
   // Specular Lighting pass
   //
   if(!m_Lighting.EvaluateSpecular(m_Device,&m_RMesh[m_CurCache],m_Material))
   {
      m_Draw = false;
      return;

   }
/*
   //
   // Clear the alpha, for the alpha blending passes
   //
   if(!m_Lighting.ClearAlpha(m_Device,&m_RMesh[m_CurCache],m_Material))
   {
      m_Draw = false;
      return;
   }

*/
   SetRenderStates(false);
   //
   // Hack to keep max from drawing over 
   // the top me sometimes
   //
   m_Device->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
   m_Device->SetRenderState(D3DRS_ALPHAREF,0xFE);
   m_Device->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_LESS);//D3DCMP_GREATEREQUAL);
// m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
// m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);

   m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   m_Device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
   m_Device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
   ResetDXStates();


}


HRESULT MaxVertexShader::InternalCheck()
{
// int            i;
   D3DCAPS9    Caps;
   D3DDISPLAYMODE Mode;
// MatShaderInfo  *Info;
   TCHAR error[256];
   _stprintf(error,_T("%s"),GetString(IDS_ERROR));

      
   m_Ready = false;
   bool ref = false;

   if(m_Device)
   {
      m_Device->GetDeviceCaps(&Caps);
      m_Device->GetDisplayMode(0,&Mode);

      // for those mad Laptop users
      if(Caps.DeviceType == D3DDEVTYPE_REF)
         ref = true;
      
      if(!ref)
      {
         if(Mode.Format != D3DFMT_A8R8G8B8 && Mode.Format != D3DFMT_X8R8G8B8)
         {
            TSTR  Str = GetString(IDS_ERROR_TRUE_COLOR);
            MessageBox(NULL,Str,error,MB_OK | MB_ICONWARNING | MB_SETFOREGROUND | MB_APPLMODAL);
            return(E_FAIL);
         }   

         if(Caps.PixelShaderVersion < D3DPS_VERSION(1,1))
         {
            TSTR  Str = GetString(IDS_ERROR_NO_PS);
            MessageBox(NULL,Str,error,MB_OK | MB_ICONWARNING | MB_SETFOREGROUND | MB_APPLMODAL);

            m_GWindow = NULL;

            return(E_FAIL);
         }

         if(!(Caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP) || 
            !(Caps.TextureCaps & D3DPTEXTURECAPS_PROJECTED))
         {
            TSTR  Str = GetString(IDS_ERROR_NO_CUBEMAP);
            MessageBox(NULL,Str,error,MB_OK | MB_ICONWARNING | MB_SETFOREGROUND | MB_APPLMODAL);

            m_GWindow = NULL;

            return(E_FAIL);
         }
      }


      if(!m_Lighting.Init(m_Device))
      {
         m_Ready = false;
         return (E_FAIL);
      }


   }

   return(S_OK);

}

//_____________________________________
//
// ConfirmDevice 
//
//_____________________________________

HRESULT MaxVertexShader::ConfirmDevice(ID3D9GraphicsWindow *d3dgw)
{
   int            i;
   MatShaderInfo  *Info;

   m_Device = d3dgw->GetDevice();
   
   m_Ready = false;
   bool ref = false;


   if(m_Device && !badDevice)
   {

      m_GWindow = d3dgw;
      m_Ready   = true;


      MatMgr::GetIPtr()->LoadDefaults(m_Device,true);

      if(m_Material)
      {
         if(m_ChannelUse != m_Material->m_ChannelUse)
         {
            Info = MatMgr::GetIPtr()->GetMatInfo(m_ChannelUse);

            m_Material->Set(*Info);
            m_Material->SetStatesFromChannels(Info->m_BestChannelUse);

            for(i=0; i < CHANNEL_MAX; i++)
            {  
               m_Material->m_Shader[i].m_Name = m_ChannelName[i];
            }  

            m_BuildTexture = true;

            CreateTextures();
         }

      }


   }

   return(S_OK);
}

//_____________________________________
//
// SetCheck 
//
//_____________________________________

void MaxVertexShader::SetCheck()
{
	MaxShader   *Shader;
	int           i;
	BOOL       Check;

	Shader = (MaxShader *)GetRefTarg();

	if(Shader)
	{
		for(i=0; i < CHANNEL_MAX; i++)
		{
			ParamID pid = DIFFUSE_ON + i;
			if((m_ChannelUse & (1 << i)) && 
				(m_Material->m_ChannelLoaded & (1 << i)))
			{
				Check   = true;   
				gUpdate = false;

				Shader->m_PBlock->SetValue(pid,m_T,Check);
			}
			else
			{
				Check   = false;  
				gUpdate = false;
				Shader->m_PBlock->SetValue(pid,m_T,Check);
			}
		}
	}

}

//_____________________________________
//
// CreateTextures 
//
//_____________________________________

HRESULT MaxVertexShader::CreateTextures()
{
   
   if(m_BuildTexture && m_Device && m_Material)
   {
      m_Material->ReLoadAll(m_Device);
      m_ChannelUse   = m_Material->m_ChannelLoaded;
      m_BuildTexture = false;

   }

   return(S_OK);
}


//_____________________________________
//
// InitValid 
//
//_____________________________________

HRESULT MaxVertexShader::InitValid(Mesh *mesh, INode *node)
{  
   MaxShader   * Shader = (MaxShader *)GetRefTarg();
   if(Shader)
   {
      if(Shader->m_VS)
      {
         Shader->m_VS->m_RMesh[Shader->m_VS->m_CurCache].Invalidate();
      }
   }
   return(S_OK);
}

//_____________________________________
//
// InitValid 
//
//_____________________________________

HRESULT MaxVertexShader::InitValid(MNMesh *mnmesh, INode *node)
{
   MaxShader   * Shader = (MaxShader *)GetRefTarg();
   if(Shader)
   {
      if(Shader->m_VS)
      {
         Shader->m_VS->m_RMesh[Shader->m_VS->m_CurCache].Invalidate();
      }
   }
   return(S_OK);
}

HRESULT MaxVertexShader::SetCubemapMatrices()
{

   D3DXMATRIX matWorld = m_GWindow->GetWorldXform();
   D3DXMATRIX matView = m_GWindow->GetViewXform();
   D3DXMATRIX matProj = m_GWindow->GetProjXform();
   D3DXMATRIX matTemp1;
   D3DXMATRIX matTemp2;
   D3DXMATRIX matTemp3;

   D3DXMatrixMultiply(&matTemp1, &matWorld, &matView);
   D3DXMatrixMultiply(&matTemp2, &matTemp1, &matProj);

   // Transform from Model Space to H-Clip space
   D3DXMatrixTranspose(&matTemp3, &matTemp2);
   m_Device->SetVertexShaderConstantF(NH_CV_WORLDVIEWPROJ_0, (float*)&matTemp3, 4);

   // Conversion from 3ds max coords to Direct3D coords:
   //
   // 3ds max:  (Up, Front, Right) == (+Z, +Y, +X)
   //
   // Direct3D: (Up, Front, Right) == (+Y, +Z, +X)
   //
   // Conversion from 3ds max to Direct3D coords:
   //
   // 3ds max * conversion matrix = Direct3D
   //
   // [ x y z w ] * | +1  0  0  0 | = [ X Y Z W ]
   //               |  0  0 +1  0 |
   //               |  0 +1  0  0 |
   //               |  0  0  0 +1 |
   //

   // Conversion matrix
   D3DXMatrixIdentity(&matTemp2);
   matTemp2.m[1][1] = 0.0f;
   matTemp2.m[1][2] = 1.0f;
   matTemp2.m[2][1] = 1.0f;
   matTemp2.m[2][2] = 0.0f;

   // worldview
   D3DXMatrixMultiply(&matTemp3, &matView, &matTemp2);
   D3DXMatrixTranspose(&matTemp1, &matTemp3);
   m_Device->SetVertexShaderConstantF(NH_CV_WORLDVIEW_0, (float*)&matTemp1, 4);

   D3DXMatrixInverse(&matTemp1, NULL, &matTemp3);

   // No Transpose necessary for setting the Shader Constants
   m_Device->SetVertexShaderConstantF(NH_CV_WORLDVIEWIT_0, (float*)&matTemp1, 3);


   // Transform from Model Space to Direct3D World Space
   D3DXMatrixMultiply(&matTemp3, &matWorld, &matTemp2);

   D3DXMatrixTranspose(&matTemp1, &matTemp3);
   m_Device->SetVertexShaderConstantF(NH_CV_WORLD_0, (float*)&matTemp1, 4);



   // Use the Transpose of the Inverse of this Transform for the
   // Transformation of Normals from Model Space to Direct3D World Space
   D3DXMatrixInverse(&matTemp1, NULL, &matTemp3);

   // No Transpose necessary for setting the Shader Constants
   m_Device->SetVertexShaderConstantF(NH_CV_WORLDIT_0, (float*)&matTemp1, 3);

   // Camera position in Direct3D Camera Space
   D3DXVECTOR3 camPos, camWpos;
   camPos.x = camPos.y = camPos.z = 0.0f;

   // Inverse of View Transform to Transform Camera from Direct3D Camera Space
   // to 3ds max World Space
   D3DXMatrixInverse(&matTemp1, NULL, &matView);

   // Multiply in Transform to convert from 3ds max World Space
   // to Direct3D World Space
   D3DXMatrixMultiply(&matTemp3, &matTemp1, &matTemp2);

   // Find Camera position in Direct3D World Space
   D3DXVec3TransformCoord(&camWpos, &camPos, &matTemp3);
   m_Device->SetVertexShaderConstantF(NH_CV_EYE_WORLD, (float*)&camWpos, 1);

   return S_OK;
}

HRESULT MaxVertexShader::SetMatrices()
{
   D3DXMATRIX  MatWorld;
   D3DXMATRIX  MatView;
   D3DXMATRIX  MatProj;
   D3DXMATRIX  MatTemp;
   D3DXMATRIX  MatWorldView;
   D3DXMATRIX  MatWorldViewIT;
   D3DXMATRIX  MatWorldViewProj;
   D3DXMATRIX  MatViewIT;
   D3DXMATRIX  MatWorldIT;

   MatWorld = m_GWindow->GetWorldXform();
   MatView  = m_GWindow->GetViewXform();
   MatProj  = m_GWindow->GetProjXform();

   D3DXMatrixMultiply(&MatTemp,&MatWorld,&MatView);
   D3DXMatrixMultiply(&MatWorldViewProj,&MatTemp,&MatProj);
   D3DXMatrixMultiply(&MatWorldView,&MatWorld,&MatView);
   D3DXMatrixInverse(&MatWorldViewIT,NULL,&MatWorldView);
   D3DXMatrixInverse(&MatViewIT,NULL,&MatView);
   D3DXMatrixInverse(&MatWorldIT,NULL,&MatWorld);
   //
   // Projection transform
   //
   D3DXMatrixTranspose(&MatWorldViewProj,&MatWorldViewProj);
   m_Device->SetVertexShaderConstantF(CV_WORLDVIEWPROJ_0,(float*)&MatWorldViewProj(0,0),4);
   //
   // Worldview transform
   //
   D3DXMatrixTranspose(&MatWorldView,&MatWorldView);
   m_Device->SetVertexShaderConstantF(CV_WORLDVIEW_0,(float*)&MatWorldView(0,0),4);
   //
   // World transform
   //
   D3DXMatrixTranspose(&MatWorld,&MatWorld);
   m_Device->SetVertexShaderConstantF(CV_WORLD_0,(float*)&MatWorld(0,0),4);
   //
   // Inverse worldview transform
   //
   m_Device->SetVertexShaderConstantF(CV_WORLDVIEWIT_0,(float*)&MatWorldViewIT(0,0),1);
   m_Device->SetVertexShaderConstantF(CV_WORLDVIEWIT_1,(float*)&MatWorldViewIT(1,0),1);
   m_Device->SetVertexShaderConstantF(CV_WORLDVIEWIT_2,(float*)&MatWorldViewIT(2,0),1);
   //
   // Inverse world transform
   //
   m_Device->SetVertexShaderConstantF(CV_WORLDIT_0,(float*)&MatWorldIT(0,0),1);
   m_Device->SetVertexShaderConstantF(CV_WORLDIT_1,(float*)&MatWorldIT(1,0),1);
   m_Device->SetVertexShaderConstantF(CV_WORLDIT_2,(float*)&MatWorldIT(2,0),1);

   //
   // Inverse view transform
   //
   m_Device->SetVertexShaderConstantF(CV_EYE,(float*)D3DXVECTOR4(MatViewIT.m[3][0], 
                                           MatViewIT.m[3][1], 
                                           MatViewIT.m[3][2], 
                                           0.0f),1);


   m_Device->SetVertexShaderConstantF(CV_ZERO,(float*)D3DXVECTOR4(0.0f,0.0f,0.0f,0.0f),1);
   m_Device->SetVertexShaderConstantF(CV_ONE, (float*)D3DXVECTOR4(1.0f,1.0f,1.0f,1.0f),1);
   m_Device->SetVertexShaderConstantF(CV_HALF,(float*)D3DXVECTOR4(0.5f,0.5f,0.5f,0.5f),1);

   return(S_OK);
}


//_____________________________________
//
// ConfirmPixelShader 
//
//_____________________________________

HRESULT MaxVertexShader::ConfirmPixelShader(IDX9PixelShader *PS)
{
   return(S_OK);
}

//_____________________________________
//
// GetRefTarg 
//
//_____________________________________

ReferenceTarget *MaxVertexShader::GetRefTarg()
{
   return(m_RTarget);
}

//_____________________________________
//
// CreateVertexShaderCache 
//
//_____________________________________

VertexShaderCache *MaxVertexShader::CreateVertexShaderCache()
{
   return(new IDX8VertexShaderCache);
}



//_____________________________________
//
// Constructor 
//
//_____________________________________

MaxShader::MaxShader(BOOL Loading)
{
   int i;

   for(i=0; i < CHANNEL_MAX; i++) 
   {
      m_SubTex[i] = NULL;
      m_Maps[i]   = NULL;
      m_DAD[i] = NULL;
      m_ShaderUpdateArray[i] = new ShaderUpdate();

   }

   for(i=0;i<4;i++)
   {
      MappingCh[i]= 1;
   }

   m_PBlock = NULL;
   m_UVGen  = NULL;
   m_VS = NULL;

   //Defect 496257 - need to do this to stop a crash where the TimeChanged callback is called on
   //reset but as maxscript still has a reference the destructor never gets called and the timecallback
   //is not cleared.
   theHold.Suspend();
   gDefaultShaderDesc.MakeAutoParamBlocks(this);
   theHold.Resume();

   //this is used to stop unwanted messages coming through when used with Opengl or Heidi.  Just need
   //to make sure that the pointer is available elsewhere when needed, i.e in loading
   if(!Loading)
   {
      m_VS = new MaxVertexShader(this);
   }

   GetCOREInterface()->RegisterTimeChangeCallback(this);
   cubemapSize= 256;




}

MaxShader::~MaxShader()
{
   int i;

   if(m_VS)
   {
      delete m_VS;
   }

   for(i=0; i < CHANNEL_MAX; i++) 
   {
      if(m_Maps[i])
      {
         ReleaseICustButton(m_Maps[i]);
      }

      if(m_DAD[i])
      {
         delete m_DAD[i];
      }
      delete m_ShaderUpdateArray[i];
   }

   GetCOREInterface()->UnRegisterTimeChangeCallback(this);


}


IParamBlock2* MaxShader::GetParamBlockByID(BlockID id) 
{ 
   if(m_PBlock->ID() == id)
   {
      return(m_PBlock);
   }

   return(NULL); 
}


void MaxShader::GetValidity(TimeValue t, Interval &valid)
{
   Interval Valid;
   float  FValue;
// BOOL   On;
   int       i;

/*
   for(i=0; i < CHANNEL_MAX; i++)
   {  
      m_PBlock->GetValue(DIFFUSE_ON + i,t,On,valid);
   }
*/
   m_PBlock->GetValue(SPIN_BUMPSCALE,t,i,valid);
   m_PBlock->GetValue(SPIN_MIXSCALE,t,i,valid);
   m_PBlock->GetValue(SPIN_REFLECTSCALE,t,i,valid);
   m_PBlock->GetValue(SPIN_ALPHA,t,FValue,valid);
   m_PBlock->GetValue(COLOR_AMBIENT,t,m_VS->m_Ambient,valid);
   m_PBlock->GetValue(COLOR_DIFFUSE,t,m_VS->m_Diffuse,valid);
   m_PBlock->GetValue(COLOR_SPECULAR,t,m_VS->m_Specular,valid);

}


ParamDlg* MaxShader::CreateEffectDlg(HWND hwMtlEdit, IMtlParams *imp) 
{
   IAutoMParamDlg *MasterDlg;
   IParamMap2     *Map;
   HWND        Wnd;
   ICustButton    *But;
   int            i;
   
   MaxShaderDlg * paramDlg = new MaxShaderDlg(this);
   ip = imp;

   MasterDlg  = gDefaultShaderDesc.CreateParamDlgs(hwMtlEdit,imp,this);
   MaterialParamBlk.SetUserDlgProc(paramDlg);

   for(i=0; i< CHANNEL_MAX; i++)
   {
      m_DAD[i] = new MaxShaderDAD(this);
   }

   Map   = MasterDlg->GetMap();
   Wnd   = Map->GetHWnd();

   But = GetICustButton(GetDlgItem(Wnd,IDC_DIFFUSE));
   But->SetDADMgr(m_DAD[0]);
   m_Maps[0] = But;

   But = GetICustButton(GetDlgItem(Wnd,IDC_NORMAL));
   But->SetDADMgr(m_DAD[1]);
   m_Maps[1] = But;

   But = GetICustButton(GetDlgItem(Wnd,IDC_DETAIL));
   But->SetDADMgr(m_DAD[2]);
   m_Maps[2] = But;

   But = GetICustButton(GetDlgItem(Wnd,IDC_MASK));
   But->SetDADMgr(m_DAD[3]);
   m_Maps[3] = But;

   But = GetICustButton(GetDlgItem(Wnd,IDC_REFLECTION));
   But->SetDADMgr(m_DAD[4]);
   m_Maps[4] = But;

   But = GetICustButton(GetDlgItem(Wnd,IDC_BUMP));
   But->SetDADMgr(m_DAD[5]);
   m_Maps[5] = But;



   MasterDlg->InvalidateUI();
   MasterDlg->MtlChanged();

   gDefaultShaderDesc.RestoreRolloutState();

   return(MasterDlg);   
}

//_____________________________________
//
// ButtonToChannel 
//
//_____________________________________

RMatChannel MaxShader::ButtonToChannel(int Id)
{
   
   switch(Id)
   {
      case IDC_DIFFUSE:
                     return(CHANNEL_DIFFUSE);
                     break;

      case IDC_DETAIL:

                     return(CHANNEL_DETAIL);
                     break;
      case IDC_NORMAL:

                     return(CHANNEL_NORMAL);
                     break;

      case IDC_SPECULAR:
                     return(CHANNEL_SPECULAR);
                     break;

      case IDC_MASK:
                     return(CHANNEL_MASK);
                     break;

      case IDC_REFLECTION:
                     return(CHANNEL_REFLECTION);
                     break;

      case IDC_BUMP:
                     return(CHANNEL_BUMP);
                     break;

      default:
                     return(CHANNEL_DIFFUSE);
                     break;
                     
   }

}

//From ReferenceMaker
RefTargetHandle MaxShader::GetReference(int i) 
{
   switch(i) 
   {
      case 0:     
               return(m_UVGen);
               break;

      case 1:     

               return (m_PBlock);
               break;

      default: 
               return(NULL);
               break;

   }
   
}

//_____________________________________
//
// SetReference 
//
//_____________________________________

void MaxShader::SetReference(int i, RefTargetHandle rtarg) 
{

   switch(i) 
   {
      case 0:     
               m_UVGen = (UVGen *)rtarg; 
               break;
      case 1:  
               m_PBlock = (IParamBlock2 *)rtarg; 
               break;

      default: 
               break;
   }

}

//_____________________________________
//
// GetDevice 
//
//_____________________________________

LPDIRECT3DDEVICE9 MaxShader::GetDevice()
{
	GraphicsWindow    *GW;
	LPDIRECT3DDEVICE9 Device;

	ViewExp& View = GetCOREInterface()->GetActiveViewExp();

	if( ! View.IsAlive())
		return (NULL);

	GW = View.getGW();

	if(GW)
	{
		ID3D9GraphicsWindow *D3DGW = (ID3D9GraphicsWindow *)GW->GetInterface(D3D9_GRAPHICS_WINDOW_INTERFACE_ID);

		if(D3DGW)
		{
			Device = D3DGW->GetDevice();

			return(Device);
		}
	}
	return(NULL);
}

//_____________________________________
//
// Clone 
//
//_____________________________________

RefTargetHandle MaxShader::Clone(RemapDir &remap) 
{
   int            i;
   MatShaderInfo  *Info;
   MaxShader      *MNew;

   MNew = new MaxShader(FALSE);

   MNew->ReplaceReference(1,remap.CloneRef(m_PBlock));

   BaseClone(this, MNew, remap);

   MNew->m_VS->m_Device = m_VS->m_Device;

   //Is this really what was meant to happen ??  Doing the following
   //will mean the old RefTarg is used in the clone, which means that 
   //the data from the old paramblock would be used....
// MNew->m_VS->m_RTarget   = m_VS->m_RTarget;

   if(!MNew->m_VS->m_Device)
   {  
      MNew->m_VS->m_Device = GetDevice();
   }

   if(MNew->m_VS->m_Material)
   {
      delete MNew->m_VS->m_Material;

      MNew->m_VS->m_Material = new ShaderMat(*m_VS->m_Material);
   }

   for(i=0; i< CHANNEL_MAX; i++)
   {
      MNew->m_VS->m_ChannelName[i] = m_VS->m_ChannelName[i];
   }

   for(i=0; i < CHANNEL_MAX; i++)
   {  
      MNew->m_VS->m_Material->m_Shader[i].m_Name = m_VS->m_ChannelName[i];
   }  

   MNew->m_VS->m_CurCache     = 0;
   MNew->m_VS->m_MaxCache     = 0;
   MNew->m_VS->m_ChannelUse   = m_VS->m_ChannelUse;
   MNew->m_VS->m_Ambient      = m_VS->m_Ambient;
   MNew->m_VS->m_Diffuse      = m_VS->m_Diffuse;
   MNew->m_VS->m_Specular     = m_VS->m_Specular;
   MNew->m_VS->m_BumpScale    = m_VS->m_BumpScale;
   MNew->m_VS->m_MixScale     = m_VS->m_MixScale;
   MNew->m_VS->m_ReflectScale = m_VS->m_ReflectScale;
   MNew->m_VS->m_Alpha        = m_VS->m_Alpha;
   MNew->m_VS->m_T            = GetCOREInterface()->GetTime();

   for(i=0; i < MAX_MESH_CACHE; i++)
   {
      MNew->m_VS->m_Mesh[i] = NULL;
      MNew->m_VS->m_Node[i] = NULL;
   }

   if(MNew->m_VS->m_Device)
   {
      MatMgr::GetIPtr()->LoadDefaults(MNew->m_VS->m_Device,true);
   }

   Info = MatMgr::GetIPtr()->GetMatInfo(MNew->m_VS->m_ChannelUse);

   MNew->m_VS->m_Material->Set(*Info);
   MNew->m_VS->m_Material->SetStatesFromChannels(Info->m_BestChannelUse);

   MNew->m_VS->m_BuildTexture = true;
   MNew->m_VS->m_ChannelUse   = 0;

   MNew->m_VS->CreateTextures();
// MNew->m_VS->SetCheck();


   GetCOREInterface()->RedrawViews(MNew->m_VS->m_T);

   return((RefTargetHandle)MNew);

}

    
//_____________________________________
//
// SubAnim 
//
//_____________________________________

Animatable* MaxShader::SubAnim(int i) 
{
   switch(i) 
   {
      case 0: 
            return(m_UVGen);
            break;
      case 1: 

            return(m_PBlock);
            break;

      default: 

            return(m_SubTex[i-2]);
            break;

   }

}

//_____________________________________
//
// SubAnimName 
//
//_____________________________________

TSTR MaxShader::SubAnimName(int i) 
{
   switch(i) 
   {
      case 0: 
            return(GetString(IDS_COORDS));      
            break;

      case 1: 
            return(GetString(IDS_VSPARAMS));
            break;

      default: 

//          return(GetSubTexmapTVName(i-1));
            return(_T(""));
            break;

   }

}

//_____________________________________
//
// NotifyRefChanged 
//
//_____________________________________

RefResult MaxShader::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, 
                              PartID& partID, RefMessage message, BOOL propagate ) 
{


   switch(message)
   {
      case REFMSG_NODE_WSCACHE_UPDATED:
         
                           if(m_VS)
                           {
//                            m_VS->m_RMesh[m_VS->m_CurCache].Invalidate();
                           }
                           break;

      case REFMSG_TARGET_DELETED:
                           if(m_VS)
                           {
                              m_VS->m_Mesh[m_VS->m_CurCache] = NULL;
                              m_VS->m_Node[m_VS->m_CurCache] = NULL;
                           }
                           break;

      case REFMSG_CHANGE:
                           if(hTarget == m_PBlock)
                           {
                              ParamID changing_param = m_PBlock->LastNotifyParamID();
                              MaterialParamBlk.InvalidateUI(changing_param);
                              GetCOREInterface()->ForceCompleteRedraw();
                           }
                           else
                           {
                              if(m_VS)
                              {
                                 m_VS->m_RMesh[m_VS->m_CurCache].Invalidate();
                              }
                           }  
                           break;


      case REFMSG_WANT_SHOWPARAMLEVEL:{

/*                         if((partID & PART_TOPO)   || (partID & PART_GEOM) || 
                              (partID & PART_TEXMAP) || (partID & PART_MTL))
                           {
                              if(m_VS)
                              {
                                 m_VS->m_RMesh[m_VS->m_CurCache].Invalidate();
                              }
                           }

*/                         BOOL * pb = (BOOL*)(partID);
                           *pb = TRUE;
                           return REF_HALT;
                              }

                              

      default:
                           break;


   }


   return(REF_SUCCEED);
}


//_____________________________________
//
// Save 
//
//_____________________________________

IOResult MaxShader::Save(ISave *isave) 
{   
   int            i;
   unsigned long  Nb;

   for(i=0; i < CHANNEL_MAX; i++)
   {
      isave->BeginChunk(DIFFUSE_CHUNK + (i * CHUNK_STEP));
      isave->WriteWString(m_VS->m_ChannelName[i]);
      isave->EndChunk();
   }

   isave->BeginChunk(CHANNEL_CHUNK);
   isave->Write(&m_VS->m_ChannelUse,sizeof(m_VS->m_ChannelUse),&Nb); 
   isave->EndChunk();

   isave->BeginChunk(COLOR_CHUNK);
   isave->Write(&m_VS->m_Ambient,sizeof(m_VS->m_Ambient),&Nb); 
   isave->Write(&m_VS->m_Diffuse,sizeof(m_VS->m_Diffuse),&Nb); 
   isave->Write(&m_VS->m_Specular,sizeof(m_VS->m_Specular),&Nb);  
   isave->EndChunk();

   isave->BeginChunk(BUMP_SCALE_CHUNK);
   isave->Write(&m_VS->m_BumpScale,sizeof(m_VS->m_BumpScale),&Nb);   
   isave->EndChunk();

   isave->BeginChunk(MIX_SCALE_CHUNK);
   isave->Write(&m_VS->m_MixScale,sizeof(m_VS->m_MixScale),&Nb);  
   isave->EndChunk();

   isave->BeginChunk(ALPHA_CHUNK);
   isave->Write(&m_VS->m_Alpha,sizeof(m_VS->m_Alpha),&Nb);  
   isave->EndChunk();

   isave->BeginChunk(REFLECT_SCALE_CHUNK);
   isave->Write(&m_VS->m_ReflectScale,sizeof(m_VS->m_ReflectScale),&Nb);   
   isave->EndChunk();

   isave->BeginChunk(MAPPING_CHUNK);
   isave->Write(&MappingCh[0],sizeof(int),&Nb);
   isave->Write(&MappingCh[1],sizeof(int),&Nb);
   isave->Write(&MappingCh[2],sizeof(int),&Nb);
   isave->Write(&MappingCh[3],sizeof(int),&Nb);
   isave->EndChunk();



   return(IO_OK);
}

//_____________________________________
//
// Load 
//
//_____________________________________

IOResult MaxShader::Load(ILoad *iload) 
{ 
   TCHAR       *Buf;
   IOResult    Ret;
   unsigned long  Nb;
   MatShaderInfo  *Info;
   int            i;
   ShaderMat      *Mat;

   Ret = IO_OK;   

   if(!m_VS)
      m_VS = new MaxVertexShader(this);

   if(m_VS->m_Material)
   {
      delete m_VS->m_Material;
   }

   m_VS->m_Material = new ShaderMat();

   Mat = m_VS->m_Material;

   while(IO_OK == (Ret = iload->OpenChunk())) 
   {
      switch(iload->CurChunkID()) 
      {
         case DIFFUSE_CHUNK:
                        Ret = iload->ReadWStringChunk(&Buf);
                        m_VS->m_ChannelName[CHANNEL_DIFFUSE] = Buf;   
                        break;

         case NORMAL_CHUNK:
                        Ret = iload->ReadWStringChunk(&Buf);
                        m_VS->m_ChannelName[CHANNEL_NORMAL] = Buf;    
                        break;

         case SPECULAR_CHUNK:
                        Ret = iload->ReadWStringChunk(&Buf);
                        m_VS->m_ChannelName[CHANNEL_SPECULAR] = Buf;  
                        break;

         case DETAIL_CHUNK:
                        Ret = iload->ReadWStringChunk(&Buf);
                        m_VS->m_ChannelName[CHANNEL_DETAIL] = Buf;    
                        break;

         case MASK_CHUNK:
                        Ret = iload->ReadWStringChunk(&Buf);
                        m_VS->m_ChannelName[CHANNEL_MASK] = Buf;   
                        break;

         case REFLECTION_CHUNK:
                        Ret = iload->ReadWStringChunk(&Buf);
                        m_VS->m_ChannelName[CHANNEL_REFLECTION] = Buf;   
                        break;

         case BUMP_CHUNK:
                        Ret = iload->ReadWStringChunk(&Buf);
                        m_VS->m_ChannelName[CHANNEL_BUMP] = Buf;   
                        break;


         case CHANNEL_CHUNK:
                        Ret = iload->Read(&m_VS->m_ChannelUse,sizeof(m_VS->m_ChannelUse),&Nb);
                        break;


         case COLOR_CHUNK:
                        Ret = iload->Read(&m_VS->m_Ambient,sizeof(m_VS->m_Ambient),&Nb);
                        Ret = iload->Read(&m_VS->m_Diffuse,sizeof(m_VS->m_Diffuse),&Nb);
                        Ret = iload->Read(&m_VS->m_Specular,sizeof(m_VS->m_Specular),&Nb);
                        break;


         case BUMP_SCALE_CHUNK:

                        Ret = iload->Read(&m_VS->m_BumpScale,sizeof(m_VS->m_BumpScale),&Nb);
                        break;


         case MIX_SCALE_CHUNK:

                        Ret = iload->Read(&m_VS->m_MixScale,sizeof(m_VS->m_MixScale),&Nb);
                        break;


         case REFLECT_SCALE_CHUNK:

                        Ret = iload->Read(&m_VS->m_ReflectScale,sizeof(m_VS->m_ReflectScale),&Nb);
                        break;

         case ALPHA_CHUNK:

                        Ret = iload->Read(&m_VS->m_Alpha,sizeof(m_VS->m_Alpha),&Nb);
                        break;
         case MAPPING_CHUNK:
                        Ret = iload->Read(&MappingCh[0],sizeof(int),&Nb);
                        Ret = iload->Read(&MappingCh[1],sizeof(int),&Nb);
                        Ret = iload->Read(&MappingCh[2],sizeof(int),&Nb);
                        Ret = iload->Read(&MappingCh[3],sizeof(int),&Nb);
                        break;


      }


      iload->CloseChunk();

      if(Ret != IO_OK)
      {
         return(Ret);
      }
   }           


   if(Mat)
   {
      Mat->m_Ambient.r  = m_VS->m_Ambient.r;
      Mat->m_Ambient.g  = m_VS->m_Ambient.g;
      Mat->m_Ambient.b  = m_VS->m_Ambient.b;

      Mat->m_Diffuse.r  = m_VS->m_Diffuse.r;
      Mat->m_Diffuse.g  = m_VS->m_Diffuse.g;
      Mat->m_Diffuse.b  = m_VS->m_Diffuse.b;

      Mat->m_Specular.r = m_VS->m_Specular.r;
      Mat->m_Specular.g = m_VS->m_Specular.g;
      Mat->m_Specular.b = m_VS->m_Specular.b;

      Mat->m_BumpScale  = m_VS->m_BumpScale;
      Mat->m_MixScale      = m_VS->m_MixScale;
      Mat->m_ReflectScale  = m_VS->m_ReflectScale;

      Mat->m_Alpha      = m_VS->m_Alpha;

      for(i=0; i < CHANNEL_MAX; i++)
      {  
         Mat->m_Shader[i].m_Name = m_VS->m_ChannelName[i];
      }  

      if(!m_VS->m_Device)
      {  
         m_VS->m_Device = GetDevice();
      }

      if(m_VS->m_Device)
      {
         MatMgr::GetIPtr()->LoadDefaults(m_VS->m_Device,true);
      }

      Info = MatMgr::GetIPtr()->GetMatInfo(m_VS->m_ChannelUse);

      Mat->Set(*Info);
      Mat->SetStatesFromChannels(Info->m_BestChannelUse);

      m_VS->m_BuildTexture = true;
      m_VS->m_ChannelUse   = 0;
      m_VS->CreateTextures();
      m_VS->SetCheck();

   }


   return(IO_OK);
}

BaseInterface *MaxShader::GetInterface(Interface_ID id)
{
   if (id == VIEWPORT_SHADER_CLIENT_INTERFACE) {
      return static_cast<IDXDataBridge*>(this);
   }
   else if (id== VIEWPORT_SHADER9_CLIENT_INTERFACE)
   {
      return static_cast<IDX9DataBridge*>(this);
   }

   else if(id == DX9_VERTEX_SHADER_INTERFACE_ID) 
   {
      return((IDX9VertexShader *)m_VS);
   }
   else 
   {
      return(BaseInterface::GetInterface(id));
   }
}


//_____________________________________
//
// DrawMeshStrips
//
//_____________________________________


bool MaxVertexShader::DrawMeshStrips(ID3D9GraphicsWindow *d3dgw, MeshData *data)
{
   if(!data)
      return true;
   return(m_Draw);
}

//_____________________________________
//
// SetRenderStates 
//
//_____________________________________

void MaxVertexShader::SetRenderStates(bool HighFilter)
{
   int i;

   if(m_Device)
   {
      m_Device->SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);
      m_Device->SetRenderState(D3DRS_LIGHTING,FALSE);
      m_Device->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);      
      m_Device->SetRenderState(D3DRS_CULLMODE,D3DCULL_CW);
      m_Device->SetRenderState(D3DRS_STENCILENABLE,FALSE);
      m_Device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);   
      m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
      m_Device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);   
      m_Device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ZERO);
      m_Device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);

      m_Device->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
      m_Device->SetRenderState(D3DRS_SHADEMODE,D3DSHADE_GOURAUD);
      m_Device->SetRenderState(D3DRS_DITHERENABLE,TRUE);
      m_Device->SetRenderState(D3DRS_FOGENABLE,FALSE);
      m_Device->SetRenderState(D3DRS_COLORVERTEX,TRUE);
      m_Device->SetRenderState(D3DRS_SPECULARENABLE,FALSE);
      m_Device->SetRenderState(D3DRS_DEPTHBIAS,0);

      m_Device->SetRenderState(D3DRS_ALPHAREF,0x0);
      m_Device->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_ALWAYS);

      if(HighFilter)
      {
         for(i=0; i < MAX_TMUS; i++)
         {
            m_Device->SetSamplerState(i,D3DSAMP_MINFILTER,D3DTEXF_ANISOTROPIC);
            m_Device->SetSamplerState(i,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
            m_Device->SetSamplerState(i,D3DSAMP_MIPFILTER,D3DTEXF_LINEAR);
            m_Device->SetSamplerState(i,D3DSAMP_MAXANISOTROPY,4); 
            m_Device->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
         }
      }
      else
      {
         for(i=0; i < MAX_TMUS; i++)
         {
            m_Device->SetSamplerState(i,D3DSAMP_MINFILTER,D3DTEXF_POINT);
            m_Device->SetSamplerState(i,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
            m_Device->SetSamplerState(i,D3DSAMP_MIPFILTER,D3DTEXF_POINT);
            m_Device->SetSamplerState(i,D3DSAMP_MAXANISOTROPY,4); 
            m_Device->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
         }

      }

   }

}

//_____________________________________
//
// DrawWireMesh
//
//_____________________________________

bool MaxVertexShader::DrawWireMesh(ID3D9GraphicsWindow *d3dgw, WireMeshData *data)
{
   return(m_Draw);
}

//_____________________________________
//
// StartLines
//
//_____________________________________

void MaxVertexShader::StartLines(ID3D9GraphicsWindow *d3dgw, WireMeshData *data)
{
}

//_____________________________________
//
// AddLine 
//
//_____________________________________

void MaxVertexShader::AddLine(ID3D9GraphicsWindow *d3dgw, DWORD *vert, int vis)
{
}

//_____________________________________
//
// DrawLines 
//
//_____________________________________

bool MaxVertexShader::DrawLines(ID3D9GraphicsWindow *d3dgw)
{
   return(m_Draw);
}

//_____________________________________
//
// EndLines 
//
//_____________________________________

void MaxVertexShader::EndLines(ID3D9GraphicsWindow *d3dgw, GFX_ESCAPE_FN fn)
{
}

//_____________________________________
//
// StartTriangles 
//
//_____________________________________

void MaxVertexShader::StartTriangles(ID3D9GraphicsWindow *d3dgw, MeshFaceData *data)
{

}

//_____________________________________
//
// AddTriangle 
//
//_____________________________________

void MaxVertexShader::AddTriangle(ID3D9GraphicsWindow *d3dgw, DWORD index, int *edgeVis)
{
}

//_____________________________________
//
// DrawTriangles 
//
//_____________________________________

bool MaxVertexShader::DrawTriangles(ID3D9GraphicsWindow *d3dgw)
{
   return(m_Draw);
}

//_____________________________________
//
// EndTriangles 
//
//_____________________________________

void MaxVertexShader::EndTriangles(ID3D9GraphicsWindow *d3dgw, GFX_ESCAPE_FN fn)
{

}

//_____________________________________
//
// CanTryStrips 
//
//_____________________________________

bool MaxVertexShader::CanTryStrips()
{
   return(true);
}

//_____________________________________
//
// GetNumMultiPass 
//
//_____________________________________

int MaxVertexShader::GetNumMultiPass()
{
   return(1);
}

//______________________________________
//
// SetVertexShader 
//
//______________________________________

HRESULT MaxVertexShader::SetVertexShader(ID3D9GraphicsWindow *d3dgw, int numPass)
{
   return(S_OK);
}


//------------------
// render cubic map code

class PickControlNode : 
      public PickObjectProc
       {
   public:           
      MaxShader *theCA;
      BOOL pickCam;
      HWND hWnd;
      PickControlNode() {theCA=NULL; pickCam =FALSE;}
      BOOL Pick(INode *node);    
      void EnterMode();
      void ExitMode();     
      BOOL Filter(INode *node);
   };


BOOL PickControlNode::Filter(INode *node)
   {
   if (pickCam)
      {
      ObjectState os = node->EvalWorldState(GetCOREInterface()->GetTime());
      if (os.obj->SuperClassID()==CAMERA_CLASS_ID) 
         return TRUE;
      return FALSE;
      }

   else return TRUE;
   }


BOOL PickControlNode::Pick(INode *node)
   {

   if (node) {
      if (pickCam)
         {
         ObjectState os = node->EvalWorldState(GetCOREInterface()->GetTime());
         if (os.obj->SuperClassID()==CAMERA_CLASS_ID) 
            {
            CameraState cs;
            Interval iv;
            CameraObject *cam = (CameraObject *)os.obj;
            cam->EvalCameraState(GetCOREInterface()->GetTime(),iv,&cs);
//          theCA->SetNearRange(cs.nearRange,GetCOREInterface()->GetTime());
//          theCA->SetFarRange(cs.farRange,GetCOREInterface()->GetTime());
            }
         }
      else
         {
         theCA->RenderCubicMap(node);

         }


      }
   return TRUE;
   }

void PickControlNode::EnterMode()
   {
   ICustButton *iBut ;
   iBut = GetICustButton(GetDlgItem(hWnd,IDC_PICK_AND_RENDER));
   if (iBut) iBut->SetCheck(TRUE);

   }

void PickControlNode::ExitMode()
   {
   ICustButton *iBut ;
   iBut = GetICustButton(GetDlgItem(hWnd,IDC_PICK_AND_RENDER));
   if (iBut) iBut->SetCheck(FALSE);
   }
static PickControlNode thePickMode;


//______________________________________
//
// DlgProc 
//
//______________________________________

INT_PTR MaxShaderDlg::DlgProc(TimeValue t,IParamMap2 *map,HWND hwndDlg,UINT msg,WPARAM wParam,LPARAM lParam)      
{  
   int id = LOWORD(wParam);
   ICustButton *iBut ;
    switch(msg)
   {
      case WM_INITDIALOG:
      {

         iBut = GetICustButton(GetDlgItem(hwndDlg,IDC_PICK_AND_RENDER));
         iBut->SetType(CBT_CHECK);
         return true;
      }
      case WM_COMMAND:
         {
         switch (LOWORD(wParam)) 
            {
            case IDC_PICK_AND_RENDER:
               {
               shader->ip->EndPickMode();
               thePickMode.hWnd  = hwndDlg;              
               thePickMode.pickCam  = FALSE;             
               thePickMode.theCA = shader;
               shader->ip->SetPickMode(&thePickMode);
               break;
               }

            }
         break;
      }

      break;

   }
   return false;
}

//______________________________________
//
// OkToDrop 
//
//______________________________________

static Class_ID bmTexClassID(BMTEX_CLASS_ID,0);

BOOL MaxShaderDAD::OkToDrop(ReferenceTarget *dropThis, HWND hfrom, HWND hto, POINT p, SClass_ID type, BOOL isNew)
{
   if(hfrom == hto)
   {
      return(FALSE);
   }

   if(type == BITMAPDAD_CLASS_ID || (type==TEXMAP_CLASS_ID && dropThis->ClassID()==bmTexClassID))
   {
      return(true);
   }     

   return(false);

}


//______________________________________
//
// GetInstance 
//
//______________________________________

ReferenceTarget *MaxShaderDAD::GetInstance(HWND hwnd, POINT p, SClass_ID type) 
{
	DADBitmapCarrier* Bmc = NULL;
	PBBitmap * bm = NULL;

	Bmc = GetDADBitmapCarrier();

	if(Bmc)
	{
		for(int i = 0; i < CHANNEL_MAX; i++)
		{
			if(gMaxShader->m_Maps[i] && gMaxShader->m_Maps[i]->GetHwnd() == hwnd)
			{
				if(gMaxShader->m_PBlock && gMaxShader->m_VS)
				{
					ParamID pid = gMaxShader->m_PBlock->IndextoID(i);
					gMaxShader->m_PBlock->GetValue(pid, gMaxShader->m_VS->m_T, bm, FOREVER);

					if(bm)
					{
						Bmc->SetName(TSTR(bm->bi.Name()));
					}
					else
					{
						Bmc->SetName(TSTR(GetString(IDS_BITMAP_NONE)));
					}  
				}

				break;
			}
		}
	}

	return(Bmc);
}

//______________________________________
//
// Drop 
//
//______________________________________

//static Class_ID bmTexClassID(BMTEX_CLASS_ID,0);

void MaxShaderDAD::Drop(ReferenceTarget *dropThis, HWND hwnd, POINT p, SClass_ID type, DADMgr*, BOOL) 
{
	TCHAR           Name[256];
	PBBitmap       Bitmap;
	int               i;
	BitmapInfo        Info;
	DADBitmapCarrier  *Bmc;

	Name[0] = _T('\0');

	if(dropThis)
	{

		if(dropThis->SuperClassID() == BITMAPDAD_CLASS_ID)
		{
			Bmc = (DADBitmapCarrier *)dropThis;
			_tcscpy(Name,Bmc->GetName());
		}
		else if(dropThis->SuperClassID() == TEXMAP_CLASS_ID)
		{
			BitmapTex * tex = (BitmapTex*) dropThis;
			if(tex)
				_tcscpy(Name,tex->GetMapName());

		}
		else
			return;

	}

	if(!_tcslen(Name))
	{
		_tcscpy(Name,GetString(IDS_BITMAP_NONE));
	}


	for(i=0; i < CHANNEL_MAX; i++)
	{
		if(gMaxShader->m_Maps[i] && gMaxShader->m_Maps[i]->GetHwnd() == hwnd)
		{
			if(gMaxShader->m_PBlock && gMaxShader->m_VS)
			{
				AssetUser asset = IAssetManager::GetInstance()->GetAsset(Name,kBitmapAsset);
				Bitmap.bi.SetAsset(asset);
				Bitmap.bm = NULL;
				ParamID pid = DIFFUSE_FILE + i;
				gMaxShader->m_PBlock->SetValue(pid,gMaxShader->m_VS->m_T,&Bitmap);
				gMaxShader->m_Maps[i]->SetText(Name);
			}
			break;
		}
	}

}

//hacky hacky.....There must be another way to get this beast to update
void MaxShader::TimeChanged(TimeValue t) 
{
   Interval ivalid = FOREVER;
   GetValidity(t,ivalid);
   bool movie = false;

   for(int i=0; i < CHANNEL_MAX; i++)
   {  
      if(   m_VS->m_Material->m_Shader[i].IsMovie())
         movie = true;
   }  

   if(ivalid.Start()==ivalid.End() || movie)
   {
//    NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
      GetCOREInterface()->RedrawViews(t);
   }
}

#define MSEMULATOR_CLASS_ID   Class_ID(0x6de34e16, 0x4796bc9a)
#define VIEWPORTLOADER_CLASS_ID Class_ID(0x5a06293c, 0x30420c1e)


MtlBase * GetMaterialOwner(ReferenceTarget * targ)
{
   if (NULL == targ) {
     return NULL;
   }
   
   CustAttrib * vl = NULL;
   DependentIterator di(targ);
   ReferenceMaker* maker = NULL;

   // find the viewport manager first
   while ((maker = di.Next()) != NULL) 
   {
       if (maker->SuperClassID() == CUST_ATTRIB_CLASS_ID && 
           maker->ClassID()==VIEWPORTLOADER_CLASS_ID ) 
       {
          vl = (CustAttrib*)maker;
          break;
       }
   }


// now find the ViewportManager owner - which should be a Custom Attribute Container
   if(!vl) return NULL;

	 ICustAttribContainer * cc = NULL;
   DependentIterator divl(vl);
   while ((maker = divl.Next())!= NULL) 
   {
       if (maker->SuperClassID() == REF_MAKER_CLASS_ID && 
           maker->ClassID()== CUSTATTRIB_CONTAINER_CLASS_ID) 
       {
          cc = (ICustAttribContainer*)maker;
          break;
       }
   }

   if(cc)
      return (MtlBase *) cc->GetOwner();     // the owner will be the material

   return NULL;
}

static Texmap * SetUpNormalBumpTexmap(int index, Mtl * mtl, MaxShader * vs)
{
	Texmap* bTex = mtl->GetSubTexmap(index);
	Texmap* bmap = NULL;
	if(bTex == NULL || bTex->ClassID() != GNORMAL_CLASS_ID)
	{
		bTex = static_cast<Texmap*>(CreateInstance(TEXMAP_CLASS_ID, GNORMAL_CLASS_ID));
	}

	if(bTex)
	{
		bTex->GetParamBlock(gnormal_params)->GetValue(gn_map_normal,0,bmap,FOREVER);
		if(bmap == NULL)
		{
			bmap = NewDefaultBitmapTex();
			bTex->GetParamBlock(gnormal_params)->SetValue(gn_map_normal,0,bmap);
			static_cast<StdMat2*>(mtl)->SetSubTexmap(index,bTex);
		}
	}
	//! Return the BitmapTex, so we can set this up in the standard way.
	return bmap;
}

static void SetNormalBumpAmount(Mtl * mtl, int index, float bumpVal, TimeValue t)
{
	Texmap *bTex = mtl->GetSubTexmap(index);
	if(bTex == NULL || bTex->ClassID() != GNORMAL_CLASS_ID)
	{
		return;
	}

	if(bTex)
	{
		//! Need to set the bump amount in the material to 1 (100) and control the bump 
		//! in NormalBump itself.
		static_cast<StdMat2*>(mtl)->SetTexmapAmt(index,1,t);
		// note, gn_map_bump has a value of 3, not 0 like before. Thus is the pitfalls of doing
		// things wrong in the first place by hard-coding values. 
		bTex->GetParamBlock(gnormal_params)->SetValue(gn_map_bump,t,bumpVal);
	}
}

void MaxShader::OverideMaterial()
{
   int num = 0;
   int ch;
   BOOL specOn, maskOn,bumpOn;

    MtlBase * mb = GetMaterialOwner(this);

   if(!mb || mb->ClassID() != Class_ID(DMTL_CLASS_ID, 0) )
      return;

   StdMat2 * mtl = (StdMat2*)mb;
   int index;

   macroRecorder->Disable();

   TimeValue t = GetCOREInterface()->GetTime();

   //set diffuse
   Color Diffuse;
   m_PBlock->GetValue(COLOR_DIFFUSE,t,Diffuse,FOREVER);
   mtl->SetDiffuse(Diffuse,t);

   if(m_VS->m_ChannelUse != MAT_NONE_ON)
   {
      Texmap * emul = mtl->GetSubTexmap(ID_DI);

      if(!emul||emul->ClassID()!=MSEMULATOR_CLASS_ID)
      {

         emul = (Texmap*)CreateInstance(TEXMAP_CLASS_ID,MSEMULATOR_CLASS_ID);
         mtl->SetSubTexmap(ID_DI, emul);
      }


      UVGen * uvGen = emul->GetTheUVGen();
      m_PBlock->GetValue(MAPPING_DIFFUSE1,t,ch,FOREVER);
      uvGen->SetMapChannel(ch);
   }

   m_PBlock->GetValue(SPECULAR_ON,t,specOn,FOREVER);
   if(specOn)
   {
      Color c;  
      Control *cont;
      cont = m_PBlock->GetControllerByID(COLOR_SPECULAR);
      SuspendAnimate();
      if(cont)
      {
         num = cont->NumKeys();
         for(int i=0;i<num;i++)
         {
            TimeValue kt = cont->GetKeyTime(i);
            AnimateOn();
            m_PBlock->GetValue(COLOR_SPECULAR,kt,c,FOREVER);
            mtl->SetSpecular(c,kt);
         }
      }
      else
      {
         AnimateOff();
         m_PBlock->GetValue(COLOR_SPECULAR,t,c,FOREVER);
         mtl->SetSpecular(c,t);  
      }
	
      AnimateOff();
      mtl->SetShinStr(0.7f,t);
      mtl->SetShininess(0.07f,t);
      ResumeAnimate();
   }
   
   m_PBlock->GetValue(MASK_ON,t,maskOn,FOREVER);
   index = mtl->StdIDToChannel(ID_SS);
   if(maskOn)
   {
      if(m_VS->m_ChannelName[CHANNEL_SPECULAR].Length() && m_VS->m_ChannelName[CHANNEL_SPECULAR] != const_cast<const TCHAR*>(GetString(IDS_BITMAP_NONE)))
      {
         BitmapTex *bTex = NewDefaultBitmapTex();
         bTex->SetMapName(m_VS->m_ChannelName[CHANNEL_MASK].data());
         mtl->SetSubTexmap(index, bTex);
         UVGen * uvGen = bTex->GetTheUVGen();
         m_PBlock->GetValue(MAPPING_SPECULAR,t,ch,FOREVER);
         uvGen->SetMapChannel(ch);

      }
   }
   else
      mtl->SetSubTexmap(index, NULL);


   BOOL bump,normal;
   m_PBlock->GetValue(BUMP_ON,t,bump,FOREVER);
   m_PBlock->GetValue(NORMAL_ON,t,normal,FOREVER);
   bumpOn = bump || normal ? TRUE : FALSE;

   index = mtl->StdIDToChannel(ID_BU);
   if(bumpOn)
   {
      SuspendAnimate();
      if((m_VS->m_ChannelName[CHANNEL_BUMP].Length() && m_VS->m_ChannelName[CHANNEL_BUMP] != const_cast<const TCHAR*>(GetString(IDS_BITMAP_NONE)))
		  || (m_VS->m_ChannelName[CHANNEL_NORMAL].Length() && m_VS->m_ChannelName[CHANNEL_NORMAL] != const_cast<const TCHAR*>(GetString(IDS_BITMAP_NONE))))
      {
         int amnt=0;
         float bump;
         Control * cont = m_PBlock->GetControllerByID(SPIN_BUMPSCALE);
		 bool bNormalBump = m_VS->m_ChannelName[CHANNEL_NORMAL] != const_cast<const TCHAR*>(GetString(IDS_BITMAP_NONE)) && normal ? true : false;
		 BitmapTex *pTex = NULL;

		 int ChannelUsage = CHANNEL_BUMP;

		 if(!bNormalBump)
		 {
			pTex = static_cast<BitmapTex*>(mtl->GetSubTexmap(index));
			if(pTex == NULL || pTex->ClassID() != Class_ID(BMTEX_CLASS_ID, 0))
			{
				pTex = NewDefaultBitmapTex();
				mtl->SetSubTexmap(index, pTex);
			}
		 }
		 else
		 {
			pTex = static_cast<BitmapTex*>(SetUpNormalBumpTexmap(index,mtl,this));
			ChannelUsage = CHANNEL_NORMAL;
		 }

		 //FP 06/09/06: Defect 795059. Fix synchronization issue by not recording the created ParamB2Restore obj.
		 theHold.Suspend();
		 pTex->SetMapName(m_VS->m_ChannelName[ChannelUsage].data());
		 theHold.Resume();

		 
		 UVGen * uvGen = pTex->GetTheUVGen();
		 m_PBlock->GetValue(MAPPING_BUMP,t,ch,FOREVER);
		 uvGen->SetMapChannel(ch);


         if(cont)
         {
            num = cont->NumKeys();
            for(int i=0;i<num;i++)
            {
               TimeValue kt = cont->GetKeyTime(i);
               AnimateOn();
               m_PBlock->GetValue(SPIN_BUMPSCALE,kt,amnt,FOREVER);
               bump  = (float)((amnt-1000) /3000.0f);

			   if(!bNormalBump)
			   {
					mtl->SetTexmapAmt(index, bump, kt);
			   }
			   else
			   {
				   SetNormalBumpAmount(mtl,index,bump,kt);
			   }
            }
         }
         else
         {
            AnimateOff();
            m_PBlock->GetValue(SPIN_BUMPSCALE,t,amnt,FOREVER);
            bump  = (float)((amnt-1000) /3000.0f);
			if(!bNormalBump)
			{
				mtl->SetTexmapAmt(index, bump, t);
			}
			else
			{
				SetNormalBumpAmount(mtl,index,bump,t);
			}

         }

      }
      ResumeAnimate();
   }
   else
      mtl->SetSubTexmap(index, NULL);

   macroRecorder->Enable();

}




#define VIEW_UP 0
#define VIEW_DN   1
#define VIEW_LF   2
#define VIEW_RT 3
#define VIEW_FR   4
#define VIEW_BK   5

static TCHAR *suffixes[6] = { 
   _T("_UP"),
   _T("_DN"),
   _T("_LF"),
   _T("_RT"),
   _T("_FR"),
   _T("_BK"),
   };


Matrix3 TMForView( int i) {
   Matrix3 m;
   m.IdentityMatrix();
   switch (i) {
      case VIEW_UP:    // looking UP
         m.RotateX(-PI);   
         break;
      case VIEW_DN:    // looking down (top view)
         break;
      case VIEW_LF:   // looking to left
         m.RotateX(-.5f*PI);  
         m.RotateY(-.5f*PI);
         break;
      case VIEW_RT:     // looking to right
         m.RotateX(-.5f*PI);  
         m.RotateY(+.5f*PI);
         break;
      case VIEW_FR:       // looking to front (back view)
         m.RotateX(-.5f*PI);  
         m.RotateY(PI);
         break;
      case VIEW_BK:         // looking to back
         m.RotateX(-.5f*PI);  
         break;
      }
   return m;
   }


int  MaxShader::GetOutFileName(TSTR &fullname, TSTR &fnameOrig, int i) {
   TSTR path;
   TSTR ext= _T(".bmp");
   TSTR fname = _T("3dsmax_cubic_gen");
   TSTR upname;// = biOutFile.Name();
   Interval iv;
	TCHAR baseName[MAX_PATH] = {0};


   fname += suffixes[i];

   _tcscpy(baseName, GetCOREInterface()->GetDir(APP_AUTOBACK_DIR));
   if (_tcslen(baseName) && baseName[_tcslen(baseName)-1] != _T('\\')) {
      _tcscat(baseName, _T("\\"));
      }


   fullname.printf(_T("%s%s%s"),baseName,fname.data(),ext.data());
   return 1;
   }

int MaxShader::WriteBM(Bitmap *bm, const AssetUser& file) {
   AssetUser upasset = biOutFile.GetAsset();
   biOutFile.SetAsset(file);
   if (bm->OpenOutput(&biOutFile) != BMMRES_SUCCESS) 
      goto bail;
   if (bm->Write(&biOutFile,BMM_SINGLEFRAME) != BMMRES_SUCCESS) 
      goto bail;
   bm->Close(&biOutFile);
   biOutFile.SetAsset(upasset);
   return 1;

   bail:
   biOutFile.SetAsset(upasset);
   return 0;
   }



void MaxShader::RenderCubicMap(INode *node)
   {
   int res;
   BOOL success = 0;
   TSTR fname,fullname;

   Interval iv;
   TSTR f1;
   PBBitmap * cubemap;

   if (cubemapSize<=0) 
      return;


   m_PBlock->GetValue(REFLECTION_FILE,m_VS->m_T,cubemap,iv);

   AssetUser originalAsset;
   if( cubemap && cubemap->bi.GetAsset().GetId()!=kInvalidId)
   {
      // File already set
	   fname = cubemap->bi.GetAsset().GetFileName();
	   originalAsset = cubemap->bi.GetAsset();
   }
   else
   {
      //No file, let's pick one
      if(!BrowseOutFile(f1))
         return;  
      fname = f1;
	  originalAsset = IAssetManager::GetInstance()->GetAsset(fname,kBitmapAsset);
   }
   
   Interface *ip = GetCOREInterface();
   BOOL wasHid = node->IsNodeHidden();
   node->Hide(TRUE);

   // Create a blank bitmap
   Bitmap *bm = NULL;
   biOutFile.SetWidth(cubemapSize);
   biOutFile.SetHeight(cubemapSize);
   biOutFile.SetType(BMM_TRUE_64);
   biOutFile.SetAspect(1.0f);
   biOutFile.SetCurrentFrame(0);
   bm = TheManager->Create(&biOutFile);

   Matrix3 nodeTM = node->GetNodeTM(ip->GetTime());
   Matrix3 tm; 
   INode* root = ip->GetRootNode();    
   bm->Display(GetString(IDS_ACUBIC_NAME));

   // NEW WAY
   ViewParams vp;
   vp.projType = PROJ_PERSPECTIVE;
   vp.hither = .001f;
   vp.yon = 1.0e30f;
   vp.fov = PI/2.0f;
// vp.nearRange = nearRange;
// vp.farRange = farRange;
   vp.nearRange = 0;
   vp.farRange = 1.0e30f;
   BOOL saveUseEnvMap = ip->GetUseEnvironmentMap();
   ip->SetUseEnvironmentMap(true);
// ip->SetUseEnvironmentMap(useEnvMap);

   res = ip->OpenCurRenderer(&vp); 
   for (int i=0; i<6; i++) {
      tm = TMForView(i);
      tm.PreTranslate(-nodeTM.GetTrans()); 
      vp.affineTM = tm;
      if (!GetOutFileName(fullname,fname,i)) 
         goto fail;
      bm->SetWindowTitle(fname);
      res = ip->CurRendererRenderFrame(ip->GetTime(),bm,NULL,1.0f,&vp);
      if (!res) 
         goto fail;
	  AssetUser asset = IAssetManager::GetInstance()->GetAsset(fullname,kBitmapAsset);
      if (!WriteBM(bm, asset)) goto fail;
      }
   success = 1;
   fail:
   ip->CloseCurRenderer(); 
   ip->SetUseEnvironmentMap(saveUseEnvMap);

   bm->DeleteThis();
   node->Hide(wasHid);
   if (success) {

        LPDIRECT3DCUBETEXTURE9 pcubetex;
        DWORD m_dwCubeMapFlags = DDS_CUBEMAP_ALLFACES;
      D3DFORMAT m_fmt = D3DFMT_A8R8G8B8;
        HRESULT hr = GetDevice()->CreateCubeTexture(256, 1, 
            0, m_fmt, D3DPOOL_MANAGED, &pcubetex,NULL);

        if (FAILED(hr))
        {
         assert(0);
            return ;
        }

    
      LPDIRECT3DSURFACE9 psurfOrig = NULL;

      TSTR fullname;

      GetOutFileName(fullname, fname, VIEW_LF);
      hr = ((LPDIRECT3DCUBETEXTURE9)pcubetex)->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_X, 0, &psurfOrig);
      hr = D3DXLoadSurfaceFromFile(psurfOrig, NULL, NULL, fullname, NULL, D3DX_FILTER_TRIANGLE, 0, NULL);
      //    if (m_numMips > 1)
      //        hr = D3DXFilterCubeTexture((LPDIRECT3DCUBETEXTURE8)m_ptexOrig, NULL, 0, D3DX_FILTER_TRIANGLE);
      ReleasePpo(&psurfOrig);


      GetOutFileName(fullname, fname, VIEW_RT);
      hr = ((LPDIRECT3DCUBETEXTURE9)pcubetex)->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_X, 0, &psurfOrig);
      hr = D3DXLoadSurfaceFromFile(psurfOrig, NULL, NULL, fullname, NULL, D3DX_FILTER_TRIANGLE, 0, NULL);
      ReleasePpo(&psurfOrig);

      GetOutFileName(fullname, fname, VIEW_DN);
      hr = ((LPDIRECT3DCUBETEXTURE9)pcubetex)->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_Y, 0, &psurfOrig);
      hr = D3DXLoadSurfaceFromFile(psurfOrig, NULL, NULL, fullname, NULL, D3DX_FILTER_TRIANGLE, 0, NULL);
      ReleasePpo(&psurfOrig);

      GetOutFileName(fullname, fname, VIEW_UP);
      hr = ((LPDIRECT3DCUBETEXTURE9)pcubetex)->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_Y, 0, &psurfOrig);
      hr = D3DXLoadSurfaceFromFile(psurfOrig, NULL, NULL, fullname, NULL, D3DX_FILTER_TRIANGLE, 0, NULL);
      ReleasePpo(&psurfOrig);

      GetOutFileName(fullname, fname, VIEW_FR);
      hr = ((LPDIRECT3DCUBETEXTURE9)pcubetex)->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_Z, 0, &psurfOrig);
      hr = D3DXLoadSurfaceFromFile(psurfOrig, NULL, NULL, fullname, NULL, D3DX_FILTER_TRIANGLE, 0, NULL);
      ReleasePpo(&psurfOrig);

      GetOutFileName(fullname, fname, VIEW_BK);
      hr = ((LPDIRECT3DCUBETEXTURE9)pcubetex)->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_Z, 0, &psurfOrig);
      hr = D3DXLoadSurfaceFromFile(psurfOrig, NULL, NULL, fullname, NULL, D3DX_FILTER_TRIANGLE, 0, NULL);
      ReleasePpo(&psurfOrig);

		MaxSDK::Util::Path outputPath(fname);
		outputPath.ConvertToAbsolute();
      if( FAILED( D3DXSaveTextureToFile( outputPath.GetString(), D3DXIFF_DDS, pcubetex, NULL ) ) )
      {
         assert(0);
         return;
      }

      ReleasePpo(&pcubetex);

      PBBitmap Bitmap;
	  Bitmap.bi.SetAsset(originalAsset);
      Bitmap.bm = NULL;

      m_PBlock->SetValue(REFLECTION_FILE,m_VS->m_T,&Bitmap);
      m_Maps[CHANNEL_REFLECTION]->SetText(fname);

   }
}

static BOOL
file_exists(const TCHAR *filename)
{
     HANDLE findhandle;
     WIN32_FIND_DATA file;
     findhandle = FindFirstFile(filename, &file);
     FindClose(findhandle);
     if (findhandle == INVALID_HANDLE_VALUE)
        return FALSE;
     else
        return TRUE;
}

bool MaxShader::BrowseOutFile(TSTR &file) {
	BOOL silent = TheManager->SetSilentMode(TRUE);
	BitmapInfo biOutFile;
	FilterList fl;

	OPENFILENAME ofn;
	// get OPENFILENAME items
	HWND   hWnd = GetCOREInterface()->GetMAXHWnd();
	TCHAR  file_name[MAX_PATH] = _T("");
	TSTR cap_str = GetString(IDS_SELECT_CUBIC);

	fl.Append(GetString(IDS_DDS_FILTER_NAME));
	fl.Append(GetString(IDS_DDS_FILTER_DESC));

	// setup OPENFILENAME
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = DLGetWindowInstance(hWnd);
	ofn.lpstrFile = file_name;
	ofn.nMaxFile = _countof(file_name);
	ofn.lpstrFilter = fl;
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle   = cap_str;
	ofn.lpstrFileTitle = _T("");
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = _T("");
	ofn.Flags = OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_HIDEREADONLY ;


	// throw up the dialog
	while (GetSaveFileName(&ofn))
	{
		const TCHAR* p = NULL;
		DWORD i;
		TSTR fileName = ofn.lpstrFile;
		TCHAR type[MAX_PATH];
		BMMSplitFilename(ofn.lpstrFile, NULL, NULL, type);
		if (type[0] == 0 || _tcschr(type, _T('\\')) != NULL)  // no suffix, add from filter types if not *.*
		{
			for (i = 1, p = ofn.lpstrFilter; i < ofn.nFilterIndex; i++)
			{
				while (*p) p++; p++; while (*p) p++; p++; // skip to indexed type
			}
			while (*p) p++; p++; p++; // jump descripter, '*'
			if (*p == _T('.') && p[1] != _T('*')) // add if .xyz
				fileName.Append(p);
		}

		if (file_exists(fileName))
		{
			// file already exists, query owverwrite
			TSTR buf;
			MessageBeep(MB_ICONEXCLAMATION);
			buf.printf(_T("%s %s"),fileName, GetString(IDS_FILE_REPLACE));
			if (MessageBox(hWnd, buf, GetString(IDS_SAVE_AS), MB_ICONQUESTION | MB_YESNO) == IDNO)
			{
				// don't overwrite, remove the path from the name, try again
				buf = file_name + ofn.nFileOffset;
				_tcscpy(file_name, buf);
				continue;
			}
		}

		MaxSDK::Util::Path path(fileName);
		IPathConfigMgr::GetPathConfigMgr()->NormalizePathAccordingToSettings(path);
		file = path.GetString();
		return true;
		break;
	}
	return false;

}

//lets really try to keep max happy...

void MaxVertexShader::ResetDXStates()
{

   m_Device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
   m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
   m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
   m_Device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
   m_Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
   m_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
   m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
   m_Device->SetRenderState(D3DRS_FOGENABLE, FALSE);
   m_Device->SetRenderState(D3DRS_FOGCOLOR, 0xFF0000);
   m_Device->SetRenderState(D3DRS_FOGTABLEMODE,   D3DFOG_NONE );
   m_Device->SetRenderState(D3DRS_FOGVERTEXMODE,  D3DFOG_LINEAR );
   m_Device->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE );
   m_Device->SetRenderState(D3DRS_ZENABLE, TRUE);
   m_Device->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
   m_Device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
   m_Device->SetRenderState(D3DRS_DITHERENABLE, TRUE);
   m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
   m_Device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
   m_Device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
   m_Device->SetRenderState(D3DRS_CLIPPING, TRUE);
   m_Device->SetRenderState(D3DRS_LIGHTING, FALSE);
   m_Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
   m_Device->SetRenderState(D3DRS_COLORVERTEX, FALSE);

   for(int i=0;i<8;i++)
   {
      m_Device->SetTextureStageState(i, D3DTSS_COLOROP, D3DTOP_DISABLE);
      m_Device->SetTextureStageState(i, D3DTSS_COLORARG1, D3DTA_TEXTURE);
      m_Device->SetTextureStageState(i, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
      m_Device->SetTextureStageState(i, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
      m_Device->SetTextureStageState(i, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
      m_Device->SetTextureStageState(i, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
      m_Device->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      m_Device->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      m_Device->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
      m_Device->SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i);
      m_Device->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
      m_Device->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
      m_Device->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, 0);

   }

}


// when we have no textures use the diffuse
void MaxShader::SetDXData(IHardwareMaterial * pHWMtl, Mtl * pMtl)
{

   TimeValue t = GetCOREInterface()->GetTime();
// m_VS->CreateTextures();

   // numloaded = 1 What = 2 specifies that the specular has been enabled with no textures loaded...
   int numLoaded,what;
   numLoaded = m_VS->m_Material->NumLoaded(what);

   if(numLoaded==0 || (numLoaded == 1 && what == 2) || m_VS->badDevice)
   {
      Color c;
      m_PBlock->GetValue(COLOR_AMBIENT, t, c,FOREVER);
      pHWMtl->SetNumTexStages(0);
      pHWMtl->SetAmbientColor(c);
      m_PBlock->GetValue(COLOR_DIFFUSE, t, c,FOREVER);
      pHWMtl->SetDiffuseColor(c);
         
   }
}

class MaxShaderAccessor : public IAssetAccessor {
public:

   MaxShaderAccessor(IParamBlock2* aParamBlock, ParamID aId) : mParamBlock(aParamBlock), mID(aId) {}

   virtual MaxSDK::AssetManagement::AssetType GetAssetType() const { return MaxSDK::AssetManagement::kBitmapAsset; }

	// path accessor functions
	virtual MaxSDK::AssetManagement::AssetUser GetAsset() const;
	virtual bool SetAsset(const MaxSDK::AssetManagement::AssetUser& aNewAsset);

protected:
   IParamBlock2* mParamBlock;
	ParamID mID;
};

MaxSDK::AssetManagement::AssetUser MaxShaderAccessor::GetAsset() const {
   PBBitmap * bm;
   mParamBlock->GetValue(mID,0,bm,FOREVER);
   if(bm)   {
      return bm->bi.GetAsset();
   }
	else {
		return MaxSDK::AssetManagement::AssetUser();
	}
}

bool MaxShaderAccessor::SetAsset(const MaxSDK::AssetManagement::AssetUser& aNewAsset) {
   PBBitmap bm;
	bm.bi.SetAsset(aNewAsset);
   bm.bm = NULL;
   mParamBlock->SetValue(mID,0,&bm);
	return true;
}

// LAM - 4/21/03 - added
//! [Animatable.EnumAuxFiles Example]
void MaxShader::EnumAuxFiles(AssetEnumCallback& assetEnum, DWORD flags) 
{
	if ((flags & FILE_ENUM_CHECK_AWORK1) && TestAFlag(A_WORK1))
		return; 
	if (!(flags & FILE_ENUM_INACTIVE))
		return; // not needed by renderer

	PBBitmap* bm = NULL;
	for(int i=0; i < CHANNEL_MAX; i++)
	{
		ParamID pid = DIFFUSE_FILE + i;
		if (m_PBlock->GetParamDef(pid).type == TYPE_BITMAP)
		{
			ParamID pid = m_PBlock->IndextoID(i);
			m_PBlock->GetValue(pid,0,bm,FOREVER);
			if(bm)
			{
				if(flags & FILE_ENUM_ACCESSOR_INTERFACE)
				{
					IEnumAuxAssetsCallback* callback = static_cast<IEnumAuxAssetsCallback*>(&assetEnum);
					callback->DeclareAsset(MaxShaderAccessor(m_PBlock, DIFFUSE_FILE + i));
				}
				else
				{
					bm->bi.EnumAuxFiles(assetEnum,flags);
				}  
			}
		}
	}
	ReferenceTarget::EnumAuxFiles( assetEnum, flags );
}
//! [Animatable.EnumAuxFiles Example]
