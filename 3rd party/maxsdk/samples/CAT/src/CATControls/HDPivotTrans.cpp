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
#include "HDPivotTrans.h"
#include "CATNodeControl.h"

 //
 //	HDPivotTrans
 //
 //	Our class implementation.
 //
 //	Steve Nov 12 2002
 //

 //
 //	HDPivotTransClassDesc
 //
 //	This gives the MAX information about our class
 //	before it has to actually implement it.
 //
class HDPivotTransClassDesc : public ClassDesc2 {
public:
	int 			IsPublic() { return TRUE; }							// Show this in create branch?
	void *			Create(BOOL loading = FALSE) { return new HDPivotTrans(loading); }
	const TCHAR *	ClassName() { return _T("CATHDPivotTrans"); }
	SClass_ID		SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }	// This determins the type of our controller
	Class_ID		ClassID() { return HD_PIVOTTRANS_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATHDPivotTrans"); }			// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
};

// our global instance of our classdesc class.
static HDPivotTransClassDesc HDPivotTransDesc;
ClassDesc2* GetHDPivotTransDesc() { return &HDPivotTransDesc; }

MoveCtrlApparatusCMode*    HDPivotTrans::moveMode = NULL;

static ParamBlockDesc2 HDPivotTrans_t_param_blk(HDPivotTrans::pb_id, _T("HDPivotParams"), 0, &HDPivotTransDesc,
	P_AUTO_CONSTRUCT, HDPivotTrans::REF_PBLOCK,
	// params
	HDPivotTrans::PB_PRESET_PIVOT_LOCATIONS, _T("PivotLocations"), TYPE_POINT3_TAB, 0, P_VARIABLE_SIZE, IDS_PRESETPIVOTLOCATIONS,
		p_end,
	p_end
);

void HDPivotTrans::Init()
{
	prs = NULL;
	wspivot = NULL;
	pivot = NULL;
	pivottm.IdentityMatrix();

	EditPanel = 0;
	selLevel = 0;

	ipbegin = NULL;
	pblock = NULL;
	dwFileSaveVersion = CAT_VERSION_2521;
};

HDPivotTrans::HDPivotTrans(BOOL loading)
{
	Init();

	// Always create new parameter block
	HDPivotTransDesc.MakeAutoParamBlocks(this);

	if (!loading)
	{
		ReplaceReference(REF_PRS, CreatePRSControl());
		ReplaceReference(REF_WSPIVOT, CreateInterpPosition());
		ReplaceReference(REF_PIVOT, CreateInterpPosition());
	}
}

RefTargetHandle HDPivotTrans::Clone(RemapDir& remap)
{
	// make a new HDPivotTrans object to be the clone
	// call true for loading so the new HDPivotTrans doesn't
	// make new default subcontrollers.
	HDPivotTrans *newHDPivotTrans = new HDPivotTrans(TRUE);

	// clone our subcontrollers and assign them to the new object.
	newHDPivotTrans->ReplaceReference(REF_PBLOCK, CloneParamBlock(pblock, remap));

	// clone our subcontrollers and assign them to the new object.
	if (prs)		newHDPivotTrans->ReplaceReference(REF_PRS, remap.CloneRef(prs));
	if (wspivot)	newHDPivotTrans->ReplaceReference(REF_WSPIVOT, remap.CloneRef(wspivot));
	if (pivot)		newHDPivotTrans->ReplaceReference(REF_PIVOT, remap.CloneRef(pivot));

	BaseClone(this, newHDPivotTrans, remap);

	// now return the new object.
	return newHDPivotTrans;
}

HDPivotTrans::~HDPivotTrans()
{
	DeleteAllRefs();
}

void HDPivotTrans::Copy(Control *from)
{
	if (from->ClassID() == Class_ID(PRS_CONTROL_CLASS_ID, 0)) {
		ReplaceReference(REF_PRS, from);
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void HDPivotTrans::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	SetXFormPacket *ptr = (SetXFormPacket*)val;
	if (ptr->command == XFORM_SET)
	{
		IKeyControl *pivot_keys = GetKeyControlInterface(pivot);
		if (pivot_keys) {
			int numpivotkeys = pivot_keys->GetNumKeys();
			if (numpivotkeys == 0 || numpivotkeys == 1) {

				Matrix3 tm(1);
				Interval iv = FOREVER;
				prs->GetRotationController()->GetValue(0, (void*)&tm, iv, CTRL_RELATIVE);

				Point3 p3Pivot;
				iv = FOREVER;
				pivot->GetValue(0, (void*)&p3Pivot, iv, CTRL_ABSOLUTE);
				p3Pivot = VectorTransform(tm, p3Pivot);

				ptr->tmParent.SetTrans(ptr->tmParent.GetTrans() - p3Pivot);

			}
			else {
				//	Point3 p(0, 0, 0);
				//	pivot->SetValue(t, (void*)&p, 1, CTRL_ABSOLUTE);
					// translate the object in the direction of a local
					// space pivot movement, but using a world space vector.
				Interval iv = FOREVER;
				wspivot->GetValue(t, (void*)&ptr->tmParent, iv, CTRL_RELATIVE);
				//	wspivot->SetValue(t, (void*)&p, 1, CTRL_ABSOLUTE);
			}
		}
	}

	prs->SetValue(t, val, commit, method);

	if (ptr->command == XFORM_ROTATE || ptr->command == XFORM_SET) {
		UpdateWSPivot((Control*)NULL);
	}
}

void HDPivotTrans::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
{
	if (prs && wspivot && pivot)
	{
		prs->GetPositionController()->GetValue(t, val, valid, method);

		// translate the object in the direction of a local
		// space pivot movement, but using a world space vector.
		wspivot->GetValue(t, val, valid, method);

		// apply the rotations like normal
		prs->GetRotationController()->GetValue(t, val, valid, method);

		pivottm = *(Matrix3*)val;

		// move the object b
		Point3 p3Pivot;
		pivot->GetValue(t, (void*)&p3Pivot, valid, CTRL_ABSOLUTE);
		(*(Matrix3*)val).PreTranslate(-p3Pivot);

		prs->GetScaleController()->GetValue(t, val, valid, method);
	}
	tm = *(Matrix3*)val;
}

BOOL HDPivotTrans::ChangeParents(TimeValue t, const Matrix3& oldP, const Matrix3& newP, const Matrix3& tm)
{
	return prs->ChangeParents(t, oldP, newP, tm);
}

RefTargetHandle HDPivotTrans::GetReference(int i)
{
	switch (i)
	{
	case REF_PRS:		return prs;
	case REF_WSPIVOT:	return wspivot;
	case REF_PIVOT:		return pivot;
	case REF_PBLOCK:	return pblock;
	default:		return NULL;
	}
}

void HDPivotTrans::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case REF_PRS:		if (rtarg && ((Control*)rtarg)->ClassID() == Class_ID(PRS_CONTROL_CLASS_ID, 0))
		prs = (Control*)rtarg;
							else prs = NULL;
							break;
	case REF_WSPIVOT:	if (rtarg && ((Control*)rtarg)->ClassID() == Class_ID(HYBRIDINTERP_POSITION_CLASS_ID, 0))
		wspivot = (Control*)rtarg;
							else wspivot = NULL;
							break;
	case REF_PIVOT:		if (rtarg && ((Control*)rtarg)->ClassID() == Class_ID(HYBRIDINTERP_POSITION_CLASS_ID, 0))
		pivot = (Control*)rtarg;
								else pivot = NULL;
								break;
	case REF_PBLOCK:	pblock = (IParamBlock2*)rtarg;					break;
	}

}

Animatable* HDPivotTrans::SubAnim(int i)
{
	switch (i)
	{
	case SUB_PRS:		return prs;
	case SUB_PIVOT:		return pivot;
#ifdef _DEBUG
	case SUB_WSPIVOT:	return wspivot;
#endif
	case SUB_PBLOCK:		return pblock;
	default:			return NULL;
	}
}

TSTR HDPivotTrans::SubAnimName(int i)
{
	switch (i)
	{
	case SUB_PRS:		return GetString(IDS_PRS);
	case SUB_PIVOT:		return GetString(IDS_PIVOT1);
#ifdef _DEBUG
	case SUB_WSPIVOT:	return GetString(IDS_WSPIVOT);
#endif
	case SUB_PBLOCK:	return GetString(IDS_PBLOCK);
	default:			return _T("");
	}
}

class SetHDPivotTransRestore : public RestoreObj {
public:
	HDPivotTrans	*ctrl;
	SetHDPivotTransRestore(HDPivotTrans *c) {
		ctrl = c;
	}
	void Restore(int isUndo) {
		if (isUndo) {
			ctrl->UpdateWSPivot((Control*)ctrl->SubAnim(HDPivotTrans::SUB_PRS));
		}
	}
	void Redo() {
		ctrl->UpdateWSPivot((Control*)ctrl->SubAnim(HDPivotTrans::SUB_PRS));
	}
	int Size() { return 3; }
	void EndHold() {

		DisableRefMsgs();
		ctrl->UpdateWSPivot((Control*)ctrl->SubAnim(HDPivotTrans::SUB_PRS));
		EnableRefMsgs();
	}
};

RefResult HDPivotTrans::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE: {
		if (pblock == hTarg)
		{
		}/*else if(hTarg == wspivot){
			// The ws pivot only ever gets editied by the UpdateWSPivot funtion.
			return REF_STOP;
		}*/else {
		// Due to our clever undo manipulations, we shouldn't get any notifications from
		// our pivo contoller during interactive manipulations. We should really only get
		// this notification if somone is fiddling with the keys in track view.
	//	if(!theHold.RestoreOrRedoing()){
			int isUndoing = 0;
			if ((!theHold.RestoreOrRedoing() || (theHold.Restoring(isUndoing) && isUndoing) || theHold.Redoing())) {
				//	if (pivot == hTarg || prs == hTarg){
				//		if(theHold.Holding()) theHold.Put(new SetHDPivotTransRestore(this));
				//	}
				if (pivot == hTarg) {
					UpdateWSPivot((Control*)hTarg);
					if (theHold.Holding()) theHold.Put(new SetHDPivotTransRestore(this));
				}
				else if (prs == hTarg) {
					//	UpdateWSPivot((Control*)hTarg);
					if (theHold.Holding()) theHold.Put(new SetHDPivotTransRestore(this));
				}
			}
		}
		break;
	}
	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		// to the deleting object.
		if (prs == hTarg)		prs = NULL;
		if (wspivot == hTarg)	wspivot = NULL;
		if (pivot == hTarg)		pivot = NULL;
		if (pblock == hTarg)	pblock = NULL;
		break;
	}
	return REF_SUCCEED;
}

BOOL HDPivotTrans::AssignController(Animatable *control, int subAnim)
{
	switch (subAnim)
	{
	case SUB_PRS:		if (control && ((Control*)control)->ClassID() == Class_ID(PRS_CONTROL_CLASS_ID, 0))
		ReplaceReference(REF_PRS, (RefTargetHandle)control);
		break;
#ifdef _DEBUG
	case SUB_WSPIVOT:	if (control && ((Control*)control)->ClassID() == Class_ID(HYBRIDINTERP_POSITION_CLASS_ID, 0))
		ReplaceReference(REF_WSPIVOT, (RefTargetHandle)control);
		break;
#endif
	case SUB_PIVOT:		if (control && ((Control*)control)->ClassID() == Class_ID(HYBRIDINTERP_POSITION_CLASS_ID, 0))
		ReplaceReference(REF_PIVOT, (RefTargetHandle)control);
		break;
	}

	// Note: 0 here means that there is no special information we need to include
	// in this message.
	NotifyDependents(FOREVER, 0, REFMSG_CONTROLREF_CHANGE, TREE_VIEW_CLASS_ID, FALSE);	// this explicitly refreshes the tree view.		( the false here says for it not to propogate )
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);								// this refreshes all objects (controllers etc..) that reference us.  (and it propogates)
	return TRUE;
}

void HDPivotTrans::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	EditPanel = flags;
	ipbegin = ip;

	//set up the sub-object
	moveMode = new MoveCtrlApparatusCMode(this, ip);

	TSTR type(GetString(IDS_PIVOT));
	const TCHAR *ptype = type;
	ip->RegisterSubObjectTypes((const TCHAR**)&ptype, 1);

	//	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	//	ip->RedrawViews(ip->GetTime());

	prs->BeginEditParams(ip, flags, prev);
}

void HDPivotTrans::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(next);
	if (!ipbegin) return;
	prs->EndEditParams(ip, END_EDIT_REMOVEUI);

	ip->DeleteMode(moveMode);
	delete moveMode; moveMode = NULL;

	ipbegin = NULL;
}

int HDPivotTrans::Display(TimeValue, INode*, ViewExp *vpt, int flags)
{
	UNREFERENCED_PARAMETER(flags);
	if (!ipbegin) return 0;

	Interval iv;
	GraphicsWindow *gw = vpt->getGW();	// This line is here because I don't know how to initialize
										// a *gw. I will change it in the next line
	gw->setTransform(Matrix3(1));		// sets the graphicsWindow to world

	bbox.Init();

	gw->setColor(LINE_COLOR, GetUIColor(COLOR_TARGET_LINE));

	float length = 5.0f;

	// If we are assigned to a CATNodeControl, then make the cross relative to the size of the rig
	CATNodeControl *catnodecontroller = NULL;
	INode *node = FindReferencingClass<INode>(this);
	if (node && (catnodecontroller = (CATNodeControl*)node->GetTMController()->GetInterface(I_CATNODECONTROL)) != NULL) {
		length = catnodecontroller->GetCATParentTrans()->GetCATUnits() * 5.0f;
	}

	DrawCross(gw, pivottm, length, bbox);

	return 1;
}

int HDPivotTrans::HitTest(TimeValue, INode*, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	UNREFERENCED_PARAMETER(flags);
	int savedLimits, res = 0;
	GraphicsWindow *gw = vpt->getGW();
	//	Matrix3 pivottm = inode->GetNodeTM(t);
	HitRegion hr;
	MakeHitRegion(hr, type, crossing, 4, p);
	gw->setHitRegion(&hr);
	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);
	gw->clearHitCode();

	//	Display(t, inode, vpt, flags);

	//	pivot->GetValue(t, (void*)&pivottm, FOREVER, CTRL_RELATIVE);
	//	gw->setTransform(pivottm);
	////////
	gw->setColor(LINE_COLOR, GetUIColor(COLOR_TARGET_LINE));

	float length = 5.0f;
	DrawCross(gw, pivottm, length, bbox);

	//	UpdateWindow(hWnd);
	/////////

	if (gw->checkHitCode()) {
		res = TRUE;
		//		vpt->CtrlLogHit(inode,gw->getHitDistance(),i,0);
		gw->clearHitCode();
	}

	gw->setRndLimits(savedLimits);
	return res;
}

void HDPivotTrans::GetWorldBoundBox(TimeValue, INode *, ViewExp*, Box3& box)
{
	box += bbox;
}

// selection levels:
#define SEL_OBJECT	0
#define SEL_PIVOT	1

void HDPivotTrans::ActivateSubobjSel(int level, XFormModes& modes)
{
	// Set the meshes level
	selLevel = level;

	if (level != SEL_OBJECT) {
		modes = XFormModes(moveMode, NULL, NULL, NULL, NULL, NULL);
	}
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void HDPivotTrans::SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert)
{
	UNREFERENCED_PARAMETER(selected); UNREFERENCED_PARAMETER(all); UNREFERENCED_PARAMETER(invert);
	//	HoldTrack();
	while (hitRec) {
		//		if (selected) {
		//			sel.Set(hitRec->hitInfo);
		//		} else {
		//			sel.Clear(hitRec->hitInfo);
		//			}
		//		if (all)
		hitRec = hitRec->Next();
		//		else break;
	}
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void HDPivotTrans::ClearSelection(int selLevel)
{
	UNREFERENCED_PARAMETER(selLevel);
	//	HoldTrack();
	//	sel.ClearAll();
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

int HDPivotTrans::SubObjectIndex(CtrlHitRecord *)
{
	//	for (ulong i=0; i<hitRec->hitInfo; i++) {
	//		if (sel[i]) count++;
	//		}
	return 1;//count;
}
void HDPivotTrans::GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue, INode *)
{
	if (selLevel == SEL_OBJECT) return;	// shouldn't happen.

//	Matrix3 mat(1);
//	Point3 center(0,0,0);
//	mat = node->GetNodeTM(t);
//	pivot->GetValue(t, (void*)&mat, FOREVER, CTRL_RELATIVE);
//	center = mat.GetTrans();
//	cb->Center(center,0);

	cb->Center(pivottm.GetTrans(), 0);
}

void HDPivotTrans::GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue, INode *)
{
	//	Matrix3 mat = node->GetNodeTM(t);
	//	pivot->GetValue(t, (void*)&mat, FOREVER, CTRL_RELATIVE);
	//	cb->TM(mat,0);
	cb->TM(pivottm, 0);
}

class SetPivotStartRestore : public RestoreObj {
public:
	HDPivotTrans		*pt;
	SetPivotStartRestore(HDPivotTrans *pt) {
		this->pt = pt;
		DisableRefMsgs();
	}
	void Restore(int isUndo) {
		EnableRefMsgs();
		if (isUndo) {
			pt->GetPositionController()->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			//	pt->prs->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			pt->wspivot->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);;
			pt->pivot->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);;
			pt->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		}
	}
	void Redo() {}
	void EndHold() {	}
	int Size() { return 3; }
};

class SetPivotEndRestore : public RestoreObj {
public:
	HDPivotTrans		*pt;
	SetPivotEndRestore(HDPivotTrans *pt) {
		this->pt = pt;
		EnableRefMsgs();
	}
	void Restore(int isUndo) { UNREFERENCED_PARAMETER(isUndo); DisableRefMsgs(); }
	void Redo() {}
	void EndHold() {	}
	int Size() { return 3; }
};

void HDPivotTrans::SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin)
{
	UNREFERENCED_PARAMETER(localOrigin);
	Point3 p = VectorTransform(tmAxis*Inverse(partm), val);

	theHold.Put(new SetPivotStartRestore(this));

	pivot->SetValue(t, (void*)&p, 1, CTRL_RELATIVE);
	Matrix3 tm(1);
	Interval iv = FOREVER;
	prs->GetRotationController()->GetValue(t, (void*)&tm, iv, CTRL_RELATIVE);
	p = VectorTransform(tm, p);

	IKeyControl *pivot_keys = GetKeyControlInterface(pivot);
	// before any keyframes are create, we keep the WS pivot at the origin.
	// This should mean that the very 1st key on WS Pivot is ALWAYS [0, 0, 0]
	if (pivot_keys && pivot_keys->GetNumKeys() == 0 && !Animating()) {
		theHold.Put(new SetPivotEndRestore(this));
		prs->GetPositionController()->SetValue(t, (void*)&p, 1, CTRL_RELATIVE);
	}
	else {
		wspivot->SetValue(t, (void*)&p, 1, CTRL_RELATIVE);
		theHold.Put(new SetPivotEndRestore(this));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void HDPivotTrans::UpdateWSPivot(Control *changing)
{
	UNREFERENCED_PARAMETER(changing);
	if (!pivot || !wspivot) return;

	//	if(pivot->ClassID() == Class_ID(HYBRIDINTERP_POSITION_CLASS_ID, 0))	{
	IKeyControl *pivot_keys = GetKeyControlInterface(pivot);
	IKeyControl *wspivot_keys = GetKeyControlInterface(wspivot);
	if (!pivot_keys || !wspivot_keys) return;

	int numpivotkeys = pivot_keys->GetNumKeys();
	int numwspivotkeys = wspivot_keys->GetNumKeys();

	//	if(numpivotkeys==0)
	//		return;
		//
	IBezPoint3Key wspivot_key, pivot_key;
	Matrix3 tm(1);
	Point3 offset = P3_IDENTITY;
	Point3 p3Pivot = P3_IDENTITY;

	if (numpivotkeys == 0 || numpivotkeys == 1) {
		// There is a case where only one or no key exists and the SetValue happens.
/*		Matrix3 tm(1);
		prs->GetRotationController()->GetValue(0, (void*)&tm, FOREVER, CTRL_RELATIVE);

		pivot->GetValue(0, (void*)&offset, FOREVER, CTRL_ABSOLUTE);
		offset = VectorTransform(tm,offset);
		wspivot->SetValue(0, (void*)&offset, 1, CTRL_ABSOLUTE);
		return;
	*/
	}

	DisableRefMsgs();

	Interval iv = FOREVER;
	if (numwspivotkeys == 0)
		wspivot->GetValue(pivot_key.time, (void*)&offset, iv, CTRL_ABSOLUTE);
	else {
		wspivot_keys->GetKey(0, &wspivot_key);
		offset = wspivot_key.val;
	}

	if (numpivotkeys != numwspivotkeys)
		wspivot_keys->SetNumKeys(numpivotkeys);

	for (int i = 0; i < numpivotkeys; i++)
	{
		wspivot_keys->GetKey(i, &wspivot_key);
		pivot_keys->GetKey(i, &pivot_key);

		tm = Matrix3(1);
		iv = FOREVER;
		prs->GetRotationController()->GetValue(pivot_key.time, (void*)&tm, iv, CTRL_RELATIVE);
		//	tm = Inverse(tm);

		wspivot_key.time = pivot_key.time;
		wspivot_key.flags = pivot_key.flags;

		wspivot_key.intan = tm.PointTransform(pivot_key.intan);
		wspivot_key.outtan = tm.PointTransform(pivot_key.outtan);
		wspivot_key.inLength = tm.PointTransform(pivot_key.inLength);
		wspivot_key.outLength = tm.PointTransform(pivot_key.outLength);
		tm.SetTrans(offset);
		// Add on the change in pivot in rotation space
		if (i > 0)
			tm.PreTranslate(pivot_key.val - p3Pivot);

		wspivot_key.val = tm.GetTrans();
		offset = wspivot_key.val;
		p3Pivot = pivot_key.val;

		wspivot_keys->SetKey(i, &wspivot_key);
	}
	/*	}else{
			IKeyControl *wspivot_keys = GetKeyControlInterface(wspivot);
			if(!wspivot_keys) return;
			// clear all keys on the ws_pivot controller
			wspivot_keys->SetNumKeys(0);

			Matrix3 tm(1);
			Point3 offset = P3_IDENTITY;
			Point3 p3Pivot = P3_IDENTITY;
			Point3 pivot_val = P3_IDENTITY;

			Interval range = pivot->GetTimeRange(TIMERANGE_ALL|TIMERANGE_CHILDANIMS);
			Interval rotrange = prs->GetRotationController()->GetTimeRange(TIMERANGE_ALL|TIMERANGE_CHILDANIMS);
			range += rotrange.Start();
			range += rotrange.End();

			SuspendAnimate();
			AnimateOn();
			for(TimeValue t = range.Start(); t<=range.End(); t+= GetTicksPerFrame())
			{
				pivot->GetValue(t, (void*)&pivot_val, FOREVER, CTRL_ABSOLUTE);

				if(t==range.Start()){
					IBezPoint3Key wspivot_key;
					wspivot_key.time = t;
					wspivot_key.val = P3_IDENTITY;
					wspivot_keys->AppendKey(&wspivot_key);
				}else{
				//	if(pivot_val==p3Pivot) continue;

					tm = Matrix3(1);
					prs->GetRotationController()->GetValue(t, (void*)&tm, FOREVER, CTRL_RELATIVE);
					tm.SetTrans(offset);
					// Add on the change in pivot in rotation space
					tm.PreTranslate(pivot_val - p3Pivot);
					wspivot->SetValue(t, (void *)&tm.GetTrans(), 1, CTRL_ABSOLUTE);
					offset = tm.GetTrans();
				}
				p3Pivot = pivot_val;
			}
			AnimateOff();
			ResumeAnimate();
		}
	*/

	EnableRefMsgs();
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

Interval HDPivotTrans::GetTimeRange(DWORD flags) {
	Interval range, iv;
	BOOL init = FALSE;
	if (prs) {
		iv = prs->GetTimeRange(flags);
		if (!iv.Empty()) {
			range.Set(iv.Start(), iv.End());
			init = TRUE;
		}
	}
	if (pivot) {
		iv = pivot->GetTimeRange(flags);
		if (!iv.Empty()) {
			if (init) {
				range += iv.Start();
				range += iv.End();
			}
			else {
				range.Set(iv.Start(), iv.End());
				init = TRUE;
			}
		}
	}
	return range;
}

/**********************************************************************
 * Loading and saving....

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class HDPivotTransPLCB : public PostLoadCallback {
	protected:
		HDPivotTrans *bone;

	public:
		HDPivotTransPLCB(HDPivotTrans *pOwner) { bone = pOwner; }

		DWORD GetFileSaveVersion() {
		//	DbgAssert(bone);
		//	ECATParent *catparent = bone->prs->GetCATParentTrans();
		///	DbgAssert(catparent);
		//	return catparent->GetFileSaveVersion();
		}

		int Priority() { return 5; }

		void proc(ILoad *iload) {
			DbgAssert(bone->GetNode());
			// To clean up, delete the PLCB.  This class must
			// obviously have been created by calling 'new'.
			delete this;
		}
};

*/
#define SELLEVEL_CHUNKID		0x0100
#define HD_PIVOTTRANS_VERSION	0x0110

IOResult HDPivotTrans::Save(ISave *isave) {
	IOResult res;
	ULONG nb;

	// This stores the version of CAT used to save the file.
	dwFileSaveVersion = CAT_VERSION_CURRENT;
	isave->BeginChunk(HD_PIVOTTRANS_VERSION);
	isave->Write(&dwFileSaveVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	isave->BeginChunk(SELLEVEL_CHUNKID);
	res = isave->Write(&selLevel, sizeof(selLevel), &nb);
	isave->EndChunk();

	return res;
}

IOResult HDPivotTrans::Load(ILoad *iload) {
	IOResult res;
	ULONG nb;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case HD_PIVOTTRANS_VERSION:
			res = iload->Read(&dwFileSaveVersion, sizeof DWORD, &nb);
			break;
		case SELLEVEL_CHUNKID:
			iload->Read(&selLevel, sizeof(selLevel), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}
	return IO_OK;
}

