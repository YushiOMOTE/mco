#ifndef __PMT_H
#define __PMT_H

int pmt_get_counter_width (void);
u64 pmt_convert_to_usec (u64 counter);
u64 pmt_get_count (void);

#endif	/* __PMT_H */
