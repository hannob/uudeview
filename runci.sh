#!/bin/bash
set -euo pipefail

autoconf

# Test with ASAN / Address Sanitizer
./configure CFLAGS="-fsanitize=address" LDFLAGS="-fsanitize=address"
make
make check
