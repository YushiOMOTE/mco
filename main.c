#include <common.h>
#include <asm.h>
#include <timer.h>

#define VERSION "3.0"

void
main(void)
{
	printf ("KVI " VERSION "\n");
	usleep (1ULL*1000ULL*1000ULL);
	printf ("1 ");
	nsleep (1ULL*1000ULL*1000ULL*1000ULL);
	printf ("2 ");
	usleep (1ULL*1000ULL*1000ULL);
	printf ("3 ");
	nsleep (1ULL*1000ULL*1000ULL*1000ULL);
	printf ("\n");
}
