#pragma once

//**************************************************************************/
// Copyright (c) 2017 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// AUTHOR: Tristan Lapalme
//***************************************************************************/

namespace Max
{;
namespace NotificationAPI
{;

class NotificationSystemCriticalSectionObject {

public:

	NotificationSystemCriticalSectionObject();
	~NotificationSystemCriticalSectionObject();

	void Enter();
	void Leave();

private:

	CRITICAL_SECTION m_criticalSection;
};

inline NotificationSystemCriticalSectionObject::NotificationSystemCriticalSectionObject() {

	InitializeCriticalSection(&m_criticalSection);
}

inline NotificationSystemCriticalSectionObject::~NotificationSystemCriticalSectionObject() {

	DeleteCriticalSection(&m_criticalSection);
}

inline void NotificationSystemCriticalSectionObject::Enter() {

	EnterCriticalSection(&m_criticalSection);
}

inline void NotificationSystemCriticalSectionObject::Leave() {

	LeaveCriticalSection(&m_criticalSection);
}

class NotificationSystemAutoCriticalSection
{
public:

	// The constructor acquires the critical section object
	NotificationSystemAutoCriticalSection(NotificationSystemCriticalSectionObject& obj);
	// The destructor releases the critical section object
	~NotificationSystemAutoCriticalSection();

	// Enables releasing a critical section prematurely
	void Release();

	// Acquires a new critical section (after releasing the previous one)
	void Acquire(NotificationSystemCriticalSectionObject& obj);

private:
	// disable copy constructor and assignment operator
	NotificationSystemAutoCriticalSection(const NotificationSystemAutoCriticalSection&);
	NotificationSystemAutoCriticalSection& operator=(const NotificationSystemAutoCriticalSection&);

	NotificationSystemCriticalSectionObject* mObj;
};

inline NotificationSystemAutoCriticalSection::NotificationSystemAutoCriticalSection(NotificationSystemCriticalSectionObject& obj)
	: mObj(&obj)
{
	mObj->Enter();
}

inline NotificationSystemAutoCriticalSection::~NotificationSystemAutoCriticalSection()
{
	if (mObj != nullptr)
	{
		mObj->Leave();
	}
}

inline void NotificationSystemAutoCriticalSection::Release()
{
	if (mObj != nullptr)
	{
		mObj->Leave();
		mObj = nullptr;
	}
}

inline void NotificationSystemAutoCriticalSection::Acquire(NotificationSystemCriticalSectionObject& obj)
{
	Release();
	mObj = &obj;
	mObj->Enter();
}

};//end of namespace NotificationAPI
};//end of namespace Max