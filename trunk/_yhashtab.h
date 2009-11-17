/*
*    An Adaptive Hash Table
*    Sumer Cip 2009
*/

#ifndef YHASHTAB_H
#define YHASHTAB_H

#ifndef _MSC_VER
#include "stdint.h"
#endif
#include "assert.h"
#include "stdlib.h"

#define HSIZE(n) (1<<n)
#define HMASK(n) (HSIZE(n)-1)
#define SWAP(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))
#define SWAPITEM(x, y) ( {\
			SWAP(x->key, y->key); \
            SWAP(x->val, y->val);  \
            SWAP(x->free, y->free); \
			SWAP(x->accesscount, y->accesscount); \
})

#define HLOADFACTOR 0.75

struct _hitem{
    int key;
    int val;
    int free; // for recycling.
	int accesscount;
    struct _hitem *next;
};
typedef struct _hitem _hitem;

typedef struct{
    int realsize;
    int logsize;
    int count;
    int mask;
    int freecount;
    _hitem ** _table;
}_htab;

_htab *htcreate(int logsize);
void htdestroy(_htab *ht);
_hitem *hfind(_htab *ht, int key);
int hadd(_htab *ht, int key, int val);
void henum(_htab *ht, int (*fn) (_hitem *item, void *arg), void *arg);
int hcount(_htab *ht);
void hfree(_htab *ht, _hitem *item);

void hdisp(_htab *ht);

#endif
