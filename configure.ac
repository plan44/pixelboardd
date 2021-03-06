AC_INIT([pixelboardd], [0.14], [luz@plan44.ch], [pixelboardd], [http://www.plan44.ch/])
AC_PREREQ([2.59])

AC_CONFIG_AUX_DIR(configure_aux)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10 no-define foreign subdir-objects])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX

AC_CONFIG_FILES([Makefile])

# options
AC_ARG_ENABLE([debug],
[  --enable-debug    Turn on debugging],
[case "${enableval}" in
  yes) debug=true ;;
  no)  debug=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],[debug=false])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])


P44_BUILD_OW="no"
AC_ARG_ENABLE([p44buildow],
    [AC_HELP_STRING([--enable-p44buildow],
                    [plan44 build for OpenWRT (default: no)]) ],
    [
        if test "x$enableval" = "xno"; then
            P44_BUILD_OW="no"
        elif test "x$enableval" = "xyes"; then
            P44_BUILD_OW="yes"
        fi
    ]
)
if (test "x$P44_BUILD_OW" = "xyes"); then
    AC_DEFINE([P44_BUILD_OW], [1], [plan44 OpenWRT build])
fi
AM_CONDITIONAL([P44_BUILD_OW], [test "x$P44_BUILD_OW" = "xyes"])


AC_CHECK_LIB(m, atan2, [], [AC_MSG_ERROR([Could not find math lib (m)])])
AC_CHECK_LIB(rt, clock_gettime, [], [AC_MSG_ERROR([Could not find rt lib for clock_gettime])])
AC_CHECK_LIB(dl, dlopen, [], [AC_MSG_ERROR([Could not find libdl])])
AC_CHECK_LIB(json-c, json_tokener_get_error, [], [AC_MSG_ERROR([Could not find JSON-C / libjson0 with json_tokener_get_error supported (>=0.10)])])
AC_CHECK_LIB(png, png_image_begin_read_from_file, [], [AC_MSG_ERROR([Could not find libpng with png_image_begin_read_from_file supported (>=1.6)])])


PKG_PROG_PKG_CONFIG


PKG_CHECK_MODULES([JSONC], [json-c >= 0.10 ], [], [
  AC_MSG_ERROR([$JSON_PKG_ERRORS])
])

PKG_CHECK_MODULES([SQLITE3], [sqlite3], [], [
  AC_MSG_ERROR([$SQLITE3_PKG_ERRORS])
])

PKG_CHECK_MODULES([PNG], [libpng], [], [
  AC_MSG_ERROR([$PNG_PKG_ERRORS])
])


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h sys/resource.h], [], [AC_MSG_ERROR([required system header not found])])


ASSUMED_BOOST_VER=146

BOOST_REQUIRE([1.46],
    [
        unset boost_cv_version
        unset boost_cv_inc_path
        BOOST_REQUIRE([1.35],
            [
                AC_MSG_ERROR([could not find boost on your system])
            ])
        ASSUMED_BOOST_VER=135
    ])

if test $ASSUMED_BOOST_VER -eq 135; then
    AC_DEFINE([BOOST_VERSION_135], [1], [boost 1.35 or higher])
else
    AC_DEFINE([BOOST_VERSION_146], [1], [boost 1.46 or higher])
fi

BOOST_BIND
BOOST_FOREACH
BOOST_FUNCTION
BOOST_SMART_PTR

AX_PTHREAD([], [ AC_MSG_ERROR([required pthread library not found]) ])

AC_OUTPUT
