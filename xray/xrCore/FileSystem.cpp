//----------------------------------------------------
// file: FileSystem.cpp
//----------------------------------------------------

#include "stdafx.h"


#include "cderr.h"
#include "commdlg.h"

EFS_Utils*	xr_EFS	= NULL;
//----------------------------------------------------
EFS_Utils::EFS_Utils( )
{
}

EFS_Utils::~EFS_Utils()
{
}

xr_string	EFS_Utils::ExtractFileName(LPCTSTR src)
{
	string_path name;
	_wsplitpath	(src,0,0,name,0);
    return xr_string(name);
}

xr_string	EFS_Utils::ExtractFileExt(LPCTSTR src)
{
	string_path ext;
	_wsplitpath	(src,0,0,0,ext);
    return xr_string(ext);
}

xr_string	EFS_Utils::ExtractFilePath(LPCTSTR src)
{
	string_path drive,dir;
	_wsplitpath	(src,drive,dir,0,0);
    return xr_string(drive)+dir;
}

xr_string	EFS_Utils::ExcludeBasePath(LPCTSTR full_path, LPCTSTR excl_path)
{
    LPCTSTR sub		= wcsstr(full_path,excl_path);
	if (0!=sub) 	return xr_string(sub+xr_strlen(excl_path));
	else	   		return xr_string(full_path);
}

xr_string	EFS_Utils::ChangeFileExt(TCHAR* src, TCHAR* ext)
{
	xr_string	tmp;
	LPTSTR src_ext	= strext(src);
    if (src_ext){
	    size_t		ext_pos	= src_ext-src;
        tmp.assign	(src,0,ext_pos);
    }else{
        tmp			= src;
    }
    tmp				+= ext;
    return tmp;
}

xr_string	EFS_Utils::ChangeFileExt(const xr_string& src, LPCTSTR ext)
{
	return ChangeFileExt(src.c_str(),ext);
}

//----------------------------------------------------
LPCTSTR MakeFilter(string1024& dest, LPCTSTR info, LPCTSTR ext)
{
	ZeroMemory(dest,sizeof(dest));
    if (ext){
        int icnt=_GetItemCount(ext,';');
		LPTSTR dst=dest;
        if (icnt>1)
		{
            strconcat		(sizeof(dest),dst,info,TEXT(" ("),ext,TEXT(")"));
            dst				+= (xr_strlen(dst)+1);
            wcscpy_s			(dst, sizeof(dest), ext);
            dst				+= (xr_strlen(ext)+1);
        }
        for (int i=0; i<icnt; i++)
		{
            string64		buf;
            _GetItem		(ext,i,buf,';');
            strconcat		(sizeof(dest), dst,info,TEXT(" ("),buf,TEXT(")"));
            dst				+= (xr_strlen(dst)+1);
            wcscpy_s			(dst, sizeof(dest) - (dst - dest), buf);
            dst				+= (xr_strlen(buf)+1);
        }
    }
	return dest;
}

//------------------------------------------------------------------------------
// start_flt_ext = -1-all 0..n-indices
//------------------------------------------------------------------------------
bool EFS_Utils::GetOpenNameInternal( LPCTSTR initial,  LPTSTR buffer, int sz_buf, bool bMulti, LPCTSTR offset, int start_flt_ext )
{
	VERIFY				(buffer&&(sz_buf>0));
	FS_Path& P			= *FS.get_path(initial);
	string1024 			flt;
	MakeFilter			(flt,P.m_FilterCaption?P.m_FilterCaption:TEXT(""),P.m_DefExt);

	OPENFILENAME 		ofn;
	Memory.mem_fill		( &ofn, 0, sizeof(ofn) );

    if (xr_strlen(buffer))
    {
        string_path		dr;
        if (!(buffer[0]=='\\' && buffer[1]=='\\')){ // if !network
            _wsplitpath		(buffer,dr,0,0,0);

            if (0==dr[0])
            {
                string_path		bb;
            	P._update		(bb, buffer);
                wcscpy_s		(buffer, sz_buf, bb);
             }
        }
    }
    ofn.lStructSize		= sizeof(OPENFILENAME);
	ofn.hwndOwner 		= GetForegroundWindow();
	ofn.lpstrDefExt 	= P.m_DefExt;
	ofn.lpstrFile 		= buffer;
	ofn.nMaxFile 		= sz_buf;
	ofn.lpstrFilter 	= flt;
	ofn.nFilterIndex 	= start_flt_ext+2;
    ofn.lpstrTitle      = TEXT("Open a File");
    string512 path; 
	wcscpy_s				(path,(offset&&offset[0])?offset:P.m_Path);
	ofn.lpstrInitialDir = path;
	ofn.Flags =         OFN_PATHMUSTEXIST	|
                        OFN_FILEMUSTEXIST	|
                        OFN_HIDEREADONLY	|
                        OFN_FILEMUSTEXIST	|
                        OFN_NOCHANGEDIR		|
                        (bMulti?OFN_ALLOWMULTISELECT|OFN_EXPLORER:0);
                        
    ofn.FlagsEx			= OFN_EX_NOPLACESBAR;
    
	bool bRes 			= !!GetOpenFileName( &ofn );
    if (!bRes)
    {
	    u32 err 		= CommDlgExtendedError();
	    switch(err)
        {
        	case FNERR_BUFFERTOOSMALL:
            	Log(TEXT("Too many files selected."));
            break;
        }
	}
    if (bRes && bMulti)
    {
    	Log				(TEXT("buff="),buffer);
		int cnt			= _GetItemCount(buffer,0x0);
        if (cnt>1)
        {
//.            string64  	buf;
//.            string64  	dir;
//.			string4096		buf;
            TCHAR 		dir	  [255*255];
			TCHAR 		buf	  [255*255];
			TCHAR 		fns	  [255*255];

            wcscpy_s		(dir, buffer);
            wcscpy_s		(fns, dir);
            wcscat		(fns, TEXT("\\"));
            wcscat		(fns, _GetItem	(buffer,1,buf,0x0));

            for (int i=2; i<cnt; i++)
            {
                wcscat	(fns,TEXT(","));
                wcscat	(fns,dir);
                wcscat	(fns,TEXT("\\"));
				wcscat	(fns,_GetItem(buffer,i,buf,0x0));
            }
            wcscpy_s		(buffer, sz_buf, fns);
        }
    }
    xr_strlwr				(buffer);
    return 				bRes;
}

bool EFS_Utils::GetSaveName( LPCTSTR initial, string_path& buffer, LPCTSTR offset, int start_flt_ext )
{
	FS_Path& P			= *FS.get_path(initial);
	string1024 flt;
	MakeFilter(flt,P.m_FilterCaption?P.m_FilterCaption:TEXT(""),P.m_DefExt);
	OPENFILENAME ofn;
	Memory.mem_fill		( &ofn, 0, sizeof(ofn) );
    if (xr_strlen(buffer)){ 
        string_path		dr;
        if (!(buffer[0]=='\\' && buffer[1]=='\\')){ // if !network
            _wsplitpath		(buffer,dr,0,0,0);
            if (0==dr[0])	P._update(buffer,buffer); 
        }
    }
	ofn.hwndOwner 		= GetForegroundWindow();
	ofn.lpstrDefExt 	= P.m_DefExt;
	ofn.lpstrFile 		= buffer;
	ofn.lpstrFilter 	= flt;
	ofn.lStructSize 	= sizeof(ofn);
	ofn.nMaxFile 		= sizeof(buffer);
	ofn.nFilterIndex 	= start_flt_ext+2;
    ofn.lpstrTitle      = TEXT("Save a File");
    string512 path;		wcscpy_s(path,(offset&&offset[0])?offset:P.m_Path);
	ofn.lpstrInitialDir = path;
	ofn.Flags 			= OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR;
    ofn.FlagsEx			= OFN_EX_NOPLACESBAR;

	bool bRes = !!GetSaveFileName( &ofn );
    if (!bRes){
	    u32 err = CommDlgExtendedError();
	    switch(err){
        case FNERR_BUFFERTOOSMALL: 	Log(TEXT("Too many file selected.")); break;
        }
	}
    xr_strlwr(buffer);
	return bRes;
}
//----------------------------------------------------
LPCTSTR EFS_Utils::AppendFolderToName(LPTSTR tex_name, int depth, BOOL full_name)
{
	string256 _fn;
	wcscpy(tex_name,AppendFolderToName(tex_name, _fn, depth, full_name));
	return tex_name;
}

LPCTSTR EFS_Utils::AppendFolderToName(LPCTSTR src_name, LPTSTR dest_name, int depth, BOOL full_name)
{
	shared_str tmp = src_name;
    LPCTSTR s 	= src_name;
    LPTSTR d 	= dest_name;
    int sv_depth= depth;
	for (; *s&&depth; s++, d++){
		if (*s=='_'){depth--; *d='\\';}else{*d=*s;}
	}
	if (full_name){
		*d			= 0;
		if (depth<sv_depth)	wcscat(dest_name,*tmp);
	}else{
		for (; *s; s++, d++) *d=*s;
		*d			= 0;
	}
    return dest_name;
}

LPCTSTR EFS_Utils::GenerateName(LPCTSTR base_path, LPCTSTR base_name, LPCTSTR def_ext, LPTSTR out_name)
{
    int cnt = 0;
	string_path fn;
    if (base_name)	
		strconcat		(sizeof(fn), fn, base_path,base_name,def_ext);
	else 			
		wprintf_s		(fn, sizeof(fn), "%s%02d%s",base_path,cnt++,def_ext);

	while (FS.exist(fn))
	    if (base_name)	
			wprintf_s	(fn, sizeof(fn),"%s%s%02d%s",base_path,base_name,cnt++,def_ext);
        else 			
			wprintf_s	(fn, sizeof(fn), "%s%02d%s",base_path,cnt++,def_ext);
    wcscpy(out_name,fn);
	return out_name;
}

//#endif
