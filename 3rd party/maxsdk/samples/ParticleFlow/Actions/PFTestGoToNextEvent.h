/**********************************************************************
 *<
	FILE: PFTestGoToNextEvent.h

	DESCRIPTION: GoToNextEvent Test header
				 The Test sends either all or no particles to the next
				 action list

	CREATED BY: Oleg Bayborodin

	HISTORY:		created 08-27-02

 *>	Copyright (c) 2001, All Rights Reserved.
 **********************************************************************/

#ifndef _PFTESTGOTONEXTEVENT_H_
#define _PFTESTGOTONEXTEVENT_H_

#include "max.h"
#include "iparamb2.h"

#include "PFSimpleTest.h"
#include "IPFIntegrator.h"
#include "IPFSystem.h"
#include "PFClassIDs.h"

namespace PFActions {

class PFTestGoToNextEvent:	public PFSimpleTest 
{
public:
	// constructor/destructor
	PFTestGoToNextEvent();
	
	// From Animatable
	void GetClassName(TSTR& s);
	Class_ID ClassID();
	void BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev);
	void EndEditParams(IObjParam *ip,	ULONG flags,Animatable *next);

	// From ReferenceMaker
	RefResult NotifyRefChanged(const Interval& changeInt,RefTargetHandle hTarget,PartID& partID, RefMessage message, BOOL propagate);
	// From ReferenceTarget
	RefTargetHandle Clone(RemapDir &remap);

	// From BaseObject
	const TCHAR *GetObjectName();

	// from FPMixinInterface
	FPInterfaceDesc* GetDescByID(Interface_ID id);

	const	ParticleChannelMask& ChannelsUsed(const Interval& time) const;
	const	Interval ActivityInterval() const { return FOREVER; }

	// from IPViewItem interface
	bool HasCustomPViewIcons() { return true; }
	bool HasTransparentPViewIcons() const { return true; }
	HBITMAP GetActivePViewIcon();
	HBITMAP GetTruePViewIcon();
	HBITMAP GetFalsePViewIcon();
	bool HasDynamicName(TSTR& nameSuffix);

	// From IPFTest interface
	bool Proceed(IObject* pCont, PreciseTimeValue timeStart, PreciseTimeValue& timeEnd, Object* pSystem, INode* pNode, INode* actionNode, IPFIntegrator* integrator, BitArray& testResult, Tab<float>& testTime);

	// supply ClassDesc descriptor for the concrete implementation of the test
	ClassDesc* GetClassDesc() const;
};

} // end of namespace PFActions

#endif // _PFTESTGOTONEXTEVENT_H_

