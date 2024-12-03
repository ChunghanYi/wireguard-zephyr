/*
 * Copyright (c) 2024 Chunghan Yi <chunghan.yi@gmail.com>
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 * Copyright (c) 2017-2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#define WG_PORT 52840
#define STACK_SIZE 4096

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

#define RECV_BUFFER_SIZE 2048
#define STATS_TIMER 60 /* How often to print statistics (in seconds) */

#if defined(CONFIG_USERSPACE)
#include <zephyr/app_memory/app_memdomain.h>
extern struct k_mem_partition app_partition;
extern struct k_mem_domain app_domain;
#define APP_BMEM K_APP_BMEM(app_partition)
#define APP_DMEM K_APP_DMEM(app_partition)
#else
#define APP_BMEM
#define APP_DMEM
#endif

struct data {
	const char *proto;

	struct {
		int sock;
		char recv_buffer[RECV_BUFFER_SIZE];
		uint32_t counter;
		atomic_t bytes_received;
		struct k_work_delayable stats_print;
	} udp;
};

struct configs {
	struct data ipv4;
};

extern struct configs conf;

void start_udp(void);
void stop_udp(void);
void quit(void);
int init_tunnel(void);

#endif /*_COMMON_H_*/
