//**************************************************************************/
// Copyright (c) 1998-2018 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/

#ifndef DISABLE_UNIT_TESTS

#include <gtest\gtest.h>
#include "..\Bonesdef_TestsWrapper.h"

namespace
{
	void CheckFloatEquality(float a, float b)
	{
		EXPECT_FLOAT_EQ(a, b);
	}

	void CheckFloatNearEquality(float a, float b)
	{
		EXPECT_NEAR(a, b, FLT_EPSILON);
	}

	void CheckIntegerEquality(float a, float b)
	{
		EXPECT_EQ(a, b);
	}
};

// Create our test fixture for testing features of some Example class.
// Organizing tests into a fixture allows us to create objects and set up
// an environment for our tests.
class BonesDefUnitTestImp : public ::testing::Test
{
	// These macro calls declare this suite and each of the test functions.
	// If you don't include this block, CppUnit will not find your tests.
public:
	// Optionally override the base class's setUp function to create some
	// data for the tests to use.
	virtual void SetUp() override
	{
		BonesdefTestsWrapper::Init(CheckIntegerEquality, CheckFloatEquality);
	}

	// Optionally override the base class's tearDown to clear out any changes
	// and release resources.
	virtual void TearDown() override
	{
		BonesdefTestsWrapper::Clean();
	}
};

TEST_F(BonesDefUnitTestImp, WeightCountTest)
{
	BonesdefTestsWrapper::TestWeightCount();
}

TEST_F(BonesDefUnitTestImp, GetWeightTest)
{
	BonesdefTestsWrapper::TestWeight();
}

TEST_F(BonesDefUnitTestImp, BoneIndexTest)
{
	BonesdefTestsWrapper::TestBoneIndex();
}

TEST_F(BonesDefUnitTestImp, NormalizeWeightsTest)
{
	BonesdefTestsWrapper::TestNormalizeWeights();
}

#endif