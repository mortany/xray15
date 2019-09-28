/*

Copyright 2010 Autodesk, Inc.  All rights reserved. 

Use of this software is subject to the terms of the Autodesk license agreement provided at 
the time of installation or download, or which otherwise accompanies this software in either 
electronic or hard copy form. 

*/

//**************************************************************************/
// DESCRIPTION: Unwrap UI classes
// AUTHOR: Peter Watje
// DATE: 2010/08/31 
//***************************************************************************/

#include "Unwrap.h"
#include "modsres.h"
#include "graphics/IDisplayManager.h"

BOOL UnwrapMod::WtIsEnabled(int id)
{
	BOOL iret = TRUE;


	if (fnGetTVSubMode() == TVOBJECTMODE)
	{
		switch (id)
		{
			case ID_TV_VERTMODE :
			case ID_TV_EDGEMODE :
			case ID_TV_FACEMODE :
			case ID_EDIT :
			case ID_RESET :
			case ID_PAN:
			case ID_ZOOMTOOL:
			case ID_ZOOMREGION:
			case ID_UPDATEMAP:
			case ID_UVW:
			case ID_SHOWMAP:
			case ID_SHOWMULTITILE:
			case ID_PROPERTIES:
			case ID_TEXTURE_COMBO:
			case ID_ZOOMEXTENT:
			case ID_SNAP:
			case ID_PREVIEW_SELECTION:
				iret = TRUE;
				break;
			default:
				iret = FALSE;
				break;
		}
		return iret;
	}
	switch (id)
	{
	case ID_PAINT_MOVE_BRUSH:
	case ID_RELAX_MOVE_BRUSH:
	case ID_SAVE_AS_DEFAULT:
	case ID_MIRROR_GEOM_SELECTION:
 		break;
 	case ID_MIRROR_SEL_X_AXIS:
 	case ID_MIRROR_SEL_Y_AXIS:
	case ID_MIRROR_SEL_Z_AXIS:
		iret = fnGetMirrorSelectionStatus();
		break;
 		
	case ID_PELT_EDITSEAMS:
	case ID_PELT_POINTTOPOINTSEAMS:
		if  (ip && (ip->GetSubObjectLevel() == 0) )
			iret = FALSE;
		else iret = TRUE;
		break;
	case ID_UNFOLD_EDGE:
	case ID_POINT_TO_POINT_SEL:
		if  (ip && (ip->GetSubObjectLevel() == 2) )
			iret = TRUE;
		else iret = FALSE;
		break;

	case ID_STRAIGHTEN:
	case ID_ZOOMTOGIZMO:
	case ID_QMAP:
	case ID_FLATTEN_BYMATID:
	case ID_FLATTEN_BYSMOOTHINGGROUP:
	case ID_FLATTENBUTTONS:
	case ID_GROUPSETDENSITY_EDIT:
	case ID_GROUPSETDENSITY_SPINNER:
	case ID_GROUP:
	case ID_UNGROUP:
	case ID_GROUPSELECT:

		{
			if  (ip && (ip->GetSubObjectLevel() == 3) )
				iret = TRUE;
			else iret = FALSE;
			break;
		}
	case ID_TOOL_SPACE_VERTICAL:
	case ID_TOOL_SPACE_HORIZONTAL:
		{
			if  (ip && (ip->GetSubObjectLevel() == 3) )
				iret = FALSE;
			else iret = TRUE;
			break;
		}

	case ID_DISPLAYHIDDENEDGES:
	case ID_SHOWHIDDENEDGES:
		{
			if (fnIsMesh())
				iret = TRUE;
			else iret = FALSE;
			break;
		}
	case ID_SAVE:
	case ID_LOAD:
	case ID_RESET:
		{
			if (fnGetMapMode() == PELTMAP)
				iret = FALSE;
			else iret = TRUE;
			break;
		}
		/*
		case ID_PELT_MAP:
		if (fnGetTVSubMode() != TVFACEMODE)
		iret = FALSE;
		break;
		*/
	case ID_EDGERINGSELECTION:
	case ID_EDGELOOPSELECTION:
		if  (ip && (ip->GetSubObjectLevel() == 2) )
			iret = TRUE;
		else iret = FALSE;

		break;

	case ID_MAPPING_ALIGNX:
	case ID_MAPPING_ALIGNY:
	case ID_MAPPING_ALIGNZ:
	case ID_MAPPING_NORMALALIGN:
	case ID_MAPPING_FIT:
	case ID_MAPPING_CENTER:
	case ID_MAPPING_ALIGNTOVIEW:
	case ID_MAPPING_RESET:
		if (fnGetMapMode() == NOMAP)
			iret = FALSE;
		break;
	case ID_PELTDIALOG:
		if (peltData.GetPeltMapMode())
			iret = TRUE;
		else iret = FALSE;
		break;
	case ID_PW_SELTOSEAM:	
	case ID_PW_SELTOSEAM2:	
	case ID_PW_SEAMTOSEL:	
	case ID_PW_SEAMTOSEL2:		
	case ID_TOOL_ALIGN_ELEMENT:

		if (fnGetTVSubMode() == TVEDGEMODE)
			return TRUE;
		else
			iret = FALSE;
		break;
	case ID_PELT_EXPANDSELTOSEAM:	
		if (fnGetTVSubMode() == TVFACEMODE)
			return TRUE;
		else iret = FALSE;
		break;

	case ID_PELTDIALOG_RESETRIG:	
	case ID_PELTDIALOG_SELECTRIG:	
	case ID_PELTDIALOG_SELECTPELT:	
	case ID_PELTDIALOG_SNAPRIG:	
	case ID_PELTDIALOG_STRAIGHTENSEAMS:
	case ID_PELTDIALOG_MIRRORRIG:
	case ID_PELTDIALOG_RUN:
	case ID_PELTDIALOG_RELAX1:
	case ID_PELTDIALOG_RELAX2:
		if (peltData.GetPeltMapMode() && peltData.peltDialog.hWnd)
			iret = TRUE;
		else iret = FALSE;
		break;


	case ID_SELECT_OVERLAP:
		{
			if (fnGetTVSubMode() == TVFACEMODE)
				iret = TRUE;
			else iret = FALSE;			
			break;
		}
		break;

	case ID_PASTE:
		iret = copyPasteBuffer.CanPaste();
		break;
	case ID_PASTEINSTANCE:
		iret = copyPasteBuffer.CanPasteInstance(this);
		break;
	case ID_UVEDGEMODE:
		{
			if (fnGetTVSubMode() == TVEDGEMODE)
				iret = TRUE;
			else iret = FALSE;			
			break;
		}
	case ID_OPENEDGEMODE:
		{
			if (fnGetTVSubMode() == TVEDGEMODE)
				iret = TRUE;
			else iret = FALSE;			
			break;
		}
	case ID_UVEDGESELECT:
		{
			if (fnGetTVSubMode() == TVEDGEMODE)
				iret = TRUE;
			else iret = FALSE;			
			break;
		}
	case ID_OPENEDGESELECT:
		{
			if (fnGetTVSubMode() == TVEDGEMODE)
				iret = TRUE;
			else iret = FALSE;			
			break;
		}

	case ID_SNAPCENTER:
	case ID_SNAPLOWERLEFT:
	case ID_SNAPLOWERCENTER:
	case ID_SNAPLOWERRIGHT:
	case ID_SNAPUPPERLEFT:
	case ID_SNAPUPPERCENTER:
	case ID_SNAPUPPERRIGHT:
	case ID_SNAPRIGHTCENTER:
	case ID_SNAPLEFTCENTER:
	case ID_FREEFORMSNAP:
		{
			iret = TRUE;
			break;
		}
	case ID_SKETCH:
	case ID_SKETCHDIALOG:
		{
			if (fnGetTVSubMode() == TVVERTMODE) 
				iret = TRUE;
			else iret = FALSE;
			break;
		}
	case ID_SKETCHREVERSE:
		{
			if ((sketchSelMode == SKETCH_SELCURRENT) && (mode == ID_SKETCHMODE))
				iret = TRUE;
			else iret = FALSE;
			break;
		}
	case ID_POLYGONMODE:
	case ID_POLYGONSELECT:

		{
			if (fnGetTVSubMode() == TVFACEMODE)
				iret = TRUE;
			else iret = FALSE;			
			break;
		}
	case ID_TV_PAINTSELECTINC:
		{
			if (fnGetPaintMode() )
				iret = TRUE;
			else iret = FALSE;
			break;
		}
	case ID_TV_PAINTSELECTDEC:
		{
			if (fnGetPaintMode() && (fnGetPaintSize() > 1))
				iret = TRUE;
			else iret = FALSE;
			break;
		}
	case ID_FLATTENMAP:
	case ID_FLATTENMAPDIALOG:
		{
			if (fnGetPreventFlattening())
				iret = FALSE;
			else
			{
				if (fnGetTVSubMode() == TVFACEMODE) 
					iret = TRUE; 
				else iret = FALSE;
			}
			break;
		}


	case ID_NORMALMAP:
	case ID_NORMALMAPDIALOG:
	case ID_UNFOLDMAP:
	case ID_UNFOLDMAPDIALOG:
		if (fnGetTVSubMode() == TVFACEMODE) 
			iret = TRUE; 
		else iret = FALSE;
		break;

	case ID_WELD:
		if ( (fnGetTVSubMode() == TVFACEMODE) ||
			 (fnGetTVSubMode() == TVOBJECTMODE) )
			iret = FALSE; 
		else iret = TRUE; 
		break;

	case ID_SELECTINVERTEDFACES:
	case ID_PACK:
	case ID_PLANAR_MAP:
	case ID_CYLINDRICAL_MAP:
	case ID_SPHERICAL_MAP:
	case ID_BOX_MAP:
	case ID_SPLINE_MAP:
	case ID_UNFOLD_MAP:
	case ID_SETMATIDSPIN:
	case ID_SETMATIDEDIT:

		{
			if (fnGetTVSubMode() == TVFACEMODE)
				iret = TRUE;
			else iret = FALSE;			
			break;
		}
	case ID_PLANARMODE:
	case ID_SELECTBY_MATID:
	case ID_SELECTBY_SMGRP:
	case ID_SMGRPSPIN:
	case ID_SMGRPEDIT:
	case ID_MATIDSPIN:
	case ID_MATIDEDIT:
		{
			if (fnGetTVSubMode() == TVFACEMODE 
				&& !fnGetMirrorSelectionStatus())
				iret = TRUE;
			else iret = FALSE;	
			break;
		}
	case ID_SETMATID:
		{
			iret = FALSE;
			break;
		}

	
	case ID_LSCM_INTERACTIVE:
	case ID_LSCM_SOLVE:
	case ID_PEEL_DETACH:
	case ID_AUTO_PACK:
	case ID_LSCM_RESET:
		{
			iret = TRUE;
			break;
		}
	case ID_PIN:
	case ID_UNPIN:
		{
			iret = (fnGetTVSubMode() == TVVERTMODE) ? TRUE : FALSE ;
		}
		break;
	case ID_TOOL_AUTOPIN:
		{
			BOOL isChecked = FALSE;
			pblock->GetValue(unwrap_autopin, 0, isChecked, FOREVER);
			if(isChecked)  // auto pin is checked
			{
				iret = TRUE;
			}
			else
			{
				iret = (fnGetTVSubMode() == TVVERTMODE) ? TRUE : FALSE ;
			}
		}
		break;
	case ID_TOOL_FILTERPIN:
		{
			BOOL isChecked = FALSE;
			pblock->GetValue(unwrap_filterpin, 0, isChecked, FOREVER);
			if(isChecked)  // select pin is checked
			{
				iret = TRUE;
			}
			else
			{
				iret = (fnGetTVSubMode() == TVVERTMODE) ? TRUE : FALSE ;
			}
		}
		break;
	case ID_VERTTOEDGESELECT:
	case ID_VERTTOFACESELECT:
		{
			iret = (fnGetTVSubMode() == TVVERTMODE) ? TRUE : FALSE ;
		}
		break;
	case ID_EDGETOVERTSELECT:
	case ID_EDGETOFACESELECT:
		{
			iret = (fnGetTVSubMode() == TVEDGEMODE) ? TRUE : FALSE ;
		}
		break;
	case ID_FACETOVERTSELECT:
	case ID_FACETOEDGESELECT:
		{
			iret = (fnGetTVSubMode() == TVFACEMODE) ? TRUE : FALSE ;
		}
		break;
	case ID_CHECK_TYPEIN_LINKUV:
		{
			iret = (!absoluteTypeIn && (mode == ID_SCALE || (mode == ID_FREEFORMMODE && freeFormSubMode == ID_SCALE))) ? TRUE : FALSE ;
		}
		break;
	case ID_EDITU:
		{
			iret = ((mode == ID_MOVE || (mode == ID_FREEFORMMODE && (freeFormSubMode == ID_MOVE || freeFormSubMode == ID_SELECT))) ||
				   (!absoluteTypeIn && (mode == ID_SCALE || (mode == ID_FREEFORMMODE && freeFormSubMode == ID_SCALE)))  ||
				   (!absoluteTypeIn && (mode == ID_ROTATE || (mode == ID_FREEFORMMODE && freeFormSubMode == ID_ROTATE)))
				) ? TRUE : FALSE ;
		}
		break;
	case ID_EDITV:
		{
			iret = ((mode == ID_MOVE || (mode == ID_FREEFORMMODE && (freeFormSubMode == ID_MOVE || freeFormSubMode == ID_SELECT))) ||
				(!absoluteTypeIn && (mode == ID_SCALE || (mode == ID_FREEFORMMODE && freeFormSubMode == ID_SCALE)))
				) ? TRUE : FALSE ;
		}
		break;
	case ID_EDITW:
		{
			iret = (mode == ID_MOVE || (mode == ID_FREEFORMMODE && (freeFormSubMode == ID_MOVE || freeFormSubMode == ID_SELECT))
				|| mode == ID_FREEFORMMODE && freeFormSubMode == ID_SELECT)  ? TRUE : FALSE ;
		}
		break;
	case ID_RENDERUV:
		{
			iret = (MaxSDK::Graphics::IsRetainedModeEnabled());
		}
		break;
	}
	return iret;
}
