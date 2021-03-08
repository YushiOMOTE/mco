#ifndef __ALLOC_H
#define __ALLOC_H

struct boundary_tag {
	unsigned int magic;
	unsigned int size;
	unsigned int real_size;
	int index;
	struct boundary_tag *split_left;
	struct boundary_tag *split_right;
	struct boundary_tag *next;
	struct boundary_tag *prev;
};

void *malloc (size_t size);
void *realloc (void *p, size_t size);
void *calloc (size_t nobj, size_t size);
void free (void *p);

#endif	/* __ALLOC_H */
