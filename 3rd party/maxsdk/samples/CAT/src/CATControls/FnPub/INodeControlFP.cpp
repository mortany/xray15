#include <inode.h>
#include "INodeControlFP.h"
#include "../CatPlugins.h"

using namespace CatAPI;

/**********************************************************************
* Function publishing interface descriptor...
*/
class CATNodeControlFPValidator : public FPValidator
{
	bool Validate(FPInterface*, FunctionID fid, int paramNum, FPValue& val, TSTR& msg) {
		UNREFERENCED_PARAMETER(paramNum);
		switch (fid) {
		case INodeControlFP::propSetSetupModeLocks:
		case INodeControlFP::propSetAnimationLocks:
			if (val.type != TYPE_BITARRAY) {
				msg = GetString(IDS_MSG_BITARRAY);
				return false;
			}
			break;
		}
		return true;
	}
};
static CATNodeControlFPValidator catnodecontrolValidator;

FPInterfaceDesc* INodeControlFP::GetDesc()
{
	return GetFnPubDesc();
}

FPInterfaceDesc* INodeControlFP::GetFnPubDesc()
{
	// Descriptor
	static FPInterfaceDesc catnode_control_FPinterface(
		I_NODECONTROL, _T("CATNodeControlFPInterface"), 0, NULL, FP_MIXIN,

		INodeControlFP::fnAddArbBone, _T("AddArbBone"), 0, TYPE_INTERFACE, 0, 1,
			_T("AsNewGroup"), 0, TYPE_BOOL, f_keyArgDefault, FALSE,
		INodeControlFP::fnGetArbBone, _T("GetArbBone"), 0, TYPE_INTERFACE, 0, 1,
			_T("index"), 0, TYPE_INDEX,
		INodeControlFP::fnCreateArbBoneController, _T("CreateLayerMatrix3"), 0, TYPE_CONTROL, 0, 1,
			_T("AsNewGroup"), 0, TYPE_BOOL, f_keyArgDefault, FALSE,
		INodeControlFP::fnCreateLayerFloat, _T("CreateLayerFloat"), 0, TYPE_CONTROL, 0, 0,

		properties,

		INodeControlFP::propGetNumArbBones, FP_NO_FUNCTION, _T("NumArbBones"), 0, TYPE_INT,
		INodeControlFP::propGetNode, FP_NO_FUNCTION, _T("Node"), 0, TYPE_INODE,
		INodeControlFP::propGetAddress, FP_NO_FUNCTION, _T("Address"), 0, TYPE_TSTR_BV,
		INodeControlFP::propGetSetupTM, INodeControlFP::propSetSetupTM, _T("SetupTM"), 0, TYPE_MATRIX3_BV,
		INodeControlFP::propBoneDimensions, FP_NO_FUNCTION, _T("BoneDimensions"), 0, TYPE_POINT3_BV,

		INodeControlFP::propGetSetupStretchy, INodeControlFP::propSetSetupStretchy, _T("SetupStretchy"), 0, TYPE_BOOL,
		INodeControlFP::propGetAnimationStretchy, INodeControlFP::propSetAnimationStretchy, _T("AnimationStretchy"), 0, TYPE_BOOL,
		INodeControlFP::propGetEffectHierarchy, INodeControlFP::propSetEffectHierarchy, _T("EffectHierarchy"), 0, TYPE_BOOL,
		INodeControlFP::propGetUseSetupController, INodeControlFP::propSetUseSetupController, _T("UseSetupController"), 0, TYPE_BOOL,

		INodeControlFP::propNumLayerfloats, FP_NO_FUNCTION, _T("NumLayerFloats"), 0, TYPE_INT,

		INodeControlFP::propGetSetupModeInherit, INodeControlFP::propSetSetupModeInherit, _T("SetupModeInheritance"), 0, TYPE_BITARRAY,
		INodeControlFP::propGetAnimationModeInherit, INodeControlFP::propSetAnimationModeInherit, _T("AnimationModeInheritance"), 0, TYPE_BITARRAY,
		INodeControlFP::propGetRelativeToSetupMode, INodeControlFP::propSetRelativeToSetupMode, _T("RelativeToSetup"), 0, TYPE_BOOL,

		INodeControlFP::propGetSetupModeLocks, INodeControlFP::propSetSetupModeLocks, _T("SetupModeLocks"), 0, TYPE_BITARRAY,
			f_validator, &catnodecontrolValidator,
		INodeControlFP::propGetAnimationLocks, INodeControlFP::propSetAnimationLocks, _T("AnimationLocks"), 0, TYPE_BITARRAY,
			f_validator, &catnodecontrolValidator,
		INodeControlFP::propGetMirrorBone, INodeControlFP::propSetMirrorBone, _T("MirrorBone"), 0, TYPE_INODE,

		p_end
	);

	return &catnode_control_FPinterface;
}
