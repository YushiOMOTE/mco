#ifndef __LOCK_H
#define __LOCK_H

typedef u8 spinlock_t;

#define SPINLOCK_INITIALIZER ((spinlock_t)0)

static inline void
spinlock_lock (spinlock_t *l)
{
	u8 d;

	asm volatile ("3: \n"
		      " xchg  %1, %0 \n"
		      " test  %0, %0 \n"
		      " je    1f \n"
		      "2: \n"
		      " pause \n"
		      " cmp   %1, %0 \n"
		      " je    2b \n"
		      " jmp   3b \n"
		      "1: \n"
#ifdef __x86_64__
		      : "=r" (d)
#else
		      : "=abcd" (d)
#endif
		      , "+m" (*l)
		      : "0" ((u8)1)
		      : "cc");
}

static inline void
spinlock_unlock (spinlock_t *l)
{
	u8 d;

	asm volatile ("xchg %1, %0 \n"
#ifdef __x86_64__
		      : "=r" (d)
#else
		      : "=abcd" (d)
#endif
		      , "+m" (*l)
		      : "0" ((u8)0));
}

static inline void
spinlock_init (spinlock_t *l)
{
	spinlock_unlock (l);
}

#endif	/* __LOCK_H */
