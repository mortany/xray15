// LocatorAPI.cpp: implementation of the CLocatorAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable:4995)
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <sys\stat.h>
#pragma warning(default:4995)

#include "FS_internal.h"

CLocatorAPI*	xr_FS	= NULL;

#define FSLTX	TEXT("fsgame.ltx")
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CLocatorAPI::CLocatorAPI()
{
    m_Flags.zero		();
	// get page size
	SYSTEM_INFO			sys_inf;
	GetSystemInfo		(&sys_inf);
	dwAllocGranularity	= sys_inf.dwAllocationGranularity;
	dwOpenCounter		= 0;
}

CLocatorAPI::~CLocatorAPI()
{
}

void CLocatorAPI::_initialize	(u32 flags, LPCTSTR target_folder, LPCTSTR fs_fname)
{
	TCHAR _delimiter = '|'; //','
	if (m_Flags.is(flReady))return;

	Log				(TEXT("Initializing File System..."));
	m_Flags.set		(flags,TRUE);

	append_path	(TEXT("$fs_root$"), TEXT(""), 0, FALSE);

	// append application path

	Log(TEXT("Try to open file:"), fs_fname);

	if (m_Flags.is(flScanAppRoot)){
		append_path		(TEXT("$app_root$"),Core.ApplicationPath,0,FALSE);
    }
	if (m_Flags.is(flTargetFolderOnly)){
		append_path		(TEXT("$target_folder$"),target_folder,0,TRUE);
	}else{
		IReader* F		= r_open((fs_fname&&fs_fname[0])?fs_fname:FSLTX); 
		if (!F&&m_Flags.is(flScanAppRoot))
			F			= r_open(TEXT("$app_root$"),(fs_fname&&fs_fname[0])?fs_fname:FSLTX);
		R_ASSERT3		(F,"Can't open file:", (fs_fname&&fs_fname[0])?fs_fname:FSLTX);

		if (!F)
			F = r_open(Core.ApplicationPath, (fs_fname && fs_fname[0]) ? fs_fname : FSLTX);

		// append all pathes    
		string_path			buf;
		string_path		id, temp, root, add, def, capt;
		LPCTSTR			lp_add, lp_def, lp_capt;
		string16		b_v;
		while(!F->eof())
		{
			F->r_string	(buf,sizeof(buf));
			_GetItem(buf,0,id,'=');
			if (id[0]==';') continue;
			_GetItem(buf,1,temp,'=');
			int cnt		= _GetItemCount(temp,_delimiter);  R_ASSERT(cnt>=3);

			if (!(cnt >= 3))
			{
				Log(TEXT("Error in parsing path:"), buf);
			}

			u32 fl		= 0;
			_GetItem	(temp,0,b_v,_delimiter);	if (CInifile::IsBOOL(b_v)) fl |= FS_Path::flRecurse;
			_GetItem	(temp,1,b_v,_delimiter);	if (CInifile::IsBOOL(b_v)) fl |= FS_Path::flNotif;
			_GetItem	(temp,2,root,_delimiter);
			_GetItem	(temp,3,add,_delimiter);
			_GetItem	(temp,4,def,_delimiter);
			_GetItem	(temp,5,capt,_delimiter);
			xr_strlwr	(id);			if (!m_Flags.is(flBuildCopy)&&(0==xr_strcmp(id,TEXT("$build_copy$")))) continue;
			xr_strlwr	(root);
			lp_add		=(cnt>=4)?xr_strlwr(add):0;
			lp_def		=(cnt>=5)?def:0;
			lp_capt		=(cnt>=6)?capt:0;
			PathPairIt p_it = pathes.find(root);
			std::pair<PathPairIt, bool> I;
			FS_Path* P	= xr_new<FS_Path>((p_it!=pathes.end())?p_it->second->m_Path:root,lp_add,lp_def,lp_capt,fl);
			I			= pathes.insert(mk_pair(xr_strdup(id),P));
			
			R_ASSERT	(I.second);
		}
		r_close			(F);
	};

	m_Flags.set		(flReady,TRUE);

	CreateLog		(0!=wcsstr(Core.Params,TEXT("-nolog")));
}

void CLocatorAPI::_destroy		()
{
	CloseLog		();

	for(PathPairIt p_it=pathes.begin(); p_it!=pathes.end(); p_it++){
		TCHAR* str	= LPTSTR(p_it->first);
		xr_free		(str);
		xr_delete	(p_it->second);
    }
	pathes.clear	();
}

BOOL CLocatorAPI::file_find(LPCTSTR full_name, FS_File& f)
{
    intptr_t		hFile;
    _FINDDATA_T		sFile;
	// find all files    
	if (-1!=(hFile=_wfindfirst(full_name, &sFile))){
    	f			= FS_File(sFile);
        _findclose	(hFile);
        return		TRUE;
    }else{
    	return		FALSE;
    }
}

BOOL CLocatorAPI::exist			(LPCTSTR fn)
{
	return ::GetFileAttributes(fn) != u32(-1);
}

BOOL CLocatorAPI::exist			(LPCTSTR path, LPCTSTR name)
{
	string_path		temp;       
    update_path		(temp,path,name);
	return exist	(temp);
}

BOOL CLocatorAPI::exist			(string_path& fn, LPCTSTR path, LPCTSTR name)
{
    update_path		(fn,path,name);
	return exist	(fn);
}

BOOL CLocatorAPI::exist			(string_path& fn, LPCTSTR path, LPCTSTR name, LPCTSTR ext)
{
	string_path 	nm;
	strconcat		(sizeof(nm),nm,name,ext);
    update_path		(fn,path,nm);
	return exist	(fn);
}

bool ignore_name(LPCTSTR _name)
{
	// ignore processing ".svn" folders
	return ( _name[0]=='.' && _name[1]=='s' && _name[2]=='v' && _name[3]=='n' && _name[4]==0) ;
}

typedef void	(__stdcall * TOnFind)	(_wfinddata_t&, void*);
void Recurse	(LPCTSTR, bool, TOnFind, void*);
void ProcessOne	(LPCTSTR path, _wfinddata_t& F, bool root_only, TOnFind on_find_cb, void* data)
{
	string_path	N;
	wcscpy		(N,path);
	wcscat		(N,F.name);
	xr_strlwr	(N);

	if (ignore_name(N))					return;
    
	if (F.attrib&_A_HIDDEN)				return;

	if (F.attrib&_A_SUBDIR) {
    	if (root_only)					return;
		if (0==xr_strcmp(F.name,TEXT(".")))	return;
		if (0==xr_strcmp(F.name,TEXT(".."))) 	return;
		wcscat		(N,TEXT("\\"));
	    wcscpy		(F.name,N);
        on_find_cb	(F,data);
		Recurse		(F.name,root_only,on_find_cb,data);
	} else {
	    wcscpy		(F.name,N);
        on_find_cb	(F,data);
	}
}

void Recurse(LPCTSTR path, bool root_only, TOnFind on_find_cb, void* data)
{
	xr_string		fpath	= path;
	fpath			+= TEXT("*.*");

    // begin search
    _wfinddata_t		sFile;
    intptr_t		hFile;

	// find all files    
	if (-1==(hFile=_wfindfirst(fpath.c_str(), &sFile)))
    	return;
    do{
    	ProcessOne	(path,sFile, root_only, on_find_cb, data);
    }while(_wfindnext(hFile,&sFile)==0);
	_findclose		( hFile );
}

struct file_list_cb_data
{
	size_t 		base_len;
    u32 		flags;
    SStringVec* masks;
    FS_FileSet* dest;
};

void __stdcall file_list_cb(_wfinddata_t& entry, void* data)
{
	file_list_cb_data*	D		= (file_list_cb_data*)data;

    LPCTSTR end_symbol 			= entry.name+xr_strlen(entry.name)-1;
    if ((*end_symbol)!='\\'){
        // file
        if ((D->flags&FS_ListFiles) == 0)	return;
        LPCTSTR entry_begin 		= entry.name+D->base_len;
        if ((D->flags&FS_RootOnly)&&wcsstr(entry_begin,TEXT("\\")))	return;	// folder in folder
        // check extension
        if (D->masks){
            bool bOK			= false;
            for (SStringVecIt it=D->masks->begin(); it!=D->masks->end(); it++)
                if (PatternMatch(entry_begin,it->c_str())){bOK=true; break;}
            if (!bOK)			return;
        }
        xr_string fn			= entry_begin;
        // insert file entry
        if (D->flags&FS_ClampExt) fn=EFS.ChangeFileExt(fn,TEXT(""));
        D->dest->insert			(FS_File(fn,entry));
    } else {
        // folder
        if ((D->flags&FS_ListFolders) == 0)	return;
        LPCTSTR entry_begin 		= entry.name+D->base_len;
        D->dest->insert			(FS_File(entry_begin,entry));
    }
}

int CLocatorAPI::file_list(FS_FileSet& dest, LPCTSTR path, u32 flags, LPCTSTR mask)
{
	R_ASSERT		(path);
	VERIFY			(flags);
               
	string_path		fpath;
	if (path_exist(path))
    	update_path	(fpath,path,TEXT(""));
    else
    	wcscpy(fpath,path);

    // build mask
	SStringVec 		masks;
	_SequenceToList	(masks,mask);

    file_list_cb_data data;
    data.base_len	= xr_strlen(fpath);
    data.flags		= flags;
    data.masks		= masks.empty()?0:&masks;
    data.dest		= &dest;

    Recurse			(fpath,!!(flags&FS_RootOnly),file_list_cb,&data);
	return dest.size();
}

IReader* CLocatorAPI::r_open	(LPCTSTR path, LPCTSTR _fname)
{
	IReader* R		= 0;

	// correct path
	string_path		fname;
	wcscpy			(fname,_fname);
	xr_strlwr		(fname);
	if (path && path[0])
	{
		update_path(fname, path, fname);
	}
	else
	{
		Log(TEXT("IReader : can't parsing path "), path);
	}
	// Search entry
    FS_File			desc;
    if (!file_find(fname,desc)) return NULL;

	dwOpenCounter	++;

	LPCTSTR	source_name 	= &fname[0];

	Log(TEXT("IReader : try open file "), fname);
	Log(TEXT("IReader : path "), path);

    // open file
    if (desc.size<256*1024)	R = xr_new<CFileReader>			(fname);
    else			  		R = xr_new<CVirtualFileReader>	(fname);
    
	if ( R && m_Flags.is(flBuildCopy|flReady) ){
		string_path	cpy_name;
		string_path	e_cpy_name;
		FS_Path* 	P; 
		if (source_name==wcsstr(source_name,(P=get_path(TEXT("$server_root$")))->m_Path)||
        	source_name==wcsstr(source_name,(P=get_path(TEXT("$server_data_root$")))->m_Path)){
			update_path			(cpy_name,TEXT("$build_copy$"),source_name+xr_strlen(P->m_Path));
			IWriter* W = w_open	(cpy_name);
            if (W){
                W->w				(R->pointer(),R->length());
                w_close				(W);
                set_file_age(cpy_name,get_file_age(source_name));
                if (m_Flags.is(flEBuildCopy)){
                    LPCTSTR ext		= strext(cpy_name);
                    if (ext){
                        IReader* R		= 0;
                        if (0==xr_strcmp(ext,TEXT(".dds"))){
                            P			= get_path(TEXT("$game_textures$"));               
                            update_path	(e_cpy_name,TEXT("$textures$"),source_name+xr_strlen(P->m_Path));
                            // tga
                            *strext		(e_cpy_name) = 0;
                            wcscat		(e_cpy_name,TEXT(".tga"));
                            r_close		(R=r_open(e_cpy_name));
                            // thm
                            *strext		(e_cpy_name) = 0;
                            wcscat		(e_cpy_name,TEXT(".thm"));
                            r_close		(R=r_open(e_cpy_name));
                        }else if (0==xr_strcmp(ext,TEXT(".ogg"))){
                            P			= get_path(TEXT("$game_sounds$"));                               
                            update_path	(e_cpy_name,TEXT("$sounds$"),source_name+xr_strlen(P->m_Path));
                            // wav
                            *strext		(e_cpy_name) = 0;
                            wcscat		(e_cpy_name,TEXT(".wav"));
                            r_close		(R=r_open(e_cpy_name));
                            // thm
                            *strext		(e_cpy_name) = 0;
                            wcscat		(e_cpy_name,TEXT(".thm"));
                            r_close		(R=r_open(e_cpy_name));
                        }else if (0==xr_strcmp(ext,TEXT(".object"))){
                            wcscpy		(e_cpy_name,source_name);
                            // object thm
                            *strext		(e_cpy_name) = 0;
                            wcscat		(e_cpy_name,TEXT(".thm"));
                            R			= r_open(e_cpy_name);
                            if (R)		r_close	(R);
                        }
                    }
                }
            }else{
            	Log			(TEXT("!Can't build:"),source_name);
            }
		}
	}
	return R;
}

void	CLocatorAPI::r_close	(IReader* &fs)
{
	xr_delete	(fs);
}

IWriter* CLocatorAPI::w_open	(LPCTSTR path, LPCTSTR _fname)
{
	string_path	fname;
	xr_strlwr(wcscpy(fname,_fname));//,".$");
	if (path&&path[0]) update_path(fname,path,fname);
    CFileWriter* W 	= xr_new<CFileWriter>(fname,false); 
	return W;
}

IWriter* CLocatorAPI::w_open_ex	(LPCTSTR path, LPCTSTR _fname)
{
	string_path	fname;
	xr_strlwr(wcscpy(fname,_fname));//,".$");
	if (path&&path[0]) update_path(fname,path,fname);
    CFileWriter* W 	= xr_new<CFileWriter>(fname,true); 
	return W;
}

void	CLocatorAPI::w_close(IWriter* &S)
{
	if (S){
        R_ASSERT	(S->fName.size());
        xr_delete	(S);
    }
}

struct dir_delete_cb_data
{
	FS_FileSet*		folders;
    BOOL			remove_files;
};

void __stdcall dir_delete_cb(_wfinddata_t& entry, void* data)
{
	dir_delete_cb_data*	D		= (dir_delete_cb_data*)data;

    if (entry.attrib&_A_SUBDIR)	D->folders->insert	(FS_File(entry));
    else if (D->remove_files)	FS.file_delete		((LPCTSTR)entry.name);
}

BOOL CLocatorAPI::dir_delete(LPCTSTR initial, LPCTSTR nm, BOOL remove_files)
{
	string_path			fpath;
	if (initial&&initial[0])
    	update_path	(fpath,initial,nm);
    else
    	wcscpy		(fpath,nm);

    FS_FileSet 			folders;
    folders.insert		(FS_File(fpath));

    // recurse find
    dir_delete_cb_data	data;
	data.folders		= &folders;
    data.remove_files	= remove_files;
    Recurse				(fpath,false,dir_delete_cb,&data);

    // remove folders
    FS_FileSet::reverse_iterator r_it = folders.rbegin();
    for (;r_it!=folders.rend();r_it++)
		_wrmdir			(r_it->name.c_str());
    return TRUE;
}                                                

void CLocatorAPI::file_delete(LPCTSTR path, LPCTSTR nm)
{
	string_path	fname;
	if (path&&path[0])
    	update_path	(fname,path,nm);
    else
    	wcscpy		(fname, nm);
    _wunlink			(fname);
}

void CLocatorAPI::file_copy(LPCTSTR src, LPCTSTR dest)
{
	if (exist(src)){
        IReader* S		= r_open(src);
        if (S){
            IWriter* D	= w_open(dest);
            if (D){
                D->w	(S->pointer(),S->length());
                w_close	(D);
            }
            r_close		(S);
        }
	}
}

void CLocatorAPI::file_rename(LPCTSTR src, LPCTSTR dest, bool bOwerwrite)
{
	if (bOwerwrite&&exist(dest)) _wunlink(dest);
    // physically rename file
    VerifyPath			(dest);
    _wrename				(src,dest);
}

int	CLocatorAPI::file_length(LPCTSTR src)
{
	FS_File F;
    return (file_find(src,F))?F.size:-1;
}

BOOL CLocatorAPI::path_exist(LPCTSTR path)
{
    PathPairIt P 			= pathes.find((TCHAR*)path);
    return					(P!=pathes.end());
}

FS_Path* CLocatorAPI::append_path(LPCTSTR path_alias, LPCTSTR root, LPCTSTR add, BOOL recursive)
{
	VERIFY			(root/*&&root[0]*/);
	VERIFY			(false==path_exist(path_alias));
	FS_Path* P		= xr_new<FS_Path>(xr_strdup(root),add,LPCTSTR(0),LPCTSTR(0),0);

	TCHAR* buff = xr_strdup(path_alias);

	pathes.insert	(mk_pair(buff,P));
	return P;
}

FS_Path* CLocatorAPI::get_path(LPCTSTR path)
{
    PathPairIt P 			= pathes.find((TCHAR*)path); 
    //R_ASSERT2(P!=pathes.end(),path);
    return P->second;
}

LPCTSTR CLocatorAPI::update_path(string_path& dest, LPCTSTR initial, LPCTSTR src)
{
	if(get_path(initial)) return get_path(initial)->_update(dest, src);
}
/*
void CLocatorAPI::update_path(xr_string& dest, LPCTSTR initial, LPCTSTR src)
{
    return get_path(initial)->_update(dest,src);
} */

time_t CLocatorAPI::get_file_age(LPCTSTR nm)
{
	FS_File F;
    return (file_find(nm,F))?F.time_write:-1;
}

void CLocatorAPI::set_file_age(LPCTSTR nm, time_t age)
{
    // set file
    _utimbuf	tm;
    tm.actime	= age;
    tm.modtime	= age;
    int res 	= _wutime(nm,&tm);
    if (0!=res)	Msg(TEXT("!Can't set file age: '%s'. Error: '%s'"),nm,_sys_errlist[errno]);
}

BOOL CLocatorAPI::can_write_to_folder(LPCTSTR path)
{
	if (path&&path[0]){
		string_path		temp;       
        LPCTSTR fn		= TEXT("$!#%TEMP%#!$.$$$");
	    strconcat		(sizeof(temp),temp,path,path[xr_strlen(path)-1]!='\\'?TEXT("\\"):TEXT(""),fn);
		FILE* hf		= _wfopen	(temp, TEXT("wb"));
		if (hf==0)		return FALSE;
        else{
        	fclose 		(hf);
	    	_wunlink		(temp);
            return 		TRUE;
        }
    }else{
    	return 			FALSE;
    }
}

BOOL CLocatorAPI::can_write_to_alias(LPCTSTR path)
{
	string_path			temp;       
    update_path			(temp,path,TEXT(""));
	return can_write_to_folder(temp);
}

BOOL CLocatorAPI::can_modify_file(LPCTSTR fname)
{
	FILE* hf			= _wfopen	(fname, TEXT("r+b"));
    if (hf){	
    	fclose			(hf);
        return 			TRUE;
    }else{
    	return 			FALSE;
    }
}

BOOL CLocatorAPI::can_modify_file(LPCTSTR path, LPCTSTR name)
{
	string_path			temp;       
    update_path			(temp,path,name);
	return can_modify_file(temp);
}
