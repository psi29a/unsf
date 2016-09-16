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
    SET(libUnSF_LOCATION "${CMAKE_BINARY_DIR}/lib${LIBNAME}.a")
ELSE()
    ADD_LIBRARY(${LIBNAME} SHARED ${UNSF_OBJS})
    SET(libUnSF_LOCATION "${CMAKE_BINARY_DIR}/lib${LIBNAME}.so")
ENDIF()

SET_PROPERTY(TARGET ${LIBNAME} APPEND PROPERTY COMPILE_FLAGS ${EXTRA_CFLAGS})

IF(WIN32 AND UnSF_NO_UID_DEFS)
    SET_PROPERTY(TARGET ${LIBNAME} APPEND PROPERTY COMPILE_DEFINITIONS NO_UID_DEFS)
ENDIF()

SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES VERSION ${LIB_VERSION}
        SOVERSION ${LIB_MAJOR_VERSION})

# build executable
ADD_EXECUTABLE(unsf unsf.c libunsf.h)
ADD_DEPENDENCIES(unsf ${LIBNAME})
TARGET_LINK_LIBRARIES(unsf ${libUnSF_LOCATION} ${EXTRA_LIBS})