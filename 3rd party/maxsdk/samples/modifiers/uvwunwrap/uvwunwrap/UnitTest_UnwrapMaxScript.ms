-- UnitTest_UnwrapMaxScript.ms
-- This is optional, but if you want your test to be executable by hitting ctrl-e in the
-- editor window, it helps.
fileIn "MxsUnit/MxsUnit.ms"

-- MxsUnit unit test fixtures are written as structs.  
-- The runner uses the return value from calling fileIn on the script file as the
-- test fixture.  For your test to work correctly from the command-line, it MUST
-- return its fixture.  The easiest ways to ensure that the fixture is returned
-- are to have your script file contain only the fixture struct or to end it with a
-- run_test_fixture call as in this example.
struct UnwrapMaxScriptUnitTest
(
   
   -- An optional set up function to set up the scene or local data for testing.
   -- setUp is called before every test case function.
   function setUp =
   (

   ),
   
   
   -- An optional tearDown function to clear any resources.
   -- tearDown is called after every test case function.
   function tearDown =
   (
--		resetMaxFile #noprompt
	   
   ),
   
   -- A test case function
   function maxScriptTest =
   (
		resetMaxFile #noprompt
		max create mode
		Sphere radius:23.9243 pos:[-14.5367,17.6242,0] isSelected:on
		Cylinder radius:15.4321 height:28.1418 pos:[6.28769,-4.23456,0] isSelected:on
		Torus pos:[-6.02191,-24.678,0] isSelected:on radius1:13.637 radius2:6.63233
		select #($Cylinder001, $Torus001, $Sphere001)
		max modify mode
		modPanel.addModToSelection (Unwrap_UVW ()) ui:on
		assert_true ($Sphere001.modifiers[#unwrap_uvw].showTileGridlines == true)
		assert_true ($Sphere001.modifiers[#unwrap_uvw].displayPixelUnits == false)
		subobjectLevel = 1
		$Sphere001.modifiers[#unwrap_uvw].showTileGridlines = false
		$Sphere001.modifiers[#unwrap_uvw].displayPixelUnits = true
		$Sphere001.modifiers[#unwrap_uvw].edit()
		$Sphere001.modifiers[#unwrap_uvw].unwrap4.SetRelativeTypeIn off
		$Sphere001.modifiers[#unwrap_uvw].selectVertices #{100}
		windows.processPostedMessages()
		sleep 1
		assert_true ($Sphere001.modifiers[#unwrap_uvw].showTileGridlines == false)
		assert_true ($Sphere001.modifiers[#unwrap_uvw].displayPixelUnits == true)
		$Sphere001.modifiers[#unwrap_uvw].closeEditor()
		$Sphere001.modifiers[#unwrap_uvw].edit()
		$Sphere001.modifiers[#unwrap_uvw].showTileGridlines = true
		$Sphere001.modifiers[#unwrap_uvw].displayPixelUnits = false
		$Sphere001.modifiers[#unwrap_uvw].closeEditor()     
		windows.processPostedMessages()
		sleep 1
		assert_true ($Sphere001.modifiers[#unwrap_uvw].showTileGridlines == true)
		assert_true ($Sphere001.modifiers[#unwrap_uvw].displayPixelUnits == false)
   ),
   
   -- The tests array.  This publishes the test case functions to the
   -- MxsUnitRunner.  It must be named "tests"

	tests = #(maxScriptTest)
)

-- Optional, but makes the test immediately executable from the editor window.
-- run_test_fixture is implemented to do nothing if executing this test from the
-- command line runner or from the run_test_script utility method, thus preventing
-- the test from being executed twice.
-- run_test_fixture returns the fixture, so the test will work correctly from
-- the command line runner.
run_test_fixture UnwrapMaxScriptUnitTest


