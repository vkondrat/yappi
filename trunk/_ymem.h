#ifndef YMEM_H
#define YMEM_H

#include "stdlib.h"
#include "_ydebug.h"

struct dnode{
	void *ptr;
	unsigned int size;
	struct dnode *next; 
};
typedef struct dnode dnode_t; 

/* Globals */
dnode_t *dhead;
unsigned int dsize;


void *ymalloc(size_t size);
void yfree(void *p);
unsigned long ymemusage(void);

#ifdef DEBUG_MEM
#define YMEMCHECK() \
{ \
	dnode_t *v = dhead; \
	unsigned int tleak = 0;	\
	while(v) {	\
		fprintf(stderr, "[YMEM]	Leaked block: (addr:%p) (size:%d)\n", v->ptr, v->size);	\
		tleak += v->size;	\
		v = v->next;	\
	}	\
	if (tleak == 0)	\
		fprintf(stderr, "[YMEM]	Application currently has no leakage.[%d]\n", dsize);\
	else \
		fprintf(stderr, "[YMEM]	Application currently leaking %d bytes.[%d]\n", tleak, dsize); \
}
#else
#define YMEMCHECK()
#endif

#endif
