fileIn "MxsUnit/MxsUnit.ms"

struct pointcache_asset_Test
(
	theNode,
	theObj,
	basefilename,
	printResults = false,  -- use to print out 'expected results' that can then be pasted back into the test code
	fn stripNameFromFileName fname =
	(
		if matchPattern fname pattern:(basefilename+"*") do
			fname = substring fname (basefilename.count+1) -1
		fname
	),
	
	fn stripNameFromFileNames fnameArray =
	(
		local n = fnameArray.count
		for i = 1 to n do
			fnameArray[i] = stripNameFromFileName fnameArray[i] 
		ok
	),
	
	fn enumFilesProc name a = (append a name; ok),
--	fn enumFilesProc name a = append a (filenameFromPath name),
	
	fn prep =
	(
		theNode= box isSelected:on
		dummy name:theNode.name
		theNode.name += "_"
		
		basefilename =  getdir #export + "\\" + theNode.name
		files = getFiles (basefilename + "*.*")
		for f in files do deleteFile f
		
		with animate on at time 100 theNode.width *= 2
		local pc = Point_Cache ()
		theObj = pc
		addmodifier theNode pc
		pc.filename = basefilename + ".xml"
		pc.fileCount = 1
		pc.recordEnd = 3
		pc.record()
	),
	
	fn validate_results expected actual message: =
	(
		assert_equal expected.count actual.count message:message
		local count = expected.count
		for i = 1 to count do
			assert_equal expected[i] actual[i] message:message
	),

	fn validate_results_x expected actual message: = (),
	
	fn phase_1_test =
	(
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc", "Frame1.mc", "Frame2.mc", "Frame3.mc")
		validate_results expected_results names message:"non-accessor - all files"
		if printResults do format "non-accessor - all files\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"non-accessor - all files, skipSubFiles"
		if printResults do format "non-accessor - all files, skipSubFiles\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #firstSubOnly; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc", "Frame1.mc", "Frame2.mc", "Frame3.mc")
		validate_results expected_results names message:"non-accessor - all files, firstSubOnly"
		if printResults do format "non-accessor - all files, firstSubOnly\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #missing; stripNameFromFileNames names
		expected_results = #()
		validate_results expected_results names message:"non-accessor - missing files"
		if printResults do format "non-accessor - missing files\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #missing; stripNameFromFileNames names
		expected_results = #()
		validate_results expected_results names message:"non-accessor - missing files, skipSubFiles"
		if printResults do format "non-accessor - missing files, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #()
		validate_results expected_results names message:"non-accessor - missing files, firstSubOnly"
		if printResults do format "non-accessor - missing files, firstSubOnly\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #()
		validate_results expected_results names message:"non-accessor - missing files, firstSubOnly, skipSubFiles"
		if printResults do format "non-accessor - missing files, firstSubOnly, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - all files"
		if printResults do format "accessor - all files\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - all files, skipSubFiles"
		if printResults do format "accessor - all files, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor #missing; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - missing files"
		if printResults do format "accessor - missing files\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor #missing; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - missing files, skipSubFiles"
		if printResults do format "accessor - missing files, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - missing files, firstSubOnly"
		if printResults do format "accessor - missing files, firstSubOnly\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - missing files, firstSubOnly, skipSubFiles"
		if printResults do format "accessor - missing files, firstSubOnly, skipSubFiles\n\t\texpected_results = %\n" names
	),
	
	fn phase_2_test =
	(
		if printResults do format "\n\n"

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc", "Frame3.mc")
		validate_results expected_results names message:"non-accessor - all files after delete subfiles 1 and 2"
		if printResults do format "non-accessor - all files after delete subfiles 1 and 2\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"non-accessor - all files after delete subfiles 1 and 2, skipSubFiles"
		if printResults do format "non-accessor - all files after delete subfiles 1 and 2, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #missing; stripNameFromFileNames names
		expected_results = #("Frame1.mc", "Frame2.mc")
		validate_results expected_results names message:"non-accessor - missing files after delete subfiles 1 and 2"
		if printResults do format "non-accessor - missing files after delete subfiles 1 and 2\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #missing; stripNameFromFileNames names
		expected_results = #()
		validate_results expected_results names message:"non-accessor - missing files after delete subfiles 1 and 2, skipSubFiles"
		if printResults do format "non-accessor - missing files after delete subfiles 1 and 2, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #("Frame1.mc")
		validate_results expected_results names message:"non-accessor - missing files after delete subfiles 1 and 2, firstSubOnly"
		if printResults do format "non-accessor - missing files after delete subfiles 1 and 2, firstSubOnly\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #()
		validate_results expected_results names message:"non-accessor - missing files after delete subfiles 1 and 2, firstSubOnly, skipSubFiles"
		if printResults do format "non-accessor - missing files after delete subfiles 1 and 2, firstSubOnly, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc", "Frame1.mc", "Frame2.mc")
		validate_results expected_results names message:"accessor - all files after delete subfiles 1 and 2"
		if printResults do format "accessor - all files after delete subfiles 1 and 2\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - all files after delete subfiles 1 and 2, skipSubFiles"
		if printResults do format "accessor - all files after delete subfiles 1 and 2, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor #missing; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc", "Frame1.mc", "Frame2.mc")
		validate_results expected_results names message:"accessor - missing files after delete subfiles 1 and 2"
		if printResults do format "accessor - missing files after delete subfiles 1 and 2\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor #missing; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - missing files after delete subfiles 1 and 2, skipSubFiles"
		if printResults do format "accessor - missing files after delete subfiles 1 and 2, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc", "Frame1.mc")
		validate_results expected_results names message:"accessor - missing files after delete subfiles 1 and 2, firstSubOnly"
		if printResults do format "accessor - missing files after delete subfiles 1 and 2, firstSubOnly\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - missing files after delete subfiles 1 and 2, firstSubOnly, skipSubFiles"
		if printResults do format "accessor - missing files after delete subfiles 1 and 2, firstSubOnly, skipSubFiles\n\t\texpected_results = %\n" names
	),
	
	fn phase_3_test =
	(
		if printResults do format "\n\n"

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce; stripNameFromFileNames names
		expected_results = #(".xml", "Frame3.mc")
		validate_results expected_results names message:"non-accessor - all files after delete first subfile"
		if printResults do format "non-accessor - all files after delete first subfile\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"non-accessor - all files after delete first subfile, skipSubFiles"
		if printResults do format "non-accessor - all files after delete first subfile, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #missing; stripNameFromFileNames names
		expected_results = #("Frame0.mc", "Frame1.mc", "Frame2.mc")
		validate_results expected_results names message:"non-accessor - missing files after delete first subfile"
		if printResults do format "non-accessor - missing files after delete first subfile\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #missing; stripNameFromFileNames names
		expected_results = #()
		validate_results expected_results names message:"non-accessor - missing files after delete first subfile, skipSubFiles"
		if printResults do format "non-accessor - missing files after delete first subfile, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #("Frame0.mc")
		validate_results expected_results names message:"non-accessor - missing files after delete first subfile, firstSubOnly"
		if printResults do format "non-accessor - missing files after delete first subfile, firstSubOnly\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #()
		validate_results expected_results names message:"non-accessor - missing files after delete first subfile, firstSubOnly, skipSubFiles"
		if printResults do format "non-accessor - missing files after delete first subfile, firstSubOnly, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc", "Frame1.mc", "Frame2.mc")
		validate_results expected_results names message:"accessor - all files after delete first subfile"
		if printResults do format "accessor - all files after delete first subfile\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - all files after delete first subfile, skipSubFiles"
		if printResults do format "accessor - all files after delete first subfile, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor #missing; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc", "Frame1.mc", "Frame2.mc")
		validate_results expected_results names message:"accessor - missing files after delete first subfile"
		if printResults do format "accessor - missing files after delete first subfile\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor #missing; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - missing files after delete first subfile, skipSubFiles"
		if printResults do format "accessor - missing files after delete first subfile, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - missing files after delete first subfile, firstSubOnly"
		if printResults do format "accessor - missing files after delete first subfile, firstSubOnly\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #(".xml", "Frame0.mc")
		validate_results expected_results names message:"accessor - missing files after delete first subfile, firstSubOnly, skipSubFiles"
		if printResults do format "accessor - missing files after delete first subfile, firstSubOnly, skipSubFiles\n\t\texpected_results = %\n" names
	),
	
	fn phase_4_test =
	(
		if printResults do format "\n\n"

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce; stripNameFromFileNames names
		expected_results = #()
		validate_results expected_results names message:"non-accessor - all files after delete primary asset"
		if printResults do format "non-accessor - all files after delete primary asset\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles; stripNameFromFileNames names
		expected_results = #()
		validate_results expected_results names message:"non-accessor - all files after delete primary asset, skipSubFiles"
		if printResults do format "non-accessor - all files after delete primary asset, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #missing; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"non-accessor - missing files after delete primary asset"
		if printResults do format "non-accessor - missing files after delete primary asset\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #missing; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"non-accessor - missing files after delete primary asset, skipSubFiles"
		if printResults do format "non-accessor - missing files after delete primary asset, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"non-accessor - missing files after delete primary asset, firstSubOnly"
		if printResults do format "non-accessor - missing files after delete primary asset, firstSubOnly\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"non-accessor - missing files after delete primary asset, firstSubOnly, skipSubFiles"
		if printResults do format "non-accessor - missing files after delete primary asset, firstSubOnly, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"accessor - all files after delete primary asset"
		if printResults do format "accessor - all files after delete primary asset\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"accessor - all files after delete primary asset, skipSubFiles"
		if printResults do format "accessor - all files after delete primary asset, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor #missing; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"accessor - missing files after delete primary asset"
		if printResults do format "accessor - missing files after delete primary asset\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor #missing; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"accessor - missing files after delete primary asset, skipSubFiles"
		if printResults do format "accessor - missing files after delete primary asset, skipSubFiles\n\t\texpected_results = %\n" names

		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #useEnumAccessor #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"accessor - missing files after delete primary asset, firstSubOnly"
		if printResults do format "accessor - missing files after delete primary asset, firstSubOnly\n\t\texpected_results = %\n" names
		names = #();enumeratefiles theObj enumFilesProc names #visitOnce #skipSubFiles #useEnumAccessor #missing #firstSubOnly; stripNameFromFileNames names
		expected_results = #(".xml")
		validate_results expected_results names message:"accessor - missing files after delete primary asset, firstSubOnly, skipSubFiles"
		if printResults do format "accessor - missing files after delete primary asset, firstSubOnly, skipSubFiles\n\t\texpected_results = %\n" names
	),

	fn do_pointcache_asset_Test =
	(
		prep()
		phase_1_test()

		-- delete some of the sub files and check behavior when looking for missing files
		for i = 1 to 2 do
				deleteFile (basefilename + "Frame" + (i as string) + ".mc") -- delete file used by .xml
		phase_2_test()
		
		-- delete first subfile
		deleteFile (basefilename + "Frame0.mc") -- delete file used by .xml
		phase_3_test()

		-- delete asset
		deleteFile (basefilename + ".xml")-- delete .xml
		cacheOps.reload theObj
		phase_4_test()
		ok
	),

	Tests = 
	#(
		do_pointcache_asset_Test
	)
)

run_test_fixture pointcache_asset_Test script:(getThisScriptFilename())
