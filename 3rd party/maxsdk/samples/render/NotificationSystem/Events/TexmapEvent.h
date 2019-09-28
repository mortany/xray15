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
//**************************************************************************/
// DESCRIPTION: Notification API internal (private header)
// AUTHOR: David Lanier
//***************************************************************************/

#include <NotificationAPI/NotificationAPI_Events.h>
#include "IMergeableEvent.h"

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

	//The plugin receives a ITexmapEvent instance when something is changed in a Texmap
	class TexmapEvent : public ITexmapEvent, public IMergeableEvent
	{
		size_t		                    m_EventType;
		Texmap*			                m_pTexmap;
        MaxSDK::Array<ParamBlockData>   m_ParamsChangedInPBlock; //Used only in case of EventType_Texmap_ParamBlock

	public:
		TexmapEvent(TexmapEventType eventType, Texmap* pTexmap, const ParamBlockData* paramBlockData = NULL);
		virtual ~TexmapEvent(){};

		virtual NotifierType	                        GetNotifierType	    (void)const{return NotifierType_Texmap;};
		virtual size_t		                            GetEventType	    (void)const{return m_EventType;};
		virtual Texmap*			                        GetTexmap		    (void)const{return m_pTexmap;};
        virtual const MaxSDK::Array<ParamBlockData>&    GetParamBlockData   (void)const{return m_ParamsChangedInPBlock;};

        virtual void		                            DebugPrintToFile    (FILE* f)const;

         MaxSDK::Array<ParamBlockData>&                 GetParamBlockData   (void){return m_ParamsChangedInPBlock;}//Non-const, is used to change the array in Notification API when you merge 2 TexmapEvent dealing with paramblocks

         // -- inherited from IMergeableEvent
         virtual MergeResult merge_from(IMergeableEvent& old_event);
	};

};//end of namespace NotificationAPI
};//end of namespace Max