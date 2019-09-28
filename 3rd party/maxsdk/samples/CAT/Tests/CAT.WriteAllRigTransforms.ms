filein "CAT.WriteTransformDataFile.ms"
filein "CAT.UtilityFns.ms"

fn WriteAllCATRigsTransformHierarchy  =
(
	rigdir = GetRigDir()
	files = getAllRigFilesRecursive rigdir	
	for f in files do (
		
		-- Load Rig
		cpParent = LoadRig f
		forceCompleteRedraw()
		rigFilename = filenameFromPath f
		-- Write out the rig in "X" length axis pose
		cpParent.LengthAxis = "X"
		outFileName = GetTestingDataFile "AllRigsX" rigFilename
		WriteTransformDataStruct cpParent outfilename

		-- Write out the rig in "Z" length axis pose
		cpParent.LengthAxis = "Z"
		outFileName = GetTestingDataFile "AllRigsZ" rigFilename
		WriteTransformDataStruct cpParent outfilename
		
		-- Delete the rig
		resetMaxFile #noprompt 
	)
)
