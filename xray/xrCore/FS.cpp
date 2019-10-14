#include "stdafx.h"


#include "fs_internal.h"

#pragma warning(disable:4995)
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <sys\stat.h>
#pragma warning(default:4995)

//typedef void DUMMY_STUFF (const void*,const u32&,void*);
//XRCORE_API DUMMY_STUFF	*g_dummy_stuff = 0;


IC TCHAR ToUTF16(char c_)
{
	TCHAR res[2];
	MultiByteToWideChar(CP_UTF8, 0, &c_, 2, res, 2);
	return res[0];
}

#include <codecvt>

// Convert an UTF8 string to a wide Unicode String
TCHAR* wstring_convert_from_char(const char* str)
{
	int wsize = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0);
	LPWSTR wbuf = (LPWSTR)xr_malloc(wsize * sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, str, -1, wbuf, wsize);
	return wbuf;
}

void VerifyPath(LPCTSTR path)
{
	string1024 tmp;
	for(int i=0;path[i];i++)
	{
		if( path[i]!='\\' || i==0 )
			continue;

		CopyMemoryW( tmp, path, i );
		tmp[i] = 0;
        _wmkdir(tmp);
	}
}

static errno_t open_internal(LPCTSTR fn, int &handle)
{
	return				(
		_wsopen_s(
			&handle,
			fn,
			_O_RDONLY | _O_BINARY,
			_SH_DENYNO, 
            _S_IREAD
		)
	);
}

bool file_handle_internal	(LPCTSTR file_name, u32 &size, int &file_handle)
{
	if (open_internal(file_name, file_handle)) {
		Sleep			(1);
		if (open_internal(file_name, file_handle))
			return		(false);
	}
	
	size				= _filelength(file_handle);
	return				(true);
}

void *FileDownload		(LPCTSTR file_name, const int &file_handle, u32 &file_size)
{
	void				*buffer = Memory.mem_alloc(file_size);

	int					r_bytes	= read(file_handle,buffer,file_size);
	R_ASSERT3			(file_size == (u32)r_bytes,	"can't read from file : ",file_name	);

	R_ASSERT3(!_close(file_handle),"can't close file : ",file_name);

	return				(buffer);
}

void *FileDownload		(LPCTSTR file_name, u32 *buffer_size)
{
	int					file_handle;
	R_ASSERT3			(
		file_handle_internal(file_name, *buffer_size, file_handle),
		"can't open file : ",
		file_name
	);

	return				(FileDownload(file_name, file_handle, *buffer_size));
}

typedef TCHAR MARK[9];
IC void mk_mark(MARK& M, const TCHAR* S)
{	wcsncpy(M,S,8); }

//void  FileCompress	(const TCHAR *fn, const TCHAR * sign, void* data, u32 size)
//{
//	MARK M; mk_mark(M,sign);
//
//	int H	= _wopen(fn,O_BINARY|O_CREAT|O_WRONLY|O_TRUNC,S_IREAD|S_IWRITE);
//	//R_ASSERT2(H>0,fn);
//	_write	(H,&M,8);
//	_writeLZ(H,data,size);
//	_close	(H);
//}

//void*  FileDecompress	(const TCHAR*fn, const TCHAR* sign, u32* size)
//{
//	MARK M,F; mk_mark(M,sign);
//
//	int	H = _wopen	(fn,O_BINARY|O_RDONLY);
//	//R_ASSERT2(H>0,fn);
//	_read	(H,&F,8);
//	if (wcsncmp(M,F,8)!=0)		{
//		F[8]=0;		Msg(TEXT("FATAL: signatures doesn't match, file(%s) / requested(%s)"),F,sign);
//	}
//    //R_ASSERT(strncmp(M,F,8)==0);
//
//	void* ptr = 0; u32 SZ;
//	SZ = _readLZ (H, ptr, filelength(H)-8);
//	_close	(H);
//	if (size) *size = SZ;
//	return ptr;
//}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//---------------------------------------------------
// memory
CMemoryWriter::~CMemoryWriter() 
{	xr_free(data);	}

void CMemoryWriter::w	(const void* ptr, u32 count)
{
	if (position+count > mem_size) {
		// reallocate
		if (mem_size==0)	mem_size=128;
		while (mem_size <= (position+count)) mem_size*=2;
		if (0==data)		data = (BYTE*)	Memory.mem_alloc	(mem_size
#ifdef DEBUG_MEMORY_NAME
			,		"CMemoryWriter - storage"
#endif // DEBUG_MEMORY_NAME
			);
		else				data = (BYTE*)	Memory.mem_realloc	(data,mem_size
#ifdef DEBUG_MEMORY_NAME
			,	"CMemoryWriter - storage"
#endif // DEBUG_MEMORY_NAME
			);
	}
	CopyMemoryW(data+position,ptr,count);
	position		+=count;
	if (position>file_size) file_size=position;
}

//static const u32 mb_sz = 0x1000000;
bool CMemoryWriter::save_to	(LPCTSTR fn)
{
	IWriter* F 		= FS.w_open(fn);
    if (F){
	    F->w		(pointer(),size());
    	FS.w_close	(F);
        return 		true;
    }
    return false;
}


void	IWriter::open_chunk	(u32 type)
{
	w_u32(type);
	chunk_pos.push(tell());
	w_u32(0);	// the place for 'size'
}
void	IWriter::close_chunk	()
{
	VERIFY(!chunk_pos.empty());

	int pos			= tell();
	seek			(chunk_pos.top());
	w_u32			(pos-chunk_pos.top()-4);
	seek			(pos);
	chunk_pos.pop	();
}
u32	IWriter::chunk_size	()					// returns size of currently opened chunk, 0 otherwise
{
	if (chunk_pos.empty())	return 0;
	return tell() - chunk_pos.top()-4;
}

void	IWriter::w_compressed(void* ptr, u32 count)
{
	BYTE*		dest	= 0;
	unsigned	dest_sz	= 0;
	_compressLZ	(&dest,&dest_sz,ptr,count);
	
//	if (g_dummy_stuff)
//		g_dummy_stuff	(dest,dest_sz,dest);

	if (dest && dest_sz)
		w(dest,dest_sz);
	xr_free		(dest);
}

void	IWriter::w_chunk(u32 type, void* data, u32 size)
{
	open_chunk	(type);
	if (type & CFS_CompressMark)	w_compressed(data,size);
	else							w			(data,size);
	close_chunk	();
}
void 	IWriter::w_sdir	(const Fvector& D) 
{
	Fvector C;
	float mag		= D.magnitude();
	if (mag>EPS_S)	{
		C.div		(D,mag);
	} else {
		C.set		(0,0,1);
		mag			= 0;
	}
	w_dir	(C);
	w_float (mag);
}
void	IWriter::w_printf(const TCHAR* format, ...)
{
	va_list mark;
	TCHAR buf[1024];

	va_start( mark , format );
		vswprintf_s( buf , format , mark );
	va_end( mark );

	w		( buf, xr_strlen(buf) );
}

//---------------------------------------------------
// base stream
IReader*	IReader::open_chunk(u32 ID)
{
	BOOL	bCompressed;

	u32	dwSize = find_chunk(ID,&bCompressed);
	if (dwSize!=0) {
		if (bCompressed) {
			BYTE*		dest;
			unsigned	dest_sz;
			_decompressLZ(&dest,&dest_sz,pointer(),dwSize);
			return xr_new<CTempReader>	(dest,		dest_sz,		tell()+dwSize);
		} else {
			return xr_new<IReader>		(pointer(),	dwSize,			tell()+dwSize);
		}
	} else return 0;
};
void	IReader::close()
{
	IReader* temp = this;
	xr_delete(temp);
}

#include "FS_impl.h"

#ifdef TESTING_IREADER
IReaderTestPolicy::~IReaderTestPolicy()
{
	xr_delete(m_test);
};
#endif // TESTING_IREADER

#ifdef FIND_CHUNK_BENCHMARK_ENABLE
find_chunk_counter g_find_chunk_counter;
#endif // FIND_CHUNK_BENCHMARK_ENABLE

u32 IReader::find_chunk						(u32 ID, BOOL* bCompressed)
{
	return inherited::find_chunk(ID, bCompressed);
}

IReader*	IReader::open_chunk_iterator	(u32& ID, IReader* _prev)
{
	if (0==_prev)	{
		// first
		rewind		();
	} else {
		// next
		seek		(_prev->iterpos);
		_prev->close();
	}

	//	open
	if			(elapsed()<8)	return		NULL;
	ID			= r_u32	()		;
	u32 _size	= r_u32	()		;
	if ( ID & CFS_CompressMark )
	{
		// compressed
		u8*				dest	;
		unsigned		dest_sz	;
		_decompressLZ	(&dest,&dest_sz,pointer(),_size);
		return xr_new<CTempReader>	(dest,		dest_sz,	tell()+_size);
	} else {
		// normal
		return xr_new<IReader>		(pointer(),	_size,		tell()+_size);
	}
}

void	IReader::r	(void *p,int cnt)
{
	VERIFY(Pos+cnt<=Size);
	CopyMemoryW(p,pointer(),cnt);
	advance(cnt);
};

IC BOOL			is_term		(TCHAR a) { return (a==13)||(a==10); };
IC u32	IReader::advance_term_string()
{
	u32 sz		= 0;
	TCHAR*src 	= (TCHAR*)wstring_convert_from_char(data);
	while (!eof()) {
        Pos++;
        sz++;
		if (!eof()&&is_term(src[Pos])) 
		{
        	while(!eof() && is_term(src[Pos])) 
				Pos++;
			break;
		}
	}
    return sz;
}

void	IReader::r_string	(TCHAR* dest, u32 tgt_sz)
{
	TCHAR* src = (TCHAR*)wstring_convert_from_char(data + Pos);
	u32 sz 		= advance_term_string();
    R_ASSERT2(sz<(tgt_sz-1),"Dest string less than needed.");
	R_ASSERT	(!IsBadReadPtr((void*)src,sz));

#ifdef _EDITOR
	CopyMemory	 (dest,src,sz);
#else
    wcsncpy_s	(dest,tgt_sz, src,sz);
#endif
    dest[sz]	= 0;
}
void	IReader::r_string	(xr_string& dest)
{
	TCHAR*src 	= (TCHAR*)wstring_convert_from_char(data+Pos);
	u32 sz 		= advance_term_string();

	//TCHAR* buf = wstring_convert_from_char(src);

    dest.assign	(src,sz);
}
void	IReader::r_stringZ	(TCHAR*dest, u32 tgt_sz)
{
	TCHAR*src 	= (TCHAR*)wstring_convert_from_char(data);
	u32 sz 		= xr_strlen(src);
    R_ASSERT2(sz<tgt_sz,"Dest string less than needed.");
	while ((src[Pos]!=0) && (!eof())) *dest++ = src[Pos++];
	*dest		=	0;
	Pos++;
}
void 	IReader::r_stringZ	(shared_str& dest)
{
	dest		= (TCHAR*)wstring_convert_from_char(data+Pos);
    Pos			+=(dest.size()+1);
}
void	IReader::r_stringZ	(xr_string& dest)
{
    dest 		= (TCHAR*)wstring_convert_from_char(data+Pos);
    Pos			+=int(dest.size()+1);
};

void	IReader::skip_stringZ	()
{
	TCHAR *src = (TCHAR *)wstring_convert_from_char(data);
	while ((src[Pos]!=0) && (!eof())) Pos++;
	Pos		++;
};

//---------------------------------------------------
// temp stream
CTempReader::~CTempReader()
{	xr_free(data);	};
//---------------------------------------------------
// pack stream
CPackReader::~CPackReader()
{
#ifdef DEBUG
	unregister_file_mapping	(base_address,Size);
#endif // DEBUG

	UnmapViewOfFile	(base_address);
};
//---------------------------------------------------
// file stream
CFileReader::CFileReader(const TCHAR*name)
{

    data	= (char*)FileDownload(name,(u32 *)&Size);
    Pos		= 0;
};
CFileReader::~CFileReader()
{	xr_free(data);	};
//---------------------------------------------------
// compressed stream
//CCompressedReader::CCompressedReader(const TCHAR *name, const TCHAR *sign)
//{
//    data	= (TCHAR*)FileDecompress(name,sign,(u32*)&Size);
//    Pos		= 0;
//}
CCompressedReader::~CCompressedReader()
{	xr_free(data);	};


CVirtualFileRW::CVirtualFileRW(const TCHAR* cFileName)
{
	// Open the file
	hSrcFile		= CreateFile(cFileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	//R_ASSERT3		(hSrcFile!=INVALID_HANDLE_VALUE,cFileName,Debug.error2string(GetLastError()));
	Size			= (int)GetFileSize(hSrcFile, NULL);
	//R_ASSERT3		(Size,cFileName,Debug.error2string(GetLastError()));

	hSrcMap			= CreateFileMapping (hSrcFile, 0, PAGE_READWRITE, 0, 0, 0);
	//R_ASSERT3		(hSrcMap!=INVALID_HANDLE_VALUE,cFileName,Debug.error2string(GetLastError()));

	data			= (char*)MapViewOfFile (hSrcMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	//R_ASSERT3		(data,cFileName,Debug.error2string(GetLastError()));

#ifdef DEBUG
	register_file_mapping	(data,Size,cFileName);
#endif // DEBUG
}

CVirtualFileRW::~CVirtualFileRW() 
{
#ifdef DEBUG
	unregister_file_mapping	(data,Size);
#endif // DEBUG

	UnmapViewOfFile ((void*)data);
	CloseHandle		(hSrcMap);
	CloseHandle		(hSrcFile);
}

CVirtualFileReader::CVirtualFileReader(const TCHAR* cFileName)
{
	// Open the file
	hSrcFile		= CreateFile(cFileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	//R_ASSERT3		(hSrcFile!=INVALID_HANDLE_VALUE,cFileName,Debug.error2string(GetLastError()));
	Size			= (int)GetFileSize(hSrcFile, NULL);
	//R_ASSERT3		(Size,cFileName,Debug.error2string(GetLastError()));

	hSrcMap			= CreateFileMapping (hSrcFile, 0, PAGE_READONLY, 0, 0, 0);
	//R_ASSERT3		(hSrcMap!=INVALID_HANDLE_VALUE,cFileName,Debug.error2string(GetLastError()));

	data			= (char*)MapViewOfFile (hSrcMap, FILE_MAP_READ, 0, 0, 0);
	//R_ASSERT3		(data,cFileName,Debug.error2string(GetLastError()));

#ifdef DEBUG
	register_file_mapping	(data,Size,cFileName);
#endif // DEBUG
}

CVirtualFileReader::~CVirtualFileReader() 
{
#ifdef DEBUG
	unregister_file_mapping	(data,Size);
#endif // DEBUG

	UnmapViewOfFile ((void*)data);
	CloseHandle		(hSrcMap);
	CloseHandle		(hSrcFile);
}
