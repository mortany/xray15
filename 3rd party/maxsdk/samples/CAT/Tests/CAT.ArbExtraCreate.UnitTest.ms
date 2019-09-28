----------------------------------------------------------------------------------------------
-- The purpose of this test is to verify the ability to create ArbBones
-- and extra Matrix3 controllers, and for them to have their own
-- group hierarchy affected by local weights
-- Tested:
--   ArbBone and extra controllers creation and grouping
--   Layer creation and colouring
--   rig color mode
--   Local weight modification on absolute layers
----------------------------------------------------------------------------------------------

filein "CAT.TestTransformDataFile.ms"
filein "CAT.UtilityFns.ms"
fileIn "MxsUnit/MxsUnit.ms"

CATTestCreateArbExtraBones = undefined
struct CATTestCreateArbExtraBones
(
	dataDir = 0,
public
	fn setUp =
	(
		dataDir = GetTestingDataDir() + "ArbControllerCreate/"
	),
	
	fn TestArbBonesInHierarchy =
	(
		resetMaxFile #noprompt 
		
		cpt = CATParent()
		cpt.CATUnits = 0.3
		cpt.transform = matrix3 [0,0,1] [0,-1,0] [1,0,0] [0,0,0]
		cpt.AddHub()
		root = cpt.RootHub

		HubGroup1 = root.AddArbBone()
		move HubGroup1.Node [-15, 0, 0]
		HubGroup2 = HubGroup1.AddArbBone()
		HubGroup1ChildGroup = HubGroup1.AddArbBone AsNewGroup:true
		move HubGroup1ChildGroup.Node [-5, 0, 0]

		NewGroup1 = root.AddArbBone AsNewGroup:true
		move NewGroup1.Node [15, 0, 0]
		NewGroup2 = NewGroup1.AddArbBone()
		NewGroup1ChildGroup = NewGroup1.AddArbBone AsNewGroup:true
		move NewGroup1ChildGroup.Node [5, 0, 0]

		TestCATRigTransformations cpt (dataDir + "CAT.ArbBone1Initial.ms")

		-- Now test that our new bone groups have been setup correctly
		cpt.AppendLayer "BaseLayer" "ABSOLUTE"
		cpt.AppendLayer "BlendLayer" "ABSOLUTE"
		cpt.CATMode = 1 --Animate mode
		cpt.ColourMode = 2 -- Layer colouring mode
		cpt.SelectedLayer = 2 -- Ensure we set avlues to the 2nd layer

		-- Init base layer color to red
		cpt.SetLayerColor 1 (color 255 0 0)
		-- Init blend layer color to blue
		cpt.SetLayerColor 2 (color 0 0 255)
		-- Now modify the transforms for the bones

		rotate HubGroup1.Node (angleaxis 56.9176 [1,0,0])
		move HubGroup2.Node [0,-15.4148,0]
		move HubGroup1ChildGroup.Node [0,0,10.046]

		rotate NewGroup1.Node (angleaxis -40.5646 [0,0,1])
		rotate NewGroup1.Node (angleaxis -41.9317 [-0.659946,0.744194,0.103183])
		move NewGroup1.Node [0,0,-8.97182]
		move NewGroup2.Node [0,-12.7704,0]
		move NewGroup1ChildGroup.Node [0,0,8.78156]

		-- Validate all our nodes are at the appropriate places
		TestCATRigTransformations cpt (dataDir + "CAT.ArbBone2Moved.ms")

		-- Blend off the 2nd layer 50% for the new group branch and child branch
		NewGroup1ChildGroup.node.controller.LayerTrans.weights[2].value = 50
		NewGroup1.node.controller.LayerTrans.weights[2].value = 50

		-- Verify that our transforms and colors have changed appropriately
		TestCATRigTransformations cpt (dataDir + "CAT.ArbBone3GroupWeighted50.ms")
		assert_true(NewGroup1ChildGroup.node.wirecolor == (color 191 0 64))
		assert_true(NewGroup1.node.wirecolor == (color 128 0 128))
		assert_true(NewGroup2.node.wirecolor == (color 128 0 128))

		assert_true(HubGroup1.node.wirecolor == (color 0 0 255))
		assert_true(HubGroup2.node.wirecolor == (color 0 0 255))
		assert_true(HubGroup1ChildGroup.node.wirecolor == (color 0 0 255))

		-- Blend off the 2nd layer 50% for the new group branch and child branch
		HubGroup1ChildGroup.node.controller.LayerTrans.weights[2].value = 50
		HubGroup1.node.controller.LayerTrans.weights[2].value = 50

		-- Verify that our transforms and colors have changed appropriately
		TestCATRigTransformations cpt (dataDir + "CAT.ArbBone4HubWeighted50.ms")
		assert_true(NewGroup1ChildGroup.node.wirecolor == (color 223 0 32))
		assert_true(NewGroup1.node.wirecolor == (color 191 0 64))
		assert_true(NewGroup2.node.wirecolor == (color 191 0 64))

		assert_true(HubGroup1.node.wirecolor == (color 128 0 128))
		assert_true(HubGroup2.node.wirecolor ==(color 128 0 128))
		assert_true(HubGroup1ChildGroup.node.wirecolor == (color 191 0 64))
	),
	
	fn TestExtraMatrix3CtrllersInHierarchy = 
	(
		resetMaxFile #noprompt 
		
		cpt = CATParent()
		cpt.CATUnits = 0.3
		cpt.transform = matrix3 [0,0,1] [0,-1,0] [1,0,0] [0,0,0]
		cpt.AddHub()
		root = cpt.RootHub

		aBox = Box()
		aBox.width = aBox.height = aBox.length = 10
		
		boxHubGroup1 = aBox
		boxHubGroup1.controller = root.CreateLayerMatrix3()
		move boxHubGroup1 [-15, 0, 0]
		
		boxHubGroup2 = instance aBox
		boxHubGroup2.controller = boxHubGroup1.controller.CreateLayerMatrix3()
		move boxHubGroup2[-15, 0, 15]
		
		boxHubGroup1ChildGroup = instance aBox
		boxHubGroup1ChildGroup.controller = boxHubGroup1.controller.CreateLayerMatrix3 AsNewGroup:true
		move boxHubGroup1ChildGroup [-15, 0, 30]

		boxNewGroup1 = instance aBox
		boxNewGroup1.controller = root.CreateLayerMatrix3  AsNewGroup:true
		move boxNewGroup1 [15, 0, 0]
		
		boxNewGroup2 = instance aBox
		boxNewGroup2.controller = boxNewGroup1.controller.CreateLayerMatrix3()
		move boxNewGroup2[15, 0, 15]
		
		boxNewGroup1ChildGroup = instance aBox
		boxNewGroup1ChildGroup.controller = boxNewGroup1.controller.CreateLayerMatrix3 AsNewGroup:true
		move boxNewGroup1ChildGroup [15, 0, 30]
		
		cpt.controller.extraRigNodes = #(boxHubGroup1, boxHubGroup2, boxHubGroup1ChildGroup, boxNewGroup1, boxNewGroup2, boxNewGroup1NewGroup)
		TestCATRigTransformations cpt (dataDir + "CAT.ExtraCtrl1Initial.ms")

		-- Now test that our new bone groups have been setup correctly
		cpt.AppendLayer "BaseLayer" "ABSOLUTE"
		cpt.AppendLayer "BlendLayer" "ABSOLUTE"
		cpt.CATMode = 1 --Animate mode
		cpt.ColourMode = 2 -- Layer colouring mode
		cpt.SelectedLayer = 2 -- Ensure we set avlues to the 2nd layer

		-- Init base layer color to red
		cpt.SetLayerColor 1 (color 255 0 0)
		-- Init blend layer color to blue
		cpt.SetLayerColor 2 (color 0 0 255)
		-- Now modify the transforms for the bones

		rotate boxHubGroup1 (angleaxis 56.9176 [1,0,0])
		move boxHubGroup2 [0,-15.4148,0]
		move boxHubGroup1ChildGroup [0,0,10.046]

		rotate boxNewGroup1 (angleaxis -40.5646 [0,0,1])
		rotate boxNewGroup1 (angleaxis -41.9317 [-0.659946,0.744194,0.103183])
		move boxNewGroup1 [0,0,-8.97182]
		move boxNewGroup2 [0,-12.7704,0]
		move boxNewGroup1ChildGroup [0,0,8.78156]

		-- Validate all our nodes are at the appropriate places
		TestCATRigTransformations cpt (dataDir + "CAT.ExtraCtrl2Moved.ms")

		-- Blend off the 2nd layer 50% for the new group branch and child branch
		boxNewGroup1ChildGroup.controller.LayerTrans.weights[2].value = 50
		boxNewGroup1.controller.LayerTrans.weights[2].value = 50

		-- Verify that our transforms and colors have changed appropriately
		TestCATRigTransformations cpt (dataDir + "CAT.ExtraCtrl3GroupWeighted50.ms")
		assert_true(boxNewGroup1ChildGroup.wirecolor == (color 191 0 64))
		assert_true(boxNewGroup1.wirecolor == (color 128 0 128))
		assert_true(boxNewGroup2.wirecolor == (color 128 0 128))

		assert_true(boxHubGroup1.wirecolor == (color 0 0 255))
		assert_true(boxHubGroup2.wirecolor == (color 0 0 255))
		assert_true(boxHubGroup1ChildGroup.wirecolor == (color 0 0 255))

		-- Blend off the 2nd layer 50% for the new group branch and child branch
		boxHubGroup1ChildGroup.controller.LayerTrans.weights[2].value = 50
		boxHubGroup1.controller.LayerTrans.weights[2].value = 50

		-- Verify that our transforms and colors have changed appropriately
		TestCATRigTransformations cpt (dataDir + "CAT.ExtraCtrl4HubWeighted50.ms")
		assert_true(boxNewGroup1ChildGroup.wirecolor == (color 223 0 32))
		assert_true(boxNewGroup1.wirecolor == (color 191 0 64))
		assert_true(boxNewGroup2.wirecolor == (color 191 0 64))

		assert_true(boxHubGroup1.wirecolor == (color 128 0 128))
		assert_true(boxHubGroup2.wirecolor ==(color 128 0 128))
		assert_true(boxHubGroup1ChildGroup.wirecolor == (color 191 0 64))
	),
	
	fn endall =
	(
		resetMaxFile #noprompt 
	),
	
	-- The tests must be performed in this order
	tests = 
	#(
		setUp
		, TestArbBonesInHierarchy
		, TestExtraMatrix3CtrllersInHierarchy
		,endall
	)
)

run_test_fixture CATTestCreateArbExtraBones script:(getThisScriptFilename())	