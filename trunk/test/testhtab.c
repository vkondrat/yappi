#include "_yhashtab.h"

int 
main(void)
{
	_htab *ht;
	_hitem *it, *it1, *it2;
	
	ht = htcreate(1);
	
	
	hadd(ht, 1, 1);
	hadd(ht, 2, 2);
	hadd(ht, 4, 4);
	
	it = hfind(ht, 1);
	it1 = hfind(ht, 2);
	hadd(ht, 5, 5);
	
	hfree(ht, it);
	hfree(ht, it1);
	
	hsanity(ht);
	hfind(ht, 5);
	hfind(ht, 5);

	hdisp(ht);
	htdestroy(ht);

	return 0;
}
