/**********************************************************************
 *<
	FILE: camera.cpp

	DESCRIPTION:  A Simple Camera implementation

	CREATED BY: Dan Silva

	HISTORY: created 14 December 1994


	NOTE: 

	    To ensure that the camera has a valid targDist during
	    network rendering, be sure to call:

		UpdateTargDistance( TimeValue t, INode* inode );

		This call should be made prior to cameraObj->EvalWorldState(...)


 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "prim.h"
#include "gencam.h"
#include "camera.h"
#include "target.h"
#include "macrorec.h"
#include "decomp.h"
#include "iparamb2.h"
#include "MouseCursors.h"
#include "Max.h"
#include <maxscript/maxscript.h>
#include "bitmap.h"
#include "guplib.h"
#include "gup.h"

#include "ILinkTMCtrl.h"
#include "3dsmaxport.h"
#include <operationdesc.h>

#include <Graphics/Utilities/MeshEdgeRenderItem.h>
#include <Graphics/Utilities/SplineRenderItem.h>
#include <Graphics/CustomRenderItemHandle.h>
#include <Graphics/RenderNodeHandle.h>
#include <Graphics/IDisplayManager.h>

// Parameter block indices
#define PB_FOV						0
#define PB_TDIST					1
#define PB_HITHER					2
#define PB_YON						3
#define PB_NRANGE					4
#define PB_FRANGE					5
#define PB_MP_EFFECT_ENABLE			6	// mjm - 07.17.00
#define PB_MP_EFF_REND_EFF_PER_PASS	7	// mjm - 07.17.00
#define PB_FOV_TYPE					8	// MC  - 07.17.07

// Depth of Field parameter block indicies
#define PB_DOF_ENABLE	0
#define PB_DOF_FSTOP	1

#define MIN_FSTOP		0.0001f
#define MAX_FSTOP		100.0f

#define WM_SET_TYPE		WM_USER + 0x04002

#define MIN_CLIP	0.000001f
#define MAX_CLIP	1.0e32f

#define NUM_CIRC_PTS	28
#define SEG_INDEX		7

#define RELEASE_SPIN(x)   if (so->x) { ReleaseISpinner(so->x); so->x = NULL;}
#define RELEASE_BUT(x)   if (so->x) { ReleaseICustButton(so->x); so->x = NULL;}

static int waitPostLoad = 0;
static void resetCameraParams();
static HIMAGELIST hCamImages = NULL;

//------------------------------------------------------
class SimpleCamClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new SimpleCamera; }
	const TCHAR *	ClassName() { return GetString(IDS_DB_FREE_CLASS); }
	SClass_ID		SuperClassID() { return CAMERA_CLASS_ID; }
	Class_ID 		ClassID() { return Class_ID(SIMPLE_CAM_CLASS_ID,0); }
	const TCHAR* 	Category() { return _T("");  }
	void			ResetClassParams(BOOL fileReset) { if(fileReset) resetCameraParams(); }
	};

static SimpleCamClassDesc simpleCamDesc;

ClassDesc* GetSimpleCamDesc() { return &simpleCamDesc; }


// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

#define DEF_HITHER_CLIP		1.0f
#define DEF_YON_CLIP		1000.0f

// Class variables of SimpleCamera
Mesh SimpleCamera::mesh;
short SimpleCamera::meshBuilt=0;
float SimpleCamera::dlgFOV = DegToRad(45.0);
int   SimpleCamera::dlgFOVType = FOV_W;
short SimpleCamera::dlgShowCone =0;
short SimpleCamera::dlgShowHorzLine =0;
float SimpleCamera::dlgTDist = FIXED_CONE_DIST;
short SimpleCamera::dlgClip = 0;
float SimpleCamera::dlgHither = DEF_HITHER_CLIP;
float SimpleCamera::dlgYon = DEF_YON_CLIP;
float SimpleCamera::dlgNearRange = 0.0f;
float SimpleCamera::dlgFarRange = 1000.0f;
short SimpleCamera::dlgRangeDisplay = 0;
short SimpleCamera::dlgIsOrtho = 0;

short SimpleCamera::dlgDOFEnable = 1;
float SimpleCamera::dlgDOFFStop = 2.0f;
short SimpleCamera::dlgMultiPassEffectEnable = 0;
short SimpleCamera::dlgMPEffect_REffectPerPass = 0;
Tab<ClassEntry*> SimpleCamera::smCompatibleEffectList;

BOOL SimpleCamera::inCreate=FALSE;

void resetCameraParams() 
{
	SimpleCamera::dlgFOV = DegToRad(45.0);
	SimpleCamera::dlgFOVType = FOV_W;
	SimpleCamera::dlgShowCone =0;
	SimpleCamera::dlgShowHorzLine =0;
	SimpleCamera::dlgTDist = FIXED_CONE_DIST;
	SimpleCamera::dlgClip = 0;
	SimpleCamera::dlgHither = DEF_HITHER_CLIP;
	SimpleCamera::dlgYon = DEF_YON_CLIP;
	SimpleCamera::dlgNearRange = 0.0f;
	SimpleCamera::dlgFarRange = 1000.0f;
	SimpleCamera::dlgRangeDisplay = 0;
	SimpleCamera::dlgIsOrtho = 0;

	SimpleCamera::dlgDOFEnable = 1;
	SimpleCamera::dlgDOFFStop = 2.0f;
	SimpleCamera::dlgMultiPassEffectEnable = 0;
	SimpleCamera::dlgMPEffect_REffectPerPass = 0;
}

SimpleCamera * SimpleCamera::currentEditCam = NULL;
HWND SimpleCamera::hSimpleCamParams = NULL;
HWND SimpleCamera::hDepthOfFieldParams = NULL;
IObjParam *SimpleCamera::iObjParams;
ISpinnerControl *SimpleCamera::fovSpin = NULL;
ISpinnerControl *SimpleCamera::lensSpin = NULL;
ISpinnerControl *SimpleCamera::tdistSpin = NULL;
ISpinnerControl *SimpleCamera::hitherSpin = NULL;
ISpinnerControl *SimpleCamera::yonSpin = NULL;
ISpinnerControl *SimpleCamera::envNearSpin = NULL;
ISpinnerControl *SimpleCamera::envFarSpin = NULL;
ISpinnerControl *SimpleCamera::fStopSpin = NULL;
ICustButton *SimpleCamera::iFovType = NULL;

static float mmTab[9] = {
	15.0f, 20.0f, 24.0f, 28.0f, 35.0f, 50.0f, 85.0f, 135.0f, 200.0f
	};


static float GetAspect() {
	return GetCOREInterface()->GetRendImageAspect();
	};

static float GetApertureWidth() {
	return GetCOREInterface()->GetRendApertureWidth();
	}


static int GetTargetPoint(TimeValue t, INode *inode, Point3& p) {
	Matrix3 tmat;
	if (inode && inode->GetTargetTM(t,tmat)) {
		p = tmat.GetTrans();
		return 1;
		}
	else 
		return 0;
	}

static int is_between(float a, float b, float c) 
{
	float t;
	if (b>c) { t = b; b = c; c = t;}
	return((a>=b)&&(a<=c));
}

static float interp_vals(float m, float *mtab, float *ntab, int n) 
{
	float frac;
	for (int i=1; i<n; i++) {
		if (is_between(m,mtab[i-1],mtab[i])) {
			frac = (m - mtab[i-1])/(mtab[i]-mtab[i-1]);
			return((1.0f-frac)*ntab[i-1] + frac*ntab[i]);
		}
	}
	return 0.0f;
}

class CameraMeshItem : public MaxSDK::Graphics::Utilities::MeshEdgeRenderItem
{
public:
	CameraMeshItem(Mesh* pMesh)
		: MeshEdgeRenderItem(pMesh, true, false)
	{

	}
	virtual ~CameraMeshItem()
	{
	}
	virtual void Display(MaxSDK::Graphics::DrawContext& drawContext)
	{
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		if (vpt->GetViewCamera() == drawContext.GetCurrentNode())
		{
			return;
		}
		__super::Display(drawContext);
	}

	virtual void HitTest(MaxSDK::Graphics::HitTestContext& hittestContext, MaxSDK::Graphics::DrawContext& drawContext)
	{
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		if (vpt->GetViewCamera() == drawContext.GetCurrentNode())
		{
			return;
		}
		__super::HitTest(hittestContext, drawContext);
	}
};

class CameraConeItem : public MaxSDK::Graphics::Utilities::SplineRenderItem
{
protected:
	SimpleCamera* mpCamera;
public:
	CameraConeItem(SimpleCamera* cam)
		: mpCamera(cam)
	{

	}
	~CameraConeItem()
	{
		mpCamera = nullptr;
	}
	
	void BuildCone(TimeValue t, float dist, int colid, BOOL drawSides, BOOL drawDiags, Color color) {
		Point3 posArray[5], tmpArray[2];
		mpCamera->GetConePoints(t, posArray, dist);

		if (colid)
		{
			color = GetUIColor(colid);
		}
		if (drawDiags) {
			tmpArray[0] =  posArray[0];	
			tmpArray[1] =  posArray[2];	
			AddLineStrip(tmpArray, color, 2, false, false);
			tmpArray[0] =  posArray[1];	
			tmpArray[1] =  posArray[3];	
			AddLineStrip(tmpArray, color, 2, false, false);
		}
		AddLineStrip(posArray, color, 4, true, false);
		if (drawSides) {
			color = GetUIColor(COLOR_CAMERA_CONE);
			tmpArray[0] = Point3(0,0,0);
			for (int i=0; i<4; i++) {
				tmpArray[1] = posArray[i];	
				AddLineStrip(tmpArray, color, 2, false, false);
			}
		}
	}
	void BuildConeAndLine(TimeValue t, INode* inode, Color& color)
	{
		if (nullptr == mpCamera)
		{
			DbgAssert(!_T("Invalid camera object!"));
			return;
		}
		Matrix3 tm = inode->GetObjectTM(t);
		if (mpCamera->hasTarget) {
			Point3 pt;
			if (GetTargetPoint(t, inode, pt)){
				float den = Length(tm.GetRow(2));
				float dist = (den!=0)?Length(tm.GetTrans()-pt)/den : 0.0f;
				mpCamera->targDist = dist;
				if (mpCamera->hSimpleCamParams 
					&& (mpCamera->currentEditCam == mpCamera)) 
				{
					mpCamera->tdistSpin->SetValue(mpCamera->GetTDist(t), FALSE);
				}
				if (mpCamera->coneState || (mpCamera->extDispFlags & EXT_DISP_ONLY_SELECTED)) 
				{
					if(mpCamera->manualClip) {
						BuildCone(t, mpCamera->GetClipDist(t, CAM_HITHER_CLIP), COLOR_CAMERA_CLIP, FALSE, TRUE, color);
						BuildCone(t, mpCamera->GetClipDist(t, CAM_YON_CLIP), COLOR_CAMERA_CLIP, TRUE, TRUE, color);
					}
					else
					{
						BuildCone(t, dist, COLOR_CAMERA_CONE, TRUE, FALSE, color);
					}
				}
			}
		}
		else {
			if (mpCamera->coneState || (mpCamera->extDispFlags & EXT_DISP_ONLY_SELECTED))
				if(mpCamera->manualClip) {
					BuildCone(t, mpCamera->GetClipDist(t, CAM_HITHER_CLIP), COLOR_CAMERA_CLIP, FALSE, TRUE, color);
					BuildCone(t, mpCamera->GetClipDist(t, CAM_YON_CLIP), COLOR_CAMERA_CLIP, TRUE, TRUE, color);
				}
				else
					BuildCone(t, mpCamera->GetTDist(t), COLOR_CAMERA_CONE, TRUE, FALSE, color);
		}
	}

	void BuildRange(TimeValue t, INode* inode, Color& color)
	{
		if(!mpCamera->rangeDisplay)
			return;
		Matrix3 tm = inode->GetObjectTM(t);
		int cnear = 0;
		int cfar = 0;
		if(!inode->IsFrozen() && !inode->Dependent()) { 
			cnear = COLOR_NEAR_RANGE;
			cfar = COLOR_FAR_RANGE;
		}
		BuildCone(t, mpCamera->GetEnvRange(t, ENV_NEAR_RANGE),cnear, FALSE, FALSE, color);
		BuildCone(t, mpCamera->GetEnvRange(t, ENV_FAR_RANGE), cfar, TRUE, FALSE, color);
	}
	
	virtual void Realize(MaxSDK::Graphics::DrawContext& drawContext) 
	{
		INode* inode = drawContext.GetCurrentNode();
		if (nullptr == inode)
		{
			return;
		}
		inode->SetTargetNodePair(0);
		mpCamera->SetExtendedDisplay(drawContext.GetExtendedDisplayMode());
		ClearLines();
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		if ( !vpt 
			|| !vpt->IsAlive() 
			|| !mpCamera->enable)
		{
			return;
		}

		Color color(inode->GetWireColor());
		if (inode->Dependent())
		{
			color = ColorMan()->GetColorAsPoint3(kViewportShowDependencies);
		}
		else if (inode->Selected()) 
		{
			color = GetSelColor();
		}
		else if (inode->IsFrozen())
		{
			color = GetFreezeColor();
		}

		BuildConeAndLine(drawContext.GetTime(), inode, color);
		BuildRange(drawContext.GetTime(), inode, color);
		SplineRenderItem::Realize(drawContext);
	}

	virtual void Display(MaxSDK::Graphics::DrawContext& drawContext)
	{
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		if (vpt->GetViewCamera() == drawContext.GetCurrentNode())
		{
			return;
		}
		__super::Display(drawContext);
	}

	virtual void HitTest(MaxSDK::Graphics::HitTestContext& /*hittestContext*/, MaxSDK::Graphics::DrawContext& /*drawContext*/)
	{
		//We don't need cone item been hit
	}
};


class CameraTargetLineItem : public MaxSDK::Graphics::Utilities::SplineRenderItem
{
protected:
	Color mLastColor;
	float mLastDist;
public:
	CameraTargetLineItem()
		: mLastDist(-1.0f)
	{
		mLastColor = Color(0,0,0);
	}
	~CameraTargetLineItem()
	{
	}

	virtual void Realize(MaxSDK::Graphics::DrawContext& drawContext) 
	{
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		INode* inode = drawContext.GetCurrentNode();
		if ( !vpt 
			|| !vpt->IsAlive() )
		{
			return;
		}

		if(nullptr == inode)
		{
			return;
		}

		Color color(inode->GetWireColor());
		if (inode->Dependent())
		{
			color = ColorMan()->GetColorAsPoint3(kViewportShowDependencies);
		}
		else if (inode->Selected()) 
		{
			color = GetSelColor();
		}
		else if (inode->IsFrozen())
		{
			color = GetFreezeColor();
		}
		
		TimeValue t = drawContext.GetTime();
		Matrix3 tm = inode->GetObjectTM(t);
		Point3 pt;
		if (GetTargetPoint(t, inode, pt)){
			float den = Length(tm.GetRow(2));
			float dist = (den!=0)?Length(tm.GetTrans()-pt)/den : 0.0f;
			Color lineColor(inode->GetWireColor());
			if(!inode->IsFrozen() && !inode->Dependent())
			{
				// 6/25/01 2:33pm --MQM--
				// if user has changed the color of the camera,
				// use that color for the target line too
				if ( lineColor == GetUIColor(COLOR_CAMERA_OBJ) )
					lineColor = GetUIColor(COLOR_TARGET_LINE);
			}
			if(mLastColor != lineColor
				|| mLastDist != dist)
			{
				ClearLines();
				Point3 v[2] = {Point3(0,0,0), Point3(0.0f, 0.0f, -dist)};
				AddLineStrip(v, lineColor, 2, false, false);
				v[0].z = -0.02f * dist;
				AddLineStrip(v, lineColor, 2, false, true);
				mLastColor = lineColor;
				mLastDist = dist;
			}
		}

		SplineRenderItem::Realize(drawContext);
	}

	virtual void Display(MaxSDK::Graphics::DrawContext& drawContext)
	{
		return;
	}

    //! [INode.SetTargetNodePair Example]
	virtual void HitTest(MaxSDK::Graphics::HitTestContext& hittestContext, MaxSDK::Graphics::DrawContext& drawContext)
	{
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		if (vpt->GetViewCamera() == drawContext.GetCurrentNode())
		{
			return;
		}
		drawContext.GetCurrentNode()->SetTargetNodePair(0);
		__super::Display(drawContext);
	}
    //! [INode.SetTargetNodePair Example]

	virtual void OnHit(MaxSDK::Graphics::HitTestContext& /*hittestContext*/, MaxSDK::Graphics::DrawContext& drawContext)
	{
		drawContext.GetCurrentNode()->SetTargetNodePair(1);
	}
};

// This changes the relation between FOV and Focal length.
void SimpleCamera::RenderApertureChanged(TimeValue t) {
	UpdateUI(t);
	}

float SimpleCamera::MMtoFOV(float mm) {
	return float(2.0f*atan(0.5f*GetApertureWidth()/mm));
	}

float SimpleCamera::FOVtoMM(float fov)	{
	float w = GetApertureWidth();
	float mm = float((0.5f*w)/tan(fov/2.0f));
	return mm;
	}


float SimpleCamera::CurFOVtoWFOV(float cfov) {
	switch (GetFOVType()) {
		case FOV_H: {
			return float(2.0*atan(GetAspect()*tan(cfov/2.0f)));
			}
		case FOV_D: {
			float w = GetApertureWidth();
			float h = w/GetAspect();
			float d = (float)sqrt(w*w + h*h);	
			return float(2.0*atan((w/d)*tan(cfov/2.0f)));
			}
		default:
			return cfov;
		}
	}	


float SimpleCamera::WFOVtoCurFOV(float fov) {
	switch (GetFOVType()) {
		case FOV_H: {
			return float(2.0*atan(tan(fov/2.0f)/GetAspect()));
			}
		case FOV_D: {
			float w = GetApertureWidth();
			float h = w/GetAspect();
			float d = (float)sqrt(w*w + h*h);	
			return float(2.0*atan((d/w)*tan(fov/2.0f)));
			}
		default:
			return fov;
		}
	}	


static void LoadCamResources() {
	static BOOL loaded=FALSE;
	if (loaded) return;
	HBITMAP hBitmap;
	HBITMAP hMask;
	hCamImages = ImageList_Create(14,14, ILC_COLOR4|ILC_MASK, 3, 0);
	hBitmap = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_FOV));
	hMask   = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_FOVMASK));
	ImageList_Add(hCamImages,hBitmap,hMask);
	DeleteObject(hBitmap);	
	DeleteObject(hMask);	
	}

class DeleteCamResources {
	public:
		~DeleteCamResources() {
			ImageList_Destroy(hCamImages);
			}
	};

static DeleteCamResources theDelete;

static int typeName[NUM_CAM_TYPES] = {
	IDS_DB_FREE_CAM,
	IDS_DB_TARGET_CAM
	};

INT_PTR CALLBACK SimpleCamParamDialogProc( 
	HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
	{
   SimpleCamera *so = DLGetWindowLongPtr<SimpleCamera *>( hDlg);
	if ( !so && message != WM_INITDIALOG ) return FALSE;
	TimeValue t = so->iObjParams->GetTime();

	float tmpSmall, tmpLarge;

	switch ( message )
	{
		case WM_INITDIALOG:
		{
			LoadCamResources();
			so = (SimpleCamera *)lParam;
			DLSetWindowLongPtr( hDlg, so);
			SetDlgFont( hDlg, so->iObjParams->GetAppHFont() );
			
			
			CheckDlgButton( hDlg, IDC_SHOWCAMCONE, so->coneState );
			CheckDlgButton( hDlg, IDC_SHOWHORZLINE, so->horzLineState );
			CheckDlgButton( hDlg, IDC_SHOW_RANGES, so->rangeDisplay );
			CheckDlgButton( hDlg, IDC_IS_ORTHO, so->isOrtho );
			CheckDlgButton( hDlg, IDC_MANUAL_CLIP, so->manualClip );
			EnableWindow( GetDlgItem(hDlg, IDC_HITHER), so->manualClip);
			EnableWindow( GetDlgItem(hDlg, IDC_HITHER_SPIN), so->manualClip);
			EnableWindow( GetDlgItem(hDlg, IDC_YON), so->manualClip);
			EnableWindow( GetDlgItem(hDlg, IDC_YON_SPIN), so->manualClip);

			HWND hwndType = GetDlgItem(hDlg, IDC_CAM_TYPE);
			int i;
			for (i=0; i<NUM_CAM_TYPES; i++)
				SendMessage(hwndType, CB_ADDSTRING, 0, (LPARAM)GetString(typeName[i]));
			SendMessage( hwndType, CB_SETCURSEL, i, (LPARAM)0 );
			EnableWindow(hwndType,!so->inCreate);		

			CheckDlgButton( hDlg, IDC_DOF_ENABLE, so->GetDOFEnable(t) );

			CheckDlgButton(hDlg, IDC_ENABLE_MP_EFFECT, so->GetMultiPassEffectEnabled(t) );
			CheckDlgButton(hDlg, IDC_MP_EFFECT_REFFECT_PER_PASS, so->GetMPEffect_REffectPerPass() );

			// build list of multi pass effects
			int selIndex = -1;
			IMultiPassCameraEffect *pIMultiPassCameraEffect = so->GetIMultiPassCameraEffect();
			HWND hEffectList = GetDlgItem(hDlg, IDC_MP_EFFECT);
			SimpleCamera::FindCompatibleMultiPassEffects(so);
			int numClasses = SimpleCamera::smCompatibleEffectList.Count();
			for (int i=0; i<numClasses; i++)
			{
				int index = SendMessage( hEffectList, CB_ADDSTRING, 0, (LPARAM)SimpleCamera::smCompatibleEffectList[i]->CD()->ClassName() );
				if ( pIMultiPassCameraEffect && ( pIMultiPassCameraEffect->ClassID() == SimpleCamera::smCompatibleEffectList[i]->CD()->ClassID() ) )
				{
					selIndex = index;
				}
			}
			SendMessage(hEffectList, CB_SETCURSEL, selIndex, (LPARAM)0);
			EnableWindow( GetDlgItem(hDlg, IDC_PREVIEW_MP_EFFECT), so->GetMultiPassEffectEnabled(t) );
			return FALSE;
		}

		case WM_DESTROY:
			RELEASE_SPIN ( fovSpin );
			RELEASE_SPIN ( lensSpin );
			RELEASE_SPIN ( hitherSpin );
			RELEASE_SPIN ( yonSpin );
			RELEASE_SPIN ( envNearSpin );
			RELEASE_SPIN ( envFarSpin );
			RELEASE_SPIN ( fStopSpin );
			RELEASE_BUT ( iFovType );
			return FALSE;

		case CC_SPINNER_CHANGE:
			if (!theHold.Holding()) theHold.Begin();
			switch ( LOWORD(wParam) ) {
				case IDC_FOVSPINNER:
					so->SetFOV(t, so->CurFOVtoWFOV(DegToRad(so->fovSpin->GetFVal())));	
					so->lensSpin->SetValue(so->FOVtoMM(so->GetFOV(t)), FALSE);
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
				case IDC_LENSSPINNER:
					so->SetFOV(t, so->MMtoFOV(so->lensSpin->GetFVal()));	
					so->fovSpin->SetValue(RadToDeg(so->WFOVtoCurFOV(so->GetFOV(t))), FALSE);
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
				case IDC_TDISTSPINNER:
					so->SetTDist(t, so->tdistSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
#if 0	// this section leaves the spinners unconstrained
				case IDC_HITHER_SPIN:
					so->SetClipDist(t, CAM_HITHER_CLIP, so->hitherSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
				case IDC_YON_SPIN:
					so->SetClipDist(t, CAM_YON_CLIP, so->yonSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
#else	// here, we constrain hither <= yon
				case IDC_HITHER_SPIN:
				case IDC_YON_SPIN:
					tmpSmall = so->hitherSpin->GetFVal();
					tmpLarge = so->yonSpin->GetFVal();

					if(tmpSmall > tmpLarge) {
						if(LOWORD(wParam) == IDC_HITHER_SPIN) {
							so->yonSpin->SetValue(tmpSmall, FALSE);
							so->SetClipDist(t, CAM_YON_CLIP, so->yonSpin->GetFVal());	
						}
						else	{
							so->hitherSpin->SetValue(tmpLarge, FALSE);
							so->SetClipDist(t, CAM_HITHER_CLIP, so->hitherSpin->GetFVal());	
						}
					}
					if(LOWORD(wParam) == IDC_HITHER_SPIN)
						so->SetClipDist(t, CAM_HITHER_CLIP, so->hitherSpin->GetFVal());	
					else
						so->SetClipDist(t, CAM_YON_CLIP, so->yonSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
#endif
#if 0	// similar constraint comments apply here
				case IDC_NEAR_RANGE_SPIN:
					so->SetEnvRange(t, ENV_NEAR_RANGE, so->envNearSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
				case IDC_FAR_RANGE_SPIN:
					so->SetEnvRange(t, ENV_FAR_RANGE, so->envFarSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
#else
				case IDC_NEAR_RANGE_SPIN:
				case IDC_FAR_RANGE_SPIN:
					tmpSmall = so->envNearSpin->GetFVal();
					tmpLarge = so->envFarSpin->GetFVal();

					if(tmpSmall > tmpLarge) {
						if(LOWORD(wParam) == IDC_NEAR_RANGE_SPIN) {
							so->envFarSpin->SetValue(tmpSmall, FALSE);
							so->SetEnvRange(t, ENV_FAR_RANGE, so->envFarSpin->GetFVal());	
						}
						else	{
							so->envNearSpin->SetValue(tmpLarge, FALSE);
							so->SetEnvRange(t, ENV_NEAR_RANGE, so->envNearSpin->GetFVal());	
						}
					}
					if(LOWORD(wParam) == IDC_NEAR_RANGE_SPIN)
						so->SetEnvRange(t, ENV_NEAR_RANGE, so->envNearSpin->GetFVal());	
					else
						so->SetEnvRange(t, ENV_FAR_RANGE, so->envFarSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
#endif
				case IDC_DOF_FSTOP_SPIN:
					so->SetDOFFStop ( t, so->fStopSpin->GetFVal ());
					so->UpdateKeyBrackets (t);
					break;
				}
			return TRUE;

		case WM_SET_TYPE:
			theHold.Begin();
			so->SetType(wParam);
			theHold.Accept(GetString(IDS_DS_SETCAMTYPE));
			return FALSE;

		case CC_SPINNER_BUTTONDOWN:
			theHold.Begin();
			return TRUE;

		case WM_CUSTEDIT_ENTER:
		case CC_SPINNER_BUTTONUP:
			if (HIWORD(wParam) || message==WM_CUSTEDIT_ENTER) theHold.Accept(GetString(IDS_DS_PARAMCHG));
			else theHold.Cancel();
			if ((message == CC_SPINNER_BUTTONUP) && (LOWORD(wParam) == IDC_FOVSPINNER))
			{
            ExecuteMAXScriptScript(_T(" InvalidateAllBackgrounds() "), TRUE);
			}
			so->iObjParams->RedrawViews(t,REDRAW_END);
			return TRUE;

		case WM_MOUSEACTIVATE:
			so->iObjParams->RealizeParamPanel();
			return FALSE;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MOUSEMOVE:
			so->iObjParams->RollupMouseMessage(hDlg,message,wParam,lParam);
			return FALSE;

		case WM_COMMAND:			
			switch( LOWORD(wParam) )
			{
				case IDC_MANUAL_CLIP:
					so->SetManualClip( IsDlgButtonChecked( hDlg, IDC_MANUAL_CLIP) );
					EnableWindow( GetDlgItem(hDlg, IDC_HITHER), so->manualClip);
					EnableWindow( GetDlgItem(hDlg, IDC_HITHER_SPIN), so->manualClip);
					EnableWindow( GetDlgItem(hDlg, IDC_YON), so->manualClip);
					EnableWindow( GetDlgItem(hDlg, IDC_YON_SPIN), so->manualClip);
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_SHOWCAMCONE:
					so->SetConeState( IsDlgButtonChecked( hDlg, IDC_SHOWCAMCONE ) );
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_SHOWHORZLINE:
					so->SetHorzLineState( IsDlgButtonChecked( hDlg, IDC_SHOWHORZLINE ) );
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_SHOW_RANGES:
					so->SetEnvDisplay( IsDlgButtonChecked( hDlg, IDC_SHOW_RANGES ) );
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_IS_ORTHO:
					so->SetOrtho( IsDlgButtonChecked( hDlg, IDC_IS_ORTHO ) );
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_15MM:
				case IDC_20MM:
				case IDC_24MM:
				case IDC_28MM:
				case IDC_35MM:
				case IDC_50MM:
				case IDC_85MM:
				case IDC_135MM:
				case IDC_200MM:
					theHold.Begin();
					so->SetFOV(t, so->MMtoFOV(mmTab[LOWORD(wParam) - IDC_15MM]));	
					so->fovSpin->SetValue(RadToDeg(so->WFOVtoCurFOV(so->GetFOV(t))), FALSE);
					so->lensSpin->SetValue(so->FOVtoMM(so->GetFOV(t)), FALSE);
					so->iObjParams->RedrawViews(t,REDRAW_END);
					theHold.Accept(GetString(IDS_DS_CAMPRESET));
					break;		   
				case IDC_FOV_TYPE: 
					so->SetFOVType(so->iFovType->GetCurFlyOff());
					break;
				case IDC_CAM_TYPE:
				{
					int code = HIWORD(wParam);
					if (code==CBN_SELCHANGE) {
						int newType = SendMessage( GetDlgItem(hDlg,IDC_CAM_TYPE), CB_GETCURSEL, 0, 0 );
						PostMessage(hDlg,WM_SET_TYPE,newType,0);
						}
					break;
				}
				case IDC_ENABLE_MP_EFFECT:
					so->SetMultiPassEffectEnabled( t, IsDlgButtonChecked(hDlg, IDC_ENABLE_MP_EFFECT) );
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_MP_EFFECT_REFFECT_PER_PASS:
					so->SetMPEffect_REffectPerPass( IsDlgButtonChecked(hDlg, IDC_MP_EFFECT_REFFECT_PER_PASS) );
					break;
				case IDC_PREVIEW_MP_EFFECT:
					GetCOREInterface()->DisplayActiveCameraViewWithMultiPassEffect();
					break;
				case IDC_MP_EFFECT:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						int index = SendMessage( GetDlgItem(hDlg,IDC_MP_EFFECT), CB_GETCURSEL, 0, 0 );
						Tab<ClassEntry*> &effectList = SimpleCamera::GetCompatibleEffectList();
						if ( (index >= 0) && ( index < effectList.Count() ) )
							{
							IMultiPassCameraEffect *pPrevCameraEffect = so->GetIMultiPassCameraEffect();
							if ( !pPrevCameraEffect || ( pPrevCameraEffect->ClassID() != effectList[index]->CD()->ClassID() ) ) {
								IMultiPassCameraEffect *mpce = reinterpret_cast<IMultiPassCameraEffect *>( effectList[index]->CD()->Create(0) );
								theHold.Begin();
								so->SetIMultiPassCameraEffect(mpce);
								theHold.Accept(GetString(IDS_DS_MULTIPASS));
								}
							}
						else
						{
							DbgAssert(0);
						}
						so->iObjParams->RedrawViews(t);
					}
					break;
				}
			}
			return FALSE;

		default:
			return FALSE;
		}
	}



void SimpleCamera::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev )
{
	iObjParams = ip;	
	inCreate = (flags & BEGIN_EDIT_CREATE) ? 1 : 0;

	currentEditCam = this;

	if ( !hSimpleCamParams )
	{
		hSimpleCamParams = ip->AddRollupPage(
				hInstance,
				MAKEINTRESOURCE(IDD_FCAMERAPARAM),  // DS 8/15/00 
				//hasTarget ? MAKEINTRESOURCE(IDD_SCAMERAPARAM) : MAKEINTRESOURCE(IDD_FCAMERAPARAM),
				SimpleCamParamDialogProc,
				GetString(IDS_RB_PARAMETERS),
				(LPARAM)this);

		ip->RegisterDlgWnd(hSimpleCamParams);

		{
			iFovType = GetICustButton(GetDlgItem(hSimpleCamParams,IDC_FOV_TYPE));
			iFovType->SetType(CBT_CHECK);
			iFovType->SetImage(hCamImages,0,0,0,0,14,14);
			FlyOffData fod[3] = {
				{ 0,0,0,0 },
				{ 1,1,1,1 },
				{ 2,2,2,2 }
				};
			iFovType->SetFlyOff(3,fod,0/*timeout*/,0/*init val*/,FLY_DOWN);
			iFovType->SetCurFlyOff(GetFOVType());  // DS 8/8/00
//			ttips[2] = GetResString(IDS_DB_BACKGROUND);
//			iFovType->SetTooltip(1, ttips[2]);
		}

		fovSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_FOVSPINNER));
		fovSpin->SetLimits( MIN_FOV, MAX_FOV, FALSE );
		fovSpin->SetValue(RadToDeg(WFOVtoCurFOV(GetFOV(ip->GetTime()))), FALSE);
		fovSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_FOV), EDITTYPE_FLOAT );
			
		lensSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_LENSSPINNER));
		lensSpin->SetLimits( MIN_LENS, MAX_LENS, FALSE );
		lensSpin->SetValue(FOVtoMM(GetFOV(ip->GetTime())), FALSE);
		lensSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_LENS), EDITTYPE_FLOAT );
			
		hitherSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_HITHER_SPIN));
		hitherSpin->SetLimits( MIN_CLIP, MAX_CLIP, FALSE );
		hitherSpin->SetValue(GetClipDist(ip->GetTime(), CAM_HITHER_CLIP), FALSE);
		// xavier robitaille | 03.02.07 | increments proportional to the spinner value
		hitherSpin->SetAutoScale();
		hitherSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_HITHER), EDITTYPE_UNIVERSE );
			
		yonSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_YON_SPIN));
		yonSpin->SetLimits( MIN_CLIP, MAX_CLIP, FALSE );
		yonSpin->SetValue(GetClipDist(ip->GetTime(), CAM_YON_CLIP), FALSE);
		// xavier robitaille | 03.02.07 | increments proportional to the spinner value
		yonSpin->SetAutoScale();
		yonSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_YON), EDITTYPE_UNIVERSE );

		envNearSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_NEAR_RANGE_SPIN));
		envNearSpin->SetLimits( 0.0f, 999999.0f, FALSE );
		envNearSpin->SetValue(GetEnvRange(ip->GetTime(), ENV_NEAR_RANGE), FALSE);
		// alexc | 03.06.09 | increments proportional to the spinner value
        envNearSpin->SetAutoScale();
		envNearSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_NEAR_RANGE), EDITTYPE_UNIVERSE );
			
		envFarSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_FAR_RANGE_SPIN));
		envFarSpin->SetLimits( 0.0f, 999999.0f, FALSE );
		envFarSpin->SetValue(GetEnvRange(ip->GetTime(), ENV_FAR_RANGE), FALSE);
		// alexc | 03.06.09 | increments proportional to the spinner value
        envFarSpin->SetAutoScale();
		envFarSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_FAR_RANGE), EDITTYPE_UNIVERSE );

		tdistSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_TDISTSPINNER));
		tdistSpin->SetLimits( MIN_TDIST, MAX_TDIST, FALSE );
		tdistSpin->SetValue(GetTDist(ip->GetTime()), FALSE);
		tdistSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_TDIST), EDITTYPE_UNIVERSE );
		// xavier robitaille | 03.02.07 | increments proportional to the spinner value
		tdistSpin->SetAutoScale();
		// display possible multipass camera effect rollup
		IMultiPassCameraEffect *pCurCameraEffect = GetIMultiPassCameraEffect();
		if (pCurCameraEffect)
		{
			pCurCameraEffect->BeginEditParams(ip, flags, prev);
		}

	}
	else
	{
      DLSetWindowLongPtr( hSimpleCamParams, this);
		
		fovSpin->SetValue(RadToDeg(WFOVtoCurFOV(GetFOV(ip->GetTime()))),FALSE);
		hitherSpin->SetValue(GetClipDist(ip->GetTime(), CAM_HITHER_CLIP),FALSE);
		yonSpin->SetValue(GetClipDist(ip->GetTime(), CAM_YON_CLIP),FALSE);
		envNearSpin->SetValue(GetEnvRange(ip->GetTime(), ENV_NEAR_RANGE),FALSE);
		envFarSpin->SetValue(GetEnvRange(ip->GetTime(), ENV_FAR_RANGE),FALSE);
		// DS 8/15/00 
//		if(!hasTarget)
			tdistSpin->SetValue(GetTDist(ip->GetTime()),FALSE);

		SetConeState( IsDlgButtonChecked(hSimpleCamParams,IDC_SHOWCAMCONE) );
		SetHorzLineState( IsDlgButtonChecked(hSimpleCamParams,IDC_SHOWHORZLINE) );
		SetEnvDisplay( IsDlgButtonChecked(hSimpleCamParams,IDC_SHOW_RANGES) );
		SetManualClip( IsDlgButtonChecked(hSimpleCamParams,IDC_MANUAL_CLIP) );

		// display possible multipass camera effect rollup
		IMultiPassCameraEffect *pCurCameraEffect = GetIMultiPassCameraEffect();
		if (pCurCameraEffect)
		{
			pCurCameraEffect->BeginEditParams(ip, flags, prev);
		}

	}
	SendMessage( GetDlgItem(hSimpleCamParams, IDC_CAM_TYPE), CB_SETCURSEL, hasTarget, (LPARAM)0 );
}
		
void SimpleCamera::EndEditParams( IObjParam *ip, ULONG flags,Animatable *prev)
{
	dlgFOV = GetFOV(ip->GetTime());
	dlgFOVType = GetFOVType();
	dlgShowCone = IsDlgButtonChecked(hSimpleCamParams, IDC_SHOWCAMCONE );
	dlgRangeDisplay = IsDlgButtonChecked(hSimpleCamParams, IDC_SHOW_RANGES );
	dlgIsOrtho = IsDlgButtonChecked(hSimpleCamParams, IDC_IS_ORTHO);
	dlgShowHorzLine = IsDlgButtonChecked(hSimpleCamParams, IDC_SHOWHORZLINE );
	dlgClip = IsDlgButtonChecked(hSimpleCamParams, IDC_MANUAL_CLIP );
	dlgTDist = GetTDist(ip->GetTime());
	dlgHither = GetClipDist(ip->GetTime(), CAM_HITHER_CLIP);
	dlgYon = GetClipDist(ip->GetTime(), CAM_YON_CLIP);
	dlgNearRange = GetEnvRange(ip->GetTime(), ENV_NEAR_RANGE);
	dlgFarRange = GetEnvRange(ip->GetTime(), ENV_FAR_RANGE);

	dlgMultiPassEffectEnable = GetMultiPassEffectEnabled( ip->GetTime() );
	dlgMPEffect_REffectPerPass = GetMPEffect_REffectPerPass();

	IMultiPassCameraEffect *pCurCameraEffect = GetIMultiPassCameraEffect();
	if (pCurCameraEffect)
	{
		pCurCameraEffect->EndEditParams(ip, flags, prev);
	}

	// Depth of Field
	currentEditCam = NULL;

	if ( flags&END_EDIT_REMOVEUI )
	{
		if ( hDepthOfFieldParams )
		{
			ip->UnRegisterDlgWnd ( hDepthOfFieldParams );
			ip->DeleteRollupPage ( hDepthOfFieldParams );
			hDepthOfFieldParams = NULL;
		}

		ip->UnRegisterDlgWnd(hSimpleCamParams);
		ip->DeleteRollupPage(hSimpleCamParams);
		hSimpleCamParams = NULL;				
	}
	else
	{
      DLSetWindowLongPtr( hSimpleCamParams, 0);
      DLSetWindowLongPtr( hDepthOfFieldParams, 0);
	}
	iObjParams = NULL;
}


static void MakeQuad(Face *f, int a,  int b , int c , int d, int sg, int dv = 0) {
	f[0].setVerts( a+dv, b+dv, c+dv);
	f[0].setSmGroup(sg);
	f[0].setEdgeVisFlags(1,1,0);
	f[1].setVerts( c+dv, d+dv, a+dv);
	f[1].setSmGroup(sg);
	f[1].setEdgeVisFlags(1,1,0);
	}

void SimpleCamera::BuildMesh()	{

	mesh.setNumVerts(170+2);	//pw need to add 2 points to make the bounding box symmetrical
	mesh.setNumFaces(284);


	mesh.setVert(0, Point3(3.051641,3.476140,19.865290));
	mesh.setVert(1, Point3(3.051641,7.351941,19.865288));
	mesh.setVert(2, Point3(3.051641,10.708482,17.927383));
	mesh.setVert(3, Point3(3.051642,12.646385,14.570845));
	mesh.setVert(4, Point3(3.051642,12.646385,10.695045));
	mesh.setVert(5, Point3(3.051642,10.718940,7.343246));
	mesh.setVert(6, Point3(3.051642,7.362399,5.405346));
	mesh.setVert(7, Point3(3.051642,3.476141,5.400602));
	mesh.setVert(8, Point3(3.051642,0.119599,7.338503));
	mesh.setVert(9, Point3(3.051641,-1.818303,10.695044));
	mesh.setVert(10, Point3(3.051641,-1.818301,14.570848));
	mesh.setVert(11, Point3(3.051641,0.119600,17.927389));
	mesh.setVert(12, Point3(-2.927488,3.476141,19.865290));
	mesh.setVert(13, Point3(-2.927488,7.351941,19.865288));
	mesh.setVert(14, Point3(-2.927488,10.708485,17.927383));
	mesh.setVert(15, Point3(-2.927488,12.646383,14.570839));
	mesh.setVert(16, Point3(-2.927486,12.646383,10.695044));
	mesh.setVert(17, Point3(-2.927485,10.718940,7.343246));
	mesh.setVert(18, Point3(-2.927486,7.362398,5.405346));
	mesh.setVert(19, Point3(-2.927488,3.476141,5.400604));
	mesh.setVert(20, Point3(-2.927486,0.119599,7.338502));
	mesh.setVert(21, Point3(-2.927488,-1.818302,10.695044));
	mesh.setVert(22, Point3(-2.927488,-1.818301,14.570842));
	mesh.setVert(23, Point3(-2.927488,0.119600,17.927389));
	mesh.setVert(24, Point3(3.051642,14.605198,7.347991));
	mesh.setVert(25, Point3(3.051642,17.961739,5.410092));
	mesh.setVert(26, Point3(3.051642,19.899639,2.053550));
	mesh.setVert(27, Point3(3.051643,19.899641,-1.822252));
	mesh.setVert(28, Point3(3.051643,17.961742,-5.178796));
	mesh.setVert(29, Point3(3.051643,14.605200,-7.116695));
	mesh.setVert(30, Point3(3.051644,10.729396,-7.116697));
	mesh.setVert(31, Point3(3.051644,7.372853,-5.178795));
	mesh.setVert(32, Point3(3.051642,5.434954,-1.822254));
	mesh.setVert(33, Point3(3.051642,5.434955,2.053549));
	mesh.setVert(34, Point3(-2.927486,14.605195,7.347991));
	mesh.setVert(35, Point3(-2.927485,17.961739,5.410092));
	mesh.setVert(36, Point3(-2.927485,19.899643,2.053545));
	mesh.setVert(37, Point3(-2.927485,19.899641,-1.822254));
	mesh.setVert(38, Point3(-2.927485,17.961742,-5.178796));
	mesh.setVert(39, Point3(-2.927485,14.605194,-7.116697));
	mesh.setVert(40, Point3(-2.927485,10.729397,-7.116697));
	mesh.setVert(41, Point3(-2.927485,7.372854,-5.178796));
	mesh.setVert(42, Point3(-2.927485,5.434955,-1.822254));
	mesh.setVert(43, Point3(-2.927485,5.434955,2.053546));
	mesh.setVert(44, Point3(-4.024377,-5.645106,-6.857333));
	mesh.setVert(45, Point3(-4.024381,-5.645108,5.771017));
	mesh.setVert(46, Point3(4.298551,-5.645106,-6.857333));
	mesh.setVert(47, Point3(4.298550,-5.645107,5.771020));
	mesh.setVert(48, Point3(-4.024377,5.452132,-5.590975));
	mesh.setVert(49, Point3(4.298551,5.452132,-5.590975));
	mesh.setVert(50, Point3(-4.024381,-2.215711,8.686206));
	mesh.setVert(51, Point3(4.298549,-2.215710,8.686206));
	mesh.setVert(52, Point3(4.298550,5.452131,4.260479));
	mesh.setVert(53, Point3(-4.024381,5.452131,4.260478));
	mesh.setVert(54, Point3(4.298552,-2.673011,-9.176138));
	mesh.setVert(55, Point3(-4.024377,-2.673013,-9.176139));
	mesh.setVert(56, Point3(-4.024377,2.446853,-9.185402));
	mesh.setVert(57, Point3(4.298551,2.446853,-9.185402));
	mesh.setVert(58, Point3(-3.974360,3.286706,-5.111160));
	mesh.setVert(59, Point3(-3.974360,3.778829,-5.883637));
	mesh.setVert(60, Point3(-3.974360,3.580588,-6.777846));
	mesh.setVert(61, Point3(-3.974360,2.808110,-7.269970));
	mesh.setVert(62, Point3(-3.974360,1.913902,-7.071730));
	mesh.setVert(63, Point3(-3.974360,1.421778,-6.299250));
	mesh.setVert(64, Point3(-3.974360,1.620020,-5.405043));
	mesh.setVert(65, Point3(-3.974360,2.392498,-4.912918));
	mesh.setVert(66, Point3(-4.842994,3.493080,-4.816429));
	mesh.setVert(67, Point3(-5.090842,4.044086,-5.504812));
	mesh.setVert(68, Point3(-5.689195,3.988004,-6.195997));
	mesh.setVert(69, Point3(-6.287547,3.357684,-6.485096));
	mesh.setVert(70, Point3(-6.535392,2.522360,-6.202761));
	mesh.setVert(71, Point3(-6.287547,1.971353,-5.514376));
	mesh.setVert(72, Point3(-5.689195,2.027436,-4.823191));
	mesh.setVert(73, Point3(-5.090842,2.657756,-4.534092));
	mesh.setVert(74, Point3(-5.202796,3.991309,-4.104883));
	mesh.setVert(75, Point3(-5.553304,4.684474,-4.590243));
	mesh.setVert(76, Point3(-6.399501,4.971593,-4.791286));
	mesh.setVert(77, Point3(-7.245704,4.684475,-4.590243));
	mesh.setVert(78, Point3(-7.596208,3.991309,-4.104883));
	mesh.setVert(79, Point3(-7.245700,3.298144,-3.619523));
	mesh.setVert(80, Point3(-6.399501,3.011024,-3.418482));
	mesh.setVert(81, Point3(-5.553304,3.298143,-3.619523));
	mesh.setVert(82, Point3(-4.700181,4.326933,-3.625562));
	mesh.setVert(83, Point3(-5.197900,5.311228,-4.314773));
	mesh.setVert(84, Point3(-6.399501,5.718936,-4.600252));
	mesh.setVert(85, Point3(-7.601106,5.311228,-4.314773));
	mesh.setVert(86, Point3(-8.098825,4.326933,-3.625562));
	mesh.setVert(87, Point3(-7.601106,3.342639,-2.936350));
	mesh.setVert(88, Point3(-6.399501,2.934930,-2.650871));
	mesh.setVert(89, Point3(-5.197900,3.342638,-2.936352));
	mesh.setVert(90, Point3(-2.228243,-0.091865,-9.172746));
	mesh.setVert(91, Point3(-2.073412,0.786230,-9.172744));
	mesh.setVert(92, Point3(-1.627592,1.558412,-9.172746));
	mesh.setVert(93, Point3(-0.944555,2.131548,-9.172746));
	mesh.setVert(94, Point3(-0.106688,2.436507,-9.172744));
	mesh.setVert(95, Point3(0.784952,2.436507,-9.172744));
	mesh.setVert(96, Point3(1.622821,2.131547,-9.172746));
	mesh.setVert(97, Point3(2.305858,1.558413,-9.172746));
	mesh.setVert(98, Point3(2.751678,0.786228,-9.172744));
	mesh.setVert(99, Point3(2.906510,-0.091866,-9.172746));
	mesh.setVert(100, Point3(2.751678,-0.969961,-9.172746));
	mesh.setVert(101, Point3(2.305857,-1.742143,-9.172744));
	mesh.setVert(102, Point3(1.622820,-2.315279,-9.172744));
	mesh.setVert(103, Point3(0.784952,-2.620237,-9.172746));
	mesh.setVert(104, Point3(-0.106688,-2.620237,-9.172746));
	mesh.setVert(105, Point3(-0.944556,-2.315279,-9.172746));
	mesh.setVert(106, Point3(-1.627593,-1.742142,-9.172744));
	mesh.setVert(107, Point3(-2.073413,-0.969958,-9.172747));
	mesh.setVert(108, Point3(-2.228243,-0.091864,-14.836016));
	mesh.setVert(109, Point3(-2.073412,0.786230,-14.836016));
	mesh.setVert(110, Point3(-1.627591,1.558412,-14.836016));
	mesh.setVert(111, Point3(-0.944554,2.131549,-14.836016));
	mesh.setVert(112, Point3(-0.106687,2.436507,-14.836020));
	mesh.setVert(113, Point3(0.784954,2.436507,-14.836020));
	mesh.setVert(114, Point3(1.622822,2.131548,-14.836020));
	mesh.setVert(115, Point3(2.305858,1.558412,-14.836016));
	mesh.setVert(116, Point3(2.751678,0.786230,-14.836020));
	mesh.setVert(117, Point3(2.906510,-0.091865,-14.836020));
	mesh.setVert(118, Point3(2.751679,-0.969959,-14.836016));
	mesh.setVert(119, Point3(2.305858,-1.742141,-14.836016));
	mesh.setVert(120, Point3(1.622821,-2.315277,-14.836016));
	mesh.setVert(121, Point3(0.784952,-2.620236,-14.836016));
	mesh.setVert(122, Point3(-0.106688,-2.620236,-14.836016));
	mesh.setVert(123, Point3(-0.944555,-2.315276,-14.836016));
	mesh.setVert(124, Point3(-1.627592,-1.742141,-14.836016));
	mesh.setVert(125, Point3(-2.073412,-0.969958,-14.836016));
	mesh.setVert(126, Point3(-2.774064,-0.091865,-16.139450));
	mesh.setVert(127, Point3(-2.586314,0.972911,-16.139450));
	mesh.setVert(128, Point3(-2.045713,1.909259,-16.139450));
	mesh.setVert(129, Point3(-1.217464,2.604243,-16.139450));
	mesh.setVert(130, Point3(-0.201468,2.974035,-16.139450));
	mesh.setVert(131, Point3(0.879735,2.974035,-16.139450));
	mesh.setVert(132, Point3(1.895732,2.604242,-16.139450));
	mesh.setVert(133, Point3(2.723981,1.909260,-16.139450));
	mesh.setVert(134, Point3(3.264582,0.972910,-16.139450));
	mesh.setVert(135, Point3(3.452331,-0.091865,-16.139450));
	mesh.setVert(136, Point3(3.264582,-1.156641,-16.139450));
	mesh.setVert(137, Point3(2.723980,-2.092987,-16.139450));
	mesh.setVert(138, Point3(1.895732,-2.787971,-16.139450));
	mesh.setVert(139, Point3(0.879734,-3.157765,-16.139450));
	mesh.setVert(140, Point3(-0.201468,-3.157763,-16.139450));
	mesh.setVert(141, Point3(-1.217465,-2.787971,-16.139450));
	mesh.setVert(142, Point3(-2.045714,-2.092988,-16.139450));
	mesh.setVert(143, Point3(-2.586316,-1.156639,-16.139450));
	mesh.setVert(144, Point3(-2.774064,-0.091864,-19.194777));
	mesh.setVert(145, Point3(-2.586314,0.972911,-19.194771));
	mesh.setVert(146, Point3(-2.045713,1.909259,-19.194773));
	mesh.setVert(147, Point3(-1.217464,2.604243,-19.194773));
	mesh.setVert(148, Point3(-0.201467,2.974036,-19.194771));
	mesh.setVert(149, Point3(0.879735,2.974034,-19.194771));
	mesh.setVert(150, Point3(1.895733,2.604243,-19.194773));
	mesh.setVert(151, Point3(2.723981,1.909260,-19.194773));
	mesh.setVert(152, Point3(3.264582,0.972910,-19.194771));
	mesh.setVert(153, Point3(3.452332,-0.091865,-19.194773));
	mesh.setVert(154, Point3(3.264582,-1.156642,-19.194771));
	mesh.setVert(155, Point3(2.723981,-2.092989,-19.194773));
	mesh.setVert(156, Point3(1.895732,-2.787971,-19.194773));
	mesh.setVert(157, Point3(0.879734,-3.157765,-19.194771));
	mesh.setVert(158, Point3(-0.201468,-3.157763,-19.194771));
	mesh.setVert(159, Point3(-1.217464,-2.787970,-19.194777));
	mesh.setVert(160, Point3(-2.045714,-2.092988,-19.194777));
	mesh.setVert(161, Point3(-2.586316,-1.156640,-19.194773));
	mesh.setVert(162, Point3(5.033586,-3.077510,-19.223640));
	mesh.setVert(163, Point3(-4.204867,-3.077511,-19.223642));
	mesh.setVert(164, Point3(5.033587,3.017310,-19.223642));
	mesh.setVert(165, Point3(-4.204867,3.017310,-19.223642));
	mesh.setVert(166, Point3(8.950690,-5.661717,-23.902903));
	mesh.setVert(167, Point3(-8.121970,-5.661717,-23.902897));
	mesh.setVert(168, Point3(8.950689,5.601515,-23.902897));
	mesh.setVert(169, Point3(-8.121970,5.601515,-23.902897));

//pw need to ceate 2 points to make the bounding box symmetrical
	Box3 bbox;
	bbox.Init();
	for (int i =0; i < 170; i++)
		{
		bbox += mesh.getVert(i);
		}
	Point3 minVec = bbox.pmin;
	Point3 maxVec = bbox.pmax;
	float minLen = Length(minVec);
	float maxLen = Length(maxVec);
	if (fabs(minVec.x) > fabs(maxVec.x))
		{
		maxVec.x = -minVec.x;
		}
	else
		{
		minVec.x = -maxVec.x;
		}

	if (fabs(minVec.y) > fabs(maxVec.y))
		{
		maxVec.y = -minVec.y;
		}
	else
		{
		minVec.y = -maxVec.y;
		}

	if (fabs(minVec.z) > fabs(maxVec.z))
		{
		maxVec.z = -minVec.z;
		}
	else
		{
		minVec.z = -maxVec.z;
		}


	mesh.setVert(170, maxVec);
	mesh.setVert(171, minVec);

	Face f;

	f.v[0] = 10;  f.v[1] = 9;  f.v[2] = 8;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[0] = f;
	f.v[0] = 8;  f.v[1] = 7;  f.v[2] = 6;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[1] = f;
	f.v[0] = 6;  f.v[1] = 5;  f.v[2] = 4;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[2] = f;
	f.v[0] = 8;  f.v[1] = 6;  f.v[2] = 4;  	f.smGroup = 1;  f.flags = 65536; mesh.faces[3] = f;
	f.v[0] = 4;  f.v[1] = 3;  f.v[2] = 2;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[4] = f;
	f.v[0] = 2;  f.v[1] = 1;  f.v[2] = 0;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[5] = f;
	f.v[0] = 4;  f.v[1] = 2;  f.v[2] = 0;  	f.smGroup = 1;  f.flags = 65536; mesh.faces[6] = f;
	f.v[0] = 8;  f.v[1] = 4;  f.v[2] = 0;  	f.smGroup = 1;  f.flags = 65536; mesh.faces[7] = f;
	f.v[0] = 10;  f.v[1] = 8;  f.v[2] = 0;  	f.smGroup = 1;  f.flags = 65536; mesh.faces[8] = f;
	f.v[0] = 11;  f.v[1] = 10;  f.v[2] = 0;  	f.smGroup = 1;  f.flags = 65541; mesh.faces[9] = f;
	f.v[0] = 0;  f.v[1] = 1;  f.v[2] = 13;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[10] = f;
	f.v[0] = 13;  f.v[1] = 12;  f.v[2] = 0;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[11] = f;
	f.v[0] = 1;  f.v[1] = 2;  f.v[2] = 14;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[12] = f;
	f.v[0] = 14;  f.v[1] = 13;  f.v[2] = 1;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[13] = f;
	f.v[0] = 2;  f.v[1] = 3;  f.v[2] = 15;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[14] = f;
	f.v[0] = 15;  f.v[1] = 14;  f.v[2] = 2;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[15] = f;
	f.v[0] = 3;  f.v[1] = 4;  f.v[2] = 16;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[16] = f;
	f.v[0] = 16;  f.v[1] = 15;  f.v[2] = 3;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[17] = f;
	f.v[0] = 4;  f.v[1] = 5;  f.v[2] = 17;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[18] = f;
	f.v[0] = 17;  f.v[1] = 16;  f.v[2] = 4;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[19] = f;
	f.v[0] = 6;  f.v[1] = 7;  f.v[2] = 19;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[20] = f;
	f.v[0] = 19;  f.v[1] = 18;  f.v[2] = 6;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[21] = f;
	f.v[0] = 8;  f.v[1] = 9;  f.v[2] = 21;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[22] = f;
	f.v[0] = 21;  f.v[1] = 20;  f.v[2] = 8;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[23] = f;
	f.v[0] = 10;  f.v[1] = 11;  f.v[2] = 23;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[24] = f;
	f.v[0] = 23;  f.v[1] = 22;  f.v[2] = 10;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[25] = f;
	f.v[0] = 11;  f.v[1] = 0;  f.v[2] = 12;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[26] = f;
	f.v[0] = 12;  f.v[1] = 23;  f.v[2] = 11;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[27] = f;
	f.v[0] = 12;  f.v[1] = 13;  f.v[2] = 14;  	f.smGroup = 1;  f.flags = 3; mesh.faces[28] = f;
	f.v[0] = 14;  f.v[1] = 15;  f.v[2] = 16;  	f.smGroup = 1;  f.flags = 3; mesh.faces[29] = f;
	f.v[0] = 16;  f.v[1] = 17;  f.v[2] = 18;  	f.smGroup = 1;  f.flags = 3; mesh.faces[30] = f;
	f.v[0] = 14;  f.v[1] = 16;  f.v[2] = 18;  	f.smGroup = 1;  f.flags = 0; mesh.faces[31] = f;
	f.v[0] = 18;  f.v[1] = 19;  f.v[2] = 20;  	f.smGroup = 1;  f.flags = 3; mesh.faces[32] = f;
	f.v[0] = 20;  f.v[1] = 21;  f.v[2] = 22;  	f.smGroup = 1;  f.flags = 3; mesh.faces[33] = f;
	f.v[0] = 18;  f.v[1] = 20;  f.v[2] = 22;  	f.smGroup = 1;  f.flags = 0; mesh.faces[34] = f;
	f.v[0] = 14;  f.v[1] = 18;  f.v[2] = 22;  	f.smGroup = 1;  f.flags = 0; mesh.faces[35] = f;
	f.v[0] = 12;  f.v[1] = 14;  f.v[2] = 22;  	f.smGroup = 1;  f.flags = 0; mesh.faces[36] = f;
	f.v[0] = 23;  f.v[1] = 12;  f.v[2] = 22;  	f.smGroup = 1;  f.flags = 5; mesh.faces[37] = f;
	f.v[0] = 33;  f.v[1] = 32;  f.v[2] = 31;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[38] = f;
	f.v[0] = 31;  f.v[1] = 30;  f.v[2] = 29;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[39] = f;
	f.v[0] = 29;  f.v[1] = 28;  f.v[2] = 27;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[40] = f;
	f.v[0] = 31;  f.v[1] = 29;  f.v[2] = 27;  	f.smGroup = 1;  f.flags = 65536; mesh.faces[41] = f;
	f.v[0] = 27;  f.v[1] = 26;  f.v[2] = 25;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[42] = f;
	f.v[0] = 25;  f.v[1] = 24;  f.v[2] = 5;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[43] = f;
	f.v[0] = 27;  f.v[1] = 25;  f.v[2] = 5;  	f.smGroup = 1;  f.flags = 65536; mesh.faces[44] = f;
	f.v[0] = 31;  f.v[1] = 27;  f.v[2] = 5;  	f.smGroup = 1;  f.flags = 65536; mesh.faces[45] = f;
	f.v[0] = 33;  f.v[1] = 31;  f.v[2] = 5;  	f.smGroup = 1;  f.flags = 65536; mesh.faces[46] = f;
	f.v[0] = 6;  f.v[1] = 33;  f.v[2] = 5;  	f.smGroup = 1;  f.flags = 65541; mesh.faces[47] = f;
	f.v[0] = 5;  f.v[1] = 24;  f.v[2] = 34;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[48] = f;
	f.v[0] = 34;  f.v[1] = 17;  f.v[2] = 5;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[49] = f;
	f.v[0] = 24;  f.v[1] = 25;  f.v[2] = 35;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[50] = f;
	f.v[0] = 35;  f.v[1] = 34;  f.v[2] = 24;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[51] = f;
	f.v[0] = 25;  f.v[1] = 26;  f.v[2] = 36;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[52] = f;
	f.v[0] = 36;  f.v[1] = 35;  f.v[2] = 25;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[53] = f;
	f.v[0] = 26;  f.v[1] = 27;  f.v[2] = 37;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[54] = f;
	f.v[0] = 37;  f.v[1] = 36;  f.v[2] = 26;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[55] = f;
	f.v[0] = 27;  f.v[1] = 28;  f.v[2] = 38;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[56] = f;
	f.v[0] = 38;  f.v[1] = 37;  f.v[2] = 27;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[57] = f;
	f.v[0] = 28;  f.v[1] = 29;  f.v[2] = 39;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[58] = f;
	f.v[0] = 39;  f.v[1] = 38;  f.v[2] = 28;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[59] = f;
	f.v[0] = 29;  f.v[1] = 30;  f.v[2] = 40;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[60] = f;
	f.v[0] = 40;  f.v[1] = 39;  f.v[2] = 29;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[61] = f;
	f.v[0] = 30;  f.v[1] = 31;  f.v[2] = 41;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[62] = f;
	f.v[0] = 41;  f.v[1] = 40;  f.v[2] = 30;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[63] = f;
	f.v[0] = 31;  f.v[1] = 32;  f.v[2] = 42;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[64] = f;
	f.v[0] = 42;  f.v[1] = 41;  f.v[2] = 31;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[65] = f;
	f.v[0] = 33;  f.v[1] = 6;  f.v[2] = 18;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[66] = f;
	f.v[0] = 18;  f.v[1] = 43;  f.v[2] = 33;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[67] = f;
	f.v[0] = 17;  f.v[1] = 34;  f.v[2] = 35;  	f.smGroup = 1;  f.flags = 3; mesh.faces[68] = f;
	f.v[0] = 35;  f.v[1] = 36;  f.v[2] = 37;  	f.smGroup = 1;  f.flags = 3; mesh.faces[69] = f;
	f.v[0] = 37;  f.v[1] = 38;  f.v[2] = 39;  	f.smGroup = 1;  f.flags = 3; mesh.faces[70] = f;
	f.v[0] = 35;  f.v[1] = 37;  f.v[2] = 39;  	f.smGroup = 1;  f.flags = 0; mesh.faces[71] = f;
	f.v[0] = 39;  f.v[1] = 40;  f.v[2] = 41;  	f.smGroup = 1;  f.flags = 3; mesh.faces[72] = f;
	f.v[0] = 41;  f.v[1] = 42;  f.v[2] = 43;  	f.smGroup = 1;  f.flags = 3; mesh.faces[73] = f;
	f.v[0] = 39;  f.v[1] = 41;  f.v[2] = 43;  	f.smGroup = 1;  f.flags = 0; mesh.faces[74] = f;
	f.v[0] = 35;  f.v[1] = 39;  f.v[2] = 43;  	f.smGroup = 1;  f.flags = 0; mesh.faces[75] = f;
	f.v[0] = 17;  f.v[1] = 35;  f.v[2] = 43;  	f.smGroup = 1;  f.flags = 0; mesh.faces[76] = f;
	f.v[0] = 18;  f.v[1] = 17;  f.v[2] = 43;  	f.smGroup = 1;  f.flags = 5; mesh.faces[77] = f;
	f.v[0] = 47;  f.v[1] = 45;  f.v[2] = 44;  	f.smGroup = 4;  f.flags = 65539; mesh.faces[78] = f;
	f.v[0] = 44;  f.v[1] = 46;  f.v[2] = 47;  	f.smGroup = 4;  f.flags = 65539; mesh.faces[79] = f;
	f.v[0] = 52;  f.v[1] = 53;  f.v[2] = 50;  	f.smGroup = 16;  f.flags = 196611; mesh.faces[80] = f;
	f.v[0] = 50;  f.v[1] = 51;  f.v[2] = 52;  	f.smGroup = 16;  f.flags = 196611; mesh.faces[81] = f;
	f.v[0] = 56;  f.v[1] = 57;  f.v[2] = 54;  	f.smGroup = 64;  f.flags = 131075; mesh.faces[82] = f;
	f.v[0] = 54;  f.v[1] = 55;  f.v[2] = 56;  	f.smGroup = 64;  f.flags = 131075; mesh.faces[83] = f;
	f.v[0] = 51;  f.v[1] = 50;  f.v[2] = 45;  	f.smGroup = 0;  f.flags = 3; mesh.faces[84] = f;
	f.v[0] = 45;  f.v[1] = 47;  f.v[2] = 51;  	f.smGroup = 0;  f.flags = 3; mesh.faces[85] = f;
	f.v[0] = 55;  f.v[1] = 54;  f.v[2] = 46;  	f.smGroup = 0;  f.flags = 3; mesh.faces[86] = f;
	f.v[0] = 46;  f.v[1] = 44;  f.v[2] = 55;  	f.smGroup = 0;  f.flags = 3; mesh.faces[87] = f;
	f.v[0] = 57;  f.v[1] = 56;  f.v[2] = 48;  	f.smGroup = 0;  f.flags = 3; mesh.faces[88] = f;
	f.v[0] = 48;  f.v[1] = 49;  f.v[2] = 57;  	f.smGroup = 0;  f.flags = 3; mesh.faces[89] = f;
	f.v[0] = 52;  f.v[1] = 49;  f.v[2] = 48;  	f.smGroup = 0;  f.flags = 3; mesh.faces[90] = f;
	f.v[0] = 48;  f.v[1] = 53;  f.v[2] = 52;  	f.smGroup = 0;  f.flags = 3; mesh.faces[91] = f;
	f.v[0] = 56;  f.v[1] = 55;  f.v[2] = 44;  	f.smGroup = 0;  f.flags = 3; mesh.faces[92] = f;
	f.v[0] = 56;  f.v[1] = 44;  f.v[2] = 45;  	f.smGroup = 0;  f.flags = 2; mesh.faces[93] = f;
	f.v[0] = 56;  f.v[1] = 45;  f.v[2] = 50;  	f.smGroup = 0;  f.flags = 2; mesh.faces[94] = f;
	f.v[0] = 56;  f.v[1] = 50;  f.v[2] = 53;  	f.smGroup = 0;  f.flags = 2; mesh.faces[95] = f;
	f.v[0] = 48;  f.v[1] = 56;  f.v[2] = 53;  	f.smGroup = 0;  f.flags = 5; mesh.faces[96] = f;
	f.v[0] = 51;  f.v[1] = 47;  f.v[2] = 46;  	f.smGroup = 0;  f.flags = 3; mesh.faces[97] = f;
	f.v[0] = 51;  f.v[1] = 46;  f.v[2] = 54;  	f.smGroup = 0;  f.flags = 2; mesh.faces[98] = f;
	f.v[0] = 51;  f.v[1] = 54;  f.v[2] = 57;  	f.smGroup = 0;  f.flags = 2; mesh.faces[99] = f;
	f.v[0] = 51;  f.v[1] = 57;  f.v[2] = 49;  	f.smGroup = 0;  f.flags = 2; mesh.faces[100] = f;
	f.v[0] = 52;  f.v[1] = 51;  f.v[2] = 49;  	f.smGroup = 0;  f.flags = 5; mesh.faces[101] = f;
	f.v[0] = 22;  f.v[1] = 21;  f.v[2] = 9;  	f.smGroup = 0;  f.flags = 3; mesh.faces[102] = f;
	f.v[0] = 9;  f.v[1] = 10;  f.v[2] = 22;  	f.smGroup = 0;  f.flags = 3; mesh.faces[103] = f;
	f.v[0] = 60;  f.v[1] = 59;  f.v[2] = 58;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[104] = f;
	f.v[0] = 58;  f.v[1] = 65;  f.v[2] = 64;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[105] = f;
	f.v[0] = 64;  f.v[1] = 63;  f.v[2] = 62;  	f.smGroup = 1;  f.flags = 65539; mesh.faces[106] = f;
	f.v[0] = 58;  f.v[1] = 64;  f.v[2] = 62;  	f.smGroup = 1;  f.flags = 65536; mesh.faces[107] = f;
	f.v[0] = 60;  f.v[1] = 58;  f.v[2] = 62;  	f.smGroup = 1;  f.flags = 65536; mesh.faces[108] = f;
	f.v[0] = 60;  f.v[1] = 62;  f.v[2] = 61;  	f.smGroup = 1;  f.flags = 65542; mesh.faces[109] = f;
	f.v[0] = 59;  f.v[1] = 60;  f.v[2] = 68;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[110] = f;
	f.v[0] = 59;  f.v[1] = 68;  f.v[2] = 67;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[111] = f;
	f.v[0] = 67;  f.v[1] = 66;  f.v[2] = 58;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[112] = f;
	f.v[0] = 59;  f.v[1] = 67;  f.v[2] = 58;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[113] = f;
	f.v[0] = 61;  f.v[1] = 62;  f.v[2] = 70;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[114] = f;
	f.v[0] = 61;  f.v[1] = 70;  f.v[2] = 69;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[115] = f;
	f.v[0] = 69;  f.v[1] = 68;  f.v[2] = 60;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[116] = f;
	f.v[0] = 61;  f.v[1] = 69;  f.v[2] = 60;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[117] = f;
	f.v[0] = 63;  f.v[1] = 64;  f.v[2] = 72;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[118] = f;
	f.v[0] = 63;  f.v[1] = 72;  f.v[2] = 71;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[119] = f;
	f.v[0] = 71;  f.v[1] = 70;  f.v[2] = 62;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[120] = f;
	f.v[0] = 63;  f.v[1] = 71;  f.v[2] = 62;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[121] = f;
	f.v[0] = 65;  f.v[1] = 58;  f.v[2] = 66;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[122] = f;
	f.v[0] = 65;  f.v[1] = 66;  f.v[2] = 73;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[123] = f;
	f.v[0] = 73;  f.v[1] = 72;  f.v[2] = 64;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[124] = f;
	f.v[0] = 65;  f.v[1] = 73;  f.v[2] = 64;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[125] = f;
	f.v[0] = 67;  f.v[1] = 68;  f.v[2] = 76;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[126] = f;
	f.v[0] = 67;  f.v[1] = 76;  f.v[2] = 75;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[127] = f;
	f.v[0] = 75;  f.v[1] = 74;  f.v[2] = 66;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[128] = f;
	f.v[0] = 67;  f.v[1] = 75;  f.v[2] = 66;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[129] = f;
	f.v[0] = 69;  f.v[1] = 70;  f.v[2] = 78;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[130] = f;
	f.v[0] = 69;  f.v[1] = 78;  f.v[2] = 77;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[131] = f;
	f.v[0] = 77;  f.v[1] = 76;  f.v[2] = 68;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[132] = f;
	f.v[0] = 69;  f.v[1] = 77;  f.v[2] = 68;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[133] = f;
	f.v[0] = 71;  f.v[1] = 72;  f.v[2] = 80;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[134] = f;
	f.v[0] = 71;  f.v[1] = 80;  f.v[2] = 79;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[135] = f;
	f.v[0] = 79;  f.v[1] = 78;  f.v[2] = 70;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[136] = f;
	f.v[0] = 71;  f.v[1] = 79;  f.v[2] = 70;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[137] = f;
	f.v[0] = 73;  f.v[1] = 66;  f.v[2] = 74;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[138] = f;
	f.v[0] = 73;  f.v[1] = 74;  f.v[2] = 81;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[139] = f;
	f.v[0] = 81;  f.v[1] = 80;  f.v[2] = 72;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[140] = f;
	f.v[0] = 73;  f.v[1] = 81;  f.v[2] = 72;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[141] = f;
	f.v[0] = 86;  f.v[1] = 87;  f.v[2] = 88;  	f.smGroup = 1;  f.flags = 3; mesh.faces[142] = f;
	f.v[0] = 88;  f.v[1] = 89;  f.v[2] = 82;  	f.smGroup = 1;  f.flags = 3; mesh.faces[143] = f;
	f.v[0] = 82;  f.v[1] = 83;  f.v[2] = 84;  	f.smGroup = 1;  f.flags = 3; mesh.faces[144] = f;
	f.v[0] = 88;  f.v[1] = 82;  f.v[2] = 84;  	f.smGroup = 1;  f.flags = 0; mesh.faces[145] = f;
	f.v[0] = 86;  f.v[1] = 88;  f.v[2] = 84;  	f.smGroup = 1;  f.flags = 0; mesh.faces[146] = f;
	f.v[0] = 86;  f.v[1] = 84;  f.v[2] = 85;  	f.smGroup = 1;  f.flags = 6; mesh.faces[147] = f;
	f.v[0] = 75;  f.v[1] = 76;  f.v[2] = 84;  	f.smGroup = 0;  f.flags = 3; mesh.faces[148] = f;
	f.v[0] = 75;  f.v[1] = 84;  f.v[2] = 83;  	f.smGroup = 0;  f.flags = 2; mesh.faces[149] = f;
	f.v[0] = 83;  f.v[1] = 82;  f.v[2] = 74;  	f.smGroup = 0;  f.flags = 3; mesh.faces[150] = f;
	f.v[0] = 75;  f.v[1] = 83;  f.v[2] = 74;  	f.smGroup = 0;  f.flags = 4; mesh.faces[151] = f;
	f.v[0] = 77;  f.v[1] = 78;  f.v[2] = 86;  	f.smGroup = 0;  f.flags = 3; mesh.faces[152] = f;
	f.v[0] = 77;  f.v[1] = 86;  f.v[2] = 85;  	f.smGroup = 0;  f.flags = 2; mesh.faces[153] = f;
	f.v[0] = 85;  f.v[1] = 84;  f.v[2] = 76;  	f.smGroup = 0;  f.flags = 3; mesh.faces[154] = f;
	f.v[0] = 77;  f.v[1] = 85;  f.v[2] = 76;  	f.smGroup = 0;  f.flags = 4; mesh.faces[155] = f;
	f.v[0] = 79;  f.v[1] = 80;  f.v[2] = 88;  	f.smGroup = 0;  f.flags = 3; mesh.faces[156] = f;
	f.v[0] = 79;  f.v[1] = 88;  f.v[2] = 87;  	f.smGroup = 0;  f.flags = 2; mesh.faces[157] = f;
	f.v[0] = 87;  f.v[1] = 86;  f.v[2] = 78;  	f.smGroup = 0;  f.flags = 3; mesh.faces[158] = f;
	f.v[0] = 79;  f.v[1] = 87;  f.v[2] = 78;  	f.smGroup = 0;  f.flags = 4; mesh.faces[159] = f;
	f.v[0] = 81;  f.v[1] = 74;  f.v[2] = 82;  	f.smGroup = 0;  f.flags = 3; mesh.faces[160] = f;
	f.v[0] = 81;  f.v[1] = 82;  f.v[2] = 89;  	f.smGroup = 0;  f.flags = 2; mesh.faces[161] = f;
	f.v[0] = 89;  f.v[1] = 88;  f.v[2] = 80;  	f.smGroup = 0;  f.flags = 3; mesh.faces[162] = f;
	f.v[0] = 81;  f.v[1] = 89;  f.v[2] = 80;  	f.smGroup = 0;  f.flags = 4; mesh.faces[163] = f;
	f.v[0] = 108;  f.v[1] = 125;  f.v[2] = 107;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[164] = f;
	f.v[0] = 108;  f.v[1] = 107;  f.v[2] = 90;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[165] = f;
	f.v[0] = 90;  f.v[1] = 91;  f.v[2] = 109;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[166] = f;
	f.v[0] = 108;  f.v[1] = 90;  f.v[2] = 109;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[167] = f;
	f.v[0] = 92;  f.v[1] = 93;  f.v[2] = 111;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[168] = f;
	f.v[0] = 92;  f.v[1] = 111;  f.v[2] = 110;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[169] = f;
	f.v[0] = 110;  f.v[1] = 109;  f.v[2] = 91;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[170] = f;
	f.v[0] = 92;  f.v[1] = 110;  f.v[2] = 91;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[171] = f;
	f.v[0] = 94;  f.v[1] = 95;  f.v[2] = 113;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[172] = f;
	f.v[0] = 94;  f.v[1] = 113;  f.v[2] = 112;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[173] = f;
	f.v[0] = 112;  f.v[1] = 111;  f.v[2] = 93;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[174] = f;
	f.v[0] = 94;  f.v[1] = 112;  f.v[2] = 93;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[175] = f;
	f.v[0] = 96;  f.v[1] = 97;  f.v[2] = 115;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[176] = f;
	f.v[0] = 96;  f.v[1] = 115;  f.v[2] = 114;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[177] = f;
	f.v[0] = 114;  f.v[1] = 113;  f.v[2] = 95;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[178] = f;
	f.v[0] = 96;  f.v[1] = 114;  f.v[2] = 95;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[179] = f;
	f.v[0] = 98;  f.v[1] = 99;  f.v[2] = 117;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[180] = f;
	f.v[0] = 98;  f.v[1] = 117;  f.v[2] = 116;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[181] = f;
	f.v[0] = 116;  f.v[1] = 115;  f.v[2] = 97;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[182] = f;
	f.v[0] = 98;  f.v[1] = 116;  f.v[2] = 97;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[183] = f;
	f.v[0] = 100;  f.v[1] = 101;  f.v[2] = 119;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[184] = f;
	f.v[0] = 100;  f.v[1] = 119;  f.v[2] = 118;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[185] = f;
	f.v[0] = 118;  f.v[1] = 117;  f.v[2] = 99;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[186] = f;
	f.v[0] = 100;  f.v[1] = 118;  f.v[2] = 99;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[187] = f;
	f.v[0] = 102;  f.v[1] = 103;  f.v[2] = 121;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[188] = f;
	f.v[0] = 102;  f.v[1] = 121;  f.v[2] = 120;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[189] = f;
	f.v[0] = 120;  f.v[1] = 119;  f.v[2] = 101;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[190] = f;
	f.v[0] = 102;  f.v[1] = 120;  f.v[2] = 101;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[191] = f;
	f.v[0] = 104;  f.v[1] = 105;  f.v[2] = 123;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[192] = f;
	f.v[0] = 104;  f.v[1] = 123;  f.v[2] = 122;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[193] = f;
	f.v[0] = 122;  f.v[1] = 121;  f.v[2] = 103;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[194] = f;
	f.v[0] = 104;  f.v[1] = 122;  f.v[2] = 103;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[195] = f;
	f.v[0] = 106;  f.v[1] = 107;  f.v[2] = 125;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[196] = f;
	f.v[0] = 106;  f.v[1] = 125;  f.v[2] = 124;  	f.smGroup = 8;  f.flags = 131074; mesh.faces[197] = f;
	f.v[0] = 124;  f.v[1] = 123;  f.v[2] = 105;  	f.smGroup = 8;  f.flags = 131075; mesh.faces[198] = f;
	f.v[0] = 106;  f.v[1] = 124;  f.v[2] = 105;  	f.smGroup = 8;  f.flags = 131076; mesh.faces[199] = f;
	f.v[0] = 110;  f.v[1] = 111;  f.v[2] = 129;  	f.smGroup = 0;  f.flags = 3; mesh.faces[200] = f;
	f.v[0] = 110;  f.v[1] = 129;  f.v[2] = 128;  	f.smGroup = 0;  f.flags = 2; mesh.faces[201] = f;
	f.v[0] = 128;  f.v[1] = 127;  f.v[2] = 109;  	f.smGroup = 0;  f.flags = 3; mesh.faces[202] = f;
	f.v[0] = 110;  f.v[1] = 128;  f.v[2] = 109;  	f.smGroup = 0;  f.flags = 4; mesh.faces[203] = f;
	f.v[0] = 112;  f.v[1] = 113;  f.v[2] = 131;  	f.smGroup = 0;  f.flags = 3; mesh.faces[204] = f;
	f.v[0] = 112;  f.v[1] = 131;  f.v[2] = 130;  	f.smGroup = 0;  f.flags = 2; mesh.faces[205] = f;
	f.v[0] = 130;  f.v[1] = 129;  f.v[2] = 111;  	f.smGroup = 0;  f.flags = 3; mesh.faces[206] = f;
	f.v[0] = 112;  f.v[1] = 130;  f.v[2] = 111;  	f.smGroup = 0;  f.flags = 4; mesh.faces[207] = f;
	f.v[0] = 114;  f.v[1] = 115;  f.v[2] = 133;  	f.smGroup = 0;  f.flags = 3; mesh.faces[208] = f;
	f.v[0] = 114;  f.v[1] = 133;  f.v[2] = 132;  	f.smGroup = 0;  f.flags = 2; mesh.faces[209] = f;
	f.v[0] = 132;  f.v[1] = 131;  f.v[2] = 113;  	f.smGroup = 0;  f.flags = 3; mesh.faces[210] = f;
	f.v[0] = 114;  f.v[1] = 132;  f.v[2] = 113;  	f.smGroup = 0;  f.flags = 4; mesh.faces[211] = f;
	f.v[0] = 116;  f.v[1] = 117;  f.v[2] = 135;  	f.smGroup = 0;  f.flags = 3; mesh.faces[212] = f;
	f.v[0] = 116;  f.v[1] = 135;  f.v[2] = 134;  	f.smGroup = 0;  f.flags = 2; mesh.faces[213] = f;
	f.v[0] = 134;  f.v[1] = 133;  f.v[2] = 115;  	f.smGroup = 0;  f.flags = 3; mesh.faces[214] = f;
	f.v[0] = 116;  f.v[1] = 134;  f.v[2] = 115;  	f.smGroup = 0;  f.flags = 4; mesh.faces[215] = f;
	f.v[0] = 118;  f.v[1] = 119;  f.v[2] = 137;  	f.smGroup = 0;  f.flags = 3; mesh.faces[216] = f;
	f.v[0] = 118;  f.v[1] = 137;  f.v[2] = 136;  	f.smGroup = 0;  f.flags = 2; mesh.faces[217] = f;
	f.v[0] = 136;  f.v[1] = 135;  f.v[2] = 117;  	f.smGroup = 0;  f.flags = 3; mesh.faces[218] = f;
	f.v[0] = 118;  f.v[1] = 136;  f.v[2] = 117;  	f.smGroup = 0;  f.flags = 4; mesh.faces[219] = f;
	f.v[0] = 120;  f.v[1] = 121;  f.v[2] = 139;  	f.smGroup = 0;  f.flags = 3; mesh.faces[220] = f;
	f.v[0] = 120;  f.v[1] = 139;  f.v[2] = 138;  	f.smGroup = 0;  f.flags = 2; mesh.faces[221] = f;
	f.v[0] = 138;  f.v[1] = 137;  f.v[2] = 119;  	f.smGroup = 0;  f.flags = 3; mesh.faces[222] = f;
	f.v[0] = 120;  f.v[1] = 138;  f.v[2] = 119;  	f.smGroup = 0;  f.flags = 4; mesh.faces[223] = f;
	f.v[0] = 122;  f.v[1] = 123;  f.v[2] = 141;  	f.smGroup = 0;  f.flags = 3; mesh.faces[224] = f;
	f.v[0] = 122;  f.v[1] = 141;  f.v[2] = 140;  	f.smGroup = 0;  f.flags = 2; mesh.faces[225] = f;
	f.v[0] = 140;  f.v[1] = 139;  f.v[2] = 121;  	f.smGroup = 0;  f.flags = 3; mesh.faces[226] = f;
	f.v[0] = 122;  f.v[1] = 140;  f.v[2] = 121;  	f.smGroup = 0;  f.flags = 4; mesh.faces[227] = f;
	f.v[0] = 124;  f.v[1] = 125;  f.v[2] = 143;  	f.smGroup = 0;  f.flags = 3; mesh.faces[228] = f;
	f.v[0] = 124;  f.v[1] = 143;  f.v[2] = 142;  	f.smGroup = 0;  f.flags = 2; mesh.faces[229] = f;
	f.v[0] = 142;  f.v[1] = 141;  f.v[2] = 123;  	f.smGroup = 0;  f.flags = 3; mesh.faces[230] = f;
	f.v[0] = 124;  f.v[1] = 142;  f.v[2] = 123;  	f.smGroup = 0;  f.flags = 4; mesh.faces[231] = f;
	f.v[0] = 108;  f.v[1] = 109;  f.v[2] = 127;  	f.smGroup = 0;  f.flags = 3; mesh.faces[232] = f;
	f.v[0] = 108;  f.v[1] = 127;  f.v[2] = 126;  	f.smGroup = 0;  f.flags = 2; mesh.faces[233] = f;
	f.v[0] = 126;  f.v[1] = 143;  f.v[2] = 125;  	f.smGroup = 0;  f.flags = 3; mesh.faces[234] = f;
	f.v[0] = 108;  f.v[1] = 126;  f.v[2] = 125;  	f.smGroup = 0;  f.flags = 4; mesh.faces[235] = f;
	f.v[0] = 128;  f.v[1] = 129;  f.v[2] = 147;  	f.smGroup = 0;  f.flags = 3; mesh.faces[236] = f;
	f.v[0] = 128;  f.v[1] = 147;  f.v[2] = 146;  	f.smGroup = 0;  f.flags = 2; mesh.faces[237] = f;
	f.v[0] = 146;  f.v[1] = 145;  f.v[2] = 127;  	f.smGroup = 0;  f.flags = 3; mesh.faces[238] = f;
	f.v[0] = 128;  f.v[1] = 146;  f.v[2] = 127;  	f.smGroup = 0;  f.flags = 4; mesh.faces[239] = f;
	f.v[0] = 130;  f.v[1] = 131;  f.v[2] = 149;  	f.smGroup = 0;  f.flags = 3; mesh.faces[240] = f;
	f.v[0] = 130;  f.v[1] = 149;  f.v[2] = 148;  	f.smGroup = 0;  f.flags = 2; mesh.faces[241] = f;
	f.v[0] = 148;  f.v[1] = 147;  f.v[2] = 129;  	f.smGroup = 0;  f.flags = 3; mesh.faces[242] = f;
	f.v[0] = 130;  f.v[1] = 148;  f.v[2] = 129;  	f.smGroup = 0;  f.flags = 4; mesh.faces[243] = f;
	f.v[0] = 132;  f.v[1] = 133;  f.v[2] = 151;  	f.smGroup = 0;  f.flags = 3; mesh.faces[244] = f;
	f.v[0] = 132;  f.v[1] = 151;  f.v[2] = 150;  	f.smGroup = 0;  f.flags = 2; mesh.faces[245] = f;
	f.v[0] = 150;  f.v[1] = 149;  f.v[2] = 131;  	f.smGroup = 0;  f.flags = 3; mesh.faces[246] = f;
	f.v[0] = 132;  f.v[1] = 150;  f.v[2] = 131;  	f.smGroup = 0;  f.flags = 4; mesh.faces[247] = f;
	f.v[0] = 134;  f.v[1] = 135;  f.v[2] = 153;  	f.smGroup = 0;  f.flags = 3; mesh.faces[248] = f;
	f.v[0] = 134;  f.v[1] = 153;  f.v[2] = 152;  	f.smGroup = 0;  f.flags = 2; mesh.faces[249] = f;
	f.v[0] = 152;  f.v[1] = 151;  f.v[2] = 133;  	f.smGroup = 0;  f.flags = 3; mesh.faces[250] = f;
	f.v[0] = 134;  f.v[1] = 152;  f.v[2] = 133;  	f.smGroup = 0;  f.flags = 4; mesh.faces[251] = f;
	f.v[0] = 136;  f.v[1] = 137;  f.v[2] = 155;  	f.smGroup = 0;  f.flags = 3; mesh.faces[252] = f;
	f.v[0] = 136;  f.v[1] = 155;  f.v[2] = 154;  	f.smGroup = 0;  f.flags = 2; mesh.faces[253] = f;
	f.v[0] = 154;  f.v[1] = 153;  f.v[2] = 135;  	f.smGroup = 0;  f.flags = 3; mesh.faces[254] = f;
	f.v[0] = 136;  f.v[1] = 154;  f.v[2] = 135;  	f.smGroup = 0;  f.flags = 4; mesh.faces[255] = f;
	f.v[0] = 138;  f.v[1] = 139;  f.v[2] = 157;  	f.smGroup = 0;  f.flags = 3; mesh.faces[256] = f;
	f.v[0] = 138;  f.v[1] = 157;  f.v[2] = 156;  	f.smGroup = 0;  f.flags = 2; mesh.faces[257] = f;
	f.v[0] = 156;  f.v[1] = 155;  f.v[2] = 137;  	f.smGroup = 0;  f.flags = 3; mesh.faces[258] = f;
	f.v[0] = 138;  f.v[1] = 156;  f.v[2] = 137;  	f.smGroup = 0;  f.flags = 4; mesh.faces[259] = f;
	f.v[0] = 140;  f.v[1] = 141;  f.v[2] = 159;  	f.smGroup = 0;  f.flags = 3; mesh.faces[260] = f;
	f.v[0] = 140;  f.v[1] = 159;  f.v[2] = 158;  	f.smGroup = 0;  f.flags = 2; mesh.faces[261] = f;
	f.v[0] = 158;  f.v[1] = 157;  f.v[2] = 139;  	f.smGroup = 0;  f.flags = 3; mesh.faces[262] = f;
	f.v[0] = 140;  f.v[1] = 158;  f.v[2] = 139;  	f.smGroup = 0;  f.flags = 4; mesh.faces[263] = f;
	f.v[0] = 142;  f.v[1] = 143;  f.v[2] = 161;  	f.smGroup = 0;  f.flags = 3; mesh.faces[264] = f;
	f.v[0] = 142;  f.v[1] = 161;  f.v[2] = 160;  	f.smGroup = 0;  f.flags = 2; mesh.faces[265] = f;
	f.v[0] = 160;  f.v[1] = 159;  f.v[2] = 141;  	f.smGroup = 0;  f.flags = 3; mesh.faces[266] = f;
	f.v[0] = 142;  f.v[1] = 160;  f.v[2] = 141;  	f.smGroup = 0;  f.flags = 4; mesh.faces[267] = f;
	f.v[0] = 126;  f.v[1] = 127;  f.v[2] = 145;  	f.smGroup = 0;  f.flags = 3; mesh.faces[268] = f;
	f.v[0] = 126;  f.v[1] = 145;  f.v[2] = 144;  	f.smGroup = 0;  f.flags = 2; mesh.faces[269] = f;
	f.v[0] = 144;  f.v[1] = 161;  f.v[2] = 143;  	f.smGroup = 0;  f.flags = 3; mesh.faces[270] = f;
	f.v[0] = 126;  f.v[1] = 144;  f.v[2] = 143;  	f.smGroup = 0;  f.flags = 4; mesh.faces[271] = f;
	f.v[0] = 165;  f.v[1] = 163;  f.v[2] = 162;  	f.smGroup = 4;  f.flags = 65539; mesh.faces[272] = f;
	f.v[0] = 162;  f.v[1] = 164;  f.v[2] = 165;  	f.smGroup = 4;  f.flags = 65539; mesh.faces[273] = f;
	f.v[0] = 169;  f.v[1] = 168;  f.v[2] = 166;  	f.smGroup = 2;  f.flags = 3; mesh.faces[274] = f;
	f.v[0] = 166;  f.v[1] = 167;  f.v[2] = 169;  	f.smGroup = 2;  f.flags = 3; mesh.faces[275] = f;
	f.v[0] = 167;  f.v[1] = 166;  f.v[2] = 162;  	f.smGroup = 8;  f.flags = 262147; mesh.faces[276] = f;
	f.v[0] = 162;  f.v[1] = 163;  f.v[2] = 167;  	f.smGroup = 8;  f.flags = 262147; mesh.faces[277] = f;
	f.v[0] = 169;  f.v[1] = 167;  f.v[2] = 163;  	f.smGroup = 16;  f.flags = 196611; mesh.faces[278] = f;
	f.v[0] = 163;  f.v[1] = 165;  f.v[2] = 169;  	f.smGroup = 16;  f.flags = 196611; mesh.faces[279] = f;
	f.v[0] = 168;  f.v[1] = 169;  f.v[2] = 165;  	f.smGroup = 32;  f.flags = 327683; mesh.faces[280] = f;
	f.v[0] = 165;  f.v[1] = 164;  f.v[2] = 168;  	f.smGroup = 32;  f.flags = 327683; mesh.faces[281] = f;
	f.v[0] = 166;  f.v[1] = 168;  f.v[2] = 164;  	f.smGroup = 64;  f.flags = 131075; mesh.faces[282] = f;
	f.v[0] = 164;  f.v[1] = 162;  f.v[2] = 166;  	f.smGroup = 64;  f.flags = 131075; mesh.faces[283] = f;

	mesh.buildNormals();
	mesh.EnableEdgeList(1);
	meshBuilt = 1;
	}

void SimpleCamera::UpdateKeyBrackets(TimeValue t) {
	fovSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_FOV,t));
	lensSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_FOV,t));
	hitherSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_HITHER,t));
	yonSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_YON,t));
	envNearSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_NRANGE,t));
	envFarSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_FRANGE,t));
	}

void SimpleCamera::UpdateUI(TimeValue t)
{
   if ( hSimpleCamParams && !waitPostLoad && 
        DLGetWindowLongPtr<SimpleCamera*>(hSimpleCamParams)==this && pblock )
	{
		fovSpin->SetValue( RadToDeg(WFOVtoCurFOV(GetFOV(t))), FALSE );
		lensSpin->SetValue(FOVtoMM(GetFOV(t)), FALSE);
		hitherSpin->SetValue( GetClipDist(t, CAM_HITHER_CLIP), FALSE );
		yonSpin->SetValue( GetClipDist(t, CAM_YON_CLIP), FALSE );
		envNearSpin->SetValue( GetEnvRange(t, ENV_NEAR_RANGE), FALSE );
		envFarSpin->SetValue( GetEnvRange(t, ENV_FAR_RANGE), FALSE );
		UpdateKeyBrackets(t);

		CheckDlgButton(hSimpleCamParams, IDC_ENABLE_MP_EFFECT, GetMultiPassEffectEnabled(t) );
		CheckDlgButton(hSimpleCamParams, IDC_MP_EFFECT_REFFECT_PER_PASS, GetMPEffect_REffectPerPass() );

		tdistSpin->SetValue( GetTDist(t), FALSE );

		// DS 8/28/00
		SendMessage( GetDlgItem(hSimpleCamParams, IDC_CAM_TYPE), CB_SETCURSEL, hasTarget, (LPARAM)0 );
	}
}

#define CAMERA_VERSION 5	// current version // mjm - 07.17.00
#define DOF_VERSION 2		// current version // nac - 12.08.00

#define CAMERA_PBLOCK_COUNT	9

static ParamBlockDescID descV0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE, 2 } };	// TDIST

static ParamBlockDescID descV1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE, 2 },		// TDIST
	{ TYPE_FLOAT, NULL, TRUE, 3 },		// HITHER
	{ TYPE_FLOAT, NULL, TRUE, 4 } };	// YON

static ParamBlockDescID descV2[] = {
	{ TYPE_FLOAT, NULL, TRUE, 1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE, 2 },		// TDIST
	{ TYPE_FLOAT, NULL, TRUE, 3 },		// HITHER
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// YON
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// NEAR ENV RANGE
	{ TYPE_FLOAT, NULL, TRUE, 6 } };	// FAR ENV RANGE

static ParamBlockDescID descV3[] = {
	{ TYPE_FLOAT, NULL, TRUE,  1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE,  2 },		// TDIST
	{ TYPE_FLOAT, NULL, TRUE,  3 },		// HITHER
	{ TYPE_FLOAT, NULL, TRUE,  4 },		// YON
	{ TYPE_FLOAT, NULL, TRUE,  5 },		// NEAR ENV RANGE
	{ TYPE_FLOAT, NULL, TRUE,  6 },		// FAR ENV RANGE
	{ TYPE_BOOL,  NULL, FALSE, 7 } };	// MULTI PASS EFFECT - RENDER EFFECTS PER PASS

static ParamBlockDescID descV4[] = {
	{ TYPE_FLOAT, NULL, TRUE,  1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE,  2 },		// TDIST
	{ TYPE_FLOAT, NULL, TRUE,  3 },		// HITHER
	{ TYPE_FLOAT, NULL, TRUE,  4 },		// YON
	{ TYPE_FLOAT, NULL, TRUE,  5 },		// NEAR ENV RANGE
	{ TYPE_FLOAT, NULL, TRUE,  6 },		// FAR ENV RANGE
	{ TYPE_BOOL,  NULL, FALSE, 7 },		// MULTI PASS EFFECT ENABLE
	{ TYPE_BOOL,  NULL, FALSE, 8 } };	// MULTI PASS EFFECT - RENDER EFFECTS PER PASS
static ParamBlockDescID descV5[] = {
	{ TYPE_FLOAT, NULL, TRUE,  1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE,  2 },		// TDIST
	{ TYPE_FLOAT, NULL, TRUE,  3 },		// HITHER
	{ TYPE_FLOAT, NULL, TRUE,  4 },		// YON
	{ TYPE_FLOAT, NULL, TRUE,  5 },		// NEAR ENV RANGE
	{ TYPE_FLOAT, NULL, TRUE,  6 },		// FAR ENV RANGE
	{ TYPE_BOOL,  NULL, FALSE, 7 },		// MULTI PASS EFFECT ENABLE
	{ TYPE_BOOL,  NULL, FALSE, 8 }, 	// MULTI PASS EFFECT - RENDER EFFECTS PER PASS
	{ TYPE_INT,   NULL, FALSE, 9 } };	// FOV TYPE

static ParamBlockDescID dofV0[] = {
	{ TYPE_BOOL,  NULL, TRUE,  1 },		// ENABLE
	{ TYPE_FLOAT, NULL, TRUE, 2 } };	// FSTOP

static ParamBlockDescID dofV1[] = {
	{ TYPE_BOOL,  NULL, TRUE,  1 },		// ENABLE
	{ TYPE_FLOAT, NULL, TRUE, 2 } };	// FSTOP

SimpleCamera::SimpleCamera(int look) : targDist(0.0f), mpIMultiPassCameraEffect(NULL), mStereoCameraCallBack(NULL)
{
	// Disable hold, macro and ref msgs.
	SuspendAll	xDisableNotifs(TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
	hasTarget = look;
	depthOfFieldPB = NULL;

	pblock = NULL;
	ReplaceReference( 0, CreateParameterBlock( descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION ) );

	SetFOV( TimeValue(0), dlgFOV );
	SetFOVType(dlgFOVType);
	SetTDist( TimeValue(0), dlgTDist );
	SetClipDist( TimeValue(0), CAM_HITHER_CLIP, dlgHither );
	SetClipDist( TimeValue(0), CAM_YON_CLIP, dlgYon );
	SetEnvRange( TimeValue(0), ENV_NEAR_RANGE, dlgNearRange );
	SetEnvRange( TimeValue(0), ENV_FAR_RANGE, dlgFarRange );
	ReplaceReference( MP_EFFECT_REF, CreateDefaultMultiPassEffect(this) );

	SetMultiPassEffectEnabled(TimeValue(0), dlgMultiPassEffectEnable);
	SetMPEffect_REffectPerPass(dlgMPEffect_REffectPerPass);

	enable = 0;
	coneState = dlgShowCone;
	rangeDisplay = dlgRangeDisplay;
	horzLineState = dlgShowHorzLine;
	manualClip = dlgClip;
	isOrtho = dlgIsOrtho;

	BuildMesh();
}

BOOL SimpleCamera::IsCompatibleRenderer()
{
	return FALSE;
}

BOOL SimpleCamera::SetFOVControl(Control *c)
	{
	pblock->SetController(PB_FOV,c);
	return TRUE;
	}
Control * SimpleCamera::GetFOVControl() {
	return 	pblock->GetController(PB_FOV);
	}

void SimpleCamera::SetConeState(int s) {
	coneState = s;
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

void SimpleCamera::SetHorzLineState(int s) {
	horzLineState = s;
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

//--------------------------------------------

static INode* FindNodeRef(ReferenceTarget *rt);

static INode* GetNodeRef(ReferenceMaker *rm) {
	if (rm->SuperClassID()==BASENODE_CLASS_ID) return (INode *)rm;
	else return rm->IsRefTarget()?FindNodeRef((ReferenceTarget *)rm):NULL;
	}

static INode* FindNodeRef(ReferenceTarget *rt) {
	DependentIterator di(rt);
	ReferenceMaker *rm = NULL;
	INode *nd = NULL;
	while ((rm=di.Next()) != NULL) {	
		nd = GetNodeRef(rm);
		if (nd) return nd;
		}
	return NULL;
	}

//----------------------------------------------------------------

class SetCamTypeRest: public RestoreObj {
	public:
		SimpleCamera *theCam;
		int oldHasTarget;
		SetCamTypeRest(SimpleCamera *lt, int newt) {
			theCam = lt;
			oldHasTarget = lt->hasTarget;
			}
		~SetCamTypeRest() { }
		void Restore(int isUndo);
		void Redo();
		int Size() { return 1; }
		virtual TSTR Description() { return _T("Set Camera Type"); }
	};


void SetCamTypeRest::Restore(int isUndo) {
	theCam->hasTarget = oldHasTarget;
	theCam->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	theCam->NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED); // to redraw modifier stack
	}

void SetCamTypeRest::Redo() {
	theCam->hasTarget = !oldHasTarget;
	theCam->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	theCam->NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED); // to redraw modifier stack
	}



class ParamsRest: public RestoreObj {
	public:
		SimpleCamera *theCam;
		BOOL showit;
		ParamsRest(SimpleCamera *c, BOOL show) {
			theCam = c;
			showit = show;
			}
		void ParamsRest::Restore(int isUndo) {
			theCam->NotifyDependents(FOREVER, PART_OBJ, showit?REFMSG_END_MODIFY_PARAMS:REFMSG_BEGIN_MODIFY_PARAMS);
			}
		void ParamsRest::Redo() {
			theCam->NotifyDependents(FOREVER, PART_OBJ, showit?REFMSG_BEGIN_MODIFY_PARAMS:REFMSG_END_MODIFY_PARAMS);
			}
		int Size() { return 1; }
		virtual TSTR Description() { return _T("Camera Params"); }
	};


/*----------------------------------------------------------------*/

static ISubTargetCtrl* findSubCtrl(Control* ctrl, Control*& subCtrl)
{
	ISubTargetCtrl* assign = NULL;
	ISubTargetCtrl* next;
	Control* child;

	subCtrl = NULL;
	for ( next = GetSubTargetInterface(ctrl); next != NULL; next = GetSubTargetInterface(child)) {
		child = next->GetTMController();
		if (child == NULL)
			break;
		if (next->CanAssignTMController()) {
			assign = next;
			subCtrl = child;
		}
	}

	return assign;
}

static bool replaceSubLookatController(Control* old)
{
	Control* child = NULL;
	ISubTargetCtrl* assign = findSubCtrl(old, child);
	if (assign == NULL)
		return false;
	DbgAssert(assign->CanAssignTMController() && child != NULL);

	Control *tmc = NewDefaultMatrix3Controller();
	tmc->Copy(child); // doesn't copy rotation, only scale and position.
	assign->AssignTMController(tmc);

	return true;
}

static void clearTargetController(INode* node)
{
	Control* old = node->GetTMController();

	if (!replaceSubLookatController(old)) {
		Control *tmc = NewDefaultMatrix3Controller();
		tmc->Copy(old); // doesn't copy rotation, only scale and position.
		node->SetTMController(tmc);
	}
}

static bool replaceSubPRSController(Control* old, INode* targNode)
{
	Control* child = NULL;
	ISubTargetCtrl* assign = findSubCtrl(old, child);
	if (assign == NULL)
		return false;
	DbgAssert(assign->CanAssignTMController() && child != NULL);

	Control *laControl = CreateLookatControl();
	laControl->SetTarget(targNode);
	laControl->Copy(child);
	assign->AssignTMController(laControl);

	return true;
}

static void setTargetController(INode* node, INode* targNode)
{
	Control* old = node->GetTMController();
	if (!replaceSubPRSController(old, targNode)) {
		// assign lookat controller
		Control *laControl= CreateLookatControl();
		laControl->SetTarget(targNode);
		laControl->Copy(old);
		node->SetTMController(laControl);
	}
}

void SimpleCamera::SetType(int tp) {     
	if (hasTarget == tp) 
		return;

	Interface *iface = GetCOREInterface();
	TimeValue t = iface->GetTime();
	INode *nd = FindNodeRef(this);
	if (nd==NULL) 
		return;

	BOOL paramsShowing = FALSE;
	if (hSimpleCamParams && (currentEditCam == this)) { // LAM - 8/13/02 - defect 511609
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_END_MODIFY_PARAMS);
		paramsShowing = TRUE;
		if (theHold.Holding()) 
			theHold.Put(new ParamsRest(this,0));
		}

	if (theHold.Holding())
		theHold.Put(new SetCamTypeRest(this,tp));

	int oldtype = hasTarget;
	Interval v;
	float tdist = GetTDist(t,v);
	hasTarget = tp;
	bool bHandled = RaisePreCameraTargetChanged((tp!=0), tdist, t);

	if (bHandled)
	{	// if switch from target to free, update the target distance by using returned value from "RaisePreCameraTargetChanged" call.
		if (oldtype == TARGETED_CAMERA)
		{
			SetTDist(0,tdist);
		}
	}
	else //already be handled, so skip codes of creating/removing target node.
	{	
	if(oldtype==TARGETED_CAMERA) {
		tdist = targDist;
		// get rid of target, assign a PRS controller for all instances
		DependentIterator di(this);
		ReferenceMaker *rm = NULL;
		// iterate through the instances
		while ((rm=di.Next()) != NULL) {
			nd = GetNodeRef(rm);
			if (nd) {
				INode* tn = nd->GetTarget(); 
				Matrix3 tm = nd->GetNodeTM(0);
				if (tn) iface->DeleteNode(tn);  // JBW, make it safe if no target
				// CA - 03/02/05 - 621929: When a LinkTM controller was assigned, the
				// the code replaced the LinkTM controller with a PRS controller.
				// There were two problems with this. The FileLink information in the
				// LinkTM controller was lost, and the sub-controllers weren't
				// properly copied. clearTargetController does this correctly.
				clearTargetController(nd);  // doesn't copy rotation, only scale and position.
				nd->SetNodeTM(0,tm);		// preserve rotation if not animated at least
				SetTDist(0,tdist);	 //?? which one should this be for
				}
			}

		}
	else  {
		DependentIterator di(this);
		ReferenceMaker *rm = NULL;
		// iterate through the instances
		while ((rm=di.Next()) != NULL) {	
			nd = GetNodeRef(rm);
			if (nd) {
				// create a target, assign lookat controller
				Matrix3 targtm = nd->GetNodeTM(t);
				targtm.PreTranslate(Point3(0.0f,0.0f,-tdist));
				INode *targNode = iface->CreateObjectNode(new TargetObject);
				TSTR targName = nd->GetName();
				targName += GetString(IDS_DB_DOT_TARGET);
				targNode->SetName(targName);
				// CA - 03/02/05 - 621929: When a LinkTM controller was assigned, the
				// the code replaced the LinkTM controller with a Lookat controller.
				// There were two problems with this. The FileLink information in the
				// LinkTM controller was lost, and the sub-controllers weren't
				// properly copied. setTargetController does this correctly.
				setTargetController(nd, targNode);
				targNode->SetNodeTM(0, targtm); // set target transform after setting target controller so that the first update goes through
				targNode->SetIsTarget(1);   
				}
			}
		}
	}

	if (paramsShowing) {
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_BEGIN_MODIFY_PARAMS);
		if (theHold.Holding()) 
			theHold.Put(new ParamsRest(this,1));
		}

	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED); // to redraw modifier stack
	iface->RedrawViews(iface->GetTime());
	}


//---------------------------------------
void SimpleCamera::SetFOV(TimeValue t, float f) {
	pblock->SetValue( PB_FOV, t, f );
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

float SimpleCamera::GetFOV(TimeValue t,Interval& valid) {	
	float f;
	pblock->GetValue( PB_FOV, t, f, valid );
	if ( f < float(0) ) f = float(0);
	return f;
	}

void SimpleCamera::SetFOVType(int ft) 
{
	if(ft == GetFOVType()||ft < 0||ft > 2)
		return;
	pblock->SetValue( PB_FOV_TYPE, 0, ft );
	if (iFovType)
		iFovType->SetCurFlyOff(ft);
	if (fovSpin&&iObjParams)
		fovSpin->SetValue( RadToDeg(WFOVtoCurFOV(GetFOV(iObjParams->GetTime()))), FALSE );

	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

int SimpleCamera::GetFOVType()
{
	return pblock->GetInt( PB_FOV_TYPE );
}
void SimpleCamera::SetTDist(TimeValue t, float f) 
{
	static const float Epsilon = 1e-4F;
	if (abs(GetTDist(t) - f) < Epsilon)
	{
		return; // the same, no update.
	}

	bool bHandled = RaisePreSetCameraTargetDistance(f, t);
	if (bHandled)
		return;

// DS 8/15/00  begin
	if (hasTarget) {
		INode *nd = FindNodeRef(this);
		if (nd==NULL) return;
		INode* tn = nd->GetTarget(); 
		if (tn==NULL) return;
		Point3 ptarg;
		GetTargetPoint(t, nd, ptarg);
		Matrix3 tm = nd->GetObjectTM(t);
		float dtarg = Length(tm.GetTrans()-ptarg)/Length(tm.GetRow(2));

		Point3 delta(0,0,0);
		delta.z = dtarg-f;
		Matrix3 tmAxis = nd->GetNodeTM(t);
		tn->Move(t, tmAxis, delta);
		}
	else 
// DS 8/15/00  end
		{
		pblock->SetValue( PB_TDIST, t, f );
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
		}
	}

float SimpleCamera::GetTDist(TimeValue t,Interval& valid) {	
	float f;
	if (hasTarget) {
		if (mStereoCameraCallBack) // if part of stereo system, we need to ask the system to give back the correct target distance.
		{
			UpdateTargDistance(t, FindNodeRef(this));
		}
		return targDist;
		}
	else {
		pblock->GetValue( PB_TDIST, t, f, valid );
		if ( f < MIN_TDIST ) f = MIN_TDIST;
		return f;
		}
	}

void SimpleCamera::SetManualClip(int onOff)
{
	manualClip = onOff;
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

float SimpleCamera::GetClipDist(TimeValue t, int which, Interval &valid)
{
	float f;
	pblock->GetValue( PB_HITHER+which-1, t, f, valid );
	if ( f < MIN_CLIP ) f = MIN_CLIP;
	if ( f > MAX_CLIP ) f = MAX_CLIP;
	return f;
}

void SimpleCamera::SetClipDist(TimeValue t, int which, float f)
{
	pblock->SetValue( PB_HITHER+which-1, t, f );
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}


void SimpleCamera::SetEnvRange(TimeValue t, int which, float f)
{
	pblock->SetValue( PB_NRANGE+which, t, f );
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

float SimpleCamera::GetEnvRange(TimeValue t, int which, Interval &valid)
{
	float f;
	pblock->GetValue( PB_NRANGE+which, t, f, valid );
	return f;
}

void SimpleCamera::SetEnvDisplay(BOOL b, int notify) {
	rangeDisplay = b;
	if(notify)
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

void SimpleCamera::SetDOFEnable (TimeValue t, BOOL onOff) {
	}

BOOL SimpleCamera::GetDOFEnable(TimeValue t,Interval& valid) {	
	return FALSE;
	}

void SimpleCamera::SetDOFFStop (TimeValue t, float f) {
	}

float SimpleCamera::GetDOFFStop(TimeValue t,Interval& valid) {	
	return 2.0f;
	}

void SimpleCamera::SetMultiPassEffectEnabled(TimeValue t, BOOL enabled)
{
	pblock->SetValue(PB_MP_EFFECT_ENABLE, t, enabled);
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	if (hSimpleCamParams)
		EnableWindow( GetDlgItem(hSimpleCamParams, IDC_PREVIEW_MP_EFFECT), enabled );
}

BOOL SimpleCamera::GetMultiPassEffectEnabled(TimeValue t, Interval& valid)
{
	BOOL enabled;
	pblock->GetValue(PB_MP_EFFECT_ENABLE, t, enabled, valid);
	return enabled;
}

void SimpleCamera::SetMPEffect_REffectPerPass(BOOL enabled)
{
	pblock->SetValue(PB_MP_EFF_REND_EFF_PER_PASS, 0, enabled);
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

BOOL SimpleCamera::GetMPEffect_REffectPerPass()
{
	BOOL enabled;
	pblock->GetValue(PB_MP_EFF_REND_EFF_PER_PASS, 0, enabled, FOREVER);
	return enabled;
}

class MultiPassRestore: public RestoreObj {
	public:
		SimpleCamera *theCam;
		SingleRefMaker oldmp;
		SingleRefMaker newmp;
		MultiPassRestore(SimpleCamera *sc, IMultiPassCameraEffect *pOldEffect, IMultiPassCameraEffect *pNewEffect) {
			theCam = sc;
			oldmp.SetRef(pOldEffect);
			newmp.SetRef(pNewEffect);
			};
		void Restore(int isUndo) {
			theCam->SetIMultiPassCameraEffect(reinterpret_cast<IMultiPassCameraEffect *>(oldmp.GetRef()));
			}
		void Redo() {
			theCam->SetIMultiPassCameraEffect(reinterpret_cast<IMultiPassCameraEffect *>(newmp.GetRef()));
			}
		int Size() { return 1; }
		virtual TSTR Description() { return _T("Multi Pass Effect Change"); }
	};



// DS: 11/14/00 Made this undoable to fix Defect 268181.
void SimpleCamera::SetIMultiPassCameraEffect(IMultiPassCameraEffect *pIMultiPassCameraEffect)
{

	IMultiPassCameraEffect *pCurCameraEffect = GetIMultiPassCameraEffect();
	if (pCurCameraEffect == pIMultiPassCameraEffect) return; // LAM - 8/12/03
	if (theHold.Holding()) 
		theHold.Put( new MultiPassRestore(this, pCurCameraEffect, pIMultiPassCameraEffect));
	theHold.Suspend();
	if (iObjParams && pCurCameraEffect && (currentEditCam == this)) 
		pCurCameraEffect->EndEditParams(iObjParams, END_EDIT_REMOVEUI);
	ReplaceReference(MP_EFFECT_REF, pIMultiPassCameraEffect);
	if (iObjParams && pIMultiPassCameraEffect && (currentEditCam == this)) {
		pIMultiPassCameraEffect->BeginEditParams(iObjParams, inCreate ? BEGIN_EDIT_CREATE : 0, NULL);

		HWND hEffectList = GetDlgItem(hSimpleCamParams, IDC_MP_EFFECT);
		SendMessage(hEffectList, CB_RESETCONTENT, 0, (LPARAM)0);
		int numClasses = smCompatibleEffectList.Count();
		int selIndex = -1;
		for (int i=0; i<numClasses; i++)
		{
			int index = SendMessage( hEffectList, CB_ADDSTRING, 0, (LPARAM)smCompatibleEffectList[i]->CD()->ClassName() );
			if ( pIMultiPassCameraEffect && ( pIMultiPassCameraEffect->ClassID() == smCompatibleEffectList[i]->CD()->ClassID() ) )
			{
				selIndex = index;
			}
		}
		SendMessage(hEffectList, CB_SETCURSEL, selIndex, (LPARAM)0);
	}
	theHold.Resume();
}

IMultiPassCameraEffect *SimpleCamera::GetIMultiPassCameraEffect()
{
	return reinterpret_cast<IMultiPassCameraEffect *>( GetReference(MP_EFFECT_REF) );
}

// static methods
IMultiPassCameraEffect *SimpleCamera::CreateDefaultMultiPassEffect(CameraObject *pCameraObject)
{
	FindCompatibleMultiPassEffects(pCameraObject);

	IMultiPassCameraEffect *pIMultiPassCameraEffect = NULL;
	int numClasses = smCompatibleEffectList.Count();
	for (int i=0; i<numClasses; i++)
	{
		// MultiPassDOF camera effect is the default
		if ( smCompatibleEffectList[i]->CD()->ClassID() == Class_ID(0xd481815, 0x687d799c) )
		{
			pIMultiPassCameraEffect = reinterpret_cast<IMultiPassCameraEffect *>( smCompatibleEffectList[i]->CD()->Create(0) );
		}
	}
	return pIMultiPassCameraEffect;
}

void SimpleCamera::FindCompatibleMultiPassEffects(CameraObject *pCameraObject)
{
	smCompatibleEffectList.ZeroCount();
	SubClassList *subList = GetCOREInterface()->GetDllDir().ClassDir().GetClassList(MPASS_CAM_EFFECT_CLASS_ID);
	if (subList)
	{
		IMultiPassCameraEffect *pIMultiPassCameraEffect = NULL;
		int i = subList->GetFirst(ACC_PUBLIC);
		theHold.Suspend(); // LAM: added 11/12/00
		while (i >= 0)
		{			
			ClassEntry *c = &(*subList)[i];

			pIMultiPassCameraEffect = reinterpret_cast<IMultiPassCameraEffect *>( c->CD()->Create(0) );
			if ( pIMultiPassCameraEffect != NULL )
			{
				if ( pIMultiPassCameraEffect->IsCompatible(pCameraObject) )
				{
					smCompatibleEffectList.Append(1, &c);
				}
				pIMultiPassCameraEffect->MaybeAutoDelete();
			}

			i = subList->GetNext(ACC_PUBLIC);
		}
		theHold.Resume(); // LAM: added 11/12/00
	}
}

void SimpleCamera::SetOrtho(BOOL b) {
	isOrtho = b;
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}


class SCamCreateCallBack: public CreateMouseCallBack {
	public:
	SimpleCamera *ob;
		int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
		void SetObj(SimpleCamera *obj) { ob = obj; }
	};

int SCamCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	if (ob)
		ob->enable = 1;
	if (msg == MOUSE_FREEMOVE)
	{
			vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
	}
	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {

		// since we're now allowing the user to set the color of
		// the color wire-frames, we need to set the camera 
		// color to blue instead of the default random object color.
		if ( point == 0 )
		{
			ULONG handle;
			ob->NotifyDependents( FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE );
			INode * node;
			node = GetCOREInterface()->GetINodeByHandle( handle );
			if ( node ) 
			{
				Point3 color = GetUIColor( COLOR_CAMERA_OBJ );
				node->SetWireColor( RGB( color.x*255.0f, color.y*255.0f, color.z*255.0f ) );
			}
		}
		
		//mat[3] = vpt->GetPointOnCP(m);
		mat.SetTrans( vpt->SnapPoint(m,m,NULL,SNAP_IN_3D) );
		if (point==1 && msg==MOUSE_POINT) 
			return 0;
		}
	else
	if (msg == MOUSE_ABORT)
		return CREATE_ABORT;

	return TRUE;
	}

static SCamCreateCallBack sCamCreateCB;

SimpleCamera::~SimpleCamera() {
	sCamCreateCB.ob = NULL;	
	DeleteAllRefsFromMe();
	pblock = NULL;
	}

CreateMouseCallBack* SimpleCamera::GetCreateMouseCallBack() {
	sCamCreateCB.SetObj(this);
	return(&sCamCreateCB);
	}

static void RemoveScaling(Matrix3 &tm) {
	AffineParts ap;
	decomp_affine(tm, &ap);
	tm.IdentityMatrix();
	tm.SetRotate(ap.q);
	tm.SetTrans(ap.t);
	}

void SimpleCamera::GetMat(TimeValue t, INode* inode, ViewExp& vpt, Matrix3& tm) {
	if ( ! vpt.IsAlive() )
	{
		tm.Zero();
		return;
	}
	
	tm = inode->GetObjectTM(t);
	//tm.NoScale();
	RemoveScaling(tm);
	float scaleFactor = vpt.NonScalingObjectSize() * vpt.GetVPWorldWidth(tm.GetTrans()) / 360.0f;
	if (scaleFactor!=(float)1.0)
		tm.Scale(Point3(scaleFactor,scaleFactor,scaleFactor));
	}

void SimpleCamera::GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel )
	{
	box = mesh.getBoundingBox(tm);
	}

void SimpleCamera::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box ){
	if ( ! vpt || ! vpt->IsAlive() )
	{
		box.Init();
		return;
	}
	
	Matrix3 m = inode->GetObjectTM(t);
	Point3 pt;
	float scaleFactor = vpt->NonScalingObjectSize()*vpt->GetVPWorldWidth(m.GetTrans())/(float)360.0;
	box = mesh.getBoundingBox();
#if 0
	if (!hasTarget) {
		if (coneState) {
			Point3 q[4];
			GetConePoints(t,q,scaleFactor*FIXED_CONE_DIST);
			box.IncludePoints(q,4);
			}
		}
#endif
	box.Scale(scaleFactor);
	Point3 q[4];

	if (GetTargetPoint(t,inode,pt)){
		float d = Length(m.GetTrans()-pt)/Length(inode->GetObjectTM(t).GetRow(2));
		box += Point3(float(0),float(0),-d);
		if (coneState || (extDispFlags & EXT_DISP_ONLY_SELECTED)) {
			if(manualClip) {
				GetConePoints(t,q,GetClipDist(t,CAM_YON_CLIP));
				box.IncludePoints(q,4);
			}
			GetConePoints(t,q,d);
			box.IncludePoints(q,4);
			}
		}
#if 1
	else {
		if (coneState || (extDispFlags & EXT_DISP_ONLY_SELECTED)) {
			float d = GetTDist(t);
			box += Point3(float(0),float(0),-d);
			GetConePoints(t,q,d);
			box.IncludePoints(q,4);
			}
		}
#endif
	if( rangeDisplay) {
		Point3 q[4];
		float rad = max(GetEnvRange(t, ENV_NEAR_RANGE), GetEnvRange(t, ENV_FAR_RANGE));
		GetConePoints(t, q, rad);
		box.IncludePoints(q,4);
		}
	}

void SimpleCamera::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
	{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		box.Init();
		return;
	}
	
	int i,nv;
	Matrix3 tm;
	float dtarg = 0.0f;
	Point3 pt;
	GetMat(t,inode,*vpt,tm);
	nv = mesh.getNumVerts();
	box.Init();
	if(!(extDispFlags & EXT_DISP_ZOOM_EXT))
		for (i=0; i<nv; i++) 
			box += tm*mesh.getVert(i);
	else
		box += tm.GetTrans();

	tm = inode->GetObjectTM(t);
	if (hasTarget) {
		if (GetTargetPoint(t,inode,pt)) {
			dtarg = Length(tm.GetTrans()-pt)/Length(tm.GetRow(2));
			box += tm*Point3(float(0),float(0),-dtarg);
			}
		}
#if 0
	else dtarg = FIXED_CONE_DIST;
#else
	else dtarg = GetTDist(t);
#endif
	if (coneState || (extDispFlags & EXT_DISP_ONLY_SELECTED)) {
		Point3 q[4];
		if(manualClip) {
			GetConePoints(t,q,GetClipDist(t,CAM_YON_CLIP));
			box.IncludePoints(q,4,&tm);
			}
		GetConePoints(t,q,dtarg);
		box.IncludePoints(q,4,&tm);
		}
	if( rangeDisplay) {
		Point3 q[4];
		float rad = max(GetEnvRange(t, ENV_NEAR_RANGE), GetEnvRange(t, ENV_FAR_RANGE));
		GetConePoints(t, q,rad);
		box.IncludePoints(q,4,&tm);
		}
	}
 
void SimpleCamera::UpdateTargDistance(TimeValue t, INode* inode) {
	if (hasTarget /*&&hSimpleCamParams*/) {						// 5/29/01 2:31pm --MQM-- move this test case down, so we will still compute the target dist
		                                                        //   even for network rendering (when there is no hSimpleCamParams window
		Point3 pt,v[2];
		float distance;
		bool bHandled = RaisePreCameraTargetDistanceUpdated(distance, t);
		if (bHandled)
		{
			targDist = distance;
			if ( hSimpleCamParams && (currentEditCam == this))		// 5/29/01 2:31pm --MQM--, LAM - 8/13/02 - defect 511609
				tdistSpin->SetValue(distance, FALSE);
			return;
		}
		targDist = GetTargetDistance(t);//default to the current value in the PB2.
		if (GetTargetPoint(t,inode,pt)){
			Matrix3 tm = inode->GetObjectTM(t);
			float den = Length(tm.GetRow(2));
			targDist = (den!=0)?Length(tm.GetTrans()-pt)/den : 0.0f;

		// DS 8/15/00   
//			TCHAR buf[40];
//			_stprintf(buf,_T("%0.3f"),targDist);
//			SetWindowText(GetDlgItem(hSimpleCamParams,IDC_TARG_DISTANCE),buf);

			if ( hSimpleCamParams && (currentEditCam == this))		// 5/29/01 2:31pm --MQM--, LAM - 8/13/02 - defect 511609
				tdistSpin->SetValue(targDist, FALSE);
			}
		}
	}

int SimpleCamera::DrawConeAndLine(TimeValue t, INode* inode, GraphicsWindow *gw, int drawing ) {
	Matrix3 tm = inode->GetObjectTM(t);
	gw->setTransform(tm);
	gw->clearHitCode();
	if (hasTarget) {
		Point3 pt,v[3];
		if (GetTargetPoint(t,inode,pt)){
			float den = Length(tm.GetRow(2));
			float dist = (den!=0)?Length(tm.GetTrans()-pt)/den : 0.0f;
			targDist = dist;
			if (hSimpleCamParams&&(currentEditCam==this)) {
				// DS 8/15/00 
//				TCHAR buf[40];
//				_stprintf(buf,_T("%0.3f"),targDist);
//				SetWindowText(GetDlgItem(hSimpleCamParams,IDC_TARG_DISTANCE),buf);
				tdistSpin->SetValue(GetTDist(t), FALSE);
				}
			if ((drawing != -1) && (coneState || (extDispFlags & EXT_DISP_ONLY_SELECTED))) {
				if(manualClip) {
					DrawCone(t, gw, GetClipDist(t, CAM_HITHER_CLIP),COLOR_CAMERA_CLIP,0,1);
					DrawCone(t, gw, GetClipDist(t, CAM_YON_CLIP),COLOR_CAMERA_CLIP,1,1);
					}
				else
					DrawCone(t,gw,dist,COLOR_CAMERA_CONE,TRUE);
			}
			if(!inode->IsFrozen() && !inode->Dependent())
			{
				// 6/25/01 2:33pm --MQM--
				// if user has changed the color of the camera,
				// use that color for the target line too
				Color color(inode->GetWireColor());
				if ( color != GetUIColor(COLOR_CAMERA_OBJ) )
					gw->setColor( LINE_COLOR, color );
				else
					gw->setColor( LINE_COLOR, GetUIColor(COLOR_TARGET_LINE)); // old method
			}
			v[0] = Point3(0,0,0);
			if (drawing == -1)
				v[1] = Point3(0.0f, 0.0f, -0.9f * dist);
			else
				v[1] = Point3(0.0f, 0.0f, -dist);
			gw->polyline( 2, v, NULL, NULL, FALSE, NULL );	
			}
		}
	else {
		if (coneState || (extDispFlags & EXT_DISP_ONLY_SELECTED))
			if(manualClip) {
				DrawCone(t, gw, GetClipDist(t, CAM_HITHER_CLIP),COLOR_CAMERA_CLIP,0,1);
				DrawCone(t, gw, GetClipDist(t, CAM_YON_CLIP),COLOR_CAMERA_CLIP,1,1);
				}
			else
				DrawCone(t,gw,GetTDist(t),COLOR_CAMERA_CONE,TRUE);
		}
	return gw->checkHitCode();
	}

BaseInterface*	SimpleCamera::GetInterface(Interface_ID id)
{
	if (IID_STEREO_COMPATIBLE_CAMERA == id) 
		return static_cast<IStereoCompatibleCamera*>(this);
	else 
		return GenCamera::GetInterface(id); 
}

// From IStereoCompatibleCamera
bool SimpleCamera::RaisePreCameraTargetChanged(bool bTarget, float& targetDistance, TimeValue t)
{
	if (mStereoCameraCallBack)
	{
		return mStereoCameraCallBack->PreCameraTargetChanged(t, bTarget, targetDistance);
	}
	return false;
}

bool SimpleCamera::RaisePreSetCameraTargetDistance(float distance, TimeValue t)
{
	if (mStereoCameraCallBack)
	{
		return mStereoCameraCallBack->PreSetCameraTargetDistance(t, distance);
	}
	return false;
}

bool SimpleCamera::RaisePreCameraTargetDistanceUpdated(float& distance, TimeValue t)
{
	if (mStereoCameraCallBack)
	{
		return mStereoCameraCallBack->PreCameraTargetDistanceUpdated(t, distance);
	}
	return false;
}

void SimpleCamera::RegisterIStereoCompatibleCameraCallback(IStereoCameraCallback* callback)
{
	if(NULL != callback)
	{
		DbgAssert(mStereoCameraCallBack == NULL);
		mStereoCameraCallBack = callback;
	}
}
void SimpleCamera::UnRegisterIStereoCompatibleCameraCallback(IStereoCameraCallback* callback)
{
	if(NULL != callback && mStereoCameraCallBack == callback)
	{
		mStereoCameraCallBack = NULL;
	}
}

bool SimpleCamera::HasTargetNode(TimeValue t) const
{
	return (hasTarget != 0);
}

float SimpleCamera::GetTargetDistance(TimeValue t) const
{
	if (!pblock)
	{
		return MIN_TDIST;
	}
	float f;
	Interval valid;
	pblock->GetValue( PB_TDIST, t, f, valid );
	if ( f < MIN_TDIST ) f = MIN_TDIST;
	return f;
}

// From BaseObject
int SimpleCamera::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
		
	HitRegion hitRegion;
	DWORD	savedLimits;
	int res = 0;
	Matrix3 m;
	if (!enable) return  0;
	GraphicsWindow *gw = vpt->getGW();	
	MakeHitRegion(hitRegion,type,crossing,4,p);	
	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);
	GetMat(t,inode,*vpt,m);
	gw->setTransform(m);
	gw->clearHitCode();
	res = mesh.select( gw, gw->getMaterial(), &hitRegion, flags & HIT_ABORTONHIT ); 
	// if not, check the target line, and set the pair flag if it's hit
	if( !res )	{
		// this special case only works with point selection of targeted lights
		if((type != HITTYPE_POINT) || !inode->GetTarget())
			return 0;
		// don't let line be active if only looking at selected stuff and target isn't selected
		if((flags & HIT_SELONLY) && !inode->GetTarget()->Selected() )
			return 0;
		gw->clearHitCode();
		res = DrawConeAndLine(t, inode, gw, -1);
		if(res != 0)
			inode->SetTargetNodePair(1);
	}
	gw->setRndLimits(savedLimits);
	return res;
}

void SimpleCamera::Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}
	
	// Make sure the vertex priority is active and at least as important as the best snap so far
	if(snap->vertPriority > 0 && snap->vertPriority <= snap->priority) {
		Matrix3 tm = inode->GetObjectTM(t);	
		GraphicsWindow *gw = vpt->getGW();	
   	
		gw->setTransform(tm);

		Point2 fp = Point2((float)p->x, (float)p->y);
		IPoint3 screen3;
		Point2 screen2;
		Point3 vert(0.0f,0.0f,0.0f);

		gw->wTransPoint(&vert,&screen3);

		screen2.x = (float)screen3.x;
		screen2.y = (float)screen3.y;

		// Are we within the snap radius?
		int len = (int)Length(screen2 - fp);
		if(len <= snap->strength) {
			// Is this priority better than the best so far?
			if(snap->vertPriority < snap->priority) {
				snap->priority = snap->vertPriority;
				snap->bestWorld = vert * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
				}
			else // Closer than the best of this priority?
			if(len < snap->bestDist) {
				snap->priority = snap->vertPriority;
				snap->bestWorld = vert * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
				}
			}
		}
	}


int SimpleCamera::DrawRange(TimeValue t, INode *inode, GraphicsWindow *gw)
{
	if(!rangeDisplay)
		return 0;
	gw->clearHitCode();
	Matrix3 tm = inode->GetObjectTM(t);
	gw->setTransform(tm);
	int cnear = 0;
	int cfar = 0;
	if(!inode->IsFrozen() && !inode->Dependent()) { 
		cnear = COLOR_NEAR_RANGE;
		cfar = COLOR_FAR_RANGE;
		}
	DrawCone(t, gw, GetEnvRange(t, ENV_NEAR_RANGE),cnear);
	DrawCone(t, gw, GetEnvRange(t, ENV_FAR_RANGE), cfar, TRUE);
	return gw->checkHitCode();
	}

#define MAXVP_DIST 1.0e8f

void SimpleCamera::GetConePoints(TimeValue t, Point3* q, float dist) {
	if (dist>MAXVP_DIST)
		dist = MAXVP_DIST;
	float ta = (float)tan(0.5*(double)GetFOV(t));
	float w = dist * ta;
//	float h = w * (float).75; //  ASPECT ??
	float h = w / GetAspect();
	q[0] = Point3( w, h,-dist);				
	q[1] = Point3(-w, h,-dist);				
	q[2] = Point3(-w,-h,-dist);				
	q[3] = Point3( w,-h,-dist);				
	}

void SimpleCamera::DrawCone(TimeValue t, GraphicsWindow *gw, float dist, int colid, BOOL drawSides, BOOL drawDiags) {
	Point3 q[5], u[3];
	GetConePoints(t,q,dist);
	if (colid)	gw->setColor( LINE_COLOR, GetUIColor(colid));
	if (drawDiags) {
		u[0] =  q[0];	u[1] =  q[2];	
		gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
		u[0] =  q[1];	u[1] =  q[3];	
		gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
		}
	gw->polyline( 4, q, NULL, NULL, TRUE, NULL );	
	if (drawSides) {
		gw->setColor( LINE_COLOR, GetUIColor(COLOR_CAMERA_CONE));
		u[0] = Point3(0,0,0);
		for (int i=0; i<4; i++) {
			u[1] =  q[i];	
			gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
			}
		}
	}

void SimpleCamera::SetExtendedDisplay(int flags)
{
	extDispFlags = flags;
}

static MaxSDK::Graphics::Utilities::MeshEdgeKey CameraMeshKey;
static MaxSDK::Graphics::Utilities::SplineItemKey CameraSplineKey;

unsigned long SimpleCamera::GetObjectDisplayRequirement() const
{
	return 1;
}

bool SimpleCamera::PrepareDisplay(const MaxSDK::Graphics::UpdateDisplayContext& displayContext)
{
	CameraMeshKey.SetFixedSize(true);
	return true;
}

bool SimpleCamera::UpdatePerNodeItems(
	const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
	MaxSDK::Graphics::UpdateNodeContext& nodeContext,
	MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer)
{
	INode* pNode = nodeContext.GetRenderNode().GetMaxNode();
	MaxSDK::Graphics::Utilities::MeshEdgeRenderItem* pMeshItem = new CameraMeshItem(&mesh);
	if (pNode->Dependent())
	{
		Color dependentColor = ColorMan()->GetColorAsPoint3(kViewportShowDependencies);
		pMeshItem->SetColor(dependentColor);
	}
	else if (pNode->Selected()) 
	{
		pMeshItem->SetColor(Color(GetSelColor()));
	}
	else if (pNode->IsFrozen())
	{
		pMeshItem->SetColor(Color(GetFreezeColor()));
	}
	else
	{
		Color color(pNode->GetWireColor());
		pMeshItem->SetColor(color);
	}
	MaxSDK::Graphics::CustomRenderItemHandle tempHandle;
	tempHandle.Initialize();
	tempHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	tempHandle.SetCustomImplementation(pMeshItem);
	ViewExp& vpt = GetCOREInterface()->GetActiveViewExp();
	if (vpt.GetViewCamera() != pNode)
	{
		MaxSDK::Graphics::ConsolidationData data;
		data.Strategy = &MaxSDK::Graphics::Utilities::MeshEdgeConsolidationStrategy::GetInstance();
		data.Key = &CameraMeshKey;
		tempHandle.SetConsolidationData(data);
	}
	targetRenderItemContainer.AddRenderItem(tempHandle);
	/*
	MaxSDK::Graphics::Utilities::SplineRenderItem* pLineItem = new CameraConeItem(this);
	MaxSDK::Graphics::CustomRenderItemHandle coneHandle;
	coneHandle.Initialize();
	coneHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	coneHandle.SetCustomImplementation(pLineItem);
	if (vpt.GetViewCamera() != pNode)
	{
		MaxSDK::Graphics::ConsolidationData data;
		data.Strategy = &MaxSDK::Graphics::Utilities::SplineConsolidationStrategy::GetInstance();
		data.Key = &CameraSplineKey;
		coneHandle.SetConsolidationData(data);
	}
	targetRenderItemContainer.AddRenderItem(coneHandle);
	*/

	if (hasTarget)
	{
		MaxSDK::Graphics::Utilities::SplineRenderItem* pLineItem = new CameraTargetLineItem();
		MaxSDK::Graphics::CustomRenderItemHandle lineHandle;
		lineHandle.Initialize();
		lineHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
		lineHandle.SetCustomImplementation(pLineItem);
		if (vpt.GetViewCamera() != pNode)
		{
			MaxSDK::Graphics::ConsolidationData data;
			data.Strategy = &MaxSDK::Graphics::Utilities::SplineConsolidationStrategy::GetInstance();
			data.Key = &CameraSplineKey;
			lineHandle.SetConsolidationData(data);
		}
		targetRenderItemContainer.AddRenderItem(lineHandle);
	}
	return true;
}

int SimpleCamera::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	if (MaxSDK::Graphics::IsRetainedModeEnabled())
	{
		// 11/15/2010 
		// In Nitrous view port, do not draw camera itself when seeing from the camera
		if (NULL != vpt && vpt->GetViewCamera() == inode)
		{
			return 0;
		}
	}
	Matrix3 m;
	GraphicsWindow *gw = vpt->getGW();
	if (!enable) return  0;

	GetMat(t,inode,*vpt,m);
	gw->setTransform(m);
	DWORD rlim = gw->getRndLimits();
	gw->setRndLimits(GW_WIREFRAME|GW_EDGES_ONLY|GW_BACKCULL| (rlim&GW_Z_BUFFER) );
	if (inode->Selected())
		gw->setColor( LINE_COLOR, GetSelColor());
	else if(!inode->IsFrozen() && !inode->Dependent())
	{
		// 6/25/01 2:27pm --MQM--
		// use wire-color to draw the camera
		Color color(inode->GetWireColor());
		gw->setColor( LINE_COLOR, color );	
	}
	if (!MaxSDK::Graphics::IsRetainedModeEnabled())
	{
		mesh.render( gw, gw->getMaterial(), NULL, COMP_ALL);	
	}
	DrawConeAndLine(t,inode,gw,1);
#if 0
	if(horzLineState) {
		Point3 eye, tgt;
		eye = inode->GetObjTMAfterWSM(t).GetTrans();
		if(inode->GetTarget())
			tgt = inode->GetTarget()->GetObjTMAfterWSM(t).GetTrans();
		else {
			m = inode->GetObjTMAfterWSM(t);
			m.PreTranslate(Point3(0.0f, 0.0f, -GetTDist(t)));
			tgt = m.GetTrans();
		}
		tgt[1] = eye[1];
        Point3 pt[10];
        float camDist;
		float ta = (float)tan(0.5*(double)GetFOV(t));

        camDist = (float)sqrt((tgt[0]-eye[0]) * (tgt[0]-eye[0]) + (tgt[2]-eye[2]) * (tgt[2]-eye[2]));
               
        pt[0][0] = -camDist * ta;
        pt[0][1] = 0.0f;
        pt[0][2] = -camDist;
        pt[1][0] = camDist * ta;
        pt[1][1] = 0.0f;
        pt[1][2] = -camDist;
		gw->polyline(2, pt, NULL, NULL, 0, NULL);

	}
#endif
	DrawRange(t, inode, gw);
	gw->setRndLimits(rlim);
	return(0);
	}


RefResult SimpleCamera::EvalCameraState(TimeValue t, Interval& valid, CameraState* cs) {
	cs->isOrtho = IsOrtho();	
	cs->fov = GetFOV(t,valid);
	cs->tdist = GetTDist(t,valid);
	cs->horzLine = horzLineState;
	cs->manualClip = manualClip;
	cs->hither = GetClipDist(t, CAM_HITHER_CLIP, valid);
	cs->yon = GetClipDist(t, CAM_YON_CLIP, valid);
	cs->nearRange = GetEnvRange(t, ENV_NEAR_RANGE, valid);
	cs->farRange = GetEnvRange(t, ENV_FAR_RANGE, valid);
	return REF_SUCCEED;
	}

//
// Reference Managment:
//

RefResult SimpleCamera::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, 
	PartID& partID, RefMessage message, BOOL propagate ) 
	{
	switch (message) {		
		case REFMSG_WANT_SHOWPARAMLEVEL: {
			BOOL	*pb = ( BOOL * )partID;
			if ( hTarget == ( RefTargetHandle )depthOfFieldPB )
				*pb = TRUE;
			else
				*pb = FALSE;

			return REF_HALT;
			}

		case REFMSG_GET_PARAM_DIM: {
			GetParamDim *gpd = (GetParamDim*)partID;
			if ( hTarget == ( RefTargetHandle )pblock )
			{
				switch (gpd->index) {
					case PB_FOV:
						gpd->dim = stdAngleDim;
						break;				
					case PB_TDIST:
					case PB_HITHER:
					case PB_YON:
					case PB_NRANGE:
					case PB_FRANGE:
						gpd->dim = stdWorldDim;
						break;	
					case PB_FOV_TYPE:
						gpd->dim = defaultDim;
						break;	
					case PB_MP_EFFECT_ENABLE:
					case PB_MP_EFF_REND_EFF_PER_PASS:
						gpd->dim = defaultDim;
						break;				
					}
				return REF_HALT; 
			}
			return REF_STOP; 
			}

		case REFMSG_GET_PARAM_NAME: {
			GetParamName *gpn = (GetParamName*)partID;
			if ( hTarget == ( RefTargetHandle )pblock )
			{
				switch (gpn->index) {
					case PB_FOV:
						gpn->name = GetString(IDS_RB_FOV);
						break;												
					case PB_TDIST:
						gpn->name = GetString(IDS_DB_TDIST);
						break;												
					case PB_HITHER:
						gpn->name = GetString(IDS_RB_NEARPLANE);
						break;												
					case PB_YON:
						gpn->name = GetString(IDS_RB_FARPLANE);
						break;												
					case PB_NRANGE:
						gpn->name = GetString(IDS_DB_NRANGE);
						break;												
					case PB_FRANGE:
						gpn->name = GetString(IDS_DB_FRANGE);
						break;	
					case PB_FOV_TYPE:
						gpn->name = GetString(IDS_FOV_TYPE);
						break;	
					case PB_MP_EFFECT_ENABLE:
						gpn->name = GetString(IDS_MP_EFFECT_ENABLE);
						break;				
					case PB_MP_EFF_REND_EFF_PER_PASS:
						gpn->name = GetString(IDS_MP_EFF_REND_EFF_PER_PASS);
						break;				
					}
				return REF_HALT; 
			}
			return REF_STOP; 
			}
		}
	return(REF_SUCCEED);
	}


ObjectState SimpleCamera::Eval(TimeValue time){
	// UpdateUI(time);
	return ObjectState(this);
	}

Interval SimpleCamera::ObjectValidity(TimeValue time) {
	Interval ivalid;
	ivalid.SetInfinite();
	if (!waitPostLoad) {
		GetFOV(time,ivalid);
		GetTDist(time,ivalid);
		GetClipDist(time, CAM_HITHER_CLIP, ivalid);
		GetClipDist(time, CAM_YON_CLIP, ivalid);
		GetEnvRange(time, ENV_NEAR_RANGE, ivalid);
		GetEnvRange(time, ENV_FAR_RANGE, ivalid);
		GetMultiPassEffectEnabled(time, ivalid);
		UpdateUI(time);
		}
	return ivalid;	
	}


//********************************************************
// LOOKAT CAMERA
//********************************************************


//------------------------------------------------------
class LACamClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new SimpleCamera(1); }
	int 			BeginCreate(Interface *i);
	int 			EndCreate(Interface *i);
	const TCHAR *	ClassName() { return GetString(IDS_DB_TARGET_CLASS); }
    SClass_ID		SuperClassID() { return CAMERA_CLASS_ID; }
   	Class_ID		ClassID() { return Class_ID(LOOKAT_CAM_CLASS_ID,0); }
	const TCHAR* 	Category() { return _T("");  }
	void			ResetClassParams(BOOL fileReset) { if(fileReset) resetCameraParams(); }
	};

static LACamClassDesc laCamClassDesc;

extern ClassDesc* GetLookatCamDesc() {return &laCamClassDesc; }

class LACamCreationManager : public MouseCallBack, ReferenceMaker {
	private:
		CreateMouseCallBack *createCB;	
		INode *camNode,*targNode;
		SimpleCamera *camObject;
		TargetObject *targObject;
		int attachedToNode;
		IObjCreate *createInterface;
		ClassDesc *cDesc;
		Matrix3 mat;  // the nodes TM relative to the CP
		IPoint2 pt0;
		int ignoreSelectionChange;
		int lastPutCount;

		void CreateNewObject();	

		virtual void GetClassName(MSTR& s) { s = _M("LACamCreationManager"); }
		int NumRefs() { return 1; }
		RefTargetHandle GetReference(int i) { return (RefTargetHandle)camNode; } 
		void SetReference(int i, RefTargetHandle rtarg) { camNode = (INode *)rtarg; }

		// StdNotifyRefChanged calls this, which can change the partID to new value 
		// If it doesnt depend on the particular message& partID, it should return
		// REF_DONTCARE
	    RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, 
	    	PartID& partID, RefMessage message, BOOL propagate);

	public:
		void Begin( IObjCreate *ioc, ClassDesc *desc );
		void End();
		
		LACamCreationManager()
			{
			ignoreSelectionChange = FALSE;
			}
		int proc( HWND hwnd, int msg, int point, int flag, IPoint2 m );
	};


#define CID_SIMPLECAMCREATE	CID_USER + 1

class LACamCreateMode : public CommandMode {
		LACamCreationManager proc;
	public:
		void Begin( IObjCreate *ioc, ClassDesc *desc ) { proc.Begin( ioc, desc ); }
		void End() { proc.End(); }

		int Class() { return CREATE_COMMAND; }
		int ID() { return CID_SIMPLECAMCREATE; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints = 1000000; return &proc; }
		ChangeForegroundCallback *ChangeFGProc() { return CHANGE_FG_SELECTED; }
		BOOL ChangeFG( CommandMode *oldMode ) { return (oldMode->ChangeFGProc() != CHANGE_FG_SELECTED); }
		void EnterMode() {}
		void ExitMode() {}
		BOOL IsSticky() { return FALSE; }
	};

static LACamCreateMode theLACamCreateMode;

//LACamCreationManager::LACamCreationManager( IObjCreate *ioc, ClassDesc *desc )
void LACamCreationManager::Begin( IObjCreate *ioc, ClassDesc *desc )
	{
	createInterface = ioc;
	cDesc           = desc;
	attachedToNode  = FALSE;
	createCB        = NULL;
	camNode         = NULL;
	targNode        = NULL;
	camObject       = NULL;
	targObject      = NULL;
	CreateNewObject();
	}

//LACamCreationManager::~LACamCreationManager
void LACamCreationManager::End()
	{
	if ( camObject ) {
		camObject->EndEditParams( (IObjParam*)createInterface, 
	                    	          END_EDIT_REMOVEUI, NULL);
		if ( !attachedToNode ) {
			// RB 4-9-96: Normally the hold isn't holding when this 
			// happens, but it can be in certain situations (like a track view paste)
			// Things get confused if it ends up with undo...
			theHold.Suspend(); 
			camObject->DeleteAllRefsFromMe();
			camObject->DeleteAllRefsToMe();
			camObject->DeleteThis();  // JBW 11.1.99, this allows scripted plugin cameras to delete cleanly
			camObject = NULL;
			theHold.Resume();
			// RB 7/28/97: If something has been put on the undo stack since this object was created, we have to flush the undo stack.
			if (theHold.GetGlobalPutCount()!=lastPutCount) {
				GetSystemSetting(SYSSET_CLEAR_UNDO);
				}
			macroRec->Cancel();  // JBW 4/23/99
		} else if ( camNode ) {
			 // Get rid of the reference.
			theHold.Suspend();
			DeleteReference(0);  // sets camNode = NULL
			theHold.Resume();
			}
		}	
	}

RefResult LACamCreationManager::NotifyRefChanged(
	const Interval& changeInt, 
	RefTargetHandle hTarget, 
	PartID& partID,  
	RefMessage message, 
	BOOL propagate) 
	{
	switch (message) {
		case REFMSG_PRENOTIFY_PASTE:
		case REFMSG_TARGET_SELECTIONCHANGE:
		 	if ( ignoreSelectionChange ) {
				break;
				}
		 	if ( camObject && camNode==hTarget ) {
				// this will set camNode== NULL;
				theHold.Suspend();
				DeleteReference(0);
				theHold.Resume();
				goto endEdit;
				}
			// fall through

		case REFMSG_TARGET_DELETED:		
			if ( camObject && camNode==hTarget ) {
				endEdit:
				camObject->EndEditParams( (IObjParam*)createInterface, 0, NULL);
				camObject  = NULL;				
				camNode    = NULL;
				CreateNewObject();	
				attachedToNode = FALSE;
				}
			else if (targNode==hTarget) {
				targNode = NULL;
				targObject = NULL;
				}
			break;		
		}
	return REF_SUCCEED;
	}


void LACamCreationManager::CreateNewObject()
	{
	camObject = (SimpleCamera*)cDesc->Create();
	lastPutCount = theHold.GetGlobalPutCount();

    macroRec->BeginCreate(cDesc);  // JBW 4/23/99
	// Start the edit params process
	if ( camObject ) {
		camObject->BeginEditParams( (IObjParam*)createInterface, BEGIN_EDIT_CREATE, NULL );
		}	
	}

static BOOL needToss;
			
int LACamCreationManager::proc( 
				HWND hwnd,
				int msg,
				int point,
				int flag,
				IPoint2 m )
	{	
	int res = TRUE;	
	TSTR targName;
	ViewExp &vpx = createInterface->GetViewExp(hwnd); 
	if ( ! vpx.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	switch ( msg ) {
		case MOUSE_POINT:
			switch ( point ) {
		case 0: {
					pt0 = m;
					assert( camObject );					
					if ( createInterface->SetActiveViewport(hwnd) ) {
						return FALSE;
						}

					if (createInterface->IsCPEdgeOnInView()) { 
						res = FALSE;
						goto done;
						}

					// if cameras were hidden by category, re-display them
					GetCOREInterface()->SetHideByCategoryFlags(
							GetCOREInterface()->GetHideByCategoryFlags() & ~HIDE_CAMERAS);

					if ( attachedToNode ) {
				   		// send this one on its way
				   		camObject->EndEditParams( (IObjParam*)createInterface, 0, NULL);
						macroRec->EmitScript();  // JBW 4/23/99
						
						// Get rid of the reference.
						if (camNode) {
							theHold.Suspend();
							DeleteReference(0);
							theHold.Resume();
							}

						// new object
						CreateNewObject();   // creates camObject
						}

					needToss = theHold.GetGlobalPutCount()!=lastPutCount;

				   	theHold.Begin();	 // begin hold for undo
					mat.IdentityMatrix();

					// link it up
					INode *l_camNode = createInterface->CreateObjectNode( camObject);
					attachedToNode = TRUE;
					assert( l_camNode );					
					createCB = camObject->GetCreateMouseCallBack();
					createInterface->SelectNode( l_camNode );
					
						// Create target object and node
						targObject = new TargetObject;
						assert(targObject);
						targNode = createInterface->CreateObjectNode( targObject);
						assert(targNode);
						targName = l_camNode->GetName();
						targName += GetString(IDS_DB_DOT_TARGET);
						macroRec->Disable();
						targNode->SetName(targName);
						macroRec->Enable();

						// hook up camera to target using lookat controller.
						createInterface->BindToTarget(l_camNode,targNode);					

					// Reference the new node so we'll get notifications.
					theHold.Suspend();
					ReplaceReference( 0, l_camNode);
					theHold.Resume();

					// Position camera and target at first point then drag.
					mat.IdentityMatrix();
					//mat[3] = vpx.GetPointOnCP(m);
					mat.SetTrans( vpx.SnapPoint(m,m,NULL,SNAP_IN_3D) );
					createInterface->SetNodeTMRelConstPlane(camNode, mat);
						createInterface->SetNodeTMRelConstPlane(targNode, mat);
						camObject->Enable(1);

				   		ignoreSelectionChange = TRUE;
				   		createInterface->SelectNode( targNode,0);
				   		ignoreSelectionChange = FALSE;
						res = TRUE;

					// 6/25/01 2:57pm --MQM-- 
					// set our color to the default camera color
					if ( camNode ) 
					{
						Point3 color = GetUIColor( COLOR_CAMERA_OBJ );
						camNode->SetWireColor( RGB( color.x*255.0f, color.y*255.0f, color.z*255.0f ) );
					}
					}
					break;
					
				case 1:
					if (Length(m-pt0)<2)
						goto abort;
					//mat[3] = vpx.GetPointOnCP(m);
					mat.SetTrans( vpx.SnapPoint(m,m,NULL,SNAP_IN_3D) );
					macroRec->Disable();   // JBW 4/23/99
					createInterface->SetNodeTMRelConstPlane(targNode, mat);
					macroRec->Enable();
				   	ignoreSelectionChange = TRUE;
				   	createInterface->SelectNode( camNode);
				   	ignoreSelectionChange = FALSE;
					
					createInterface->RedrawViews(createInterface->GetTime());  

				    theHold.Accept(OperationDesc(IDS_DS_CREATE, GetString(IDS_DS_CREATE), hInstance, camObject));

					res = FALSE;	// We're done
					break;
				}			
			break;

		case MOUSE_MOVE:
			//mat[3] = vpx.GetPointOnCP(m);
			mat.SetTrans( vpx.SnapPoint(m,m,NULL,SNAP_IN_3D) );
			macroRec->Disable();   // JBW 4/23/99
			createInterface->SetNodeTMRelConstPlane(targNode, mat);
			macroRec->Enable();
			createInterface->RedrawViews(createInterface->GetTime());	   

			macroRec->SetProperty(camObject, _T("target"),   // JBW 4/23/99
				mr_create, Class_ID(TARGET_CLASS_ID, 0), GEOMOBJECT_CLASS_ID, 1, _T("transform"), mr_matrix3, &mat);

			res = TRUE;
			break;

		case MOUSE_FREEMOVE:
			SetCursor(UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Crosshair));
			//Snap Preview
			vpx.SnapPreview(m,m,NULL, SNAP_IN_3D);
			break;

	    case MOUSE_PROPCLICK:
			// right click while between creations
			createInterface->RemoveMode(NULL);
			break;

		case MOUSE_ABORT:
			abort:
			assert( camObject );
			camObject->EndEditParams( (IObjParam*)createInterface,0,NULL);
			macroRec->Cancel();  // JBW 4/23/99
			theHold.Cancel();	 // deletes both the camera and target.
			// Toss the undo stack if param changes have been made
			if (needToss) 
				GetSystemSetting(SYSSET_CLEAR_UNDO);
			camNode = NULL;			
			targNode = NULL;	 	
			createInterface->RedrawViews(createInterface->GetTime()); 
			CreateNewObject();	
			attachedToNode = FALSE;
			res = FALSE;						
		}
	
	done:
 
	return res;
	}

int LACamClassDesc::BeginCreate(Interface *i)
	{
	SuspendSetKeyMode();
	IObjCreate *iob = i->GetIObjCreate();
	
	//iob->SetMouseProc( new LACamCreationManager(iob,this), 1000000 );

	theLACamCreateMode.Begin( iob, this );
	iob->PushCommandMode( &theLACamCreateMode );
	
	return TRUE;
	}

int LACamClassDesc::EndCreate(Interface *i)
	{
	
	ResumeSetKeyMode();
	theLACamCreateMode.End();
	i->RemoveMode( &theLACamCreateMode );
	macroRec->EmitScript();  // JBW 4/23/99

	return TRUE;
	}

RefTargetHandle SimpleCamera::Clone(RemapDir& remap) {
	SimpleCamera* newob = new SimpleCamera();
   newob->ReplaceReference(0,remap.CloneRef(pblock));

	if ( GetReference(MP_EFFECT_REF) )
	{
		newob->ReplaceReference( MP_EFFECT_REF, remap.CloneRef( GetReference(MP_EFFECT_REF) ) );
	}

	newob->enable = enable;
	newob->hasTarget = hasTarget;
	newob->coneState = coneState;
	newob->manualClip = manualClip;
	newob->rangeDisplay = rangeDisplay;
	newob->isOrtho = isOrtho;
	BaseClone(this, newob, remap);
	return(newob);
	}

#define CAMERA_FOV_CHUNK 	0x2680
#define CAMERA_TARGET_CHUNK 0x2682
#define CAMERA_CONE_CHUNK 	0x2684
#define CAMERA_MANUAL_CLIP	0x2686
#define CAMERA_HORIZON		0x2688
#define CAMERA_RANGE_DISP	0x268a
#define CAMERA_IS_ORTHO		0x268c

// IO
IOResult SimpleCamera::Save(ISave *isave) {
		
#if 0
	ULONG nb;
	Interval valid;
	float fov;
	pblock->GetValue( 0, 0, fov, valid );
	
	isave->BeginChunk(CAMERA_FOV_CHUNK);
	isave->Write(&fov, sizeof(FLOAT), &nb);
	isave->EndChunk();
#endif

	if (hasTarget) {
		isave->BeginChunk(CAMERA_TARGET_CHUNK);
		isave->EndChunk();
		}
	if (coneState) {
		isave->BeginChunk(CAMERA_CONE_CHUNK);
		isave->EndChunk();
		}
	if (rangeDisplay) {
		isave->BeginChunk(CAMERA_RANGE_DISP);
		isave->EndChunk();
		}
	if (isOrtho) {
		isave->BeginChunk(CAMERA_IS_ORTHO);
		isave->EndChunk();
		}
	if (manualClip) {
		isave->BeginChunk(CAMERA_MANUAL_CLIP);
		isave->EndChunk();
		}
	if (horzLineState) {
		isave->BeginChunk(CAMERA_HORIZON);
		isave->EndChunk();
		}
	return IO_OK;
	}

class CameraPostLoad : public PostLoadCallback {
public:
	SimpleCamera *sc;
	Interval valid;
	CameraPostLoad(SimpleCamera *cam) { sc = cam;}
	void proc(ILoad *iload) {
		if (sc->pblock->GetVersion() != CAMERA_VERSION) {
			switch (sc->pblock->GetVersion()) {
			case 0:
				sc->ReplaceReference(0,
						UpdateParameterBlock(
							descV0, 2, sc->pblock,
							descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION));
				iload->SetObsolete();
				break;

			case 1:
				sc->ReplaceReference(0,
						UpdateParameterBlock(
							descV1, 4, sc->pblock,
							descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION));
				iload->SetObsolete();
				break;

			case 2:
				sc->ReplaceReference(0,
						UpdateParameterBlock(
							descV2, 6, sc->pblock,
							descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION));
				iload->SetObsolete();
				break;

			case 3:
				sc->ReplaceReference(0,
						UpdateParameterBlock(
							descV3, 7, sc->pblock,
							descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION));
				iload->SetObsolete();
				break;
// MC 
			case 4:
				sc->ReplaceReference(0,
					UpdateParameterBlock(
					descV4, 8, sc->pblock,
					descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION));
				iload->SetObsolete();
			break;

			default:
				assert(0);
				break;
			}
		}

		if (( sc->depthOfFieldPB != NULL ) && ( sc->depthOfFieldPB->GetVersion() != DOF_VERSION )) {
			switch (sc->depthOfFieldPB->GetVersion()) {
			case 1:
			{
				sc->ReplaceReference(DOF_REF,
						UpdateParameterBlock(
							descV0, 2, sc->depthOfFieldPB,
							descV1, 2, DOF_VERSION));
				iload->SetObsolete();

				// Replicate the existing data into an mr dof mp effect as much as possible
				IMultiPassCameraEffect *pIMultiPassCameraEffect = NULL;
				int numClasses = SimpleCamera::smCompatibleEffectList.Count();
				for (int i=0; i<numClasses; i++)
				{
					// mental ray DOF mp effect
					if ( SimpleCamera::smCompatibleEffectList[i]->CD()->ClassID() == Class_ID (0x6cc9546e, 0xb1961b9) ) // from dll\mentalray\translator\src\mrDOF.cpp
					{
						BOOL		dofEnable;
						TimeValue   t = 0;
						Interval	valid;

						sc->depthOfFieldPB->GetValue ( PB_DOF_ENABLE, t, dofEnable, valid );
						if ( dofEnable )
						{
							sc->SetMultiPassEffectEnabled ( t, dofEnable );

							pIMultiPassCameraEffect = reinterpret_cast<IMultiPassCameraEffect *>( SimpleCamera::smCompatibleEffectList[i]->CD()->Create(0) );
							sc->SetIMultiPassCameraEffect(pIMultiPassCameraEffect);
							IParamBlock2	*pb = pIMultiPassCameraEffect->GetParamBlock ( 0 );

							if ( pb )
							{
								float	fstop;
								enum
								{
									prm_fstop,
								};

								sc->depthOfFieldPB->GetValue ( PB_DOF_FSTOP, t, fstop, valid );
								pb->SetValue ( prm_fstop, t, fstop );
							}
						}
					}
				}

				break;
			}
			default:
				assert(0);
				break;
			}
		}

		waitPostLoad--;
		delete this;
	}
};

IOResult  SimpleCamera::Load(ILoad *iload) {
	IOResult res;
	enable = TRUE;
	coneState = 0;
	manualClip = 0;
	horzLineState = 0;
	rangeDisplay = 0;
	isOrtho = 0;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {
			case CAMERA_TARGET_CHUNK:
				hasTarget = 1;
				break;
			case CAMERA_CONE_CHUNK:
				coneState = 1;
				break;
// The display environment range property is not loaded and always set to false
			case CAMERA_RANGE_DISP:
				rangeDisplay = 1;
				break;
			case CAMERA_IS_ORTHO:
				isOrtho = 1;
				break;
			case CAMERA_MANUAL_CLIP:
				manualClip = 1;
				break;
			case CAMERA_HORIZON:
				horzLineState = 1;
				break;
			}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
		}
	waitPostLoad++;
	iload->RegisterPostLoadCallback(new CameraPostLoad(this));
	return IO_OK;
	}

