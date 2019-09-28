-- CAT mxs UnitTest Script
-- Written for Coverage, not for Workflow
-- Susan Amkraut 10/1/2009

fileIn "MxsUnit/MxsUnit.ms"

struct CATTestFixture
(

private
	cp = 0, -- this will become a catParent with a rig loaded onto it, and an animation layer, used by many functions
	
public
	fn setUp =
	(
--		break()
	),
	
	fn tearDown =
	(
--		break()
	),

	fn testCATParentInterface = 
	(
		print "--- CATParent Interface ---"
		-- Properties
		cp = catparent()
		showProperties cp
		a = cp.CATMode
		cp.CATMode = a
		assert_equal cp.CATMode a
		a = cp.CATName
		cp.CATName = a
		assert_equal cp.CATNAme a
		a = cp.CATUnits
		cp.CATUnits = a
		assert_equal cp.CATUnits a
		a = cp.ColourMode
		cp.ColourMode = a
		assert_equal cp.ColourMode a
		a = cp.LengthAxis
		cp.LengthAxis = a
		assert_equal cp.LengthAxis a
		a = cp.Node 
		a = cp.RootHub
		a = cp.CATVersion
		a = cp.CATRigSpace
		a = cp.CATRigNodes
		a = cp.CATRigLayerCtrls
		a = cp.RootTransformNode
		-- Methods
		-- load /save rig
		fname = getdir #plugcfg_ln
		fname = fname + "\CAT\CATRigs\Marine\Marine_NoMesh_NoBones.rg3"
		-- Loading a rig will delete the original node, and
		-- return a new one.  If the op is successful
		newcp = cp.loadrig fname
		assert_not_equal newcp undefined
		max tool zoomextents all
		cp = newcp
		fname = getdir #plugcfg_ln
		fname = fname + "\CAT\CATRigs\Test.rg3"
		res = cp.saverig fname
		assert_true res
		deletefile fname
		
		cp.getfiletagvalue fname "CharacterName"
		
		-- others
		cp2 = catparent()
		cp2.addhub()
		cp.UpdateUserProps()
		cp2.AddRootNode()
		delete cp2
	),
	
	fn testLayerRootInterface = 
	(
		print "--- LayerRoot Interface ---"
		-- Properties
		a = cp.NumLayers
		a = cp.SelectedLayer
		cp.SelectedLayer = a
		assert_equal cp.SelectedLayer a
		a = cp.SoloLayer
		cp.SoloLayer = a
		assert_equal cp.SoloLayer a
		a = cp.TrackDisplayMethod
		cp.TrackDisplayMethod = a
		assert_equal cp.TrackDisplaymethod a
		/* there are problems here.  It's not even clear this should be exposed.
		a = cp.Layers
		cp.Layers = a
		assert_equal cp.Layers a -- produces assertion error
		*/
		-- Methods
		-- load/save clip
		cp.LengthAxis = "Z" -- need to change in order to load this particular clip
		fname = getdir #plugcfg_ln
		fname = fname + "\CAT\Clips\BackFlip.clp"
		cp.LoadClip fname 0
		fname = getdir #plugcfg_ln
		fname = fname + "\CAT\Clips\Test.clp"
		res = cp.SaveClip fname 0 100 1 1
		assert_equal res true
		deletefile fname
		-- add/remove/move layers (add 2, flip positions, remove - keep backflip)
		res = cp.appendlayer "myAbsLayer" #absolute
		assert_equal res 2
		res = cp.insertlayer "myCatLayer" 2 #CATMotion
		assert_equal res true
		cp.movelayerup 3
		cp.movelayerdown 2
		cp.removelayer 3
		cp.removelayer 2
		-- save/load pose (must have a layer and be in CAT animate mode to save pose)
		cp.CATMode = 1 -- turn off setup mode, go into animate mode
		fname = getdir #plugcfg_ln
		fname = fname + "\CAT\Poses\Walk.pse"
		res = cp.savepose fname
		assert_equal res true
		cp.loadpose fname 0
		deletefile fname
		-- more layer methods
		cp.CreatePasteLayerTransformNode()
		cp.CopyLayer 1
		cp.PasteLayer() -- makes a second layer same as the first (docs have wrong #params)
		cp.RemoveLayer 2 -- remove the second layer
		cp.CATMode = 0 -- turn off animate and go into setup mode
		res = cp.collapsetimerangetolayer 0 100 5 -- can't get this to do anything visible
		assert_equal res true
		cp.collapseposetocurlayer() -- seems to collpase the setup pose to frame 0 of selected layer
		-- load different file types (future tests)
		-- cp.LoadHTR fname camfile
		-- cp.LoadBVH fname camfile
		-- cp.LoadFBX fname camfile
		-- cp.LoadBIP fname camfile
	),

	fn testLayerControlInterface = 
	(
		print "--- LayerControl Interface ---"
		-- Properties
		arm = $Marine_NoMesh_NoBonesRUpperarm3
		c = arm[3].layertrans
		c = c.controller
		c.layerroot
		c.weights
		a = c.usesetupcontroller
		c.usesetupcontroller = a
		assert_equal c.usesetupcontroller a
		c.setupcontroller
		a = c.additivetosetupvalue
		c.additivetosetupvalue = a
		assert_equal c.additivetosetupvalue a
		c.selectedlayerctrl
		c.numlayers
		c.layercontrollers
		-- Methods
		cp.selectedLayer = 1 -- need this to do the next operation
		c.BakeCurrentLayerSettings()
	),
	
	fn testLayerMatrix3Interface = 
	(
		print "--- LayerMatrix3 Interface ---"
		arm = $Marine_NoMesh_NoBonesRUpperarm3
		c = arm[3].layertrans
		c = c.controller
		-- Properties
		a = c.setupVal
		c.setupVal = a
	),
	
	fn testCATControlInterface = 
	(
		print "--- CATControl Interface ---"
		-- Properties
		arm = $Marine_NoMesh_NoBonesRUpperarm3
		ctrl = arm.transform.controller
		ctrl.CATParent
		a = ctrl.Name
		ctrl.Name = a
		assert_equal ctrl.name a
		-- Methods (incomplete... no time)
		/*
		  <void>PasteLayer <control>source <integer>fromindex <integer>toindex <boolean>instance
			<void>PasteRig <control>source <boolean>mirrordata
			<boolean>SaveClip <TSTR>filename <time>StartTime <time>EndTime
			<node>LoadClip <TSTR>filename <time>time <boolean>mirrordata
			<boolean>SavePose <TSTR>filename
			<node>LoadPose <TSTR>filename <time>time <boolean>mirrordata
			<void>CollapsePoseToCurLayer()
			<void>CollapseTimeRangeToCurrLayer <time>StartTime <time>EndTime <time>Frequency
			<void>ResetTransforms()
		*/
	),
	
	fn testBoneAccess = 
	(
		print "--- bone access ---"
		arm = $Marine_NoMesh_NoBonesRUpperarm3
		adr = arm[3].address -- gets the general cat bone "address"
		arm2 = cp.getbonebyaddress adr -- this should return the upper arm
		assert_equal arm arm2
		cp.CATRigNodes -- returns all the bones in an array
		cp.CATRigLayerCtrls -- returns all the layer controllers in a rig
		rib = $Marine_NoMesh_NoBonesRibcage
		rib[3].layertrans.controller -- gets the cat node control interface
		pelv = $Marine_NoMesh_NoBonesPelvis
		numlimbs = pelv[3].limbs.count -- number of limbs attached to pelvis hub
		assert_equal numlimbs 2
	),
	
	fn testCATNodeControlInterface = 
	(
		print "--- CATNodeControl Interface ---"
		arm = $Marine_NoMesh_NoBonesRUpperarm3
		-- Properties:
		ctrl = arm.transform.controller -- get cat arm control
		ctrl.numarbbones
		ctrl.node
		ctrl.address
		a = ctrl.SetupTM
		ctrl.SetupTM = a
		ctrl.BoneDimensions
		a = ctrl.SetupStretchy
		ctrl.SetupStretchy = a
		assert_equal ctrl.SetupStretchy a
		a = ctrl.EffectHierarchy
		ctrl.EffectHierarchy = a
		assert_equal ctrl.EffectHierarchy a
		ctrl.NumLayerFloats
		a = ctrl.SetupModeLocks
		ctrl.SetupModeLocks = a
		assert_equal ctrl.SetupModeLocks.count a.count
		a = ctrl.AnimationLocks
		ctrl.AnimationLocks = a
		assert_equal ctrl.AnimationLocks.count a.count
		a = ctrl.MirrorBone
		ctrl.MirrorBone = a
		assert_equal ctrl.MirrorBone a
		-- Methods
		-- the following are commented out because they all crash due to the same bug
		-- they also crash in the UI (bug was logged)
		-- I call these down below on a different rig and it works
		--ctrl.AddArbBone()
		--ctrl.GetArbBone 1
		--ctrl.CreateLayerFloat()
	),
	
	fn testExtraRigNodesInterface = 
	(
		print "--- ExtraRigNodes Interface ---"
		arm = $Marine_NoMesh_NoBonesRUpperarm3
		ctrl = arm.transform.controller -- get cat arm control
		-- Properties & Methods
		bone()
		b = #($bone001)
		ctrl.AddExtraRigNodes b
		rignodes = ctrl.extrarignodes
		assert_equal rignodes[1] $bone001
		a = ctrl.ExtraRigNodes
		ctrl.ExtraRigNodes = a
		ctrl.RemoveExtraRigNodes b
		rignodes = ctrl.extrarignodes
		assert_equal rignodes.count 0
		delete $bone001
	),
	
	fn testHubInterface = 
	(
		print "--- Hub Interface ---"
		pelvis = $Marine_NoMesh_NoBonesPelvis
		ctrl = pelvis.transform.controller
		-- Properties
		a = ctrl.pinhub
		ctrl.pinhub = a
		assert_equal ctrl.pinhub a
		-- Methods
		-- create new catparent and add parts to it
		-- this rig won't have any animation
		cp2 = catparent()
		cp2.addhub()
		max tool zoomextents all
		ctrl = cp2.roothub
		ctrl.addarm()
		ctrl.addleg()
		ctrl.addspine()
		ctrl.addtail()
		max tool zoomextents all
	),
	
	fn testLimbInterface = 
	(
		print "--- Limb Interface ---"
		leg = $Marine_NoMesh_NoBonesRThigh1
		legctrl = leg[3][1]
		-- Properties
		legctrl.IKTarget
		legctrl.UpNode
		legctrl.IsLeg
		legctrl.IsArm
		a = legctrl.LMR
		legctrl.LMR = a
		assert_equal legctrl.LMR a
		legctrl.SymLimb
		legctrl.Palm
		legctrl.Collarbone
		-- Methods
		legctrl.createIKTarget()
		-- legctrl.removeIKTarget() -- crash
		legctrl.MoveIKTargetToEndOfLimb 0
		legctrl.createUpNode()
		-- legctrl.removeUpNode() -- crash
		legctrl.createPalmAnkle()
		legctrl.removePalmAnkle()
		legctrl.createCollarBone()
		legctrl.removeCollarBone()
	),
	
	-- CATNodeControl Methods not called above due to crash
	-- These methods must be called on the new rig created above to avoid the crash
	fn testCATNodeControlInterface2 = 
	(
		print "--- CATNodeControl Interface ---"
		arm = $Marine_NoMesh_NoBonesLUpperarm2
		ctrl = arm.transform.controller
		a = ctrl.AddArbBone()
		b = ctrl.GetArbBone 1
		assert_equal a b
		ctrl.CreateLayerFloat()
	),
	
	fn testLayerInfoInterface = 
	(
		print "--- LayerInfo Interface ---"
		li = cp.layers.controller[1].controller
		-- Properties
		a = li.LayerName
		li.LayerName = a
		assert_equal li.LayerName a
		a = li.LayerColour
		li.LayerColour = a
		assert_equal li.LayerColour a
		li.LayerType
		a = li.LayerEnabled
		li.layerEnabled = a
		assert_equal li.layerEnabled a
		a = li.TransformNodeOn
		li.TransformNodeOn = a
		assert_equal li.TransformNodeOn a
		a = li.RemoveDisplacement
		li.RemoveDisplacement = a
		assert_equal li.RemoveDisplacement a
		li.LayerIndex
		li.CATParent
		li.weights
		li.timewarp
		li.transform
		-- Methods
		a = li.GetTransformNode()
		assert_undefined a
	),
	
	fn testCATObjectInterface = 
	(
		print "--- CATObject Interface ---"
		leg = $Marine_NoMesh_NoBonesRThigh1
		-- Properties
		leg.TMController
		a = leg.UseCustomMesh
		leg.UseCustomMesh = a
		assert_equal leg.useCustomMesh a
		-- Methods
		box()
		leg.copymeshfromnode $box002
		-- leg.PasteRig <object>pasteobject <boolean>mirrordata (not sure what to send this)
		delete $box002
	),
	
	fn endall =
	(
			resetMaxFile #noprompt 
	),
	
	-- The tests must be performed in this order
	tests = 
	#(
		testCATParentInterface
		, testLayerRootInterface
		, testLayerControlInterface
		, testLayerMatrix3Interface
		, testCATControlInterface
		, testBoneAccess
		, testCATNodeControlInterface
		, testExtraRigNodesInterface
		, testHubInterface
		, testLimbInterface
		, testCATNodeControlInterface2
		, testLayerInfoInterface
		, testCATObjectInterface
		,endall
	)
)

run_test_fixture CATTestFixture script:(getThisScriptFilename())