//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2009 Autodesk, Inc.  All rights reserved.
//  Copyright 2003 Character Animation Technologies.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

/**********************************************************************
	This is the layermanager component of CAT's NLA system.
	This controller is responsible for adding and removing
	layers. Loading and saving data, etc
 **********************************************************************/
#include "CATPlugins.h"

 // Max Stuff
#include "macrorec.h"
#include <custcont.h>

//MaxScript
#include <maxscript/maxscript.h>
#include <maxscript/compiler/parser.h>

#include <maxscript/maxwrapper/mxsobjects.h>
#include <maxicon.h>

// Rig Structure
// for CAT2 PLCB
#include "../CATObjects/CATParent.h"
#include "CATNodeControl.h"

// Layers
#include "CATClipRoot.h"
#include "CATClipWeights.h"

#include "CATHierarchyFunctions.h"
#include "CATMotionLayer.h"
#include "CATFilePaths.h"
#include "LayerTransform.h"
#include "CATCharacterRemap.h"

#include "TrackViewFunctions.h"
#include "Callback.h"

// These should go somewhere else some day
#define TRACKVIEW_BUTTON_WIDTH	30
#define TRACKVIEW_BUTTON_HEIGHT	30
#define KEYFRAME_BUTTON_WIDTH	35
#define KEYFRAME_BUTTON_HEIGHT	30

IObjParam *CATClipRoot::ip = NULL;
BOOL OpenLayerManagerWindow(INode* catparent_node);
BOOL RefreshLayerManagerWindow(INode* catparent_node);
void CleanCATClipRootPointer(CATClipRoot* pDeadRoot);

// Class description and stuff
class CATClipRootClassDesc : public ILayerRootDesc {
public:
	int IsPublic() { return FALSE; }
	void *Create(BOOL loading = FALSE) { return new CATClipRoot(loading); }
	void *CreateNew(class ICATParentTrans *catparenttrans) {
		CATClipRoot* root = new CATClipRoot(FALSE);
		root->catparenttrans = catparenttrans;
		return root;
	}

	const TCHAR *ClassName() { return GetString(IDS_CL_CATCLIPROOT); }
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID ClassID() { return CATCLIPROOT_CLASS_ID; }
	const TCHAR *Category() { return GetString(IDS_CATEGORY); }

	const TCHAR *InternalName() { return _T("CATClipRoot"); }
	HINSTANCE HInstance() { return hInstance; }

	//////////////////////////////////////////////////////////////////
	// this is the core of the new CAT api. Plugins can register
	// themselves with CAT to add custom layer types.
	Tab<ClassDesc2*> tabLayerTypes;
	void	RegisterLayerType(ClassDesc2 *desc) {
		int numlayertypes = tabLayerTypes.Count();
		tabLayerTypes.SetCount(numlayertypes + 1);
		tabLayerTypes[numlayertypes] = desc;
		//	tabLayerTypes.Append(1, &desc);
	}
	int		NumLayerTypes() { return tabLayerTypes.Count(); }
	ClassDesc2*		GetLayerType(int i) { return tabLayerTypes[i]; }
};

static CATClipRootClassDesc CATClipRootDesc;
ILayerRootDesc* GetCATClipRootDesc() { return &CATClipRootDesc; }

class PasteLayerInfo : SingleRefMaker {
public:
	CATClipRoot*	cliproot;
	NLAInfo*		layerinfo;
	//	TimeValue		t;
	PasteLayerInfo() {
		cliproot = NULL;
		layerinfo = NULL;
	};
	virtual void GetClassName(MSTR& s) { s = _M("PasteLayerInfo"); }

};

// this structure holds the copy/paste layer information
static PasteLayerInfo g_PasteLayerData;

//*********************************************************************
// Function publishing interface descriptor...
//
class MethodValidator : public FPValidator
{
	bool Validate(FPInterface*, FunctionID, int, FPValue& val, TSTR& msg) {
		if (_tcsicmp(val.s, _T("absolute")) &&
			_tcsicmp(val.s, _T("relativelocal")) &&
			_tcsicmp(val.s, _T("relativeworld")) &&
			_tcsicmp(val.s, _T("catmotion")) &&
			(_tcsicmp(val.s, _T("Mocap")) && CATClipRootDesc.NumLayerTypes() > 0)) {
			msg = GetString(IDS_VLD_LYR_TYPE);
			return false;
		}
		return true;
	}
};

ClipLayerMethod CATClipRoot::MethodFromString(const TCHAR *method) {
	if (!_tcsicmp(method, _T("absolute")))		return LAYER_ABSOLUTE;
	if (!_tcsicmp(method, _T("relativelocal")))	return LAYER_RELATIVE;
	if (!_tcsicmp(method, _T("relativeworld")))	return LAYER_RELATIVE_WORLD;
	if (!_tcsicmp(method, _T("catmotion")))		return LAYER_CATMOTION;
	DbgAssert(0);// the validator should have filtered anythiung else out by this stage
	return LAYER_IGNORE;
}

static MethodValidator methodValidator;

static FPInterfaceDesc clipRootOpsFPInterfaceDesc(
	LAYERROOT_INTERFACE_FP, _T("LayerRootFPInterface"), 0, &CATClipRootDesc, FP_MIXIN,

	CATClipRoot::fnAppendLayer, _T("AppendLayer"), 0, TYPE_INT, 0, 2,
		_T("name"), 0, TYPE_TSTR,
		_T("method"), 0, TYPE_NAME, f_validator, &methodValidator,
	CATClipRoot::fnInsertLayer, _T("InsertLayer"), 0, TYPE_BOOL, 0, 3,
		_T("name"), 0, TYPE_TSTR,
		_T("layerID"), 0, TYPE_INDEX,
		_T("method"), 0, TYPE_NAME, f_validator, &methodValidator,
	CATClipRoot::fnRemoveLayer, _T("RemoveLayer"), 0, TYPE_VOID, 0, 1,
		_T("layerID"), 0, TYPE_INDEX,
	CATClipRoot::fnMoveLayerUp, _T("MoveLayerUp"), 0, TYPE_VOID, 0, 1,
		_T("layerID"), 0, TYPE_INDEX,
	CATClipRoot::fnMoveLayerDown, _T("MoveLayerDown"), 0, TYPE_VOID, 0, 1,
		_T("layerID"), 0, TYPE_INDEX,
	CATClipRoot::fnGetLayerColor, _T("GetLayerColor"), 0, TYPE_COLOR, 0, 1,
		_T("index"), 0, TYPE_INDEX,
	CATClipRoot::fnSetLayerColor, _T("SetLayerColor"), 0, TYPE_BOOL, 0, 2,
		_T("index"), 0, TYPE_INDEX,
		_T("newColor"), 0, TYPE_COLOR,
	CATClipRoot::fnSaveClip, _T("SaveClip"), 0, TYPE_BOOL, 0, 5,
		_T("filename"), 0, TYPE_TSTR,
		_T("starttime"), 0, TYPE_TIMEVALUE,
		_T("endtime"), 0, TYPE_TIMEVALUE,
		_T("startlayer"), 0, TYPE_INDEX,
		_T("endlayer"), 0, TYPE_INDEX,

	CATClipRoot::fnSavePose, _T("SavePose"), 0, TYPE_BOOL, 0, 1,
		_T("filename"), 0, TYPE_TSTR,

	CATClipRoot::fnLoadClip, _T("LoadClip"), 0, TYPE_INODE, 0, 7,
		_T("filename"), 0, TYPE_TSTR,
		_T("starttime"), 0, TYPE_TIMEVALUE,
		_T("scaledata"), 0, TYPE_BOOL, f_keyArgDefault, TRUE,
		_T("transformdata"), 0, TYPE_BOOL, f_keyArgDefault, TRUE,
		_T("mirrordata"), 0, TYPE_BOOL, f_keyArgDefault, FALSE,
		_T("mirrorworldX"), 0, TYPE_BOOL, f_keyArgDefault, FALSE,
		_T("mirrorworldY"), 0, TYPE_BOOL, f_keyArgDefault, FALSE,

	CATClipRoot::fnLoadPose, _T("LoadPose"), 0, TYPE_INODE, 0, 7,
		_T("filename"), 0, TYPE_TSTR,
		_T("starttime"), 0, TYPE_TIMEVALUE,
		_T("scaledata"), 0, TYPE_BOOL, f_keyArgDefault, TRUE,
		_T("transformdata"), 0, TYPE_BOOL, f_keyArgDefault, TRUE,
		_T("mirrordata"), 0, TYPE_BOOL, f_keyArgDefault, FALSE,
		_T("mirrorworldX"), 0, TYPE_BOOL, f_keyArgDefault, FALSE,
		_T("mirrorworldY"), 0, TYPE_BOOL, f_keyArgDefault, FALSE,

	CATClipRoot::fnCreatePasteLayerTransformNode, _T("CreatePasteLayerTransformNode"), 0, TYPE_INODE, 0, 0,

	CATClipRoot::fnGetFileTagValue, _T("GetFileTagValue"), 0, TYPE_TSTR_BV, 0, 2,
		_T("filename"), 0, TYPE_TSTR,
		_T("tag"), 0, TYPE_TSTR,

	CATClipRoot::fnLoadHTR, _T("LoadHTR"), 0, TYPE_BOOL, 0, 2,
		_T("filename"), 0, TYPE_TSTR,
		_T("camfile"), 0, TYPE_TSTR,

	CATClipRoot::fnLoadBVH, _T("LoadBVH"), 0, TYPE_BOOL, 0, 2,
		_T("filename"), 0, TYPE_TSTR,
		_T("camfile"), 0, TYPE_TSTR,

	CATClipRoot::fnLoadFBX, _T("LoadFBX"), 0, TYPE_BOOL, 0, 2,
		_T("filename"), 0, TYPE_TSTR,
		_T("camfile"), 0, TYPE_TSTR,

	CATClipRoot::fnLoadBIP, _T("LoadBIP"), 0, TYPE_BOOL, 0, 2,
		_T("filename"), 0, TYPE_TSTR,
		_T("camfile"), 0, TYPE_TSTR,

	CATClipRoot::fnCollapsePoseToCurLayer, _T("CollapsePoseToCurLayer"), 0, TYPE_VOID, 0, 0,
	CATClipRoot::fnCollapseTimeRangeToLayer, _T("CollapseTimeRangeToLayer"), 0, TYPE_BOOL, 0, 7,
		_T("StartTime"), 0, TYPE_TIMEVALUE,
		_T("Endtime"), 0, TYPE_TIMEVALUE,
		_T("Frequency"), 0, TYPE_TIMEVALUE,
		_T("regularplot"), 0, TYPE_BOOL, f_keyArgDefault, FALSE,
		_T("NumPasses"), 0, TYPE_INT, f_keyArgDefault, 2,
		_T("PosDeltaThreshold"), 0, TYPE_FLOAT, f_keyArgDefault, 1.0,
		_T("RotDeltaThreshold"), 0, TYPE_FLOAT, f_keyArgDefault, 5.0,

	CATClipRoot::fnCopyLayer, _T("CopyLayer"), 0, TYPE_VOID, 0, 1,
		_T("LayerID"), 0, TYPE_INDEX,

	CATClipRoot::fnPasteLayer, _T("PasteLayer"), 0, TYPE_VOID, 0, 2,
		_T("Instance"), 0, TYPE_BOOL, f_keyArgDefault, FALSE,
		_T("CopyLayerInfo"), 0, TYPE_BOOL, f_keyArgDefault, TRUE,

	properties,

	CATClipRoot::propNumLayers, FP_NO_FUNCTION, _T("NumLayers"), 0, TYPE_INT,
	CATClipRoot::propGetSelectedLayer, CATClipRoot::propSetSelectedLayer, _T("SelectedLayer"), 0, TYPE_INDEX,
	CATClipRoot::propGetSoloLayer, CATClipRoot::propSetSoloLayer, _T("SoloLayer"), 0, TYPE_INDEX,
	CATClipRoot::propGetTrackDisplayMethod, CATClipRoot::propSetTrackDisplayMethod, _T("TrackDisplayMethod"), 0, TYPE_INT,

	p_end
);

FPInterfaceDesc* CATClipRoot::GetDescByID(Interface_ID id) {
	if (id == LAYERROOT_INTERFACE_FP) return &clipRootOpsFPInterfaceDesc;
	return &nullInterface;
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class CATClipRootPLCB : public PostLoadCallback {
protected:
	CATClipRoot *pClipRoot;

public:
	// Leave this public, as the loading code needs access to ot
	CATParent* mpCATParent;

	CATClipRootPLCB(CATClipRoot *pOwner) { pClipRoot = pOwner; mpCATParent = NULL; }

	DWORD GetFileSaveVersion() {
		DbgAssert(pClipRoot);
		return pClipRoot->GetFileSaveVersion();
	}

	int Priority() { return 6; }

	void proc(ILoad *)
	{
		if (!pClipRoot || pClipRoot->TestAFlag(A_IS_DELETED))
		{
			delete this;
			return;
		}

		if (mpCATParent != NULL)
		{
			// We have a CATParentTrans pointer to assign.  Ensure that
			// we either have a NULL CPT pointer on the ClipRoot, or that
			// it matches the one here.
			ICATParentTrans* pCATParentTrans = mpCATParent->GetCATParentTrans();
			DbgAssert(pCATParentTrans == NULL || pCATParentTrans == pClipRoot->GetCATParentTrans());
			pClipRoot->catparenttrans = pCATParentTrans;
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

//////////////////////////////////////////////////////////////////////////
// Clip Hierarchy Rollout UI
//////////////////////////////////////////////////////////////////////////
static INT_PTR CALLBACK ClipRolloutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class ClipRolloutData : public TimeChangeCallback, public ReferenceMaker
{
	ICATParentTrans *catparenttrans;

	CATClipRoot		*pClipRoot;
	HWND			hDlg;

	TimeValue		tvTime;

	ICustButton		*btnCATWiki;

	HIMAGELIST		hImageCATModes;
	ICustButton		*flyCATMode;
	ICustButton		*btnCATWindow;

	ICustButton		*btnRemoveLayer;
	ICustButton		*btnMoveUp;
	ICustButton		*btnMoveDown;

	ICustButton		*btnCopyLayer;
	ICustButton		*btnPasteLayer;

	HIMAGELIST		hImageLayerListboxIcons;

	HIMAGELIST		hImageColourModeFlyoff;
	HIMAGELIST		hImageAddLayerFlyoff;

	HWND			hListClipLayers;

	ICustEdit		*edtClipName;
	ISpinnerControl *spnGlobalWeight;
	ISpinnerControl *spnLocalWeight;
	ISpinnerControl *spnTimeWarp;

	ICustButton		*btnTransformGizmo;
	ICustButton		*btnCollapsePoseToLayer;
	ICustButton		*btnCollapseToLayer;
	ICustButton		*btnShowRangeView;
	ICustButton		*btnShowGlobalWeightsView;
	ICustButton		*btnShowLocalWeightsView;
	ICustButton		*btnShowTimeWarpsView;
	ICustButton		*btnLayerManagerWindow;

	IColorSwatch	*swatchLayerColour;

	HWND			hChkIgnore;
	HWND			hChkSolo;
	//	HWND			hChkRelWorld;
	HWND			hChkEnableGlobalWeights;

	HBITMAP			hBitmapTickANDBits;
	HBITMAP			hBitmapTickORBits;

	MaskedBitmap	*pmbmTickIcon;
	MaskedBitmap	*pmbmSoloIcon;
	MaskedBitmap	*pmbmCATMotionLayerIcon;
	MaskedBitmap	*pmbmAbsoluteLayerIcon;
	MaskedBitmap	*pmbmRelativeLayerIcon;
	MaskedBitmap	*pmbmRelativeWorldLayerIcon;
	MaskedBitmap	*pmbmLayerColourIcon;
	MaskedBitmap	*pmbmLayerColourFrameIcon;

	BOOL InitControls(HWND hWnd, CATClipRoot *clipRoot);
	void ReleaseControls(HWND hWnd);

	void RefreshAddLayerButtonState(int nLayer);
	void RefreshRemoveButtonState(int nLayer);
	void RefreshMoveUpDownButtonState(int nLayer);
	void RefreshGhostButtonState(int nLayer);
	void RefreshKeyClipButtonState(int nLayer);
	void RefreshLayerColourSwatchState(int nLayer);
	void RefreshIgnoreSoloStates(int nLayer);
	void RefreshLayerNameEdit(int nLayer);
	void RefreshSpinners(int nLayer);
	void RefreshCurrLayer(int nLayer);

	ICustButton		*flyColourModes;
	ICustButton		*flyAddLayer;

	ReferenceTarget* mGlobalCtrl;
	ReferenceTarget* mLocalCtrl;

	enum CONTROLLIST {
		GLOBAL,
		LOCAL,
		NUM_CONTROLS
	};

public:
	void RefreshCATRigModeButtonState();
	void RefreshColourModeButtonState();

	void RefreshClipLayersList(int nFrom = 0, int nTo = -1);
	void SelectLayer(int nLayer, int bAlreadySelected = FALSE, int bUpdateColours = TRUE);

	void TimeChanged(TimeValue t) {
		if (pClipRoot != NULL)
		{
			tvTime = t;

			// Updates all time-dependent controls
			RefreshClipLayersList();

			int nLayer = pClipRoot->GetSelectedLayer();
			RefreshRemoveButtonState(nLayer);
			RefreshSpinners(nLayer);
		}
	}

	HWND GetHwnd() { return hDlg; }

	ClipRolloutData() : btnShowGlobalWeightsView(NULL)
		, btnShowLocalWeightsView(NULL)
		, btnShowTimeWarpsView(NULL)
		, hBitmapTickANDBits(NULL)
		, hBitmapTickORBits(NULL)
		, hImageAddLayerFlyoff(NULL)
		, hImageCATModes(NULL)
		, hImageColourModeFlyoff(NULL)
		, hImageLayerListboxIcons(NULL)
		, hChkEnableGlobalWeights(NULL)
		, hChkIgnore(NULL)
		, hChkSolo(NULL)
		, pmbmAbsoluteLayerIcon(NULL)
		, pmbmCATMotionLayerIcon(NULL)
		, pmbmLayerColourIcon(NULL)
		, pmbmLayerColourFrameIcon(NULL)
		, pmbmRelativeLayerIcon(NULL)
		, pmbmRelativeWorldLayerIcon(NULL)
		, pmbmSoloIcon(NULL)
		, pmbmTickIcon(NULL)

	{
		catparenttrans = NULL;
		pClipRoot = NULL;
		hDlg = NULL;

		btnCATWiki = NULL;

		flyCATMode = NULL;
		btnCATWindow = NULL;

		btnRemoveLayer = NULL;
		btnMoveUp = NULL;
		btnMoveDown = NULL;

		btnCopyLayer = NULL;
		btnPasteLayer = NULL;

		hListClipLayers = NULL;

		edtClipName = NULL;
		spnGlobalWeight = NULL;
		spnLocalWeight = NULL;
		spnTimeWarp = NULL;

		btnCollapsePoseToLayer = NULL;
		btnCollapseToLayer = NULL;
		btnTransformGizmo = NULL;
		btnShowRangeView = NULL;
		btnLayerManagerWindow = NULL;

		swatchLayerColour = NULL;

		tvTime = 0;

		flyColourModes = NULL;
		flyAddLayer = NULL;

		mGlobalCtrl = NULL;
		mLocalCtrl = NULL;
	};

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	virtual int NumRefs() { return NUM_CONTROLS; }

	virtual RefTargetHandle GetReference(int i)
	{
		switch (i)
		{
		case GLOBAL: return mGlobalCtrl; break;
		case LOCAL: return mLocalCtrl; break;
		default: return nullptr;
		}
	}

protected:

	virtual void SetReference(int i, RefTargetHandle rtarg)
	{
		switch (i)
		{
		case GLOBAL: mGlobalCtrl = rtarg; break;
		case LOCAL: mLocalCtrl = rtarg; break;
		default: DbgAssert(false);
		}
	}

	virtual RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL)
	{
		TimeChanged(GetCOREInterface()->GetTime());
		return REF_SUCCEED;
	}

};

// Declare a static global instance of the clip rollout data,
// so that the clip root can use it in EndEditParams() to
// delete the rollout (instead of having to remember the HWND
// returned by Interface::AddRollupPage().  Making it static
// of course means that there can only ever be one clip rollout
// active at a time.
static ClipRolloutData staticClipRollout;
static HWND hwndOldMouseCapture;

BOOL OpenLayerManagerWindow(INode* catparent_node)
{
	ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
	MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
	four_value_locals_tls(name, fn, catparent_node, result);

	try
	{
		ScopedSaveCurrentFrames scopedSaveCurrentFrames(_tls);
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("CreateCATLayerManager"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		if (vl.fn != NULL && vl.fn->tag == class_tag(MAXScriptFunction)) {
			Value* args[3];

			// CaptureAnimation takes 4 parameters, the catparents node
			// src hierarchy node,
			// and an optional keyword paramter, replace, which tells
			// convertToArchMat whether to replace all reference to
			// the old material by the new one.
			args[0] = &keyarg_marker;								// Separates keyword params from mandatory
			args[1] = vl.catparent_node = Name::intern(_T("catparentnode"));	// Keyword "catparentnode"
			args[2] = MAXNode::intern(catparent_node);

			// Call the funtion and save the result.
			vl.result = static_cast<MAXScriptFunction*>(vl.fn)->apply(args, 3);
		}
	}
	catch (MAXScriptException& e)
	{
		ProcessMAXScriptException(e, _T("CreateCATLayerManager Script"), false, false, true);
	}
	catch (...)
	{
		ProcessMAXScriptException(UnknownSystemException(), _T("CreateCATLayerManager Script"), false, false, true);
	}

	// converted will be NULL if the conversion failed.
	return TRUE;
}

BOOL RefreshLayerManagerWindow(INode* catparent_node)
{
	ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
	MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
	four_value_locals_tls(name, fn, catparent_node, result);

	try
	{
		ScopedSaveCurrentFrames scopedSaveCurrentFrames(_tls);
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("RefreshCATLayerManager"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		if (vl.fn != NULL && vl.fn->tag == class_tag(MAXScriptFunction)) {
			Value* args[3];

			// CaptureAnimation takes 4 parameters, the catparents node
			// src hierarchy node,
			// and an optional keyword paramter, replace, which tells
			// convertToArchMat whether to replace all reference to
			// the old material by the new one.
			args[0] = &keyarg_marker;								// Separates keyword params from mandatory
			args[1] = vl.catparent_node = Name::intern(_T("catparentnode"));	// Keyword "catparentnode"
			args[2] = MAXNode::intern(catparent_node);

			// Call the funtion and save the result.
			vl.result = static_cast<MAXScriptFunction*>(vl.fn)->apply(args, 3);
		}
	}
	catch (MAXScriptException& e)
	{
		ProcessMAXScriptException(e, _T("RefreshCATLayerManager Script"), false, false, true);
	}
	catch (...)
	{
		ProcessMAXScriptException(UnknownSystemException(), _T("RefreshCATLayerManager Script"), false, false, true);
	}

	// converted will be NULL if the conversion failed.
	return TRUE;
}

BOOL DoCollapse(INode* catparent_node)
{
	ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
	MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
	four_value_locals_tls(name, fn, catparent_node, result);

	try
	{
		ScopedSaveCurrentFrames scopedSaveCurrentFrames(_tls);
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("CATCollapseLayers"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		if (vl.fn != NULL && vl.fn->tag == class_tag(MAXScriptFunction)) {
			Value* args[2];

			// CaptureAnimation takes 4 parameters, the catparents node
			// src hierarchy node,
			// and an optional keyword paramter, replace, which tells
			// convertToArchMat whether to replace all reference to
			// the old material by the new one.
			args[0] = vl.catparent_node = MAXNode::intern(catparent_node);
			args[1] = &keyarg_marker;								// Separates keyword params from mandatory

			// Call the funtion and save the result.
			vl.result = static_cast<MAXScriptFunction*>(vl.fn)->apply(args, 2);
		}
	}
	catch (MAXScriptException& e)
	{
		ProcessMAXScriptException(e, _T("CATCollapseLayers Script"), false, false, true);
	}
	catch (...)
	{
		ProcessMAXScriptException(UnknownSystemException(), _T("CATCollapseLayers Script"), false, false, true);
	}

	// converted will be NULL if the conversion failed.
	return TRUE;
}

/*************************
 * LoadClip dialogue
*/
class CopyInstanceDlg
{
private:
	CATClipRoot* pClipRoot;
	HWND hWnd;
public:

	CopyInstanceDlg() : hWnd(NULL), pClipRoot(NULL) {
	}

	void InitControls() {
		SendMessage(GetDlgItem(hWnd, IDC_RDO_COPY), BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	}

	void ReleaseControls() {
	}

	void SetClipRoot(CATClipRoot *pClipRoot) {
		this->pClipRoot = pClipRoot;
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM)
	{
		static HWND hwndOldMouseCapture;
		switch (uMsg) {
		case WM_INITDIALOG:

			this->hWnd = hWnd;

			// Take away any mouse capture that was active.
			hwndOldMouseCapture = GetCapture();
			SetCapture(NULL);

			// Centre the dialog in its parent window and set the
			// focus to the OK button, if it exists.
			CenterWindow(hWnd, GetParent(hWnd));
			if (GetDlgItem(hWnd, IDC_BTN_OK)) SetFocus(GetDlgItem(hWnd, IDC_BTN_OK));
			InitControls();

			return FALSE;

		case WM_DESTROY:
			ReleaseControls();

			// Restore the mouse capture.
			SetCapture(hwndOldMouseCapture);
			return TRUE;

		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
			{
				switch (LOWORD(wParam)) {
				case IDC_RDO_COPY:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_INSTANCE), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					break;

				case IDC_RDO_INSTANCE:
					SendMessage(GetDlgItem(hWnd, IDC_RDO_COPY), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					break;

				case IDC_BTN_OK:
				{
					BOOL instance = FALSE;
					if (SendMessage(GetDlgItem(hWnd, IDC_RDO_INSTANCE), BM_GETCHECK, 0, 0) == BST_CHECKED)
						instance = TRUE;
					int targetLayerIdx = pClipRoot->GetSelectedLayer() + 1;
					pClipRoot->PasteLayer(instance, 1, targetLayerIdx, NULL);
				}
				case IDC_BTN_CANCEL:
					EndDialog(hWnd, LOWORD(wParam));
					return TRUE;
				}
			}
			break;
			}
		}
		return FALSE;
	}
};

static CopyInstanceDlg CopyInstanceDlgClass;

static INT_PTR CALLBACK CopyInstanceDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return CopyInstanceDlgClass.DlgProc(hWnd, message, wParam, lParam);
};

/************************************************************************/
/* End CollapseRollout stuff                                            */
/************************************************************************/

// This is the actual callback that just calls the class method
// of the dialog callback function.
//
static INT_PTR CALLBACK ClipRolloutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return staticClipRollout.DlgProc(hWnd, message, wParam, lParam);
};

INT_PTR CALLBACK ClipRolloutData::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int weightTxtExtent = -1;

	int nLayer = (int)SendMessage(hListClipLayers, LB_GETCURSEL, 0, 0);
	if (pClipRoot && nLayer == pClipRoot->NumLayers()) nLayer = -1;

	switch (message) {
	case WM_INITDIALOG:
		if (!InitControls(hWnd, (CATClipRoot*)lParam))
			DestroyWindow(hWnd);
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return FALSE;
	case WM_DESTROY:
		ReleaseControls(hWnd);
		break;
	case WM_CUSTEDIT_ENTER:
		//		if (HIWORD(wParam)==EN_CHANGE) {
		if (nLayer >= 0 && edtClipName->GotReturn()) {
			TCHAR strbuf[128];
			edtClipName->GetText(strbuf, 128);
			pClipRoot->RenameLayer(nLayer, strbuf);
			//	RefreshClipLayersList(nLayer, nLayer);
		}
		//		}
		break;
	case CC_COLOR_SEL:
		// Reset the NotifyAfterAccept.  Our colour
		// swatch will only notify before or after, it
		// seems incapable of doing both.  We want
		// to update dynamically, so set to interactive mode
		swatchLayerColour->SetNotifyAfterAccept(FALSE);
		theHold.Begin();
		break;
	case CC_COLOR_CHANGE:
		if (LOWORD(wParam) == IDC_LAYER_COLOUR)
		{
			Color clr(swatchLayerColour->GetColor());
			pClipRoot->GetLayer(nLayer)->SetColour(clr);
			GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
		}
		break;
	case CC_COLOR_CLOSE:
	{
		Color clr(swatchLayerColour->GetColor());

		// Have we been canceled or accepted?  If only there
		// were some way to tell!!
		// We work around this by simply canceling whatever has
		// been set
		theHold.Cancel();
		// Now we need to find the final value.  We -could- take
		// the current value and set it, except that if the value
		// was canceled, the swatch hasn't been updated yet.
		// So we dodge this by changing the NotifyAfterAccept to true
		// Now we get one more message to actually set the value.
		// If we didn't set this, we'd also get one more message
		// to cancel the color set, but notifying that the color has changed
		// back to the original.  Freaking brilliant.
		swatchLayerColour->SetNotifyAfterAccept(TRUE);

		// The cancel above will cause the muscle to refresh the UI,
		// which will refresh the colour swatch with the undone colour.
		// Re-set the user selected colour back to the colour swatch,
		// so that the final notify will get the correct colour.
		swatchLayerColour->SetColor(clr, FALSE);
	}
	break;
	// If a spinner has been pressed on start a new undo
	case CC_SPINNER_BUTTONDOWN:
		if (LOWORD(wParam) == IDC_SPIN_CLIPWEIGHT)
			GetCOREInterface()->RedrawViews(tvTime, REDRAW_BEGIN);
		theHold.Begin();
		return TRUE;
	case CC_SPINNER_BUTTONUP:
		switch (LOWORD(wParam))
		{
		case IDC_SPIN_CLIPWEIGHT:
			if (HIWORD(wParam)) {
				theHold.Accept(GetString(IDS_HLD_GLOBWGT));
				catparenttrans->UpdateColours();
				GetCOREInterface()->RedrawViews(tvTime, REDRAW_END);
			}
			else				theHold.Cancel();
			break;
		case IDC_SPIN_CLIPWEIGHTLOCAL:
			if (HIWORD(wParam))	theHold.Accept(GetString(IDS_HLD_LOCALWGT));
			else				theHold.Cancel();
			break;
		case IDC_SPIN_TIMEWARP:
			if (HIWORD(wParam))	theHold.Accept(GetString(IDS_HLD_TIMEWARP));
			else				theHold.Cancel();
			break;
		}
		break;
	case CC_SPINNER_CHANGE:
		switch (LOWORD(wParam)) {
		case IDC_SPIN_CLIPWEIGHT:
			// Change the weight in the hierarchy when spinning,
			// but only update the clip layers list box once the
			// drag operation is complete.
			if (nLayer >= 0) {
				// If we are here from a cust-edit, we are not holding yet.
				HoldActions hold(IDS_HLD_GLOBWGT);
				float dNewWeight = spnGlobalWeight->GetFVal() / 100.0f;
				pClipRoot->GetLayer(nLayer)->SetWeight(tvTime, dNewWeight);
			}
			break;
		case IDC_SPIN_CLIPWEIGHTLOCAL:
			// Change the weight in the hierarchy when spinning,
			// but only update the clip layers list box once the
			// drag operation is complete.
			if (nLayer >= 0) {
				// If we are here from a cust-edit, we are not holding yet.
				HoldActions hold(IDS_HLD_LOCALWGT);
				float dNewWeight = spnLocalWeight->GetFVal() / 100.0f;
				pClipRoot->SetLayerWeightLocal(nLayer, tvTime, dNewWeight);
			}
			break;
		case IDC_SPIN_TIMEWARP:
			if (nLayer >= 0) {
				// If we are here from a cust-edit, we are not holding yet.
				HoldActions hold(IDS_HLD_TIMEWARP);
				float dNewTime = spnTimeWarp->GetFVal();
				pClipRoot->GetLayer(nLayer)->SetTime(GetCOREInterface()->GetTime(), dNewTime*GetTicksPerFrame());
				catparenttrans->UpdateCharacter();
				catparenttrans->UpdateColours(FALSE);
				GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			}
			break;

		}
		break;

	case WM_COMMAND:
		switch (HIWORD(wParam)) {
			// This section handles all the ordinary button things.
		case BN_BUTTONUP:
			switch (LOWORD(wParam)) {
			case IDC_BTN_CATWINDOW:
			{
				NLAInfo* pInfo = pClipRoot->GetLayer(nLayer);
				ICATHierarchyRoot* pCATRoot = GetICATHierarchyRootInterface(pInfo);
				DbgAssert(pCATRoot != NULL);
				if (pInfo != NULL)
					pCATRoot->ICreateCATWindow();
				break;
			}
			case IDC_BTN_REMOVE:
				// Removes a layer from the hierarchy and selects the layer
				// that takes its place, or the last layer in the list (this
				// is all handled by SelectLayer()).
				pClipRoot->RemoveLayer(nLayer);
				break;
			case IDC_BTN_MOVE_UP:
				// Moves the current layer up one space and selects it again.
				// Note we must deselect all layers before this operation, in
				// order to kill any UIs associated with the selected layer.
				if (nLayer > 0) {
					pClipRoot->MoveLayerUp(nLayer);
				}
				break;

			case IDC_BTN_MOVE_DOWN:
				// Moves the current layer down one space and selects it again.
				// Note we must deselect all layers before this operation, in
				// order to kill any UIs associated with the selected layer.
				if (nLayer >= 0 && nLayer + 1 < pClipRoot->NumLayers()) {
					pClipRoot->MoveLayerDown(nLayer);
				}
				break;

				// Show the ghost for this layer.
			case IDC_BTN_GHOST:
				if (nLayer >= 0) {
					HoldActions hold(IDS_TT_LYRGIZMO);
					if (btnTransformGizmo->IsChecked()) {
						// Create the ghostie and update the ghost button, in
						// case the ghost thingy failed (so it will stay unchecked).
						pClipRoot->GetLayer(nLayer)->CreateTransformNode();
						RefreshGhostButtonState(nLayer);
					}
					else {
						// Delete the ghostie and update the ghost button, in
						// case it failed (so it will stay checked).
						pClipRoot->GetLayer(nLayer)->DestroyTransformNode();
						RefreshGhostButtonState(nLayer);
					}
					GetCOREInterface()->RedrawViews(tvTime);
				}
				break;
				// Key the character's current pose into the selected layer.
			case IDC_BTN_KEYCLIP:
				// here we are goin to pop up the Collpase Dialogue
				DbgAssert(catparenttrans);
				catparenttrans->CollapsePoseToCurLayer(GetCOREInterface()->GetTime());
				break;
			case IDC_BTN_COLLAPSELAYER:
				DbgAssert(catparenttrans);
				//		CollapseDlgClass.SetClipRoot(pClipRoot);
				//		DialogBox(hInstance, MAKEINTRESOURCE(IDD_COLLAPSELAYER_MODAL), GetCOREInterface()->GetMAXHWnd(), CollapseDlgProc);
				DoCollapse(catparenttrans->GetNode());
				break;
			case IDC_BTN_RANGEVIEW:
				pClipRoot->ShowRangesView();
				break;
			case IDC_BTN_LAYERMANAGER_WINDOW:
				OpenLayerManagerWindow(catparenttrans->GetNode());
				break;
			case IDC_BTN_GLOBAL_WEIGHTSVIEW:
				pClipRoot->ShowGlobalWeightsView();
				break;
			case IDC_BTN_LOCAL_WEIGHTSVIEW:
				pClipRoot->ShowLocalWeightsView(pClipRoot->localBranchWeights);
				break;
			case IDC_BTN_GRAPH_TIMEWARP:
				pClipRoot->ShowTimeWarpView();
				break;
			case IDC_BTN_COPY:
				pClipRoot->CopyLayer(nLayer);
				btnPasteLayer->Enable();
				break;
			case IDC_BTN_PASTE:
				if (pClipRoot->IsLayerCopied()) {
					CopyInstanceDlgClass.SetClipRoot(pClipRoot);
					DialogBox(hInstance, MAKEINTRESOURCE(IDD_COPY_INSTANCE), GetCOREInterface()->GetMAXHWnd(), CopyInstanceDlgProc);
					//	pClipRoot->PasteLayer(FALSE);
					RefreshClipLayersList();
					RefreshCATRigModeButtonState();
				}
				break;
			case IDC_BTN_SETUPMODE:
				if (!theHold.Holding())
					theHold.Begin();

				catparenttrans->SetCATMode(flyCATMode->IsChecked() ? NORMAL : SETUPMODE);
				// TODO: this callback should be Called "LayerSystemChanged"
				pClipRoot->NotifyActiveLayerChangeCallbacks();
				GetCOREInterface()->RedrawViews(tvTime);
				if (theHold.Holding())
					theHold.Accept(GetString(IDS_HLD_CATMODE));
				break;
			}
			break;
		case BN_FLYOFF:
			switch (LOWORD(wParam)) {
			case IDC_FLY_COLOURMODES:
				switch (flyColourModes->GetCurFlyOff()) {
				case 0: catparenttrans->SetColourMode(COLOURMODE_CLASSIC);	break;
				case 1: catparenttrans->SetColourMode(COLOURMODE_BLEND);	break;
					break;
				}
				break;
			case IDC_FLY_ADDLAYER:
				if (nLayer == -1) // nothing selected - add to end
					nLayer = pClipRoot->NumLayers();
				else nLayer++; // add below selected
				switch (flyAddLayer->GetCurFlyOff()) {
				case 0:
					pClipRoot->InsertLayer(GetString(IDS_LYR_ANIM), nLayer, LAYER_ABSOLUTE);
					break;
				case 1:
					pClipRoot->InsertLayer(GetString(IDS_LYR_ADJ), nLayer, LAYER_RELATIVE);
					break;
				case 2:
					pClipRoot->InsertLayer(GetString(IDS_LYR_ADJ_WORLD), nLayer, LAYER_RELATIVE_WORLD);
					break;
				case 3:
					pClipRoot->InsertLayer(GetString(IDS_LYR_CATMOTION), nLayer, LAYER_CATMOTION);
					break;
				}
				break;
			}
			break;
			// This section handles the radio buttons.  When the radio changes,
			// we have to update the list as well, so the "Abs"/"Rel" display
			// reflects the change.  Also refresh the ghost button and key clips
			// button in case either of those may somehow depend on the method.
		case BN_CLICKED:
			switch (LOWORD(wParam)) {
			case IDC_CHK_IGNORE:
				if (nLayer >= 0) {
					pClipRoot->GetLayer(nLayer)->EnableLayer(!IS_CHECKED(hDlg, IDC_CHK_IGNORE));
					GetCOREInterface()->RedrawViews(tvTime);
				}
				break;
			case IDC_CHK_SOLO:
				if (nLayer >= 0) {
					if (IS_CHECKED(hDlg, IDC_CHK_SOLO)) 	pClipRoot->SoloLayer(nLayer);
					else									pClipRoot->SoloLayer(-1);
					RefreshClipLayersList();
				}
				break;
			case IDC_CHK_ENABLEGLOBALWEIGHTS:
				if (nLayer >= 0) {
					if (IS_CHECKED(hDlg, IDC_CHK_ENABLEGLOBALWEIGHTS))
						pClipRoot->GetLayer(nLayer)->ClearFlag(LAYER_DISABLE_WEIGHTS_HIERARCHY);
					else pClipRoot->GetLayer(nLayer)->SetFlag(nLayer, LAYER_DISABLE_WEIGHTS_HIERARCHY);
					RefreshSpinners(nLayer);
					break;
				}
#ifdef DISPLAY_LAYER_ROLLOUTS_TOGGLE
			case IDC_CHK_DISPLAY_LAYER_ROLLOUTS:
				pClipRoot->ShowLayerRollouts(IS_CHECKED(hDlg, IDC_CHK_DISPLAY_LAYER_ROLLOUTS));
				break;
#endif
			}
			break;

			// This section handles the clip layer list box stuff.
		case LBN_SELCHANGE:
			switch (LOWORD(wParam)) {
			case IDC_LBX_CLIPLAYERS:
				SelectLayer(nLayer, nLayer != LB_ERR);
				break;
			}
			break;
		case LBN_DBLCLK:
			switch (LOWORD(wParam)) {
			case IDC_LBX_CLIPLAYERS:
				if (nLayer >= 0) {
					pClipRoot->GetLayer(nLayer)->EnableLayer(!pClipRoot->GetLayer(nLayer)->LayerEnabled());
					GetCOREInterface()->RedrawViews(tvTime);
				}
				break;
			}
			break;
		}
		break;

		// This is associated with owner-drawn list-boxes.
		//
		// IMPORTANT: This message is sent before WM_INITDIALOG, so we cannot
		// use any of our class members as they will be uninitialised.
	case WM_MEASUREITEM:
		if (IDC_LBX_CLIPLAYERS != wParam)
			return FALSE;
		FastListMeasureItem(GetDlgItem(hWnd, wParam), wParam, lParam);
		return TRUE;

	case WM_DRAWITEM:
		switch (wParam) {
		case IDC_LBX_CLIPLAYERS:
		{
			LPDRAWITEMSTRUCT lpdi = (LPDRAWITEMSTRUCT)lParam;

			if (lpdi->itemAction != ODA_FOCUS) {
				HBRUSH hBackgroundBrush;
				HBRUSH hForegroundBrush;
				HBRUSH hDisabledBrush;
				HBRUSH hSoloBrush = CreateSolidBrush(RGB(100, 0, 0));
				COLORREF colBackgroundCol;
				COLORREF colForegroundCol;
				COLORREF colDisabledCol;
				COLORREF colSoloCol = 0;

				int nThisLayer = lpdi->itemID;
				if (nThisLayer == pClipRoot->NumLayers()) nThisLayer = -1;

				// Erase the background of the item and choose text
				// foreground and background colours.
				if (lpdi->itemState & ODS_SELECTED) {
					hForegroundBrush = GetSysColorBrush(COLOR_HIGHLIGHTTEXT);
					hBackgroundBrush = GetSysColorBrush(COLOR_HIGHLIGHT);
					hDisabledBrush = GetSysColorBrush(COLOR_GRAYTEXT);
					colForegroundCol = GetSysColor(COLOR_HIGHLIGHTTEXT);
					colBackgroundCol = GetSysColor(COLOR_HIGHLIGHT);
					colDisabledCol = GetSysColor(COLOR_GRAYTEXT);
					colSoloCol = RGB(100, 0, 0);
				}
				else {
					hForegroundBrush = ColorMan()->GetBrush(kWindowText);
					colForegroundCol = ColorMan()->GetColor(kWindowText);
					hDisabledBrush = GetSysColorBrush(COLOR_GRAYTEXT);
					hBackgroundBrush = ColorMan()->GetBrush(kWindow);
					colBackgroundCol = ColorMan()->GetColor(kWindow);
					colDisabledCol = GetSysColor(COLOR_GRAYTEXT);
				}

				SetTextColor(lpdi->hDC, colForegroundCol);
				SetBkColor(lpdi->hDC, colBackgroundCol);

				int nDrawCentre;
				RECT rcDrawRect = lpdi->rcItem;
				FillRect(lpdi->hDC, &rcDrawRect, hBackgroundBrush);
				nDrawCentre = rcDrawRect.top + (rcDrawRect.bottom - rcDrawRect.top + 1) / 2;

				if (nThisLayer < pClipRoot->NumLayers()) {
					NLAInfo *layerinfo = (nThisLayer >= 0) ? pClipRoot->GetLayer(nThisLayer) : NULL;
					int nSoloLayer = pClipRoot->GetSoloLayer();
					bool bLayerIsPartOfSolo = false;
					bool bLayerDisabled = (layerinfo ? !layerinfo->LayerEnabled() : false);

					if (nThisLayer >= 0 && nSoloLayer >= 0 && layerinfo) {
						if (nSoloLayer == nThisLayer) {
							bLayerIsPartOfSolo = true;
							bLayerDisabled = false;
							// PT
							hForegroundBrush = hSoloBrush;
						}
						else if (nSoloLayer == layerinfo->GetParentLayer()) {
							bLayerIsPartOfSolo = true;
							// PT
							hForegroundBrush = hSoloBrush;
						}
						else {
							// If the layer is not part of the solo make it a disabled sorta colour.
							hForegroundBrush = hDisabledBrush;
							colForegroundCol = colDisabledCol;
							SetTextColor(lpdi->hDC, colForegroundCol);
						}
					}

					// If the layer is disabled, make it a disabled sorta colour.
					if (bLayerDisabled) {
						hForegroundBrush = hDisabledBrush;
						colForegroundCol = colDisabledCol;
						SetTextColor(lpdi->hDC, colForegroundCol);
					}

					// get off the left
					rcDrawRect.left += MaxSDK::UIScaled(2);

					// Choose the appropriate layer method icon and draw it.
					MaskedBitmap *pmbmLayerIcon = NULL;
					if (!layerinfo) pmbmLayerIcon = NULL;
					else if (layerinfo->GetMethod() == LAYER_CATMOTION)			pmbmLayerIcon = pmbmCATMotionLayerIcon;
					else if (layerinfo->GetMethod() == LAYER_ABSOLUTE)			pmbmLayerIcon = pmbmAbsoluteLayerIcon;
					else if (layerinfo->GetMethod() == LAYER_RELATIVE)			pmbmLayerIcon = pmbmRelativeLayerIcon;
					else if (layerinfo->GetMethod() == LAYER_RELATIVE_WORLD)	pmbmLayerIcon = pmbmRelativeWorldLayerIcon;
					//	else {
				//		pmbmLayerIcon = pClipRoot->GetLayer(nThisLayer)->GetIcon();
				//	}

					if (pmbmLayerIcon) {
						HBRUSH hBrush = hForegroundBrush;
						if (bLayerIsPartOfSolo && !(lpdi->itemState & ODS_SELECTED))
							hBrush = CreateSolidBrush(RGB(240, 0, 0));

						pmbmLayerIcon->MaskBlit(lpdi->hDC, rcDrawRect.left, nDrawCentre - pmbmLayerIcon->Height() / 2, hBrush);

						if (bLayerIsPartOfSolo && !(lpdi->itemState & ODS_SELECTED))
							DeleteObject(hBrush);
					}

					rcDrawRect.left += pmbmCATMotionLayerIcon->Width() + MaxSDK::UIScaled(1);

					// Draw the layer colour box
					if (nThisLayer >= 0 && layerinfo) {
						if (layerinfo->ApplyAbsolute()) {
							pmbmLayerColourFrameIcon->MaskBlit(
								lpdi->hDC,
								rcDrawRect.left,
								nDrawCentre - pmbmLayerColourFrameIcon->Height() / 2,
								hForegroundBrush);

							HBRUSH hLayerColourBrush = CreateSolidBrush(pClipRoot->GetLayer(nThisLayer)->GetColour().toRGB());
							pmbmLayerColourIcon->MaskBlit(
								lpdi->hDC,
								rcDrawRect.left,
								nDrawCentre - pmbmLayerColourIcon->Height() / 2,
								hLayerColourBrush);
							DeleteObject(hLayerColourBrush);
						}
					}

					rcDrawRect.left += pmbmLayerColourFrameIcon->Width() + MaxSDK::UIScaled(1);

					// If the layer is disabled, make it a disabled sorta colour.
					if (bLayerDisabled) SetTextColor(lpdi->hDC, colDisabledCol);
					else if (bLayerIsPartOfSolo) SetTextColor(lpdi->hDC, colSoloCol);

					// Draw the global weight.  Use the average width of the font to
					// determine how large the weights rectangle should be.

					if (weightTxtExtent < 0)
					{
						HDC hdcListBox = GetDC(hListClipLayers);
						SIZE size;
						DLGetTextExtent(hdcListBox, _T("100%"), &size);
						weightTxtExtent = size.cx;
						ReleaseDC(hListClipLayers, hdcListBox);
					}
					RECT rcWeight = rcDrawRect;
					rcWeight.right = rcWeight.left + weightTxtExtent;
					rcDrawRect.left = rcWeight.right + MaxSDK::UIScaled(2);

					if (nThisLayer >= 0) {
						TCHAR strWeight[10] = { 0 };
						_stprintf(strWeight, _T("%d%%"), (int)(pClipRoot->GetCombinedLayerWeightLocal(nThisLayer, tvTime) * 100.0f + 0.5f));
						DrawText(lpdi->hDC, strWeight, (int)_tcslen(strWeight), &rcWeight, DT_RIGHT);
					}

					// Draw the layer name.
					RECT rcLayerName = rcDrawRect;
					tstring strLabel;

					if (nThisLayer >= 0 && layerinfo != NULL)
						strLabel = layerinfo->GetName().data();
					else
						strLabel = GetString(IDS_AVAILABLE);

					DrawText(lpdi->hDC, strLabel.c_str(), (int)strLabel.length(), &rcLayerName, 0);
				}
				DeleteObject(hSoloBrush);

				return TRUE;
			}
		}
		// We make DefWindowProc() handle anything we didn't process,
		// such as drawing the focus rectangle.
		return FALSE;
		}
		break;

	case WM_NOTIFY:
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

// Initialises all Max custom controls for the dialog, and grabs
// necessary window handles for other controls.  Also stores initial
// data assosciated with the dialog (ie, the two parameters passed).
//
// Returns: TRUE if successful,
//          FALSE if the dialog is already open.
//

BOOL ClipRolloutData::InitControls(HWND hWnd, CATClipRoot *clipRoot)
{
	//	HBITMAP hBitmap, hMask;
	HDC hdc;
	//	MaxBmpFileIcon* pIcon;

		// Warn if there's already a clip rollout open.
	DbgAssert(hDlg == NULL);
	// Fail if we don't have our cliproot
	if (!clipRoot) return FALSE;

	pClipRoot = clipRoot;
	catparenttrans = pClipRoot->GetCATParentTrans();
	hDlg = hWnd;

	// Any operations that require the window's DC will use this.
	// We must remember to release it at the end of this function.
	hdc = GetDC(hWnd);

	// Register a time change callback, and initialise our
	// local cache of the current time.
	GetCOREInterface()->RegisterTimeChangeCallback(this);
	tvTime = GetCOREInterface()->GetTime();

	// Initialise the INI files so we can read button text and tooltips
	CatDotIni catCfg;

	// Create the tick icon, used for drawing the navigator list boxes.
	// We should probably do this before doing anything with those boxes!
	// These must align on at least a SHORT boundary
	static const BYTE lpTickIconBits_8x8[] = {
		// 8 4 2 1|8 4 2 1
0x02, 0x00,		// - - - - - - X -
0x06, 0x00,		// - - - - - X X -
0x8e, 0x00,		// X - - - X X X -
0xdc, 0x00,		// X X - X X X - -
0xf8, 0x00,		// X X X X X - - -
0x70, 0x00,		// - X X X - - - -
0x20, 0x00,		// - - X - - - - -
0x00, 0x00,		// - - - - - - - -
	};

	static const BYTE lpTickIconBits_6x6[] = {
		// 8 4 2 1|8 4 x x
0x08, 0x00,		// - - - - X - - -
0x18, 0x00,		// - - - X X - - -
0xb0, 0x00,		// X - X X - - - -
0xe0, 0x00,		// X X X - - - - -
0x40, 0x00,		// - X - - - - - -
0x00, 0x00,		// - - - - - - - -
	};

	static const BYTE lpSoloIconBits_6x6[] = {
		// 8 4 2 1|8 4 x x
0x70, 0x00,		// - X X X - - - -
0xf8, 0x00,		// X X X X X - - -
0xf8, 0x00,		// X X X X X - - -
0xf8, 0x00,		// X X X X X - - -
0x70, 0x00,		// - X X X - - - -
0x00, 0x00,		// - - - - - - - -
	};

	static const BYTE lpCATMotionLayerIconBits_16x12[] = {
		// 8 4 2 1|8 4 2 1|8 4 2 1|8 4 2 1
0x00, 0x1c,		// - - - - - - - - - - - X X X - -
0x00, 0x1c,		// - - - - - - - - - - - X X X - -
0x00, 0xcc,		// - - - - - - - - X X - - X X - -
0x01, 0xf0,		// - - - - - - - X X X X X - - - -
0x01, 0x3a,		// - - - - - - - X - - X X X - X -
0x00, 0x6e,		// - - - - - - - - - X X - X X X -
0x00, 0x6c,		// - - - - - - - - - X X - X X - -
0x03, 0x60,		// - - - - - - X X - X X - - - - -
0x07, 0xf0,		// - - - - - X X X X X X X - - - -
0x01, 0xd8,		// - - - - - - - X X X - X X - - -
0x00, 0x18,		// - - - - - - - - - - - X X - - -
0x00, 0x1c,		// - - - - - - - - - - - X X X - -
	};

	static const BYTE lpAbsoluteLayerIconBits_16x12[] = {
		// 8 4 2 1|8 4 2 1|8 4 2 1|8 4 2 1
0x00, 0x00,		// - - - - - - - - - - - - - - - -
0x00, 0x00,		// - - - - - - - - - - - - - - - -
0x72, 0x00,		// - X X X - - X - - - - - - - - -
0x8a, 0x00,		// X - - - X - X - - - - - - - - -
0x8a, 0x00,		// X - - - X - X - - - - - - - - -
0x8b, 0x8e,		// X - - - X - X X X - - - X X X -
0xfa, 0x50,		// X X X X X - X - - X - X - - - -
0x8a, 0x5c,		// X - - - X - X - - X - X X X - -
0x8a, 0x42,		// X - - - X - X - - X - - - - X -
0x8b, 0x9c,		// X - - - X - X X X - - X X X - -
0x00, 0x00,		// - - - - - - - - - - - - - - - -
0x00, 0x00,		// - - - - - - - - - - - - - - - -
	};

	static const BYTE lpRelativeWorldLayerIconBits_16x12[] = {
		// 8 4 2 1|8 4 2 1|8 4 2 1|8 4 2 1
0x00, 0x00,		// - - - - - - - - - - - - - - - -
0x00, 0x00,		// - - - - - - - - - - - - - - - -
0x00, 0x92, 	// - - - - - - - - X - - - - - X -
0x18, 0x92, 	// - - - X X - - - X - - X - - X -
0x18, 0x92,		// - - - X X - - - X - - X - - X -
0x7e, 0xaa, 	// - X X X X X X - X - X - X - X -
0x7e, 0xaa, 	// - X X X X X X - X - X - X - X -
0x18, 0x44, 	// - - - X X - - - - X - - - X - -
0x18, 0x44, 	// - - - X X - - - - X - - - X - -
0x00, 0x44, 	// - - - - - - - - - X - - - X - -
0x00, 0x00,		// - - - - - - - - - - - - - - - -
0x00, 0x00,		// - - - - - - - - - - - - - - - -
	};
	static const BYTE lpRelativeLayerIconBits_16x12[] = {
		// 8 4 2 1|8 4 2 1|8 4 2 1|8 4 2 1
0x00, 0x00,		// - - - - - - - - - - - - - - - -
0x00, 0x00,		// - - - - - - - - - - - - - - - -
0x00, 0x20, 	// - - - - - - - - - - X - - - - -
0x18, 0x20, 	// - - - X X - - - - - X - - - - -
0x18, 0x20, 	// - - - X X - - - - - X - - - - -
0x7e, 0x20, 	// - X X X X X X - - - X - - - - -
0x7e, 0x20, 	// - X X X X X X - - - X - - - - -
0x18, 0x20, 	// - - - X X - - - - - X - - - - -
0x18, 0x20, 	// - - - X X - - - - - X - - - - -
0x00, 0x3e, 	// - - - - - - - - - - X X X X X -
0x00, 0x00,		// - - - - - - - - - - - - - - - -
0x00, 0x00,		// - - - - - - - - - - - - - - - -
	};

	static const BYTE lpLayerColourIcon_10x10[] = {
		// 8 4 2 1|8 4 2 1|8 4 x x x x x x
0x00, 0x00,		// - - - - - - - - - - - - - - - -
0x7f, 0x80,		// - X X X X X X X X - - - - - - -
0x7f, 0x80,		// - X X X X X X X X - - - - - - -
0x7f, 0x80,		// - X X X X X X X X - - - - - - -
0x7f, 0x80,		// - X X X X X X X X - - - - - - -
0x7f, 0x80,		// - X X X X X X X X - - - - - - -
0x7f, 0x80,		// - X X X X X X X X - - - - - - -
0x7f, 0x80,		// - X X X X X X X X - - - - - - -
0x7f, 0x80,		// - X X X X X X X X - - - - - - -
0x00, 0x00,		// - - - - - - - - - - - - - - - -
	};

	static const BYTE lpLayerColourFrameIcon_10x10[] = {
		// 8 4 2 1|8 4 2 1|8 4 x x x x x x
0xff, 0xc0,		// X X X X X X X X X X - - - - - -
0x80, 0x40,		// X - - - - - - - - X - - - - - -
0x80, 0x40,		// X - - - - - - - - X - - - - - -
0x80, 0x40,		// X - - - - - - - - X - - - - - -
0x80, 0x40,		// X - - - - - - - - X - - - - - -
0x80, 0x40,		// X - - - - - - - - X - - - - - -
0x80, 0x40,		// X - - - - - - - - X - - - - - -
0x80, 0x40,		// X - - - - - - - - X - - - - - -
0x80, 0x40,		// X - - - - - - - - X - - - - - -
0xff, 0xc0,		// X X X X X X X X X X - - - - - -
	};

	pmbmTickIcon = new MaskedBitmap(hdc, 6, 6, lpTickIconBits_6x6);
	pmbmSoloIcon = new MaskedBitmap(hdc, 6, 6, lpSoloIconBits_6x6);
	pmbmLayerColourIcon = new MaskedBitmap(hdc, 10, 10, lpLayerColourIcon_10x10);
	pmbmLayerColourFrameIcon = new MaskedBitmap(hdc, 10, 10, lpLayerColourFrameIcon_10x10);
	//	pmbmLayerColourFrameIcon = new MaskedBitmap(hdc, 10, 10, lpRelativeLayerLeftAlignIconBits_10x10);

	pmbmCATMotionLayerIcon = new MaskedBitmap(hdc, 16, 12, lpCATMotionLayerIconBits_16x12);
	pmbmAbsoluteLayerIcon = new MaskedBitmap(hdc, 16, 12, lpAbsoluteLayerIconBits_16x12);
	pmbmRelativeLayerIcon = new MaskedBitmap(hdc, 16, 12, lpRelativeLayerIconBits_16x12);
	pmbmRelativeWorldLayerIcon = new MaskedBitmap(hdc, 16, 12, lpRelativeWorldLayerIconBits_16x12);

	//
	// Initialise custom Max controls
	//
	edtClipName = GetICustEdit(GetDlgItem(hDlg, IDC_EDIT_CLIPNAME));
	edtClipName->WantReturn(TRUE);
	//	edtClipName->SetNotifyOnKillFocus(FALSE);

	spnGlobalWeight = SetupFloatSpinner(hDlg, IDC_SPIN_CLIPWEIGHT, IDC_EDIT_CLIPWEIGHT, 0.0f, 100.0f, 0.0f, 1.0f);

	spnLocalWeight = SetupFloatSpinner(hDlg, IDC_SPIN_CLIPWEIGHTLOCAL, IDC_EDIT_CLIPWEIGHTLOCAL, 0.0f, 100.0f, 0.0f, 1.0f);

	if (!clipRoot->localBranchWeights) spnLocalWeight->Disable();

	Interval animrange = GetCOREInterface()->GetAnimRange();
	int tpf = GetTicksPerFrame();
	spnTimeWarp = SetupFloatSpinner(hDlg, IDC_SPIN_TIMEWARP, IDC_EDIT_TIMEWARP, (float)animrange.Start() / (float)tpf, (float)animrange.End() / (float)tpf, 0.0f, 1.0f);

	//////////////////////////////////////////////////////////////////
	// Colour mode button states will not remain in this UI, but
	// are piloted here during development.
	/*
	FlyOffData fodCATModes[3] = {
		{ 0,0,0,0 },	// classic
		{ 1,1,1,1 },	// active
		{ 2,2,2,2 },	// blended
	};*/

	flyCATMode = GetICustButton(GetDlgItem(hDlg, IDC_BTN_SETUPMODE));
	//	flyCATMode->SetFlyOff(3, fodCATModes, 0, 0, FLY_DOWN);
	flyCATMode->SetType(CBT_CHECK);
	flyCATMode->SetButtonDownNotify(TRUE);
	flyCATMode->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("flyCATMode"), GetString(IDS_TT_MODETOGGLE)));
	hImageCATModes = ImageList_Create(24, 24, ILC_COLOR24 | ILC_MASK, 3, 3);
	LoadMAXFileIcon(_T("CAT_CATMode"), hImageCATModes, kBackground, FALSE);
	flyCATMode->SetImage(hImageCATModes, 0, 1, 0, 1, 24, 24);

	// Setup the CATWindow button
	ICustButton *btnCATWindow = GetICustButton(GetDlgItem(hDlg, IDC_BTN_CATWINDOW));
	btnCATWindow->SetType(CBT_PUSH);
	btnCATWindow->SetButtonDownNotify(TRUE);
	btnCATWindow->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCATWindow"), GetString(IDS_TT_CMEDITOR)));
	btnCATWindow->SetImage(hIcons, 16, 16, 16 + 25, 16 + 25, 24, 24);
	SAFE_RELEASE_BTN(btnCATWindow);

	// Setup the RemoveClip button
	btnRemoveLayer = GetICustButton(GetDlgItem(hDlg, IDC_BTN_REMOVE));
	btnRemoveLayer->SetType(CBT_PUSH);
	btnRemoveLayer->SetButtonDownNotify(TRUE);
	btnRemoveLayer->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnRemoveLayer"), GetString(IDS_TT_DELLYR)));
	btnRemoveLayer->SetImage(hIcons, 7, 7, 7 + 25, 7 + 25, 24, 24);

	// Setup the MoveUp button
	btnMoveUp = GetICustButton(GetDlgItem(hDlg, IDC_BTN_MOVE_UP));
	btnMoveUp->SetType(CBT_PUSH);
	btnMoveUp->SetButtonDownNotify(TRUE);
	btnMoveUp->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnMoveUp"), GetString(IDS_TT_LYRUP)));
	btnMoveUp->SetImage(hIcons, 14, 14, 14 + 25, 14 + 25, 24, 24);

	// Setup the MoveDown button
	btnMoveDown = GetICustButton(GetDlgItem(hDlg, IDC_BTN_MOVE_DOWN));
	btnMoveDown->SetType(CBT_PUSH);
	btnMoveDown->SetButtonDownNotify(TRUE);
	btnMoveDown->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnMoveDown"), GetString(IDS_TT_LYRDN)));
	btnMoveDown->SetImage(hIcons, 15, 15, 15 + 25, 15 + 25, 24, 24);

	// Setup the ghost button.
	btnTransformGizmo = GetICustButton(GetDlgItem(hDlg, IDC_BTN_GHOST));
	btnTransformGizmo->SetType(CBT_CHECK);
	btnTransformGizmo->SetButtonDownNotify(TRUE);
	btnTransformGizmo->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnTransformGizmo"), GetString(IDS_TT_LYRGIZMO)));
	btnTransformGizmo->SetImage(hIcons, 9, 9, 9 + 25, 9 + 25, 24, 24);

	// Setup the key clip button.
	btnCollapsePoseToLayer = GetICustButton(GetDlgItem(hDlg, IDC_BTN_KEYCLIP));
	btnCollapsePoseToLayer->SetType(CBT_PUSH);
	btnCollapsePoseToLayer->SetButtonDownNotify(TRUE);
	btnCollapsePoseToLayer->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCollapsePoseToLayer"), GetString(IDS_TT_POSETOLYR)));
	btnCollapsePoseToLayer->SetImage(hIcons, 10, 10, 10 + 25, 10 + 25, 24, 24);

	// Setup the key clip button.
	btnCollapseToLayer = GetICustButton(GetDlgItem(hDlg, IDC_BTN_COLLAPSELAYER));
	btnCollapseToLayer->SetType(CBT_PUSH);
	btnCollapseToLayer->SetButtonDownNotify(TRUE);
	btnCollapseToLayer->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCollapseToLayer"), GetString(IDS_TT_LYRCLPS)));
	if (catparenttrans->GetCATMode() == SETUPMODE) btnCollapseToLayer->Enable(FALSE);
	else									 btnCollapseToLayer->Enable(TRUE);
	btnCollapseToLayer->SetImage(hIcons, 12, 12, 12 + 25, 12 + 25, 24, 24);

	// Setup the RangeView button.
	btnShowRangeView = GetICustButton(GetDlgItem(hDlg, IDC_BTN_RANGEVIEW));
	btnShowRangeView->SetType(CBT_PUSH);
	btnShowRangeView->SetButtonDownNotify(TRUE);
	btnShowRangeView->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnShowRangeView"), GetString(IDS_TT_LYRRANGES)));
	btnShowRangeView->SetImage(hIcons, 1, 1, 1 + 25, 1 + 25, 24, 24);

	// Setup the Button Layer Manager button.  // SA: I think this button is always invisible
	btnLayerManagerWindow = GetICustButton(GetDlgItem(hDlg, IDC_BTN_LAYERMANAGER_WINDOW));
	btnLayerManagerWindow->SetType(CBT_PUSH);
	btnLayerManagerWindow->SetButtonDownNotify(TRUE);
	btnLayerManagerWindow->SetImage(hIcons, 2, 2, 2 + 25, 2 + 25, 24, 24);
	if (GET_MAX_RELEASE(Get3DSMAXVersion()) < 9000) {
		btnLayerManagerWindow->Disable();
		btnLayerManagerWindow->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnLayerManagerWindow"), GetString(IDS_TT_SORRY)));
	}
	else {
		btnLayerManagerWindow->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnLayerManagerWindow"), GetString(IDS_TT_FLTLYRMGR)));
	}

	// Setup the TrackView button.
	btnShowGlobalWeightsView = GetICustButton(GetDlgItem(hDlg, IDC_BTN_GLOBAL_WEIGHTSVIEW));
	btnShowGlobalWeightsView->SetType(CBT_PUSH);
	btnShowGlobalWeightsView->SetButtonDownNotify(TRUE);
	btnShowGlobalWeightsView->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnShowGlobalWeightsView"), GetString(IDS_TT_CEGLOBWGTS)));
	btnShowGlobalWeightsView->SetImage(hIcons, 0, 0, 0 + 25, 0 + 25, 24, 24);

	// Setup the TrackView button.
	btnShowLocalWeightsView = GetICustButton(GetDlgItem(hDlg, IDC_BTN_LOCAL_WEIGHTSVIEW));
	btnShowLocalWeightsView->SetType(CBT_PUSH);
	btnShowLocalWeightsView->SetButtonDownNotify(TRUE);
	btnShowLocalWeightsView->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnShowLocalWeightsView"), GetString(IDS_TT_CELOCWGTS)));
	btnShowLocalWeightsView->SetImage(hIcons, 0, 0, 0 + 25, 0 + 25, 24, 24);

	// Setup the TrackView button.
	btnShowTimeWarpsView = GetICustButton(GetDlgItem(hDlg, IDC_BTN_GRAPH_TIMEWARP));
	btnShowTimeWarpsView->SetType(CBT_PUSH);
	btnShowTimeWarpsView->SetButtonDownNotify(TRUE);
	btnShowTimeWarpsView->SetImage(hIcons, 0, 0, 0 + 25, 0 + 25, 24, 24);
	btnShowTimeWarpsView->SetTooltip(TRUE, GetString(IDS_TT_CETIMEWARPS));

	// Setup the layer colour swatch
	swatchLayerColour = GetIColorSwatch(GetDlgItem(hDlg, IDC_LAYER_COLOUR), RGB(0, 0, 0), GetString(IDS_LYR_COLOR));
	// In order to have a clean undo stack, where we
	// don't have other actions in the middle of the
	// color changes, we need to prevent the user from
	// interacting with the rest of max while the dialog is open
	swatchLayerColour->SetModal();
	RefreshLayerColourSwatchState(pClipRoot->GetSelectedLayer());

	// Setup the MoveUp button
	btnCopyLayer = GetICustButton(GetDlgItem(hDlg, IDC_BTN_COPY));
	btnCopyLayer->SetType(CBT_PUSH);
	btnCopyLayer->SetButtonDownNotify(TRUE);
	btnCopyLayer->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCopyLayer"), GetString(IDS_TT_COPYLYR)));
	btnCopyLayer->SetImage(hIcons, 5, 5, 5 + 25, 5 + 25, 24, 24);

	if (pClipRoot->GetSelectedLayer() == -1) btnCopyLayer->Disable();

	// Setup the MoveDown button
	btnPasteLayer = GetICustButton(GetDlgItem(hDlg, IDC_BTN_PASTE));
	btnPasteLayer->SetType(CBT_PUSH);
	btnPasteLayer->SetButtonDownNotify(TRUE);
	btnPasteLayer->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteLayer"), GetString(IDS_TT_PASTELYR)));
	btnPasteLayer->SetImage(hIcons, 3, 3, 3 + 25, 3 + 25, 24, 24);

	if (!pClipRoot->IsLayerCopied()) btnPasteLayer->Disable();

	//
	// Get handles to standard Windows controls
	//
	hListClipLayers = GetDlgItem(hDlg, IDC_LBX_CLIPLAYERS);

	// Get the width of the list box in screen coordinates, and set
	// the tab stops using that information.
	RECT rcListBoxRect;
	GetClientRect(hListClipLayers, &rcListBoxRect);
	POINT pos = { rcListBoxRect.right, rcListBoxRect.bottom };
	ClientToScreen(hListClipLayers, &pos);

	TEXTMETRIC text;
	HDC hdcListBox = GetDC(hListClipLayers);
	GetTextMetrics(hdcListBox, &text);
	ReleaseDC(hListClipLayers, hdcListBox);

	int tabstops[2] = {
		MulDiv(text.tmAveCharWidth * 5, 4, LOWORD(GetDialogBaseUnits())),
		MulDiv(text.tmAveCharWidth * 12, 4, LOWORD(GetDialogBaseUnits()))
	};

	SendMessage(hListClipLayers, LB_SETTABSTOPS, 2, (LPARAM)tabstops);

	hChkIgnore = GetDlgItem(hDlg, IDC_CHK_IGNORE);
	hChkSolo = GetDlgItem(hDlg, IDC_CHK_SOLO);
	//	hChkRelWorld = GetDlgItem(hDlg, IDC_CHK_RELWORLD);
	hChkEnableGlobalWeights = GetDlgItem(hDlg, IDC_CHK_ENABLEGLOBALWEIGHTS);

#ifdef DISPLAY_LAYER_ROLLOUTS_TOGGLE
	SET_CHECKED(hDlg, IDC_CHK_DISPLAY_LAYER_ROLLOUTS, pClipRoot->TestFlag(CLIP_FLAG_SHOW_LAYER_ROLLOUTS));
#endif

	//////////////////////////////////////////////////////////////////
	// Colour mode button states will not remain in this UI, but
	// are piloted here during development.
	FlyOffData fodColourModes[2] = {
		{ 0,0,1,1 },	// classic
//		{ 2,2,3,3 },	// active
		{ 4,4,5,5 },	// blended
//		{ 6,6,7,7 },	// symmetry
//		{ 8,8,9,9 }		// stretchy
	};

	flyColourModes = GetICustButton(GetDlgItem(hDlg, IDC_FLY_COLOURMODES));
	flyColourModes->SetFlyOff(2, fodColourModes, 0, 0, FLY_DOWN);
	flyColourModes->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("flyColourModes"), GetString(IDS_TT_RIGCLRMODE)));
	hImageColourModeFlyoff = ImageList_Create(24, 24, ILC_COLOR24 | ILC_MASK, 3, 3);
	LoadMAXFileIcon(_T("CAT_ColourMode"), hImageColourModeFlyoff, kBackground, FALSE);
	flyColourModes->SetImage(hImageColourModeFlyoff, 0, 0, 0, 0, 24, 24);

	//////////////////////////////////////////////////////////////////
	// Colour mode button states will not remain in this UI, but
	// are piloted here during development.
	FlyOffData fodAddLayer[4] = {
		{ 0,0,1,1 },	// absolute
		{ 2,2,3,3 },	// relative L
		{ 4,4,5,5 },	// relative W
		{ 6,6,7,7 },	// CATMotion
//		{ 8,8,9,9 }		// stretchy
	};

	flyAddLayer = GetICustButton(GetDlgItem(hDlg, IDC_FLY_ADDLAYER));
	flyAddLayer->SetFlyOff(4, fodAddLayer, 0, 0, FLY_DOWN);
	flyAddLayer->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("flyAddLayer"), GetString(IDS_TT_ADDLYR)));
	hImageAddLayerFlyoff = ImageList_Create(35, 20, ILC_COLOR24 | ILC_MASK, 3, 3);
	LoadMAXFileIcon(_T("CAT_AddLayer"), hImageAddLayerFlyoff, kBackground, FALSE);
	flyAddLayer->SetImage(hImageAddLayerFlyoff, 0, 0, 0, 0, 35, 20);
	//////////////////////////////////////////////////////////////////

	// Add the layers to the List box and select
	// the currently active clip layer.
	RefreshClipLayersList();
	RefreshCATRigModeButtonState();
	SelectLayer(pClipRoot->GetSelectedLayer(), FALSE, FALSE);

	ReleaseDC(hWnd, hdc);

	return TRUE;
}

// Releases all Max custom controls and NULLS out any members whose
// non-NULL value indicates that the dialog is still in use...
void ClipRolloutData::ReleaseControls(HWND hWnd)
{
	UNUSED_PARAM(hWnd);
	DbgAssert(hDlg == hWnd);

	theHold.Suspend();
	DeleteAllRefsFromMe();
	theHold.Resume();

	GetCOREInterface()->UnRegisterTimeChangeCallback(this);

	SAFE_DELETE(pmbmTickIcon);
	SAFE_DELETE(pmbmSoloIcon);
	SAFE_DELETE(pmbmLayerColourIcon);
	SAFE_DELETE(pmbmLayerColourFrameIcon);

	//	for(int i=0;i<pClipRoot->NumLayers(); i++){
	//		pClipRoot->GetLayer(i)->DestroyLayerIcon();
	//	}
	SAFE_DELETE(pmbmCATMotionLayerIcon);
	SAFE_DELETE(pmbmAbsoluteLayerIcon);
	SAFE_DELETE(pmbmRelativeLayerIcon);
	SAFE_DELETE(pmbmRelativeWorldLayerIcon);

	SAFE_RELEASE_BTN(flyCATMode);

	//
	// Clip stuff
	//
	SAFE_RELEASE_BTN(btnRemoveLayer);
	SAFE_RELEASE_BTN(btnMoveUp);
	SAFE_RELEASE_BTN(btnMoveDown);
	SAFE_RELEASE_BTN(btnCopyLayer);
	SAFE_RELEASE_BTN(btnPasteLayer);
	SAFE_RELEASE_BTN(btnCollapseToLayer);

	SAFE_RELEASE_EDIT(edtClipName);
	SAFE_RELEASE_SPIN(spnGlobalWeight);
	SAFE_RELEASE_SPIN(spnLocalWeight);
	SAFE_RELEASE_SPIN(spnTimeWarp);
	SAFE_RELEASE_BTN(btnShowGlobalWeightsView);
	SAFE_RELEASE_BTN(btnShowLocalWeightsView);
	SAFE_RELEASE_BTN(btnShowTimeWarpsView);

	SAFE_RELEASE_BTN(btnCollapsePoseToLayer);
	SAFE_RELEASE_BTN(btnTransformGizmo);

	SAFE_RELEASE_BTN(btnCATWiki);
	SAFE_RELEASE_BTN(btnShowRangeView);
	SAFE_RELEASE_BTN(btnLayerManagerWindow);

	SAFE_RELEASE_COLORSWATCH(swatchLayerColour);

	SAFE_RELEASE_BTN(flyColourModes);
	SAFE_RELEASE_BTN(flyAddLayer);

	ImageList_Destroy(hImageCATModes);
	hImageCATModes = NULL;

	//	ImageList_Destroy(hLayerMethodsList);
	ImageList_Destroy(hImageAddLayerFlyoff);
	hImageAddLayerFlyoff = NULL;
	ImageList_Destroy(hImageColourModeFlyoff);
	hImageColourModeFlyoff = NULL;

	// These members indicate the dialog is in use, if they
	// are non-NULL.
	hDlg = NULL;
	pClipRoot = NULL;
	catparenttrans = NULL;
}

// Reloads the clip layers list box with strings, between
// the layers nFrom and nTo (inclusive).  If nTo is -1, this
// means all values from nFrom to the end of the list.  The
// current list box selection and top item index are preserved.
//
void ClipRolloutData::RefreshClipLayersList(int nFrom/*=0*/, int nTo/*=-1*/)
{
	UNREFERENCED_PARAMETER(nFrom); UNREFERENCED_PARAMETER(nTo);
	int nCount = (int)SendMessage(hListClipLayers, LB_GETCOUNT, 0, 0);
	int nTarget = pClipRoot->NumLayers() + 1;

	if (nCount < nTarget) {
		for (int i = nCount; i < nTarget; i++)
			SendMessage(hListClipLayers, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)_T(""));
	}
	else if (nTarget < nCount) {
		for (int i = nCount - 1; i >= nTarget; i--)
			SendMessage(hListClipLayers, LB_DELETESTRING, i, 0);
	}
	else {
		InvalidateRect(hListClipLayers, NULL, FALSE);
	}
	return;
}

// This function handles everything to do with selecting a new
// layer in the clip layers list box.  It should be called by
// all parts of the UI when switching a layer, including when
// a message is sent from the list box to say the user has
// selected a new layer.
//
void ClipRolloutData::SelectLayer(int nLayer, int bAlreadySelected/*=FALSE*/, BOOL bUpdateColours/*=TRUE*/) {
	UNREFERENCED_PARAMETER(bUpdateColours);

	// we reselect layer -1 every
	// time the UI is displayed.
	if ((nLayer == pClipRoot->NumLayers()) || (nLayer < 0)) {
		nLayer = -1;
	}
	if (pClipRoot->GetSelectedLayer() != nLayer)
		pClipRoot->SelectLayer(nLayer);

	ReplaceReference(GLOBAL, pClipRoot->GetLayer(nLayer));
	ReplaceReference(LOCAL, pClipRoot->localBranchWeights);

	// Select in the list box if requested.
	RefreshClipLayersList();
	if (!bAlreadySelected)
		SendMessage(hListClipLayers, LB_SETCURSEL, (nLayer == -1 ? pClipRoot->NumLayers() : nLayer), 0);

	RefreshCurrLayer(nLayer);
}

void ClipRolloutData::RefreshCurrLayer(int nLayer) {

	// Update control state for the new selected layer.
	RefreshAddLayerButtonState(nLayer);
	RefreshRemoveButtonState(nLayer);
	RefreshMoveUpDownButtonState(nLayer);
	RefreshGhostButtonState(nLayer);
	RefreshKeyClipButtonState(nLayer);
	RefreshLayerColourSwatchState(nLayer);
	RefreshLayerNameEdit(nLayer);
	RefreshSpinners(nLayer);
	RefreshIgnoreSoloStates(nLayer);
	RefreshColourModeButtonState();
	RefreshCATRigModeButtonState();

	if (nLayer == -1) btnCopyLayer->Disable();
	else {
		/*	if(pClipRoot->GetLayer(nLayer)->GetMethod()==LAYER_CATMOTION)
				 btnCopyLayer->Disable();
			else*/ btnCopyLayer->Enable();
	}

	if (!pClipRoot->IsLayerCopied())
		btnPasteLayer->Disable();
	else btnPasteLayer->Enable();

	// If we do not have a layer selected, or if the layer already has a timewarp, we just
	// display the existing layer.  If the selected layer doesn't have a timewarp,
	// then clicking this button will create it, so let people know this fact!
	NLAInfo* pInfo = pClipRoot->GetSelectedLayerInfo();
	if (pInfo == NULL || pInfo->GetTimeController() != NULL)
		btnShowTimeWarpsView->SetTooltip(TRUE, GetString(IDS_TT_CETIMEWARPS));
	else
		btnShowTimeWarpsView->SetTooltip(TRUE, GetString(IDS_TT_CREATETIMEWARPS));
}

// A layer can be added provided it is not added in front of the
// the first layer (which holds CAT Motion).
//
void ClipRolloutData::RefreshAddLayerButtonState(int nLayer) {
	UNREFERENCED_PARAMETER(nLayer);
	btnCollapseToLayer->Enable(catparenttrans->GetCATMode() != SETUPMODE);
}

// A layer can be removed provided it is not the 'available' dummy
// or the first layer (which holds CAT Motion).
//
void ClipRolloutData::RefreshRemoveButtonState(int nLayer) {
	btnRemoveLayer->Enable(nLayer >= 0);
}

// A layer can be moved up provided there is a layer above it
// which is not the first layer (which holds CAT Motion).
// A layer can be moved down provided it is not the first layer
// (which holds CAT Motion), and there is a layer below it.
//
void ClipRolloutData::RefreshMoveUpDownButtonState(int nLayer) {
	btnMoveUp->Enable(nLayer > 0);
	btnMoveDown->Enable(nLayer >= 0 && nLayer + 1 < pClipRoot->NumLayers());
}

// Refreshes the status of the ghost button.
void ClipRolloutData::RefreshGhostButtonState(int nLayer) {
	if (nLayer < 0 || pClipRoot->GetLayer(nLayer)->GetMethod() == LAYER_RELATIVE) {
		ShowWindow(GetDlgItem(hDlg, IDC_BTN_GHOST), SW_SHOW);
		ShowWindow(GetDlgItem(hDlg, IDC_BTN_CATWINDOW), SW_HIDE);
		btnTransformGizmo->Enable(FALSE);
		btnTransformGizmo->SetCheck(FALSE);
	}
	else if (pClipRoot->GetLayer(nLayer)->GetMethod() == LAYER_CATMOTION) {
		ShowWindow(GetDlgItem(hDlg, IDC_BTN_GHOST), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_BTN_CATWINDOW), SW_SHOW);
	}
	else {// Absolute and Capture Layers
		ShowWindow(GetDlgItem(hDlg, IDC_BTN_CATWINDOW), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_BTN_GHOST), SW_SHOW);
		btnTransformGizmo->Enable(pClipRoot->GetLayer(nLayer)->CanTransform());
		btnTransformGizmo->SetCheck(pClipRoot->GetLayer(nLayer)->TransformNodeOn());
	}
}

// Refreshes the status of the key clip button.
void ClipRolloutData::RefreshKeyClipButtonState(int nLayer) {
	if (nLayer < 0 || (pClipRoot->GetLayer(nLayer)->GetMethod() == LAYER_CATMOTION))
		btnCollapsePoseToLayer->Enable(FALSE);
	else btnCollapsePoseToLayer->Enable(TRUE);
}

void ClipRolloutData::RefreshLayerColourSwatchState(int nLayer) {
	// Disable swatch if no ApplyAbsolute layer is selected.
	swatchLayerColour->SetColor((nLayer >= 0 && pClipRoot->GetLayer(nLayer)->ApplyAbsolute()) ? pClipRoot->GetLayer(nLayer)->GetColour().toRGB() : ColorMan()->GetColor(kBackground));
	swatchLayerColour->Activate(nLayer >= 0 && pClipRoot->GetLayer(nLayer)->ApplyAbsolute());
	swatchLayerColour->Enable(nLayer >= 0 && pClipRoot->GetLayer(nLayer)->ApplyAbsolute());
}

void ClipRolloutData::RefreshIgnoreSoloStates(int nLayer) {
	if (nLayer == -1) {
		EnableWindow(hChkIgnore, FALSE);
		EnableWindow(hChkSolo, FALSE);
		//		EnableWindow(hChkRelWorld, FALSE);
	}
	else {
		// we cant ignore the CATMotion layer
		EnableWindow(hChkIgnore, nLayer > 0);
		EnableWindow(hChkSolo, pClipRoot->GetLayer(nLayer)->ApplyAbsolute());

		int nSolo = pClipRoot->GetSoloLayer();
		bool bHasSoloEnableCheck = (nLayer == nSolo || (nSolo >= 0 && pClipRoot->GetLayer(nLayer)->GetParentLayer() == nSolo));
		SendMessage(hChkIgnore, BM_SETCHECK, pClipRoot->GetLayer(nLayer)->LayerEnabled() ? BST_UNCHECKED : BST_CHECKED, 0);
		SendMessage(hChkSolo, BM_SETCHECK, bHasSoloEnableCheck ? BST_CHECKED : BST_UNCHECKED, 0);

		//		EnableWindow(hChkRelWorld,  (pClipRoot->GetLayer(nLayer)->GetMethod() == LAYER_RELATIVE || pClipRoot->GetLayerMethod(nLayer) == LAYER_RELATIVE_WORLD) && nLayer > 0);
		//		SendMessage(hChkRelWorld, BM_SETCHECK, pClipRoot->GetLayer(nLayer)->GetMethod() == LAYER_RELATIVE_WORLD ? BST_CHECKED : BST_UNCHECKED, 0);
	}
}

// Loads the layer name edit box with the current layer name.
void ClipRolloutData::RefreshLayerNameEdit(int nLayer) {
	if (nLayer == -1) {
		edtClipName->SetText(_T(""));
		edtClipName->Enable(FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_LBL_LAYERNAME), FALSE);
	} /*else if (nLayer == 0) {
		edtClipName->SetText(_T("CATMotion"));
		edtClipName->Enable(FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_LBL_LAYERNAME), FALSE);
	}*/ else {
		edtClipName->Enable(TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_LBL_LAYERNAME), TRUE);
		edtClipName->SetText(pClipRoot->GetLayer(nLayer)->GetName().data());
	}
}

// Loads the weights edit/spinner with the current layer weight.
void ClipRolloutData::RefreshSpinners(int nLayer) {
	// If no layer is currently selected, disable all spinners
	if ((nLayer == -1) || pClipRoot->NumLayers() == 0) {
		spnGlobalWeight->Enable(FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_LBL_GLOBALWEIGHT), FALSE);
		spnGlobalWeight->SetValue(100.0f, FALSE);
		spnGlobalWeight->SetKeyBrackets(FALSE);

		// PT 18/02/04
		spnLocalWeight->Enable(FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_LBL_LOCALWEIGHT), FALSE);
		spnLocalWeight->SetValue(100.0f, FALSE);
		spnLocalWeight->SetKeyBrackets(FALSE);

		spnTimeWarp->Enable(FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_LBL_TIMEWARP), FALSE);
		spnTimeWarp->SetValue(0.0f, FALSE);
		spnTimeWarp->SetKeyBrackets(FALSE);

		EnableWindow(hChkEnableGlobalWeights, FALSE);
		//	SendMessage(hChkEnableGlobalWeights, BM_SETCHECK, pClipRoot->WeightHierarchyEnabled(nLayer) ? BST_UNCHECKED : BST_CHECKED, 0);
	}
	else {
		EnableWindow(hChkEnableGlobalWeights, TRUE);

		NLAInfo* pCurrInfo = pClipRoot->GetLayer(nLayer);
		if (pCurrInfo == NULL)
			return;

		BOOL enableWeight = (nLayer > 0) ? TRUE : FALSE;
		if (pClipRoot->WeightHierarchyEnabled(nLayer)) {
			spnGlobalWeight->Enable(enableWeight);
			EnableWindow(GetDlgItem(hDlg, IDC_LBL_GLOBALWEIGHT), enableWeight);
			Interval iv = FOREVER;
			spnGlobalWeight->SetValue(100.0f * pCurrInfo->GetWeight(tvTime, iv), FALSE);
			spnGlobalWeight->SetKeyBrackets(pCurrInfo->GetWeightsController()->IsKeyAtTime(tvTime, 0));

			SendMessage(hChkEnableGlobalWeights, BM_SETCHECK, BST_CHECKED, 0);
		}
		else {
			spnGlobalWeight->Enable(FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_LBL_GLOBALWEIGHT), FALSE);
			spnGlobalWeight->SetValue(100.0f, FALSE);
			spnGlobalWeight->SetKeyBrackets(FALSE);

			SendMessage(hChkEnableGlobalWeights, BM_SETCHECK, BST_UNCHECKED, 0);
		}

		if (pClipRoot->localBranchWeights) {
			spnLocalWeight->Enable(enableWeight);
			EnableWindow(GetDlgItem(hDlg, IDC_LBL_LOCALWEIGHT), enableWeight);
			spnLocalWeight->SetValue(100.0f * pClipRoot->GetLayerWeightLocal(nLayer, tvTime), FALSE);
			spnLocalWeight->SetKeyBrackets(pClipRoot->localBranchWeights->GetLayer(nLayer)->IsKeyAtTime(tvTime, 0));
		}
		else {
			spnLocalWeight->Enable(FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_LBL_LOCALWEIGHT), FALSE);
		}

		spnTimeWarp->Enable(TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_LBL_TIMEWARP), TRUE);
		spnTimeWarp->SetValue((float)pCurrInfo->GetTime(tvTime) / ((float)GetTicksPerFrame()), FALSE);
		Control *pTimeWarp = pCurrInfo->GetTimeController();
		if (pTimeWarp != NULL)
			spnTimeWarp->SetKeyBrackets(pTimeWarp->IsKeyAtTime(tvTime, 0));
	}
}

void ClipRolloutData::RefreshCATRigModeButtonState() {
	if (pClipRoot->NumLayers() == 0) {
		flyCATMode->Disable();
		btnShowRangeView->Disable();;
		btnShowGlobalWeightsView->Disable();
		btnShowLocalWeightsView->Disable();
		btnShowTimeWarpsView->Disable();
	}
	else {
		flyCATMode->Enable();
		btnShowRangeView->Enable();
		btnShowGlobalWeightsView->Enable();
		btnShowLocalWeightsView->Enable();
		btnShowTimeWarpsView->Enable();
	}
	switch (catparenttrans->GetCATMode()) {
	case SETUPMODE:				flyCATMode->SetCheck(FALSE);	break;
	case NORMAL:				flyCATMode->SetCheck(TRUE);		break;
	}
}

//
// Refreshes the selection state of the colour mode flyoff.  Note,
// this is pretty much the only place in all of CAT that uses the
// function CATParent::GetColourMode().  Everyone else should use
// CATParent::GetEffectiveColourMode(), unless they really need
// to know what the internal colourmode setting is.
//
void ClipRolloutData::RefreshColourModeButtonState() {
	flyColourModes->Enable(catparenttrans->GetCATMode() != SETUPMODE);

	switch (catparenttrans->GetColourMode()) {
	case COLOURMODE_CLASSIC:	flyColourModes->SetCurFlyOff(0);		break;
	case COLOURMODE_BLEND:		flyColourModes->SetCurFlyOff(1);		break;
	}
}

void CATClipRoot::RefreshLayerRollout() {
	if (staticClipRollout.GetHwnd() != NULL) {
		staticClipRollout.RefreshClipLayersList();
		staticClipRollout.RefreshColourModeButtonState();
		staticClipRollout.RefreshCATRigModeButtonState();
		staticClipRollout.SelectLayer(nSelected);
		InvalidateRect(staticClipRollout.GetHwnd(), NULL, TRUE);
	}
}

void CATClipRoot::UpdateUI(BOOL refresh_trackviews/*=FALSE*/, BOOL due_to_undo/*=FALSE*/) {
	RefreshLayerRollout();

	if (due_to_undo) {
		RefreshLayerManagerWindow(catparenttrans->GetNode());
	}

	if (refresh_trackviews) {
		if ((idRangesView >= 0) && IsTrackViewOpen(idRangesView))			ShowRangesView();
		if ((idGlobalWeightsView >= 0) && IsTrackViewOpen(idGlobalWeightsView))	ShowGlobalWeightsView();
		if ((idLocalWeightsView >= 0) && IsTrackViewOpen(idLocalWeightsView))	ShowLocalWeightsView(localBranchWeights);
		if ((idTimeWarpView >= 0) && IsTrackViewOpen(idTimeWarpView))		ShowTimeWarpView();
	}
}

//
// Returns the requested interface class.
//
BaseInterface* CATClipRoot::GetInterface(Interface_ID id) {
	if (id == LAYERROOT_INTERFACE_FP) return (ILayerRootFP*)this;// The Script Interface
	return Control::GetInterface(id);
}

//
// Initialise all our stuff
//
void CATClipRoot::Init()
{
	flags = 0;

	nNumLayers = 0;
	nSelected = -1;
	nSolo = -1;
	dwFileVersion = 0;

	catparenttrans = NULL;
	localLayerControl = NULL;

	// this is just an idea so far
	// but this pointer is used so the UI can
	// display the weights of a branch of the clip hierarchy.
	// Global * Local = Effective
	localBranchWeights = NULL;

	idRangesView = idGlobalWeightsView = idLocalWeightsView = idTimeWarpView = -1;

	upgraded = FALSE;
}

class RootLayerChangeRestore : public RestoreObj {
public:
	CATClipRoot		*root;
	int				indexundo, indexredo;
	int				numlayers;
	int				msg;
	BOOL refreshUIundo; BOOL refreshUIredo;

	RootLayerChangeRestore(CATClipRoot *r, int message, int index, BOOL refreshUIundo, BOOL refreshUIredo) : indexredo(0) {
		root = r;
		indexundo = index;
		msg = message;
		numlayers = root->NumLayers();
		this->refreshUIundo = refreshUIundo;
		this->refreshUIredo = refreshUIredo;

		switch (msg) {
		case CLIP_LAYER_SELECT:			indexundo = root->GetSelectedLayer();			break;
		case CLIP_LAYER_SOLOED:			indexundo = root->GetSoloLayer();		break;
		}
	}

	void Restore(int /*isUndo*/) {

		// the reference system will automaticly replace the references
		// we just need to make sure the layer ids are all good
		switch (msg) {
		case CLIP_LAYER_INSERT:
			root->RemoveController(indexundo);
			break;
		case CLIP_LAYER_REMOVE:
			root->InsertController(indexundo, LAYER_IGNORE);
			break;
		case CLIP_LAYER_MOVEUP:
			root->MoveControllerUp(indexundo);
			break;
		case CLIP_LAYER_MOVEDOWN:
			root->MoveControllerDown(indexundo);
			break;
		case CLIP_LAYER_SELECT:
			indexredo = root->GetSelectedLayer();
			root->SelectLayer(indexundo);
			break;
		case CLIP_LAYER_SOLOED:
			indexredo = root->GetSoloLayer();
			root->nSolo = indexundo;
			break;
		}
		root->UpdateUI(TRUE, TRUE);
	}

	void Redo() {
		// the reference system will automaticly replace the references
		// we just need to make sure the layer ids are all good
		switch (msg) {
		case CLIP_LAYER_INSERT:
			root->InsertController(indexundo, LAYER_IGNORE);
			break;
		case CLIP_LAYER_REMOVE:
			root->RemoveController(indexundo);
			break;
		case CLIP_LAYER_MOVEUP:
			root->MoveControllerUp(indexundo);
			break;
		case CLIP_LAYER_MOVEDOWN:
			root->MoveControllerDown(indexundo);
			break;
		case CLIP_LAYER_SELECT:
			root->SelectLayer(indexredo);
			break;
		case CLIP_LAYER_SOLOED:
			root->nSolo = indexredo;
			break;
		}
		root->UpdateUI(TRUE, TRUE);
	}

	int Size() { return 3; }
	void EndHold() { root->ClearAFlag(A_HELD); }
};

// Resizes our layers list.  Any new items get initialised with
// NewWhateverIAmController(), which returns a new instance of
// whatever sort of controller this value branch is supposed to
// hold.  If 'loading' is TRUE, the items are just initialised
// to NULL.  It's the loader's responsibility to set the
// references.
void CATClipRoot::ResizeList(int n, BOOL /*loading*/ /*=FALSE*/)
{
	int nOldLayers = nNumLayers;
	nNumLayers = n;
	tabLayers.SetCount(n);

	// Initialize all layers to NULL
	for (int i = nOldLayers; i < n; i++)
		tabLayers[i] = NULL;

	// Now assign new references.
	/*if (!loading)
	{
		for(int i = nOldLayers; i<n; i++)
			ReplaceReference(i, (ReferenceTarget*)CreateInstance(GetNLAInfoDesc()->SuperClassID(), GetNLAInfoDesc()->ClassID()));
	}*/
}

// Here, data is the id of the slot we're inserting
// into.  We first resize the list, which puts a new
// entry at the end.  Then we save a pointer to the
// new entry and set A_LOCK_TARGET to stop it from
// being auto-deleted.  This allows us to shuffle
// everything along and then stick it in the new gap
// we've created.  Yaay.
void CATClipRoot::InsertController(int n, ClipLayerMethod method)
{
	if (n < 0)
		return;
	if (n > nNumLayers)
		n = nNumLayers;

	// The undo needs to happen after the ReplaceReference, so we register it first...
	if (theHold.Holding())
		theHold.Put(new RootLayerChangeRestore(this, CLIP_LAYER_INSERT, n, FALSE, FALSE));

	ResizeList(nNumLayers + 1, TRUE);

	for (int i = nNumLayers - 1; i > n; i--) {
		// Shuffle controllers up 1 index.
		tabLayers[i] = tabLayers[i - 1];
		// update the index param
		if (tabLayers[i]) tabLayers[i]->SetIndex(i);
	}

	tabLayers[n] = NULL;

	// This function is used from within the undo system, so we may not actually
	// want to put a new controller in here as the undo system will stick the one in it has in the undo stack
	if (!theHold.RestoreOrRedoing()) {
		ClassDesc2* pNewInfoDesc = NULL;
		switch (method)
		{
		case LAYER_ABSOLUTE:
		case LAYER_RELATIVE:
		case LAYER_RELATIVE_WORLD:
			pNewInfoDesc = GetNLAInfoDesc();
			break;
		case LAYER_CATMOTION:
			pNewInfoDesc = GetCATMotionLayerDesc();
			break;
		default:
			DbgAssert("ERROR: Unknown layer type");
			pNewInfoDesc = GetNLAInfoDesc();
		}
		NLAInfo* pNewInfo = (NLAInfo*)CreateInstance(pNewInfoDesc->SuperClassID(), pNewInfoDesc->ClassID());
		pNewInfo->SetMethod(method);
		ReplaceReference(n, pNewInfo);
	}

	RefreshParentLayerCache();
}

// To remove, we replace each reference in the list
// with its successor.  At the end of the list, we
// replace the reference with NULL.  Now we have
// one reference for each item, except for the one
// we deleted, which should be gone =)
void CATClipRoot::RemoveController(int n)
{
	DbgAssert(n >= 0 && n < nNumLayers);
	if (n < 0 || n >= nNumLayers)
		return;

	if (!theHold.RestoreOrRedoing())
		DeleteReference(n);
	else
		DbgAssert(tabLayers[n] == NULL);

	for (int i = n; i < nNumLayers - 1; i++) {
		tabLayers[i] = tabLayers[i + 1];
		// undate the index param
		if (tabLayers[i]) tabLayers[i]->SetIndex(i);
	}

	// The following line should handle changing
	// the value of nNumLayers as well.
	ResizeList(nNumLayers - 1);

	// the restore object needs to be last on the FILO undo queue
	if (theHold.Holding())
		theHold.Put(new RootLayerChangeRestore(this, CLIP_LAYER_REMOVE, n, FALSE, FALSE));

	RefreshParentLayerCache();
}

void CATClipRoot::MoveControllerUp(int n)
{
	if (n <= 0 || n >= nNumLayers)
		return;

	NLAInfo *ctrlTarget = tabLayers[n - 1];
	tabLayers[n - 1] = tabLayers[n];
	tabLayers[n] = ctrlTarget;

	// update the index params
	tabLayers[n - 1]->SetIndex(n - 1);
	tabLayers[n]->SetIndex(n);

	if (theHold.Holding())
		theHold.Put(new RootLayerChangeRestore(this, CLIP_LAYER_MOVEUP, n, FALSE, FALSE));

	RefreshParentLayerCache();
}

void CATClipRoot::MoveControllerDown(int n)
{
	MoveControllerUp(n + 1);
}

//
// This function gets called when we need to notify value
// branches of a change.  We have a bit of control over how
// the message propagates through the tree, by setting
// 'flags'.
//
void CATClipRoot::CATMessages(TimeValue t, UINT msg, int data, DWORD flags)
{
	switch (msg) {
	case CLIP_LAYER_APPEND:		InsertController(nNumLayers, (ClipLayerMethod)flags);	break;
	case CLIP_LAYER_INSERT:		InsertController(data, (ClipLayerMethod)flags);	break;
	case CLIP_LAYER_MOVEUP:		MoveControllerUp(data);									break;
	case CLIP_LAYER_MOVEDOWN:	MoveControllerDown(data);								break;
		// We only send these messages so that we can
		// use one undo object for the whole CATClipRoot
//		case CLIP_LAYER_SOLOED:		nSolo = id;							return;
	}

	// tell the rest of the character
	ICATParentTrans* pCPTrans = GetCATParentTrans();
	if (pCPTrans != NULL)
		pCPTrans->CATMessage(t, msg, data);

	// These message needs to be processed last to allow
	// clips to remove their UI from the old selected layer
	switch (msg) {
	case CLIP_LAYER_REMOVE:		RemoveController(data);	break;
	case CLIP_LAYER_SELECT:		nSelected = data;
	}

	// Don't forget to update the UI if necessary
	UpdateUI(TRUE);
}

// Searches for the layer specified by 'name' and returns its
// index number.  If the name is not found, returns -1.
int CATClipRoot::GetLayerIndex(const TCHAR* name) const
{
	//	for (int i=0; i<nNumLayers; i++)
	///		if (!_tcsicmp(name, vecLayers[i].strName.data()))
	//			return i;Get
	for (int i = 0; i < nNumLayers; i++)
		if (!_tcsicmp(name, tabLayers[i]->GetName().data()))
			return i;
	return -1;
}

// Get the evaluation method of the specified layer.
ClipLayerMethod CATClipRoot::GetLayerMethod(ULONG id) const
{
	const NLAInfo* pInfo = GetLayer(id);
	if (pInfo != NULL)
		return pInfo->GetMethod();

	return LAYER_IGNORE;
}

// Adds a layer onto the end of the list and returns its index.
//
int CATClipRoot::AppendLayer(const TSTR& name, ClipLayerMethod method)
{
	// Now in CAT2, instead of havin an special method to append layers,
	// we simply 'Insert' a the end o the stack.
	int nLayers = nNumLayers;
	if (InsertLayer(name, nLayers, method))
		return nLayers;

	return -1;
}

// Inserts a layer at index specified by 'at'.  Returns TRUE if successful.
//
BOOL CATClipRoot::InsertLayer(const TSTR& name, int at, ClipLayerMethod method)
{
	if (at == -1) at = nNumLayers;
	if (at < 0 || at > nNumLayers)
		return FALSE;

	// Do not allow relative layers to be created at index 0
	if (at == 0 && (method == LAYER_RELATIVE || method == LAYER_RELATIVE_WORLD))
		return FALSE;

	// Suspend animate, set key mode and disable macro recorder
	AnimateSuspend suspend(TRUE, TRUE, FALSE);

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; }

	// The main purpose of this line is to make sure all the
	// UI's for all the CATclipValues are safely removed
	SelectLayer(-1);

	{
#ifdef _DEBUG // Temporarily disable consistency checking in CATClipValue::NumLayers...
		bIsModifyingLayers = TRUE;
#endif
		// Insert a new layer.
		CATMessages(0, CLIP_LAYER_INSERT, at, method);
#ifdef _DEBUG
		bIsModifyingLayers = FALSE;
#endif
	}

	NLAInfo* pNewInfo = GetLayer(at);
	pNewInfo->Initialise(this, name, method, at);

	//////////////////////////////////////////////////
	// Allow the Layer to configure itself now all the
	// controllers have been created across the character
	SelectLayer(at);
	pNewInfo->PostLayerCreateCallback();

	if (theHold.Holding() && newundo) {

		theHold.Accept(GetString(IDS_HLD_ADDLYR));

		if (GetCATParentTrans()->GetEffectiveColourMode() != COLOURMODE_CLASSIC)
			GetCATParentTrans()->UpdateColours(FALSE, TRUE);
	}

	return TRUE;
}

// Removes layer at index specified by 'at'.
//
void CATClipRoot::RemoveLayer(int at)
{
	if (at < 0 || at >= nNumLayers)
		return;

	// Macro-recorder support
	MACRO_DISABLE;
	HoldActions hold(IDS_HLD_REMLYR);

	Interface* pCore = GetCOREInterface();
	pCore->DisableSceneRedraw();

	// This really is a slow operation, disable evaluation
	// until we've finished
	{
		CATEvaluationLock lock;

		// If the 'copied' layer is being deleted, clear the paste layer buffer.
		if (IsLayerCopied() && g_PasteLayerData.layerinfo == tabLayers[at]) {
			g_PasteLayerData.cliproot = NULL;
			g_PasteLayerData.layerinfo = NULL;
		}

		// Move the current layer selection (GB 31-Jul-03).  This
		// will work even if the last layer is removed, in which
		// case nSelected-1 must be -1, therefore selecting no layer.
		// GB 04-Aug-03: This should be done BEFORE deleting the layer!
		if (nSelected >= at) SelectLayer(at - 1);
		if (GetSoloLayer() == at) SoloLayer(-1);

		tabLayers[at]->PreLayerRemoveCallback();

		{
#ifdef _DEBUG // Temporarily disable consistency checking in CATClipValue::NumLayers...
			bIsModifyingLayers = TRUE;
#endif
			// Tell the rest of the hierarcy to delete the layer.
			CATMessages(0, CLIP_LAYER_REMOVE, at);
#ifdef _DEBUG
			bIsModifyingLayers = FALSE;
#endif
		}

		// if we have just removed the last layer then put the character into setupmode
		if (nNumLayers == 0) {
			SelectNone();
			catparenttrans->SetColourMode(COLOURMODE_CLASSIC);
			catparenttrans->SetCATMode(SETUPMODE);
		}
	} // EvaluationLock

	catparenttrans->UpdateColours(FALSE, TRUE);

	pCore->EnableSceneRedraw();
	pCore->RedrawViews(pCore->GetTime());

	// Macro-recorder support
	MACRO_ENABLE;
}

// Clones the specified layer, gives it the specified name,
// and adds it to the end of the layers list.
//
BOOL CATClipRoot::CloneLayer(int nLayer, const TSTR& newName)
{
	UNREFERENCED_PARAMETER(nLayer); UNREFERENCED_PARAMETER(newName);
	DbgAssert(false);
	return TRUE;
}

// Swaps a layer with the layer above it.
//
void CATClipRoot::MoveLayerUp(int nLayer)
{
	if (nLayer <= 0 || nLayer >= nNumLayers) return;

	// Do not allow users to set a non-absolute layer as the first layer.
	if (nLayer == 1 && !GetLayer(nLayer)->ApplyAbsolute())
		return;

	// Macro-recorder support
	MACRO_DISABLE;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	SelectLayer(-1);

	// Tell the rest of the hierarchy to move the layer up.
	CATMessages(0, CLIP_LAYER_MOVEUP, nLayer);

	SelectLayer(nLayer - 1);
	if (GetSoloLayer() == nLayer) SoloLayer(nLayer - 1);

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_LYRUP));
	}

	// This shouldn't be necessary, but for some reason views are not updating
	GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());

	// Macro-recorder support
	MACRO_ENABLE;
}

// Swaps a layer with the layer beneath it.
//
void CATClipRoot::MoveLayerDown(int nLayer)
{
	// Hold actions here to ensure we get the right
	// string in the Undo list.
	HoldActions(IDS_HLD_LYRDOWN);
	MoveLayerUp(nLayer + 1);
	SelectLayer(nLayer + 1);
}

// Renames a layer.
//
BOOL CATClipRoot::RenameLayer(int nLayer, const TSTR& newName)
{
	if (nLayer < 0 || nLayer >= nNumLayers || !tabLayers[nLayer]) return FALSE;
	tabLayers[nLayer]->SetName(newName);
	return TRUE;
}

//
// A parent layer is the first absolute layer above a relative layer.
// An absolute layer effectively controls all relative layers below it
// until the next absolute layer.  The parent layer is cached in the
// vecLayers array.  By definition the parent layer of an absolute
// layer is -1 (or a relative layer with no parent, as is the case with
// the CATMotion layer).  The following function MUST be called whenever
// the structure or order of the layers changes.  We always maintain a
// valid cache of the parent layers.  Note this function must also be
// called during Load(), as the values are not saved.
//
void CATClipRoot::RefreshParentLayerCache()
{
	int nParent = -1;
	for (int i = 0; i < nNumLayers; i++) {
		//	DbgAssert(tabLayers[i]);
			// this method can be called from within and undo
		if (tabLayers[i]) {
			if (GetLayer(i)->ApplyAbsolute()) {
				nParent = i;
				tabLayers[i]->SetParentLayer(-1);
			}
			else {
				tabLayers[i]->SetParentLayer(nParent);
			}
			// undate the index param
			tabLayers[i]->SetIndex(i);
		}
	}
	//GetCATParentTrans()->CATMessage(CLIP_WEIGHTS_CHANGED, -1);
}

//
// This tests if a layer contributes to the final result from the
// specified locale.  A layer has no contribution in the following
// cases:
//  (1) The layer is not in the specified range.
//  (2) The layer method is LAYER_IGNORE.
//  (3) We are not in solo state and the layer is disabled.
//  (4) We are in solo state and this layer is not the soloed
//      absolute layer.
//  (5) We are in solo state and this layer is not a child
//      of the soloed absolute layer.
//  (6) We are in solo state and this layer is a child of
//      the soloed absolute layer but is disabled during solo.
//	(7) We have a weight of 0%
//  (8) There is an absolute layer further down the list with
//      an effective weighting of 100%.
//
// Note:
//   For absolute layers, solo overrides the enable state.
//   For relative layers, enable state overrides the solo.
//

//BOOL CATClipRoot::LayerHasContribution(int nLayer, TimeValue t, const LayerRange &range/*=LAYER_RANGE_ALL*/) {
//	return LayerHasContribution(nLayer, t, range, weights);
//}

//BOOL CATClipRoot::LayerHasContribution(int nLayer, TimeValue t, LayerRange range, CATClipThing *pLocale)
BOOL CATClipRoot::LayerHasContribution(int nLayer, TimeValue t, LayerRange range, CATClipWeights *localweights)
{
	range.Limit(0, nNumLayers - 1);

	// Case (1): Is the layer in range?
	if (!range.Contains(nLayer)) return FALSE;

	NLAInfo *layerinfo = GetLayer(nLayer);

	// Case (2): Is the layer method LAYER_IGNORE?
	if (layerinfo->GetMethod() == LAYER_IGNORE) return FALSE;

	// If we are in the solo state, a value will be returned
	// immediately.  No further checks need to be performed.
	//
	if (nSolo == nLayer) {
		// Case (4): If we are the soloed layer, we have a contribution.
		return TRUE;
	}
	else if (nSolo >= 0) {
		// Cases (5) and (6) are tied together with this test.
		if (nSolo == layerinfo->GetParentLayer() && !layerinfo->TestFlag(LAYER_NO_SOLO)) {
			return layerinfo->LayerEnabled();
		}
		else {
			return FALSE;
		}
	}
	else {
		// Case (3): Is the layer enabled?
		if (!layerinfo->LayerEnabled()) return FALSE;
	}

	// Case 7. We must have some weight to contribute
	if (localweights) {

		Interval valid = FOREVER;
		if (localweights->GetWeight(t, nLayer, valid) == 0.0f) return FALSE;

		// Case (8): Check all absolute layers below this layer
		// within the range.  Note that at this point, we know
		// we are not in a solo state.  We test the layer weight
		// but also Case (3).
		valid = FOREVER;
		for (int i = nLayer + 1; i <= range.nLast; i++) {
			if (GetLayer(i)->ApplyAbsolute() && GetLayer(i)->LayerEnabled()) {
				if (localweights->GetWeight(t, i, valid) == 1.0f) return FALSE;
			}
		}
	}
	else {
		Interval valid = FOREVER;
		for (int i = nLayer + 1; i <= range.nLast; i++) {
			if (GetLayer(i)->ApplyAbsolute() && GetLayer(i)->LayerEnabled()) {
				if (GetLayer(i)->GetWeight(t, valid) == 1.0f) return FALSE;
			}
		}
	}
	return TRUE;
}

// Sets a layer as the active layer.  Specify -1 to deselect, or
// call SelectNone().  If a layer is active, any key transformations
// get applied only to its keys.  The time range for all parent
// nodes in the hierarchy is just the time range of the active layer.
// GetValue() grabs unweighted data straight out of the selected
// anim, and SetValue() pumps data straight in.
//
void CATClipRoot::SelectLayer(int n)
{
	if (n >= nNumLayers/* || n < 0*/) n = nNumLayers - 1;

	// Don't reselect an already selected layer.
	if (nSelected == n)
		return;

	// if you are selecting a new level and you are in subobject selection mode,
	// then get out of subobject selection mode
	if (nSelected != n)
	{
		Interface *ip = GetCOREInterface();
		if (ip->GetSubObjectLevel() > 0)
			ip->SetSubObjectLevel(0);
	}

	// this method is called from within an Undo, so we really want to avoid
	// registering a new Unod object during a restore
	BOOL newundo = FALSE;
	if (!theHold.Holding() && !theHold.RestoreOrRedoing()) {
		theHold.Begin();
		newundo = TRUE;
	}

	if (theHold.Holding()) theHold.Put(new RootLayerChangeRestore(this, CLIP_LAYER_SELECT, n, newundo, FALSE));
	CATMessages(0, CLIP_LAYER_SELECT, n);

	if (newundo && theHold.Holding()) {
		if (n == -1)	theHold.Accept(GetString(IDS_HLD_SLOTSEL));
		else		theHold.Accept(GetLayer(n)->GetName() + GetString(IDS_HLD_LYRSEL));
	}
	NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);

	// Don't forget to update the UI
	RefreshLayerRollout();

	NotifyActiveLayerChangeCallbacks();
}

// Deselects layer.
void CATClipRoot::SelectNone()
{
	SelectLayer(-1);
	//	CATMessages(0, CLIP_LAYER_SELECT, -1);
	//	nSelected = -1;
	//	NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);
	//	NotifyActiveLayerChangeCallbacks();
}

int CATClipRoot::GetSelectedLayer() const
{
	return nSelected;
}

void CATClipRoot::SoloLayer(int id)
{
	// Do not solo relative (or non-existent) layers
	NLAInfo* pLayer = GetLayer(id);
	if (id >= 0 && (pLayer == NULL || !pLayer->ApplyAbsolute()))
		return;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) {
		theHold.Begin();
		newundo = TRUE;
	}
	if (theHold.Holding()) theHold.Put(new RootLayerChangeRestore(this, CLIP_LAYER_SOLOED, id, newundo, FALSE));

	nSolo = id;
	RefreshParentLayerCache();

	if (newundo && theHold.Holding()) {
		//This undo object is just to refresh the UI on a redo
		theHold.Put(new RootLayerChangeRestore(this, CLIP_LAYER_SELECT, id, FALSE, TRUE));

		if (id == -1)	theHold.Accept(GetString(IDS_HLD_LYRSOLO));
		else			theHold.Accept(GetString(IDS_HLD_LYRSOLO));
		if (catparenttrans)
		{
			catparenttrans->UpdateCharacter();
			catparenttrans->UpdateColours();
		}
	}
}

int CATClipRoot::GetSoloLayer() const
{
	return nSolo;
}

// Calls The Import BVH Script Function
///BOOL ImportBVH(INode* catparent_node, TSTR file=_T(""), TSTR camfile=_T(""), BOOL quiet=FALSE)
BOOL DoScriptImport(const TSTR& function, INode* catparent_node, const TSTR& file = _T(""), const TSTR& camfile = _T(""), BOOL quiet = FALSE)
{
	ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
	MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
	seven_value_locals_tls(name, fn, catparent_node, file, camfile, result, quiet);

	try
	{
		ScopedSaveCurrentFrames scopedSaveCurrentFrames(_tls);
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(function.data());
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		if (vl.fn != NULL && vl.fn->tag == class_tag(MAXScriptFunction)) {
			Value* args[7];

			// CaptureAnimation takes 4 parameters, the catparents node
			// src hierarchy node,
			// and an optional keyword paramter, replace, which tells
			// convertToArchMat whether to replace all reference to
			// the old material by the new one.
			args[0] = vl.catparent_node = MAXNode::intern(catparent_node);
			args[1] = file.length() > 3 ? Name::intern(file.data()) : &undefined;

			args[2] = &keyarg_marker;								// Separates keyword params from mandatory

			// Keyword paramters are given in pairs, first the name
			// followed by the value. Missing keyword parameters
			// are assigned defaults from the function definition.
			args[3] = vl.camfile = Name::intern(_T("CamFile"));	// Keyword "CamFile"
			args[4] = camfile.length() > 3 ? Name::intern(camfile.data()) : &undefined;

			// Keyword paramters are given in pairs, first the name
			// followed by the value. Missing keyword parameters
			// are assigned defaults from the function definition.
			args[5] = vl.quiet = Name::intern(_T("quiet"));	// Keyword "quiet"
			args[6] = quiet ? &true_value : &false_value;	// Value true or false based on argument

			// Call the funtion and save the result.
			vl.result = static_cast<MAXScriptFunction*>(vl.fn)->apply(args, 7);
		}
	}
	catch (MAXScriptException& e)
	{
		ProcessMAXScriptException(e, _T("DoScriptImport Script"), false, false, true);
	}
	catch (...)
	{
		ProcessMAXScriptException(UnknownSystemException(), _T("DoScriptImport Script"), false, false, true);
	}

	// converted will be NULL if the conversion failed.
	return TRUE;
}

BOOL CATClipRoot::LoadHTR(TSTR filename, TSTR camfile, BOOL quiet/*=FALSE*/)
{
	DbgAssert(catparenttrans);
	DoScriptImport(_T("CATImportHTR"), catparenttrans->GetNode(), filename, camfile, quiet);
	return TRUE;
}

BOOL CATClipRoot::LoadBVH(TSTR filename, TSTR camfile, BOOL quiet/*=FALSE*/)
{
	DbgAssert(catparenttrans);
	DoScriptImport(_T("CATImportBVH"), catparenttrans->GetNode(), filename, camfile, quiet);
	return TRUE;
}

BOOL CATClipRoot::LoadBIP(TSTR filename, TSTR camfile, BOOL quiet/*=FALSE*/)
{
	DbgAssert(catparenttrans);
	if (!camfile) camfile = _T("");
	DoScriptImport(_T("CATImportBip"), catparenttrans->GetNode(), filename, camfile, quiet);
	return TRUE;
}

BOOL CATClipRoot::LoadFBX(TSTR filename, TSTR camfile, BOOL quiet/*=FALSE*/)
{
	DbgAssert(catparenttrans);
	if (!camfile) camfile = _T("");
	DoScriptImport(_T("CATImportFBX"), catparenttrans->GetNode(), filename, camfile, quiet);
	return TRUE;
}

////////////////////////////////////////////////////////////////
// These are used for registering and unregistering callbacks
// that get notified when the active layer is changed (which
// happens in SelectLayer()).
//
static CATActiveLayerChangeCallbackRegister gActiveLayerChangeCallbackRegister;

void CATClipRoot::NotifyActiveLayerChangeCallbacks() {
	gActiveLayerChangeCallbackRegister.NotifyActiveLayerChange(nSelected);
}

void CATClipRoot::RegisterActiveLayerChangeCallback(CATActiveLayerChangeCallback *pCallBack) {
	gActiveLayerChangeCallbackRegister.RegisterCallback(pCallBack);
}

void CATClipRoot::UnRegisterActiveLayerChangeCallback(CATActiveLayerChangeCallback *pCallBack) {
	gActiveLayerChangeCallbackRegister.UnRegisterCallback(pCallBack);
}

/*
// Accessors to the main layer weight.
float CATClipRoot::GetLayerWeight(int n, TimeValue t) const
{
	// 1st Layer is ALWAYS weight 1.0.
	if(n <= 0 || !tabLayers[n]) return 1.0f;
	DbgAssert(n < nNumLayers);
	float weight;
	// CAT2
	weight = tabLayers[n]->GetWeight(t, FOREVER);
	return weight;
}

void CATClipRoot::SetLayerWeight(int n, TimeValue t, float weight)
{
	DbgAssert(n < nNumLayers);
	// CAT2
	tabLayers[n]->SetWeight(t, weight);
}
*/

float CATClipRoot::GetLayerWeightLocal(int n, TimeValue t) const
{
	// ST - 18/12/03 CATMotion is ALWAYS weight 1.
	if (n == 0) return 1.0f;
	DbgAssert(n < nNumLayers);
	float weight = 0.0f;
	Interval iv = FOREVER;
	if (localBranchWeights)
		localBranchWeights->GetLayer(n)->GetValue(t, (void*)&weight, iv, CTRL_ABSOLUTE);
	return weight;
}

void CATClipRoot::SetLayerWeightLocal(int n, TimeValue t, float weight)
{
	DbgAssert(n < nNumLayers);
	if (localBranchWeights)
		localBranchWeights->GetLayer(n)->SetValue(t, (void*)&weight, 1, CTRL_ABSOLUTE);
}

/*
// the interval for a timing graph will always be 'instant'
// so isn't helpfull in generating an interval for the CATClipValue that called it
TimeValue CATClipRoot::GetLayerTime(int id, TimeValue t)
{
	return tabLayers[id] ? tabLayers[id]->GetTime(t, FOREVER) : t;
};

void CATClipRoot::SetLayerTime(int id, TimeValue inputt, TimeValue valuet)
{
	if(tabLayers[id]) tabLayers[id]->SetTime(inputt, valuet);
}
*/

float CATClipRoot::GetCombinedLayerWeightLocal(int n, TimeValue t)
{
	// ST - 18/12/03 CATMotion is ALWAYS weight 1.
	if (n == 0) return 1.0f;
	Interval iv = FOREVER;
	if (localBranchWeights)
		return localBranchWeights->GetWeight(t, n, iv);
	// return the global weight instead
	iv = FOREVER;
	NLAInfo* pInfo = GetLayer(n);
	if (pInfo != NULL)
		return pInfo->GetWeight(t, iv);

	// Failure, return 1.0f;
	return 1.0f;
}

CATClipRoot::CATClipRoot(BOOL loading/*=FALSE*/) : nClipTreeViewID(0L),
nNumHubGroups(0), flagsbegin(0L), nNumProps(0)
{
	UNREFERENCED_PARAMETER(loading);
	Init();
}

// Check to see if the ranges and weightview are
// are still alive. If they are, force their deletion
CATClipRoot::~CATClipRoot()
{
	DeleteAllRefs();

	if (g_PasteLayerData.cliproot == this) {
		g_PasteLayerData.cliproot = NULL;
		g_PasteLayerData.layerinfo = NULL;
	}

	// We need to ensure, that if we are deleted all CATClipValues
	// are neutralized.  Because there are no references, they
	// may end up holding a hanging pointer.
	CleanCATClipRootPointer(this);
}

// Do whatever here...
void CATClipRoot::RenameTreeView()
{

}

// Load/Save stuff
enum {
	CLIP_THING_CHUNK,
	CLIP_NUMLAYERS_CHUNK,
	CLIP_LAYERNAMES_CHUNK,
	CLIP_LAYERFLAGS_CHUNK,
	CLIP_LAYERUNITS_CHUNK,
	CLIP_TRANSFORMS_CHUNK,
	CLIP_RANGES_CHUNK,
	CLIP_WEIGHTVIEW_CHUNK,
	CLIP_SELECTED_CHUNK,
	CLIP_CATPARENT_CHUNK,
	CLIP_GHOSTUNITS_CHUNK,
	CLIP_GHOSTCOLOURS_CHUNK,
	CLIP_GHOSTOBJECTS_CHUNK,
	CLIP_KALIFTING_CHUNK,
	CLIP_SOLO_CHUNK,
	CLIP_NUMHUBGROUPS_CHUNK,
	CLIP_NUMPROPS_CHUNK,
	CLIP_FILEVERSION_CHUNK,
	CLIP_TIMING_CHUNK,
	CLIP_CATPARENTTRANS_CHUNK,
	CLIP_FLAGS_CHUNK,
};

IOResult CATClipRoot::Save(ISave *isave)
{
	DWORD nb, refID;

	// Save out our file version
	isave->BeginChunk(CLIP_FILEVERSION_CHUNK);
	dwFileVersion = CAT_VERSION_CURRENT;
	isave->Write(&dwFileVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	isave->BeginChunk(CLIP_FLAGS_CHUNK);
	isave->Write(&flags, sizeof(DWORD), &nb);
	isave->EndChunk();

	if (g_bSavingCAT3Rig) {
		int temp;
		// Number of layers
		isave->BeginChunk(CLIP_NUMLAYERS_CHUNK);
		temp = 0;
		isave->Write(&temp, sizeof(int), &nb);
		isave->EndChunk();

		// Stores the current layer selection.
		isave->BeginChunk(CLIP_SELECTED_CHUNK);
		temp = -1;
		isave->Write(&temp, sizeof(int), &nb);
		isave->EndChunk();

		// Stores the current solo layer.
		isave->BeginChunk(CLIP_SOLO_CHUNK);
		temp = -1;
		isave->Write(&temp, sizeof(int), &nb);
		isave->EndChunk();

	}
	else {
		// Number of layers
		isave->BeginChunk(CLIP_NUMLAYERS_CHUNK);
		isave->Write(&nNumLayers, sizeof(int), &nb);
		isave->EndChunk();

		// Stores the current layer selection.
		isave->BeginChunk(CLIP_SELECTED_CHUNK);
		isave->Write(&nSelected, sizeof(int), &nb);
		isave->EndChunk();

		// Stores the current solo layer.
		isave->BeginChunk(CLIP_SOLO_CHUNK);
		isave->Write(&nSolo, sizeof(int), &nb);
		isave->EndChunk();
	}

	refID = isave->GetRefID((void*)catparenttrans);
	isave->BeginChunk(CLIP_CATPARENTTRANS_CHUNK);
	isave->Write(&refID, sizeof(DWORD), &nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult CATClipRoot::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb = 0L, refID = 0L;

	// The PLCB will delete itself after it has been run
	CATClipRootPLCB* plcb = new CATClipRootPLCB(this);

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case CLIP_THING_CHUNK:
			break;
		case CLIP_FILEVERSION_CHUNK:
			res = iload->Read(&dwFileVersion, sizeof(DWORD), &nb);
			break;
		case CLIP_FLAGS_CHUNK:
			res = iload->Read(&flags, sizeof(DWORD), &nb);
			break;
		case CLIP_NUMLAYERS_CHUNK:
		{
			res = iload->Read(&nNumLayers, sizeof(int), &nb);
			tabLayers.SetCount(nNumLayers);
			for (int i = 0; i < nNumLayers; i++)
				tabLayers[i] = NULL;
			break;
		}
		case CLIP_SELECTED_CHUNK:
			res = iload->Read(&nSelected, sizeof(int), &nb);
			break;
		case CLIP_SOLO_CHUNK:
			res = iload->Read(&nSolo, sizeof(int), &nb);
			break;
		case CLIP_CATPARENT_CHUNK:
			res = iload->Read(&refID, sizeof(int), &nb);
			if (res == IO_OK && refID != (DWORD)-1)
				iload->RecordBackpatch(refID, (void**)&plcb->mpCATParent);
			break;
		case CLIP_CATPARENTTRANS_CHUNK:
			res = iload->Read(&refID, sizeof(int), &nb);
			if (res == IO_OK && refID != (DWORD)-1)
				iload->RecordBackpatch(refID, (void**)&catparenttrans);
			break;
			//
			// Miscellaneous
			//
		case CLIP_NUMHUBGROUPS_CHUNK:
			res = iload->Read(&nNumHubGroups, sizeof(int), &nb);
			break;
		case CLIP_NUMPROPS_CHUNK:
			res = iload->Read(&nNumProps, sizeof(int), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(plcb);

	return IO_OK;
}

void CATClipRoot::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev/*=NULL*/)
{
	UNREFERENCED_PARAMETER(prev);

	// During collapsing we switch to the Hierarchy panel to speed things up.
	// We don't want the LayerManager rollout being destroyed
	//if(TestFlag(CLIP_FLAG_KEYFREEFORM)) return;

	this->ip = ip;
	flagsbegin = flags;
	ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_LAYER_MANAGER_ROLLOUT), ClipRolloutProc, GetString(IDS_LAYERMANAGER), (LPARAM)this, 0, ROLLUP_CAT_SYSTEM);
}

void CATClipRoot::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next/*=NULL*/)
{
	UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(next);

	//if(TestFlag(CLIP_FLAG_KEYFREEFORM)) return;

	if (staticClipRollout.GetHwnd()) {
		ip->DeleteRollupPage(staticClipRollout.GetHwnd());
	}
	this->ip = NULL;
}

//////////////////////////////////////////////////////////////////////////
// UI methods for the clip system to use internally
// TODO: finnish this off and make it go
void CATClipRoot::BeginEditLayers(IObjParam *ip, ULONG flags, CATClipValue* layerctrl, Animatable *prev/*=NULL*/)
{
	this->localLayerControl = layerctrl;
	this->localBranchWeights = layerctrl->GetWeightsCtrl();
	BeginEditParams(ip, flags, prev);
}

void CATClipRoot::EndEditLayers(IObjParam *ip, ULONG flags, CATClipValue* layerctrl, Animatable *next/*=NULL*/)
{
	UNREFERENCED_PARAMETER(layerctrl);
	this->localLayerControl = NULL;
	this->localBranchWeights = NULL;
	EndEditParams(ip, flags, next);
}

void CATClipRoot::ShowLayerRollouts(BOOL tf)
{
	UNREFERENCED_PARAMETER(tf);
#ifdef DISPLAY_LAYER_ROLLOUTS_TOGGLE
	if (tf)	SetFlag(CLIP_FLAG_SHOW_LAYER_ROLLOUTS);
	else	ClearFlag(CLIP_FLAG_SHOW_LAYER_ROLLOUTS);
	if (!localLayerControl || !ip) return;
	if (tf) {
		localLayerControl->BeginEditLayers(ip, flagsbegin, catparenttrans->GetCATMode());
	}
	else {
		localLayerControl->EndEditLayers(ip, flagsbegin, catparenttrans->GetCATMode());
	}
#endif
}

//////////////////////////////////////////////////////////////////////////

// TODO...  maybe need to catch changes to transforms in order
// to update pointer.
RefResult CATClipRoot::NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL)
{
	return REF_SUCCEED;
}

// We override these methods to do special things with
// key ranges.  This allows us to use the track view as
// a non-linear animation editor.  Isn't that cool?!
Interval CATClipRoot::GetLayerTimeRange(int index, DWORD flags)
{
	CATNodeControl* pRootHub = GetRootHub();
	if (pRootHub != NULL)
		return pRootHub->GetLayerTimeRange(index, flags);
	return NEVER;
}

void CATClipRoot::EditLayerTimeRange(int index, Interval range, DWORD flags)
{
	CATNodeControl* pRootHub = GetRootHub();
	if (pRootHub != NULL)
		pRootHub->EditLayerTimeRange(index, range, flags);
}

void CATClipRoot::MapLayerKeys(int index, TimeMap *map, DWORD flags)
{
	CATNodeControl* pRootHub = GetRootHub();
	if (pRootHub != NULL)
		pRootHub->MapLayerKeys(index, map, flags);
}

void CATClipRoot::SetLayerORT(int index, int ort, int type)
{
	CATNodeControl* pRootHub = GetRootHub();
	if (pRootHub != NULL)
		pRootHub->SetLayerORT(index, ort, type);
}

//////////////////////////////////////////////////////
// For CAT2, our CATclip root is abandoning the CATClipThing Class Hierarchy
int CATClipRoot::NumRefs() {
	// Do not save our layers to the rig file
	if (g_bSavingCAT3Rig) {
		return 0;
	}
	else {
		return nNumValues + nNumBranches + nNumLayers;
	}
}

RefTargetHandle CATClipRoot::GetReference(int i)
{
	// Do not save our layers to the rig file
	if (g_bSavingCAT3Rig) {

	}
	else {
		i -= (nNumValues + nNumBranches);
		if (i < nNumLayers)	return tabLayers[i];
	}
	return NULL;

}

void CATClipRoot::SetReference(int i, RefTargetHandle rtarg)
{
	i -= (nNumValues + nNumBranches);
	if (i < nNumLayers)	tabLayers[i] = (NLAInfo*)rtarg;;

}

Animatable* CATClipRoot::SubAnim(int i)
{
	{
		i -= (nNumValues + nNumBranches);
		if (i < nNumLayers)	return tabLayers[i];
		else				return NULL;
	}
}

// GB 30-Jul-03: Changed this function to call ValueName() and
// BranchName() to get the correct names for values and branches,
// rather than using the locally-stored names (tabValueNames and
// tabBranchNames).  These are used only as fallbacks if we can't
// get the name from somewhere else.
TSTR CATClipRoot::SubAnimName(int i)
{
	if (i < (nNumValues + nNumBranches))
		return GetString(IDS_CAT1_SUBANIM); // CATClipThing::SubAnimName(i);
	else {
		i -= (nNumValues + nNumBranches);
		if (i < nNumLayers)	return tabLayers[i]->GetName();
		else 				return _T("");
	}

}

RefTargetHandle CATClipRoot::Clone(RemapDir& remap)
{
	CATClipRoot *ctrl = (CATClipRoot*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATCLIPROOT_CLASS_ID);
	DbgAssert(ctrl);

	remap.PatchPointer((RefTargetHandle*)&ctrl->catparenttrans, catparenttrans);

	int numlayers = NumLayers();
	ctrl->ResizeList(numlayers, TRUE);
	for (int i = 0; i < numlayers; i++) {
		ctrl->ReplaceReference(i, remap.CloneRef(tabLayers[i]));
	}

	ctrl->nSelected = nSelected;

	BaseClone(this, ctrl, remap);

	return ctrl;
}

void CATClipRoot::CopyLayer(int n) {
	g_PasteLayerData.cliproot = this;
	g_PasteLayerData.layerinfo = GetLayer(n);
}

BOOL CATClipRoot::IsLayerCopied() {
	if (!g_PasteLayerData.cliproot ||
		g_PasteLayerData.cliproot->AsControl()->TestAFlag(A_IS_DELETED) ||
		g_PasteLayerData.cliproot->GetCATParentTrans()->GetNode()->TestAFlag(A_IS_DELETED)
		) return FALSE;

	for (int i = 0; i < g_PasteLayerData.cliproot->NumLayers(); i++)
		if (g_PasteLayerData.layerinfo == g_PasteLayerData.cliproot->GetLayer(i))
			return TRUE;

	return FALSE;
}

void CATClipRoot::PasteLayer(BOOL instance, BOOL duplicatelayerinfo, int targetLayerIdx, CATCharacterRemap* pRemap)
{
	UNREFERENCED_PARAMETER(duplicatelayerinfo);
	if (!IsLayerCopied()) return;

	int tolayer = (targetLayerIdx >= 0) ? targetLayerIdx : nSelected;
	if (tolayer == -1) tolayer = nNumLayers;
	if (tolayer < 0 || tolayer > nNumLayers)
		return;

	// Macro-recorder support
	MACRO_DISABLE;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
	if (theHold.Holding()) theHold.Put(new RootLayerChangeRestore(this, CLIP_LAYER_PASTED, tolayer, newundo, FALSE));

	Interface* pCore = GetCOREInterface();
	ICATParentTrans*  pCATParent = GetCATParentTrans();

	if (pCATParent == NULL)
		return;

	pCore->DisableSceneRedraw();
	SuspendAnimate();

	//////////////////////////////////////////////////////////////
	// Prepare a new layer to be the target of the paste operation

	if (g_PasteLayerData.layerinfo->GetMethod() == LAYER_CATMOTION) {
		// This layer is about to be replaced anyway so don't build a CATMotion layer
		InsertLayer(g_PasteLayerData.layerinfo->GetName(), tolayer, LAYER_ABSOLUTE);
	}
	else
		InsertLayer(g_PasteLayerData.layerinfo->GetName(), tolayer, g_PasteLayerData.layerinfo->GetMethod());

	// Make sure our new layer contains setup pose before we start copying data into it.
	// This means that any rig elements that are not affected by the layer copy still have an appropriate pose.
	if (g_PasteLayerData.layerinfo->ApplyAbsolute()) {
		pCATParent->CATMessage(pCore->GetTime(), CLIP_LAYER_DUMP_SETUP_POSE_TO_LAYER, tolayer);
	}

	//////////////////////////////////////////////////////////////
	// Begin the Pasting Operation
	NLAInfo* newlayer = NULL;
	CATCharacterRemap *remap = NULL;
	if (pRemap == NULL)
	{
		remap = new CATCharacterRemap();
		pCATParent->BuildMapping(g_PasteLayerData.cliproot->GetCATParentTrans(), *remap, TRUE);
	}
	else
		remap = pRemap;

	// If we are instancing, keep the original layer info
	if (instance)
	{
		newlayer = g_PasteLayerData.layerinfo;
	}
	else
	{
		newlayer = (NLAInfo*)remap->FindMapping(g_PasteLayerData.layerinfo);
		if (newlayer == NULL)
		{
			RemapDir* rmdir = NewRemapDir();
			newlayer = static_cast<NLAInfo*>(g_PasteLayerData.layerinfo->Clone(*rmdir));
			remap->AddEntry(g_PasteLayerData.layerinfo, newlayer);
			rmdir->DeleteThis();
		}
	}
	newlayer->SetNLARoot(this);
	newlayer->SetIndex(tolayer);
	ReplaceReference(tolayer, newlayer);

	// reset pointers to layerinfo and catparenttrans in layer transform
	// the node and its layerTransform control have not been cloned  (SA 1/10)
	INode *node = newlayer->GetTransformNode();
	if (node)
	{
		Control *c = node->GetTMController();
		if (c)
		{
			LayerTransform *lt = (LayerTransform*)c;
			lt->ReInitialize(newlayer);
		}
	}

	// prepare flags for the rig hierarchy
	DWORD flags = 0;
	flags |= (instance ? PASTELAYERFLAG_INSTANCE : 0);

	// now rip thru the rest of the hierarchy pasting the layer
	ICATParentTrans* pDstParent = GetCATParentTrans();
	ICATParentTrans* pSrcParent = g_PasteLayerData.cliproot->GetCATParentTrans();

	if (pSrcParent != NULL &&
		pSrcParent->GetRootHub() &&
		pDstParent != NULL &&
		pDstParent->GetRootHub())
	{
		// We need a remap to handle cloning (if not instancing)
		RemapDir* rmdir = NewRemapDir();

		for (CATCharacterRemap::iterator itr = remap->begin(); itr != remap->end(); itr++)
		{
			CATClipValue* pSrc = dynamic_cast<CATClipValue*>(itr->first);
			CATClipValue* pDst = dynamic_cast<CATClipValue*>(itr->second);

			// If we have a match, then copy layer across.
			if (pSrc != NULL && pDst != NULL)
			{
				// Only copy and paste into the same CATParent character.  It is possible
				// for a single rig to contain multiple CATrigs (eg Marama), and if we tried
				// to copy between items not present in the rig this CATClipRoot belongs
				// to we can cause all sorts of problems.
				if (pSrc->GetCATParentTrans() == pSrcParent &&
					pDst->GetCATParentTrans() == pDstParent)
				{
					pDst->PasteLayer(pSrc, g_PasteLayerData.layerinfo->GetIndex(), tolayer, flags, *rmdir);
				}
			}
		}

		rmdir->DeleteThis();
	}

	if (pRemap == NULL)
		delete remap;
	remap = NULL;

	if (newlayer->GetMethod() == LAYER_CATMOTION) {
		// If the structer of the original rig is different from that of this rig,
		// then we need to prune branches from the CATMotionHierarchy that are no longer needed
		((CATMotionLayer*)newlayer)->PostPaste_CleanupLayer();
	}

	// Pasting Done!!!
	//////////////////////////////////////////////////////////////

	SelectLayer(tolayer);

	ResumeAnimate();
	pCore->EnableSceneRedraw();

	if (theHold.Holding() && newundo) {
		//This undo object is just to refresh the UI on a redo
		theHold.Put(new RootLayerChangeRestore(this, -1, nSelected, FALSE, newundo));

		theHold.Accept(GetString(IDS_HLD_LYRPASTE));

		if (pCATParent->GetEffectiveColourMode() != COLOURMODE_CLASSIC)
			catparenttrans->UpdateColours(FALSE, TRUE);

		catparenttrans->UpdateCharacter();
		UpdateUI(TRUE);
	}

	// Macro-recorder support
	MACRO_ENABLE;
}

void CATClipRoot::AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	PreResetCleanup();
	CloseRangesView();
	CloseGlobalWeightsView();
	CloseLocalWeightsView();
	CloseTimeWarpView();

	// kill the copy layer cache if it pointed to us
	if (g_PasteLayerData.cliproot == this) {
		g_PasteLayerData.cliproot = NULL;
		g_PasteLayerData.layerinfo = NULL;
	}
	// if we are deleting the character make sure we delete all the transform nodes
	for (int i = 0; i < NumLayers(); i++) {
		GetLayer(i)->AddSystemNodes(nodes, ctxt);
	}
}

// ----------------- ILayerRootFP functions ------------------
int CATClipRoot::AppendLayer(const MSTR& name, const TCHAR *method)
{
	int newlayerindex = AppendLayer(name, MethodFromString(method)) + 1;
	RefreshLayerRollout();// script registers a new undo, and so we need to explicitly refresh the UI
	return newlayerindex;
}

BOOL CATClipRoot::InsertLayer(const MSTR& name, int layer, const TCHAR *method)
{
	BOOL result = InsertLayer(name, layer, MethodFromString(method));
	RefreshLayerRollout();// script registers a new undo, and so we need to explicitly refresh the UI
	return result;
}

Color CATClipRoot::GetLayerColor(int index)
{
	NLAInfo* pLayer = GetLayer(index);
	if (pLayer != NULL && pLayer->ApplyAbsolute())
		return pLayer->GetColour();

	// If we don't have a good layer selected, return error color (black)
	return 0;
}

BOOL CATClipRoot::SetLayerColor(int index, Color *newColor)
{
	if (newColor == NULL)
		return FALSE;
	NLAInfo* pLayer = GetLayer(index);
	if (pLayer != NULL && pLayer->ApplyAbsolute())
	{
		pLayer->SetColour(*newColor);
		return TRUE;
	}
	return FALSE;
}

BOOL CATClipRoot::SaveClip(const MSTR& filename, TimeValue start_t, TimeValue end_t, int from_layer, int to_layer)
{
	DbgAssert(catparenttrans);
	return catparenttrans->SaveClip(filename, start_t, end_t, from_layer, to_layer);
};

BOOL CATClipRoot::SavePose(const MSTR& filename) {
	DbgAssert(catparenttrans);
	return catparenttrans->SavePose(filename);
};

INode* CATClipRoot::LoadClip(const MSTR& filename, TimeValue t, BOOL scale_data, BOOL transformData, BOOL mirrorData, BOOL mirrorWorldX, BOOL mirrorWorldY)
{
	DbgAssert(catparenttrans);
	return catparenttrans->LoadClip(filename, t, scale_data, transformData, mirrorData, mirrorWorldX, mirrorWorldY);
};

INode* CATClipRoot::LoadPose(const MSTR& filename, TimeValue t, BOOL scale_data, BOOL transformData, BOOL mirrorData, BOOL mirrorWorldX, BOOL mirrorWorldY)
{
	DbgAssert(catparenttrans);;
	return catparenttrans->LoadPose(filename, t, scale_data, transformData, mirrorData, mirrorWorldX, mirrorWorldY);
};
INode* CATClipRoot::CreatePasteLayerTransformNode() {
	return catparenttrans->CreatePasteLayerTransformNode();
}

TSTR CATClipRoot::GetFileTagValue(const MSTR& filename, const MSTR& tag)
{
	DbgAssert(catparenttrans);;
	return catparenttrans->GetFileTagValue(filename, tag);
};

void CATClipRoot::CollapsePoseToCurLayer(TimeValue t)
{
	DbgAssert(catparenttrans);;
	catparenttrans->CollapsePoseToCurLayer(t);
};

BOOL CATClipRoot::CollapseTimeRangeToLayer(TimeValue start_t, TimeValue end_t, TimeValue iKeyFreq, BOOL regularplot, int numpasses, float posdelta, float rotdelta)
{
	DbgAssert(catparenttrans);
	return catparenttrans->CollapseTimeRangeToLayer(start_t, end_t, iKeyFreq, regularplot, numpasses, posdelta, rotdelta);
};

int CATClipRoot::GetTrackDisplayMethod() const {
	return (TestFlag(CLIP_FLAG_SHOW_ALL_LAYERS) ? 3 : (TestFlag(CLIP_FLAG_SHOW_CONTRIBUTING_LAYERS) ? 2 : 1));
};

void CATClipRoot::SetTrackDisplayMethod(int n) {
	switch (n) {
	case TRACK_DISPLAY_METHOD_ACTIVE:
		ClearFlag(CLIP_FLAG_SHOW_CONTRIBUTING_LAYERS);
		ClearFlag(CLIP_FLAG_SHOW_ALL_LAYERS);
		break;
	case TRACK_DISPLAY_METHOD_CONTRIBUTING:
		SetFlag(CLIP_FLAG_SHOW_CONTRIBUTING_LAYERS);
		ClearFlag(CLIP_FLAG_SHOW_ALL_LAYERS);
		break;
	case TRACK_DISPLAY_METHOD_ALL:
		ClearFlag(CLIP_FLAG_SHOW_CONTRIBUTING_LAYERS);
		SetFlag(CLIP_FLAG_SHOW_ALL_LAYERS);
		break;
	};
	CATMessages(0, CAT_TDM_CHANGED, n);
};

void  CATClipRoot::ShowRangesView()
{
	if (NumLayers() == 0) return;
	ITreeViewOps	*pRangesViewOps = NULL;
	// See if the window is open already;
	if (idRangesView >= 0) {
		if (NumLayers() == 0) {
			if (IsTrackViewOpen(idRangesView)) {
				CloseTrackView(idRangesView);
				return;
			}
			else return;
		}
		pRangesViewOps = GetTrackView(idRangesView);
	}
	if (NumLayers() == 0) return;
	if (!pRangesViewOps) {
		idRangesView = GetNumAvailableTrackViews();
		pRangesViewOps = GetTrackView(catparenttrans->GetCATName() + _T(": Layer Ranges")); // globalize? no - used to get tv, could be stored in file
	}
	if (pRangesViewOps) {
		ITrackViewNode *pTrackViewNode = CreateITrackViewNode(TRUE);
		pTrackViewNode->HideChildren(FALSE);
		pRangesViewOps->SetRootTrack(pTrackViewNode);
		ITreeViewUI_ShowControllerWindow(pRangesViewOps);
		pRangesViewOps->fpGetUIInterface()->LoadUILayout(GetString(IDS_DOPE_SHEET_LAYOUT));
		pRangesViewOps->SetEditMode(MODE_EDITRANGES);

		pTrackViewNode->AddController(this, _T("Layer Ranges"), CAT_TRACKVIEW_TVCLSID);
		pRangesViewOps->ZoomOn(this, 0);
	}
}
void CATClipRoot::CloseRangesView()
{
	if ((idRangesView >= 0) && IsTrackViewOpen(idRangesView))			CloseTrackView(idRangesView);
}

void CATClipRoot::ShowGlobalWeightsView()
{
	ITreeViewOps	*pGlobalWeightsViewOps = NULL;
	// See if the window is open already;
	if (idGlobalWeightsView >= 0) {
		if (NumLayers() == 0) {
			if (IsTrackViewOpen(idGlobalWeightsView)) {
				CloseTrackView(idGlobalWeightsView);
				return;
			}
			else return;
		}
		pGlobalWeightsViewOps = GetTrackView(idGlobalWeightsView);
	}
	if (NumLayers() == 0) return;
	if (!pGlobalWeightsViewOps) {
		idGlobalWeightsView = GetNumAvailableTrackViews();
		pGlobalWeightsViewOps = GetTrackView(catparenttrans->GetCATName() + _T(": Global Layer Weights")); // don't globalize tv names (?)
	}
	if (pGlobalWeightsViewOps) {
		ITrackViewNode *pTrackViewNode = CreateITrackViewNode(CAT_HIDE_TV_ROOT_TRACKS);

		pGlobalWeightsViewOps->SetRootTrack(pTrackViewNode);
		ITreeViewUI_ShowControllerWindow(pGlobalWeightsViewOps);

		pGlobalWeightsViewOps->SetEditMode(MODE_EDITFCURVE);

		for (int i = 0; i < NumLayers(); i++)
			pTrackViewNode->AddController(GetLayer(i)->GetWeightsController(), GetLayer(i)->GetName(), CAT_TRACKVIEW_TVCLSID);
		pGlobalWeightsViewOps->ExpandTracks();
		pGlobalWeightsViewOps->ZoomOn(pTrackViewNode, 0);
	}

	/*	if(pGlobalWeightsView && pGlobalWeightsView->IsActive()) pGlobalWeightsView->Flush();

		ITrackViewNode *pTrackViewNode = CreateITrackViewNode(CAT_HIDE_TV_ROOT_TRACKS);
		for(int i=0;i<NumLayers(); i++)
			pTrackViewNode->AddController(GetWeightsController(i), GetLayerName(i), CAT_TRACKVIEW_TVCLSID);

		pGlobalWeightsView = GetCOREInterface()->CreateTreeViewChild(pTrackViewNode, GetCOREInterface()->GetMAXHWnd(), TVSTYLE_MAXIMIZEBUT |TVSTYLE_INVIEWPORT|TVSTYLE_INMOTIONPAN, 0, OPENTV_SPECIAL);

		pGlobalWeightsView->ExpandTracks();
		pGlobalWeightsView->ZoomOn(pTrackViewNode, 0);
	*/
}
void CATClipRoot::CloseGlobalWeightsView()
{
	if ((idGlobalWeightsView >= 0) && IsTrackViewOpen(idGlobalWeightsView))	CloseTrackView(idGlobalWeightsView);
}

void CATClipRoot::ShowLocalWeightsView(CATClipValue *localweights)
{
	if (!localweights) return;
	ITreeViewOps	*pLocalWeightsViewOps = NULL;
	// See if the window is open already;
	if (idLocalWeightsView >= 0) {
		if (NumLayers() == 0) {
			if (IsTrackViewOpen(idLocalWeightsView)) {
				CloseTrackView(idLocalWeightsView);
				return;
			}
			else return;
		}
		pLocalWeightsViewOps = GetTrackView(idLocalWeightsView);
	}
	if (NumLayers() == 0) return;
	if (!pLocalWeightsViewOps) {
		idLocalWeightsView = GetNumAvailableTrackViews();
		pLocalWeightsViewOps = GetTrackView(catparenttrans->GetCATName() + _T(": Local Layer Weights")); // don't globalize tv names (?)
	}
	if (pLocalWeightsViewOps) {
		ITrackViewNode *pTrackViewNode = CreateITrackViewNode(CAT_HIDE_TV_ROOT_TRACKS);
		pLocalWeightsViewOps->SetRootTrack(pTrackViewNode);
		ITreeViewUI_ShowControllerWindow(pLocalWeightsViewOps);

		pLocalWeightsViewOps->SetEditMode(MODE_EDITFCURVE);

		for (int i = 0; i < NumLayers(); i++)
			pTrackViewNode->AddController(localweights->GetLayer(i), GetLayer(i)->GetName(), CAT_TRACKVIEW_TVCLSID);
		pLocalWeightsViewOps->ExpandTracks();
		pLocalWeightsViewOps->ZoomOn(pTrackViewNode, 0);
	}
}
void CATClipRoot::CloseLocalWeightsView()
{
	if ((idLocalWeightsView >= 0) && IsTrackViewOpen(idLocalWeightsView))	CloseTrackView(idLocalWeightsView);
}

void CATClipRoot::ShowTimeWarpView()
{
	ITreeViewOps	*pTimeWarpViewOps = NULL;
	// See if the window is open already;
	if (idTimeWarpView >= 0) {
		if (NumLayers() == 0) {
			if (IsTrackViewOpen(idTimeWarpView)) {
				CloseTrackView(idTimeWarpView);
				return;
			}
			else return;
		}
		pTimeWarpViewOps = GetTrackView(idTimeWarpView);
	}
	if (NumLayers() == 0) return;
	if (!pTimeWarpViewOps) {
		idTimeWarpView = GetNumAvailableTrackViews();
		pTimeWarpViewOps = GetTrackView(catparenttrans->GetCATName() + _T(": Layer Time Warp Curves"));  // don't globalize tv names (?)
	}
	if (pTimeWarpViewOps) {
		ITrackViewNode *pTrackViewNode = CreateITrackViewNode(CAT_HIDE_TV_ROOT_TRACKS);
		pTrackViewNode->HideChildren(FALSE);
		pTimeWarpViewOps->SetRootTrack(pTrackViewNode);
		ITreeViewUI_ShowControllerWindow(pTimeWarpViewOps);

		pTimeWarpViewOps->SetEditMode(MODE_EDITFCURVE);

		// Add all Info's to the cuve editor
		for (int i = 0; i < NumLayers(); i++) {
			NLAInfo* pInfo = GetLayer(i);
			if (pInfo == NULL)
				continue;

			// If the current layer is selected, ensure it has
			// a time warp available.
			if (i == GetSelectedLayer())
				pInfo->CreateTimeController();

			// Add the info the the curve editor
			HoldSuspend hs;
			pTrackViewNode->AddController(GetLayer(i), GetLayer(i)->GetName(), CAT_TRACKVIEW_TVCLSID);
		}

		pTimeWarpViewOps->ExpandTracks();

		int focus = (GetSelectedLayer() >= 0) ? GetSelectedLayer() : 0;
		pTimeWarpViewOps->ZoomOn(pTrackViewNode, focus);
		pTimeWarpViewOps->SelectTrackByIndex(4);
	}
}
void CATClipRoot::CloseTimeWarpView()
{
	if ((idTimeWarpView >= 0) && IsTrackViewOpen(idTimeWarpView))		CloseTrackView(idTimeWarpView);
}

void CATClipRoot::PreResetCleanup() {
	for (int i = 0; i < NumLayers(); i++) {
		if (GetLayer(i) && GetLayer(i)->ClassID() == GetCATMotionLayerDesc()->ClassID()) {
			CATHierarchyRoot *pHierarchyRoot = (CATHierarchyRoot*)((CATMotionLayer*)GetLayer(i))->GetReference(CATMotionLayer::REF_CATHIERARHYROOT);
			if (pHierarchyRoot) {
				pHierarchyRoot->IDestroyCATWindow();
			}
		}
	}
}

CATNodeControl* CATClipRoot::GetRootHub()
{
	ICATParentTrans* pCPTrans = GetCATParentTrans();
	if (pCPTrans != NULL)
		return pCPTrans->GetRootHub();
	return NULL;
}

//////////////////////////////////////////////////////////////////////////

class CleanDeadRootFromRigElements : public Animatable::EnumAnimList
{
private:
	CATClipRoot* mpDeadPtr;

public:

	CleanDeadRootFromRigElements(CATClipRoot* pDeadPtr)
		: mpDeadPtr(pDeadPtr)
	{	}

	virtual	bool proc(Animatable *theAnim)
	{
		CATClipValue *pValue = dynamic_cast<CATClipValue*>(theAnim);
		if (pValue != NULL)
		{
			__TRY{
				// Clean the pointer on any CATClipValues
				if (pValue->GetRoot() == mpDeadPtr)
					pValue->SetCATParentTrans(NULL);
			} __CATCH(...) {}
		}

		return true;
	}

};

void CleanCATClipRootPointer(CATClipRoot* pDeadRoot)
{
	CleanDeadRootFromRigElements cleaner(pDeadRoot);
	Animatable::EnumerateAllAnimatables(cleaner);
}

