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

#define LAYER_TRANSFORM_CLASS_ID		Class_ID(0x5c061e5c, 0x1dba326e)

#include "CATPlugins.h"

class NLAInfo;
class LayerTransform : public Control {
private:
	friend class LayerTransformRolloutData;
	friend class SetTMRestore;

	class ICATParentTrans *catparenttrans;

	Control *prs;
	Matrix3 tmOffset;
	Matrix3 tmTransform;
	Box3 bbox;
	BOOL transformsloaded;

	///////////////////////////////////////////////////////////////////////////
	TSTR file;	int layerindex;	int flags; TimeValue startT; class CATControl* bodypart;
	Interval ghostTimeRange;
public:
	NLAInfo *layerinfo;

	enum REF_ENUM
	{
		PRS
	};

	//From Animatable
	Class_ID ClassID() { return LAYER_TRANSFORM_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("LayerTransform"); } //GetString(IDS_CL_LAYER_TRANSFORM);}

	RefTargetHandle Clone(RemapDir &remap);
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	int NumSubs() { return 1; }
	TSTR SubAnimName(int) { return GetString(IDS_LYR_TRANSFORM); }
	Animatable* SubAnim(int i) { return (i == PRS) ? prs : NULL; }

	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return (i == PRS) ? prs : NULL; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { if (i == PRS) prs = (Control*)rtarg; }
public:

	// I don't know what this other interface system is, but is is the one used to manage Systems
	// here the 'Master' controller can manage deleting/merging the whole character
	void* GetInterface(ULONG id) {
		if (id == I_MASTER) 	return (void *)this;
		else 				return Control::GetInterface(id);
	}

	// this guy never gets shown
	BOOL BypassTreeView() { return true; };
	BOOL BypassPropertyLevel() { return true; };

	//		int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	//		IParamBlock2* GetParamBlock(int i) { return pblock; }		// return i'th ParamBlock
	//		IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	LayerTransform(BOOL loading);
	~LayerTransform();

	//
	// from class Control:
	//
	int IsKeyable() { return FALSE; };
	void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags) { if (prs) prs->CopyKeysFromTime(src, dst, flags); }
	void AddNewKey(TimeValue t, DWORD flags) { if (prs) prs->AddNewKey(t, flags); }
	BOOL IsKeyAtTime(TimeValue t, DWORD flags) { if (prs) return prs->IsKeyAtTime(t, flags); return FALSE; }
	BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt) { if (prs) return prs->GetNextKeyTime(t, flags, nt); return FALSE; }
	int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags) { if (prs) return prs->GetKeyTimes(times, range, flags); return 0; }
	int GetKeySelState(BitArray &sel, Interval range, DWORD flags) { if (prs) return prs->GetKeySelState(sel, range, flags); return 0; }

	// GB 07-Nov-2003: Make key filters on the Time Slider work correctly
	Control *GetPositionController() { return prs ? prs->GetPositionController() : NULL; }
	Control *GetRotationController() { return prs ? prs->GetRotationController() : NULL; }
	Control *GetScaleController() { return prs ? prs->GetScaleController() : NULL; }
	BOOL SetPositionController(Control *c) { return prs ? prs->SetPositionController(c) : FALSE; }
	BOOL SetRotationController(Control *c) { return prs ? prs->SetRotationController(c) : FALSE; }
	BOOL SetScaleController(Control *c) { return prs ? prs->SetScaleController(c) : FALSE; }

	// currenty the layer transform does not work ralative to the nodes parent.
	// that is so that it is consistant between transformnode and no tranform node.
	// We don't want the change parent system to do anything.
	virtual BOOL ChangeParents(TimeValue, const Matrix3&, const Matrix3&, const Matrix3&) { return TRUE; };

	BOOL IsLeaf() { return FALSE; }
	void MouseCycleStarted(TimeValue t);
	void MouseCycleCompleted(TimeValue t);

	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); }

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	void GetWorldBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box);

	//////////////////////////////////////////////////////////////////////////
	// LayerTransform

	// Display a ghost of the character
	BOOL GetDisplayGhost();//{ return layerinfo->TestFlag(LAYER_DISPLAY_GHOST); }
	void SetDisplayGhost(BOOL b);//{ layerinfo->SetFlag(LAYER_DISPLAY_GHOST, b);; NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE); }
//	BOOL GetDisplayGhost(){ return (ghostTimeRange!=NEVER) }
//	void SetDisplayGhost(BOOL b){ ghostTimeRange==FOREVER; NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE); }

	INode*		CreateINode(TSTR name, float size, Color clr);
	void		ReInitialize(NLAInfo* layerinfo);
	INode*		Initialise(NLAInfo* layerinfo, Control *prs);
	NLAInfo*	GetLayerInfo() { return layerinfo; }

	///////////////////////////////////////////////

	INode*		BuildNode();
	INode*		Initialise(ICATParentTrans* catparenttrans, TSTR name, TSTR file, int layerindex, int flags, TimeValue startT, CATControl* bodypart);
	void		SetGhostTimeRange(Interval range) { ghostTimeRange = range; };

	BOOL		TransformsLoaded() { return transformsloaded; };
	void		SetTransforms(Matrix3 tmOffset, Matrix3 tmTransform);//{ this->tmOffset = tmOffset; this->tmTransform = tmTransform; };
	void		GetTransforms(Matrix3 &tmOffset, Matrix3 &tmTransform) { tmOffset = this->tmOffset; tmTransform = this->tmTransform; };

};

extern ClassDesc2* GetLayerTransformDesc();
