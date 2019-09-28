fileIn "MxsUnit/MxsUnit.ms"

struct mxsagni_misc_Tests
(
	fn test_sysinfo_MAXpriority_enum_values = 
	(
		local current_priority = sysinfo.MAXpriority
		local read_write_priorities = #(#low,#belowNormal,#normal,#aboveNormal,#high)
		for p in read_write_priorities do (sysinfo.MAXpriority = p;assert_equal p sysinfo.MAXpriority)
		-- special case - setting #backgroundBegin doesn't change reported priority in win7, but does set priority to #low in win10
		for p in read_write_priorities do 
		(	
			sysinfo.MAXpriority = p;
			sysinfo.MAXpriority = #backgroundBegin
			assert_true (sysinfo.MAXpriority == p or sysinfo.MAXpriority == #low)
			sysinfo.MAXpriority = #backgroundEnd
			assert_equal p sysinfo.MAXpriority
		)
		-- special case - setting realtime priority sets priority to high if not running with admin priv. 
		sysinfo.MAXpriority = #realtime
		assert_true (sysinfo.MAXpriority == #realtime or sysinfo.MAXpriority == #high)
		sysinfo.MAXpriority = current_priority
	),
	
	fn test_get_set_ini =
	(
		local TEST_INI_FILE = @"$temp\get_set_ini_test_file.ini"
		deletefile TEST_INI_FILE
		local long_line = "XXXXXXXXXXXXXXXX"
		do long_line += long_line while long_line.count < 16384
		local res = setIniSetting TEST_INI_FILE "section" "key" long_line
		assert_true res
		local read_long_line = getINISetting TEST_INI_FILE "section" "key"
		assert_equal long_line.count read_long_line.count
		long_line += substring long_line 2 -1
		res = setIniSetting TEST_INI_FILE "section" "key" long_line
		assert_true res
		read_long_line = getINISetting TEST_INI_FILE "section" "key"
		assert_equal long_line.count read_long_line.count
		deletefile TEST_INI_FILE
		-- following will write / attempt to read a 32k key value. 
		long_line += "A"
		res = setIniSetting TEST_INI_FILE "section" "key" long_line
		assert_true res
		read_long_line = getINISetting TEST_INI_FILE "section" "key"
		-- due to GetPrivateProfileString behavior, we will get back a zero length string, usually....
		if read_long_line.count != long_line.count do
			assert_equal 0 read_long_line.count
	),
	
	fn test_SingleRefMaker_TypedSingleRefMaker =
	(
		local res = ValidateSingleRefMaker()
		assert_true res
	),

	fn test_used_in_scene =
	(
		resetmaxfile #noprompt
		local t1 = teapot()
		local m1=standardmaterial()
		assert_false (isMtlUsedInSceneMtl m1)
		assert_false (isUsedInScene m1)
		t1.material = m1
		assert_true (isMtlUsedInSceneMtl m1)
		assert_true (isUsedInScene m1)
		m1.diffusemap = dent()
		assert_true (isMtlUsedInSceneMtl m1.diffusemap)
		assert_true (isUsedInScene m1.diffusemap)
		local t2 = teapot()
		local m2=multimaterial()
		t2.material = m2
		assert_true (isMtlUsedInSceneMtl m2)
		assert_true (isUsedInScene m2)
		local subMtl = m2[1]
		assert_true (isMtlUsedInSceneMtl subMtl)
		assert_true (isUsedInScene subMtl)
		assert_false (isMtlUsedInSceneMtl subMtl topLevelMtlOnly:true)
		test_used_in_scene_ca= attributes test_used_in_scene_ca_def
		(
			parameters main 
			(
				myMtl type:#material 
				myTexMap type:#texturemap  
			)
		)
		custattributes.add t1 test_used_in_scene_ca
		local m3=standardmaterial()
		local tmap1 = dent()
		t1.myMtl = m3
		t1.myTexMap = tmap1
		assert_false (isMtlUsedInSceneMtl m3)
		assert_true (isUsedInScene m3)
		assert_false (isMtlUsedInSceneMtl tmap1)
		assert_true (isUsedInScene tmap1)
		
		assert_false (isMtlUsedInSceneMtl m3 skipCustAttributes:true)
		assert_false (isUsedInScene m3 skipCustAttributes:true)
		assert_false (isMtlUsedInSceneMtl tmap1 skipCustAttributes:true)
		assert_false (isUsedInScene tmap1 skipCustAttributes:true)
		
		assert_false (isMtlUsedInSceneMtl m3 skipCustAttributes:false)
		assert_true (isUsedInScene m3 skipCustAttributes:false)
		assert_false (isMtlUsedInSceneMtl tmap1 skipCustAttributes:false)
		assert_true (isUsedInScene tmap1 skipCustAttributes:false)
		
		custattributes.add m1 test_used_in_scene_ca
		m1.myMtl = m3
		m1.myTexMap = tmap1
		assert_false (isMtlUsedInSceneMtl m3)
		assert_false (isMtlUsedInSceneMtl tmap1)
		assert_false (isMtlUsedInSceneMtl m3 skipCustAttributes:true)
		assert_false (isMtlUsedInSceneMtl tmap1 skipCustAttributes:true)
		assert_true (isMtlUsedInSceneMtl m3 skipCustAttributes:false)
		assert_true (isMtlUsedInSceneMtl tmap1 skipCustAttributes:false)
		
		saveMaxFile @"$temp\test_used_in_scene.max" quiet:true
		
		assert_equal 2 sceneMaterials.count
		
		local nodeRefs = for i = 1 to (refs.getnumrefs t2) collect (refs.getreference t2 i)
		for r in nodeRefs where r != undefined do assert_true (isUsedInScene r) 
		
		delete t2
		assert_false (isMtlUsedInSceneMtl m2)
		assert_false (isUsedInScene m2)
		
		for r in nodeRefs where r != undefined and (classof r != Base_LayerBase_Layer) do assert_false (isUsedInScene r) message:("ref: " + r as string)
		for r in nodeRefs where r != undefined and (classof r == Base_LayerBase_Layer) do assert_true (isUsedInScene r) message:("ref: " + r as string)

		assert_equal 2 sceneMaterials.count
		UpdateSceneMaterialLib()
		assert_equal 1 sceneMaterials.count
		
		resetmaxfile #noprompt
		xrefs.addNewXRefFile @"$temp\test_used_in_scene.max" 
		local xref = xrefs.getXRefFile 1
		local mtl = xref.tree.children[1].material
		assert_false (isMtlUsedInSceneMtl mtl)
		assert_false (isUsedInScene mtl)
		assert_true (isMtlUsedInSceneMtl mtl includeXrefScenes:true)
		assert_true (isUsedInScene mtl includeXrefScenes:true)
		resetmaxfile #noprompt
	),

	fn meshop_attach_test_op count = 
	(
		with undo off with redraw off
		(
			t = Teapot pos:[-200,200,0] isSelected:on radius:10 segs:64
			converttomesh t
			copies = for i = 1 to (count-1) collect (c = copy t; c.pos.x += (i*10); c)
			for n in copies do meshop.attach t n
			copies = for i = 1 to (count-1) collect (c = copy t; c.pos.y += (i*10); c)
			for n in copies do meshop.attach t n
			copies = for i = 1 to (count-1) collect (c = copy t; c.pos.z += (i*10); c)
			for n in copies do meshop.attach t n
			copies = t = n = undefined
			delete objects
			ok
		)
	),
	
	fn meshop_attach_test = 
	(
		resetmaxfile #noprompt
		startMem = (((sysinfo.getMAXMemoryInfo())[3]/(1024*1024)) as integer)
		meshop_attach_test_op 4
		endMem = (((sysinfo.getMAXMemoryInfo())[3]/(1024*1024)) as integer)
		memLeakage = endMem - startMem
		format "meshop_attach_test leakage: %MB\n" memLeakage
		assert_true(endMem - startMem < 20) message:("leakage: " + memLeakage as string + "MB")
	),

	fn meshop_polyop_get_listarg_test =
	(
		acouple = #{1..3,6}
		
		b_mesh = box pos:[100,100,0]
		converttomesh b_mesh
		nverts = b_mesh.numVerts
		nfaces = b_mesh.numFaces
		
		verts1 = for i = 1 to nverts collect (meshop.getVert b_mesh i)
		verts2 = meshop.getVerts b_mesh #all
		assert_equal nverts verts2.count
		for i = 1 to nverts do assert_equal verts1[i] verts2[i]

		verts1 = for i in acouple collect (meshop.getVert b_mesh i)
		verts2 = meshop.getVerts b_mesh acouple
		assert_equal verts1.count verts2.count
		for i = 1 to verts1.count do assert_equal verts1[i] verts2[i]

		faces1 = for i = 1 to nfaces collect (getFace b_mesh i)
		faces2 = for i = 1 to nfaces collect (meshop.getFace b_mesh i)
		faces3 = meshop.getFaces b_mesh #all
		assert_equal nfaces faces3.count
		for i = 1 to nfaces do assert_equal faces1[i] faces2[i]
		for i = 1 to nfaces do assert_equal faces1[i] faces3[i]
		
		faces1 = for i in acouple collect (getFace b_mesh i)
		faces2 = for i in acouple collect (meshop.getFace b_mesh i)
		faces3 = meshop.getFaces b_mesh acouple
		assert_equal faces1.count faces3.count
		for i = 1 to faces1.count do assert_equal faces1[i] faces2[i]
		for i = 1 to faces1.count do assert_equal faces1[i] faces3[i]
		
		b_poly = box pos:[100,100,0]
		converttopoly b_poly 
		nfaces = polyop.getNumFaces b_poly
		nverts = polyop.getNumVerts b_poly
		nedges = polyop.getNumEdges b_poly
		
		verts1 = for i = 1 to nedges collect (polyop.getEdgeVerts b_poly i)
		verts2 = polyop.getEdgesVerts b_poly #all
		assert_equal nedges verts2.count
		for i = 1 to nedges do for j = 1 to verts1[i].count do assert_equal verts1[i][j] verts2[i][j]

		verts1 = for i in acouple collect (polyop.getEdgeVerts b_poly i)
		verts2 = polyop.getEdgesVerts b_poly acouple
		assert_equal verts1.count verts2.count
		for i = 1 to verts1.count do for j = 1 to verts1[i].count do assert_equal verts1[i][j] verts2[i][j]

		faces1 = for i = 1 to nedges collect (polyop.getEdgeFaces b_poly i)
		faces2 = polyop.getEdgesFaces b_poly #all
		assert_equal nedges faces2.count
		for i = 1 to nedges do for j = 1 to faces1[i].count do assert_equal faces1[i][j] faces2[i][j]

		faces1 = for i in acouple collect (polyop.getEdgeFaces b_poly i)
		faces2 = polyop.getEdgesFaces b_poly acouple
		assert_equal faces1.count faces2.count
		for i = 1 to faces1.count do for j = 1 to faces1[i].count do assert_equal faces1[i][j] faces2[i][j]

		verts1 = for i = 1 to nfaces collect (polyop.getFaceVerts b_poly i)
		verts2 = polyop.getFacesVerts b_poly #all
		assert_equal nfaces verts2.count
		for i = 1 to nfaces do for j = 1 to verts1[i].count do assert_equal verts1[i][j] verts2[i][j]

		verts1 = for i in acouple collect (polyop.getFaceVerts b_poly i)
		verts2 = polyop.getFacesVerts b_poly acouple
		assert_equal verts1.count verts2.count
		for i = 1 to verts1.count do for j = 1 to verts1[i].count do assert_equal verts1[i][j] verts2[i][j]

		edges1 = for i = 1 to nfaces collect (polyop.getFaceEdges b_poly i)
		edges2 = polyop.getFacesEdges b_poly #all
		assert_equal nfaces edges2.count
		for i = 1 to nfaces do for j = 1 to edges1[i].count do assert_equal edges1[i][j] edges2[i][j]

		edges1 = for i in acouple collect (polyop.getFaceEdges b_poly i)
		edges2 = polyop.getFacesEdges b_poly acouple
		assert_equal edges1.count edges2.count
		for i = 1 to edges1.count do for j = 1 to edges1[i].count do assert_equal edges1[i][j] edges2[i][j]

		verts1 = for i = 1 to nverts collect (polyop.getVert b_poly i)
		verts2 = polyop.getVerts b_poly #all
		assert_equal nverts verts2.count
		for i = 1 to nverts do assert_equal verts1[i] verts2[i]

		verts1 = for i in acouple collect (polyop.getVert b_poly i)
		verts2 = polyop.getVerts b_poly acouple
		assert_equal verts1.count verts2.count
		for i = 1 to verts1.count do assert_equal verts1[i] verts2[i]
	),
	
	fn getLogFileFileNumLines =
	(
		local nlines = 0;
		local f = openfile (logsystem.getNetLogFileName())
		try (while not (eof f) do (readline f; nlines +=1))
		catch ()
		close f
		nlines
	),

	fn test_systemTools_sceneIODebugEnabled =
	(
		local sceneIODebugEnabled = systemTools.sceneIODebugEnabled
		resetmaxfile #noprompt
		systemTools.sceneIODebugEnabled = false;
		local longevity = logsystem.longevity
		logsystem.longevity = #forever
		local nlines1 = getLogFileFileNumLines()
		holdmaxfile()
		local nlines2 = getLogFileFileNumLines()
		systemTools.sceneIODebugEnabled = true;
		holdmaxfile()
		local nlines3 = getLogFileFileNumLines()
		systemTools.sceneIODebugEnabled = sceneIODebugEnabled;
		logsystem.longevity = longevity
		format "test_systemTools_sceneIODebugEnabled : % : %\n" (nlines2 - nlines1) (nlines3 - nlines2)
		assert_true ((nlines3 - nlines2) > (nlines2 - nlines1 + 400))
	),

	fn test_getMAXClass =
	(
		assert_equal box (getMAXClass box.superclassid box.classid)
		local bad_cid = #(0xdeadL, 0xbeefL)
		assert_undefined (getMAXClass box.superclassid bad_cid)
		local c = (getMAXClass box.superclassid bad_cid create:true)
		assert_true (iskindof c maxclass)
		assert_false (c.creatable)
	),

	fn test_persistents =
	(
		persistent_globals = persistents.removeAll()
		persistent_globals = persistents.gather()
		assert_equal 0 persistent_globals.count

		execute "global my_non_persistent_global = 10"
		assert_true (globalVars.isGlobal #my_non_persistent_global)
		assert_false (persistents.isPersistent #my_non_persistent_global)
		globalVars.remove #my_non_persistent_global
		assert_false (globalVars.isGlobal #my_non_persistent_global)
		assert_false (persistents.isPersistent #my_non_persistent_global)

		execute "persistent global my_persistent_global = 10"
		persistent_globals = persistents.gather()
		assert_equal 1 persistent_globals.count
		assert_equal #my_persistent_global persistent_globals[1]
		assert_true (globalVars.isGlobal #my_persistent_global)
		assert_true (persistents.isPersistent #my_persistent_global)
		
		ss = stringStream ""
		persistents.show to:ss
		seek ss 0
		l = readline ss
		assert_equal #my_persistent_global ((filterstring l ":")[1] as name)
		persistents.remove #my_persistent_global
		assert_true (globalVars.isGlobal #my_persistent_global)
		assert_false (persistents.isPersistent #my_persistent_global)
		persistents.make #my_persistent_global
		assert_true (globalVars.isGlobal #my_persistent_global)
		assert_true (persistents.isPersistent #my_persistent_global)
		globalVars.remove #my_persistent_global
		assert_false (globalVars.isGlobal #my_persistent_global)
		assert_false (persistents.isPersistent #my_persistent_global)
	),
	
	fn test_memStreamMgr_eos =
	(
		s = "Hello World"
		f = memStreamMgr.openString s
		assert_false (f.eos())
		assert_string_equal (f.readline()) "Hello World"
		assert_true (f.eos())
		assert_string_equal (f.readline()) ""
		assert_true (f.eos())
		assert_string_equal (f.readline()) ""
		assert_string_equal (f.readline()) ""
		assert_string_equal (f.readline()) ""
		assert_string_equal (f.readline()) ""
		assert_string_equal (f.readline()) ""
		memStreamMgr.close f

		s = "Hello World\r"
		f = memStreamMgr.openString s
		assert_false (f.eos())
		assert_string_equal (f.readline()) "Hello World"
		assert_true (f.eos())
		assert_string_equal (f.readline()) ""
		assert_true (f.eos())
		memStreamMgr.close f

		s = "Hello World\r\n"
		f = memStreamMgr.openString s
		assert_false (f.eos())
		assert_string_equal (f.readline()) "Hello World"
		assert_true (f.eos())
		assert_string_equal (f.readline()) ""
		assert_true (f.eos())
		memStreamMgr.close f

		s = "Hello World\rHello World\r"
		f = memStreamMgr.openString s
		assert_false (f.eos())
		assert_string_equal (f.readline()) "Hello World"
		assert_false (f.eos())
		assert_string_equal (f.readline()) "Hello World"
		assert_true (f.eos())
		assert_string_equal (f.readline()) ""
		assert_true (f.eos())
		memStreamMgr.close f

		s = "Hello World\r\nHello World\r\n"
		f = memStreamMgr.openString s
		assert_false (f.eos())
		assert_string_equal (f.readline()) "Hello World"
		assert_false (f.eos())
		assert_string_equal (f.readline()) "Hello World"
		assert_true (f.eos())
		assert_string_equal (f.readline()) ""
		assert_true (f.eos())
		memStreamMgr.close f

		s = "Hello World\r\r"
		f = memStreamMgr.openString s
		assert_false (f.eos())
		assert_string_equal (f.readline()) "Hello World"
		assert_false (f.eos())
		assert_string_equal (f.readline()) ""
		assert_true (f.eos())
		assert_string_equal (f.readline()) ""
		assert_true (f.eos())
		memStreamMgr.close f

		s = "H"
		f = memStreamMgr.openString s
		assert_false (f.eos())
		assert_string_equal (f.peekchar()) "H"
		assert_string_equal (f.readchar()) "H"
		assert_true (f.eos())
		assert_string_equal (f.peekchar()) ""
		assert_string_equal (f.readchar()) ""
		assert_true (f.eos())
		assert_string_equal (f.peekchar()) ""
		assert_string_equal (f.readchar()) ""
		assert_string_equal (f.peekchar()) ""
		assert_string_equal (f.readchar()) ""
		assert_string_equal (f.peekchar()) ""
		assert_string_equal (f.readchar()) ""
		assert_string_equal (f.peekchar()) ""
		assert_string_equal (f.readchar()) ""
		assert_string_equal (f.peekchar()) ""
		assert_string_equal (f.readchar()) ""
		memStreamMgr.close f

		s = "H\r"
		f = memStreamMgr.openString s
		assert_false (f.eos())
		assert_string_equal (f.peekchar()) "H"
		assert_string_equal (f.readchar()) "H"
		assert_false (f.eos())
		assert_string_equal (f.peekchar()) "\r"
		assert_string_equal (f.readchar()) "\r"
		assert_true (f.eos())
		memStreamMgr.close f
	),
	
	fn test_getOSVersion = 
	(
		local osVersion = systemTools.GetOSVersion()
		assert_equal 3 osVersion.count
		assert_not_equal 0 osVersion[1] -- major version
--		assert_not_equal 0 osVersion[2]  -- minor version may be == 0
		assert_not_equal 0 osVersion[3] -- build number
	),
	
	fn test_maxversion =
	(
		version_info = maxversion() -- #(21000, 52, 0, 21, 2, 0, 0, 2019, ".2 Update")
		buildNumbers = filterstring (systemTools.GetBuildNumber()) "." -- #("21", "2", "0", "0")
		assert_equal 9 version_info.count
		assert_equal 4 buildNumbers.count
		assert_equal (buildNumbers[1] as integer) (version_info[4])
		assert_equal (buildNumbers[2] as integer) (version_info[5])
		assert_equal (buildNumbers[3] as integer) (version_info[6])
		assert_equal (buildNumbers[4] as integer) (version_info[7])
		assert_not_equal 0 (version_info[4])
	),
	
	fn test_getCommandLine =
	(
		deletefile @"$temp\test_getCommandLine.txt"
		local thisFileName = getSourceFileName()
		local thisDir = pathConfig.removePathLeaf thisFileName
		local scriptFile =  thisDir + "\\test_getCommandLine.mxs"
		local maxio_exe_path = symbolicPaths.getPathValue"$max" + "\\3dsmaxio.exe"
		local max_exe = "3dsmax.exe"
		if doesFileExist maxio_exe_path then
		(
			max_exe = "3dsmaxio.exe"
		)
        local cmd_log = pathConfig.appendPath (getDir #temp) "mxsagni_misc_Tests_hiddenDOScmd_output.log"
		local commandline = "\"\"" + symbolicPaths.getPathValue"$max" + "\\" + max_exe + "\" -silent -U MAXScript \"" + scriptFile + "\"" + " > \"" + cmd_log + "\"\""
		format "test_getCommandLine - running command: %\n" commandline
		HiddenDosCommand commandline donotwait:false ExitCode:&exitcode3dsmax
		assert_equal 0 exitcode3dsmax message:(" 3ds Max exit code is not 0: " + (exitcode3dsmax as string) )

        if (exitcode3dsmax != 0) do (
            fin = openfile cmd_log mode:"r"
            seek fin #eof
            maxlen = filepos fin
            seek fin 0
            res = readChars fin maxlen errorAtEOF:false
            format "3dsmax output:\n"
            format "%\n" res
            close fin
        )

		local f = openfile @"$temp\test_getCommandLine.txt"
		local l1 = readline f
		local l2 = readValue f
		close f
		
		assert_defined (findstring l1 max_exe)
		assert_defined (findstring l1 "-silent")
		assert_defined (findstring l1 "-U")
		assert_defined (findstring l1 "MAXScript")
		assert_defined (findstring l1 "test_getCommandLine")
		assert_equal "-silent" (l2[1])
		assert_equal "-U" (l2[2])
		assert_equal "MAXScript" (l2[3])
		assert_equal scriptFile (l2[4])
		assert_equal 4 l2.count
	),
	
	fn test_sysinfo_getMAX_XXX_Count =
	(
		local h = sysinfo.getMAXHandleCount()
		assert_not_equal 0 h
		assert_equal Integer (classof h)
		local u = sysinfo.getMAXUserObjectCount()
		assert_not_equal 0 u
		assert_equal Integer (classof u)
		local g = sysinfo.getMAXGDIObjectCount()
		assert_not_equal 0 g
		assert_equal Integer (classof g)
	),

	Tests = 
	#(
		test_sysinfo_MAXpriority_enum_values,
		test_get_set_ini,
		test_SingleRefMaker_TypedSingleRefMaker,
		test_used_in_scene,
		meshop_attach_test,
		meshop_polyop_get_listarg_test,
		test_systemTools_sceneIODebugEnabled,
		test_getMAXClass,
		test_persistents,
		test_memStreamMgr_eos,
		test_getOSVersion,
		test_maxversion,
		test_getCommandLine,
		test_sysinfo_getMAX_XXX_Count
	)
)

run_test_fixture mxsagni_misc_Tests script:(getThisScriptFilename())
