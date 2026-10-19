#include <cstdarg>
#include <cstdio>

void vku_internal_printf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	std::vprintf(fmt, args);
	va_end(args);
	std::fflush(stdout);
}
