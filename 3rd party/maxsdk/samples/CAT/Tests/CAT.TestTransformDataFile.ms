
filein "CAT.UtilityFns.ms"

fn IsVecEquivalent pLeft pRight errorMargin:0.00005 =
(
	eq = true
	if (pLeft != pRight)  do 
	(
		if (verbosity > 3) do (
			print ((pLeft as string) + ":" + (pRight as string))
		)
		eq = false
		lenLeft = length pLeft
		lenRight = length pRight
		if (verbosity > 3) do ( 
			print (abs (lenLeft - lenRight) as string + " : " + (errorMargin * ((lenLeft + lenRight) / 2)) as string)
		)
		-- Compare equivalence of vector length.  Our error margin is the allowable difference in
		-- length between the 2 vectors.  That means if our vectors have an average length of 1000, and margin
		-- is 0.005%, the maximum variability between the 2 vectors 999.975 to 1000.025
		-- subject to float limits of course ( the two lengths would actually fail, because the numbers
		-- exceed the float limits.
		if (abs (lenLeft - lenRight) <= (errorMargin * ((lenLeft + lenRight) / 2))) then (
			-- Calculate the angle between the vectors.  The closer to 1 this is, the smaller the angle
			dirTolerance = (dot pLeft pRight) / (lenLeft * lenRight)
			if (verbosity > 3) do (
				print ("Angle Tolerance : " + dirTolerance as string)
			)
			eq = (dirTolerance > (1 - errorMargin) and dirTolerance < (1 + errorMargin))
			if not eq do (Warn (dirTolerance as string))
		)
	)
	eq
)

fn IsMtxEquivalent mtxLeft mtxRight margin:0.00005 =
(
	-- We give the position a greater margin of tolerance here, because even very small
	-- rounding errors will scale to become larger position errors.  These errors are unavoidable
	-- (and acceptable), so we let the position errors grow much larger before failing this test.
	eq = (IsVecEquivalent mtxLeft[1] mtxRight[1] errorMargin:margin) and (IsVecEquivalent mtxLeft[2] mtxRight[2] errorMargin:margin) and (IsVecEquivalent mtxLeft[3] mtxRight[3] errorMargin:margin) and (IsVecEquivalent mtxLeft[4] mtxRight[4] errorMargin:(margin * 5))
	eq
)

-- globals
global writeTransformData = undefined
struct TransformDataFileStruct (
	IDoNotExist = true
)

-------------------------------------------------------------------------------------------------------------
-- Compare our current transforms to an original one
-- This fn iterates the CAT tree
fn CompareToOriginal catnode margin:0.00005 = 
(
	success = true
	-- Compare against original.  tmOriginals should
	-- have a member variable matching the name of the current node.
	try
	(
		catname = GenerateCATTransformName catnode.Name
		-- does the property file include this data?
		if (PropertyExists writeTransformData catname) then
		(
			tmOrigForNode = execute ("writeTransformData." + catname)
			tmCurrent = catnode.transform
			
			isEquiv = true
			if (classof catnode.controller == CATBoneSegTrans and catnode.controller.SegID > 1) then
			(
				-- BoneSegs have a slightly laxer margin of error.  This is because the
				-- the bone seg blending algo was tweaked to get a better result
				isEquiv = IsMtxEquivalent tmCurrent tmOrigForNode margin:(margin * 10)
			)
			else
			(
				isEquiv = IsMtxEquivalent tmCurrent tmOrigForNode margin:margin
			)
			if not (isEquiv) do
			(
				Warn ("Transform Difference - " + catnode.Name)
				Warn ("Curr: " + (tmCurrent as string) + " Orig: " + (tmOrigForNode as string))
				success = false
			)
		)
		else
		(
			-- Old versions may not include named CATParent data, so just ignore it
			if (classof catnode != CATParent) do
			(
				Error ("No matching original data for: " + catnode.name)
				success = false
			)
		)
	)
	catch
	(
		except = getCurrentException()
		print except
		success = false
		success
	)
	
	success 
)

-- Test the passed CAT rig (catparent) vs the data
-- file to see if its transforms are equivalent.
-- Returns true if testing happened, false if it couldn't find the origDataFile
fn TestCATRigTransformations catrig origDataFile margin:0.00005 = 
(
	-- Reset used data
	allCatNames = #()
	iUniqueTag = 1
	
	-- This file MUST define a struct called origDataFileFileStruct
	if FileExists origDataFile then
	(		
		filein origDataFile
		
		-- Create the data structure that holds the
		-- information we are testing against
		writeTransformData = TransformDataFileStruct()
		
		-- Init CAT parameters to match data file
		allPropNames = getPropNames writeTransformData
		if (0 != findItem allPropNames #CATName) do
		(
			catrig.CATName = writeTransformData.CATName
		)
		if (0 != findItem allPropNames #CATUnits) do
		(
			catrig.CATUnits = writeTransformData.CATUnits
		)
		if (0 != findItem allPropNames #LengthAxis) do
		(
			catrig.LengthAxis = writeTransformData.LengthAxis
		)
		if (0 != findItem allPropNames #tmCATParent) do
		(
			-- Set CATParent first, so everything else is in the right place relative to it
			catrig.controller.prs.value = writeTransformData.tmCATParent
		)
		for rignode in catrig.CATRigNodes do (
			CompareToOriginal rignode margin:margin
			--print ("Comparing: " + rignode.name + " with success: " + (res as string))
		)
		allCatNames = #()
		iUniqueTag = 1
		
		true
	)
	else
	(
		-- return false, we didn't test anything
		assert_true(false)
		false
	)
)

fn TestCATSetValueDrift catrig = 
(
	success = true
	for catnode in catrig.catrignodes do 
	(
		if (catnode != catrig) do
		(
			tmCurrent = catnode.transform
			-- Skip BoneSegTrans that aren't the first bone
			if (classof catnode.controller != BoneSegTrans or catnode.controller.SegID == 1) do
			(
				Note ("Drift Testing: " + catnode.name)
				-- Reset the same position 50 times, to try and catch drift
				for i = 1 to 50 do 
				(
					catnode.transform = catnode.transform
				)
				if not (IsMtxEquivalent tmCurrent catnode.transform margin:0.005) do
				(
					Error ("Drifiting Found - " + catnode.Name)
					Warn ("Curr: " + (tmCurrent as string) + " Orig: " + (tmOrigForNode as string))
					success = false
					break
				)
			)
		)
	)
	assert_true(success)
	if not success do
	(
		Error ("SetValue drifting failed for: " + catrig.CATName)
	)
	success
)
