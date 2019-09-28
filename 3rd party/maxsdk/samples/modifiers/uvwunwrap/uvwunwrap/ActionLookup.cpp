/*

Copyright 2010 Autodesk, Inc.  All rights reserved. 

Use of this software is subject to the terms of the Autodesk license agreement provided at 
the time of installation or download, or which otherwise accompanies this software in either 
electronic or hard copy form. 

*/

#include "unwrap.h"
#include "modsres.h"

inline int GetDPIRevisedValue(int input, bool bRescale)
{
	return (bRescale? MaxSDK::UIScaled(input) : input);
}

int UnwrapMod::AddActionToToolbar(ToolBarFrame *toolBar, int id, bool bRescale)
{
	int buttonWidth = GetDPIRevisedValue(28, bRescale);
	int buttonHeight = GetDPIRevisedValue(28, bRescale);

	int buttonImageWidth = GetDPIRevisedValue(24, bRescale);
	int buttonImageHeight = GetDPIRevisedValue(24, bRescale);
	
	int toolBarSize = GetDPIRevisedValue(iToolBarHeight, bRescale);
	
	int spinnerWidth = GetDPIRevisedValue(18, bRescale);
	int spinnerHeight = GetDPIRevisedValue(16, bRescale);
	int editorHeight = GetDPIRevisedValue(16, bRescale);
	int comboHeight = GetDPIRevisedValue(16, bRescale);
	int staticHeight = GetDPIRevisedValue(14, bRescale);

	bool isPixelUnitSpinner = false;

	int ctrlSize = buttonWidth;
	switch (id)
	{
//SEPARATORS
	case ID_SEPARATOR4:
		{
			ctrlSize = 4 * toolBarSize;
			toolBar->GetToolBar()->AddTool(ToolOtherItem(_M("static"), ctrlSize, staticHeight, id,WS_CHILD|WS_VISIBLE, -1, _M(" "), 0));
			break;
		}
	case ID_SEPARATOR3:
		{
			ctrlSize = 3 * toolBarSize;
			toolBar->GetToolBar()->AddTool(ToolOtherItem(_M("static"), ctrlSize, staticHeight, id,WS_CHILD|WS_VISIBLE, -1, _M(" "), 0));
			break;
		}
	case ID_SEPARATOR2:
		{
			ctrlSize = 2 * toolBarSize;
			toolBar->GetToolBar()->AddTool(ToolOtherItem(_M("static"), ctrlSize, staticHeight, id,WS_CHILD|WS_VISIBLE, -1, _M(" "), 0));
			break;
		}
	case ID_SEPARATOR1:
		{
			ctrlSize = 1 * toolBarSize;
			toolBar->GetToolBar()->AddTool(ToolOtherItem(_M("static"), ctrlSize, staticHeight, id,WS_CHILD|WS_VISIBLE, -1, _M(" "), 0));
			break;
		}

	case ID_SEPARATORHALF:
		{
			ctrlSize = toolBarSize / 2;
			toolBar->GetToolBar()->AddTool(ToolOtherItem(_M("static"), ctrlSize, staticHeight, id,WS_CHILD|WS_VISIBLE, -1, _M(" "), 0));
			break;
		}

	case ID_SEPARATORBAR:
		{
			ctrlSize = GetDPIRevisedValue(10, bRescale);
			toolBar->GetToolBar()->AddTool(ToolOtherItem(_M("static"), ctrlSize, staticHeight, id,WS_CHILD|WS_VISIBLE, -1, _M(" |"), 0));
			break;
		}

	case ID_MIRROR_GEOM_SELECTION:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SymmetricalSelection"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_MIRROR_GEOM_SELECTION));
		}
		break;
	case ID_MIRROR_SEL_X_AXIS:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/AlignToX"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_MIRROR_SEL_X_AXIS));
		}
		break;
	case ID_MIRROR_SEL_Y_AXIS:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/AlignToY"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_MIRROR_SEL_Y_AXIS));
		}
		break;
	case ID_MIRROR_SEL_Z_AXIS:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/AlignToZ"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_MIRROR_SEL_Z_AXIS));
		}
		break;

//TVSELECTIONS
	case ID_TV_VERTMODE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/Vertex"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
								GetString(IDS_TOOLTIP_TV_VERT_MODE));
			break;
		}
	case ID_TV_EDGEMODE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/Edge"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_EDGE_MODE));
			break;
		}
	case ID_TV_FACEMODE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/Polygon"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_FACE_MODE));
			break;
		}
	case ID_TV_ELEMENTMODE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SelectByElementUVToggle"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_ELEMENT_MODE));
			break;
		}
	case ID_TV_INCSEL:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/GrowUVSelection"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_INCSEL));
			break;
		}
	case ID_TV_DECSEL:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/ShrinkUVSelection"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_DECSEL));
			break;
		}
	case ID_TV_LOOP:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/LoopUV"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_LOOP));
			break;
		}
	case ID_TV_LOOPGROW:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/GrowLoopUV"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_LOOP_GROW));
			break;
		}
	case ID_TV_LOOPSHRINK:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/ShrinkLoopUV"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_LOOP_SHRINK));
			break;
		}

	case ID_TV_RING:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/RingUV"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_RING));
			break;
		}

	case ID_TV_RINGGROW:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/GrowRingUV"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_RING_GROW));
			break;
		}

	case ID_TV_RINGSHRINK:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/ShrinkRingUV"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_RING_SHRINK));
			break;
		}

	case ID_TV_PAINTSELECTMODE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/UVPaintSelection"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_PAINTSELECTMODE));
			break;
		}
	case ID_TV_PAINTSELECTINC:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/EnlargeBrushSize"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_PAINTSELECTINC));
			break;
		}
	case ID_TV_PAINTSELECTDEC:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/ShrinkBrushSize"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TV_PAINTSELECTDEC));
			break;
		}


//SOFT SELECTION
	case ID_SOFTSELECTION:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SoftSelection"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_SOFTSELECTION));
			break;
		}
	case ID_SOFTSELECTIONSTR:
	case ID_SOFTSELECTIONSTR_TEXT:					//this ID comes when we load since this is item is 9 elements
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(_M("static"), GetDPIRevisedValue(4, bRescale), staticHeight, ID_SOFTSELECTIONSTR_TEXT,WS_CHILD|WS_VISIBLE, -1, GetString(IDS_EMPTY), 0));
			ctrlSize = GetDPIRevisedValue(4, bRescale);
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(38, bRescale), editorHeight, ID_SOFTSELECTIONSTR_EDIT,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += GetDPIRevisedValue(38, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth,	spinnerHeight, ID_SOFTSELECTIONSTR_SPINNER,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;
			ISpinnerControl* uSpin = GetISpinner(itoolBar->GetItemHwnd(ID_SOFTSELECTIONSTR_SPINNER));
			uSpin->LinkToEdit(itoolBar->GetItemHwnd(ID_SOFTSELECTIONSTR_EDIT),EDITTYPE_FLOAT);
			uSpin->SetLimits(-9999999, 9999999, FALSE);
			uSpin->SetAutoScale();
			ReleaseISpinner(uSpin);

			break;
		}

	case ID_FALLOFF_SPACE:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/SoftSelectionFallOffInXY"));
			flyOffIcons.append(_M("EditUVW/SoftSelectionFallOffInUV"));
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SoftSelectionFallOffInXY"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons,falloffSpace);
			break;
		}
	case ID_FALLOFF:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/SoftSelectionLinearFallOff"));
			flyOffIcons.append(_M("EditUVW/SoftSelectionSmoothFallOff"));
			flyOffIcons.append(_M("EditUVW/SoftSelectionSlowOutFallOff"));
			flyOffIcons.append(_M("EditUVW/SoftSelectionFastOutFallOff"));
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SoftSelectionLinearFallOff"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons,0);
			break;
		}


	case ID_LIMITSOFTSEL:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/LimitSoftSelectionByEdges"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_LIMITSOFTSEL));
			break;
		}
	case ID_SOFTSELECTIONLIMIT:
	case ID_SOFTSELECTIONLIMIT_TEXT:					//this ID comes when we load since this is item is 9 elements
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(_M("static"), GetDPIRevisedValue(4, bRescale), staticHeight, ID_SOFTSELECTIONLIMIT_TEXT,WS_CHILD|WS_VISIBLE, -1, GetString(IDS_EMPTY), 0));
			ctrlSize = GetDPIRevisedValue(4, bRescale);
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(22, bRescale), editorHeight, ID_SOFTSELECTIONLIMIT_EDIT,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += GetDPIRevisedValue(22, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_SOFTSELECTIONLIMIT_SPINNER,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;
			ISpinnerControl* uSpin = GetISpinner(itoolBar->GetItemHwnd(ID_SOFTSELECTIONLIMIT_SPINNER));
			uSpin->LinkToEdit(itoolBar->GetItemHwnd(ID_SOFTSELECTIONLIMIT_EDIT),EDITTYPE_INT);
			uSpin->SetLimits(-9999999, 9999999, FALSE);
			uSpin->SetAutoScale();
			ReleaseISpinner(uSpin);

			break;
		}
//GEOM SELECTION -- reusing related uv sel icons
	case ID_GEOM_ELEMENT:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SelectByElement"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_GEOM_ELEMENT_MODE));
			break;
		}
	case ID_GEOMEXPANDFACESEL:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/GrowUVSelection"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_GEOM_EXPANDSEL));
			break;
		}
	case ID_GEOMCONTRACTFACESEL:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/ShrinkUVSelection"), buttonImageWidth, buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_GEOM_CONTRACTSEL));
			break;
		}
	case ID_EDGELOOPSELECTION:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/LoopUV"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_GEOM_LOOP));
			break;
		}
	case ID_EDGERINGSELECTION:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/RingUV"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_GEOM_RING));
			break;
		}
	case ID_IGNOREBACKFACE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/IgnoreBackfacing"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_GEOM_IGNOREBACKFACE));
			break;
		}
	case ID_POINT_TO_POINT_SEL:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/PointToPointEdgeSelection"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_GEOM_POINT_TO_POINT_SEL));
			break;
		}
	case ID_PLANARMODE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SelectByPlanarAngle"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_GEOM_PLANARMODE));
			ctrlSize = buttonWidth;
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(30, bRescale), editorHeight, ID_PLANAREDIT,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += GetDPIRevisedValue(30, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_PLANARSPIN,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;
			ISpinnerControl* uSpin = GetISpinner(itoolBar->GetItemHwnd(ID_PLANARSPIN));
			uSpin->LinkToEdit(itoolBar->GetItemHwnd(ID_PLANAREDIT),EDITTYPE_FLOAT);
			uSpin->SetLimits(0, 180, FALSE);
			uSpin->SetAutoScale();
			uSpin->SetValue(fnGetGeomPlanarModeThreshold(), FALSE);
			ReleaseISpinner(uSpin);

			break;
		}
	case ID_SET_MIRROR_THRESHOLD_EDIT:
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(48, bRescale), editorHeight, ID_SET_MIRROR_THRESHOLD_EDIT, WS_CHILD | WS_VISIBLE, -1, NULL, 0));
			ctrlSize = GetDPIRevisedValue(48, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_SET_MIRROR_THRESHOLD_SPINNER, WS_CHILD | WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;
			ISpinnerControl* uSpin1 = GetISpinner(itoolBar->GetItemHwnd(ID_SET_MIRROR_THRESHOLD_SPINNER));
			uSpin1->LinkToEdit(itoolBar->GetItemHwnd(ID_SET_MIRROR_THRESHOLD_EDIT), EDITTYPE_UNIVERSE);
			uSpin1->SetLimits(0.0001f, 100.0f, FALSE);
			uSpin1->SetScale(0.0005f);
			uSpin1->SetValue(fnGetMirrorThreshold(), FALSE);
			uSpin1->SetTooltip(TRUE, GetString(IDS_SET_MIRROR_THRESHOLD));
			ReleaseISpinner(uSpin1);

			break;
		}
		break;
	case ID_SELECTBY_SMGRP:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SelectBySmoothingGroup"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_GEOM_SELECTBYSMGRP));
			ctrlSize = buttonWidth;
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(27, bRescale), editorHeight, ID_SMGRPEDIT,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += GetDPIRevisedValue(27, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_SMGRPSPIN,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;
			ISpinnerControl* uSpin = GetISpinner(itoolBar->GetItemHwnd(ID_SMGRPSPIN));
			uSpin->LinkToEdit(itoolBar->GetItemHwnd(ID_SMGRPEDIT),EDITTYPE_INT);
			uSpin->SetLimits(1, 32, FALSE);
			uSpin->SetAutoScale();
			uSpin->SetValue(0, FALSE);
			ReleaseISpinner(uSpin);
			break;
		}			

//TRANSFORM MODES
	case ID_MOVE:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/MoveSubObjects"));
			flyOffIcons.append(_M("EditUVW/MoveHorizontallySubObjects"));
			flyOffIcons.append(_M("EditUVW/MoveVerticallySubObjects"));
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/MoveSubObjects"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons,move);

			break;
		}
	case ID_ROTATE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("Common/RotateCW"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_MODE_ROTATE));
			break;
		}
	case ID_SCALE:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/ScaleSubObjects"));
			flyOffIcons.append(_M("EditUVW/ScaleHorizontallySubObjects"));
			flyOffIcons.append(_M("EditUVW/ScaleVerticallySubObjects"));
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/ScaleSubObjects"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons,scale);

			break;
		}
	case ID_FREEFORMMODE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/FreeformMode"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_MODE_FREEFORM));
			break;
		}
	case ID_WELD:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/TargetWeld"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_MODE_WELDTARGET));
			break;
		}

//TOOLS
	case ID_MIRROR:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/MirrorVerticallySubObjects"));
			flyOffIcons.append(_M("EditUVW/MirrorHorizontallySubObjects"));
			flyOffIcons.append(_M("EditUVW/FlipHorizontallySubObjects"));
			flyOffIcons.append(_M("EditUVW/FlipVerticallySubObjects"));
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/MirrorVerticallySubObjects"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons,mirror);

			break;
		}
	case ID_BREAK:
	case ID_BREAKBYSUBOBJECT:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/Break"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_BREAK));
			break;
		}

	case ID_TOOL_WELDDROPDOWN:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/WeldSubObject"));
			flyOffIcons.append(_M("EditUVW/WeldAllSeams"));
			flyOffIcons.append(_M("EditUVW/WeldAnyMatchWithSelected"));
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/WeldSubObject"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons,0);
			break;
		}
	case ID_WELDTHRESH_EDIT:
		{
			ICustToolbar *itoolBar = toolBar->GetToolBar();

			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(40, bRescale), editorHeight, ID_WELDTHRESH_EDIT,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize = GetDPIRevisedValue(40, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_WELDTHRESH_SPINNER,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;
			isPixelUnitSpinner = true; // Special handling for Pixel Unit spinner

			break;
		}
	case ID_ROTATE_90_CCW:
		{			
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/RotateMinus90AroundPivot"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_ROTATE_90_CCW));
			break;
		}
	case ID_ROTATE_90_CW:
		{			
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/Rotate90AroundPivot"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_ROTATE_90_CW));
			break;
		}

	case ID_FREEFORMSNAP:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/PivotUpperLeft"));
			flyOffIcons.append(_M("EditUVW/PivotUpperRight"));
			flyOffIcons.append(_M("EditUVW/PivotLowerLeft"));
			flyOffIcons.append(_M("EditUVW/PivotLowerRight"));
			flyOffIcons.append(_M("EditUVW/PivotCenter"));
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/PivotCenter"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons, 4);

			break;
		}
	case ID_TOOL_ALIGN_LINEAR:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/LinearAlign"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_ALIGN_LINEAR));
			break;
		}
	case ID_TOOL_SPACE_VERTICAL:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/SpaceVertically"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_SPACE_VERTICAL));
			break;
		}
	case ID_TOOL_SPACE_HORIZONTAL:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/SpaceHorizontally"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_SPACE_HORIZONTAL));
			break;
		}
	case ID_TOOL_ALIGN_ELEMENT:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/AlignToEdge"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_ALIGN_ELEMENT));
			break;
		}

	case ID_STRAIGHTEN:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/StraightenSelection"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_STRAIGHTEN));
			break;
		}

	case ID_ALIGNH_BUTTONS:  
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/AlignHorizontallyToPivot"));
			flyOffIcons.append(_M("EditUVW/AlignHorizontallyInPlace"));
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/AlignHorizontallyToPivot"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons,0);
			break;
			break;
		}
	case ID_ALIGNV_BUTTONS:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/AlignVerticallyToPivot"));
			flyOffIcons.append(_M("EditUVW/AlignVerticallyInPlace"));
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/AlignVerticallyToPivot"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons, 0);
			break;
		}

	case ID_RELAXONECLICK:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/RelaxUntilFlat"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_RELAXONECLICK));
			break;
		}
	case ID_RELAXBUTTONS:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/RelaxCustom"));
			flyOffIcons.append(_M("EditUVW/RelaxTool"));
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/RelaxCustom"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons,0);
			break;
		}
	case ID_STITCHBUTTONS:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/StitchCustom"));
			flyOffIcons.append(_M("EditUVW/StitchTool"));
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/StitchCustom"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons,0);
			break;
		}
	case ID_STITCHDIALOG:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/StitchTool"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_STITCHDIALOG));
			break;
		}
	case ID_STITCHSOURCE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/StitchToSource"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_STITCHSOURCE));
			break;

		}
	case ID_STITCHAVERAGE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/StitchToAverage"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_STITCHAVERAGE));
			break;
		}
	case ID_STITCHTARGET:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/StitchToTarget"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_STITCHTARGET));
			break;
		}	
	case ID_FLATTENMAP:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/FlattenByPolygonAngle"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_FLATTENMAP));
			break;
		}	
	case ID_FLATTENBYMATID:   //this is our old by material thta flattens all the material IDs into there own quadrants 
	case ID_FLATTEN_BYMATID:  //this is the new one where the mat id is used to define cluster
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/FlattenByMaterialID"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_TOOLTIP_TOOL_FLATTENMAPBYMATID));
			break;
		}	
	case ID_FLATTEN_BYSMOOTHINGGROUP:  //this is the new one where the mat id is used to define cluster
	{
		toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/FlattenBySmoothingGroup"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
			GetString(IDS_TOOLTIP_TOOL_FLATTENMAPBYSMOOTHINGGROUP));
		break;
	}	

	case ID_FLATTENBUTTONS:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/FlattenCustom"));
			flyOffIcons.append(_M("EditUVW/FlattenTool"));
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/FlattenCustom"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				flyOffIcons,0);
			break;	
		}

	case ID_FLATTENMAPDIALOG:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/FlattenTool"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_FLATTENMAPDIALOG));
			break;
		}
	case ID_PEEL_DETACH:
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(_T("BUTTON"), GetDPIRevisedValue(62, bRescale),	staticHeight, ID_PEEL_DETACH,WS_CHILD|WS_VISIBLE|BS_CHECKBOX|BS_VCENTER, -1, GetString(IDS_EMPTY), 0));
			HWND hwnd = itoolBar->GetItemHwnd(ID_PEEL_DETACH);
			if (sPeelDetach)
				SendMessage(hwnd,BM_SETCHECK,BST_CHECKED,0);
			else
				SendMessage(hwnd,BM_SETCHECK,BST_UNCHECKED,0);
			break;
		}
	case ID_AUTO_PACK:
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(_T("BUTTON"), GetDPIRevisedValue(62, bRescale), staticHeight, ID_AUTO_PACK, WS_CHILD|WS_VISIBLE|BS_CHECKBOX|BS_VCENTER, -1, GetString(IDS_EMPTY), 0));
			HWND hwnd = itoolBar->GetItemHwnd(ID_AUTO_PACK);
			if (sAutoPack)
				SendMessage(hwnd,BM_SETCHECK,BST_CHECKED,0);
			else
				SendMessage(hwnd,BM_SETCHECK,BST_UNCHECKED,0);
			break;
		}
	case ID_LSCM_INTERACTIVE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/PeelMode"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_LSCM_INTERACTIVE));
			break;
		}
	case ID_LSCM_SOLVE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/QuickPeel"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_LSCM_SOLVE));
			break;
		}
	case ID_LSCM_RESET:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/ResetPeel"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_LSCM_RESET));
			break;
		}
	case ID_PIN:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/Pin"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_PIN));
			break;
		}
	case ID_UNPIN:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/Unpin"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_UNPIN));
			break;

		}
	case ID_TOOL_FILTERPIN:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SelectPinnedVerts"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_FILTERPIN));
			break;
		}
	case ID_TOOL_AUTOPIN:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/AutoPinMovedVertices"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_AUTOPIN));
			break;
		}

	case ID_PACK_TIGHT:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/PackTogether"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_PACK_TIGHT));
			break;			
		}
	case ID_PACK_FULL:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/PackNormalize"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_PACK_FULL));
			break;				
		}

	case ID_PACKBUTTONS:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/PackCustom"));
			flyOffIcons.append(_M("EditUVW/PackToolDialogue"));
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/PackCustom"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				flyOffIcons,0);
			break;	
		}
	case ID_RESCALECLUSTER:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/RescaleElements"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_PACK_RESCALECLUSTER));
			break;	
		}
	case ID_GROUP:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/GroupSelected"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_GROUP));
			break;	
		}
	case ID_UNGROUP:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/Ungroup"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_UNGROUP));
			break;	
		}
	case ID_GROUPSELECT:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/SelectGroup"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_GROUPSELECT));
			break;	
		}
	case ID_GROUPSETDENSITY_EDIT:
		{
			ICustToolbar *itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(30, bRescale), editorHeight, ID_GROUPSETDENSITY_EDIT,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize = GetDPIRevisedValue(30, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_GROUPSETDENSITY_SPINNER,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;
			ISpinnerControl* uSpin = GetISpinner(itoolBar->GetItemHwnd(ID_GROUPSETDENSITY_SPINNER));
			uSpin->LinkToEdit(itoolBar->GetItemHwnd(ID_GROUPSETDENSITY_EDIT),EDITTYPE_FLOAT);
			uSpin->SetLimits(0.0001f, 1.0f, FALSE);
			uSpin->SetAutoScale();
			ReleaseISpinner(uSpin);

			break;	
		}
//OPTIONS
	case ID_UPDATEMAP:
		{
			ICustToolbar *itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolButtonItem(CTB_PUSHBUTTON,0+68, 0+68, 0+68, 0+68, 16, 15, 70, 22, id));
			ICustButton *but  = itoolBar->GetICustButton(id);
			but->SetTooltip(TRUE,GetString(IDS_TOOLTIP_TOOL_UPDATEMAP));
			but->SetImage(NULL,0,0,0,0,0,0);
			but->SetText(GetString(IDS_RB_UPDATE));
			ReleaseICustButton(but);
			break;
		}

	case ID_UVW:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/DrawTextureVerticesInUV"));
			flyOffIcons.append(_M("EditUVW/DrawTextureVerticesInVW"));
			flyOffIcons.append(_M("EditUVW/DrawTextureVerticesInUW"));
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/DrawTextureVerticesInUV"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				flyOffIcons, 0);
			break;

		}

	case ID_SHOWMAP:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/ShowActiveMapInDialog"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_SHOWMAP));
			break;
		}
	case ID_SHOWMULTITILE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/ShowMultiTileInDialog"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_SHOWMULTITILE));
			break;
		}
	case ID_PROPERTIES:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/OptionsDialog"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_PROPERTIES));
			break;
		}
	case ID_TEXTURE_COMBO:
		{
			ICustToolbar *itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(_T("combobox"), GetDPIRevisedValue(150, bRescale), comboHeight, id,CBS, -1, NULL, 0));
			ctrlSize = GetDPIRevisedValue(150, bRescale);
			hTextures = itoolBar->GetItemHwnd(ID_TEXTURE_COMBO);

			HFONT hFont;
			hFont = CreateFont(GetUIFontHeight(),0,0,0,FW_LIGHT,0,0,0,GetUIFontCharset(),0,0,0, VARIABLE_PITCH | FF_SWISS, _T(""));
			SendMessage(hTextures, WM_SETFONT, (WPARAM)hFont, MAKELONG(0, 0));
			break;
		}



//TYPEIN
	case ID_ABSOLUTETYPEIN:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/AbsoluteTypeIn"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				NULL);
			break;
		}
	case ID_ABSOLUTETYPEIN_SPINNERS:
	case ID_STATICU:					//this ID comes when we load since this is item is 9 elements
		{
			//TODO: we need need consider HDPI here!
			ICustToolbar *itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(_M("static"), GetDPIRevisedValue(18, bRescale), staticHeight, ID_STATICU,WS_CHILD|WS_VISIBLE, -1, _M(" U:"), 0));
			ctrlSize = GetDPIRevisedValue(18, bRescale);
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(34, bRescale), editorHeight, ID_EDITU,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += GetDPIRevisedValue(30, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_SPINNERU,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;

			itoolBar->AddTool(ToolOtherItem(_M("static"), GetDPIRevisedValue(18, bRescale), staticHeight, ID_STATICV,WS_CHILD|WS_VISIBLE, -1, _M("V:"), 0));
			ctrlSize += GetDPIRevisedValue(18, bRescale);
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(34, bRescale), editorHeight, ID_EDITV,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += GetDPIRevisedValue(30, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_SPINNERV,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;


			itoolBar->AddTool(ToolOtherItem(_M("static"), GetDPIRevisedValue(18, bRescale), staticHeight, ID_STATICW,WS_CHILD|WS_VISIBLE, -1, _M("W:"), 0));
			ctrlSize += GetDPIRevisedValue(18, bRescale);
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(34, bRescale), editorHeight, ID_EDITW,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += GetDPIRevisedValue(30, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_SPINNERW,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;

			itoolBar->AddTool(ToolOtherItem(_M("static"), GetDPIRevisedValue(15, bRescale), staticHeight, ID_TYPEIN_STATICL,WS_CHILD|WS_VISIBLE, -1, _M("L:"), 0));
			ctrlSize += GetDPIRevisedValue(15, bRescale);
			itoolBar->AddTool(ToolOtherItem(_T("BUTTON"), GetDPIRevisedValue(18, bRescale),	staticHeight, ID_CHECK_TYPEIN_LINKUV,WS_CHILD|WS_VISIBLE|BS_CHECKBOX, -1, GetString(IDS_EMPTY), 0));
			ctrlSize += GetDPIRevisedValue(18, bRescale);
			HWND hCheckBoxHWnd = itoolBar->GetItemHwnd(ID_CHECK_TYPEIN_LINKUV);
			if(mTypeInLinkUV)
			{
				SendMessage(hCheckBoxHWnd,BM_SETCHECK,BST_CHECKED,0);
			}
			else
			{
				SendMessage(hCheckBoxHWnd,BM_SETCHECK,BST_UNCHECKED,0);
			}
			isPixelUnitSpinner = true; // Special handling for Pixel Unit spinner

			break;
		}
//VIEW


	case ID_LOCKSELECTED:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("Common/Lock"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_LOCKEDSELECTED));
			break;
		}

	case ID_HIDE:  
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/HideSubObjects"));
			flyOffIcons.append(_M("EditUVW/UnhideAll"));

			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/HideSubObjects"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				flyOffIcons,hide);
			break;
		}

	case ID_FREEZE:  
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/FreezeSubObjects"));
			flyOffIcons.append(_M("EditUVW/UnfreezeAll"));

			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/FreezeSubObjects"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				flyOffIcons,freeze);
			break;
		}

	case ID_FILTERSELECTED:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/DisplayOnlySelectedPolygons"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_FILTERSELECTEDFACES));
			break;
		}
	case ID_FILTER_MATID:
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(_T("combobox"), GetDPIRevisedValue(140, bRescale) , comboHeight, ID_FILTER_MATID, CBS, -1, NULL, 0));

			hMatIDs = itoolBar->GetItemHwnd(ID_FILTER_MATID);
			HFONT hFont;			// Add for Japanese version
			hFont = CreateFont(GetUIFontHeight(),0,0,0,FW_LIGHT,0,0,0,GetUIFontCharset(),0,0,0, VARIABLE_PITCH | FF_SWISS, _T(""));
			SendMessage(hMatIDs, WM_SETFONT, (WPARAM)hFont, MAKELONG(0, 0));
			SendMessage(hMatIDs, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_PW_ID_ALLID));	
			SendMessage(hMatIDs,CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );			
			break;
		}
	case ID_PAN:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/PanView"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_PAN));
			break;
		}

	case ID_ZOOMTOOL:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("Common/Zoom"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_ZOOMTOOL));
			break;
		}

	case ID_ZOOMREGION: 
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/ZoomToRegion"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_ZOOMREGION));
			break;
		}

	case ID_ZOOMEXTENT:    
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/ZoomExtentsToAllTextures"));
			flyOffIcons.append(_M("EditUVW/ZoomExtentsCurrentSelection"));
			flyOffIcons.append(_M("EditUVW/ZoomExtentsToSelectedObjects"));

			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/ZoomExtentsToAllTextures"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				flyOffIcons,zoomext);
			break;
		}

	case ID_SNAP:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/SnapToggle"));
			flyOffIcons.append(_M("EditUVW/SnapSettings"));
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SnapToggle"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				flyOffIcons,0);
			break;

		}
//EDIT UVS
	case ID_EDIT:
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolButtonItem(CTB_PUSHBUTTON,0, 0, 0, 0, 16, 15, 140, 22, id));
			ctrlSize = GetDPIRevisedValue(140, bRescale);
			ICustButton *but  = itoolBar->GetICustButton(id);
			but->SetTooltip(TRUE,GetString(IDS_EDIT));
			but->SetImage(NULL,0,0,0,0,0,0);
			but->SetText(GetString(IDS_EDIT));
			break;
		}	

	case ID_TWEAKUVW:
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolButtonItem(CTB_CHECKBUTTON,0, 0, 0, 0, 16, 15, 140, 22, id));
			ctrlSize = GetDPIRevisedValue(140, bRescale);;
			ICustButton *but  = itoolBar->GetICustButton(id);
			but->SetTooltip(TRUE,GetString(IDS_TWEAKUVW));
			but->SetImage(NULL,0,0,0,0,0,0);
			but->SetText(GetString(IDS_TWEAKUVW));

			break;
		}	
	case ID_QMAP:
		{		
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/QuickPlanarMap"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_QMAP));
			break;
		}
	case ID_QUICKMAP_DISPLAY:
		{			
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/DisplayQuickPlanarMap"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_QMAP_DISPLAY));
			break;
		}
	case ID_QUICKMAP_ALIGN:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/AlignToX"));
			flyOffIcons.append(_M("EditUVW/AlignToY"));
			flyOffIcons.append(_M("EditUVW/AlignToZ"));
			flyOffIcons.append(_M("EditUVW/AlignToPlanarMap"));

			int align =  0;
			pblock->GetValue(unwrap_qmap_align,0,align,FOREVER);

			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/AlignToPlanarMap"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				flyOffIcons,align);
			break;
		}

	case ID_PELT_EDITSEAMS:
		{	
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/EditSeams"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_EDIT_SEAMS));
			break;
		}
	case ID_PELT_POINTTOPOINTSEAMS:
		{			
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/PointToPointSeams"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_POINTTOPOINTSEAMS));
			break;
		}
	case ID_PW_SELTOSEAM2:
		{	
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/ConvertEdgeToSeams"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_SELTOSEAMS));
			break;
		}

	case ID_PELT_EXPANDSELTOSEAM:
		{		
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/ExpandPolygonSelectionToSeams"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_EXPANDSELTOSEAM));
			break;
		}
	case ID_PLANAR_MAP: // TODO: it uses quick planar map icon, seems not consistent with other quick map mode!
		{			
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/PlanarMap"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_PLANAR_MAP));
			break;
		}			
	case ID_CYLINDRICAL_MAP:
		{		
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/CylindricalMap"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_CYLINDRICAL_MAP));
			break;
		}			
	case ID_SPHERICAL_MAP:
		{		
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SphericalMap"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_SPHERICAL_MAP));
			break;
		}			
	case ID_BOX_MAP:
		{
				toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/BoxMap"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_BOX_MAP));
			break;
		}	
	case ID_PELT_MAP:
		{		
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/PeltMap"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_PELT_MAP));
			break;
		}	
	case ID_SPLINE_MAP:
		{		
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SplineMapping"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_SPLINE_MAP));
			break;
		}
	/*case ID_UNFOLD_MAP:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/UnfoldStripFromLoop"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_UNFOLD_MAP));
			break;
		}*/
	case ID_UNFOLD_EDGE:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/UnfoldStripFromLoop"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_UNFOLD_EDGE));
			break;
		}
	case ID_MAPPING_ALIGNX:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/AlignToX"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_ALIGNX));
			break;
		}			

	case ID_MAPPING_ALIGNY:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/AlignToY"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_ALIGNY));
			break;
		}			
	case ID_MAPPING_ALIGNZ:
		{		
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/AlignToZ"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_ALIGNZ));
			break;
		}			
	case ID_MAPPING_NORMALALIGN:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/BestAlign"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_NORMALIGN));
			break;

		}			
	case ID_MAPPING_ALIGNTOVIEW:
		{
			toolBar->AddButton(id, ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/ViewAlign"), buttonImageWidth, buttonImageHeight, buttonWidth, buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_ALIGNVIEW));
			break;
		}	
	case ID_MAPPING_FIT:
		{
			ICustToolbar *itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolButtonItem(CTB_PUSHBUTTON,0+68, 0+68, 0+68, 0+68, 16, 15, 28, 22, id));
			ctrlSize = GetDPIRevisedValue(28, bRescale);
			ICustButton *but  = itoolBar->GetICustButton(id);
			but->SetTooltip(TRUE,GetString(IDS_TOOLTIP_TOOL_MAPPING_FIT));
			but->SetImage(NULL,0,0,0,0,0,0);
			but->SetText(GetString(IDS_FIT));
			ReleaseICustButton(but);
			break;
		}
	case ID_MAPPING_CENTER:
		{
			ICustToolbar *itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolButtonItem(CTB_PUSHBUTTON,0+68, 0+68, 0+68, 0+68, 16, 15, 40, 22, id));
			ctrlSize = GetDPIRevisedValue(40, bRescale);
			ICustButton *but  = itoolBar->GetICustButton(id);
			but->SetTooltip(TRUE,GetString(IDS_TOOLTIP_TOOL_MAPPING_CENTER));
			but->SetImage(NULL,0,0,0,0,0,0);
			but->SetText(GetString(IDS_CENTER));
			ReleaseICustButton(but);
			break;
		}
	case ID_MAPPING_RESET:
		{				
			toolBar->AddButton(id,ToolButtonItem(CTB_PUSHBUTTON, _M("EditUVW/ResetMappingGizmo"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight, id),
				GetString(IDS_TOOLTIP_TOOL_MAPPING_RESET));
			break;
		}	

	case ID_PACK_RESCALE:
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(_T("BUTTON"), GetDPIRevisedValue(62, bRescale),	staticHeight, ID_PACK_RESCALE,WS_CHILD|WS_VISIBLE|BS_CHECKBOX|BS_VCENTER, -1, GetString(IDS_EMPTY), 0));
			ctrlSize += GetDPIRevisedValue(62, bRescale);
			HWND hwnd = itoolBar->GetItemHwnd(ID_PACK_RESCALE);
			if (mPackTempRescale)
				SendMessage(hwnd,BM_SETCHECK,BST_CHECKED,0);
			else
				SendMessage(hwnd,BM_SETCHECK,BST_UNCHECKED,0);
			break;
		}
	case ID_PACK_ROTATE:
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(_T("BUTTON"), GetDPIRevisedValue(60, bRescale),	staticHeight, ID_PACK_ROTATE,WS_CHILD|WS_VISIBLE|BS_CHECKBOX|BS_VCENTER, -1, GetString(IDS_EMPTY), 0));
			ctrlSize = GetDPIRevisedValue(60, bRescale);
			HWND hwnd = itoolBar->GetItemHwnd(ID_PACK_ROTATE);
			if (mPackTempRotate)
				SendMessage(hwnd,BM_SETCHECK,BST_CHECKED,0);
			else
				SendMessage(hwnd,BM_SETCHECK,BST_UNCHECKED,0);
			break;
		}
	case ID_PACK_PADDINGEDIT:
		//this ID comes when we load since this is item is 9 elements
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(40, bRescale), editorHeight, ID_PACK_PADDINGEDIT,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize = GetDPIRevisedValue(40, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_PACK_PADDINGSPINNER,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;
			isPixelUnitSpinner = true; // Special handling for Pixel Unit spinner

			break;
		}
	case ID_BRUSH_STRENGTHEDIT:
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(38, bRescale), editorHeight, ID_BRUSH_STRENGTHEDIT,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize = GetDPIRevisedValue(38, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_BRUSH_STRENGTHSPINNER,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;
			ISpinnerControl* uSpin = GetISpinner(itoolBar->GetItemHwnd(ID_BRUSH_STRENGTHSPINNER));
			uSpin->LinkToEdit(itoolBar->GetItemHwnd(ID_BRUSH_STRENGTHEDIT),EDITTYPE_FLOAT);
			uSpin->SetLimits(0.0f, 10000.0f, FALSE);
			uSpin->SetValue(sPaintFullStrengthSize,FALSE);
			uSpin->SetAutoScale();
			
			ReleaseISpinner(uSpin);
		}
		break;
	case ID_BRUSH_FALLOFFEDIT:
		{
			ICustToolbar* itoolBar = toolBar->GetToolBar();
			itoolBar->AddTool(ToolOtherItem(CUSTEDITWINDOWCLASS, GetDPIRevisedValue(38, bRescale), editorHeight, ID_BRUSH_FALLOFFEDIT,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize = GetDPIRevisedValue(38, bRescale);
			itoolBar->AddTool(ToolOtherItem(SPINNERWINDOWCLASS, spinnerWidth, spinnerHeight, ID_BRUSH_FALLOFFSPINNER,WS_CHILD|WS_VISIBLE, -1, NULL, 0));
			ctrlSize += spinnerWidth;
			ISpinnerControl* uSpin = GetISpinner(itoolBar->GetItemHwnd(ID_BRUSH_FALLOFFSPINNER));
			uSpin->LinkToEdit(itoolBar->GetItemHwnd(ID_BRUSH_FALLOFFEDIT),EDITTYPE_FLOAT);
			uSpin->SetLimits(0.0f, 10000.0f, FALSE);
			uSpin->SetValue(sPaintFallOffSize,FALSE);
			uSpin->SetAutoScale();
			
			ReleaseISpinner(uSpin);
		}
		break;
	case ID_PAINT_MOVE_BRUSH:
		{
			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/PaintMovement"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				GetString(IDS_PAINT_MOVE_BRUSH));
		}
		break;
	case ID_RELAX_MOVE_BRUSH:
		{
			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/RelaxByPolygonAngles"));
			flyOffIcons.append(_M("EditUVW/RelaxByEdgeAngles"));
			flyOffIcons.append(_M("EditUVW/RelaxByCenters"));

			toolBar->AddButton(id, ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/RelaxByEdgeAngles"), buttonImageWidth, buttonImageHeight, buttonWidth, buttonHeight, id),
				flyOffIcons, 0);
		}
		break;
	case ID_BRUSH_FALLOFF_TYPE:
		{

			FlyOffIconList flyOffIcons;
			flyOffIcons.append(_M("EditUVW/SoftSelectionLinearFallOff"));
			flyOffIcons.append(_M("EditUVW/SoftSelectionSmoothFallOff"));
			flyOffIcons.append(_M("EditUVW/SoftSelectionSlowOutFallOff"));
			flyOffIcons.append(_M("EditUVW/SoftSelectionFastOutFallOff"));

			toolBar->AddButton(id,ToolButtonItem(CTB_CHECKBUTTON, _M("EditUVW/SoftSelectionLinearFallOff"), buttonImageWidth,buttonImageHeight,buttonWidth,buttonHeight,id),
				flyOffIcons, 0);
		}
		break;
	default:   //no custom implementation so just do a simple text button
		{
			int size = AddDefaultActionToBar(toolBar->GetToolBar(), id);
			ctrlSize = GetDPIRevisedValue(size, bRescale);
			break;
		}
	}

	if( isPixelUnitSpinner ) // Special handling for Pixel Unit spinners
		SetupPixelUnitSpinner( id );

	return ctrlSize;
}

void UnwrapMod::SetupPixelUnitSpinner(int id)
{
	switch(id)
	{
	case ID_WELDTHRESH_SPINNER:
	case ID_WELDTHRESH_EDIT:
		{
			int axis=0;
			bool spinnerPixelUnits = IsSpinnerPixelUnits(ID_WELDTHRESH_SPINNER,&axis);
			ISpinnerControl* weldThreshSpin = GetUIManager()->GetSpinnerControl(ID_WELDTHRESH_SPINNER);
			ICustEdit* weldThreshEdit = GetUIManager()->GetEditControl(ID_WELDTHRESH_EDIT);
			if( (weldThreshSpin==NULL) || (weldThreshEdit==NULL) ) break;
			weldThreshSpin->LinkToEdit(weldThreshEdit->GetHwnd(), (spinnerPixelUnits?  EDITTYPE_INT : EDITTYPE_FLOAT));
			weldThreshSpin->SetLimits(0.0f,  (spinnerPixelUnits?  (10.0f*GetScalePixelUnits(axis)) : 10.0f), FALSE);
			weldThreshSpin->SetScale(0.1f);
			GetUIManager()->SetSpinFValue( ID_WELDTHRESH_SPINNER, fnGetWeldThresold() );
		}
		break;
	case ID_PACK_PADDINGSPINNER:
	case ID_PACK_PADDINGEDIT:
		{
			int axis=0;
			bool spinnerPixelUnits = IsSpinnerPixelUnits(ID_PACK_PADDINGSPINNER,&axis);
			ISpinnerControl* paddingSpin = GetUIManager()->GetSpinnerControl(ID_PACK_PADDINGSPINNER);
			ICustEdit* paddingEdit = GetUIManager()->GetEditControl(ID_PACK_PADDINGEDIT);
			if( (paddingSpin==NULL) || (paddingEdit==NULL) ) break;
			paddingSpin->LinkToEdit(paddingEdit->GetHwnd(), (spinnerPixelUnits?  EDITTYPE_INT : EDITTYPE_FLOAT) );
			paddingSpin->SetLimits(0.0f, (spinnerPixelUnits?  GetScalePixelUnits(axis) : 1.0f), FALSE);
			if( spinnerPixelUnits )
				paddingSpin->SetScale(1.0f);
			else paddingSpin->SetAutoScale();
			GetUIManager()->SetSpinFValue( ID_PACK_PADDINGSPINNER, mPackTempPadding );
		}
		break;
	case ID_ABSOLUTETYPEIN_SPINNERS:
	case ID_STATICU:
		{
			ISpinnerControl* uSpin = GetUIManager()->GetSpinnerControl(ID_SPINNERU);
			ICustEdit* uEdit = GetUIManager()->GetEditControl(ID_EDITU);
			if( (uSpin==NULL) || (uEdit==NULL) ) break;
			bool spinnerPixelUnitsU = IsSpinnerPixelUnits(ID_SPINNERU);
			uSpin->LinkToEdit(uEdit->GetHwnd(),(spinnerPixelUnitsU?  EDITTYPE_INT : EDITTYPE_FLOAT));
			uSpin->SetLimits(-9999999, 9999999, FALSE);
			uSpin->SetAutoScale( spinnerPixelUnitsU? FALSE:TRUE );
			if( spinnerPixelUnitsU )
				uSpin->SetScale(1.0f);

			ISpinnerControl* vSpin = GetUIManager()->GetSpinnerControl(ID_SPINNERV);
			ICustEdit* vEdit = GetUIManager()->GetEditControl(ID_EDITV);
			if( (vSpin==NULL) || (vEdit==NULL) ) break;
			bool spinnerPixelUnitsV = IsSpinnerPixelUnits(ID_SPINNERV);
			vSpin->LinkToEdit(vEdit->GetHwnd(),(spinnerPixelUnitsV?  EDITTYPE_INT : EDITTYPE_FLOAT));
			vSpin->SetLimits(-9999999, 9999999, FALSE);
			vSpin->SetAutoScale( spinnerPixelUnitsV? FALSE:TRUE );
			if( spinnerPixelUnitsV )
				vSpin->SetScale(1.0f);

			ISpinnerControl* wSpin = GetUIManager()->GetSpinnerControl(ID_SPINNERW);
			ICustEdit* wEdit = GetUIManager()->GetEditControl(ID_EDITW);
			if( (wSpin==NULL) || (wEdit==NULL) ) break;
			bool spinnerPixelUnitsW = IsSpinnerPixelUnits(ID_SPINNERW);
			wSpin->LinkToEdit(wEdit->GetHwnd(),(spinnerPixelUnitsW?  EDITTYPE_INT : EDITTYPE_FLOAT));
			wSpin->SetLimits(-9999999, 9999999, FALSE);
			wSpin->SetAutoScale( spinnerPixelUnitsW? FALSE:TRUE );
			if( spinnerPixelUnitsW )
				wSpin->SetScale(1.0f);
		}
		break;
	}
}

void UnwrapMod::SetupPixelUnitSpinners()
{
	SetupPixelUnitSpinner( ID_WELDTHRESH_SPINNER );
	SetupPixelUnitSpinner( ID_PACK_PADDINGSPINNER);
	SetupPixelUnitSpinner( ID_ABSOLUTETYPEIN_SPINNERS);
}
