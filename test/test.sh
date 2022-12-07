#!/bin/bash
set -euo pipefail

MYDIR="$(dirname "$0")"
TMPD="$(mktemp -d)"

# All test files ending with .good.msg contain the
# file doc/test.txt with various encodings.
for fn in "$MYDIR"/*.good.msg; do
	"$MYDIR"/../unix/uudeview -i -p "$TMPD" "$fn"

	diff "$MYDIR"/../doc/test.txt "$TMPD"/test.txt

	rm "$TMPD"/test.txt
done

# The files ending with .bad.msg are sample files
# triggering bugs like memory safety violations. They
# do not contain valid data, thus we do not expect
# extracted files, we just do not want them to crash.
for fn in "$MYDIR"/*.bad.msg; do
	"$MYDIR"/../unix/uudeview -i -p "$TMPD" "$fn"
done

rmdir "$TMPD"
