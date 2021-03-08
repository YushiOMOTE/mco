#ifndef __PANIC_H
#define __PANIC_H

#define panic(X...) do { printf(X); while (1); } while (0)

#endif	/* __PANIC_H */
