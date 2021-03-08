#ifndef __PRINTF_H
#define __PRINTF_H

int printf (const char *format, ...)
	__attribute__ ((format (printf, 1, 2)));
int vprintf (const char *format, va_list ap);
int snprintf (char *str, size_t size, const char *format, ...)
	__attribute__ ((format (printf, 3, 4)));
int vsnprintf (char *str, size_t size, const char *format, va_list ap);

#endif	/* __PRINTF_H */
