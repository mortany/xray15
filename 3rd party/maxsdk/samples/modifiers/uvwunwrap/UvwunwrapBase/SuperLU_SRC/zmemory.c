
/*! @file zmemory.c
 * \brief Memory details
 *
 * <pre>
 * -- SuperLU routine (version 3.0) --
 * Univ. of California Berkeley, Xerox Palo Alto Research Center,
 * and Lawrence Berkeley National Lab.
 * October 15, 2003
 * </pre>
 */
#include "slu_zdefs.h"

/* Constants */
#define NO_MEMTYPE  4      /* 0: lusup;
			      1: ucol;
			      2: lsub;
			      3: usub */
#define GluIntArray(n)   (5 * (n) + 5)

/* Internal prototypes */
void  *zexpand (int *, MemType,int, int, GlobalLU_t *);
int   zLUWorkInit (int, int, int, int **, doublecomplex **, LU_space_t);
void  copy_mem_doublecomplex (int, void *, void *);
void  zStackCompress (GlobalLU_t *);
void  zSetupSpace (void *, int, LU_space_t *);
void  *zuser_malloc (int, int);
void  zuser_free (int, int);

/* External prototypes (in memory.c - prec-indep) */
extern void    copy_mem_int    (int, void *, void *);
extern void    user_bcopy      (char *, char *, int);

/*! \brief Headers for 4 types of dynamatically managed memory */
typedef struct e_node {
    int size;      /* length of the memory that has been used */
    void *mem;     /* pointer to the new malloc'd store */
} ExpHeader;

typedef struct {
    int  size;
    int  used;
    int  top1;  /* grow upward, relative to &array[0] */
    int  top2;  /* grow downward */
    void *array;
} LU_stack_t;

/* Variables local to this file */
static ExpHeader *expanders = 0; /* Array of pointers to 4 types of memory */
static LU_stack_t stack;
static int no_expand;

/* Macros to manipulate stack */
#define StackFull(x)         ( x + stack.used >= stack.size )
#define NotDoubleAlign(addr) ( (long int)addr & 7 )
#define DoubleAlign(addr)    ( ((long int)addr + 7) & ~7L )
#define TempSpace(m, w)      ( (2*w + 4 + NO_MARKER) * m * sizeof(int) + \
			      (w + 1) * m * sizeof(doublecomplex) )
#define Reduce(alpha)        ((alpha + 1) / 2)  /* i.e. (alpha-1)/2 + 1 */




/*! \brief Setup the memory model to be used for factorization.
 *  
 *    lwork = 0: use system malloc;
 *    lwork > 0: use user-supplied work[] space.
 */
void zSetupSpace(void *work, int lwork, LU_space_t *MemModel)
{
    if ( lwork == 0 ) {
	*MemModel = SYSTEM; /* malloc/free */
    } else if ( lwork > 0 ) {
	*MemModel = USER;   /* user provided space */
	stack.used = 0;
	stack.top1 = 0;
	stack.top2 = (lwork/4)*4; /* must be word addressable */
	stack.size = stack.top2;
	stack.array = (void *) work;
    }
}



void *zuser_malloc(int bytes, int which_end)
{
    void *buf;
    
    if ( StackFull(bytes) ) return (NULL);

    if ( which_end == HEAD ) {
	buf = (char*) stack.array + stack.top1;
	stack.top1 += bytes;
    } else {
	stack.top2 -= bytes;
	buf = (char*) stack.array + stack.top2;
    }
    
    stack.used += bytes;
    return buf;
}


void zuser_free(int bytes, int which_end)
{
    if ( which_end == HEAD ) {
	stack.top1 -= bytes;
    } else {
	stack.top2 += bytes;
    }
    stack.used -= bytes;
}



/*! \brief 
 *
 * <pre>
 * mem_usage consists of the following fields:
 *    - for_lu (float)
 *      The amount of space used in bytes for the L\U data structures.
 *    - total_needed (float)
 *      The amount of space needed in bytes to perform factorization.
 *    - expansions (int)
 *      Number of memory expansions during the LU factorization.
 * </pre>
 */
int zQuerySpace(SuperMatrix *L, SuperMatrix *U, mem_usage_t *mem_usage)
{
    SCformat *Lstore;
    NCformat *Ustore;
    register int n, iword, dword, panel_size = sp_ienv(1);

    Lstore = L->Store;
    Ustore = U->Store;
    n = L->ncol;
    iword = sizeof(int);
    dword = sizeof(doublecomplex);

    /* For LU factors */
    mem_usage->for_lu = (float)( (4*n + 3) * iword + Lstore->nzval_colptr[n] *
				 dword + Lstore->rowind_colptr[n] * iword );
    mem_usage->for_lu += (float)( (n + 1) * iword +
				 Ustore->colptr[n] * (dword + iword) );

    /* Working storage to support factorization */
    mem_usage->total_needed = mem_usage->for_lu +
	(float)( (2 * panel_size + 4 + NO_MARKER) * n * iword +
		(panel_size + 1) * n * dword );

    mem_usage->expansions = --no_expand;

    return 0;
} /* zQuerySpace */

/*! \brief Allocate storage for the data structures common to all factor routines.
 *
 * <pre>
 * For those unpredictable size, make a guess as FILL * nnz(A).
 * Return value:
 *     If lwork = -1, return the estimated amount of space required, plus n;
 *     otherwise, return the amount of space actually allocated when
 *     memory allocation failure occurred.
 * </pre> 
 */
int
zLUMemInit(fact_t fact, void *work, int lwork, int m, int n, int annz,
	  int panel_size, SuperMatrix *L, SuperMatrix *U, GlobalLU_t *Glu,
	  int **iwork, doublecomplex **dwork)
{
    int      info, iword, dword;
    SCformat *Lstore;
    NCformat *Ustore;
    int      *xsup, *supno;
    int      *lsub, *xlsub;
    doublecomplex   *lusup;
    int      *xlusup;
    doublecomplex   *ucol;
    int      *usub, *xusub;
    int      nzlmax, nzumax, nzlumax;
    int      FILL = sp_ienv(6);
    
    Glu->n    = n;
    no_expand = 0;
    iword     = sizeof(int);
    dword     = sizeof(doublecomplex);

    if ( !expanders )	
        expanders = (ExpHeader*)SUPERLU_MALLOC(NO_MEMTYPE * sizeof(ExpHeader));
    if ( !expanders ) ABORT("SUPERLU_MALLOC fails for expanders");
    
    if ( fact != SamePattern_SameRowPerm ) {
	/* Guess for L\U factors */
	nzumax = nzlumax = FILL * annz;
	nzlmax = SUPERLU_MAX(1, FILL/4.) * annz;

	if ( lwork == -1 ) {
	    return ( GluIntArray(n) * iword + TempSpace(m, panel_size)
		    + (nzlmax+nzumax)*iword + (nzlumax+nzumax)*dword + n );
        } else {
	    zSetupSpace(work, lwork, &Glu->MemModel);
	}
	
#if ( PRNTlevel >= 1 )
	printf("zLUMemInit() called: FILL %ld, nzlmax %ld, nzumax %ld\n", 
	       FILL, nzlmax, nzumax);
	fflush(stdout);
#endif	
	
	/* Integer pointers for L\U factors */
	if ( Glu->MemModel == SYSTEM ) {
	    xsup   = intMalloc(n+1);
	    supno  = intMalloc(n+1);
	    xlsub  = intMalloc(n+1);
	    xlusup = intMalloc(n+1);
	    xusub  = intMalloc(n+1);
	} else {
	    xsup   = (int *)zuser_malloc((n+1) * iword, HEAD);
	    supno  = (int *)zuser_malloc((n+1) * iword, HEAD);
	    xlsub  = (int *)zuser_malloc((n+1) * iword, HEAD);
	    xlusup = (int *)zuser_malloc((n+1) * iword, HEAD);
	    xusub  = (int *)zuser_malloc((n+1) * iword, HEAD);
	}

	lusup = (doublecomplex *) zexpand( &nzlumax, LUSUP, 0, 0, Glu );
	ucol  = (doublecomplex *) zexpand( &nzumax, UCOL, 0, 0, Glu );
	lsub  = (int *)    zexpand( &nzlmax, LSUB, 0, 0, Glu );
	usub  = (int *)    zexpand( &nzumax, USUB, 0, 1, Glu );

	while ( !lusup || !ucol || !lsub || !usub ) {
	    if ( Glu->MemModel == SYSTEM ) {
		SUPERLU_FREE(lusup); 
		SUPERLU_FREE(ucol); 
		SUPERLU_FREE(lsub); 
		SUPERLU_FREE(usub);
	    } else {
		zuser_free((nzlumax+nzumax)*dword+(nzlmax+nzumax)*iword, HEAD);
	    }
	    nzlumax /= 2;
	    nzumax /= 2;
	    nzlmax /= 2;
	    if ( nzlumax < annz ) {
		printf("Not enough memory to perform factorization.\n");
		return (zmemory_usage(nzlmax, nzumax, nzlumax, n) + n);
	    }
#if ( PRNTlevel >= 1)
	    printf("zLUMemInit() reduce size: nzlmax %ld, nzumax %ld\n", 
		   nzlmax, nzumax);
	    fflush(stdout);
#endif
	    lusup = (doublecomplex *) zexpand( &nzlumax, LUSUP, 0, 0, Glu );
	    ucol  = (doublecomplex *) zexpand( &nzumax, UCOL, 0, 0, Glu );
	    lsub  = (int *)    zexpand( &nzlmax, LSUB, 0, 0, Glu );
	    usub  = (int *)    zexpand( &nzumax, USUB, 0, 1, Glu );
	}
	
    } else {
	/* fact == SamePattern_SameRowPerm */
	Lstore   = L->Store;
	Ustore   = U->Store;
	xsup     = Lstore->sup_to_col;
	supno    = Lstore->col_to_sup;
	xlsub    = Lstore->rowind_colptr;
	xlusup   = Lstore->nzval_colptr;
	xusub    = Ustore->colptr;
	nzlmax   = Glu->nzlmax;    /* max from previous factorization */
	nzumax   = Glu->nzumax;
	nzlumax  = Glu->nzlumax;
	
	if ( lwork == -1 ) {
	    return ( GluIntArray(n) * iword + TempSpace(m, panel_size)
		    + (nzlmax+nzumax)*iword + (nzlumax+nzumax)*dword + n );
        } else if ( lwork == 0 ) {
	    Glu->MemModel = SYSTEM;
	} else {
	    Glu->MemModel = USER;
	    stack.top2 = (lwork/4)*4; /* must be word-addressable */
	    stack.size = stack.top2;
	}
	
	lsub  = expanders[LSUB].mem  = Lstore->rowind;
	lusup = expanders[LUSUP].mem = Lstore->nzval;
	usub  = expanders[USUB].mem  = Ustore->rowind;
	ucol  = expanders[UCOL].mem  = Ustore->nzval;;
	expanders[LSUB].size         = nzlmax;
	expanders[LUSUP].size        = nzlumax;
	expanders[USUB].size         = nzumax;
	expanders[UCOL].size         = nzumax;	
    }

    Glu->xsup    = xsup;
    Glu->supno   = supno;
    Glu->lsub    = lsub;
    Glu->xlsub   = xlsub;
    Glu->lusup   = lusup;
    Glu->xlusup  = xlusup;
    Glu->ucol    = ucol;
    Glu->usub    = usub;
    Glu->xusub   = xusub;
    Glu->nzlmax  = nzlmax;
    Glu->nzumax  = nzumax;
    Glu->nzlumax = nzlumax;
    
    info = zLUWorkInit(m, n, panel_size, iwork, dwork, Glu->MemModel);
    if ( info )
	return ( info + zmemory_usage(nzlmax, nzumax, nzlumax, n) + n);
    
    ++no_expand;
    return 0;
    
} /* zLUMemInit */

/*! \brief Allocate known working storage. Returns 0 if success, otherwise
   returns the number of bytes allocated so far when failure occurred. */
int
zLUWorkInit(int m, int n, int panel_size, int **iworkptr, 
            doublecomplex **dworkptr, LU_space_t MemModel)
{
    int    isize, dsize, extra;
    doublecomplex *old_ptr;
    int    maxsuper = sp_ienv(3),
           rowblk   = sp_ienv(4);

    isize = ( (2 * panel_size + 3 + NO_MARKER ) * m + n ) * sizeof(int);
    dsize = (m * panel_size +
	     NUM_TEMPV(m,panel_size,maxsuper,rowblk)) * sizeof(doublecomplex);
    
    if ( MemModel == SYSTEM ) 
	*iworkptr = (int *) intCalloc(isize/sizeof(int));
    else
	*iworkptr = (int *) zuser_malloc(isize, TAIL);
    if ( ! *iworkptr ) {
	fprintf(stderr, "zLUWorkInit: malloc fails for local iworkptr[]\n");
	return (isize + n);
    }

    if ( MemModel == SYSTEM )
	*dworkptr = (doublecomplex *) SUPERLU_MALLOC(dsize);
    else {
	*dworkptr = (doublecomplex *) zuser_malloc(dsize, TAIL);
	if ( NotDoubleAlign(*dworkptr) ) {
	    old_ptr = *dworkptr;
	    *dworkptr = (doublecomplex*) DoubleAlign(*dworkptr);
	    *dworkptr = (doublecomplex*) ((double*)*dworkptr - 1);
	    extra = (char*)old_ptr - (char*)*dworkptr;
#ifdef DEBUG	    
	    printf("zLUWorkInit: not aligned, extra %d\n", extra);
#endif	    
	    stack.top2 -= extra;
	    stack.used += extra;
	}
    }
    if ( ! *dworkptr ) {
	fprintf(stderr, "malloc fails for local dworkptr[].");
	return (isize + dsize + n);
    }
	
    return 0;
}


/*! \brief Set up pointers for real working arrays.
 */
void
zSetRWork(int m, int panel_size, doublecomplex *dworkptr,
	 doublecomplex **dense, doublecomplex **tempv)
{
    doublecomplex zero = {0.0, 0.0};

    int maxsuper = sp_ienv(3),
        rowblk   = sp_ienv(4);
    *dense = dworkptr;
    *tempv = *dense + panel_size*m;
    zfill (*dense, m * panel_size, zero);
    zfill (*tempv, NUM_TEMPV(m,panel_size,maxsuper,rowblk), zero);     
}
	
/*! \brief Free the working storage used by factor routines.
 */
void zLUWorkFree(int *iwork, doublecomplex *dwork, GlobalLU_t *Glu)
{
    if ( Glu->MemModel == SYSTEM ) {
	SUPERLU_FREE (iwork);
	SUPERLU_FREE (dwork);
    } else {
	stack.used -= (stack.size - stack.top2);
	stack.top2 = stack.size;
/*	zStackCompress(Glu);  */
    }
    
    SUPERLU_FREE (expanders);	
    expanders = 0;
}

/*! \brief Expand the data structures for L and U during the factorization.
 * 
 * <pre>
 * Return value:   0 - successful return
 *               > 0 - number of bytes allocated when run out of space
 * </pre>
 */
int
zLUMemXpand(int jcol,
	   int next,          /* number of elements currently in the factors */
	   MemType mem_type,  /* which type of memory to expand  */
	   int *maxlen,       /* modified - maximum length of a data structure */
	   GlobalLU_t *Glu    /* modified - global LU data structures */
	   )
{
    void   *new_mem;
    
#ifdef DEBUG    
    printf("zLUMemXpand(): jcol %d, next %d, maxlen %d, MemType %d\n",
	   jcol, next, *maxlen, mem_type);
#endif    

    if (mem_type == USUB) 
    	new_mem = zexpand(maxlen, mem_type, next, 1, Glu);
    else
	new_mem = zexpand(maxlen, mem_type, next, 0, Glu);
    
    if ( !new_mem ) {
	int    nzlmax  = Glu->nzlmax;
	int    nzumax  = Glu->nzumax;
	int    nzlumax = Glu->nzlumax;
    	fprintf(stderr, "Can't expand MemType %d: jcol %d\n", mem_type, jcol);
    	return (zmemory_usage(nzlmax, nzumax, nzlumax, Glu->n) + Glu->n);
    }

    switch ( mem_type ) {
      case LUSUP:
	Glu->lusup   = (doublecomplex *) new_mem;
	Glu->nzlumax = *maxlen;
	break;
      case UCOL:
	Glu->ucol   = (doublecomplex *) new_mem;
	Glu->nzumax = *maxlen;
	break;
      case LSUB:
	Glu->lsub   = (int *) new_mem;
	Glu->nzlmax = *maxlen;
	break;
      case USUB:
	Glu->usub   = (int *) new_mem;
	Glu->nzumax = *maxlen;
	break;
    }
    
    return 0;
    
}



void
copy_mem_doublecomplex(int howmany, void *old, void *new)
{
    register int i;
    doublecomplex *dold = old;
    doublecomplex *dnew = new;
    for (i = 0; i < howmany; i++) dnew[i] = dold[i];
}

/*! \brief Expand the existing storage to accommodate more fill-ins.
 */
void
*zexpand (
	 int *prev_len,   /* length used from previous call */
	 MemType type,    /* which part of the memory to expand */
	 int len_to_copy, /* size of the memory to be copied to new store */
	 int keep_prev,   /* = 1: use prev_len;
			     = 0: compute new_len to expand */
	 GlobalLU_t *Glu  /* modified - global LU data structures */
	)
{
    float    EXPAND = 1.5;
    float    alpha;
    void     *new_mem, *old_mem;
    int      new_len, tries, lword, extra, bytes_to_copy;

    alpha = EXPAND;

    if ( no_expand == 0 || keep_prev ) /* First time allocate requested */
        new_len = *prev_len;
    else {
	new_len = alpha * *prev_len;
    }
    
    if ( type == LSUB || type == USUB ) lword = sizeof(int);
    else lword = sizeof(doublecomplex);

    if ( Glu->MemModel == SYSTEM ) {
	new_mem = (void *) SUPERLU_MALLOC((size_t)new_len * lword);
	if ( no_expand != 0 ) {
	    tries = 0;
	    if ( keep_prev ) {
		if ( !new_mem ) return (NULL);
	    } else {
		while ( !new_mem ) {
		    if ( ++tries > 10 ) return (NULL);
		    alpha = Reduce(alpha);
		    new_len = alpha * *prev_len;
		    new_mem = (void *) SUPERLU_MALLOC((size_t)new_len * lword);
		}
	    }
	    if ( type == LSUB || type == USUB ) {
		copy_mem_int(len_to_copy, expanders[type].mem, new_mem);
	    } else {
		copy_mem_doublecomplex(len_to_copy, expanders[type].mem, new_mem);
	    }
	    SUPERLU_FREE (expanders[type].mem);
	}
	expanders[type].mem = (void *) new_mem;
	
    } else { /* MemModel == USER */
	if ( no_expand == 0 ) {
	    new_mem = zuser_malloc(new_len * lword, HEAD);
	    if ( NotDoubleAlign(new_mem) &&
		(type == LUSUP || type == UCOL) ) {
		old_mem = new_mem;
		new_mem = (void *)DoubleAlign(new_mem);
		extra = (char*)new_mem - (char*)old_mem;
#ifdef DEBUG		
		printf("expand(): not aligned, extra %d\n", extra);
#endif		
		stack.top1 += extra;
		stack.used += extra;
	    }
	    expanders[type].mem = (void *) new_mem;
	}
	else {
	    tries = 0;
	    extra = (new_len - *prev_len) * lword;
	    if ( keep_prev ) {
		if ( StackFull(extra) ) return (NULL);
	    } else {
		while ( StackFull(extra) ) {
		    if ( ++tries > 10 ) return (NULL);
		    alpha = Reduce(alpha);
		    new_len = alpha * *prev_len;
		    extra = (new_len - *prev_len) * lword;	    
		}
	    }

	    if ( type != USUB ) {
		new_mem = (void*)((char*)expanders[type + 1].mem + extra);
		bytes_to_copy = (char*)stack.array + stack.top1
		    - (char*)expanders[type + 1].mem;
		user_bcopy(expanders[type+1].mem, new_mem, bytes_to_copy);

		if ( type < USUB ) {
		    Glu->usub = expanders[USUB].mem =
			(void*)((char*)expanders[USUB].mem + extra);
		}
		if ( type < LSUB ) {
		    Glu->lsub = expanders[LSUB].mem =
			(void*)((char*)expanders[LSUB].mem + extra);
		}
		if ( type < UCOL ) {
		    Glu->ucol = expanders[UCOL].mem =
			(void*)((char*)expanders[UCOL].mem + extra);
		}
		stack.top1 += extra;
		stack.used += extra;
		if ( type == UCOL ) {
		    stack.top1 += extra;   /* Add same amount for USUB */
		    stack.used += extra;
		}
		
	    } /* if ... */

	} /* else ... */
    }

    expanders[type].size = new_len;
    *prev_len = new_len;
    if ( no_expand ) ++no_expand;
    
    return (void *) expanders[type].mem;
    
} /* zexpand */


/*! \brief Compress the work[] array to remove fragmentation.
 */
void
zStackCompress(GlobalLU_t *Glu)
{
    register int iword, dword, ndim;
    char    *last, *fragment;
    int      *ifrom, *ito;
    doublecomplex   *dfrom, *dto;
    int      *xlsub, *lsub, *xusub, *usub, *xlusup;
    doublecomplex   *ucol, *lusup;
    
    iword = sizeof(int);
    dword = sizeof(doublecomplex);
    ndim = Glu->n;

    xlsub  = Glu->xlsub;
    lsub   = Glu->lsub;
    xusub  = Glu->xusub;
    usub   = Glu->usub;
    xlusup = Glu->xlusup;
    ucol   = Glu->ucol;
    lusup  = Glu->lusup;
    
    dfrom = ucol;
    dto = (doublecomplex *)((char*)lusup + xlusup[ndim] * dword);
    copy_mem_doublecomplex(xusub[ndim], dfrom, dto);
    ucol = dto;

    ifrom = lsub;
    ito = (int *) ((char*)ucol + xusub[ndim] * iword);
    copy_mem_int(xlsub[ndim], ifrom, ito);
    lsub = ito;
    
    ifrom = usub;
    ito = (int *) ((char*)lsub + xlsub[ndim] * iword);
    copy_mem_int(xusub[ndim], ifrom, ito);
    usub = ito;
    
    last = (char*)usub + xusub[ndim] * iword;
    fragment = (char*) (((char*)stack.array + stack.top1) - last);
    stack.used -= (long int) fragment;
    stack.top1 -= (long int) fragment;

    Glu->ucol = ucol;
    Glu->lsub = lsub;
    Glu->usub = usub;
    
#ifdef DEBUG
    printf("zStackCompress: fragment %d\n", fragment);
    /* for (last = 0; last < ndim; ++last)
	print_lu_col("After compress:", last, 0);*/
#endif    
    
}

/*! \brief Allocate storage for original matrix A
 */
void
zallocateA(int n, int nnz, doublecomplex **a, int **asub, int **xa)
{
    *a    = (doublecomplex *) doublecomplexMalloc(nnz);
    *asub = (int *) intMalloc(nnz);
    *xa   = (int *) intMalloc(n+1);
}


doublecomplex *doublecomplexMalloc(int n)
{
    doublecomplex *buf;
    buf = (doublecomplex *) SUPERLU_MALLOC((size_t)n * sizeof(doublecomplex)); 
    if ( !buf ) {
	ABORT("SUPERLU_MALLOC failed for buf in doublecomplexMalloc()\n");
    }
    return (buf);
}

doublecomplex *doublecomplexCalloc(int n)
{
    doublecomplex *buf;
    register int i;
    doublecomplex zero = {0.0, 0.0};
    buf = (doublecomplex *) SUPERLU_MALLOC((size_t)n * sizeof(doublecomplex));
    if ( !buf ) {
	ABORT("SUPERLU_MALLOC failed for buf in doublecomplexCalloc()\n");
    }
    for (i = 0; i < n; ++i) buf[i] = zero;
    return (buf);
}


int zmemory_usage(const int nzlmax, const int nzumax, 
		  const int nzlumax, const int n)
{
    register int iword, dword;

    iword   = sizeof(int);
    dword   = sizeof(doublecomplex);
    
    return (10 * n * iword +
	    nzlmax * iword + nzumax * (iword + dword) + nzlumax * dword);

}
