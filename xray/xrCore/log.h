#ifndef logH
#define logH

#define VPUSH(a)	a.x,a.y,a.z

void 	XRCORE_API	__cdecl		Msg	(LPCTSTR format, ...);
void 	XRCORE_API		Log			(LPCTSTR msg);
void 	XRCORE_API		Log			(LPCTSTR msg);
void 	XRCORE_API		Log			(LPCTSTR msg, LPCTSTR			dop);
void 	XRCORE_API		Log			(LPCTSTR msg, u32			dop);
void 	XRCORE_API		Log			(LPCTSTR msg, int  			dop);
void 	XRCORE_API		Log			(LPCTSTR msg, float			dop);
void 	XRCORE_API		Log			(LPCTSTR msg, const Fvector& dop);
void 	XRCORE_API		Log			(LPCTSTR msg, const Fmatrix& dop);
void 	XRCORE_API		LogWinErr	(LPCTSTR msg, long 			err_code);

typedef void	( * LogCallback)	(LPCTSTR string);
LogCallback	XRCORE_API			SetLogCB	(LogCallback cb);
void 							CreateLog	(BOOL no_log=FALSE);
void 							InitLog		();
void 							CloseLog	();
void	XRCORE_API				FlushLog	();

extern 	XRCORE_API	xr_vector<shared_str>*		LogFile;
extern 	XRCORE_API	BOOL						LogExecCB;

XRCORE_API LPCTSTR logFullName();

#endif

