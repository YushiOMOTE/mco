#include <common.h>
#include <init.h>
#include <page.h>
#include <drivers/pci.h>
#include <string.h>

extern struct init_data __init_begin[], __init_end[];
extern struct init_data __deinit_begin[], __deinit_end[];

static void 
init_scan (char *id, 
	   struct init_data begin[], struct init_data end[])
{
	int len;
	struct init_data *p;

	len = strlen (id);
	for (p = begin; p != end; p++) {
		if (memcmp (p->id, id, len) == 0)
			p->func ();
	}
}

static void
init_do_init (char *id)
{
	init_scan (id,
		   __init_begin, __init_end);
}

static void 
init_do_deinit (char *id)
{
	init_scan (id,
		   __deinit_begin, __deinit_end);
}

static void
init_swap (struct init_data *p, struct init_data *q)
{
	struct init_data tmp;

	memcpy (&tmp, p,    sizeof (struct init_data));
	memcpy (p,    q,    sizeof (struct init_data));
	memcpy (q,    &tmp, sizeof (struct init_data));
}

static int
init_cmp (struct init_data *p, struct init_data *q)
{
	int diff;

	diff = strcmp (p->id, q->id);
	if (diff > 0)
		return 1;
	if (diff < 0)
		return 0;
	diff = strcmp (p->filename, q->filename);
	if (diff > 0)
		return 1;
	return 0;
}

static void
init_init (void)
{
	struct init_data *p, *q;

	for (p = __init_begin; p != __init_end; p++)
		for (q = p + 1; q != __init_end; q++)
			if (init_cmp (p, q))
				init_swap (p, q);
	for (p = __deinit_begin; p != __deinit_end; p++)
		for (q = p + 1; q != __deinit_end; q++)
			if (init_cmp (q, p))
				init_swap (p, q);
}

static char *init_list[] = {
	"bsp", 
	"driver", 
	"ap", 
	NULL,
};

void 
init (void)
{
	void *main (void);
	int i;

	printf ("System started.\n");

	init_init ();

	i = 0;
	while (init_list[i++]) {
		init_do_init (init_list[i - 1]);
	}
	main ();
	while (--i >= 0) {
		init_do_deinit (init_list[i]);
	}

	printf ("System stopped.\n");
}

void 
init_ap (u8 apic_id)
{
	printf ("AP: Processor %02x started.\n",
		(u8)apic_id);
}
