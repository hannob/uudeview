#!/bin/bash
set -euo pipefail

# Check where tcl/tk include files are.
# Enabled on ASAN builds, disabled on others, so we spot build
# errors for both cases.
TCLTK=""
if [ -f "/usr/include/tk.h" ] && [ -f "/usr/include/tcl.h" ]; then
	TCLTK="--enable-tcl=/usr/include/ --enable-tk=/usr/include"
elif [ -f "/usr/include/tk/tk.h" ] && [ -f "/usr/include/tcl/tcl.h" ]; then
	TCLTK="--enable-tcl=/usr/include/tcl/ --enable-tk=/usr/include/tk/"
fi

autoreconf -i

# Test with ASAN / Address Sanitizer
export ASAN_OPTIONS="abort_on_error=1"
./configure $TCLTK CFLAGS="-fsanitize=address -U_FORTIFY_SOURCE" LDFLAGS="-fsanitize=address -U_FORTIFY_SOURCE"
make clean
make
make check

# Test with clang and MSAN / Memory Sanitizer
export MSAN_OPTIONS="abort_on_error=1"
./configure CC=clang LD=clang CFLAGS="-fsanitize=memory -U_FORTIFY_SOURCE" LDFLAGS="-fsanitize=memory -U_FORTIFY_SOURCE"
make clean
make
make check

# Test with clang and UBSAN / Undefined Behavior Sanitizer
export UBSAN_OPTIONS="halt_on_error=1:abort_on_error=1"
./configure CC=clang LD=clang CFLAGS="-fsanitize=undefined" LDFLAGS="-fsanitize=undefined"
make clean
make
make check
