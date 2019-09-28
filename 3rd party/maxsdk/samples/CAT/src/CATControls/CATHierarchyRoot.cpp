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

// This is the new CAT root class which handles all the special cases
// the root must manage. Weights branch.

#include "CATPlugins.h"
#include "macrorec.h"

 // Rig Structure
#include "ICATParent.h"
#include <CATAPI/CATClassID.h>
#include "Hub.h"

#include "ref.h"
// CATMotion
#include "Ease.h"
#include "CATHierarchyRoot.h"
#include "CATWindow.h"
#include "CATHierarchyBranch2.h"
#include "CATMotionHub2.h"
#include "CATMotionLayer.h"

// I put these in just so we could close the tree view window
#include "TrackViewFunctions.h"
#include "itreevw.h"
#include "tvnode.h"

//
//	CATHierarchyRootClassDesc
//
//	This gives the MAX information about our class
//	before it has to actually implement it.
//

// Will update 5 times a second
#define WAIT_CLOCKS CLOCKS_PER_SEC/10
#define FREQUENCY   100 // MILLISECONDS

//typedef struct TimeOutStruct TimerStruct;

class CATHierarchyRootClassDesc : public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }							// Show this in create branch?
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading);  return new CATHierarchyRoot(loading); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATHIERARCHYROOT); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }	// This determins the type of our controller
	Class_ID		ClassID() { return CATHIERARCHYROOT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATHierarchyRoot"); }			// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
};

// our global instance of our classdesc class.
static CATHierarchyRootClassDesc CATHierarchyRootDesc;
ClassDesc2* GetCATHierarchyRootDesc() { return &CATHierarchyRootDesc; }

static FPInterfaceDesc cathierarchyroot_FP_Interface(
	CATHIERARCHYROOT_INTERFACE, _T("CATMotion Functions"), 0, &CATHierarchyRootDesc, FP_MIXIN,

	ICATHierarchyRoot::fnShowCATWindow, _T("CreateCATWindow"), 0, TYPE_VOID, 0, 0,
	ICATHierarchyRoot::fnLoadPreset, _T("LoadPreset"), 0, TYPE_BOOL, 0, 2,
		_T("filename"), 0, TYPE_TSTR,
		_T("newlayer"), 0, TYPE_BOOL,
	ICATHierarchyRoot::fnSavePreset, _T("SavePreset"), 0, TYPE_BOOL, 0, 1,
		_T("filename"), 0, TYPE_TSTR,

	ICATHierarchyRoot::fnAddLayer, _T("AddLayer"), 0, TYPE_VOID, 0, 0,
	ICATHierarchyRoot::fnRemoveLayer, _T("RemoveLayer"), 0, TYPE_VOID, 0, 1,
		_T("index"), 0, TYPE_INT,

	ICATHierarchyRoot::fnCreateFootprints, _T("CreateFootPrints"), 0, TYPE_VOID, 0, 0,

	ICATHierarchyRoot::fnResetFootprints, _T("ResetFootPrints"), 0, TYPE_VOID, 0, 1,
		_T("bOnlySelected"), 0, TYPE_BOOL,

	ICATHierarchyRoot::fnRemoveFootprints, _T("RemoveFootPrints"), 0, TYPE_VOID, 0, 1,
		_T("bOnlySelected"), 0, TYPE_BOOL,

	ICATHierarchyRoot::fnSnapToGround, _T("SnapToGround"), 0, TYPE_BOOL, 0, 2,
		_T("grnd"), 0, TYPE_INODE,
		_T("bOnlySelected"), 0, TYPE_BOOL,
	ICATHierarchyRoot::fnUpdateDistCovered, _T("UpdateDistCovered"), 0, TYPE_VOID, 0, 0,
	ICATHierarchyRoot::fnUpdateSteps, _T("UpdateSteps"), 0, TYPE_VOID, 0, 0,

	properties,

	ICATHierarchyRoot::propGetFootStepMasks, ICATHierarchyRoot::propSetFootStepMasks, _T("FootStepMasks"), 0, TYPE_BOOL,
	ICATHierarchyRoot::propGetManualUpdates, ICATHierarchyRoot::propSetManualUpdates, _T("ManualUpdates"), 0, TYPE_BOOL,

	p_end
);

//  Get Descriptor method for Mixin Interface
//  *****************************************
FPInterfaceDesc* ICATHierarchyRoot::GetDesc()
{
	return &cathierarchyroot_FP_Interface;
}

void CATHierarchyRoot::GetClassName(TSTR& s) { s = GetString(IDS_CL_CATHIERARCHYROOT); }

static ParamBlockDesc2 cathierarchy_root_param_blk(CATHierarchyRoot::PBLOCK_REF, _T("params"), 0, &CATHierarchyRootDesc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, CATHierarchyRoot::CATBRANCHPB,
	IDD_CATROLLOUT, IDS_CL_CATHIERARCHYROOT, 0, 0, NULL,//&CATRootDataParamsCallback,

	CATHierarchyRoot::PB_BRANCHTAB, _T("Branches"), TYPE_FLOAT_TAB, 0, P_ANIMATABLE + P_VARIABLE_SIZE, IDS_LAYERNAME,
		p_end,
	CATHierarchyRoot::PB_BRANCHNAMESTAB, _T("BranchNames"), TYPE_STRING_TAB, 0, P_VARIABLE_SIZE, IDS_LAYERNAMES,
		p_end,
	CATHierarchyRoot::PB_EXPANDABLE, _T("Expanable"), TYPE_INT, 0, 0,
		p_end,
	CATHierarchyRoot::PB_BRANCHPARENT, _T("BranchParent"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	CATHierarchyRoot::PB_BRANCHINDEX, _T("BranchIndex"), TYPE_INT, 0, 0,
		p_end,
	CATHierarchyRoot::PB_BRANCHOWNER, _T("BranchOwner"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,

	CATHierarchyRoot::PB_WEIGHTS, _T("Weights"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.0f,
		p_end,
	CATHierarchyRoot::PB_CATPARENT, _T("CATParent"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	CATHierarchyRoot::PB_PATH_NODE, _T("PathNode"), TYPE_INODE, 0, IDS_PATH_NODE,
		p_ui, TYPE_PICKNODEBUTTON, IDC_BTN_PATH,
		p_prompt, GetString(IDS_PICKNODEPATH),
		p_end,
	CATHierarchyRoot::PB_GROUND_NODE, _T("GroundNode"), TYPE_INODE, 0, IDS_GROUND_NODE,
		p_ui, TYPE_PICKNODEBUTTON, IDC_BTN_GROUND,
		p_prompt, GetString(IDS_SNAPGROUND),
		p_end,
	// Almost obolete... almost.
	CATHierarchyRoot::PB_PATH_FACING, _T("PathFacingRatio"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 1.0f,
		p_end,
	CATHierarchyRoot::PB_MAX_STEP_LENGTH, _T("MaxStepLength"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 160.0f,
		p_end,
	CATHierarchyRoot::PB_MAX_STEP_TIME, _T("MaxStepTime"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 25.0f,		// Its 25 frames at 30 fps, or 30 frames at 25 fps
		p_end,
	CATHierarchyRoot::PB_CATMOTION_FLAGS, _T("CATMotionFlags"), TYPE_INT, P_RESET_DEFAULT, 0,
		p_default, 0,
		p_end,
	CATHierarchyRoot::PB_KA_LIFTING, _T("Retargetting"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.0f,
		p_end,
	CATHierarchyRoot::PB_ACTIVELEAF, _T("ActiveLeaf"), TYPE_INT, 0, 0,
		p_default, -1,
		p_end,
	CATHierarchyRoot::PB_START_TIME, _T("StartTime"), TYPE_FLOAT, 0, 0,
		p_default, 0.0f,
		p_end,
	CATHierarchyRoot::PB_END_TIME, _T("EndTime"), TYPE_FLOAT, 0, 0,
		p_default, 100.0f,		// Its 25 frames at 30 fps, or 30 frames at 25 fps
		p_end,

	// New CAT2 stuff
	CATHierarchyRoot::PB_STEP_EASE, _T("StepEase"), TYPE_FLOAT, P_ANIMATABLE, IDS_STEP_EASE,
		p_end,
	CATHierarchyRoot::PB_DISTCOVERED, _T("DistCovered"), TYPE_FLOAT, P_ANIMATABLE, IDS_PATH_DISTCOVERED,
		p_end,
	CATHierarchyRoot::PB_CATMOTIONHUB, _T("CATMotionHub"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	CATHierarchyRoot::PB_NLAINFO, _T("CATMotionHub"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,

	// Walk Mode Stuff
	CATHierarchyRoot::PB_DIRECTION, _T("Direction"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.0f,		// Its 25 frames at 30 fps, or 30 frames at 25 fps
		p_end,
	CATHierarchyRoot::PB_GRADIENT, _T("Gradient"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.0f,
		p_end,
	CATHierarchyRoot::PB_WALKMODE, _T("WalkMode"), TYPE_INT, 0, 0,
		p_default, 0,
		p_end,

	p_end
);

float CATHierarchyRoot::GetMaxStepDist(TimeValue t) {
	ICATParentTrans *catparenttrans = GetCATParentTrans();
	if (catparenttrans)
		return (pblock->GetFloat(PB_MAX_STEP_LENGTH, t) * catparenttrans->GetCATUnits());
	else return 0.0f;
}

RefTargetHandle CATHierarchyRoot::GetReference(int)
{
	return pblock;
}

void CATHierarchyRoot::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	CATHierarchyRootDesc.BeginEditParams(ip, this, flags, prev);
}

void CATHierarchyRoot::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	CATHierarchyRootDesc.EndEditParams(ip, this, flags, next);
}

int CATHierarchyRoot::AddLayer()
{
	// Macro-recorder support
	MACRO_DISABLE;

	// this index will be set in the notify leaves function
	int index = 0;
	pblock->EnableNotifications(FALSE);
	NotifyLeaves(CAT_LAYER_ADD, index);
	pblock->EnableNotifications(TRUE);
	NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED, NOTIFY_ALL, FALSE);

	pblock->EnableNotifications(FALSE);
	// this notification would cause the root
	// to try and update the rollout before
	// this function has returned
	GetWeights()->SetActiveLayer(index);
	pblock->EnableNotifications(TRUE);

	MACRO_ENABLE;

	// im not sure if we ever really need this to return a value
	return index;  // add 1 to be script-friendly.
}

void CATHierarchyRoot::InsertLayer(int index)
{
	pblock->EnableNotifications(FALSE);
	NotifyLeaves(CAT_LAYER_INSERT, index);
	pblock->EnableNotifications(TRUE);

	NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED, NOTIFY_ALL, FALSE);
}

/*
 *	Remove the currently selected layer, unless its BaseLayer,
 */
void CATHierarchyRoot::RemoveLayer(int index)
{
	// Macro-recorder support
	MACRO_DISABLE;

	// now we can remove the first layer
	// this means you can load a layer and remove
	// the first one so you only have 1
	if (index >= 0) {
		pblock->EnableNotifications(FALSE);
		if (GetWeights()->GetActiveLayer() >= GetWeights()->GetNumLayers())
			GetWeights()->SetActiveLayer(GetWeights()->GetNumLayers() - 2);
		NotifyLeaves(CAT_LAYER_REMOVE, index);

		pblock->EnableNotifications(TRUE);
		NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED, NOTIFY_ALL, FALSE);
	}

	pblock->EnableNotifications(FALSE);
	// this notification would cause the root
	// to try and update the rollout before
	// this function has returned
	GetWeights()->SetActiveLayer(index - 1);
	pblock->EnableNotifications(TRUE);

	if (theHold.Holding()) theHold.Accept(_T(""));
	GetCOREInterface()->FlushUndoBuffer();
	SetSaveRequiredFlag();

	MACRO_ENABLE;
}

/*
 *	The graphing branches are all the branches currently being displaye in the CATWindow
 */
void CATHierarchyRoot::AddGraphingBranch(CATHierarchyBranch* ctrlGraphingBranch)
{
	int newNumGraphs = tabGraphingBranches.Count() + 1;
	tabGraphingBranches.Resize(newNumGraphs);
	tabGraphingBranches.SetCount(newNumGraphs);
	tabGraphingBranches[newNumGraphs - 1] = (CATHierarchyBranch2*)ctrlGraphingBranch;
};

// Tell all our Controller to draw themselves
void CATHierarchyRoot::GetYRange(float &minY, float &maxY)
{
	TimeValue t = GetCOREInterface()->GetTime();
	minGraphY = 0.0f;
	maxGraphY = 0.0f;

	for (int i = 0; i < tabGraphingBranches.Count(); i++)
		tabGraphingBranches[i]->GetYRange(t, minGraphY, maxGraphY);

	// safety check so that we don't try to scale the graph infiinitely
	if (minGraphY == maxGraphY) {
		minGraphY -= 5.0f;
		maxGraphY += 5.0f;
	}
	minY = minGraphY;
	maxY = maxGraphY;
}

// Tell all our Controller to draw themselves
void CATHierarchyRoot::DrawGraph(CATHierarchyBranch* ctrlActiveBranch, int nSelectedKey,
	HDC hGraphDC, const RECT& rcGraphRect)
{
	// are we linked to a graphics window of some sorts?

	// GB 12-Jul-2003: replaced with real DC stored in class.
//	if(!iGraphBuff) return;
//	iGraphBuff->Erase();
//	HDC hGraphDC = iGraphBuff->GetDC();

	if (!hGraphDC) return;

	TimeValue t = GetCOREInterface()->GetTime();

	int nGraphWidth = (rcGraphRect.right - rcGraphRect.left);
	int nGraphHeight = (rcGraphRect.bottom - rcGraphRect.top);

	// Erase the graph.
	RECT rcMiddle;
	CopyRect(&rcMiddle, &rcGraphRect);

	HBRUSH hbrushBlue, hbrushCyan;
	hbrushBlue = CreateSolidBrush(RGB(3, 105, 40));
	hbrushCyan = CreateSolidBrush(RGB(85, 164, 81));

	if (ctrlActiveBranch->GetNumLimbs() == 0) {
		FillRect(hGraphDC, &rcGraphRect, hbrushBlue);
	}
	else {
		FillRect(hGraphDC, &rcGraphRect, hbrushCyan);
		rcMiddle.left += nGraphWidth / 4;
		rcMiddle.right -= nGraphWidth / 4;
		FillRect(hGraphDC, &rcMiddle, hbrushBlue);
	}
	DeleteObject(hbrushBlue);
	DeleteObject(hbrushCyan);

	float verticalMargins = (float)nGraphHeight / 10.0f;
	float heightMin = verticalMargins;
	float heightMax = (float)nGraphHeight - verticalMargins;
	float beta = (heightMax - heightMin) / (maxGraphY - minGraphY);
	float alpha = heightMin - beta * minGraphY;
	//	float Zero = heightMin - beta * minGraphY;

	//	float GraphYscale = (maxGraphY - minGraphY)/(dGraphHeight - (verticalMargins * 2));

	bool isActive;

	// get the min and max Y ranges so that we can
	// scale all data as we draw the graphs
	for (int i = 0; i < tabGraphingBranches.Count(); i++)
	{
		if (tabGraphingBranches[i]->GetUIType() == UI_GRAPH)
		{
			if (ctrlActiveBranch == tabGraphingBranches[i])
				isActive = true;
			else
				isActive = false;

			tabGraphingBranches[i]->DrawGraph(
				t,
				isActive,
				nSelectedKey,
				hGraphDC,
				nGraphWidth,
				nGraphHeight,
				alpha,
				beta);
		}
	}

	//	for(int i = 0; i < tabGraphingBranches.Count(); i++)
	//		tabGraphingBranches[i]->BranchDrawHandles(t);

	//	iGraphBuff->Blit();
	//	InvalidateRect(hGraphWnd, &rcGraphRect, FALSE);

}

BOOL CATHierarchyRoot::SavePreset(const TSTR& filename)
{
	CATPresetWriter save(filename.data());
	if (!save.ok()) {
		::MessageBox(GetCOREInterface()->GetMAXHWnd(), GetString(IDS_ERR_FILENOCREATE), GetString(IDS_ERR_FILECREATEERR), MB_OK);
		return FALSE;
	}

	// Insert a comment with a timestamp at the top of the file.  Chop
	// the '\n' off the end of the result of ctime().
	time_t ltime;
	time(&ltime);
	TCHAR *timestr = _tctime(&ltime);
	timestr[_tcslen(timestr) - 1] = _T('\0');
	save.Comment(GetString(IDS_EXPORTTIME), timestr);

	// Begin saving at root.
	TimeValue t = GetCOREInterface()->GetTime();
	//	SavePreset(&save, t);

		// Now save each branch
	for (int i = 0; i < GetNumBranches(); i++) {
		save.BeginBranch(GetBranchName(i));
		((CATHierarchyBranch2*)GetBranch(i))->SavePreset(&save, t);
		save.EndBranch();
	}

	return TRUE;
	//#endif
}

HWND CATHierarchyRoot::CreateCATWindow()
{
	if (!hCATWnd) {
		hCATWnd = ::CreateCATWindow(hInstance, GetCOREInterface()->GetMAXHWnd(), this);
	}
	else {
		ShowWindow(hCATWnd, SW_RESTORE);
		SetWindowPos(hCATWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	return hCATWnd;
}

/*
BOOL CATHierarchyRoot::SavePreset(CATPresetWriter *save, TimeValue t)
{
	// Now save each branch
	for (int i=0; i<nNumBranches; i++) {
		save->BeginBranch(tabBranchNames[i]);
		((CATHierarchyBranch2*)GetBranch()[i])->SavePreset(save, t);
		save->EndBranch();
	}
	return TRUE;
}
*/

/**********************************************************************
 * Read in frame data
 */
 /*
 static DWORD WINAPI dummyThreadFn(LPVOID arg) {

	 return 0;
 }
 */

BOOL CATHierarchyRoot::LoadPreset(const TSTR& filename, const TSTR& name, BOOL bNewLayer)
{
	if (!GetWeights()) {
		::MessageBox(GetCOREInterface()->GetMAXHWnd(), GetString(IDS_NOHIERARCHY), GetString(IDS_ERROR), MB_OK);
		return FALSE;
	}

	//	if (!theHold.Holding()) theHold.Begin();

	Interface *ip = GetCOREInterface();
	BOOL ok = TRUE;
	CATPresetReader load(filename.data());

	if (!load.ok()) {
		::MessageBox(GetCOREInterface()->GetMAXHWnd(), GetString(FILENOFND), GetString(IDS_ERROR), MB_OK);
		return FALSE;
	}

	load.ShowProgress(GetString(IDS_LOADPRESET));

	TimeValue t = ip->GetTime();

	// Add a new preset with the desired name.  The value
	// 'slot' is the sub-anim number to use.  We subtract
	// 1 because IAppendPreset() returns values intended
	// for MaxScript.  The bonus of using this function
	// is that it traverses the entire hierarchy and re-
	// sizes all the preset things so the new slot is
	// ready for our values.

	int slot = GetWeights()->GetActiveLayer();
	if (bNewLayer) {
		slot = AddLayer();
		GetWeights()->SetActiveLayer(GetWeights()->GetNumLayers() - 1);
		float on = 1.0f;
		GetWeights()->GetActiveLayerControl()->SetValue(t, (void*)&on);

		if (slot < 0) {
			MessageBox(ip->GetMAXHWnd(), GetString(IDS_PRESETNOCREATE), GetString(IDS_ERROR), MB_OK);
			return FALSE;
		}
	}
	else if (slot < 0 || slot >= GetWeights()->GetNumLayers()) {
		MessageBox(ip->GetMAXHWnd(), GetString(IDS_PRESETNOLOAD), GetString(IDS_ERROR), MB_OK);
		return FALSE;
	}

	pblock->EnableNotifications(FALSE);
	// Macro-recorder support
	MACRO_DISABLE;

	bool done = false;
	CATHierarchyBranch2 *branch;
	int iNumBranches = GetNumBranches();

	// All branches are initialized to not be loaded
	for (int i = 0; i < iNumBranches; i++)
		if (GetBranch(i)) GetBranch(i)->loadedPreset = FALSE;

	while (!done && load.ok() && ok) {
		ok = load.NextClause();

		switch (load.CurClauseID()) {
			// This is end of input.
		case motionEnd:
			done = true;
			continue;

			// This is a branch.  If the branch controller exists,
			// call its loader.  Otherwise spit out a warning and
			// skip the entire branch.
		case motionBeginBranch:
			branch = (CATHierarchyBranch2*)GetBranch(load.CurName());

			if (!branch)
			{
				// Start at 2, so to skip globals and limbphases.
				for (int i = 2; i < iNumBranches; i++)
				{
					branch = (CATHierarchyBranch2*)GetBranch(i);
					if (branch && branch->loadedPreset == FALSE) break;
					else branch = NULL;
				}
			}

			if (branch) {
				branch->LoadPreset(&load, t, slot);
				branch->loadedPreset = TRUE;
			}
			else {
				load.WarnNoSuchBranch();
				load.SkipBranch();
			}
			break;

			// This is end of branch.  We are done.  If this is the
			// root, there's no branch to end, but that will be
			// picked up by the parser, as it counts branch levels,
			// so we don't worry about it.  Yaay!
		case motionEndBranch:
			done = true;
			continue;

			// Whoah, we've aborted for some reason!
		case motionAbort:
			pblock->EnableNotifications(TRUE);
			MACRO_ENABLE;
			return FALSE;
		}
	}

	GetWeights()->SetLayerName(slot, name);

	if (theHold.Holding()) theHold.Accept(_T(""));
	ip->FlushUndoBuffer();
	SetSaveRequiredFlag();

	MACRO_ENABLE;
	pblock->EnableNotifications(TRUE);

	// ST - We must call this because the notifies that
	// normally trigger one are disabled during a load
	IUpdateSteps();

	return ok;
}

//	CATHierarchyRoot  Implementation.
//
//	Make it work
//
//	Steve T. 12 Nov 2002
void CATHierarchyRoot::Init()
{
	pblock = NULL;
	hCATWnd = NULL;
	pCATTreeView = NULL;

	nodeGround = NULL;

	currWindowPane = NULL;

	branchCopy = NULL;

	btnWalkOnSpot = NULL;
	btnPath = NULL;

	iGraphBuff = NULL;

	// Here, we assume that there are no updates running
	// simultaneously to our creation
//	theTimeOut.threadHandle = NULL;
//	InitializeCriticalSection(&csect);
//	isHolding = FALSE;
	bCanUpdate = FALSE;
	bUpdatingSteps = FALSE;
	bUpdatingDistCovered = FALSE;
}

//
// This function gets called when the presets list is
// changing.  We pass it on to our presets and our
// branches.
//
void CATHierarchyRoot::NotifyLeaves(UINT msg, int &data)//, TSTR name)
{
	GetWeights()->NotifyLeaf(msg, data);//, name);
	int i;
	for (i = 0; i < GetNumBranches(); i++)
	{
		if (GetBranch(i)) ((CATHierarchyBranch2*)GetBranch(i))->NotifyLeaves(msg, data);
	}
}

CATHierarchyBranch2 *CATHierarchyRoot::GetGlobalsBranch() {
	return (CATHierarchyBranch2*)GetBranch(GetString(IDS_GLOBALHIERARCHY));
}

CATHierarchyBranch2 *CATHierarchyRoot::GetLimbPhasesBranch() {
	return (CATHierarchyBranch2*)GetBranch(GetString(IDS_LIMBPHASES));
}

BOOL CATHierarchyRoot::LoadClip(CATRigReader *load, Interval range, int flags)
{
	ICATParentTrans *catparenttrans = GetCATParentTrans();
	if (!catparenttrans) return FALSE;

	BOOL done = FALSE;
	BOOL ok = TRUE;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idHubParams) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idSubNum:
			{
				int subNum;
				load->GetValue(subNum);

				load->NextClause();
				if (load->CurIdentifier() == idController)
				{
					if (subNum > NumSubs())
					{
						pblock->EnableNotifications(FALSE);
						subNum -= NumSubs();
						switch (subNum)
						{
						case PB_PATH_FACING:
						case PB_MAX_STEP_TIME:
						case PB_MAX_STEP_LENGTH:
						case PB_KA_LIFTING:
						{
							ParamID pid = (ParamID)subNum;
							Control* ctrl = pblock->GetControllerByID(pid);
							if (ctrl)
								load->ReadController(ctrl, range, 1.0f, flags);
							else
								load->SkipGroup();
							break;
						}
						default:
							load->AssertOutOfPlace();
							break;

						}
						pblock->EnableNotifications(TRUE);
					}
					else
					{
						load->AssertOutOfPlace();
						load->SkipGroup();
					}
				}
			}
			break;
			default:
				load->AssertOutOfPlace();
			}
			break;
		case rigAbort:
		case rigEnd:
			ok = FALSE;
		case rigEndGroup:
			done = TRUE;
			break;
		}
	}
	return ok && load->ok();
}

BOOL CATHierarchyRoot::SaveClip(CATRigWriter *save, int flags, Interval range)
{
	save->BeginGroup(idCATParams);

	int subNum;

	Control* leafPathFacing = pblock->GetControllerByID(PB_PATH_FACING);
	if (leafPathFacing)
	{
		subNum = NumSubs() + PB_PATH_FACING;
		save->Write(idSubNum, subNum);
		//	leafPathFacing->SaveClip(save, bClip, range);
		save->WriteController(leafPathFacing, flags, range);
	}

	Control* leafMaxStepTime = pblock->GetControllerByID(PB_MAX_STEP_TIME);
	if (leafMaxStepTime)
	{
		subNum = NumSubs() + PB_MAX_STEP_TIME;
		save->Write(idSubNum, subNum);
		//	layerSwivel->SaveClip(save, bClip, range);
		save->WriteController(leafMaxStepTime, flags, range);
	}

	Control *leafMaxStepLength = pblock->GetControllerByID(PB_MAX_STEP_LENGTH);
	if (leafMaxStepLength)
	{
		subNum = NumSubs() + PB_MAX_STEP_LENGTH;
		save->Write(idSubNum, subNum);
		save->WriteController(leafMaxStepLength, flags, range);
	}
	/*
		Control *leafKALifting = pblock->GetControllerByID(PB_KA_LIFTING);
		if(ctrlLegWeight)
		{
			subNum = NumSubs() + PB_KA_LIFTING;
			save->Write(idSubNum, subNum);
			save->WriteController(ctrlLegWeight, bClip, range);
		}
	*/
	save->EndGroup();
	return TRUE;
}

//
// Whether we're loading or not, we never start with any
// references so there's nothing to initialise.
//
CATHierarchyRoot::CATHierarchyRoot(BOOL loading)
	: CATHierarchyBranch(loading)
{
	Init();
	CATHierarchyRootDesc.MakeAutoParamBlocks(this);
}

ECATParent* CATHierarchyRoot::GetCATParent() {
	if (GetLayerInfo())
	{
		ICATParentTrans* pCATParenTrans = GetLayerInfo()->GetCATParentTrans();
		if (pCATParenTrans != NULL)
			return pCATParenTrans->GetCATParent();
	}
	return ((ECATParent*)pblock->GetReferenceTarget(PB_CATPARENT));
};

ICATParentTrans*	CATHierarchyRoot::GetCATParentTrans() {
	// phtaylor 10-5-07 : Blue GFX bug
	// This method gets called when upgrading old 1.4 files all the way to CAT 2.5
	// We really need to return a CATParentTrans at all costs.
	NLAInfo *layerinfo = GetLayerInfo();
	if (layerinfo) {
		ICATParentTrans* cpt = layerinfo->GetCATParentTrans();
		if (cpt)
			return cpt;
	}
	ECATParent* pECATParent = (ECATParent*)pblock->GetReferenceTarget(PB_CATPARENT);
	if (pECATParent)
		return pECATParent->GetCATParentTrans();
	return NULL;
};

void  CATHierarchyRoot::SetStartTime(float dStartTime) {
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
	//	if(theHold.Holding()) theHold.Put(new SetCCFlagRestore(this));

	pblock->SetValue(PB_START_TIME, 0, dStartTime);

	if (theHold.Holding() && newundo) { theHold.Accept(GetString(IDS_STARTTIMECH)); }
};

void  CATHierarchyRoot::SetEndTime(float dEndTime) {
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
	pblock->SetValue(PB_END_TIME, 0, dEndTime);
	if (theHold.Holding() && newundo) { theHold.Accept(GetString(IDS_ENDTIMECH)); }
};

void CATHierarchyRoot::SetisStepMasks(BOOL bStepMasks)
{
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	bStepMasks ? SetFlag(CATMOTIONFLAG_STEPMASKS) : ClearFlag(CATMOTIONFLAG_STEPMASKS);

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_STEPMASKCH));
		if (GetCATParentTrans())
			GetCATParentTrans()->UpdateCharacter();
	}
}

void  CATHierarchyRoot::SetWalkMode(WalkMode wm) {
	if (GetWalkMode() == wm)
		return;
	pblock->SetValue(PB_WALKMODE, 0, wm);
};

void CATHierarchyRoot::Initialise(Control* clipRoot, NLAInfo *layerinfo, int index)
{
	UNREFERENCED_PARAMETER(clipRoot);
	DisableRefMsgs();

	//	SetisWalkOnSpot(TRUE);
	SetWalkMode(WALK_ON_SPOT);
	//	SetisManualRanges(TRUE);
	SetisManualUpdates(TRUE);

	int tpf = GetTicksPerFrame();
	Interface* ip = GetCOREInterface();
	SetStartTime((float)(ip->GetAnimRange().Start() / tpf));
	SetEndTime((float)(ip->GetAnimRange().End() / tpf));

	Ease *ctrlDistCovered = (Ease*)CreateInstance(CTRL_FLOAT_CLASS_ID, EASE_CLASS_ID);
	DbgAssert(ctrlDistCovered);
	pblock->SetControllerByID(PB_DISTCOVERED, 0, (Control*)ctrlDistCovered, FALSE);

	Ease *ctrlStepEase = (Ease*)CreateInstance(CTRL_FLOAT_CLASS_ID, EASE_CLASS_ID);
	DbgAssert(ctrlStepEase);
	pblock->SetControllerByID(PB_STEP_EASE, 0, (Control*)ctrlStepEase, FALSE);

	CATHierarchyLeaf* weights = (CATHierarchyLeaf*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATHIERARCHYLEAF_CLASS_ID);
	weights->AddLayer(1);
	pblock->SetControllerByID(PB_WEIGHTS, 0, weights, FALSE);
	weights->SetDefaultVal(1.0f);

	pblock->SetValue(PB_NLAINFO, 0, layerinfo);

	CATHierarchyBranch2 *branchGlobals = (CATHierarchyBranch2*)AddBranch(GetString(IDS_GLOBALHIERARCHY)); //GetString(IDS_LIMBPHASES));
	branchGlobals->SetUIType(UI_FOOTSTEPS);

	float val;

	CATHierarchyLeaf* leafMaxStepLength = (CATHierarchyLeaf*)branchGlobals->AddLeaf(GetString(IDS_MAX_STEP_LENGTH));
	val = pblock->GetFloat(PB_MAX_STEP_LENGTH);
	leafMaxStepLength->SetDefaultVal(val);
	leafMaxStepLength->SetValue(0, (void*)&val, TRUE, CTRL_ABSOLUTE);
	pblock->SetControllerByID(PB_MAX_STEP_LENGTH, 0, leafMaxStepLength, FALSE);

	CATHierarchyLeaf* leafMaxStepTime = (CATHierarchyLeaf*)branchGlobals->AddLeaf(GetString(IDS_MAX_STEP_TIME));
	val = pblock->GetFloat(PB_MAX_STEP_TIME);
	leafMaxStepTime->SetDefaultVal(val);
	leafMaxStepTime->SetValue(0, (void*)&val, TRUE, CTRL_ABSOLUTE);
	pblock->SetControllerByID(PB_MAX_STEP_TIME, 0, leafMaxStepTime, FALSE);

	//////////////////////////////////////////////////
	// Walk Mode Stuff
	CATHierarchyLeaf* leafDirection = (CATHierarchyLeaf*)branchGlobals->AddLeaf(GetString(IDS_DIRECTION));
	val = pblock->GetFloat(PB_DIRECTION);
	leafDirection->SetDefaultVal(val);
	leafDirection->SetValue(0, (void*)&val, TRUE, CTRL_ABSOLUTE);
	pblock->SetControllerByID(PB_DIRECTION, 0, leafDirection, FALSE);

	CATHierarchyLeaf* leafGradient = (CATHierarchyLeaf*)branchGlobals->AddLeaf(GetString(IDS_GRADIENT));
	val = pblock->GetFloat(PB_GRADIENT);
	leafGradient->SetDefaultVal(val);
	leafGradient->SetValue(0, (void*)&val, TRUE, CTRL_ABSOLUTE);
	pblock->SetControllerByID(PB_GRADIENT, 0, leafGradient, FALSE);
	//////////////////////////////////////////////////

	CATHierarchyLeaf* leafKALifting = (CATHierarchyLeaf*)branchGlobals->AddLeaf(GetString(IDS_KABODYLIFTING));
	val = pblock->GetFloat(PB_KA_LIFTING);
	leafKALifting->SetDefaultVal(val);
	leafKALifting->SetValue(0, (void*)&val, TRUE, CTRL_ABSOLUTE);
	pblock->SetControllerByID(PB_KA_LIFTING, 0, leafKALifting, FALSE);
	// this puts the CATLeaf controller onto the first layer of the clip systems KA lifting controller
//	pClipRootInterface->SetKALiftingCtrl(0, leafKALifting);

	CATHierarchyLeaf* leafPathFacing = (CATHierarchyLeaf*)branchGlobals->AddLeaf(GetString(IDS_PATHFACING));
	val = pblock->GetFloat(PB_PATH_FACING);
	leafPathFacing->SetDefaultVal(val);
	leafPathFacing->SetValue(0, (void*)&val, TRUE, CTRL_ABSOLUTE);
	pblock->SetControllerByID(PB_PATH_FACING, 0, leafPathFacing, FALSE);

	CATHierarchyBranch2 *LimbPhases = (CATHierarchyBranch2*)AddBranch(GetString(IDS_LIMBPHASES));//_T("LimbPhases")); //
	LimbPhases->SetUIType(UI_LIMBPHASES);
	LimbPhases->SetisWelded(FALSE);

	EnableRefMsgs();

	// Here we kick off the building of the CATMotion layer
	ICATParentTrans* pCATParent = GetCATParentTrans();
	if (pCATParent != NULL)
	{
		Hub* pHub = static_cast<Hub*>(pCATParent->GetRootHub());
		if (pHub != NULL)
		{
			CATMotionHub2* catmotionhub = (CATMotionHub2*)CreateInstance(CTRL_MATRIX3_CLASS_ID, CATMOTIONHUB2_CLASS_ID);
			catmotionhub->Initialise(index, (Hub*)GetCATParentTrans()->GetRootHub(), this);
			pblock->SetValue(PB_CATMOTIONHUB, 0, catmotionhub);
		}
	}

	SetisManualUpdates(FALSE);

	// this will create a default stepease curve.
	IUpdateSteps();
}

// When deleting, set our daddy and weights pointers
// to NULL, just in case they for some stupid reason
// get used after we've deleted.
CATHierarchyRoot::~CATHierarchyRoot()
{
	if (hCATWnd) {
		DestroyWindow(hCATWnd);
	}

	DeleteAllRefs();
}

Animatable* CATHierarchyRoot::SubAnim(int i)
{
	if (i == 0)						return GetWeights();
	if ((i - 1) < GetNumBranches())	return (Animatable*)GetBranch(i - 1);
	if ((i - 1) == GetNumBranches())	return pblock->GetControllerByID(PB_STEP_EASE);
	return NULL;
}

TSTR CATHierarchyRoot::SubAnimName(int i)
{
	if (i == 0)						return GetString(IDS_WEIGHTS);
	if ((i - 1) < GetNumBranches())	return GetBranchName(i - 1);
	if ((i - 1) == GetNumBranches())	return GetString(IDS_TIMEWARP);
	return _T("");
}

/**********************************************************************
 * Published Functions....
 */
class SetCATMotionRestore : public RestoreObj {
public:
	CATHierarchyRoot	*root;
	int msg;
	SetCATMotionRestore(CATHierarchyRoot *r, int msg) {
		root = r;
		this->msg = msg;// the type of acion that registered this undo.
		switch (msg) {
		case CATMOTION_FOOTSTEPS_CHANGED:			break;
		case CATMOTION_FOOTSTEPS_CREATE:			break;
		case CATMOTION_FOOTSTEPS_REMOVE:			break;
		case CATMOTION_FOOTSTEPS_RESET:				break;
		case CATMOTION_FOOTSTEPS_SNAP_TO_GROUND:	break;
		case CATMOTION_PATHNODE_SET:				break;
		}
	}
	void Restore(int isUndo) {
		switch (msg) {
		case CATMOTION_FOOTSTEPS_CHANGED:			break;
		case CATMOTION_FOOTSTEPS_CREATE:			break;
		case CATMOTION_FOOTSTEPS_REMOVE:			break;
		case CATMOTION_FOOTSTEPS_RESET:				break;
		case CATMOTION_FOOTSTEPS_SNAP_TO_GROUND:	break;
		case CATMOTION_PATHNODE_SET:				break;
		}
		if (isUndo) {
			root->UpdateUI();
		}
	}
	void Redo() {
		switch (msg) {
		case CATMOTION_FOOTSTEPS_CHANGED:			break;
		case CATMOTION_FOOTSTEPS_CREATE:			break;
		case CATMOTION_FOOTSTEPS_REMOVE:			break;
		case CATMOTION_FOOTSTEPS_RESET:				break;
		case CATMOTION_FOOTSTEPS_SNAP_TO_GROUND:	break;
		case CATMOTION_PATHNODE_SET:				break;
		}
		root->UpdateUI();
	}
	int Size() { return 1; }
	void EndHold() {}
};

BOOL CATHierarchyRoot::ILoadPreset(const TSTR& file, BOOL bNewLayer)
{
	TSTR path, filename, extension;
	SplitFilename(file, &path, &filename, &extension);
	return LoadPreset(file, filename, bNewLayer);
};

BOOL CATHierarchyRoot::ISavePreset(const TSTR& filename)
{
	return SavePreset(filename);
};

BOOL CATHierarchyRoot::ISetPathNode(INode *pathnode)
{
	ICATParentTrans* catparenttrans = GetCATParentTrans();
	if (!catparenttrans) return FALSE;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); 	newundo = TRUE; }
	theHold.Put(new SetCATMotionRestore(this, CATMOTION_PATHNODE_SET));

	Ease *ctrlDistCovered = (Ease*)pblock->GetControllerByID(PB_DISTCOVERED);
	if (!ctrlDistCovered) {
		ctrlDistCovered = (Ease*)CreateInstance(CTRL_FLOAT_CLASS_ID, EASE_CLASS_ID);
		DbgAssert(ctrlDistCovered);
		pblock->SetControllerByID(PB_DISTCOVERED, 0, (Control*)ctrlDistCovered, FALSE);
	}
	Ease *ctrlStepEase = (Ease*)pblock->GetControllerByID(PB_STEP_EASE);
	if (!ctrlStepEase) {
		ctrlStepEase = (Ease*)CreateInstance(CTRL_FLOAT_CLASS_ID, EASE_CLASS_ID);
		DbgAssert(ctrlStepEase);
		pblock->SetControllerByID(PB_STEP_EASE, 0, (Control*)ctrlStepEase, FALSE);
	}

	pblock->EnableNotifications(FALSE);
	pblock->SetValue(PB_PATH_NODE, 0, pathnode);
	pblock->EnableNotifications(TRUE);

	if (GetPathNode()) {
		IUpdateDistCovered();
		if (catparenttrans->GetCATMode() != SETUPMODE) {
			SetWalkMode(WALK_ON_PATHNODE);
		}
	}

	if (newundo) theHold.Accept(GetString(IDS_SET_CMPATHNODE));

	UpdateUI();
	catparenttrans->UpdateCharacter();
	if (!GetCOREInterface()->IsAnimPlaying())
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());

	return TRUE;
};

void CATHierarchyRoot::ICreateFootPrints()
{
	ICATParentTrans* catparenttrans = GetCATParentTrans();
	if (!catparenttrans) return;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); 	newundo = TRUE; }
	if (theHold.Holding())
		theHold.Put(new SetCATMotionRestore(this, CATMOTION_FOOTSTEPS_CREATE));

	//	theHold.SuperBegin();
	CATMotionMessage(CATMOTION_FOOTSTEPS_CREATE);
	//	theHold.SuperAccept(_T("CreatePrints"));
	if (newundo) theHold.Accept(GetString(IDS_TT_CREATEFPRINTOBJS));
};

void CATHierarchyRoot::IResetFootPrints(BOOL bOnlySelected)
{
	ICATParentTrans* catparenttrans = GetCATParentTrans();
	if (!catparenttrans) return;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); 	newundo = TRUE; }
	if (theHold.Holding())
		theHold.Put(new SetCATMotionRestore(this, CATMOTION_FOOTSTEPS_RESET));

	CATMotionMessage(CATMOTION_FOOTSTEPS_RESET, bOnlySelected);

	if (newundo) theHold.Accept(GetString(IDS_RESET_CMFPRINTS));

	catparenttrans->UpdateCharacter();
	if (!GetCOREInterface()->IsAnimPlaying())
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
};

void CATHierarchyRoot::IRemoveFootPrints(BOOL bOnlySelected)
{
	ICATParentTrans* catparenttrans = GetCATParentTrans();
	if (!catparenttrans) return;

	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); 	newundo = TRUE; }
	if (theHold.Holding())
		theHold.Put(new SetCATMotionRestore(this, CATMOTION_FOOTSTEPS_REMOVE));

	CATMotionMessage(CATMOTION_FOOTSTEPS_REMOVE, bOnlySelected);

	if (newundo) theHold.Accept(GetString(IDS_TT_REMCMFPRINTS));

	catparenttrans->UpdateCharacter();
	if (!GetCOREInterface()->IsAnimPlaying())
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
};

BOOL CATHierarchyRoot::ISnapToGround(INode *ground, BOOL bOnlySelected)
{
	//	return FALSE;

	ICATParentTrans* catparenttrans = GetCATParentTrans();
	if (!catparenttrans) return FALSE;

	HoldActions hold(IDS_SNAP_CMFPRINTS);
	if (theHold.Holding())
		theHold.Put(new SetCATMotionRestore(this, CATMOTION_FOOTSTEPS_SNAP_TO_GROUND));

	SetGroundNode(ground);
	CATMotionMessage(CATMOTION_FOOTSTEPS_SNAP_TO_GROUND, bOnlySelected);

	GetCATParentTrans()->UpdateCharacter();
	if (!GetCOREInterface()->IsAnimPlaying())
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
	return TRUE;
};

void CATHierarchyRoot::UpdateUI() {
	if (currWindowPane) currWindowPane->TimeChanged(GetCOREInterface()->GetTime());
}

void CATHierarchyRoot::IUpdateDistCovered()
{
	theHold.Suspend();
	UpdateDistCovered();
	theHold.Resume();
};

void CATHierarchyRoot::IUpdateSteps()
{
	theHold.Suspend();
	UpdateSteps();
	theHold.Resume();
}

class PathNodeMoveRestore : public RestoreObj {
public:
	CATHierarchyRoot	*root;
	int footprintid;
	TimeValue t;
	PathNodeMoveRestore(CATHierarchyRoot *r) : t(0), footprintid(0) {
		root = r;
	}
	void Restore(int isUndo) {
		if (isUndo) {
			root->IUpdateDistCovered();
		}
	}
	void Redo() {
		root->IUpdateDistCovered();
	}
	void EndHold() {
		root->GetPathNode()->ClearAFlag(A_PLUGIN3);
		root->IUpdateDistCovered();
	}

	int Size() { return 1; }
};

RefResult CATHierarchyRoot::NotifyRefChanged(const Interval& iv, RefTargetHandle hTarg, PartID& partID, RefMessage msg, BOOL propagate)
{
	UNREFERENCED_PARAMETER(iv);
	UNREFERENCED_PARAMETER(partID);
	// ANY messages coming through here MUST be
	// held up while the timer thread is updating
	// footprints.
	switch (msg) {
	case REFMSG_CHANGE:
		// Check to see what has changed, and send appropriate message
		if (hTarg == pblock)
		{
			int isUndoing = 0;
			switch (pblock->LastNotifyParamID()) {
			case PB_START_TIME:
			case PB_END_TIME:
				// we don't want to re-calcuate footprints if the Undo system is doing a temp undo
				if (theHold.Restoring(isUndoing) && !isUndoing) {
					break;
				}
			case PB_CATMOTION_FLAGS:
				IUpdateDistCovered();
				break;
			case PB_PATH_NODE: {
				if (CATEvaluationLock::IsEvaluationLocked())
					break;

				// If the path node has been deleted, then
				// we need to switch back to Walk on the Spot mode
				if (pblock->GetINode(PB_PATH_NODE) == NULL)
				{
					if (GetWalkMode() == WALK_ON_PATHNODE)
						SetWalkMode(WALK_ON_SPOT);
				}

				// When you drag the path node around the screen in max, each time you move the mouse
				// max calls Restore on theHold to put things back to where you started the drag.
				// restore gets called many times, but with isUndo set to false because its not A
				// real undo, its a 'put back' undo. Notifications get sent for the 'put back' undo,
				// and for the movement of the path node due to your mouse. Here we only want to update
				// if we are doing a real undo, or a redo.
				// We also disable the undo system during an Update so that each footprint does not
				///register its own undo object.
				if (!GetisManualUpdates() && (!theHold.RestoreOrRedoing() || (theHold.Restoring(isUndoing) && isUndoing) || theHold.Redoing())) {
					// regisster an undo only if we are not already undoing
					if (GetPathNode() && !theHold.RestoreOrRedoing()) {
						if (!GetPathNode()->TestAFlag(A_PLUGIN3) && theHold.Holding()) {
							theHold.Put(new PathNodeMoveRestore(this));
							GetPathNode()->SetAFlag(A_PLUGIN3);
						}
					}
					else {
						IUpdateDistCovered();
					}
					UpdateUI();
				}

				break;
			}

			case PB_MAX_STEP_TIME:
			case PB_MAX_STEP_LENGTH:
			case PB_WEIGHTS: {
				if (CATEvaluationLock::IsEvaluationLocked())
					break;
				if (!GetisManualUpdates() && (!theHold.RestoreOrRedoing() || (theHold.Restoring(isUndoing) && isUndoing) || theHold.Redoing())) {
					IUpdateSteps();
				}
				if (GetWalkMode() == WALK_ON_LINE) {
					IUpdateDistCovered();
				}
				UpdateUI();
				break;
			}
			case PB_GROUND_NODE:
				CATMotionMessage(CATMOTION_FOOTSTEPS_SNAP_TO_GROUND);
				pblock->EnableNotifications(FALSE);
				pblock->SetValue(PB_GROUND_NODE, 0, (INode*)NULL);
				pblock->EnableNotifications(TRUE);
				break;
			case PB_DIRECTION:
			case PB_GRADIENT:
			case PB_WALKMODE:
				IUpdateDistCovered();
				break;
			}
		}
		break;
	case REFMSG_SUBANIM_STRUCTURE_CHANGED:
	{
		if (hTarg == pblock)
		{
			switch (pblock->LastNotifyParamID()) {
			case PB_GROUND_NODE:
			{
				GetCATParentTrans()->NotifyDependents(FOREVER, PART_ALL, CATMOTION_FOOTSTEPS_SNAP_TO_GROUND);
				pblock->SetValue(PB_GROUND_NODE, 0, (INode*)NULL);	// Otherwise, this message will only be sent once
//					LeaveCriticalSection(&csect);
				return REF_STOP;
			}
			break;
			case PB_PATH_NODE:
				if (!pblock->GetINode(PB_PATH_NODE)) {
					//	SetisWalkOnSpot(TRUE);
					SetWalkMode(WALK_ON_SPOT);
				}
				break;
			}
		}
	}
	break;
	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		// to the deleting object.
	//	if(layerTrans == hTarg)				layerTrans = NULL;
		break;
	case REFMSG_FLAGDEPENDENTS:
		// We don't want to pass on change messages up the CAT
		// hierarchy.  So stop them right here.
		return REF_STOP;
	}
	return REF_SUCCEED;
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
class CATHierarchyRootPLCB : public PostLoadCallback {
protected:
	CATHierarchyRoot *root;

public:
	CATHierarchyRootPLCB(CATHierarchyRoot *pOwner) { root = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(root);
		ICATParentTrans *catparenttrans = root->GetCATParentTrans();
		if (catparenttrans) return catparenttrans->GetFileSaveVersion();
		ECATParent *catparent = root->GetCATParent();
		if (catparent) return catparent->GetFileSaveVersion();
		return 0;
	}

	int Priority() { return 5; }

	void proc(ILoad *) {

		//			if (GetFileSaveVersion() < CAT_VERSION_1150)
		//			{
		//				if(root->GetisWalkOnSpot())
		//				{
		//					float maxstepdist = root->GetMaxStepDist(0)/2.0f;
		//					root->pblock->SetValue(CATHierarchyRoot::PB_MAX_STEP_LENGTH, 0, maxstepdist);
		//					iload->SetObsolete();
		//				}
		//			}

		/*			if (GetFileSaveVersion() < CAT_VERSION_1301)
					{
						// Manual Ranges should be turned on at all times from now on
						if(!root->TestFlag(CATMOTIONFLAG_MANUALRANGES)){
							root->SetisManualRanges(TRUE);
							Interval animationrange = GetCOREInterface()->GetAnimRange();
							root->SetStartTime((animationrange.Start()/GetTicksPerFrame())+50);
							root->SetEndTime((animationrange.End()/GetTicksPerFrame())-50);
						}
					}
		*/

		if (GetFileSaveVersion() < CAT_VERSION_2438)
		{
			CATHierarchyBranch2 *pGlobalsBranch = root->GetGlobalsBranch();
			DisableRefMsgs();
			if (pGlobalsBranch)
			{
				// Here we are adding the new Branches for Direction and Gradient controls for CATMotion
				float val = 0.0f;
				CATHierarchyLeaf* leafDirection = pGlobalsBranch->AddLeaf(GetString(IDS_DIRECTION));
				val = root->pblock->GetFloat(CATHierarchyRoot::PB_DIRECTION);
				leafDirection->SetDefaultVal(val);
				leafDirection->SetValue(0, (void*)&val, TRUE, CTRL_ABSOLUTE);
				root->pblock->SetControllerByID(CATHierarchyRoot::PB_DIRECTION, 0, leafDirection, FALSE);

				CATHierarchyLeaf* leafGradient = pGlobalsBranch->AddLeaf(GetString(IDS_GRADIENT));
				val = root->pblock->GetFloat(CATHierarchyRoot::PB_GRADIENT);
				leafGradient->SetDefaultVal(val);
				leafGradient->SetValue(0, (void*)&val, TRUE, CTRL_ABSOLUTE);
				root->pblock->SetControllerByID(CATHierarchyRoot::PB_GRADIENT, 0, leafGradient, FALSE);

			}
			if (root->TestFlag(CATMOTIONFLAG_WALKONSPOT)) {
				root->SetWalkMode(CATHierarchyRoot::WALK_ON_SPOT);
				root->ClearFlag(CATMOTIONFLAG_WALKONSPOT);
			}
			else root->SetWalkMode(CATHierarchyRoot::WALK_ON_PATHNODE);

			EnableRefMsgs();
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

//////////////////////////////////////////////////////////////////////////
// Loading and saving....
//
DWORD CATHierarchyRoot::GetFileSaveVersion() {
	ICATParentTrans *catparenttrans = GetCATParentTrans();
	if (catparenttrans) return catparenttrans->GetFileSaveVersion();
	ECATParent *catparent = GetCATParent();
	if (catparent) return catparent->GetFileSaveVersion();
	return 0;
}

IOResult CATHierarchyRoot::Save(ISave *)
{
	return IO_OK;
}

IOResult CATHierarchyRoot::Load(ILoad *iload)
{
	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new CATHierarchyRootPLCB(this));

	return IO_OK;
}

//*********************************************************************
// UpdateSteps() recalculates the position of footsteps
//

void CATHierarchyRoot::UpdateDistCovered()
{
	if (!pblock || bUpdatingDistCovered || (!GetLayerInfo())) return;
	ICATParentTrans* catparenttrans = GetCATParentTrans();
	if (!catparenttrans) return;

	bUpdatingDistCovered = TRUE;
	DisableRefMsgs();

	Ease *ctrlDistCovered = (Ease*)pblock->GetControllerByID(PB_DISTCOVERED);
	if (!ctrlDistCovered) {
		ctrlDistCovered = (Ease*)CreateInstance(CTRL_FLOAT_CLASS_ID, EASE_CLASS_ID);
		DbgAssert(ctrlDistCovered);
		pblock->SetControllerByID(PB_DISTCOVERED, 0, (Control*)ctrlDistCovered, FALSE);
	}
	ctrlDistCovered->SetNumKeys(0);

	Interval iCATMotion = GetCATMotionRange();
	INode *pathnode = GetPathNode();
	float totDist = 0.0f;
	Point3 p3LastPos, p3ThisPos;
	Matrix3 tmPathNode;
	int tpf = GetTicksPerFrame();

	if (pathnode)tmPathNode = pathnode->GetNodeTM(iCATMotion.Start());
	else		tmPathNode = catparenttrans->GettmCATParent(iCATMotion.Start());
	p3LastPos = tmPathNode.GetTrans();

	// Whizz through the animation and sample the path node each frame      *
	// adding up distances as we go.                                        *
	for (int t = (iCATMotion.Start() + (tpf * 2)); t < iCATMotion.End(); t += (tpf * 2)) {
		if (pathnode && GetWalkMode() == WALK_ON_PATHNODE) {
			tmPathNode = pathnode->GetNodeTM(t);
		}
		else {
			tmPathNode.PreTranslate(Point3(0.0f, (GetMaxStepDist(t) / (float)GetMaxStepTime(t)) * (float)(tpf * 2), 0.0f));
		}
		p3ThisPos = tmPathNode.GetTrans();
		if (p3LastPos != p3ThisPos)
			totDist += Length(p3LastPos - p3ThisPos);
		ctrlDistCovered->SetValue(t, (void*)&totDist);
		p3LastPos = p3ThisPos;
	}
	EnableRefMsgs();
	bUpdatingDistCovered = FALSE;

	// if we have just changed the distcovered graph
	// we definitely need to update steps as well
	UpdateSteps();
}

void CATHierarchyRoot::UpdateSteps()
{
	if (!pblock || bUpdatingSteps || (!GetLayerInfo())) return;

	ICATParentTrans* catparenttrans = GetCATParentTrans();
	// in some cases, like undoing a CATMotionLayer Add, we have no CATParent
	if (!catparenttrans) return;

	bUpdatingSteps = TRUE;
	pblock->EnableNotifications(FALSE);
	//	DisableRefMsgs();

	Interval iCATMotion = GetCATMotionRange();

	int tpf = GetTicksPerFrame();
	int t = iCATMotion.Start();
	int maxStepTime, maxStepTimeThisStep, maxStepTimeNextStep;

	Ease *ctrlStepEase = (Ease*)pblock->GetControllerByID(PB_STEP_EASE);
	if (!ctrlStepEase)
	{
		ctrlStepEase = (Ease*)CreateInstance(CTRL_FLOAT_CLASS_ID, EASE_CLASS_ID);
		DbgAssert(ctrlStepEase);
		pblock->SetControllerByID(PB_STEP_EASE, 0, (Control*)ctrlStepEase, FALSE);
	}
	ctrlStepEase->SetNumKeys(0);

	if (GetWalkMode() != WALK_ON_PATHNODE)
	{
		//	The walk on spot ease graph is a simple linear graph,
		//	where it simply takes maxStepTime at time 0 and uses
		//	it to create a linear graph with gradient maxTime over
		//	BASE60. The two keys give entire graph with one gradient

		SuspendAnimate();
		AnimateOff();

		float dStepEaseVal;
		maxStepTime = GetMaxStepTime(t);
		int numSteps = 1;

		//	ST - Originally running on the spot was 'non animatable', ie, you had
		//	one run loop and that was that. Dont expect anyone to actually
		//	use this, but now the running accurately represents the iCATMotion
		//	at current time (not time 0, as before)

		while (t <= (iCATMotion.End() + maxStepTime))
		{
			maxStepTimeThisStep = GetMaxStepTime(t);
			maxStepTimeNextStep = GetMaxStepTime(t + maxStepTimeThisStep);
			maxStepTime = (maxStepTimeThisStep + maxStepTimeNextStep) / 2;

			if (!(maxStepTime > 0)) break;
			t += maxStepTime;

			dStepEaseVal = (float)(numSteps * STEPTIME100);
			ctrlStepEase->SetValue(t, (void*)&dStepEaseVal);

			numSteps++;
		}
		ResumeAnimate();
	}
	else
	{
		Ease *ctrlDistCovered = (Ease*)pblock->GetControllerByID(PB_DISTCOVERED);
		DbgAssert(ctrlDistCovered);

		Interval ivPathMoving = ctrlDistCovered->GetExtents();
		float distance = 0.0f;
		int stepNumber = 0;
		float stepValue = 0.0f;
		float maxStepLength;//, maxStepLengthThisStep, maxStepLengthNextStep;
		float nextStepDist;
		int nextStepTime, maxStepTime, nextDistTime = t;

		SuspendAnimate();
		AnimateOff();

		ctrlStepEase->SetValue(t, (void*)&stepValue);
		if (ivPathMoving.InInterval(t))
			distance = ctrlDistCovered->GetValue(t);

		while (t <= iCATMotion.End())	// plot the stepgraph
		{
			// here we try to find the average step time and step length based on 3 samples
			maxStepTimeThisStep = GetMaxStepTime(t);
			maxStepTimeNextStep = GetMaxStepTime(t + maxStepTimeThisStep);
			maxStepTime = (maxStepTimeThisStep + maxStepTimeNextStep) / 2;

			maxStepTime = (maxStepTime +
				GetMaxStepTime((t + (maxStepTime / 2))) +
				GetMaxStepTime(t + maxStepTime)
				) / 3;

			float maxStepLengthThisStep = GetMaxStepDist(t);
			float maxStepLengthNextStep = GetMaxStepDist(ctrlDistCovered->GetTime(distance + maxStepLengthThisStep));
			maxStepLength = (maxStepLengthThisStep + maxStepLengthNextStep) / 2.0f;

			maxStepLength = (maxStepLength +
				GetMaxStepDist(ctrlDistCovered->GetTime(distance + (maxStepLength / 2.0f))) +
				GetMaxStepDist(ctrlDistCovered->GetTime(distance + maxStepLength))
				) / 3.0f;

			//************************************************************************
			// All new even faster way to calculate steps. no guessing, no searching *
			//************************************************************************
			stepNumber++;

			nextStepDist = distance + maxStepLength;
			nextStepTime = t + maxStepTime;
			nextDistTime = ctrlDistCovered->GetTime(nextStepDist);

			// If we are out of range of the path, simply take the
			// maximum timed footsteps  possible
			if (t > ivPathMoving.End())
				nextDistTime = nextStepTime;

			if (nextStepTime > nextDistTime) {
				t = max(nextDistTime, t + tpf);	// Our smallest step must be at least one frame, this
				distance = nextStepDist;		// prevents an infinate loop when we are walking o
			}
			else {
				t = nextStepTime;
				distance = ctrlDistCovered->GetValue(max(min(nextStepTime, ivPathMoving.End()), ivPathMoving.Start()));
				//				distance = ctrlDistCovered->GetValue(nextStepTime);
			}
			stepValue = (float)(stepNumber * STEPTIME100);
			ctrlStepEase->SetValue(t, (void*)&stepValue);

		}

		ResumeAnimate();
	}

	pblock->EnableNotifications(TRUE);

	// This message is expressly for the feet, telling them to recalculate their footsteps
	CATMotionMessage(CATMOTION_FOOTSTEPS_CHANGED);

	bUpdatingSteps = FALSE;

	catparenttrans->UpdateCharacter();
}

void CATHierarchyRoot::GettmPath(TimeValue t, Matrix3 &val, Interval &valid, float pathOffset, BOOL isFoot)
{
	ICATParentTrans* catparenttrans = GetCATParentTrans();
	if (!catparenttrans) return;
	val = catparenttrans->GettmCATParent(t);
	int lengthaxis = catparenttrans->GetLengthAxis();

	float fwRatio = GetFWRatio(t);
	float direction = -DegToRad(GetDirection(t));
	float gradient = DegToRad(GetGradient(t));
	Point3 offsetvec(P3_IDENTITY);

	switch (GetWalkMode()) {
	case WALK_ON_LINE: {
		float disttraveled = ((Ease*)GetDistCovered())->GetValue(t);
		if (lengthaxis == X)
			offsetvec.Set(sin(direction) * disttraveled, cos(direction) * disttraveled * cos(gradient), disttraveled*sin(gradient));
		else offsetvec.Set(-sin(direction) * disttraveled, cos(direction) * disttraveled * cos(gradient), disttraveled*sin(gradient));
		ModVec(offsetvec, lengthaxis);
		val.PreTranslate(offsetvec);
	}
	case WALK_ON_SPOT:
	{
		offsetvec.Set(0.0f, pathOffset * cos(gradient), pathOffset * sin(gradient) * cos(direction));
		ModVec(offsetvec, lengthaxis);
		val.PreTranslate(offsetvec);

		if (isFoot || fwRatio > 0.0f) {
			AngAxis ax;
			if (lengthaxis == X) {
				ax.angle = gradient * (isFoot ? 1.0f : fwRatio);
				ax.axis = Point3(cos(-direction), sin(-direction), 0.0f);
				ModVec(ax.axis, lengthaxis);
			}
			else {
				ax.angle = -gradient * (isFoot ? 1.0f : fwRatio);
				ax.axis = Point3(cos(-direction), sin(direction), 0.0f);
				ModVec(ax.axis, lengthaxis);
			}

			Matrix3 tm;
			Quat(ax).MakeMatrix(tm);
			val = tm * val;
		}
		return;
	}
	case WALK_ON_PATHNODE:
	{
		Matrix3 result(1);
		INode *pathnode = GetPathNode();
		if (!pathnode) return;

		//***********************************************************************
		// Our Path node is not a shape object. so we will use the brand        *
		// spanking new control method that doesn't use a					    *
		// path at all. YAY                                                     *
		//***********************************************************************
		Ease *ctrlDistCovered = (Ease*)GetDistCovered();
		if (!ctrlDistCovered) return;

		// At fwRatio 1, the whole charachter faces the
		// same way as the node, and offsets are directly forward
		float fwDist;

		Interval pathRange = ctrlDistCovered->GetExtents();
		if (!pathRange.InInterval(t))
		{
			if (t < pathRange.Start())
			{
				if (pathOffset < 0) fwRatio = 1.0f;
				t = pathRange.Start();
			}
			else
			{
				if (pathOffset > 0) fwRatio = 1.0f;
				t = pathRange.End();
			}
		}

		// For turning on the spot! In these cases
		// the offsetDist will take the time out of
		// the section of time where the node is turning,
		// to the point where the node is again moving.
		// Even with fwRatio at 1.
		if (fwRatio < 1.0f)
		{
			float dCharDist, BodyPartDist;
			dCharDist = ctrlDistCovered->GetValue(t);
			BodyPartDist = dCharDist + (pathOffset * (1.0f - fwRatio));
			t = ctrlDistCovered->GetTime(BodyPartDist);
			fwDist = pathOffset*fwRatio;

			// When the character stops moveing, then the offsets end
			// up getting all cancelled out, and the Charachter loses
			// forward positions, so this little hack is to detect that and stop it
			// by detecting when we are passed our movement buffer, and

			// Old method caused a jump in objects when the path ended on a curve.
			// Changing it so the time taken back to is always the final time path movement

			// if fwRatio is 1, newT is locked to the actual pathTimes

			if (!pathRange.InInterval(t))
			{
				// ST 28/10 This is the right idea, but it dosent
				// handle the situation where the t is in range,
				// but then is path offset'ed beyond the end of the path
	//			newT = t;

				if (t < pathRange.Start())
				{
					t = pathRange.Start();
					fwDist = BodyPartDist;
				}
				else
				{
					t = pathRange.End();
					fwDist = BodyPartDist - ctrlDistCovered->GetValue(t);
				}
				//			fwDist = pathOffset;
				//			fwDist += (BodyPartDist - dCharDist);
				//			fwDist = max(pathOffset, fwDist);
			}
		}
		else
			fwDist = pathOffset;

		// this is to avoid a bug in Max where by if an object is hidden,
		// then the GetObjTMAfterWSM stops getting evaluated. annoying!
		if (pathnode->IsHidden()) pathnode->EvalWorldState(t, TRUE);

		val = pathnode->GetObjTMAfterWSM(t);
		val.PreTranslate(Point3(0.0f, fwDist, 0.0f));
		return;
	}
	default:
		valid.SetInstant(t);
		val.IdentityMatrix();
	}
}

Control* CATHierarchyRoot::GetStepEaseGraph() const {
	return pblock->GetControllerByID(PB_STEP_EASE);
};

float CATHierarchyRoot::GetStepEaseValue(TimeValue t) const {
	Ease *stepEaseGraph = (Ease*)pblock->GetControllerByID(PB_STEP_EASE);
	if (stepEaseGraph) return stepEaseGraph->GetValue(t);
	return 0.0f;
}

Interval CATHierarchyRoot::GetStepEaseExtents()
{
	Ease *stepEaseGraph = (Ease*)pblock->GetControllerByID(PB_STEP_EASE);
	if (stepEaseGraph)
		return stepEaseGraph->GetExtents();
	return NEVER;
}

void CATHierarchyRoot::CATMotionMessage(UINT msg, int data/*=0*/)
{
	Control *catmotionhub = (Control*)pblock->GetReferenceTarget(PB_CATMOTIONHUB);
	if (!catmotionhub) return;
	if (catmotionhub->ClassID() == GetCATMotionHub2Desc()->ClassID())
		((CATMotionHub2*)catmotionhub)->CATMotionMessage(GetCOREInterface()->GetTime(), msg, data);
};

void CATHierarchyRoot::AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	Control *catmotionhub = (Control*)pblock->GetReferenceTarget(PB_CATMOTIONHUB);
	if (!catmotionhub) return;
	if (catmotionhub->ClassID() == GetCATMotionHub2Desc()->ClassID())
		((CATMotionHub2*)catmotionhub)->AddSystemNodes(nodes, ctxt);
}

int CATHierarchyRoot::GetLayerIndex()
{
	NLAInfo* layerinfo = GetLayerInfo();
	if (layerinfo) return layerinfo->GetIndex();
	return -1;
}

class RootAndRemap
{
public:
	CATHierarchyRoot* root;
	CATHierarchyRoot* clonedroot;
	RemapDir* remap;

	RootAndRemap(CATHierarchyRoot* root, CATHierarchyRoot* clonedroot, RemapDir* remap) {
		this->root = root;
		this->clonedroot = clonedroot;
		this->remap = remap;
	}
};

void CATHierarchyRootCloneNotify(void *param, NotifyInfo *)
{
	RootAndRemap *rootAndremap = (RootAndRemap*)param;

	Control *hub = (Control*)rootAndremap->root->pblock->GetReferenceTarget(CATHierarchyRoot::PB_CATMOTIONHUB);
	Control *clonedhub = (Control*)rootAndremap->clonedroot->pblock->GetReferenceTarget(CATHierarchyRoot::PB_CATMOTIONHUB);
	if (hub) {
		Control *newhub = (Control*)rootAndremap->remap->FindMapping(hub);
		if (newhub && (newhub != clonedhub)) {
			rootAndremap->clonedroot->pblock->SetValue(CATHierarchyRoot::PB_CATMOTIONHUB, 0, (ReferenceTarget*)newhub);
		}
	}

	UnRegisterNotification(CATHierarchyRootCloneNotify, rootAndremap, NOTIFY_NODE_CLONED);

	delete rootAndremap;
}

RefTargetHandle CATHierarchyRoot::Clone(RemapDir& remap) {
	CATHierarchyRoot *newCATHierarchyRoot = new CATHierarchyRoot(TRUE);
	remap.AddEntry(this, newCATHierarchyRoot);

	Control *leafWeights = (Control*)remap.CloneRef(pblock->GetControllerByID(PB_WEIGHTS));
	Control *leafPathFacing = (Control*)remap.CloneRef(pblock->GetControllerByID(PB_PATH_FACING));
	Control *leafMaxStepTime = (Control*)remap.CloneRef(pblock->GetControllerByID(PB_MAX_STEP_LENGTH));
	Control *leafMaxStepDist = (Control*)remap.CloneRef(pblock->GetControllerByID(PB_MAX_STEP_TIME));
	Control *leafKALifting = (Control*)remap.CloneRef(pblock->GetControllerByID(PB_KA_LIFTING));
	Control *leafDirection = (Control*)remap.CloneRef(pblock->GetControllerByID(PB_DIRECTION));
	Control *leafGradient = (Control*)remap.CloneRef(pblock->GetControllerByID(PB_GRADIENT));

	newCATHierarchyRoot->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	newCATHierarchyRoot->pblock->SetControllerByID(PB_WEIGHTS, 0, leafWeights, FALSE);
	newCATHierarchyRoot->pblock->SetControllerByID(PB_PATH_FACING, 0, leafPathFacing, FALSE);
	newCATHierarchyRoot->pblock->SetControllerByID(PB_MAX_STEP_LENGTH, 0, leafMaxStepTime, FALSE);
	newCATHierarchyRoot->pblock->SetControllerByID(PB_MAX_STEP_TIME, 0, leafMaxStepDist, FALSE);
	newCATHierarchyRoot->pblock->SetControllerByID(PB_KA_LIFTING, 0, leafKALifting, FALSE);
	newCATHierarchyRoot->pblock->SetControllerByID(PB_DIRECTION, 0, leafDirection, FALSE);
	newCATHierarchyRoot->pblock->SetControllerByID(PB_GRADIENT, 0, leafGradient, FALSE);

	RegisterNotification(CATHierarchyRootCloneNotify, new RootAndRemap(this, newCATHierarchyRoot, &remap), NOTIFY_NODE_CLONED);

	BaseClone(this, newCATHierarchyRoot, remap);
	return newCATHierarchyRoot;
};

/*
void CATHierarchyRoot::PasteLayer(CATHierarchyRoot* fromctrl, DWORD flags, RemapDir &remap)
{

	pblock->EnableNotifications(FALSE);
	pblock->SetValue(PB_PATH_NODE, 0, pblock->GetINode(PB_PATH_NODE));
	pblock->EnableNotifications(TRUE);

	Ease *ctrlDistCovered = (Ease*)pblock->GetControllerByID(PB_DISTCOVERED);
	if(ctrlDistCovered){

	}
}
*/
