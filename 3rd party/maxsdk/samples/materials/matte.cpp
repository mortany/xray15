 /**********************************************************************
 *<
   FILE: matte.cpp

   DESCRIPTION:  A Matte material

   CREATED BY: Dan Silva

   HISTORY:  1/13/98 Updated to Param2 by Peter Watje

 *>   Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "mtlhdr.h"
#include "mtlres.h"
#include "mtlresOverride.h"
#include "notify.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "stdmat.h" // standard texmap ids
#include "shaders.h" // HAS_MATTE_MTL
#include "buildver.h"
#include "toneop.h"

#include "3dsmaxport.h"

extern HINSTANCE hInstance;

static Class_ID matteClassID(MATTE_CLASS_ID,0);

static const TCHAR* shadowIllumOutStr = _T("shadowIllumOut");

#define PB_REF    0
#define REFLMAP_REF     1

enum { matte_params, };  // pblock ID
// grad_params param IDs
enum 
{ 
   matte_opaque_alpha,
   matte_apply_atmosphere, matte_atmosphere_depth,
   matte_receive_shadows, matte_affect_alpha,
   matte_shadow_brightness, matte_color,
   matte_reflection_amount, matte_reflection_map,
   matte_use_reflection_map, matte_additive_reflection
};

static Color black(0,0,0);
static Color white(1,1,1);

class Matte : public Mtl, public IReshading
{  
   public:
      IParamBlock2 *pblock;   // ref #0
      Texmap *reflmap;  // ref #1
      IParamBlock *savepb;   // for saving pblock when saving Max R2 files
      Interval ivalid;
      ReshadeRequirements mReshadeRQ; // mjm - 06.02.00
      float amblev,reflAmt;
      Color col;
      BOOL fogBG;
      BOOL useReflMap;
      BOOL shadowBG;
      BOOL opaque;
      BOOL shadowAlpha;
      BOOL fogObjDepth;
      BOOL additiveReflection;
      int version;

      Matte(BOOL loading);
      ~Matte();
      void NotifyChanged() {NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);}
      void SetAmbLev(float v, TimeValue t);
      void SetReflAmt(float v, TimeValue t);
      void SetShadColor(Color c, TimeValue t){  col= c;  pblock->SetValue(matte_color, t, c); }
               
   
      // From MtlBase and Mtl
      void SetAmbient(Color c, TimeValue t) {}     
      void SetDiffuse(Color c, TimeValue t) {}     
      void SetSpecular(Color c, TimeValue t) {}
      void SetShininess(float v, TimeValue t) {}            
      
      Color GetAmbient(int mtlNum=0, BOOL backFace=FALSE) { return black; }
       Color GetDiffuse(int mtlNum=0, BOOL backFace=FALSE) { return white; }
      Color GetSpecular(int mtlNum=0, BOOL backFace=FALSE) { return black; }
      float GetXParency(int mtlNum=0, BOOL backFace=FALSE){ return 0.0f; }
      float GetShininess(int mtlNum=0, BOOL backFace=FALSE){ return 0.0f; }      
      float GetShinStr(int mtlNum=0, BOOL backFace=FALSE){ return 1.0f; }
            
      void EnableStuff();
      ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp);
      
      void Shade(ShadeContext& sc);
      void Update(TimeValue t, Interval& valid);
      void Init();
      void Reset();
      Interval Validity(TimeValue t);
      
      Class_ID ClassID() {return matteClassID; }
      SClass_ID SuperClassID() {return MATERIAL_CLASS_ID;}
      void GetClassName(TSTR& s) {s=GetString(IDS_DS_MATTE_SHADOW);}  

      void DeleteThis() {delete this;} 

      ULONG LocalRequirements(int subMtlNum) {  
#if 1
         ULONG flags  =  fogBG  ?  ( fogObjDepth ? 0 : MTLREQ_NOATMOS )  :  MTLREQ_NOATMOS;

         // > 10/9/02 - 2:38pm --MQM-- 
         // if the tone operator is active, we need to match it's
         // "process background" flag for our material requirements.  
         // otherwise we will have a matte-plane that gets tone-op'd 
         // hovering over a background plane that doesn't.
         ToneOperatorInterface* toneOpInterface = static_cast<ToneOperatorInterface*>( GetCOREInterface(TONE_OPERATOR_INTERFACE) );
         if ( toneOpInterface )
         {
            ToneOperator *pToneOp = toneOpInterface->GetToneOperator();
            if ( pToneOp )
            {
               if ( !pToneOp->GetProcessBackground() )
                  flags |= MTLREQ_NOEXPOSURE;   // <-- new MTLREQ flag to disable toneop
            }
         }
         
         return flags;
#else
         return fogBG  ?  ( fogObjDepth ? 0 : MTLREQ_NOATMOS )  :  MTLREQ_NOATMOS;
#endif
         }


      // Methods to access texture maps of material
      int NumSubTexmaps() { return 1; }
      Texmap* GetSubTexmap(int i) { return reflmap; }
      void SetSubTexmap(int i, Texmap *m) {
         ReplaceReference(REFLMAP_REF,m);
//       if (dlg) dlg->UpdateSubTexNames();
         mReshadeRQ = RR_NeedPreshade;
//       NotifyChanged();
         }
      TSTR GetSubTexmapSlotName(int i) {
         return GetString(IDS_DS_MAP); 
         }
      int NumSubs() {return 2;} 
      Animatable* SubAnim(int i);
      TSTR SubAnimName(int i);
      int SubNumToRefNum(int subNum) {return subNum;}

      // From ref
      int NumRefs() {return GetSavingVersion()==2000?1:2;}
      RefTargetHandle GetReference(int i);
private:
      virtual void SetReference(int i, RefTargetHandle rtarg);
public:

      IOResult Save(ISave *isave); 
      IOResult Load(ILoad *iload);
       
      RefTargetHandle Clone(RemapDir &remap);
      RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, 
         PartID& partID, RefMessage message, BOOL propagate);

// JBW: direct ParamBlock access is added
      int   NumParamBlocks() { return 1; }               // return number of ParamBlocks in this instance
      IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
      IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

// begin - ke/mjm - 03.16.00 - merge reshading code
      BOOL SupportsRenderElements(){ return TRUE; }
//    BOOL SupportsReShading(ShadeContext& sc);
      ReshadeRequirements GetReshadeRequirements() { return mReshadeRQ; } // mjm - 06.02.00
      void PreShade(ShadeContext& sc, IReshadeFragment* pFrag);
      void PostShade(ShadeContext& sc, IReshadeFragment* pFrag, int& nextTexIndex, IllumParams* ip);
// end - ke/mjm - 03.16.00 - merge reshading code

      bool IsOutputConst( ShadeContext& sc, int stdID );
      bool EvalColorStdChannel( ShadeContext& sc, int stdID, Color& outClr);
      bool EvalMonoStdChannel( ShadeContext& sc, int stdID, float& outVal);

      void* GetInterface(ULONG id);

   };

class MatteClassDesc:public ClassDesc2 {
   public:
   int         IsPublic() {return TRUE;}
   void *         Create(BOOL loading) {return new Matte(loading);}
   const TCHAR *  ClassName() {return GetString(IDS_DS_MATTE_SHADOW_CDESC); } // mjm - 2.3.99
   SClass_ID      SuperClassID() {return MATERIAL_CLASS_ID;}
   Class_ID       ClassID() {return matteClassID;}
   const TCHAR*   Category() {return _T("");}
// JBW: new descriptor data accessors added.  Note that the 
//      internal name is hardwired since it must not be localized.
   const TCHAR*   InternalName() { return _T("Matte"); } // returns fixed parsable name (scripter-visible name)
   HINSTANCE      HInstance() { return hInstance; }         // returns owning module handle
   };
static MatteClassDesc matteCD;
ClassDesc* GetMatteDesc() {return &matteCD;}

// shader rollout dialog proc
class MatteDlgProc : public ParamMap2UserDlgProc 
   {
   public:
      INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) { return FALSE; }
      void SetThing(ReferenceTarget *m) { 
         Matte *mtl = (Matte *)m;
         if (mtl) mtl->EnableStuff();
         }
      void DeleteThis() { }
   };

static MatteDlgProc matteDlgProc;

class MattePBAccessor : public PBAccessor
{
public:
   void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
   {
      Matte* p = (Matte*)owner;
      if (p!=NULL) {
         IParamMap2 *map = p->pblock->GetMap();
         if (map) {
            switch (id) {
               case matte_opaque_alpha:
                  map->Enable(matte_affect_alpha, v.i? FALSE: TRUE);
                  break;
               case matte_reflection_map:
                     map->Enable(matte_reflection_amount, v.r?TRUE: FALSE);
                  break;
               }
            }
         }

   }
};

static MattePBAccessor matte_accessor;

static ParamBlockDesc2 matte_param_blk ( matte_params, _T("parameters"),  0, &matteCD, P_AUTO_CONSTRUCT + P_AUTO_UI, PB_REF, 
   //rollout
   IDD_MATTE, IDS_DS_MATTE_SHADOW_PAR, 0, 0, &matteDlgProc, 
   // params
   matte_opaque_alpha,  _T("opaqueAlpha"), TYPE_BOOL,       0,          IDS_PW_OALPHA,
      p_default,     FALSE,
      p_ui,       TYPE_SINGLECHECKBOX, IDC_MATTE_OPAQUE,
      p_accessor,    &matte_accessor,
      p_end,

   matte_apply_atmosphere, _T("applyAtmosphere"), TYPE_BOOL,         0,          IDS_PW_APPLYATMOS,
      p_default,     FALSE,
      p_ui,       TYPE_SINGLECHECKBOX, IDC_MATTE_FOGBG,
      p_end,
   matte_atmosphere_depth, _T("atmosphereDepth"), TYPE_INT,          0,          IDS_PW_ATMOSDEPTH,
      p_default,     0,
      p_range,    0, 1,
      p_ui,       TYPE_RADIO, 2, IDC_MATTE_FOG_BGDEPTH, IDC_MATTE_FOG_OBJDEPTH,
      p_end,
   matte_receive_shadows,  _T("receiveShadows"), TYPE_BOOL,       0,          IDS_PW_RECEIVESHADOWS,
      p_default,     TRUE,
      p_ui,       TYPE_SINGLECHECKBOX, IDC_MATTE_SHADOW,
      p_end,
   matte_affect_alpha,  _T("affectAlpha"), TYPE_BOOL,       0,          IDS_PW_AFFECTALPHA,
      p_default,     TRUE,
      p_ui,       TYPE_SINGLECHECKBOX, IDC_MATTE_SHADALPHA,
      p_enabled,     FALSE,
      p_end,
   matte_shadow_brightness,   _T("shadowBrightness"), TYPE_FLOAT,       P_ANIMATABLE,  IDS_DS_SHADOW_BRITE,
      p_default,     0.0,
      p_range,    0.0, 1.0,
      p_ui,          TYPE_SPINNER, EDITTYPE_FLOAT, IDC_MATTE_AMB_EDIT, IDC_MATTE_AMB_SPIN, 0.1, 
      p_end, 
   matte_color, _T("color"),  TYPE_RGBA,           P_ANIMATABLE,  IDS_DS_SHAD_COLOR,   
      p_default,     Color(0,0,0), 
      p_ui,       TYPE_COLORSWATCH, IDC_SHAD_COLOR, 
      p_end,
   matte_reflection_amount,   _T("amount"), TYPE_PCNT_FRAC, P_ANIMATABLE,  IDS_DS_REFLAMT,
      p_default,     0.5,
      p_range,    0.0, 100.0,
      p_ui,          TYPE_SPINNER, EDITTYPE_FLOAT, IDC_MATTE_REFLAMT_EDIT, IDC_MATTE_REFLAMT_SPIN, 0.1, 

      p_end,  
   matte_reflection_map, _T("map"),    TYPE_TEXMAP,         P_OWNERS_REF,  IDS_DS_MAP,
      p_refno,    REFLMAP_REF,
      p_subtexno,    0,    
      p_ui,       TYPE_TEXMAPBUTTON, IDC_MATTE_REFL_MAP,
      p_accessor,    &matte_accessor,

      p_end,
   matte_use_reflection_map,  _T("useReflMap"), TYPE_BOOL,     0,  IDS_DS_USEREFL,
      p_default,     TRUE,
      p_ui,       TYPE_SINGLECHECKBOX, IDC_MATTE_USEREFL,
      p_end,
   matte_additive_reflection, _T("additiveReflection"), TYPE_INT,       0,  IDS_DS_USEREFL,
      p_default,     TRUE,
      p_ui,       TYPE_SINGLECHECKBOX, IDC_MATTE_ADDITIVE_REFL,
      p_end,


   p_end
   );

//--- Matte Material -------------------------------------------------


static ParamBlockDescID pbdesc0[] = {
   {TYPE_FLOAT, NULL, TRUE,matte_shadow_brightness }};   // ambient light level


static ParamBlockDescID pbdesc1[] = {
   {TYPE_FLOAT, NULL, TRUE,matte_shadow_brightness },
   {TYPE_RGBA, NULL, TRUE ,matte_color }
   };   

static ParamBlockDescID pbdesc[] = {
   {TYPE_FLOAT, NULL, TRUE,matte_shadow_brightness },
   {TYPE_RGBA,  NULL, TRUE,matte_color },
   {TYPE_FLOAT, NULL, TRUE,matte_reflection_amount }
   };   

#define MATTE_VERSION 3
#define NUM_OLDVERSIONS 3
#define NPARAMS 3

// Array of old versions
static ParamVersionDesc versions[NUM_OLDVERSIONS] = {
   ParamVersionDesc(pbdesc0,1,0),   
   ParamVersionDesc(pbdesc1,2,1),
   ParamVersionDesc(pbdesc,3,2)  
   };


//static ParamVersionDesc curVersion(pbdesc,NPARAMS,MATTE_VERSION);

// Code for saving Max R2 files -------------------
/*
static void NotifyPreSaveOld(void *param, NotifyInfo *info) {
   Matte *mt = (Matte *)param;
   if (GetSavingVersion()==2000) {
      mt->savepb = mt->pblock;
      mt->pblock = UpdateParameterBlock(pbdesc,3,mt->savepb,pbdesc1,2,1);
      }
   }

static void NotifyPostSaveOld(void *param, NotifyInfo *info) {
   Matte *mt = (Matte *)param;
   if (mt->savepb) {
      mt->pblock->DeleteThis();
      mt->pblock = mt->savepb;
      mt->savepb = NULL;
      }
   }
   */
//----------------------------------------------------------------

Matte::Matte(BOOL loading) : mReshadeRQ(RR_None) // mjm - 06.02.00
   {  
// dlg = NULL;
   pblock = NULL;
   savepb = NULL;  // for saving pblock when saving Max R2 files
   reflmap = NULL;
   ivalid.SetEmpty();
   fogBG = FALSE;    // DS 11/9/96
   shadowBG = FALSE;
   shadowAlpha = FALSE;
   opaque = FALSE;
   fogObjDepth = FALSE;
   additiveReflection = FALSE;
// RegisterNotification(NotifyPreSaveOld, (void *)this, NOTIFY_FILE_PRE_SAVE_OLD);
// RegisterNotification(NotifyPostSaveOld, (void *)this, NOTIFY_FILE_POST_SAVE_OLD);

   matteCD.MakeAutoParamBlocks(this);  // make and intialize paramblock2
   Init();
   }

Matte::~Matte() {
//  UnRegisterNotification(NotifyPreSaveOld, (void *)this, NOTIFY_FILE_PRE_SAVE_OLD);
// InRegisterNotification(NotifyPostSaveOld, (void *)this, NOTIFY_FILE_POST_SAVE_OLD);
   }

void Matte::Init()
   {
   SetAmbLev(0.0f,0);
   SetShadColor(Color(0.0f,0.0f,0.0f), TimeValue(0));
   fogBG = FALSE;
   shadowBG = FALSE;
   shadowAlpha = FALSE;
   opaque = TRUE;
   fogObjDepth = FALSE;
   additiveReflection = TRUE;
   }
      
void Matte::Reset()
   {
   matteCD.Reset(this, TRUE); // reset all pb2's
   Init();
   }
   
void* Matte::GetInterface(ULONG id)
{
   if( id == IID_IReshading )
      return (IReshading*)( this );
   else
      return Mtl::GetInterface(id);
}
   
void Matte::SetAmbLev(float v, TimeValue t) {
    amblev= v;
   pblock->SetValue( matte_shadow_brightness, t, v);
   }

void Matte::SetReflAmt(float v, TimeValue t) {
    reflAmt= v;
   pblock->SetValue( matte_reflection_amount, t, v);
   }


void Matte::EnableStuff() {
   if (pblock) {
      IParamMap2 *map = pblock->GetMap();
      if (map) {
         map->Enable(matte_reflection_amount, reflmap?TRUE:FALSE);
         map->Enable(matte_affect_alpha, opaque?FALSE:TRUE);
         }
      }
   }

ParamDlg* Matte::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
   {
   // create the rollout dialogs
   IAutoMParamDlg* masterDlg = matteCD.CreateParamDlgs(hwMtlEdit, imp, this);

   EnableStuff();

   return masterDlg;

   }

static Color blackCol(0.0f,0.0f,0.0f);

#define CLAMP(x) (((x)>1.0f)?1.0f:((x)<0.0f)?0.0f:(x))

static inline void Clamp(Color &c) {
   c.r = CLAMP(c.r);
   c.g = CLAMP(c.g);
   c.b = CLAMP(c.b);
   }

inline void Clamp( float& val ){ 
   if( val < 0.0f ) val = 0.0f;
   else if( val > 1.0f ) val = 1.0f;
}

inline void LBound( Color& c, float min = 0.0f )
{
   if( c.r < min ) c.r = min;
   if( c.g < min ) c.g = min;
   if( c.b < min ) c.b = min;
}

// returns shadow fraction & shadowClr
// move this util to shade context at first opportunity
float IllumShadow( ShadeContext& sc, Color& shadowClr ) 
{ 
   IlluminateComponents illumComp;
   IIlluminationComponents* pIComponents = NULL; 
   Color illumClr(0,0,0);
   Color illumClrNS(0,0,0);
   shadowClr.Black();
   Point3 N = sc.Normal();

   // poke flag to let Light Tracer/Radiosity so they will
   // separate out the shadow value.   
   int oldXID = sc.xshadeID;
   sc.xshadeID |= SHADECONTEXT_GUESS_SHADOWS_FLAG;
   
   for (int i = 0; i < sc.nLights; i++) {
      LightDesc* l = sc.Light( i );
      pIComponents = (IIlluminationComponents*)l->GetInterface( IID_IIlluminationComponents );
      if( pIComponents ){
         // use component wise illuminate routines
         if (!pIComponents->Illuminate( sc, N, illumComp ))
            continue;

         LBound( illumComp.shadowColor );
         illumClr += (illumComp.finalColor - illumComp.shadowColor ) * illumComp.geometricAtten;
         illumClrNS += illumComp.finalColorNS * illumComp.geometricAtten;
         if( illumComp.rawColor != illumComp.filteredColor ){
            // light is filtered by a transparent object, sum both filter & user shadow color
            shadowClr += illumComp.finalColor * illumComp.geometricAtten; //attenuated filterColor 
         } else {
            // no transparency, sum in just the shadow color
            shadowClr += illumComp.shadowColor * illumComp.geometricAtten;
         }

      } else {
         // no component interface, shadow clr is black
         Color lightCol;
         Point3 L;
         register float NL, diffCoef;
         if (!l->Illuminate(sc, N, lightCol, L, NL, diffCoef))
            continue;
         if (diffCoef <= 0.0f)     
            continue;
         illumClr += diffCoef * lightCol;

         if( sc.shadow ){
            sc.shadow = FALSE;
            l->Illuminate(sc, N, lightCol, L, NL, diffCoef);
            illumClrNS += diffCoef * lightCol;
            sc.shadow = TRUE;
         } else {
            illumClrNS = illumClr;
         }
      }
   }// end, for each light

   // > 9/24/02 - 2:31pm --MQM-- 
   // restore xshadeID
   sc.xshadeID = oldXID;

   float intensNS = Intens(illumClrNS);
   float intens = Intens(illumClr);
   float atten = (intensNS > 0.01f)? intens/intensNS : 1.0f;
   if (atten > 1.0f)
      atten = 1.0f/atten;

   return atten;
}

static BOOL  gUseLocalShadowClr = FALSE;

#ifdef _DEBUG
static int stopX = -1;
static int stopY = -1;
#endif

// if this function changes, please also check SupportsReShading, PreShade and PostShade
// end - ke/mjm - 03.16.00 - merge reshading code
// [attilas|29.5.2000] if this function changes, please also check EvalColorStdChannel
void Matte::Shade(ShadeContext& sc)
{
   Color c,t, shadowClr;
   float atten;
   float reflA;

   // > 6/15/02 - 11:12am --MQM-- 
   // for renderer prepass, we need to at least call
   // illuminate so that Light Tracer can cache shading
   if ( SHADECONTEXT_IS_PREPASS( sc ) )
   {
      Color    lightCol;
      Point3   L;
      float    NL = 0.0f, diffCoef = 0.0f;
      LightDesc *l = NULL;
      for ( int i = 0;  i < sc.nLights;  i++ )
      {
         l = sc.Light( i );
         if ( NULL != l )
            l->Illuminate( sc, sc.Normal(), lightCol, L, NL, diffCoef );
      }
      return;
   }

#ifdef _DEBUG
   IPoint2 sp = sc.ScreenCoord();
   if ( sp.x == stopX && sp.y == stopY )
      sp.x = stopX;
#endif

   if (gbufID) sc.SetGBufferID(gbufID);
   IllumParams ip( 1, &shadowIllumOutStr);
   IllumParams ipNS(0, NULL);
   ip.ClearInputs(); ip.ClearOutputs();
   ipNS.ClearInputs(); ipNS.ClearOutputs();
   ip.hasComponents = ipNS.hasComponents = HAS_MATTE_MTL; 
   
   // get background color & transparency
   if (!opaque) sc.Execute(0x1000); // DS: 6/24/99:use black bg when AA filtering (#192348)
   sc.GetBGColor(c, t, fogBG&&(!fogObjDepth) );
   if (!opaque) sc.Execute(0x1001); // DS: 6/24/99:use black bg when AA filtering (#192348)

   if (shadowBG && sc.shadow) {
      /********
      sc.shadow = 0;
      Color col0 = sc.DiffuseIllum();
      sc.shadow = 1;
      Color scol = sc.DiffuseIllum();
      float f = Intens(col0);
      atten = (f>0.0f)?Intens(scol)/f:1.0f;
      if (atten>1.0f) atten = 1.0f/atten;
      ********/
      atten = IllumShadow( sc, shadowClr );
      atten = amblev + (1.0f-amblev) * atten; 

      // key on black user-set shadow clr
      if( gUseLocalShadowClr || col.r != 0.0f || col.g != 0.0f || col.b != 0.0f )
            shadowClr = col;

      ipNS.finalC = ipNS.diffIllumOut = c;
      c *= atten;
      ip.diffIllumOut = c;

      shadowClr *= 1.0f - atten;
      ip.finalC = sc.out.c = c + shadowClr;

      ip.SetUserIllumOutput( 0, shadowClr );

      if (shadowAlpha)
         t *= atten;
   } else {
      sc.out.c  = 
      ipNS.finalC = ipNS.diffIllumOut = ip.finalC = ip.diffIllumOut = c;
   }


   // add the reflections
   if (reflmap && useReflMap) {
      AColor rcol;
      if (reflmap->HandleOwnViewPerturb()) {
         sc.TossCache(reflmap);
         rcol = reflmap->EvalColor(sc);
      } else 
         rcol = sc.EvalEnvironMap(reflmap, sc.ReflectVector());

      Color rc;
      rc = Color(rcol.r,rcol.g,rcol.b)*reflAmt;
      ip.reflIllumOut = ipNS.reflIllumOut = rc;

      if( additiveReflection ) {
         // additive compositing of reflections
         sc.out.c += rc; ip.finalC += rc; ipNS.finalC += rc;
      } else {
         reflA = Intens( rc );
         // over compositing of reflections 
         sc.out.c = (1.0f - reflA) * sc.out.c + rc; 
         ip.finalC = (1.0f - reflA) * ip.finalC + rc;
         ipNS.finalC = (1.0f - reflA) * ipNS.finalC + rc;
      }
   }

   // render elements
   Clamp( t );
   Clamp( reflA );
   ip.finalT = ipNS.finalT = sc.out.t = opaque ? black: additiveReflection? t : Color(reflA,reflA,reflA) ; 
   int nEles = sc.NRenderElements();
   if( nEles != 0 ){
      ip.pShader = ipNS.pShader = NULL; // no shader on matte mtl
      ip.stdIDToChannel = ipNS.stdIDToChannel = NULL;
      ip.pMtl = ipNS.pMtl = this;
      ip.finalAttenuation = ipNS.finalAttenuation = 1.0f;

      for( int i=0; i < nEles; ++i ){
         IRenderElement* pEle = sc.GetRenderElement(i);
         if( pEle->IsEnabled() ){
            MaxRenderElement* pMaxEle = (MaxRenderElement*)pEle->GetInterface( MaxRenderElement::IID );
            if( pEle->ShadowsApplied() )
               pMaxEle->PostIllum( sc, ip );
            else
               pMaxEle->PostIllum( sc, ipNS );
         }
      }
   }

}


void Matte::PreShade(ShadeContext& sc, IReshadeFragment* pFrag)
{
   // save reflection texture
   if ( reflmap ){
      AColor rcol;
      if (reflmap->HandleOwnViewPerturb()) {
         sc.TossCache(reflmap);
         rcol = reflmap->EvalColor(sc);
      } else 
         rcol = sc.EvalEnvironMap(reflmap, sc.ReflectVector());
      pFrag->AddColorChannel( rcol );
   }
}

void Matte::PostShade(ShadeContext& sc, IReshadeFragment* pFrag, int& nextTexIndex, IllumParams*)
{
   Color c,t;
   float atten;

   if (!opaque)
      sc.Execute(0x1000); // DS: 6/24/99:use black bg when AA filtering (#192348)

   sc.GetBGColor(c,t,fogBG && !fogObjDepth);
   if (!opaque)
      sc.Execute(0x1001); // DS: 6/24/99:use black bg when AA filtering (#192348)

   if (shadowBG && sc.shadow)
   {
//    sc.shadow = 0;
//    Color col0 = sc.DiffuseIllum();
//    sc.shadow = 1;
//    Color scol = sc.DiffuseIllum();
//    float f = Intens(col0);
//    atten = (f>0.0f)?Intens(scol)/f:1.0f;
//    if (atten>1.0f) atten = 1.0f/atten;
//    atten = amblev+(1.0f-amblev)*atten;
//    sc.out.c = c*atten + (1.0f-atten)*col;
      Color shadowClr;
      atten = IllumShadow( sc, shadowClr );
      atten = amblev + (1.0f-amblev) * atten;

      // key on black user-set shadow clr
      if( gUseLocalShadowClr || col.r != 0.0f || col.g != 0.0f || col.b != 0.0f )
            shadowClr = col;

      c *= atten;
      shadowClr *= 1.0f - atten;

      sc.out.c = c + shadowClr;

      if (shadowAlpha)
         t *= atten;
   } else
      sc.out.c  = c;

   sc.out.t = opaque ? black : t;  
   Clamp(sc.out.t);

   // code for compositing: -1 for opaque , else -2
   sc.out.ior = (opaque) ? -1.0f : -2.0f;
   if( reflmap ){
      if( useReflMap ){
         Color rc = pFrag->GetColorChannel( nextTexIndex++ );
         sc.out.c += rc * reflAmt;
      } else {
         nextTexIndex++;   // skip
      }
   }
}

void Matte::Update(TimeValue t, Interval& valid)
   {  
   ivalid = FOREVER;
   pblock->GetValue(matte_shadow_brightness,t,amblev,ivalid);
   pblock->GetValue(matte_color,t,col,ivalid);
   pblock->GetValue(matte_reflection_amount,t,reflAmt,ivalid);


   pblock->GetValue(matte_opaque_alpha,t,opaque,ivalid);
   pblock->GetValue(matte_apply_atmosphere,t,fogBG,ivalid);
   pblock->GetValue(matte_receive_shadows,t,shadowBG,ivalid);
   pblock->GetValue(matte_affect_alpha,t,shadowAlpha,ivalid);
   pblock->GetValue(matte_atmosphere_depth,t,fogObjDepth,ivalid);
   pblock->GetValue( matte_use_reflection_map,t, useReflMap, ivalid);
   pblock->GetValue( matte_additive_reflection,t, additiveReflection, ivalid);

   if (reflmap&&useReflMap)
      reflmap->Update(t,ivalid);
   EnableStuff();

   valid &= ivalid;
   }

Interval Matte::Validity(TimeValue t)
   {
   Interval valid = FOREVER;  
   Update(t,valid);
   return valid;
   }

Animatable* Matte::SubAnim(int i)
   {
   switch (i) {
      case 0: return pblock;
      case 1: return reflmap;
      default: return NULL;
      }
   }

TSTR Matte::SubAnimName(int i)
   {
   switch (i) {
      case 0: return GetString(IDS_DS_PARAMETERS);
      case 1: return GetString(IDS_DS_MAP);
      default: return _T("");
      }
   }

RefTargetHandle Matte::GetReference(int i)
   {
   switch (i) {
      case 0: return pblock;
      case 1: return reflmap;
      default: return NULL;
      }
   }
 
void Matte::SetReference(int i, RefTargetHandle rtarg)
   {
   switch (i) {
      case 0: pblock = (IParamBlock2*)rtarg; break;
      case 1: reflmap = (Texmap*)rtarg; 
//       matte_param_blk.InvalidateUI();

         break;
      }
   }

RefTargetHandle Matte::Clone(RemapDir &remap)
   {
   Matte *mtl = new Matte(FALSE);
   *((MtlBase*)mtl) = *((MtlBase*)this);  // copy superclass stuff
   mtl->ReplaceReference(PB_REF,remap.CloneRef(pblock));
   mtl->fogBG = fogBG;
   mtl->shadowBG = shadowBG;
   mtl->shadowAlpha = shadowAlpha;
   mtl->opaque = opaque;
   mtl->fogObjDepth = fogObjDepth;
   mtl->useReflMap = useReflMap;
   if (reflmap)
      mtl->ReplaceReference(REFLMAP_REF,remap.CloneRef(reflmap));
   BaseClone(this, mtl, remap);
   return mtl;
   }

RefResult Matte::NotifyRefChanged(
      const Interval& changeInt, 
      RefTargetHandle hTarget, 
      PartID& partID, 
	  RefMessage message, 
	  BOOL propagate)
   {
   switch (message) {
      case REFMSG_CHANGE:
         if (hTarget == pblock)
            {
            ivalid.SetEmpty();
         // see if this message came from a changing parameter in the pblock,
         // if so, limit rollout update to the changing item and update any active viewport texture
            ParamID changing_param = pblock->LastNotifyParamID();
            matte_param_blk.InvalidateUI(changing_param);
            if( (changing_param == matte_receive_shadows)
               ||(changing_param == matte_shadow_brightness)
               ||(changing_param == matte_color)
               ||(changing_param == matte_reflection_amount)
               ||(changing_param == matte_use_reflection_map))
            {
               mReshadeRQ = RR_NeedReshade;
            } else if (changing_param == -1) 
               mReshadeRQ = RR_NeedPreshade;
             else 
               mReshadeRQ = RR_None;
         }

         if (hTarget != NULL) {

            switch (hTarget->SuperClassID()) {
               case TEXMAP_CLASS_ID: {
                     mReshadeRQ = RR_NeedPreshade;
               } break;
//             default:
//                mReshadeRQ =RR_NeedReshade;
//             break;
            }

         }
         break;

      case REFMSG_SUBANIM_STRUCTURE_CHANGED:
//       if (partID == 0) 
//          mReshadeRQ = RR_None;
//       else {
//          mReshadeRQ = RR_NeedPreshade;
//          NotifyChanged();
//       }
         break;
      
   }
   return REF_SUCCEED;
}


#define MTL_HDR_CHUNK 0x4000
#define FOGBG_CHUNK 0x0001
#define SHADOWBG_CHUNK 0x0002
#define MATTE_VERSION_CHUNK 0003
#define OPAQUE_CHUNK 0x0004
#define FOG_OBJDEPTH_CHUNK 0x0005
#define SHADALPHA_CHUNK 0x0010

IOResult Matte::Save(ISave *isave) { 
   ULONG nb;
   isave->BeginChunk(MTL_HDR_CHUNK);
   IOResult res = MtlBase::Save(isave);
   if (res!=IO_OK) return res;
   isave->EndChunk();
/*
   if (fogBG) {
      isave->BeginChunk(FOGBG_CHUNK);
      isave->EndChunk();
      }
   if (shadowBG) {
      isave->BeginChunk(SHADOWBG_CHUNK);
      isave->EndChunk();
      }
   if (shadowAlpha) {
      isave->BeginChunk(SHADALPHA_CHUNK);
      isave->EndChunk();
      }
   if (opaque) {
      isave->BeginChunk(OPAQUE_CHUNK);
      isave->EndChunk();
      }
   if (fogObjDepth) {
      isave->BeginChunk(FOG_OBJDEPTH_CHUNK);
      isave->EndChunk();
      }
*/
   int vers = 2;

   isave->BeginChunk(MATTE_VERSION_CHUNK);
   isave->Write(&vers, sizeof(vers), &nb);
   isave->EndChunk();

   return IO_OK;
   }

//watje
class MattePostLoadCallback:public  PostLoadCallback
{
public:
   Matte      *s;
   MattePostLoadCallback(Matte *r) {s=r;}
   void proc(ILoad *iload);
};


void MattePostLoadCallback::proc(ILoad *iload)
{

   s->pblock->SetValue(matte_opaque_alpha,0,s->opaque);
   s->pblock->SetValue(matte_apply_atmosphere,0,s->fogBG);
   s->pblock->SetValue(matte_receive_shadows,0,s->shadowBG);
   s->pblock->SetValue(matte_affect_alpha,0,s->shadowAlpha);
   s->pblock->SetValue(matte_atmosphere_depth,0,s->fogObjDepth);

   delete this;

}
 


IOResult Matte::Load(ILoad *iload) { 
   ULONG nb;
   int id;
   version = 0;
   IOResult res;
   
   opaque = FALSE;

   while (IO_OK==(res=iload->OpenChunk())) {
      switch(id = iload->CurChunkID())  {
         case MTL_HDR_CHUNK:
            res = MtlBase::Load(iload);
            ivalid.SetEmpty();
            break;
         case FOGBG_CHUNK:
            fogBG = TRUE;
            break;
         case SHADOWBG_CHUNK:
            shadowBG = TRUE;
            break;
         case SHADALPHA_CHUNK:
            shadowAlpha = TRUE;
            break;
         case OPAQUE_CHUNK:
            opaque = TRUE;
            break;
         case FOG_OBJDEPTH_CHUNK:
            fogObjDepth = TRUE;
            break;
         case MATTE_VERSION_CHUNK:
            iload->Read(&version,sizeof(int), &nb);
            break;
         }
      iload->CloseChunk();
      if (res!=IO_OK) 
         return res;
      }

   if (version  < 2)
      {

   // JBW: register old version ParamBlock to ParamBlock2 converter
      ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &matte_param_blk, this, PB_REF);
      iload->RegisterPostLoadCallback(plcb);

      MattePostLoadCallback* matteplcb = new MattePostLoadCallback(this);
      iload->RegisterPostLoadCallback(matteplcb);
      }
   return IO_OK;
                                                      
   }

//-----------------------------------------------------------------------------


//????????????????????????????????????????????????????????????????????????
//  Only the Reflection channel could not be constant
//
bool Matte::IsOutputConst
( 
   ShadeContext& sc, // describes context of evaluation
   int stdID            // must be ID_AM, ect
)
{
   if ( stdID == ID_RL ) // Reflection (value 9)
   {
         if ( reflmap && 
                useReflMap && 
                reflmap->IsOutputMeaningful(sc) ) 
            return false;
   }
   
   return true;
}

//????????????????????????????????????????????????????????????????????????
// The stdID parameter doesn't really have a meaning in this case.
// 
bool Matte::EvalColorStdChannel
( 
   ShadeContext& sc, // describes context of evaluation
   int stdID,           // must be ID_AM, ect
   Color& outClr        // output var
)
{
   switch ( stdID )
   {
      case ID_BU: // Bump (value 8)
      case ID_RR: // Refraction (value 10)
      case ID_DP: // Displacement (value 11)
      case ID_SI: // Self-illumination (value 5)
      case ID_FI: // Filter color (value 7)
         return false;
         break;

      case ID_RL: // Reflection (value 9)
         if ( sc.doMaps &&
                reflmap && 
                useReflMap && 
                reflmap->IsOutputMeaningful(sc) ) 
         {
            AColor rcol;
            rcol.Black();
            if ( reflmap->HandleOwnViewPerturb() ) 
            {
               sc.TossCache(reflmap);
               rcol = reflmap->EvalColor(sc);
            }
            else 
            {
               rcol = sc.EvalEnvironMap( reflmap, sc.ReflectVector() );
            }
            Color rc;
            rc = Color(rcol.r,rcol.g,rcol.b)*reflAmt;
            outClr += rc;
         }
         else
            return false;

      case ID_AM: // Ambient (value 0)
         outClr = GetAmbient();
         break;
      
      case ID_DI: // Diffuse (value 1)
         outClr = GetDiffuse();
         break;
      
      case ID_SP: // Specular (value 2)
         outClr = GetSpecular();
         break;
      
      case ID_SH: // Shininess (value 3).  In R3 and later this is called Glossiness.
         outClr.r = outClr.g = outClr.b = GetShininess();
         break;

      case ID_SS: // Shininess strength (value 4).  In R3 and later this is called Specular Level.
         outClr.r = outClr.g = outClr.b = GetShinStr();
         break;

      case ID_OP: // Opacity (value 6)
         outClr.r = outClr.g = outClr.b = GetXParency();
         break;

      default:
         // Should never happen
         //DbgAssert( false );
         return false;
         break;
   }
   return true;
}

//????????????????????????????????????????????????????????????????????????
// The stdID parameter doesn't really have a meaning in this case.
// 
bool Matte::EvalMonoStdChannel
( 
   ShadeContext& sc, // describes context of evaluation
   int stdID,           // must be ID_AM, ect
   float& outVal        // output var
)
{
   switch ( stdID )
   {
      case ID_BU: // Bump (value 8)
      case ID_RR: // Refraction (value 10)
      case ID_DP: // Displacement (value 11)
      case ID_SI: // Self-illumination (value 5)
      case ID_FI: // Filter color (value 7)
         return false;
         break;

      case ID_RL: // Reflection (value 9)
         if ( sc.doMaps &&
                reflmap && 
                useReflMap && 
                reflmap->IsOutputMeaningful(sc) ) 
         {
            if ( reflmap->HandleOwnViewPerturb() ) 
            {
               sc.TossCache(reflmap);
               outVal = reflmap->EvalMono(sc);
            }
            else 
            {
               AColor rcol;
               rcol = sc.EvalEnvironMap( reflmap, sc.ReflectVector() );
               Color rc;
               rc = Color(rcol.r,rcol.g,rcol.b)*reflAmt;
               outVal = Intens(rc);
            }
         }
         else
            return false;
      break;

      case ID_AM: // Ambient (value 0)
         outVal = Intens( GetAmbient() );
         break;
      
      case ID_DI: // Diffuse (value 1)
         outVal = Intens( GetDiffuse() );
         break;
      
      case ID_SP: // Specular (value 2)
         outVal = Intens( GetSpecular() );
         break;
      
      case ID_SH: // Shininess (value 3).  In R3 and later this is called Glossiness.
         outVal = GetShininess();
         break;

      case ID_SS: // Shininess strength (value 4).  In R3 and later this is called Specular Level.
         outVal = GetShinStr();
         break;

      case ID_OP: // Opacity (value 6)
         outVal = GetXParency();
         break;

      default:
         // Should never happen
         //DbgAssert( false );
         return false;
         break;
   }
   return true;
}
