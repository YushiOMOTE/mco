#ifndef __STRING_H
#define __STRING_H

/* FIXME: Use SSE. */

static inline void *
memset (void *addr, int val, int len)
{
	char *p;

	p = addr;
	while (len--)
		*p++ = val;
	return addr;
}

static inline void *
memcpy (void *dest, void *src, int len)
{
	return __builtin_memcpy (dest, src, len);
}

static inline int
strcmp (char *s1, char *s2)
{
	int r, c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;
		r = c1 - c2;
	} while (!r && c1);

	return r;
}

static inline int
memcmp (void *p1, void *p2, int len)
{
	int r, i;
	char *q1, *q2;

	q1 = p1;
	q2 = p2;
	for (r = 0, i = 0; !r && i < len; i++)
		r = *q1++ - *q2++;
	return r;
}

static inline int
strlen (char *p)
{
	return __builtin_strlen (p);
}

#endif	/* __STRING_H */
