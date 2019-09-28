--##################################################
-- Modifiers Skin Unit Test
-- Author: Peter Watje
-- Created:	March 2013
--##################################################
--##################################################
fileIn "MxsUnit/MxsUnit.ms"

struct Obj_Prop_UnitTest
(
private


	--##################################################
	-- Helper functions
	--##################################################


	function validateProperties experimental expected =
	(
		if (experimental.count != expected.count) then
		(
			assert_equal expected.count experimental.count message:"Incorrect number of properties"
		)
		else
		(
			for i in 1 to experimental.count do
			(
				assert_equal expected[i] experimental[i] message:("Incorrect Value for " + testprops[i])
			)
		)
	),

	function changeValue valueName pvalue properties =
	(
		for i in 1 to properties.count do
		(
			if testprops[i] == valueName do
			(
				properties[i] = pvalue
				return true
			)
		)
		local str = stringstream ""
		format "Value(%): %, not found in property array" valueName pvalue to:str
		assert_true false message:(str as string)
	),

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
/*
	Regression test for  MAXX_11350
	Max script index issue
	Fix : the code as thinking the index was 1 based, set it to zero base
*/
	function skin_MAXX_11350 =
	(
		resetMaxFile #noPrompt
		Cylinder smooth:on heightsegs:5 capsegs:1 sides:18 height:57.0311 radius:11.2255 mapcoords:on pos:[-1.18015,0.751877,0] isSelected:on
		Dummy transform:(matrix3 [1,0,0] [0,0,1] [0,-1,0] [-0.879765,0,1.46628]) isSelected:on
		Dummy transform:(matrix3 [1,0,0] [0,0,1] [0,-1,0] [-0.879765,-1.2434e-006,28.4457]) isSelected:on
		Dummy transform:(matrix3 [1,0,0] [0,0,1] [0,-1,0] [-0.293255,-2.3458e-006,53.6657]) isSelected:on

		local testNode = $Cylinder001
		local testMod = Skin()
		addModifier testNode testMod
		clearSelection()

		skinOps.addBone testMod $Dummy001 1 node:testNode
		skinOps.addBone testMod $Dummy002 1 node:testNode
		skinOps.addBone testMod $Dummy003 1 node:testNode
		
		forceCompleteRedraw()

		iret = false
			
		skinOps.rigidVertex testMod 1 false node:testNode
		iret = skinOps.isRigidVertex testMod 1 node:testNode
		assert_true (iret == 0)
			
		skinOps.rigidVertex testMod 1 true node:testNode
		iret = skinOps.isRigidVertex testMod 1 node:testNode
		assert_true (iret == 1)

		skinOps.rigidVertex testMod 2 false node:testNode
		iret = skinOps.isRigidVertex testMod 2 node:testNode
		assert_true (iret == 0)

		skinOps.rigidVertex testMod 2 true node:testNode
		iret = skinOps.isRigidVertex testMod 2 node:testNode
		assert_true (iret == 1)
	
		
		skinOps.rigidHandle testMod 1 false node:testNode
		iret = skinOps.isRigidHandle testMod 1 node:testNode
		assert_true (iret == 0)

		skinOps.rigidHandle testMod 1 true node:testNode
		iret = skinOps.isRigidHandle testMod 1 node:testNode
		assert_true (iret == 1)

		skinOps.rigidHandle testMod 2 false node:testNode
		iret = skinOps.isRigidHandle testMod 2 node:testNode
		assert_true (iret == 0)

		skinOps.rigidHandle testMod 2 true node:testNode
		iret = skinOps.isRigidHandle testMod 2 node:testNode
		assert_true (iret == 1)
			
			
		skinOps.UnNormalizeVertex testMod 1 false node:testNode
		iret = skinOps.isUnNormalizeVertex testMod 1 node:testNode

		assert_true (iret == 0)

		skinOps.UnNormalizeVertex testMod 1 true node:testNode
		iret = skinOps.isUnNormalizeVertex testMod 1 node:testNode
		assert_true (iret == 1)

		skinOps.UnNormalizeVertex testMod 2 false node:testNode
		iret = skinOps.isUnNormalizeVertex testMod 2 node:testNode
		assert_true (iret == 0)

		skinOps.UnNormalizeVertex testMod 2 true node:testNode
		iret = skinOps.isUnNormalizeVertex testMod 2 node:testNode
		assert_true (iret == 1)
		
	),

	function skin_DQ_UnitTest =
	(
		resetMaxFile #noPrompt
		Cylinder smooth:on heightsegs:5 capsegs:1 sides:8 height:56.8222 radius:10.9067 mapcoords:on pos:[-0.902115,1.30113,0] isSelected:on
		Box lengthsegs:1 widthsegs:1 heightsegs:1 length:15.2588 width:15.41124 height:30.8898 mapcoords:on pos:[1.39257,0.63385,0] isSelected:on
		Box lengthsegs:1 widthsegs:1 heightsegs:1 length:15.34133 width:17.71352 height:31.4761 mapcoords:on pos:[1.39257,0.63385,31] isSelected:on
		Select $BOx002
		$.parent = $Box001
		sliderTime = 20f
		set animate on
		rotate $ (angleaxis -95.0535 [0,1,0])
		set animate off
		sliderTime = 0f
		local testNode = $Cylinder001
		local testMod = Skin()
		addModifier testNode testMod
		clearSelection()

		skinOps.addBone testMod $Box001 1 node:testNode
		skinOps.addBone testMod $Box002 1 node:testNode
		testMod.cross_radius = 28.1854
		sliderTime = 20f
		numVerts = meshop.getNumVerts testNode
		for i = 1 to numVerts/2 do
		(
			skinops.setVertexDQWeight testMod i 0.5 node:testNode
		)

		for i = numVerts/2 to numVerts do
		(
			skinops.setVertexDQWeight testMod i 1.0 node:testNode
		)
		testMod.enableDQ = off



		testMod.enableDQ = on



	),
	
	function skin_VoxelHeatMap_UnitTest =
	(

		if systemTools.IsGpuPresent() then 
		(
            resetMaxFile #noPrompt
            max create mode
            Cylinder smooth:on heightsegs:5 capsegs:1 sides:8 height:56.8222 radius:10.9067 mapcoords:on pos:[-0.902115,1.30113,0] isSelected:on
            Box lengthsegs:1 widthsegs:1 heightsegs:1 length:15.2588 width:15.41124 height:30.8898 mapcoords:on pos:[1.39257,0.63385,0] isSelected:on
            Box lengthsegs:1 widthsegs:1 heightsegs:1 length:15.34133 width:17.71352 height:31.4761 mapcoords:on pos:[1.39257,0.63385,31] isSelected:on
            Select $BOx002
            $.parent = $Box001
            sliderTime = 20f
            set animate on
            rotate $ (angleaxis -95.0535 [0,1,0])
            set animate off
            sliderTime = 0f
            local testNode = $Cylinder001
            local testMod = Skin()
            addModifier testNode testMod
            clearSelection()
            
            skinOps.addBone testMod $Box001 1 node:testNode
            skinOps.addBone testMod $Box002 1 node:testNode
            max create mode
            max modify mode
            	
            format "heatmap falloff			\n"
            skinOps.voxelWeighting testMod 1 0.0 -1 0 off on node:testNode
            max create mode
            max modify mode
            
            
            skinOps.voxelWeighting testMod 1 1.0 -1 0 off on node:testNode
            max create mode
            max modify mode
            
            format "heatmap max influence		\n"
            skinOps.voxelWeighting testMod 1 1.0 -2 0 off on node:testNode
            max create mode
            max modify mode
            
            skinOps.voxelWeighting testMod 1 1.0 -1 0 off on node:testNode
            max create mode
            max modify mode
            	
            
            format "heatmap autonub		\n"
            skinOps.voxelWeighting testMod 1 1.0 -1 0 off off node:testNode
            max create mode
            max modify mode
            
            format "voxel falloff		\n"
            skinOps.voxelWeighting testMod 0 0.0 -1 64 off on node:testNode
            max create mode
            max modify mode
            
            skinOps.voxelWeighting testMod 0 1.0 -1 64 off on node:testNode
            max create mode
            max modify mode
            
            format "voxel density		\n"
            skinOps.voxelWeighting testMod 0 0.5 -1 0 off on node:testNode
            max create mode
            max modify mode
            
            skinOps.voxelWeighting testMod 0 0.5 -1 64 off on node:testNode
            max create mode
            max modify mode
            
            skinOps.voxelWeighting testMod 0 0.5 -1 128 off on node:testNode
            max create mode
            max modify mode
            	
            format "voxel winding		\n"
            skinOps.voxelWeighting testMod 0 0.5 1 64 on on node:testNode
		)
		else
		(
			format "Skipping VoxelHeatMap test because feature depends on GPU presence; no GPU found\n"
		)
	),


	/* Fixes issue when saving while editing DQ weights */
	
	function skin_MAXX_33477 =
	(
        Cylinder smooth:on heightsegs:24 capsegs:1 sides:18 height:180.925 radius:11.8185 mapcoords:on pos:[0.398957,1.51602,0] isSelected:on
		Box lengthsegs:1 widthsegs:1 heightsegs:1 length:10 width:10 height:50 mapcoords:on
		max tool animmode
		set animate off

		Box lengthsegs:1 widthsegs:1 heightsegs:1 length:11.5901 width:10.471 height:52.5281 mapcoords:on pos:[-19.6352,-1.91157,0] isSelected:on
		Box lengthsegs:1 widthsegs:1 heightsegs:1 length:13.08 width:8.72368 height:57.0647 mapcoords:on pos:[-32.0862,-1.04684,0] isSelected:on
		max move
		clearSelection()
		select $Box002
		$.pos = [-19.6352,-1.91157,68.6842]
		$.pos = [1.02133,-1.91157,57.6354]
		select $Box003
		$.pos = [0.295071,-1.04684,113.328]

		select $Box003
		$.parent = $Box002
		select $Box002
		$.parent = $Box001

		select $Box001
		selectMore $Box002
		selectMore $Box003

		sliderTime = 30f
		max tool animmode
		set animate on
		clearSelection()
		select $Box002
		rotate $ (angleaxis -63.458 [0,1,0])
		select $Box003
		rotate $ (angleaxis -63.458 [0,1,0])
		set animate off
		sliderTime = 1f
		select $Cylinder001
		max modify mode
		modPanel.addModToSelection (Skin ()) ui:on
						
							  
				  
		skinOps.addBone $.modifiers[#Skin] $Box001 1
		skinOps.addBone $.modifiers[#Skin] $Box002 1
		skinOps.addBone $.modifiers[#Skin] $Box003 1
		$.modifiers[#Skin].cross_radius = 17.1675
		sliderTime = 46f
		sliderTime = 0f
		subobjectLevel = 1
		$.modifiers[#Skin].filter_vertices = on
		skinOps.SelectVertices $.modifiers[#Skin] #{1..452}
		$.modifiers[#Skin].showNoEnvelopes = on
		sliderTime = 30f
		skinOps.enableDQOverrideWeighting $.modifiers[#Skin] true
		skinOps.SelectVertices $.modifiers[#Skin] #{110..271, 273..289, 294..296, 302..304}
		skinOps.setWeight $.modifiers[#Skin] 1
		p = getVert $ 443
		holdMaxFile() 
		fetchMaxFile quiet:true
		sliderTime = 30f
		select $Cylinder001
		p1 = getVert $ 443
		d = Length (p1-p)
		assert_true (d < 0.0001)
	),
	
	Tests =
	#(
		skin_MAXX_11350, skin_DQ_UnitTest, skin_VoxelHeatMap_UnitTest, skin_MAXX_33477
	)
)

/*
foo = Obj_Prop_UnitTest()
foo.setup()
foo.skin_MAXX_11350()
foo.skin_DQ_UnitTest()
foo.skin_VoxelHeatMap_UnitTest()
*/

run_test_fixture Obj_Prop_UnitTest script:(getThisScriptFilename())