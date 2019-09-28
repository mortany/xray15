----------------------------------------------------------------------------------------------------
-- Test whether the CAT transform stack matches in set and get
-- under different inheritance combinations as well as whether
-- the child obeys the inheritance options while transforming
-- the parent.
----------------------------------------------------------------------------------------------------

filein "CAT.TestTransformDataFile.ms"
fileIn "MxsUnit/MxsUnit.ms"

struct CATTestInheritance
(
private

	mTestRoot = undefined,
	mTestBone = undefined,
	mTestNode = undefined,
	mTestParent = undefined,

public

	fn reset =
	(
		resetMaxFile #noprompt
	),

	fn createTestRig =
	(
		reset()

		-- Create a chest and an arm
		mTestRoot = CATParent()
		mTestRoot.transform = matrix3 [0,0,1] [0,-1,0] [1,0,0] [0,0,0]
		mTestRoot.CATUnits = 1.0
		mTestRoot.AddHub()
		mTestRoot.RootHub.AddArm()

		-- Transform some body parts
		mTestParent = mTestRoot.RootHub.Node
		scale mTestParent [1.1,1.2,1.3]
		mTestBone = mTestRoot.RootHub.Limbs[1].Bones[1]
		mTestNode = mTestBone.Node
		scale mTestNode [1.1,1.2,1.3]
		move mTestNode [10,-5,5]
		rotate mTestNode (angleaxis -15 [0,0,1])
	),

	fn IsPoint3Equivalent a b errorMargin:0.005 =
	(
		eq = false
		diff = a - b
		if ((abs(diff.x) < errorMargin) and \
			(abs(diff.y) < errorMargin) and \
			(abs(diff.z) < errorMargin)) then
			eq = true
		eq
	),

	fn testMovement origTM inheritPos inheritRot inheritScale =
	(
		if (not mTestBone.RelativeToSetup) do
		(
			undo on
			(
				-- Verify position inheritance behavior
				offset = [10,0,0]
				move mTestParent offset

				-- The bone should move if inherits position otherwise remain still
				if (inheritPos) then origTM.pos += offset
				assert_true (IsMtxEquivalent origTM mTestNode.transform)
			)
			max undo

			undo on
			(
				-- Verify rotation inheritance behavior
				rotAngle = 90
				rotate mTestParent (angleaxis rotAngle [0,0,1])

				-- The bone should rotate if inherits rotation othersie remain still
				origTM.pos = [0,0,0]
				if (inheritRot) then
				(
					rotMtx = (rotateZMatrix rotAngle)
					origTM *= rotMtx
				)
				boneTM = mTestNode.transform
				boneTM.pos = [0,0,0]
				assert_true (IsMtxEquivalent origTM boneTM)
			)
			max undo

			undo on
			(
				-- Verify scale inheritance behavior
				scaleFactor = [2,2,2]
				scale mTestParent scaleFactor

				-- The bone should scale if inherits scale otherwise remain unit scale
				childScale = mTestNode.transform.scale
				if (inheritScale) then
					assert_true (IsPoint3Equivalent childScale scaleFactor)
				else assert_true (IsPoint3Equivalent childScale [1,1,1])
			)
			max undo
		)
	),

	fn testSetupModeInheritance inheritance =
	(
		-- Vertify the bone transform after inheritance change
		tm = mTestNode.transform
		mTestBone.SetupModeInheritance = inheritance
		assert_true (IsMtxEquivalent tm mTestNode.transform)

		-- Vertify the equality of set/get
		tm = mTestNode.transform
		mTestNode.transform = mTestNode.transform
		assert_true (IsMtxEquivalent tm mTestNode.transform)

		testMovement tm inheritance[1] inheritance[2] false
	),

	fn testAllSetupModeInheritance =
	(
		testSetupModeInheritance #{3} -- Inherit none
		testSetupModeInheritance #{1,3} -- Inherit position
		testSetupModeInheritance #{2,3} -- Inherit rotation
		testSetupModeInheritance #{1..2} -- Inherit both
	),

	fn testAnimationModeInheritance inheritance =
	(
		-- Vertify the bone transform after inheritance change
		tm = mTestNode.transform
		mTestBone.AnimationModeInheritance = inheritance
		assert_true (IsMtxEquivalent tm mTestNode.transform)

		-- Vertify the equality of set/get
		tm = mTestNode.transform
		mTestNode.transform = mTestNode.transform
		assert_true (IsMtxEquivalent tm mTestNode.transform)

		testMovement tm inheritance[1] inheritance[2] inheritance[3]
	),

	fn testAllAnimationModeInheritance =
	(
		testAnimationModeInheritance #{4} -- Inherit none
		testAnimationModeInheritance #{1,4} -- Inherit position
		testAnimationModeInheritance #{2,4} -- Inherit rotation
		testAnimationModeInheritance #{3,4} -- Inherit scale
		testAnimationModeInheritance #{1..2,4} -- Inherit position and rotation
		testAnimationModeInheritance #{2..3,4} -- Inherit rotation and scale
		testAnimationModeInheritance #{1,3} -- Inherit position and scale
		testAnimationModeInheritance #{1..3} -- Inherit all
	),

	fn testSetupMode =
	(
		createTestRig()

		-- Setup Mode + Static Matrix
		testAllSetupModeInheritance()

		-- Setup Mode + Animation Controller
		mTestBone.UseSetupController = true
		testAllSetupModeInheritance()

		-- Setup Mode + Static Matrix + ASTP
		mTestBone.UseSetupController = false
		mTestBone.RelativeToSetup = true
		testAllSetupModeInheritance()

		-- Setup Mode + Animation Controller + ATSP
		mTestBone.UseSetupController = true
		testAllSetupModeInheritance()
	),

	fn testAnimationMode =
	(
		createTestRig()

		-- Animation Mode
		mTestRoot.AppendLayer "Animation Layer" #Absolute
		mTestRoot.CATMode = 1
		testAllAnimationModeInheritance()

		-- Animation Mode + ATSP + Static Matix
		mTestBone.RelativeToSetup = true
		testAllAnimationModeInheritance()

		-- Animation Mode + ATSP + Animation Controller
		mTestBone.UseSetupController = true
		testAllAnimationModeInheritance()
	),

	fn testCATUnits =
	(
		createTestRig()
		trans1 = mTestNode.transform[4]

		-- Remove the collarbone so that the layer control of the bone will have non-zero translation
		mTestRoot.RootHub.Limbs[1].RemoveCollarbone()

		-- Setup Mode
		scaleFactor = 2
		mTestRoot.CATUnits = scaleFactor
		trans2 = mTestNode.transform[4]
		assert_true (IsPoint3Equivalent (trans1 * scaleFactor) trans2)

		-- Animation Mode
		mTestRoot.AppendLayer "Animation Layer" #Absolute
		mTestRoot.CATMode = 1

		scaleFactor = 3
		mTestRoot.CATUnits = scaleFactor
		trans3 = mTestNode.transform[4]
		assert_true (IsPoint3Equivalent (trans1 * scaleFactor) trans3)
	),

	tests = #(
		testSetupMode,
		testAnimationMode,
		testCATUnits,
		reset
		)
)

run_test_fixture CATTestInheritance script:(getThisScriptFilename())