#!/bin/sh
#
# Build script for travis-ci.org builds to handle compiles and static
# analysis when ANALYZE=true.
#
if [ "$ANALYZE" = "true" ]; then
    if [ "$CC" = "clang" ]; then
        scan-build make -f Makefile.linux -j2
    else
        cppcheck --error-exitcode=1 --quiet .
        cppcheck --template "{file}({line}): {severity} ({id}): {message}" \
            --enable=style --force --std=c99 -j 2 --inline-suppr \
            unsf.c 2> cppcheck.txt
        if [ -s cppcheck.txt ]; then
            cat cppcheck.txt
            exit 0
        fi
    fi
else
    make -f Makefile.linux
fi
