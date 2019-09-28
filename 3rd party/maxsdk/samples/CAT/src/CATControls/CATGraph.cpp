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

#include "CATPlugins.h"
#include "BezierInterp.h"

#include "LimbData2.h"
#include "CATMotionLimb.h"
#include "CATHierarchyBranch2.h"
#include "CATHierarchyLeaf.h"

#include "CATGraph.h"
#include "Ease.h"

CATGraph::CATGraph()
	: catmotionlimb(NULL)
	, pblock(NULL)
	, dim(DEFAULT_DIM)
	, flipval(1.0f)
{

}

CATGraph::~CATGraph()
{
	DeleteAllRefs();
}

RefTargetHandle CATGraph::GetReference(int i)
{
	switch (i)
	{
	case PBLOCK_REF:		return pblock;
	case REF_CATMOTIONLIMB: return catmotionlimb;
	default:
		return NULL;
	}
}

void CATGraph::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case PBLOCK_REF:			pblock = (IParamBlock2*)rtarg;					break;
	case REF_CATMOTIONLIMB:		catmotionlimb = (CATMotionLimb*)rtarg;			break;
	default:					DbgAssert(!_M("BadIndex in CATGraph::SetReference"));	break;
	}
}

CATMotionLimb* CATGraph::GetCATMotionLimb() {
	return catmotionlimb;
};

void CATGraph::SetCATMotionLimb(Control* newLimb) {
	ReplaceReference(REF_CATMOTIONLIMB, newLimb);
};

/*
CATMotionLimb* CATGraph::GetLimb()
{
	return (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
};

void CATGraph::SetLimb(Control* newLimb)
{
	pblock->SetValue(PB_LIMBDATA, 0, newLimb);
};
*/

CATHierarchyBranch2* CATGraph::GetBranch()
{
	return (CATHierarchyBranch2*)pblock->GetReferenceTarget(PB_CATBRANCH);
};
void CATGraph::SetBranch(CATHierarchyBranch* newBranch)
{
	// it is possible for this function to be called
	// mid-destruction
	if (pblock != NULL && GetBranch() != newBranch)
		pblock->SetValue(PB_CATBRANCH, 0, newBranch);
};

COLORREF CATGraph::GetGraphColour()
{
	if (GetCATMotionLimb())
		return asRGB(GetCATMotionLimb()->GetLimbColour());
	return asRGB(Color(1.0f, 0.0f, 0.0f));// t
};
TSTR CATGraph::GetGraphName()
{
	if (GetCATMotionLimb())
		return GetCATMotionLimb()->GetLimbName();
	else return _T("Buuuuuug");
};

void CATGraph::CloneCATGraph(CATGraph* catgraph, RemapDir &remap)
{
	remap.AddEntry(this, catgraph);
	catgraph->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));
	catgraph->ReplaceReference(REF_CATMOTIONLIMB, remap.CloneRef(catmotionlimb));
	catgraph->dim = dim;
	catgraph->flipval = flipval;

}

BOOL CATGraph::AssignController(Animatable *control, int subAnim)
{
	DbgAssert(pblock);
	if (pblock == NULL)
		return FALSE;

	ParamID pid = pblock->IndextoID(subAnim);
	if ((subAnim < pblock->NumParams()) &&
		(pblock->GetParameterType(pid) == TYPE_FLOAT) &&
		(control->ClassID() == CATHIERARCHYLEAF_CLASS_ID))
	{
		// Stephen, is this correct???
		pblock->SetControllerByIndex(subAnim, 0, (Control*)control, FALSE);
		InitControls();
		return TRUE;
	}
	else
		return FALSE;
}

void CATGraph::PasteGraph(CATHierarchyBranch2* branchCopy)
{
	CATHierarchyBranch2* branchThis = GetBranch();
	int nNumKeys = GetNumGraphKeys();
	if (nNumKeys != branchCopy->GetNumGraphKeys()) return;

	TimeValue t = GetCOREInterface()->GetTime();

	float fTimeVal = 50.0f;
	float fPrevKeyTime = 0.0f;	float fNextKeyTime = 100.0f;
	float minVal = -1000.0f;	float maxVal = 1000.0f;
	float fValueVal = 0.0f, fTangentVal = 0.0f, fInTanLenVal = 0.333f, fOutTanLenVal = 0.333f;

	Control *ctrlCopyTime = NULL;
	Control *ctrlCopyValue = NULL;
	Control *ctrlCopyTangent = NULL;
	Control *ctrlCopyInTanLen = NULL;
	Control *ctrlCopyOutTanLen = NULL;
	Control *ctrlPasteTime = NULL;
	Control *ctrlPasteValue = NULL;
	Control *ctrlPasteTangent = NULL;
	Control *ctrlPasteInTanLen = NULL;
	Control *ctrlPasteOutTanLen = NULL;

	Control *ctrlSlider = NULL;

	Interval iv;
	for (int i = 0; i < nNumKeys; i++)
	{
		branchCopy->GetGraphKey(i,
			&ctrlCopyTime, fTimeVal, fPrevKeyTime, fNextKeyTime,
			&ctrlCopyValue, fValueVal, minVal, maxVal,
			&ctrlCopyTangent, fTangentVal,
			&ctrlCopyInTanLen, fInTanLenVal,
			&ctrlCopyOutTanLen, fOutTanLenVal,
			&ctrlSlider);

		branchThis->GetGraphKey(i,
			&ctrlPasteTime, fTimeVal, fPrevKeyTime, fNextKeyTime,
			&ctrlPasteValue, fValueVal, minVal, maxVal,
			&ctrlPasteTangent, fTangentVal,
			&ctrlPasteInTanLen, fInTanLenVal,
			&ctrlPasteOutTanLen, fOutTanLenVal,
			&ctrlSlider);

		iv = FOREVER;
		if (ctrlCopyTime && ctrlPasteTime) {
			ctrlCopyTime->GetValue(t, (void*)&fTimeVal, iv);
			ctrlPasteTime->SetValue(t, (void*)&fTimeVal);
		}
		if (ctrlCopyValue && ctrlPasteValue) {
			ctrlCopyValue->GetValue(t, (void*)&fValueVal, iv);
			ctrlPasteValue->SetValue(t, (void*)&fValueVal);
		}
		if (ctrlCopyTangent && ctrlPasteTangent) {
			ctrlCopyTangent->GetValue(t, (void*)&fTangentVal, iv);
			ctrlPasteTangent->SetValue(t, (void*)&fTangentVal);
		}
		if (ctrlCopyInTanLen && ctrlPasteInTanLen) {
			ctrlCopyInTanLen->GetValue(t, (void*)&fInTanLenVal, iv);
			ctrlPasteInTanLen->SetValue(t, (void*)&fInTanLenVal);
		}
		if (ctrlCopyValue && ctrlPasteValue) {
			ctrlCopyOutTanLen->GetValue(t, (void*)&fOutTanLenVal, iv);
			ctrlPasteOutTanLen->SetValue(t, (void*)&fOutTanLenVal);
		}
	}
}

void CATGraph::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method)
{
	valid.SetInstant(t);

	int LoopT;
	if (GetCATMotionLimb()) {
		// StepGraph is used in the calculatino of the mask
		// so passing false as the 4th param stops a circular loop
		GetCATMotionLimb()->GetStepTime(t, 1.0f, LoopT, FALSE);
		LoopT = LoopT % STEPTIME100;
	}
	else
	{
		if (method == CTRL_RELATIVE)
		{
			LoopT = (int)*(float*)val % STEPTIME100;
		}
		else
		{
			LoopT = t % STEPTIME100;
		}
	}

	*(float*)val = GetYval(t, LoopT);
}

/*
Animatable* CATGraph::SubAnim(int i)
{
	return pblock->GetController(i + 2);
}
TSTR CATGraph::SubAnimName(int i)
{
	switch(i)
	{
	default:
		return _T("");
	}
}
*/
void CATGraph::RegisterLimb(int stringID, CATMotionLimb *catmotionlimb, CATHierarchyBranch* branch)
{
	// Adds a Attribute Graph controller to the hierarchy.
	// First we add a branch using AddBranch, that will return
	// the branch that we will build up to look like our attribute
	//	if(!(stringID > 0)) return;

	pblock->EnableNotifications(FALSE);
	SetCATMotionLimb(catmotionlimb);

	// create the root of this new Sub-Hierarchy
	CATHierarchyBranch2 *graphBranch = (CATHierarchyBranch2*)branch->AddBranch(GetString(stringID));
	DbgAssert(graphBranch);
	SetBranch(graphBranch);

	graphBranch->SetExpandable(FALSE);
	graphBranch->AddLimb(catmotionlimb->GetLimb());

	CATHierarchyLeaf* ctrlLeaf = NULL;

	// store the guy who will be used to build the hierarchy
	graphBranch->AddControllerRef(this);

	// Set the UI flags
	graphBranch->SetUIType(UI_GRAPH);

	for (int i = 0; i < pblock->NumParams(); i++)
	{
		ParamID pid = pblock->IndextoID(i);
		ParamType2 attributeType = pblock->GetParameterType(pid);
		TSTR strParamName = pblock->GetLocalName(pid);
		DbgAssert(catmotionlimb != NULL);			// If it fails, we need to know!
		TSTR strLimbName = catmotionlimb->GetLimbName();

		if (attributeType == TYPE_FLOAT || attributeType == TYPE_TIMEVALUE || attributeType == TYPE_ANGLE)
		{
			float attributeVal = pblock->GetFloat(pid, 0);

			CATHierarchyBranch2 *ctrlBranch = (CATHierarchyBranch2*)graphBranch->AddBranch(strParamName);
			ctrlBranch->SetSubAnimIndex(i);
			// we have a doubly linked Hierarchy
			ctrlBranch->SetBranchParent((ReferenceTarget*)graphBranch);

			// Defaulting to welded branched
			ctrlLeaf = (CATHierarchyLeaf*)ctrlBranch->AddLeaf(GetString(IDS_LIMBS));
			// Default val is stored on the branch so we need a pointer
			ctrlLeaf->SetLeafParent(ctrlBranch);
			ctrlLeaf->SetDefaultVal(attributeVal);

			pblock->SetControllerByIndex(i, 0, (Control*)ctrlLeaf, TRUE);
		}
	}
}

void CATGraph::UnRegisterLimb(CATMotionLimb* limbOld)
{
	UNREFERENCED_PARAMETER(limbOld);
}

/* ************************************************************************** */
// Tell this branchs Controllers to draw themselves
void CATGraph::DrawGraph(TimeValue t,
	bool	isActive,
	int		nSelectedKey,
	HDC		hGraphDC,
	int		iGraphWidth,
	int		iGraphHeight,
	float	alpha,
	float	beta)
{
	UNREFERENCED_PARAMETER(isActive);
	UNREFERENCED_PARAMETER(nSelectedKey);
	int		LoopT;
	float	posX, oldX = 0.0f;//, avgY, oldAvgY;
	int		LineLength = (iGraphWidth / 120);

	// Create the Pens
	Tab <float> dYpos;
	HPEN hGraphPen = CreatePen(PS_SOLID, 2, GetGraphColour());
	SelectObject(hGraphDC, hGraphPen);
	float dOldY = 0.0f, dNewY = 0.0f;

	for (posX = 0; posX < iGraphWidth; posX += LineLength)
	{
		LoopT = (int)(((float)posX / (float)iGraphWidth) * STEPTIME100);
		dNewY = GetYval(t, LoopT);
		dNewY = iGraphHeight - (alpha + (beta * dNewY));
		if (posX)
		{
			MoveToEx(hGraphDC, (int)oldX, (int)dOldY, NULL);
			LineTo(hGraphDC, (int)posX, (int)dNewY);
		}
		dOldY = dNewY;
		oldX = posX;
	}
	DeleteObject(hGraphPen);
}

/* ************************************************************************** **
** DESCRIPTION: Draws the handles.											  **
**   numSegments..... number of bezier segments, equal to the number of keys. **
** NOTE: in the loop, each Bezier segment draws 2 tangents + 1 point.         **
** ************************************************************************** */
void CATGraph::DrawKeys(const TimeValue	t,
	const int	iGraphWidth,
	const int	iGraphHeight,
	const float		alpha,
	const float		beta,
	HDC				&hGraphDC,
	const int		nSelectedKey)
{
	int			numKeys;
	Point2		point1, outTan, inTan, point2;

	CATKey		key1st, key1, key2, TempKey1, TempKey2;
	bool		isOutTan1, isInTan1, isOutTan2, isInTan2, key1Hot, key2Hot, key1stHot;

	float		widthScaleCoeff(iGraphWidth / float(STEPTIME100));
	HPEN		keyPen = CreatePen(PS_SOLID, 1, RGB(250, 250, 250));
	HPEN		hotKeyPen = CreatePen(PS_SOLID, 1, RGB(250, 0, 0));

	isOutTan2 = isInTan2 = false;
	numKeys = GetNumGraphKeys();

	GetCATKey(0, t, key1st, isInTan1, isOutTan1);
	//	key1st.x *= widthScaleCoeff;
	//	key1st.y = iGraphHeight - (alpha + (beta * key1st.y));
	key1 = key1st;
	if (1 == nSelectedKey) {
		key1stHot = true;
		key1Hot = true;
	}
	else {
		key1Hot = false;
		key1stHot = false;
	}

	for (int j(1); j <= numKeys; ++j)
	{
		if (j == numKeys) {
			key2 = key1st;
			key2.x += STEPTIME100;
			key2Hot = key1stHot;
		}
		else {
			GetCATKey(j, t, key2, isInTan2, isOutTan2);
			if ((j + 1) == nSelectedKey)	key2Hot = true;
			else						key2Hot = false;
		}

		if (isInTan2 || isOutTan1)
		{
			computeControlKeys(key1, key2, outTan, inTan);
			if (isOutTan1) {
				outTan.x = widthScaleCoeff * outTan.x;
				outTan.y = iGraphHeight - (alpha + (beta * outTan.y));
			}
			if (isInTan2) {
				inTan.x = widthScaleCoeff * inTan.x;
				inTan.y = iGraphHeight - (alpha + (beta * inTan.y));
			}

		}
		TempKey1 = key1;
		TempKey1.x *= widthScaleCoeff;
		TempKey1.y = iGraphHeight - (alpha + (beta * key1.y));
		TempKey2 = key2;
		TempKey2.x *= widthScaleCoeff;
		TempKey2.y = iGraphHeight - (alpha + (beta * key2.y));

		DrawKey(TempKey1, isOutTan1, outTan, key1Hot, TempKey2, isInTan2, inTan, key2Hot, hGraphDC, keyPen, hotKeyPen, iGraphWidth);

		key1 = key2;
		isOutTan1 = isOutTan2;
		isInTan1 = isInTan2;
		key1Hot = key2Hot;

	}
	DeleteObject(keyPen);
	DeleteObject(hotKeyPen);
}
/* ************************************************************************** **
** DESCRIPTION: Draws in hGraphDC a square at position 'center'.			  **
** CONTEXT: CATHierarchyBranch2::BranchDrawHandles                             **
** ************************************************************************** */
void	CATGraph::DrawKeyKnob(Point2 center,
	HDC			&hGraphDC,
	const int iGraphWidth,
	const int		size,
	const bool hot)
{
	UNREFERENCED_PARAMETER(hot);
	if (center.x < 0) center.x += iGraphWidth;
	else if (center.x > iGraphWidth) center.x -= iGraphWidth;

	MoveToEx(hGraphDC, (int)(center.x - size), (int)(center.y - size), NULL);
	LineTo(hGraphDC, (int)(center.x - size), (int)(center.y + size));
	LineTo(hGraphDC, (int)(center.x + size), (int)(center.y + size));
	LineTo(hGraphDC, (int)(center.x + size), (int)(center.y - size));
	LineTo(hGraphDC, (int)(center.x - size), (int)(center.y - size));
}

/* ************************************************************************** **
** DESCRIPTION: Draws in hGraphDC a tangent at position 'center'.			  **
** CONTEXT: CATHierarchyBranch2::BranchDrawHandles                             **
** ************************************************************************** */
void	CATGraph::DrawKey(
	const CATKey key1,
	const bool		isOutTan1,
	const Point2	outTan,
	const bool		key1Hot,
	const CATKey key2,
	const bool		isInTan2,
	const Point2	inTan,
	const bool		key2Hot,
	HDC		&hGraphDC,
	HPEN	&keyPen,
	HPEN	&hotKeyPen,
	const int iGraphWidth)
{
	Point2 center1(key1.x, key1.y);
	Point2 center2(key2.x, key2.y);

	if (key2Hot)
		SelectObject(hGraphDC, hotKeyPen);
	else SelectObject(hGraphDC, keyPen);

	if (isInTan2) {
		DrawKeyLine(hGraphDC, inTan, center2, iGraphWidth, key2Hot);
		DrawKeyKnob(inTan, hGraphDC, iGraphWidth, 1, key2Hot);
	}

	if (key1Hot)
		SelectObject(hGraphDC, hotKeyPen);
	else SelectObject(hGraphDC, keyPen);

	if (isOutTan1) {
		DrawKeyLine(hGraphDC, center1, outTan, iGraphWidth, key1Hot);
		DrawKeyKnob(outTan, hGraphDC, iGraphWidth, 1, key1Hot);
	}
	DrawKeyKnob(center1, hGraphDC, iGraphWidth, 2, key1Hot);
}

/* ************************************************************************** **
** DESCRIPTION: Draws in hGraphDC a square at position 'center'.			  **
** CONTEXT: CATHierarchyBranch2::BranchDrawHandles                             **
** ************************************************************************** */
void	CATGraph::DrawKeyLine(HDC			&hGraphDC,
	const Point2	start,
	const Point2	end,
	const int iGraphWidth,
	const bool hot)
{
	UNREFERENCED_PARAMETER(hot);
	if (end.x > iGraphWidth) {
		// the whole line is outside the graph area
		if (start.x > iGraphWidth) {
			MoveToEx(hGraphDC, (int)(start.x - iGraphWidth), (int)start.y, NULL);
			LineTo(hGraphDC, (int)(end.x - iGraphWidth), (int)end.y);
			return;
		}

		float BreakPointY = start.y + ((start.y - end.y) * ((end.x - iGraphWidth) / (end.x - start.x)));

		MoveToEx(hGraphDC, (int)start.x, (int)start.y, NULL);
		LineTo(hGraphDC, iGraphWidth, (int)BreakPointY);

		MoveToEx(hGraphDC, 0, (int)BreakPointY, NULL);
		LineTo(hGraphDC, (int)(end.x - iGraphWidth), (int)end.y);
		return;
	}

	if (start.x < 0.0f) {
		// the whole line is outside the graph area
		if (end.x < 0.0f) {
			MoveToEx(hGraphDC, (int)(start.x + iGraphWidth), (int)(start.y + iGraphWidth), NULL);
			LineTo(hGraphDC, (int)(end.x + iGraphWidth), (int)(end.y + iGraphWidth));
			return;
		}

		float BreakPointY = start.y + ((start.y - end.y) * start.x / (end.x - start.x));

		MoveToEx(hGraphDC, (int)(start.x + iGraphWidth), (int)start.y, NULL);
		LineTo(hGraphDC, iGraphWidth, (int)BreakPointY);

		MoveToEx(hGraphDC, 0, (int)BreakPointY, NULL);
		LineTo(hGraphDC, (int)end.x, (int)end.y);
		return;
	}

	MoveToEx(hGraphDC, (int)start.x, (int)start.y, NULL);
	LineTo(hGraphDC, (int)end.x, (int)end.y);
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class CATGraphPLCB : public PostLoadCallback {
protected:
	CATGraph *catgraph;
	TSTR graphname;
	TSTR ownername;
	CATHierarchyBranch *branch;

public:
	CATGraphPLCB(CATGraph *pOwner) : branch(NULL) {
		catgraph = pOwner;
		catgraph->GetClassName(graphname);
	}

	DWORD GetFileSaveVersion() {
		branch = catgraph->GetBranch();
		DbgAssert(branch);
		if (!branch) {
			//	catgraph->DeleteThis();
			catgraph = NULL;
			return CAT_VERSION_1730;
		}
		return branch->GetCATRoot()->GetFileSaveVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad *iload) {

		// In the limb PLCB we are trying to get rid of the Swivel controller now that in
		// CAT2 we don't use swivels, but in the process, we have actually deleted a footbend
		// controller and now the PLCB is going to crash if we happen to be the PLCB for the deleted controller
		if (catgraph->TestAFlag(A_LOCK_TARGET)) { // !catgraph->GetRefList().FirstItem()){
			// we have no references to us, we should be dead.
			// Kill ourselvs and bail.
			catgraph->ClearAFlag(A_LOCK_TARGET);
			catgraph->MaybeAutoDelete();
			delete this;
			return;
		}

		if (catgraph) catgraph->InitControls();

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

#define		CATGRAPHCHUNK_DIM				1
#define		CATGRAPHCHUNK_NAME_OBSOLETE		2
#define		CATGRAPHCHUNK_FLIPVAL			3

IOResult CATGraph::Save(ISave *isave)
{
	DWORD nb;

	//////////////////////////////////////////////////////////////////////////
	// our references
	isave->BeginChunk(CATGRAPHCHUNK_DIM);
	isave->Write(&dim, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(CATGRAPHCHUNK_FLIPVAL);
	isave->Write(&flipval, sizeof(float), &nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult CATGraph::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case CATGRAPHCHUNK_DIM:
			res = iload->Read(&dim, sizeof(int), &nb);
			break;
		case CATGRAPHCHUNK_FLIPVAL:
			res = iload->Read(&flipval, sizeof(float), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new CATGraphPLCB(this));

	return IO_OK;
}

Animatable* CATGraph::SubAnim(int i)
{
	if (i == 0) return pblock;
	return NULL;
}

TSTR CATGraph::SubAnimName(int i)
{
	if (i == 0) return GetString(IDS_PBLOCK);
	return GetBranch()->GetCATRoot()->GetLayerName(i - 1);
}

// Total number of keys.
int CATGraph::GetNumKeys() {
	CATHierarchyBranch2* ctrlBranch = GetBranch();
	if (!ctrlBranch) return 0;
	CATHierarchyRoot *root = ctrlBranch->GetCATRoot();
	if (!root) return 0;

	TimeValue endtime = root->GetEndTime();
	return  ((int)root->GetStepEaseValue(endtime) / STEPTIME100) * GetNumGraphKeys();
};

// Fill in 'key' with the ith key
void CATGraph::GetKey(int i, IKey *key) {

	CATHierarchyBranch2* ctrlBranch = GetBranch();
	if (!ctrlBranch) return;
	CATHierarchyRoot *root = ctrlBranch->GetCATRoot();
	if (!root) return;
	Ease *stepease = (Ease*)root->GetStepEaseGraph();
	if (!stepease) return;

	TimeValue endtime = root->GetEndTime();
	TimeValue starttime = root->GetStartTime();
	int numkeys = GetNumKeys();

	int numcatgraphkeys = GetNumGraphKeys();
	int loopindex = (int)floor((float)i / (float)numcatgraphkeys);
	int keyindex = i - (loopindex * numcatgraphkeys);
	TimeValue t = (TimeValue)((float)starttime + (((float)i / (float)numkeys) * (float)(endtime - starttime)));

	// Get the CATKey that is used to draw the CATWindow Graph
	CATKey	catkey;
	bool	isOutTan, isInTan;
	GetCATKey(keyindex, t, catkey, isInTan, isOutTan);

	TimeValue loopstarttime = stepease->GetTime((float)(loopindex * STEPTIME100));
	TimeValue loopendtime = stepease->GetTime((float)((loopindex + 1) * STEPTIME100));

	//	key->time = stepease->GetTime(((catkey.x/BASE)*tpf) + loopstarttime);
	key->time = loopstarttime + (TimeValue)((catkey.x / (float)STEPTIME100) * (float)(loopendtime - loopstarttime));
	key->flags = 0;
};

int CATGraph::AppendKey(IKey *key) {
	UNREFERENCED_PARAMETER(key);
	return 0;
};

////////////////////////////////////////

TimeValue CATGraph::GetKeyTime(int index) {
	IKey key;
	GetKey(index, &key);
	return key.time;
};

int CATGraph::GetKeyIndex(TimeValue) {
	return 0;
};

BOOL CATGraph::GetNextKeyTime(TimeValue, DWORD flags, TimeValue &nt) {
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(nt);
	return 0;
};

void CATGraph::CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags) {
	UNREFERENCED_PARAMETER(src);
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(dst);
};

void CATGraph::DeleteKeyAtTime(TimeValue) {
};

BOOL CATGraph::IsKeyAtTime(TimeValue, DWORD flags) {
	UNREFERENCED_PARAMETER(flags);
	return 0;
};

int CATGraph::GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags) {
	UNREFERENCED_PARAMETER(times);
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(range);
	return 0;
};

int CATGraph::GetKeySelState(BitArray &sel, Interval range, DWORD flags) {
	UNREFERENCED_PARAMETER(sel);
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(range);
	return 0;
};

