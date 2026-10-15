#include <string>
#include <iostream>

using std::cout;
using std::string;

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

LIB_API void print(const string& message)
{
    cout << message << "\n";
}