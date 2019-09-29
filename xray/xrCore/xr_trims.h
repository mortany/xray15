#ifndef xr_trimsH
#define xr_trimsH

// refs
struct xr_token;

#ifdef __BORLANDC__
	XRCORE_API 	AnsiString&	_Trim					( AnsiString& str );
	XRCORE_API 	LPCTSTR		_GetItem				( LPCTSTR src, int, AnsiString& p, TCHAR separator=',', LPCTSTR ="", bool trim=true);
	XRCORE_API 	LPCTSTR		_GetItems 				( LPCTSTR src, int idx_start, int idx_end, AnsiString& dst, TCHAR separator );
	XRCORE_API 	LPCTSTR		_CopyVal 				( LPCTSTR src, AnsiString& dst, TCHAR separator=',' );
	XRCORE_API 	AnsiString	_ListToSequence			( const AStringVec& lst );
	XRCORE_API 	AnsiString	_ListToSequence2		( const AStringVec& lst );
	XRCORE_API 	void 		_SequenceToList			( AStringVec& lst, LPCTSTR in, TCHAR separator=',' );
	XRCORE_API 	AnsiString&	_ReplaceItem 			( LPCTSTR src, int index, LPCTSTR new_item, AnsiString& dst, TCHAR separator );
	XRCORE_API 	AnsiString&	_ReplaceItems 			( LPCTSTR src, int idx_start, int idx_end, LPCTSTR new_items, AnsiString& dst, TCHAR separator );
	XRCORE_API 	AnsiString 	FloatTimeToStrTime		(float v, bool h=true, bool m=true, bool s=true, bool ms=false);
	XRCORE_API 	float 		StrTimeToFloatTime		(LPCTSTR buf, bool h=true, bool m=true, bool s=true, bool ms=false);
#endif

XRCORE_API int		    	_GetItemCount			( LPCTSTR , TCHAR separator=',');
XRCORE_API LPTSTR	    	_GetItem				( LPCTSTR, int, LPTSTR, TCHAR separator=',', LPCTSTR =TEXT(""), bool trim=true );
XRCORE_API LPTSTR	    	_GetItems				( LPCTSTR, int, int, LPTSTR, TCHAR separator=',');
XRCORE_API LPCTSTR	    	_SetPos					( LPCTSTR src, u32 pos, TCHAR separator=',' );
XRCORE_API LPCTSTR	    	_CopyVal				( LPCTSTR src, LPTSTR dst, TCHAR separator=',' );
XRCORE_API LPTSTR	    	_Trim					( LPTSTR str );
XRCORE_API LPTSTR	    	_TrimLeft				( LPTSTR str );
XRCORE_API LPTSTR	    	_TrimRight				( LPTSTR str );
XRCORE_API LPTSTR	    	_ChangeSymbol			( LPTSTR name, TCHAR src, TCHAR dest );
XRCORE_API u32		    	_ParseItem				( LPCTSTR src, xr_token* token_list );
XRCORE_API u32		    	_ParseItem				( LPTSTR src, int ind, xr_token* token_list );
XRCORE_API LPTSTR 	    	_ReplaceItem 			( LPCTSTR src, int index, LPCTSTR new_item, LPTSTR dst, TCHAR separator );
XRCORE_API LPTSTR 	    	_ReplaceItems 			( LPCTSTR src, int idx_start, int idx_end, LPCTSTR new_items, LPTSTR dst, TCHAR separator );
XRCORE_API void 	    	_SequenceToList			( LPSTRVec& lst, LPCTSTR in, TCHAR separator=',' );
XRCORE_API void 			_SequenceToList			( RStringVec& lst, LPCTSTR in, TCHAR separator=',' );
XRCORE_API void 			_SequenceToList			( SStringVec& lst, LPCTSTR in, TCHAR separator=',' );

XRCORE_API xr_string& 		_Trim					( xr_string& src );
XRCORE_API xr_string& 		_TrimLeft				( xr_string& src );
XRCORE_API xr_string&		_TrimRight				( xr_string& src );
XRCORE_API xr_string&   	_ChangeSymbol			( xr_string& name, TCHAR src, TCHAR dest );
XRCORE_API LPCTSTR		 	_CopyVal 				( LPCTSTR src, xr_string& dst, TCHAR separator=',' );
XRCORE_API LPCTSTR			_GetItem				( LPCTSTR src, int, xr_string& p, TCHAR separator=',', LPCTSTR =TEXT(""), bool trim=true );
XRCORE_API xr_string		_ListToSequence			( const SStringVec& lst );
XRCORE_API shared_str		_ListToSequence			( const RStringVec& lst );

#endif
