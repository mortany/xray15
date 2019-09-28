/*
@param TestName:        SkinWrap
@param Author:          philippe.groarke@autodesk.com
@param Created:         09/05/2018
@param LastMod:         09/05/2018
@param ModBy:           Lord Voldemort
@param Description:     Test face and vertex normal computation.
@param Status:          Online

Unit Test For
https://jira.autodesk.com/browse/MAXX-45311
*/

/**
 * HELPERS
 */
 
function check_face_normals skin_mesh skinwrap_mesh = (
	--print "Skin Face Normals"
	skin_face_normals = #()
	for i = 1 to (meshop.getNumFaces skin_mesh) do (
		--format "Face : %,\tNormal : %\n" i (getFaceNormal skin_mesh i)
		append skin_face_normals (getFaceNormal skin_mesh i)
	)
	
	--print "SkinWrap Face Normals"
	skinwrap_face_normals = #()
	for i = 1 to (meshop.getNumFaces skinwrap_mesh) do (
		--format "Face : %,\tNormal : %\n" i (getFaceNormal skinwrap_mesh i)
		append skinwrap_face_normals (getFaceNormal skinwrap_mesh i)
	)

	assert_equal skin_face_normals.count skinwrap_face_normals.count message:"cubes don't have same number of faces"

	for i = 1 to skin_face_normals.count do (
		assert_point3_equal skin_face_normals[i] skinwrap_face_normals[i] tolerance:0.01 message:"normals are broken"
	)
)

function check_vert_normals skin_mesh skinwrap_mesh = (
	--print "Skin Vertex Normals"
	skin_vert_normals = #()
	for i = 1 to (meshop.getNumVerts skin_mesh) do (
		--format "Vert : %,\tNormal : %\n" i (getNormal skin_mesh i)
		append skin_vert_normals (getNormal skin_mesh i)
	)

	--print "SkinWrap Vertex Normals"
	skinwrap_vert_normals = #()
	for i = 1 to (meshop.getNumVerts skinwrap_mesh) do (
		--format "Vert : %,\tNormal : %\n" i (getNormal skinwrap_mesh i)
		append skinwrap_vert_normals (getNormal skinwrap_mesh i)
	)

	assert_equal skin_vert_normals.count skinwrap_vert_normals.count message:"cubes don't have same number of faces"

	for i = 1 to skin_vert_normals.count do (
		assert_point3_equal skin_vert_normals[i] skinwrap_vert_normals[i] tolerance:0.01 message:"normals are broken"
	)
)


function BakeThat a num_verts = (
	a.filter_vertices = on
	skinOps.SelectVertices a #{1..num_verts}
	skinOps.bakeSelectedVerts a
)

/**
 * SET UP
 */
resetmaxfile(#noprompt)
print "Setup"

myMesh = (Box length:20.0 width:20.0 height:20.0 pos:[0,0,0] isSelected:off wirecolor:(color 255 255 255))
myMesh2 = (Box length:20.0 width:20.0 height:20.0 pos:[0,0,0] isSelected:off wirecolor:(color 255 255 255))
		
boneArr = #(	
bone1 = BoneSys.createBone [-10,0,0] [10,0,0] [0,0,1],
bone2 = BoneSys.createBone [10,0,0] [-10,10,0] [0,0,1],
bone3 = BoneSys.createBone [-10,10,0] [-10,10,20] [0,0,1],
bone4 = BoneSys.createBone [-10,10,20] [-10,-10,20] [0,0,1]
)

addmodifier myMesh (skin())
skinmesh = myMesh.modifiers[#Skin] 
skinmesh.bone_limit = 3
	
for b in boneArr do (
	skinOps.addbone skinmesh b -1
)

BakeThat skinmesh myMesh.numverts

addmodifier myMesh2 (skin_wrap())
skinwrap = myMesh2.modifiers[#SkinWrap] 
skinwrap.meshList = #(myMesh)

/**
 * ANIMATE
 */
print "Animate"
slidertime = 100

max tool animmode
move bone1 [0,0,0]
move bone2 [0,0,-20]
move bone3 [0,20,0]
move bone4 [0,0,20]

for i in boneArr do (
-- in coordsys #local rotate i (angleaxis 22.5 [0,0,1])
	in coordsys #local rotate i (angleaxis -5.0 [0,1,0])
)
max tool animmode

	
/**
 * COMPARE FACE AND VERTICES NORMALS
 */
skinwrap.engine = 0 -- Face deformation

print "Checking face deformation normals."
slidertime = 0
check_face_normals myMesh myMesh2
check_vert_normals myMesh myMesh2

slidertime = 100
check_face_normals myMesh myMesh2
check_vert_normals myMesh myMesh2

skinwrap.engine = 1 -- Vertex deformation
skinwrap.falloff = 10.0

print "Checking vertex deformation normals."
slidertime = 0
check_face_normals myMesh myMesh2
check_vert_normals myMesh myMesh2

slidertime = 100
check_face_normals myMesh myMesh2
check_vert_normals myMesh myMesh2