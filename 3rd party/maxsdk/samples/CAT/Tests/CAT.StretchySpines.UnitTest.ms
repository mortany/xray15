-------------------------------------------------------------------------------------------------------------------
-- Test Spine SetValues on Base Human
-- The primary goal of this script is to test stretchy spines
--
-- Major Points Tested:
--   Spine non-scaling Absolute SetValues
--   Spine non-scaling Relative SetValues
--   Spine scaling Absolute SetValues
--   Spine scaling Relative SetValues
--   -- TODO: Finish adding the code to test scale inheritance.
-------------------------------------------------------------------------------------------------------------------

filein "CAT.TestTransformDataFile.ms"
filein "CAT.UtilityFns.ms"
fileIn "MxsUnit/MxsUnit.ms"
CATSpineTestFixture = undefined
struct CATSpineTestFixture
(
private
	rigPath = 0,
	
public
	fn setUp =
	(
		-- load rigs from beside this file
		rigPath = getdir #scripts + ("\\automxs\\include\\CAT_Files\\") --GetTestingDataDir()
	),

	fn TestStdSpineSetValue =
	(
		cp = CATParent()
		cp.catunits = 0.3
		cp.transform = (matrix3 [-1.62921e-007,0,1] [0,1,0] [-1,0,-1.62921e-007] [0,0,0])
		cp.LoadRig (rigPath + "BaseHumanStdSpine.rig")

		pelvisTransform = $BaseHumanStdSpinePelvis.transform
		ribcageTransform = $BaseHumanStdSpineRibcage.transform
		cp.AppendLayer "NewLayer" #Absolute 
		cp.CATMode = 1

		assert_true (IsMtxEquivalent $BaseHumanStdSpinePelvis.transform pelvisTransform)
		assert_true (IsMtxEquivalent $BaseHumanStdSpineRibcage.transform ribcageTransform)

		-- Test ABS setvalue, without stretch, in range
		ribcageTransform = (matrix3 [0.282188,0.906918,0.312842] [-0.920491,0.164074,0.35465] [0.27031,-0.388046,0.881109] [-10.8988,-5.13042,54.4162])
		$BaseHumanStdSpineRibcage.transform = ribcageTransform
		assert_true (IsMtxEquivalent $BaseHumanStdSpineRibcage.transform ribcageTransform)

		-- ABS SetValue, without stretch, out of range.
		ribcageTransform = (matrix3 [-0.475893,-0.717794,0.508231] [-0.867924,0.28982,-0.403375] [0.142245,-0.633069,-0.760914] [-10.3274,-22.4426,68.9264])
		$BaseHumanStdSpineRibcage.transform = ribcageTransform
		assert_true (IsMtxEquivalent $BaseHumanStdSpineRibcage.transform (matrix3 [-0.475893,-0.717794,0.508231] [-0.867925,0.28982,-0.403376] [0.142245,-0.633069,-0.760914] [-5.16183,-12.7257,53.8507]))

		-- Relative set value
		toolMode.coordsys #local
		rotate $BaseHumanStdSpineRibcage (angleaxis -81.686 [0.773102,0.0808327,-0.62911])
		move $BaseHumanStdSpineRibcage[0,-16.9188,17.277]
		assert_true (IsMtxEquivalent $BaseHumanStdSpineRibcage.transform (matrix3 [-0.155757,-0.0628084,0.985797] [-0.534074,-0.834176,-0.137533] [0.830966,-0.54791,0.0963847] [-0.314648,-11.8276,57.1597]))

		-- Laod the stretchy version
		delete cp
	),
	
	fn TestStretchySpineSetValue = 
	(
		cp = CATParent()
		cp.catunits = 0.3
		cp.transform = (matrix3 [-1.62921e-007,0,1] [0,1,0] [-1,0,-1.62921e-007] [0,0,0])
		cp.LoadRig (rigPath + "BaseHumanStretchySpine.rig")

		pelvisTransform = $BaseHumanStretchySpinePelvis.transform
		ribcageTransform = $BaseHumanStretchySpineRibcage.transform

		cp.AppendLayer "NewLayer" #Absolute 
		cp.CATMode = 1

		assert_true (IsMtxEquivalent $BaseHumanStretchySpinePelvis.transform pelvisTransform)
		assert_true (IsMtxEquivalent $BaseHumanStretchySpineRibcage.transform ribcageTransform)

		ribcageTransform = (matrix3 [0.282188,0.906918,0.312842] [-0.920491,0.164074,0.35465] [0.27031,-0.388046,0.881109] [-10.8988,-5.13042,54.4162])
		$BaseHumanStretchySpineRibcage.transform = ribcageTransform
		assert_true (IsMtxEquivalent $BaseHumanStretchySpineRibcage.transform ribcageTransform)

		-- ABS SetValue, without stretch, out of range.
		ribcageTransform = (matrix3 [-0.475893,-0.717794,0.508231] [-0.867924,0.28982,-0.403375] [0.142245,-0.633069,-0.760914] [-10.3274,-22.4426,68.9264])
		$BaseHumanStretchySpineRibcage.transform = ribcageTransform
		assert_true (IsMtxEquivalent $BaseHumanStretchySpineRibcage.transform ribcageTransform)

		-- Relative set value
		toolMode.coordsys #local

		spineScale = $BaseHumanStretchySpineSpine3.transform.scale
		rotate $BaseHumanStretchySpineRibcage (angleaxis -81.686 [0.773102,0.0808327,-0.62911])
		assert_true (IsVecEquivalent $BaseHumanStretchySpineSpine3.transform.scale spineScale) -- rotate shouldn't cause stretch

		spineScale = $BaseHumanStretchySpineSpine3.transform.scale
		move $BaseHumanStretchySpineRibcage[0,-16.9188,17.277]
		assert_true (IsMtxEquivalent $BaseHumanStretchySpineRibcage.transform (matrix3 [-0.155758,-0.0628079,0.985798] [-0.534074,-0.834176,-0.137532] [0.830966,-0.54791,0.096385] [1.21405,-26.8091,96.8193]))

		-- Finally, just verify we have proper scale on the spine
		assert_false (IsVecEquivalent $BaseHumanStretchySpineSpine3.transform.scale spineScale) -- move will cause stretch
		assert_true (IsVecEquivalent $BaseHumanStretchySpineSpine3.transform.scale [3.07143, 1, 1])

		delete cp
	),
	
	fn TestStdSpineScaling= 
	(
		---- Test Scaling
		cp = CATParent()
		cp.catunits = 0.3
		cp.transform = (matrix3 [-1.62921e-007,0,1] [0,1,0] [-1,0,-1.62921e-007] [0,0,0])
		cp.LoadRig (rigPath + "BaseHumanStdSpine.rig")

		pelvisTransform = $BaseHumanStdSpinePelvis.transform
		ribcageTransform = $BaseHumanStdSpineRibcage.transform
		cp.AppendLayer "NewLayer" #Absolute 
		cp.CATMode = 1

		toolMode.coordsys #local
		scale $BaseHumanStdSpinePelvis [2.75236,1,2.75236]

		-- Was the scale inheritted all the way up the spine?
		assert_true (IsVecEquivalent $BaseHumanStdSpineSpine3.transform.scale $BaseHumanStdSpinePelvis.transform.scale)
		assert_true (IsVecEquivalent $BaseHumanStdSpineRibcage.transform.scale $BaseHumanStdSpinePelvis.transform.scale)

		assert_true ((IsMtxEquivalent $BaseHumanStdSpineRibcage.transform (matrix3 [-3.27382e-007,0,2.75236] [0,1,0] [-2.75236,-3.28107e-007,-3.28107e-007] [-1.35253e-005,-1.78891,99.1905])))

		-- Now, remove the scale from this
		mtx = $BaseHumanStdSpineRibcage.transform
		trans = mtx.Translation 
		mtx = mtx.Rotation as matrix3
		mtx.translation = trans
		$BaseHumanStdSpineRibcage.transform = mtx
		
		-- Check that the spine has the right scale interpolated from base to tip
		assert_true (IsMtxEquivalent $BaseHumanStdSpineRibcage.transform (matrix3 [0,-0.00318201,1.00001] [-1.19373e-007,0.999888,0.00318162] [-1,-1.19588e-007,0] [-8.16966e-006,-0.137807,80.7269]))
		assert_true (IsVecEquivalent $BaseHumanStdSpineSpine4.transform.scale [1.11224,0.999901,1.11223])
		assert_true (IsVecEquivalent $BaseHumanStdSpineSpine3.transform.scale [1.67487,0.999935,1.67486])
		assert_true (IsVecEquivalent $BaseHumanStdSpineSpine2.transform.scale [2.24322,0.999969,2.24321])
		assert_true (IsVecEquivalent $BaseHumanStdSpineSpine1.transform.scale [2.67703,0.999996,2.67703])

		delete cp
	),
	
	fn endall =
	(
		resetMaxFile #noprompt 
	),
	
	-- The tests must be performed in this order
	tests = 
	#(
		setUp
		, TestStdSpineSetValue
		, TestStretchySpineSetValue
		, TestStdSpineScaling
		,endall
	)
)

run_test_fixture CATSpineTestFixture script:(getThisScriptFilename())