//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#include "resource.h"

#include "CustomSceneStreamManager.h"
#include <notify.h>
#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>

// method exported from core for being able to register CIP data when saving streams to scene file
CoreExport void CIP_LogCustomSceneStreamInfo(unsigned long numStreamsSaved);

using namespace MaxSDK::CustomFileStreamAPI;

static const DWORD kCustomFileStreamHeaderSize = sizeof(CustomFileStreamHeader);

static CustomSceneStreamManager sCustomSceneStreamManager (CUSTOMSCENESTREAMMANAGER_INTERFACE, _T("CustomSceneStreamManager"), 0, NULL, FP_CORE,
	CustomSceneStreamManager::kGetStreamName, _T("getStreamName"), 0, TYPE_STRING, FP_NO_REDRAW, 1,
		_T("index"), 0, TYPE_INDEX,
	CustomSceneStreamManager::kGetStreamByName, _T("getStreamByName"), 0, TYPE_INDEX, FP_NO_REDRAW, 1,
		_T("name"), 0, TYPE_STRING,
	CustomSceneStreamManager::kDoesStreamExist, _T("doesStreamExist"), 0, TYPE_bool, FP_NO_REDRAW, 1,
		_T("which"), 0, TYPE_VALUE,
	CustomSceneStreamManager::kGetStreamFlag, _T("getStreamFlag"), 0, TYPE_INT, FP_NO_REDRAW, 2,
		_T("which"), 0, TYPE_VALUE,
		_T("privateFlag"), 0, TYPE_bool, f_keyArgDefault, false,
	CustomSceneStreamManager::kSetStreamFlag, _T("setStreamFlag"), 0, TYPE_VOID, FP_NO_REDRAW, 3,
		_T("which"), 0, TYPE_VALUE,
		_T("value"), 0, TYPE_INT, 
		_T("privateFlag"), 0, TYPE_bool, f_keyArgDefault, false,
	CustomSceneStreamManager::kIsStreamDataAnArray, _T("isStreamDataAnArray"), 0, TYPE_bool, FP_NO_REDRAW, 1,
		_T("which"), 0, TYPE_VALUE,
	CustomSceneStreamManager::kGetStreamData, _T("getStreamData"), 0, TYPE_STRING, FP_NO_REDRAW, 1,
		_T("which"), 0, TYPE_VALUE,
	CustomSceneStreamManager::kGetStreamDataAsArray, _T("getStreamDataAsArray"), 0, TYPE_STRING_TAB_BV, FP_NO_REDRAW, 1,
		_T("which"), 0, TYPE_VALUE,
	CustomSceneStreamManager::kSetStreamData, _T("setStreamData"), 0, TYPE_VOID, FP_NO_REDRAW, 2,
		_T("which"), 0, TYPE_VALUE,
		_T("string"), 0, TYPE_STRING,
	CustomSceneStreamManager::kSetStreamDataAsArray, _T("setStreamDataAsArray"), 0, TYPE_VOID, FP_NO_REDRAW, 2,
		_T("which"), 0, TYPE_VALUE,
		_T("string_array"), 0, TYPE_STRING_TAB_BR, f_inOut, FPP_IN_PARAM,
	CustomSceneStreamManager::kIsStreamPersistent, _T("isStreamPersistent"), 0, TYPE_bool, FP_NO_REDRAW, 1,
		_T("which"), 0, TYPE_VALUE,
	CustomSceneStreamManager::kSetStreamPersistent, _T("setStreamPersistent"), 0, TYPE_VOID, FP_NO_REDRAW, 2,
		_T("which"), 0, TYPE_VALUE,
		_T("persistent"), 0, TYPE_bool, 
	CustomSceneStreamManager::kGetSaveNonPersistentStream, _T("getSaveNonPersistentStream"), 0, TYPE_bool, FP_NO_REDRAW, 1,
		_T("which"), 0, TYPE_VALUE,
	CustomSceneStreamManager::kSetSaveNonPersistentStream, _T("setSaveNonPersistentStream"), 0, TYPE_VOID, FP_NO_REDRAW, 2,
		_T("which"), 0, TYPE_VALUE,
		_T("val"), 0, TYPE_bool, 
	CustomSceneStreamManager::kGetNoLoadOnSceneLoad, _T("getNoLoadOnSceneLoad"), 0, TYPE_bool, FP_NO_REDRAW, 1,
		_T("which"), 0, TYPE_VALUE,
	CustomSceneStreamManager::kSetNoLoadOnSceneLoad, _T("setNoLoadOnSceneLoad"), 0, TYPE_VOID, FP_NO_REDRAW, 2,
		_T("which"), 0, TYPE_VALUE,
		_T("val"), 0, TYPE_bool, 
	CustomSceneStreamManager::kDeleteStream, _T("deleteStream"), 0, TYPE_VOID, FP_NO_REDRAW, 1,
		_T("which"), 0, TYPE_VALUE,
	CustomSceneStreamManager::kCreateStream, _T("createStream"), 0, TYPE_INDEX, FP_NO_REDRAW, 4,
		_T("name"), 0, TYPE_STRING,
		_T("persistent"), 0, TYPE_bool, f_keyArgDefault, true,
		_T("saveNonPersistentStream"), 0, TYPE_bool, f_keyArgDefault, true,
		_T("noLoadOnSceneLoad"), 0, TYPE_bool, f_keyArgDefault, false,
	CustomSceneStreamManager::kRegisterCallback, _T("registerCallback"), 0, TYPE_VOID, FP_NO_REDRAW, 3,
		_T("callback_fn"), 0, TYPE_VALUE,
		_T("callback_id"), 0, TYPE_VALUE,
		_T("event"), 0, TYPE_ENUM, CustomSceneStreamManager::callback_event_types,
	CustomSceneStreamManager::kUnregisterCallbacks, _T("unregisterCallbacks"), 0, TYPE_VOID, FP_NO_REDRAW, 3,
		_T("callback_fn"), 0, TYPE_VALUE, f_keyArgDefault, NULL,
		_T("callback_id"), 0, TYPE_VALUE, f_keyArgDefault, NULL,
		_T("event"), 0, TYPE_ENUM, CustomSceneStreamManager::callback_event_types, f_keyArgDefault, -1,
	CustomSceneStreamManager::kGetCallbacks, _T("getCallbacks"), 0, TYPE_VALUE, FP_NO_REDRAW, 3,
		_T("callback_fn"), 0, TYPE_VALUE, f_keyArgDefault, NULL,
		_T("callback_id"), 0, TYPE_VALUE, f_keyArgDefault, NULL,
		_T("event"), 0, TYPE_ENUM, CustomSceneStreamManager::callback_event_types, f_keyArgDefault, -1,

	properties,
		CustomSceneStreamManager::kGetNumStreams, FP_NO_FUNCTION, _M("numEntries"), 0, TYPE_INT,
		CustomSceneStreamManager::kIsLocked, CustomSceneStreamManager::kSetLocked, _M("locked"), 0, TYPE_bool,

	enums, 
		CustomSceneStreamManager::callback_event_types, 3,
			_T("postLoad"),		CustomSceneStreamManager::kPostLoad,
			_T("preSave"),		CustomSceneStreamManager::kPreSave,
			_T("postSave"),		CustomSceneStreamManager::kPostSave,
   p_end
);

void CustomSceneStreamManager::init()
{
	RegisterNotification(&NotifyProc, this, NOTIFY_SYSTEM_PRE_NEW);
	RegisterNotification(&NotifyProc, this, NOTIFY_SYSTEM_PRE_RESET);
	RegisterNotification(&NotifyProc, this, NOTIFY_FILE_PRE_OPEN);
	RegisterNotification(&NotifyProc, this, NOTIFY_FILE_POST_OPEN_PROCESS);
	RegisterNotification(&NotifyProc, this, NOTIFY_FILE_POST_SAVE_PROCESS);
	RegisterNotification(&NotifyProc, this, NOTIFY_SYSTEM_SHUTDOWN);
}

CustomSceneStreamManager::~CustomSceneStreamManager()
{
	UnRegisterNotification(&NotifyProc, this);
}

// virtual methods....
bool CustomSceneStreamManager::DoesStreamExist(const TCHAR* name) const
{
	return (GetStreamByNameInternal(name) != -1);
}

int CustomSceneStreamManager::GetStreamByName(const TCHAR* name) const
{
	int res = GetStreamByNameInternal(name);
	if (res == -1)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_NAMED_STREAM_DOESNT_EXIST) + name;
		throw MAXException(msg);
	}
	return res;
}

int CustomSceneStreamManager::GetNumStreams() const
{
	return (int)mData.size();
}

const TCHAR* CustomSceneStreamManager::GetStreamName(int index) const
{
	ValidateIndex(index);
	return mData[index].mName;
}

DWORD CustomSceneStreamManager::GetStreamFlag(int index, bool privateFlag) const
{
	ValidateIndex(index);
	return privateFlag ? mData[index].mHeader.mPrivateFlags : mData[index].mHeader.mPublicFlags;
}

void CustomSceneStreamManager::SetStreamFlag(int index, DWORD val, bool privateFlag)
{
	ValidateIndex(index);
	if (privateFlag)
		mData[index].mHeader.mPrivateFlags = val;
	else
		mData[index].mHeader.mPublicFlags = val;
}

bool CustomSceneStreamManager::IsStreamDataAnArray(int index) const
{
	ValidateIndex(index);
	size_t nChars = mData[index].mData.AllocatedChars();
	if (nChars != 0)  // don't count trailing null
		nChars--;
	return (nChars != mData[index].mData.Length());
}

const TCHAR* CustomSceneStreamManager::GetStreamData(int index) const
{
	ValidateIndex(index);
	return mData[index].mData;
}

void CustomSceneStreamManager::GetStreamDataAsArray(int index, Tab<const TCHAR*>& data) const
{
	data.ZeroCount();
	ValidateIndex(index);
	const TCHAR* string = mData[index].mData.data();
	size_t len = _tcslen(string);
	size_t nChars = mData[index].mData.AllocatedChars();
	// handle case of just a single string...
	if (nChars == len + 1)
		nChars++;
	const TCHAR* end_of_string = string + nChars;
	while (string + len + 1 < end_of_string)
	{
		data.Append(1, &string);
		string += len + 1; // plus null char
		len = _tcslen(string);
	}
}

void CustomSceneStreamManager::SetStreamData(int index, const TCHAR* data)
{
	ValidateIndex(index);
	mData[index].mData.Resize(0);
	if (data == nullptr)
		data = _T("");
	mData[index].mData = data;
}

void CustomSceneStreamManager::SetStreamDataAsArray(int index, const Tab<const TCHAR*>& dataArray)
{
	ValidateIndex(index);
	size_t dataSize = 0; // terminating null automatically included via dataForWrite
	for (int j = 0; j < dataArray.Count(); j++)
	{
		const TCHAR* data = dataArray[j];
		if (data == nullptr)
			data = _T("");
		dataSize += (_tcslen(data) + 1);
	}
	mData[index].mData.Resize(0);
	TCHAR* string = mData[index].mData.dataForWrite(dataSize);
	size_t dataLeft = dataSize;
	for (int j = 0; j < dataArray.Count(); j++)
	{
		const TCHAR* data = dataArray[j];
		if (data == nullptr)
			data = _T("");
		_tcscpy_s(string, dataLeft, data);
		size_t len = _tcslen(data) + 1;
		string += len;
		dataLeft -= len;
	}
	*string = _T('\0');
}

bool CustomSceneStreamManager::IsStreamPersistent(int index) const
{
	ValidateIndex(index);
	return (mData[index].mHeader.mPrivateFlags & kPersistentStream) != 0;
}

void CustomSceneStreamManager::SetStreamPersistent(int index, int persistent)
{
	ValidateIndex(index);
	if (persistent)
		mData[index].mHeader.mPrivateFlags |= kPersistentStream;
	else
		mData[index].mHeader.mPrivateFlags &= ~kPersistentStream;
}

bool CustomSceneStreamManager::GetSaveNonPersistentStream(int index) const
{
	ValidateIndex(index);
	return (mData[index].mHeader.mPrivateFlags & kSaveNonPersistentStream) != 0;
}

void CustomSceneStreamManager::SetSaveNonPersistentStream(int index, int val)
{
	ValidateIndex(index);
	if (val)
		mData[index].mHeader.mPrivateFlags |= kSaveNonPersistentStream;
	else
		mData[index].mHeader.mPrivateFlags &= ~kSaveNonPersistentStream;
}

bool CustomSceneStreamManager::GetNoLoadOnSceneLoad(int index) const
{
	ValidateIndex(index);
	return (mData[index].mHeader.mPrivateFlags & kNoLoadOnSceneLoad) != 0;
}

void CustomSceneStreamManager::SetNoLoadOnSceneLoad(int index, int val)
{
	ValidateIndex(index);
	if (val)
		mData[index].mHeader.mPrivateFlags |= kNoLoadOnSceneLoad;
	else
		mData[index].mHeader.mPrivateFlags &= ~kNoLoadOnSceneLoad;
}

void CustomSceneStreamManager::DeleteStream(int index)
{
	ValidateIndex(index);
	mData.erase(mData.begin() + index);
}

int CustomSceneStreamManager::CreateStream(const TCHAR* name, bool persistent, bool saveNonPersistentStream, bool noLoadOnSceneLoad)
{
	if (name == nullptr)
		return -1;

	if (GetStreamByNameInternal(name) != -1)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_NAMED_STREAM_ALREADY_EXISTS) + name;
		throw MAXException(msg);
	}

	if (_tcslen(name) > kMaxStreamNameLength)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NAME_TOO_LONG) + name;
		throw MAXException(msg);
	}

	if (_tcslen(name) == 0)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NAME_EMPTY);
		throw MAXException(msg);
	}

	PersistedStream persistedStream;
	persistedStream.mName = name;
	if (saveNonPersistentStream)
		persistedStream.mHeader.mPrivateFlags = kSaveNonPersistentStream;
	if (persistent)
		persistedStream.mHeader.mPrivateFlags |= kPersistentStream;
	if (noLoadOnSceneLoad)
		persistedStream.mHeader.mPrivateFlags |= kNoLoadOnSceneLoad;
	mData.emplace_back(persistedStream);
	return (int)mData.size() - 1;
}

void CustomSceneStreamManager::Reset()
{
	if (!mLocked)
		mData.clear();
}

bool CustomSceneStreamManager::IsLocked() const
{
	return mLocked;
}

void CustomSceneStreamManager::SetLocked(bool locked)
{
	mLocked = locked;
}

// end virtual methods

void CustomSceneStreamManager::ValidateIndex(int index) const
{
	if (index < 0 || index >= mData.size())
	{
		TSTR msg;
		msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_INDEX_OUT_OF_RANGE), index + 1);
		throw MAXException(msg);
	}
}

int CustomSceneStreamManager::ValidateWhich(Value* which, bool throwOnError) const
{
	if (which == nullptr)
		return -1;

	int res;
	bool byName = false;
	if (is_integer_number(which))
		res = which->to_int() - 1;
	else
	{
		res = GetStreamByNameInternal(which->to_string());
		byName = true;
	}
	if (res < 0 || res >= mData.size())
	{
		if (throwOnError)
		{
			TSTR msg;
			if (byName)
				msg = MaxSDK::GetResourceStringAsMSTR(IDS_NAMED_STREAM_DOESNT_EXIST) + which->to_string();
			else if (mData.size() == 0)
				msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_INDEX_OUT_OF_RANGE_NO_ENTRIES), which->to_int());
			else
				msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_INDEX_OUT_OF_RANGE_RANGE), which->to_int(), 1, (int)mData.size());
			throw MAXException(msg);
		}
		res = -1;
	}
	return res;
}

DWORD CustomSceneStreamManager::GetStreamFlag(Value* which, bool privateFlag) const
{
	return GetStreamFlag(ValidateWhich(which), privateFlag);
}

void CustomSceneStreamManager::SetStreamFlag(Value* which, DWORD val, bool privateFlag)
{
	SetStreamFlag(ValidateWhich(which), val, privateFlag);
}

bool CustomSceneStreamManager::IsStreamDataAnArray(Value* which) const
{
	return IsStreamDataAnArray(ValidateWhich(which));
}

const TCHAR* CustomSceneStreamManager::GetStreamData(Value* which) const
{
	return GetStreamData(ValidateWhich(which));
}

Tab<const TCHAR*> CustomSceneStreamManager::GetStreamDataAsArray(Value* which) const
{
	Tab<const TCHAR*> data;
	GetStreamDataAsArray(ValidateWhich(which), data);
	return data;
}

void CustomSceneStreamManager::SetStreamData(Value* which, const TCHAR* data)
{
	SetStreamData(ValidateWhich(which), data);
}

void CustomSceneStreamManager::SetStreamDataAsArray(Value* which, const Tab<const TCHAR*>& data)
{
	SetStreamDataAsArray(ValidateWhich(which), data);
}

bool CustomSceneStreamManager::IsStreamPersistent(Value* which) const
{
	return IsStreamPersistent(ValidateWhich(which));
}

void CustomSceneStreamManager::SetStreamPersistent(Value* which, int persistent)
{
	SetStreamPersistent(ValidateWhich(which), persistent);
}

bool CustomSceneStreamManager::GetSaveNonPersistentStream(Value* which) const
{
	return GetSaveNonPersistentStream(ValidateWhich(which));
}

void CustomSceneStreamManager::SetSaveNonPersistentStream(Value* which, int val)
{
	SetSaveNonPersistentStream(ValidateWhich(which), val);
}

bool CustomSceneStreamManager::GetNoLoadOnSceneLoad(Value* which) const
{
	return GetNoLoadOnSceneLoad(ValidateWhich(which));
}

void CustomSceneStreamManager::SetNoLoadOnSceneLoad(Value* which, int val)
{
	SetNoLoadOnSceneLoad(ValidateWhich(which), val);
}

int CustomSceneStreamManager::GetStreamByNameInternal(const TCHAR* name) const
{
	size_t len = (name != nullptr) ? _tcslen(name) : 0;
	if (len == 0 || len > kMaxStreamNameLength)
		return -1;

	for (int i = 0; i < mData.size(); i++)
	{
		if (mData[i].mName == name)
			return i;
	}
	return -1;
}

bool CustomSceneStreamManager::DoesStreamExist(Value* which) const
{
	int i = ValidateWhich(which, false);
	return (i != -1);
}

void CustomSceneStreamManager::DeleteStream(Value* which)
{
	DeleteStream(ValidateWhich(which));
}

void CustomSceneStreamManager::NotifyProc(void *param, NotifyInfo *info)
{
	if (info == nullptr || param == nullptr)
		return;

	CustomSceneStreamManager* instance = (CustomSceneStreamManager*)param;
	NotifyFileProcess *nfp = (NotifyFileProcess*)info->callParam;
	switch (info->intcode)
	{
	case NOTIFY_SYSTEM_PRE_NEW:
	case NOTIFY_SYSTEM_PRE_RESET:
	case NOTIFY_FILE_PRE_OPEN:
		instance->Reset();
		break;
	case NOTIFY_FILE_POST_OPEN_PROCESS:
		instance->LoadFromFile(nfp->fname);
		instance->RunCallbacks(kPostLoad, nfp->fname);
		break;
	case NOTIFY_FILE_POST_SAVE_PROCESS:
		instance->RunCallbacks(kPreSave, nfp->fname);
		instance->SaveToFile(nfp->fname, false, false);
		instance->RunCallbacks(kPostSave, nfp->fname);
		break;
	default:
		break;
	}
}

bool CustomSceneStreamManager::LoadFromFile(const TCHAR* fileName)
{
	if (fileName == nullptr)
		return false;

	if (!mLocked)
	{
		IStorage* pFileIStorage = nullptr;
		IStorage* pIStorage = OpenStorageForRead(fileName, pFileIStorage);
		std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
		std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);
		if (pIStorage == nullptr)
			return false;

		STATSTG statstg;
		memset(&statstg, 0, sizeof(statstg));
		// Get an enumerator for this storage.
		IEnumSTATSTG *penum = nullptr;
		HRESULT hr = pIStorage->EnumElements(0, nullptr, 0, &penum);
		// define unique_ptr for guaranteed cleanup of penum
		std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIEnumAutoPtr(penum);
		if (SUCCEEDED(hr))
		{
			hr = penum->Next(1, &statstg, 0);
			while (S_OK == hr)
			{
				if (STGTY_STREAM == statstg.type && L'\005' != statstg.pwcsName[0])
				{
					IStream* pIStream = OpenStreamForRead(pIStorage, statstg.pwcsName);
					std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);
					if (ValidateStream(pIStream))
					{
						PersistedStream persistedStream;
						persistedStream.mName = statstg.pwcsName;
						LARGE_INTEGER zero = { 0, 0 };
						pIStream->Seek(zero, STREAM_SEEK_SET, nullptr);
						ULONG nb;
						hr = pIStream->Read(&persistedStream.mHeader, sizeof(persistedStream.mHeader), &nb);
						if (FAILED(hr))
							break;
						// load only if private flag bit kNoLoadOnSceneLoad is not set
						if ((persistedStream.mHeader.mPrivateFlags & kNoLoadOnSceneLoad) == 0)
						{
							// clear SaveNonPersistentStream bit from private data just in case set
							persistedStream.mHeader.mPrivateFlags &= ~kSaveNonPersistentStream;
							// get the length of the stream
							unsigned __int64 nBytes = statstg.cbSize.QuadPart;
							nBytes -= kCustomFileStreamHeaderSize; // remove header size
							size_t nChars = (nBytes / sizeof(TCHAR));
							// read the data from the stream
							TCHAR* data = persistedStream.mData.dataForWrite(nChars);
							data[nChars] = _T('\0');
							hr = pIStream->Read(data, nBytes, &nb);
							if (FAILED(hr))
								break;
							mData.emplace_back(persistedStream);
						}
					}
				}
				CoTaskMemFree(statstg.pwcsName);
				statstg.pwcsName = nullptr;
				hr = penum->Next(1, &statstg, 0);
			}
		}
		if (statstg.pwcsName)
			CoTaskMemFree(statstg.pwcsName);
		if (FAILED(hr))
			return false;
		return true;
	}
	return false;
}

bool CustomSceneStreamManager::SaveToFile(const TCHAR* fileName, bool saveAllStreams, bool overwiteExistingStreams)
{
	if (fileName == nullptr)
		return false;

	if (!mLocked)
	{
		unsigned long numStreamsSaved = 0;
		// don't create storage unless we have something to store in it
		bool haveDataToStore = false;
		for (auto streamData = mData.cbegin(); streamData != mData.cend(); streamData++)
		{
			if (!(saveAllStreams || streamData->mHeader.mPrivateFlags & (kSaveNonPersistentStream | kPersistentStream)))
				continue;
			haveDataToStore = true;
			break;
		}
		if (!haveDataToStore)
		{
			CIP_LogCustomSceneStreamInfo(numStreamsSaved);
			return true;
		}
		IStorage* pFileIStorage = nullptr;
		IStorage* pIStorage = OpenStorageForWrite(fileName, pFileIStorage);
		std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
		std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);
		if (pIStorage == nullptr)
			return false;

		for (auto streamData = mData.cbegin(); streamData != mData.cend(); streamData++)
		{
			if (!(saveAllStreams || streamData->mHeader.mPrivateFlags & (kSaveNonPersistentStream | kPersistentStream)))
				continue;
			IStream* pIStream = OpenStreamForRead(pIStorage, streamData->mName);
			if (pIStream)
			{
				pIStream->Release();
				pIStream = nullptr;
				// either don't overwrite existing stream, or delete existing stream
				if (!overwiteExistingStreams)
					continue;
				pIStorage->DestroyElement(streamData->mName);
			}
			// need to clear SaveNonPersistentStream bit from private data
			CustomFileStreamHeader header = streamData->mHeader;
			header.mPrivateFlags &= ~kSaveNonPersistentStream;
			pIStream = OpenStreamForWrite(pIStorage, streamData->mName, header.mPrivateFlags, header.mPublicFlags, header.mVersion);
			if (pIStream == nullptr)
				continue;
			std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);
			size_t nChars = streamData->mData.AllocatedChars();
			if (nChars != 0)  // don't write trailing null
				nChars--;
			ULONG nb;
			HRESULT hr = pIStream->Write(streamData->mData.data(), (ULONG)(nChars * sizeof(TCHAR)), &nb);
			if (FAILED(hr))
				continue;
			numStreamsSaved++;
		}
		CIP_LogCustomSceneStreamInfo(numStreamsSaved);
		return true;
	}
	return false;
}

void CustomSceneStreamManager::RegisterCallback(Value* callback, Value* callbackId, int callbackType)
{
	ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
	MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
	one_value_local_tls(callback);
	vl.callback = callback;

	int parameterCount = 0;
	if (!GetMAXScriptFunctionParameterCount(vl.callback, parameterCount))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_EXPECTED_FUNCTION) + callback->to_string();
		throw MAXException(msg);
	}
	if (parameterCount != 2)
	{
		TSTR msg;
		msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_FUNCTION_ARG_COUNT_ERROR), 2, parameterCount);
		throw MAXException(msg);
	}
	vl.callback = CreateWrappedMAXScriptFunction(vl.callback, _tls);

	Name* id = nullptr;
	if (is_name(callbackId))
		id = (Name*)callbackId;
	if (id == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_EXPECTED_NAME) + callbackId->to_string();
		throw MAXException(msg);
	}

	if (mMXSCallbackFns == NULL)
	{
		mMXSCallbackFns = new (GC_PERMANENT) Array(0);
		mMXSCallbackIds = new (GC_PERMANENT) Array(0);
	}
	mMXSCallbackFns->append(vl.callback); // if the function is wrapped, we want the wrapper, not the raw function
	mMXSCallbackIds->append(id);
	mMXSCallbackType.Append(1, &callbackType);
}

void CustomSceneStreamManager::RunCallbacks(CallbackTypes callbackType, const TCHAR* fname)
{
	if (fname == nullptr)
		return;

	ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
	MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
	two_value_locals_tls(fileName, index);
	vl.fileName = new String(fname);
	for (int i = 0; i < mMXSCallbackType.Count(); i++)
	{
		if (mMXSCallbackType[i] == callbackType)
		{
			try
			{
				ScopedSaveCurrentFrames scopedSaveCurrentFrames(_tls);
				Value* args[2] = { vl.fileName,  CallbackTypeToName(callbackType)};
				mMXSCallbackFns->data[i]->apply(args, 2, nullptr);
				continue;
			}
			catch (MAXScriptException& e)
			{
				ProcessMAXScriptException(e, MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_RUNNING_CALLBACK), true, true, true);
			}
			catch (...)
			{
				ProcessMAXScriptException(UnknownSystemException(), MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_RUNNING_CALLBACK), true, true, true);
			}

			// will only get here if error occurred 
			vl.index = Integer::intern(i + 1);
			mMXSCallbackFns->deleteItem_vf(&vl.index, 1);
			mMXSCallbackIds->deleteItem_vf(&vl.index, 1);
			mMXSCallbackType.Delete(i, 1);
			i--;
		}
	}
}

void CustomSceneStreamManager::UnregisterCallbacks(Value* callback, Value* callbackId, int callbackType)
{
	ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
	MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
	one_value_local_tls(index);
	for (int i = mMXSCallbackType.Count() - 1; i >= 0; i--)
	{
		if (IsSameFunction(mMXSCallbackFns->data[i], callback) || callback == nullptr)
		{
			if (mMXSCallbackIds->data[i] == callbackId || callbackId == nullptr)
			{
				if (mMXSCallbackType[i] == callbackType || callbackType == -1)
				{
					vl.index = Integer::intern(i + 1);
					mMXSCallbackFns->deleteItem_vf(&vl.index, 1);
					mMXSCallbackIds->deleteItem_vf(&vl.index, 1);
					mMXSCallbackType.Delete(i, 1);
				}
			}
		}
	}
}

Value* CustomSceneStreamManager::GetCallbacks(Value* callback, Value* callbackId, int callbackType) const
{
	ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
	MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
	two_typed_value_locals_tls(Array* result, Array* entry);
	vl.result = new Array(0);
	for (int i = mMXSCallbackType.Count() - 1; i >= 0; i--)
	{
		if (IsSameFunction(mMXSCallbackFns->data[i], callback) || callback == nullptr)
		{
			if (mMXSCallbackIds->data[i] == callbackId || callbackId == nullptr)
			{
				if (mMXSCallbackType[i] == callbackType || callbackType == -1)
				{
					vl.entry = new Array(3);
					vl.entry->append(mMXSCallbackFns->data[i]);
					vl.entry->append(mMXSCallbackIds->data[i]);
					vl.entry->append(CallbackTypeToName(mMXSCallbackType[i]));
					vl.result->append(vl.entry);
				}
			}
		}
	}
	return_value_tls(vl.result);
}

Value* CustomSceneStreamManager::CallbackTypeToName(int callbackType) const
{
	static Name* n_postLoad = (Name*)Name::intern(_T("postLoad"));
	static Name* n_preSave = (Name*)Name::intern(_T("preSave"));
	static Name* n_postSave = (Name*)Name::intern(_T("postSave"));
	static Name* n_unknown = (Name*)Name::intern(_T("unknown"));
	Value* result;
	switch (callbackType)
	{
	case kPostLoad:
		result = n_postLoad;
		break;
	case kPreSave:
		result = n_preSave;
		break;
	case kPostSave:
		result = n_postSave;
		break;
	default:
		DbgAssert(false);
		result = n_unknown;
	}
	return result;

}

bool CustomSceneStreamManager::IsSameFunction(Value* fn1, Value* fn2) const
{
	return ::IsSameMAXScriptFunction(fn1, fn2);
}
