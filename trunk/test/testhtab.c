#include "_yhashtab.h"

int 
main(void)
{
	_htab *ht;
	_hitem *it;
	
	ht = htcreate(1);
	
	
	it = hadd(ht, 1, 1);
	it = hadd(ht, 2, 2);
	hfree(ht, it);
	hdisp(ht);
	//printf("hadd it:%x\n", it);
	//printf("hadd it val:%d\n", it->val);
	it = hadd(ht, 3, 3);
	it = hadd(ht, 4, 4);
	it = hadd(ht, 5, 5);
	hfree(ht, it);

	hdisp(ht);
	return 0;
	
	hadd(ht, 1, 1);
	it = hfind(ht, 1);
	hfree(ht, it);
	hadd(ht, 9, 9);
	hadd(ht, 3, 3);
	hadd(ht, 1, 1);
	it = hfind(ht, 3);
	hfree(ht, it);
	hsanity(ht);
	hadd(ht, 4, 4);
	hadd(ht, 5, 5);
	hadd(ht, 6, 6);
	
	
	
	it = hfind(ht, 5);
	hfree(ht, it);
	hsanity(ht);	
	it = hfind(ht, 1);
	hsanity(ht);
	
	hfree(ht, it);
	hsanity(ht);	
	hadd(ht, 1, 1);
	hsanity(ht);
	hadd(ht, 5, 5);
	hsanity(ht);	
	it = hfind(ht, 5);
	it = hfind(ht, 5);
	it = hfind(ht, 1);
	it = hfind(ht, 1);
	it = hfind(ht, 1);	
	hsanity(ht);
	it = hfind(ht, 9);
	hfree(ht, it);
	hsanity(ht);
	hadd(ht, 2, 2);
	hsanity(ht);
	hdisp(ht);
	htdestroy(ht);
	return 0;
}
