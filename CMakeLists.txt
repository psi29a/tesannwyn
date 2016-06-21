PROJECT(tesannwyn C)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Datadiver version
SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 2)
SET(VERSION_RELEASE 5)
SET(TESANNWYN_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE}")

# lib versions
SET(SOVERSION 1)
SET(VERSION 1.0.0)

INCLUDE(CMakeDependentOption)
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckCSourceCompiles)
INCLUDE(TestBigEndian)

# Set a default build type if none was specified
IF(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    MESSAGE(STATUS "Setting build type to 'Debug' as none was specified.")
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF()
SET(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

check_c_compiler_flag("-Wall" HAVE_WALL_SWITCH)
if(HAVE_WALL_SWITCH)
    add_definitions("-Wall -g")
endif()

# Platform specific defines
IF(UNIX)
    # allow 'large' files in 32 bit builds
    ADD_DEFINITIONS(
            -D_LARGEFILE_SOURCE
            -D_FILE_OFFSET_BITS=64
            -D_LARGE_FILES
    )
ENDIF()

IF(WIN32)
    ADD_DEFINITIONS(
            -DWIN32_LEAN_AND_MEAN
            -D_CRT_SECURE_NO_WARNINGS
    )
ENDIF()

# ############## Compiler specific settings ###############
IF(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
    ADD_DEFINITIONS(
            -Wall -W
    )

    IF(NOT WIN32 AND NOT CYGWIN)
        SET(OLD_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
        SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS} -Werror")
        CHECK_C_SOURCE_COMPILES("int foo(void) __attribute__((visibility(\"default\")));
                                 int main(void) {return 0;}" HAVE_VISIBILITY_DEFAULT)
        IF(HAVE_VISIBILITY_DEFAULT)
            CHECK_C_COMPILER_FLAG(-fvisibility=hidden HAVE_VISIBILITY_HIDDEN)
        ENDIF()
        SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS}")
    ENDIF()

    IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
        ADD_DEFINITIONS( -ggdb3 -O0 )
    ELSEIF(CMAKE_BUILD_TYPE STREQUAL "Release")
        ADD_DEFINITIONS( -O3 )
    ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")
ENDIF()

IF("${CMAKE_C_COMPILER_ID}" MATCHES "SunPro")
    SET(OLD_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS} -xldscope=hidden")
    # __SUNPRO_C >= 0x590
    #   CHECK_C_SOURCE_COMPILES("int foo(void) __attribute__((visibility(\"default\")));
    #                            int main(void) {return 0;}" HAVE_VISIBILITY_DEFAULT)
    # __SUNPRO_C >= 0x550
    CHECK_C_SOURCE_COMPILES("__global int foo(void);
                             int main(void) {return 0;}" HAVE_LDSCOPE_GLOBAL)
    IF(HAVE_LDSCOPE_GLOBAL)# OR HAVE_VISIBILITY_DEFAULT
        SET(HAVE_LDSCOPE_HIDDEN 1)
    ENDIF()
    SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS}")
ENDIF()

CHECK_C_SOURCE_COMPILES("int main(void) {__builtin_expect(0,0); return 0;}" HAVE___BUILTIN_EXPECT)

CHECK_C_SOURCE_COMPILES("static inline int static_foo() {return 0;}
                         int main(void) {return 0;}" HAVE_C_INLINE)
CHECK_C_SOURCE_COMPILES("static __inline__ int static_foo() {return 0;}
                         int main(void) {return 0;}" HAVE_C___INLINE__)
CHECK_C_SOURCE_COMPILES("static __inline int static_foo() {return 0;}
                         int main(void) {return 0;}" HAVE_C___INLINE)

#CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
#CHECK_INCLUDE_FILE(inttypes.h HAVE_INTTYPES_H)

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

FIND_LIBRARY(M_LIBRARY m REQUIRED)
SET(CMAKE_REQUIRED_INCLUDES math.h)
SET(CMAKE_REQUIRED_LIBRARIES m)

# ############ common libtesannwyn #############

# Setup our tesannwyn library that we link to
SET(tesannwyn_library_SRCS
        libtesannwyn.c
        )

SET(tesannwyn_library_HDRS
        libtesannwyn.h
        )

# set our target paths
SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
SET(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

# set our library names
IF(WIN32 AND NOT CMAKE_COMPILER_IS_MINGW) # windows uses *.lib for both static and dynamic, workaround
    SET(LIBRARY_DYN_NAME "tesannwyn_dynamic")
    SET(LIBRARY_STATIC_NAME "tesannwyn_static")
ELSE() # everyone else uses .a and .so
    SET(LIBRARY_DYN_NAME "tesannwyn")
    SET(LIBRARY_STATIC_NAME "tesannwyn")
ENDIF()

# always build our dynamic library
ADD_LIBRARY(libtesannwyn_dynamic SHARED
        ${tesannwyn_library_SRCS}
        ${tesannwyn_library_HDRS}
        )

TARGET_LINK_LIBRARIES(libtesannwyn_dynamic
        ${M_LIBRARY}
        )

SET_TARGET_PROPERTIES(libtesannwyn_dynamic PROPERTIES
        SOVERSION ${SOVERSION}
        VERSION ${VERSION}
        OUTPUT_NAME ${LIBRARY_DYN_NAME} CLEAN_DIRECT_OUTPUT 1
        )

# Set our default and then look at the possible locations
SET(TESANNWYNLIB "${CMAKE_BINARY_DIR}/lib${LIBRARY_DYN_NAME}.so")

INCLUDE_DIRECTORIES("${tesannwyn_SOURCE_DIR}")

SET(tesannwyn_SRCS
        tes3_import.c
        tes3_export.c
        tesannwyn.c)

SET(tesannwyn_HDRS
        defs.h
        tes3_import.h
        tes3_export.h)

ADD_EXECUTABLE(tesannwyn ${tesannwyn_SRCS} ${tesannwyn_HDRS})
ADD_DEPENDENCIES(tesannwyn libtesannwyn_dynamic)
TARGET_LINK_LIBRARIES(tesannwyn ${TESANNWYNLIB} ${M_LIBRARY})
INSTALL(TARGETS tesannwyn RUNTIME DESTINATION bin)
