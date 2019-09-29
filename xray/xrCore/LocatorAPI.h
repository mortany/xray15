// LocatorAPI.h: interface for the CLocatorAPI class.
//
//////////////////////////////////////////////////////////////////////

#ifndef LocatorAPIH
#define LocatorAPIH
#pragma once

#pragma warning(push)
#pragma warning(disable:4995)
#include <io.h>
#pragma warning(pop)

#include "LocatorAPI_defs.h"

class XRCORE_API CStreamReader;

class XRCORE_API CLocatorAPI  
{
	friend class FS_Path;
public:
	struct	file
	{
		LPCTSTR					name;			// low-case name
		u32						vfs;			// 0xffffffff - standart file
		u32						crc;			// contents CRC
		u32						ptr;			// pointer inside vfs
		u32						size_real;		// 
		u32						size_compressed;// if (size_real==size_compressed) - uncompressed
        u32						modif;			// for editor
	};
	struct	archive
	{
		shared_str				path;
		void					*hSrcFile, *hSrcMap;
		u32						size;
		CInifile*				header;
		u32						vfs_idx;
		archive():hSrcFile(NULL),hSrcMap(NULL),header(NULL),size(0),vfs_idx(u32(-1)){}
		void					open();
		void					close();
	};
    DEFINE_VECTOR				(archive,archives_vec,archives_it);
    archives_vec				m_archives;
	void						LoadArchive		(archive& A, LPCTSTR entrypoint=NULL);

private:
	struct	file_pred: public 	std::binary_function<file&, file&, bool> 
	{	
		IC bool operator()	(const file& x, const file& y) const
		{	return xr_strcmp(x.name,y.name)<0;	}
	};
	DEFINE_MAP_PRED				(LPCTSTR,FS_Path*,PathMap,PathPairIt,pred_str);
	PathMap						pathes;

	DEFINE_SET_PRED				(file,files_set,files_it,file_pred);

    int							m_iLockRescan	; 
    void						check_pathes	();

	files_set					m_files			;
	BOOL						bNoRecurse		;

	xrCriticalSection			m_auth_lock		;
	u64							m_auth_code		;

	void						Register		(LPCTSTR name, u32 vfs, u32 crc, u32 ptr, u32 size_real, u32 size_compressed, u32 modif);
	void						ProcessArchive	(LPCTSTR path);
	void						ProcessOne		(LPCTSTR path, const _wfinddata_t& F);
	bool						Recurse			(LPCTSTR path);	

	files_it					file_find_it	(LPCTSTR n);
public:
	enum{
		flNeedRescan			= (1<<0),
		flBuildCopy				= (1<<1),
		flReady					= (1<<2),
		flEBuildCopy			= (1<<3),
		flEventNotificator      = (1<<4),
		flTargetFolderOnly		= (1<<5),
		flCacheFiles			= (1<<6),
		flScanAppRoot			= (1<<7),
		flNeedCheck				= (1<<8),
		flDumpFileActivity		= (1<<9),
	};    
	Flags32						m_Flags			;
	u32							dwAllocGranularity;
	u32							dwOpenCounter;

private:
			void				check_cached_files	(LPTSTR fname, const u32 &fname_size, const file &desc, LPCTSTR &source_name);

			void				file_from_cache_impl(IReader *&R, LPTSTR fname, const file &desc);
			void				file_from_cache_impl(CStreamReader *&R, LPTSTR fname, const file &desc);
	template <typename T>
			void				file_from_cache		(T *&R, LPTSTR fname, const u32 &fname_size, const file &desc, LPCTSTR &source_name);
			
			void				file_from_archive	(IReader *&R, LPCTSTR fname, const file &desc);
			void				file_from_archive	(CStreamReader *&R, LPCTSTR fname, const file &desc);

			void				copy_file_to_build	(IWriter *W, IReader *r);
			void				copy_file_to_build	(IWriter *W, CStreamReader *r);
	template <typename T>
			void				copy_file_to_build	(T *&R, LPCTSTR source_name);

			bool				check_for_file		(LPCTSTR path, LPCTSTR _fname, string_path& fname, const file *&desc);
	
	template <typename T>
	IC		T					*r_open_impl		(LPCTSTR path, LPCTSTR _fname);

private:
			void				setup_fs_path		(LPCTSTR fs_name, string_path &fs_path);
			void				setup_fs_path		(LPCTSTR fs_name);
			IReader				*setup_fs_ltx		(LPCTSTR fs_name);

public:
								CLocatorAPI			();
								~CLocatorAPI		();
	void						_initialize			(u32 flags, LPCTSTR target_folder=0, LPCTSTR fs_name=0);
	void						_destroy			();

	CStreamReader*				rs_open				(LPCTSTR initial, LPCTSTR N);
	IReader*					r_open				(LPCTSTR initial, LPCTSTR N);
	IC IReader*					r_open				(LPCTSTR N){return r_open(0,N);}
	void						r_close				(IReader* &S);
	void						r_close				(CStreamReader* &fs);

	IWriter*					w_open				(LPCTSTR initial, LPCTSTR N);
	IC IWriter*					w_open				(LPCTSTR N){return w_open(0,N);}
	IWriter*					w_open_ex			(LPCTSTR initial, LPCTSTR N);
	IC IWriter*					w_open_ex			(LPCTSTR N){return w_open_ex(0,N);}
	void						w_close				(IWriter* &S);

	const file*					exist				(LPCTSTR N);
	const file*					exist				(LPCTSTR path, LPCTSTR name);
	const file*					exist				(string_path& fn, LPCTSTR path, LPCTSTR name);
	const file*					exist				(string_path& fn, LPCTSTR path, LPCTSTR name, LPCTSTR ext);

    BOOL 						can_write_to_folder	(LPCTSTR path); 
    BOOL 						can_write_to_alias	(LPCTSTR path); 
    BOOL						can_modify_file		(LPCTSTR fname);
    BOOL						can_modify_file		(LPCTSTR path, LPCTSTR name);

    BOOL 						dir_delete			(LPCTSTR path,LPCTSTR nm,BOOL remove_files);
    BOOL 						dir_delete			(LPCTSTR full_path,BOOL remove_files){return dir_delete(0,full_path,remove_files);}
    void 						file_delete			(LPCTSTR path,LPCTSTR nm);
    void 						file_delete			(LPCTSTR full_path){file_delete(0,full_path);}
	void 						file_copy			(LPCTSTR src, LPCTSTR dest);
	void 						file_rename			(LPCTSTR src, LPCTSTR dest,bool bOwerwrite=true);
    int							file_length			(LPCTSTR src);

    u32  						get_file_age		(LPCTSTR nm);
    void 						set_file_age		(LPCTSTR nm, u32 age);

	xr_vector<TCHAR*>*			file_list_open		(LPCTSTR initial, LPCTSTR folder,	u32 flags=FS_ListFiles);
	xr_vector<TCHAR*>*			file_list_open		(LPCTSTR path,					u32 flags=FS_ListFiles);
	void						file_list_close		(xr_vector<LPTSTR>* &lst);
                                                     
    bool						path_exist			(LPCTSTR path);
    FS_Path*					get_path			(LPCTSTR path);
    FS_Path*					append_path			(LPCTSTR path_alias, LPCTSTR root, LPCTSTR add, BOOL recursive);
    LPCTSTR						update_path			(string_path& dest, LPCTSTR initial, LPCTSTR src);

	int							file_list			(FS_FileSet& dest, LPCTSTR path, u32 flags=FS_ListFiles, LPCTSTR mask=0);

	bool						load_all_unloaded_archives();
	void						unload_archive		(archive& A);

	void						auth_generate		(xr_vector<xr_string>&	ignore, xr_vector<xr_string>&	important);
	u64							auth_get			();
	void						auth_runtime		(void*);

	void						rescan_path			(LPCTSTR full_path, BOOL bRecurse);
	// editor functions
	void						rescan_pathes		();
	void						lock_rescan			();
	void						unlock_rescan		();
};

extern XRCORE_API	CLocatorAPI*					xr_FS;
#define FS (*xr_FS)

#endif // LocatorAPIH

