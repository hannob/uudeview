#!/bin/bash
set -euo pipefail

autoreconf -i

# Test with ASAN / Address Sanitizer
export ASAN_OPTIONS="abort_on_error=1"
./configure CFLAGS="-fsanitize=address -U_FORTIFY_SOURCE" LDFLAGS="-fsanitize=address -U_FORTIFY_SOURCE"
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
