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

#include "Bonesdef_TestsWrapper.h"
#include <cmath>
#include <limits>

std::function<void(int, int)> BonesdefTestsWrapper::IntegerTestHandler = nullptr;
std::function<void(float, float)> BonesdefTestsWrapper::FloatTestHandler = nullptr;
BoneVertexMgr BonesdefTestsWrapper::gBoneVertexMgr = nullptr;
Tab<VertexListClass*> BonesdefTestsWrapper::gVertexPtrList;

namespace {
	VertexInfluenceListClass GetInstance(int bones, float influences, float normalizedInfluences, int curveIds, int segIds, float u,
		Point3 tangents, Point3	oPoints)
	{
		VertexInfluenceListClass data;
		data.Bones = bones;
		data.Influences = influences;
		data.normalizedInfluences = normalizedInfluences;
		data.SubCurveIds = curveIds;
		data.SubSegIds = segIds;
		data.u = u;
		data.Tangents = tangents;
		data.OPoints = oPoints;

		return data;
	}

	bool AreSame(float a, float b)
	{
		return std::fabs(a - b) < std::numeric_limits<float>::epsilon();
	}
};

// a 16 vertices plane influenced by 4 bones
bool BonesdefTestsWrapper::Init(std::function<void(int, int)> iTester, std::function<void(float, float)> fTester)
{
	BindIntegerCallback(iTester);
	BindFloatCallback(fTester);

	gBoneVertexMgr = std::make_shared<BoneVertexDataManager>();
	gVertexPtrList.SetCount(16);	
	gBoneVertexMgr->SetVertexDataPtr(&gVertexPtrList);

	// vertex 0
	auto vrtxId = 0;
	auto *vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	VertexInfluenceListClass obj = GetInstance(0, 0.009f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(2, 1.0f, 0.0f, 0, 0, 0.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
	vc->AppendWeight(obj);
	gVertexPtrList[0] = vc;

	// vertex 1
	vrtxId = 1;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(0, 0.032f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(2, 0.506f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(3, 0.003f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[1] = vc;

	// vertex 2
	vrtxId = 2;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(1, 0.032f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(2, 0.003f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(3, 0.506f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[2] = vc;

	// vertex 3
	vrtxId = 3;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(1, 0.009f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(3, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[3] = vc;

	// vertex 4
	vrtxId = 4;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(0, 0.437f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(2, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[4] = vc;

	// vertex 5
	vrtxId = 5;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(0, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(2, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(3, 0.087f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[5] = vc;

	// vertex 6
	vrtxId = 6;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(1, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(2, 0.087f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(3, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[6] = vc;

	// vertex 7
	vrtxId = 7;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(1, 0.437f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(3, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[7] = vc;

	// vertex 8
	vrtxId = 8;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(0, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(2, 0.437f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[8] = vc;

	// vertex 9
	vrtxId = 9;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(0, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(2, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(3, 0.087f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[9] = vc;

	// vertex 10
	vrtxId = 10;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(1, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(2, 0.087f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(3, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[10] = vc;

	// vertex 11
	vrtxId = 10;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(1, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(3, 0.437f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[11] = vc;

	// vertex 12
	vrtxId = 12;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(0, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(2, 0.009f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[12] = vc;

	// vertex 13
	vrtxId = 13;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(0, 0.506f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(1, 0.003f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(2, 0.032f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[13] = vc;

	// vertex 14
	vrtxId = 14;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(0, 0.003f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(1, 0.506f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(3, 0.032f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[14] = vc;

	// vertex 15
	vrtxId = 15;
	vc = new VertexListClass();
	vc->SetVertexDataManager(vrtxId, gBoneVertexMgr);
	obj = GetInstance(1, 1.000f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	obj = GetInstance(3, 0.009f, 0.0f, 0, 0, 0.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
	vc->AppendWeight(obj);
	gVertexPtrList[15] = vc;

	return true;
};

bool BonesdefTestsWrapper::Clean()
{
	for (auto i=0; i< gVertexPtrList.Count(); ++i)
	{
		if (gVertexPtrList[i] != nullptr)
		{
			delete gVertexPtrList[i];
			gVertexPtrList[i] = nullptr;
		}
	}
	return true;
}

int BonesdefTestsWrapper::GetWeightCount(int vertexId)
{
	if (vertexId < gVertexPtrList.Count() && gVertexPtrList[vertexId])
	{
		return gVertexPtrList[vertexId]->WeightCount();
	}
	return -1;
}

int BonesdefTestsWrapper::GetBoneIndex(int vertexId, int boneIndex)
{
	if (vertexId < gVertexPtrList.Count() && gVertexPtrList[vertexId])
	{
		return gVertexPtrList[vertexId]->GetBoneIndex(boneIndex);
	}
	return -1;
}

float BonesdefTestsWrapper::GetWeight(int vertexId, int boneIndex)
{
	if (vertexId < gVertexPtrList.Count() && gVertexPtrList[vertexId])
	{
		return gVertexPtrList[vertexId]->GetWeight(boneIndex);
	}
	return 0.0f;
}

void BonesdefTestsWrapper::NormalizeWeights(int vertexId)
{
	if (vertexId < gVertexPtrList.Count() && gVertexPtrList[vertexId])
	{
		return gVertexPtrList[vertexId]->NormalizeWeights();
	}
}

float BonesdefTestsWrapper::GetNormalizedWeight(int vertexId, int boneIndex)
{
	if (vertexId < gVertexPtrList.Count() && gVertexPtrList[vertexId])
	{
		return gVertexPtrList[vertexId]->GetNormalizedWeight(boneIndex);
	}
	return 0.0f;
}

//
void BonesdefTestsWrapper::BindIntegerCallback(std::function<void(int, int)> fct) {
	IntegerTestHandler = std::bind(fct, std::placeholders::_1, std::placeholders::_2);
}

void BonesdefTestsWrapper::BindFloatCallback(std::function<void(float, float)> fct)
{
	FloatTestHandler = std::bind(fct, std::placeholders::_1, std::placeholders::_2);
}

void BonesdefTestsWrapper::TestWeightCount()
{
	// for vertex 0
	auto vrtxId = 0;
	auto totalWeights = GetWeightCount(vrtxId);
	if (IntegerTestHandler)
	{
		IntegerTestHandler(totalWeights, 2);
	}

	// for vertex 5
	vrtxId = 5;
	totalWeights = GetWeightCount(vrtxId);
	if (IntegerTestHandler)
	{
		IntegerTestHandler(totalWeights, 3);
	}

	// for vertex 10
	vrtxId = 10;
	totalWeights = GetWeightCount(vrtxId);
	if (IntegerTestHandler)
	{
		IntegerTestHandler(totalWeights, 3);
	}
}

void BonesdefTestsWrapper::TestBoneIndex()
{
	// for vertex 2
	auto vrtxId = 2;
	auto index = 0;
	auto boneIndex = GetBoneIndex(vrtxId, index);
	if (IntegerTestHandler)
	{
		IntegerTestHandler(boneIndex, 1);
	}
	//
	index = 1;
	boneIndex = GetBoneIndex(vrtxId, index);
	if (IntegerTestHandler)
	{
		IntegerTestHandler(boneIndex, 2);
	}
	//
	index = 2;
	boneIndex = GetBoneIndex(vrtxId, index);
	if (IntegerTestHandler)
	{
		IntegerTestHandler(boneIndex, 3);
	}

	// for vertex 7
	vrtxId = 7;
	index = 0;
	boneIndex = GetBoneIndex(vrtxId, index);
	if (IntegerTestHandler)
	{
		IntegerTestHandler(boneIndex, 1);
	}
	//
	index = 1;
	boneIndex = GetBoneIndex(vrtxId, index);
	if (IntegerTestHandler)
	{
		IntegerTestHandler(boneIndex, 3);
	}
}

void BonesdefTestsWrapper::TestWeight()
{
	// for vertex 8
	auto vrtxId = 8;
	auto index = 0;
	auto weight = GetWeight(vrtxId, index);
	if (FloatTestHandler)
	{
		FloatTestHandler(weight, 1.000f);
	}
	//
	index = 1;
	weight = GetWeight(vrtxId, index);
	if (FloatTestHandler)
	{
		FloatTestHandler(weight, 0.437f);
	}

	// for vertex 15
	vrtxId = 15;
	index = 0;
	weight = GetWeight(vrtxId, index);
	if (FloatTestHandler)
	{
		FloatTestHandler(weight, 1.000f);
	}
	//
	index = 1;
	weight = GetWeight(vrtxId, index);
	if (FloatTestHandler)
	{
		FloatTestHandler(weight, 0.009f);
	}
}

void BonesdefTestsWrapper::TestNormalizeWeights()
{
	// for vertex 8
	auto vrtxId = 8;
	auto index = 0;
	NormalizeWeights(vrtxId);
	auto weight0 = GetNormalizedWeight(vrtxId, index);
	index = 1;
	auto weight1 = GetNormalizedWeight(vrtxId, index);
	auto sum = weight0 + weight1;
	if (FloatTestHandler)
	{
		FloatTestHandler(sum, 1.000f);
	}

	// for vertex 10
	vrtxId = 10;
	index = 0;
	NormalizeWeights(vrtxId);
	weight0 = GetNormalizedWeight(vrtxId, index);
	index = 1;
	weight1 = GetNormalizedWeight(vrtxId, index);
	index = 2;
	auto weight2 = GetNormalizedWeight(vrtxId, index);
	sum = weight0 + weight1 + weight2;
	if (FloatTestHandler)
	{
		FloatTestHandler(sum, 1.000f);
	}
}

