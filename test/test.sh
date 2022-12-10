#!/bin/bash
set -euo pipefail

MYDIR="$(dirname "$0")"
TMPD="$(mktemp -d)"

# All test files ending with .good.msg contain the
# file doc/test.txt with various encodings.
for fn in "$MYDIR"/*.good.msg; do
	echo -e "\033[0;32mTesting good test case $fn\033[0m"
	"$MYDIR"/../unix/uudeview -i -p "$TMPD" "$fn"

	diff "$MYDIR"/../doc/test.txt "$TMPD"/test.txt

	rm "$TMPD"/test.txt
done

# The files ending with .bad.msg are sample files
# triggering bugs like memory safety violations. They
# do not contain valid data, thus we do not expect
# extracted files, we just do not want them to crash.
for fn in "$MYDIR"/*.bad.msg; do
	echo -e "\033[0;33mTesting bad test case $fn\033[0m"
	"$MYDIR"/../unix/uudeview -i -p "$TMPD" "$fn"
done

# Files ending with .fail.msg trigger memory safety bugs,
# they differ from .bad.msg fails that uudeview will try
# to decode input, so we expect them to return an error,
# still they should not crash.
for fn in "$MYDIR"/*.fail.msg; do
	echo -e "\033[1;33mTesting fail test case $fn\033[0m"
	# uudeview returns 2 on errors
	"$MYDIR"/../unix/uudeview -i -p "$TMPD" "$fn" && false || [ $? -eq 2 ]
done

# Test encoding and decoding of a random file of random size.
head -c $RANDOM /dev/urandom > "$TMPD/test.bin"
cp "$TMPD/test.bin" "$TMPD/_test.bin"
for enc in b u x y; do
	echo -e "\033[0;35mTesting uuenview -$enc\033[0m"
	echo -en "From:a\nTo:b\nSubject:test\n\n" | \
		"$MYDIR"/../unix/uuenview -$enc -a "$TMPD/test.bin" > \
		"$TMPD/test.$enc.msg"
	rm "$TMPD/test.bin"
	"$MYDIR"/../unix/uudeview -i -p "$TMPD" "$TMPD/test.$enc.msg"
	diff "$TMPD/_test.bin" "$TMPD/test.bin"
	rm "$TMPD/test.$enc.msg"
done

echo -e "\033[0;36mTesting split uuencoded file\033[0m"
uuenview -200 -od "$TMPD" "$TMPD/test.bin"
rm "$TMPD/test.bin"
uudeview "$TMPD"/test.* -p "$TMPD" -i
diff "$TMPD/_test.bin" "$TMPD/test.bin"

rm -r "$TMPD"

echo
echo -e "\033[1;34mAll tests ran as expected\033[0m"
