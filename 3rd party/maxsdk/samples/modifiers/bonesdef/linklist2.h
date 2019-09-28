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
#include <assert1.h>
#include <vector>
#include <memory>

template <typename T>
class LinkedEntryTv2 : public MaxHeapOperators
{
public:
	T data;
	LinkedEntryTv2(T& d) {
		data = d;
	}
};

template <typename T, typename TE>
class LinkedListTv2 : public MaxHeapOperators
{
private:
	std::vector<std::shared_ptr<TE>> mData;
	static T mInvalidInstance;
public:
	LinkedListTv2() = default;

	~LinkedListTv2() {
		New();
	}

	void New()
	{
		for (auto i = 0; i < mData.size(); ++i) {
			mData[i].reset();
		}
		mData.clear();
	}

	int	Count() {
		return static_cast<int>(mData.size());
	}

	void Append(T& item)
	{
		mData.push_back(std::make_shared<TE>(item));
	}

	T& operator[](int index)
	{
		const auto cnt = static_cast<int>(mData.size());
		DbgAssert((index >= 0 && index < cnt));
		if (index >= 0 && index < cnt) {
			return mData[index]->data;
		}
		// should never get here!
		return mInvalidInstance;
	}

};

template <typename T, typename TE>
T LinkedListTv2<T, TE>::mInvalidInstance;