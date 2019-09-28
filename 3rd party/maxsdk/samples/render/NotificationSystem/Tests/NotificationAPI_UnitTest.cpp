
// C++ Unit tests

// Using a global guard macro allows us to prevent the build system from
// cluttering up production builds with unit test code.  
//
// You can remove this unit test from the build by adding /DDISABLE_UNIT_TESTS
// to the CL environment variable.
#ifndef DISABLE_UNIT_TESTS

#include "NotificationAPI_UnitTest.h"


using namespace MaxSDK::NotificationAPI;


void NotificationAPITest::SetUp()
{
	// nothing for now
}

void NotificationAPITest::TearDown()
{
	// nothing for now
}


TEST_F(NotificationAPITest, TestNotificationAPIManagerClient)
{
    INotificationManager* pManager = INotificationManager::GetManager();
    ASSERT_NE(nullptr, pManager) << _T("I can't create a INotificationManager*");

    //Create 3 listeners in different modes
    INotificationClient* m_NotificationListenerAlpha    = pManager->RegisterNewImmediateClient();
    EXPECT_NE(nullptr, m_NotificationListenerAlpha) << _T("I can't create m_NotificationListenerAlpha");
    INotificationClient* m_NotificationListenerBeta     = pManager->RegisterNewOnDemandClient();
	EXPECT_NE(nullptr, m_NotificationListenerBeta) << _T("I can't create m_NotificationListenerBeta");
    INotificationClient* m_NotificationListenerGamma    = pManager->RegisterNewImmediateClient();
	EXPECT_NE(nullptr, m_NotificationListenerGamma) << _T("I can't create m_NotificationListenerGamma");

    //Get them from the manager and check their modes
    size_t NumNotifEnginesConnected = pManager->NumClients();
	EXPECT_EQ((size_t)3, NumNotifEnginesConnected) << _T("Something is wrong in registration of notification listeners");
    
    const INotificationClient* notifEngineAlpha = pManager->GetClient(0);
	EXPECT_NE(nullptr, notifEngineAlpha) << _T("notifEngineAlpha is NULL");

    const INotificationClient* notifEngineBeta = pManager->GetClient(1);
	EXPECT_NE(nullptr, notifEngineBeta) << _T("notifEngineBeta is NULL");

    const INotificationClient* notifEngineGamma = pManager->GetClient(2);
	EXPECT_NE(nullptr, notifEngineGamma) << _T("notifEngineGamma is NULL");
    
    //Remove the listeners
    if (m_NotificationListenerAlpha){
       bool bResAlpha = pManager->RemoveClient(m_NotificationListenerAlpha);
	   EXPECT_TRUE(bResAlpha) << _T("Could not UnRegisterListener m_NotificationListenerAlpha");
       m_NotificationListenerAlpha = NULL;
    }

    if (m_NotificationListenerBeta){
       bool bResBeta = pManager->RemoveClient(m_NotificationListenerBeta);
	   EXPECT_TRUE(bResBeta) << _T("Could not UnRegisterListener m_NotificationListenerBeta");
       m_NotificationListenerBeta = NULL;
    }

    if (m_NotificationListenerGamma){
       bool bResGamma = pManager->RemoveClient(m_NotificationListenerGamma);
	   EXPECT_TRUE(bResGamma) << _T("Could not UnRegisterListener m_NotificationListenerGamma");
       m_NotificationListenerGamma = NULL;
    }

    NumNotifEnginesConnected = pManager->NumClients();
	EXPECT_EQ((size_t)0, NumNotifEnginesConnected ) << _T("Something is wrong in deletion of notification listeners");
}

#endif //End of #ifndef DISABLE_UNIT_TESTS