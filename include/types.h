#ifndef __TYPES_H
#define __TYPES_H

#define NULL ((void *)0)
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned long int	size_t;
typedef unsigned long int	addr_t;
typedef unsigned long int	virt_t;
typedef unsigned long long int	phys_t;
typedef enum {
	false = 0,
	true = 1,
} bool;

#endif	/* __TYPES_H */
