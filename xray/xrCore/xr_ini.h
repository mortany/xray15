#ifndef xr_iniH
#define xr_iniH

// refs
class	CInifile;
struct	xr_token;


class XRCORE_API CInifile
{
public:
	struct XRCORE_API	Item
	{
		shared_str	first;
		shared_str	second;
#ifdef DEBUG
		shared_str	comment;
#endif
		Item() : first(0), second(0)
#ifdef DEBUG
			, comment(0)
#endif
		{};
	};
	typedef xr_vector<Item>				Items;
	typedef Items::const_iterator		SectCIt;
	typedef Items::iterator				SectIt_;
    struct XRCORE_API	Sect {
		shared_str		Name;
		Items			Data;

		BOOL			line_exist	(LPCTSTR L, LPCTSTR* val=0);
	};
	typedef	xr_vector<Sect*>		Root;
	typedef Root::iterator			RootIt;

	static CInifile*	Create		( LPCTSTR szFileName, BOOL ReadOnly=TRUE);
	static void			Destroy		( CInifile*);
    static IC BOOL		IsBOOL		( LPCTSTR B)	{ return (xr_strcmp(B,TEXT("on"))==0 || xr_strcmp(B,TEXT("yes"))==0 || xr_strcmp(B,TEXT("true"))==0 || xr_strcmp(B,TEXT("1"))==0);}
private:
	enum{eSaveAtEnd = (1<<0), eReadOnly= (1<<1), eOverrideNames=(1<<2),};
	Flags8			m_flags;
	string_path		m_file_name;
	Root			DATA;

	void			Load			(IReader* F, LPCTSTR path);
public:
				CInifile		( IReader* F, LPCTSTR path=0 );
				CInifile		( LPCTSTR szFileName, BOOL ReadOnly=TRUE, BOOL bLoadAtStart=TRUE, BOOL SaveAtEnd=TRUE);
	virtual 	~CInifile		( );
    bool		save_as         ( LPCTSTR new_fname=0 );
	void		save_as			(IWriter& writer);
	void		set_override_names(BOOL b){m_flags.set(eOverrideNames,b);}
	void		save_at_end		(BOOL b){m_flags.set(eSaveAtEnd,b);}
	LPCTSTR		fname			( ) { return m_file_name; };

	Sect&		r_section		( LPCTSTR S			);
	Sect&		r_section		( const shared_str& S	);
	BOOL		line_exist		( LPCTSTR S, LPCTSTR L );
	BOOL		line_exist		( const shared_str& S, const shared_str& L );
	u32			line_count		( LPCTSTR S			);
	u32			line_count		( const shared_str& S	);
	BOOL		section_exist	( LPCTSTR S			);
	BOOL		section_exist	( const shared_str& S	);
	Root&		sections		( ){return DATA;}

	CLASS_ID	r_clsid			( LPCTSTR S, LPCTSTR L );
	CLASS_ID	r_clsid			( const shared_str& S, LPCTSTR L )				{ return r_clsid(*S,L);			}
	LPCTSTR 		r_string		( LPCTSTR S, LPCTSTR L);															// оставляет кавычки
	LPCTSTR 		r_string		( const shared_str& S, LPCTSTR L)				{ return r_string(*S,L);		}	// оставляет кавычки
	shared_str		r_string_wb		( LPCTSTR S, LPCTSTR L);															// убирает кавычки
	shared_str		r_string_wb		( const shared_str& S, LPCTSTR L)				{ return r_string_wb(*S,L);		}	// убирает кавычки
	u8	 		r_u8			( LPCTSTR S, LPCTSTR L );
	u8	 		r_u8			( const shared_str& S, LPCTSTR L )				{ return r_u8(*S,L);			}
	u16	 		r_u16			( LPCTSTR S, LPCTSTR L );
	u16	 		r_u16			( const shared_str& S, LPCTSTR L )				{ return r_u16(*S,L);			}
	u32	 		r_u32			( LPCTSTR S, LPCTSTR L );
	u32	 		r_u32			( const shared_str& S, LPCTSTR L )				{ return r_u32(*S,L);			}
	u64	 		r_u64			( LPCTSTR S, LPCTSTR L );
	s8	 		r_s8			( LPCTSTR S, LPCTSTR L );
	s8	 		r_s8			( const shared_str& S, LPCTSTR L )				{ return r_s8(*S,L);			}
	s16	 		r_s16			( LPCTSTR S, LPCTSTR L );
	s16	 		r_s16			( const shared_str& S, LPCTSTR L )				{ return r_s16(*S,L);			}
	s32	 		r_s32			( LPCTSTR S, LPCTSTR L );
	s32	 		r_s32			( const shared_str& S, LPCTSTR L )				{ return r_s32(*S,L);			}
	s64	 		r_s64			( LPCTSTR S, LPCTSTR L );
	float		r_float			( LPCTSTR S, LPCTSTR L );
	float		r_float			( const shared_str& S, LPCTSTR L )				{ return r_float(*S,L);			}
	Fcolor		r_fcolor		( LPCTSTR S, LPCTSTR L );
	Fcolor		r_fcolor		( const shared_str& S, LPCTSTR L )				{ return r_fcolor(*S,L);		}
	u32			r_color			( LPCTSTR S, LPCTSTR L );
	u32			r_color			( const shared_str& S, LPCTSTR L )				{ return r_color(*S,L);			}
	Ivector2	r_ivector2		( LPCTSTR S, LPCTSTR L );
	Ivector2	r_ivector2		( const shared_str& S, LPCTSTR L )				{ return r_ivector2(*S,L);		}
	Ivector3	r_ivector3		( LPCTSTR S, LPCTSTR L );
	Ivector3	r_ivector3		( const shared_str& S, LPCTSTR L )				{ return r_ivector3(*S,L);		}
	Ivector4	r_ivector4		( LPCTSTR S, LPCTSTR L );
	Ivector4	r_ivector4		( const shared_str& S, LPCTSTR L )				{ return r_ivector4(*S,L);		}
	Fvector2	r_fvector2		( LPCTSTR S, LPCTSTR L );
	Fvector2	r_fvector2		( const shared_str& S, LPCTSTR L )				{ return r_fvector2(*S,L);		}
	Fvector3	r_fvector3		( LPCTSTR S, LPCTSTR L );
	Fvector3	r_fvector3		( const shared_str& S, LPCTSTR L )				{ return r_fvector3(*S,L);		}
	Fvector4	r_fvector4		( LPCTSTR S, LPCTSTR L );
	Fvector4	r_fvector4		( const shared_str& S, LPCTSTR L )				{ return r_fvector4(*S,L);		}
	BOOL		r_bool			( LPCTSTR S, LPCTSTR L );
	BOOL		r_bool			( const shared_str& S, LPCTSTR L )				{ return r_bool(*S,L);			}
	int			r_token			( LPCTSTR S, LPCTSTR L,	const xr_token *token_list);
	BOOL		r_line			( LPCTSTR S, int L,	LPCTSTR* N, LPCTSTR* V );
	BOOL		r_line			( const shared_str& S, int L,	LPCTSTR* N, LPCTSTR* V );

    void		w_string		( LPCTSTR S, LPCTSTR L, LPCTSTR			V, LPCTSTR comment=0 );
	void		w_u8			( LPCTSTR S, LPCTSTR L, u8				V, LPCTSTR comment=0 );
	void		w_u16			( LPCTSTR S, LPCTSTR L, u16				V, LPCTSTR comment=0 );
	void		w_u32			( LPCTSTR S, LPCTSTR L, u32				V, LPCTSTR comment=0 );
	void		w_u64			( LPCTSTR S, LPCTSTR L, u64				V, LPCTSTR comment=0 );
	void		w_s64			( LPCTSTR S, LPCTSTR L, s64				V, LPCTSTR comment=0 );
    void		w_s8			( LPCTSTR S, LPCTSTR L, s8				V, LPCTSTR comment=0 );
	void		w_s16			( LPCTSTR S, LPCTSTR L, s16				V, LPCTSTR comment=0 );
	void		w_s32			( LPCTSTR S, LPCTSTR L, s32				V, LPCTSTR comment=0 );
	void		w_float			( LPCTSTR S, LPCTSTR L, float				V, LPCTSTR comment=0 );
    void		w_fcolor		( LPCTSTR S, LPCTSTR L, const Fcolor&		V, LPCTSTR comment=0 );
    void		w_color			( LPCTSTR S, LPCTSTR L, u32				V, LPCTSTR comment=0 );
    void		w_ivector2		( LPCTSTR S, LPCTSTR L, const Ivector2&	V, LPCTSTR comment=0 );
	void		w_ivector3		( LPCTSTR S, LPCTSTR L, const Ivector3&	V, LPCTSTR comment=0 );
	void		w_ivector4		( LPCTSTR S, LPCTSTR L, const Ivector4&	V, LPCTSTR comment=0 );
	void		w_fvector2		( LPCTSTR S, LPCTSTR L, const Fvector2&	V, LPCTSTR comment=0 );
	void		w_fvector3		( LPCTSTR S, LPCTSTR L, const Fvector3&	V, LPCTSTR comment=0 );
	void		w_fvector4		( LPCTSTR S, LPCTSTR L, const Fvector4&	V, LPCTSTR comment=0 );
	void		w_bool			( LPCTSTR S, LPCTSTR L, BOOL				V, LPCTSTR comment=0 );

    void		remove_line		( LPCTSTR S, LPCTSTR L );
};

// Main configuration file
extern XRCORE_API CInifile *pSettings;


#endif //__XR_INI_H__
