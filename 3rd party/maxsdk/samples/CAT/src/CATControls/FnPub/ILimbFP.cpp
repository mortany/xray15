
#include "ILimbFP.h"

FPInterfaceDesc* ILimbFP::GetDesc()
{
	return GetFnPubDesc();
}

FPInterfaceDesc* ILimbFP::GetFnPubDesc()
{
	/**********************************************************************
	 * Function publishing interface descriptor...
	 */
	static FPInterfaceDesc iLimb_FPinterface(
		CatAPI::LIMB_INTERFACE_FP, _T("LimbFPInterface"), 0, NULL, FP_MIXIN,

		ILimbFP::fnCreateIKTarget, _T("CreateIKTarget"), 0, TYPE_INODE, 0, 0,
		ILimbFP::fnRemoveIKTarget, _T("RemoveIKTarget"), 0, TYPE_VOID, 0, 0,
		ILimbFP::fnMoveIKTargetToEndOfLimb, _T("MoveIKTargetToEndOfLimb"), 0, TYPE_VOID, 0, 1,
			_T("time"), 0, TYPE_TIMEVALUE,

		ILimbFP::fnCreateUpNode, _T("CreateUpNode"), 0, TYPE_INODE, 0, 0,
		ILimbFP::fnRemoveUpNode, _T("RemoveUpNode"), 0, TYPE_VOID, 0, 0,

		ILimbFP::fnCreatePalmAnkle, _T("CreatePalmAnkle"), 0, TYPE_INTERFACE, 0, 0,
		ILimbFP::fnRemovePalmAnkle, _T("RemovePalmAnkle"), 0, TYPE_VOID, 0, 0,
		ILimbFP::fnCreateCollarbone, _T("CreateCollarbone"), 0, TYPE_INTERFACE, 0, 0,
		ILimbFP::fnRemoveCollarbone, _T("RemoveCollarbone"), 0, TYPE_VOID, 0, 0,

		properties,

		ILimbFP::propIKTarget, FP_NO_FUNCTION, _T("IKTarget"), 0, TYPE_INODE,
		ILimbFP::propUpNode, FP_NO_FUNCTION, _T("UpNode"), 0, TYPE_INODE,
		ILimbFP::propIsLeg, FP_NO_FUNCTION, _T("IsLeg"), 0, TYPE_BOOL,
		ILimbFP::propIsArm, FP_NO_FUNCTION, _T("IsArm"), 0, TYPE_BOOL,
		ILimbFP::propGetLMR, ILimbFP::propSetLMR, _T("LMR"), 0, TYPE_INT,
		ILimbFP::propGetSymLimb, FP_NO_FUNCTION, _T("SymLimb"), 0, TYPE_INTERFACE,
		ILimbFP::propPalm, FP_NO_FUNCTION, _T("Palm"), 0, TYPE_INTERFACE,
		ILimbFP::propCollarbone, FP_NO_FUNCTION, _T("Collarbone"), 0, TYPE_INTERFACE,

		p_end
	);

	return &iLimb_FPinterface;
}
