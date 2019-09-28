//
// Copyright 2015 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#include "unwrap.h"
#include <Graphics/IDisplayManager.h>
#include "ToolRelax.h"
#include "iMenuMan.h"


//--- Mouse procs for modes -----------------------------------------------
int SelectMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	if (mod == nullptr)
	{
		return 0;
	}

	toggle = flags&MOUSE_CTRL;
	subtract = flags&MOUSE_ALT;

	switch (msg)
	{
	case MOUSE_DBLCLICK:
	{
		// Hit test
		Tab<TVHitData> hits;
		Rect rect;
		rect.left = m.x - 2;
		rect.right = m.x + 2;
		rect.top = m.y - 2;
		rect.bottom = m.y + 2;

		if (mod->HitTest(rect, hits, subtract))
		{
			theHold.Begin();
			mod->HoldSelection();
			theHold.Accept(GetString(IDS_PW_SELECT_UVW));

			theHold.Suspend();

			mod->DoubleClickSelect(hits, toggle);
			mod->InvalidateView();
			theHold.Resume();

			return 1;
		}
		break;
	}
	case MOUSE_POINT:
	{
		if (point == 0)
		{
			// First click
			region = FALSE;
			// Hit test
			Tab<TVHitData> hits;
			Rect rect;
			rect.left = m.x - 2;
			rect.right = m.x + 2;
			rect.top = m.y - 2;
			rect.bottom = m.y + 2;
			// First hit test sel only
			mod->centeron = 0;
			if (toggle && subtract)
			{
				mod->centeron = 1;
				return subproc(hWnd, msg, point, flags, m);
			}

			// First hit test sel only
			if (((!toggle && !subtract && mod->HitTest(rect, hits, TRUE)) || (mod->lockSelected == 1))
				|| ((mod->freeFormSubMode != ID_SELECT) && (mod->mode == ID_FREEFORMMODE))
				)
			{
				return subproc(hWnd, msg, point, flags, m);
			}
			else
				// Next hit test everything
				if (mod->HitTest(rect, hits, subtract))
				{
					theHold.Begin();
					mod->HoldSelection();
					theHold.Accept(GetString(IDS_PW_SELECT_UVW));

					theHold.Suspend();
					if ((flags & MOUSE_SHIFT) 
						&& hits.Count() == 1) //only run when clicking.
					{
						if (mod->fnGetTVSubMode() == TVFACEMODE)
						{
							mod->UVFaceLoop(hits);
						}
						else if (mod->fnGetTVSubMode() == TVEDGEMODE)
						{
							mod->UVEdgeRing(hits);
						}
					}
					else if (!toggle && !subtract) // if the alt/ctrl short-cut key is not pressed
					{
						mod->ClearSelect();
						mod->Select(hits, subtract, FALSE);
					}
					else                     // if the alt/ctrl short-cut key is pressed
					{
						mod->Select(hits, subtract, FALSE);
					}

					mod->InvalidateView();
					theHold.Resume();

					if (mod->showVerts)
					{
						mod->NotifyDependents(FOREVER, PART_DISPLAY, REFMSG_CHANGE);
						if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
					}
					if (mod->mode == ID_FREEFORMMODE)
					{
						mod->RebuildFreeFormData();
						mod->HitTest(rect, hits, FALSE);
						if ((toggle || subtract) && (mod->freeFormSubMode != ID_SCALE))
							return FALSE;
					}
					else
					{
						if (toggle || subtract) return FALSE;
					}
					return subproc(hWnd, msg, point, flags, m);
				}
				else
				{
					region = TRUE;
					lm = om = m;
					XORDottedRect(hWnd, om, m);
				}
		}
		else
		{
			// Second click
			if (region)
			{
				Rect rect;
				if (mod->mode == ID_WELD)
				{
					rect.left = om.x - 2;
					rect.right = om.x + 2;
					rect.top = om.y - 2;
					rect.bottom = om.y + 2;
				}
				else
				{
					rect.left = om.x;
					rect.top = om.y;
					rect.right = m.x;
					rect.bottom = m.y;
					rect.Rectify();
				}
				Tab<TVHitData> hits;
				theHold.Begin();
				mod->HoldSelection();
				theHold.Accept(GetString(IDS_PW_SELECT_UVW));
				theHold.Suspend();
				if (!toggle && !subtract)
					mod->ClearSelect();
				if (mod->HitTest(rect, hits, subtract)) {
					mod->Select(hits, subtract, TRUE);
				}
				theHold.Resume();

				//when the region selection is changed,the status and value in the type-in spins get updated.
				mod->SetupTypeins();

				if (mod->showVerts)
				{
					mod->NotifyDependents(FOREVER, PART_DISPLAY, REFMSG_CHANGE);
					if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
				}

				mod->InvalidateView();
			}
			else
			{
				return subproc(hWnd, msg, point, flags, m);
			}
		}
		break;
	}
	case MOUSE_MOVE:
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		if (region)
		{
			XORDottedRect(hWnd, om, lm, 0, true);
			XORDottedRect(hWnd, om, m);
			lm = m;
		}
		else
		{
			SetCursor(GetXFormCur());
			return subproc(hWnd, msg, point, flags, m);
		}
		break;
	}
	case MOUSE_FREEMOVE:
	{

		Tab<TVHitData> hits;
		Rect rect;
		rect.left = m.x - 2;
		rect.right = m.x + 2;
		rect.top = m.y - 2;
		rect.bottom = m.y + 2;

		if ((flags&MOUSE_CTRL) && (flags&MOUSE_ALT))
			GetCOREInterface()->ReplacePrompt(GetString(IDS_PW_MOUSE_CENTER));
		else if (flags&MOUSE_CTRL)
			GetCOREInterface()->ReplacePrompt(GetString(IDS_PW_MOUSE_ADD));
		else if (flags&MOUSE_ALT)
			GetCOREInterface()->ReplacePrompt(GetString(IDS_PW_MOUSE_SUBTRACT));
		else if (flags&MOUSE_SHIFT)
			GetCOREInterface()->ReplacePrompt(GetString(IDS_PW_MOUSE_CONSTRAIN));
		else GetCOREInterface()->ReplacePrompt(GetString(IDS_PW_MOUSE_SELECTTV));

		if (mod->HitTest(rect, hits, FALSE))
		{
			{
				if (mod->mode == ID_FREEFORMMODE)
				{
					SetCursor(GetXFormCur());
				}

				if (mod->fnGetTVSubMode() == TVVERTMODE)
				{
					MeshTopoData *ld = nullptr;

					int vID = -1;
					if (hits.Count())
					{
						mod->ClearTVSelectionPreview();
						for (int i = 0; i < hits.Count(); i++)
						{
							vID = hits[i].mID;
							int ldID = hits[i].mLocalDataID;
							ld = mod->GetMeshTopoData(ldID);
							if (ld && vID != -1 && mod->fnGetSelectionPreview() &&
								ld->IsTVVertVisible(vID) && (!ld->GetTVVertFrozen(vID)) && (!ld->GetTVVertHidden(vID)))
							{
								ld->SetTVVertSelectPreview(vID, TRUE);
							}
						}
						mod->InvalidateView();
					}

					if ((ld && ld->GetTVVertSelected(vID)) || (mod->mode == ID_FREEFORMMODE))
					{
						SetCursor(GetXFormCur());
					}
					else
					{
						SetCursor(mod->selCur);
					}
				}
				else if (mod->fnGetTVSubMode() == TVEDGEMODE)
				{
					MeshTopoData *ld = NULL;
					int edgeID = -1;
					if (hits.Count())
					{
						mod->ClearTVSelectionPreview();
						for (int i = 0; i < hits.Count(); i++)
						{
							edgeID = hits[i].mID;
							int ldID = hits[i].mLocalDataID;
							ld = mod->GetMeshTopoData(ldID);
							if (ld && edgeID != -1 && mod->fnGetSelectionPreview() &&
								(!ld->GetTVEdgeHidden(edgeID)))
							{
								ld->SetTVEdgeSelectedPreview(edgeID, TRUE);
							}
						}
						mod->InvalidateView();
					}

					if ((ld && ld->GetTVEdgeSelected(edgeID)) || (mod->mode == ID_FREEFORMMODE))
					{
						SetCursor(GetXFormCur());
					}
					else
					{
						SetCursor(mod->selCur);
					}
				}
				else if (mod->fnGetTVSubMode() == TVFACEMODE)
				{
					MeshTopoData *ld = nullptr;
					int fID = -1;

					if (hits.Count())
					{
						mod->ClearTVSelectionPreview();
						for (int i = 0; i < hits.Count(); i++)
						{
							fID = hits[i].mID;
							int ldID = hits[i].mLocalDataID;
							ld = mod->GetMeshTopoData(ldID);
							if (ld && fID != -1 && mod->fnGetSelectionPreview() &&
								(ld->IsFaceVisible(fID)) && (!ld->GetFaceFrozen(fID)))
							{
								ld->SetFaceSelectedPreview(fID, TRUE);
								if (mod->fnGetPolyMode() && ld->GetMesh())
								{
									ld->PolySelection(TRUE, TRUE);
								}
							}
						}
						mod->InvalidateView();
					}

					if ((ld && ld->GetFaceSelected(fID)) || (mod->mode == ID_FREEFORMMODE))
						SetCursor(GetXFormCur());
					else
						SetCursor(mod->selCur);
				}

				if (mod->mode == ID_FREEFORMMODE)
				{
					if ((mod->freeFormSubMode == ID_MOVE || mod->freeFormSubMode == ID_SELECT) && mod->freeFromViewValidate == FALSE)
					{
						mod->freeFromViewValidate = TRUE;
						mod->InvalidateView();
					}
				}

			}
		}
		else
		{
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			if (!toggle && !subtract)
			{
				if (mod->ClearTVSelectionPreview())
				{
					mod->InvalidateView();
				}
			}
		}

		return subproc(hWnd, msg, point, flags, m);
	}
	case MOUSE_ABORT:
	{
		if (region)
		{
			InvalidateRect(hWnd, NULL, FALSE);
		}
		else
		{
			return subproc(hWnd, msg, point, flags, m);
		}
		break;
	}
	}
	return 1;
}

int PaintSelectMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	static IPoint2 lastm;
	static int lastRadius;

	if (mod == nullptr)
	{
		return 0;
	}

	switch (msg) {
	case MOUSE_POINT:
		if (point == 0) {
			// First click
			toggle = flags&MOUSE_CTRL;
			subtract = flags&MOUSE_ALT;

			// Hit test
			Tab<TVHitData> hits;
			Rect rect;
			rect.left = m.x - mod->fnGetPaintSize();
			rect.top = m.y - mod->fnGetPaintSize();
			rect.right = m.x + mod->fnGetPaintSize();
			rect.bottom = m.y + mod->fnGetPaintSize();
			// First hit test sel only
			// Next hit test everything
			//hold the selection
			theHold.Begin();
			mod->HoldSelection();//theHold.Put (new TSelRestore (mod));

			if (!toggle && !subtract)
			{
				mod->ClearSelect();
				mod->InvalidateView();
				UpdateWindow(mod->hDialogWnd);
			}

			if (mod->HitTest(rect, hits, subtract))
			{
				mod->Select(hits, subtract, TRUE);
				mod->InvalidateView();
				if (mod->fnGetSyncSelectionMode())
				{
					mod->fnSyncGeomSelection();
				}
				else UpdateWindow(mod->hDialogWnd);
			}
			om = m;
		}
		else
		{
			mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
			if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
			theHold.Accept(GetString(IDS_PW_SELECT_UVW));
		}
		break;

	case MOUSE_MOVE:
	{
		float len = Length(om - m);
		if (len > ((float)mod->fnGetPaintSize()*2.0f))
		{
			int ct = (len / (mod->fnGetPaintSize()*2.0f));
			Point2 start, end, vec;
			start.x = (float)om.x;
			start.y = (float)om.y;
			end.x = (float)m.x;
			end.y = (float)m.y;
			vec = (end - start) / ct;
			Point2 current;
			current = start;
			toggle = flags&MOUSE_CTRL;
			subtract = flags&MOUSE_ALT;
			BOOL redraw = FALSE;
			for (int i = 0; i < (ct + 1); i++)
			{
				m.x = (int)current.x;
				m.y = (int)current.y;
				Rect rect;
				rect.left = m.x - mod->fnGetPaintSize();
				rect.top = m.y - mod->fnGetPaintSize();
				rect.right = m.x + mod->fnGetPaintSize();
				rect.bottom = m.y + mod->fnGetPaintSize();
				rect.Rectify();
				Tab<TVHitData> hits;
				if (mod->HitTest(rect, hits, subtract))
				{
					mod->Select(hits, subtract, TRUE);
					redraw = TRUE;
				}
				current += vec;
			}
			if (redraw)
			{
				mod->InvalidateView();
				if (mod->fnGetSyncSelectionMode())
				{
					mod->fnSyncGeomSelection();
				}
				else UpdateWindow(mod->hDialogWnd);
			}
		}
		else
		{
			Rect rect;
			rect.left = m.x - mod->fnGetPaintSize();
			rect.top = m.y - mod->fnGetPaintSize();
			rect.right = m.x + mod->fnGetPaintSize();
			rect.bottom = m.y + mod->fnGetPaintSize();
			rect.Rectify();
			Tab<TVHitData> hits;
			toggle = flags&MOUSE_CTRL;
			subtract = flags&MOUSE_ALT;
			if (mod->HitTest(rect, hits, subtract))
			{
				mod->Select(hits, subtract, TRUE);
				mod->InvalidateView();
				UpdateWindow(mod->hDialogWnd);
			}
			mod->fnSyncGeomSelection();
		}

		om = m;

		IPoint2 r = lastm;
		if (!first)
		{
			r.x += lastRadius;
			XORDottedCircle(hWnd, lastm, r, 0, true);
		}
		first = FALSE;

		r = m;
		r.x += mod->fnGetPaintSize();
		XORDottedCircle(hWnd, m, r);

		lastm = m;
		lastRadius = mod->fnGetPaintSize();
	}
	break;
	case MOUSE_FREEMOVE:
	{
		IPoint2 r = lastm;
		if (!first)
		{
			r.x += lastRadius;
			XORDottedCircle(hWnd, lastm, r, 0, true);
		}
		first = FALSE;

		r = m;
		r.x += mod->fnGetPaintSize();
		XORDottedCircle(hWnd, m, r);

		lastm = m;
		lastRadius = mod->fnGetPaintSize();;
		break;
	}

	case MOUSE_ABORT:
	{
		//cancel the hold
		theHold.Restore();
		theHold.Cancel();
		break;
	}
	}
	return 1;
}

void PaintMoveMode::ClearHitData()
{
	int count = hits.Count();
	int localDataID;
	int index;
	MeshTopoData* meshTopoData = nullptr;
	for (int i = 0; i < count && mod; ++i)
	{
		localDataID = hits[i].mLocalDataID;
		index = hits[i].mID;
		meshTopoData = mod->GetMeshTopoData(localDataID);//mod->mMeshTopoData[localDataID];
		if (meshTopoData)
		{
			meshTopoData->SetTVVertInfluence(index, 0.0f);
		}
	}
	hits.ZeroCount();
	bDirty = true;
}

void PaintMoveMode::UpdateHitData(HWND hwnd, IPoint2 m)
{
	if (bDirty)
	{
		ClearHitData();
		if (mod->HitTestCircle(m, mod->fnGetPaintFallOffSize(), hits))
		{
			mod->UpdatePaintInfluence(hits, Point2(m.x, m.y), hitsInflu);
			bDirty = FALSE;
			om = m;
		}
	}

	IPoint2 delta = m - om;
	float xzoom, yzoom;
	int width, height;
	mod->ComputeZooms(hwnd, xzoom, yzoom, width, height);
	Point2 mv;
	mv.x = delta.x / xzoom;
	mv.y = -delta.y / yzoom;
	mod->MovePaintPoints(hits, mv, hitsInflu);
	if (mod->update && mod->ip)
		mod->ip->RedrawViews(mod->ip->GetTime());
	UpdateWindow(mod->hDialogWnd);
	SetCursor(GetCOREInterface()->GetSysCursor(SYSCUR_MOVE));
}

void PaintMoveMode::DrawGizmo(HWND hWnd, IPoint2 m)
{
	static IPoint2 lastm;
	static float lastInnerRadius;
	static float lastOuterRadius;

	IPoint2 r = lastm;
	if (!first) //erase the old!
	{
		r.x += lastInnerRadius;
		XORDottedCircle(hWnd, lastm, r, 1);
		r = lastm;
		r.x += lastOuterRadius;
		XORDottedCircle(hWnd, lastm, r, 0);
	}

	first = FALSE;
	lastm = m;
	lastInnerRadius = mod->fnGetPaintFullStrengthSize();
	lastOuterRadius = mod->fnGetPaintFallOffSize();

	r = m;
	r.x += lastInnerRadius;
	XORDottedCircle(hWnd, m, r, 1);
	r = m;
	r.x += lastOuterRadius;
	XORDottedCircle(hWnd, m, r, 0);
}

void PaintMoveMode::ResetCursorPos(HWND hWnd, IPoint2& m)
{
	if (bResetCursor)
	{
		POINT pt;
		pt.x = cursorPt2.x;
		pt.y = cursorPt2.y;
		ClientToScreen(hWnd, &pt);
		SetCursorPos(pt.x, pt.y);
		m = cursorPt2;
		bResetCursor = FALSE;
	}
}

void PaintMoveMode::StartHolding()
{
	if (!bHolding)
	{
		theHold.Begin();
		mod->HoldPoints();
		bHolding = TRUE;
	}
}

void PaintMoveMode::EndHolding(bool bCancel)
{
	if (bHolding)
	{
		if (bCancel)
		{
			theHold.Cancel();
		}
		else
		{
			theHold.Accept(GetString(IDS_PW_MOVE_UVW));
		}
		bHolding = FALSE;
	}
}

void PaintMoveMode::StartPaintMove()
{
	//hold our selection
	for (int ldID = 0; ldID < mod->GetMeshTopoDataCount(); ldID++)
	{
		MeshTopoData *ld = mod->GetMeshTopoData(ldID);
		ld->HoldSelection();
	}
	currentMode = mod->fnGetTVSubMode();
	mod->ClearSelection(currentMode);
	mod->fnSetTVSubMode(TVVERTMODE);
	// Hit test
	ClearHitData();
	mod->PlugControllers();
	bHolding = FALSE;
}

void PaintMoveMode::EndPaintMove()
{
	ClearHitData();
	//restore our selection
	for (int ldID = 0; ldID < mod->GetMeshTopoDataCount(); ldID++)
	{
		MeshTopoData *ld = mod->GetMeshTopoData(ldID);
		ld->RestoreSelection();
	}
	mod->InvalidateView();
	if (mod->ip)
		mod->ip->RedrawViews(mod->ip->GetTime());
	mod->fnSetTVSubMode(currentMode);
	UpdateWindow(mod->hDialogWnd);
}

bool PaintMoveMode::AdjustBrushRadius(HWND hWnd, int flags, IPoint2& m)
{
	if (!mInRelax && (flags & MOUSE_SHIFT || flags & MOUSE_CTRL))
	{
		bResetCursor = TRUE;
		EndHolding(false);
		BOOL bFallOffRadius = (flags & MOUSE_CTRL);
		float radius = bFallOffRadius ? mod->fnGetPaintFallOffSize() : mod->fnGetPaintFullStrengthSize();
		radius -= (m.y - lastPt2.y);
		bFallOffRadius ? mod->fnSetPaintFallOffSize(radius) : mod->fnSetPaintFullStrengthSize(radius);
		ClearHitData();
		// when adjusting radius, keep the gizmo where it used to be.
		SetCursor(NULL);
		DrawGizmo(hWnd, cursorPt2);
		return true;
	}
	return false;
}

bool PaintMoveMode::IsRelaxButtonPressed(int flags)
{
	return (flags&MOUSE_ALT)?true:false;
}

bool PaintMoveMode::TestStartRelax(HWND hWnd, int flags, IPoint2& m)
{
	if (mInRelax) return false;

	if (IsRelaxButtonPressed(flags))
	{
		SetCursor(NULL);
		DrawGizmo(hWnd, m);
		BOOL curElement = mod->fnGetTVElementMode();
		mod->fnSetTVElementMode(FALSE);
		mod->ClearSelect();
		mod->Select(hits, FALSE, TRUE);
		mod->fnSetTVElementMode(curElement);

		if (hits.Count() > 0)
		{
			mInRelax = true;
			// start relax
			mod->RelaxThreadOp(mod->KThreadStart, hWnd);
			return true;
		}
	}
	return false;
}

bool PaintMoveMode::TestEndRelax(HWND hWnd)
{
	if (!mInRelax) return false;
	mInRelax = false;
	mod->RelaxThreadOp(mod->KThreadEnd, hWnd);

	mod->ClearSelection(mod->fnGetTVSubMode());
	ClearHitData();
	SetCursor(NULL);

	return true;
}

int PaintMoveMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	if (mod == nullptr)
	{
		return 0;
	}

	switch (msg)
	{
	case MOUSE_POINT:
		if (point == 0) // First click
		{
			StartPaintMove();
			theHold.SuperBegin();
			DbgAssert(!bHolding);
			bResetCursor = FALSE;
			StartHolding();
			UpdateHitData(hWnd, m);
			if (TestStartRelax(hWnd, flags, m)) userStartRelax = true;
			
			lastPt2 = m;
			cursorPt2 = m;
		}
		else // record this operation in undo system and notify all the dependents
		{
			if (TestEndRelax(hWnd)) userStartRelax = false;
			EndHolding(false);
			theHold.SuperAccept(GetString(IDS_PW_MOVE_UVW));
			EndPaintMove();
		}
		break;
	case MOUSE_MOVE:
	{
		if (!AdjustBrushRadius(hWnd, flags, m) && !userStartRelax)
		{
			StartHolding();
			ResetCursorPos(hWnd, m);
			theHold.Restore();
			UpdateHitData(hWnd, m);
			cursorPt2 = m;
		}
		else if (userStartRelax)
		{
			TestEndRelax(hWnd);
			UpdateHitData(hWnd, m);
			TestStartRelax(hWnd, flags, m);
			cursorPt2 = m;
		}

		lastPt2 = m;
	}
	break;
	case MOUSE_FREEMOVE:
		ResetCursorPos(hWnd, m);
		DrawGizmo(hWnd, m);
		break;
	case MOUSE_ABORT:
	{
		TestEndRelax(hWnd);
		EndHolding(true);
		theHold.SuperCancel();
		EndPaintMove();
	}
	break;
	}
	return 1;
}

bool RelaxMoveMode::IsRelaxButtonPressed(int flags)
{
	if ((flags&MOUSE_SHIFT) || (flags&MOUSE_CTRL)) return false;
	else return true;
}

int MoveMode::subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	if (mod == nullptr)
	{
		return 0;
	}
	switch (msg)
	{
	case MOUSE_POINT:
		if (point == 0)
		{
			//VSNAP
			mod->BuildSnapBuffer();
			mod->PlugControllers();

			theHold.SuperBegin();
			theHold.Begin();
			mod->HoldPoints();
			om = m;
			//convert our sub selection type to vertex selection
			mod->tempVert = Point3(0.0f, 0.0f, 0.0f);

			mod->SetupTypeins();
		}
		else
		{

			TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap2.MoveSelected"));
			macroRecorder->FunctionCall(mstr, 1, 0,
				mr_point3, &mod->tempVert);

			if (mod->tempVert == Point3(0.0f, 0.0f, 0.0f))
			{
				theHold.Cancel();
				theHold.SuperCancel();
			}
			else
			{
				theHold.Accept(GetString(IDS_PW_MOVE_UVW));
				theHold.SuperAccept(GetString(IDS_PW_MOVE_UVW));
			}

			//Set the result value to the UVW spins.
			mod->SetupTypeins();

			mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);

			if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
		}
		break;

	case MOUSE_MOVE: {
		UnwrapMod::SuspendSelSyncGuard suspendSelSync(*mod);
		theHold.Restore();
		float xzoom, yzoom;
		int width, height;
		IPoint2 delta = m - om;
		if (flags&MOUSE_SHIFT && mod->move == 0) {
			if (abs(delta.x) > abs(delta.y)) delta.y = 0;
			else delta.x = 0;
		}
		else if (mod->move == 1) {
			delta.y = 0;
		}
		else if (mod->move == 2) {
			delta.x = 0;
		}
		mod->ComputeZooms(hWnd, xzoom, yzoom, width, height);
		Point2 mv;
		mv.x = delta.x / xzoom;
		mv.y = -delta.y / yzoom;
		//check if moving points or gizmo

		mod->tempVert.x = mv.x;
		mod->tempVert.y = mv.y;
		mod->tempVert.z = 0.0f;

		mod->TransferSelectionStart();
		mod->MovePoints(mv);
		mod->TransferSelectionEnd(FALSE, FALSE);

		//Set the delta value to the UVW spins.
		mod->GetUIManager()->SetSpinFValue(ID_SPINNERU, mv.x);
		mod->GetUIManager()->SetSpinFValue(ID_SPINNERV, mv.y);

		if (mod->update && mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
		UpdateWindow(hWnd);
		break;
	}


	case MOUSE_ABORT:
		theHold.Cancel();
		theHold.SuperCancel();
		if (mod->fnGetConstantUpdate())
			mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
		if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
		mod->InvalidateView();
		break;
	}
	return 1;
}

#define ZOOM_FACT	0.01f
#define ROT_FACT	DegToRad(0.5f)

int RotateMode::subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	if (mod == nullptr)
	{
		return 0;
	}
	static BOOL rotateAroundPivot = FALSE;
	switch (msg) {
	case MOUSE_POINT:
		if (point == 0) {

			mod->PlugControllers();

			theHold.SuperBegin();
			theHold.Begin();
			mod->HoldPoints();

			if (mod->centeron)
			{
				mod->center.x = (float)m.x;
				mod->center.y = (float)m.y;
				mod->tempCenter.x = mod->center.x;
				mod->tempCenter.y = mod->center.y;
				rotateAroundPivot = FALSE;
			}
			else
			{
				mod->center.x = mod->freeFormPivotScreenSpace.x;
				mod->center.y = mod->freeFormPivotScreenSpace.y;

				mod->tempCenter.x = mod->center.x;
				mod->tempCenter.y = mod->center.y;
				mod->centeron = TRUE;
				rotateAroundPivot = TRUE;
				mod->origSelCenter = mod->selCenter;
			}

			om = m;
			mod->inRotation = TRUE;

			mod->SetupTypeins();
		}
		else {

			float angle = float(m.y - om.y)*ROT_FACT;
			mod->tempHwnd = hWnd;
			if (mod->centeron)
			{
				TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap2.RotateSelected"));
				macroRecorder->FunctionCall(mstr, 2, 0,
					mr_float, angle,
					mr_point3, &mod->axisCenter);
			}
			else
			{
				TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap2.RotateSelectedCenter"));
				macroRecorder->FunctionCall(mstr, 1, 0,
					mr_float, angle);
			}
			macroRecorder->EmitScript();

			if (angle == 0.0f)
			{
				theHold.Cancel();
				theHold.SuperCancel();
			}
			else
			{
				theHold.Accept(GetString(IDS_PW_ROTATE_UVW));
				theHold.SuperAccept(GetString(IDS_PW_ROTATE_UVW));
			}
			
			mod->TransferSelectionStart();
			//recompute pivot point
			mod->RecomputePivotOffset();
			mod->inRotation = FALSE;
			mod->TransferSelectionEnd(FALSE, FALSE);

			mod->SetupTypeins();

			mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
			if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
			mod->InvalidateView();

		}

		break;

	case MOUSE_MOVE:
		{
			UnwrapMod::SuspendSelSyncGuard suspendSelSync(*mod);
			theHold.Restore();
		}
		
		//convert our sub selection type to vertex selection
		mod->TransferSelectionStart();

		if (rotateAroundPivot)
		{
			Point3 vecA, vecB;
			Point3 a, b, center;
			a.x = (float)om.x;
			a.y = (float)om.y;
			a.z = 0.f;

			b.x = (float)m.x;
			b.y = (float)m.y;
			b.z = 0.0f;
			center.x = mod->freeFormPivotScreenSpace.x;
			center.y = mod->freeFormPivotScreenSpace.y;
			center.z = 0.0f;

			vecA = Normalize(a - center);
			vecB = Normalize(b - center);
			Point3 cross = CrossProd(vecA, vecB);
			float dot = DotProd(vecA, vecB);
			float angle = 0.0f;
			if (dot >= 1.0f)
			{
				angle = 0.0f;
			}
			else
			{
				if (cross.z < 0.0f)
					angle = acos(DotProd(vecA, vecB));
				else angle = -acos(DotProd(vecA, vecB));
			}

			if (flags&MOUSE_ALT)
			{
				angle = floor(angle * 180.0f / PI) * PI / 180.0f;
			}
			else if (flags&MOUSE_CTRL)
			{
				int iangle = (int)floor(angle * 180.0f / PI);
				int addOffset = iangle % 5;
				angle = (float)(iangle - addOffset) * PI / 180.0f;
			}

			int i1, i2;
			mod->GetUVWIndices(i1, i2);
			if ((i1 == 0) && (i2 == 2)) angle *= -1.0f;
			//convert our sub selection type to vertex selection


			mod->RotatePoints(hWnd, angle);
			//convert our sub selection type to current selection

			//Set the delta value to the UVW spins.
			mod->GetUIManager()->SetSpinFValue(ID_SPINNERU, angle*180.0f / PI);
		}
		else if (mod->centeron)
		{
			float fTemAngle = float(m.y - om.y)*ROT_FACT;
			mod->RotatePoints(hWnd, fTemAngle);

			//Set the delta value to the UVW spins.
			mod->GetUIManager()->SetSpinFValue(ID_SPINNERU, fTemAngle*180.0f / PI);
		}


		mod->TransferSelectionEnd(FALSE, FALSE);

		if (mod->update && mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
		UpdateWindow(hWnd);
		break;

	case MOUSE_ABORT:

		theHold.Cancel();
		theHold.SuperCancel();
		if (mod->fnGetConstantUpdate())
			mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
		if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
		mod->InvalidateView();
		mod->inRotation = FALSE;
		break;
	}
	return 1;
}

int ScaleMode::subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	static BOOL scaleAroundPivot = FALSE;

	static float tempXScale = 0.0f;
	static float tempYScale = 0.0f;

	static float xLength = 0.0f;
	static float yLength = 0.0f;

	static float angle = 0.0f;

	static float tempCenterX = 0.0f;
	static float tempCenterY = 0.0f;

	static float tempXLength = 0.0f;
	static float tempYLength = 0.0f;


	static float xAltLength = 0.0f;
	static float yAltLength = 0.0f;

	static float xScreenPivot = 0.0f;
	static float yScreenPivot = 0.0f;

	if (mod == nullptr)
	{
		return 0;
	}

	switch (msg) {
	case MOUSE_POINT:
		if (point == 0) {

			mod->tempVert = Point3(0.0f, 0.0f, 0.0f);


			mod->PlugControllers();

			theHold.SuperBegin();
			theHold.Begin();
			mod->HoldPoints();

			if (mod->centeron)
			{
				mod->center.x = (float)m.x;
				mod->center.y = (float)m.y;
				om = m;
				mod->tempCenter.x = mod->center.x;
				mod->tempCenter.y = mod->center.y;
				mod->tempAmount = 1.0f;
				scaleAroundPivot = FALSE;
			}
			else
			{
				float cx = mod->freeFormPivotScreenSpace.x;
				float cy = mod->freeFormPivotScreenSpace.y;

				float tx = m.x;
				float ty = m.y;
				tempXLength = tx - cx;
				tempYLength = ty - cy;

				xAltLength = tx - mod->freeFormPivotScreenSpace.x;
				yAltLength = ty - mod->freeFormPivotScreenSpace.y;

				xScreenPivot = mod->freeFormPivotScreenSpace.x;
				yScreenPivot = mod->freeFormPivotScreenSpace.y;


				mod->center.x = cx;
				mod->center.y = cy;
				tempCenterX = cx;
				tempCenterY = cy;

				om = m;
				mod->tempCenter.x = mod->center.x;
				mod->tempCenter.y = mod->center.y;
				Point3 originalPt = mod->selCenter + mod->freeFormPivotOffset;
				scaleAroundPivot = TRUE;
			}

			mod->SetupTypeins();
		}
		else {

			mod->tempHwnd = hWnd;
			if (mod->centeron)
			{
				TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap2.ScaleSelected"));
				macroRecorder->FunctionCall(mstr, 3, 0,
					mr_float, mod->tempAmount,
					mr_int, mod->tempDir,
					mr_point3, &mod->tempCenter);
			}
			else
			{
				TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap2.ScaleSelectedCenter"));
				macroRecorder->FunctionCall(mstr, 2, 0,
					mr_float, mod->tempAmount,
					mr_int, mod->tempDir);
			}
			macroRecorder->EmitScript();

			if (mod->tempAmount == 1.0f)
			{
				theHold.Cancel();
				theHold.SuperCancel();
			}
			else
			{
				theHold.Accept(GetString(IDS_PW_SCALE_UVW));
				theHold.SuperAccept(GetString(IDS_PW_SCALE_UVW));
			}

			mod->SetupTypeins();

			mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
			if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
			mod->InvalidateView();
		}

		break;

	case MOUSE_MOVE: {
		if (scaleAroundPivot)
		{
			UnwrapMod::SuspendSelSyncGuard suspendSelSync(*mod);
			theHold.Restore();
			IPoint2 delta = om - m;
			int direction = 0;
			float xScale = 1.0f, yScale = 1.0f;

			xLength = tempXLength;
			yLength = tempYLength;

			if (xLength < 50.0 && xLength > 0.0f)
				xLength = 50.0;
			if (xLength > -50.0 && xLength < 0.0f)
				xLength = -50.0;

			if (yLength < 50.0 && yLength > 0.0f)
				yLength = 50.0;
			if (yLength > -50.0 && yLength < 0.0f)
				yLength = -50.0;



			if ((abs(delta.x) > abs(delta.y) && (flags&MOUSE_SHIFT)) || (mod->scale == 1))
			{
				delta.y = 0;
				direction = 1;
			}
			else if ((abs(delta.x) < abs(delta.y) && (flags&MOUSE_SHIFT)) || (mod->scale == 2))
			{
				delta.x = 0;
				direction = 2;
			}


			if (direction == 0)
			{
				if (yLength != 0.0f)
				{
					if (delta.y > 0)
					{
						yScale = (1.0f + delta.y / fabs(yLength));
					}
					else
					{
						yScale = (1.0f + ((float)delta.y*0.25f) / fabs(yLength)); //we scale down the delta by .25 to slow the scale down speed since we limit the scale down amount
					}
				}
				else
				{
					yScale = 1.0f;
				}

				if (yScale <= 0.0f)
				{
					yScale = 0.0001f;
				}

				xScale = yScale;
			}
			else if (direction == 1)
			{
				if (xLength != 0.0f)
				{
					if (delta.x > 0)
					{
						xScale = (1.0f - delta.x / fabs(xLength));
					}
					else
					{
						xScale = (1.0f - ((float)delta.x*0.25f) / fabs(xLength)); //we scale down the delta by .25 to slow the scale down speed since we limit the scale down amount
					}
				}
				else
				{
					xScale = 1.0f;
				}

				if (xScale <= 0.0f)
				{
					xScale = 0.0001f;
				}

			}
			else if (direction == 2)
			{
				if (yLength != 0.0f)
				{
					if (delta.y > 0)
					{
						yScale = (1.0f + delta.y / fabs(yLength));
					}
					else
					{
						yScale = (1.0f + ((float)delta.y*0.25f) / fabs(yLength)); //we scale down the delta by .25 to slow the scale down speed since we limit the scale down amount
					}
				}
				else
				{
					yScale = 1.0f;
				}

				if (yScale <= 0.0f)
				{
					yScale = 0.0001f;
				}
			}
			mod->center.x = xScreenPivot;
			mod->center.y = yScreenPivot;

			if (mod->ip)
			{
				xScale = GetCOREInterface()->SnapPercent(xScale);
				yScale = GetCOREInterface()->SnapPercent(yScale);
			}

			tempXScale = xScale;
			tempYScale = yScale;


			//convert our sub selection type to vertex selection
			mod->TransferSelectionStart();

			mod->ScalePointsXY(hWnd, xScale, yScale);

			mod->tempDir = direction;
			mod->tempAmount = xScale;
			if (direction == 2)
				mod->tempAmount = yScale;

			//recompute pivot point
			mod->RecomputePivotOffset();

			//convert our sub selection type to current selection
			mod->TransferSelectionEnd(FALSE, FALSE);

			//Set the delta value to the UVW spins.
			mod->GetUIManager()->SetSpinFValue(ID_SPINNERU, xScale*100);
			mod->GetUIManager()->SetSpinFValue(ID_SPINNERV, yScale*100);

			if (mod->update &&mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
			UpdateWindow(hWnd);
		}
		else
		{
			UnwrapMod::SuspendSelSyncGuard suspendSelSync(*mod);
			theHold.Restore();
			IPoint2 delta = om - m;
			int direction = 0;
			if (flags&MOUSE_SHIFT) {
				if (abs(delta.x) > abs(delta.y))
				{
					delta.y = 0;
					direction = 1;
				}
				else
				{
					delta.x = 0;
					direction = 2;
				}
			}
			else if (mod->scale > 0)
			{
				if (mod->scale == 1)
				{
					delta.y = 0;
					direction = 1;
				}
				else if (mod->scale == 2)
				{
					delta.x = 0;
					direction = 2;
				}

			}

			float z = 0.0f;
			if (direction == 0)
			{
				if (delta.y < 0)
					z = (1.0f / (1.0f - ZOOM_FACT*delta.y));
				else z = (1.0f + ZOOM_FACT*delta.y);
			}
			else if (direction == 1)
			{
				if (delta.x < 0)
					z = (1.0f / (1.0f - ZOOM_FACT*delta.x));
				else z = (1.0f + ZOOM_FACT*delta.x);

			}
			else if (direction == 2)
			{
				if (delta.y < 0)
					z = (1.0f / (1.0f - ZOOM_FACT*delta.y));
				else z = (1.0f + ZOOM_FACT*delta.y);
			}

			if (mod->ip)
				z = GetCOREInterface()->SnapPercent(z);

			mod->tempDir = direction;
			mod->tempAmount = z;


			mod->TransferSelectionStart();
			mod->ScalePoints(hWnd, z, direction);
			mod->TransferSelectionEnd(FALSE, FALSE);

			//Set the delta value to the UVW spins.
			if(direction == 0)
			{
				mod->GetUIManager()->SetSpinFValue(ID_SPINNERU, z*100);
				mod->GetUIManager()->SetSpinFValue(ID_SPINNERV, z*100);
			}
			else if (direction == 1)
			{
				mod->GetUIManager()->SetSpinFValue(ID_SPINNERU, z*100);
			}
			else if (direction == 2)
			{
				mod->GetUIManager()->SetSpinFValue(ID_SPINNERV, z*100);
			}

			if (mod->update &&mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
			UpdateWindow(hWnd);

		}
		break;
	}

	case MOUSE_ABORT:

		theHold.Cancel();
		theHold.SuperCancel();
		if (mod->fnGetConstantUpdate())
			mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
		if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
		mod->InvalidateView();
		break;
	}
	return 1;
}

int FreeFormMode::subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{

	static float tempXScale = 0.0f;
	static float tempYScale = 0.0f;

	static float xLength = 0.0f;
	static float yLength = 0.0f;

	static float angle = 0.0f;

	static float tempCenterX = 0.0f;
	static float tempCenterY = 0.0f;

	static float tempXLength = 0.0f;
	static float tempYLength = 0.0f;


	static float xAltLength = 0.0f;
	static float yAltLength = 0.0f;

	static float xScreenPivot = 0.0f;
	static float yScreenPivot = 0.0f;

	if (mod == nullptr)
	{
		return 0;
	}

	switch (msg) {
	case MOUSE_POINT:
		if (point == 0)
		{
			mod->tempVert = Point3(0.0f, 0.0f, 0.0f);
			dragging = TRUE;
			if (mod->freeFormSubMode == ID_MOVE)
			{
				//lock the axis that will be used for moving selection.
				Box3 freeFormBox;
				Box3 boundXForMove;
				Box3 boundYForMove;		
				Box3 boundBothForXYMove;
				mod->ConstructFreeFormBoxesForMovingDirection(freeFormBox,boundXForMove,boundYForMove, boundBothForXYMove);
				//The mouse position should be in the UVW space, not in the screen space.
				Point3 mousePInUV(0.0f,0.0f,0.0f);
				int i1 = 0;
				int i2 = 0;
				mod->GetUVWIndices(i1, i2);

				float xzoom = 1.0f;
				float yzoom = 1.0f;
				int width = 1;
				int height =1;
				mod->ComputeZooms(mod->hView, xzoom, yzoom, width, height);
				int tx = (width - int(xzoom)) / 2;
				int ty = (height - int(yzoom)) / 2;
				mousePInUV[i1] = (m.x - tx - mod->xscroll)/xzoom;
				mousePInUV[i2] = (height - ty + mod->yscroll - m.y)/yzoom;

				if (boundXForMove.Contains(mousePInUV) || 
					boundYForMove.Contains(mousePInUV) ||
					boundBothForXYMove.Contains(mousePInUV))
				{
					if (boundBothForXYMove.Contains(mousePInUV))
					{
						mod->freeFormMoveAxisLocked = eMoveBoth;
					}
					else if(boundXForMove.Contains(mousePInUV))
					{
						mod->freeFormMoveAxisLocked = eMoveX;
					}
					else
					{
						mod->freeFormMoveAxisLocked = eMoveY;
					}
				}

				//VSNAP
				mod->BuildSnapBuffer();
				mod->PlugControllers();

				theHold.SuperBegin();
				theHold.Begin();
				mod->HoldPoints();
				om = m;

				//Part of the fix for MAXX-32379 - we need to make sure the spinners are updated
				mod->SetupTypeins();
			}
			else if (mod->freeFormSubMode == ID_ROTATE)
			{
				mod->PlugControllers();

				mod->inRotation = TRUE;
				theHold.SuperBegin();
				theHold.Begin();


				om = m;
				mod->center.x = mod->freeFormPivotScreenSpace.x;
				mod->center.y = mod->freeFormPivotScreenSpace.y;

				mod->tempCenter.x = mod->center.x;
				mod->tempCenter.y = mod->center.y;
				mod->centeron = TRUE;
				mod->origSelCenter = mod->selCenter;

				//Part of the fix for MAXX-32379 - we need to make sure the spinners are updated
				mod->SetupTypeins();
			}
			else if (mod->freeFormSubMode == ID_SCALE)
			{
				mod->PlugControllers();

				theHold.SuperBegin();
				theHold.Begin();
				theHold.Put(new UnwrapPivotRestore(mod));
				float cx = mod->freeFormCornersScreenSpace[mod->scaleCornerOpposite].x;
				float cy = mod->freeFormCornersScreenSpace[mod->scaleCornerOpposite].y;
				float tx = mod->freeFormCornersScreenSpace[mod->scaleCorner].x;
				float ty = mod->freeFormCornersScreenSpace[mod->scaleCorner].y;
				tempXLength = tx - cx;
				tempYLength = ty - cy;

				xAltLength = tx - mod->freeFormPivotScreenSpace.x;
				yAltLength = ty - mod->freeFormPivotScreenSpace.y;

				xScreenPivot = mod->freeFormPivotScreenSpace.x;
				yScreenPivot = mod->freeFormPivotScreenSpace.y;


				mod->center.x = cx;
				mod->center.y = cy;
				tempCenterX = cx;
				tempCenterY = cy;
				om = m;
				mod->tempCenter.x = mod->center.x;
				mod->tempCenter.y = mod->center.y;
				Point3 originalPt = mod->selCenter + mod->freeFormPivotOffset;
				
				//Part of the fix for MAXX-32379 - we need to make sure the spinners are updated
				mod->SetupTypeins();
			}
			else if (mod->freeFormSubMode == ID_MOVEPIVOT)
			{
				theHold.Begin();
				theHold.Put(new UnwrapPivotRestore(mod));
				om = m;
				mod->origSelCenter = mod->selCenter;
			}


		}
		else
		{
			mod->inRotation = FALSE;
			dragging = FALSE;

			if (mod->freeFormSubMode == ID_MOVE)
			{

				TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap2.MoveSelected"));
				macroRecorder->FunctionCall(mstr, 1, 0,
					mr_point3, &mod->tempVert);

				if (mod->tempVert == Point3(0.0f, 0.0f, 0.0f))
				{
					theHold.Cancel();
					theHold.SuperCancel();
				}
				else
				{
					theHold.Accept(GetString(IDS_PW_MOVE_UVW));
					theHold.SuperAccept(GetString(IDS_PW_MOVE_UVW));
				}

				//Set the result value to the UVW spins.
				mod->SetupTypeins();
			}
			else if (mod->freeFormSubMode == ID_SCALE)
			{
				TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap2.ScaleSelectedXY"));
				macroRecorder->FunctionCall(mstr, 3, 0,
					mr_float, tempXScale,
					mr_float, tempYScale,
					mr_point3, &mod->axisCenter);

				theHold.Accept(GetString(IDS_PW_SCALE_UVW));
				theHold.SuperAccept(GetString(IDS_PW_SCALE_UVW));

				//Set the result value to the UVW spins.
				mod->SetupTypeins();
			}
			else if (mod->freeFormSubMode == ID_ROTATE)
			{
				TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap2.RotateSelected"));
				macroRecorder->FunctionCall(mstr, 2, 0,
					mr_float, angle,
					mr_point3, &mod->axisCenter);

				if (angle == 0.0f)
				{
					theHold.Cancel();
					theHold.SuperCancel();
				}
				else
				{

					theHold.Accept(GetString(IDS_PW_ROTATE_UVW));
					theHold.SuperAccept(GetString(IDS_PW_ROTATE_UVW));
				}
				//recompute pivot point
				Box3 bounds;
				bounds.Init();
				for (int ldID = 0; ldID < mod->GetMeshTopoDataCount(); ldID++)
				{
					MeshTopoData *ld = mod->GetMeshTopoData(ldID);
					if (ld == NULL)
					{
						DbgAssert(0);
					}
					else
					{
						int vselCount = ld->GetNumberTVVerts();

						int i1, i2;
						mod->GetUVWIndices(i1, i2);
						for (int i = 0; i < vselCount; i++)
						{
							if (ld->GetTVVertSelected(i))
							{
								//get bounds
								Point3 p = Point3(0.0f, 0.0f, 0.0f);
								p[i1] = ld->GetTVVert(i)[i1];
								p[i2] = ld->GetTVVert(i)[i2];
								bounds += p;
							}
						}
					}
				}

				Point3 originalPt = (mod->selCenter + mod->freeFormPivotOffset);
				mod->freeFormPivotOffset = originalPt - bounds.Center();

				//Set the result value to the UVW spins.
				mod->SetupTypeins();

			}
			else if (mod->freeFormSubMode == ID_MOVEPIVOT)
			{

				TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.setPivotOffset"));
				macroRecorder->FunctionCall(mstr, 1, 0,
					mr_point3, &mod->freeFormPivotOffset);

				theHold.Accept(GetString(IDS_PW_PIVOTRESTORE));
			}
			mod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
			mod->InvalidateView();
		}
		break;

	case MOUSE_MOVE: {

		if (mod->freeFormSubMode == ID_MOVE)
		{
			UnwrapMod::SuspendSelSyncGuard suspendSelSync(*mod);
			theHold.Restore();

			float xzoom, yzoom;
			int width, height;
			IPoint2 delta = m - om;
			if (flags&MOUSE_SHIFT && mod->move == 0) {
				if (abs(delta.x) > abs(delta.y)) delta.y = 0;
				else delta.x = 0;
			}
			else if (mod->move == 1 || mod->freeFormMoveAxisLocked == eMoveX) {
				delta.y = 0;
			}
			else if (mod->move == 2 || mod->freeFormMoveAxisLocked == eMoveY) {
				delta.x = 0;
			}
			mod->ComputeZooms(hWnd, xzoom, yzoom, width, height);
			Point2 mv;
			mv.x = delta.x / xzoom;
			mv.y = -delta.y / yzoom;
			//check if moving points or gizmo

			mod->tempVert.x = mv.x;
			mod->tempVert.y = mv.y;
			mod->tempVert.z = 0.0f;

			//convert our sub selection type to vertex selection
			mod->TransferSelectionStart();

			mod->MovePoints(mv);

			//convert our sub selection type to current selection
			mod->TransferSelectionEnd(FALSE, FALSE);

			//Set the delta value to the UVW spins.
			mod->GetUIManager()->SetSpinFValue(ID_SPINNERU, mv.x);
			mod->GetUIManager()->SetSpinFValue(ID_SPINNERV, mv.y);
		}

		else if (mod->freeFormSubMode == ID_ROTATE)
		{
			UnwrapMod::SuspendSelSyncGuard suspendSelSync(*mod);
			theHold.Restore();
			Point3 vecA, vecB;
			Point3 a, b, center;
			a.x = (float)om.x;
			a.y = (float)om.y;
			a.z = 0.f;

			b.x = (float)m.x;
			b.y = (float)m.y;
			b.z = 0.0f;
			center.x = mod->freeFormPivotScreenSpace.x;
			center.y = mod->freeFormPivotScreenSpace.y;
			center.z = 0.0f;

			vecA = Normalize(a - center);
			vecB = Normalize(b - center);
			Point3 cross = CrossProd(vecA, vecB);
			float dot = DotProd(vecA, vecB);
			if (dot >= 1.0f)
			{
				angle = 0.0f;
			}
			else
			{
				if (cross.z < 0.0f)
					angle = acos(DotProd(vecA, vecB));
				else angle = -acos(DotProd(vecA, vecB));
			}

			if (flags&MOUSE_ALT)
			{
				angle = floor(angle * 180.0f / PI) * PI / 180.0f;
			}
			else if (flags&MOUSE_CTRL)
			{
				int iangle = (int)floor(angle * 180.0f / PI);
				int addOffset = iangle % 5;
				angle = (float)(iangle - addOffset) * PI / 180.0f;
			}

			int i1, i2;
			mod->GetUVWIndices(i1, i2);
			if ((i1 == 0) && (i2 == 2)) angle *= -1.0f;
			//convert our sub selection type to vertex selection
			mod->TransferSelectionStart();

			mod->RotatePoints(hWnd, angle);
			//convert our sub selection type to current selection
			mod->TransferSelectionEnd(FALSE, FALSE);

			//set the u spinner to the angle value.
			mod->GetUIManager()->SetSpinFValue(ID_SPINNERU, angle);

		}
		else if (mod->freeFormSubMode == ID_SCALE)
		{
			UnwrapMod::SuspendSelSyncGuard suspendSelSync(*mod);
			theHold.Restore();
			IPoint2 delta = om - m;
			int direction = 0;
			float xScale = 1.0f, yScale = 1.0f;

			{
				xLength = tempXLength;
				yLength = tempYLength;
			}

			if (flags&MOUSE_SHIFT) {
				if (abs(delta.x) > abs(delta.y))
				{
					delta.y = 0;
					direction = 1;
				}
				else
				{
					delta.x = 0;
					direction = 2;
				}
			}


			if (direction == 0)
			{
				if (xLength != 0.0f)
					xScale = 1.0f - (delta.x / xLength);
				else xScale = 0.0f;
				if (yLength != 0.0f)
					yScale = 1.0f - (delta.y / yLength);
				else yScale = 0.0f;
			}
			else if (direction == 1)
			{
				if (xLength != 0.0f)
					xScale = 1.0f - (delta.x / xLength);
				else xScale = 0.0f;

			}
			else if (direction == 2)
			{
				if (yLength != 0.0f)
					yScale = 1.0f - (delta.y / yLength);
				else yScale = 0.0f;
			}
			if (flags&MOUSE_CTRL)
			{
				if (yScale > xScale)
					xScale = yScale;
				else yScale = xScale;
			}

			if (flags&MOUSE_ALT)
			{
				mod->center.x = xScreenPivot;
				mod->center.y = yScreenPivot;
			}
			else
			{
				mod->center.x = tempCenterX;
				mod->center.y = tempCenterY;
			}


			if (mod->ip)
			{
				xScale = GetCOREInterface()->SnapPercent(xScale);
				yScale = GetCOREInterface()->SnapPercent(yScale);
			}

			tempXScale = xScale;
			tempYScale = yScale;

			//convert our sub selection type to vertex selection
			mod->TransferSelectionStart();

			mod->ScalePointsXY(hWnd, xScale, yScale);

			//convert our sub selection type to current selection
			mod->TransferSelectionEnd(FALSE, FALSE);

			mod->freeFormPivotOffset.x *= xScale;
			mod->freeFormPivotOffset.y *= yScale;

			//Set the delta value to the UVW spins.
			mod->GetUIManager()->SetSpinFValue(ID_SPINNERU, xScale*100.0f);
			mod->GetUIManager()->SetSpinFValue(ID_SPINNERV, yScale*100.0f);
			
		}
		else if (mod->freeFormSubMode == ID_MOVEPIVOT)
		{
			UnwrapMod::SuspendSelSyncGuard suspendSelSync(*mod);
			theHold.Restore();
			IPoint2 delta = om - m;
			if (flags&MOUSE_SHIFT)
			{
				if (abs(delta.x) > abs(delta.y))
					delta.y = 0;
				else delta.x = 0;
			}
			float xzoom, yzoom;
			int width, height;

			mod->ComputeZooms(hWnd, xzoom, yzoom, width, height);
			Point3 mv(0.0f, 0.0f, 0.0f);
			int i1, i2;
			mod->GetUVWIndices(i1, i2);
			mv[i1] = -delta.x / xzoom;
			mv[i2] = delta.y / yzoom;
			//				mv.z = 0.0f;
			mod->freeFormPivotOffset += mv;

		}

		if (mod->update && mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
		UpdateWindow(hWnd);
		break;
	}

	case MOUSE_ABORT:

		mod->inRotation = FALSE;
		dragging = FALSE;
		theHold.Cancel();
		if (mod->freeFormSubMode != ID_MOVEPIVOT)
			theHold.SuperCancel();

		if (mod->fnGetConstantUpdate())
			mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);

		if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
		mod->InvalidateView();
		break;
	}

	return 1;
}



int WeldMode::subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	if (mod == nullptr)
	{
		return 0;
	}

	BOOL hitOnlyShared = FALSE;
	mod->pblock->GetValue(unwrap_weldonlyshared, 0, hitOnlyShared, FOREVER);

	switch (msg)
	{
	case MOUSE_POINT:
	{
		if (point == 0) {
			if (hitOnlyShared)
			{
				for (int ldID = 0; ldID < mod->GetMeshTopoDataCount(); ldID++)
				{
					MeshTopoData *ld = mod->GetMeshTopoData(ldID);
					ld->BuildVertexClusterList();
				}
			}
			theHold.Begin();
			mod->HoldPointsAndFaces();
			om = m;
			mod->tWeldHit = -1;
			mod->tWeldHitLD = NULL;
		}
		else {
			if (mod->WeldPoints(hWnd, m))
			{
				theHold.Accept(GetString(IDS_PW_WELD_UVW));
			}
			else {
				theHold.Accept(GetString(IDS_PW_MOVE_UVW));
			}

			if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
		}
		break;
	}
	case MOUSE_MOVE:
	{
		Tab<TVHitData> hits;
		Rect rect;
		if (mod->fnGetTVSubMode() == TVVERTMODE)
		{
			rect.left = m.x - 2;
			rect.right = m.x + 2;
			rect.top = m.y - 2;
			rect.bottom = m.y + 2;
		}
		else
		{
			rect.left = m.x - 3;
			rect.right = m.x + 3;
			rect.top = m.y - 3;
			rect.bottom = m.y + 3;
		}
		mod->tWeldHit = -1;
		mod->tWeldHitLD = NULL;

		BOOL sel = TRUE;

		if (mod->HitTest(rect, hits, FALSE))
		{
			MeshTopoData *selID = NULL;

			int testGeomIndex1 = -1;
			int testGeomIndex2 = -1;
			for (int i = 0; i < hits.Count(); i++)
			{
				MeshTopoData *ld = mod->GetMeshTopoData(hits[i].mLocalDataID);
				int index = hits[i].mID;
				if (mod->fnGetTVSubMode() == TVVERTMODE)
				{
					if (ld->GetTVVertSelected(index))//mod->vsel[hits[i]])
					{
						selID = ld;
						if (hitOnlyShared)
							testGeomIndex1 = ld->GetTVVertGeoIndex(index);
					}
				}
				else if (mod->fnGetTVSubMode() == TVEDGEMODE)
				{
					if (ld->GetTVEdgeSelected(index))//mod->esel[hits[i]])
					{
						selID = ld;
						if (hitOnlyShared)
						{
							testGeomIndex1 = ld->GetTVVertGeoIndex(ld->GetTVEdgeVert(index, 0));
							testGeomIndex2 = ld->GetTVVertGeoIndex(ld->GetTVEdgeVert(index, 1));
						}
					}
				}
			}

			for (int i = 0; i < hits.Count(); i++)
			{
				MeshTopoData *ld = mod->GetMeshTopoData(hits[i].mLocalDataID);
				int index = hits[i].mID;
				if (ld == selID)
				{
					if (mod->fnGetTVSubMode() == TVVERTMODE)
					{
						if (!ld->GetTVVertSelected(index))//mod->vsel[hits[i]])
						{

							if (hitOnlyShared)
							{
								if (ld->GetTVVertGeoIndex(index) == testGeomIndex1)
									sel = FALSE;
							}
							else
								sel = FALSE;
						}
					}
					else if (mod->fnGetTVSubMode() == TVEDGEMODE)
					{
						if (!ld->GetTVEdgeSelected(index))//mod->esel[hits[i]])
						{
							int edgeCount = ld->GetTVEdgeNumberTVFaces(index);//mod->TVMaps.ePtrList[hits[i]]->faceList.Count();
							if (edgeCount == 1)
							{
								if (hitOnlyShared)
								{
									int geomIndex1 = ld->GetTVVertGeoIndex(ld->GetTVEdgeVert(index, 0));
									int geomIndex2 = ld->GetTVVertGeoIndex(ld->GetTVEdgeVert(index, 1));
									if (((geomIndex1 == testGeomIndex1) && (geomIndex2 == testGeomIndex2)) ||
										((geomIndex1 == testGeomIndex2) && (geomIndex2 == testGeomIndex1)))
									{
										sel = FALSE;
										mod->tWeldHit = index;//hits[i];
										mod->tWeldHitLD = ld;
									}
								}
								else
								{
									sel = FALSE;
									mod->tWeldHit = index;//hits[i];
									mod->tWeldHitLD = ld;
								}
							}
						}
					}
				}
			}
		}
		if (!sel)
			SetCursor(mod->weldCurHit);
		else SetCursor(mod->weldCur);

		UnwrapMod::SuspendSelSyncGuard suspendSelSync(*mod);
		theHold.Restore();

		float xzoom, yzoom;
		int width, height;
		IPoint2 delta = m - om;
		if (flags&MOUSE_SHIFT && mod->move == 0) {
			if (abs(delta.x) > abs(delta.y)) delta.y = 0;
			else delta.x = 0;
		}
		else if (mod->move == 1) {
			delta.y = 0;
		}
		else if (mod->move == 2) {
			delta.x = 0;
		}
		mod->ComputeZooms(hWnd, xzoom, yzoom, width, height);
		Point2 mv;
		mv.x = delta.x / xzoom;
		mv.y = -delta.y / yzoom;

		mod->TransferSelectionStart();
		mod->MovePoints(mv);
		mod->TransferSelectionEnd(FALSE, FALSE);

		if (mod->update && mod->ip)
			mod->ip->RedrawViews(mod->ip->GetTime());
		UpdateWindow(hWnd);
		break;
	}
	case MOUSE_ABORT:
	{
		theHold.Cancel();
		//			theHold.SuperCancel();
		if (mod->fnGetConstantUpdate())
			mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
		if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
		break;
	}
	}

	return 1;
}

void PanMode::ResetInitialParams()
{
	oxscroll = mod->xscroll;
	oyscroll = mod->yscroll;

	reset = TRUE;
}

int PanMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	if (mod == nullptr)
	{
		return 0;
	}

	switch (msg) {
	case MOUSE_POINT:
		if (point == 0) {
			om = m;
			oxscroll = mod->xscroll;
			oyscroll = mod->yscroll;
		}
		break;
	case MOUSE_MOVE: {
		//need reset if a MMB Pan follows MMB Scroll.
		if (reset)
		{
			om = m;
			reset = FALSE;
		}

		IPoint2 delta = m - om;
		mod->xscroll = oxscroll + float(delta.x);
		mod->yscroll = oyscroll + float(delta.y);
		mod->InvalidateView();

		if (MaxSDK::Graphics::IsRetainedModeEnabled())
		{
			//Under the Nitrous mode,let the flag keeps true that will not trigger the data of vertex,edge and face update.
			//The world matrix will use the new zoom and scroll value to update the display result.
			mod->viewValid = TRUE;
		}
		else
		{
			//Under the legacy mode, need to dirty this flag to trigger the background redraw.
			mod->tileValid = FALSE;
		}

		SetCursor(GetPanCursor());
		break;
	}
	case MOUSE_ABORT:
		//watje tile
		mod->tileValid = FALSE;

		mod->xscroll = oxscroll;
		mod->yscroll = oyscroll;

		mod->InvalidateView();
		break;
	case MOUSE_FREEMOVE:
		SetCursor(GetPanCursor());
		break;
	}
	return 1;
}

int ZoomMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	if (mod == nullptr)
	{
		return 0;
	}
	switch (msg) {
	case MOUSE_POINT:
		if (point == 0) {
			om = m;
			ozoom = mod->zoom;
			oxscroll = mod->xscroll;
			oyscroll = mod->yscroll;
		}
		break;

	case MOUSE_MOVE: {
		//watje tile
		mod->tileValid = FALSE;

		Rect rect;
		GetClientRect(hWnd, &rect);
		int centerX = (rect.w() - 1) / 2 - om.x;
		int centerY = (rect.h() - 1) / 2 - om.y;

		IPoint2 delta = om - m;
		float z;

		int iDirection = 0;
		MaxSDK::CUI::GetValueOfOperationParameter(MaxSDK::CUI::EOperationParameter_ZoomInDirection, iDirection);
		float fVerticalCoefficience = 1.0;
		float fHorizontalCoefficience = 1.0;
		if (iDirection & MaxSDK::CUI::EZoomInDirection_North)
		{
			fVerticalCoefficience = delta.y < 0 ? (1.0f / (1.0f - ZOOM_FACT*delta.y)) : (1.0f + ZOOM_FACT*delta.y);
		}
		else if (iDirection & MaxSDK::CUI::EZoomInDirection_South)
		{
			fVerticalCoefficience = delta.y < 0 ? (1.0f - ZOOM_FACT*delta.y) : (1.0f / (1.0f + ZOOM_FACT*delta.y));
		}

		if (iDirection & MaxSDK::CUI::EZoomInDirection_East)
		{
			fHorizontalCoefficience = delta.x < 0 ? (1.0f - ZOOM_FACT*delta.x) : (1.0 / (1.0f + ZOOM_FACT*delta.x));
		}
		else if (iDirection & MaxSDK::CUI::EZoomInDirection_West)
		{
			fHorizontalCoefficience = delta.x > 0 ? (1.0f + ZOOM_FACT*delta.x) : (1.0 / (1.0f - ZOOM_FACT*delta.x));;
		}
		z = fVerticalCoefficience * fHorizontalCoefficience;

		mod->zoom = ozoom * z;
		mod->xscroll = (oxscroll + centerX)*z;
		mod->yscroll = (oyscroll + centerY)*z;

		mod->xscroll -= centerX;
		mod->yscroll -= centerY;

		mod->InvalidateView();
		SetCursor(mod->zoomCur);
		break;
	}

	case MOUSE_ABORT:
		//watje tile
		mod->tileValid = FALSE;

		mod->zoom = ozoom;
		mod->xscroll = oxscroll;
		mod->yscroll = oyscroll;
		mod->InvalidateView();
		break;

	case MOUSE_FREEMOVE:
		SetCursor(mod->zoomCur);
		break;
	}
	return 1;
}

int ZoomRegMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	switch (msg) {
	case MOUSE_POINT:
		if (point == 0)
		{
			lm = om = m;
			XORDottedRect(hWnd, om, lm, 0, true);
		}
		else
		{
			if (om != m)
			{
				//watje tile
				mod->tileValid = FALSE;

				Rect rect;
				GetClientRect(hWnd, &rect);
				IPoint2 mcent = (om + m) / 2;
				IPoint2 scent = rect.GetCenter();
				IPoint2 delta = m - om;
				float rat1, rat2;
				if ((delta.x != 0) && (delta.y != 0))
				{
					rat1 = float(rect.w() - 1) / float(fabs((double)delta.x));
					rat2 = float(rect.h() - 1) / float(fabs((double)delta.y));
					float rat = rat1 < rat2 ? rat1 : rat2;
					mod->zoom *= rat;
					delta = scent - mcent;
					mod->xscroll += delta.x;
					mod->yscroll += delta.y;
					mod->xscroll *= rat;
					mod->yscroll *= rat;
				}
			}
			mod->InvalidateView();
		}
		break;
	case MOUSE_MOVE:
		XORDottedRect(hWnd, om, lm, 0, true);
		XORDottedRect(hWnd, om, m);
		lm = m;
		SetCursor(mod->zoomRegionCur);
		break;
	case MOUSE_ABORT:
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case MOUSE_FREEMOVE:
		SetCursor(mod->zoomRegionCur);
		break;
	}
	return 1;
}

int RightMouseMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	if (mod == nullptr)
	{
		return 0;
	}
	switch (msg) {
	case MOUSE_POINT:
	case MOUSE_PROPCLICK:
		int ct = 0;
		for (int ldID = 0; ldID < mod->GetMeshTopoDataCount(); ldID++)
		{
			ct += mod->GetMeshTopoData(ldID)->GetTVVertSel().NumberSet();
		}

		if ((mod->mode == ID_SKETCHMODE) && (mod->sketchSelMode == SKETCH_SELPICK) && (ct > 0))
		{

			mod->sketchSelMode = SKETCH_DRAWMODE;
			if (mod->sketchType == SKETCH_LINE)
				GetCOREInterface()->ReplacePrompt(GetString(IDS_PW_SKETCHPROMPT_LINE));
			else if (mod->sketchType == SKETCH_FREEFORM)
			{
				GetCOREInterface()->ReplacePrompt(GetString(IDS_PW_SKETCHPROMPT_FREEFORM));
			}
			else if (mod->sketchType == SKETCH_BOX)
			{
				GetCOREInterface()->ReplacePrompt(GetString(IDS_PW_SKETCHPROMPT_BOX));
			}
			else if (mod->sketchType == SKETCH_CIRCLE)
			{
				GetCOREInterface()->ReplacePrompt(GetString(IDS_PW_SKETCHPROMPT_CIRCLE));
			}
		}
		//check if in pan zoom or soom region mode
		else if (
			(mod->mode == ID_PAN) ||
			(mod->mode == ID_ZOOMTOOL) ||
			(mod->mode == ID_ZOOMREGION) ||
			(mod->mode == ID_WELD) ||
			(mod->mode == ID_SKETCHMODE) ||
			(mod->mode == ID_TV_PAINTSELECTMODE) ||
			(mod->mode == ID_PAINT_MOVE_BRUSH) ||
			(mod->mode == ID_RELAX_MOVE_BRUSH)
			)
		{
			if (!(
				(mod->oldMode == ID_PAN) ||
				(mod->oldMode == ID_ZOOMTOOL) ||
				(mod->oldMode == ID_ZOOMREGION) ||
				(mod->oldMode == ID_WELD) ||
				(mod->oldMode == ID_SKETCHMODE) ||
				(mod->oldMode == ID_TV_PAINTSELECTMODE) ||
				(mod->oldMode == ID_PAINT_MOVE_BRUSH) ||
				(mod->oldMode == ID_RELAX_MOVE_BRUSH)
				))
			{

				mod->SetMode(mod->oldMode);
			}
			else
			{
				mod->SetMode(ID_MOVE);
			}
		}
		else {
			IMenuContext *pContext = GetCOREInterface()->GetMenuManager()->GetContext(kIUVWUnwrapQuad);
			DbgAssert(pContext);
			DbgAssert(pContext->GetType() == kMenuContextQuadMenu);
			IQuadMenuContext *pQMContext = (IQuadMenuContext *)pContext;
			int curIndex = pQMContext->GetCurrentMenuIndex();
			IQuadMenu *pMenu = pQMContext->GetMenu(curIndex);
			DbgAssert(pMenu);
			pMenu->TrackMenu(hWnd, pQMContext->GetShowAllQuads(curIndex));
		}
		break;
	}
	return 1;
}

void MiddleMouseMode::ResetInitialParams()
{
	oxscroll = mod->xscroll;
	oyscroll = mod->yscroll;
	ozoom = mod->zoom;
	reset = TRUE;
}

int MiddleMouseMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	static int modeType = 0;
	if (mod == nullptr)
	{
		return 0;
	}
	switch (msg) {
	case MOUSE_POINT:
		if (point == 0) {
			inDrag = TRUE;
			BOOL ctrl = flags & MOUSE_CTRL;
			BOOL alt = flags & MOUSE_ALT;

			if (ctrl && alt)
			{
				modeType = ID_ZOOMTOOL;
				ozoom = mod->zoom;
			}
			else modeType = ID_PAN;

			om = m;
			oxscroll = mod->xscroll;
			oyscroll = mod->yscroll;
		}
		//tile
		else
		{
			inDrag = FALSE;
			mod->tileValid = FALSE;
			mod->InvalidateView();

		}
		break;

	case MOUSE_MOVE: {
		if (reset)
		{
			om = m;
		}
		reset = FALSE;
		//watje tile
		mod->tileValid = FALSE;

		if (modeType == ID_PAN)
		{
			IPoint2 delta = m - om;
			mod->xscroll = oxscroll + float(delta.x);
			mod->yscroll = oyscroll + float(delta.y);
			mod->InvalidateView();

			if (MaxSDK::Graphics::IsRetainedModeEnabled())
			{
				//Under the Nitrous mode,let the flag keeps true that will not trigger the data of vertex,edge and face update.
				//The world matrix will use the new zoom and scroll value to update the display result.
				mod->viewValid = TRUE;
			}

			SetCursor(GetPanCursor());
		}
		else if (modeType == ID_ZOOMTOOL)
		{
			IPoint2 delta = om - m;
			float z;
			if (delta.y < 0)
				z = (1.0f / (1.0f - ZOOM_FACT*delta.y));
			else z = (1.0f + ZOOM_FACT*delta.y);
			mod->zoom = ozoom * z;
			mod->xscroll = oxscroll*z;
			mod->yscroll = oyscroll*z;
			mod->InvalidateView();

			SetCursor(mod->zoomCur);
		}
		break;
	}
	case MOUSE_ABORT:
		//watje tile
		mod->tileValid = FALSE;
		inDrag = FALSE;

		mod->xscroll = oxscroll;
		mod->yscroll = oyscroll;
		if (modeType == ID_ZOOMTOOL)
			mod->zoom = ozoom;

		mod->InvalidateView();

		break;
	}
	return 1;
}

bool UnwrapSubModSelectionProcessor::HasOverrideDoubleClickProc() const
{
	return true;
}

void UnwrapSubModSelectionProcessor::OverrideDoubleClickProc(ViewExp* vpt, int flags)
{
	UnwrapMod* pMod = dynamic_cast<UnwrapMod*>(obj);
	if (pMod && pMod->fnGetTVSubMode() == TVEDGEMODE)
	{
		pMod->MouseGuidedEdgeLoopSelect(vpt->GetSubObjHitList().ClosestHit(), flags);
	}
}
