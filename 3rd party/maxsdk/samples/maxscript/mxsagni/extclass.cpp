/**********************************************************************
 *<
	FILE: ExtClass.cpp

	DESCRIPTION: MXSAgni extension classes

	CREATED BY: Ravi Karra, 1998

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

//#include "pch.h"
#include "buildver.h"
#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/3dmath.h>
#include <maxscript/maxwrapper/mxsobjects.h>
#include <maxscript/foundation/strings.h>
#include <maxscript/maxwrapper/maxclasses.h>
#include <maxscript/util/mxsZipPackage.h>
#include <maxscript/maxwrapper/mxsmaterial.h>
#include <maxscript/foundation/functions.h>
#include <maxscript/foundation/structs.h>
#include <maxscript/mxsplugin/mxsPlugin.h>
#include <macrorec.h>
#include <notify.h>

#include "resource.h"

#include "ExtClass.h"
#include "agnidefs.h"

#include <maxscript\macros\define_external_functions.h>
#	include "namedefs.h"

#include <maxscript\macros\define_instantiation_functions.h>
#	include <maxscript\protocols\bigmatrix.inl>
#	include <maxscript\protocols\box.inl>
#	include <maxscript\protocols\notekey.inl>
#	include <maxscript\protocols\xrefs.inl>

#ifdef ScripterExport
	#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )
/* ------------------ BigMatrixValue indexing functions -------------------- */

Value*
BigMatrixValue::get_vf(Value** arg_list, int count)
{
	Value *arg, *result;
	int	  index;
	check_arg_count(get, 2, count + 1);
	arg = arg_list[0];
	if (!is_number(arg) || (index = arg->to_int()) < 1)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ARRAY_INDEX_MUST_BE_VE_NUMBER_GOT), arg);

	if (index > bm.m)
		result = &undefined;
	else
	{
		rowArray->row = index - 1;
		result = rowArray;
	}

	return result;
}

Value*
BigMatrixValue::put_vf(Value** arg_list, int count)
{
	throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_RK_CANNOT_SET_BIGMATRIX_PROPERTY), arg_list[1]->to_string());
	return arg_list[0];
}

visible_class_instance (BigMatrixRowArray, "BigMatrixRowArray")

BigMatrixRowArray::BigMatrixRowArray(BigMatrixValue* bmv) 
{
	tag = class_tag(BigMatrixRowArray);
	this->bmv = bmv;
	row = 0; // LAM - 5/14/01
}

void
BigMatrixRowArray::sprin1(CharStream* s)
{
	s->puts(_T("#("));
	int maxItemsC = bmv->bm.n;

	BOOL print_20_elements = !GetPrintAllElements();

	if (print_20_elements)
		maxItemsC = __min(maxItemsC,20);

	for (int i = 0; i < maxItemsC; i++)
	{
		s->printf(_T("%.2f"), bmv->bm[row][i]);
		if (i < maxItemsC - 1)
			s->puts(_T(","));
	}
	if (print_20_elements && bmv->bm.n > 20)
		s->puts(_T(", ..."));
	s->puts(_T(")"));	
}

Value*
BigMatrixRowArray::get_vf(Value** arg_list, int count)
{
	Value *arg, *result;
	int	  index;
	check_arg_count(get, 2, count + 1);
	arg = arg_list[0];
	if (!is_number(arg) || (index = arg->to_int()) < 1)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ARRAY_INDEX_MUST_BE_VE_NUMBER_GOT), arg);

	if (index > bmv->bm.n)
		result = &undefined;
	else
		result = Float::intern(bmv->bm[row][index - 1]);  // array indexes are 1-based !!!
	return result;
}

Value*
BigMatrixRowArray::put_vf(Value** arg_list, int count)
{
	Value *arg;
	int	  index;

	check_arg_count(put, 3, count + 1);
	arg = arg_list[0];
	if (!is_number(arg) || (index = arg->to_int()) < 1)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ARRAY_INDEX_MUST_BE_VE_NUMBER_GOT), arg);
	if (index > bmv->bm.n)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_RK_ARRAY_INDEX_MUST_BE_LESS_THAN_NUMBER_OF_COLUMNS_GOT), arg);	
	
	bmv->bm[row][index - 1] = arg_list[1]->to_float();
	return arg_list[1];
}

void
BigMatrixRowArray::gc_trace()
{
	/* trace sub-objects & mark me */
	Value::gc_trace();
	if (bmv != NULL && bmv->is_not_marked())
		bmv->gc_trace();
}

/* ------------------- BigMatrixValue class instance -------------- */

visible_class_instance (BigMatrixValue, "BigMatrix")

Value*
BigMatrixValueClass::apply(Value** arg_list, int count, CallContext* cc)
{
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	two_value_locals_tls(a0, a1);
	Value* result = NULL;	
	if (count == 2)
	{
		vl.a0 = arg_list[0]->eval();
		vl.a1 = arg_list[1]->eval();
		if (!is_number(vl.a0) || vl.a0->to_int() < 1)
			throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ARRAY_INDEX_MUST_BE_VE_NUMBER_GOT), vl.a0);
		if (!is_number(vl.a1) || vl.a1->to_int() < 1)
			throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ARRAY_INDEX_MUST_BE_VE_NUMBER_GOT), vl.a1);
		result = new BigMatrixValue (vl.a0->to_int(), vl.a1->to_int());
	}
	else
		check_arg_count(BigMatrix, 2, count);
	return result;
}

/* -------------------- BigMatrixValue methods ----------------------- */
BigMatrixValue::BigMatrixValue(const BigMatrix& from)
{
	tag = class_tag(BigMatrixValue);
	rowArray = NULL; // LAM - 6/07/02 - protect against gc while uninitialized
	bm = from;
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_value_local_tls(_this);
	vl._this = this;
	rowArray = new BigMatrixRowArray(this);
}

BigMatrixValue::BigMatrixValue(int mm, int nn)
{
	tag = class_tag(BigMatrixValue);
	rowArray = NULL; // LAM - 6/07/02 - protect against gc while uninitialized
	int res = bm.SetSize(mm, nn);
	if (res == -1)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_BIGMATRIX_BAD_SIZE));

	for (int m=0; m < mm; m++)
		for (int n=0; n < nn; n++)
			bm[m][n] = 0.0f;

	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_value_local_tls(_this);
	vl._this = this;
	rowArray = new BigMatrixRowArray(this);
}

void
BigMatrixValue::gc_trace()
{
	/* trace sub-objects & mark me */
	Value::gc_trace();
	if (rowArray != NULL && rowArray->is_not_marked())
		rowArray->gc_trace();
}

void
BigMatrixValue::sprin1(CharStream* s)
{
	s->puts(_T("#BigMatrix(\n"));
	int maxItemsR = bm.m;
	int maxItemsC = bm.n;

	BOOL print_20_elements = !GetPrintAllElements();

	if (print_20_elements)
	{
		maxItemsR = __min(maxItemsR,20);
		maxItemsC = __min(maxItemsC,20);
	}
	for (int m = 0; m < maxItemsR; m++)
	{
		s->puts(_T("\t["));
		for (int n = 0; n < maxItemsC; n++)
		{
			s->printf(_T("%.2f"), bm[m][n]);
			if (n < bm.n-1) 
				s->puts(_T(","));
		}
		if (print_20_elements && bm.n > 20)
			s->puts(_T(", ..."));
		s->puts(_T("]"));
		if (m < bm.m-1) 
			s->puts(_T(","));
		s->puts(_T("\n"));
	}
	if (print_20_elements && bm.m > 20)
		s->puts(_T("\t..."));
	s->puts(_T(")\n"));
}

Value*
BigMatrixValue::get_property(Value** arg_list, int count)
{
	Value* prop = arg_list[0];
	if (prop == n_rows)
		return_value (Integer::intern(bm.m));
	else if (prop == n_columns)
		return_value (Integer::intern(bm.n));
	return Value::get_property(arg_list, count);
}

Value*
BigMatrixValue::set_property(Value** arg_list, int count)
{
	Value* prop = arg_list[1];	
	if (prop == n_rows)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_RK_CANNOT_SET_BIGMATRIX_PROPERTY), arg_list[1]->to_string());
	else if (prop == n_columns)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_RK_CANNOT_SET_BIGMATRIX_PROPERTY), arg_list[1]->to_string());
	return Value::set_property(arg_list, count);
}

Value*
BigMatrixValue::plus_vf(Value** arg_list, int count)
{
	if (!is_bigmatrix(arg_list[0]))
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_RK_RIGHT_HAND_OPERATOR_SHOULD_BE_A_BIGMATRIX_GOT), arg_list[0]);
	BigMatrix& bmat = ((BigMatrixValue*)arg_list[0])->to_bigmatrix();
	if (bmat.m != bm.m || bmat.n != bm.n)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_RK_RIGHT_HAND_OPERATOR_SHOULD_BE_EQUAL_SIZE));
	
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_value_local_tls(result);
	vl.result = new BigMatrixValue(bm.m, bm.n);
		for (int m = 0; m < bm.m; m++)
			for (int n = 0; n < bm.n; n++)
				((BigMatrixValue*)vl.result)->bm[m][n] = bm[m][n] + bmat[m][n];
	return_value_tls(vl.result);
}

Value*														
BigMatrixValue::transpose_vf(Value** arg_list, int count)			
{															
	check_arg_count(transpose, 1, count + 1);
	BigMatrix t;
	bm.SetTranspose(t);
	return new BigMatrixValue(t);
}

Value*														
BigMatrixValue::setSize_vf(Value** arg_list, int count)
{
	check_arg_count(setSize, 3, count + 1);
	int mm = arg_list[0]->to_int();
	int nn = arg_list[1]->to_int();
	if (mm < 1)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ARRAY_INDEX_MUST_BE_VE_NUMBER_GOT), arg_list[1]);
	if (nn < 1)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ARRAY_INDEX_MUST_BE_VE_NUMBER_GOT), arg_list[2]);
	int res = bm.SetSize(mm, nn);
	if (res == -1)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_BIGMATRIX_BAD_SIZE));

	for (int m=0; m < mm; m++)
		for (int n=0; n < nn; n++)
			bm[m][n] = 0.0f;
	rowArray->row = 0;
	return this;
}

Value*														
BigMatrixValue::clear_vf(Value** arg_list, int count)
{
	check_arg_count(clear, 1, count + 1);
	bm.Clear();
	rowArray->row = 0;
	return this;
}

#define Matrix3Value BigMatrixValue
//#	include "defimpl.h"	
	def_mut_mat_fn( invert,			1, bm.Invert())
	def_mut_mat_fn( identity,		1, bm.Identity())
//	def_mut_mat_fn( clear,			1, bm.Clear())
//	def_mut_mat_fn( setSize,		3, bm.SetSize(arg_list[0]->to_int(), arg_list[1]->to_int()))
#undef Matrix3Value


/* ---------------------- MAXNoteKeyArray class ----------------------- */

visible_class_instance (MAXNoteKeyArray, "MAXNoteKeyArray")

MAXNoteKeyArray::MAXNoteKeyArray(DefNoteTrack* nt)
{
	tag = class_tag(MAXNoteKeyArray);
	note_track = NULL;
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_value_local_tls(_this);
	vl._this = this;
	note_track = (MAXNoteTrack*)MAXNoteTrack::intern(nt);
}
	
MAXNoteKeyArray::MAXNoteKeyArray(MAXNoteTrack* nt)
{
	tag = class_tag(MAXNoteKeyArray);
	note_track = nt;
}

void
MAXNoteKeyArray::gc_trace()
{
	Value::gc_trace();
	if (note_track && note_track->is_not_marked())
		note_track->gc_trace();
}

void
MAXNoteKeyArray::sprin1(CharStream* s)
{
	if (deletion_check_test(note_track))
	{
		s->printf(_T("<Key array for deleted note track>"));
		return;
	}
		
	int		num_keys = note_track->note_track->NumKeys();
	int		i;
	s->puts(_T("#keys("));
	for (i = 0; i < num_keys; i++)
	{
		float frame = note_track->note_track->GetKeyTime(i) / (float)GetTicksPerFrame();
		s->printf(_T("%gf"), frame);
		if (i < num_keys - 1)
			s->puts(_T(", "));
	}
	s->puts(_T(")"));	
}

Value*
MAXNoteKeyArray::get_vf(Value** arg_list, int count)
{
	deletion_check(note_track);
	Value *arg, *result;
	int	  index;

	check_arg_count(get, 2, count + 1);
	arg = arg_list[0];
	if (!is_number(arg) || (index = arg->to_int()) < 1)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ARRAY_INDEX_MUST_BE_VE_NUMBER_GOT), arg);

	if (index > note_track->note_track->NumKeys())
		result = &undefined;
	else
		result = new MAXNoteKey(note_track, index - 1);  // array indexes are 1-based !!!

	return result;
}

Value*
MAXNoteKeyArray::put_vf(Value** arg_list, int count)
{
	deletion_check(note_track);
	throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ASSIGNING_TO_KEY_ARRAY_DIRECTLY_NOT_YET_SUPPORTED));
	return arg_list[1];
}

Value*
MAXNoteKeyArray::append_vf(Value** arg_list, int count)
{
	deletion_check(note_track);
	throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_APPENDING_KEYS_DIRECTLY_NOT_YET_SUPPORTED));
	return this;
}

Value*
MAXNoteKeyArray::deleteItem_vf(Value** arg_list, int count)
{
	//	deleteItem <key_array> index
	deletion_check(note_track);
	check_arg_count(deleteKey, 2, count + 1);
	int index = arg_list[0]->to_int() - 1;
	if (index < 0 || index >= note_track->note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_INDEX_OUT_OF_RANGE_IN_DELETEITEM), arg_list[0]);

	note_track->note_track->DeleteKeyByIndex(index);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	return &ok;
}

Value*
MAXNoteKeyArray::findItem_vf(Value** arg_list, int count)
{
	// findItem <array> <item>   -- returns index of 1st matching item or 0 if not found
	deletion_check(note_track);
	check_arg_count(findItem, 2, count + 1);
	MAXNoteKey* item = (MAXNoteKey*)arg_list[0];
	type_check(item, MAXNoteKey, _T("findItem <MAXNoteKeyArray>"));
	
	if (item->note_track->note_track == note_track->note_track)
		return_value (Integer::intern(item->key_index + 1));
	else
		return_value (Integer::intern(0));
}

Value*
MAXNoteKeyArray::getNoteKeyTime_vf(Value** arg_list, int count)
{
	//	getNoteKeyTime <key_array> <index>
	deletion_check(note_track);
	check_arg_count(getNoteKeyTime, 2, count+1);
	int index = arg_list[0]->to_int() - 1;
	if (index < 0 || index >= note_track->note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_INDEX_OUT_OF_RANGE_IN_GETNOTEKEYTIME), arg_list[0]);	
	return_value (MSTime::intern(note_track->note_track->GetKeyTime(index)));
}

Value*
MAXNoteKeyArray::getNoteKeyIndex_vf(Value** arg_list, int count)
{
	//	getNoteKeyIndex <key_array> <index>
	deletion_check(note_track);
	check_arg_count(getNoteKeyIndex, 2, count+1);
	TimeValue t = arg_list[0]->to_timevalue();
	for (int i=0; i < note_track->note_track->NumKeys(); i++)
		if (t==note_track->note_track->GetKeyTime(i)) return_value (Integer::intern(i+1));
	return &undefined;	
}

Value*
MAXNoteKeyArray::sortNoteKeys_vf(Value** arg_list, int count)
{
	//	sortNoteKeys <key_array>
	deletion_check(note_track);
	check_arg_count(sortKeys, 1, count + 1);
	note_track->note_track->keys.KeysChanged();
	note_track->note_track->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	return &ok;
}

Value*
MAXNoteKeyArray::addNewNoteKey_vf(Value** arg_list, int count)
{
	//	addNewNoteKey <key_array> <time> [#select]
	deletion_check(note_track);
	TimeValue t;
	Value*   arg0 = arg_list[0];
	int      i = 0;
	if (count < 1)
		throw ArgCountError (_T("addNewNoteKey "), 2, count + 1);
	else if (is_number(arg0) || is_time(arg0))
	{
		t = arg0->to_timevalue();
		i = 1;
	}
	else
		throw TypeError (MaxSDK::GetResourceStringAsMSTR(IDS_INVALID_TIME_VALUE_FOR_ADDNEWNOTEKEY), arg0);

	DWORD flags = 0;
	while (i < count)				// pick up any remaining flags
	{
		if (arg_list[i] == n_select)
			flags |= ADDKEY_SELECT;
		else 
			throw TypeError (MaxSDK::GetResourceStringAsMSTR(IDS_UNRECOGNIZED_FLAG_FOR_ADDNEWNOTEKEY), arg_list[i]);
		i++;
	}
	note_track->note_track->AddNewKey(t, flags);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	int n = note_track->note_track->keys.Count();
	for ( int k = 0; k < n; k++ ) {
		if (note_track->note_track->keys[k]->time == t) {
			return_value( new MAXNoteKey(note_track, k) );
			}
		}
	return &undefined;
}

Value*
MAXNoteKeyArray::deleteNoteKeys_vf(Value** arg_list, int count)
{
	//	deleteNoteKeys <key_array> [#allKeys] [#selection] [#slide] [#rightToLeft]
	deletion_check(note_track);
	Interval iv;
	Value*   arg0 = arg_list[0];
	int      i = 0;

	DWORD flags = TRACK_DOALL;
	while (i < count)				// pick up any remaining flags
	{
		if (arg_list[i] == n_allKeys)
			flags |= TRACK_DOALL;
		else if (arg_list[i] == n_selection)
		{
			flags &= ~TRACK_DOALL;
			flags |= TRACK_DOSEL;
		}
		else if (arg_list[i] == n_slide)
			flags |= TRACK_SLIDEUNSEL;
		else if (arg_list[i] == n_rightToLeft)
			flags |= TRACK_RIGHTTOLEFT;
		else 
			throw TypeError (MaxSDK::GetResourceStringAsMSTR(IDS_UNRECOGNIZED_FLAG_FOR_DELETENOTEKEYS), arg_list[i]);
		i++;
	}
	note_track->note_track->DeleteKeys(flags);
	note_track->note_track->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	return &ok;
}

Value*
MAXNoteKeyArray::deleteNoteKey_vf(Value** arg_list, int count)
{
	//	deleteNoteKey <key_array> index
	deletion_check(note_track);
	check_arg_count(deleteNoteKey, 2, count + 1);
	int index = arg_list[0]->to_int() - 1;
	if (index < 0 || index >= note_track->note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_INDEX_OUT_OF_RANGE_IN_DELETENOTEKEY), arg_list[0]);

	note_track->note_track->keys.Delete(index,1);
	note_track->note_track->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	return &ok;
}

Value*
MAXNoteKeyArray::map(node_map& m)
{
	deletion_check(note_track);
	int		num_keys = note_track->note_track->NumKeys();
	int		i;
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	three_value_locals_tls(key, result, arg_save);

	for (i = 0; i < num_keys; i++)
	{
		ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext(false, _tls);
		try
		{
			ScopedSaveCurrentFrames scopedSaveCurrentFrames(_tls);
			vl.key = new MAXNoteKey(note_track, i);

			if (m.vfn_ptr != NULL)
				vl.result = (vl.key->*(m.vfn_ptr))(m.arg_list, m.count);
			else
			{
				// temporarily replace 1st arg with this key
				vl.arg_save = m.arg_list[0];

				m.arg_list[0] = vl.key;
				if (m.flags & NM_MXS_FN)
					vl.result = ((MAXScriptFunction*)m.cfn_ptr)->apply(m.arg_list, m.count);
				else
					vl.result = (*m.cfn_ptr)(m.arg_list, m.count);
				m.arg_list[0] = vl.arg_save;
			}
			if (vl.result == &loopExit)
				break;
			if (m.collection != NULL && vl.result != &dontCollect)
				m.collection->append(vl.result);
		}
		catch (LoopContinue)
		{
		}
		vl.result = vl.key = NULL;
		_tls->current_result = NULL;
		check_interrupts();
	}

	return &ok;
}

Value* MAXNoteKeyArray::join_vf(Value** arg_list, int count) { throw NoMethodError (_T("join"), this); return &undefined; }
Value* MAXNoteKeyArray::sort_vf(Value** arg_list, int count) { throw NoMethodError (_T("sort"), this); return &undefined; }

/* ---------------- MAXNoteKeyArray property access ------------------------- */

Value*
MAXNoteKeyArray::get_count(Value** arg_list, int count)
{
	deletion_check(note_track);
	return_value (Integer::intern(note_track->note_track->NumKeys()));
}

Value*
MAXNoteKeyArray::set_count(Value** arg_list, int count)
{
	deletion_check(note_track);
	throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_CANNOT_SET_KEY_ARRAY_SIZE_DIRECTLY));
	return &undefined;
}

/* --------------------- MAXNoteKey methods -------------------------------- */

visible_class_instance (MAXNoteKey, "MAXNoteKey")

MAXNoteKey::MAXNoteKey(DefNoteTrack* nt, int ikey)
{
	tag = class_tag(MAXNoteKey);
	note_track = NULL;
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_value_local_tls(_this);
	vl._this = this;
	note_track = (MAXNoteTrack*)MAXNoteTrack::intern(nt);
	key_index = ikey;
}

MAXNoteKey::MAXNoteKey(MAXNoteTrack* nt, int ikey)
{
	tag = class_tag(MAXNoteKey);
	note_track = nt;
	key_index = ikey;
}

void
MAXNoteKey::gc_trace()
{
	Value::gc_trace();
	if (note_track && note_track->is_not_marked())
		note_track->gc_trace();
}

void
MAXNoteKey::sprin1(CharStream* s)
{
	if (deletion_check_test(note_track))
	{
		s->printf(_T("<Key for deleted note_track>"));
		return;
	}
	if (key_index >= note_track->note_track->NumKeys())
		s->printf(_T("#Note <deleted key>"));
	else
		s->printf(_T("#Note key(%d @ %gf)"), key_index + 1, note_track->note_track->GetKeyTime(key_index) / (float)GetTicksPerFrame());
}


Value*
MAXNoteKey::delete_vf(Value** arg_list, int count)
{
	//	delete <key>
	deletion_check(note_track);
	check_arg_count(delete, 1, count + 1);
	if (key_index >= note_track->note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_NO_LONGER_EXISTS_IN_CONTROLLER), note_track);

	note_track->note_track->DeleteKeyByIndex(key_index);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	return &ok;
}

Value*
MAXNoteKey::get_time(Value** arg_list, int count)
{
	deletion_check(note_track);
	if (key_index >= note_track->note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_NO_LONGER_EXISTS_IN_CONTROLLER), note_track);
	return_value (MSTime::intern(note_track->note_track->GetKeyTime(key_index)));
}

Value*
MAXNoteKey::set_time(Value** arg_list, int count)
{
	deletion_check(note_track);
	TimeValue t = arg_list[0]->to_timevalue();
	if (key_index >= note_track->note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_NO_LONGER_EXISTS_IN_CONTROLLER), note_track);

	note_track->note_track->keys[key_index]->time = t;
	note_track->note_track->keys.KeysChanged();
	note_track->note_track->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	return arg_list[0];
}

Value*
MAXNoteKey::get_selected(Value** arg_list, int count)
{
	deletion_check(note_track);
	if (key_index >= note_track->note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_NO_LONGER_EXISTS_IN_CONTROLLER), note_track);
	return note_track->note_track->keys[key_index]->TestFlag(NOTEKEY_SELECTED) ? &true_value : &false_value;
}

Value*
MAXNoteKey::set_selected(Value** arg_list, int count)
{
	deletion_check(note_track);
	if (!is_integer_number(arg_list[0]))
		type_check(arg_list[0], Boolean, _T("<key>.selected"));
	if (key_index >= note_track->note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_NO_LONGER_EXISTS_IN_CONTROLLER), note_track);
	note_track->note_track->SelectKeyByIndex(key_index, arg_list[0]->to_bool());
	return arg_list[0];
}

Value*
MAXNoteKey::get_value(Value** arg_list, int count)
{
	deletion_check(note_track);
	if (key_index >= note_track->note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_NO_LONGER_EXISTS_IN_CONTROLLER), note_track);
	return new String(note_track->note_track->keys[key_index]->note);
}

Value*
MAXNoteKey::set_value(Value** arg_list, int count)
{
	deletion_check(note_track);
	type_check(arg_list[0], String, _T("<key>.value"));
	if (key_index >= note_track->note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_NO_LONGER_EXISTS_IN_CONTROLLER), note_track);
	note_track->note_track->keys[key_index]->note = TSTR(arg_list[0]->to_string());	
	note_track->note_track->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	return arg_list[0];
}

/*-------------------- NoteTrack functions -------------------------*/
Animatable* setup_anim_access(Value* val)
{
	if (!val->is_kind_of(class_tag(MAXWrapper)))
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_NO_SUCH_NOTETRACK_FUNCTION_FOR), val);
	Animatable* res = is_node(val) ? val->to_node() : (Animatable*)((MAXWrapper*)val)->get_max_object();
	return res;
}			

Value*
add_note_track_cf(Value** arg_list, int count)
{
	check_arg_count(addNoteTrack, 2, count);
	Animatable* anim = setup_anim_access(arg_list[0]);
	if (!anim) return &false_value;
	Value* note = arg_list[1];
	if (!is_notetrack(note)) 
		throw TypeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_FUNCTION_NEEDS_NOTETRACK_GOT), note);	
	anim->AddNoteTrack(((MAXNoteTrack*)note)->to_notetrack());
	return &true_value;
}

Value*
delete_note_track_cf(Value** arg_list, int count)
{
	check_arg_count(deleteNoteTrack, 2, count);
	Animatable* anim = setup_anim_access(arg_list[0]);
	if (!anim)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_NO_ANIMATABLE_ASSIGNED_TO), arg_list[0]);
	Value* note = arg_list[1];	
	if (!is_notetrack(note)) 
		throw TypeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_FUNCTION_NEEDS_NOTETRACK_GOT), note);	
	anim->DeleteNoteTrack(((MAXNoteTrack*)note)->to_notetrack());
	return &ok;
}

Value*
has_note_tracks_cf(Value** arg_list, int count)
{
	check_arg_count(hasNoteTracks, 1, count);
	Animatable* anim = setup_anim_access(arg_list[0]);
	return (anim && anim->HasNoteTracks()) ? &true_value : &false_value;
}

Value*
num_note_tracks_cf(Value** arg_list, int count)
{
	check_arg_count(numNoteTracks, 1, count);
	Animatable* anim = setup_anim_access(arg_list[0]);
	return_value (Integer::intern(anim ? anim->NumNoteTracks() : 0));
}

Value*
get_note_track_cf(Value** arg_list, int count)
{
	check_arg_count(getNoteTrack, 2, count);
	Animatable* anim = setup_anim_access(arg_list[0]);
	if (!anim)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_NO_ANIMATABLE_ASSIGNED_TO), arg_list[0]);
	int index = arg_list[1]->to_int();	
	if (index < 1)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ARRAY_INDEX_MUST_BE_VE_NUMBER_GOT), arg_list[1]);
	else if(index > anim->NumNoteTracks())
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INDEX_SHOULD_BE_LE_NUM_NOTETRACKS_GOT), arg_list[1]);
	return_value (MAXNoteTrack::intern((DefNoteTrack *)anim->GetNoteTrack(index-1)));
}

/* ------------------- MAXNoteTrack class instance -------------- */

visible_class_instance (MAXNoteTrack, "NoteTrack")

Value*
MAXNoteTrackClass::apply(Value** arg_list, int count, CallContext* cc)
{
	check_arg_count(NoteTrack, 1, count);
	return new MAXNoteTrack(arg_list[0]->eval()->to_string());
}

/* -------------------- MAXNoteTrack methods -------------------- */

MAXNoteTrack::MAXNoteTrack(const TCHAR* iname)
{
	tag = class_tag(MAXNoteTrack);
	note_track = (DefNoteTrack*)NewDefaultNoteTrack();
	note_track->trackName = iname;
	make_ref(0, note_track);	
}

MAXNoteTrack::MAXNoteTrack(DefNoteTrack* dnt)
{
	tag = class_tag(MAXNoteTrack);
	note_track = dnt;
	make_ref(0, note_track);
}

Value*
MAXNoteTrack::intern(DefNoteTrack* dnt)
{
	if (dnt == NULL) return &undefined;

	MAXNoteTrack* mxc;
	MAXNoteTrack** mxcp;
	if (maxwrapper_cache_get(MAXNoteTrack, dnt, mxc, mxcp) &&
	    mxc->note_track == dnt)
		return mxc;
	else
	{
		FindMAXWrapperEnum cb(dnt,class_tag(MAXNoteTrack));
		dnt->DoEnumDependents(&cb);
		if (cb.result) 
		{	*mxcp = (MAXNoteTrack*)cb.result;
			return cb.result;
		}
		return (*mxcp = new MAXNoteTrack (dnt));
	}
}

void
MAXNoteTrack::sprin1(CharStream* s)
{
	check_for_deletion();
	s->printf(_T("Notetrack:%s"), note_track->trackName);
}

void
MAXNoteTrack::gc_trace()
{
	Value::gc_trace();
}

Value*
MAXNoteTrack::copy_vf(Value** arg_list, int count)
{
	check_for_deletion();
	check_arg_count_with_keys(copy, 1, count + 1);

	// clone object, default works on Ref(0)
	ReferenceTarget* ref = get_max_object();
	if (ref != NULL)
	{
		ReferenceTarget* nref = CloneRefHierarchy(ref);
		return intern((DefNoteTrack*)nref);
	}
	else 
		return &undefined;
}

Value*
MAXNoteTrack::get_property(Value** arg_list, int count)
{
	check_for_deletion();
	Value* prop = arg_list[0];
	if (prop == n_name)
	{
		return new String(note_track->trackName);
	}
	return Value::get_property(arg_list, count);
}

Value*
MAXNoteTrack::set_property(Value** arg_list, int count)
{
	check_for_deletion();
	Value* prop = arg_list[1];	
	if (prop == n_name) 
	{
		note_track->trackName = arg_list[0]->to_string();
		return arg_list[0];
	}
	return Value::set_property(arg_list, count);
}

Value*
MAXNoteTrack::get_keys(Value** arg_list, int count)
{
	check_for_deletion();
	return new MAXNoteKeyArray(note_track);
}

Value*
MAXNoteTrack::set_keys(Value** arg_list, int count)
{
	throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_CANNOT_DIRECTLY_SET_KEY_ARRAY));
	return &undefined;
}

Value*
MAXNoteTrack::getNoteKeyTime_vf(Value** arg_list, int count)
{
	//	getNoteKeyTime <notetrack> <index>
	check_for_deletion();
	check_arg_count(getNoteKeyTime, 2, count+1);
	int index = arg_list[0]->to_int() - 1;
	if (index < 0 || index >= note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_INDEX_OUT_OF_RANGE_IN_GETNOTEKEYTIME), arg_list[0]);	
	return_value (MSTime::intern(note_track->GetKeyTime(index)));
}

Value*
MAXNoteTrack::getNoteKeyIndex_vf(Value** arg_list, int count)
{
	check_for_deletion();
	check_arg_count(getNoteKeyIndex, 2, count+1);
	TimeValue t = arg_list[0]->to_timevalue();
	for (int i=0; i < note_track->NumKeys(); i++)
		if (t==note_track->GetKeyTime(i)) return_value (Integer::intern(i+1));
	return &undefined;	
}

Value*
MAXNoteTrack::sortNoteKeys_vf(Value** arg_list, int count)
{
	//	sortNoteKeys <notetrack>
	check_for_deletion();
	check_arg_count(sortKeys, 1, count + 1);
	note_track->keys.KeysChanged();
	note_track->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	return &ok;
}

Value*
MAXNoteTrack::addNewNoteKey_vf(Value** arg_list, int count)
{
	//	addNewNoteKey <notetrack> <time> [#select]
	check_for_deletion();
	TimeValue t;
	Value*   arg0 = arg_list[0];
	int      i = 0;
	if (count < 1)
		throw ArgCountError (_T("addNewNoteKey "), 2, count + 1);
	else if (is_number(arg0) || is_time(arg0))
	{
		t = arg0->to_timevalue();
		i = 1;
	}
	else
		throw TypeError (MaxSDK::GetResourceStringAsMSTR(IDS_INVALID_TIME_VALUE_FOR_ADDNEWNOTEKEY), arg0);

	DWORD flags = 0;
	while (i < count)				// pick up any remaining flags
	{
		if (arg_list[i] == n_select)
			flags |= ADDKEY_SELECT;
		else 
			throw TypeError (MaxSDK::GetResourceStringAsMSTR(IDS_UNRECOGNIZED_FLAG_FOR_ADDNEWNOTEKEY), arg_list[i]);
		i++;
	}
	note_track->AddNewKey(t, flags);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	int n = note_track->keys.Count();
	for ( int k = 0; k < n; k++ ) {
		if (note_track->keys[k]->time == t) {
			return_value( new MAXNoteKey(note_track, k) );
			}
		}
	return &undefined;
}

Value*
MAXNoteTrack::deleteNoteKeys_vf(Value** arg_list, int count)
{
	//	deleteNoteKeys <notetrack> [#allKeys] [#selection] [#slide] [#rightToLeft]
	check_for_deletion();
	Interval iv;
	Value*   arg0 = arg_list[0];
	int      i = 0;

	DWORD flags = TRACK_DOALL;
	while (i < count)				// pick up any remaining flags
	{
		if (arg_list[i] == n_allKeys)
			flags |= TRACK_DOALL;
		else if (arg_list[i] == n_selection)
		{
			flags &= ~TRACK_DOALL;
			flags |= TRACK_DOSEL;
		}
		else if (arg_list[i] == n_slide)
			flags |= TRACK_SLIDEUNSEL;
		else if (arg_list[i] == n_rightToLeft)
			flags |= TRACK_RIGHTTOLEFT;
		else 
			throw TypeError (MaxSDK::GetResourceStringAsMSTR(IDS_UNRECOGNIZED_FLAG_FOR_DELETENOTEKEYS), arg_list[i]);
		i++;
	}
	note_track->DeleteKeys(flags);
	note_track->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	return &ok;
}

Value*
MAXNoteTrack::deleteNoteKey_vf(Value** arg_list, int count)
{
	//	deleteNoteKey <notetrack> index
	check_for_deletion();
	check_arg_count(deleteNoteKey, 2, count + 1);
	int index = arg_list[0]->to_int() - 1;
	if (index < 0 || index >= note_track->NumKeys())
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_KEY_INDEX_OUT_OF_RANGE_IN_DELETENOTEKEY), arg_list[0]);

	note_track->keys.Delete(index,1);
	note_track->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	return &ok;
}

// #if 0 // No filter functionality for now
class MSTrackViewFilter : public TrackViewFilter
{
	Value* filter_fn;
public:	
	MSTrackViewFilter(Value* flt) { filter_fn = flt; }
	BOOL proc(Animatable *anim, Animatable *client,int subNum);
	BOOL TextColor(Animatable *anim, Animatable *client, int subNum, COLORREF& color) 
	{ 
		return FALSE; 
	}
};

BOOL MSTrackViewFilter::proc(Animatable *anim, Animatable *client,int subNum)
{
	// if (!anim || !client || client->SubAnim(subNum)!= NULL || filter_fn == &undefined) return TRUE;
	ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
	MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
	one_typed_value_local_tls(TrackViewPickValue *tvp_val);
	BOOL res = FALSE; 
	try 
	{
		ScopedSaveCurrentFrames scopedSaveCurrentFrames(_tls);
		TrackViewPick tvp; 
		tvp.anim = (ReferenceTarget*)anim; 
		tvp.client = (ReferenceTarget*)client;
		tvp.subNum = subNum;		
		vl.tvp_val = new TrackViewPickValue(tvp);
		Value *arg_list[] = { vl.tvp_val };
		res = filter_fn->apply(arg_list, 1)->to_bool();
	}
	catch (MAXScriptException& e)
	{
		filter_fn = &undefined;
		ProcessMAXScriptException(e, _T("TrackView Filter Function"), true, true, true);
	}
	catch (...)
	{
		filter_fn = &undefined;
		ProcessMAXScriptException(UnknownSystemException(), _T("TrackView Filter Function"), true, true, true);
	}
	return res;
}
// #endif



Value* 
tvw_pickTrackDlg_cf(Value** arg_list, int count)
{
	// trackView.pickTrackDlg [#mutliple]
	BOOL mult = FALSE;
	TrackViewFilter *filter = NULL;
	int pos_count = count_with_keys();
	for (int i=0; i < pos_count; i++)
	{
		Value *val = arg_list[i];
		if ( val == n_multiple)
			mult = TRUE;
		else if(is_function(val))
			filter = new MSTrackViewFilter(val);
		else
		{
			if (filter) delete filter;
			throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_RK_UNKNOWN_PICKTRACKDLG_ARG), val);
		}
	}
	Value* tmp;
	DWORD flags = int_key_arg(options,tmp,0);

	if (mult)
	{
		MaxSDK::Array<TrackViewPick> picks;
		if(!MAXScript_interface->TrackViewPickMultiDlg(MAXScript_interface->GetMAXHWnd(), &picks, filter, flags))
		{
			if (filter) 
				delete filter;
			return &undefined;
		}
		if (filter) 
			delete filter;
		MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
		one_typed_value_local_tls(Array* result); 
		vl.result = new Array((int)picks.length());
		for(int n = 0; n < picks.length(); n++)
			vl.result->append(new TrackViewPickValue(picks[n])); 
		return_value_tls(vl.result);		
	}
	else
	{
		TrackViewPick pick;
		if(MAXScript_interface->TrackViewPickDlg(MAXScript_interface->GetMAXHWnd(), &pick, filter, flags))
		{
			if (filter) delete filter;
			return new TrackViewPickValue(pick);
		}
		if (filter) delete filter;
		return &undefined;
	}
}

						
/* ------------------- MAXFilter methods  ---------------------- */

visible_class_instance (MAXFilter, "MAXFilterClass")

MAXFilter::MAXFilter(FilterKernel* ifilter)
{
	tag = class_tag(MAXFilter);
	filter = ifilter;
	make_ref(0, filter);	
}

Value*
MAXFilter::intern(FilterKernel* ifilter)
{
	if (ifilter == NULL) return &undefined;

	MAXFilter* mxa;
	MAXFilter** mxap;
	if (maxwrapper_cache_get(MAXFilter, ifilter, mxa, mxap) &&
	    mxa->filter == ifilter)
		return mxa;
	else
	{
		FindMAXWrapperEnum cb(ifilter,class_tag(MAXFilter));
		ifilter->DoEnumDependents(&cb);
		if (cb.result) 
		{	*mxap = (MAXFilter*)cb.result;
			return cb.result;
		}
		return (*mxap = new MAXFilter (ifilter));
	}
}

void
MAXFilter::sprin1(CharStream* s)
{
	TSTR cnm;
	check_for_deletion(); 
	filter->GetClassName(cnm);
	s->printf(_T("%s:%s"), cnm, filter->name);
}

Value*
MAXFilter::get_name(Value** arg_list, int count)
{
	check_for_deletion();
	return new String (filter->name);
}

Value*
MAXFilter::set_name(Value** arg_list, int count)
{
	check_for_deletion();
	filter->name = arg_list[0]->to_string();
	filter->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	return arg_list[0];
}

// some render related functions
Value*
set_anti_alias_filter(Value* val)
{
	Renderer* rend = MAXScript_interface->GetCurrentRenderer();	
	if (GetScanRendererInterface(rend))
	{
		IScanRenderer* srend = (IScanRenderer*)rend;
		srend->SetAntiAliasFilter(val->to_filter());
	}	
	return val;
}

Value*
get_anti_alias_filter()
{
	Renderer* rend = MAXScript_interface->GetCurrentRenderer();	
	if (GetScanRendererInterface(rend))
	{
		IScanRenderer* srend = (IScanRenderer*)rend;
		return_value (MAXFilter::intern(srend->GetAntiAliasFilter()));
	}
	return &undefined;
}
Value*
make_max_filter(MAXClass* cls, ReferenceTarget* existing, Value** arg_list, int count)
{
	FilterKernel* e;
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_value_local_tls(new_filter);
	check_arg_count_with_keys(filter create, 0, count);

	AnimateSuspend as; // LAM - 6/27/03 - defect 505170
	HoldSuspend hs; // LAM - 6/12/04 - defect 571821

	// wrap given Filter or or create the MAX object & initialize it
	if (existing == NULL)
	{
		if (cls->md_flags & md_no_create)
			throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_CANNOT_CREATE), cls->name->to_string());
			
		e = (FilterKernel*)MAXScript_interface->CreateInstance(cls->sclass_id, cls->class_id);
		if (e == NULL)
			throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_CANNOT_CREATE), cls->name->to_string());

		cls->initialize_object(e);

		if (count > 0)	// process keyword args 
		{
			cls->apply_keyword_parms(e, arg_list, count);
			cls->superclass->apply_keyword_parms(e, arg_list, count);
		}
	}
	else
		e = (FilterKernel*)existing;

	Value* name_obj = key_arg(name);
	if (name_obj != &unsupplied)		// name
		e->name = name_obj->to_string();

	// make the wrapper 
	vl.new_filter = MAXFilter::intern(e);
	return_value_tls (vl.new_filter);
}

ScripterExport MAXSuperClass filter_class
	(_T("Filter"), FILTER_KERNEL_CLASS_ID, &maxwrapper_class, make_max_filter,
		p_end
	);

// LAM: 9/10/01 More scanline renderer exposure

#undef def_bool_scanRenderer_prop
#undef def_int_scanRenderer_prop
#undef def_float_scanRenderer_prop
#undef GetScanRenderer2Interface

#define GetScanRendererInterface2(rend)	GetScanRendererInterface(rend)

#define def_bool_scanRenderer_prop(name, getter, setter, iface)						\
	Value* get_scanRenderer_##name()												\
	{																				\
		Renderer* rend = MAXScript_interface->GetCurrentRenderer();					\
		if (GetScanRendererInterface##iface(rend))									\
			return ((IScanRenderer##iface*)rend)->getter() ? &true_value : &false_value;	\
		return &undefined;															\
	}																				\
	Value* set_scanRenderer_##name(Value* val)										\
	{																				\
		Renderer* rend = MAXScript_interface->GetCurrentRenderer();					\
		if (GetScanRendererInterface##iface(rend))									\
			((IScanRenderer##iface*)rend)->setter(val->to_bool());					\
		return val;																	\
	}

#define def_int_scanRenderer_prop(name, getter, setter, iface)						\
	Value* get_scanRenderer_##name()												\
	{																				\
		Renderer* rend = MAXScript_interface->GetCurrentRenderer();					\
		if (GetScanRendererInterface##iface(rend))									\
			return_value (Integer::intern(((IScanRenderer##iface*)rend)->getter()));			\
		return &undefined;															\
	}																				\
	Value* set_scanRenderer_##name(Value* val)										\
	{																				\
		Renderer* rend = MAXScript_interface->GetCurrentRenderer();					\
		if (GetScanRendererInterface##iface(rend))									\
			((IScanRenderer##iface*)rend)->setter(val->to_int());					\
		return val;																	\
	}

#define def_float_scanRenderer_prop(name, getter, setter, iface)					\
	Value* get_scanRenderer_##name()												\
	{																				\
		Renderer* rend = MAXScript_interface->GetCurrentRenderer();					\
		if (GetScanRendererInterface##iface(rend))									\
			return_value (Float::intern(((IScanRenderer##iface*)rend)->getter()));			\
		return &undefined;															\
	}																				\
	Value* set_scanRenderer_##name(Value* val)										\
	{																				\
		Renderer* rend = MAXScript_interface->GetCurrentRenderer();					\
		if (GetScanRendererInterface##iface(rend))									\
			((IScanRenderer##iface*)rend)->setter(val->to_float());					\
		return val;																	\
	}

def_bool_scanRenderer_prop	( mapping,					GetMapping, SetMapping, 2);
def_bool_scanRenderer_prop	( shadows, 					GetShadows, SetShadows, 2);
def_bool_scanRenderer_prop	( autoReflect, 				GetAutoReflect, SetAutoReflect, 2);
def_bool_scanRenderer_prop	( forceWireframe, 			GetForceWire, SetForceWire, 2);
def_float_scanRenderer_prop	( wireThickness, 			GetWireThickness, SetWireThickness, 2);
def_bool_scanRenderer_prop	( antiAliasing, 			GetAntialias, SetAntialias, 2);
def_bool_scanRenderer_prop	( filterMaps, 				GetFilter, SetFilter, 2);
def_bool_scanRenderer_prop	( objectMotionBlur, 		GetObjMotBlur, SetObjMotBlur, 2);
def_float_scanRenderer_prop	( objectBlurDuration, 		GetObjBlurDuration, SetObjBlurDuration, 2); 
def_int_scanRenderer_prop	( objectBlurSamples, 		GetNBlurSamples, SetNBlurSamples, 2);
def_int_scanRenderer_prop	( objectBlurSubdivisions,	GetNBlurFrames, SetNBlurFrames, 2);
def_bool_scanRenderer_prop	( imageMotionBlur, 			GetVelMotBlur, SetVelMotBlur, 2);
def_float_scanRenderer_prop	( imageBlurDuration, 		GetVelBlurDuration, SetVelBlurDuration, 2);
def_int_scanRenderer_prop	( autoReflectLevels, 		GetAutoReflLevels, SetAutoReflLevels, 2);
def_int_scanRenderer_prop	( colorClampType, 			GetColorClampType, SetColorClampType, 2);
def_bool_scanRenderer_prop	( imageBlurEnv, 			GetApplyVelBlurEnv, SetApplyVelBlurEnv, 2);
def_bool_scanRenderer_prop	( imageBlurTrans, 			GetVelBlurTrans, SetVelBlurTrans, 2);
def_bool_scanRenderer_prop	( conserveMemory, 			GetMemFrugal, SetMemFrugal, 2);

def_float_scanRenderer_prop	( antiAliasFilterSz, 		GetAntiAliasFilterSz, SetAntiAliasFilterSz, 2);
def_bool_scanRenderer_prop	( pixelSamplerEnable, 		GetPixelSamplerEnable, SetPixelSamplerEnable, 2);

def_bool_scanRenderer_prop	( enableSSE, 				IsSSEEnabled, SetEnableSSE, 2);

/* ------------------- NodeEventCallbackValue class instance -------------- */

class ReleaseNodeEventCallbackValueEnumerator : public ValueMapper
{
public:
	void map(Value* val)
	{
		NodeEventCallbackValue *pNodeEventCallbackValue = static_cast<NodeEventCallbackValue*>(val);
		GetISceneEventManager()->UnRegisterCallback(pNodeEventCallbackValue->m_CallbackKey);
		pNodeEventCallbackValue->m_CallbackKey = 0;
	}
};

void NodeEventCallbackValue_Shutdown(void* param, NotifyInfo* info)
{
	ReleaseNodeEventCallbackValueEnumerator releaseNodeEventCallbackValueEnumerator;
	Collectable::for_all_values(nullptr, &releaseNodeEventCallbackValueEnumerator, class_tag(NodeEventCallbackValue));
}


visible_class_instance (NodeEventCallbackValue, "NodeEventCallback")

Value*
NodeEventCallbackValueClass::apply(Value** arg_list, int count, CallContext* cc)
{
	check_arg_count_with_keys(NodeEventCallback, 0, count);
	// do any error detection here, not in the NodeEventCallbackValue ctor

	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(NodeEventCallbackValue *res);
	// apply is a lazy call - args in arg list haven't been evaluated
	Value **evald_args, **ap, **eap;
	int i;
	value_local_array_tls(evald_args, count);
	for (i = count, ap = arg_list, eap = evald_args; i--; eap++, ap++)
		*eap = (*ap)->eval();

	vl.res = new NodeEventCallbackValue ();
	// apply keyword arguments
	for (i = count, ap = evald_args; i--; ap++)
	{
		if (*ap == &keyarg_marker)
		{
			int k;
			for (k = i/2, ap++; k--; ap += 2)
			{
//				*ap = Name::intern(*ap);
				Value* args[2] = { *(ap+1), *ap }; // flip order...
				vl.res->set_property(args,2);
			}
			break;
		}
	}

	BOOL polling = vl.res->polling;
	BOOL mouseUp = vl.res->mouseUp;
	int delayMilliseconds = vl.res->delayMilliseconds;
	if( polling && (mouseUp ||  (delayMilliseconds>0)) )
	{	// Polling mode cannot be used when either mouseUp or a delay is used
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_NODEEVENT_POLLING_CONFLICT));
	}

	// register to unregister NodeEventCallbackValue instances
	static BOOL sShutdownCallbackRegistered = RegisterNotification(NodeEventCallbackValue_Shutdown, nullptr,  NOTIFY_SYSTEM_SHUTDOWN);
	vl.res->m_CallbackKey = GetISceneEventManager()->RegisterCallback(vl.res, polling, delayMilliseconds, mouseUp);
	return_value_tls (vl.res);
}

/* -------------------- NodeEventCallbackValue methods ----------------------- */
NodeEventCallbackValue::NodeEventCallbackValue()
{
	tag = class_tag(NodeEventCallbackValue);
	m_CallbackKey = 0;
	enabled = true;
	polling = false;
	processing = false;
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_value_local_tls(_this);
	vl._this = this; // protect me against gc during array creation
	def_nodeeventcallback_event_types();
	m_CallbackFunctions = new Array(elements(nodeeventcallbackEventTypes));
	// Array needs to have its size set to be correctly gc_traced.
	m_CallbackFunctions->size = m_CallbackFunctions->data_size;
}

NodeEventCallbackValue::~NodeEventCallbackValue()
{
	if (m_CallbackKey)
		GetISceneEventManager()->UnRegisterCallback(m_CallbackKey);
}

void
NodeEventCallbackValue::gc_trace()
{
	/* trace sub-objects & mark me */
	Value::gc_trace();
	if (m_CallbackFunctions != NULL && m_CallbackFunctions->is_not_marked())
		m_CallbackFunctions->gc_trace();
}

void
NodeEventCallbackValue::sprin1(CharStream* s)
{
	s->printf(_T("NodeEventCallback<0x%p>"), this);
}

Value*
NodeEventCallbackValue::get_property(Value** arg_list, int count)
{
	Name* prop = (Name*)arg_list[0];
	int index = EventNameToIndex(prop);
	if (index >= 0)
		return m_CallbackFunctions->data[index];
	else if (prop == n_enabled)
		return bool_result (enabled);
	else if (prop == n_polling)
		return bool_result(polling);
	else if (prop == n_delay)
		return_value( Integer::intern(delayMilliseconds) );
	else if (prop == n_mouseUp)
		return bool_result(mouseUp);
	else
		return Value::get_property(arg_list, count);
}

Value*
NodeEventCallbackValue::set_property(Value** arg_list, int count)
{
	Value* val = arg_list[0];
	Name* prop = (Name*)arg_list[1];
	bool need_reregister = false;
	int index = EventNameToIndex(prop);
	if (index >= 0)
	{
		if (!val->_is_function())
			throw TypeError (_T("NodeEventCallback function:"), val, &MAXScriptFunction_class);
		m_CallbackFunctions->data[index] = val;
	}
	else if (prop == n_enabled)
		enabled = val->to_bool();
	else if (prop == n_polling)
	{
		BOOL oldVal = polling;
		polling = val->to_bool();
		need_reregister = oldVal != polling;
	}
	else if( prop == n_delay )
	{
		int oldVal = delayMilliseconds;
		delayMilliseconds = val->to_int();
		need_reregister = oldVal != delayMilliseconds;
	}
	else if( prop == n_mouseUp )
	{
		BOOL oldVal = mouseUp;
		mouseUp = val->to_bool();
		need_reregister = oldVal != mouseUp;
	}
	else
		return Value::set_property(arg_list, count);
	if (need_reregister && m_CallbackKey != 0)
	{
		GetISceneEventManager()->UnRegisterCallback(m_CallbackKey);
		m_CallbackKey = GetISceneEventManager()->RegisterCallback(this, polling, polling ? 0 : delayMilliseconds, polling ? FALSE : mouseUp);
	}
	return val;
}

Value*
NodeEventCallbackValue::get_props_vf(Value** arg_list, int count)
{
	// getPropNames <NodeEventCallback>  -- returns array of prop names
	def_nodeeventcallback_event_types();
	int numEvents = elements(nodeeventcallbackEventTypes);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	vl.result = new Array (numEvents+1);

	vl.result->append(n_enabled);
	for (int i = 0; i < numEvents; i++)
		vl.result->append(nodeeventcallbackEventTypes[i].val);
	return_value_tls(vl.result);
}

int 
NodeEventCallbackValue::EventNameToIndex(Value *eventName)
{
	def_nodeeventcallback_event_types();
	return ::ConvertValueToID(nodeeventcallbackEventTypes, elements(nodeeventcallbackEventTypes), eventName, -2);
}

void
NodeEventCallbackValue::CallHandler(Value *eventName, NodeKeyTab& nodes)
{
	if (!enabled)
		return;

	DbgAssert( processing==1 );

	// get the index for callbackBegin and callbackEnd since we don't want call the all event handler
	// when handling those events
	int callbackBegin_index = EventNameToIndex(n_callbackBegin);
	int callbackEnd_index = EventNameToIndex(n_callbackEnd);

	// get the event function if specified
	Value* event_fn = &undefined;
	int event_index = EventNameToIndex(eventName);
	if (event_index >= 0)
		event_fn = m_CallbackFunctions->data[event_index];

	// get the all function if specified
	Value* all_fn = &undefined;
	int all_index = EventNameToIndex(n_all);
	if (all_index >= 0 && event_index != callbackBegin_index && event_index != callbackEnd_index)
		all_fn = m_CallbackFunctions->data[all_index];

	if (event_fn != &undefined || all_fn != &undefined)
	{
		// calling from outside
		ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
		MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
		// protect myself from gc while running
		one_value_local_tls(me);
		vl.me = this;

		MacroRecorder::MacroRecorderDisable macroRecorderDisable;

		try 
		{
			ScopedSaveCurrentFrames scopedSaveCurrentFrames(_tls);
			two_typed_value_locals_tls(Array *nodes, Value *fn);
			vl.nodes = new Array(nodes.Count());
			for (int i = 0; i < nodes.Count(); i++)
				vl.nodes->append(Integer::intern(nodes[i]));
			Value* args[2] = { eventName, vl.nodes };

			// call the 'all' handler function
			if (all_fn != &undefined)
			{
				vl.fn = all_fn; // protect function from gc
				m_CallbackFunctions->data[all_index] = &undefined; // blocks recursion
				all_fn->apply(args, 2);
				m_CallbackFunctions->data[all_index] = vl.fn;
			}

			// call the handler function. Re-acquire since may have been removed via all handler. Also make sure still enabled
			event_fn = m_CallbackFunctions->data[event_index];
			if (event_fn != &undefined && enabled)
			{
				vl.fn = event_fn; // protect function from gc
				m_CallbackFunctions->data[event_index] = &undefined; // blocks recursion
				event_fn->apply(args, 2);
				m_CallbackFunctions->data[event_index] = vl.fn;
			}
		}
		catch (MAXScriptException& e)
		{
			ProcessMAXScriptException(e, MaxSDK::GetResourceStringAsMSTR(IDS_NODEEVENTCALLBACK), true, true, true);
		}
		catch (...)
		{
			ProcessMAXScriptException(UnknownSystemException(), MaxSDK::GetResourceStringAsMSTR(IDS_NODEEVENTCALLBACK), true, true, true);
		}
	}

}

// ---------- Hierarchy Events ----------
// Changes to the scene structure of an object
void
NodeEventCallbackValue::Added( NodeKeyTab& nodes )
{
	CallHandler(n_added, nodes);
}

void
NodeEventCallbackValue::Deleted( NodeKeyTab& nodes )
{
	CallHandler(n_deleted, nodes);
}

void
NodeEventCallbackValue::LinkChanged( NodeKeyTab& nodes )
{
	CallHandler(n_linkChanged, nodes);
}

void
NodeEventCallbackValue::LayerChanged( NodeKeyTab& nodes )
{
	CallHandler(n_layerChanged, nodes);
}

void
NodeEventCallbackValue::GroupChanged( NodeKeyTab& nodes )
{
	CallHandler(n_groupChanged, nodes);
}

void
NodeEventCallbackValue::HierarchyOtherEvent( NodeKeyTab& nodes )
{
	CallHandler(n_hierarchyOtherEvent, nodes);
}

// ---------- Model Events ----------
// Changes to the geometry or parameters of an object

void
NodeEventCallbackValue::ModelStructured( NodeKeyTab& nodes )
{
	CallHandler(n_modelStructured, nodes);
}

void
NodeEventCallbackValue::GeometryChanged( NodeKeyTab& nodes )
{
	CallHandler(n_geometryChanged, nodes);
}

void
NodeEventCallbackValue::TopologyChanged( NodeKeyTab& nodes )
{
	CallHandler(n_topologyChanged, nodes);
}

void
NodeEventCallbackValue::MappingChanged( NodeKeyTab& nodes )
{
	CallHandler(n_mappingChanged, nodes);
}

void
NodeEventCallbackValue::ExtentionChannelChanged( NodeKeyTab& nodes )
{
	CallHandler(n_extentionChannelChanged, nodes);
}

void
NodeEventCallbackValue::ModelOtherEvent( NodeKeyTab& nodes )
{
	CallHandler(n_modelOtherEvent, nodes);
}

// ---------- Material Events ----------
// Changes to the material on an object

void
NodeEventCallbackValue::MaterialStructured( NodeKeyTab& nodes )
{
	CallHandler(n_materialStructured, nodes);
}

void
NodeEventCallbackValue::MaterialOtherEvent( NodeKeyTab& nodes )
{
	CallHandler(n_materialOtherEvent, nodes);
}

// ---------- Controller Events ----------
// Changes to the controller on an object

void
NodeEventCallbackValue::ControllerStructured( NodeKeyTab& nodes )
{
	CallHandler(n_controllerStructured, nodes);
}

void
NodeEventCallbackValue::ControllerOtherEvent( NodeKeyTab& nodes )
{
	CallHandler(n_controllerOtherEvent, nodes);
}

// ---------- Property Events ----------
// Changes to object properties

void
NodeEventCallbackValue::NameChanged( NodeKeyTab& nodes )
{
	CallHandler(n_nameChanged, nodes);
}

void
NodeEventCallbackValue::WireColorChanged( NodeKeyTab& nodes )
{
	CallHandler(n_wireColorChanged, nodes);
}

void
NodeEventCallbackValue::RenderPropertiesChanged( NodeKeyTab& nodes )
{
	CallHandler(n_renderPropertiesChanged, nodes);
}

void
NodeEventCallbackValue::DisplayPropertiesChanged( NodeKeyTab& nodes )
{
	CallHandler(n_displayPropertiesChanged, nodes);
}

void
NodeEventCallbackValue::UserPropertiesChanged( NodeKeyTab& nodes )
{
	CallHandler(n_userPropertiesChanged, nodes);
}

void
NodeEventCallbackValue::PropertiesOtherEvent( NodeKeyTab& nodes )
{
	CallHandler(n_propertiesOtherEvent, nodes);
}

// ---------- Display/Interaction Events ----------
// Changes to the display or interactivity state of an object

void
NodeEventCallbackValue::SubobjectSelectionChanged( NodeKeyTab& nodes )
{
	CallHandler(n_subobjectSelectionChanged, nodes);
}

void
NodeEventCallbackValue::SelectionChanged( NodeKeyTab& nodes )
{
	CallHandler(n_selectionChanged, nodes);
}

void
NodeEventCallbackValue::HideChanged( NodeKeyTab& nodes )
{
	CallHandler(n_hideChanged, nodes);
}

void
NodeEventCallbackValue::FreezeChanged( NodeKeyTab& nodes )
{
	CallHandler(n_freezeChanged, nodes);
}

void
NodeEventCallbackValue::DisplayOtherEvent( NodeKeyTab& nodes )
{
	CallHandler(n_displayOtherEvent, nodes);
}

// ----------  ----------

void
NodeEventCallbackValue::CallbackBegin()
{
	processing++;
	NodeKeyTab nodes_empty;
	CallHandler(n_callbackBegin, nodes_empty);
}

void
NodeEventCallbackValue::CallbackEnd()
{
	NodeKeyTab nodes_empty;
	CallHandler(n_callbackEnd, nodes_empty);
	processing--;
}

BaseInterface* 
NodeEventCallbackValue::GetInterface(Interface_ID id)
{
	BaseInterface* l_interface = Value::GetInterface(id);
	if (l_interface == NULL)
		l_interface = INodeEventCallback::GetInterface(id);
	return l_interface;
}

Value*
triggerNodeEventCallback_cf(Value** arg_list, int count)
{
	// triggerNodeEventCallback <NodeEventCallbackValue>
	check_arg_count(triggerNodeEventCallback, 1, count);
	NodeEventCallbackValue* item = (NodeEventCallbackValue*)arg_list[0];
	type_check(item, NodeEventCallbackValue, _T("triggerNodeEventCallback <NodeEventCallbackValue>"));
	if (item->polling && item->enabled)
		GetISceneEventManager()->TriggerMessages(item->m_CallbackKey);
	return &ok;
}

class CollectNodeEventCallbackValueEnumerator : public ValueMapper
{
public:
	CollectNodeEventCallbackValueEnumerator(Array* instances) : m_instances(instances){}
	void map(Value* val)
	{
		m_instances->append(val);
	}

	Array* m_instances;
};

Value*
getNodeEventCallbacks_cf(Value** arg_list, int count)
{
	// getNodeEventCallbacks()
	check_arg_count(getNodeEventCallbacks, 0, count);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array *instances);
	vl.instances = new Array(0);
	CollectNodeEventCallbackValueEnumerator collector(vl.instances);
	Collectable::for_all_values(nullptr, &collector, class_tag(NodeEventCallbackValue));
	return_value(vl.instances);
}

void ProcessCallbackFunctionDisplay(NodeEventCallbackValue *pNodeEventCallbackValue, Value* eventID, Value* cbFn, Array* resArray, CharStream* out)
{
	TSTR fnName;
	while (is_structMethod(cbFn) || (is_pluginMethod(cbFn)))
	{
		// walk down to the function
		if (is_structMethod(cbFn))
		{
			StructMethod* sm = (StructMethod*)cbFn;
			if (sm->owning_def)
			{
				fnName.append(sm->owning_def->Name()->to_string());
				fnName.append(_T("::"));
			}
			cbFn = sm->fn;
		}
		else if (is_pluginMethod(cbFn))
		{
			PluginMethod* pm = (PluginMethod*)cbFn;
			if (pm->this_plugin)
			{
				fnName.append(pm->this_plugin->pc->name->to_string());
				fnName.append(_T("::"));
			}
			cbFn = pm->fn;
		}
	}

	DbgAssert(is_function(cbFn));
	if (!(is_function(cbFn)))
		return;

	Function* fn = (Function*)cbFn;
	if (fn->struct_name)
	{
		fnName.append(fn->struct_name);
		fnName.append(_T("::"));
	}
	fnName.append(fn->name);

	TSTR sourceFile;
	if (is_maxscriptfunction(cbFn))
	{
		MAXScriptFunction* mxsfn = (MAXScriptFunction*)cbFn;
		SourceFileWrapper* body = (SourceFileWrapper*)mxsfn->body;
		if (is_sourcefilewrapper(body) && body->file_name)
		{
			sourceFile = body->file_name->to_string();
		}
	}

	if (resArray)
	{
		Array* cb_info = new Array(4);
		resArray->append(cb_info);
		cb_info->append(pNodeEventCallbackValue);
		cb_info->append(eventID);
		cb_info->append(new String(fnName));
		if (sourceFile.isNull())
			cb_info->append(&undefined);
		else
			cb_info->append(new String(sourceFile));
	}
	else
	{
		pNodeEventCallbackValue->sprin1(out);
		out->printf(_T(", enabled:%s"), pNodeEventCallbackValue->enabled ? _T("true") : _T("false"));
		out->printf(_T(", polling:%s"), pNodeEventCallbackValue->polling ? _T("true") : _T("false"));
		out->printf(_T(", delay:%d"), pNodeEventCallbackValue->delayMilliseconds);
		out->printf(_T(", mouseUp:%s"), pNodeEventCallbackValue->mouseUp ? _T("true") : _T("false"));
		out->printf(_T(", eventId:#%s"), eventID->to_string());
		out->printf(_T(", function:\"%s\""), fnName.data());
		out->printf(_T(", sourceFile:\"%s\"\n"), sourceFile.isNull() ? _T("") : sourceFile.data());
	}
}

Value*
showNodeEventCallbacks_cf(Value** arg_list, int count)
{
	// showNodeEventCallbacks [to:stream] [asArray:<bool>]
	check_arg_count_with_keys(showNodeEventCallbacks, 0, count);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	two_typed_value_locals_tls(Array* instances, Array* res);

	Value* ov = key_arg_or_default(to, _tls->current_stdout);
	if (!is_charstream(ov))
		type_check(ov, CharStream, _T("showNodeEventCallbacks to:"));
	CharStream* out = (CharStream*)ov;

	BOOL asArray = bool_key_arg(asArray, ov, FALSE);
	if (asArray)
		vl.res = new Array(0);

	vl.instances = new Array(0);
	CollectNodeEventCallbackValueEnumerator collector(vl.instances);
	Collectable::for_all_values(nullptr, &collector, class_tag(NodeEventCallbackValue));

	def_nodeeventcallback_event_types();

	for (int i = 0; i < vl.instances->size; i++)
	{
		NodeEventCallbackValue *pNodeEventCallbackValue = static_cast<NodeEventCallbackValue*>(vl.instances->data[i]);
		for (int j = 0; j < elements(nodeeventcallbackEventTypes); j++)
		{
			Value* cbFn = pNodeEventCallbackValue->m_CallbackFunctions->data[j];
			if (cbFn != &undefined)
				ProcessCallbackFunctionDisplay(pNodeEventCallbackValue, nodeeventcallbackEventTypes[j].val,cbFn, vl.res, out);
		}
	}
	if (asArray)
		return_value_tls(vl.res);
	return &ok;
}

void MXSAgni_init2()
{
#	include "ExtClass_glbls.h"
}
