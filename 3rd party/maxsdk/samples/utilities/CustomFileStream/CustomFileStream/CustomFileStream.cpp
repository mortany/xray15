//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include "resource.h"

#include "CustomFileStream.h"

using namespace MaxSDK::CustomFileStreamAPI;

static const DWORD kCustomFileStreamHeaderSize = sizeof(CustomFileStreamHeader);

static CustomFileStream sCustomSceneStream(CUSTOMFILESTREAM_INTERFACE, _T("CustomFileStream"), 0, NULL, FP_CORE,
	CustomFileStream::kGetStreamNames, _T("getStreamNames"), 0, TYPE_TSTR_TAB_BV, FP_NO_REDRAW, 1,
		_T("fileName"), 0, TYPE_FILENAME,
	CustomFileStream::kDoesStorageExist, _T("doesStorageExist"), 0, TYPE_bool, FP_NO_REDRAW, 1,
		_T("fileName"), 0, TYPE_FILENAME,
	CustomFileStream::kDoesStreamExist, _T("doesStreamExist"), 0, TYPE_bool, FP_NO_REDRAW, 2,
		_T("fileName"), 0, TYPE_FILENAME,
		_T("streamName"), 0, TYPE_STRING,
	CustomFileStream::kIsCustomFileStreamOperable, _T("isCustomFileStreamOperable"), 0, TYPE_bool, FP_NO_REDRAW, 2,
		_T("fileName"), 0, TYPE_FILENAME,
		_T("streamName"), 0, TYPE_STRING,
	CustomFileStream::kDeleteStream, _T("deleteStream"), 0, TYPE_bool, FP_NO_REDRAW, 2,
		_T("fileName"), 0, TYPE_FILENAME,
		_T("streamName"), 0, TYPE_STRING,
	CustomFileStream::kReadStream, _T("readStream"), 0, TYPE_TSTR_BV, FP_NO_REDRAW, 2,
		_T("fileName"), 0, TYPE_FILENAME,
		_T("streamName"), 0, TYPE_STRING,
	CustomFileStream::kWriteStream, _T("writeStream"), 0, TYPE_bool, FP_NO_REDRAW, 6,
		_T("fileName"), 0, TYPE_FILENAME,
		_T("streamName"), 0, TYPE_STRING,
		_T("content"), 0, TYPE_STRING,
		_T("persistent"), 0, TYPE_bool, f_keyArgDefault, true,
		_T("noLoadOnSceneLoad"), 0, TYPE_bool, f_keyArgDefault, false,
		_T("append"), 0, TYPE_bool, f_keyArgDefault, false,
	CustomFileStream::kReadStreamArray, _T("readStreamArray"), 0, TYPE_TSTR_TAB_BV, FP_NO_REDRAW, 2,
		_T("fileName"), 0, TYPE_FILENAME,
		_T("streamName"), 0, TYPE_STRING,
	CustomFileStream::kWriteStreamArray, _T("writeStreamArray"), 0, TYPE_bool, FP_NO_REDRAW, 6,
		_T("fileName"), 0, TYPE_FILENAME,
		_T("streamName"), 0, TYPE_STRING,
		_T("contentArray"), 0, TYPE_STRING_TAB_BV,
		_T("persistent"), 0, TYPE_bool, f_keyArgDefault, true,
		_T("noLoadOnSceneLoad"), 0, TYPE_bool, f_keyArgDefault, false,
		_T("append"), 0, TYPE_bool, f_keyArgDefault, false,
	CustomFileStream::kGetStreamFlags, _T("getStreamFlags"), 0, TYPE_DWORD, FP_NO_REDRAW, 3,
		_T("fileName"), 0, TYPE_FILENAME,
		_T("streamName"), 0, TYPE_STRING,
		_T("getPrivate"), 0, TYPE_bool, f_keyArgDefault, false,
	CustomFileStream::kSetStreamFlags, _T("setStreamFlags"), 0, TYPE_bool, FP_NO_REDRAW, 4,
		_T("fileName"), 0, TYPE_FILENAME,
		_T("streamName"), 0, TYPE_STRING,
		_T("flags"), 0, TYPE_DWORD,
		_T("setPrivate"), 0, TYPE_bool, f_keyArgDefault, false,
	CustomFileStream::kIsStreamDataAnArray, _T("isStreamDataAnArray"), 0, TYPE_bool, FP_NO_REDRAW, 2,
		_T("fileName"), 0, TYPE_FILENAME,
		_T("streamName"), 0, TYPE_STRING,
	p_end
);

Tab<TSTR*> CustomFileStream::fpGetStreamNames(const TCHAR * fileName) const
{
	MaxSDK::Array<TSTR> streamNames;
	GetStreamNames(fileName, streamNames);
	Tab<TSTR*> res;
	for (int i = 0; i < streamNames.length(); i++)
	{
		TSTR* streamName = new TSTR(streamNames[i]);
		res.Append(1, &streamName);
	}
	return res;
}

void CustomFileStream::GetStreamNames(const TCHAR * fileName, MaxSDK::Array<TSTR>& streamNames) const
{
	streamNames.setLengthUsed(0);
	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}

	IStorage* pFileIStorage = nullptr;
	IStorage* pIStorage = OpenStorageForRead(fileName, pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);
	if (pFileIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_READ) + fileName;
		throw MAXException(msg);
	}

	std::vector<std::wstring> streamNameVec;
	if (pIStorage)
	{
		bool result = ::GetStreamNames(pIStorage, streamNameVec);
		if (!result)
		{
			TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_PERFORMING_OLE_OPERATION) + _T("EnumElements");
			throw MAXException(msg);
		}
	}

	for (auto name : streamNameVec)
	{
		streamNames.append(name.c_str());
	}
}

bool CustomFileStream::DoesStorageExist(const TCHAR* fileName) const
{
	DbgAssert(fileName);
	if (fileName == nullptr)
		return false;

	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}
	return DoesCustomFileStreamStorageExist(fileName);
}

bool CustomFileStream::DoesStreamExist(const TCHAR* fileName, const TCHAR* streamName) const
{
	DbgAssert(fileName);
	DbgAssert(streamName);
	if (fileName == nullptr || streamName == nullptr)
		return false;

	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}

	IStorage* pFileIStorage = nullptr;
	IStorage* pIStorage = OpenStorageForRead(fileName, pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);
	if (pFileIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_READ) + fileName;
		throw MAXException(msg);
	}

	if (pIStorage == nullptr)
		return false;

	IStream* pIStream = OpenStreamForRead(pIStorage, streamName);
	if (pIStream == nullptr)
		return false;
	// define unique_ptr for guaranteed cleanup of pIStream
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);

	if (ValidateStream(pIStream))
		return true;
	return false;
}

bool CustomFileStream::IsCustomFileStreamOperable(const TCHAR* fileName, const TCHAR* streamName) const
{
	DbgAssert(fileName);
	DbgAssert(streamName);
	if (fileName == nullptr || streamName == nullptr)
		return false;

	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}

	IStorage* pFileIStorage = nullptr;
	IStorage* pIStorage = OpenStorageForRead(fileName, pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);
	if (pFileIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_READ) + fileName;
		throw MAXException(msg);
	}
	if (pIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_READ) + streamName;
		throw MAXException(msg);
	}

	IStream* pIStream = OpenStreamForRead(pIStorage, streamName);
	if (pIStream == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_READ) + streamName;
		throw MAXException(msg);
	}
	// define unique_ptr for guaranteed cleanup of pIStream
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);

	return ValidateStream(pIStream);
}

bool CustomFileStream::DeleteStream(const TCHAR* fileName, const TCHAR* streamName) const
{
	DbgAssert(fileName);
	DbgAssert(streamName);
	if (fileName == nullptr || streamName == nullptr)
		return false;

	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}

	kDeleteStream_result result = DeleteFileStream(fileName, streamName);
	switch (result) 
	{
	case kDeleteStream_result::kFileStorageOpenFailed:
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_WRITE) + fileName;
		throw MAXException(msg);
	}
	case kDeleteStream_result::kCustomDataStorageOpenFailed:
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_WRITE) + streamName;
		throw MAXException(msg);
	}
	case kDeleteStream_result::kStreamDoesNotExist:
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_READ) + streamName;
		throw MAXException(msg);
	}
	case kDeleteStream_result::kDestroyElementFailed:
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_PERFORMING_OLE_OPERATION) + _T("DestroyElement");
		throw MAXException(msg);
	}
	}
	return true;
}

TSTR CustomFileStream::ReadStream(const TCHAR* fileName, const TCHAR* streamName) const
{
	DbgAssert(fileName);
	DbgAssert(streamName);
	if (fileName == nullptr || streamName == nullptr)
		return _T("");

	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}

	IStorage* pFileIStorage = nullptr;
	IStorage* pIStorage = OpenStorageForRead(fileName, pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);
	if (pFileIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_READ) + fileName;
		throw MAXException(msg);
	}
	if (pIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_READ) + streamName;
		throw MAXException(msg);
	}

	IStream* pIStream = OpenStreamForRead(pIStorage, streamName);
	if (pIStream == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_READ) + streamName;
		throw MAXException(msg);
	}
	// define unique_ptr for guaranteed cleanup of pIStream
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);

	if (!ValidateStream(pIStream))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_CUSTOMFILESTREAM_CREATED_STREAM) + streamName;
		throw MAXException(msg);
	}

	std::wstring content;
	bool res = ReadStreamContents(pIStream, content);
	if (!res)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_PERFORMING_OLE_OPERATION) + _T("Read");
		throw MAXException(msg);
	}

	return content.c_str();
}

#define CREATE_STRM_FLAGS (STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE)

bool CustomFileStream::WriteStream(const TCHAR * fileName, const TCHAR* streamName, const TCHAR* content, bool persistent, bool noLoadOnSceneLoad, bool append) const
{
	DbgAssert(fileName);
	DbgAssert(streamName);
	if (fileName == nullptr || streamName == nullptr)
		return false;

	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}

	if (_tcslen(streamName) > kMaxStreamNameLength)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NAME_TOO_LONG) + streamName;
		throw MAXException(msg);
	}

	if (_tcslen(streamName) == 0)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NAME_EMPTY);
		throw MAXException(msg);
	}

	IStorage* pFileIStorage = nullptr;
	IStorage* pIStorage = OpenStorageForWrite(fileName, pFileIStorage);
	if (pIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_WRITE) + fileName;
		throw MAXException(msg);
	}
	// define unique_ptr for guaranteed cleanup of pIStorage
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);

	DWORD privateFlags = 0;
	if (persistent)
		privateFlags |= kPersistentStream;
	if (noLoadOnSceneLoad)
		privateFlags |= kNoLoadOnSceneLoad;

	IStream* pIStream = OpenStreamForWrite(pIStorage, streamName, privateFlags);
	if (pIStream == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_WRITE) + streamName;
		throw MAXException(msg);
	}
	// define unique_ptr for guaranteed cleanup of pIStream
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);

	if (!ValidateStream(pIStream))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_CUSTOMFILESTREAM_CREATED_STREAM) + streamName;
		throw MAXException(msg);
	}

	if (content == nullptr)
		content = _T("");

	if (append)
	{
		// make sure contents not an array
		wchar_t lastChar = L'\0';
		if (GetLastCharacterOfContent(pIStream, lastChar))
		{
			if (lastChar == L'\0')
			{
				TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_APPENDING_STRING_TO_ARRAY_STREAM) + streamName;
				throw MAXException(msg);
			}
		}

		LARGE_INTEGER zero = { 0, 0 };
		pIStream->Seek(zero, STREAM_SEEK_END, nullptr);
	}
	else
	{
		std::wstring data = content;
		return WriteStreamContents(pIStream, data);
	}

	ULONG nb;
	HRESULT hres = pIStream->Write(content, (ULONG)(_tcslen(content) * sizeof(TCHAR)), &nb);
	if (FAILED(hres))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_PERFORMING_OLE_OPERATION) + _T("Write");
		throw MAXException(msg);
	}
	return true;
}

Tab<TSTR*> CustomFileStream::fpReadStreamArray(const TCHAR* fileName, const TCHAR* streamName) const
{
	MaxSDK::Array<TSTR> resultArray;
	ReadStreamArray(fileName, streamName, resultArray);
	Tab<TSTR*> res;
	for (int i = 0; i < resultArray.length(); i++)
	{
		TSTR* string = new TSTR(resultArray[i]);
		res.Append(1, &string);
	}
	return res;
}

void CustomFileStream::ReadStreamArray(const TCHAR* fileName, const TCHAR* streamName, MaxSDK::Array<TSTR>& contentArray) const
{
	contentArray.setLengthUsed(0);
	DbgAssert(fileName);
	DbgAssert(streamName);
	if (fileName == nullptr || streamName == nullptr)
		return;

	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}

	IStorage* pFileIStorage = nullptr;
	IStorage* pIStorage = OpenStorageForRead(fileName, pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);
	if (pFileIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_READ) + fileName;
		throw MAXException(msg);
	}
	if (pIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_READ) + streamName;
		throw MAXException(msg);
	}

	IStream* pIStream = OpenStreamForRead(pIStorage, streamName);
	if (pIStream == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_READ) + streamName;
		throw MAXException(msg);
	}
	// define unique_ptr for guaranteed cleanup of pIStream
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);

	if (!ValidateStream(pIStream))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_CUSTOMFILESTREAM_CREATED_STREAM) + streamName;
		throw MAXException(msg);
	}

	std::vector<std::wstring> content;
	bool res = ReadStreamContents(pIStream, content);
	if (!res)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_PERFORMING_OLE_OPERATION) + _T("Read");
		throw MAXException(msg);
	}

	for (auto i : content)
		contentArray.append(i.c_str());
}

bool CustomFileStream::WriteStreamArray(const TCHAR * fileName, const TCHAR * streamName, const Tab<const TCHAR*>& contentArray, bool persistent, bool noLoadOnSceneLoad, bool append) const
{
	DbgAssert(fileName);
	DbgAssert(streamName);
	if (fileName == nullptr || streamName == nullptr)
		return false;

	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}

	if (_tcslen(streamName) > kMaxStreamNameLength)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NAME_TOO_LONG) + streamName;
		throw MAXException(msg);
	}

	if (_tcslen(streamName) == 0)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NAME_EMPTY);
		throw MAXException(msg);
	}

	IStorage* pFileIStorage = nullptr;
	IStorage* pIStorage = OpenStorageForWrite(fileName, pFileIStorage);
	if (pIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_WRITE) + fileName;
		throw MAXException(msg);
	}
	// define unique_ptr for guaranteed cleanup of pIStorage
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);

	DWORD privateFlags = 0;
	if (persistent)
		privateFlags |= kPersistentStream;
	if (noLoadOnSceneLoad)
		privateFlags |= kNoLoadOnSceneLoad;

	IStream* pIStream = OpenStreamForWrite(pIStorage, streamName, privateFlags);
	if (pIStream == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_WRITE) + streamName;
		throw MAXException(msg);
	}
	// define unique_ptr for guaranteed cleanup of pIStream
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);

	if (!ValidateStream(pIStream))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_CUSTOMFILESTREAM_CREATED_STREAM) + streamName;
		throw MAXException(msg);
	}

	if (append)
	{
		// make sure contents not a string
		wchar_t lastChar = L'\0';
		if (GetLastCharacterOfContent(pIStream, lastChar))
		{
			if (lastChar != L'\0')
			{
				TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_APPENDING_ARRAY_TO_STRING_STREAM) + streamName;
				throw MAXException(msg);
			}
		}

		LARGE_INTEGER zero = { 0, 0 };
		pIStream->Seek(zero, STREAM_SEEK_END, nullptr);
	}
	else
	{
		// already immediately after header, set new size
		ULARGE_INTEGER uzero = { kCustomFileStreamHeaderSize, 0 };
		pIStream->SetSize(uzero);
	}

	if (!append)
	{
		// for testing WriteStreamContents with std::vector<std::wstring> arg
		std::vector<std::wstring> data;
		for (int i = 0; i < contentArray.Count(); i++)
			data.emplace_back(contentArray[i]);
		return WriteStreamContents(pIStream, data);

	}
	else
	{
		ULONG nb;
		HRESULT hres = S_OK;
		for (int i = 0; i < contentArray.Count() && SUCCEEDED(hres); i++)
		{
			const TCHAR* content = contentArray[i];
			if (content == nullptr)
				content = _T("");
			hres = pIStream->Write(content, (ULONG)((_tcslen(content) + 1) * sizeof(TCHAR)), &nb); // writing null terminator
		}
		if (FAILED(hres))
		{
			TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_PERFORMING_OLE_OPERATION) + _T("Write");
			throw MAXException(msg);
		}
	}
	return true;
}

DWORD CustomFileStream::GetStreamFlags(const TCHAR* fileName, const TCHAR* streamName, bool getPrivate) const
{
	DbgAssert(fileName);
	DbgAssert(streamName);
	if (fileName == nullptr || streamName == nullptr)
		return 0;

	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}

	IStorage* pFileIStorage = nullptr;
	IStorage* pIStorage = OpenStorageForRead(fileName, pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);
	if (pFileIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_READ) + fileName;
		throw MAXException(msg);
	}
	if (pIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_READ) + streamName;
		throw MAXException(msg);
	}

	IStream* pIStream = OpenStreamForRead(pIStorage, streamName);
	if (pIStream == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_READ) + streamName;
		throw MAXException(msg);
	}
	// define unique_ptr for guaranteed cleanup of pIStream
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);

	CustomFileStreamHeader l_CustomFileStreamHeader;
	if (!ValidateStream(pIStream, &l_CustomFileStreamHeader))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_CUSTOMFILESTREAM_CREATED_STREAM) + streamName;
		throw MAXException(msg);
	}

	return (getPrivate ? l_CustomFileStreamHeader.mPrivateFlags : l_CustomFileStreamHeader.mPublicFlags);
}

bool CustomFileStream::SetStreamFlags(const TCHAR* fileName, const TCHAR* streamName, DWORD flags, bool setPrivate) const
{
	DbgAssert(fileName);
	DbgAssert(streamName);
	if (fileName == nullptr || streamName == nullptr)
		return false;

	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}

	IStorage* pFileIStorage = nullptr;
	IStorage* pIStorage = OpenStorageForWrite(fileName, pFileIStorage);
	if (pIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_WRITE) + fileName;
		throw MAXException(msg);
	}
	// define unique_ptr for guaranteed cleanup of pIStorage
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);

	// only set flags on existing streams
	IStream* pIStream = OpenStreamForRead(pIStorage, streamName);
	if (pIStream == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_SET_FLAGS_ON_NONEXISTANT_STREAM) + streamName;
		throw MAXException(msg);
	}
	CustomFileStreamHeader l_CustomFileStreamHeader;
	bool validStream = ValidateStream(pIStream, &l_CustomFileStreamHeader);
	pIStream->Release();

	if (!validStream)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_CUSTOMFILESTREAM_CREATED_STREAM) + streamName;
		throw MAXException(msg);
	}

	pIStream = OpenStreamForWrite(pIStorage, streamName, 0, 0);
	if (pIStream == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_WRITE) + streamName;
		throw MAXException(msg);
	}
	// define unique_ptr for guaranteed cleanup of pIStream
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);

	if (setPrivate)
		l_CustomFileStreamHeader.mPrivateFlags = flags;
	else
		l_CustomFileStreamHeader.mPublicFlags = flags;

	LARGE_INTEGER zero = { 0, 0 };
	pIStream->Seek(zero, STREAM_SEEK_SET, nullptr);
	ULONG nb;
	HRESULT hr = pIStream->Write(&l_CustomFileStreamHeader, sizeof(l_CustomFileStreamHeader), &nb);
	if (FAILED(hr))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_PERFORMING_OLE_OPERATION) + _T("Write");
		throw MAXException(msg);
	}

	return SUCCEEDED(hr);
}

bool CustomFileStream::IsStreamDataAnArray(const TCHAR* fileName, const TCHAR* streamName) const
{
	DbgAssert(fileName);
	DbgAssert(streamName);
	if (fileName == nullptr || streamName == nullptr)
		return false;

	if (!DoesFileExist(fileName, false))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_FILE_NOT_FOUND) + fileName;
		throw MAXException(msg);
	}

	IStorage* pFileIStorage = nullptr;
	IStorage* pIStorage = OpenStorageForRead(fileName, pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);
	if (pFileIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STORAGE_NOT_OPENED_FOR_READ) + fileName;
		throw MAXException(msg);
	}
	if (pIStorage == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_READ) + streamName;
		throw MAXException(msg);
	}

	IStream* pIStream = OpenStreamForRead(pIStorage, streamName);
	if (pIStream == nullptr)
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_OPENED_FOR_READ) + streamName;
		throw MAXException(msg);
	}
	// define unique_ptr for guaranteed cleanup of pIStream
	std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);

	if (!ValidateStream(pIStream))
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_STREAM_NOT_CUSTOMFILESTREAM_CREATED_STREAM) + streamName;
		throw MAXException(msg);
	}

	return MaxSDK::CustomFileStreamAPI::IsStreamContentAnArray(pIStream);
}

// Get the last character of the stream content. If no content, return false otherwise return true
bool CustomFileStream::GetLastCharacterOfContent(IStream* pIStream, wchar_t& theChar) const
{
	DbgAssert(pIStream);
	if (pIStream == nullptr)
		return false;

	kGetLastCharacterOfContent_result result = MaxSDK::CustomFileStreamAPI::GetLastCharacterOfContent(pIStream, theChar);
	switch (result)
	{
	case kGetLastCharacterOfContent_result::kStreamSeekFailure:
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_PERFORMING_OLE_OPERATION) + _T("Seek");
		throw MAXException(msg);
	}
	case kGetLastCharacterOfContent_result::kStreamReadFailure:
	{
		TSTR msg = MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_PERFORMING_OLE_OPERATION) + _T("Read");
		throw MAXException(msg);
	}
	}
	return result == kGetLastCharacterOfContent_result::kOk;
}

CustomFileStream::~CustomFileStream()
{

}
