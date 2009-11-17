/*
*    An Hash Table with following attributes:
*	   - Lazy deletion
*	   - Separate chaining
*	   - move-to-front heuristic
*	   - special integer hash for better uniformity on pointer keys.
*	   - load factor of 0.75 ( inspired from Java.HashMap)
*
*    Sumer Cip 2009
*/

#include "_yhashtab.h"
#include "_ydebug.h"

static int
_hgrow(_htab *ht)
{
    int i;       
    _htab *dummy; 
    _hitem *p, *next;   

    dummy = htcreate(ht->logsize+1);
    if (!dummy)
		return 0;
    for(i=0;i<ht->realsize;i++) {
        p = ht->_table[i];       
        while(p) {
            next = p->next;
            if (!hadd(dummy, p->key, p->val))
            	return 0;
            free(p);           
            p = next;
        }
    }       
   
    free(ht->_table);   
    ht->_table = dummy->_table;
    ht->logsize = dummy->logsize;
    ht->realsize = dummy->realsize;
    ht->mask = dummy->mask;
    free(dummy);
    return 1;
}

unsigned int
_hhash(_htab *ht, int a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return (unsigned int)(a & ht->mask);
}


_htab *
htcreate(int logsize)
{
    int i;
    _htab *ht;
   
    ht = (_htab *)malloc(sizeof(_htab));
    if (!ht)
		return NULL;
    ht->logsize = logsize;
    ht->realsize = HSIZE(logsize);
    ht->mask = HMASK(logsize);
    ht->count = 0;
    ht->freecount = 0;
    ht->_table = (_hitem **)malloc(ht->realsize * sizeof(_hitem *));
    if (!ht->_table) {
    	free(ht);
    	return NULL;
    }

    for(i=0;i<ht->realsize;i++)
        ht->_table[i] = NULL;    
           
    return ht;
}


void
htdestroy(_htab *ht)
{
    int i;
    _hitem *p, *next;
   
    for(i=0;i<ht->realsize;i++) {
        p = ht->_table[i];
        while(p) {
            next = p->next;           
            free(p);
            p = next;
        }
    }
    
    free(ht->_table);
    free(ht);
}


int
hadd(_htab *ht, int key, int val)
{
    int h;
    _hitem *new,*p,*bucketend;
   
    h = _hhash(ht, key);
    bucketend = p = ht->_table[h];
    new = NULL;
    while(p) {
        if ((p->key == key) && (!p->free))
            return 0;    
        if ((p->free) && (!new))
            new = p;  
		bucketend = p;
        p = p->next;
    }       
    // have a free slot?
    if (new) {
		new->key = key;
        new->val = val;
        new->free = 0;
		new->accesscount = 0;		
        ht->freecount--;        
    } else {       
        new = (_hitem *)malloc(sizeof(_hitem));
        if (!new)
        	return 0;
        new->key = key;
        new->val = val;
        new->free = 0;
		new->accesscount = 0;
		if (!bucketend) {
			new->next = ht->_table[h]; // add to front
			ht->_table[h] = new;
        } else {
			bucketend->next = new;
			new->next = NULL;
		}
		ht->count++;
    }
	// need resizing?   
    if (  ((ht->count - ht->freecount) / (double)ht->realsize) >= HLOADFACTOR) {
		
		#if 0
		printf("grow needed because load factor: (%d, %d, %d) %0.6f.\n", 
			ht->count, 
			ht->freecount, 
			ht->realsize, 
			((ht->count - ht->freecount) / (double)ht->realsize));
		#endif
		
        if (!_hgrow(ht))
			return 0;
    }
    return 1;
}


_hitem *
hfind(_htab *ht, int key)
{
    int h;    
    _hitem *p, *prev;
       
    h = _hhash(ht, key);
    p = ht->_table[h];
    if (!p)
        return NULL;
    if ((p->key == key) && (!p->free)) {
		p->accesscount++;
        return p;
	}
   
    prev = p;
    p=p->next;
    while(p) {
        if ((p->key == key) && (!p->free)) {
			p->accesscount++;            
			if (prev->accesscount < p->accesscount)
				SWAPITEM(prev, p);
			return prev;
        }   
        prev = p;
        p = p->next;
    }
    return NULL;
}

// enums non-free items
void
henum(_htab *ht, int (*enumfn)(_hitem *item, void *arg), void *arg)
{
    int rc, i;
    _hitem *p, *next;
   
    for(i=0;i<ht->realsize;i++) {       
        p = ht->_table[i];
        while(p) {
            next = p->next;
			if (!p->free) {
				rc = enumfn(p, arg); // item may be freed.
				if(rc)
					return;
			}
            p = next;       
        }
    }       
}

int
hcount(_htab *ht)
{
    return (ht->count - ht->freecount);
}

void
hfree(_htab *ht, _hitem *item)
{
	_hitem *bend, *p;
	
    item->free = 1;
    ht->freecount++;
	
	// should move the freed item to the end to avoid
	// redundant cache misses.
	bend = NULL;
	p = item->next;
	while(p) {
		bend = p;
		p = p->next;
	}
	if (bend)
		SWAPITEM(bend, item);
}

// at every point, the table must be like following:
//  - free items are always at the end of the bucket.
//  - accesscounts shall be found ordered per-bucket.
//  - nitems/ht->realsize must be smaller than 0.75.
void
hsanity(_htab *ht)
{
	int i;
	_hitem *p, *next;

	for(i=0;i<ht->realsize;i++) {
		p = ht->_table[i];
		if (!p)
			continue;
		next = p->next;
		while((p) && (next)) {
			//printf("vals:(%d,%d)(%x, %x)\n", p->val, next->val, p, next);
			if (p->free) { // if one item is free then next item should be free, too.

				assert(next->free);
			} else if (!next->free){
				assert(p->accesscount >= next->accesscount);
			}
			p = next;
			next = p->next;
		}
	}
	assert(((ht->count - ht->freecount) / (double)ht->realsize) < HLOADFACTOR);
}

void 
hdisp(_htab *ht)
{
	int i,j;
	_hitem *p;

	for(i=0;i<ht->realsize;i++) {
		p = ht->_table[i];
		yprint("row %d : ", i);
		j = 0;
		while(p) {
			yprint(" - bucket item %d : %x(value:%d)(accesscount:%d)(free:%d)", j, (uintptr_t)p, p->val, p->accesscount, p->free);
			p = p->next;
			j++;
		}
	}
}
