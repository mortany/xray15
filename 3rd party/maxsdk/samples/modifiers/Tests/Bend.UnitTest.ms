/*
@param TestName:        Bend
@param Author:          philippe.groarke@autodesk.com
@param Created:         01/21/2019
@param LastMod:         01/21/2019
@param ModBy:           Lord Voldemort
@param Description:     Test bendy bender so it doesn't move in z direction when it shouldn't (bad bad boi)
@param Status:          Online

Unit Test For Bend
*/

/**
 * HELPERS
 */

function check_vert_heights editable_mesh = (
	assert_float 0.0 (getVert editable_mesh 1).z tolerance: 0.00001 \
		message: "Bend is moving object in z axis, which shouldn't happen."
	assert_float 0.0 (getVert editable_mesh 2).z tolerance: 0.00001 \
		message: "Bend is moving object in z axis, which shouldn't happen."
	assert_float 0.0 (getVert editable_mesh 3).z tolerance: 0.00001 \
		message: "Bend is moving object in z axis, which shouldn't happen."
	assert_float 0.0 (getVert editable_mesh 4).z tolerance: 0.00001 \
		message: "Bend is moving object in z axis, which shouldn't happen."
)

/**
 * REGRESSIONS
 */

-- https://jira.autodesk.com/browse/MAXX-48335
function MAXX48335 = (
	bendy_bender_mesh = (Box length:20.0 width:20.0 height:20.0 pos:[0,0,0] isSelected:off wirecolor:(color 255 255 255))
	addmodifier bendy_bender_mesh (bend())

	print "Testing bend for math imprecisions."

	with animate on
	(
		at time 0 bendy_bender_mesh.bend.angle = 0.0
		at time 25 bendy_bender_mesh.bend.angle = 5.0
		at time 50 bendy_bender_mesh.bend.angle = 0.0
	)

	for i = 0 to 10 do (
		slidertime = i
		check_vert_heights bendy_bender_mesh
	)
)

/**
 * EXECUTE TESTS
 */
resetmaxfile(#noprompt)
MAXX48335()
