-- UnitTest_UnwrapPeel.ms
-- This is optional, but if you want your test to be executable by hitting ctrl-e in the
-- editor window, it helps.

fileIn "MxsUnit/MxsUnit.ms"

-- MxsUnit unit test fixtures are written as structs.  
-- The runner uses the return value from calling fileIn on the script file as the
-- test fixture.  For your test to work correctly from the command-line, it MUST
-- return its fixture.  The easiest ways to ensure that the fixture is returned
-- are to have your script file contain only the fixture struct or to end it with a
-- run_test_fixture call as in this example.
struct UnwrapPeelUnitTest
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
   function peelPinTest_MAXX_35040 =
   (		
		Cylinder radius:9.71432 height:36.5804 pos:[-0.407924,0.407934,0] isSelected:on mapcoords:on
		$.heightsegs = 5
		$.capsegs = 1
		$.sides = 18

		max modify mode
		modPanel.addModToSelection (Unwrap_UVW ()) ui:on
		subobjectLevel = 1
		$Cylinder001.modifiers[#unwrap_uvw].unwrap6.selectVerticesByNode #{111..127} $
		$Cylinder001.modifiers[#unwrap_uvw].unwrap6.selectVerticesByNode #{1..19, 111..127} $
		$Cylinder001.modifiers[#unwrap_uvw].unwrap6.selectVerticesByNode #{1..19, 24, 42, 60, 78, 96, 111..127} $

		$Cylinder001.modifiers[#unwrap_uvw].unwrap6.PinSelected $
		$Cylinder001.modifiers[#unwrap_uvw].unwrap6.selectVerticesByNode #{92..95, 97..110} $
		verts = #{92..95, 97..110}	
		vertsArray = verts as array
		numVerts = vertsArray.Count
		initPosArray = #()


		subobjectLevel = 3
		$Cylinder001.modifiers[#unwrap_uvw].unwrap6.selectFacesByNode #{1..216} $

		--peel mode
		$Cylinder001.modifiers[#unwrap_uvw].unwrap6.LSCMInteractive TRUE
			
		--record init pos
		for i = 1 to numVerts do
		(
			index = vertsArray[i]
			p = $Cylinder001.modifiers[#unwrap_uvw].unwrap6.GetVertexPositionByNode 0 index $
			append initPosArray p
		)		

		--record new pos
		subobjectLevel = 1
		$Cylinder001.modifiers[#unwrap_uvw].unwrap6.selectVerticesByNode #{92..95, 97..110} $
		$.modifiers[#unwrap_uvw].unwrap2.MoveSelected [0,0.1,0]

		$Cylinder001.modifiers[#unwrap_uvw].unwrap6.LSCMInteractive FALSE

		-- get our new offset
		-- record the offset they should all be the same so diff at the end should be 0
		initialOffset = 0
		totalChange = 0;
		for i = 1 to numVerts do
		(
			index = vertsArray[i]
			p = $Cylinder001.modifiers[#unwrap_uvw].unwrap6.GetVertexPositionByNode 0 index $
			if (i == 1) then
			(
				initialOffset = Length (p - initPosArray[i])	
				format "% offset %\n" i initialOffset
			)
			else
			(
				offset = Length (p - initPosArray[i])
				format "% offset %\n" i initialOffset				
				totalChange += abs(offset - initialOffset)
			)
			
		)	
		
		format "%\n" totalChange
		assert  (totalChange < 0.001)
     
   ),
   
   -- The tests array.  This publishes the test case functions to the
   -- MxsUnitRunner.  It must be named "tests"

	tests = #(peelPinTest_MAXX_35040)
)

-- Optional, but makes the test immediately executable from the editor window.
-- run_test_fixture is implemented to do nothing if executing this test from the
-- command line runner or from the run_test_script utility method, thus preventing
-- the test from being executed twice.
-- run_test_fixture returns the fixture, so the test will work correctly from
-- the command line runner.
run_test_fixture UnwrapPeelUnitTest


