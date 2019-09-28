
// C++ Unit tests

// Using a global guard macro allows us to prevent the build system from
// cluttering up production builds with unit test code. 
//
// You can remove this unit test from the build by adding /DDISABLE_UNIT_TESTS
// to the CL environment variable.

#pragma once

#ifndef DISABLE_UNIT_TESTS

#include <gtest/gtest.h>


//Public Notification API
#include "NotificationAPI/NotificationAPI_Events.h"
#include "NotificationAPI/NotificationAPI_Subscription.h"


// Create our test fixture for testing features of some Example class.
// Organizing tests into a fixture allows us to create objects and set up
// an environment for our tests.
class NotificationAPITest : public ::testing::Test
{

public:
   // Optionally override the base class's setUp function to create some
   // data for the tests to use.
   virtual void SetUp();

   // Optionally override the base class's tearDown to clear out any changes
   // and release resources.
   virtual void TearDown();
  
};


#endif //End of #ifndef DISABLE_UNIT_TESTS