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

#include "CatPlugins.h"
#include <CATAPI/CATClassID.h>
#include "HIPivotTrans.h"
#include "CATNodeControl.h"
#include "Limbdata2.h"

 //
 //	HIPivotTransClassDesc
 //
 //	This gives the MAX information about our class
 //	before it has to actually implement it.
 //
class HIPivotTransClassDesc : public ClassDesc2 {
public:
	int 			IsPublic() { return TRUE; }							// Show this in create branch?
	void *			Create(BOOL loading = FALSE) { return new HIPivotTrans(loading); }
	const TCHAR *	ClassName() { return _T("CATHIPivotTrans"); }
	SClass_ID		SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }	// This determins the type of our controller
	Class_ID		ClassID() { return HI_PIVOTTRANS_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATHIPivotTrans"); }			// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
};

// our global instance of our classdesc class.
static HIPivotTransClassDesc HIPivotTransDesc;
ClassDesc2* GetHIPivotTransDesc() { return &HIPivotTransDesc; }

MoveCtrlApparatusCMode*    HIPivotTrans::mMoveMode = NULL;
RotateCtrlApparatusCMode*    HIPivotTrans::mRotateMode = NULL;

class HIPivotTransParamDlgCallBack// : public TimeChangeCallback
{
	HWND hDlg;
	HIPivotTrans* bone;

	ISpinnerControl *ccSpinLength;
	ISpinnerControl *ccSpinWidth;
	ISpinnerControl *ccSpinHeight;

	ICustButton* btnCollapseStack;
	ICustButton* btnAddHIPivotTrans;

public:

	HIPivotTransParamDlgCallBack()
	{
		ccSpinLength = NULL;
		ccSpinWidth = NULL;
		ccSpinHeight = NULL;

		btnCollapseStack = NULL;
		btnAddHIPivotTrans = NULL;

		bone = NULL;
		hDlg = NULL;
	}

	BOOL IsDlgOpen() { return (bone != NULL); }
	HWND GetHWnd() { return hDlg; }

	void InitControls(HWND hWnd, HIPivotTrans* /*owner*/)
	{
		hDlg = hWnd;
	}
	void ReleaseControls(HWND /*hWnd*/) {

		SAFE_RELEASE_SPIN(ccSpinLength);
		SAFE_RELEASE_SPIN(ccSpinWidth);
		SAFE_RELEASE_SPIN(ccSpinHeight);

		SAFE_RELEASE_BTN(btnAddHIPivotTrans);
		bone = NULL;
		hDlg = NULL;
	}

	void Update()
	{
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM /*wParam*/, LPARAM lParam)
	{
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, (HIPivotTrans*)lParam);
			break;
		case WM_DESTROY:

			//			ReleaseControls(hWnd);
			return FALSE;

		case WM_NOTIFY:
			break;

		default:
			return FALSE;
		}
		return TRUE;
	}

	virtual void DeleteThis() { }//delete this; }
};

static HIPivotTransParamDlgCallBack HIPivotTransParamsCallback;

void HIPivotTrans::Init()
{
	prs = NULL;
	pivotpos = NULL;
	pivotrot = NULL;

	EditPanel = 0;
	ipbegin = NULL;
};

HIPivotTrans::HIPivotTrans(BOOL loading)
{
	Init();

	if (!loading)
	{
		ReplaceReference(PRS, CreatePRSControl());
		ReplaceReference(PIVOTPOS, NewDefaultPoint3Controller());
		ReplaceReference(PIVOTROT, NewDefaultPoint3Controller());
	}
}

RefTargetHandle HIPivotTrans::Clone(RemapDir& remap)
{
	// make a new HIPivotTrans object to be the clone
	// call true for loading so the new HIPivotTrans doesn't
	// make new default subcontrollers.
	HIPivotTrans *newHIPivotTrans = new HIPivotTrans(TRUE);

	// clone our subcontrollers and assign them to the new object.
	if (prs)		newHIPivotTrans->ReplaceReference(PRS, remap.CloneRef(prs));
	if (pivotpos)	newHIPivotTrans->ReplaceReference(PIVOTPOS, remap.CloneRef(pivotpos));
	if (pivotrot)	newHIPivotTrans->ReplaceReference(PIVOTROT, remap.CloneRef(pivotrot));

	// The pointer will be reconnected to the new object. See :RefAdded
//	remap.PatchPointer((RefTargetHandle*)&newHIPivotTrans->obj, (RefTargetHandle)obj);

	BaseClone(this, newHIPivotTrans, remap);

	// now return the new object.
	return newHIPivotTrans;
}

HIPivotTrans::~HIPivotTrans()
{
	DeleteAllRefs();
}

void HIPivotTrans::Copy(Control *from)
{
	if (from->ClassID() == Class_ID(PRS_CONTROL_CLASS_ID, 0)) {
		ReplaceReference(PRS, from);
	}
	else {
		if (from->GetPositionController()) { prs->SetPositionController(from->GetPositionController()); }
		if (from->GetRotationController()) { prs->SetRotationController(from->GetRotationController()); }
		if (from->GetScaleController()) { prs->SetScaleController(from->GetScaleController()); }
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void HIPivotTrans::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	prs->SetValue(t, val, commit, method);
}

void HIPivotTrans::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
{
	if (prs && pivotpos && pivotrot)
	{
		Matrix3* pVal = reinterpret_cast<Matrix3*>(val);
		prs->GetValue(t, val, valid, method);

		Point3 p3PivotPos;
		Point3 p3PivotRot;
		pivotpos->GetValue(t, (void*)&p3PivotPos, valid, CTRL_ABSOLUTE);
		pivotrot->GetValue(t, (void*)&p3PivotRot, valid, CTRL_ABSOLUTE);

		CATNodeControl* pNodeCtrl = GetACATNodeControl();
		if (pNodeCtrl)
		{
			Point3 objdim = pNodeCtrl->GetObjDim();
			p3PivotPos *= objdim;
		}

		pVal->PreTranslate(p3PivotPos);
		pVal->PreRotateX(-p3PivotRot.x);
		pVal->PreRotateY(p3PivotRot.y);
		pVal->PreRotateZ(p3PivotRot.z);
		pVal->PreTranslate(-p3PivotPos);
	}
}

RefTargetHandle HIPivotTrans::GetReference(int i)
{
	switch (i)
	{
	case PRS:		return prs;
	case PIVOTPOS:	return pivotpos;
	case PIVOTROT:	return pivotrot;
	default:		return NULL;
	}
}

void HIPivotTrans::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case PRS:		prs = (Control*)rtarg; 				break;
	case PIVOTPOS:	pivotpos = (Control*)rtarg; 		break;
	case PIVOTROT:	pivotrot = (Control*)rtarg; 		break;
	}

}

Animatable* HIPivotTrans::SubAnim(int i)
{
	switch (i)
	{
	case PRS:		return prs;
	case PIVOTPOS:	return pivotpos;
	case PIVOTROT:	return pivotrot;
	default:		return NULL;
	}
}

TSTR HIPivotTrans::SubAnimName(int i)
{
	switch (i)
	{
	case PRS:		return GetString(IDS_PRS);
	case PIVOTPOS:	return GetString(IDS_PIVOTPOS);
	case PIVOTROT:	return GetString(IDS_PIVOTROT);
	default:		return _T("");
	}
}

RefResult HIPivotTrans::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		break;
	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		// to the deleting object.
		if (prs == hTarg)		prs = NULL;
		if (pivotpos == hTarg)	pivotpos = NULL;
		if (pivotrot == hTarg)	pivotrot = NULL;
		break;
	}
	return REF_SUCCEED;
}

BOOL HIPivotTrans::AssignController(Animatable *control, int subAnim)
{
	switch (subAnim)
	{
	case PRS:		ReplaceReference(PRS, (RefTargetHandle)control);				break;
	case PIVOTPOS:	ReplaceReference(PIVOTPOS, (RefTargetHandle)control);			break;
	case PIVOTROT:	ReplaceReference(PIVOTROT, (RefTargetHandle)control);			break;
	}

	// Note: 0 here means that there is no special information we need to include
	// in this message.
	NotifyDependents(FOREVER, 0, REFMSG_CONTROLREF_CHANGE, TREE_VIEW_CLASS_ID, FALSE);	// this explicitly refreshes the tree view.		( the false here says for it not to propogate )
	return TRUE;
}

void HIPivotTrans::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	EditPanel = flags;
	ipbegin = ip;
	if (flags&BEGIN_EDIT_MOTION)// || flags&BEGIN_EDIT_LINKINFO)
	{
		DbgAssert(mMoveMode == NULL && mRotateMode == NULL);
		mMoveMode = new MoveCtrlApparatusCMode(this, ip);
		mRotateMode = new RotateCtrlApparatusCMode(this, ip);

		TSTR type(GetString(IDS_PIVOT));
		const TCHAR *ptype = type;
		ip->RegisterSubObjectTypes((const TCHAR**)&ptype, 1);
	}
	prs->BeginEditParams(ip, flags, prev);
}

void HIPivotTrans::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(next);
	if (EditPanel&BEGIN_EDIT_MOTION)// || EditPanel&BEGIN_EDIT_LINKINFO)
	{

		ip->DeleteMode(mMoveMode);
		ip->DeleteMode(mRotateMode);
		SAFE_DELETE(mMoveMode);
		SAFE_DELETE(mRotateMode);
	}
	prs->EndEditParams(ip, END_EDIT_REMOVEUI);
	ipbegin = NULL;
}

int HIPivotTrans::Display(TimeValue t, INode* pNode, ViewExp *vpt, int flags)
{
	UNREFERENCED_PARAMETER(flags);
	if (!ipbegin) return 0;

	Interval iv;
	GraphicsWindow *gw = vpt->getGW();	// This line is here because I don't know how to initialize
										// a *gw. I will change it in the next line
	gw->setTransform(Matrix3(1));		// sets the graphicsWindow to world
	gw->setColor(LINE_COLOR, GetUIColor(COLOR_TARGET_LINE));

	// If we are assigned to a CATNodeControl, then make the cross relative to the size of the rig
	float length = 5.0f;
	CATNodeControl *catnodecontroller = dynamic_cast<CATNodeControl*>(pNode->GetTMController());
	if (catnodecontroller != NULL)
		length *= catnodecontroller->GetCATUnits();

	Box3 bbox;
	Matrix3 tmCurrent = pNode->GetNodeTM(t);

	Point3 p3SubOffset(Point3::Origin);
	if (pivotpos != NULL)
	{
		pivotpos->GetValue(t, &p3SubOffset, FOREVER);
		tmCurrent.PreTranslate(p3SubOffset);
	}
	DrawCross(gw, tmCurrent, length, bbox);

	return 1;
}

int HIPivotTrans::HitTest(TimeValue t, INode* pNode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	UNREFERENCED_PARAMETER(flags);
	int savedLimits, res = 0;
	GraphicsWindow *gw = vpt->getGW();
	HitRegion hr;
	MakeHitRegion(hr, type, crossing, 4, p);
	gw->setHitRegion(&hr);
	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);
	gw->clearHitCode();

	Display(t, pNode, vpt, flags);

	if (gw->checkHitCode()) {
		res = TRUE;
		gw->clearHitCode();
	}

	gw->setRndLimits(savedLimits);
	return res;
}

void HIPivotTrans::GetWorldBoundBox(TimeValue t, INode* pNode, ViewExp*, Box3& box)
{
	float length = 5;
	CATNodeControl* pCATCtrl = dynamic_cast<CATNodeControl*>(pNode->GetTMController());
	if (pCATCtrl != NULL)
		length *= pCATCtrl->GetCATUnits();
	length /= 2;

	Matrix3 tmCurrent = pNode->GetNodeTM(t);
	const Point3& p3Current = tmCurrent.GetTrans();
	box += (p3Current + Point3(length, length, length));
	box += (p3Current - Point3(length, length, length));
}

// selection levels:
#define SEL_OBJECT	0
#define SEL_PIVOT	1

void HIPivotTrans::ActivateSubobjSel(int level, XFormModes& modes)
{
	if (level != SEL_OBJECT)
	{
		modes.move = mMoveMode;
		modes.rotate = mRotateMode;
		//modes = XFormModes(mMoveMode,NULL,NULL,NULL,NULL,NULL);
	}
}

void HIPivotTrans::SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert)
{
	UNREFERENCED_PARAMETER(selected); UNREFERENCED_PARAMETER(all); UNREFERENCED_PARAMETER(invert);
}

void HIPivotTrans::ClearSelection(int selLevel)
{
	UNREFERENCED_PARAMETER(selLevel);
}

int HIPivotTrans::SubObjectIndex(CtrlHitRecord *)
{
	return 1;
}

void HIPivotTrans::GetSubObjectCenters(SubObjAxisCallback* cb, TimeValue t, INode* node)
{
	Matrix3 mat = node->GetNodeTM(t);
	//pivotrot->GetValue(t, (void*)&mat, FOREVER, CTRL_RELATIVE);
	Point3 p3SubPos = Point3::Origin;
	pivotpos->GetValue(t, &p3SubPos, FOREVER, CTRL_ABSOLUTE);
	mat.PreTranslate(p3SubPos);
	const Point3& center = mat.GetTrans();
	cb->Center(center, 0);
}

void HIPivotTrans::GetSubObjectTMs(SubObjAxisCallback* cb, TimeValue t, INode* node)
{
	Matrix3 mat = node->GetNodeTM(t);
	Point3 p3SubPos = Point3::Origin;
	pivotpos->GetValue(t, &p3SubPos, FOREVER, CTRL_ABSOLUTE);
	mat.PreTranslate(p3SubPos);
	cb->TM(mat, 0);
}

void HIPivotTrans::SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin)
{
	UNREFERENCED_PARAMETER(localOrigin);
	Point3 p = VectorTransform(tmAxis*Inverse(partm), val);
	HoldActions	hold(IDS_HLD_PIVOTEDIT);
	pivotpos->SetValue(t, (void*)&p, 1, CTRL_RELATIVE);
}

void HIPivotTrans::SubRotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat &val, BOOL localOrigin)
{
	UNREFERENCED_PARAMETER(localOrigin);

	Matrix3 tmLocal = tmAxis*Inverse(partm);
	Matrix3 tmRot;
	val.MakeMatrix(tmRot);

	Matrix3 rotOffset = Inverse(tmLocal) * tmRot * tmLocal;
	float asEuler[3] = { 0, 0, 0 };
	MatrixToEuler(rotOffset, asEuler, EULERTYPE_XYZ);

	HoldActions	hold(IDS_HLD_PIVOTEDIT);
	pivotrot->SetValue(t, asEuler, 1, CTRL_RELATIVE);
}

/**********************************************************************
 * Loading and saving....
*/

// NOTE: This class used to (back in CAT1.X) load and save things.

//////////////////////////////////////////////////////////////////////

CATNodeControl* HIPivotTrans::GetACATNodeControl()
{
	return FindReferencingClass<CATNodeControl>(this);
}