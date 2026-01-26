//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <vector>
#include <string>

#ifndef scast
	#define scast static_cast
#endif

namespace KalaMake::Core
{
	using std::vector;
	using std::string;
	using std::string_view;

	using u8 = uint8_t;

	constexpr string_view EXE_VERSION_NUMBER = "1.0";
	constexpr string_view KMA_VERSION_NUMBER = "1.0";
	constexpr string_view KMA_VERSION_NAME = "#KMA VERSION 1.0";

	constexpr size_t MIN_NAME_LENGTH = 1;
	constexpr size_t MAX_NAME_LENGTH = 20;

	enum class ActionType : u8
	{
		T_INVALID = 0u,

		//what is the name of the the binary
		T_NAME = 1u,
		//what is the target type of the binary
		T_BINARY_TYPE = 2u,
		//which compiler is used to compile this source code
		T_COMPILER = 3u,
		//which language standard is used to compile this source code
		T_STANDARD = 4u,
		//where are the source code files located
		T_SOURCES = 5u,

		//where is the binary built to
		T_BUILD_PATH = 6u,
		//where are the obj files of the binary built to,
		//ignored for languages that dont generate obj files
		T_OBJ_PATH = 7u,
		//where are the header files of the binary located,
		//ignored for languages that dont use header files
		T_HEADERS = 8u,
		//where are the linked libraries of the binary located,
		//ignored for languages that dont use linked libraries
		T_LINKS = 9u,
		//where are the debug linked libraries of the binary located,
		//ignored for languages that dont use linked libraries
		T_DEBUG_LINKS = 10u,

		//what is your preferred warning level during compilation and linking, defaults to 'none'
		T_WARNING_LEVEL = 11u,
		//what defines are attached to the binary
		T_DEFINES = 12u,
		//what language standard extensions will be used
		T_EXTENSIONS = 13u,

		//what flags will be passed to the compiler
		T_FLAGS = 14u,
		//what debug flags will be passed to the compiler
		T_DEBUG_FLAGS = 15u,
		//what kalamake-specific flags will trigger extra actions
		T_CUSTOM_FLAGS = 16u
	};

	enum class BinaryType : u8
	{
		B_INVALID = 0u,

		//creates a runnable executable
		B_EXECUTABLE = 1u,

		//creates a linkable .lib on MSVC,
		//creates a linkable .a on GNU
		B_LINK_ONLY = 2u,

		//creates a .dll on MSVC,
		//creates a .so on GNU
		B_RUNTIME_ONLY = 3u,

		//creates a .dll and a linkable .lib on MSVC,
		//creates a .so on GNU, same as runtime-only
		B_LINK_RUNTIME = 4u
	};

	enum class WarningLevel : u8
	{
		W_INVALID = 0u,

		//no warnings
		//  MSVC: /W0
		//  GNU:  -w
		W_NONE = 1u,

		//very basic warnings
		//  MSVC: /W1
		//  GNU:  -Wall
		W_BASIC = 2u,

		//common, useful warnings
		//  MSVC: /W2
		//  GNU:  -Wall
		//        -Wextra
		W_NORMAL = 3u,

		//strong warnings, recommended default
		//  MSVC: /W3
		//  GNU:  -Wall
		//        -Wextra
		//        -Wpedantic
		W_STRONG = 4u,

		//very strict, high signal warnings
		//  MSVC: /W4
		//  GNU:  -Wall
		//        -Wextra
		//        -Wpedantic
		//        -Wshadow
		//        -Wconversion
		//        -Wsign-conversion
		W_STRICT = 5u,

		//everything
		//  MSVC: (cl + clang-cl):       /Wall
		// 
		//  MSVC/GNU (clang + clang++):  -Wall
		//                               -Wextra
		//                               -Wpedantic
		//                               -Weverything
		// 
		//  GNU (GCC + G++):             -Wall
		//                               -Wextra
		//                               -Wpedantic
		//                               -Wshadow
		//                               -Wconversion
		//                               -Wsign-conversion
		//                               -Wcast-align
		//                               -Wnull-dereference
		//                               -Wdouble-promotion
		//                               -Wformat=2
		W_ALL = 6u
	};

	enum class CustomFlag : u8
	{
		F_INVALID = 0u,

		//uses the multithreaded benefits of ninja for faster compilation
		F_USE_NINJA = 1u,

		//will not generate obj files for obj-compatible languages, compiles and links directly
		F_NO_OBJ = 2u,

		//fails the build if the compiler cannot support the requested standard, ignored on GNU
		//  MSVC: /permissive-
		//  GNU:  nothing
		F_STANDARD_REQUIRED = 3u,

		//treats all warnings as errors
		//  MSVC: /WX
		//  GNU:  -Werror
		F_WARNINGS_AS_ERRORS = 4u,

		//used only for the --generate command in kalamake,
		//exports the compilation commands to your solution file type you selected
		F_EXPORT_COMPILE_COMMANDS = 5u,

		//create debug build
		//  MSVC: /Od /Zi
		//  GNU:  -O0 -g
		F_BUILD_DEBUG = 6u,
		//only create standard release build
		//  MSVC: /O2
		//  GNU:  -O2
		F_BUILD_RELEASE = 7u,
		//only create release with debug symbols
		//  MSVC: /O2 /Zi
		//  GNU:  -O2 -g
		F_BUILD_RELDEBUG = 8u,
		//only create minimum size release build
		//  MSVC: /O1
		//  GNU:  -Os
		F_BUILD_MINSIZEREL = 9u
	};

	class KalaMakeCore
	{
	public:
		static void Initialize(const vector<string>& params);

		static void PrintError(string_view message);
	};
}