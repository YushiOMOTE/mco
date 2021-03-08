#include <common.h>
#include <acpi.h>
#include <drivers/hpet.h>
#include <drivers/pmt.h>

void 
nsleep (u64 nsec)
{
	int width;
	u64 elapsed;
	u64 time0, time1;

	elapsed = 0;
	time0 = hpet_get_count ();
	width = hpet_get_counter_width ();
	do {
		time1 = hpet_get_count ();
		if (time1 == time0)
			continue;
		if (time1 < time0) {
			elapsed += ((1<<width) - time1);
			elapsed += time0;
		} else {
			elapsed += (time1 - time0);
		}
		time0 = time1;
	} while (hpet_convert_to_nsec (elapsed) < nsec);
}

void 
usleep (u64 usec)
{
	int width;
	u64 elapsed;
	u32 time0, time1;

	elapsed = 0;
	time0 = hpet_get_count ();
	width = hpet_get_counter_width ();
	do {
		time1 = hpet_get_count ();
		if (time1 == time0)
			continue;
		if (time1 < time0) {
			elapsed += ((1<<width) - time1);
			elapsed += time0;
		} else {
			elapsed += (time1 - time0);
		}
		time0 = time1;
	} while (hpet_convert_to_nsec (elapsed) / 1000 < usec);
}
