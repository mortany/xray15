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

#pragma once

 /**********************************************************************
	 ILayerFloatControlFP:
	 This controller controlls all Matrix3 Transform values in CAT
  */
#define I_GIZMOTRANSFORM_FP		Interface_ID(0xd0619b4, 0x7cd4a86)
class IGizmoTransformFP : public FPMixinInterface {
public:
	virtual INode*		GetTargetNode() = 0;
	virtual void		SetTargetNode(INode* n) = 0;

	enum {
		propGetTargetNode,
		propSetTargetNode
	};

	BEGIN_FUNCTION_MAP
		PROP_FNS(propGetTargetNode, GetTargetNode, propSetTargetNode, SetTargetNode, TYPE_INODE);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc() { return GetDescByID(I_GIZMOTRANSFORM_FP); }
};

#define GIZMOTRANSFORM_CLASS_ID		Class_ID(0x19cf487d, 0x98072bd)

#include "CATPlugins.h"

class NLAInfo;
class GizmoTransform : public Control, public IGizmoTransformFP {
private:
	friend class GizmoTransformRolloutData;

	Matrix3 tmOffset;
	INode	*target_node;
	DWORD	dwVersion;
	ULONG	flagsbegin;

	IObjParam *ipbegin;
public:
	BOOL isselected;
	int commandmode;
	int coordsys;

	enum REF_ENUM
	{
		TARGETNODE
	};

	//From Animatable
	Class_ID ClassID() { return GIZMOTRANSFORM_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("CATGizmoTransform"); } //GetString(IDS_CL_GIZMOTRANSFORM);}

	RefTargetHandle Clone(RemapDir &remap);
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	int NumSubs() { return 1; }
	TSTR SubAnimName(int) { return GetString(IDS_TARG_CONT); }
	Animatable* SubAnim(int i) { return ((i == TARGETNODE) && target_node) ? target_node->GetTMController() : NULL; }

	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return (i == TARGETNODE) ? target_node : NULL; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);// { if(i == TARGETNODE) target_node = (INode*)rtarg;	NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);	}
public:

	// I don't know what this other interface system is, but is is the one used to manage Systems
	// here the 'Master' controller can manage deleting/merging the whole character
	void* GetInterface(ULONG id) {
		//	if (id==I_MASTER && target_node) 	return (void *)target_node->GetTMController()->GetInterface(id);
		//	else 								return NULL;
		return Control::GetInterface(id);
	}

	//		int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	//		IParamBlock2* GetParamBlock(int i) { return pblock; }		// return i'th ParamBlock
	//		IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	GizmoTransform(BOOL loading);
	~GizmoTransform();

	//
	// from class Control:
	//
	int IsKeyable() { return FALSE; };
	void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags) { if (target_node) target_node->GetTMController()->CopyKeysFromTime(src, dst, flags); }
	void AddNewKey(TimeValue t, DWORD flags) { if (target_node) target_node->GetTMController()->AddNewKey(t, flags); }
	BOOL IsKeyAtTime(TimeValue t, DWORD flags) { if (target_node) return target_node->GetTMController()->IsKeyAtTime(t, flags); return FALSE; }
	BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt) { if (target_node) return target_node->GetTMController()->GetNextKeyTime(t, flags, nt); return FALSE; }
	int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags) { if (target_node) return target_node->GetTMController()->GetKeyTimes(times, range, flags); return 0; }
	int GetKeySelState(BitArray &sel, Interval range, DWORD flags) { if (target_node) return target_node->GetTMController()->GetKeySelState(sel, range, flags); return 0; }

	// GB 07-Nov-2003: Make key filters on the Time Slider work correctly
	Control *GetPositionController() { return target_node ? target_node->GetTMController()->GetPositionController() : NULL; }
	Control *GetRotationController() { return target_node ? target_node->GetTMController()->GetRotationController() : NULL; }
	Control *GetScaleController() { return target_node ? target_node->GetTMController()->GetScaleController() : NULL; }
	BOOL SetPositionController(Control *c) { return target_node ? target_node->GetTMController()->SetPositionController(c) : FALSE; }
	BOOL SetRotationController(Control *c) { return target_node ? target_node->GetTMController()->SetRotationController(c) : FALSE; }
	BOOL SetScaleController(Control *c) { return target_node ? target_node->GetTMController()->SetScaleController(c) : FALSE; }

	// From control -- for apparatus manipulation
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) { return target_node ? target_node->GetTMController()->Display(t, inode, vpt, flags) : 0; };
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box) { if (target_node)		 target_node->GetTMController()->GetWorldBoundBox(t, inode, vpt, box); };
	int  HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) { return target_node ? target_node->GetTMController()->HitTest(t, inode, type, crossing, flags, p, vpt) : 0; };
	void ActivateSubobjSel(int level, XFormModes& modes) { if (target_node)		 target_node->GetTMController()->ActivateSubobjSel(level, modes); };
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all) { if (target_node)		 target_node->GetTMController()->SelectSubComponent(hitRec, selected, all); };
	void ClearSelection(int selLevel) { if (target_node)		 target_node->GetTMController()->ClearSelection(selLevel); };
	int  SubObjectIndex(CtrlHitRecord *hitRec) { return target_node ? target_node->GetTMController()->SubObjectIndex(hitRec) : 0; };
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert) { if (target_node)		 target_node->GetTMController()->SelectSubComponent(hitRec, selected, all, invert); };

	void GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node) { if (target_node)		 target_node->GetTMController()->GetSubObjectCenters(cb, t, node); };
	void GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node) { if (target_node)		 target_node->GetTMController()->GetSubObjectTMs(cb, t, node); };
	void SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE) { if (target_node)		 target_node->GetTMController()->SubMove(t, partm, tmAxis, val, localOrigin); };
	void SubRotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin = FALSE) { if (target_node)		 target_node->GetTMController()->SubRotate(t, partm, tmAxis, val, localOrigin); };
	void SubScale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE) { if (target_node)		 target_node->GetTMController()->SubScale(t, partm, tmAxis, val, localOrigin); };

	// currenty the layer transform does not work ralative to the nodes parent.
	// that is so that it is consistant between transformnode and no tranform node.
	// We don't want the change parent system to do anything.
	virtual BOOL ChangeParents(TimeValue, const Matrix3&, const Matrix3&, const Matrix3&) { return TRUE; };

	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	bool GetLocalTMComponents(TimeValue t, TMComponentsArg& cmpts, Matrix3Indirect& parentMatrix);

	void CommitValue(TimeValue t) { if (target_node) GetTargetTMController()->CommitValue(t); }
	void RestoreValue(TimeValue t) { if (target_node) GetTargetTMController()->RestoreValue(t); }

	BOOL IsReplaceable() { return TRUE; }
	BOOL IsLeaf() { return FALSE; }
	BOOL CanCopyAnim() { return FALSE; }

	BOOL CanApplyEaseMultCurves() { return false; }
	BOOL CanInstanceController() { return false; };

	BOOL BypassTreeView() { return target_node ? TRUE : FALSE; }
	BOOL BypassTrackBar() { return target_node ? TRUE : FALSE;; }
	BOOL BypassPropertyLevel() { return FALSE; }
	BOOL InvisibleProperty() { return FALSE; }

	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); }

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	FPInterfaceDesc* GetDescByID(Interface_ID id);

	virtual BaseInterface* GetInterface(Interface_ID id)
	{
		if (id == I_GIZMOTRANSFORM_FP) return (IGizmoTransformFP*)this;
		return Control::GetInterface(id);
	}

	//////////////////////////////////////////////////////////////////////////
	// GizmoTransform
//		INode*		Initialise(INode* node);

	DWORD GetVersion() { return dwVersion; }

	// GizmoTransformFP
	virtual INode*		GetTargetNode() { return target_node; };
	virtual void		SetTargetNode(INode* n) { ReplaceReference(TARGETNODE, n); };

	Control* GetTargetTMController() { return target_node ? target_node->GetTMController() : NULL; }

};

extern ClassDesc2* GetGizmoTransformDesc();
