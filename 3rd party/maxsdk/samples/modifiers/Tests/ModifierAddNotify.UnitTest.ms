/*
    @param TestName:     ModifierAddNotify.UnitTest.ms
    @param Author:       me.max.team.core@autodesk.com
    @param Created:      2017-10
    @param LastMod:      2017-10
    @param ModBy:        me.max.team.core@autodesk.com
    @param Description:  A test for exercising the broadcast notification callbacks related to addModifier(), e.g. NOTIFY_POST_MODIFIER_ADDED, for every Modifier class. 
                         A modifier's callback implementation must filter for the correct modifier type before attempting to access modifier data (MAXX-31728).
    @param Status:       Online
*/

fileIn "MxsUnit/MxsUnit.ms"

struct ModifierAddNotify_UnitTest
(
	--##################################################
	-- Unit tests
	--##################################################
public
	function setup =
	(
		resetMaxFile #noPrompt
	),

	function teardown =
	(
		
	),

	function test_addModifiers =
	(
		for modClass in modifier.classes do
		(
			-- format "Testing modifier: %\n" modClass
			local testBox =  box()
			-- Note: Preset_Maker.creatable == false but validModifier testBox Preset_Maker returns true
			if (modClass.creatable and (validModifier testBox modClass)) then
			(
				local testMod = undefined
				assert_true ( testMod == undefined ) message:("testMod should be initialized as undefined: " + modClass as string)
				try
				(
					testMod = createInstance modClass
				)
				catch (
					format "Error creating modifier: %\n" modClass
				)
				assert_true ( testMod != undefined ) message:("Could not create modifier: " + modClass as string)

				assert_true ( testBox.modifiers.count == 0 ) message:("New box object should have zero modifiers")
				local addError = false
				try
				(
					addModifier testBox testMod
				)
				catch
				(
					addError = true
					format "Error adding modifier: %\n" modClass
				)
				assert_true ( testBox.modifiers[1] != undefined ) message:("Could not add modifier: " + modClass as string)
				assert_false ( addError ) message:("Error in addModifier(): " + modClass as string)
				assert_false ( addError and testBox.modifiers[1] != undefined) message:("Post-add error in addModifier(): " + modClass as string)
			)
			else
			(
				-- format "Modifier not creatable or invalid for Box: %\n" modClass
			)
		)
	),

	function test_addModifiersRepeat =
	(
		-- Repeat test_addModifiers to reproduce the intermittent exception reported in MAXX-31728
		local count = 60
		for i = 1 to count do
		(
			format "Repeat % of % ------------------------------------\n" i count
			resetMaxFile #noPrompt
			test_addModifiers()
		)
	),

	Tests =
	#(
		test_addModifiersRepeat
	),
	
	function run =
	(
		for test in Tests do
		(
			setup()
			test()
			teardown()
		)
	)
)

if undefined == run_test_fixture then
(
	/*	Note: For manual local testing with builds that do not contain yet the fix for MAXX-31728,
		recommended to SetQuietMode(true) to avoid mCloth popups "Warning: Object has isolated vertices"
	*/
	format "-- No fixture!\n"
	tester = ModifierAddNotify_UnitTest()
	tester.run()
	format "Done\n"
)
else
(
	format "-- Fixture found!\n"
	run_test_fixture ModifierAddNotify_UnitTest script:(getThisScriptFilename())
)
