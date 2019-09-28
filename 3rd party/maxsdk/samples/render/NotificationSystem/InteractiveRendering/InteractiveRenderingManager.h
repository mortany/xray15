/******************************************************************************
* Copyright 2013 Autodesk, Inc.  All rights reserved. 
* This computer source code and related instructions and comments are the 
* unpublished confidential and proprietary information of Autodesk, Inc. and 
* are protected under applicable copyright and trade secret law.  They may not 
* be disclosed to, copied or used by any third party without the prior written 
* consent of Autodesk, Inc.
******************************************************************************/
//Author : David Lanier

#pragma once

#include "NotificationAPI/InteractiveRenderingAPI_Subscription.h"

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

//Implementation of IInteractiveRenderingManager
class InteractiveRenderingManager : public IInteractiveRenderingManager
{
protected:
    MaxSDK::Array<IInteractiveRenderingClient*> m_Clients;//Clients are customers of this API listening to some events

public:
    InteractiveRenderingManager();//!< Constructor
    virtual ~InteractiveRenderingManager(){}//!< Destructor
     
    //From IInteractiveRenderingManager

    /** Use this function to create an immediate client which is the way to monitor interactive rendering events
    * and get notified as soon as a change happens
    * @param notificationClient : a Notification client to use to get notifications
	* @param version : may pass the version of Interactive Rendering client you want to use, -1 = latest.
    * @return a pointer on IInteractiveRenderingManager
	*/
    //!< 
	virtual IImmediateInteractiveRenderingClient* RegisterNewImmediateClient(Max::NotificationAPI::IImmediateNotificationClient* notificationClient, int version = -1);

    /** Use this function to create an immediate client which is the way to monitor interactive rendering events
    * and get notified as soon as a change happens
    * We are using an ImmediateNotification client to get notifications from NotificationAPI and we sort and store the events until you ask for them
    * @param version : may pass the version of Interactive Rendering client you want to use, -1 = latest.
    * @return a pointer on IInteractiveRenderingManager
	*/
    //!< 
	virtual IOnDemandInteractiveRenderingClient* RegisterNewOnDemandClient(int version = -1);


    /** RemoveClient function, to stop monitoring. Cleaning is done by the manager.
	* @param client : the notification client you want to unregister.
    * @return true if succeeded, false if the client was not found
	*/
    //!< 
	virtual bool RemoveClient(IInteractiveRenderingClient* client);
        
    /** NumNotificationClients function
    * @return the number of notification clients connected
	*/
    //!< 
    virtual size_t NumClients()const;

    /** GetNotificationClient function
	* @param index : the index of the Notification clients to retrieve
    * @return a pointer on a INotificationClient or NULL if index is out of range
	*/
    //!< 
    virtual const IInteractiveRenderingClient* GetClient(size_t index)const;

    /** DebugPrintToFile function
	* @param file : a valid FILE pointer to print debug information to
	*/
    //!< 
    virtual void DebugPrintToFile(FILE* file)const;
};

}}; //end of namespace 