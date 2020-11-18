#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2020 Google, Inc

# Just a fixed size for now, but it's passed to the tests and they're supposed
# to respect it.
SIZE=1024
BLOCK=kselftest-dm-user-block
CONTROL=kselftest-dm-user-control
unset FIO

while [ x"$1" != x"--" ]
do
    case "$1" in
    "-s")    SIZE="$2";                             shift 2;;
    "-f")    FIO="$2";                              shift 2;;
    *)       echo "$0: unknown argument $1" >&2;    exit  1;;
    esac
done
shift

# Run the benchmark again via dm-user, to see what the overhead is.
dmsetup create $BLOCK << EOF
0 $SIZE user 0 $SIZE $CONTROL
EOF

dmsetup resume $BLOCK

"$@" -s $SIZE -c /dev/dm-user/$CONTROL &

yes | mkfs.ext2 /dev/mapper/$BLOCK
mount /dev/mapper/$BLOCK /mnt
cp "$FIO" /mnt/benchmark.fio
(cd /mnt; fio benchmark.fio)
umount /mnt

# Mount again and read the whole thing, just to see if there's any corruption.
mount /dev/mapper/$BLOCK /mnt
find /mnt -type f | xargs cat > /dev/null
umount /mnt

dmsetup remove $BLOCK

# Make sure the daemon actually responds to DM closing it.
wait
