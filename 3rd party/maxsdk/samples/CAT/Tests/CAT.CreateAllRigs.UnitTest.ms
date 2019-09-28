-------------------------------------------------------------------------------------------------------------------
-- Test Create All CAT Rigs
-- Basic sanity checking - can we create all rigs, and does the transform for each
-- bone on the rig match the transform from Max 2010?  This ensures that the most
-- basic level a CAT rig will be created, and that adding an Absolute layer accurately
-- bakes transform data to match the setup pose with all default values.
--
-- Major Points Tested:
--   CAT Rig Creation
--   Absolute SetValues
-------------------------------------------------------------------------------------------------------------------

filein "CAT.TestTransformDataFile.ms"
filein "CAT.UtilityFns.ms"
fileIn "MxsUnit/MxsUnit.ms"

CATTestCreateAllRigs = undefined
struct CATTestCreateAllRigs
(
public
	fn setUp =
	(
	),

	fn TestCATRig catrig rigFilename lengthAxis = 
	(
		Note("Testing Setup: " + catrig.name + " in lengthAxis: " + lengthAxis)
		dataDir = "AllRigs" + lengthAxis
		origDataFile = GetTestingDataFile dataDir rigFilename
		res = TestCATRigTransformations catrig origDataFile
		assert_true(res)
		if (res == false) then
			Error ("File Not Found : " + origDataFile)
		
		-- Test adding a absolute layer.
		catrig.AppendLayer "TestLayer" #ABSOLUTE
		catrig.CATMode = 1
		forceCompleteRedraw()
		
		-- When testing the animation settings, we give a slightly larger margin of error.
		-- This allows for the additional errors that will be generated in the SetValue phase.
		Note("Testing Anim: " + catrig.name + " in lengthAxis: " + lengthAxis)
		res = TestCATRigTransformations catrig origDataFile margin:0.0002
		assert_true(res)
		if (res == false) then
			Error ("File Not Found : " + origDataFile)

		-- TODO: Test drifting?
		-- Test that SetValues do not drift.
		--Note("Testing SetValue Drift: " + catrig.name + " in lengthAxis: " + lengthAxis)
		--TestCATSetValueDrift catrig
		
		-- Reset back to setupmode
		catrig.RemoveLayer 1
		assert_true(catrig.CATMode == 0)
	),
	
	fn TestAllCATRigsTransformHierarchy =
	(
		rigdir = GetRigDir()
		
		files = getAllRigFilesRecursive rigdir
	
		for f in files do (
			rigFilename = (filenameFromPath f)
			print ("Testing: " + rigFilename)
		
			-- Create the rig
			catrig = LoadRig f
			forceCompleteRedraw()

			-- Test the rig in X length axis form
			TestCATRig catrig rigFilename "X"
				
			-- Test the rig in Z length axis form
			TestCATRig catrig rigFilename "Z"

			resetMaxFile #noprompt 
			break
		)
	),
	
	fn endall =
	(
	),
	
	-- The tests must be performed in this order
	tests = 
	#(
		setUp
		, TestAllCATRigsTransformHierarchy
		,endall
	)
)

run_test_fixture CATTestCreateAllRigs script:(getThisScriptFilename())	
