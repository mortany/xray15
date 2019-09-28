filein "CAT.ApplyTransformDataFile.ms"

fn CreateCATRigForAnim = 
(
	catrig = LoadRig "C:\Code\CAT\CAT Demo Material\Relic Files\MetaRig2012.rig"
	catrig.AppendLayer "BakedLayer" "ABSOLUTE"
	catrig.CATMode = 1 -- Set to animation mode
	-- Set limbs to FK
	allNodes = catrig.CATRigNodes
	for catnode in allNodes do
	( 
		if (classof catnode.controller == BoneSegTrans) do 
		(
			catnode.controller.limb.layerIKFKRatio = 1
		)
	)
	catrig
)

-- Call this function to do everything
fn ImportCATAnimations sourcedir =
(
	files = getFiles (sourcedir + "/*")
	for f in files do (
		srcFilename = (filenameFromPath f)
		
		-- split out the filename to find the appropriate components
		animData = filterString srcFilename "."
		
		-- The first element is the name of the CATParent: Find it
		catrig = execute ("$" + animData[1])
		-- If cpt == NULL, then we need to create a new CPT
		if (catrig == undefined) do
		(
			catrig = CreateCATRigForAnim()
		)
		
		-- The 2nd element should be frame
		if (animData[2] != "Frame") do
		(
			print ("Damn.  What do we have here? " + f)
			continue
		)
		
		frameNum = animData[3] as float
		(
			with animate on 
			(
				at time frameNum ApplyCATRigTransformations catrig f
			)
		)
	)
)