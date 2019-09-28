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

#include "ExtraRigNodes.h"
#include "CATNodeControl.h"
#include "CATCharacterRemap.h"

/////////////////////////////////////////////////////////////////////////////////////
//
ExtraRigNodes::ExtraRigNodes()
	: hERNWnd(NULL)
{
}

ExtraRigNodes::~ExtraRigNodes() {}

void ExtraRigNodes::RemoveExtraRigNodes(Tab<INode*> removednodes)
{
	// Expand the systems
	Tab<ULONG> removedHandles;
	removedHandles.Resize(removednodes.Count());
	for (int i = 0; i < removednodes.Count(); i++)
	{
		INode *extranode = removednodes[i];

		if (!extranode)
			continue;

		// Allow nodes scheduled for deletion to be removed.
		ULONG hExtraNode = extranode->GetHandle();
		removedHandles.Append(1, &hExtraNode);

		// Only search for systems on live nodes
		if (extranode->TestAFlag(A_IS_DELETED) || !extranode->GetTMController())
			continue;

		// if a master controller is defined, remove all children as well.
		ReferenceTarget* pMaster = (ReferenceTarget*)extranode->GetTMController()->GetInterface(I_MASTER);
		if (pMaster != NULL)
		{
			INodeTab extranode_systemnodes;
			pMaster->GetSystemNodes(extranode_systemnodes, kSNCFileSave);
			if (extranode_systemnodes.Count() > 0)
			{
				// Append ALL new nodes, duplicates are fine here.
				for (int j = 0; j < extranode_systemnodes.Count(); j++)
				{
					// Check to see if this node has been added.
					INode* pNewNode = extranode_systemnodes[j];
					if (pNewNode == NULL)
						continue;

					ULONG hNewNode = pNewNode->GetHandle();
					removedHandles.Append(1, &hNewNode);
				}
			}
		}
	}

	// now remove the selected nodes
	int iCurIdx = 0;
	int iCurCount = tabExtraNodeHandles.Count();
	for (int i = 0; i < iCurCount; i++)
	{
		BOOL removed = FALSE;
		for (int j = 0; j < removedHandles.Count(); j++)
		{
			if (tabExtraNodeHandles[i] == removedHandles[j])
			{
				removed = TRUE;
				break;
			}
		}
		// If this node has NOT been removed, then keep it in the tab.
		if (!removed)
			tabExtraNodeHandles[iCurIdx++] = tabExtraNodeHandles[i];

		// Safety first - we should never exceed this number.
		DbgAssert(iCurIdx <= iCurCount);
	}
	// finally, anything beyond the iCurIdx has been deleted, trim the array.
	tabExtraNodeHandles.SetCount(iCurIdx);
}

void ExtraRigNodes::AddExtraRigNodes(Tab<INode*> newnodes)
{
	// Expand the systems
	for (int i = 0; i < newnodes.Count(); i++) {
		INode *extranode = newnodes[i];

		if (!extranode)
			continue;

		// if a master controller is defined, remove all children as well.
		ReferenceTarget* pMaster = (ReferenceTarget*)extranode->GetTMController()->GetInterface(I_MASTER);
		if (pMaster)
		{
			INodeTab extranode_systemnodes;
			// Just add all new nodes
			pMaster->GetSystemNodes(extranode_systemnodes, kSNCFileSave);
			if (extranode_systemnodes.Count() > 0)
				newnodes.Append(extranode_systemnodes.Count(), extranode_systemnodes.Addr(0));
		}
	}

	// Make sure this ERN has not been added to another bone already
	INodeTab tabNodes;
	CATNodeControl *cnc = (CATNodeControl*)GetInterface(I_CATNODECONTROL);
	if (cnc)
		cnc->GetCATParentTrans()->AddSystemNodes(tabNodes, kSNCFileSave);
	else
		AddSystemNodes(tabNodes, kSNCFileSave);

	// now add the nodes
	for (int i = 0; i < newnodes.Count(); i++)
	{
		if (newnodes[i] == NULL)
			continue;

		BOOL nodeadded = FALSE;

		// Is ERN on another bone?
		for (int k = 0; k < tabNodes.Count(); k++)
		{
			if (tabNodes[k] == newnodes[i])
			{
				nodeadded = TRUE;
				break;
			}
		}

		ULONG hNewNode = newnodes[i]->GetHandle();
		if (!nodeadded)
		{
			// Is the bone on this node?
			for (int k = 0; k < tabExtraNodeHandles.Count(); k++)
			{
				if (hNewNode == tabExtraNodeHandles[k])
				{
					nodeadded = TRUE;
					break;
				}
			}
		}

		if (!nodeadded)
			tabExtraNodeHandles.Append(1, &hNewNode);
	}
}

void ExtraRigNodes::AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	int nExtraNodes = INumExtraRigNodes();
	for (int i = 0; i < nExtraNodes; i++)
	{
		INode *pNode = IGetExtraRigNode(i);
		if (pNode != NULL)
		{
			nodes.AppendNode(pNode);

			// if a master controller is defined, add all children as well.
			ReferenceTarget* pMaster = (ReferenceTarget*)pNode->GetTMController()->GetInterface(I_MASTER);
			if (pMaster != NULL)
			{
				// Expand the system
				INodeTab extranode_systemnodes;
				pMaster->GetSystemNodes(extranode_systemnodes, ctxt);
				for (int j = 0; j < extranode_systemnodes.Count(); ++j)
				{
					INode* aNode = extranode_systemnodes[j];
					nodes.AppendNode(pNode);
				}
			}
		}
	}
}

void ExtraRigNodes::ISetExtraRigNodes(Tab<INode*> nodes)
{
	tabExtraNodeHandles.SetCount(0);
	tabExtraNodeHandles.Resize(nodes.Count());
	for (int j = 0; j < nodes.Count(); j++)
	{
		if (nodes[j] != NULL)
		{
			ULONG hdl = nodes[j]->GetHandle();
			tabExtraNodeHandles.Append(1, &(hdl));
		}
	}
	tabExtraNodeHandles.Shrink();
};

Tab<INode*>	ExtraRigNodes::IGetExtraRigNodes()
{
	Tab<INode*> dAllERNodes;
	int nERNodes = tabExtraNodeHandles.Count();
	Interface* pCore = GetCOREInterface();

	// Convert all stored handles to pointers, but compact the array
	dAllERNodes.Resize(nERNodes);
	for (int i = 0; i < nERNodes; i++)
	{
		INode* pNode = pCore->GetINodeByHandle(tabExtraNodeHandles[i]);
		if (pNode != NULL)
			dAllERNodes.Append(1, &pNode);
	}
	return dAllERNodes;
};

INode* ExtraRigNodes::IGetExtraRigNode(int i)
{
	// Out of range
	if (i < 0 || i >= tabExtraNodeHandles.Count())
		return NULL;

	return GetCOREInterface()->GetINodeByHandle(tabExtraNodeHandles[i]);
}

void ExtraRigNodes::IAddExtraRigNode(INode* node, int)
{
	if (node != NULL)
	{
		ULONG hdl = node->GetHandle();

		for (int i = 0; i < tabExtraNodeHandles.Count(); i++)
		{
			if (tabExtraNodeHandles[i] == hdl)
				return; // already added, job done
		}
		tabExtraNodeHandles.Append(1, &(hdl));
	}
}

BOOL ExtraRigNodes::BuildMapping(ExtraRigNodes* srcctrl, CATCharacterRemap &remap)
{
	if (srcctrl == NULL)
		return FALSE;

	Tab<INode*> srcExtraNodes = srcctrl->IGetExtraRigNodes();
	Tab<INode*> dstExtraNodes = IGetExtraRigNodes();

	remap.BuildMapping(srcExtraNodes, dstExtraNodes);
	return TRUE;
}

class PostPatchERNodes : public PostPatchProc
{
private:
	Tab<INode*> mdAllERNodes;
	ExtraRigNodes* mpClonedExtraRigNodes;

public:
	PostPatchERNodes(Tab<INode*>& dERNodes, ExtraRigNodes* pERNodeClass)
		: mdAllERNodes(dERNodes)
		, mpClonedExtraRigNodes(pERNodeClass)
	{
		DbgAssert(pERNodeClass != NULL);
	}

	~PostPatchERNodes() {};

	// Our proc needs to set the cloned handles on the Cloned ERNode class
	int Proc(RemapDir& remap)
	{
		// First, replace all the original node pointers with the cloned versions
		for (int i = 0; i < mdAllERNodes.Count(); i++)
		{
			INode* pNode = (INode*)remap.FindMapping(mdAllERNodes[i]);
			// If the node wasn't cloned, keep the original pointer
			if (pNode != NULL)
				mdAllERNodes[i] = pNode;
		}

		// Set the list on the ERN window.
		mpClonedExtraRigNodes->ISetExtraRigNodes(mdAllERNodes);
		return TRUE;
	}
};

void ExtraRigNodes::CloneERNNodes(ExtraRigNodes* clonedxrn, RemapDir& remap)
{
	Tab<INode*> dAllERNodes = IGetExtraRigNodes();
	remap.AddPostPatchProc(new PostPatchERNodes(dAllERNodes, clonedxrn), true);
}

void ExtraRigNodes::PasteERNNodes(RemapDir &remap, ExtraRigNodes *pasteExtraNodes)
{
	UNREFERENCED_PARAMETER(remap); UNREFERENCED_PARAMETER(pasteExtraNodes);
	DbgAssert("This doens't work yet.");
	return;

#ifdef JUNK_CODE
	Interface *ip = GetCOREInterface();

	INodeTab pasteERNodes;
	pasteExtraNodes->ExtraRigNodes::AddSystemNodes(pasteERNodes, kSNCClone);
	int numERN = pasteERNodes.Count();

	INodeTab currERNodes;
	ExtraRigNodes::AddSystemNodes(currERNodes, kSNCClone);
	int numCurrentERN = currERNodes.Count();

	/*	if(numERN < numCurrentERN){
			int selectioncount = ip->GetSelNodeCount();
			for(int i=(currERNodes.Count()-1); i>=numERN; i--){
				if(currERNodes[i]){
					if ((selectioncount == 1) && (currERNodes[i] == ip->GetSelNode(0)))
						ip->DeSelectNode(currERNodes[i]);
					// These nodes are simply being deleted and not replaced
					currERNodes[i]->Delete(0, FALSE);
				}
			}
		}
	*/
	currERNodes.SetCount(numERN);
	if (numERN == 0) return;

	/*	INodeTab clonednodes;
		INodeTab resultSource;
		INodeTab resultTarget;
		for(int i=0; i<numERN; i++){
			clonednodes.Append( 1, &pasteERNodes[i] );
		}

		Point3 offset(0,0,0);
		bool expandHierarchies = true;
		CloneType cloneType = NODE_COPY;
		if(ip->CloneNodes(clonednodes, offset, expandHierarchies, cloneType, &resultSource, &resultTarget)){

			for(int i=0; i<resultTarget.Count(); i++){
				// Reparent this object onto the remapped parent
				INode* parentnode = (INode*)remap.FindMapping(resultSource[i]->GetParentNode());
				if(parentnode){
					parentnode->AttachChild(resultTarget[i], FALSE);
				}
			}

			AddExtraRigNodes(resultTarget);
		}
	*/
	INodeTab resultTarget;

	for (int i = numCurrentERN; i < numERN; i++) {
		Object *tempobj = (Object*)CreateInstance(HELPER_CLASS_ID, Class_ID(POINTHELP_CLASS_ID, 0));
		INode *node = GetCOREInterface()->CreateObjectNode(tempobj);
		remap.AddEntry(pasteERNodes[i], node);

		TSTR name = pasteERNodes[i]->GetName();
		ip->MakeNameUnique(name);
		node->SetName(name);

		Control *ctrl = (Control*)remap.CloneRef(pasteERNodes[i]->GetTMController());
		Object *obj = (Object*)remap.CloneRef(pasteERNodes[i]->GetObjectRef());

		node->SetTMController(ctrl);
		node->SetObjectRef(obj);

		// Reparent this object onto the remapped parent
		INode* parentnode = (INode*)remap.FindMapping(pasteERNodes[i]->GetParentNode());
		if (parentnode) {
			parentnode->AttachChild(node, FALSE);
		}

		resultTarget.Append(1, &node);
	}

	for (int i = 0; i < resultTarget.Count(); i++) {
		resultTarget[i]->GetTMController()->PostCloneNode();
	}

	AddExtraRigNodes(resultTarget);
#endif

};

//////////////////////////////////////////////////////////////////////
// Loading/Saving
//

#define		ERN_NUM_NODES		1
#define		ERN_NODES			2

class ExtraRigNodesPLCB : public PostLoadCallback {
protected:
	ExtraRigNodes *xrn;
	Tab<INode*> mdAllERNodes;

public:
	ExtraRigNodesPLCB(ExtraRigNodes *pOwner) { xrn = pOwner; }

	int Priority() { return 5; }

	void proc(ILoad *) {

		// This will reload all the handles. If the file was merged and the handles all need to be remapped
		xrn->ISetExtraRigNodes(mdAllERNodes);

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}

	IOResult Load(ILoad* iload)
	{
		IOResult res = IO_OK;
		DWORD nb;
		ULONG id;
		int nNodes = 0;

		while (IO_OK == (res = iload->OpenChunk())) {
			switch (iload->CurChunkID()) {
			case ERN_NUM_NODES: {
				res = iload->Read(&nNodes, sizeof(int), &nb);
				mdAllERNodes.SetCount(nNodes);
				for (int i = 0; i < nNodes; i++)
					mdAllERNodes[i] = NULL;
				break;
			}
			case ERN_NODES:
				for (int i = 0; i < nNodes; i++) {
					res = iload->Read(&id, sizeof(ULONG), &nb);
					if (res == IO_OK && id != 0xffffffff)
						iload->RecordBackpatch(id, (void**)mdAllERNodes.Addr(i));
				}
				break;
			}
			iload->CloseChunk();
			if (res != IO_OK) return res;
		}
		return res;
	}
};

IOResult ExtraRigNodes::Save(ISave *isave)
{
	DWORD nb, refID;

	// Save out the compact version of our ERN nodes.
	Tab<INode*> dAllERNodes = IGetExtraRigNodes();
	int nNumNodes = dAllERNodes.Count();

	isave->BeginChunk(ERN_NUM_NODES);
	isave->Write(&nNumNodes, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(ERN_NODES);
	for (int i = 0; i < nNumNodes; i++)
	{
		refID = isave->GetRefID(dAllERNodes[i]);
		isave->Write(&refID, sizeof(ULONG), &nb);
	}
	isave->EndChunk();

	// Compact our own list, as saving clears the undo buffer,
	// and any deleted nodes are now gone for good.
	// We can do this by simply setting the compact array
	// back onto our class.
	if (nNumNodes != tabExtraNodeHandles.Count())
		ISetExtraRigNodes(dAllERNodes);

	return IO_OK;
}

IOResult ExtraRigNodes::Load(ILoad *iload)
{
	ExtraRigNodesPLCB* pNewPLCB = new ExtraRigNodesPLCB(this);

	// Our PLCB actually recieves all the data to be loaded, so
	// we just hand off loading to it.  This is because the pointers
	// to be loaded are only stored there (our handles are not constant)
	IOResult res = pNewPLCB->Load(iload);
	if (res != IO_ERROR)
		iload->RegisterPostLoadCallback(pNewPLCB);
	else
		delete pNewPLCB;

	return res;
}

//////////////////////////////////////////////////////////////////////////////////////
//
class SelectExtraNodes : public HitByNameDlgCallback
{
private:
	ExtraRigNodes *xrn;
public:
	const MCHAR *dialogTitle() { return GetString(IDS_PICK_EXTRA_NODES); }
	virtual const MCHAR *buttonText() { return GetString(IDS_PICKNODES); }
	virtual BOOL singleSelect() { return FALSE; }
	virtual BOOL useFilter() { return TRUE; }
	virtual int filter(INode *node)
	{
		DbgAssert(xrn);
		INodeTab tabNodes;
		CATNodeControl *cnc = (CATNodeControl*)xrn->GetInterface(I_CATNODECONTROL);
		if (cnc) {
			cnc->GetCATParentTrans()->AddSystemNodes(tabNodes, kSNCFileSave);
		}
		else {
			xrn->AddSystemNodes(tabNodes, kSNCFileSave);
		}
		int k;
		for (k = 0; k < tabNodes.Count(); k++) {
			if (tabNodes[k] == node)	return 0;
		}

#ifdef CAT_DEBUG
		TSTR nodename = node->GetName();
		TSTR lastnodename;
		if (k > 0) lastnodename = tabNodes[k - 1]->GetName();
#endif
		return 1;
	}
	void Setup(ExtraRigNodes *xrn) { this->xrn = xrn; }
	void proc(INodeTab &nodeTab) {
		xrn->AddExtraRigNodes(nodeTab);
	}
};

static SelectExtraNodes select_xrnodes;

//
// Superclass for any CAT Window Pane.
//
class ERNWindow : public CATDialog
{
protected:
	void InitControls();

	WORD GetDialogTemplateID() const { return IDD_EXTRABONES_WINDOW; }
	HWND hParent;

	ExtraRigNodes *xrn;

	HWND lbxExtraNodesList;
	ICustButton* btnAddExtraNode;
	ICustButton* btnRemoveExtraNode;

public:
	ERNWindow() : btnRemoveExtraNode(NULL)
		, btnAddExtraNode(NULL)
		, lbxExtraNodesList(NULL)
	{
		hParent = NULL;
		this->xrn = NULL;
	}
	ERNWindow(ExtraRigNodes *xrn) : CATDialog()
		, btnRemoveExtraNode(NULL)
		, btnAddExtraNode(NULL)
		, lbxExtraNodesList(NULL) {
		hParent = NULL;
		this->xrn = xrn;
	}

	void ReleaseControls();
	void DoSelectionChanged();
	void RemoveSelectedItems();
	void RefreshExtraNodesList(TCHAR *currentFile = NULL);
	void Refresh();

	// The dialog callback is where all the fun stuff goes down...
	INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

};

void ERNWindow::InitControls()
{
	// Initialise the INI files so we can read button text and tooltips
	CatDotIni catCfg;

	lbxExtraNodesList = GetDlgItem(hDlg, IDC_XRN_LIST);

	btnAddExtraNode = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADD));
	btnAddExtraNode->SetType(CBT_PUSH);
	btnAddExtraNode->SetButtonDownNotify(TRUE);

	btnRemoveExtraNode = GetICustButton(GetDlgItem(hDlg, IDC_BTN_REMOVE));
	btnRemoveExtraNode->SetType(CBT_PUSH);
	btnRemoveExtraNode->SetButtonDownNotify(TRUE);
	btnRemoveExtraNode->SetImage(hIcons, 7, 7, 7 + 25, 7 + 25, 24, 24);
}

void ERNWindow::ReleaseControls()
{
	SAFE_RELEASE_BTN(btnAddExtraNode);
	SAFE_RELEASE_BTN(btnRemoveExtraNode);

	if (xrn != NULL)
		xrn->UnlinkERNWindow();

	hDlg = NULL;
	xrn = NULL;
}

void ERNWindow::DoSelectionChanged()
{
	int item = (int)SendMessage(lbxExtraNodesList, LB_GETCURSEL, 0, 0);
	if (item != LB_ERR) {
		btnRemoveExtraNode->Enable();
	}
	else {
		btnRemoveExtraNode->Disable();
	}
}
void ERNWindow::RemoveSelectedItems()
{
	int nNumSel = (int)SendMessage(lbxExtraNodesList, LB_GETSELCOUNT, 0, 0);
	if (nNumSel == 0)
		return;

	Tab<LONG> tabSelectedNodes;
	tabSelectedNodes.SetCount(nNumSel);
	SendMessage(lbxExtraNodesList, LB_GETSELITEMS, (WPARAM)nNumSel, (LPARAM)tabSelectedNodes.Addr(0));
	INodeTab nodes;
	INode *xr_node;
	Tab<INode*> dAllERNodes = xrn->IGetExtraRigNodes();
	for (int i = 0; i < nNumSel; i++)
	{
		xr_node = dAllERNodes[tabSelectedNodes[i]];
		DbgAssert(xr_node != NULL);
		if (!xr_node || xr_node->TestAFlag(A_IS_DELETED) || !xr_node->GetTMController()) continue;
		nodes.Append(1, &xr_node);
	}
	xrn->RemoveExtraRigNodes(nodes);
}

void ERNWindow::RefreshExtraNodesList(TCHAR *currentFile/*=NULL*/)
{
	UNREFERENCED_PARAMETER(currentFile);
	SendMessage(lbxExtraNodesList, LB_RESETCONTENT, 0, 0);

	Tab<INode*> dAllERNodes = xrn->IGetExtraRigNodes();
	for (int i = 0; i < dAllERNodes.Count(); i++)
	{
		INode* xr_node = dAllERNodes[i];
		DbgAssert(xr_node != NULL);
		if (!xr_node || xr_node->TestAFlag(A_IS_DELETED) || !xr_node->GetTMController()) continue;
		TSTR xrn_name = xr_node->GetName();
		SendMessage(lbxExtraNodesList, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)xrn_name.data());
	}
}

void ERNWindow::Refresh()
{
	RefreshExtraNodesList();
}

INT_PTR CALLBACK ERNWindow::DialogProc(HWND /*hDlg*/, UINT uMsg, WPARAM wParam, LPARAM)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		InitControls();
		RefreshExtraNodesList();
		DoSelectionChanged();
		return FALSE;
	case WM_DESTROY:
		ReleaseControls();
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_XRN_LIST:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				DoSelectionChanged();
				break;
			case LBN_DBLCLK:
				break;
			}
		case IDC_BTN_ADD:
			switch (HIWORD(wParam)) {
			case BN_BUTTONUP:
				select_xrnodes.Setup(xrn);
				GetCOREInterface()->DoHitByNameDialog(&select_xrnodes);
				RefreshExtraNodesList();
				break;
			}
			break;
		case IDC_BTN_REMOVE:
			RemoveSelectedItems();
			RefreshExtraNodesList();
			break;
		}
		return FALSE;
	}
	return FALSE;
}

HWND CreateERNWindow(HINSTANCE hInstance, HWND hWndParent, HWND hWndOwner, ExtraRigNodes *xrn/*=NULL*/) {
	ERNWindow *dlgData = new ERNWindow(xrn);
	HWND hDlg = dlgData->Create(hInstance, hWndParent);

	if (!hDlg) {// the window wasnt created, display an error
		LPVOID lpMsgBuf;

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR)&lpMsgBuf,
			0,
			NULL
		);
		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
		MessageBox(NULL, (LPCTSTR)lpMsgBuf, GetString(IDS_ERROR), MB_OK | MB_ICONINFORMATION);
		// Free the buffer.
		LocalFree(lpMsgBuf);
	}

	if (dlgData) {
		if (hWndOwner) {
			// Repositions the window so it is aligned to the Left of the owner window
			RECT rcOwnerWndRect, rcWndRect;
			GetWindowRect(hWndOwner, &rcOwnerWndRect);
			GetWindowRect(dlgData->GetDlg(), &rcWndRect);
			SetWindowPos(
				dlgData->GetDlg(), HWND_TOP,
				rcOwnerWndRect.left - (rcWndRect.right - rcWndRect.left), rcOwnerWndRect.top,
				0, 0, SWP_NOSIZE);
		}

		dlgData->Show();
	}
	return hDlg;
}

HWND ExtraRigNodes::CreateERNWindow(HWND hWndOwner)
{
	if (!hERNWnd) {
		hERNWnd = ::CreateERNWindow(hInstance, GetCOREInterface()->GetMAXHWnd(), hWndOwner, this);
	}
	else {
		ShowWindow(hERNWnd, SW_RESTORE);
		SetWindowPos(hERNWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	return hERNWnd;
}

///////////////////////////////////////////////////////////////////////////////////////
// Rollout class in case we want to disp0lay this page as a rollout
class ERNRollout : public ERNWindow
{
public:
	HWND GetHWnd() { return hDlg; }

	INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		if (uMsg == WM_INITDIALOG) {
			xrn = (ExtraRigNodes*)lParam;
			this->hDlg = hDlg;
		}
		return ERNWindow::DialogProc(hDlg, uMsg, wParam, lParam);
	}
};

static ERNRollout staticERNRollout;

static INT_PTR CALLBACK ERNRolloutProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return staticERNRollout.DialogProc(hDlg, uMsg, wParam, lParam);
};

void ExtraRigNodes::ICreateERNRollout(IObjParam *ip)
{
	TSTR strXRNLabel = GetString(IDS_EXTRA_RIG_NODES);
	ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_EXTRABONES_ROLLOUT), ERNRolloutProc, strXRNLabel, (LPARAM)this);
}

void ExtraRigNodes::IDestroyERNRollout(IObjParam *ip)
{
	if (staticERNRollout.GetHWnd()) {
		ip->DeleteRollupPage(staticERNRollout.GetHWnd());
	}
}

/**********************************************************************
 * Function publishing interface descriptor...
 */
static FPInterfaceDesc ern_FPinterface(
	I_EXTRARIGNODES_FP, _T("ExtraRigNodesInterface"), 0, NULL, FP_MIXIN,

	ExtraRigNodes::fnAddExtraRigNodes, _T("AddExtraRigNodes"), 0, TYPE_VOID, 0, 1,
		_T("nodes"), 0, TYPE_INODE_TAB_BV,
	ExtraRigNodes::fnRemoveExtraRigNodes, _T("RemoveExtraRigNodes"), 0, TYPE_VOID, 0, 1,
		_T("nodes"), 0, TYPE_INODE_TAB_BV,

	properties,

	ExtraRigNodes::propGetExtraRigNodes, ExtraRigNodes::propSetExtraRigNodes, _T("ExtraRigNodes"), 0, TYPE_INODE_TAB_BV,

	p_end
);

FPInterfaceDesc* ExtraRigNodes::GetDescByID(Interface_ID id) {
	if (id == I_EXTRARIGNODES_FP) return &ern_FPinterface;
	return &nullInterface;
}
