/* UDP packet reception codes */
/*
 * Copyright (c) 2024 Chunghan Yi <chunghan.yi@gmail.com>
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(wg, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/net/socket.h>

#include "common.h"
#include "wireguardif.h"

extern struct netif *wg_netif;

static void process_udp4(void);

K_THREAD_DEFINE(udp4_thread_id, STACK_SIZE,
		process_udp4, NULL, NULL, NULL,
		THREAD_PRIORITY,
		IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0, -1);

static int start_udp_proto(struct data *data, struct sockaddr *bind_addr,
			   socklen_t bind_addrlen)
{
	int ret;

	data->udp.sock = socket(bind_addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if (data->udp.sock < 0) {
		NET_ERR("Failed to create UDP socket (%s): %d", data->proto, errno);
		return -errno;
	}

	ret = bind(data->udp.sock, bind_addr, bind_addrlen);
	if (ret < 0) {
		NET_ERR("Failed to bind UDP socket (%s): %d", data->proto, errno);
		ret = -errno;
	}

	return ret;
}

static int process_udp(struct data *data)
{
	int ret = 0;
	int r;
	struct pbuf u;
	struct sockaddr_in unknownaddr;
	socklen_t len = sizeof(struct sockaddr_in);
	ip_addr_t addr;
	struct wireguard_device *device = (struct wireguard_device *)(wg_netif->state);

	//NET_INFO("Waiting for UDP packets on port %d (%s)...", WG_PORT, data->proto);

	size_t u_len = 2048; /* TBD */
	u.payload = malloc(u_len);

	do {
		r = recvfrom(data->udp.sock, u.payload, u_len,
				0, (struct sockaddr *)&unknownaddr, &len);
		if (r < 0) {
			NET_ERR("UDP (%s): Connection error %d", data->proto, errno);
			ret = -errno;
			break;
		} else if (r) {
			atomic_add(&data->udp.bytes_received, r);
		}

		NET_DBG("<<  Received a UDP packet: size %d from %s:%d",
				r, inet_ntoa(unknownaddr.sin_addr), ntohs(unknownaddr.sin_port));
		u.len = u.tot_len = r;
		addr.u_addr.ip4.addr = unknownaddr.sin_addr.s_addr;
		wireguardif_network_rx(device, &u, &addr, ntohs(unknownaddr.sin_port));
	} while (true);

	if (u.payload)
		free(u.payload);
	return ret;
}

static void process_udp4(void)
{
	int ret;
	struct sockaddr_in addr4;

	(void)memset(&addr4, 0, sizeof(addr4));
	addr4.sin_family = AF_INET;
	addr4.sin_port = htons(WG_PORT);

	ret = start_udp_proto(&conf.ipv4, (struct sockaddr *)&addr4, sizeof(addr4));
	if (ret < 0) {
		quit();
		return;
	}

	if (wg_netif == NULL) {
		quit();
		return;
	}
	wg_netif->sockfd = conf.ipv4.udp.sock;

	while (ret == 0) {
		ret = process_udp(&conf.ipv4);
		if (ret < 0) {
			quit();
		}
	}
}

static void print_stats(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct data *data = CONTAINER_OF(dwork, struct data, udp.stats_print);
	int total_received = atomic_get(&data->udp.bytes_received);

	if (total_received) {
		if ((total_received / STATS_TIMER) < 1024) {
			LOG_INF("%s UDP: Received %d B/sec", data->proto,
				total_received / STATS_TIMER);
		} else {
			LOG_INF("%s UDP: Received %d KiB/sec", data->proto,
				total_received / 1024 / STATS_TIMER);
		}

		atomic_set(&data->udp.bytes_received, 0);
	}

	k_work_reschedule(&data->udp.stats_print, K_SECONDS(STATS_TIMER));
}

void start_udp(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV4)) {
#if defined(CONFIG_USERSPACE)
		k_mem_domain_add_thread(&app_domain, udp4_thread_id);
#endif

		k_work_init_delayable(&conf.ipv4.udp.stats_print, print_stats);
		k_thread_name_set(udp4_thread_id, "udp4");
		k_thread_start(udp4_thread_id);
		k_work_reschedule(&conf.ipv4.udp.stats_print,
				  K_SECONDS(STATS_TIMER));
	}
}

void stop_udp(void)
{
	/* Not very graceful way to close a thread, but as we may be blocked
	 * in recvfrom call it seems to be necessary
	 */
	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		k_thread_abort(udp4_thread_id);
		if (conf.ipv4.udp.sock >= 0) {
			(void)close(conf.ipv4.udp.sock);
		}
	}
}
