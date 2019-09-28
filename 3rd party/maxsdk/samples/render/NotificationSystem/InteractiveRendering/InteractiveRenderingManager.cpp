/******************************************************************************
* Copyright 2013 Autodesk, Inc.  All rights reserved. 
* This computer source code and related instructions and comments are the 
* unpublished confidential and proprietary information of Autodesk, Inc. and 
* are protected under applicable copyright and trade secret law.  They may not 
* be disclosed to, copied or used by any third party without the prior written 
* consent of Autodesk, Inc.
******************************************************************************/
//Author : David Lanier

#include "InteractiveRenderingManager.h"

// Local includes
#include "Clients/ImmediateInteractiveRenderingClient.h"
#include "Clients/OnDemandInteractiveRenderingClient.h"
// std includes
#include <time.h>

using namespace Max::NotificationAPI;

//-------------------------------------------------------------------
// ----------			class IInteractiveRenderingManager	---------
//-------------------------------------------------------------------

InteractiveRenderingManager g_InteractiveRenderingManagerInstance;

IInteractiveRenderingManager* IInteractiveRenderingManager::GetManager()
{
    return static_cast<IInteractiveRenderingManager*>(&g_InteractiveRenderingManagerInstance);
}
        
//-------------------------------------------------------------------
// ----------			class InteractiveRenderingManager	---------
//-------------------------------------------------------------------

InteractiveRenderingManager::InteractiveRenderingManager()
{
    m_Clients.removeAll();
}
		
IImmediateInteractiveRenderingClient* InteractiveRenderingManager::RegisterNewImmediateClient(Max::NotificationAPI::IImmediateNotificationClient* notificationClient, int version /*= -1*/)
{
    if (! notificationClient){
        DbgAssert(0 && _T("notificationClient is NULL !"));
        return NULL;
    }

    ImmediateInteractiveRenderingClient* client  = NULL;
    switch(version){
        //Handle here any versionning for the interactive rendering API
        case -1:
        default:
            //Create latest version
			client = new ImmediateInteractiveRenderingClient(*notificationClient);
        break;
    }
    DbgAssert(NULL != client && _T("client is NULL"));

    m_Clients.append(client);

    return client;
}

IOnDemandInteractiveRenderingClient* InteractiveRenderingManager::RegisterNewOnDemandClient(int version /*= -1*/)
{
    IOnDemandInteractiveRenderingClient* client  = NULL;
    switch(version){
        //Handle here any versionning for the interactive rendering API
        case -1:
        default:
            //Create latest version
            {
                NotificationAPI::IImmediateNotificationClient* notification_client = NotificationAPI::INotificationManager::GetManager()->RegisterNewImmediateClient();
                DbgAssert(NULL != notification_client);
                if(notification_client != nullptr)
                {
			        client = new OnDemandInteractiveRenderingClient(*notification_client);
                }
            }
        break;
    }
    DbgAssert(NULL != client && _T("client is NULL"));

    if(client != nullptr)
    {
        m_Clients.append(client);   
    }

    return client;
}

bool InteractiveRenderingManager::RemoveClient(IInteractiveRenderingClient* client)
{
    DbgAssert(client && _T("client is NULL !"));
    if (client){
        const size_t index = m_Clients.find(client);
        if (index == (size_t)-1){
            DbgAssert(0 && _T("client pointer not found in m_Clients !"));
            return false;
        }

        IInteractiveRenderingClient* pClient = m_Clients[index];

        delete pClient;
		pClient = NULL;

        m_Clients.removeAt(index);
        
        return true;
    }

    return false;
}
        
size_t InteractiveRenderingManager::NumClients()const
{
    return m_Clients.length();
}

const IInteractiveRenderingClient* InteractiveRenderingManager::GetClient(size_t index)const
{
    const size_t NumClients = m_Clients.length();

    if (index > NumClients){
        DbgAssert(0 && _T("Invalid index"));
        return NULL;
    }
    
    DbgAssert(NULL != m_Clients[index] &&_T("client is null"));
    return m_Clients[index];
}

void InteractiveRenderingManager::DebugPrintToFile(FILE* file)const
{
    if (! file){
        DbgAssert(0 &&_T("file is NULL"));
        return;
    }

    time_t ltime;
	time(&ltime);
    _ftprintf(file, _T("** InteractiveRenderingManager debugging : %s **\n"),_tctime(&ltime));

	size_t indent = 1; //Start with one tab char
    
    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }
    
    const size_t numClients = m_Clients.length();
    _ftprintf(file, indentString);
    _ftprintf(file, _T("** Num clients : %llu **\n"), numClients);
    for (size_t i=0;i<numClients;++i){
        IInteractiveRenderingClient* pClient = m_Clients[i];
        if (NULL == pClient){
            DbgAssert(0 && _T("NULL == pClient"));
            _ftprintf(file, _T("\n** ERROR : IInteractiveRenderingClient pointer #%llu is NULL !! **\n"), i);
            continue;
        }

        _ftprintf(file, indentString);
        _ftprintf(file, _T("** Client #%llu : **\n"), i);
        pClient->DebugPrintToFile(file);
    }
}
