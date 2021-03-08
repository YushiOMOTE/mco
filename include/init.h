#ifndef __INIT_H
#define __INIT_H

#define register_init(id, func) struct init_data __init_func_##func	\
	__attribute__ ((__section__ (".init_func"), aligned (1))) = {	\
		__FILE__,						\
		id,							\
		func							\
	}
#define register_deinit(id, func) struct init_data __init_func_##func	\
	__attribute__ ((__section__ (".deinit_func"), aligned (1))) = {	\
		__FILE__,						\
		id,							\
		func							\
	}

struct init_data {
	char *filename;
	char *id;
	void (*func) (void);
} __attribute__ ((packed));

#endif	/* __INIT_H */
