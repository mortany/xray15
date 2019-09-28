#include "IExtraRigNodesFP.h"

FPInterfaceDesc* IExtraRigNodesFP::GetDesc()
{
	return GetFnPubDesc();
}

/**********************************************************************
 * Function publishing interface descriptor...
 */
FPInterfaceDesc* IExtraRigNodesFP::GetFnPubDesc()
{
	static FPInterfaceDesc ern_FPinterface(
		I_EXTRARIGNODES_FP, _T("ExtraRigNodesInterface"), 0, NULL, FP_MIXIN,

		IExtraRigNodesFP::fnAddExtraRigNodes, _T("AddExtraRigNodes"), 0, TYPE_VOID, 0, 1,
			_T("nodes"), 0, TYPE_INODE_TAB_BV,
		IExtraRigNodesFP::fnRemoveExtraRigNodes, _T("RemoveExtraRigNodes"), 0, TYPE_VOID, 0, 1,
			_T("nodes"), 0, TYPE_INODE_TAB_BV,

		properties,

		IExtraRigNodesFP::propGetExtraRigNodes, IExtraRigNodesFP::propSetExtraRigNodes, _T("ExtraRigNodes"), 0, TYPE_INODE_TAB_BV,

		p_end
	);

	return &ern_FPinterface;
}
