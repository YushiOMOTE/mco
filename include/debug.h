#ifndef __DEBUG_H
#define __DEBUG_H

#if defined(__DEBUG_PRINTD) || defined(__DEBUG_PRINT)
#define printd(X...) do { printf(X); } while (0)
#else
#define printd(X...) do { } while (0)
#endif
#if defined(__DEBUG_PRINTW) || defined(__DEBUG_PRINT)
#define printw(X...) do { printf(X); } while (0)
#else
#define printw(X...) do { } while (0)
#endif
#if defined(__DEBUG_PRINTE) || defined(__DEBUG_PRINT)
#define printe(X...) do { printf(X); } while (0)
#else
#define printe(X...) do { } while (0)
#endif
#if defined(__DEBUG_PRINTI) || defined(__DEBUG_PRINT)
#define printi(X...) do { printf(X); } while (0)
#else
#define printi(X...) do { } while (0)
#endif

#endif	/* __DEBUG_H */
