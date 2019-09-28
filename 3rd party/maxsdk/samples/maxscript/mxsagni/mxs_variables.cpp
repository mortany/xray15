/**********************************************************************
*<
FILE: mxs_variables.cpp

DESCRIPTION: 

CREATED BY: Larry Minton

HISTORY: Created 15 April 2007

*>	Copyright (c) 2007, All Rights Reserved.
**********************************************************************/

#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/maxwrapper/mxsobjects.h>
#include <maxscript/foundation/strings.h>

#include "resource.h"

#ifdef ScripterExport
#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

#include <maxscript\macros\define_instantiation_functions.h>

// ============================================================================

#include <maxscript\macros\define_external_functions.h>
#	include "namedefs.h"

#include <maxscript\macros\define_instantiation_functions.h>
#	include "mxs_variables_wraps.h"

// -----------------------------------------------------------------------------------------

class GatherKeyMapper : public HashTabMapper 
{
public:
	Array* results;
	Value* filterFn;
	GatherKeyMapper (Array* resultsArray, Value* filter) {results=resultsArray; filterFn = filter;}
	void map(const void* key, const void* val) 
	{
		bool keep = true;
		if (filterFn)
		{
			Value* args[2] = {(Value*)key, (Value*)val};
			keep = filterFn->apply_no_alloc_frame(args, 2)->to_bool() ? true : false;
		}
		if (keep)
			results->append((Value*)key);
	}
};

Value*
persistents_gather_cf(Value** arg_list, int count)
{
	// persistents.gather()
	check_arg_count(persistents.gather, 0, count);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	vl.result = new Array (0);
	if (persistents)
	{
		GatherKeyMapper gkm (vl.result, nullptr);
		persistents->map_keys_and_vals(&gkm);
	}
	return_value_tls(vl.result);
}

Value*
persistents_make_cf(Value** arg_list, int count)
{
	// persistents.make <global_name>
	check_arg_count(persistents.make, 1, count);
	Value* glob_name = Name::intern(arg_list[0]->to_string());
	GlobalThunk* t = (GlobalThunk*)globals->get(glob_name);
	if (!t)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_NO_SUCH_GLOBAL), glob_name);
	if (persistents == NULL)
		persistents = new (GC_PERMANENT) HashTable (17, default_eq_fn, default_hash_fn, KEY_IS_OBJECT + VALUE_IS_OBJECT);
	persistents->put_new(t->name, t);
	return &ok;
}

Value*
persistents_ispersistent_cf(Value** arg_list, int count)
{
	// persistents.ispersistent <global_name>
	check_arg_count(persistents.isPersistent, 1, count);
	Value* glob_name = arg_list[0];
	return (persistents && persistents->get(glob_name)) ? &true_value : &false_value;
}

Value*
pesistents_remove_cf(Value** arg_list, int count)
{
	// persistents.remove <persistent_global_name>
	check_arg_count(persistents.remove, 1, count);
	Value* glob_name = arg_list[0];
	if (persistents)
		persistents->remove(glob_name);
	return &ok;
}

Value*
pesistents_removeAll_cf(Value** arg_list, int count)
{
	// persistents.removeAll ()
	check_arg_count(persistents.removeAll, 0, count);
	if (persistents)
		ms_make_collectable(persistents);
	persistents = NULL;
	return &ok;
}

class ShowPersistentsMapper : public HashTabMapper 
{
private:
	CharStream* m_out;
public:
	ShowPersistentsMapper(CharStream* out) : m_out(out) {}
	void map(const void* key, const void* val) 
	{
		m_out->puts(((Value*)key)->to_string());
		m_out->puts(_T(": "));
		((Value*)val)->eval()->sprint(m_out);
	}
};

Value*
pesistents_show_cf(Value** arg_list, int count)
{
	// persistents.show to:<stream>
	check_arg_count_with_keys(persistents.show, 0, count);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	Value* ov = key_arg_or_default(to, _tls->current_stdout);
	if (!is_charstream(ov))
		type_check(ov, CharStream, _T("apropos to:"));
	CharStream* out = (CharStream*)ov;
	if (persistents)
	{
		ShowPersistentsMapper spm (out);
		persistents->map_keys_and_vals(&spm);
	}
	return &ok;
}

Value*
globalVars_set_cf(Value** arg_list, int count)
{
	// globalVars.set <global_name> <value>
	check_arg_count(globalVars.set, 2, count);
	Value* glob_name = Name::intern(arg_list[0]->to_string());
	GlobalThunk* t = (GlobalThunk*)globals->get(glob_name);
	if (!t)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_NO_SUCH_GLOBAL), glob_name);
	Value* args[1] = { arg_list[1] };
	return t->assign_vf(args,1);
}

Value*
globalVars_get_cf(Value** arg_list, int count)
{
	// globalVars.get <global_name>
	check_arg_count(globalVars.get, 1, count);
	Value* glob_name = Name::intern(arg_list[0]->to_string());
	GlobalThunk* t = (GlobalThunk*)globals->get(glob_name);
	if (!t)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_NO_SUCH_GLOBAL), glob_name);
	return t->eval();
}

Value*
globalVars_getTypeTag_cf(Value** arg_list, int count)
{
	// globalVars.getTypeTag <global_name> 
	check_arg_count(globalVars.getTypeTag, 1, count);
	Value* glob_name = Name::intern(arg_list[0]->to_string());
	GlobalThunk* t = (GlobalThunk*)globals->get(glob_name);
	if (!t)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_NO_SUCH_GLOBAL), glob_name);
	if (t->tag > INTERNAL_TAGS)
		return_value (Name::intern(t->tag->name));
	else
		return_value (IntegerPtr::intern((INT_PTR)t->tag));
}

Value*
globalVars_getValueTag_cf(Value** arg_list, int count)
{
	// globalVars.getValueTag <global_name> 
	check_arg_count(globalVars.getValueTag, 1, count);
	Value* glob_name = Name::intern(arg_list[0]->to_string());
	GlobalThunk* t = (GlobalThunk*)globals->get(glob_name);
	if (!t)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_NO_SUCH_GLOBAL), glob_name);
	Value *v = t->eval();
	if (v->tag > INTERNAL_TAGS)
		return_value (Name::intern(v->tag->name));
	else
		return_value (IntegerPtr::intern((INT_PTR)t->tag));
}

Value*
globalVars_gather_cf(Value** arg_list, int count)
{
	// globalVars.gather filter:<fn>
	check_arg_count_with_keys(globalVars.gather, 0, count);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	vl.result = new Array (0);
	Value* fv = key_arg(filter);
	if (fv != &unsupplied)
	{
		if (!is_function(fv)) 
			throw TypeError (_T("globalVars.gather filter:"), fv, &MAXScriptFunction_class);
	}
	else
		fv = nullptr;
	GatherKeyMapper gkm (vl.result, fv);
	globals->map_keys_and_vals(&gkm);
	return_value_tls(vl.result);
}

Value*
globalVars_isglobal_cf(Value** arg_list, int count)
{
	// globalVars.isGlobal <global_name>
	check_arg_count(globalVars.isGlobal, 1, count);
	Value* glob_name = Name::intern(arg_list[0]->to_string());
	GlobalThunk* t = (GlobalThunk*)globals->get(glob_name);
	return (t) ? &true_value : &false_value;
}

Value*
globalVars_remove_cf(Value** arg_list, int count)
{
	// globalVars.remove <global_name>
	check_arg_count(globalVars.remove, 1, count);
	Value* glob_name = Name::intern(arg_list[0]->to_string());
	GlobalThunk* t = (GlobalThunk*)globals->get(glob_name);
	if (t)
	{
		if (persistents)
			persistents->remove(glob_name);
		globals->remove(glob_name);
	}
	return (t) ? &true_value : &false_value;
}
