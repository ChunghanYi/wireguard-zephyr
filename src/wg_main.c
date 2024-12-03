/* wireguard startup codes */
/*
 * Copyright (c) 2024 Chunghan Yi <chunghan.yi@gmail.com>
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wg, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <errno.h>
#include <zephyr/shell/shell.h>

#include <zephyr/net/net_core.h>

#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>

#include "common.h"
#include "wireguard_vpn.h"
#include "wireguardif.h"
#include "wg_timer.h"

#define APP_BANNER "wireguard"

int init_dhcp4_client(void);

static struct k_sem quit_lock;
static struct net_mgmt_event_callback mgmt_cb;
static bool connected;
K_SEM_DEFINE(run_app, 0, 1);
static bool want_to_quit;

#if defined(CONFIG_USERSPACE)
K_APPMEM_PARTITION_DEFINE(app_partition);
struct k_mem_domain app_domain;
#endif

#define EVENT_MASK (NET_EVENT_L4_CONNECTED | \
		    NET_EVENT_L4_DISCONNECTED)

APP_DMEM struct configs conf = {
	.ipv4 = {
		.proto = "IPv4",
	},
};

struct netif *wg_netif = NULL;

void quit(void)
{
	k_sem_give(&quit_lock);
}

static void start_udp_service(void)
{
	LOG_INF("Starting udp service...");

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		start_udp();
	}
}

static void stop_udp_service(void)
{
	LOG_INF("Stopping udp service...");

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		stop_udp();
	}
}

static void event_handler(struct net_mgmt_event_callback *cb,
			  uint32_t mgmt_event, struct net_if *iface)
{
	//ARG_UNUSED(iface);
	ARG_UNUSED(cb);

	if (wg_netif) {
		wg_netif->eth_if = iface;
	}

	if ((mgmt_event & EVENT_MASK) != mgmt_event) {
		return;
	}

	if (want_to_quit) {
		k_sem_give(&run_app);
		want_to_quit = false;
	}

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");

		connected = true;
		k_sem_give(&run_app);

		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		if (connected == false) {
			LOG_INF("Waiting network to be connected");
		} else {
			LOG_INF("Network disconnected");
			connected = false;
		}

		k_sem_reset(&run_app);

		return;
	}
}

static void init_app(void)
{
#if defined(CONFIG_USERSPACE)
	struct k_mem_partition *parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&app_partition
	};

	int ret = k_mem_domain_init(&app_domain, ARRAY_SIZE(parts), parts);

	__ASSERT(ret == 0, "k_mem_domain_init() failed %d", ret);
	ARG_UNUSED(ret);
#endif

	k_sem_init(&quit_lock, 0, K_SEM_MAX_LIMIT);

	LOG_INF(APP_BANNER);

	wg_netif = (struct netif *)malloc(sizeof(struct netif));
	if (wg_netif == NULL) {
		LOG_ERR("Could not allocate struct netif");
	}

	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		net_mgmt_init_event_callback(&mgmt_cb,
					     event_handler, EVENT_MASK);
		net_mgmt_add_event_callback(&mgmt_cb);

		conn_mgr_mon_resend_status();
	}

	init_dhcp4_client();
	wireguard_setup();
	init_tunnel();
}

static int cmd_quit(const struct shell *sh,
			  size_t argc, char *argv[])
{
	want_to_quit = true;

	conn_mgr_mon_resend_status();

	quit();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sample_commands,
	SHELL_CMD(quit, NULL,
		  "Quit the WG application\n",
		  cmd_quit),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sample, &sample_commands,
		   "WG application commands", NULL);

int main(void)
{
	init_app();

	if (!IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		/* If the config library has not been configured to start the
		 * app only after we have a connection, then we can start
		 * it right away.
		 */
		k_sem_give(&run_app);
	}

	/* Wait for the connection. */
	k_sem_take(&run_app, K_FOREVER);

	start_udp_service();

	k_sem_take(&quit_lock, K_FOREVER);

	if (connected) {
		stop_udp_service();
	}

	stop_wg_timer();
	if (wg_netif)
		free(wg_netif);
	return 0;
}
