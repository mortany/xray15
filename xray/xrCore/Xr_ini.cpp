#include "stdafx.h"


#include "fs_internal.h"

XRCORE_API CInifile *pSettings	= NULL;

CInifile* CInifile::Create(const TCHAR* szFileName, BOOL ReadOnly)
{	return xr_new<CInifile>(szFileName,ReadOnly); }

void CInifile::Destroy(CInifile* ini)
{	xr_delete(ini); }

bool sect_pred(const CInifile::Sect *x, LPCTSTR val)
{
	return xr_strcmp(*x->Name,val)<0;
};

bool item_pred(const CInifile::Item& x, LPCTSTR val)
{
    if ((!x.first) || (!val))	return x.first<val;
    else				   		return xr_strcmp(*x.first,val)<0;
}

//------------------------------------------------------------------------------
//Тело функций Inifile
//------------------------------------------------------------------------------
XRCORE_API BOOL _parse(LPTSTR dest, LPCTSTR src)
{
	BOOL bInsideSTR = false;
	if (src) 
	{
		while (*src) 
		{
			if (isspace((u8)*src)) 
			{
				if (bInsideSTR)
				{
					*dest++ = *src++;
					continue;
				}
				while (*src && isspace(*src))
				{
					++src;
				}
				continue;
			} else if (*src=='"') 
			{
				bInsideSTR = !bInsideSTR;
			}
			*dest++ = *src++;
		}
	}
	*dest = 0;
	return bInsideSTR;
}

XRCORE_API void _decorate(LPTSTR dest, LPCTSTR src)
{
	if (src) 
	{
		BOOL bInsideSTR = false;
		while (*src) 
		{
			if (*src == ',') 
			{
				if (bInsideSTR) { *dest++ = *src++; }
				else			{ *dest++ = *src++; *dest++ = ' '; }
				continue;
			} else if (*src=='"') 
			{
				bInsideSTR = !bInsideSTR;
			}
			*dest++ = *src++;
		}
	}
	*dest = 0;
}
//------------------------------------------------------------------------------

BOOL	CInifile::Sect::line_exist( LPCTSTR L, LPCTSTR* val )
{
	SectCIt A = std::lower_bound(Data.begin(),Data.end(),L,item_pred);
    if (A!=Data.end() && xr_strcmp(*A->first,L)==0){
    	if (val) *val = *A->second;
    	return TRUE;
    }
	return FALSE;
}
//------------------------------------------------------------------------------

CInifile::CInifile(IReader* F ,LPCTSTR path)
{
	m_file_name[0]	= 0;
	m_flags.zero	();
	m_flags.set		(eSaveAtEnd,		FALSE);
	m_flags.set		(eReadOnly,			TRUE);
	m_flags.set		(eOverrideNames,	FALSE);
	Load			(F,path);
}

CInifile::CInifile(LPCTSTR szFileName, BOOL ReadOnly, BOOL bLoad, BOOL SaveAtEnd)
{
	m_file_name[0]	= 0;
	m_flags.zero	();
	if(szFileName)
		wcscpy_s		(m_file_name,szFileName);

	m_flags.set		(eSaveAtEnd, SaveAtEnd);
	m_flags.set		(eReadOnly, ReadOnly);

	if (bLoad)
	{	
    	string_path	path,folder; 
		_wsplitpath	(m_file_name, path, folder, 0, 0 );
        wcscat		(path,folder);
		IReader* R 	= FS.r_open(szFileName);
        if (R){
			Load		(R,path);
			FS.r_close	(R);
        }
	}
}

CInifile::~CInifile( )
{
	if (!m_flags.test(eReadOnly) && m_flags.test(eSaveAtEnd)) 
	{
		if (!save_as())
			Log		(TEXT("!Can't save inifile:"),m_file_name);
	}

	RootIt			I = DATA.begin();
	RootIt			E = DATA.end();
	for ( ; I != E; ++I)
		xr_delete	(*I);
}

static void	insert_item(CInifile::Sect *tgt, const CInifile::Item& I)
{
	CInifile::SectIt_	sect_it		= std::lower_bound(tgt->Data.begin(),tgt->Data.end(),*I.first,item_pred);
	if (sect_it!=tgt->Data.end() && sect_it->first.equal(I.first))
	{ 
		sect_it->second	= I.second;
#ifdef DEBUG
		sect_it->comment= I.comment;
#endif
	}else{
		tgt->Data.insert	(sect_it,I);
	}
}

IC BOOL	is_empty_line_now(IReader* F) 
{ 
	TCHAR* a0 = (TCHAR*)F->pointer()-4;
	TCHAR* a1 = (TCHAR*)(F->pointer())-3;
	TCHAR* a2 = (TCHAR*)F->pointer()-2;
	TCHAR* a3 = (TCHAR*)(F->pointer())-1;
    
	return (*a0==13) && ( *a1==10) && (*a2==13) && ( *a3==10); 
};

void	CInifile::Load(IReader* F, LPCTSTR path)
{
	R_ASSERT(F);
	Sect		*Current = 0;
	string4096	str;
	string4096	str2;
	
	BOOL bInsideSTR = FALSE;

	while (!F->eof())
	{
		F->r_string		(str,sizeof(str));
		_Trim			(str);
		LPTSTR comm		= wcschr(str,';');
		LPTSTR comm_1	= wcschr(str,'/');
		
		if(comm_1 && (*(comm_1+1)=='/') && ((!comm) || (comm && (comm_1<comm) )) )
		{
			comm = comm_1;
		}

#ifdef DEBUG
		LPTSTR comment	= 0;
#endif
		if (comm) 
		{
			//."bla-bla-bla;nah-nah-nah"
			TCHAR quot = '"';
			bool in_quot = false;
			
			LPCTSTR q1		= wcschr(str,quot);
			if(q1 && q1<comm)
			{
				LPCTSTR q2 = wcschr(++q1,quot);
				if(q2 && q2>comm)
					in_quot = true;
			}

			if(!in_quot)
			{
				*comm		= 0;
#ifdef DEBUG
			comment		= comm+1;
#endif
			}
		}

		
        if (str[0] && (str[0]=='#') && wcsstr(str,TEXT("#include"))) //handle includes
		{
        	string_path		inc_name;	
			R_ASSERT		(path&&path[0]);
        	if (_GetItem	(str,1,inc_name,'"'))
			{
            	string_path	fn,inc_path,folder;
                strconcat	(sizeof(fn),fn,path,inc_name);

				

				_wsplitpath	(fn,inc_path,folder, 0, 0 );
				wcscat		(inc_path,folder);
            	IReader* I 	= FS.r_open(fn); R_ASSERT3(I,"Can't find include file:", inc_name);
            	Load		(I,inc_path);
                FS.r_close	(I);
            }
        } 
		else if (str[0] && (str[0]=='[')) //new section ?
		{
			// insert previous filled section
			if (Current)
			{
				//store previous section
				RootIt I		= std::lower_bound(DATA.begin(),DATA.end(),*Current->Name,sect_pred);
				if ((I!=DATA.end())&&((*I)->Name==Current->Name))
					//Debug.fatal(DEBUG_INFO,"Duplicate section '%s' found.",*Current->Name);
				DATA.insert		(I,Current);
			}
			Current				= xr_new<Sect>();
			Current->Name		= 0;
			// start new section
			//R_ASSERT3(strchr(str,']'),"Bad ini section found: ",str);
			LPCTSTR inherited_names = wcsstr(str,TEXT("]:"));
			if (0!=inherited_names)
			{
				VERIFY2				(m_flags.test(eReadOnly),"Allow for readonly mode only.");
				inherited_names		+= 2;
				int cnt				= _GetItemCount(inherited_names);
				
				for (int k=0; k<cnt; ++k)
				{
					xr_string	tmp;
					_GetItem	(inherited_names,k,tmp);
					Sect& inherited_section = r_section(tmp.c_str());
					for (SectIt_ it =inherited_section.Data.begin(); it!=inherited_section.Data.end(); it++)
						insert_item	(Current,*it);
				}
			}
			*wcschr(str,']') 	= 0;
			Current->Name 		= xr_strlwr(str+1);
		} 
		else // name = value
		{
			if (Current)
			{
				string4096			value_raw;
				TCHAR*		name	= str;
				TCHAR*		t		= wcschr(name,'=');
				if (t)		
				{
					*t				= 0;
					_Trim			(name);
					++t;
					wcscpy_s		(value_raw, sizeof(value_raw), t);
					bInsideSTR		= _parse(str2, value_raw);
					if(bInsideSTR)//multiline str value
					{
						while(bInsideSTR)
						{
							wcscat_s		(value_raw, sizeof(value_raw),TEXT("\r\n"));
							string4096		str_add_raw;
							F->r_string		(str_add_raw, sizeof(str_add_raw));
							wcscat_s(value_raw, sizeof(value_raw),str_add_raw);
							bInsideSTR		= _parse(str2, value_raw);
                            if(bInsideSTR)
                            {
                            	if( is_empty_line_now(F) )
									wcscat_s(value_raw, sizeof(value_raw),TEXT("\r\n"));
                            }
						}
					}
				} else 
				{
					_Trim	(name);
					str2[0]	= 0;
				}

				Item		I;
				I.first		= (name[0]?name:NULL);
				I.second	= (str2[0]?str2:NULL);
#ifdef DEBUG
				I.comment	= m_flags.test(eReadOnly)?0:comment;
#endif

				if (m_flags.test(eReadOnly)) 
				{
					if (*I.first)							insert_item	(Current,I);
				} else 
				{
					if	(
							*I.first
							|| *I.second 
#ifdef DEBUG
							|| *I.comment
#endif
						)
						insert_item	(Current,I);
				}
			}
		}
	}
	if (Current)
	{
		RootIt I		= std::lower_bound(DATA.begin(),DATA.end(),*Current->Name,sect_pred);
		if ((I!=DATA.end())&&((*I)->Name==Current->Name))
		DATA.insert		(I,Current);
	}
}

void CInifile::save_as	(IWriter& writer)
{
    string4096		temp,val;
    for (RootIt r_it=DATA.begin(); r_it!=DATA.end(); ++r_it)
	{
		wprintf_s(temp,sizeof(temp),"[%s]",*(*r_it)->Name);
        writer.w_string	(temp);
        for (SectCIt s_it=(*r_it)->Data.begin(); s_it!=(*r_it)->Data.end(); ++s_it)
        {
            const Item&	I = *s_it;
            if (*I.first) 
			{
                if (*I.second) {
                    _decorate	(val,*I.second);
#ifdef DEBUG
                    if (*I.comment) {
                        // name, value and comment
                        sprintf_s	(temp,sizeof(temp),"%8s%-32s = %-32s ;%s"," ",*I.first,val,*I.comment);
                    } else
#endif
					{
                        // only name and value
						wprintf_s(temp,sizeof(temp),"%8s%-32s = %-32s"," ",*I.first,val);
                    }
                } else {
#ifdef DEBUG
                    if (*I.comment) {
                        // name and comment
                        sprintf_s(temp,sizeof(temp),"%8s%-32s = ;%s"," ",*I.first,*I.comment);
                    } else
#endif
					{
                        // only name
						wprintf_s(temp,sizeof(temp),"%8s%-32s = "," ",*I.first);
                    }
                }
            } else {
                // no name, so no value
#ifdef DEBUG
                if (*I.comment)
					sprintf_s		(temp,sizeof(temp),"%8s;%s"," ",*I.comment);
                else
#endif
					temp[0]		= 0;
            }
            _TrimRight			(temp);
            if (temp[0])		writer.w_string	(temp);
        }
        writer.w_string			(TEXT(" "));
    }
}

bool CInifile::save_as	(LPCTSTR new_fname)
{
	// save if needed
    if (new_fname && new_fname[0])
		wcscpy_s		(m_file_name, new_fname);

    R_ASSERT			(m_file_name&&m_file_name[0]);
    IWriter* F			= FS.w_open_ex(m_file_name);
    if (!F)
		return			(false);

	save_as				(*F);
    FS.w_close			(F);
    return				(true);
}

BOOL	CInifile::section_exist( LPCTSTR S )
{
	RootIt I = std::lower_bound(DATA.begin(),DATA.end(),S,sect_pred);
	return (I!=DATA.end() && xr_strcmp(*(*I)->Name,S)==0);
}

BOOL	CInifile::line_exist( LPCTSTR S, LPCTSTR L )
{
	if (!section_exist(S)) return FALSE;
	Sect&	I = r_section(S);
	SectCIt A = std::lower_bound(I.Data.begin(),I.Data.end(),L,item_pred);
	return (A!=I.Data.end() && xr_strcmp(*A->first,L)==0);
}

u32		CInifile::line_count(LPCTSTR Sname)
{
	Sect&	S = r_section(Sname);
	SectCIt	I = S.Data.begin();
	u32	C = 0;
	for (; I!=S.Data.end(); I++)	if (*I->first) C++;
	return  C;
}


//--------------------------------------------------------------------------------------
CInifile::Sect&	CInifile::r_section		( const shared_str& S	)					{ return	r_section(*S);		}
BOOL			CInifile::line_exist	( const shared_str& S, const shared_str& L )	{ return	line_exist(*S,*L);	}
u32				CInifile::line_count	( const shared_str& S	)					{ return	line_count(*S);		}
BOOL			CInifile::section_exist	( const shared_str& S	)					{ return	section_exist(*S);	}

//--------------------------------------------------------------------------------------
// Read functions
//--------------------------------------------------------------------------------------
CInifile::Sect& CInifile::r_section( LPCTSTR S )
{
	TCHAR	section[256]; wcscpy_s(section,sizeof(section),S); xr_strlwr(section);
	RootIt I = std::lower_bound(DATA.begin(),DATA.end(),section,sect_pred);
	return	**I;
}

LPCTSTR	CInifile::r_string(LPCTSTR S, LPCTSTR L)
{
	Sect&	I = r_section(S);
	SectCIt	A = std::lower_bound(I.Data.begin(),I.Data.end(),L,item_pred);
	if (A!=I.Data.end() && xr_strcmp(*A->first,L)==0)	return *A->second;
	return 0;
}

shared_str		CInifile::r_string_wb(LPCTSTR S, LPCTSTR L)	{
	LPCTSTR		_base		= r_string(S,L);
	
	if	(0==_base)					return	shared_str(0);

	string4096						_original;
	wcscpy_s						(_original,_base);
	u32			_len				= xr_strlen(_original);
	if	(0==_len)					return	shared_str(TEXT(""));
	if	('"'==_original[_len-1])	_original[_len-1]=0;				// skip end
	if	('"'==_original[0])			return	shared_str(&_original[0] + 1);	// skip begin
	return									shared_str(_original);
}

u8 CInifile::r_u8(LPCTSTR S, LPCTSTR L)
{
	LPCTSTR		C = r_string(S,L);
	return		u8(_wtoi(C));
}
u16 CInifile::r_u16(LPCTSTR S, LPCTSTR L)
{
	LPCTSTR		C = r_string(S,L);
	return		u16(_wtoi(C));
}
u32 CInifile::r_u32(LPCTSTR S, LPCTSTR L)
{
	LPCTSTR		C = r_string(S,L);
	return		u32(_wtoi(C));
}
u64 CInifile::r_u64(LPCTSTR S, LPCTSTR L)
{
	LPCTSTR		C = r_string(S,L);
#ifndef _EDITOR
	return		_wcstoui64(C,NULL,10);
#else
	return		(u64)_wtoi64(C);
#endif
}

s64 CInifile::r_s64(LPCTSTR S, LPCTSTR L)
{
	LPCTSTR		C = r_string(S,L);
	return		_wtoi64(C);
}
s8 CInifile::r_s8(LPCTSTR S, LPCTSTR L)
{
	LPCTSTR		C = r_string(S,L);
	return		s8(_wtoi(C));
}
s16 CInifile::r_s16(LPCTSTR S, LPCTSTR L)
{
	LPCTSTR		C = r_string(S,L);
	return		s16(_wtoi(C));
}
s32 CInifile::r_s32(LPCTSTR S, LPCTSTR L)
{
	LPCTSTR		C = r_string(S,L);
	return		s32(_wtoi(C));
}
float CInifile::r_float(LPCTSTR S, LPCTSTR L)
{
	LPCTSTR		C = r_string(S,L);
	return		float(_wtof( C ));
}
Fcolor CInifile::r_fcolor( LPCTSTR S, LPCTSTR L )
{
	LPCTSTR		C = r_string(S,L);
	Fcolor		V={0,0,0,0};
	wscanf		(C,"%f,%f,%f,%f",&V.r,&V.g,&V.b,&V.a);
	return V;
}
u32 CInifile::r_color( LPCTSTR S, LPCTSTR L )
{
	LPCTSTR		C = r_string(S,L);
	u32			r=0,g=0,b=0,a=255;
	wscanf		(C,"%d,%d,%d,%d",&r,&g,&b,&a);
	return color_rgba(r,g,b,a);
}

Ivector2 CInifile::r_ivector2( LPCTSTR S, LPCTSTR L )
{
	LPCTSTR		C = r_string(S,L);
	Ivector2	V={0,0};
	wscanf		(C,"%d,%d",&V.x,&V.y);
	return V;
}
Ivector3 CInifile::r_ivector3( LPCTSTR S, LPCTSTR L )
{
	LPCTSTR		C = r_string(S,L);
	Ivector		V={0,0,0};
	wscanf		(C,"%d,%d,%d",&V.x,&V.y,&V.z);
	return V;
}
Ivector4 CInifile::r_ivector4( LPCTSTR S, LPCTSTR L )
{
	LPCTSTR		C = r_string(S,L);
	Ivector4	V={0,0,0,0};
	wscanf		(C,"%d,%d,%d,%d",&V.x,&V.y,&V.z,&V.w);
	return V;
}
Fvector2 CInifile::r_fvector2( LPCTSTR S, LPCTSTR L )
{
	LPCTSTR		C = r_string(S,L);
	Fvector2	V={0.f,0.f};
	wscanf		(C,"%f,%f",&V.x,&V.y);
	return V;
}
Fvector3 CInifile::r_fvector3( LPCTSTR S, LPCTSTR L )
{
	LPCTSTR		C = r_string(S,L);
	Fvector3	V={0.f,0.f,0.f};
	wscanf		(C,"%f,%f,%f",&V.x,&V.y,&V.z);
	return V;
}
Fvector4 CInifile::r_fvector4( LPCTSTR S, LPCTSTR L )
{
	LPCTSTR		C = r_string(S,L);
	Fvector4	V={0.f,0.f,0.f,0.f};
	wscanf		(C,"%f,%f,%f,%f",&V.x,&V.y,&V.z,&V.w);
	return V;
}
BOOL	CInifile::r_bool( LPCTSTR S, LPCTSTR L )
{
	LPCTSTR		C = r_string(S,L);
	VERIFY2		(
		xr_strlen(C) <= 5,
		make_string(
			"\"%s\" is not a valid bool value, section[%s], line[%s]",
			C,
			S,
			L
		)
	);
	TCHAR		B[8];
	wcsncpy		(B,C,7);
	B[7]		= 0;
	xr_strlwr		(B);
    return 		IsBOOL(B);
}
CLASS_ID CInifile::r_clsid( LPCTSTR S, LPCTSTR L)
{
	LPCTSTR		C = r_string(S,L);
	return		TEXT2CLSID(C);
}
int		CInifile::r_token	( LPCTSTR S, LPCTSTR L, const xr_token *token_list)
{
	LPCTSTR		C = r_string(S,L);
	for( int i=0; token_list[i].name; i++ )
		if( !_wcsicmp(C,token_list[i].name) )
			return token_list[i].id;
	return 0;
}
BOOL	CInifile::r_line( LPCTSTR S, int L, const TCHAR** N, const TCHAR** V )
{
	Sect&	SS = r_section(S);
	if (L>=(int)SS.Data.size() || L<0 ) return FALSE;
	for (SectCIt I=SS.Data.begin(); I!=SS.Data.end(); I++)
		if (!(L--)){
			*N = *I->first;
			*V = *I->second;
			return TRUE;
		}
	return FALSE;
}
BOOL	CInifile::r_line( const shared_str& S, int L, const TCHAR** N, const TCHAR** V )
{
	return r_line(*S,L,N,V);
}

//--------------------------------------------------------------------------------------------------------
// Write functions
//--------------------------------------------------------------------------------------
void CInifile::w_string( LPCTSTR S, LPCTSTR L, LPCTSTR V, LPCTSTR comment)
{
	R_ASSERT			(!m_flags.test(eReadOnly));

	// section
	string256			sect;
	_parse				(sect,S);
	xr_strlwr			(sect);
	
	if (!section_exist(sect))	
	{
		// create _new_ section
		Sect			*NEW = xr_new<Sect>();
		NEW->Name		= sect;
		RootIt I		= std::lower_bound(DATA.begin(),DATA.end(),sect,sect_pred);
		DATA.insert		(I,NEW);
	}

	// parse line/value
	string4096			line;
	_parse				(line,L);
	string4096			value;	
	_parse				(value,V);

	// duplicate & insert
	Item	I;
	Sect&	data	= r_section	(sect);
	I.first			= (line[0]?line:0);
	I.second		= (value[0]?value:0);

#ifdef DEBUG
	I.comment		= (comment?comment:0);
#endif
	SectIt_	it		= std::lower_bound(data.Data.begin(),data.Data.end(),*I.first,item_pred);

    if (it != data.Data.end()) 
	{
	    // Check for "first" matching
    	if (0==xr_strcmp(*it->first, *I.first)) 
		{
			BOOL b = m_flags.test(eOverrideNames);
			//R_ASSERT2(b,make_string("name[%s] already exist in section[%s]",line,sect).c_str());
            *it  = I;
		} else 
		{
			data.Data.insert(it,I);
        }
    } else {
		data.Data.insert(it,I);
    }
}
void	CInifile::w_u8			( LPCTSTR S, LPCTSTR L, u8				V, LPCTSTR comment )
{
	string128 temp; wprintf_s		(temp,sizeof(temp),"%d",V);
	w_string	(S,L,temp,comment);
}
void	CInifile::w_u16			( LPCTSTR S, LPCTSTR L, u16				V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%d",V);
	w_string	(S,L,temp,comment);
}
void	CInifile::w_u32			( LPCTSTR S, LPCTSTR L, u32				V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%d",V);
	w_string	(S,L,temp,comment);
}

void	CInifile::w_u64			( LPCTSTR S, LPCTSTR L, u64				V, LPCTSTR comment )
{
	string128 temp; 
#ifndef _EDITOR
	_ui64tow_s			(V, temp, sizeof(temp), 10);
#else
    _ui64toa			(V, temp, 10);
#endif
	w_string			(S,L,temp,comment);
}

void	CInifile::w_s64			( LPCTSTR S, LPCTSTR L, s64				V, LPCTSTR comment )
{
	string128			temp;
#ifndef _EDITOR
	_i64tow_s			(V, temp, sizeof(temp), 10);
#else
    _i64toa				(V, temp, 10);
#endif
	w_string			(S,L,temp,comment);
}

void	CInifile::w_s8			( LPCTSTR S, LPCTSTR L, s8				V, LPCTSTR comment )
{
	string128 temp; wprintf_s		(temp,sizeof(temp),"%d",V);
	w_string	(S,L,temp,comment);
}
void	CInifile::w_s16			( LPCTSTR S, LPCTSTR L, s16				V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%d",V);
	w_string	(S,L,temp,comment);
}
void	CInifile::w_s32			( LPCTSTR S, LPCTSTR L, s32				V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%d",V);
	w_string	(S,L,temp,comment);
}
void	CInifile::w_float		( LPCTSTR S, LPCTSTR L, float				V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%f",V);
	w_string	(S,L,temp,comment);
}
void	CInifile::w_fcolor		( LPCTSTR S, LPCTSTR L, const Fcolor&		V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%f,%f,%f,%f", V.r, V.g, V.b, V.a);
	w_string	(S,L,temp,comment);
}

void	CInifile::w_color		( LPCTSTR S, LPCTSTR L, u32				V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%d,%d,%d,%d", color_get_R(V), color_get_G(V), color_get_B(V), color_get_A(V));
	w_string	(S,L,temp,comment);
}

void	CInifile::w_ivector2	( LPCTSTR S, LPCTSTR L, const Ivector2&	V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%d,%d", V.x, V.y);
	w_string	(S,L,temp,comment);
}

void	CInifile::w_ivector3	( LPCTSTR S, LPCTSTR L, const Ivector3&	V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%d,%d,%d", V.x, V.y, V.z);
	w_string	(S,L,temp,comment);
}

void	CInifile::w_ivector4	( LPCTSTR S, LPCTSTR L, const Ivector4&	V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%d,%d,%d,%d", V.x, V.y, V.z, V.w);
	w_string	(S,L,temp,comment);
}
void	CInifile::w_fvector2	( LPCTSTR S, LPCTSTR L, const Fvector2&	V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%f,%f", V.x, V.y);
	w_string	(S,L,temp,comment);
}

void	CInifile::w_fvector3	( LPCTSTR S, LPCTSTR L, const Fvector3&	V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%f,%f,%f", V.x, V.y, V.z);
	w_string	(S,L,temp,comment);
}

void	CInifile::w_fvector4	( LPCTSTR S, LPCTSTR L, const Fvector4&	V, LPCTSTR comment )
{
	string128 temp; wprintf_s(temp,sizeof(temp),"%f,%f,%f,%f", V.x, V.y, V.z, V.w);
	w_string	(S,L,temp,comment);
}

void	CInifile::w_bool		( LPCTSTR S, LPCTSTR L, BOOL				V, LPCTSTR comment )
{
	w_string	(S,L,V?TEXT("on"):TEXT("off"),comment);
}

void	CInifile::remove_line	( LPCTSTR S, LPCTSTR L )
{
	R_ASSERT	(!m_flags.test(eReadOnly));

    if (line_exist(S,L)){
		Sect&	data	= r_section	(S);
		SectIt_ A = std::lower_bound(data.Data.begin(),data.Data.end(),L,item_pred);
    	R_ASSERT(A!=data.Data.end() && xr_strcmp(*A->first,L)==0);
        data.Data.erase(A);
    }
}

