-- The functions in this file provide an easy and convenient way to serialize a MxS representation
-- of a CAT (or any thing) hierarchy.  A file is created at outfilename, and the children of the hub of the
-- CAThierarchy are baked into a structed called TransformDataFileStruct.
-- The out file can be read directly as MaxScript.

filein"CAT.UtilityFns.ms"

fn WriteCATTransforms catnode outfile =
(
	-- remove any spaces from the string
	catname = GenerateCATTransformName catnode.name
	format (catname + " = " +  (catnode.transform as string) + ",\n") to:outfile
	
	--for child in catnode.children do 
	--	WriteCATTransforms child outfile
	
	--if getInterface catnode.controller "ExtraRigNodesInterface" != undefined do 
	--(
	--	for extraBone in catnode.controller.extraRigNodes do 
	--		WriteCATTransforms extraBone outfile
	--)
)

fn WriteTransformDataStruct cpParent outfilename =
(
	outfile = createfile outfilename 
	format "struct TransformDataFileStruct \n(\n" to:outfile
	
	-- Initialize out data stuff
	iUniqueTag = 1
	allCatNames = #()
	
	-- for Bip01, make sure we get it's root node (not the pelvis, an extra bone)
	--catroot = cpParent.roothub.node
	--while catroot.parent != undefined do
	--(
	--	catroot = catroot.parent
	--)
	
	for rignode in cpParent.CATRigNodes do 
	(
		WriteCATTransforms rignode outfile
	)
	--WriteCATTransforms cpParent outfile
	--WriteCATTransforms catroot outfile
	
	format ("CATName = \"" + cpParent.CATName + "\",\n") to:outfile
	format ("CATUnits = " + (cpParent.CATUnits as string) + ",\n") to:outfile
	-- Support CAT 1.4
	if PropertyExists cpParent "LengthAxis" do 
	(
		format ("LengthAxis = \"" + cpParent.LengthAxis + "\",\n") to:outfile
	)
	
	if PropertyExists cpParent.controller "PRS" then
	(
		-- Access the PRS controller directly, this avoids weird CAT hacking of the input SetValues on CATParentTrans.
		format ("tmCATParent = " + (cpParent.controller.prs.value as string) + "\n)") to:outfile
	)
	else
	(
		format ("tmCATParent = " + (cpParent.controller.value as string) + "\n)") to:outfile
	)
	
	-- clean up
	allCatNames = #()
	iUniqueTag = 1
	close outfile
)