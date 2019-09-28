//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <CustomFileStreamAPI\CustomFileStreamAPI.h>
#include "ICustomSceneStreamManager.h"
#include <maxapi.h>

class Array;

class CustomSceneStreamManager : public ICustomSceneStreamManager
{
public:
	struct PersistedStream
	{
		TSTR mName;				// the persisted stream's name
		MaxSDK::CustomFileStreamAPI::CustomFileStreamHeader mHeader;	// the persisted stream's header
		TSTR mData = _T("");	// the persisted stream's content data. 
								// If content is string array, mData will contain embedded null characters
								// Initialize mData so that it has allocated data
	};

	// described in notify.h
	typedef struct {
		int iType;
		const TCHAR *fname;
	} NotifyFileProcess;

	enum CallbackTypes
	{
		kPostLoad,
		kPreSave,
		kPostSave
	};

	enum CustomSceneStreamManagerFps
	{
		kGetStreamName,
		kGetStreamByName,
		kDoesStreamExist,
		kGetStreamFlag, kSetStreamFlag,
		kIsStreamDataAnArray,
		kGetStreamData,
		kGetStreamDataAsArray,
		kSetStreamData,
		kSetStreamDataAsArray,
		kIsStreamPersistent, kSetStreamPersistent,
		kGetSaveNonPersistentStream, kSetSaveNonPersistentStream,
		kGetNoLoadOnSceneLoad, kSetNoLoadOnSceneLoad,
		kDeleteStream,
		kCreateStream,
		kGetNumStreams,
		kIsLocked, kSetLocked,
		kRegisterCallback,
		kUnregisterCallbacks,
		kGetCallbacks
	};

	// IDs of enumeration types used by function published methods
	enum FPEnums
	{
		callback_event_types,
	};

	DECLARE_DESCRIPTOR_INIT(CustomSceneStreamManager)
	BEGIN_FUNCTION_MAP
		FN_1(kGetStreamName, TYPE_STRING, GetStreamName, TYPE_INDEX);
		FN_1(kGetStreamByName, TYPE_INDEX, GetStreamByName, TYPE_STRING);
		FN_1(kDoesStreamExist, TYPE_bool, DoesStreamExist, TYPE_VALUE);
		FN_2(kGetStreamFlag, TYPE_DWORD, GetStreamFlag, TYPE_VALUE, TYPE_bool);
		VFN_3(kSetStreamFlag, SetStreamFlag, TYPE_VALUE, TYPE_DWORD, TYPE_bool);
		FN_1(kIsStreamDataAnArray, TYPE_bool, IsStreamDataAnArray, TYPE_VALUE);
		FN_1(kGetStreamData, TYPE_STRING, GetStreamData, TYPE_VALUE);
		FN_1(kGetStreamDataAsArray, TYPE_STRING_TAB_BV, GetStreamDataAsArray, TYPE_VALUE);
		VFN_2(kSetStreamData, SetStreamData, TYPE_VALUE, TYPE_STRING);
		VFN_2(kSetStreamDataAsArray, SetStreamDataAsArray, TYPE_VALUE, TYPE_STRING_TAB_BR);
		FN_1(kIsStreamPersistent, TYPE_bool, IsStreamPersistent, TYPE_VALUE);
		VFN_2(kSetStreamPersistent, SetStreamPersistent, TYPE_VALUE, TYPE_bool);
		FN_1(kGetSaveNonPersistentStream, TYPE_bool, GetSaveNonPersistentStream, TYPE_VALUE);
		VFN_2(kSetSaveNonPersistentStream, SetSaveNonPersistentStream, TYPE_VALUE, TYPE_bool);
		FN_1(kGetNoLoadOnSceneLoad, TYPE_bool, GetNoLoadOnSceneLoad, TYPE_VALUE);
		VFN_2(kSetNoLoadOnSceneLoad, SetNoLoadOnSceneLoad, TYPE_VALUE, TYPE_bool);
		VFN_1(kDeleteStream, DeleteStream, TYPE_VALUE);
		FN_4(kCreateStream, TYPE_INDEX, CreateStream, TYPE_STRING, TYPE_bool, TYPE_bool, TYPE_bool);
		RO_PROP_FN(kGetNumStreams, GetNumStreams, TYPE_INT);
		PROP_FNS(kIsLocked, IsLocked, kSetLocked, SetLocked, TYPE_bool);
		VFN_3(kRegisterCallback, RegisterCallback, TYPE_VALUE, TYPE_VALUE, TYPE_ENUM);
		VFN_3(kUnregisterCallbacks, UnregisterCallbacks, TYPE_VALUE, TYPE_VALUE, TYPE_ENUM);
		FN_3(kGetCallbacks, TYPE_VALUE, GetCallbacks, TYPE_VALUE, TYPE_VALUE, TYPE_ENUM);
	END_FUNCTION_MAP

	virtual bool DoesStreamExist(const TCHAR* name) const override;
	virtual int GetStreamByName(const TCHAR* name) const override;
	virtual int GetNumStreams() const override;
	virtual const TCHAR* GetStreamName(int i) const override;
	virtual DWORD GetStreamFlag(int index, bool privateFlag) const override;
	virtual void SetStreamFlag(int index, DWORD val, bool privateFlag) override;
	virtual bool IsStreamDataAnArray(int index) const override;
	virtual const TCHAR* GetStreamData(int index) const override;
	void GetStreamDataAsArray(int index, Tab<const TCHAR*>& data) const override;
	virtual void SetStreamData(int index, const TCHAR* data) override;
	virtual void SetStreamDataAsArray(int index, const Tab<const TCHAR*>& data) override;
	virtual bool IsStreamPersistent(int index) const override;
	virtual void SetStreamPersistent(int index, int persistent) override;
	virtual bool GetSaveNonPersistentStream(int index) const override;
	virtual void SetSaveNonPersistentStream(int index, int val) override;
	virtual bool GetNoLoadOnSceneLoad(int index) const override;
	virtual void SetNoLoadOnSceneLoad(int index, int val) override;
	virtual void DeleteStream(int index) override;
	virtual int CreateStream(const TCHAR* name, bool persistent, bool saveNonPersistentStream, bool noLoadOnSceneLoad) override;
	virtual void Reset() override;
	virtual bool IsLocked() const override;
	virtual void SetLocked(bool locked) override;

	void ValidateIndex(int index) const;
	int ValidateWhich(Value* which, bool throwOnError = true) const;
	DWORD GetStreamFlag(Value* which, bool privateFlag) const;
	void SetStreamFlag(Value* which, DWORD val, bool privateFlag);
	bool IsStreamDataAnArray(Value* which) const;
	const TCHAR* GetStreamData(Value* which) const;
	Tab<const TCHAR*> GetStreamDataAsArray(Value* which) const;
	void SetStreamData(Value* which, const TCHAR* data);
	void SetStreamDataAsArray(Value* which, const Tab<const TCHAR*>& data);
	bool IsStreamPersistent(Value* which) const;
	void SetStreamPersistent(Value* which, int persistent);
	bool GetSaveNonPersistentStream(Value* which) const;
	void SetSaveNonPersistentStream(Value* which, int val);
	bool GetNoLoadOnSceneLoad(Value* which) const;
	void SetNoLoadOnSceneLoad(Value* which, int val);
	int GetStreamByNameInternal(const TCHAR* name) const;
	bool DoesStreamExist(Value* which) const;
	void DeleteStream(Value* which);
	static void NotifyProc(void *param, NotifyInfo *info);
	bool LoadFromFile(const TCHAR* fileName);
	bool SaveToFile(const TCHAR* fileName, bool saveAllStreams, bool overwiteExistingStreams);

	void RegisterCallback(Value* callback, Value* callbackId, int callbackType);
	void RunCallbacks(CallbackTypes callbackType, const TCHAR* fname);
	void UnregisterCallbacks(Value* callback, Value* callbackId, int callbackType);
	Value* GetCallbacks(Value* callback, Value* callbackId, int callbackType) const;
	Value* CallbackTypeToName(int callbackType) const;
	bool IsSameFunction(Value* fn1, Value* fn2) const;
	~CustomSceneStreamManager();

private:
	std::vector<PersistedStream> mData;
	bool mLocked = false;
	Array* mMXSCallbackFns = nullptr;
	Array* mMXSCallbackIds = nullptr;
	Tab<int> mMXSCallbackType;
};
