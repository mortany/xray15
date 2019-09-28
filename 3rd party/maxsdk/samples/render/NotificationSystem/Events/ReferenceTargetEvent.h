#pragma once

//**************************************************************************/
// Copyright (c) 2013 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//***************************************************************************/

#include <NotificationAPI/NotificationAPI_Events.h>
#include "IMergeableEvent.h"

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

	class ReferenceTargetEvent : public IReferenceTargetEvent, public IMergeableEvent
	{
		size_t		                    m_EventType;
		ReferenceTarget*			                m_refTarg;
        MaxSDK::Array<ParamBlockData>   m_ParamsChangedInPBlock; //Used only in case of EventType_ReferenceTarget_ParamBlock

	public:
		ReferenceTargetEvent(ReferenceTargetEventType eventType, ReferenceTarget* refTarg, const ParamBlockData* paramBlockData = NULL);
		virtual ~ReferenceTargetEvent(){};

		virtual NotifierType	                        GetNotifierType	    (void)const{return NotifierType_ReferenceTarget;};
		virtual size_t		                            GetEventType	    (void)const{return m_EventType;};
		virtual ReferenceTarget*			                        GetReferenceTarget		    (void)const{return m_refTarg;};
        virtual const MaxSDK::Array<ParamBlockData>&    GetParamBlockData   (void)const{return m_ParamsChangedInPBlock;};

        virtual void		                            DebugPrintToFile    (FILE* f)const;

         MaxSDK::Array<ParamBlockData>&                 GetParamBlockData   (void){return m_ParamsChangedInPBlock;}//Non-const, is used to change the array in Notification API when you merge 2 ReferenceTargetEvent dealing with paramblocks

         // -- inherited from IMergeableEvent
         virtual MergeResult merge_from(IMergeableEvent& old_event);
	};

};//end of namespace NotificationAPI
};//end of namespace Max