/*
    @param TestName:    NodeEventCallback.UnitTest
    @param Author:      Larry Minton
    @param Created:     September 25, 2018
    @param LastMod:     December 13, 2018
    @param ModBy:       Amy Goldenberg
    @param Desc:        Tests node event callbacks
    @param Status:      Online
*/

fileIn "MxsUnit/MxsUnit.ms"

struct NodeEventCallback_Test
(
	processing_callbacks = false,
	add_count = 0,
	delete_count = 0,
	all_count = 0,
	fn add_CallbackFn ev nd = 
	(
--		print "in add_CallbackFn"
		assert_true processing_callbacks
		add_count += 1
	),
	fn delete_CallbackFn ev nd = 
	(
--		print "in delete_CallbackFn"
		assert_true processing_callbacks
		delete_count += 1
	),
	fn all_CallbackFn ev nd =
	(
--		print "in all_CallbackFn"
		assert_true processing_callbacks
		all_count += 1
	),
	fn begin_CallbackFn ev nd = 
	(
--		print "in begin_CallbackFn"
		assert_false processing_callbacks
		processing_callbacks = true
	),
	fn end_CallbackFn ev nd = 
	(
--		print "in end_CallbackFn"
		assert_true processing_callbacks
		processing_callbacks = false
	),
	
	fn test_NodeEventCallback =
	(
		gc()  -- try to make sure any NodeEventCallbacks from previous runs are deleted
		local nodeEventCallbacks = getNodeEventCallbacks()
		assert_equal 0 nodeEventCallbacks.count message:"test expects 0 existing node event callbacks"
		local callbackItem = NodeEventCallback added:add_CallbackFn deleted:delete_CallbackFn all:all_CallbackFn callbackBegin:begin_CallbackFn callbackEnd:end_CallbackFn
		nodeEventCallbacks = getNodeEventCallbacks()
		assert_equal 1 nodeEventCallbacks.count 
		assert_equal callbackItem (nodeEventCallbacks[1])
			
		local ss = stringStream ""
		showNodeEventCallbacks to:ss
	--	print ss
		seek ss 0
		local i = 0
		while not (eof ss) do 
		(
			i += 1
			local item = readline ss
			local elements = filterstring item "<>,"
--			format "elements: %\n" elements
			if i == 1 then
			(
				local expected = #("NodeEventCallback", "<<varies>", "enabled:true", "polling:false", "delay:0", "mouseUp:false", "eventId:#added", "function:\"NodeEventCallback_Test::add_CallbackFn\"", "sourceFile:\""+getThisScriptFilename()+"\"")
				elements[2] = "<<varies>"
				assert_equal expected.count elements.count
				for j = 1 to expected.count do
					assert_string_equal (expected[j]) (trimLeft elements[j])
			)
			else if i == 5 do
			(
				local expected = #("NodeEventCallback", "<<varies>", "enabled:true", "polling:false", "delay:0", "mouseUp:false", "eventId:#all", "function:\"NodeEventCallback_Test::all_CallbackFn\"", "sourceFile:\""+getThisScriptFilename()+"\"")
				elements[2] = "<<varies>"
				assert_equal expected.count elements.count
				for j = 1 to expected.count do
					assert_string_equal (expected[j]) (trimLeft elements[j])
			)
		)
			
		local nodeEventCallbackData = showNodeEventCallbacks asArray:true
--		print nodeEventCallbackData
		assert_equal 5 nodeEventCallbackData.count
		local item = nodeEventCallbackData[1]
		assert_equal callbackItem (item[1])
		assert_equal #added (item[2])
		assert_string_equal "NodeEventCallback_Test::add_CallbackFn" (item[3])
		assert_string_equal (getThisScriptFilename()) (item[4])
		item = nodeEventCallbackData[5]
		assert_equal callbackItem (item[1])
		assert_equal #all (item[2])
		assert_string_equal "NodeEventCallback_Test::all_CallbackFn" (item[3])
		assert_string_equal (getThisScriptFilename()) (item[4])
			
		local t = teapot()
		local keep_looping = true
		for i = 1 to 5 while keep_looping do -- keep looping until the add_count value changes, max of 5 loops
		(	
			windows.processpostedmessages();sleep .1
			windows.processpostedmessages()
			keep_looping = (add_count == 0)
		)
		assert_false processing_callbacks
		assert_equal 1 add_count
		assert_equal 0 delete_count
		assert_equal 1 all_count
		
		delete t
		keep_looping = true
		for i = 1 to 5 while keep_looping do -- keep looping until the delete_count value changes, max of 5 loops
		(	
			windows.processpostedmessages();sleep .1
			windows.processpostedmessages()
			keep_looping = (delete_count == 0)
		)
		assert_false processing_callbacks
		assert_equal 1 add_count
		assert_equal 1 delete_count
		assert_equal 2 all_count

		callbackItem.polling = true -- events shouldn't fire until triggerNodeEventCallback called
	
		t = teapot()
		keep_looping = true
		for i = 1 to 5 while keep_looping do -- keep looping unless the add_count value changes, max of 5 loops
		(	
			windows.processpostedmessages();sleep .1
			windows.processpostedmessages()
			keep_looping = (add_count == 1)
		)
		assert_equal 1 add_count
		triggerNodeEventCallback callbackItem
		keep_looping = true
		for i = 1 to 5 while keep_looping do -- keep looping until the add_count value changes, max of 5 loops
		(	
			windows.processpostedmessages();sleep .1
			windows.processpostedmessages()
			keep_looping = (add_count == 1)
		)
		assert_equal 2 add_count
		assert_equal 1 delete_count
		assert_equal 3 all_count
		
		delete t
		keep_looping = true
		for i = 1 to 5 while keep_looping do -- keep looping unless the delete_count value changes, max of 5 loops
		(	
			windows.processpostedmessages();sleep .1
			windows.processpostedmessages()
			keep_looping = (delete_count == 1)
		)
		assert_equal 1 delete_count
		triggerNodeEventCallback callbackItem
		keep_looping = true
		for i = 1 to 5 while keep_looping do -- keep looping until the delete_count value changes, max of 5 loops
		(	
			windows.processpostedmessages();sleep .1
			windows.processpostedmessages()
			keep_looping = (delete_count == 1)
		)
		assert_equal 2 delete_count
		
		callbackItem.enabled = false
		nodeEventCallbacks = callbackItem = undefined;gc()
	),

	Tests = 
	#(
		test_NodeEventCallback
	)
)

run_test_fixture NodeEventCallback_Test script:(getThisScriptFilename())
