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
#include <CustomFileStreamAPI\CustomFileStreamAPI.h>
#include <assert.h>


namespace MaxSDK
{
	namespace CustomFileStreamAPI
	{
		static const DWORD kCustomFileStreamHeader_size = sizeof(CustomFileStreamHeader);

#define CREATE_STRM_FLAGS (STGM_CREATE | STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE)
#define OPEN_READ_STRM_FLAGS (STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE)
#define OPEN__WRITE_STRM_FLAGS (STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE)

		bool DoesCustomFileStreamStorageExist(const TCHAR* fileName)
		{
			IStorage* pFileIStorage = nullptr;
			IStorage* pIStorage = OpenStorageForRead(fileName, pFileIStorage);
			std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
			std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);
			return pIStorage != nullptr;
		}

		// This opens the file as an OLE structured storage file with read/write access
		IStorage* OpenStorageForWrite(const TCHAR* fileName, IStorage* & pFileIStorage)
		{
			pFileIStorage = nullptr;
			assert(fileName);
			if (!fileName)
				return nullptr;

			STGOPTIONS StgOptions = { 0 };
			StgOptions.usVersion = 1;		/* Win2k+ */
			StgOptions.ulSectorSize = 4096;	/* 512 limits us to 2GB */
			DWORD mode = (STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE);
			HRESULT hres = StgOpenStorageEx(fileName, mode, STGFMT_DOCFILE, 0, &StgOptions, nullptr, IID_IStorage, (void **)&pFileIStorage);
			if (hres != S_OK && pFileIStorage)
			{
				assert(false);
				pFileIStorage->Release();
				pFileIStorage = nullptr;
			}
			if (pFileIStorage == nullptr)
				return nullptr;

			IStorage* pIStorage2 = nullptr;
			hres = pFileIStorage->OpenStorage(kCustomFileStreamStorageName, nullptr, OPEN__WRITE_STRM_FLAGS, nullptr, 0, &pIStorage2);
			if (hres != S_OK && pIStorage2)
			{
				assert(false);
				pIStorage2->Release();
				pIStorage2 = nullptr;
			}
			if (hres == STG_E_FILENOTFOUND)
			{
				hres = pFileIStorage->CreateStorage(kCustomFileStreamStorageName, OPEN__WRITE_STRM_FLAGS, 0, 0, &pIStorage2);
				if (hres != S_OK && pIStorage2)
				{
					assert(false);
					pIStorage2->Release();
					pIStorage2 = nullptr;
				}
			}
			if (pIStorage2 == nullptr && pFileIStorage != nullptr)
			{
				pFileIStorage->Release();
				pFileIStorage = nullptr;
			}
			return pIStorage2;
		}

		// This opens the file as an OLE structured storage file with read access
		IStorage* OpenStorageForRead(const TCHAR* fileName, IStorage* & pFileIStorage)
		{
			pFileIStorage = nullptr;
			assert(fileName);
			if (!fileName)
				return nullptr;

			STGOPTIONS StgOptions = { 0 };
			StgOptions.usVersion = 1;		/* Win2k+ */
			StgOptions.ulSectorSize = 4096;	/* 512 limits us to 2GB */
			DWORD mode = (STGM_DIRECT | STGM_READ | STGM_SHARE_DENY_WRITE);
			HRESULT hres = StgOpenStorageEx(fileName, mode, STGFMT_DOCFILE, 0, &StgOptions, nullptr, IID_IStorage, (void **)&pFileIStorage);
			if (hres != S_OK && pFileIStorage)
			{
				assert(false);
				pFileIStorage->Release();
				pFileIStorage = nullptr;
			}
			if (pFileIStorage == nullptr)
				return nullptr;

			IStorage* pIStorage2 = nullptr;
			hres = pFileIStorage->OpenStorage(kCustomFileStreamStorageName, nullptr, OPEN_READ_STRM_FLAGS, nullptr, 0, &pIStorage2);
			if (hres != S_OK && pIStorage2)
			{
				assert(false);
				pIStorage2->Release();
				pIStorage2 = nullptr;
			}
			if (pIStorage2 == nullptr && pFileIStorage != nullptr)
			{
				pFileIStorage->Release();
				pFileIStorage = nullptr;
			}
			return pIStorage2;
		}

		// This opens the stream with read/write access, creating it if not present
		IStream* OpenStreamForWrite(IStorage* pIStorage, const TCHAR* streamName, DWORD privateFlags, DWORD publicFlags, WORD version)
		{
			assert(pIStorage);
			assert(streamName);
			if (!pIStorage || !streamName)
				return nullptr;

			bool isNewStream = false;
			LPSTREAM pIStream;
			DWORD mode = (STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE);
			HRESULT hres = pIStorage->OpenStream(streamName, nullptr, mode, 0, &pIStream);
			if (hres != S_OK || pIStream == nullptr)
			{
				hres = pIStorage->CreateStream(streamName, CREATE_STRM_FLAGS, 0, 0, &pIStream);
				isNewStream = true;
			}
			if (hres != S_OK && pIStream)
			{
				assert(false);
				pIStream->Release();
				pIStream = nullptr;
				isNewStream = false;
			}

			if (isNewStream && pIStream)
			{
				ULONG nb;
				// Add the header.
				CustomFileStreamHeader l_CustomFileStreamHeader; // use default values
				l_CustomFileStreamHeader.mVersion = version;
				l_CustomFileStreamHeader.mPrivateFlags = privateFlags;
				l_CustomFileStreamHeader.mPublicFlags = publicFlags;
				hres = pIStream->Write(&l_CustomFileStreamHeader, sizeof(l_CustomFileStreamHeader), &nb);
			}
			if (hres != S_OK && pIStream)
			{
				assert(false);
				pIStream->Release();
				pIStream = nullptr;
			}

			return pIStream;
		}

		// This opens the stream with read access
		IStream* OpenStreamForRead(IStorage* pIStorage, const TCHAR* streamName)
		{
			assert(pIStorage);
			assert(streamName);
			if (!pIStorage || !streamName)
				return nullptr;

			LPSTREAM pIStream;
			DWORD mode = (STGM_READ | STGM_SHARE_EXCLUSIVE);
			HRESULT hres = pIStorage->OpenStream(streamName, nullptr, mode, 0, &pIStream);
			if (hres != S_OK && pIStream)
			{
				assert(false);
				pIStream->Release();
				pIStream = nullptr;
			}
			return pIStream;
		}

		// validates that stream was created via CustomFileStream methods, captures
		// private and public flag values if wanted, leaves IStream immediately past header.
		bool ValidateStream(IStream* pIStream, CustomFileStreamHeader *stream_header)
		{
			assert(pIStream);
			if (pIStream == nullptr)
				return false;
			// get the length of the stream
			STATSTG statstg;
			memset(&statstg, 0, sizeof(statstg));
			pIStream->Stat(&statstg, STATFLAG_NONAME);
			unsigned __int64 nBytes = statstg.cbSize.QuadPart;
			if (nBytes < kCustomFileStreamHeader_size)
				return false;
			LARGE_INTEGER zero = { 0, 0 };
			pIStream->Seek(zero, STREAM_SEEK_SET, nullptr);
			CustomFileStreamHeader l_CustomFileStreamHeader;
			ULONG nb;
			HRESULT hr = pIStream->Read(&l_CustomFileStreamHeader, sizeof(l_CustomFileStreamHeader), &nb);
			if (FAILED(hr) || nb != sizeof(l_CustomFileStreamHeader))
				return false;
			if (stream_header)
				*stream_header = l_CustomFileStreamHeader;
			return true;
		}

		unsigned __int64 GetStreamContentByteCount(IStream* pIStream)
		{
			assert(pIStream);
			if (pIStream == nullptr)
				return 0;

			// get the length of the stream
			STATSTG statstg;
			memset(&statstg, 0, sizeof(statstg));
			pIStream->Stat(&statstg, STATFLAG_NONAME);
			unsigned __int64 nBytes = statstg.cbSize.QuadPart;
			nBytes -= kCustomFileStreamHeader_size; // remove header size
			return nBytes;
		}

		bool SeekToStartOfStreamContent(IStream* pIStream)
		{
			assert(pIStream);
			if (pIStream == nullptr)
				return false;
			LARGE_INTEGER zero = { kCustomFileStreamHeader_size, 0 };
			HRESULT hr = pIStream->Seek(zero, STREAM_SEEK_SET, nullptr);
			return SUCCEEDED(hr);
		}

		// not exposed in public SDK
		bool ReadStreamContents(IStream* pIStream, std::vector<wchar_t>& content)
		{
			assert(pIStream);
			if (pIStream == nullptr)
				return false;

			if (!SeekToStartOfStreamContent(pIStream))
				return false;

			unsigned __int64 nBytes = GetStreamContentByteCount(pIStream);
			size_t nChars = (nBytes / sizeof(wchar_t)) + 1; // +1 for null char
			content.resize(nChars);

			ULONG nb;
			HRESULT hr = pIStream->Read(&content[0], nBytes, &nb);
			return SUCCEEDED(hr);
		}

		bool ReadStreamContents(IStream* pIStream, std::wstring& content)
		{
			std::vector<wchar_t> stream_content;
			bool res = ReadStreamContents(pIStream, stream_content);
			if (res)
				content = stream_content.data();
			return res;
		}

		bool ReadStreamContents(IStream* pIStream, std::vector<std::wstring>& content)
		{
			std::vector<wchar_t> stream_content;
			bool res = ReadStreamContents(pIStream, stream_content);
			if (res)
			{
				const wchar_t* string = stream_content.data(); // guaranteed to contain at least a null char
				size_t nChars = stream_content.size();
				assert(nChars != 0);
				if (nChars == 0)
					return false;
				nChars -= 1; // don't include the final null char
				size_t len = wcslen(string);
				// handle case of just a string
				if (nChars == len)
				{
					content.emplace_back(string);
				}
				else
				{
					const wchar_t* end_of_strings = string + nChars;
					while (true)
					{
						content.emplace_back(string);
						size_t len = wcslen(string);
						string += len + 1; // plus null char
						if (string >= end_of_strings)
							break;
					}
				}
			}
			return res;
		}

		bool WriteStreamContents(IStream* pIStream, const std::wstring& content)
		{
			assert(pIStream);
			if (pIStream == nullptr)
				return false;

			ULARGE_INTEGER uzero = { kCustomFileStreamHeader_size, 0 };
			pIStream->SetSize(uzero);
			LARGE_INTEGER zero = { 0, 0 };
			pIStream->Seek(zero, STREAM_SEEK_END, nullptr);
			size_t nBytes = content.size() * sizeof(wchar_t);
			if (nBytes == 0)
				return true;
			ULONG nb;
			HRESULT hres = pIStream->Write(content.data(), (ULONG)nBytes, &nb);
			return SUCCEEDED(hres);
		}

		bool WriteStreamContents(IStream* pIStream, const std::vector<std::wstring>& content)
		{
			assert(pIStream);
			if (pIStream == nullptr)
				return false;

			ULARGE_INTEGER uzero = { kCustomFileStreamHeader_size, 0 };
			pIStream->SetSize(uzero);
			LARGE_INTEGER zero = { 0, 0 };
			pIStream->Seek(zero, STREAM_SEEK_END, nullptr);

			ULONG nb;
			HRESULT hres = S_OK;
			for (auto i : content)
			{
				hres = pIStream->Write(i.c_str(), (ULONG)((i.length() + 1) * sizeof(wchar_t)), &nb); // writing null terminator
				if (FAILED(hres))
					return false;
			}
			return true;
		}

		bool GetStreamNames(IStorage* pIStorage, std::vector<std::wstring>& streamNames)
		{
			assert(pIStorage);
			if (pIStorage == nullptr)
				return false;

			STATSTG statstg;
			memset(&statstg, 0, sizeof(statstg));
			// Get an enumerator for this storage.
			IEnumSTATSTG *penum = nullptr;
			HRESULT hres = pIStorage->EnumElements(0, nullptr, 0, &penum);
			// define unique_ptr for guaranteed cleanup of penum
			std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIEnumAutoPtr(penum);
			if (SUCCEEDED(hres))
			{
				hres = penum->Next(1, &statstg, 0);
				while (S_OK == hres)
				{
					// from MS sample code: If the first character of name is the '\005' reserved 
					// character, it is a stream/storage property set. Ignore
					if (STGTY_STREAM == statstg.type && L'\005' != statstg.pwcsName[0])
					{
						IStream* pIStream = OpenStreamForRead(pIStorage, statstg.pwcsName);
						std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStreamAutoPtr(pIStream);
						if (ValidateStream(pIStream))
						{
							streamNames.emplace_back(statstg.pwcsName);
						}
					}
					CoTaskMemFree(statstg.pwcsName);
					statstg.pwcsName = nullptr;
					hres = penum->Next(1, &statstg, 0);
				}
			}
			if (statstg.pwcsName)
				CoTaskMemFree(statstg.pwcsName);
			return SUCCEEDED(hres);
		}

		kDeleteStream_result DeleteFileStream(const TCHAR* fileName, const TCHAR* streamName)
		{
			if (fileName == nullptr || streamName == nullptr)
				return kDeleteStream_result::kBadArgument;

			IStorage* pFileIStorage = nullptr;
			IStorage* pIStorage = OpenStorageForWrite(fileName, pFileIStorage);
			std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr1(pFileIStorage);
			std::unique_ptr<IUnknown, IUnknownDestructorPolicy> pIStorageAutoPtr2(pIStorage);
			if (pFileIStorage == nullptr)
				return kDeleteStream_result::kFileStorageOpenFailed;
			if (pIStorage == nullptr)
				return kDeleteStream_result::kCustomDataStorageOpenFailed;

			{
				// create scope just for testing to make sure stream exists
				IStream* pIStream = OpenStreamForRead(pIStorage, streamName);
				if (pIStream == nullptr)
					return kDeleteStream_result::kStreamDoesNotExist;
				pIStream->Release();
			}

			// Delete the stream
			HRESULT hres = pIStorage->DestroyElement(streamName);
			if (FAILED(hres))
				return kDeleteStream_result::kDestroyElementFailed;
			return kDeleteStream_result::kOk;
		}

		kGetLastCharacterOfContent_result GetLastCharacterOfContent(IStream* pIStream, wchar_t& theChar)
		{
			if (pIStream == nullptr)
				return kGetLastCharacterOfContent_result::kBadArgument;

			unsigned __int64 nBytes = GetStreamContentByteCount(pIStream);
			if (nBytes)
			{
				LARGE_INTEGER offset;
				offset.QuadPart = -static_cast<int>(sizeof(wchar_t));
				HRESULT hr = pIStream->Seek(offset, STREAM_SEEK_END, nullptr);
				if (FAILED(hr))
					return kGetLastCharacterOfContent_result::kStreamSeekFailure;
				hr = pIStream->Read(&theChar, sizeof(wchar_t), nullptr);
				if (FAILED(hr))
					return kGetLastCharacterOfContent_result::kStreamReadFailure;
				return kGetLastCharacterOfContent_result::kOk;
			}
			return kGetLastCharacterOfContent_result::kNoStreamContent;
		}

		bool IsStreamContentAnArray(IStream* pIStream)
		{
			// is considered an array if have contents and last char is null
			wchar_t lastChar = L'\0';
			return (GetLastCharacterOfContent(pIStream, lastChar) == kGetLastCharacterOfContent_result::kOk && (lastChar == L'\0'));
		}

	}
}
