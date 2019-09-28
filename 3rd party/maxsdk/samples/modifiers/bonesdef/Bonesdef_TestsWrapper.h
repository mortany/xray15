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

#pragma once

#include "Bonesdef_VertexWeights.h"
#include <memory>
#include <functional>

class BonesdefTestsWrapper : public MaxHeapOperators
{
public:
	__declspec(dllexport) static bool Init(std::function<void(int, int)> iTester, std::function<void(float, float)> fTester);
	__declspec(dllexport) static bool Clean();

	__declspec(dllexport) static void TestWeightCount();
	__declspec(dllexport) static void TestBoneIndex();
	__declspec(dllexport) static void TestWeight();
	__declspec(dllexport) static void TestNormalizeWeights();


private:
	static void BindFloatCallback(std::function<void(float, float)> fct);
	static void BindIntegerCallback(std::function<void(int, int)> fct);
	static int GetWeightCount(int vertexId);
	static int GetBoneIndex(int vertexId, int boneIndex);
	static float GetWeight(int vertexId, int boneIndex);
	static void NormalizeWeights(int vertexId);
	static float GetNormalizedWeight(int vertexId, int boneIndex);
	//
	static BoneVertexMgr gBoneVertexMgr;
	static Tab<VertexListClass*> gVertexPtrList;
	static std::function<void(float, float)> FloatTestHandler;
	static std::function<void(int, int)> IntegerTestHandler;
};

