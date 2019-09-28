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

//! CustomSceneStreamManager interface ID 
#define CUSTOMSCENESTREAMMANAGER_INTERFACE   Interface_ID(0x98595E92, 0xE11720D8)

/*! \brief This interface is used to support persisting of custom scene streams containing user specified
string data in 3ds Max scene files across a scene file load / save.
Note that these methods will throw MAXExceptions on error conditions.
*/
class ICustomSceneStreamManager : public FPStaticInterface
{
public:
	/*! \brief Test to see if the specified CustomFileStream name is present in the manager
	\param name The stream name.
	\return true if the stream is present.
	*/
	virtual bool DoesStreamExist(const TCHAR* name) const = 0;
	/*! \brief Get the index of the specified CustomFileStream name in the manager.
	\param name The stream name.
	\return The index of the stream in the manager.
	*/
	virtual int GetStreamByName(const TCHAR* name) const = 0;
	/*! \brief Get the number of CustomFileStreams held by the manager.
	\return The number of CustomFileStreams held by the manager.
	*/
	virtual int GetNumStreams() const = 0;
	/*! \brief Get the name of the indexed CustomFileStream in the manager.
	\param index The index.
	\return The name of the indexed CustomFileStream in the manager.
	*/
	virtual const TCHAR* GetStreamName(int index) const = 0;
	/*! \brief Get the public or private flags of the indexed CustomFileStream in the manager.
	\param index The index.
	\param privateFlag If true, get the private flags, else get the public flags.
	\return The public or private flags of the indexed CustomFileStream in the manager..
	*/
	virtual DWORD GetStreamFlag(int index, bool privateFlag) const = 0;
	/*! \brief Set the public or private flags of the indexed CustomFileStream in the manager.
	\param index The index.
	\param val The new flags value.
	\param privateFlag If true, set the private flags, else set the public flags.
	*/
	virtual void SetStreamFlag(int index, DWORD val, bool privateFlag) = 0;
	/*! \brief Get whether the content of the indexed CustomFileStream in the manager is an array of strings.
	\param index The index.
	\return Whether the content of the indexed CustomFileStream in the manager is an array of strings.
	*/
	virtual bool IsStreamDataAnArray(int index) const = 0;
	/*! \brief Get the content of the indexed CustomFileStream in the manager as a string.
	\param index The index.
	\return The content of the indexed CustomFileStream in the manager as a string.
	*/
	virtual const TCHAR* GetStreamData(int index) const = 0;
	/*! \brief Get the content of the indexed CustomFileStream in the manager as an array of strings.
	\param index The index.
	\return The content of the indexed CustomFileStream in the manager as an array of strings.
	*/
	virtual void GetStreamDataAsArray(int index, Tab<const TCHAR*>& data) const = 0;
	/*! \brief Sets the content of the indexed CustomFileStream in the manager.
	\param index The index.
	\param data The new stream content.
	*/
	virtual void SetStreamData(int index, const TCHAR* data) = 0;
	/*! \brief Sets the content of the indexed CustomFileStream in the manager.
	\param index The index.
	\param data The new stream content.
	*/
	virtual void SetStreamDataAsArray(int index, const Tab<const TCHAR*>& data) = 0;
	/*! \brief Get whether the indexed CustomFileStream in the manager is flagged as persistent.
	\param index The index.
	\return Whether the indexed CustomFileStream in the manager is flagged as persistent.
	*/
	virtual bool IsStreamPersistent(int index) const = 0;
	/*! \brief Set whether the indexed CustomFileStream in the manager is flagged as persistent.
	\param index The index.
	\param persistent Whether the CustomFileStream is persistent.
	*/
	virtual void SetStreamPersistent(int index, int persistent) = 0;
	/*! \brief Get whether the indexed CustomFileStream in the manager is to be saved to the scene file even if not flagged as persistent.
	\param index The index.
	\return Whether the indexed CustomFileStream in the manager is to be saved to the scene file even if not flagged as persistent.
	*/
	virtual bool GetSaveNonPersistentStream(int index) const = 0;
	/*! \brief Set whether the indexed CustomFileStream in the manager is to be saved to the scene file even if not flagged as persistent.
	\param index The index.
	\param val Whether the indexed CustomFileStream in the manager is to be saved to the scene file even if not flagged as persistent.
	*/
	virtual void SetSaveNonPersistentStream(int index, int val) = 0;
	/*! \brief Get whether the indexed CustomFileStream in the manager is flagged as be written to the scene file but not read when the scene file is read.
	\param index The index.
	\return Whether the indexed CustomFileStream in the manager is flagged as be written to the scene file but not read when the scene file is read.
	*/
	virtual bool GetNoLoadOnSceneLoad(int index) const = 0;
	/*! \brief Set whether the indexed CustomFileStream in the manager is flagged as be written to the scene file but not read when the scene file is read.
	\param index The index.
	\param val  Whether the indexed CustomFileStream in the manager is flagged as be written to the scene file but not read when the scene file is read.
	*/
	virtual void SetNoLoadOnSceneLoad(int index, int val) = 0;
	/*! \brief Delete the indexed CustomFileStream in the manager.
	\param index The index.
	*/
	virtual void DeleteStream(int index) = 0;
	/*! \brief Create the specified CustomFileStream in the manager.
	\param name The stream name.
	\param persistent Whether the CustomFileStream is flagged as persistent.
	\param saveNonPersistentStream Whether the CustomFileStream is flagged to be saved even if not flagged as persistent.
	\param noLoadOnSceneLoad Whether the CustomFileStream is flagged as not to be loaded when the scene file is loaded.
	\return The index of the new stream in the manager.
	*/
	virtual int CreateStream(const TCHAR* name, bool persistent, bool saveNonPersistentStream, bool noLoadOnSceneLoad) = 0;
	/*! \brief Removes all CustomFileStreams from the manager in not locked.
	*/
	virtual void Reset() = 0;
	/*! \brief Tests whether the manager is locked. When locked, when loading a scene file the manager is not populated with the CustomFileStreams from
	that file, when saving a scene file the CustomFileStreams in the manager are not written to the scene file, and a Reset does not reset the manager's
	content.
	\return Whether the manager is locked.
	*/
	virtual bool IsLocked() const = 0;
	/*! \brief Locks and unlocks the manager.
	\param locked Whether the manager should be locked.
	*/
	virtual void SetLocked(bool locked) = 0;
};

ICustomSceneStreamManager* GetICustomSceneStreamManager() { return (ICustomSceneStreamManager*)GetCOREInterface(CUSTOMSCENESTREAMMANAGER_INTERFACE); }
