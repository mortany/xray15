/**********************************************************************
*<
FILE: IntervalArray.cpp

DESCRIPTION:	This plugin shows how to code a maxscript function
				that adds one function to the maxsript language. This
				function is called interval array, and returns an array
				of numbers. The start of the array starts at one number
				and proceeds by evenly spaced intervals to a high number.

3DSMAX USAGE:	In 3DSMax, open up the maxscript listener
				and type in (for example)

				h = intervalarray 12.3 78.9 3

				and it will return the array

				#(12.3, 34.5, 56.7, 78.9)

CREATED BY:		Chris Johnson

HISTORY:		Started in March 2005

*>	Copyright (c) 2005, All Rights Reserved.
**********************************************************************/

#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/arrays.h>

void IntervalArrayInit()
{
	//Todo: Place initialization code here. This gets called when Maxscript goes live
	//during max startup.
}

// Declare C++ function and register it with MAXScript
#include <maxscript\macros\define_instantiation_functions.h>
	def_visible_primitive(IntervalArray, "IntervalArray");

Value* IntervalArray_cf(Value **arg_list, int count)
{
	//--------------------------------------------------------
	//Maxscript usage:
	//--------------------------------------------------------
	// <array> IntervalArray <Start:Number> <End:Number> <steps:Integer>
	check_arg_count(IntervalArray, 3, count);
	Value* pBegin = arg_list[0];
	Value* pEnd = arg_list[1];
	Value* pStep = arg_list[2];
	
	//First example of how to type check an argument
	if ( ! (is_number(pBegin)))
	{
		throw RuntimeError(_T("Expected a Number for the first argument, in function IntervalArray"));
	}
	if ( ! (is_number(pEnd)))
	{
		throw RuntimeError(_T("Expected a Number for the second argument, in function IntervalArray"));
	}
	//Second example of how to type check an argument
	integer_type_check(pStep, _T("Expected an Integer for the step size" ));
	
	float begin = pBegin->to_float();
	float end   = pEnd->to_float();
	int steps   = pStep->to_int();
	
	
	if (steps <= 0) { throw RuntimeError(_T("Expected a positive Number for Steps, in function IntervalArray"));}
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* rArray);
	vl.rArray = new Array (steps);

	float data = begin;
	float increment = ((end-begin)/steps);

	for (int i = 0 ; i <= steps ; i++)
	{
		Value* aTempNumber = Float::heap_intern(data);
		vl.rArray->append(aTempNumber);
		data += increment;
	}
	return_value_tls(vl.rArray);
}



