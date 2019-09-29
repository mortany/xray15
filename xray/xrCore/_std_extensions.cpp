#include "stdafx.h"


#include <time.h>

#ifdef BREAK_AT_STRCMP
int								xr_strcmp				( const char* S1, const char* S2 )
{
#ifdef DEBUG_MEMORY_MANAGER
	Memory.stat_strcmp	++;
#endif // DEBUG_MEMORY_MANAGER
	int res				= (int)strcmp(S1,S2);
	return				res;
}
#endif

TCHAR*	timestamp				(string64& dest)
{
	string64	temp;

	/* Set time zone from TZ environment variable. If TZ is not set,
	* the operating system is queried to obtain the default value 
	* for the variable. 
	*/
	_tzset		();
	u32			it;

	// date
	_wstrdate	( temp );
	for (it=0; it<xr_strlen(temp); it++)
		if ('/'==temp[it]) temp[it]='-';
	strconcat	(sizeof(dest), dest, temp, TEXT("_") );

	// time
	_wstrtime	( temp );
	
	for (it=0; it<xr_strlen(temp); it++)
		if (':'==temp[it]) temp[it]='-';
	wcscat( dest, temp);
	
	return dest;
}

