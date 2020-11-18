#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2020 Google, Inc

BLOCK=kselftest-dm-user-block
CONTROL=kselftest-dm-user-control
unset SIZE
unset NPROC
unset NOP

while [ x"$1" != x"--" ]
do
    case "$1" in
    "-s")    SIZE="$2";                             shift 2;;
    "-n")    NOP="$2";                              shift 2;;
    "-p")    NPROC="$2";                            shift 2;;
    *)       echo "$0: unknown argument $1" >&2;    exit  1;;
    esac
done
shift

# Runs the fs stress tests
dmsetup create $BLOCK << EOF
0 $SIZE user 0 $SIZE $CONTROL
EOF

dmsetup resume $BLOCK

"$@" -s $SIZE -c /dev/dm-user/$CONTROL &

yes | mkfs.ext2 /dev/mapper/$BLOCK
mount /dev/mapper/$BLOCK /mnt
/usr/xfstests/ltp/fsstress -d /mnt/ -n "$NOP" -p "$NPROC"
umount /mnt

# Mount again and read the whole thing, just to see if there's any corruption.
mount /dev/mapper/$BLOCK /mnt
find /mnt -type f | xargs cat > /dev/null
umount /mnt

dmsetup remove $BLOCK

# Make sure the daemon actually responds to DM closing it.
wait
