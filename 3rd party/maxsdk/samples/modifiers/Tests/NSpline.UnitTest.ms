--##################################################
-- Modifiers UI NSpline Unit Test
-- Author:	 Peter Watje
-- Created:	April 2, 2014
--##################################################

fileIn "MxsUnit/MxsUnit.ms"

struct NSpline_UnitTest
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

	function nSpline =
	(
		--Defect MAXX-16473 Normalize Spline modifier produces incorrect segment lengths
		Circle radius:20 pos:[-2.89447,-1.19127,0] isSelected:on
		modPanel.addModToSelection (createInstance Normalize_Spl forceCreate:true) ui:on
		$.modifiers[#Normalize_Spl].length = 20
		maxOps.CollapseNodeTo $ 1 on

		numK = numKnots $ 1
		assert_true (numK == 7)


		Rectangle length:10 width:10 cornerRadius:0 pos:[-35,35,0] isSelected:on
		modPanel.addModToSelection (createInstance Normalize_Spl forceCreate:true) ui:on
		modPanel.setCurrentObject $.modifiers[#Normalize_Spl]
		$.modifiers[#Normalize_Spl].length = 1	
		maxOps.CollapseNodeTo $ 1 on
		
		numK = numKnots $ 1
		assert_true (numK == 41)
		
			
		knt1 = getKnotPoint $ 1 1
		knt2 = getKnotPoint $ 1 2
		l = length(knt1-knt2)
		eps = abs(1.0 - l)
		assert_true (eps < 0.001)
		
		Circle radius:20 pos:[-2.89447,-1.19127,0] isSelected:on
		modPanel.addModToSelection (createInstance Normalize_Spl forceCreate:true) ui:on
		$.modifiers[#Normalize_Spl].length = 500
		maxOps.CollapseNodeTo $ 1 on

		numK = numKnots $ 1
		assert_true (numK == 4)
		
			
	),

	Tests =
	#(
		nSpline
	)
)

 --Useful for debugging. Do not remove.
/*
foo = NSpline_UnitTest()
foo.setup()
foo.nSpline()
resetmaxfile(#noprompt)
*/
run_test_fixture NSpline_UnitTest script:(getThisScriptFilename())	