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
#include "ICustomFileStream.h"

class CustomFileStream : public ICustomFileStream
{
public:
	enum CustomSceneStreamFps
	{
		kGetStreamNames,
		kDoesStorageExist,
		kDoesStreamExist,
		kIsCustomFileStreamOperable,
		kDeleteStream,
		kReadStream,
		kWriteStream,
		kReadStreamArray,
		kWriteStreamArray,
		kGetStreamFlags,
		kSetStreamFlags,
		kIsStreamDataAnArray
	};

	DECLARE_DESCRIPTOR(CustomFileStream)
	BEGIN_FUNCTION_MAP
		FN_1(kGetStreamNames, TYPE_TSTR_TAB_BV, fpGetStreamNames, TYPE_FILENAME);
		FN_1(kDoesStorageExist, TYPE_bool, DoesStorageExist, TYPE_FILENAME);
		FN_2(kDoesStreamExist, TYPE_bool, DoesStreamExist, TYPE_FILENAME, TYPE_STRING);
		FN_2(kIsCustomFileStreamOperable, TYPE_bool, IsCustomFileStreamOperable, TYPE_FILENAME, TYPE_STRING);
		FN_2(kDeleteStream, TYPE_bool, DeleteStream, TYPE_FILENAME, TYPE_STRING);
		FN_2(kReadStream, TYPE_TSTR_BV, ReadStream, TYPE_FILENAME, TYPE_STRING);
		FN_6(kWriteStream, TYPE_bool, WriteStream, TYPE_FILENAME, TYPE_STRING, TYPE_STRING, TYPE_bool, TYPE_bool, TYPE_bool);
		FN_2(kReadStreamArray, TYPE_TSTR_TAB_BV, fpReadStreamArray, TYPE_FILENAME, TYPE_STRING);
		FN_6(kWriteStreamArray, TYPE_bool, WriteStreamArray, TYPE_FILENAME, TYPE_STRING, TYPE_STRING_TAB_BV, TYPE_bool, TYPE_bool, TYPE_bool);
		FN_3(kGetStreamFlags, TYPE_DWORD, GetStreamFlags, TYPE_FILENAME, TYPE_STRING, TYPE_bool);
		FN_4(kSetStreamFlags, TYPE_bool, SetStreamFlags, TYPE_FILENAME, TYPE_STRING, TYPE_DWORD, TYPE_bool);
		FN_2(kIsStreamDataAnArray, TYPE_bool, IsStreamDataAnArray, TYPE_FILENAME, TYPE_STRING);
	END_FUNCTION_MAP

	virtual void GetStreamNames(const TCHAR* fileName, MaxSDK::Array<TSTR>& streamNames) const override;
	virtual bool DoesStorageExist(const TCHAR* fileName) const override;
	virtual bool DoesStreamExist(const TCHAR* fileName, const TCHAR* streamName) const override;
	virtual bool IsCustomFileStreamOperable(const TCHAR* fileName, const TCHAR* streamName) const override;
	virtual bool DeleteStream(const TCHAR* fileName, const TCHAR* streamName) const override;
	virtual TSTR ReadStream(const TCHAR* fileName, const TCHAR* streamName) const override;
	virtual bool WriteStream(const TCHAR * fileName, const TCHAR* streamName, const TCHAR* content, bool persistent, bool loadOnLoad, bool append) const override;
	virtual void ReadStreamArray(const TCHAR* fileName, const TCHAR* streamName, MaxSDK::Array<TSTR>& contentArray) const override;
	virtual bool WriteStreamArray(const TCHAR * fileName, const TCHAR * streamName, const Tab<const TCHAR*>& contentArray, bool persistent, bool noLoadOnSceneLoad, bool append) const override;
	virtual DWORD GetStreamFlags(const TCHAR* fileName, const TCHAR* streamName, bool getPrivate) const override;
	virtual bool SetStreamFlags(const TCHAR* fileName, const TCHAR* streamName, DWORD flags, bool setPrivate) const override;
	virtual bool IsStreamDataAnArray(const TCHAR* fileName, const TCHAR* streamName) const override;
	virtual bool GetLastCharacterOfContent(IStream* pIStream, wchar_t& theChar) const override;

	// FPS wrapper method for GetStreamNames
	Tab<TSTR*> fpGetStreamNames(const TCHAR* fileName) const;
	// FPS wrapper method for ReadStreamArray
	Tab<TSTR*> fpReadStreamArray(const TCHAR* fileName, const TCHAR* streamName) const;

	~CustomFileStream();
};
