cmake_minimum_required(VERSION 3.29.2)

set(PROGRAM_VERSION "KalaMake")
set(PROGRAM_VERSION_NUMBER 1.0.0.0)

project("KalaMake" VERSION ${PROGRAM_VERSION_NUMBER} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Runtime lib handling for MSVC
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Platform Detection
if (WIN32)
    message(STATUS "[KALAMAKE] Platform = Windows")
elseif(UNIX)
	message(STATUS "[KALAMAKE] Platform = Unix")
else()
    message(FATAL_ERROR "[KALAMAKE] Unsupported platform. Only Windows and Unix are supported.")
endif()

# Build Type Detection
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(IS_DEBUG TRUE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(IS_RELEASE TRUE)
else()
    message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'! Must be Debug, Release, or RelWithDebInfo.")
endif()

# Directory Setup
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(EXT_SHARED_DIR "${CMAKE_SOURCE_DIR}/_external_shared")

set(EXT_CLI_DIR "${EXT_SHARED_DIR}/KalaCLI")

if (WIN32)
	# Library Paths
	if(IS_RELEASE)
		set(CLI_LIBRARY_PATH "${EXT_CLI_DIR}/release/KalaCLI1.lib")
	else()
		set(CLI_LIBRARY_PATH "${EXT_CLI_DIR}/debug/KalaCLI1d.lib")
	endif()
else()
	# Library Paths
	if(IS_RELEASE)
		set(CLI_LIBRARY_PATH "${EXT_CLI_DIR}/release/libKalaCLI1.so")
	else()
		set(CLI_LIBRARY_PATH "${EXT_CLI_DIR}/debug/libKalaCLI1d.so")
	endif()
endif()

# Source Files
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*/*.cpp"
)

# Executable
add_executable(KalaMake ${SOURCE_FILES})

# Enable exception handling for MSVC
if (MSVC)
    target_compile_options(KalaMake PRIVATE /EHsc)
endif()

set_target_properties(KalaMake PROPERTIES OUTPUT_NAME "KalaMake")
target_compile_features(KalaMake PRIVATE cxx_std_20)

# Includes
file(GLOB_RECURSE HEADERS
	configure_depends
	"${CMAKE_SOURCE_DIR}/include/*.hpp"
)
target_sources(KalaMake PRIVATE ${HEADERS})
target_include_directories(KalaMake PRIVATE
	"${INCLUDE_DIR}"
	"${EXT_SHARED_DIR}"
	"${EXT_SHARED_DIR}/KalaCLI/include"
)

if (WIN32)
	# Preprocessor Defines
	target_compile_definitions(KalaMake PRIVATE 
		KS_CONTAINER_CONCEPTS # allow string_utils.hpp to enable string-enum conversion
		WIN32_LEAN_AND_MEAN   # cleaner windows.h
		NOMINMAX              # skips min/max windows macros in favor of the std variants
		UNICODE               # selects wide api variants over ansi for UTF16 windows functions
		_UNICODE              # uses wide text types in the C runtime layer
	)
endif()

# Link libraries
target_link_libraries(KalaMake PRIVATE
	${CLI_LIBRARY_PATH})

# Hide console in release mode
#if(IS_RELEASE)
#    set_target_properties(KalaMake PROPERTIES WIN32_EXECUTABLE TRUE)
#    if (MSVC)
#        set_target_properties(KalaMake PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
#    endif()
#endif()

# Installation
set(CMAKE_INSTALL_BINDIR bin)
install(TARGETS KalaMake DESTINATION ${CMAKE_INSTALL_BINDIR})

# Copy external binaries
if(IS_RELEASE)
	set(BIN_KALACLI "${EXT_CLI_DIR}/release")
else()
	set(BIN_KALACLI "${EXT_CLI_DIR}/debug")
endif()

if (WIN32)
	if (IS_RELEASE)
		file(GLOB BIN_FILES
			"${BIN_KALACLI}/*.dll"
		)
	else()
		file(GLOB BIN_FILES
			"${BIN_KALACLI}/*.dll"
			"${BIN_KALACLI}/*.pdb"
		)
	endif()
else()
	if (IS_RELEASE)
		file(GLOB BIN_FILES
			"${BIN_KALACLI}/*.so"
		)
	else()
		file(GLOB BIN_FILES
			"${BIN_KALACLI}/*.so"
		)
	endif()
endif()

set(RUNTIME_DIR "${CMAKE_BINARY_DIR}/runtime")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${RUNTIME_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${RUNTIME_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${RUNTIME_DIR}")
set(BIN_TARGET_DIR "${RUNTIME_DIR}")

foreach(BIN_FILE ${BIN_FILES})
    if(EXISTS "${BIN_FILE}")
        file(COPY
            "${BIN_FILE}"
            DESTINATION "${BIN_TARGET_DIR}"
        )
    endif()
endforeach()

# Copy important files
set(COPY_TARGETS
    README.md
    LICENSE.md
    logo.png
)

foreach(FILE ${COPY_TARGETS})
    if(EXISTS "${CMAKE_SOURCE_DIR}/${FILE}")
        file(COPY
            "${CMAKE_SOURCE_DIR}/${FILE}"
            DESTINATION "${BIN_TARGET_DIR}"
        )
    endif()
endforeach()

set(RESOURCE_DIRS docs files)

foreach(DIR ${RESOURCE_DIRS})
    if(EXISTS "${CMAKE_SOURCE_DIR}/${DIR}")
        file(REMOVE_RECURSE "${BIN_TARGET_DIR}/${DIR}")
        file(COPY
            "${CMAKE_SOURCE_DIR}/${DIR}"
            DESTINATION "${BIN_TARGET_DIR}"
        )
    endif()
endforeach()

if(EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
    file(COPY
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        DESTINATION "${CMAKE_SOURCE_DIR}"
    )
endif()
