/*
*    An Hash Table with following attributes:
*
*	   - Lazy deletion
*	   - Separate chaining
*	   - move-to-front heuristic according
*	   - move-to-end heuristic according to free state of the item(not %100)
*	   - special integer hash for better uniformity on pointer keys.
*	   - load factor of 0.75 ( inspired from Java.HashMap)
*	   - not concurrent
*
*	Important:
*		Please do not hold direct references to the _hitem * objects.
*		These objects can be changed by the hash table resizing code and
*		client code will end up having dangling pointers. The obvious way
*		to use hash table is to look up the table and do stuff with the
*		returned _hitem object before another call is made to hadd() from
*		our thread or other thread.
*
*
*    Sumer Cip 2009
*/

#include "_yhashtab.h"
#include "_ydebug.h"
#include "_ymem.h"

static int
_hgrow(_htab *ht)
{
    int i;       
    _htab *dummy; 
    _hitem *p, *next, *it;   

    dummy = htcreate(ht->logsize+1);
    if (!dummy)
		return 0;
    for(i=0;i<ht->realsize;i++) {
    	p = ht->_table[i];
        while(p) {
            next = p->next;
			if (!hadd(dummy, p->key, p->val))
				return 0;
			it = hfind(dummy, p->key);
			if (!it)
				return 0;
			it->accesscount = p->accesscount;
			it->free = p->free;
            yfree(p);           
            p = next;
        }
    }       
   
    yfree(ht->_table);   
    ht->_table = dummy->_table;
    ht->logsize = dummy->logsize;
    ht->realsize = dummy->realsize;
    ht->mask = dummy->mask;
    yfree(dummy);
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
   
    ht = (_htab *)ymalloc(sizeof(_htab));
    if (!ht)
		return NULL;
    ht->logsize = logsize;
    ht->realsize = HSIZE(logsize);
    ht->mask = HMASK(logsize);
    ht->count = 0;
    ht->freecount = 0;
    ht->_table = (_hitem **)ymalloc(ht->realsize * sizeof(_hitem *));
    if (!ht->_table) {
    	yfree(ht);
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
            yfree(p);
            p = next;
        }
    }    
    yfree(ht->_table);
    yfree(ht);
}


int
hadd(_htab *ht, int key, int val)
{
    int h;
    _hitem *p, *bucketend, *inew;
   
    h = _hhash(ht, key);
    for(inew = NULL, p = bucketend = ht->_table[h]; p; bucketend = p, p = p->next) {
	    if ((p->key == key) && (!p->free)) // check if key already inserted.
            return 0;    
        if ((p->free) && (!inew)) // get the first free item.
            inew = p;
	}
	
    if (inew) {
		INITITEM(inew, key, val);
        ht->freecount--;        
    } else {       
        inew = (_hitem *)ymalloc(sizeof(_hitem));
        if (!inew)
        	return 0;
        INITITEM(inew, key, val);
		if (!bucketend) {
			// add to front
			inew->next = NULL; 
			ht->_table[h] = inew;
        } else {
			bucketend->next = inew;
			inew->next = NULL;
		}
		ht->count++;
    }
	// need resizing?   
    if (((ht->count - ht->freecount) / (double)ht->realsize) >= HLOADFACTOR) {
#ifdef YDEBUG
		yprint("grow needed because load factor: (%d, %d, %d) %0.6f.\n",
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
   
	for (prev = p, p = p->next; p; prev = p, p = p->next) {
        if ((p->key == key) && (!p->free)) {
			p->accesscount++;
			// accesscount is smaller then swap with it. if prev item
			// is free then also swap to help free item to move towards the end
			// of the bucket to reduce cache misses.
			if ((prev->accesscount < p->accesscount) || (prev->free))
				SWAPITEM(prev, p);
			return prev;
        }   
    }
    return NULL;
}


void
henum(_htab *ht, int (*enumfn)(_hitem *item, void *arg), void *arg)
{
// enums non-free items
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
	_hitem *next;
	
    item->free = 1;
    ht->freecount++;

    // help a little bit to reduce cache misses by moving the free item closer to the
    // end of the bucket, if possible.
    next = item->next;
    if (next) {
    	if (!next->free) {
    		SWAPITEM(next, item);
    	}
    }

}


#ifdef YDEBUG
void
hsanity(_htab *ht)
{
// at every point, the table must be like following:
//  - accesscounts shall be found ordered per-bucket.
//  - nitems/ht->realsize must be smaller than 0.75.
	int i;
	_hitem *p, *next;

	for(i=0;i<ht->realsize;i++) {
		p = ht->_table[i];
		if (!p)
			continue;
		next = p->next;
		while((p) && (next)) {
			//printf("vals:(%d,%d)(%x, %x)\n", p->val, next->val, p, next);
			if ((!p->free) && (!next->free) )
				assert(p->accesscount >= next->accesscount);
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
			yprint(" - bucket item %d : %x(value:%d)(accesscount:%d)(free:%d)", 
				j, (uintptr_t)p, 
				p->val, 
				p->accesscount, 
				p->free);
			p = p->next;
			j++;
		}
		yprint("\n");
	}
}
#endif