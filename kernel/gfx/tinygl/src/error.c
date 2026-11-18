#include <stdarg.h>
#include "zgl.h"

void gl_fatal_error(char *format, ...)
{
  char buf[256];
  va_list ap;

  va_start(ap,format);
  kvsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);

  kprintf("TinyGL: fatal error: %s\n", buf);
  __builtin_trap();
}
