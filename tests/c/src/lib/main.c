#include <stdio.h>

#ifdef _WIN32
	#ifdef LIB_EXPORT
		#define LIB_API  __declspec(dllexport)
	#else
		#define LIB_API  __declspec(dllimport)
	#endif
#elif __linux__
	#define LIB_API  __attribute__((visibility("default")))
#else
	#define LIB_API 
#endif

LIB_API void print(char* msg)
{
    printf("[LOG] %s\n", msg);
}