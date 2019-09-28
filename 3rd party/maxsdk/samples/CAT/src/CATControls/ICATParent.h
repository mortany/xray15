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

#include "../CATAPI.h"
#include "cat.h"

#include "FnPub/ICATParentFP.h"

#include "iparamb2.h"
#include "simpobj.h"

class CATNodeControl;
class CATCharacterRemap;

#define I_CATPARENTTRANS	0x51be6a8e

class ICATParentTransClassDesc : public ClassDesc2 {
public:
	virtual void *	CreateNew(class ECATParent *catparent, INode* node) = 0;
};

// our global instance of our classdesc class.
extern ICATParentTransClassDesc* GetCATParentTransDesc();

class ICATParentTrans : public Control, public ICATParentFP
{
public:
	virtual class ECATParent * GetCATParent() = 0;

	virtual void	AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt) = 0;
	virtual BOOL	BuildMapping(ICATParentTrans* pasteicatparenttrans, CATCharacterRemap &remap, BOOL includeERNnodes) = 0;

	virtual void	UpdateCharacter() = 0;

	virtual CATColourMode GetEffectiveColourMode() = 0;

	virtual Matrix3 GettmCATParent(TimeValue t) = 0;

	virtual void UpdateColours(BOOL bRedraw = TRUE, BOOL bRecalculate = TRUE) = 0;

	virtual INode* LoadClip(TSTR filename, int layerindex, int flags, TimeValue startT, class CATControl* bodypart) = 0;
	virtual BOOL SaveClip(TSTR filename, int flags, Interval timerange, Interval layerrange, class CATControl* bodypart) = 0;

	virtual BOOL SaveClip(TSTR filename, TimeValue start_t, TimeValue end_t, int from_layer, int to_layer) = 0;
	virtual BOOL SavePose(TSTR filename) = 0;

	virtual INode* LoadClip(TSTR filename, TimeValue t, BOOL scale_data, BOOL transformData, BOOL mirrorData, BOOL mirrorWorldX, BOOL mirrorWorldY) = 0;
	virtual INode* LoadPose(TSTR filename, TimeValue t, BOOL scale_data, BOOL transformData, BOOL mirrorData, BOOL mirrorWorldX, BOOL mirrorWorldY) = 0;
	virtual INode* CreatePasteLayerTransformNode() = 0;

	virtual void CollapsePoseToCurLayer(TimeValue t) = 0;
	virtual BOOL CollapseTimeRangeToLayer(TimeValue start_t, TimeValue end_t, TimeValue iKeyFreq, BOOL regularplot, int numpasses, float posdelta, float rotdelta) = 0;

	virtual void PasteLayer(INode* from, int fromindex, int toindex, BOOL instance) = 0;

	virtual TSTR GetFileTagValue(TSTR filename, TSTR tag) = 0;

	// The following template overload is there to catch functions that do not explicitly
	// specify a TimeValue for the first parameter.  Because the 2nd parameter is a UINT,
	// it will cast to TimeValue silently, which is a problem for us.
	template <typename T>
	void CATMessage(T t, UINT msg, int data = -1) { t.DoNotCompileYourCallingTheWrongFunction(); }

	virtual void	CATMessage(TimeValue t, UINT msg, int data = -1) = 0;

	///////////////////////////////////////////////
	// Used internally
	// When loading Max file we need to know what was the version of the saved file
	virtual DWORD GetFileSaveVersion() = 0;

	// We usually need to access a little more info on the hub that what is available on IHub
	// TODO: Remove this fn when we blend this class with the straight CATParentTrans
	virtual CATNodeControl* GetRootHub() = 0;

	// Access to the function publishing interfaces
	BaseInterface* GetInterface(Interface_ID id) {
		if (id == CATPARENT_INTERFACE_FP)	return (ICATParentFP*)this;
		return Control::GetInterface(id);
	}

	void* GetInterface(ULONG id) {
		if (id == I_CATPARENTTRANS)		return (void*)this;
		return Control::GetInterface(id);
	}

#ifdef FBIK
	///////////////////////////////////////////////
	// FBIK
	virtual void	EvaluateCharacter(TimeValue t) = 0;
#endif

};

#define CATPARENT_CLASS_ID	Class_ID(0x56ae72e5, 0x389b6659)
#define E_CATPARENT			0x61bb44c2

class ECATParent : public SimpleObject2 {
public:

	// Access to the function publishing interfaces
	BaseInterface* GetInterface(Interface_ID id) {
		return SimpleObject2::GetInterface(id);
	}

	// Access to our custome interface classes
	void* GetInterface(ULONG id) {
		if (id == E_CATPARENT) 	return (void*)this;
		return SimpleObject2::GetInterface(id);
	}

	virtual void	SetRigFilename(TSTR name) = 0;
	virtual TSTR	GetRigFilename() = 0;
	virtual BOOL	GetReloadRigOnSceneLoad() = 0;
	virtual void	SetReloadRigOnSceneLoad(BOOL tf) = 0;

	virtual void	SetRigFileModifiedTime(TSTR str) = 0;
	virtual TSTR	GetRigFileModifiedTime() = 0;

	virtual BOOL SaveMesh(class CATRigWriter* save) = 0;
	virtual BOOL LoadMesh(class CATRigReader* load) = 0;

	virtual ICATParentTrans* GetCATParentTrans() = 0;

	template <class T>
	void CATMessage(T t, UINT msg, int data /* = -1 */) { thisShouldNotCompile; }
	virtual void	CATMessage(TimeValue t, UINT msg, int data = -1) = 0;

	virtual DWORD GetFileSaveVersion() = 0;
};
