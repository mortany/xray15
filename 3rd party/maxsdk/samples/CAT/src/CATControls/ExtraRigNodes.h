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

//**************************************************************************/
// DESCRIPTION: A interface to be added to CAT controllers that manages "extra nodes"
//***************************************************************************/

#pragma once

#include "CATPlugins.h"
#include "FnPub/IExtraRigNodesFP.h"

#define I_EXTRARIGNODES		0x1d9846991

class CATCharacterRemap;

class ExtraRigNodes : public IExtraRigNodesFP
{
protected:

	// All our Arbitrary Bones
	Tab<ULONG>	tabExtraNodeHandles;

	HWND hERNWnd;

public:

	ExtraRigNodes();
	~ExtraRigNodes();

	//////////////////////////////////////////////////////////////////////////
	// Methods from IExtraRigNodesFP

	// 2 methods so we can add and remove groups of nodes at a time
	virtual void AddExtraRigNodes(Tab<INode*> newnodes);
	virtual void RemoveExtraRigNodes(Tab<INode*> removednodes);

	// Returns the array of Extra Rig nodes that we are currently storing
	virtual Tab<INode*>	IGetExtraRigNodes();
	virtual void		ISetExtraRigNodes(Tab<INode*> nodes);

	virtual int			INumExtraRigNodes() { return tabExtraNodeHandles.Count(); }
	virtual INode*		IGetExtraRigNode(int i);
	virtual void		IAddExtraRigNode(INode* nodes, int i);

	////////////////////////////////////////////////////////////////////////////
	// General Maintenance
	virtual	void AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	BOOL BuildMapping(ExtraRigNodes* pastectrl, CATCharacterRemap &remap);
	void PasteERNNodes(RemapDir &remap, ExtraRigNodes *pasteExtraNodes);

	void CloneERNNodes(ExtraRigNodes* clonedxrn, RemapDir& remap);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	//////////////////////////////////////////////////////////////////////////
	HWND CreateERNWindow(HWND hWndOwner);
	void UnlinkERNWindow() { hERNWnd = NULL; }
	virtual void ICreateERNWindow(HWND hWndOwner = NULL) { hERNWnd = CreateERNWindow(hWndOwner); };
	virtual void IDestroyERNWindow() { if (hERNWnd) DestroyWindow(hERNWnd); };

	virtual void ICreateERNRollout(IObjParam *ip);
	virtual void IDestroyERNRollout(IObjParam *ip);

	FPInterfaceDesc* GetDescByID(Interface_ID id);

	virtual void* GetInterface(ULONG id)
	{
		if (id == I_EXTRARIGNODES)	return (void*)this;
		return NULL;
	}
};
