//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2009 Autodesk, Inc.  All rights reserved.
//  Copyright 2003 Character Animation Technologies.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// Full Body IK
// This project involved pre-evaluating the pose of the character,
// and then relaxing it using a full body solver.
//#define FBIK

// The copy and paste rigging methodology could be extended to pasting
// regular max stuff using the clone tools in Max.
// This project did not go too far without a generic way to
// Mirror pasted objects. Users also have not requested this feature.
//#define PASTE_EXTRA_RIG_PARTS

// Giving the IK system soft limits would be very nice and something
// that users have regularly requested.
// Although quite technically possible, I ran into problems defining the UI.
// Soft IK solvers that I have seen use an fcurve to define how the falloff occurs.
// My idea was to some how define a math function with only one or 2 parameters,
// that somehow defines the function curve. This makes it a lot simpler to
// Save rig presets and also the little widgets we use in CAT are far too small
// to display this kind of graph.
//#define SOFTIK

#include "CATControls/cat.h"

#pragma warning(push)
#pragma warning(disable:4238) // necessary for _BV types

/**********************************************************************
 */

 // Class description and stuff
class ILayerRootDesc : public ClassDesc2 {
public:
	// Used internally
	virtual void *CreateNew(class ICATParentTrans *catparenttrans) = 0;

	//////////////////////////////////////////////////////////////////
	// this is the core of the new CAT api. Plugins can register
	// themselves with CAT to add custom layer types.
	virtual void	RegisterLayerType(ClassDesc2 *desc) = 0;
	virtual int		NumLayerTypes() = 0;
	virtual ClassDesc2*		GetLayerType(int i) = 0;
};

extern ILayerRootDesc* GetCATClipRootDesc();

#define I_LAYERINFO_FP		Interface_ID(0xd83dc1, 0x4c851b6c)
class ILayerInfoFP : public FPMixinInterface {
public:
	virtual TSTR	GetName() = 0;
	virtual void	SetName(TSTR str) = 0;

	// layers can be either Absolute, Relative, or Relative world
	virtual TSTR	GetLayerType() = 0;

	virtual void	SetColour(Color colour) = 0;
	virtual Color	GetColour() = 0;

	virtual Matrix3	GetTransform(TimeValue t, Interval &valid) = 0;
	virtual void	SetTransform(TimeValue t, Matrix3 tm) = 0;

	virtual BOOL	LayerEnabled() = 0;
	virtual void	EnableLayer(BOOL bEnable = TRUE) = 0;

	virtual INode	*GetTransformNode() = 0;
	virtual BOOL	IGetTransformNodeOn() = 0;
	virtual void	ISetTransformNodeOn(BOOL on) = 0;

	virtual BOOL	IGetRemoveDisplacementOn() = 0;
	virtual void	ISetRemoveDisplacementOn(BOOL on) = 0;

	virtual int		GetIndex() = 0;
	virtual INode*	IGetCATParent() = 0;

	enum {
		fnGetTransformNode,
		propGetName,
		propSetName,
		propGetColour,
		propSetColour,
		propGetLayerType,
		propGetEnabled,
		propSetEnabled,

		propGetTransformNodeOn,
		propSetTransformNodeOn,
		propGetRemoveDisplacementOn,
		propSetRemoveDisplacementOn,
		propGetLayerIndex,
		propGetCATParent
	};

	BEGIN_FUNCTION_MAP
		FN_0(fnGetTransformNode, TYPE_INODE, GetTransformNode);
		PROP_FNS(propGetName, GetName, propSetName, SetName, TYPE_TSTR_BV);
		PROP_FNS(propGetColour, GetColour, propSetColour, SetColour, TYPE_COLOR_BV);
		RO_PROP_FN(propGetLayerType, GetLayerType, TYPE_TSTR_BV);
		PROP_FNS(propGetEnabled, LayerEnabled, propSetEnabled, EnableLayer, TYPE_BOOL);
		PROP_FNS(propGetTransformNodeOn, IGetTransformNodeOn, propSetTransformNodeOn, ISetTransformNodeOn, TYPE_BOOL);
		PROP_FNS(propGetRemoveDisplacementOn, IGetRemoveDisplacementOn, propSetRemoveDisplacementOn, ISetRemoveDisplacementOn, TYPE_BOOL);
		RO_PROP_FN(propGetLayerIndex, GetIndex, TYPE_INDEX);
		RO_PROP_FN(propGetCATParent, IGetCATParent, TYPE_INODE);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc() { return GetDescByID(I_LAYERINFO_FP); }
};
/**********************************************************************
	ILayerControlFP:
	This controller holds all layers
 */
#define I_LAYERCONTROL_FP		Interface_ID(0x201726a8, 0x269a54c5)
class ILayerControlFP : public FPMixinInterface {
public:
	//	virtual void		MakeLayerSetupValue(int i)=0;

	virtual Control*	GetLayerRoot() = 0;
	virtual Control*	GetLocalWeights() = 0;
	//virtual Control*	IGetCATControl()=0;

	virtual BOOL		GetUseSetupController() = 0;
	virtual void		SetUseSetupController(BOOL tf) = 0;
	virtual Control*		GetSetupController() = 0;

	virtual BOOL		GetRelativeBone() = 0;
	virtual void		SetRelativeBone(BOOL tf) = 0;

	virtual Control*	GetSelectedLayerCtrl() = 0;

	virtual void		BakeCurrentLayerSettings() = 0;

	virtual int				IGetNumLayers() = 0;
	virtual Tab <Control*>	IGetLayerCtrls() = 0;

	enum {
		fnMakeLayerSetupValue,
		fnBakeCurrentLayerSettings,
		propGetLayerRoot,
		propGetLocalWeights,
		propGetUseSetupController,
		propSetUseSetupController,
		propGetSetupController,
		propGetRelativeBone,
		propSetRelativeBone,
		propSelectedLayerCtrl,
		propNumLayers,
		propLayers
	};

	BEGIN_FUNCTION_MAP
//		VFN_1(fnMakeLayerSetupValue,		MakeLayerSetupValue,	TYPE_INT);
		VFN_0(fnBakeCurrentLayerSettings, BakeCurrentLayerSettings);
		RO_PROP_FN(propGetLayerRoot, GetLayerRoot, TYPE_CONTROL);
		RO_PROP_FN(propGetLocalWeights, GetLocalWeights, TYPE_CONTROL);
		PROP_FNS(propGetUseSetupController, GetUseSetupController, propSetUseSetupController, SetUseSetupController, TYPE_BOOL);
		RO_PROP_FN(propGetSetupController, GetSetupController, TYPE_CONTROL);
		PROP_FNS(propGetRelativeBone, GetRelativeBone, propSetRelativeBone, SetRelativeBone, TYPE_BOOL);
		RO_PROP_FN(propSelectedLayerCtrl, GetSelectedLayerCtrl, TYPE_CONTROL);
		RO_PROP_FN(propNumLayers, IGetNumLayers, TYPE_INT);
		RO_PROP_FN(propLayers, IGetLayerCtrls, TYPE_CONTROL_TAB_BV);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc() { return GetDescByID(I_LAYERCONTROL_FP); }
};

/**********************************************************************
	ILayerFloatControlFP:
	This controller controlls all Matrix3 Transform values in CAT
 */
#define I_LAYERMATRIX3CONTROL_FP		Interface_ID(0x7c5e151c, 0x9027d60)
class ILayerMatrix3ControlFP : public FPMixinInterface {
public:
	virtual Matrix3		GetSetupMatrix() = 0;
	virtual void		SetSetupMatrix(Matrix3 m) = 0;

	enum {
		propGetSetupMatrix,
		propSetSetupMatrix
	};

	BEGIN_FUNCTION_MAP
		PROP_FNS(propGetSetupMatrix, GetSetupMatrix, propSetSetupMatrix, SetSetupMatrix, TYPE_MATRIX3_BV);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc() { return GetDescByID(I_LAYERMATRIX3CONTROL_FP); }
};

/**********************************************************************
	ILayerFloatControlFP:
	This controller controlls all float values in CAT such as IKFK ratios
 */
#define I_LAYERFLOATCONTROL_FP		Interface_ID(0xd5a1fd7, 0x50610eb0)
class ILayerFloatControlFP : public FPMixinInterface {
public:
	virtual float		GetSetupVal() = 0;
	virtual void		SetSetupVal(float v) = 0;

	enum {
		propGetSetupVal,
		propSetSetupVal
	};

	BEGIN_FUNCTION_MAP
		PROP_FNS(propGetSetupVal, GetSetupVal, propSetSetupVal, SetSetupVal, TYPE_FLOAT);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc() { return GetDescByID(I_LAYERFLOATCONTROL_FP); }
};

/**********************************************************************
	ILayerFloatControlFP:
	This controller controlls all float values in CAT such as IKFK ratios
 */
#define I_LAYERWEIGHTSCONTROL_FP		Interface_ID(0x7b534647, 0x1dd36ea7)
class ILayerWeightsControlFP : public FPMixinInterface {
public:
	virtual float		GetCombinedWeight(int index, TimeValue t) = 0;
	virtual Control*	GetParentWeightController() = 0;
	virtual Control*	GetLayerController(int index) = 0;

	enum {
		fnGetCombinedWeight,
		fnGetLayerController,
		propGetParentWeightController
	};

	BEGIN_FUNCTION_MAP
		FNT_1(fnGetCombinedWeight, TYPE_FLOAT, GetCombinedWeight, TYPE_INDEX);
		FN_1(fnGetLayerController, TYPE_FLOAT, GetLayerController, TYPE_INDEX);
		RO_PROP_FN(propGetParentWeightController, GetParentWeightController, TYPE_CONTROL);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc() { return GetDescByID(I_LAYERWEIGHTSCONTROL_FP); }
};

#pragma warning(pop)

