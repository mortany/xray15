/*************************************************************** 
@param Author                      : chris.p.johnson@autodesk.com
@param Created                     : July 2010
@param Desc                        : Tests setting and creating a curve control
*********************************************************************** */	

fileIn "MxsUnit/MxsUnit.ms"

rollout rltTestCurveControl "Curve Control"
(
	-- Curve control Properties
	local ccProps = #(
		#visible, 
		#numCurves, 
		#x_range, 
		#y_range, 
		#displayModes, 
		#zoomValues, 
		#scrollValues, 
		#commandMode
	)
 
	-- Curve properties 
	local curveProps = #(
		#name,
		#animatable,
		#color,
		#disabledColor,
		#width,
		#disabledWidth,
		#style,
		#disabledStyle,
		#numPoints,
		#lookupTableSize
	)
  
	-- Curve Point properties
	local cpProps = #(
		#value,
		#inTangent,
		#outTangent,
		#bezier,
		#corner,
		#lock_x,
		#lock_y, 
		#selected,
		#end,
		#noXConstraint
	)
 
	button btnPrintProperties "Print Properties"
	checkBox chk_visible "Visible" checked:true
	checkBox chk_enable "Enable" checked:true 
 
	CurveControl cc_test "Curve Control:" 
		height:200 
		width:400
		align:#center
		numCurves:2
		visible:true
		x_range:[-100,100]
		y_range:[-100,100]
		scrollValues:[-100,100]
		commandMode:#move_xy
		-- The following parameters default to all flags if not specified
		--uiFlags:#(#drawBG, #drawgrid, #upperToolbar, #showReset, #scrollBars, #constrainY, #xvalue)
		rcmFlags:#(#delete)
		asPopup:false
 
	on chk_visible changed state do 
	(
		cc_test.visible = state
	)
	on chk_enable changed state do 
	(
		cc_test.enabled = state
	)
	on rltTestCurveControl open do
	(
		 zoom cc_test #all
		local colors = #(red, green, blue)
		local styles = #(#solid, #dash, #dot, #dashDot, #dashDotDot, #null, #insideFrame)
		local num = cc_test.numCurves 
		  
		-- Initialize curve properties
		for i=2 to num do
		(
			local crv = cc_test.curves[i]
			local ci = ((mod (i-1) colors.count)+1) as integer
			local si = ((mod (i-1) styles.count)+1) as integer
			 
			crv.name = "Curve" + i as string
			crv.color = colors[ci]
			crv.disabledColor = colors[ci]*0.5
			crv.style = styles[si]
			--crv.width = crv.disabledWidth = i
			crv.numPoints = i*2   
			 
			local del = (cc_test.x_range.y - cc_test.x_range.x)/(crv.numPoints-1)
			-- format "del:%\n" del
			-- Place intermediate points equally spaced
			for j=1 to crv.numPoints do
			(
				local cp = crv.points[j]
				format "% end: % -> " j cp.end
				--cp.corner = true
				cp.value = [cc_test.x_range.x+(j-1)*del, cp.value.y]
				cp.inTangent = [0.2,0.2]
				cp.outTangent = [0.2,-0.2]
				crv.points[j] = cp
				format "%\n" crv.points[j].end
				format "value: %\n" crv.points[j].value
			)
		)
	)
 
	function doPrintProperties = 
	(
		local tab = "\t"
		-- print the CC properties
		format "Curve Control Properties\n"
		format (tab + "class: %\n") (classof cc_test)
		format "classof button: %\n" (classof chk_visible)
		for p in ccProps do 
			format (tab + "% : %\n") (p as string) (getProperty cc_test p)
		format (tab + "Curves:\n")
		tab += "\t"
		for i = 1 to cc_test.numCurves do
		(
			local crv = cc_test.curves[i]
			-- print each Curve's properties
			format (tab + "<%>:Curve%\n") (classof crv) i  
			for p in curveProps do 
				format (tab + "\t% : %\n") (p as string) (getProperty crv p)
			  
			format (tab + "\t<%>Points:\n") (classof crv.points)
			for j = 1 to crv.numPoints do
			(
				local cp = crv.points[j]
				format (tab + "\t\t<%>Point%\n") (classof cp) j
				-- Print each curve point properties
				for pp in cpProps do
					format (tab + "\t\t\t% : %\n") (pp as string) (getProperty cp pp)
			)
		)
	)
	
	on btnPrintProperties pressed do
	(
		clearlistener()
		doPrintProperties()
	)
 
	-- Curve control event handlers
	on cc_test selChanged ci val do format "curve % numSelected : %\n" ci val
	on cc_test ptChanged ci val 	do format "curve % ptChanged : %\n" ci val
	on cc_test tangentChanged ci val type do format "curve % tangentChanged : % %\n" ci val (type as string)
	on cc_test deleted ci pi 		do format "curve % deleted : %\n" ci pi
	on cc_test reset ci 				do format "curve % resetted\n" ci
)
 
struct CurveControlTests
(
public
	-- Unit test fixture setup
	function Setup =
	(
		resetmaxFile #noPrompt
	),

	-- Unit test fixture teardown
	function TearDown =
	(
		-- add here fixture teardown code
	),
	
	function testCreateCurveControl =
	(
		curveFloater = newRolloutFloater "Curve Control Test" 500 400
		addRollout rltTestCurveControl curveFloater
		rltTestCurveControl.doPrintProperties()
	),
	
	Tests =
	#(
		testCreateCurveControl
	)
	
)

if undefined == run_test_fixture then
(
	format "-- Unit test fixture not found. Running tests outside of fixture.\n"
	TestCase = CurveControlTests()
	TestCase.setup()
	TestCase.testCreateCurveControl()
	TestCase.Teardown()
)
else
(
	format "-- Fixture found\n"
	run_test_fixture CurveControlTests script:(getThisScriptFilename())
)
