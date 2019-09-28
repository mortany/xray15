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

#include "CATMuscle.h"
#include "IMuscle.h"
#include "icurvctl.h"
#include "../DataRestoreObj.h"

#define STRANDFLAG_DELETING				(1<<3)

#define STRANDFLAG_HANDLES_VISIBLE		(1<<6)
#define STRANDFLAG_KEEP_ROLLOUTS		(1<<12)
#define STRANDFLAG_ENABLE_SQUASHSTRETCH	(1<<14)

extern ClassDesc2* GetMuscleStrandDesc();

struct SphereParams
{
	SphereParams()
		: UStart(0)
		, UEnd(0)
		, tmWorldSpace(1)
		, tmLocalSpace(1)
		, pSphereNode(NULL)
	{	}

	float UStart;
	float UEnd;
	Matrix3 tmWorldSpace;
	Matrix3 tmLocalSpace;
	INode* pSphereNode;
};
typedef Tab <SphereParams> TabSphereParams;

class MuscleStrand : public SimpleObject2, public IMuscleStrand, public ICATMuscleSystemMaster, IDataRestoreOwner<float>, IDataRestoreOwner<int>, IDataRestoreOwner<TabSphereParams> {
protected:
	friend class MuscleStrandDlgCallBack;
	friend class MuscleStrandPasteRestore;
	friend class SphereParamsRestore;
	friend class MuscleStrandCreateCallBack;

	DWORD dVersion;

	// left middle right like limbs on CAT
	Axis mirroraxis;
	int lmr;
	float handlesize;

	Matrix3 tm;

	Color clrColour;
	TSTR strName;

	int flags;
	DEFORMER_TYPE	deformer_type;
	TabSphereParams mTabSphereParams;

	ICurveCtl		*mpCCtl;

	float lengthCurrent, scaleCurrent;
	float scaleShort, lengthShort;

	// keeps track of the panel our rollout is being displayed on
	int flagsBegin;
	// Class Interface pointer
	IObjParam *ipRollout;

	BOOL bDeleting;

	enum REFS {
		//	PB_REF,
		NODE_ST,
		NODE_ST_HDL,

		NODE_EN,
		NODE_EN_HDL,
		CC_PROFILE,
		NUMREFS
	};

	static const int NUMNODEREFS = 4;

	// Handles
	Tab <INode*>	tabHandles;

	// caches of our handle positions
	Matrix3	ws_hdl_tm[NUMNODEREFS];
	// if we decide to delete our handles, then we cache our offsets
	Matrix3	ls_hdl_tm[NUMNODEREFS];

	Interval handlesvalid;

public:
	// we only have one pb, but we need to give it and ID
	enum {
		pb_idParams
	};

	/////////////////////////////////////////////////////////
	// class IMuscleStrand

	/////////////////////////////////////////////////////////
	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack();

	// Class Object
	BOOL HasUVW() { return TRUE; };
	void SetGenUVW(BOOL) { return; };

	// If we say we are a WorldSpaceObject,
	// we will not be instanced (which is not good)
	BOOL IsWorldSpaceObject() { return TRUE; }

	// From Animatable
	void BeginEditParams(IObjParam  *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

	// From SimpleObject
	void RefreshUI();

	/////////////////////////////////////////////////////////
	// from class ReferenceMaker
	//
	int NumRefs() { return NUMREFS; };
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_GENCOLOR; }
	void InitNodeName(TSTR& s) { s = GetString(IDS_NODE_NAME_MUSCLE_STRAND); }

	//From Animatable
	Class_ID ClassID() { return MUSCLESTRAND_CLASS_ID; }
	SClass_ID SuperClassID() { return HELPER_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_MUSCLESTRAND); }

	void Init();
	MuscleStrand(BOOL loading = FALSE);
	~MuscleStrand();
	void DeleteThis() { delete this; }

	void GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt);
	RefTargetHandle Clone(RemapDir &remap);

	// From SimpleObject
	void BuildMesh(TimeValue t);

	int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);

	BaseInterface* GetInterface(Interface_ID id);
	/////////////////////////////////////////////////////////
	// Class MuscleStrand

	void SetFlag(USHORT f, BOOL tf = TRUE);// { if(tf) flags |= f; else flags &= ~f; };
	void ClearFlag(USHORT f) { SetFlag(f, FALSE); }
	BOOL TestFlag(USHORT f) const { return (flags & f) == f; };

	void CreateHandleNode(int refid, DWORD dwWireColor, INode* nodeparent = NULL);
	void CreateHandles(INode* pStrandNode);
	void RemoveHandles();

	void UpdateHandles(TimeValue t, Interval&valid);
	Point3	GetSegPos(TimeValue t, int vindex, int uindex, Interval&valid);

	void Create(INode* node);
	virtual void AssignSegTransTMController(INode *node);

	void SetValue(TimeValue t, SetXFormPacket *ptr, int commit, Control* pOriginator);

	// Loading/Saving
	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);

	////////////////////////////////////////////////////////////
	// For the procedural spine sub-objet selection
	static MoveModBoxCMode *moveMode;
	static SelectModBoxCMode *selMode;
	int subobject_level;
	Box3	bbox;

	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box);

	void	DrawStartGizmo(class GraphicsWindow *gw, int sphereid);
	void	DrawEndGizmo(class GraphicsWindow *gw, int sphereid);

	int		Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	int		NumSubObjTypes() { return 1; };
	ISubObjType *GetSubObjType(int i);

	int		HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);

	void	SelectSubComponent(HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert = FALSE);
	void	ClearSelection(int selLevel);
	void	SelectAll(int selLevel);
	void	InvertSelection(int selLevel);
	int		SubObjectIndex(HitRecord *hitRec);

	void	ActivateSubobjSel(int level, XFormModes& modes);
	void	GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node, ModContext *mc);
	void	GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node, ModContext *mc);

	DWORD	GetSubselState() { return subobject_level; };

	void	Move(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE);

	/////////////////////////////////////////////////////////////////

	void GetTrans(TimeValue t, int nStrand, int nSeg, Matrix3 &tm, Interval &valid);
	void UpdateMuscleStrand();
	void UpdateName(const MCHAR* nodeName);

	/////////////////////////////////////////////////////////
	//ICATMuscleClass

	// LMR is used to help in naming and pasting muscles.
	// Left = -1; Middle = 0; Right = 1;
	virtual int		GetLMR() { return lmr; };
	virtual void	SetLMR(int newlmr) { lmr = newlmr; };

	virtual TSTR	GetName() { return strName; };
	virtual void	SetName(const TSTR& newname);

	virtual Color*	GetColour() { return &clrColour; };
	virtual void	SetColour(Color* clr);

	DEFORMER_TYPE	GetDeformerType() { return deformer_type; };
	void			SetDeformerType(DEFORMER_TYPE type);

	virtual TSTR	IGetDeformerType();
	virtual void	ISetDeformerType(const TSTR& s);

	/////////////////////////////////////////////////////////
	// IMuscle Strand
	FPInterfaceDesc* GetDescByID(Interface_ID id);

	virtual Axis	GetMirrorAxis() { return mirroraxis; };
	virtual void	SetMirrorAxis(Axis newmirroraxis) { mirroraxis = newmirroraxis; };

	float GetHandleSize() { return handlesize; }
	void  SetHandleSize(float sz);

	Matrix3 GetLSHandleTM(int id, TimeValue t);
	void SetLSHandleTM(int id, TimeValue t, Matrix3 &tmm, BOOL bMirror);

	virtual BOOL	GetHandlesVisible() { return TestFlag(STRANDFLAG_HANDLES_VISIBLE); };
	virtual void	SetHandlesVisible(BOOL tf);

	virtual int		GetNumSpheres() { return mTabSphereParams.Count(); };
	virtual void	SetNumSpheres(int n);

	// access to the haqndle nodes
	virtual Tab <INode*>	GetHandles() { return tabHandles; };

	virtual float	GetSphereRadius(int id);//{ return tabSphereRadius[id]; }
//	virtual void	SetSphereRadius(int id, float v);
	virtual float	GetSphereUStart(int id) { return (id < GetNumSpheres()) ? mTabSphereParams[id].UStart : 0.0f; }
	virtual void	SetSphereUStart(int id, float v);
	virtual float	GetSphereUEnd(int id) { return (id < GetNumSpheres()) ? mTabSphereParams[id].UEnd : 0.0f; }
	virtual void	SetSphereUEnd(int id, float v);

	virtual float	GetLength();

	virtual BOOL	GetSquashStretch() { return TestFlag(STRANDFLAG_ENABLE_SQUASHSTRETCH); };
	virtual void	SetSquashStretch(BOOL tf) { SetFlag(STRANDFLAG_ENABLE_SQUASHSTRETCH, tf); };

	virtual float	GetCurrentLength() { return lengthCurrent; }
	virtual float	GetCurrentScale() { return scaleCurrent; }

	virtual float	GetDefaultLength() { return lengthShort; }
	virtual void	SetDefaultLength(float v);
	virtual float	GetSquashStretchScale() { return scaleShort; }
	virtual void	SetSquashStretchScale(float v);

	// Paste the settings from another muscle onto this muscle
	virtual void	PasteMuscleStrand(ReferenceTarget* pasteRef);

	//////////////////////////////////////////////////////////////////////////
	// From IDataRestoreOwner
	void OnRestoreDataChanged(float val)
	{
		SetHandleSize(val);
		RefreshUI();
	}
	void OnRestoreDataChanged(int val)
	{
		UpdateMuscleStrand();
		RefreshUI();
	}
	void OnRestoreDataChanged(TabSphereParams)
	{
		UpdateMuscleStrand();
		RefreshUI();
	}
};
