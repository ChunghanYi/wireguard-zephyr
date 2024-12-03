/*
 * Copyright (c) 2024 Chunghan Yi <chunghan.yi@gmail.com>
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(wg, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>

#include "wireguardif.h"
#include "lwip_h/ip4.h"

extern struct netif *wg_netif;

/* User data for the interface callback */
struct ud {
	struct net_if *my_iface;
	struct net_if *ethernet;
};

/* The MTU value here is just an arbitrary number for testing purposes */
#define WIREGUARD_MTU 1420
#define WG_VIRTUAL "zwg"
static const char *dev_name = WG_VIRTUAL;

struct virtual_wg_context {
	struct net_if *iface;
	struct net_if *attached_to;
	bool status;
	bool init_done;
};

static void virtual_wg_iface_init(struct net_if *iface)
{
	struct virtual_wg_context *ctx = net_if_get_device(iface)->data;
	char name[sizeof("zwg-+##########")];
	static int count;

	if (ctx->init_done) {
		return;
	}

	ctx->iface = iface;
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	snprintk(name, sizeof(name), "zwg-%d", count + 1);
	count++;
	net_virtual_set_name(iface, name);
	(void)net_virtual_set_flags(iface, NET_L2_POINT_TO_POINT);

	ctx->init_done = true;
}

static struct virtual_wg_context virtual_wg_context_data1 = {
};

static enum virtual_interface_caps
virtual_wg_get_capabilities(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return (enum virtual_interface_caps)0;
}

static int virtual_wg_interface_start(const struct device *dev)
{
	struct virtual_wg_context *ctx = dev->data;

	if (ctx->status) {
		return -EALREADY;
	}

	ctx->status = true;

	LOG_DBG("Starting iface %d", net_if_get_by_iface(ctx->iface));

	return 0;
}

static int virtual_wg_interface_stop(const struct device *dev)
{
	struct virtual_wg_context *ctx = dev->data;

	if (!ctx->status) {
		return -EALREADY;
	}

	ctx->status = false;

	LOG_DBG("Stopping iface %d", net_if_get_by_iface(ctx->iface));

	return 0;
}

static int virtual_wg_interface_send(struct net_if *iface,
				       struct net_pkt *pkt)
{
	struct virtual_wg_context *ctx = net_if_get_device(iface)->data;
	int r;
	struct pbuf u;
	ip_addr_t addr;
	struct ip_hdr *ip;
	int real_len = net_pkt_get_len(pkt);

	if (ctx->attached_to == NULL) {
		return -ENOENT;
	}

	u.payload = pkt->frags->data;
	r = real_len;
	ip = (struct ip_hdr *)u.payload;
	LOG_DBG("<< Sending a VPN message: size %d from SRC = %"PRIu32".%"PRIu32".%"PRIu32".%"PRIu32" to DST = %"PRIu32".%"PRIu32".%"PRIu32".%"PRIu32"",
			r,
			(ntohl(ip->src.addr)  >> 24) & 0xFF,
			(ntohl(ip->src.addr)  >> 16) & 0xFF,
			(ntohl(ip->src.addr)  >>  8) & 0xFF,
			(ntohl(ip->src.addr)  >>  0) & 0xFF,
			(ntohl(ip->dest.addr) >> 24) & 0xFF,
			(ntohl(ip->dest.addr) >> 16) & 0xFF,
			(ntohl(ip->dest.addr) >>  8) & 0xFF,
			(ntohl(ip->dest.addr) >>  0) & 0xFF);

	u.len = u.tot_len = r;
	addr.u_addr.ip4.addr = ip->dest.addr;
	wireguardif_output(wg_netif, &u, &addr);

	net_pkt_unref(pkt);
	return NET_CONTINUE;
}

static enum net_verdict virtual_wg_interface_recv(struct net_if *iface,
						    struct net_pkt *pkt)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(pkt);

	return NET_CONTINUE;
}

static int virtual_wg_interface_attach(struct net_if *virtual_iface,
					 struct net_if *iface)
{
	struct virtual_wg_context *ctx = net_if_get_device(virtual_iface)->data;

	LOG_INF("This tunnel interface %d/%p attached to %d/%p",
		net_if_get_by_iface(virtual_iface), virtual_iface,
		net_if_get_by_iface(iface), iface);

	ctx->attached_to = iface;

	return 0;
}

static const struct virtual_interface_api virtual_wg_iface_api = {
	.iface_api.init = virtual_wg_iface_init,

	.get_capabilities = virtual_wg_get_capabilities,
	.start = virtual_wg_interface_start,
	.stop = virtual_wg_interface_stop,
	.send = virtual_wg_interface_send,
	.recv = virtual_wg_interface_recv,
	.attach = virtual_wg_interface_attach,
};

NET_VIRTUAL_INTERFACE_INIT(virtual_wg1, WG_VIRTUAL, NULL, NULL,
			   &virtual_wg_context_data1,
			   NULL,
			   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			   &virtual_wg_iface_api,
			   WIREGUARD_MTU);

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct ud *ud = user_data;

	if ((net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) &&
	    (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL))) {
		return;
	}

	if (!ud->ethernet && net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		ud->ethernet = iface;
		return;
	}

	if (!ud->my_iface && net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL) &&
	    net_if_get_device(iface)->data == &virtual_wg_context_data1) {
		ud->my_iface = iface;
		return;
	}
}

static int setup_iface(struct net_if *iface,
		       const char *ipv6_addr,
		       const char *ipv4_addr,
		       const char *peer6addr,
		       const char *peer4addr,
		       const char *netmask)
{
	struct virtual_interface_req_params params = { 0 };
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV4) &&
	    net_if_flag_is_set(iface, NET_IF_IPV4)) {

		if (net_addr_pton(AF_INET, ipv4_addr, &addr4)) {
			LOG_ERR("Invalid address: %s", ipv4_addr);
			return -EINVAL;
		}

		ifaddr = net_if_ipv4_addr_add(iface, &addr4,
					      NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			LOG_ERR("Cannot add %s to interface %p",
				ipv4_addr, iface);
			return -EINVAL;
		}

		if (netmask) {
			struct in_addr nm;

			if (net_addr_pton(AF_INET, netmask, &nm)) {
				LOG_ERR("Invalid netmask: %s", netmask);
				return -EINVAL;
			}

			net_if_ipv4_set_netmask_by_addr(iface, &addr4, &nm);
		}

		if (!peer4addr || *peer4addr == '\0') {
			goto done;
		}

		params.family = AF_INET;

		if (net_addr_pton(AF_INET, peer4addr, &addr4)) {
			LOG_ERR("Cannot parse peer %s address %s to tunnel",
				"IPv4", peer4addr);
		} else {
			net_ipaddr_copy(&params.peer4addr, &addr4);

			ret = net_mgmt(
				NET_REQUEST_VIRTUAL_INTERFACE_SET_PEER_ADDRESS,
				iface, &params, sizeof(params));
			if (ret < 0 && ret != -ENOTSUP) {
				LOG_ERR("Cannot set peer %s address %s to "
					"interface %d (%d)",
					"IPv4", peer4addr,
					net_if_get_by_iface(iface),
					ret);
			}
		}

		params.mtu = NET_ETH_MTU - sizeof(struct net_ipv4_hdr);

		ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_MTU,
			       iface, &params,
			       sizeof(struct virtual_interface_req_params));
		if (ret < 0 && ret != -ENOTSUP) {
			LOG_ERR("Cannot set interface %d MTU to %d (%d)",
				net_if_get_by_iface(iface), params.mtu, ret);
		}
	}

done:
	return 0;
}

int init_tunnel(void)
{
#define MAX_NAME_LEN 32
	char buf[MAX_NAME_LEN];
	struct ud ud;
	int ret;

	LOG_INF("Start tunnel service (dev %s/%p)", dev_name, device_get_binding(dev_name));

	memset(&ud, 0, sizeof(ud));
	net_if_foreach(iface_cb, &ud);

	LOG_INF("Tunnel interface %d (%s / %p)",
		net_if_get_by_iface(ud.my_iface),
		net_virtual_get_name(ud.my_iface, buf, sizeof(buf)),
		ud.my_iface);

#if 0
	LOG_INF("Ethernet interface %d (%p)",
		net_if_get_by_iface(ud.ethernet), ud.ethernet);
#endif

	/* Attach the network interfaces on top of the Ethernet interface */
	net_virtual_interface_attach(ud.my_iface, ud.ethernet);

	ret = setup_iface(ud.my_iface,
			  NULL,
			  CONFIG_NET_CONFIG_VPN_IPV4_ADDR,
			  NULL, NULL,
			  CONFIG_NET_CONFIG_VPN_IPV4_NETMASK);
	if (ret < 0) {
		LOG_ERR("Cannot set IP address to test interface");
	}

	if (wg_netif) {
		wg_netif->tun_if = ud.my_iface;
	}

	return 0;
}
