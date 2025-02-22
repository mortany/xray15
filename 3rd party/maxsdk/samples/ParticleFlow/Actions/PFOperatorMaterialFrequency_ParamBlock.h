/**********************************************************************
 *<
	FILE:			PFOperatorMaterialFrequency_ParamBlock.h

	DESCRIPTION:	ParamBlocks2 for MaterialFrequency Operator (declaration)
											 
	CREATED BY:		Oleg Bayborodin

	HISTORY:		created 06-14-2002

	*>	Copyright 2008 Autodesk, Inc.  All rights reserved.

	Use of this software is subject to the terms of the Autodesk license
	agreement provided at the time of installation or download, or which
	otherwise accompanies this software in either electronic or hard copy form.  

 **********************************************************************/

#ifndef  _PFOPERATORMATERIALFREQUENCY_PARAMBLOCK_H_
#define  _PFOPERATORMATERIALFREQUENCY_PARAMBLOCK_H_

#include "max.h"
#include "iparamm2.h"

#include "PFOperatorMaterialFrequencyDesc.h"

namespace PFActions {

const int	kMaterialFrequency_minNumMtls = 1;
const float kMaterialFrequency_minRate = 0.01f;
const float kMaterialFrequency_maxRate = 100.0f;


// block IDs
enum { kMaterialFrequency_mainPBlockIndex };


// param IDs
enum {	kMaterialFrequency_assignMaterial,
		kMaterialFrequency_material,
		kMaterialFrequency_assignID,
		kMaterialFrequency_showInViewport,
		kMaterialFrequency_id1,
		kMaterialFrequency_id2,
		kMaterialFrequency_id3,
		kMaterialFrequency_id4,
		kMaterialFrequency_id5,
		kMaterialFrequency_id6,
		kMaterialFrequency_id7,
		kMaterialFrequency_id8,
		kMaterialFrequency_id9,
		kMaterialFrequency_id10,
		kMaterialFrequency_randomSeed,
		kMaterialFrequency_version_obsolete,
		kMaterialFrequency_mtlIDOffset
};

// User Defined Reference Messages from PB
enum {	kMaterialFrequency_RefMsg_InitDlg = REFMSG_USER + 13279,
		kMaterialFrequency_RefMsg_NewRand };

// dialog messages
enum {	kMaterialFrequency_message_assignMaterial = 100,		// assign material
		kMaterialFrequency_message_assignID,					// assign material ID
		kMaterialFrequency_message_numSubMtls };				// number of sub-mtls

class PFOperatorMaterialFrequencyDlgProc : public ParamMap2UserDlgProc
{
public:
	INT_PTR DlgProc( TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
	void DeleteThis() {}
private:
	static void UpdateAssignMaterialDlg( HWND hWnd, int assign, int assignID, Mtl* mtl);
	static void UpdateNumSubMtlsDlg( HWND hWnd, Mtl* mtl);
	static void UpdateAssignIDDlg( HWND hWnd, int assign);
};

extern PFOperatorMaterialFrequencyDesc ThePFOperatorMaterialFrequencyDesc;
extern ParamBlockDesc2 materialFrequency_paramBlockDesc;
extern FPInterfaceDesc materialFrequency_action_FPInterfaceDesc;
extern FPInterfaceDesc materialFrequency_operator_FPInterfaceDesc;
extern FPInterfaceDesc materialFrequency_PViewItem_FPInterfaceDesc;


} // end of namespace PFActions

#endif // _PFOPERATORMATERIALFREQUENCY_PARAMBLOCK_H_