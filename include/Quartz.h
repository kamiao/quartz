#ifndef _SIIT_QUARTZ_H_
#define _SIIT_QUARTZ_H_

#if defined(_WIN32)
#if defined(QUARTZ_EXPORTS)
#define QUARTZ_API __declspec(dllexport)
#else
#define QUARTZ_API __declspec(dllimport)
#endif
#else
#if !defined(NO_GCC_API_ATTRIBUTE) && defined(__GNUC__) && (__GNUC__ >= 4)
#define QUARTZ_API __attribute__ ((visibility ("default")))
#else
#define QUARTZ_API
#endif
#endif

#endif // !_SIIT_QUARTZ_H_
