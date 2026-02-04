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
		set(CLI_LIBRARY_PATH "${EXT_CLI_DIR}/release/libKalaCLI1.dll.a")
	else()
		set(CLI_LIBRARY_PATH "${EXT_CLI_DIR}/debug/libKalaCLI1d.dll.a")
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

# Preprocessor Defines
target_compile_definitions(KalaMake PRIVATE 
	KS_CONTAINER_CONCEPTS # allow string_utils.hpp to enable string-enum conversion
	WIN32_LEAN_AND_MEAN   # cleaner windows.h
	NOMINMAX              # skips min/max windows macros in favor of the std variants
	UNICODE               # selects wide api variants over ansi for UTF16 windows functions
	_UNICODE              # uses wide text types in the C runtime layer
)

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

set(BIN_TARGET_DIR "$<TARGET_FILE_DIR:KalaMake>")

foreach(BIN_FILE ${BIN_FILES})
    get_filename_component(BIN_NAME ${BIN_FILE} NAME)
    add_custom_command(TARGET KalaMake POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
			"${BIN_FILE}"
			"${BIN_TARGET_DIR}/${BIN_NAME}"
    )
endforeach()

# Copy MinGW dlls
if (MINGW AND WIN32)
    set(MINGW_BIN_DIR "/usr/x86_64-w64-mingw32/bin")

    set(MINGW_DLLS
        libstdc++-6.dll
        libgcc_s_seh-1.dll
        libwinpthread-1.dll
    )

    foreach(dll ${MINGW_DLLS})
        add_custom_command(TARGET KalaMake POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_BIN_DIR}/${dll}"
                "$<TARGET_FILE_DIR:KalaMake>/${dll}"
        )
    endforeach()
endif()

# Copy important files
set(COPY_TARGETS README.md LICENSE.md CHANGES.md)
foreach(FILE ${COPY_TARGETS})
add_custom_command(TARGET KalaMake POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f 
        "$<TARGET_FILE_DIR:KalaMake>/${FILE}"
    COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_SOURCE_DIR}/${FILE}"
		"$<TARGET_FILE_DIR:KalaMake>/${FILE}"
)
endforeach()

# Copy entire docs folder
add_custom_command(TARGET KalaMake POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf
        "$<TARGET_FILE_DIR:KalaMake>/docs"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/docs"
        "$<TARGET_FILE_DIR:KalaMake>/docs"
)

# Package
include(CPack)
