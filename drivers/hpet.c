#define __DEBUG_PRINT
#include <common.h>
#include <mmio.h>
#include <acpi.h>
#include <init.h>
#include <drivers/hpet.h>

#define HPET_REG_CAP     0x000
#define HPET_REG_PERIOD  0x004
#define HPET_REG_CONFIG  0x010
#define HPET_REG_COUNTER_LOW  0x0f0
#define HPET_REG_COUNTER_HIGH 0x0f4

#define HPET_REG_CAP_64W 0x2000
#define HPET_REG_CONFIG_ENABLE 0x1

static int hpet_counter_width;
static u64 hpet_period;

int
hpet_get_counter_width (void)
{
	return hpet_counter_width;
}

u64
hpet_convert_to_nsec (u64 count)
{
	return ((count * hpet_period) / 1000000);
}

u64
hpet_get_count (void)
{
	u64 base;
	u64 value;

	base = acpi_get_hpet_mmiobase ();

	value = 0;
	if (hpet_counter_width == 64) {
		mmio_read64 (base + HPET_REG_COUNTER_LOW, &value);
	} else {
		mmio_read32 (base + HPET_REG_COUNTER_LOW, 
			     (u32 *)&value);
	}

	return value;
}

static void 
hpet_init (void)
{
	u32 value;
	u64 base;

	base = acpi_get_hpet_mmiobase ();

	mmio_read32 (base + HPET_REG_CAP, &value);
	if (value & HPET_REG_CAP_64W) {
		hpet_counter_width = 64;
	} else {
		hpet_counter_width = 32;
	}

	mmio_read32 (base + HPET_REG_CONFIG, &value);
	value &= ~HPET_REG_CONFIG_ENABLE;
	mmio_write32 (base + HPET_REG_CONFIG, value);

	mmio_read32 (base + HPET_REG_PERIOD, &value);
	hpet_period = value;

	mmio_write32 (base + HPET_REG_COUNTER_LOW, 0);
	mmio_write32 (base + HPET_REG_COUNTER_HIGH, 0);

	mmio_read32 (base + HPET_REG_CONFIG, &value);
	value |= HPET_REG_CONFIG_ENABLE;
	mmio_write32 (base + HPET_REG_CONFIG, value);

	printd ("HPET: Initialized (Period: %llx, Width: %d)\n",
		hpet_period, hpet_counter_width);
}

static void 
hpet_deinit (void)
{

}

register_init ("bsp2", hpet_init);
register_deinit ("bsp2", hpet_deinit);
