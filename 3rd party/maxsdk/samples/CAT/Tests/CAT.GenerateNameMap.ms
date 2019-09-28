-- The point of this file is to create a 2d array that allows us to map names on an existing rig to names that have been
-- baked via the WriteRigNameStruct function.  Call GenerateNameMap, passing existing catparent and the name of the
-- file written by WriteRigNameStruct, and then use the returned array to find the mapping from old rig to new one.

fn min a b = (
	res
	if a > b then ( res = b )
	else (res = a)
	res
)

fn GenerateNameMapTail newTail structTail = 
(
	nBones = min(newTail.bones.count structTail.bones.count)
	for i = 1 to nBones do
	(
		append nameMap #(newTail.bones[i].node.name, structTail.bones[i])
	)
)

fn GenerateNameMapLimb newLimb structLimb nameMap = 
(
	nBones = min newLimb.bones.count structLimb.bones.count
	for i = 1 to nBones do
	(
		append nameMap #(newLimb.bones[i].BoneSegs[1].node.name, structLimb.bones[i])
	)
	
	if (newLimb.collarbone != undefined and structLimb.collarbone != undefined) do
	(
		append nameMap #(newLimb.collarbone.node.name, structLimb.collarbone)
	)
	if (newLimb.palm != undefined and structLimb.palm != undefined) do
	(
		newPalm = newLimb.palm
		structPalm = structLimb.palm
		append nameMap #(newPalm.node.name, structPalm.name)
		nDigits = min newPalm.digits.count structPalm.digits.count
		for i = 1 to nDigits do
		(
			newDigit = newPalm.digits[i]
			structDigit = structPalm.digits[i]
			nDigitSegs = min newDigit.Bones.count structDigit.count
			for j = 1 to nDigitSegs do 
			(
				append nameMap #(newDigit.bones[j].node.name, structDigit[j])
			)
		)
	)
	if (newLimb.IKTarget != undefined and structLimb.IKTarget != undefined) do
	(
		append nameMap #(newLimb.IKTarget.name, structLimb.IKTarget)
	)
)

fn GenerateNameMapHub newRootHub structRootHub nameMap = 
(
	Append nameMap #(newRootHub.node.name, structRootHub.name)
	
	-- Find a matching limb on this pRootHub
	-- Our criteria is simply LMR
	for pLimb in newRootHub.Limbs do 
	(
		for i = 1 to structRootHub.Limbs.Count do
		(
			pLimbStruct = structRootHub.Limbs[i]
			if (pLimbStruct != undefined) and (pLimbStruct.LMR == pLimb.LMR) do
			(
				GenerateNameMapLimb pLimb pLimbStruct nameMap
				structRootHub.Limbs[i] = undefined -- its been matched now
				break
			)
		)
	)
	
	-- No ,matching criteria for tails
	nTails = min newRootHub.Tails.count structRootHub.tails.count
	for i = 1 to nTails do
	(
		GenerateNameMapTail newRootHub.Tails[i] structRootHub.tails[i]
	)
	
	-- Neither for hubs
	nChildHubs = min newRootHub.Spines.count structRootHub.Spines.count
	for i = 1 to nChildHubs do
	(
		GenerateNameMapHub newRootHub.Spines[i].tiphub structRootHub.Spines[i] nameMap
	)
)


-- Create a 2D array, where each element is an array containing the current name of a CAT element and its matching
-- name from the CAT rig structure passed
fn GenerateNameMap cpParent catStructFile = 
(
	nameMap = #()
	filein catStructFile
	
	
	newRootHub = cpParent.roothub
	structRootHub = pRootNode
	
	GenerateNameMapHub newRootHub structRootHub nameMap
	nameMap
)