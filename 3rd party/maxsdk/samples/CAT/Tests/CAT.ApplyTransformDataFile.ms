
filein "CAT.UtilityFns.ms"
filein "CAT.GenerateNameMap.ms"
filein "CAT.TestTransformDataFile.ms"

-- globals
global newTransformData = undefined
struct TransformDataFileStruct (
	IDoNotExist = true
)

fn FindMatchingName strName nameMap =
(
	result = strName + " is Not In Map"
	for pair in nameMap do (
		if pair[1] == strName do ( result = pair[2] )
	)
	return result
)

-------------------------------------------------------------------------------------------------------------
-- Compare our current transforms to an original one
-- This fn iterates the CAT tree
fn ApplyToNode catnode nameMap = 
(
	-- find the name of the original node
	origName = FindMatchingName catnode.Name nameMap
	
	-- What is the correct name for this element?
	catname = GenerateCATTransformName origName
	-- does the property file include this data?
	if (PropertyExists newTransformData catname) then
	(
		tmOrigForNode = execute ("newTransformData." + catname)
		catnode.transform = tmOrigForNode
		
		isEquiv = IsMtxEquivalent catnode.transform tmOrigForNode
		if not(IsMtxEquivalent catnode.transform tmOrigForNode) do (
			Warn(catname + ": Has NOT matched original data")
		)
	)
	else
		Warn(catname + "\t Not found in data file")
)

-- Test the passed CAT rig (catparent) vs the data
-- file to see if its transforms are equivalent.
fn ApplyCATRigTransformations catrig originaldata originalNames = 
(
	-- Reset used data
	allCatNames = #()
	iUniqueTag = 1
	
	-- This file MUST define a struct called OriginalDataFileStruct
	if FileExists originaldata and FileExists originalNames then
	(		
		
		Note ("Applying " + originaldata + " to " + catrig.name)
		
		-- Create the data structure that holds the
		-- information we are testing against
		filein originaldata	-- Load the transform data
		newTransformData = TransformDataFileStruct()
		
		-- Create the name mapping from old rig to this one.
		nameMap = GenerateNameMap catrig originalNames
		
		-- Init CAT parameters to match data file
		if (PropertyExists newTransformData "CATName") do
		(
			catrig.CATName = newTransformData.CATName
		)
		if (PropertyExists newTransformData "tmCATParent") do
		(
			-- Skip the CATParentTrans SetValue, because it screws things up.
			-- TODO: Fix the SetValue, so we don't gotta do this.
			catrig.controller.prs.value = newTransformData.tmCATParent
		)
		if (PropertyExists newTransformData "CATUnits") do
		(
			catrig.CATUnits = newTransformData.CATUnits
		)
		if (PropertyExists newTransformData "LengthAxis") then
		(
			catrig.LengthAxis = newTransformData.LengthAxis
		)
		else
		(
			catrig.LengthAxis = "Z"
		)
		
		allNodes = catrig.CATRigNodes
		for catnode in allNodes do
		( 
			ApplyToNode catnode nameMap
		)
		
		allCatNames = #()
		iUniqueTag = 1
	)
	else 
	(
		Warn ("File not found: " + originaldata)
	)
)