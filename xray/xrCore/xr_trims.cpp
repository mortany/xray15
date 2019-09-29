#include "stdafx.h"


LPTSTR _TrimLeft( LPTSTR str )
{
	LPTSTR p 	= str;
	while( *p && (u8(*p)<=u8(' ')) ) p++;
    if (p!=str){
		LPTSTR t = str;
        for (; *p; t++,p++) *t=*p;
        *t = 0;
    }
	return str;
}

LPTSTR _TrimRight( LPTSTR str )
{
	LPTSTR p 	= str+xr_strlen(str);
	while( (p!=str) && (u8(*p)<=u8(' ')) ) p--;
    *(++p) 		= 0;
	return str;
}

LPTSTR _Trim( LPTSTR str )
{
	_TrimLeft( str );
	_TrimRight( str );
	return str;
}

LPCTSTR _SetPos (LPCTSTR src, u32 pos, TCHAR separator )
{
	LPCTSTR	res			= src;
	u32		p			= 0;
	while( (p<pos) && (0!=(res=wcschr(res,separator))) )
	{
		res		++;
		p		++;
	}
	return		res;
}

LPCTSTR _CopyVal ( LPCTSTR src, LPTSTR dst, TCHAR separator )
{
	LPCTSTR	p;
	size_t	n;
	p			= wcschr( src, separator );
	n			= (p>0) ? (p-src) : xr_strlen(src);
	wcsncpy		( dst, src, n );
	dst[n]		= 0;
	return		dst;
}

int	_GetItemCount ( LPCTSTR src, TCHAR separator )
{
	u32		cnt			= 0;
	if (src&&src[0]){
		LPCTSTR	res			= src;
		LPCTSTR	last_res	= res;
		while( 0!=(res= wcschr(res,separator)) )
		{
			res		++;
			last_res=res;
			cnt		++;
			if (res[0]==separator) break;
		}
		if (xr_strlen(last_res)) cnt++;
	}
	return		cnt;
}

LPTSTR _GetItem ( LPCTSTR src, int index, LPTSTR dst, TCHAR separator, LPCTSTR def, bool trim )
{
	LPCTSTR	ptr;
	ptr			= _SetPos	( src, index, separator );
	if( ptr )	_CopyVal	( ptr, dst, separator );
		else	wcscpy		( dst, def );
	if (trim)	_Trim		( dst );
	return		dst;
}

LPTSTR _GetItems ( LPCTSTR src, int idx_start, int idx_end, LPTSTR dst, TCHAR separator )
{
	LPTSTR n = dst;
    int level = 0;
 	for (LPCTSTR p=src; *p!=0; p++){
    	if ((level>=idx_start)&&(level<idx_end))
			*n++ = *p;
    	if (*p==separator) level++;
        if (level>=idx_end) break;
    }
    *n = '\0';
	return dst;
}

u32 _ParseItem ( LPCTSTR src, xr_token* token_list )
{
	for( int i=0; token_list[i].name; i++ )
		if( !wcsicmp(src,token_list[i].name) )
			return token_list[i].id;
	return u32(-1);
}

u32 _ParseItem ( LPTSTR src, int ind, xr_token* token_list )
{
	TCHAR dst[128];
	_GetItem(src, ind, dst);
	return _ParseItem(dst, token_list);
}

LPTSTR _ReplaceItems( LPCTSTR src, int idx_start, int idx_end, LPCTSTR new_items, LPTSTR dst, TCHAR separator ){
	LPTSTR n = dst;
    int level = 0;
    bool bCopy = true;
	for (LPCTSTR p=src; *p!=0; p++){
    	if ((level>=idx_start)&&(level<idx_end)){
        	if (bCopy){
            	for (LPCTSTR itm = new_items; *itm!=0;) *n++ = *itm++;
                bCopy=false;
            }
	    	if (*p==separator) *n++ = separator;
        }else{
			*n++ = *p;
        }
    	if (*p==separator) level++;
    }
    *n = '\0';
	return dst;
}

LPTSTR _ReplaceItem ( LPCTSTR src, int index, LPCTSTR new_item, LPTSTR dst, TCHAR separator ){
	LPTSTR n = dst;
    int level = 0;
    bool bCopy = true;
	for (LPCTSTR p=src; *p!=0; p++){
    	if (level==index){
        	if (bCopy){
            	for (LPCTSTR itm = new_item; *itm!=0;) *n++ = *itm++;
                bCopy=false;
            }
	    	if (*p==separator) *n++ = separator;
        }else{
			*n++ = *p;
        }
    	if (*p==separator) level++;
    }
    *n = '\0';
	return dst;
}

LPTSTR _ChangeSymbol ( LPTSTR name, TCHAR src, TCHAR dest )
{
    TCHAR						*sTmpName = name;
    while(sTmpName[0] ){
		if (sTmpName[0] == src) sTmpName[0] = dest;
		sTmpName ++;
	}
	return						name;
}

xr_string& _ChangeSymbol	( xr_string& name, TCHAR src, TCHAR dest )
{
	for (xr_string::iterator it=name.begin(); it!=name.end(); it++) 
    	if (*it==src) *it=xr_string::value_type(dest);
    return  name;
}

#ifdef M_BORLAND
AnsiString& _ReplaceItem 	( LPCTSTR src, int index, LPCTSTR new_item, AnsiString& dst, TCHAR separator )
{
	dst = "";
    int level = 0;
    bool bCopy = true;
	for (LPCTSTR p=src; *p!=0; p++){
    	if (level==index){
        	if (bCopy){
            	for (LPCTSTR itm = new_item; *itm!=0;) dst += *itm++;
                bCopy=false;
            }
	    	if (*p==separator) dst += separator;
        }else{
			dst += *p;
        }
    	if (*p==separator) level++;
    }
	return dst;
}

AnsiString& _ReplaceItems ( LPCTSTR src, int idx_start, int idx_end, LPCTSTR new_items, AnsiString& dst, TCHAR separator )
{
	dst = "";
    int level = 0;
    bool bCopy = true;
	for (LPCTSTR p=src; *p!=0; p++){
    	if ((level>=idx_start)&&(level<idx_end)){
        	if (bCopy){
            	for (LPCTSTR itm = new_items; *itm!=0;) dst += *itm++;
                bCopy=false;
            }
	    	if (*p==separator) dst += separator;
        }else{
			dst += *p;
        }
    	if (*p==separator) level++;
    }
	return dst;
}

AnsiString& _Trim( AnsiString& str )
{
	return str=str.Trim();
}

LPCTSTR _CopyVal ( LPCTSTR src, AnsiString& dst, TCHAR separator )
{
	LPCTSTR	p;
	u32		n;
	p			= strchr	( src, separator );
	n			= (p>0) ? (p-src) : xr_strlen(src);
	dst			= src;
	dst			= dst.Delete(n+1,dst.Length());
	return		dst.c_str();
}

LPCTSTR _GetItems ( LPCTSTR src, int idx_start, int idx_end, AnsiString& dst, TCHAR separator )
{
	int level = 0;
	for (LPCTSTR p=src; *p!=0; p++){
		if ((level>=idx_start)&&(level<idx_end))
			dst += *p;
		if (*p==separator) level++;
		if (level>=idx_end) break;
	}
	return dst.c_str();
}

LPCTSTR _GetItem ( LPCTSTR src, int index, AnsiString& dst, TCHAR separator, LPCTSTR def, bool trim )
{
	LPCTSTR	ptr;
	ptr			= _SetPos	( src, index, separator );
	if( ptr )	_CopyVal	( ptr, dst, separator );
	else	dst = def;
	if (trim)	dst			= dst.Trim();
	return		dst.c_str();
}

AnsiString _ListToSequence(const AStringVec& lst)
{
	AnsiString out;
	out = "";
	if (lst.size()){
		out			= lst.front();
		for (AStringVec::const_iterator s_it=lst.begin()+1; s_it!=lst.end(); s_it++)
			out		+= AnsiString(",")+(*s_it);
	}
	return out;
}

AnsiString _ListToSequence2(const AStringVec& lst)
{
	AnsiString out;
	out = "";
	if (lst.size()){
		out			= lst.front();
		for (AStringVec::const_iterator s_it=lst.begin()+1; s_it!=lst.end(); s_it++){
			out		+= AnsiString("\n")+(*s_it);
		}
	}
	return out;
}

void _SequenceToList(AStringVec& lst, LPCTSTR in, TCHAR separator)
{
	lst.clear();
	int t_cnt=_GetItemCount(in,separator);
	AnsiString T;
	for (int i=0; i<t_cnt; i++){
		_GetItem(in,i,T,separator,0);
        _Trim(T);
        if (!T.IsEmpty()) lst.push_back(T);
	}
}

AnsiString FloatTimeToStrTime(float v, bool _h, bool _m, bool _s, bool _ms)
{
	AnsiString buf="";
    int h=0,m=0,s=0,ms;
    AnsiString t;
    if (_h){ h=iFloor(v/3600); 					t.sprintf("%02d",h); buf += t;}
    if (_m){ m=iFloor((v-h*3600)/60);			t.sprintf("%02d",m); buf += buf.IsEmpty()?t:":"+t;}
    if (_s){ s=iFloor(v-h*3600-m*60);			t.sprintf("%02d",s); buf += buf.IsEmpty()?t:":"+t;}
    if (_ms){ms=iFloor((v-h*3600-m*60-s)*1000.f);t.sprintf("%03d",ms);buf += buf.IsEmpty()?t:"."+t;}
    return buf;
}

float StrTimeToFloatTime(LPCTSTR buf, bool _h, bool _m, bool _s, bool _ms)
{
    float t[4]	= {0.f,0.f,0.f,0.f};
    int   rm[4];
    int idx		= 0;
    if (_h) rm[0]=idx++;
    if (_m) rm[1]=idx++;
    if (_s) rm[2]=idx++;
    if (_ms)rm[3]=idx;
    int cnt = _GetItemCount(buf,':');
    AnsiString tmp;
    for (int k=0; k<cnt; k++){
    	_GetItem(buf,k,tmp,':');
        t[rm[k]]=atof(tmp.c_str());
    }
    return t[0]*3600+t[1]*60+t[2];
}
#endif

void _SequenceToList(LPSTRVec& lst, LPCTSTR in, TCHAR separator)
{
	int t_cnt=_GetItemCount(in,separator);
	string1024 T;
	for (int i=0; i<t_cnt; i++){
		_GetItem(in,i,T,separator,0);
        _Trim(T);
        if (xr_strlen(T)) lst.push_back(xr_strdup(T));
	}
}

void _SequenceToList(RStringVec& lst, LPCTSTR in, TCHAR separator)
{
	lst.clear	();
	int t_cnt	= _GetItemCount(in,separator);
	xr_string	T;
	for (int i=0; i<t_cnt; i++){
		_GetItem(in,i,T,separator,0);
        _Trim	(T);
        if (T.size()) lst.push_back(T.c_str());
	}
}

void _SequenceToList(SStringVec& lst, LPCTSTR in, TCHAR separator)
{
	lst.clear	();
	int t_cnt	= _GetItemCount(in,separator);
	xr_string	T;
	for (int i=0; i<t_cnt; i++){
		_GetItem(in,i,T,separator,0);
		_Trim	(T);
		if (T.size()) lst.push_back(T.c_str());
	}
}

xr_string	_ListToSequence(const SStringVec& lst)
{
	static xr_string	out;
	out = TEXT("");
	if (lst.size()){
    	out			= lst.front();
		for (SStringVec::const_iterator s_it=lst.begin()+1; s_it!=lst.end(); s_it++)
        	out		+= xr_string(TEXT(","))+(*s_it);
	}
	return out;
}

xr_string& _TrimLeft( xr_string& str )
{
	LPCTSTR b		= str.c_str();
	LPCTSTR p 		= str.c_str();
	while( *p && (u8(*p)<=u8(' ')) ) p++;
    if (p!=b)
    	str.erase	(0,p-b);
	return str;
}

xr_string& _TrimRight( xr_string& str )
{
	LPCTSTR b		= str.c_str();
    size_t l		= str.length();
    if (l){
        LPCTSTR p 		= str.c_str()+l-1;
        while( (p!=b) && (u8(*p)<=u8(' ')) ) p--;
        if (p!=(str+b))	str.erase	(p-b+1,l-(p-b));
    }
	return str;
}

xr_string& _Trim( xr_string& str )
{
	_TrimLeft		( str );
	_TrimRight		( str );
	return str;
}

LPCTSTR _CopyVal ( LPCTSTR src, xr_string& dst, TCHAR separator )
{
	LPCTSTR		p;
	ptrdiff_t	n;
	p			= wcschr( src, separator );
	n			= (p>0) ? (p-src) : xr_strlen(src);
	dst			= src;
	dst			= dst.erase	(n,dst.length());
	return		dst.c_str();
}

LPCTSTR _GetItem ( LPCTSTR src, int index, xr_string& dst, TCHAR separator, LPCTSTR def, bool trim )
{
	LPCTSTR	ptr;
	ptr			= _SetPos	( src, index, separator );
	if( ptr )	_CopyVal	( ptr, dst, separator );
	else	dst = def;
	if (trim)	_Trim		(dst);
	return		dst.c_str	();
}

shared_str	_ListToSequence(const RStringVec& lst)
{
	xr_string		out;
	if (lst.size()){
    	out			= *lst.front();
		for (RStringVec::const_iterator s_it=lst.begin()+1; s_it!=lst.end(); s_it++){
        	out		+= TEXT(",");
            out		+= **s_it;
        }
	}
	return shared_str	(out.c_str());
}

