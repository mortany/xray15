#ifndef STRING_CONCATENATIONS_H
#define STRING_CONCATENATIONS_H

#ifndef _EDITOR

LPTSTR	XRCORE_API				strconcat				( int dest_sz, TCHAR* dest, LPCTSTR S1, LPCTSTR S2);
LPTSTR	XRCORE_API				strconcat				( int dest_sz, TCHAR* dest, LPCTSTR S1, LPCTSTR S2, LPCTSTR S3);
LPTSTR	XRCORE_API				strconcat				( int dest_sz, TCHAR* dest, LPCTSTR S1, LPCTSTR S2, LPCTSTR S3, LPCTSTR S4);
LPTSTR	XRCORE_API				strconcat				( int dest_sz, TCHAR* dest, LPCTSTR S1, LPCTSTR S2, LPCTSTR S3, LPCTSTR S4, LPCTSTR S5);
LPTSTR	XRCORE_API				strconcat				( int dest_sz, TCHAR* dest, LPCTSTR S1, LPCTSTR S2, LPCTSTR S3, LPCTSTR S4, LPCTSTR S5, LPCTSTR S6);

#else // _EDITOR
// obsolete: should be deleted as soon borland work correctly with new strconcats
IC TCHAR*						strconcat				( int dest_sz,  TCHAR* dest, LPCTSTR S1, LPCTSTR S2)
{	return strcat(strcpy(dest,S1),S2); }

// dest = S1+S2+S3
IC TCHAR*						strconcat				( int dest_sz,  TCHAR* dest, LPCTSTR S1, LPCTSTR S2, LPCTSTR S3)
{	return strcat(strcat(strcpy(dest,S1),S2),S3); }

// dest = S1+S2+S3+S4
IC TCHAR*						strconcat				( int dest_sz,  TCHAR* dest, LPCTSTR S1, LPCTSTR S2, LPCTSTR S3, LPCTSTR S4)
{	return strcat(strcat(strcat(strcpy(dest,S1),S2),S3),S4); }

// dest = S1+S2+S3+S4+S5
IC TCHAR*						strconcat				( int dest_sz,  TCHAR* dest, LPCTSTR S1, LPCTSTR S2, LPCTSTR S3, LPCTSTR S4, LPCTSTR S5)
{	return strcat(strcat(strcat(strcat(strcpy(dest,S1),S2),S3),S4),S5); }

// dest = S1+S2+S3+S4+S5+S6
IC TCHAR*						strconcat				( int dest_sz,  TCHAR* dest, LPCTSTR S1, LPCTSTR S2, LPCTSTR S3, LPCTSTR S4, LPCTSTR S5, LPCTSTR S6)
{	return strcat(strcat(strcat(strcat(strcat(strcpy(dest,S1),S2),S3),S4),S5),S6); }

#endif

// warning: do not comment this macro, as stack overflow check is very light 
// (consumes ~1% performance of STRCONCAT macro)
#ifndef _EDITOR
#define STRCONCAT_STACKOVERFLOW_CHECK

#ifdef STRCONCAT_STACKOVERFLOW_CHECK

#define STRCONCAT(dest, ...) \
	do { \
	xray::core::detail::string_tupples	STRCONCAT_tupples_unique_identifier(__VA_ARGS__); \
	u32 STRCONCAT_buffer_size = STRCONCAT_tupples_unique_identifier.size(); \
	xray::core::detail::check_stack_overflow(STRCONCAT_buffer_size); \
	(dest) = (LPTSTR)_alloca(STRCONCAT_buffer_size); \
	STRCONCAT_tupples_unique_identifier.concat	(dest); \
	} while (0)

#else //#ifdef STRCONCAT_STACKOVERFLOW_CHECK

#define STRCONCAT(dest, ...) \
	do { \
	xray::core::detail::string_tupples	STRCONCAT_tupples_unique_identifier(__VA_ARGS__); \
	(dest)		       = (LPTSTR)_alloca(STRCONCAT_tupples_unique_identifier.size()); \
	STRCONCAT_tupples_unique_identifier.concat	(dest); \
	} while (0)

#endif //#ifdef STRCONCAT_STACKOVERFLOW_CHECK

#endif //_EDITOR
#include "string_concatenations_inline.h"


#endif // #ifndef STRING_CONCATENATIONS_H