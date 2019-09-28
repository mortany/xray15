/**********************************************************************
 *<
	FILE: EditPolyUISubobj.cpp

	DESCRIPTION: UI code for Edit Subobject dialogs in Edit Poly Modifier

	CREATED BY: Steve Anderson

	HISTORY: created May 2004

 *>	Copyright (c) 2004 Discreet, All Rights Reserved.
 **********************************************************************/

#include "epoly.h"
#include "EditPoly.h"
#include "EditPolyUI.h"

static EditPolySubobjControlDlgProc theSubobjControlDlgProc;
EditPolySubobjControlDlgProc *TheSubobjDlgProc () { return &theSubobjControlDlgProc; }

void EditPolySubobjControlDlgProc::SetEnables (HWND hWnd)
{
	// The only reason we disable things in this dialog is if they're not animatable,
	// and we're in animation mode.
	bool animateMode = mpMod->getParamBlock()->GetInt (epm_animation_mode) != 0;
	bool edg = (mpMod->GetEPolySelLevel() == EPM_SL_EDGE) ? true : false;

#ifndef EDIT_POLY_DISABLE_IN_ANIMATE
	animateMode = false;
#endif

	ICustButton* but = NULL;
	but = GetICustButton (GetDlgItem (hWnd, IDC_INSERT_VERTEX));
	if (but) {
		if (animateMode) but->Disable();
		else but->Enable();
		ReleaseICustButton (but);
	}

	but = GetICustButton (GetDlgItem (hWnd, IDC_FS_EDIT_TRI));
	if (but) {
		if (animateMode) but->Disable();
		else but->Enable();
		ReleaseICustButton (but);
	}

	but = GetICustButton (GetDlgItem (hWnd, IDC_TURN_EDGE));
	if (but) {
		if (animateMode) but->Disable();
		else but->Enable();
		ReleaseICustButton (but);
	}

	but = GetICustButton (GetDlgItem (hWnd, IDC_CREATE_SHAPE));
	if (but) {
		if (animateMode) but->Disable();
		else but->Enable();
		ReleaseICustButton (but);
	}

	but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_CREATE_SHAPE));
	if (but) {
		if (animateMode) but->Disable();
		else but->Enable();
		ReleaseICustButton (but);
	}

	but = GetICustButton (GetDlgItem (hWnd, IDC_HARD_EDGE));
	if (but != NULL) {
		but->SetRightClickNotify(TRUE);
		but->Enable (edg);
		ReleaseICustButton (but);
	}

	but = GetICustButton (GetDlgItem (hWnd, IDC_SMOOTH_EDGE));
	if (but != NULL) {
		but->SetRightClickNotify(TRUE);
		but->Enable (edg);
		ReleaseICustButton (but);
	}
}

void EditPolySubobjControlDlgProc::UpdatePerDataDisplay (TimeValue t, int selLevel, int dataChannel, HWND hWnd) {
	if (!mpMod) return;
	ISpinnerControl *spin=NULL;
	float defaultValue = 1.0f, value = 0.0f;
	int num = 0;
	bool uniform(true);

	switch (selLevel) {
	case EPM_SL_VERTEX:
		switch (dataChannel) {
		case VDATA_WEIGHT:
			spin = GetISpinner (GetDlgItem (hWnd, IDC_VS_WEIGHTSPIN));
			break;
		case VDATA_CREASE:
			spin = GetISpinner (GetDlgItem (hWnd, IDC_VS_CREASESPIN));
			defaultValue = 0.0f;
			break;
		}
		value = mpMod->GetVertexDataFloatValue (dataChannel, &num, &uniform, MN_SEL, NULL);
		break;

	case EPM_SL_EDGE:
	case EPM_SL_BORDER:
		switch (dataChannel) {
		case EDATA_KNOT:
			spin = GetISpinner (GetDlgItem (hWnd, IDC_ES_WEIGHTSPIN));
			break;
		case EDATA_CREASE:
			spin = GetISpinner (GetDlgItem (hWnd, IDC_ES_CREASESPIN));
			defaultValue = 0.0f;
			break;
		}
		value = mpMod->GetEdgeDataFloatValue (dataChannel, &num, &uniform, MN_SEL, NULL);
		break;
	}

	if (!spin) return;

	if (num == 0) {	// Nothing selected - use default.
		spin->SetValue (defaultValue, false);
		spin->SetIndeterminate (true);
	} else {
		if (!uniform) {	// Data not uniform
			spin->SetIndeterminate (TRUE);
		} else {
			// Set the readout.
			spin->SetIndeterminate(FALSE);
			spin->SetValue (value, FALSE);
		}
	}
	ReleaseISpinner(spin);
}

INT_PTR EditPolySubobjControlDlgProc::DlgProc (TimeValue t, IParamMap2 *pmap, HWND hWnd,
						   UINT msg, WPARAM wParam, LPARAM lParam) {
	ICustButton* but = NULL;
	ISpinnerControl* spin = NULL;
	IColorSwatch *swatch = NULL;
	HWND colorOptions = NULL;

	switch (msg) {
	case WM_INITDIALOG:
		but = GetICustButton(GetDlgItem(hWnd,IDC_INSERT_VERTEX));
		if (but) {
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);
		}

		but = GetICustButton(GetDlgItem(hWnd,IDC_BRIDGE));
		if (but) {
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_BRIDGE));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton(GetDlgItem(hWnd,IDC_EXTRUDE));
		if (but) {
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_EXTRUDE));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton(GetDlgItem(hWnd,IDC_BEVEL));
		if (but) {
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_BEVEL));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_OUTLINE));
		if (but) {
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_OUTLINE));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_INSET));
		if (but) {
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_INSET));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton(GetDlgItem(hWnd,IDC_CHAMFER));
		if (but) {
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_CHAMFER));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_BREAK));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_WELD));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton(GetDlgItem(hWnd,IDC_TARGET_WELD));
		if (but) {
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_HINGE_FROM_EDGE));
		if (but) {
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_HINGE_FROM_EDGE));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_EXTRUDE_ALONG_SPLINE));
		if (but) {
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_EXTRUDE_ALONG_SPLINE));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_CONNECT_EDGES));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_CREATE_SHAPE));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		but = GetICustButton(GetDlgItem(hWnd,IDC_FS_EDIT_TRI));
		if (but) {
			but->SetType(CBT_CHECK);
			if (mpMod->GetMNSelLevel() == MNM_SL_EDGE)
				but->SetTooltip (true, GetString (IDS_EDIT_TRIANGULATION));
			ReleaseICustButton(but);
		}

		but = GetICustButton(GetDlgItem(hWnd,IDC_TURN_EDGE));
		if (but) {
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);
		}

		but = GetICustButton (GetDlgItem (hWnd, IDC_SETTINGS_BRIDGE_EDGE));
		if (but) {
			but->SetImage (GetPolySelImageHandler()->LoadPlusMinus(), 4,4,5,5, 12, 12);
			but->SetTooltip (true, GetString (IDS_SETTINGS));
			ReleaseICustButton(but);
		}

		// Set up the weight spinner, if present:
		spin = SetupFloatSpinner (hWnd, IDC_VS_WEIGHTSPIN, IDC_VS_WEIGHT, 0.0f, 9999999.0f, 1.0f, .1f);
		if (spin) {
			spin->SetAutoScale(TRUE);
			ReleaseISpinner (spin);
			spin = NULL;
		}

		// Set up the crease spinner, if present:
		spin = SetupFloatSpinner (hWnd, IDC_VS_CREASESPIN, IDC_VS_CREASE, 0.0f, 1.0f, 1.0f, .1f);
		if (spin) {
			spin->SetAutoScale(TRUE);
			ReleaseISpinner (spin);
			spin = NULL;
		}

		// Set up the edge data spinners, if present:
		spin = SetupFloatSpinner (hWnd, IDC_ES_WEIGHTSPIN, IDC_ES_WEIGHT, 0.0f, 9999999.0f, 1.0f, .1f);
		if (spin) {
			spin->SetAutoScale(TRUE);
			ReleaseISpinner (spin);
			spin = NULL;
		}

		spin = SetupFloatSpinner (hWnd, IDC_ES_CREASESPIN, IDC_ES_CREASE, 0.0f, 1.0f, 1.0f, .1f);
		if (spin) {
			spin->SetAutoScale(TRUE);
			ReleaseISpinner (spin);
			spin = NULL;
		}

		mpMod->UpdateHardEdgeColorDisplay(hWnd);
		
		// Load up Hard Edge display option
		mpMod->UpdateHardEdgeDisplayType(hWnd);

		SetEnables (hWnd);
		mUIValid = false;
		break;

	case WM_PAINT:
		if (mUIValid) return FALSE;
		switch (mpMod->GetMNSelLevel()) {
		case MNM_SL_VERTEX: 
			UpdatePerDataDisplay (t, EPM_SL_VERTEX, VDATA_WEIGHT, hWnd);
			UpdatePerDataDisplay (t, EPM_SL_VERTEX, VDATA_CREASE, hWnd);
			break;
		case MNM_SL_EDGE: 
			UpdatePerDataDisplay (t, EPM_SL_EDGE, EDATA_KNOT, hWnd);
			UpdatePerDataDisplay (t, EPM_SL_EDGE, EDATA_CREASE, hWnd);
			break;
		}
		mUIValid = true;
		return FALSE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_REMOVE:
			switch (mpMod->GetMNSelLevel()) {
			case MNM_SL_VERTEX: 
				mpMod->EpModButtonOp (ep_op_remove_vertex); 
				break;
			case MNM_SL_EDGE: 
				mpMod->EpModButtonOp (GetKeyState(VK_CONTROL)<0 ? ep_op_remove_edge_remove_verts: ep_op_remove_edge); 
				break;
			}
			break;

		case IDC_BREAK:
			mpMod->EpModButtonOp (ep_op_break);
			break;

		case IDC_SETTINGS_BREAK:
			mpMod->EpModPopupDialog (ep_op_break); 
			break;

		case IDC_SPLIT:
			mpMod->EpModButtonOp (ep_op_split);
			break;

		case IDC_INSERT_VERTEX:
			switch (mpMod->GetMNSelLevel()) {
			case MNM_SL_EDGE: mpMod->EpModToggleCommandMode (ep_mode_divide_edge); break;
			case MNM_SL_FACE: mpMod->EpModToggleCommandMode (ep_mode_divide_face); break;
			}
			break;

		case IDC_CAP:
			mpMod->EpModButtonOp (ep_op_cap);
			break;

		case IDC_EXTRUDE:
			switch (mpMod->GetMNSelLevel()) {
			case MNM_SL_VERTEX: mpMod->EpModToggleCommandMode (ep_mode_extrude_vertex); break;
			case MNM_SL_EDGE: mpMod->EpModToggleCommandMode (ep_mode_extrude_edge); break;
			case MNM_SL_FACE: mpMod->EpModToggleCommandMode (ep_mode_extrude_face); break;
			}
			break;

		case IDC_SETTINGS_EXTRUDE:
			switch (mpMod->GetMNSelLevel()) {
			case MNM_SL_VERTEX: mpMod->EpModPopupDialog (ep_op_extrude_vertex); break;
			case MNM_SL_EDGE: mpMod->EpModPopupDialog (ep_op_extrude_edge); break;
			case MNM_SL_FACE: mpMod->EpModPopupDialog (ep_op_extrude_face); break;
			}
			break;

		case IDC_BEVEL:
			mpMod->EpModToggleCommandMode (ep_mode_bevel);
			break;

		case IDC_SETTINGS_BEVEL:
			mpMod->EpModPopupDialog (ep_op_bevel);
			break;

		case IDC_OUTLINE:
			mpMod->EpModToggleCommandMode (ep_mode_outline);
			break;

		case IDC_SETTINGS_OUTLINE:
			mpMod->EpModPopupDialog (ep_op_outline);
			break;

		case IDC_INSET:
			mpMod->EpModToggleCommandMode (ep_mode_inset_face);
			break;

		case IDC_SETTINGS_INSET:
			mpMod->EpModPopupDialog (ep_op_inset);
			break;

		case IDC_CHAMFER:
			switch (mpMod->GetMNSelLevel()) {
			case MNM_SL_VERTEX:
				mpMod->EpModToggleCommandMode (ep_mode_chamfer_vertex);
				break;
			case MNM_SL_EDGE:
				mpMod->EpModToggleCommandMode (ep_mode_chamfer_edge);
				break;
			}
			break;

		case IDC_SETTINGS_CHAMFER:
			switch (mpMod->GetMNSelLevel()) {
			case MNM_SL_VERTEX: mpMod->EpModPopupDialog (ep_op_chamfer_vertex); break;
			case MNM_SL_EDGE: mpMod->EpModPopupDialog (ep_op_chamfer_edge); break;
			}
			break;

		case IDC_WELD:
			switch (mpMod->GetEPolySelLevel()) {
			case EPM_SL_VERTEX: mpMod->EpModButtonOp (ep_op_weld_vertex); break;
			case EPM_SL_EDGE: mpMod->EpModButtonOp (ep_op_weld_edge); break;
			}
			break;

		case IDC_SETTINGS_WELD:
			switch (mpMod->GetEPolySelLevel()) {
			case EPM_SL_VERTEX: mpMod->EpModPopupDialog (ep_op_weld_vertex); break;
			case EPM_SL_EDGE: mpMod->EpModPopupDialog (ep_op_weld_edge); break;
			}
			break;

		case IDC_TARGET_WELD:
			mpMod->EpModToggleCommandMode (ep_mode_weld);
			break;

		case IDC_REMOVE_ISO_VERTS:
			mpMod->EpModButtonOp (ep_op_remove_iso_verts);
			break;

		case IDC_REMOVE_ISO_MAP_VERTS:
			mpMod->EpModButtonOp (ep_op_remove_iso_map_verts);
			break;

		case IDC_CREATE_SHAPE:
			mpMod->EpModButtonOp (ep_op_create_shape);
			break;

		case IDC_SETTINGS_CREATE_SHAPE:
			mpMod->EpModPopupDialog (ep_op_create_shape);
			break;

		case IDC_CONNECT_EDGES:
			mpMod->EpModButtonOp (ep_op_connect_edge);
			break;

		case IDC_SETTINGS_CONNECT_EDGES:
			mpMod->EpModPopupDialog (ep_op_connect_edge);
			break;

		case IDC_CONNECT_VERTICES:
			mpMod->EpModButtonOp (ep_op_connect_vertex);
			break;

		case IDC_HINGE_FROM_EDGE:
			mpMod->EpModToggleCommandMode (ep_mode_hinge_from_edge);
			break;

		case IDC_SETTINGS_HINGE_FROM_EDGE:
			mpMod->EpModPopupDialog (ep_op_hinge_from_edge);
			break;

		case IDC_EXTRUDE_ALONG_SPLINE:
			mpMod->EpModEnterPickMode (ep_mode_pick_shape);
			break;

		case IDC_SETTINGS_EXTRUDE_ALONG_SPLINE:
			mpMod->EpModPopupDialog (ep_op_extrude_along_spline);
			break;

		case IDC_BRIDGE:
			int currentMode;
			currentMode = mpMod->EpModGetCommandMode ();
			if ((currentMode != ep_op_bridge_border) && 
				(currentMode != ep_op_bridge_polygon) &&
				(currentMode != ep_op_bridge_edge) &&
				mpMod->EpModReadyToBridgeSelected()) {

				if (mpMod->GetEPolySelLevel() == EPM_SL_BORDER) {
					mpMod->EpModButtonOp (ep_op_bridge_border);
				} else if (mpMod->GetEPolySelLevel() == EPM_SL_FACE) {
					mpMod->EpModButtonOp (ep_op_bridge_polygon);
				} else if (mpMod->GetEPolySelLevel() == EPM_SL_EDGE) {
					mpMod->EpModButtonOp (ep_op_bridge_edge);
				}
				// Don't leave the button pressed-in.
				but = GetICustButton(GetDlgItem(hWnd,IDC_BRIDGE));
				if (but) {
					but->SetCheck (false);
					ReleaseICustButton (but);
				}
			} else {
				switch (mpMod->GetEPolySelLevel()) {
				case EPM_SL_BORDER: 
					mpMod->EpModToggleCommandMode (ep_mode_bridge_border); 
					break;
				case EPM_SL_FACE: 
					mpMod->EpModToggleCommandMode (ep_mode_bridge_polygon); 
					break;
				case EPM_SL_EDGE: 
					mpMod->EpModToggleCommandMode (ep_mode_bridge_edge); 
					break;
				}
			}
			break;

		case IDC_SETTINGS_BRIDGE:
			switch (mpMod->GetEPolySelLevel()) {
			case EPM_SL_BORDER: 
				mpMod->EpModPopupDialog (ep_op_bridge_border); 
				break;
			case EPM_SL_FACE: 
				mpMod->EpModPopupDialog (ep_op_bridge_polygon); 
				break;
			case EPM_SL_EDGE: 
				mpMod->EpModPopupDialog (ep_op_bridge_edge); 
				break;
			}
			break;

		case IDC_FS_EDIT_TRI:
			mpMod->EpModToggleCommandMode (ep_mode_edit_tri);
			break;

		case IDC_TURN_EDGE:
			mpMod->EpModToggleCommandMode (ep_mode_turn_edge);
			break;

		case IDC_FS_RETRIANGULATE:
			mpMod->EpModButtonOp (ep_op_retriangulate);
			break;

		case IDC_FS_FLIP_NORMALS:
			switch (mpMod->GetEPolySelLevel()) {
			case EPM_SL_FACE: mpMod->EpModButtonOp (ep_op_flip_face); break;
			case EPM_SL_ELEMENT: mpMod->EpModButtonOp (ep_op_flip_element); break;
			}
			break;

		case IDC_HARD_EDGE:
			if(HIWORD(wParam) == BN_RIGHTCLICK)
				mpMod->EpModButtonOp (ep_op_select_hard_edges);
			else
				mpMod->EpModButtonOp (ep_op_make_hard_edges);
			break;

		case IDC_SMOOTH_EDGE:
			if(HIWORD(wParam) == BN_RIGHTCLICK)
				mpMod->EpModButtonOp (ep_op_select_smooth_edges);
			else
				mpMod->EpModButtonOp (ep_op_make_smooth_edges);
			break;

		case IDC_HARD_EDGE_DISPLAY: {
			int option = GetCheckBox(hWnd, IDC_HARD_EDGE_DISPLAY) ? 1 : 0;
			theHold.Begin ();
			mpMod->getParamBlock()->SetValue(epm_hard_edge_display, 0, option);
			theHold.Accept(GetString(IDS_CHANGE_HARD_EDGE_DISPLAY));
			}
			break;

		}
		break;

	case CC_SPINNER_BUTTONDOWN:
		switch (LOWORD(wParam)) {
		case IDC_VS_WEIGHTSPIN:
		case IDC_VS_CREASESPIN:
			if (!mSpinningWC) {
				theHold.Begin ();
				mpMod->EpModSetOperation (ep_op_set_vertex_data);
				mSpinningWC = true;
			}
			break;
		case IDC_ES_WEIGHTSPIN:
		case IDC_ES_CREASESPIN:
			if (!mSpinningWC) {
				theHold.Begin ();
				mpMod->EpModSetOperation (ep_op_set_edge_data);
				mSpinningWC = true;
			}
			break;
		}
		break;

	case CC_COLOR_DROP:
	case CC_COLOR_CLOSE:
		if(LOWORD(wParam) == IDC_HARD_COLOR) {
			swatch = GetIColorSwatch(GetDlgItem(hWnd, IDC_HARD_COLOR));
			if(swatch != NULL) {
				// Grab color from the swatch and stuff it into the parameter block
				Color theColor(swatch->GetColor());
				theHold.Begin ();
				mpMod->getParamBlock()->SetValue(epm_hard_edge_display_color, 0, theColor);
				theHold.Accept(GetString(IDS_CHANGE_HARD_EDGE_COLOR));
				ReleaseIColorSwatch(swatch);
			}
		}
		break;

	case WM_CUSTEDIT_ENTER:
	case CC_SPINNER_BUTTONUP:
		if (!mSpinningWC)
			break;
		if (HIWORD(wParam) || msg==WM_CUSTEDIT_ENTER)
		{
			mpMod->EpModCommitUnlessAnimating (t);
			switch (LOWORD(wParam)) {
				case IDC_VS_WEIGHT:
				case IDC_VS_WEIGHTSPIN:
				case IDC_ES_WEIGHT:
				case IDC_ES_WEIGHTSPIN:
					theHold.Accept (GetString(IDS_CHANGEWEIGHT));
					break;

				case IDC_VS_CREASE:
				case IDC_VS_CREASESPIN:
				case IDC_ES_CREASE:
				case IDC_ES_CREASESPIN:
					theHold.Accept (GetString(IDS_CHANGE_CREASE_VALS));
					break;
			}
		}
		else theHold.Cancel();
		mpMod->EpModRefreshScreen ();
		mSpinningWC = false;
		break;

	case CC_SPINNER_CHANGE:
		spin = (ISpinnerControl*)lParam;
		switch (LOWORD(wParam)) {
		case IDC_VS_WEIGHTSPIN:
		case IDC_VS_CREASESPIN:
			if (!mSpinningWC) {
				theHold.Begin ();
				mpMod->EpModSetOperation (ep_op_set_vertex_data);
				mSpinningWC = true;
			}
			mpMod->getParamBlock()->SetValue (epm_data_channel, t, (LOWORD(wParam)==IDC_VS_WEIGHTSPIN) ? VDATA_WEIGHT : VDATA_CREASE);	
			break;
		case IDC_ES_WEIGHTSPIN:
		case IDC_ES_CREASESPIN:
			if (!mSpinningWC) {
				theHold.Begin ();
				mpMod->EpModSetOperation (ep_op_set_edge_data);
				mSpinningWC = true;
			}
			mpMod->getParamBlock()->SetValue (epm_data_channel, t, (LOWORD(wParam)==IDC_ES_WEIGHTSPIN) ? EDATA_KNOT : EDATA_CREASE);	
			break;
		}
		mpMod->getParamBlock()->SetValue (epm_data_value, t, spin->GetFVal());
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

