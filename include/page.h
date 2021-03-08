#ifndef __PAGE_H
#define __PAGE_H

#define PAGESIZE 4096

void *page_alloc (int pages);
int page_free (void *p, int pages);

#endif	/* __PAGE_H */
