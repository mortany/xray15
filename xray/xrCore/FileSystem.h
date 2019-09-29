//----------------------------------------------------
// file: FileSystem.h
//----------------------------------------------------

#ifndef FileSystemH
#define FileSystemH

#define BACKUP_FILE_LEVEL 5

class XRCORE_API EFS_Utils {
	DEFINE_MAP	(xr_string,void*,HANDLEMap,HANDLEPairIt);

    HANDLEMap 	m_LockFiles;
protected:
	bool 		GetOpenNameInternal		(LPCTSTR initial, LPTSTR buffer, int sz_buf, bool bMulti=false, LPCTSTR offset=0, int start_flt_ext=-1 );
public:
				EFS_Utils		();
	virtual 	~EFS_Utils		();
	void 		_initialize		(){}
    void 		_destroy		(){}

	LPCTSTR		GenerateName	(LPCTSTR base_path, LPCTSTR base_name, LPCTSTR def_ext, LPTSTR out_name);

	bool 		GetOpenName		(LPCTSTR initial, string_path& buffer, int sz_buf, bool bMulti=false, LPCTSTR offset=0, int start_flt_ext=-1 );
	bool 		GetOpenName		(LPCTSTR initial, xr_string& buf, bool bMulti=false, LPCTSTR offset=0, int start_flt_ext=-1 );

	bool 		GetSaveName		(LPCTSTR initial, string_path& buffer, LPCTSTR offset=0, int start_flt_ext=-1 );
	bool 		GetSaveName		(LPCTSTR initial, xr_string& buf, LPCTSTR offset=0, int start_flt_ext=-1 );

	void 		MarkFile		(TCHAR* fn, bool bDeleteSource);

	xr_string 	AppendFolderToName(xr_string& tex_name, int depth, BOOL full_name);

	LPCTSTR		AppendFolderToName(LPTSTR tex_name, int depth, BOOL full_name);
	LPCTSTR		AppendFolderToName(LPCTSTR src_name, LPTSTR dest_name, int depth, BOOL full_name);

	BOOL		LockFile		(LPCTSTR fn, bool bLog=true);
	BOOL		UnlockFile		(LPCTSTR fn, bool bLog=true);
	BOOL		CheckLocking	(LPCTSTR fn, bool bOnlySelf, bool bMsg);

    xr_string	ChangeFileExt	(TCHAR* src, TCHAR* ext);
    xr_string	ChangeFileExt	(const xr_string& src, LPCTSTR ext);

    xr_string	ExtractFileName		(LPCTSTR src);
    xr_string	ExtractFilePath		(LPCTSTR src);
    xr_string	ExtractFileExt		(LPCTSTR src);
    xr_string	ExcludeBasePath		(LPCTSTR full_path, LPCTSTR excl_path);
};
extern XRCORE_API	EFS_Utils*	xr_EFS;
#define EFS (*xr_EFS)

#endif /*_INCDEF_FileSystem_H_*/

