fileIn "MxsUnit/MxsUnit.ms"

struct UnitTest_CATMuscle
(

public
	function setup =
	(
	),

	function teardown =
	(
	),
	
	fn testPropertiesRouting = 
	(
		c = catmuscle()
		c.AddCollisionObject (sphere())
		assert_equal 0 (c.GetCollisionObjectDistortion 1)--> 0.0
		c.SetCollisionObjectDistortion 1 .5 --> OK
		assert_equal .5 (c.GetCollisionObjectDistortion 1) --> 0.5
	),

	Tests = 
	#(
		testPropertiesRouting
	)
)

run_test_fixture UnitTest_CATMuscle script:(getThisScriptFilename())