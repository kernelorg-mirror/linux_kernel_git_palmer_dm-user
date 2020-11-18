# SPDX-License-Identifier: GPL-2.0
# Copyright 2020 Palmer Dabbelt <palmerdabbelt@google.com>

# Top-level run script for dm-user kernel self tests.  This just runs a bunch
# of different tests back to back, relying on the kernel selftest infrastructure
# to tease out the success/failure of each.  The tests all use the same global
# directories and such, so it's not like there's a whole lot
#
# The actual test code should be fairly portable, but the scripts that run it
# aren't.  See the README for more information.

# Runs various FIO scripts against an ext2-based filesystem backed by dm-user.
if test -e /usr/bin/fio
then
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-example
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-short
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-parallel -w   1
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-parallel -w   4
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-parallel -w  16
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-parallel -w  64
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-parallel -w 256
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-parallel -w   1 -b /dev/vdb
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-parallel -w   4 -b /dev/vdb
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-parallel -w  16 -b /dev/vdb
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-parallel -w  64 -b /dev/vdb
    ./harness-fio.sh      -s 3000000 -f fio-rand-read-1G.fio -- ./daemon-parallel -w 256 -b /dev/vdb

    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-example
    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-short
    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-parallel -w   1
    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-parallel -w   4
    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-parallel -w  16
    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-parallel -w  64
    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-parallel -w 256
    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-parallel -w   1 -b /dev/vdb
    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-parallel -w   4 -b /dev/vdb
    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-parallel -w  16 -b /dev/vdb
    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-parallel -w  64 -b /dev/vdb
    ./harness-fio.sh      -s 3000000 -f fio-verify-1G.fio    -- ./daemon-parallel -w 256 -b /dev/vdb
else
    echo "Unable to find /usr/bin/fio"
fi

# Runs fsstress from xfstests against an ext2-based filesystem backed by
# dm-user.
if test -e /usr/xfstests/ltp/fsstress
then
    ./harness-fsstress.sh -s 3000000 -p   1 -n 10000 -- ./daemon-example
    ./harness-fsstress.sh -s 3000000 -p   4 -n 10000 -- ./daemon-example
    ./harness-fsstress.sh -s 3000000 -p  16 -n 10000 -- ./daemon-example
    ./harness-fsstress.sh -s 3000000 -p  64 -n 10000 -- ./daemon-example
    ./harness-fsstress.sh -s 3000000 -p 256 -n 10000 -- ./daemon-example

    ./harness-fsstress.sh -s 3000000 -p   1 -n 10000 -- ./daemon-short
    ./harness-fsstress.sh -s 3000000 -p   4 -n 10000 -- ./daemon-short
    ./harness-fsstress.sh -s 3000000 -p  16 -n 10000 -- ./daemon-short
    ./harness-fsstress.sh -s 3000000 -p  64 -n 10000 -- ./daemon-short
    ./harness-fsstress.sh -s 3000000 -p 256 -n 10000 -- ./daemon-short

    ./harness-fsstress.sh -s 3000000 -p 64 -n 10000 -- ./daemon-parallel -w    1
    ./harness-fsstress.sh -s 3000000 -p 64 -n 10000 -- ./daemon-parallel -w    4
    ./harness-fsstress.sh -s 3000000 -p 64 -n 10000 -- ./daemon-parallel -w   16
    ./harness-fsstress.sh -s 3000000 -p 64 -n 10000 -- ./daemon-parallel -w   64
    ./harness-fsstress.sh -s 3000000 -p 64 -n 10000 -- ./daemon-parallel -w  256

    ./harness-fsstress.sh -s 3000000 -p  1 -n 10000 -- ./daemon-parallel -w    1 -b /dev/vdb
    ./harness-fsstress.sh -s 3000000 -p 64 -n 10000 -- ./daemon-parallel -w    1 -b /dev/vdb
    ./harness-fsstress.sh -s 3000000 -p 64 -n 10000 -- ./daemon-parallel -w    4 -b /dev/vdb
    ./harness-fsstress.sh -s 3000000 -p 64 -n 10000 -- ./daemon-parallel -w   16 -b /dev/vdb
    ./harness-fsstress.sh -s 3000000 -p 64 -n 10000 -- ./daemon-parallel -w   64 -b /dev/vdb
    ./harness-fsstress.sh -s 3000000 -p 64 -n 10000 -- ./daemon-parallel -w  256 -b /dev/vdb
else
    echo "Unable to find /usr/xfstests/ltp/fsstress"
fi
