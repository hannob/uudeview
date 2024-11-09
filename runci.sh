#!/bin/bash
set -euo pipefail

TKINC=/usr/include/tk/
TCLINC=/usr/include/tcl/

autoreconf -i


# Test with ASAN / Address Sanitizer
export ASAN_OPTIONS="abort_on_error=1"
./configure CFLAGS="-fsanitize=address -U_FORTIFY_SOURCE" LDFLAGS="-fsanitize=address -U_FORTIFY_SOURCE" --enable-tk=$TKINC --enable-tcl=$TCLINC
make clean
make
make check
make xdeview

# Test with clang and MSAN / Memory Sanitizer
export MSAN_OPTIONS="abort_on_error=1"
./configure CC=clang LD=clang CFLAGS="-fsanitize=memory -U_FORTIFY_SOURCE" LDFLAGS="-fsanitize=memory -U_FORTIFY_SOURCE" --enable-tk=$TKINC --enable-tcl=$TCLINC
make clean
make
make check
make xdeview

# Test with clang and UBSAN / Undefined Behavior Sanitizer
export UBSAN_OPTIONS="halt_on_error=1:abort_on_error=1"
./configure CC=clang LD=clang CFLAGS="-fsanitize=undefined" LDFLAGS="-fsanitize=undefined" --enable-tk=$TKINC --enable-tcl=$TCLINC
make clean
make
make check
make xdeview
