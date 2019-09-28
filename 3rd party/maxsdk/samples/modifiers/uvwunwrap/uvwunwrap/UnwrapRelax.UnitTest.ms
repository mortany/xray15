-- UnitTest_UnwrapRelax.ms
-- This is optional, but if you want your test to be executable by hitting ctrl-e in the
-- editor window, it helps.
fileIn "MxsUnit/MxsUnit.ms"

-- MxsUnit unit test fixtures are written as structs.  
-- The runner uses the return value from calling fileIn on the script file as the
-- test fixture.  For your test to work correctly from the command-line, it MUST
-- return its fixture.  The easiest ways to ensure that the fixture is returned
-- are to have your script file contain only the fixture struct or to end it with a
-- run_test_fixture call as in this example.
struct UnwrapRelaxedUnitTest
(
   
   -- An optional set up function to set up the scene or local data for testing.
   -- setUp is called before every test case function.
   function setUp =
   (
		resetMaxFile #noprompt
   ),
   
   
   -- An optional tearDown function to clear any resources.
   -- tearDown is called after every test case function.
   function tearDown =
   (
--		resetMaxFile #noprompt
	   
   ),
   
   -- A test case function
   function relaxThreadedSelection =
   (		
		max create mode
		Sphere radius:23.9243 pos:[-14.5367,17.6242,0] isSelected:on
		Cylinder radius:15.4321 height:28.1418 pos:[6.28769,-4.23456,0] isSelected:on
		Torus pos:[-6.02191,-24.678,0] isSelected:on radius1:13.637 radius2:6.63233
		select #($Cylinder001, $Torus001, $Sphere001)
		max modify mode
		modPanel.addModToSelection (Unwrap_UVW ()) ui:on
		subobjectLevel = 1
		subobjectLevel = 3
		$Sphere001.modifiers[#unwrap_uvw].unwrap6.selectFacesByNode #{1..148, 165..206, 231..268, 297..330, 361..394, 427..456, 491..520, 557..582, 623..644, 689..706, 757..766} $Sphere001
		$Torus001.modifiers[#unwrap_uvw].unwrap6.selectFacesByNode #{5..16, 29..40, 53..64, 77..88, 101..112, 125..136, 149..160, 171..182, 195..206, 217..228, 241..250, 263..274, 287..296, 309..320, 333..344, 357..368, 381..392, 405..416, 429..440, 455..466, 479..492, 505..516, 531..542, 557..566} $Torus001
		$Cylinder001.modifiers[#unwrap_uvw].unwrap6.selectFacesByNode #{33..48, 69..84, 105..120, 141..156, 177..192, 199..216} $Cylinder001
		$Sphere001.modifiers[#unwrap_uvw].unwrap6.relaxThreaded #start --start
		sleep 2
		$Sphere001.modifiers[#unwrap_uvw].unwrap6.relaxThreaded #restart --restart
		sleep 2
		$Sphere001.modifiers[#unwrap_uvw].unwrap6.relaxThreaded #restart  --this is the race condition where it trie to close and restart relax at the same time
		$Sphere001.modifiers[#unwrap_uvw].unwrap6.relaxThreaded #stop	
     
   ),
	function peltRelax_MAXX_41185 =
   (
	   Sphere radius:32.8729 pos:[0.215733,1.07865,0] isSelected:on;
		max modify mode
		modPanel.addModToSelection (Unwrap_UVW ()) ui:on
		$Sphere001.modifiers[#unwrap_uvw].unwrap6.selectFacesByNode #{131..136, 191..204, 255..270, 317..332, 381..394, 445..456, 509..520, 573..582, 637..644, 703..706} $
		$Sphere001.modifiers[#unwrap_uvw].edit()
		actionMan.executeAction 2077580866 "40144"  -- Unwrap UVW: Pelt Map
		subobjectLevel = 1
		actionMan.executeAction 2077580866 "-25386"  -- Unwrap UVW: Pelt Dialog Select Stretcher
		actionMan.executeAction 2077580866 "40157"  -- Unwrap UVW: Pelt Dialog Relax Simulation Heavy
		$Sphere001.modifiers[#unwrap_uvw].unwrap6.relaxThreaded #start --start
		sleep 2
		$Sphere001.modifiers[#unwrap_uvw].unwrap6.relaxThreaded #restart --restart
		sleep 2
		$Sphere001.modifiers[#unwrap_uvw].unwrap6.relaxThreaded #restart  --this is the race condition where it trie to close and restart relax at the same time
		$Sphere001.modifiers[#unwrap_uvw].unwrap6.relaxThreaded #stop				
   ),
   -- The tests array.  This publishes the test case functions to the
   -- MxsUnitRunner.  It must be named "tests"

	tests = #(relaxThreadedSelection,peltRelax_MAXX_41185)
)

-- Optional, but makes the test immediately executable from the editor window.
-- run_test_fixture is implemented to do nothing if executing this test from the
-- command line runner or from the run_test_script utility method, thus preventing
-- the test from being executed twice.
-- run_test_fixture returns the fixture, so the test will work correctly from
-- the command line runner.
run_test_fixture UnwrapRelaxedUnitTest


