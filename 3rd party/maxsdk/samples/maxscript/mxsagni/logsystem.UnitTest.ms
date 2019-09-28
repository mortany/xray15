fileIn "MxsUnit/MxsUnit.ms"

struct logsystem_Test
(
	logTypes = #(#error, #warning, #info, #debug),
	testLogFileName = @"$temp\logsystem_Test.log",
	origLogTypeVals,
	origLogFileName,

	fn setup = 
	(
		origLogTypeVals = for t in logTypes collect (logsystem.getLogLevel t)
		origLogFileName = logsystem.getLogName()
	),
	fn teardown = 
	(	
		local i = 0
		for t in logTypes do (logsystem.setLogLevel t (origLogTypeVals[i+=1]))
		deleteFile testLogFileName
		logsystem.logName origLogFileName
	),
	fn countFileLines fname = 
	(
		if not (doesfileexist fname) then 0
		else
		(
			local f = openfile fname
			local n = 0
			while not (eof f) do (readline f; n+=1)
			close f
			n
		)
	),
	fn test_methods =
	(
		-- all off
		logsystem.setLogLevel error:false warning:false info:false debug:false
		for t in logTypes do assert_false (logsystem.getLogLevel t)
		
		-- one at a time on, specifying all settings
		logsystem.setLogLevel error:true warning:false info:false debug:false
		assert_true (logsystem.getLogLevel #error)
		for t in logTypes where t != #error do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:false warning:true info:false debug:false
		assert_true (logsystem.getLogLevel #warning)
		for t in logTypes where t != #warning do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:false warning:false info:true debug:false
		assert_true (logsystem.getLogLevel #info)
		for t in logTypes where t != #info do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:false warning:false info:false debug:true
		assert_true (logsystem.getLogLevel #debug)
		for t in logTypes where t != #debug do assert_false (logsystem.getLogLevel t)

		-- two on at a time
		logsystem.setLogLevel error:true warning:true info:false debug:false
		for t in logTypes where t == #error or t ==#warning do assert_true (logsystem.getLogLevel t)
		for t in logTypes where t != #error and t !=#warning do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:true warning:false info:true debug:false
		for t in logTypes where t == #error or t ==#info do assert_true (logsystem.getLogLevel t)
		for t in logTypes where t != #error and t !=#info do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:true warning:false info:false debug:true
		for t in logTypes where t == #error or t ==#debug do assert_true (logsystem.getLogLevel t)
		for t in logTypes where t != #error and t !=#debug do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:false warning:true info:false debug:true
		for t in logTypes where t == #warning or t ==#debug do assert_true (logsystem.getLogLevel t)
		for t in logTypes where t != #warning and t !=#debug do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:false warning:false info:true debug:true
		for t in logTypes where t == #info or t ==#debug do assert_true (logsystem.getLogLevel t)
		for t in logTypes where t != #info and t !=#debug do assert_false (logsystem.getLogLevel t)

		-- all on
		logsystem.setLogLevel error:true warning:true info:true debug:true
		for t in logTypes do assert_true (logsystem.getLogLevel t)

		-- one at a time on, specifying only the 1 setting as keyword arg
		logsystem.setLogLevel error:false warning:false info:false debug:false
		logsystem.setLogLevel error:true
		assert_true (logsystem.getLogLevel #error)
		for t in logTypes where t != #error do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:false warning:false info:false debug:false
		logsystem.setLogLevel warning:true
		assert_true (logsystem.getLogLevel #warning)
		for t in logTypes where t != #warning do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:false warning:false info:false debug:false
		logsystem.setLogLevel info:true
		assert_true (logsystem.getLogLevel #info)
		for t in logTypes where t != #info do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:false warning:false info:false debug:false
		logsystem.setLogLevel debug:true
		assert_true (logsystem.getLogLevel #debug)
		for t in logTypes where t != #debug do assert_false (logsystem.getLogLevel t)

		-- one at a time off, specifying only the 1 setting as keyword arg
		logsystem.setLogLevel error:true warning:true info:true debug:true
		logsystem.setLogLevel error:false
		assert_false (logsystem.getLogLevel #error)
		for t in logTypes where t != #error do assert_true (logsystem.getLogLevel t)
		logsystem.setLogLevel error:true warning:true info:true debug:true
		logsystem.setLogLevel warning:false
		assert_false (logsystem.getLogLevel #warning)
		for t in logTypes where t != #warning do assert_true (logsystem.getLogLevel t)
		logsystem.setLogLevel error:true warning:true info:true debug:true
		logsystem.setLogLevel info:false
		assert_false (logsystem.getLogLevel #info)
		for t in logTypes where t != #info do assert_true (logsystem.getLogLevel t)
		logsystem.setLogLevel error:true warning:true info:true debug:true
		logsystem.setLogLevel debug:false
		assert_false (logsystem.getLogLevel #debug)
		for t in logTypes where t != #debug do assert_true (logsystem.getLogLevel t)

		-- one at a time on, specifying only the 1 setting using <enum> <bool> arg syntax
		logsystem.setLogLevel error:false warning:false info:false debug:false
		logsystem.setLogLevel #error true
		assert_true (logsystem.getLogLevel #error)
		for t in logTypes where t != #error do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:false warning:false info:false debug:false
		logsystem.setLogLevel #warning true
		assert_true (logsystem.getLogLevel #warning)
		for t in logTypes where t != #warning do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:false warning:false info:false debug:false
		logsystem.setLogLevel #info true
		assert_true (logsystem.getLogLevel #info)
		for t in logTypes where t != #info do assert_false (logsystem.getLogLevel t)
		logsystem.setLogLevel error:false warning:false info:false debug:false
		logsystem.setLogLevel #debug true
		assert_true (logsystem.getLogLevel #debug)
		for t in logTypes where t != #debug do assert_false (logsystem.getLogLevel t)

		-- one at a time off, specifying only the 1 setting using <enum> <bool> arg syntax
		logsystem.setLogLevel error:true warning:true info:true debug:true
		logsystem.setLogLevel #error false
		assert_false (logsystem.getLogLevel #error)
		for t in logTypes where t != #error do assert_true (logsystem.getLogLevel t)
		logsystem.setLogLevel error:true warning:true info:true debug:true
		logsystem.setLogLevel #warning false
		assert_false (logsystem.getLogLevel #warning)
		for t in logTypes where t != #warning do assert_true (logsystem.getLogLevel t)
		logsystem.setLogLevel error:true warning:true info:true debug:true
		logsystem.setLogLevel #info false
		assert_false (logsystem.getLogLevel #info)
		for t in logTypes where t != #info do assert_true (logsystem.getLogLevel t)
		logsystem.setLogLevel error:true warning:true info:true debug:true
		logsystem.setLogLevel #debug false
		assert_false (logsystem.getLogLevel #debug)
		for t in logTypes where t != #debug do assert_true (logsystem.getLogLevel t)

		-- count number out output lines from logsys when all off
		deleteFile testLogFileName
		logsystem.logName testLogFileName
		logsystem.setLogLevel error:false warning:false info:false debug:false
		for t in logTypes do logsystem.logEntry t t
		logsystem.logEntry #error error:true
		logsystem.logEntry #warning warning:true
		logsystem.logEntry #info info:true
		logsystem.logEntry #debug debug:true
		logsystem.logEntry #debug  -- defaults to debug:true
		assert_equal 0 (countFileLines testLogFileName)
		
		-- count number out output lines from logsys when 1 on
		for t in logTypes where t != #debug do 
		(
		deleteFile testLogFileName
		logsystem.setLogLevel error:false warning:false info:false debug:false
		logsystem.setLogLevel t true
		for t_inner in logTypes do logsystem.logEntry t_inner t_inner
		logsystem.logEntry #error error:true
		logsystem.logEntry #warning warning:true
		logsystem.logEntry #info info:true
		logsystem.logEntry #debug debug:true
		logsystem.logEntry #debug  -- defaults to debug:true
		assert_equal 2 (countFileLines testLogFileName)
		)
		
		-- count number of output lines from logsys when debug on
		deleteFile testLogFileName
		logsystem.setLogLevel error:false warning:false info:false debug:true
		for t in logTypes do logsystem.logEntry t t
		logsystem.logEntry #error error:true
		logsystem.logEntry #warning warning:true
		logsystem.logEntry #info info:true
		logsystem.logEntry #debug debug:true
		logsystem.logEntry #debug  -- defaults to debug:true
		assert_equal 3 (countFileLines testLogFileName)
		deleteFile testLogFileName
		
		-- turn on just info and debug level output, log an entry with higher level severity and info level severity, make sure logged as a info level
		logsystem.setLogLevel error:false warning:false info:true debug:true
		logsystem.logEntry #error error:true info:true
		assert_equal 1 (countFileLines testLogFileName)
		if (countFileLines testLogFileName) == 1 do
		(
			local f = openfile testLogFileName
			local l = readline f
			close f
			assert_equal "INF:" ((filterstring l " ")[3])
		)
		deleteFile testLogFileName
		
		-- count number of output lines from logsys when disabled
		logsystem.setLogLevel error:false warning:false info:true debug:true
		local old_enabled = logsystem.enabled
		logsystem.enabled = false
		logsystem.logEntry #error error:true info:true
		logsystem.enabled = old_enabled
		assert_equal 0 (countFileLines testLogFileName)
		deleteFile testLogFileName
	),
	
	fn test_load_save_state =
	(
		logsystem.loadState()
		local old_logdays = logsystem.logdays
		local old_logSize = logsystem.logSize
		local old_longevity = logsystem.longevity

		logsystem.logdays = old_logdays + 10
		logsystem.logSize =  old_logSize + 10
		logsystem.longevity =  if old_longevity == #size then #forever else #size

		logsystem.SaveState()
		
		logsystem.logdays = old_logdays
		logsystem.logSize =  old_logSize
		logsystem.longevity =  old_longevity

		logsystem.loadState()
		assert_equal (old_logdays + 10) logsystem.logdays message:"logdays not persisted"
		assert_equal (old_logSize + 10) logsystem.logSize message:"logSize not persisted"
		assert_equal (if old_longevity == #size then #forever else #size) logsystem.longevity message:"longevity not persisted"

		logsystem.logdays = old_logdays
		logsystem.logSize =  old_logSize
		logsystem.longevity =  old_longevity
		logsystem.SaveState()
		assert_equal old_logdays logsystem.logdays message:"logdays not restored"
		assert_equal old_logSize logsystem.logSize message:"logSize not restored"
		assert_equal old_longevity logsystem.longevity message:"longevity not restored"
	),

	Tests = 
	#(
		test_methods,
		test_load_save_state
	)
)

run_test_fixture logsystem_Test script:(getThisScriptFilename())
