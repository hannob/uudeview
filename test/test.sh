#!/bin/bash
set -euo pipefail

MYDIR="$(dirname "$0")"
TMPD="$(mktemp -d)"

for fn in "$MYDIR"/*.msg; do
	"$MYDIR"/../unix/uudeview -i -p "$TMPD" "$fn"

	diff "$MYDIR"/../doc/test.txt "$TMPD"/test.txt

	rm "$TMPD"/test.txt
done

rmdir "$TMPD"
