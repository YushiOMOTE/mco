#include <common.h>
#include <acpi.h>
#include <asm.h>

int 
pmt_get_counter_width (void)
{
	return 24;
}

u64
pmt_convert_to_usec (u64 count)
{
	return ((count * 1000000) / 3579545);
}

u64
pmt_get_count (void)
{
	u16 base;
	u32 t;

	base = acpi_get_pmt_timer_iobase ();
	if (!base)
		return -1;

	asm_in32 (base, &t);
	t &= 0xffffff;

	return t;
}
