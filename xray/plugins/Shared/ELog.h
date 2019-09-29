//----------------------------------------------------
// file: Log.h
//----------------------------------------------------

#ifndef ELogH
#define ELogH

class ECORE_API CLog{
public:
	bool 		in_use;
public:
				CLog	(){in_use=false;}
	void 		Msg   	(TMsgDlgType mt, LPCTSTR _Format, ...);
	int 		DlgMsg 	(TMsgDlgType mt, LPCTSTR _Format, ...);
	int 		DlgMsg 	(TMsgDlgType mt, TMsgDlgButtons btn, LPCTSTR _Format, ...);
};

void ECORE_API ELogCallback(LPCTSTR txt);

extern ECORE_API CLog ELog;

#endif /*_INCDEF_NETDEVICELOG_H_*/

