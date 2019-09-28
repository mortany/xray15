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
#include <CATAPI/CATClassID.h>

////////////////////////////////////////////////////////////////
//
#define CATMUSCLE_SYSTEMMASTER_INTERFACE_ID Interface_ID(0x78054328, 0x507b443d)

class ICATMuscleSystemMaster : public BaseInterface {
public:
	virtual float GetHandleSize() = 0;
	virtual void RefreshUI() = 0;
	virtual void SetValue(TimeValue t, SetXFormPacket *ptr, int commit, Control* pOriginator) = 0;
	virtual void GetTrans(TimeValue t, int nStrand, int nSeg, Matrix3 &tm, Interval &valid) = 0;
};

#define  GetCATMuscleSystemInterface(ref) ((ICATMuscleSystemMaster*)ref->GetInterface(CATMUSCLE_SYSTEMMASTER_INTERFACE_ID))

/**********************************************************************
 * ICATMuscleClass : Superclass for CATMuscle Objects
 */
const TSTR muscle_bones_str = _T("Bones");  // don't globalize - used for mxs  SA 10/09
const TSTR muscle_patch_str = _T("Patch");

class ICATMuscleClass : public FPMixinInterface
{
public:
	enum DEFORMER_TYPE {
		DEFORMER_MESH,
		DEFORMER_BONES
	};

	virtual TSTR	IGetDeformerType() = 0;
	virtual void	ISetDeformerType(const TSTR& s) = 0;

	// LMR is used to help in naming and pasting muscles.
	// Left = -1; Middle = 0; Right = 1;
	virtual int		GetLMR() = 0;
	virtual void	SetLMR(int lmr) = 0;

	virtual TSTR	GetName() = 0;
	virtual void	SetName(const TSTR& newname) = 0;

	virtual Color*	GetColour() = 0;
	virtual void	SetColour(Color* clr) = 0;

	virtual float	GetHandleSize() = 0;
	virtual void	SetHandleSize(float sz) = 0;

	virtual BOOL	GetHandlesVisible() = 0;
	virtual void	SetHandlesVisible(BOOL tf) = 0;

	// access to the handle nodes
	virtual Tab <INode*>	GetHandles() = 0;

};

/**********************************************************************
 * IMuscle: Published functions for CATMuscles
 */
#define CATMUSCLE_INTERFACE_ID Interface_ID(0x68054328, 0x507b443d)
#define GetCATMuscleInterface(cd) ((IMuscle*)cd->GetInterface(CATMUSCLE_INTERFACE_ID))

#pragma warning(push)
#pragma warning(disable:4238) // necessary for _BV types

class IMuscle : public ICATMuscleClass {
public:

	virtual int		GetNumVSegs() = 0;
	virtual void	SetNumVSegs(int n) = 0;
	virtual int		GetNumUSegs() = 0;
	virtual void	SetNumUSegs(int n) = 0;

	virtual BOOL	GetMiddleHandles() = 0;
	virtual void	SetMiddleHandles(BOOL tf) = 0;

	// Paste the settings from another muscle onto this muscle
	virtual void	PasteMuscle(ReferenceTarget* pasteRef) = 0;

	// Collision detection stuff
	virtual void	AddColObj(INode *node) = 0;

	virtual void	RemoveColObj(int n) = 0;
	virtual int		GetNumColObjs() = 0;
	virtual INode*	GetColObj(int index) = 0;

	inline float GetColObjDistortion(int n, TimeValue t) { Interval iv = FOREVER; return GetColObjDistortion(n, t, iv); }
	virtual float	GetColObjDistortion(int n, TimeValue t, Interval& valid) = 0;
	virtual void	SetColObjDistortion(int n, float newVal, TimeValue t) = 0;

	inline float GetColObjHardness(int n, TimeValue t) { Interval iv = FOREVER; return GetColObjHardness(n, t, iv); }
	virtual float	GetColObjHardness(int n, TimeValue t, Interval& valid) = 0;
	virtual void	SetColObjHardness(int n, float newVal, TimeValue t) = 0;

	virtual BOOL	GetObjXDistortion(int n) = 0;
	virtual void	SetObjXDistortion(int n, BOOL tf) = 0;
	virtual BOOL	GetInvertCollision(int n) = 0;
	virtual void	SetInvertCollision(int n, BOOL tf) = 0;
	virtual BOOL	GetSmoothCollision(int n) = 0;
	virtual void	SetSmoothCollision(int n, BOOL tf) = 0;

	virtual void	MoveColObjUp(int n) = 0;
	virtual void	MoveColObjDown(int n) = 0;

	/////////////////////////////////////////////////////////////////
	FPInterfaceDesc* GetDesc() { return GetDescByID(CATMUSCLE_INTERFACE_ID); }

	enum {

		fnAddColObj,
		fnRemoveColObj,
		fnGetColObj,

		fnGetColObjDistortion,
		fnSetColObjDistortion,

		fnGetColObjHardness,
		fnSetColObjHardness,

		fnMoveColObjUp,
		fnMoveColObjDown,

		fnPasteMuscle,

		// properties
		fnGetDeformerType,
		fnSetDeformerType,

		fnGetName,
		fnSetName,

		fnGetColour,
		fnSetColour,

		fnGetLMR,
		fnSetLMR,

		fnGetNumVSegs,
		fnSetNumVSegs,

		fnGetNumUSegs,
		fnSetNumUSegs,

		fnGetHandleSize,
		fnSetHandleSize,

		fnGetHandleVis,
		fnSetHandleVis,

		fnGetMiddleHandle,
		fnSetMiddleHandle,

		fnGetHandles,

		fnGetNumColObjs
	};

	BEGIN_FUNCTION_MAP

		VFN_1(fnAddColObj, AddColObj, TYPE_INODE);
		VFN_1(fnRemoveColObj, RemoveColObj, TYPE_INT);
		FN_1(fnGetColObj, TYPE_INODE, GetColObj, TYPE_INT);

		FNT_1(fnGetColObjDistortion, TYPE_FLOAT, GetColObjDistortion, TYPE_INT);
		VFNT_2(fnSetColObjDistortion, SetColObjDistortion, TYPE_INT, TYPE_FLOAT);

		FNT_1(fnGetColObjHardness, TYPE_FLOAT, GetColObjHardness, TYPE_INT);
		VFNT_2(fnSetColObjHardness, SetColObjHardness, TYPE_INT, TYPE_FLOAT);

		VFN_1(fnMoveColObjUp, MoveColObjUp, TYPE_INT);
		VFN_1(fnMoveColObjDown, MoveColObjDown, TYPE_INT);

		VFN_1(fnPasteMuscle, PasteMuscle, TYPE_REFTARG);

		//////////////////////////////////////////////////////////////////////////
		// properties
		PROP_FNS(fnGetDeformerType, IGetDeformerType, fnSetDeformerType, ISetDeformerType, TYPE_TSTR_BV);

		PROP_FNS(fnGetName, GetName, fnSetName, SetName, TYPE_TSTR_BV);
		PROP_FNS(fnGetColour, GetColour, fnSetColour, SetColour, TYPE_COLOR);
		PROP_FNS(fnGetLMR, GetLMR, fnSetLMR, SetLMR, TYPE_INT);
		PROP_FNS(fnGetNumVSegs, GetNumVSegs, fnSetNumVSegs, SetNumVSegs, TYPE_INT);
		PROP_FNS(fnGetNumUSegs, GetNumUSegs, fnSetNumUSegs, SetNumUSegs, TYPE_INT);

		PROP_FNS(fnGetHandleSize, GetHandleSize, fnSetHandleSize, SetHandleSize, TYPE_FLOAT);
		PROP_FNS(fnGetHandleVis, GetHandlesVisible, fnSetHandleVis, SetHandlesVisible, TYPE_BOOL);
		PROP_FNS(fnGetMiddleHandle, GetMiddleHandles, fnSetMiddleHandle, SetMiddleHandles, TYPE_BOOL);

		RO_PROP_FN(fnGetHandles, GetHandles, TYPE_INODE_TAB_BV);
		RO_PROP_FN(fnGetNumColObjs, GetNumColObjs, TYPE_INT);

	END_FUNCTION_MAP

};

/**********************************************************************
 * IHdlTrans: Published functions for IHdlTrans
 */
#define IHDLTRANS_INTERFACE_ID Interface_ID(0x24054328, 0x507b443d)

class IHdlTrans : public FPMixinInterface {
public:

	FPInterfaceDesc* GetDesc() { return GetDescByID(IHDLTRANS_INTERFACE_ID); }

};

/**********************************************************************
 * IMuscle: Published functions for CATMuscles
 */
#define CATMUSCLESTRAND_INTERFACE_ID Interface_ID(0x68054328, 0x507b443d)
#define GetCATMuscleStrandInterface(cd) ((IMuscle*)cd->GetInterface(CATMUSCLESTRAND_INTERFACE_ID))

class IMuscleStrand : public ICATMuscleClass {
public:

	virtual int		GetNumSpheres() = 0;
	virtual void	SetNumSpheres(int n) = 0;

	virtual float	GetSphereRadius(int id) = 0;
	//	virtual void	SetSphereRadius(int id, float v)=0;;
	virtual float	GetSphereUStart(int id) = 0;
	virtual void	SetSphereUStart(int id, float v) = 0;
	virtual float	GetSphereUEnd(int id) = 0;
	virtual void	SetSphereUEnd(int id, float v) = 0;;

	virtual BOOL	GetSquashStretch() = 0;
	virtual void	SetSquashStretch(BOOL tf) = 0;;

	virtual float	GetCurrentLength() = 0;
	virtual float	GetCurrentScale() = 0;

	virtual float	GetDefaultLength() = 0;
	virtual void	SetDefaultLength(float v) = 0;
	virtual float	GetSquashStretchScale() = 0;
	virtual void	SetSquashStretchScale(float v) = 0;

	//	virtual float	GetLongLength()=0;
	//	virtual void	SetLongLength(float v)=0;;
	//	virtual float	GetLongScale()=0;
	//	virtual void	SetLongScale(float v)=0;;

		// Paste the settings from another muscle onto this muscle
	virtual void	PasteMuscleStrand(ReferenceTarget* pasteRef) = 0;

	enum {
		fnPasteStrand,
		fnGetSphereRadius,
		fnSetSphereRadius,

		fnGetSphereUStart,
		fnSetSphereUStart,
		fnGetSphereUEnd,
		fnSetSphereUEnd,

		// properties
		fnGetLMR,
		fnSetLMR,

		fnGetHandleSize,
		fnSetHandleSize,

		fnGetHandleVis,
		fnSetHandleVis,

		fnGetHandles,

		fnGetNumSpheres,
		fnSetNumSpheres,

		fnGetCurrentLength,
		fnGetCurrentScale,

		fnGetSquashStretch,
		fnSetSquashStretch,

		fnGetDefaultLength,
		fnSetDefaultLength,
		fnGetSquashStretchScale,
		fnSetSquashStretchScale,
	};

	BEGIN_FUNCTION_MAP
		VFN_1(fnPasteStrand, PasteMuscleStrand, TYPE_REFTARG);
		FN_1(fnGetSphereRadius, TYPE_FLOAT, GetSphereRadius, TYPE_INT);
		//	VFN_2(fnSetSphereRadius,						SetSphereRadius,		TYPE_INT,		TYPE_FLOAT);

		FN_1(fnGetSphereUStart, TYPE_FLOAT, GetSphereUStart, TYPE_INT);
		VFN_2(fnSetSphereUStart, SetSphereUStart, TYPE_INT, TYPE_FLOAT);
		FN_1(fnGetSphereUEnd, TYPE_FLOAT, GetSphereUEnd, TYPE_INT);
		VFN_2(fnSetSphereUEnd, SetSphereUEnd, TYPE_INT, TYPE_FLOAT);

		//////////////////////////////////////////////////////////////////////////
		// properties
		PROP_FNS(fnGetLMR, GetLMR, fnSetLMR, SetLMR, TYPE_INT);
		PROP_FNS(fnGetHandleSize, GetHandleSize, fnSetHandleSize, SetHandleSize, TYPE_FLOAT);
		PROP_FNS(fnGetHandleVis, GetHandlesVisible, fnSetHandleVis, SetHandlesVisible, TYPE_BOOL);

		//	RO_PROP_FN(fnGetSt,				GetSt,		TYPE_INODE);
		//	RO_PROP_FN(fnGetStHdl,			GetStHdl,	TYPE_INODE);

		//	RO_PROP_FN(fnGetEn,				GetEn,		TYPE_INODE);
		//	RO_PROP_FN(fnGetEnHdl,			GetEnHdl,	TYPE_INODE);
		RO_PROP_FN(fnGetHandles, GetHandles, TYPE_INODE_TAB_BV);

		PROP_FNS(fnGetNumSpheres, GetNumSpheres, fnSetNumSpheres, SetNumSpheres, TYPE_INT);

		RO_PROP_FN(fnGetCurrentLength, GetCurrentLength, TYPE_FLOAT);
		RO_PROP_FN(fnGetCurrentScale, GetCurrentScale, TYPE_FLOAT);
		PROP_FNS(fnGetSquashStretch, GetSquashStretch, fnSetSquashStretch, SetSquashStretch, TYPE_BOOL);
		PROP_FNS(fnGetDefaultLength, GetDefaultLength, fnSetDefaultLength, SetDefaultLength, TYPE_FLOAT);
		PROP_FNS(fnGetSquashStretchScale, GetSquashStretchScale, fnSetSquashStretchScale, SetSquashStretchScale, TYPE_FLOAT);

	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc() { return GetDescByID(CATMUSCLESTRAND_INTERFACE_ID); }
};
#pragma warning(pop)
/**********************************************************************
 * IMuscle: Published functions for CATMuscles
 */
#define SPRINGMASTER_INTERFACE_ID Interface_ID(0x19787c4e, 0x41bc4ad5)
#define GetSpringMasterInterface(cd) ((ISpringMaster*)cd->GetInterface(SPRINGMASTER_INTERFACE_ID))

class ISpringMaster : public FPMixinInterface {
public:

	// Paste the settings from another muscle onto this muscle
	virtual int  AddSpring(Control* springtrans, INode* node) = 0;
	virtual int  AddSpring(Control* springtrans1, Control* springtrans2) = 0;
	virtual int  NumSprings() = 0;

	virtual BOOL GetFBIK() = 0;
	virtual void SetFBIK(BOOL b) = 0;

	FPInterfaceDesc* GetDesc() { return GetDescByID(SPRINGMASTER_INTERFACE_ID); }

	enum {
		fnAddNodeSpring,
		fnAddSpring,
		propNumSprings,
		propGetFBIK,
		propSetFBIK
	};

	BEGIN_FUNCTION_MAP
		VFN_2(fnAddNodeSpring, AddSpring, TYPE_CONTROL, TYPE_INODE);
		VFN_2(fnAddSpring, AddSpring, TYPE_CONTROL, TYPE_CONTROL);
		RO_PROP_FN(propNumSprings, NumSprings, TYPE_INT);
		PROP_FNS(propGetFBIK, GetFBIK, propSetFBIK, SetFBIK, TYPE_BOOL);
	END_FUNCTION_MAP

};
