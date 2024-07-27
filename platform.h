#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef __MINGW32__
#define OS_FSEEK fseeko64
#define OS_FTELL ftello64
#endif
#ifdef __INSANEOS__
#define OS_FSEEK fseek
#define OS_FTELL ftell
#endif
#ifdef __linux__
#define OS_FSEEK fseeko
#define OS_FTELL ftello
#endif

#endif
