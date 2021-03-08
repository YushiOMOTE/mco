#ifndef __HPET_H
#define __HPET_H

int hpet_get_counter_width (void);
u64 hpet_convert_to_nsec (u64 count);
u64 hpet_get_count (void);

#endif	/* __HPET_H */
