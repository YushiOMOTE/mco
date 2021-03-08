#ifndef __MMIO_H
#define __MMIO_H

#define MMIO_BASE 0x8000000000ULL

static inline void 
mmio_read8 (u64 phys, u8 *val)
{
	*val = *(u8 *)(void *)(MMIO_BASE | phys);
}

static inline void 
mmio_read16 (u64 phys, u16 *val)
{
	*val = *(u16 *)(void *)(MMIO_BASE | phys);
}

static inline void 
mmio_read32 (u64 phys, u32 *val)
{
	*val = *(u32 *)(void *)(MMIO_BASE | phys);
}

static inline void
mmio_read64 (u64 phys, u64 *val)
{
	*val = *(u64 *)(void *)(MMIO_BASE | phys);
}

static inline void 
mmio_write8 (u64 phys, u8 val)
{
	*(u8 *)(void *)(MMIO_BASE | phys) = val;
}

static inline void 
mmio_write16 (u64 phys, u16 val)
{
	*(u16 *)(void *)(MMIO_BASE | phys) = val;
}

static inline void 
mmio_write32 (u64 phys, u32 val)
{
	*(u32 *)(void *)(MMIO_BASE | phys) = val;
}

static inline void 
mmio_write64 (u64 phys, u64 val)
{
	*(u64 *)(void *)(MMIO_BASE | phys) = val;
}

#endif	/* __MMIO_H */
