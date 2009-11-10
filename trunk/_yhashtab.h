/*
*    An Adaptive Hash Table
*    Sumer Cip 2009
*/

#ifndef YHASHTAB_H
#define YHASHTAB_H

#define HSIZE(n) (1<<n)
#define HMASK(n) (HSIZE(n)-1)
#define SWAP(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))

struct _hitem{
    int key;
    int val;
    int free; // for recycling.
    struct _hitem *next;
};
typedef struct _hitem _hitem;

typedef struct{
    int realsize;
    int logsize;
    int count;
    int mask;
    int growfactor;
    int freecount;
    _hitem ** _table;
}_htab;

_htab *htcreate(int logsize, int growfactor);
void htdestroy(_htab *ht);
_hitem *hfind(_htab *ht, int key);
int hadd(_htab *ht, int key, int val);
void henum(_htab *ht, int (*fn) (_hitem *item, void *arg), void *arg);
int hcount(_htab *ht);
void hfree(_htab *ht, _hitem *item);

#endif
