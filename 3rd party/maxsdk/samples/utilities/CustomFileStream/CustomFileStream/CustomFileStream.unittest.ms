/*
    @file CustomFileStream.unittest.ms
    @param Author:          Larry Minton
    @param Description:     This script tests the functionality of the CustomFileStream and CustomSceneStreamManager FPS interfaces.
    @param Status:          Online
*/

fileIn "MxsUnit/MxsUnit.ms"

struct UnitTest_CustomFileStream
(
	fname = @"$temp\UnitTest_CustomFileStream_test.max",
	newStream = "XXX",
	test_string1= "hello world",
	test_string2= "Life's a beach!" ,
	test_string3= " - Fun in the sun!",
	dataArray1 = #("a?a*a","b/b:b","c\\c\"c"),
	dataArray2 = #("ddd","eee","fff"),
	dataArray3 = #("ggg", ""),
	dataArray4 = #(""),
	dataArray5 = #(),

	dataArray1_2 = #(),
	init_dataArray1_2 = (join dataArray1_2 dataArray1; join dataArray1_2 dataArray2; ok),
	
	callbackFn_call_args = #(),
	
	--------------------------------------------
	-- Unit Test methods
	--------------------------------------------	
	function setup =
	(
	),

	function teardown =
	(
	),
	
	function callbackFn fname type =
	(
		append callbackFn_call_args (datapair fname type)
	),
	
	fn test_EnsureNoDeferredLoad =
	(
		print "test_EnsureNoDeferredLoad"
		local thisDir = getfilenamepath (getThisScriptFilename())
		loadMaxFile (thisDir + @"TestData\FileWithCustomFileStreams.max") quiet:true
		assert_true (CustomSceneStreamManager.doesStreamExist newStream)
		res = CustomSceneStreamManager.getStreamData newStream --> "hello world"
		assert_string_equal test_string1 res
	),
	
	fn test_CustomSceneStream =
	(
		print "test_CustomSceneStream"
		if ::CustomFileStream == undefined do
		(
			print "CustomFileStream undefined - test skipped"
			return ok
		)
		
--		showinterface CustomFileStream
		
		resetmaxfile #noprompt
		savemaxfile fname

		assert_false (CustomFileStream.DoesStorageExist fname)
		try 
		(
			CustomFileStream.readStream fname newStream --> -- Runtime error: Stream cannot be opened: "XXX"
			assert_true false
		)
		catch()
		try 
		(
			CustomFileStream.deleteStream fname newStream --> -- Runtime error: Stream cannot be opened: "XXX"
			assert_true false
		)
		catch()
		try 
		(
			CustomFileStream.deleteStream fname "VideoPostQueue" --> 
			assert_true false
		)
		catch()		
		assert_true (CustomFileStream.writeStream fname "1234567890123456789012345678901" test_string1)
		assert_true (CustomFileStream.deleteStream fname "1234567890123456789012345678901")
		try 
		(
			CustomFileStream.writeStream fname "12345678901234567890123456789012" test_string1 --> name too long
			assert_true (CustomFileStream.deleteStream fname "12345678901234567890123456789012")
			assert_true false
		)
		catch()
		try 
		(
			CustomFileStream.writeStream fname "" test_string1 --> name empty
			assert_true (CustomFileStream.deleteStream fname "")
			assert_true false
		)
		catch()
	
		assert_true (CustomFileStream.DoesStorageExist fname)
		
		res = CustomFileStream.getStreamNames fname --> #()
		assert_equal 0 res.count
		assert_false (CustomFileStream.doesStreamExist fname newStream)
		assert_true (CustomFileStream.writeStream fname newStream test_string1)
		res = CustomFileStream.getStreamNames fname --> #("XXX")
		assert_equal 1 res.count
		assert_true (CustomFileStream.doesStreamExist fname newStream)
		res = CustomFileStream.readStream fname newStream --> "hello world"
		assert_string_equal test_string1 res
		assert_false (CustomFileStream.isStreamDataAnArray fname newStream)
		assert_true (CustomFileStream.isCustomFileStreamOperable fname newStream)
		assert_true (CustomFileStream.writeStream fname newStream test_string2)
		res = CustomFileStream.readStream fname newStream --> "Life's a beach!"
		assert_string_equal test_string2 res
		assert_true (CustomFileStream.writeStream fname newStream test_string3 append:true)
		res = CustomFileStream.readStream fname newStream --> "Life's a beach! - Fun in the sun!"
		assert_string_equal (test_string2+test_string3) res
		assert_false (CustomFileStream.isStreamDataAnArray fname newStream)
		assert_true (CustomFileStream.writeStream fname newStream "")
		res = CustomFileStream.readStream fname newStream --> ""
		assert_string_equal "" res
		assert_false (CustomFileStream.isStreamDataAnArray fname newStream)
		assert_true (CustomFileStream.deleteStream fname newStream)
		res = CustomFileStream.getStreamNames fname --> #()
		assert_equal 0 res.count
		assert_false (CustomFileStream.doesStreamExist fname newStream)

		assert_true (CustomFileStream.writeStreamArray fname newStream dataArray1)
		res = CustomFileStream.readStreamArray fname newStream
		assert_equal dataArray1.count res.count
		for i = 1 to dataArray1.count do assert_string_equal dataArray1[i] res[i] message:(i as string)
		assert_true (CustomFileStream.isStreamDataAnArray fname newStream)

		assert_true (CustomFileStream.writeStreamArray fname newStream dataArray2 append:true)
		res = CustomFileStream.readStreamArray fname newStream
		assert_equal dataArray1_2.count res.count
		for i = 1 to dataArray1_2.count do assert_string_equal dataArray1_2[i] res[i] message:(i as string)
		assert_true (CustomFileStream.isStreamDataAnArray fname newStream)
		assert_true (CustomFileStream.writeStreamArray fname newStream dataArray3)
		res = CustomFileStream.readStreamArray fname newStream
		assert_equal dataArray3.count res.count
		for i = 1 to dataArray3.count do assert_string_equal dataArray3[i] res[i] message:(i as string)
		assert_true (CustomFileStream.writeStreamArray fname newStream dataArray4)
		res = CustomFileStream.readStreamArray fname newStream
		assert_equal dataArray4.count res.count
		for i = 1 to dataArray4.count do assert_string_equal dataArray4[i] res[i] message:(i as string)
		assert_true (CustomFileStream.writeStreamArray fname newStream dataArray5)
		res = CustomFileStream.readStreamArray fname newStream
		assert_equal 1 res.count  -- special case - no such this as a completely empty stream - interpretted as a null string
		assert_string_equal "" res[1]
		assert_false (CustomFileStream.isStreamDataAnArray fname newStream)
		assert_true (CustomFileStream.deleteStream fname newStream)

		assert_true (CustomFileStream.writeStream fname newStream test_string1)
		res = CustomFileStream.getStreamFlags fname newStream
		assert_equal 0 res
		assert_true (CustomFileStream.setStreamFlags fname newStream 9)
		res = CustomFileStream.getStreamFlags fname newStream
		assert_equal 9 res
		res = CustomFileStream.getStreamFlags fname newStream getPrivate:true
		assert_equal 1 res
		assert_true (CustomFileStream.setStreamFlags fname newStream 11 setPrivate:true)
		res = CustomFileStream.getStreamFlags fname newStream getPrivate:true
		assert_equal 11 res
		assert_true (CustomFileStream.deleteStream fname newStream)
		
		assert_true (CustomFileStream.writeStream fname newStream test_string1)
		res = CustomFileStream.getStreamFlags fname newStream getPrivate:true
		assert_equal 1 res
		assert_true (CustomFileStream.deleteStream fname newStream)
		assert_true (CustomFileStream.writeStream fname newStream test_string1 persistent:false)
		res = CustomFileStream.getStreamFlags fname newStream getPrivate:true
		assert_equal 0 res
		assert_true (CustomFileStream.deleteStream fname newStream)
		assert_true (CustomFileStream.writeStream fname newStream test_string1 noLoadOnSceneLoad:true)
		res = CustomFileStream.getStreamFlags fname newStream getPrivate:true
		assert_equal 5 res
		assert_true (CustomFileStream.deleteStream fname newStream)

		assert_true (CustomFileStream.writeStreamArray fname newStream dataArray1)
		res = CustomFileStream.getStreamFlags fname newStream getPrivate:true
		assert_equal 1 res
		assert_true (CustomFileStream.deleteStream fname newStream)
		assert_true (CustomFileStream.writeStreamArray fname newStream dataArray1 persistent:false)
		res = CustomFileStream.getStreamFlags fname newStream getPrivate:true
		assert_equal 0 res
		assert_true (CustomFileStream.deleteStream fname newStream)
		assert_true (CustomFileStream.writeStreamArray fname newStream dataArray1 noLoadOnSceneLoad:true)
		res = CustomFileStream.getStreamFlags fname newStream getPrivate:true
		assert_equal 5 res
		assert_true (CustomFileStream.deleteStream fname newStream)

		assert_true (CustomFileStream.writeStreamArray fname newStream dataArray1 append:true)
		res = CustomFileStream.readStreamArray fname newStream
		assert_equal dataArray1.count res.count
		for i = 1 to dataArray1.count do assert_string_equal dataArray1[i] res[i] message:(i as string)
		assert_true (CustomFileStream.isStreamDataAnArray fname newStream)
		try 
		(
			CustomFileStream.writeStream fname newStream test_string1 append:true
			assert_true false
		)
		catch()
		assert_true (CustomFileStream.deleteStream fname newStream)
		
		assert_true (CustomFileStream.writeStream fname newStream test_string1 append:true)
		res = CustomFileStream.readStream fname newStream --> "hello world"
		assert_string_equal test_string1 res
		assert_false (CustomFileStream.isStreamDataAnArray fname newStream)
		try 
		(
			CustomFileStream.writeStreamArray fname newStream dataArray1 append:true
			assert_true false
		)
		catch()
		assert_true (CustomFileStream.deleteStream fname newStream)
	),

	fn test_CustomSceneStreamManager =
	(
		print "test_CustomSceneStreamManager"
		if ::CustomSceneStreamManager == undefined do
		(
			print "CustomSceneStreamManager undefined - test skipped"
			return ok
		)
		
--		showinterface CustomSceneStreamManager
		
		resetmaxfile #noprompt
		savemaxfile fname
		n = CustomSceneStreamManager.numEntries
		assert_equal 0 n
				
		----------------------------------------------------------------
		-- we write data to a file using CustomFileStream methods, and then load that file to
		-- populate the CustomSceneStreamManager

		dataVals = #()
		dataVals[1] = test_string1
		dataVals[2] = test_string2
		dataVals[3] = ""
		dataVals[4] = dataArray1
		dataVals[5] = dataArray3
		dataVals[6] = dataArray4
		dataVals[7] = dataArray5
				
		assert_true (CustomFileStream.writeStream fname "stream1" dataVals[1])
		assert_true (CustomFileStream.writeStream fname "stream2" dataVals[2])
		assert_true (CustomFileStream.writeStream fname "stream3" dataVals[3])
		assert_true (CustomFileStream.writeStreamArray fname "stream4" dataVals[4])
		assert_true (CustomFileStream.writeStreamArray fname "stream5" dataVals[5])
		assert_true (CustomFileStream.writeStreamArray fname "stream6" dataVals[6])
		assert_true (CustomFileStream.writeStreamArray fname "stream7" dataVals[7])

		assert_true (CustomFileStream.setStreamFlags fname "stream1" 1)
		assert_true (CustomFileStream.setStreamFlags fname "stream2" 2)
		assert_true (CustomFileStream.setStreamFlags fname "stream3" 3)
		assert_true (CustomFileStream.setStreamFlags fname "stream4" 4)
		assert_true (CustomFileStream.setStreamFlags fname "stream5" 5)
		assert_true (CustomFileStream.setStreamFlags fname "stream6" 6)
		assert_true (CustomFileStream.setStreamFlags fname "stream7" 7)

		loadmaxfile fname
		n = CustomSceneStreamManager.numEntries
		assert_equal 7 n

		for i = 1 to n do
		(
			local name = CustomSceneStreamManager.getStreamName i
			index = CustomSceneStreamManager.getStreamByName name
			public_flag = CustomSceneStreamManager.getStreamFlag i
			private_flag = CustomSceneStreamManager.getStreamFlag i privateFlag:true
			data = undefined
			if (CustomSceneStreamManager.isStreamDataAnArray i) then
			(
				data = CustomSceneStreamManager.getStreamDataAsArray i
				for j = 1 to dataVals[i].count do assert_string_equal dataVals[i][j] data[j]
			)
			else
			(
				data = CustomSceneStreamManager.getStreamData i
				if i != 7 then
					assert_string_equal dataVals[i] data
				else
					assert_string_equal "" data  -- special case - no such this as a completely empty stream - interpretted as a null string
			)
			assert_equal i public_flag
			assert_equal 1 private_flag
--			format "% : % : % : % : %\n" name index public_flag private_flag data
		)

		for i = 1 to n do
		(
			local name = CustomSceneStreamManager.getStreamName i
			index = CustomSceneStreamManager.getStreamByName name
			public_flag = CustomSceneStreamManager.getStreamFlag name
			private_flag = CustomSceneStreamManager.getStreamFlag name privateFlag:true
			data = undefined
			if (CustomSceneStreamManager.isStreamDataAnArray name) then
			(
				data = CustomSceneStreamManager.getStreamDataAsArray name
			)
			else
			(
				data = CustomSceneStreamManager.getStreamData name
			)
--			format "% : % : % : % : %\n" name index public_flag private_flag data
		)

		assert_false (CustomSceneStreamManager.doesStreamExist "stream0")
		assert_false (CustomSceneStreamManager.doesStreamExist 99)
		try 
		(
			CustomSceneStreamManager.getStreamByName "stream0"
			assert_true false
		)
		catch()
		try 
		(
			CustomSceneStreamManager.getStreamData "stream0"
			assert_true false
		)
		catch()
		try 
		(
			CustomSceneStreamManager.getStreamData 99
			assert_true false
		)
		catch()
		try 
		(
			CustomSceneStreamManager.getStreamData red
			assert_true false
		)
		catch()
		
		assert_true (CustomSceneStreamManager.createStream "1234567890123456789012345678901")
		CustomSceneStreamManager.deleteStream "1234567890123456789012345678901"
		try 
		(
			CustomSceneStreamManager.writeStream "12345678901234567890123456789012" newStream test_string1 --> name too long
			CustomSceneStreamManager.deleteStream "12345678901234567890123456789012"
			assert_true false
		)
		catch()
		try 
		(
			CustomSceneStreamManager.writeStream "" newStream test_string1 --> name empty
			CustomSceneStreamManager.deleteStream ""
			assert_true false
		)
		catch()

		newStream = "stream8"
		res = CustomSceneStreamManager.createStream newStream
		assert_equal 8 res
		res = CustomSceneStreamManager.getStreamData newStream
		assert_string_equal "" res
		assert_true (CustomSceneStreamManager.isStreamPersistent newStream)
		assert_true (CustomSceneStreamManager.getSaveNonPersistentStream newStream)
		assert_equal 0 (CustomSceneStreamManager.getStreamFlag newStream)
		assert_equal 3 (CustomSceneStreamManager.getStreamFlag newStream privateFlag:true)

		CustomSceneStreamManager.setStreamData newStream dataVals[1]
		res = CustomSceneStreamManager.getStreamData newStream
		assert_string_equal dataVals[1] res
		res = CustomSceneStreamManager.getStreamDataAsArray newStream
		assert_equal 1 res.count
		assert_string_equal dataVals[1] res[1]
		CustomSceneStreamManager.setStreamDataAsArray newStream dataVals[1]
		res = CustomSceneStreamManager.getStreamDataAsArray newStream
		assert_equal 1 res.count
		assert_string_equal dataVals[1] res[1]

		-- test persistence
		savemaxfile fname
		resetmaxfile #noprompt
		n = CustomSceneStreamManager.numEntries
		assert_equal 0 n
		loadmaxfile fname
		n = CustomSceneStreamManager.numEntries
		assert_equal 8 n
		for i = 1 to n do
		(
			local name = CustomSceneStreamManager.getStreamName i
			index = CustomSceneStreamManager.getStreamByName name
			public_flag = CustomSceneStreamManager.getStreamFlag i
			private_flag = CustomSceneStreamManager.getStreamFlag i privateFlag:true
			data = undefined
			if (CustomSceneStreamManager.isStreamDataAnArray i) then
			(
				data = CustomSceneStreamManager.getStreamDataAsArray i
			)
			else
			(
				data = CustomSceneStreamManager.getStreamData i
			)
--			format "% : % : % : % : %\n" name index public_flag private_flag data
		)

		-- clear persistence flag
		for i = 1 to n do
		(
			assert_true (CustomSceneStreamManager.isStreamPersistent i)
			assert_equal 1 (CustomSceneStreamManager.getStreamFlag i privateFlag:true)
			CustomSceneStreamManager.setStreamPersistent i false
			assert_false (CustomSceneStreamManager.isStreamPersistent i)
			assert_equal 0 (CustomSceneStreamManager.getStreamFlag i privateFlag:true)
		)
		savemaxfile fname
		resetmaxfile #noprompt
		n = CustomSceneStreamManager.numEntries
		assert_equal 0 n
		loadmaxfile fname
		n = CustomSceneStreamManager.numEntries
		assert_equal 0 n

		newStream = "stream9"
		res = CustomSceneStreamManager.createStream newStream
		assert_equal 1 res
		assert_true (CustomSceneStreamManager.isStreamPersistent newStream)
		CustomSceneStreamManager.deleteStream newStream
		res = CustomSceneStreamManager.createStream newStream persistent:false
		assert_equal 1 res
		assert_false (CustomSceneStreamManager.isStreamPersistent newStream)
		CustomSceneStreamManager.deleteStream newStream

		CustomSceneStreamManager.createStream newStream
		CustomSceneStreamManager.setStreamData newStream dataVals[1]
		CustomSceneStreamManager.setStreamPersistent newStream false
		assert_false (CustomSceneStreamManager.isStreamPersistent newStream)
		assert_true (CustomSceneStreamManager.getSaveNonPersistentStream newStream)
		CustomSceneStreamManager.setSaveNonPersistentStream newStream false
		assert_false (CustomSceneStreamManager.isStreamPersistent newStream)
		assert_false (CustomSceneStreamManager.getSaveNonPersistentStream newStream)
		CustomSceneStreamManager.setSaveNonPersistentStream newStream true
		assert_false (CustomSceneStreamManager.isStreamPersistent newStream)
		assert_true (CustomSceneStreamManager.getSaveNonPersistentStream newStream)
		savemaxfile fname
		loadmaxfile fname
		assert_true (CustomSceneStreamManager.doesStreamExist newStream)
		assert_false (CustomSceneStreamManager.isStreamPersistent newStream)
		assert_false (CustomSceneStreamManager.getSaveNonPersistentStream newStream)
		savemaxfile fname
		loadmaxfile fname
		assert_false (CustomSceneStreamManager.doesStreamExist newStream)
		
		CustomSceneStreamManager.createStream newStream
		CustomSceneStreamManager.setStreamData newStream dataVals[1]
		CustomSceneStreamManager.setNoLoadOnSceneLoad newStream true
		assert_true (CustomSceneStreamManager.getNoLoadOnSceneLoad newStream)
		savemaxfile fname
		loadmaxfile fname
		assert_false (CustomSceneStreamManager.doesStreamExist newStream)
		assert_true (CustomFileStream.doesStreamExist fname newStream)
		
		CustomSceneStreamManager.createStream newStream noLoadOnSceneLoad:true
		assert_true (CustomSceneStreamManager.getNoLoadOnSceneLoad newStream)
		CustomSceneStreamManager.setStreamData newStream dataVals[1]
		savemaxfile fname
		loadmaxfile fname
		assert_false (CustomSceneStreamManager.doesStreamExist newStream)
		assert_true (CustomFileStream.doesStreamExist fname newStream)
		
		CustomSceneStreamManager.createStream newStream
		CustomSceneStreamManager.setStreamData newStream dataVals[1]
		CustomSceneStreamManager.registerCallback callbackFn #test #postLoad
		CustomSceneStreamManager.registerCallback callbackFn #test #preSave
		CustomSceneStreamManager.registerCallback callbackFn #test #postSave
		callbackFn_call_args.count = 0
		savemaxfile fname
		loadmaxfile fname
		assert_equal 3 callbackFn_call_args.count
		for i = 1 to 3 do
			assert_true (pathconfig.pathsResolveEquivalent fname callbackFn_call_args[1].v1)
		assert_equal #preSave callbackFn_call_args[1].v2
		assert_equal #postSave callbackFn_call_args[2].v2
		assert_equal #postLoad callbackFn_call_args[3].v2
		local callback_fns = CustomSceneStreamManager.getCallbacks callback_fn:callbackFn
		assert_equal 3 callback_fns.count
		callback_fns = CustomSceneStreamManager.getCallbacks event:#postSave
		assert_equal 1 callback_fns.count
		CustomSceneStreamManager.unregisterCallbacks callback_id:#test event:#postSave
		CustomSceneStreamManager.unregisterCallbacks callback_id:#test event:#preSave
		callbackFn_call_args.count = 0
		savemaxfile fname
		loadmaxfile fname
		assert_equal 1 callbackFn_call_args.count
		CustomSceneStreamManager.unregisterCallbacks callback_fn:callbackFn
		callback_fns = CustomSceneStreamManager.getCallbacks callback_id:#test
		assert_equal 0 callback_fns.count
		
		resetmaxfile #noprompt
		assert_equal 1 (CustomSceneStreamManager.createStream newStream)
		assert_false (CustomSceneStreamManager.isStreamDataAnArray newStream)
		assert_string_equal "" ((CustomSceneStreamManager.getStreamDataAsArray newStream)[1])
		assert_string_equal "" (CustomSceneStreamManager.getStreamData newStream)		
		savemaxfile fname
		loadmaxfile fname
		assert_string_equal "" ((CustomSceneStreamManager.getStreamDataAsArray newStream)[1])
		assert_string_equal "" (CustomSceneStreamManager.getStreamData newStream)		
	),

	Tests = 
	#(
		test_EnsureNoDeferredLoad,
		test_CustomSceneStream,
		test_CustomSceneStreamManager
	)
)

if (run_test_fixture != undefined) then
(
	run_test_fixture UnitTest_CustomFileStream script:(getThisScriptFilename())
	ok
)
else
(
	local foo = UnitTest_CustomFileStream()
	for test in foo.tests do 
	(
		foo.setup()
		test()
		foo.teardown()
	)
	ok
)

