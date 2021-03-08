#ifndef __ASM_H
#define __ASM_H

static inline void
asm_in8 (u32 port, u8 *data)
{
	asm volatile ("inb %%dx,%%al"
		      : "=a" (*data)
		      : "d" (port));
}

static inline void
asm_in16 (u32 port, u16 *data)
{
	asm volatile ("inw %%dx,%%ax"
		      : "=a" (*data)
		      : "d" (port));
}

static inline void
asm_in32 (u32 port, u32 *data)
{
	asm volatile ("inl %%dx,%%eax"
		      : "=a" (*data)
		      : "d" (port));
}

static inline void
asm_out8 (u32 port, u8 data)
{
	asm volatile ("outb %%al,%%dx"
		      :
		      : "a" (data)
		      , "d" (port));
}

static inline void
asm_out16 (u32 port, u16 data)
{
	asm volatile ("outw %%ax,%%dx"
		      :
		      : "a" (data)
		      , "d" (port));
}

static inline void
asm_out32 (u32 port, u32 data)
{
	asm volatile ("outl %%eax,%%dx"
		      :
		      : "a" (data)
		      , "d" (port));
}

static inline void 
asm_rdtsc (u64 *a)
{
	u64 r;
	asm volatile ( "rdtsc" : "=A"(r) );
	*a = r;
}

static inline void 
asm_cpuid (u32 code, u32 subcode,
	   u32* a, u32* b, u32 *c, u32 *d)
{
	asm volatile ("cpuid"
		      : "=a" (*a), 
			"=b" (*b), 
			"=c" (*c), 
			"=d" (*d)
		      : "a" (code), 
			"c" (subcode));
}

static inline void
asm_rdmsr (u32 num, u64 *value)
{
	u32 a, d;

	asm volatile ("rdmsr"
		      : "=a" (a), "=d" (d)
		      : "c" (num));
	*value = (u64)a | ((u64)d << 32);
}

static inline void
asm_wrmsr (u32 num, u64 value)
{
	u32 a, d;

	a = (u32)value;
	d = (u32)(value >> 32);
	asm volatile ("wrmsr"
		      :
		      : "c" (num), "a" (a), "d" (d));
}

#endif	/* __ASM_H */
