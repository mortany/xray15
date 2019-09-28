--------------------------------------------------------------------------------
-- Unit tests for the Morpher modifier
--
-- Author:	Attila Szabo                     
-- Created: Sep.09.2010
--------------------------------------------------------------------------------
fileIn "MxsUnit/MxsUnit.ms"

struct UnitTest_Morpher
(
private
	NUM_MAX_MORPH_CHANNELS = 100,
	NUM_MAX_PROGRESSIVE_TARGETS = 25,
	
	mMorphHostObject = undefined,
	mMorphTargets = #(), -- array of NUM_MORPH_CHANNELS objects
	mMorphProgressiveTargets = #(), -- array [NUM_MORPH_CHANNELS, NUM_PROGRESSIVE_TARGETS] objects
	
	EMTPTY_CHANNEL_NAME="- empty -",
	
	function doSetup numChannels numTargetsPerChannel =
	(
		assert_true (1 <= numChannels)
		assert_true (numChannels <= NUM_MAX_MORPH_CHANNELS)
		assert_true (0 <= numTargetsPerChannel)
		assert_true (numTargetsPerChannel <= NUM_MAX_PROGRESSIVE_TARGETS)
		
		-- Create the set of objects used as morph targets and object to be morphed
		mMorphHostObject = Plane length:10 width:10 name:"MorphHost"
		convertToMesh mMorphHostObject
		for i=1 to numChannels do 
		(
			maxOps.CloneNodes mMorphHostObject offset:[i*(10+2), 0, 0] cloneType:#copy newNodes:&newMorphTarget
			mMorphTargets[i] = newMorphTarget[1]
			mMorphTargets[i].name="MorphTarget"+(i as string)
			local progressiveTargets=#()
			for j = 1 to numTargetsPerChannel do
			(
				maxOps.CloneNodes newMorphTarget[1] offset:[0, j*(10+2), 0] cloneType:#copy newNodes:&newMorphProgressiveTraget
				progressiveTargets[j] = newMorphProgressiveTraget[1]
				progressiveTargets[j].name=mMorphTargets[i].name+"." +(j as string)

			)		
			mMorphProgressiveTargets[i] = progressiveTargets 
		)
		
		-- setup a morpher with targets
		local morpherMod = Morpher()
		addModifier mMorphHostObject morpherMod
		WM3_RebuildInternalCache morpherMod
		for i=1 to numChannels do 
		(
			WM3_MC_SetTarget morpherMod i mMorphTargets[i]
			WM3_MC_SetValue morpherMod i (i as float)
			for j = 1 to numTargetsPerChannel do
			(
				WM3_AddProgressiveMorphNode morpherMod i mMorphProgressiveTargets[i][j]
				assert_true (undefined != (WM3_GetProgressiveMorphNode morpherMod i j))
			)		
		)		
	),
	
	function doTestChannelSwap morphId1 morphId2 = 
	(
		local morphTarget1 = WM3_MC_GetTarget mMorphHostObject.morpher morphId1
		local morphTarget2 = WM3_MC_GetTarget mMorphHostObject.morpher morphId2
		assert_true (morphTarget1 != morphTarget2)
		with undo on
		(
			WM3_SwapMorph mMorphHostObject.morpher morphId1  morphId2
			assert_true (WM3_MC_GetTarget mMorphHostObject.morpher morphId1 == morphTarget2)
		)
		max undo
		assert_true (WM3_MC_GetTarget mMorphHostObject.morpher morphId1 == morphTarget1)
		max redo
		assert_true (WM3_MC_GetTarget mMorphHostObject.morpher morphId1 == morphTarget2)
		max undo
		assert_true (WM3_MC_GetTarget mMorphHostObject.morpher morphId1 == morphTarget1)
	),

	function doTestChannelMove morphId1 morphId2 = 
	(
		local morphTarget1 = WM3_MC_GetTarget mMorphHostObject.morpher morphId1
		local morphTarget2 = WM3_MC_GetTarget mMorphHostObject.morpher morphId2
		assert_true (morphTarget1 != morphTarget2)
		with undo on
		(
			WM3_MoveMorph mMorphHostObject.morpher morphId1  morphId2
			assert_true (WM3_MC_GetTarget mMorphHostObject.morpher morphId1 == undefined)
			assert_true (WM3_MC_GetTarget mMorphHostObject.morpher morphId2 == morphTarget1)
		)
		max undo
		assert_true (WM3_MC_GetTarget mMorphHostObject.morpher morphId1 == morphTarget1)
		assert_true (WM3_MC_GetTarget mMorphHostObject.morpher morphId2 == morphTarget2)
		max redo
		assert_true (WM3_MC_GetTarget mMorphHostObject.morpher morphId2 == morphTarget1)
		max undo
		assert_true (WM3_MC_GetTarget mMorphHostObject.morpher morphId1 == morphTarget1)
		assert_true (WM3_MC_GetTarget mMorphHostObject.morpher morphId2 == morphTarget2)
	),

public
	function Setup =
	(
		resetMaxFile #noprompt
	),
	
	function teardown =
	(
	
	),
	
	function testMorpherModifierValidity =
	(
		assert_true (IsValidMorpherMod (Morpher()))
		assert_false (IsValidMorpherMod (Bend()))
	),
	
	function testNumChannels = 
	(
		assert_equal NUM_MAX_MORPH_CHANNELS (WM3_NumberOfChannels (Morpher()))
	),
	
	function testChannelName = 
	(
		local morpherMod = Morpher()
		-- getting name 
		for i = 1 to NUM_MAX_MORPH_CHANNELS do 
		(
			assert_equal EMTPTY_CHANNEL_NAME (WM3_MC_GetName morpherMod i)
		)
		-- accessing out of range channels 
		assert_equal EMTPTY_CHANNEL_NAME (WM3_MC_GetName morpherMod -1)
		assert_equal EMTPTY_CHANNEL_NAME (WM3_MC_GetName morpherMod 101) -- This was buggy
		assert_equal EMTPTY_CHANNEL_NAME (WM3_MC_GetName morpherMod 10000)
		
		-- setting names
		local newName = "testChannelName"
		for i = 1 to NUM_MAX_MORPH_CHANNELS do 
		(
			WM3_MC_SetName morpherMod i newName
			assert_equal newName (WM3_MC_GetName morpherMod i)
		)
	),
	
	function testChannelActiveState = 
	(
		local morphChannelId = 1
		doSetup morphChannelId 0
		assert_true (WM3_MC_IsActive mMorphHostObject.morpher morphChannelId)
		WM3_MC_SetActive mMorphHostObject.morpher morphChannelId false
		assert_false (WM3_MC_IsActive mMorphHostObject.morpher morphChannelId)
	),
	
	function testChannelHasDataState = 
	(
		local morpherMod = Morpher()
		assert_false (WM3_MC_HasData morpherMod 1)
		
		local morphChannelId = 1
		doSetup morphChannelId 0
		assert_true (WM3_MC_HasData mMorphHostObject.morpher morphChannelId)
		local morphTarget = WM3_MC_GetTarget mMorphHostObject.morpher morphChannelId
		assert_true (undefined != morphTarget)
		with undo on 
		(
			delete morphTarget
		)
		assert_true (WM3_MC_HasData mMorphHostObject.morpher morphChannelId)
		max undo
		assert_true (WM3_MC_HasData mMorphHostObject.morpher morphChannelId)
		max redo 
		assert_true (WM3_MC_HasData mMorphHostObject.morpher morphChannelId)
		max undo
		assert_true (undefined != morphTarget)
	),

	function testChannelIsValidState = 
	(
		local morpherMod = Morpher()
		assert_true (WM3_MC_IsValid morpherMod 1)
		
		local morphChannelId = 1
		doSetup morphChannelId 0
		assert_true (WM3_MC_IsValid mMorphHostObject.morpher morphChannelId)
		local morphTarget = WM3_MC_GetTarget mMorphHostObject.morpher morphChannelId
		assert_true (undefined != morphTarget)
		with undo on 
		(
			delete morphTarget
		)
		assert_true (WM3_MC_IsValid mMorphHostObject.morpher morphChannelId)
		max undo
		assert_true (WM3_MC_IsValid mMorphHostObject.morpher morphChannelId)
		assert_true (undefined != morphTarget)
	),

	function testChannelMorphWeight = 
	(
		local morphChannelId = 1
		doSetup morphChannelId 0
		local startWeight = 30.0
		local endWeight = 60.0
		local startTime = 0
		local endTime = 100
		with animate on
		(
			at time startTime WM3_MC_SetValue mMorphHostObject.morpher morphChannelId startWeight
			at time endTime WM3_MC_SetValue mMorphHostObject.morpher morphChannelId endWeight
		)
		assert_true (at time startTime (WM3_MC_GetValue mMorphHostObject.morpher morphChannelId) == startWeight)
		assert_true (at time endTime (WM3_MC_GetValue mMorphHostObject.morpher morphChannelId) == endWeight)
	),

	function testChannelMorphTarget = 
	(
		local morphChannelId = 1
		doSetup morphChannelId 0
		assert_true (WM3_MC_HasTarget mMorphHostObject.morpher morphChannelId)
		assert_false (WM3_MC_HasTarget mMorphHostObject.morpher (morphChannelId+1))
		-- can't set  morph target for channel that already has one
		maxOps.CloneNodes mMorphHostObject cloneType:#copy newNodes:&newMorphTarget
		assert_true (undefined != newMorphTarget[1])
		assert_false (WM3_MC_SetTarget mMorphHostObject.morpher morphChannelId newMorphTarget[1])
			
		local morphTarget = WM3_MC_GetTarget mMorphHostObject.morpher morphChannelId
		with undo on
		(
			delete morphTarget
		)
		assert_false (WM3_MC_HasTarget mMorphHostObject.morpher morphChannelId)
		max undo
		assert_true (WM3_MC_HasTarget mMorphHostObject.morpher morphChannelId)
		max redo
		assert_false (WM3_MC_HasTarget mMorphHostObject.morpher morphChannelId)
		max undo
		assert_true (WM3_MC_HasTarget mMorphHostObject.morpher morphChannelId)
		
		local boxObject = Box()
		WM3_MC_SetTarget mMorphHostObject.morpher (morphChannelId+1) boxObject
		assert_undefined (WM3_MC_GetTarget mMorphHostObject.morpher (morphChannelId+1))
	),
	
	function testProgressiveMorphTension = 
	(
		local numMorphChannels = 1
		local numProgMorphs = NUM_MAX_PROGRESSIVE_TARGETS-1
		doSetup numMorphChannels numProgMorphs
		local tension = 0.7
		assert_true (WM3_SetProgressiveMorphTension mMorphHostObject.morpher numMorphChannels tension)
		assert_true (tension == (WM3_GetProgressiveMorphTension mMorphHostObject.morpher numMorphChannels))
		assert_true (WM3_SetProgressiveMorphTension mMorphHostObject.morpher numMorphChannels (2*tension))
		assert_true (1.0 == (WM3_GetProgressiveMorphTension mMorphHostObject.morpher numMorphChannels))
	),
	
	function testProgressiveMorphWeight = 
	(
		local numMorphChannels = 1
		local numProgMorphs = NUM_MAX_PROGRESSIVE_TARGETS
		doSetup numMorphChannels numProgMorphs
		local weight = 70.0
		local progMT = WM3_GetProgressiveMorphNode mMorphHostObject.morpher numMorphChannels numProgMorphs
		assert_true (undefined != progMT)
		assert_true (WM3_SetProgressiveMorphWeight mMorphHostObject.morpher numMorphChannels progMT weight)
		assert_true (weight == (WM3_GetProgressiveMorphWeight mMorphHostObject.morpher numMorphChannels progMT))
		assert_true (WM3_SetProgressiveMorphWeight mMorphHostObject.morpher numMorphChannels progMT (2*weight))
		assert_true (100.0 == (WM3_GetProgressiveMorphWeight mMorphHostObject.morpher numMorphChannels progMT))
	),

	function testGetProgressiveMorphTarget = 
	(
		local numMorphChannels = 1
		local numProgMorphs = NUM_MAX_PROGRESSIVE_TARGETS
		doSetup numMorphChannels numProgMorphs
		assert_true ((numProgMorphs+1) == (WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels))

		local progMT = WM3_GetProgressiveMorphNode mMorphHostObject.morpher numMorphChannels 0
		assert_true (undefined == progMT)
		for i=1 to (WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels) do
		(
			progMT = WM3_GetProgressiveMorphNode mMorphHostObject.morpher numMorphChannels i
			assert_true (undefined != progMT)
		)
	),
	
	function testAddProgressiveMorphTarget =
	(
		local numMorphChannels = 1
		local numProgMorphs = NUM_MAX_PROGRESSIVE_TARGETS-1
		doSetup numMorphChannels numProgMorphs
		assert_true ((numProgMorphs+1) == (WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels))

		assert_true (undefined != mMorphTargets[1])
		maxOps.CloneNodes mMorphTargets[1] cloneType:#copy newNodes:&newMorphTarget

		with undo on
		(
			assert_true (WM3_AddProgressiveMorphNode mMorphHostObject.morpher numMorphChannels newMorphTarget[1])
			assert_true ((numProgMorphs+2) == (WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels))
		)
		
		max undo
		assert_true ((numProgMorphs+1) == (WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels))
		max redo
		assert_true ((numProgMorphs+2) == (WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels))
		
		assert_false (WM3_AddProgressiveMorphNode mMorphHostObject.morpher (numMorphChannels+1) newMorphTarget[1])
	),
	
	function testDeleteProgressiveMorphTarget = 
	(
		local numMorphChannels = 1
		local numProgMorphs = NUM_MAX_PROGRESSIVE_TARGETS
		doSetup numMorphChannels numProgMorphs
		local numProgressiveMorphs = WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels
		assert_true ((numProgMorphs+1) == numProgressiveMorphs)

		with undo on
		(
			assert_true (WM3_DeleteProgressiveMorphNode mMorphHostObject.morpher numMorphChannels 1)
			assert_true (WM3_DeleteProgressiveMorphNode mMorphHostObject.morpher numMorphChannels numProgMorphs)
			assert_true ( 2 == numProgressiveMorphs - (WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels))
		)
		
		max undo
		assert_true ( 0 == numProgressiveMorphs - (WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels))
		max redo
		assert_true ( 2 == numProgressiveMorphs - (WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels))
	),

	function testChannelSwap = 
	(
		local numMorphChannels = NUM_MAX_MORPH_CHANNELS-1
		doSetup numMorphChannels 0
		doTestChannelSwap 1 2
		doTestChannelSwap 1 NUM_MAX_MORPH_CHANNELS
	),
	
	function testChannelMove = 
	(
		local numMorphChannels = NUM_MAX_MORPH_CHANNELS-1
		doSetup numMorphChannels 0
		doTestChannelMove 1 2
		doTestChannelMove 1 NUM_MAX_MORPH_CHANNELS
	),
	
	function testChannelDelete = 
	(
		local numMorphChannels = 1
		local numProgMorphs = NUM_MAX_PROGRESSIVE_TARGETS
		doSetup numMorphChannels numProgMorphs
		
		with undo on
		(
			WM3_MC_Delete mMorphHostObject.morpher numMorphChannels
			assert_false (WM3_MC_HasData mMorphHostObject.morpher numMorphChannels)
			assert_false (WM3_MC_HasTarget mMorphHostObject.morpher numMorphChannels)
			assert_true (0 == WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels)
		)
		max undo
		assert_true (WM3_MC_HasData mMorphHostObject.morpher numMorphChannels)
		assert_true (WM3_MC_HasTarget mMorphHostObject.morpher numMorphChannels)
		assert_true ((numProgMorphs +1) == WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels)
		max redo
		assert_false (WM3_MC_HasData mMorphHostObject.morpher numMorphChannels)
		assert_false (WM3_MC_HasTarget mMorphHostObject.morpher numMorphChannels)
		assert_true (0 == WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels)
		max undo
		assert_true (WM3_MC_HasData mMorphHostObject.morpher numMorphChannels)
		assert_true (WM3_MC_HasTarget mMorphHostObject.morpher numMorphChannels)
		assert_true ((numProgMorphs +1) == WM3_NumberOfProgressiveMorphs mMorphHostObject.morpher numMorphChannels)
		
		assert_true (WM3_MC_Delete mMorphHostObject.morpher (numMorphChannels+1))
	),
/*
	Regression test for  MAXX_11745
	Morpher performance issue
	Fix : the morpher was calling ui and sending notfiy messages during modifier which it should not
*/
	function morph_MAXX_11745 =
	(
		Sphere radius:24.6729 smooth:on segs:32 chop:0 slice:off sliceFrom:0 sliceTo:0 mapcoords:on recenter:off pos:[-24.9893,24.561,0] isSelected:on
		Sphere radius:22.5079 smooth:on segs:32 chop:0 slice:off sliceFrom:0 sliceTo:0 mapcoords:on recenter:off pos:[37.1411,-8.16643,0] isSelected:on
		macros.run "Modifier Stack" "Convert_to_Poly"			
		
		Sphere radius:16.1261 smooth:on segs:32 chop:0 slice:off sliceFrom:0 sliceTo:0 mapcoords:on recenter:off pos:[108.698,-36.6727,0] isSelected:on
		macros.run "Modifier Stack" "Convert_to_Mesh"
		Sphere radius:5.29694 smooth:on segs:32 chop:0 slice:off sliceFrom:0 sliceTo:0 mapcoords:on recenter:off pos:[174.969,-27.7785,0] isSelected:on
		macros.run "Modifier Stack" "Convert_to_Mesh"			
		Sphere radius:14.9287 smooth:on segs:32 chop:0 slice:off sliceFrom:0 sliceTo:0 mapcoords:on recenter:off pos:[200.603,-69.7415,0] isSelected:on
		macros.run "Modifier Stack" "Convert_to_Mesh"
		
		max modify mode
		modPanel.addModToSelection (Bend ()) ui:on
		$.modifiers[#Bend].BendAngle = 46.5
		sliderTime = 30f
		set animate on
		$.modifiers[#Bend].BendAngle = -36.5
				
		forcecompleteredraw()
		
		select $Sphere001
		m = Morpher ()
		modPanel.addModToSelection (m) ui:on
		WM3_MC_BuildFromNode $Sphere001.morpher 1 $Sphere003
		WM3_MC_BuildFromNode $Sphere001.morpher 2 $Sphere004
		WM3_MC_BuildFromNode $Sphere001.morpher 3 $Sphere005

		WM3_MC_SetValue $Sphere001.morpher 1 100.0
		WM3_MC_SetValue $Sphere001.morpher 2 100.0
		WM3_MC_SetValue $Sphere001.morpher 3 100.0

		select $Sphere002
		modPanel.addModToSelection (m) ui:on
		
		forcecompleteredraw()
		for i = 1 to 30 do
		(
			sliderTime = i
		)
		
	),
/*
	Support more than 100 channels
*/
	function morph_MAXX_25061 =
	(
		Plane length:49.413 width:72.5279 pos:[8.76186,3.63534,0] isSelected:on
		convertTo $ TriMeshGeometry
		actionMan.executeAction 0 "50001"
		toolMode.coordsys #view
		select $Plane001
		modPanel.addModToSelection (XForm ()) ui:on
		subobjectLevel = 1
		modPanel.setCurrentObject $.modifiers[#XForm]
		actionMan.executeAction 0 "40074"  -- Snaps: Angle Snap Toggle
		$.modifiers[#XForm].gizmo.rotation += quat 0 -0.707107 0 0.707107
		subobjectLevel = 0
		modPanel.setCurrentObject $.modifiers[#XForm]
			
			
		Plane length:5.0584 width:5.1658 pos:[-65.7168,-6.21194,0] isSelected:on
		macros.run "Modifier Stack" "Convert_to_Mesh"

		Plane length:35.0584 width:54.1658 pos:[-165.7168,-6.21194,0] isSelected:on
		macros.run "Modifier Stack" "Convert_to_Mesh"


		select $Plane003
		modPanel.addModToSelection (Morpher ()) ui:on
			
		WM3_MC_BuildFromNode $Plane003.morpher 1 $Plane001
		WM3_MC_BuildFromNode $Plane003.morpher 105 $Plane002
			
		set animate on
		slidertime = 0
		WM3_MC_SetValue $Plane003.morpher 1 0.0
		slidertime = 10
		WM3_MC_SetValue $Plane003.morpher 1 100.0
		slidertime = 20
		WM3_MC_SetValue $Plane003.morpher 1 0.0	

		slidertime = 30
		WM3_MC_SetValue $Plane003.morpher 105 0.0
		slidertime = 40
		WM3_MC_SetValue $Plane003.morpher 105 100.0
		slidertime = 50
		WM3_MC_SetValue $Plane003.morpher 105 0.0	
		set animate off
			
		holdMaxFile() 
		fetchMaxFile quiet:true
		slidertime = 10
		tmesh = snapshotAsMesh  $Plane003
		vert = getVert tmesh 1
		dif = vert[2] + 30.9184f
		dif = abs dif
		assert_true (dif < 0.01)
		slidertime = 40
		tmesh1 = snapshotAsMesh  $Plane003
		vert1 = getVert tmesh1 1
		assert_true (vert1.z == 0.0)
		
	),
	
	function morph_MAXX_25369 =
	(
		Teapot radius:3.40092 smooth:on segs:2 body:on handle:on spout:on lid:on mapcoords:on pos:[-65.2503,48.0488,0] isSelected:on
		macros.run "Modifier Stack" "Convert_to_Mesh"
		select $Teapot001

		for i = 1 to 300 do
		(
			maxOps.cloneNodes $Teapot001 cloneType:#copy newnodes:&nnl
		)

		select $Teapot001
		move $ [-10.2929,0,0]

		modPanel.addModToSelection (Morpher ()) ui:on
			
		select #($Teapot032, $Teapot031, $Teapot033, $Teapot034, $Teapot035, $Teapot036, $Teapot037, $Teapot038, $Teapot039, $Teapot040, $Teapot041, $Teapot042, $Teapot043, $Teapot044, $Teapot045, $Teapot046, $Teapot047, $Teapot048, $Teapot049, $Teapot050, $Teapot051, $Teapot052, $Teapot053, $Teapot054, $Teapot055, $Teapot056, $Teapot057, $Teapot058, $Teapot059, $Teapot060, $Teapot061, $Teapot062, $Teapot063, $Teapot064, $Teapot065, $Teapot066, $Teapot067, $Teapot068, $Teapot069, $Teapot070, $Teapot071, $Teapot072, $Teapot073, $Teapot074, $Teapot075, $Teapot076, $Teapot077, $Teapot078, $Teapot079, $Teapot080, $Teapot081, $Teapot082, $Teapot083, $Teapot084, $Teapot085, $Teapot086, $Teapot087, $Teapot088, $Teapot089, $Teapot090, $Teapot091, $Teapot092, $Teapot093, $Teapot094, $Teapot095, $Teapot096, $Teapot097, $Teapot098, $Teapot099, $Teapot100, $Teapot101, $Teapot102, $Teapot103, $Teapot027, $Teapot026, $Teapot025, $Teapot024, $Teapot030, $Teapot029, $Teapot028, $Teapot002, $Teapot006, $Teapot008, $Teapot003, $Teapot005, $Teapot009, $Teapot007, $Teapot004, $Teapot012, $Teapot011, $Teapot010, $Teapot015, $Teapot014, $Teapot013, $Teapot018, $Teapot017, $Teapot016, $Teapot021, $Teapot020, $Teapot019, $Teapot022, $Teapot023, $Teapot104, $Teapot105, $Teapot106, $Teapot107, $Teapot108, $Teapot109, $Teapot110, $Teapot111, $Teapot112, $Teapot113, $Teapot114, $Teapot115, $Teapot116, $Teapot117, $Teapot118, $Teapot119, $Teapot120, $Teapot121, $Teapot122, $Teapot123, $Teapot124, $Teapot125, $Teapot126, $Teapot127, $Teapot128, $Teapot129, $Teapot130, $Teapot131, $Teapot132, $Teapot133, $Teapot134, $Teapot135, $Teapot136, $Teapot137, $Teapot138, $Teapot139, $Teapot140, $Teapot141, $Teapot142, $Teapot143, $Teapot144, $Teapot145, $Teapot146, $Teapot147, $Teapot148, $Teapot149, $Teapot150, $Teapot151, $Teapot152, $Teapot153, $Teapot154, $Teapot155, $Teapot156, $Teapot157, $Teapot158, $Teapot159, $Teapot160, $Teapot161, $Teapot162, $Teapot163, $Teapot164, $Teapot165, $Teapot166, $Teapot167, $Teapot168, $Teapot169, $Teapot170, $Teapot171, $Teapot172, $Teapot173, $Teapot174, $Teapot175, $Teapot176, $Teapot177, $Teapot178, $Teapot179, $Teapot180, $Teapot181, $Teapot182, $Teapot183, $Teapot184, $Teapot185, $Teapot186, $Teapot187, $Teapot188, $Teapot189, $Teapot190, $Teapot191, $Teapot192, $Teapot193, $Teapot194, $Teapot195, $Teapot196, $Teapot197, $Teapot198, $Teapot199, $Teapot200, $Teapot201, $Teapot202, $Teapot203, $Teapot204, $Teapot205, $Teapot206, $Teapot207, $Teapot208, $Teapot209, $Teapot210, $Teapot211, $Teapot212, $Teapot213, $Teapot214, $Teapot215, $Teapot216, $Teapot217, $Teapot218, $Teapot219, $Teapot220, $Teapot221, $Teapot222, $Teapot223, $Teapot224, $Teapot225, $Teapot226, $Teapot227, $Teapot228, $Teapot229, $Teapot230, $Teapot231, $Teapot232, $Teapot233, $Teapot234, $Teapot235, $Teapot236, $Teapot237, $Teapot238, $Teapot239, $Teapot240, $Teapot241, $Teapot242, $Teapot243, $Teapot244, $Teapot245, $Teapot246, $Teapot247, $Teapot248, $Teapot249, $Teapot250, $Teapot251, $Teapot252, $Teapot253, $Teapot254, $Teapot255, $Teapot256, $Teapot257, $Teapot258, $Teapot259, $Teapot260, $Teapot261, $Teapot262, $Teapot263, $Teapot264, $Teapot265, $Teapot266, $Teapot267, $Teapot268, $Teapot269, $Teapot270, $Teapot271, $Teapot272, $Teapot273, $Teapot274, $Teapot275, $Teapot276, $Teapot277, $Teapot278, $Teapot279, $Teapot280, $Teapot281, $Teapot282, $Teapot283, $Teapot284, $Teapot285, $Teapot286, $Teapot287, $Teapot288, $Teapot289, $Teapot290, $Teapot291, $Teapot292, $Teapot293, $Teapot294, $Teapot295, $Teapot296, $Teapot297, $Teapot298, $Teapot299, $Teapot300, $Teapot301)
		tp = #()
		TP = selection as array 
			
		for i = 1 to 300 do (WM3_MC_BuildFromNode $Teapot001.morpher i TP[i])	

		node151 = WM3_MC_GetTarget $Teapot001.morpher 150
		node202 = WM3_MC_GetTarget $Teapot001.morpher 201
		node151.name
		node202.name	
		assert_true (node151.name == "Teapot151")
		assert_true (node202.name == "Teapot202")
	),
		
	Tests = 
	#(
		testMorpherModifierValidity,
		testNumChannels,
		testChannelName,
		testChannelActiveState,
		testChannelHasDataState,
		testChannelIsValidState,
		testChannelMorphWeight,
		testChannelMorphTarget,
		testProgressiveMorphTension,
		testProgressiveMorphWeight,
		testGetProgressiveMorphTarget,
		testAddProgressiveMorphTarget,
		testDeleteProgressiveMorphTarget,
		testChannelSwap,
		testChannelMove,
		testChannelDelete,
		morph_MAXX_11745,
		morph_MAXX_25061,
		morph_MAXX_25369
	)
)

run_test_fixture UnitTest_Morpher script:(getThisScriptFilename())
