
#include "IHubFP.h"
#include "iparamb2.h"

FPInterfaceDesc* IHubFP::GetDesc() {
	return GetFnPubDesc();
}

FPInterfaceDesc* IHubFP::GetFnPubDesc()
{
	/**********************************************************************
	 * Function publishing interface descriptor...
	 */
	static FPInterfaceDesc idHubInterfaceDesc(
		CatAPI::HUB_INTERFACE_FP, _T("HubFPInterface"), 0, NULL, FP_MIXIN,

		IHubFP::fnAddArm, _T("AddArm"), 0, TYPE_VOID, 0, 2,
			_T("AddCollarbone"), 0, TYPE_BOOL, f_keyArgDefault, TRUE,
			_T("AddPalm"), 0, TYPE_BOOL, f_keyArgDefault, TRUE,
		IHubFP::fnAddLeg, _T("AddLeg"), 0, TYPE_VOID, 0, 2,
			_T("AddCollarbone"), 0, TYPE_BOOL, f_keyArgDefault, FALSE,
			_T("AddAnkle"), 0, TYPE_BOOL, f_keyArgDefault, TRUE,
		IHubFP::fnAddSpine, _T("AddSpine"), 0, TYPE_VOID, 0, 1,
			_T("NumBones"), 0, TYPE_INT, f_keyArgDefault, 5,
		IHubFP::fnAddTail, _T("AddTail"), 0, TYPE_VOID, 0, 1,
			_T("NumBones"), 0, TYPE_INT, f_keyArgDefault, 5,

		properties,

		IHubFP::propGetPinHub, IHubFP::propSetPinHub, _T("PinHub"), 0, TYPE_BOOL,

		p_end
	);

	return &idHubInterfaceDesc;
}
