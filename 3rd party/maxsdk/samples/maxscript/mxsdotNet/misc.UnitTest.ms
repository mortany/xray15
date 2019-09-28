fileIn "MxsUnit/MxsUnit.ms"

struct misc_mxsdotnet_test
(
	fn clr_data_type_alias_test =
	(
		type = dotnetclass "bool[]"; v = dotnet.ValueToDotNetObject #(false,true) type; assert_equal false (v.get 0); assert_equal true (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "byte[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0 (v.get 0); assert_equal 1 (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "sbyte[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0 (v.get 0); assert_equal 1 (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "char[]"; v = dotnet.ValueToDotNetObject #("a", "b") type; assert_equal "a" (v.get 0); assert_equal "b" (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "decimal[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0 (v.get 0); assert_equal 1 (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "double[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0.d00 (v.get 0); assert_equal 1.d00 (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "float[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0. (v.get 0); assert_equal 1. (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "int[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0 (v.get 0); assert_equal 1 (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "uint[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0 (v.get 0); assert_equal 1 (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "long[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0 (v.get 0); assert_equal 1 (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "ulong[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0 (v.get 0); assert_equal 1 (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "object[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0 (v.get 0); assert_equal 1 (v.get 1);assert_equal (dotnet.gettype (dotnetclass "int")) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "short[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0 (v.get 0); assert_equal 1 (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "ushort[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0 (v.get 0); assert_equal 1 (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "string[]"; v = dotnet.ValueToDotNetObject #("aa", "bb") type; assert_equal "aa" (v.get 0); assert_equal "bb" (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
		type = dotnetclass "System.Single[]"; v = dotnet.ValueToDotNetObject #(0,1) type; assert_equal 0. (v.get 0); assert_equal 1. (v.get 1);assert_equal ((dotnet.gettype type).GetElementType()) (dotnet.gettype(v.get 0 asDotNetObject:true))
	)
	,
	
	fn array_array_test = 
	(
		type = dotnetclass "int[][]"
		v = dotnet.ValueToDotNetObject #(#(0,1), #(2,3,4)) type
		assert_equal Array (classof (v.get 0))
		assert_equal Array (classof (v.get 1))
		assert_equal 1 ((v.get 0 asDotNetObject:true).get 1);
		assert_equal 4 ((v.get 1 asDotNetObject:true).get 2);
	)
	,
	
	fn values_as_float_array_test =
	(
		typef = dotnetclass "System.Single[]" --> dotNetClass:System.Single[]
		vf = dotnet.ValueToDotNetObject x_axis typef --> dotNetObject:System.Single[]
		assert_equal 1.0 (vf.get 0) --> 1.0
		assert_equal Float (classof (vf.get 0))
		typed = dotnetclass "System.Double[]" --> dotNetClass:System.Double[]
		vd = dotnet.ValueToDotNetObject x_axis typed --> dotNetObject:System.Double[]
		assert_equal 1.0 (vd.get 0) --> 1.0d0
		assert_equal Double (classof (vd.get 0))
		typefa = dotnetclass "System.Single[][]" --> dotNetClass:System.Single[][]
		vfa = dotnet.ValueToDotNetObject #([1,2], [1,2,3], [1,2,3,4], (quat 0.854433 0.00437304 0.0050751 0.519518), (eulerAngles -117.399 -0.757267 0.126046)) typefa --> dotNetObject:System.Single[][]
		vfa0 = vfa.get 0 --> #(1.0, 2.0)
		vfa0_expected = #(1.0, 2.0)
		assert_equal vfa0_expected.count vfa0.count
		for i = 1 to vfa0_expected.count do assert_float vfa0_expected[i] vfa0[i]
		vfa1 = vfa.get 1 --> #(1.0, 2.0, 3.0)
		vfa1_expected = #(1.0, 2.0, 3.0)
		assert_equal vfa1_expected.count vfa1.count
		for i = 1 to vfa1_expected.count do assert_float vfa1_expected[i] vfa1[i]
		vfa2 = vfa.get 2 --> #(1.0, 2.0, 3.0, 4.0)
		vfa2_expected = #(1.0, 2.0, 3.0, 4.0)
		assert_equal vfa2_expected.count vfa2.count
		for i = 1 to vfa2_expected.count do assert_float vfa2_expected[i] vfa2[i]
		vfa3 = vfa.get 3 --> #(0.854433, 0.00437304, 0.0050751, 0.519518)
		vfa3_expected = #(0.854433, 0.00437304, 0.0050751, 0.519518)
		assert_equal vfa3_expected.count vfa3.count
		for i = 1 to vfa3_expected.count do assert_float vfa3_expected[i] vfa3[i]
		vfa4 = vfa.get 4 --> #(-2.049, -0.0132168, 0.00219992)
		vfa4_expected = #(-2.049, -0.0132168, 0.00219992)
		assert_equal vfa4_expected.count vfa4.count
		for i = 1 to vfa4_expected.count do assert_float vfa4_expected[i] vfa4[i]
			
		typef2a = dotnetclass "System.Single[,]" --> dotNetClass:System.Single[,]
		testTM = Matrix3 1
		RotateX testTM 10.0
		RotateY testTM 20.0
		RotateZ testTM 30.0
		translate testTM [100, 200, 300]
		--> testTM = (matrix3 [0.813798,0.469846,-0.34202] [-0.44097,0.882564,0.163176] [0.378522,0.0180283,0.925417] [100,200,300])

		vf = dotnet.ValueToDotNetObject testTM typef2a --> dotNetObject:System.Single[,]
		rank = vf.rank 
		assert_equal 2 rank
		expected_vals = #(4, 4)
		lengths = for i = 1 to vf.rank collect (vf.getlength (i-1))
		assert_string_equal (expected_vals as string) (lengths as string)
		v = for i1 = 1 to 4 collect for i2 = 1 to 4 collect  
			(vf.getvalue (i1-1) (i2-1))
		expected_vals = #(#(0.813798, -0.44097, 0.378522, 100.0), #(0.469846, 0.882564, 0.0180283, 200.0), #(-0.34202, 0.163176, 0.925417, 300.0), #(0.0, 0.0, 0.0, 1.0))
		assert_string_equal (expected_vals as string) (v as string)
	)
	,
	
	fn index_get_set_test =
	(
		typef = dotnetclass "System.Single[]" --> dotNetClass:System.Single[]
		vf = dotnet.ValueToDotNetObject x_axis typef --> dotNetObject:System.Single[]
		v1_g = vf.get 0 --> 1.0
		v1_i = vf[0] --> 1.0
		assert_equal v1_g v1_i
		vf[0] = 9
		v1_i = vf[0] --> 9.0
		assert_equal 9 v1_i
		exception_thrown = false
		try (vf[0] = "A")
		catch (exception_thrown = true)
		assert_true exception_thrown
		exception_thrown = false
		try (vf["A"] = 1)
		catch (exception_thrown = true)
		assert_true exception_thrown
	)
	,

	fn n_dim_array_test = 
	(
		testTM = Matrix3 1
		RotateX testTM 10.0
		RotateY testTM 20.0
		RotateZ testTM 30.0
		translate testTM [100, 200, 300]
		--> testTM = (matrix3 [0.813798,0.469846,-0.34202] [-0.44097,0.882564,0.163176] [0.378522,0.0180283,0.925417] [100,200,300])
		
		type = dotnetclass "System.Single[,][]" --> dotNetClass:System.Single[,][]
		a = #((matrix3 1), testTM)
		vf = dotnet.ValueToDotNetObject a type --> dotNetObject:System.Single[,][]
		v = vf.get 0
		expected_vals = #(#(1.0, 0.0, 0.0, 0.0), #(0.0, 1.0, 0.0, 0.0), #(0.0, 0.0, 1.0, 0.0), #(0.0, 0.0, 0.0, 1.0))
		assert_string_equal (expected_vals as string) (v as string)
		v = vf.get 1
		expected_vals = #(#(0.813798, -0.44097, 0.378522, 100.0), #(0.469846, 0.882564, 0.0180283, 200.0), #(-0.34202, 0.163176, 0.925417, 300.0), #(0.0, 0.0, 0.0, 1.0))
		assert_string_equal (expected_vals as string) (v as string)

		type = dotnetclass "int[,,]" --> dotNetClass:System.Int32[,,]
		a = #(#(#(1, 2, 3), #(4, 5, 6)), #(#(11, 12, 13), #(14, 15, 16)))
		vf = dotnet.ValueToDotNetObject a type --> dotNetObject:System.Int32[,,]
		rank = vf.rank 
		assert_equal 3 rank
		expected_vals = #(2, 2, 3)
		lengths = for i = 1 to vf.rank collect (vf.getlength (i-1))
		assert_string_equal (expected_vals as string) (lengths as string)
		for i1 = 1 to 2 do for i2 = 1 to 2 do for i3 = 1 to 3 do 
			assert_equal (a[i1][i2][i3]) (vf.getvalue (i1-1) (i2-1) (i3-1))

		type = dotnetclass "int[,,][]" --> dotNetClass:System.Int32[,,][]
		b = #(a)
		vf = dotnet.ValueToDotNetObject b type --> dotNetObject:System.Int32[,,][]
		v = vf[0]
		assert_string_equal (a as string) (v as string)
	)
	,
	
	fn dotnet_array_to_value_test =
	(
		testTM = (matrix3 [0.813798,0.469846,-0.34202] [-0.44097,0.882564,0.163176] [0.378522,0.0180283,0.925417] [100,200,300])
		type = dotnetclass "System.Single[,]" --> dotNetClass:System.Single[,]
		v = dotnet.ValueToDotNetObject testTM type --> dotNetObject:System.Single[,]
		res = dotnet.DotNetArrayToValue v matrix3
		assert_matrix_equal testTM res
			
		type = dotnetclass "System.Single[]" --> dotNetClass:System.Single[]
		
		testP2 = [10,20]
		v = dotnet.ValueToDotNetObject testP2 type --> dotNetObject:System.Single[]
		res = dotnet.DotNetArrayToValue v point2
		assert_point2_equal testP2 res
			
		testP3 = [10,20,30]
		v = dotnet.ValueToDotNetObject testP3 type --> dotNetObject:System.Single[]
		res = dotnet.DotNetArrayToValue v point3
		assert_point3_equal testP3 res
			
		testP4 = [10,20,30,40]
		v = dotnet.ValueToDotNetObject testP4 type --> dotNetObject:System.Single[]
		res = dotnet.DotNetArrayToValue v point4
		assert_point4_equal testP4 res
		
		testQ = quat -0.845253 -0.330536 0.024501 -0.419157
		v = dotnet.ValueToDotNetObject testQ type --> dotNetObject:System.Single[]
		res = dotnet.DotNetArrayToValue v quat
		assert_equal testQ res
		
		testEA = EulerAngles 10 20 30
		v = dotnet.ValueToDotNetObject testEA type --> dotNetObject:System.Single[]
		res = dotnet.DotNetArrayToValue v EulerAngles
		assert_equal testEA res
	)
	,
	-- It must be named "tests"
	tests = 
	#(
		clr_data_type_alias_test,
		array_array_test,
		values_as_float_array_test,
		index_get_set_test,
		n_dim_array_test,
		dotnet_array_to_value_test
	)
)

run_test_fixture misc_mxsdotnet_test script:(getThisScriptFilename())
