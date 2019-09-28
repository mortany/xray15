/*

Copyright 2010 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/

#include "unwrap.h"
#include "modsres.h"
//*********************************************************
// Undo record for TV positions and flags
//*********************************************************
TVertRestore::TVertRestore(MeshTopoData *d) : ld(d), updateView(FALSE), uvsel(d->GetTVVertSel())
{
	ld->CopyTVData(undo);
}

void TVertRestore::Restore(int isUndo)
{
	if (isUndo)
	{
		ld->CopyTVData(redo);
		rvsel = ld->GetTVVertSel();
	}

	ld->PasteTVData(undo);
	ld->SetTVVertSel(uvsel);	

	ld->RaiseTVDataChanged(updateView);
}

void TVertRestore::Redo()
{
	ld->PasteTVData(redo);
	ld->SetTVVertSel(rvsel);	

	ld->RaiseTVDataChanged(updateView);
}
void TVertRestore::EndHold()
{
	updateView = TRUE;
}

TSTR TVertRestore::Description()
{
	return TSTR(GetString(IDS_PW_UVW_VERT_EDIT));
}

HidePeltDialogRestore::HidePeltDialogRestore(UnwrapMod* pMod) : mMod(pMod)
{}

HidePeltDialogRestore::~HidePeltDialogRestore()
{}

void HidePeltDialogRestore::Restore(int isUndo)
{
	if (isUndo && mMod->mapMapMode == PELTMAP)
	{
		mMod->peltData.SetPeltMapMode(mMod, FALSE);
		mMod->mapMapMode = NOMAP;
	}
}

void HidePeltDialogRestore::Redo()
{}

void HidePeltDialogRestore::EndHold()
{}

TSTR HidePeltDialogRestore::Description()
{
	return TSTR(_T("Internal Operation"));
}


//*********************************************************
// Undo record for TV posiitons and face topology
//*********************************************************
TVertAndTFaceRestore::TVertAndTFaceRestore(MeshTopoData *d) 
	: ld(d), update(FALSE),
		uvsel(d->GetTVVertSel()),
		ufsel(d->GetFaceSel()),
		uesel(d->GetTVEdgeSel()),
		ugesel(d->GetGeomEdgeSel()),
		ugvsel(d->GetGeomVertSel())
{
	ld->CopyTVData(undo);
	ld->CopyFaceData(fundo);
}

TVertAndTFaceRestore::~TVertAndTFaceRestore()
{
	int ct = fundo.Count();
	for (int i = 0; i < ct; i++)
	{
		if (fundo[i]->vecs) delete fundo[i]->vecs;
		fundo[i]->vecs = NULL;

		if (fundo[i]) delete fundo[i];
		fundo[i] = NULL;
	}

	ct = fredo.Count();
	for (int i = 0; i < ct; i++)
	{
		if (fredo[i]->vecs) delete fredo[i]->vecs;
		fredo[i]->vecs = NULL;

		if (fredo[i]) delete fredo[i];
		fredo[i] = NULL;
	}
}

void TVertAndTFaceRestore::Restore(int isUndo)
{
	if (isUndo)
	{
		ld->CopyTVData(redo);
		ld->CopyFaceData(fredo);

		rvsel = ld->GetTVVertSel();
		rfsel = ld->GetFaceSel();
		resel = ld->GetTVEdgeSel();
		rgesel = ld->GetGeomEdgeSel();
		rgvsel = ld->GetGeomVertSel();
	}

	ld->PasteTVData(undo);
	ld->PasteFaceData(fundo);

	ld->SetTVVertSel(uvsel);
	ld->SetFaceSelectionByRef(ufsel);
	ld->SetTVEdgeSel(uesel);
	ld->SetGeomEdgeSel(ugesel);
	ld->SetGeomVertSel(ugvsel);
	ld->SetTVEdgeInvalid();
	ld->BuildTVEdges();
	ld->BuildVertexClusterList();

	ld->RaiseTVertFaceChanged(update);
}
void TVertAndTFaceRestore::Redo()
{
	ld->PasteTVData(redo);
	ld->PasteFaceData(fredo);

	ld->SetTVVertSel(rvsel);
	ld->SetFaceSelectionByRef(rfsel);
	ld->SetTVEdgeSel(resel);
	ld->SetGeomEdgeSel(rgesel);
	ld->SetGeomVertSel(rgvsel);
	ld->SetTVEdgeInvalid();

	ld->BuildTVEdges();
	ld->BuildVertexClusterList();

	ld->RaiseTVertFaceChanged(update);
}

void TVertAndTFaceRestore::EndHold()
{
	update = TRUE;
}

TSTR TVertAndTFaceRestore::Description() { return TSTR(GetString(IDS_PW_UVW_EDIT)); }


//*********************************************************
// Undo record for selection of point in the dialog window
//*********************************************************

TSelRestore::TSelRestore(MeshTopoData *ald) 
	: bUpdateView(FALSE), ld(ald),
		undo(ald->GetTVVertSel()),
		eundo(ald->GetTVEdgeSel()),
		fundo(ald->GetFaceSel()),
		gvundo(ald->GetGeomVertSel()),
		geundo(ald->GetGeomEdgeSel())
{}

void TSelRestore::Restore(int isUndo)
{
	if (isUndo)
	{
		redo = ld->GetTVVertSel();
		eredo = ld->GetTVEdgeSel();
		fredo = ld->GetFaceSel();

		gvredo = ld->GetGeomVertSel();
		geredo = ld->GetGeomEdgeSel();
	}

	ld->SetTVVertSel(undo);
	ld->SetTVEdgeSel(eundo);
	ld->SetFaceSelectionByRef(fundo);
	ld->SetGeomVertSel(gvundo);
	ld->SetGeomEdgeSel(geundo);

	ld->RaiseSelectionChanged(bUpdateView);
}
void TSelRestore::Redo()
{
	ld->SetTVVertSel(redo);
	ld->SetTVEdgeSel(eredo);
	ld->SetFaceSelectionByRef(fredo);
	ld->SetGeomVertSel(gvredo);
	ld->SetGeomEdgeSel(geredo);

	ld->RaiseSelectionChanged(bUpdateView);
}
void TSelRestore::EndHold() 
{
	bUpdateView = TRUE;
}

TSTR TSelRestore::Description() { return TSTR(GetString(IDS_PW_SELECT_UVW)); }


//*********************************************************
// Undo record for a reset operation
//*********************************************************


ResetRestore::ResetRestore(UnwrapMod *m) : rchan(0)
{
	mod = m;
	uchan = mod->channel;
}

ResetRestore::~ResetRestore()
{}

void ResetRestore::Restore(int isUndo)
{
	if (isUndo)
	{

		rchan = mod->channel;
	}
	mod->channel = uchan;
	if (mod->ip)
	{
		if (mod->iMapID)
			mod->iMapID->SetValue(mod->channel, FALSE);

		if (mod->channel == 1)
		{
			CheckRadioButton(mod->hParams, IDC_MAP_CHAN1, IDC_MAP_CHAN2, IDC_MAP_CHAN2);
		}
		else
		{
			CheckRadioButton(mod->hParams, IDC_MAP_CHAN1, IDC_MAP_CHAN2, IDC_MAP_CHAN1);
		}
	}

	mod->RebuildEdges();
	mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	if (mod->editMod == mod && mod->hView) mod->InvalidateView();
}

void ResetRestore::Redo()
{
	mod->channel = rchan;
	if (mod->ip)
	{

		if (mod->iMapID)
			mod->iMapID->SetValue(mod->channel, FALSE);
		if (mod->channel == 1)
		{
			CheckRadioButton(mod->hParams, IDC_MAP_CHAN1, IDC_MAP_CHAN2, IDC_MAP_CHAN2);
		}
		else
		{
			CheckRadioButton(mod->hParams, IDC_MAP_CHAN1, IDC_MAP_CHAN2, IDC_MAP_CHAN1);
		}
	}

	mod->RebuildEdges();
	mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	if (mod->editMod == mod && mod->hView) mod->InvalidateView();
}

void ResetRestore::EndHold(){}
TSTR ResetRestore::Description() { return TSTR(GetString(IDS_PW_RESET_UNWRAP)); }

UnwrapPivotRestore::UnwrapPivotRestore(UnwrapMod *m)
{
	mod = m;
	upivot = mod->freeFormPivotOffset;
}

void UnwrapPivotRestore::Restore(int isUndo) {
	if (isUndo) {
		rpivot = mod->freeFormPivotOffset;
	}

	mod->freeFormPivotOffset = upivot;
	mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	mod->InvalidateView();
}

void UnwrapPivotRestore::Redo() {
	mod->freeFormPivotOffset = rpivot;

	mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	mod->InvalidateView();
}

TSTR UnwrapPivotRestore::Description() { return TSTR(GetString(IDS_PW_PIVOTRESTORE)); }

UnwrapSeamAttributesRestore::UnwrapSeamAttributesRestore(UnwrapMod *m) : rThick(0),
rReflatten(0), rShowMapSeams(0), rShowPeltSeams(0)
{
	mod = m;

	uReflatten = mod->fnGetPreventFlattening();
	uThick = mod->fnGetThickOpenEdges();
	uShowMapSeams = mod->fnGetViewportOpenEdges();
	uShowPeltSeams = mod->fnGetAlwayShowPeltSeams();
}

void UnwrapSeamAttributesRestore::Restore(int isUndo)
{
	if (isUndo)
	{
		rReflatten = mod->fnGetPreventFlattening();
		rThick = mod->fnGetThickOpenEdges();
		rShowMapSeams = mod->fnGetViewportOpenEdges();
		rShowPeltSeams = mod->fnGetAlwayShowPeltSeams();
	}

	mod->fnSetPreventFlattening(uReflatten);
	mod->fnSetThickOpenEdges(uThick);
	mod->fnSetViewportOpenEdges(uShowMapSeams);
	mod->fnSetAlwayShowPeltSeams(uShowPeltSeams);

	mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	mod->InvalidateView();
}

void UnwrapSeamAttributesRestore::Redo()
{
	mod->fnSetPreventFlattening(rReflatten);
	mod->fnSetThickOpenEdges(rThick);
	mod->fnSetViewportOpenEdges(rShowMapSeams);
	mod->fnSetAlwayShowPeltSeams(rShowPeltSeams);

	mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	mod->InvalidateView();
}

TSTR UnwrapSeamAttributesRestore::Description() { return TSTR(GetString(IDS_PW_PIVOTRESTORE)); }

UnwrapMapAttributesRestore::UnwrapMapAttributesRestore(UnwrapMod *m) : rAlign(0),
rPreview(0), rNormalize(0)
{
	mod = m;

	uPreview = mod->fnGetQMapPreview();
	uNormalize = mod->fnGetNormalizeMap();
	uAlign = mod->GetQMapAlign();
}

void UnwrapMapAttributesRestore::Restore(int isUndo)
{
	if (isUndo)
	{
		rPreview = mod->fnGetQMapPreview();
		rNormalize = mod->fnGetNormalizeMap();
		rAlign = mod->GetQMapAlign();
	}

	int align = uAlign;

	mod->fnSetNormalizeMap(uNormalize);
	mod->SetQMapPreview(uPreview);
	mod->GetUIManager()->SetFlyOut(ID_QUICKMAP_ALIGN, align, TRUE);
	mod->GetUIManager()->UpdateCheckButtons();

	mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	mod->InvalidateView();
}

TSTR UnwrapMapAttributesRestore::Description() { return TSTR(GetString(IDS_PW_PIVOTRESTORE)); }

void UnwrapMapAttributesRestore::Redo()
{
	mod->fnSetNormalizeMap(rNormalize);
	int align = rAlign;

	mod->fnSetNormalizeMap(rNormalize);
	mod->SetQMapPreview(rPreview);
	mod->GetUIManager()->SetFlyOut(ID_QUICKMAP_ALIGN, align, TRUE);
	mod->GetUIManager()->UpdateCheckButtons();

	mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	mod->InvalidateView();
}

HoldSuspendedOffGuard::HoldSuspendedOffGuard(bool bSuspended)
{
	mbTempSuspended = bSuspended;
	if (mbTempSuspended)
	{
		theHold.Resume();
	}
}

HoldSuspendedOffGuard::~HoldSuspendedOffGuard()
{
	if (mbTempSuspended)
	{
		theHold.Suspend();
	}
}


