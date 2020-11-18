# SPDX-License-Identifier: GPL-2.0
.PHONY: all clean

top_srcdir = ../../../..
INCLUDES := -I../ -Iinclude/ -I$(top_srcdir)/usr/include
CFLAGS := $(CFLAGS) -g -O2 -Wall -static -D_GNU_SOURCE -pthread $(INCLUDES)
KSFT_KHDR_INSTALL := 1

TEST_GEN_FILES := \
	daemon-example \
	daemon-parallel \
	daemon-short

TEST_PROGS := \
	fio-rand-read-1G.fio \
	fio-verify-1G.fio \
	harness-fio.sh \
	harness-fsstress.sh \
	run.sh

$(TEST_GEN_FILES): khdr

include ../lib.mk
