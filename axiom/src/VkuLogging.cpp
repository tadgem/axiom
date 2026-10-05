// vku routes its diagnostics through an application provided printf-style
// function (see vku/Log.h). Axiom provides the implementation so that linking
// against the vku library (and the vku SDL backend) resolves.
#include <cstdarg>
#include <cstdio>

void vku_internal_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::vprintf(fmt, args);
    va_end(args);
    std::fflush(stdout);
}
