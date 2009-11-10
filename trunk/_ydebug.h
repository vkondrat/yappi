#ifndef YDEBUG_H
#define YDEBUG_H
#include "stdio.h"

#ifdef DEBUG_CALL
#define dprintf(fmt, args...) fprintf(stderr, "[&] [yappi-dbg] " fmt "\n", ## args)
#else
#define dprintf(fmt, args...)
#endif

#define yerr(fmt, args...) fprintf(stderr, "[*]	[yappi-err]	" fmt "\n", ## args)
#define yinfo(fmt, args...) fprintf(stderr, "[+] [yappi-info] " fmt "\n", ## args)
#define yprint(fmt, args...) fprintf(stderr, fmt,  ## args)


#endif
