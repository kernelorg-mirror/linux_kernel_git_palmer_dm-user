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
#include <sys/mman.h>
#include "logging.h"

#define SECTOR_SIZE 512
#define MAX_WORKER_COUNT 256

#define MAX(a, b) ((a) > (b) ? (a) : (b))

struct test_context {
	char *control_dev;
	size_t block_bytes;
	char *store;
	long worker_count;
	char *backing_path;
};

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

void *simple_daemon(void *context_uc)
{
	struct test_context *context = context_uc;
	char *store = context->store;
	int control_fd = open(context->control_dev, O_RDWR);

	if (control_fd < 0) {
		ksft_print_msg("Unable to open control device %s\n", context->control_dev);
		return (void *)(RET_FAIL);
	}

	while (1) {
		struct dm_user_message msg;
		__u64 type;
		char *base;

		if (read_all(control_fd, &msg, sizeof(msg)) < 0) {
			if (errno == ENOTBLK)
				return (void *)(RET_PASS);

			perror("unable to read msg");
			return (void *)(RET_FAIL);
		}

		base = store + msg.sector * SECTOR_SIZE;
		if (base + msg.len > store + context->block_bytes) {
			fprintf(stderr, "access out of bounds\n");
			return (void *)(RET_FAIL);
		}

		type = msg.type;
		switch (type) {
		case DM_USER_REQ_MAP_READ:
			msg.type = DM_USER_RESP_SUCCESS;
			break;
		case DM_USER_REQ_MAP_WRITE:
			msg.type = DM_USER_RESP_SUCCESS;
			if (read_all(control_fd, base, msg.len) < 0) {
				if (errno == ENOTBLK)
					return (void *)(RET_PASS);

				perror("unable to read buf");
				return (void *)(RET_FAIL);
			}
			break;
		case DM_USER_REQ_MAP_FLUSH:
			msg.type = DM_USER_RESP_SUCCESS;
			sync();
			break;
		default:
			msg.type = DM_USER_RESP_UNSUPPORTED;
			break;
		}

		if (write_all(control_fd, &msg, sizeof(msg)) < 0) {
			if (errno == ENOTBLK)
				return (void *)(RET_PASS);

			perror("unable to write msg");
			return (void *)(RET_FAIL);
		}

		if (type == DM_USER_REQ_MAP_READ) {
			if (write_all(control_fd, base, msg.len) < 0) {
				if (errno == ENOTBLK)
					return (void *)(RET_PASS);

				perror("unable to write buf");
				return (void *)(RET_FAIL);
			}
		}
	}

	/* The daemon doesn't actully terminate for this test. */
	perror("Unable to read from control device");
	return (void *)(RET_FAIL);
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
	int done = 0;
	int c;
	struct test_context context = {
		.control_dev	= NULL,
		.block_bytes	= 0,
		.worker_count   = 1,
		.backing_path   = NULL,
	};
	pthread_t daemon[MAX_WORKER_COUNT];
	void *pthread_ret;

	prctl(PR_SET_IO_FLUSHER, 0, 0, 0, 0);

	while ((c = getopt(argc, argv, "h:v:c:s:w:b:")) != -1) {
		switch (c) {
		case 'h':
			usage(basename(argv[0]));
			exit(0);
		case 'v':
			log_verbosity(atoi(optarg));
			break;
		case 'c':
			context.control_dev = strdup(optarg);
			break;
		case 's':
			context.block_bytes = atoi(optarg) * SECTOR_SIZE;
			break;
		case 'w':
			context.worker_count = atoi(optarg);
			break;
		case 'b':
			context.backing_path = strdup(optarg);
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
		       context.block_bytes);

	ret = RET_PASS;

	if (context.backing_path == NULL) {
		ksft_print_msg("Using an in-memory backing store\n");
		context.store = malloc(context.block_bytes);
		for (size_t i = 0; i < context.block_bytes/sizeof(size_t); ++i)
			((size_t *)(context.store))[i] = i;
	} else {
		int backing_fd = open(context.backing_path, O_RDWR);

		ksft_print_msg("Using %s as a backing store\n", context.backing_path);
		if (backing_fd < 0) {
			perror("Unable to open backing store");
			ksft_print_msg("Unable to open backing store %s\n", context.backing_path);
			return RET_FAIL;
		}

		context.store = mmap(NULL, context.block_bytes,
				     PROT_READ | PROT_WRITE, MAP_SHARED,
				     backing_fd, 0);
	}

	for (size_t i = 0; i < context.worker_count; ++i)
		if (pthread_create(&daemon[i], NULL, &simple_daemon, &context) < 0)
			ret = RET_ERROR;

	while (!done)  {
		for (size_t i = 0; i < context.worker_count; ++i) {
			if (pthread_tryjoin_np(daemon[i], &pthread_ret) == 0) {
				if (pthread_ret != RET_PASS)
					ret = RET_ERROR;
				done = 1;
			}
		}

		sleep(1);
	}

	print_result(basename(argv[0]), ret);
	exit(ret);
}
