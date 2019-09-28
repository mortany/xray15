-------------------------------------------------------------------------------------------------------------------
-- The point of this file is to validate removing body parts with a CATMotion layer assigned.
-- It validates that the item is removed correctly, if the CATMotion layer is updated
-- and that the appropriate pointers are removed from the rig structure.  It also 
-- checks undo/redo for each action.
-------------------------------------------------------------------------------------------------------------------

fileIn "MxsUnit/MxsUnit.ms"
CATTestFixture = undefined
struct CATTestFixture
(

private
	cp = 0, -- this will become a catParent
	catmotionroot = 0,
	pelvis = 0,
	pelvisBranch = 0,
	
	fn DoUndo = ( max undo),
	
	fn DoRedo = ( actionMan.executeAction 0 "40016" ), -- Undo
	
	fn ReCreateCATMotionLayer = (
		cp.RemoveLayer 1
		cp.AppendLayer "CATMotion" #CATMotion
		catmotionroot = cp.controller.layers.controller[1].CAThierarchyRoot.controller
		catmotionroot
	),

	fn DoFlushUndos = (
		clearUndoBuffer()
		-- Trigger a complete charater evaluation
		cp.CATMode = 1
		SliderTime = slidertime + 1
		cp.CATMode = 0
	),

	fn TestLimbHierarchy limbBranch limb = (
		
		if (limb.IsArm) then (
			assert_equal limbBranch[1].Name "Bend"
		) else (
			assert_equal limbBranch[1].Name "KneeAngle"
		)
		assert_equal limbBranch[2].Name "Twist"
		assert_equal limbBranch[3].Name "CrossSwing"
		assert_equal limbBranch[4].Name "Swing"
		assert_equal limbBranch[5].Name "OffsetRot"
		assert_equal limbBranch[6].Name "LegWeight"
		assert_equal limbBranch[7].Name "LiftPlantMod"
		
		v = 8
		if (limb.Collarbone != undefined) do ( assert_equal limbBranch[v].Name "CollarBone"; v = v+1; )
		if (limb.Palm != undefined) do (
			if (limb.IsArm) then (assert_equal limbBranch[v].Name "Palm"; v = v + 1 )
			else ( assert_equal limbBranch[v].Name "Ankle"; v = v + 1 )
		)
		if (limb.IsLeg) do (
			if (limb.iktarget != undefined) do ( assert_equal limbBranch[v].Name "FootPlatform"; v = v + 1 )
		)
	),
	
public
	leg= 0,
	legBranch = 0,
	
	fn setUp =
	(
		-- Create our CATParent
		while theHold.Holding() do ( theHold.Accept("SomeString"))
	),
	
	fn tearDown =
	(
--		break()
	),

	fn testHub = (
		cp = CATParent()
		cp.catunits = 0.3
		cp.AddHub()
		ReCreateCATMotionLayer()
		
		-- Sanity - do we have a layer for the hub?
		assert_equal catmotionroot.Branches.count 3
		assert_equal catmotionroot.BranchNames.count 3
		assert_equal catmotionroot.BranchNames[3]	"Group"
			
		-- First Test, can we delete the hub?
		theHold.Begin()
		delete cp.RootHub.Node
		theHold.Accept("DeleteNode")

		-- test - has the CATHierarchyBranch for the hub been removed?
		assert_equal catmotionroot.Branches.count 2
		
		DoUndo()
		assert_equal catmotionroot.Branches.count 3
		
		DoRedo()
		assert_equal catmotionroot.Branches.count 2
	
		DoFlushUndos()
	),
	
	fn testCreateSingleArmLegTail = (
		cp.AddHub()
		pelvis = cp.RootHub
		pelvis.AddArm()
		pelvis.AddLeg()
		pelvis.AddTail()
		ReCreateCATMotionLayer()

		-- Do we have phases for the arm and leg?
		assert_equal catmotionroot[3].Name "LimbPhases"
		assert_equal catmotionroot[3].NumSubs 2

		-- Get our pelvis branch
		pelvisBranch = catmotionroot[4]
		assert_equal pelvisBranch.Name (pelvis.Name + "Group")
		assert_equal pelvisBranch.NumSubs 4
	),
	
	fn testTail = (
		tail = pelvis.Tails[1]
		tailBranch = pelvisBranch[4]
		
		-- Test the tail first.
		assert_equal  tailBranch.Name "Tail"
		
		-- this deletes a number of bones, but not the tail itself.  Nothing should change
		tail.numBones = 2
		assert_equal  pelvisBranch.NumSubs 4
		assert_equal  pelvisBranch[4].Name "Tail"
		
		-- Now, delete the tail from the system
		theHold.Begin()
		delete tail.bones[1].node
		theHold.Accept("DeleteBone")
		
		assert_equal  pelvisBranch.NumSubs 3
		DoUndo()
		assert_equal  pelvisBranch.NumSubs 4
		assert_equal  pelvisBranch[4].Name "Tail"
		DoRedo()
		assert_equal  pelvisBranch.NumSubs 3

		DoFlushUndos()
	),
	
	fn testArm = (
		armBranch = pelvisBranch[2]
		arm = pelvis.limbs[1]
		
		-- Now, test arms
		assert_equal armBranch.Name "Arms"
		assert_true arm.IsArm

		TestLimbHierarchy armBranch arm
		
		assert_equal armBranch[8].Name "CollarBone"
		assert_equal armBranch[9].Name "Palm"
		-- try deleting the palm

		theHold.Begin()
		delete arm.Palm.Node
		theHold.Accept("DeleteBone")
		
		TestLimbHierarchy armBranch arm
		DoUndo()
		TestLimbHierarchy armBranch arm
		DoRedo()
		TestLimbHierarchy armBranch arm

		theHold.Begin()
		arm.RemoveCollarbone()
		theHold.Accept("DeleteBone")

		TestLimbHierarchy armBranch arm
		DoUndo()
		TestLimbHierarchy armBranch arm
		DoRedo()
		TestLimbHierarchy armBranch arm

		theHold.Begin()
		delete arm.Bones[1].Node
		theHold.Accept("DeleteBone")
		
		-- Do we have phases for the leg still?
		assert_equal catmotionroot[3].Name "LimbPhases"
		assert_equal catmotionroot[3].NumSubs 1
		assert_equal pelvis.limbs.count 1

		DoFlushUndos()
	),
	
	fn testLeg = (
		-- Now test the legs
		leg = pelvis.limbs[1]
		legBranch = pelvisBranch[2]
		
		assert_equal pelvisBranch[2].Name "Legs"
		assert_true leg.IsLeg
		--TestLimbHierarchy legBranch leg

		-- Try removing the ankle first
		theHold.Begin()
		delete leg.Palm.Node
		theHold.Accept("DeleteBone")
		
		TestLimbHierarchy legBranch leg
		DoUndo()
		TestLimbHierarchy legBranch leg
		DoRedo()
		TestLimbHierarchy legBranch leg

		DoFlushUndos()
		
		-- Now the foot platform
		theHold.Begin()
		delete leg.IKTarget
		theHold.Accept("DeleteBone")
		
		TestLimbHierarchy legBranch leg
		DoUndo()
		TestLimbHierarchy legBranch leg
		DoRedo()
		TestLimbHierarchy legBranch leg

		theHold.Begin()
		delete  leg.Bones[1].Node
		theHold.Accept("DeleteBone")
		
		assert_not_equal catmotionroot[3].Name "LimbPhases"
		assert_equal pelvis.limbs.count 0
		
		DoFlushUndos()
	),
	
	fn endall = (
			--resetMaxFile #noprompt 
	),
	
	-- The tests must be performed in this order
	tests = 
	#(
		setUp
		, testHub
		, testCreateSingleArmLegTail
		, testTail
		, testArm
		, testLeg
		,endall
	)
)

run_test_fixture CATTestFixture script:(getThisScriptFilename())