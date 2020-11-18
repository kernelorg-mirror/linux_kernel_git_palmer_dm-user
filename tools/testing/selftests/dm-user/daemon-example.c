// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2020 Google, Inc
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <pthread.h>
#include <linux/dm-user.h>
#include <sys/prctl.h>
#include "logging.h"

#define SECTOR_SIZE 512

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int write_all(int fd, void *buf, size_t len)
{
	char *buf_c = buf;
	ssize_t total = 0;
	ssize_t once;

	while (total < len) {
		once = write(fd, buf_c + total, len - total);
		if (once <= 0)
			return once;
		total += once;
	}

	return total;
}

int read_all(int fd, void *buf, size_t len)
{
	char *buf_c = buf;
	ssize_t total = 0;
	ssize_t once;

	while (total < len) {
		once = read(fd, buf_c + total, len - total);
		if (once <= 0)
			return once;
		total += once;
	}

	return total;
}

int simple_daemon(char *control_dev,
		  size_t block_bytes,
		  char *store)

{
	int control_fd = open(control_dev, O_RDWR);

	if (control_fd < 0) {
		ksft_print_msg("Unable to open control device %s\n", control_dev);
		return RET_FAIL;
	}

	while (1) {
		struct dm_user_message msg;
		__u64 type;
		char *base;

		if (read_all(control_fd, &msg, sizeof(msg)) < 0) {
			if (errno == ENOTBLK)
				return RET_PASS;

			perror("unable to read msg");
			return RET_FAIL;
		}

		base = store + msg.sector * SECTOR_SIZE;
		if (base + msg.len > store + block_bytes) {
			fprintf(stderr, "access out of bounds\n");
			return RET_FAIL;
		}

		type = msg.type;
		switch (type) {
		case DM_USER_REQ_MAP_WRITE:
			msg.type = DM_USER_RESP_SUCCESS;
			if (read_all(control_fd, base, msg.len) < 0) {
				if (errno == ENOTBLK)
					return RET_PASS;

				perror("unable to read buf");
				return RET_FAIL;
			}
			break;
		case DM_USER_REQ_MAP_FLUSH:
			/* Nothing extra to do on flush, we're in memory. */
		case DM_USER_REQ_MAP_READ:
			msg.type = DM_USER_RESP_SUCCESS;
			break;
		default:
			msg.type = DM_USER_RESP_UNSUPPORTED;
			break;
		}

		if (write_all(control_fd, &msg, sizeof(msg)) < 0) {
			if (errno == ENOTBLK)
				return RET_PASS;

			perror("unable to write msg");
			return RET_FAIL;
		}

		if (type == DM_USER_REQ_MAP_READ) {
			if (write_all(control_fd, base, msg.len) < 0) {
				if (errno == ENOTBLK)
					return RET_PASS;

				perror("unable to write buf");
				return RET_FAIL;
			}
		}
	}

	/* The daemon doesn't actully terminate for this test. */
	perror("Unable to read from control device");
	return RET_FAIL;
}

void usage(char *prog)
{
	printf("Usage: %s\n", prog);
	printf("  -h			Display this help message\n");
	printf("  -v L			Verbosity level: %d=QUIET %d=CRITICAL %d=INFO\n",
	       VQUIET, VCRITICAL, VINFO);
	printf("  -c <control dev>	Control device to use for the test\n");
	printf("  -s <sectors>		The number of sectors in the device\n");
}

int main(int argc, char *argv[])
{
	int ret = RET_PASS;
	int c;
	char *control_dev = NULL;
	long block_bytes = 1024;
	char *store;

	prctl(PR_SET_IO_FLUSHER, 0, 0, 0, 0);

	while ((c = getopt(argc, argv, "h:v:c:s:")) != -1) {
		switch (c) {
		case 'h':
			usage(basename(argv[0]));
			exit(0);
		case 'v':
			log_verbosity(atoi(optarg));
			break;
		case 'c':
			control_dev = strdup(optarg);
			break;
		case 's':
			block_bytes = atoi(optarg) * SECTOR_SIZE;
			break;
		default:
			usage(basename(argv[0]));
			exit(1);
		}
	}

	ksft_print_header();
	ksft_set_plan(1);
	ksft_print_msg("%s: block_bytes=%zu\n",
		       basename(argv[0]),
		       block_bytes);

	store = malloc(block_bytes);
	for (size_t i = 0; i < block_bytes/sizeof(size_t); ++i)
		((size_t *)(store))[i] = i;

	ret = simple_daemon(control_dev, block_bytes, store);

	print_result(basename(argv[0]), ret);
	exit(ret);
	return ret;
}
