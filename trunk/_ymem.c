#include "_ymem.h"

void *
ymalloc(size_t size)
{	
	void *p = malloc(size);
	if (!p) {
		yerr("malloc(%d) failed. No memory?", size);
		return NULL;
	}
#ifdef DEBUG_MEM
	if (dhead)
		dprintf("_ymalloc(%d) called[%p].[old_head:%p]", size, p, dhead->ptr);
	else
		dprintf("_ymalloc(%d) called[%p].[old_head:nil]", size, p);
	dnode_t *v = malloc(sizeof(dnode_t));
	v->ptr = p;
	v->size = size;
	v->next = dhead;
	dhead = v;
	dsize++;	
#endif
	return p;
}

void 
yfree(void *p)
{
#ifdef DEBUG_MEM
	dnode_t *v = dhead;
	dnode_t *prev = NULL;	
	while(v) {
		if (v->ptr == p) {
			if (prev)
				prev->next = v->next;
			else 				
				dhead = v->next;
			
			dprintf("_yfree(%p) called.", p);
			free(v);
			dsize--;
			break;	
		}
		prev = v;
		v = v->next;
	}
#endif
	free(p);
}
