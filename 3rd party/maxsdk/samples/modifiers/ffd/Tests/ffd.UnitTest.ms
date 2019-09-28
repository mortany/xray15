--##################################################
-- Modifiers FFD Unit Test
-- Author:	 Larry Minton
-- Created:	May 25, 2018
--##################################################

fileIn "MxsUnit/MxsUnit.ms"

struct FFD_UnitTest
(
	--##################################################
	-- Unit tests
	--##################################################
public
	function setup =
	(
		resetMaxFile #noPrompt
	),

	function teardown =
	(
		
	),

	function SpaceFFD_animated_cp_with_modifier_test =
	(
		--Defect MAXX-42020 Crash when assigning some modifiers to animated FFD space warp
		ffdb = SpaceFFDBox()
		animateAll  ffdb
		with animate on at time 100 for p in (getsubanimnames ffdb.baseobject[#master]) do setProperty ffdb p ((getproperty ffdb p)+[10,10,10])
		addmodifier ffdb (bend())
		classof ffdb
		ffdb_c = copy ffdb
		classof ffdb_c
		
		ffdc = spaceFFDCyl radius:50 height:100
		animateAll  ffdc
		with animate on at time 100 for p in (getsubanimnames ffdc.baseobject[#master]) do setProperty ffdc p ((getproperty ffdc p)+[10,10,10])
		addmodifier ffdc (bend())
		classof ffdc
		ffdc_c =copy ffdc
		classof ffdc_c
	),

	Tests =
	#(
		SpaceFFD_animated_cp_with_modifier_test
	)
)

run_test_fixture FFD_UnitTest script:(getThisScriptFilename())	