#include <common.h>
#include <asm.h>

void 
putchar (char ch)
{
	u8 data;

	if (ch == '\n')
		putchar ('\r');

	do {
		asm_in8 (0x3f8+5, &data);
	} while (!(data & 0x20));

	asm_out8 (0x3f8, (u8) ch);
}
