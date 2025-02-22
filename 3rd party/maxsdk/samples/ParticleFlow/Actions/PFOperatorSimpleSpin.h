/**********************************************************************
 *<
	FILE:			PFOperatorSimpleSpin.h

	DESCRIPTION:	SimpleSpin Operator header
					Operator to effect speed unto particles

	CREATED BY:		David C. Thompson

	HISTORY:		created 02-01-02

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/

#ifndef _PFOPERATORSIMPLESPIN_H_
#define _PFOPERATORSIMPLESPIN_H_

#include "IPFOperator.h"
#include "PFSimpleOperator.h"
#include "RandObjLinker.h"

namespace PFActions {

class PFOperatorSimpleSpin:	public PFSimpleOperator 
{
public:
	PFOperatorSimpleSpin();

	// From Animatable
	void GetClassName(TSTR& s);
	Class_ID ClassID();
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

	// From ReferenceMaker
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	// From ReferenceTarget
	RefTargetHandle Clone(RemapDir &remap);

	// From BaseObject
	const TCHAR *GetObjectName();

	// from FPMixinInterface
	FPInterfaceDesc* GetDescByID(Interface_ID id);

	// From IPFAction interface
	const ParticleChannelMask& ChannelsUsed(const Interval& time) const;
	const Interval ActivityInterval() const { return FOREVER; }

	// From IPFOperator interface
	bool Proceed(IObject* pCont, PreciseTimeValue timeStart, PreciseTimeValue& timeEnd, Object* pSystem, INode* pNode, INode* actionNode, IPFIntegrator* integrator);

	// from IPViewItem interface
	bool HasCustomPViewIcons() { return true; }
	bool HasTransparentPViewIcons() const { return true; }
	HBITMAP GetActivePViewIcon();
	HBITMAP GetInactivePViewIcon();
	bool HasDynamicName(TSTR& nameSuffix);

	// supply ClassDesc descriptor for the concrete implementation of the operator
	ClassDesc* GetClassDesc() const;

	bool	SupportRand() const { return true; }
	int		GetRand();
	void	SetRand(int seed);
};

} // end of namespace PFActions

#endif // _PFOPERATORSIMPLESPIN_H_