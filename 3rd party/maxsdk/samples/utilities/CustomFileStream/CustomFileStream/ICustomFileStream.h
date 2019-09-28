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

#include <ifnpub.h>
#include <GetCOREInterface.h>
#include <containers/Array.h>

//! CustomFileStream interface ID
#define CUSTOMFILESTREAM_INTERFACE   Interface_ID(0x95E98592, 0x2E10D178)

/*! \brief This interface is used to support OLE structured storage streams containing user specified string data in 3ds Max scene files or
other OLE Structured Storage based files. Note that these methods will throw MAXExceptions on error conditions.
*/
class ICustomFileStream : public FPStaticInterface
{
public:
	/*! \brief Read the CustomFileStream names present in a file
	\param fileName The file name.
	\param streamNames [out] The stream names.
	*/
	virtual void GetStreamNames(const TCHAR* fileName, MaxSDK::Array<TSTR>& streamNames) const = 0;
	/*! \brief Test to see if the IStorage containing CustomFileStreams is present in a file
	\param fileName The file name.
	\return true if the IStorage is present.
	*/
	virtual bool DoesStorageExist(const TCHAR* fileName) const = 0;
	/*! \brief Test to see if the specified CustomFileStream name is present in a file
	\param fileName The file name.
	\param streamName The stream name.
	\return true if the stream is present.
	*/
	virtual bool DoesStreamExist(const TCHAR* fileName, const TCHAR* streamName) const = 0;
	/*! \brief Test to see if the specified CustomFileStream name was created via CustomFileStream methods. 
	Verifies the stream is long enough to contain the stream header data, that the header data was 
	read successfully, and the version number is equal to or less than the current version number.
	\param fileName The file name.
	\param streamName The stream name.
	\return true if the stream can be operated on by the other methods.
	*/
	virtual bool IsCustomFileStreamOperable(const TCHAR* fileName, const TCHAR* streamName) const = 0;
	/*! \brief Delete the specified CustomFileStream from the file
	\param fileName The file name.
	\param streamName The stream name.
	\return true if the stream was deleted.
	*/
	virtual bool DeleteStream(const TCHAR* fileName, const TCHAR* streamName) const = 0;
	/*! \brief Read the string data from the specified CustomFileStream in a file
	\param fileName The file name.
	\param streamName The stream name.
	\return The stream contents. If stream contained an array of strings, returns only the first element.
	*/
	virtual TSTR ReadStream(const TCHAR* fileName, const TCHAR* streamName) const = 0;
	/*! \brief Write string data to the specified CustomFileStream in a file, creating stream if necessary
	\param fileName The file name.
	\param streamName The stream name.
	\param content The string content to write
	\param persistent If CustomFileStream is created, whether the stream is flagged as persistent.
	\param noLoadOnSceneLoad If CustomFileStream is created, whether the stream is flagged as not to be loaded when scene file is loaded.
	\param append If true, append content string to existing stream content if present
	\return true if the write was successful.
	*/
	virtual bool WriteStream(const TCHAR * fileName, const TCHAR* streamName, const TCHAR* content, bool persistent, bool noLoadOnSceneLoad, bool append) const = 0;
	/*! \brief Read the string data as an string array from the specified CustomFileStream in a file
	\param fileName The file name.
	\param streamName The stream name.
	\param contentArray [out] The stream contents as an array of strings. If the stream contents is a single string, array will have one element containing that string.
	*/
	virtual void ReadStreamArray(const TCHAR* fileName, const TCHAR* streamName, MaxSDK::Array<TSTR>& contentArray) const = 0;
	/*! \brief Write string data to the specified CustomFileStream in a file, creating stream if necessary
	\param fileName The file name.
	\param streamName The stream name.
	\param contentArray The array of string content to write
	\param persistent If CustomFileStream is created, whether the stream is flagged as persistent.
	\param noLoadOnSceneLoad If CustomFileStream is created, whether the stream is flagged as not to be loaded when scene file is loaded.
	\param append If true, append content string to existing stream content if present
	\return true if the write was successful.
	*/
	virtual bool WriteStreamArray(const TCHAR * fileName, const TCHAR * streamName, const Tab<const TCHAR*>& contentArray, bool persistent, bool noLoadOnSceneLoad, bool append) const = 0;
	/*! \brief Read the public or private flags from the specified CustomFileStream in a file
	\param fileName The file name.
	\param streamName The stream name.
	\param getPrivate If true, get the private flags, else get the public flags.
	\return The the public or private flags.
	*/
	virtual DWORD GetStreamFlags(const TCHAR* fileName, const TCHAR* streamName, bool getPrivate) const = 0;
	/*! \brief Set the public or private flags in the specified CustomFileStream in a file
	\param fileName The file name.
	\param streamName The stream name.
	\param flags - the flags value to set
	\param setPrivate If true, set the private flags, else set the public flags.
	\return The the public or private flags.
	*/
	virtual bool SetStreamFlags(const TCHAR* fileName, const TCHAR* streamName, DWORD flags, bool setPrivate) const = 0;
	/*! \brief Test to see if the specified CustomFileStream stream contains string array content.
	\param fileName The file name.
	\param streamName The stream name.
	\return true if the stream contains string array content.
	*/
	virtual bool IsStreamDataAnArray(const TCHAR* fileName, const TCHAR* streamName) const = 0;
	/*! \brief Get the last character of the content in an OLE IStream. Used for determining whether the stream contains string array content
	\param pIStream The OLE IStream.
	\param theChar [out] The last character of the content in the OLE IStream.
	\return true if the stream contains content and theChar was set.
	*/
	virtual bool GetLastCharacterOfContent(IStream* pIStream, wchar_t& theChar) const = 0;
};

ICustomFileStream* GetICustomFileStream() { return (ICustomFileStream*)GetCOREInterface(CUSTOMFILESTREAM_INTERFACE); }