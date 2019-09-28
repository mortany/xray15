////**************************************************************************/
//// Copyright (c) 1998-2018 Autodesk, Inc.
//// All rights reserved.
//// 
//// These coded instructions, statements, and computer programs contain
//// unpublished proprietary information written by Autodesk, Inc., and are
//// protected by Federal copyright law. They may not be disclosed to third
//// parties or copied or duplicated in any form, in whole or in part, without
//// the prior written consent of Autodesk, Inc.
////**************************************************************************/
//// DESCRIPTION: Entry point for the BonesDef Unit Tests.
//// AUTHOR: Autodesk Inc.
////***************************************************************************/

#include <windows.h>
#include <gtest/gtest.h>
#include <string>


/**
* \brief Return the full path of the directory containing the given full file
* path.
* \param filePath Full path of the file for which to retrieve the directory
* path.
* \return Full path of the directory containing the given full file path.
*/
const std::wstring GetDirectoryPathOfFilePath(const std::string& filePath) {
	size_t lastDirectorySeparatorIndex = filePath.find_last_of("\\/");
	if (lastDirectorySeparatorIndex == std::string::npos) {
		return L".";
	}

	std::string directoryPath = filePath.substr(0, lastDirectorySeparatorIndex);
	return std::wstring(directoryPath.begin(), directoryPath.end());
}


/**
* \brief Entry point for the executable.
* \remarks To support testing of 3ds Max plugins located in the "/stdplugs"
* subdirectory:
*   1. In the "*.test.vcxproj" file, set the library targeted for testing to
*      "Delay Load" (e.g.: "Delay Loaded Dlls: Bonesdef.dlm").
*   2. Set the output directory of the test executable to "$(MaxBuild)", so the
*      executable is put in the same directory as "gtestmain.dll", "maxutil.dll",
*      "core.dll", etc.
*   3. In the test project, create an "int main()" function to serve as an
*      entry point for the test executable, and add "/stdplugs" to the list of
*      directories where Windows should look to attempt to resolve delay-loaded
*      libraries.
* \remarks The 3ds Max test runner will not start the test executable from its
* directory, but rather from another location on disk. This requires formatting
* the path of the DLL directory so its full path is provided instead of a
* relative one.
*/
int main(int argc, char** argv) {
	/// Format path to the "/stdplugs" subdirectory where the library to test is
	/// located:
	const char* executableFilePath = argv[0];
	const std::wstring DLLDirectory = GetDirectoryPathOfFilePath(executableFilePath) + L"/stdplugs/";

	BOOL setDLLDirectoryWasSuccessful = SetDllDirectory(DLLDirectory.data());
	if (!setDLLDirectoryWasSuccessful) {
		wprintf(L"Unable to add '%ls' to the list of DLL directories.\n", DLLDirectory.c_str());
		return -1;
	}
	::testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
