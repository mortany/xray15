/**********************************************************************
 *<
	FILE:			PFOperatorMaterialStatic_ParamBlock.cpp

	DESCRIPTION:	ParamBlocks2 for MaterialStatic Operator (definition)
											 
	CREATED BY:		Oleg Bayborodin

	HISTORY:		created 06-14-2002

	*>	Copyright 2008 Autodesk, Inc.  All rights reserved.

	Use of this software is subject to the terms of the Autodesk license
	agreement provided at the time of installation or download, or which
	otherwise accompanies this software in either electronic or hard copy form.  

 **********************************************************************/

#include "PFOperatorMaterialStatic_ParamBlock.h"

#include "PFOperatorMaterialStatic.h"
#include "PFActions_GlobalVariables.h"
#include "PFActions_GlobalFunctions.h"
#include "IPFAction.h"
#include "IPFOperator.h"
#include "IPViewItem.h"

#include "resource.h"

namespace PFActions {

// Each plug-in should have one instance of the ClassDesc class
PFOperatorMaterialStaticDesc		ThePFOperatorMaterialStaticDesc;
// Dialog Proc
PFOperatorMaterialStaticDlgProc		ThePFOperatorMaterialStaticDlgProc;

// MaterialStatic Operator param block
ParamBlockDesc2 materialStatic_paramBlockDesc 
(
	// required block spec
		kMaterialStatic_mainPBlockIndex, 
		_T("Parameters"),
		IDS_PARAMETERS,
		&ThePFOperatorMaterialStaticDesc,
		P_AUTO_CONSTRUCT + P_AUTO_UI,
	// auto constuct block refno : none
		kMaterial_reference_pblock,
	// auto ui parammap specs : none
		IDD_MATERIALSTATIC_PARAMETERS, 
		IDS_PARAMETERS,
		0,
		0, // open/closed
		&ThePFOperatorMaterialStaticDlgProc,
	// required param specs
		// assign materialStatic
			kMaterialStatic_assignMaterial, _T("Assign_Material"),
									TYPE_BOOL,
									P_RESET_DEFAULT,
									IDS_ASSIGNMATERIAL,
			// optional tagged param specs
					p_default,		TRUE,
					p_ui,			TYPE_SINGLECHECKBOX,	IDC_ASSIGNMATERIAL,
			p_end,
		// materialStatic
			kMaterialStatic_material,	_T("Assigned_Material"),
									TYPE_MTL,
									P_RESET_DEFAULT|P_SHORT_LABELS|P_NO_REF,
									IDS_ASSIGNEDMATERIAL,
			// optional tagged param specs
					p_ui,			TYPE_MTLBUTTON,	IDC_MATERIAL,
			p_end,
		// assign materialStatic ID
			kMaterialStatic_assignID, _T("Assign_Material_ID"),
									TYPE_BOOL,
									P_RESET_DEFAULT,
									IDS_ASSIGNMATERIALID,
			// optional tagged param specs
					p_default,		FALSE,
					p_ui,			TYPE_SINGLECHECKBOX,	IDC_ASSIGNID,
			p_end,
		// show mtl IDs in viewport
			kMaterialStatic_showInViewport, _T("Show_In_Viewport"),
									TYPE_BOOL,
									P_RESET_DEFAULT,
									IDS_SHOWINVIEWPORT,
			// optional tagged param specs
					p_default,		FALSE,
					p_ui,			TYPE_SINGLECHECKBOX,	IDC_SHOWINVIEWPORT,
			p_end,
		// sub-material ID Offset
			kMaterialStatic_mtlIDOffset, _T("SubMtl_ID_Offset"),
									TYPE_INT,
									P_RESET_DEFAULT,
									IDS_SUBMTLIDOFFSET,
			// optional tagged param specs
					p_default,		0,
					p_range,		0, 1000,
					p_ui,			TYPE_SPINNER,	EDITTYPE_POS_INT,	IDC_MTLIDOFFSET,	IDC_MTLIDOFFSETSPIN,	0.25f,
			p_end,
		// materialStatic ID animation type
			kMaterialStatic_type,	_T("Type"),
									TYPE_INT,
									P_RESET_DEFAULT,
									IDS_TYPE,
			// optional tagged param specs
					p_default,		kMaterialStatic_type_id,
					p_range,		kMaterialStatic_type_id, kMaterialStatic_type_random,
					p_ui,			TYPE_RADIO, kMaterialStatic_type_num, IDC_TYPE_ID, IDC_TYPE_CYCLE, IDC_TYPE_RANDOM,
			p_end,
		// materialStatic ID
			kMaterialStatic_materialID, _T("Material_ID"),
									TYPE_INT,
									P_RESET_DEFAULT|P_ANIMATABLE,
									IDS_MATERIALID,
			// optional tagged param specs
					p_default,		1,
					p_range,		1, 65535,
					p_ui,			TYPE_SPINNER,	EDITTYPE_POS_INT,	IDC_MATERIALID,	IDC_MATERIALIDSPIN,	1.0f,
			p_end,
		// number of sub-materials
			kMaterialStatic_numSubMtls, _T("Number_of_Sub_Materials"),
									TYPE_INT,
									P_RESET_DEFAULT,
									IDS_NUMSUBMTLS,
			// optional tagged param specs
					p_default,		0,
					p_range,		0, 65535,
					p_ui,			TYPE_SPINNER,	EDITTYPE_POS_INT,	IDC_NUMSUBS,	IDC_NUMSUBSSPIN,	1.0f,
			p_end,
		// rate type
			kMaterialStatic_rateType,	_T("Rate_Type"),
									TYPE_INT,
									P_RESET_DEFAULT,
									IDS_RATETYPE,
			// optional tagged param specs
					p_default,		kMaterialStatic_rateType_particle,
					p_range,		0, kMaterialStatic_rateType_num-1,
					p_ui,			TYPE_RADIO, kMaterialStatic_rateType_num, IDC_PERSECOND, IDC_PERPARTICLE,
			p_end,
		// number of sub-materialStatics per second
			kMaterialStatic_ratePerSecond, _T("Rate_Per_Second"),
									TYPE_FLOAT,
									P_RESET_DEFAULT|P_ANIMATABLE,
									IDS_RATEPERSECOND,
			// optional tagged param specs
					p_default,		30.0f,
					p_range,		kMaterialStatic_minRate, kMaterialStatic_maxRate,
					p_ui, 			TYPE_SPINNER, EDITTYPE_POS_FLOAT, IDC_RATEPERSEC, IDC_RATEPERSECSPIN, SPIN_AUTOSCALE,
			p_end,
		// number of sub-materialStatics per particle
			kMaterialStatic_ratePerParticle, _T("Rate_Per_Particle"),
									TYPE_FLOAT,
									P_RESET_DEFAULT|P_ANIMATABLE,
									IDS_RATEPERPARTICLE,
			// optional tagged param specs
					p_default,		1.0f,
					p_range,		kMaterialStatic_minRate, kMaterialStatic_maxRate,
					p_ui, 			TYPE_SPINNER, EDITTYPE_POS_FLOAT, IDC_RATEPERPART, IDC_RATEPERPARTSPIN, SPIN_AUTOSCALE,
			p_end,
		// loop frames
			kMaterialStatic_loop,	_T("Loop"),
									TYPE_BOOL,
									P_RESET_DEFAULT,
									IDS_LOOP,
			// optional tagged param specs
					p_default,		TRUE,
					p_ui,			TYPE_SINGLECHECKBOX,	IDC_LOOP,
			p_end,
		// random seed
			kMaterialStatic_randomSeed, _T("Random_Seed"),
									TYPE_INT,
									P_RESET_DEFAULT|P_ANIMATABLE,
									IDS_RANDOMSEED,
			// optional tagged param specs
					p_default,		12345, // initial random seed
					p_range,		1, 999999999,
					p_ui,			TYPE_SPINNER,	EDITTYPE_POS_INT,	IDC_SEED,	IDC_SEEDSPIN,	1.0f,
			p_end,

		p_end 
);

//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
//|							Function Publishing System						 |
//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
FPInterfaceDesc materialStatic_action_FPInterfaceDesc(
			PFACTION_INTERFACE, 
			_T("action"), 0, 
			&ThePFOperatorMaterialStaticDesc, FP_MIXIN, 
				
			IPFAction::kInit,	_T("init"),		0,	TYPE_bool, 0, 5,
				_T("container"), 0, TYPE_IOBJECT,
				_T("particleSystem"), 0, TYPE_OBJECT,
				_T("particleSystemNode"), 0, TYPE_INODE,
				_T("actions"), 0, TYPE_OBJECT_TAB_BR,
				_T("actionNodes"), 0, TYPE_INODE_TAB_BR, 
			IPFAction::kRelease, _T("release"), 0, TYPE_bool, 0, 1,
				_T("container"), 0, TYPE_IOBJECT,
			// reserved for future use
			//IPFAction::kChannelsUsed, _T("channelsUsed"), 0, TYPE_VOID, 0, 2,
			//	_T("interval"), 0, TYPE_INTERVAL_BR,
			//	_T("channels"), 0, TYPE_FPVALUE,
			IPFAction::kActivityInterval, _T("activityInterval"), 0, TYPE_INTERVAL_BV, 0, 0,
			IPFAction::kIsFertile, _T("isFertile"), 0, TYPE_bool, 0, 0,
			IPFAction::kIsNonExecutable, _T("isNonExecutable"), 0, TYPE_bool, 0, 0,
			IPFAction::kSupportRand, _T("supportRand"), 0, TYPE_bool, 0, 0,
			IPFAction::kGetRand, _T("getRand"), 0, TYPE_INT, 0, 0,
			IPFAction::kSetRand, _T("setRand"), 0, TYPE_VOID, 0, 1,
				_T("randomSeed"), 0, TYPE_INT,
			IPFAction::kNewRand, _T("newRand"), 0, TYPE_INT, 0, 0,
			IPFAction::kIsMaterialHolder, _T("isMaterialHolder"), 0, TYPE_bool, 0, 0,
			IPFAction::kGetMaterial, _T("getMaterial"), 0, TYPE_MTL, 0, 0,
			IPFAction::kSetMaterial, _T("setMaterial"), 0, TYPE_bool, 0, 1,
				_T("materialStatic"), 0, TYPE_MTL,
			IPFAction::kSupportScriptWiring, _T("supportScriptWiring"), 0, TYPE_bool, 0, 0,

		p_end
);

FPInterfaceDesc materialStatic_operator_FPInterfaceDesc(
			PFOPERATOR_INTERFACE,
			_T("operator"), 0,
			&ThePFOperatorMaterialStaticDesc, FP_MIXIN,

			IPFOperator::kProceed, _T("proceed"), 0, TYPE_bool, 0, 7,
				_T("container"), 0, TYPE_IOBJECT,
				_T("timeStart"), 0, TYPE_TIMEVALUE,
				_T("timeEnd"), 0, TYPE_TIMEVALUE_BR,
				_T("particleSystem"), 0, TYPE_OBJECT,
				_T("particleSystemNode"), 0, TYPE_INODE,
				_T("actionNode"), 0, TYPE_INODE,
				_T("integrator"), 0, TYPE_INTERFACE,

		p_end
);

FPInterfaceDesc materialStatic_PViewItem_FPInterfaceDesc(
			PVIEWITEM_INTERFACE,
			_T("PViewItem"), 0,
			&ThePFOperatorMaterialStaticDesc, FP_MIXIN,

			IPViewItem::kNumPViewParamBlocks, _T("numPViewParamBlocks"), 0, TYPE_INT, 0, 0,
			IPViewItem::kGetPViewParamBlock, _T("getPViewParamBlock"), 0, TYPE_REFTARG, 0, 1,
				_T("index"), 0, TYPE_INDEX,
			IPViewItem::kHasComments, _T("hasComments"), 0, TYPE_bool, 0, 1,
				_T("actionNode"), 0, TYPE_INODE,
			IPViewItem::kGetComments, _T("getComments"), 0, TYPE_STRING, 0, 1,
				_T("actionNode"), 0, TYPE_INODE,
			IPViewItem::kSetComments, _T("setComments"), 0, TYPE_VOID, 0, 2,
				_T("actionNode"), 0, TYPE_INODE,
				_T("comments"), 0, TYPE_STRING,

		p_end
);


//+--------------------------------------------------------------------------+
//|							Dialog Process									 |
//+--------------------------------------------------------------------------+

INT_PTR PFOperatorMaterialStaticDlgProc::DlgProc( TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg,
											   WPARAM wParam, LPARAM lParam )
{
//	BOOL alignTo, angleDistortion;
	IParamBlock2* pblock;

	switch ( msg )
	{
	case WM_INITDIALOG:
		// Send the message to notify the initialization of dialog
		map->GetParamBlock()->NotifyDependents( FOREVER, (PartID)map, kMaterialStatic_RefMsg_InitDlg );
		break;
	case WM_DESTROY:
		break;
	case WM_COMMAND:
		switch ( LOWORD( wParam ) )
		{
		case IDC_NEW:
			map->GetParamBlock()->NotifyDependents( FOREVER, PART_OBJ, kMaterialStatic_RefMsg_NewRand );
			break;
		case kMaterialStatic_message_assignMaterial:
			pblock = map->GetParamBlock();
			if (pblock != NULL)
				UpdateAssignMaterialDlg( hWnd, pblock->GetInt(kMaterialStatic_assignMaterial, t),
												pblock->GetInt(kMaterialStatic_assignID, t));
			break;
		case kMaterialStatic_message_assignID:
		case kMaterialStatic_message_type:
		case kMaterialStatic_message_rateType:
			pblock = map->GetParamBlock();
			if (pblock != NULL)
			{
				UpdateAssignIDDlg( hWnd, pblock->GetInt(kMaterialStatic_assignID, t),
										pblock->GetInt(kMaterialStatic_type, t),
										pblock->GetInt(kMaterialStatic_rateType, t) );
				UpdateAssignMaterialDlg( hWnd, pblock->GetInt(kMaterialStatic_assignMaterial, t),
												pblock->GetInt(kMaterialStatic_assignID, t));
			}
			break;
		}
		break;
	}
	return FALSE;
}

void PFOperatorMaterialStaticDlgProc::UpdateAssignMaterialDlg( HWND hWnd, int assign, int assignID )
{
	ICustButton* but = GetICustButton(GetDlgItem(hWnd, IDC_MATERIAL));
	but->Enable( assign != 0);
	ReleaseICustButton(but);

	BOOL enable = (assign != 0) || (assignID != 0);
	EnableWindow( GetDlgItem( hWnd, IDC_TEXT_MTLIDOFFSET), enable);

	ISpinnerControl* spin = GetISpinner(GetDlgItem(hWnd, IDC_MTLIDOFFSETSPIN));
	spin->Enable( enable );
	ReleaseISpinner( spin );
}

void PFOperatorMaterialStaticDlgProc::UpdateAssignIDDlg( HWND hWnd, int assign, int type, int rateType)
{
	ISpinnerControl *spin;
	bool enableAssign = (assign != 0);
	
	EnableWindow( GetDlgItem( hWnd, IDC_SHOWINVIEWPORT), enableAssign);
	EnableWindow( GetDlgItem( hWnd, IDC_TYPE_ID), enableAssign);
	EnableWindow( GetDlgItem( hWnd, IDC_TYPE_CYCLE), enableAssign);
	EnableWindow( GetDlgItem( hWnd, IDC_TYPE_RANDOM), enableAssign);

	bool enableMtlID = (enableAssign && (type == kMaterialStatic_type_id));
	spin = GetISpinner(GetDlgItem(hWnd, IDC_MATERIALIDSPIN));
	spin->Enable( enableMtlID );
	ReleaseISpinner( spin );
	
	bool enableIt = (enableAssign && (type != kMaterialStatic_type_id));
	EnableWindow( GetDlgItem( hWnd, IDC_NUMSUBSTEXT), enableIt);
	EnableWindow( GetDlgItem( hWnd, IDC_RATEFRAME), enableIt);
	EnableWindow( GetDlgItem( hWnd, IDC_PERSECOND), enableIt);
	EnableWindow( GetDlgItem( hWnd, IDC_PERPARTICLE), enableIt);
	spin = GetISpinner(GetDlgItem(hWnd, IDC_NUMSUBSSPIN));
	spin->Enable( enableIt );
	ReleaseISpinner( spin );
	bool enableRate = (enableIt && (rateType == kMaterialStatic_rateType_second));
	spin = GetISpinner(GetDlgItem(hWnd, IDC_RATEPERSECSPIN));
	spin->Enable( enableRate );
	ReleaseISpinner( spin );
	EnableWindow( GetDlgItem( hWnd, IDC_LOOP), enableIt && (type == kMaterialStatic_type_cycle));
	enableRate = (enableIt && (rateType == kMaterialStatic_rateType_particle));
	spin = GetISpinner(GetDlgItem(hWnd, IDC_RATEPERPARTSPIN));
	spin->Enable( enableRate );
	ReleaseISpinner( spin );
}



} // end of namespace PFActions