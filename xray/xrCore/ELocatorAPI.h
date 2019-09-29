// LocatorAPI.h: interface for the CLocatorAPI class.
//
//////////////////////////////////////////////////////////////////////

#ifndef ELocatorAPIH
#define ELocatorAPIH
#pragma once

#include "LocatorAPI_defs.h"

class XRCORE_API CLocatorAPI  
{
	friend class FS_Path;
public:
private:
	DEFINE_MAP_PRED				(LPCTSTR,FS_Path*,PathMap,PathPairIt,pred_str);
	PathMap						pathes;
public:
	enum{
		flNeedRescan			= (1<<0),
		flBuildCopy				= (1<<1),                                
		flReady					= (1<<2),
		flEBuildCopy			= (1<<3),
//.		flEventNotificator      = (1<<4),
		flTargetFolderOnly		= (1<<5),
		flCacheFiles			= (1<<6),
		flScanAppRoot			= (1<<7),
	};    
	Flags32						m_Flags			;
	u32							dwAllocGranularity;
	u32							dwOpenCounter;
public:
								CLocatorAPI		();
								~CLocatorAPI	();
	void						_initialize		(u32 flags, LPCTSTR target_folder=0, LPCTSTR fs_fname=0);
	void						_destroy		();

	IReader*					r_open			(LPCTSTR initial, LPCTSTR N);
	IC IReader*					r_open			(LPCTSTR N){return r_open(0,N);}
	void						r_close			(IReader* &S);

	IWriter*					w_open			(LPCTSTR initial, LPCTSTR N);
	IWriter*					w_open_ex		(LPCTSTR initial, LPCTSTR N);
	IC IWriter*					w_open			(LPCTSTR N){return w_open(0,N);}
	IC IWriter*					w_open_ex		(LPCTSTR N){return w_open_ex(0,N);}
	void						w_close			(IWriter* &S);

	BOOL						exist			(LPCTSTR N);
	BOOL						exist			(LPCTSTR path, LPCTSTR name);
	BOOL						exist			(string_path& fn, LPCTSTR path, LPCTSTR name);
	BOOL						exist			(string_path& fn, LPCTSTR path, LPCTSTR name, LPCTSTR ext);

    BOOL 						can_write_to_folder	(LPCTSTR path); 
    BOOL 						can_write_to_alias	(LPCTSTR path); 
    BOOL						can_modify_file	(LPCTSTR fname);
    BOOL						can_modify_file	(LPCTSTR path, LPCTSTR name);

    BOOL 						dir_delete			(LPCTSTR initial, LPCTSTR N,BOOL remove_files);
    BOOL 						dir_delete			(LPCTSTR full_path,BOOL remove_files){return dir_delete(0,full_path,remove_files);}
    void 						file_delete			(LPCTSTR path,LPCTSTR nm);
    void 						file_delete			(LPCTSTR full_path){file_delete(0,full_path);}
	void 						file_copy			(LPCTSTR src, LPCTSTR dest);
	void 						file_rename			(LPCTSTR src, LPCTSTR dest,bool bOwerwrite=true);
    int							file_length			(LPCTSTR src);

    time_t 						get_file_age		(LPCTSTR nm);
    void 						set_file_age		(LPCTSTR nm, time_t age);

    BOOL						path_exist			(LPCTSTR path);
    FS_Path*					get_path			(LPCTSTR path);
    FS_Path*					append_path			(LPCTSTR path_alias, LPCTSTR root, LPCTSTR add, BOOL recursive);
    LPCTSTR						update_path			(string_path& dest, LPCTSTR initial, LPCTSTR src);

	BOOL						file_find			(LPCTSTR full_name, FS_File& f);

	int							file_list			(FS_FileSet& dest, LPCTSTR path, u32 flags=FS_ListFiles, LPCTSTR mask=0);
//.    void						update_path			(xr_string& dest, LPCTSTR initial, LPCTSTR src);
};

extern XRCORE_API	CLocatorAPI*					xr_FS;
#define FS (*xr_FS)

#endif // ELocatorAPIH

