// Most of below code is taken from cProfile module.  


#ifndef YTIMING_H
#define YTIMING_H


#include "Python.h"

#if !defined(HAVE_LONG_LONG)
#error "This module requires long longs!"
#endif

#ifdef MS_WINDOWS

#include <windows.h>

static long long
tickcount(void)
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

static double
tickfactor(void)
{
	LARGE_INTEGER li;
	if (QueryPerformanceFrequency(&li))
		return 1.0 / li.QuadPart;
	else
		return 0.000001;  /* unlikely */
}

#else /* !MS_WINDOWS */

#ifndef HAVE_GETTIMEOFDAY
#error "This module requires gettimeofday() on non-Windows platforms!"
#endif

#if (defined(PYOS_OS2) && defined(PYCC_GCC))
#include <sys/time.h>
#else
#include <sys/resource.h>
#include <sys/times.h>
#endif

static long long
tickcount(void)
{
	struct timeval tv;
	long long rc;
#ifdef GETTIMEOFDAY_NO_TZ
	gettimeofday(&tv);
#else
	gettimeofday(&tv, (struct timezone *)NULL);
#endif
	rc = tv.tv_sec;
	rc = rc * 1000000 + tv.tv_usec;
	return rc;
}

static double
tickfactor(void)
{
	return 0.000001;
}

#endif /* else */
#endif

