CMAKE_MINIMUM_REQUIRED(VERSION 2.8.5)

PROJECT(UnSF)

INCLUDE(CheckLibraryExists)
INCLUDE(GNUInstallDirs)

IF(NOT LIBTYPE)
    SET(LIBTYPE STATIC)
ENDIF()

SET(LIB_MAJOR_VERSION "1")
SET(LIB_MINOR_VERSION "1")
SET(LIB_REVISION "0")
SET(LIB_VERSION "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_REVISION}")

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
ENDIF()
IF(NOT CMAKE_DEBUG_POSTFIX)
    SET(CMAKE_DEBUG_POSTFIX "" CACHE STRING
        "Library postfix for debug builds. Normally left blank."
        FORCE)
ENDIF()

# Some systems need libm for some of the following math functions to work
CHECK_LIBRARY_EXISTS(m pow "" HAVE_LIBM)
IF(HAVE_LIBM)
    SET(EXTRA_LIBS m ${EXTRA_LIBS})
    SET(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} m)
ENDIF()

if(DEFINED SHARE_INSTALL_DIR)
    message(WARNING "SHARE_INSTALL_DIR is deprecated.  Use the variables provided by the GNUInstallDirs module instead")
    set(CMAKE_INSTALL_DATADIR "${SHARE_INSTALL_DIR}")
endif()

if(DEFINED LIB_SUFFIX)
    message(WARNING "LIB_SUFFIX is deprecated.  Use the variables provided by the GNUInstallDirs module instead")
endif()


SET(LIBNAME "UnSF")
IF(WIN32)
    ADD_DEFINITIONS("-D_WIN32 -D_WIN32_WINNT=0x0502")

    # This option is mainly for static linking UnSF into another project
    # that already defines the IDs. It is up to that project to ensure all
    # required IDs are defined.
    OPTION(UNSF_NO_UID_DEFS "Do not define GUIDs, IIDs, CLSIDs, or PropertyKeys" OFF)

    IF(MINGW)
        OPTION(UNSF_BUILD_IMPORT_LIB "Build an import .lib using dlltool (requires sed)" ON)
        IF(NOT DLLTOOL)
            IF(HOST)
                SET(DLLTOOL "${HOST}-dlltool")
            ELSE()
                SET(DLLTOOL "dlltool")
            ENDIF()
        ENDIF()
    ENDIF()
ENDIF()

SET(EXTRA_CFLAGS "")
IF(MSVC)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
    ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_DEPRECATE)
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} /wd4098")

    OPTION(FORCE_STATIC_VCRT "Force /MT for static VC runtimes" OFF)
    IF(FORCE_STATIC_VCRT)
        FOREACH(flag_var
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            IF(${flag_var} MATCHES "/MD")
                STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            ENDIF()
        ENDFOREACH(flag_var)
    ENDIF()
ELSE()
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Winline -Wall")
    #CHECK_C_COMPILER_FLAG(-Wextra HAVE_W_EXTRA)
    IF(HAVE_W_EXTRA)
        SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wextra")
    ENDIF()

    IF(UNSF_WERROR)
        SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Werror")
    ENDIF()

    # We want RelWithDebInfo to actually include debug stuff (define _DEBUG
    # instead of NDEBUG)
    FOREACH(flag_var  CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        IF(${flag_var} MATCHES "-DNDEBUG")
            STRING(REGEX REPLACE "-DNDEBUG" "-D_DEBUG" ${flag_var} "${${flag_var}}")
        ENDIF()
    ENDFOREACH()

    #CHECK_C_SOURCE_COMPILES("int foo() __attribute__((destructor));
    #                         int main() {return 0;}" HAVE_GCC_DESTRUCTOR)

    option(UNSF_STATIC_LIBGCC "Force -static-libgcc for static GCC runtimes" OFF)
    if(UNSF_STATIC_LIBGCC)
        set(OLD_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
        set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} -static-libgcc)
        check_c_source_compiles(
"#include <stdlib.h>
int main()
{
    return 0;
}"
            HAVE_STATIC_LIBGCC_SWITCH
        )
        if(HAVE_STATIC_LIBGCC_SWITCH)
            set(EXTRA_LIBS ${EXTRA_LIBS} -static-libgcc)
        endif()
        set(CMAKE_REQUIRED_LIBRARIES ${OLD_REQUIRED_LIBRARIES})
        unset(OLD_REQUIRED_LIBRARIES)
    endif()
ENDIF()

# create our pkg-config: unsf.pc
SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(exec_prefix "\${prefix}")
SET(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
SET(bindir "\${exec_prefix}/${CMAKE_INSTALL_BINDIR}")
SET(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
SET(PACKAGE_VERSION "${LIB_VERSION}")

CONFIGURE_FILE(
        "${UnSF_SOURCE_DIR}/unsf.pc.in"
        "${UnSF_BINARY_DIR}/unsf.pc"
        @ONLY)

# build library
SET(UNSF_OBJS libunsf.c)

IF(LIBTYPE STREQUAL "STATIC")
    ADD_LIBRARY(${LIBNAME} STATIC ${UNSF_OBJS})
    IF(MSVC)
        SET(libUnSF_LOCATION "${CMAKE_BINARY_DIR}\\Release\\${LIBNAME}.lib")
    ELSE()
        SET(libUnSF_LOCATION "${CMAKE_BINARY_DIR}/lib${LIBNAME}.a")
    ENDIF()
ELSE()
    ADD_LIBRARY(${LIBNAME} SHARED ${UNSF_OBJS})
    IF(MSVC)
        SET(libUnSF_LOCATION "${CMAKE_BINARY_DIR}\\Release\\${LIBNAME}.dll")
    ELSE()
        SET(libUnSF_LOCATION "${CMAKE_BINARY_DIR}/lib${LIBNAME}.so")
    ENDIF()
ENDIF()


IF(WIN32 AND NOT LIBTYPE STREQUAL "STATIC")
    SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES PREFIX "")

    IF(MINGW AND UNSF_BUILD_IMPORT_LIB)
        FIND_PROGRAM(SED_EXECUTABLE NAMES sed DOC "sed executable")
        FIND_PROGRAM(DLLTOOL_EXECUTABLE NAMES "${DLLTOOL}" DOC "dlltool executable")
        IF(NOT SED_EXECUTABLE OR NOT DLLTOOL_EXECUTABLE)
            MESSAGE(STATUS "")
            IF(NOT SED_EXECUTABLE)
                MESSAGE(STATUS "WARNING: Cannot find sed, disabling .def/.lib generation")
            ENDIF()
            IF(NOT DLLTOOL_EXECUTABLE)
                MESSAGE(STATUS "WARNING: Cannot find dlltool, disabling .def/.lib generation")
            ENDIF()
        ELSE()
            SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES LINK_FLAGS "-Wl,--output-def,${LIBNAME}.def")
            ADD_CUSTOM_COMMAND(TARGET ${LIBNAME} POST_BUILD
                    COMMAND "${SED_EXECUTABLE}" -i -e "s/ @[^ ]*//" ${LIBNAME}.def
                    COMMAND "${DLLTOOL_EXECUTABLE}" -d ${LIBNAME}.def -l ${LIBNAME}.lib -D ${LIBNAME}.dll
                    COMMENT "Stripping ordinals from ${LIBNAME}.def and generating ${LIBNAME}.lib..."
                    VERBATIM
                    )
        ENDIF()
    ENDIF()
ENDIF()

target_compile_definitions(${LIBNAME} PRIVATE UNSF_BUILD=1 UNSF_STATIC=1)

SET_PROPERTY(TARGET ${LIBNAME} APPEND PROPERTY COMPILE_FLAGS ${EXTRA_CFLAGS})

IF(WIN32 AND UnSF_NO_UID_DEFS)
    SET_PROPERTY(TARGET ${LIBNAME} APPEND PROPERTY COMPILE_DEFINITIONS NO_UID_DEFS)
ELSEIF(NOT WIN32)
    SET_PROPERTY(TARGET ${LIBNAME} APPEND PROPERTY POSITION_INDEPENDENT_CODE TRUE)
ENDIF()

SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES VERSION ${LIB_VERSION}
        SOVERSION ${LIB_MAJOR_VERSION})

# build executable
ADD_EXECUTABLE(sf2pat unsf.c libunsf.h)
ADD_DEPENDENCIES(sf2pat ${LIBNAME})
TARGET_LINK_LIBRARIES(sf2pat ${libUnSF_LOCATION} ${EXTRA_LIBS})
