#include <EASTL/string.h>
#include <EAStdC/EASprintf.h>
#include <cstdio>

// int Vsnprintf8(char8_t* pDestination, size_t n,
// const char8_t* pFormat, va_list arguments) {
//#ifdef _MSC_VER
// return _vsnprintf(pDestination, n, pFormat, arguments);
//#else
// return vsnprintf(pDestination, n, pFormat, arguments);
//#endif
//}

int Vsnprintf8(char8_t *pDestination, size_t n, const char8_t *pFormat,
               va_list arguments) {
  return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}

int Vsnprintf16(char16_t *pDestination, size_t n, const char16_t *pFormat,
                va_list arguments) {
  return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}
