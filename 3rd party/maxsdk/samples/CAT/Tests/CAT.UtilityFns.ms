
-- Verbosity:  
--	0: Dont print anything
--	1: Print Only Errors
--	2: Print Errors and Warnings
--	3: Print Warnings, Errors, and Notes
--	4: Print All, And the IsEquivalent stuff too
global int verbosity = 1

-- Logging functions 
fn Note str =
(
	if (verbosity > 2) do ( print ("NOTE: " + str) )
)
fn Warn str = 
(
	if (verbosity > 1) do ( print ("Warning: " + str) )
)
fn Error str = 
(
	if (verbosity > 0) do print ("Error: " + str)
)

-- Find the local directory where rigs are kept
fn GetRigDir =
(
	v = maxVersion()
	dir = 0
	if (v[1] < 14000) then ( dir = GetDir #plugcfg + "\\CAT\\CATRigs\\" )
	else ( dir = GetDir #plugcfg_ln + "\\CAT\\CATRigs\\" )
	dir
)

fn GetTestingDataDir = 
(
	scriptPath = getSourceFileName()
	dataPath = getFilenamePath scriptPath
	dataPath = dataPath + "CATTestingData\\"
)

fn GetTestingDataFile testFeature rigFileName =
(
	v = GetTestingDataDir() + testFeature + "/" + rigFileName + "TransData.ms"
	v
)

fn getFilesRecursive root pattern =
(
	local my_files = getFiles (root + "/" + pattern)
		
	local dir_array = GetDirectories (root+"/*")
	for d in dir_array do
		join my_files (getFilesRecursive d pattern)
	my_files
)

-- Iterate over all files in a directory structure, and return
-- a list of all rig and rg3 files under the root directory
fn getAllRigFilesRecursive root =
(
	rigfiles = getFilesRecursive root "*.rig"
	join rigfiles (getFilesRecursive root "*.rg3")
	rigfiles
)

-- Does the passed file exist?
fn FileExists fname = (getfiles fname).count != 0

-- Is the named property part of this file?
fn PropertyExists entity propname = 
(
	allProps = getPropNames entity
	itemHash = execute("#" + propname)
	result = (0 != findItem allProps itemHash)
	result --return result
)

-- Utility fn allows loading the rig in either 2011 or 2012
fn LoadRig rigfile = 
(
	-- If we are loading an RG3 rig older than Tekken, 
	-- then use the LoadFile command to fix the file-units issue
	if (getFilenameType rigFile == ".rg3") and (maxVersion())[1] < 15000 then
	(
		-- LoadFile, skip the LoadRig bit.  Its bit of a cheat, but older
		-- versions of Max can't handle merging files with different sized units
		loadMaxFile rigfile useFileUnits:true quiet:true
		for o in objects do 
		(
			if (classof o == CATParent) do
			(
				-- Hack... ensure we return the base CATParent in Marama rigs
				if (findString o.Name "Face" == undefined) do
				(
					return o
				)
			)
		)
	)
	else	
	(
		-- Load Rig
		cpParent = CATParent()
		cpParent.CATUnits = 0.3f
		
		v2011or2012 = cpParent.LoadRig rigfile
		if classof v2011or2012 == CATParent then
			cpParent = v2011or2012
			
		cpParent
	)
)

allCatNames = #()
iUniqueTag = 1
-- Generate a unique name for a CAT node.
-- Used by transform Write/Test
fn GenerateCATTransformName catname =
(
	-- Can't use substituteString in Max7
--	catname = substituteString catname " " "_"
	for i = 1 to catname.count do (
		if (catname[i] == " ") do ( catname[i] = "_" )
	)
			
	-- ensure the name is unique
	while (findItem allCatNames catname) != 0 do
	(
		catname = catname + (iUniqueTag as string)
		iUniqueTag = iUniqueTag + 1
	)
	append allCatNames catname
	catname = "tm" + catname
	catname
)
