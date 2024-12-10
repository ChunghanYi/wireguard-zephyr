# wireguard-zephyr
<span style="color:#d3d3d3">WireGuard for Zephyr RTOS</span>
## Generate a wireguard static key(Curve25519 private/public keypair)
```
$ cd scripts <br>
$ ./genkey.sh <br>
-rw-rw-r-- 1 chyi chyi 45 11월 17 09:28 privatekey
-rw-rw-r-- 1 chyi chyi 45 11월 17 09:28 publickey
```
## How to build and run
  Caution: <br>
  You must first copy this folder under samples/net/sockets. <br>
  You must edit the prj.conf and src/wireguard_vpn.h files for vpn settings.<br><br>
```
$ cp -R ./wireguard-zephyr $YOUR_PATH/zephyr/samples/net/sockets/wireguard

$ vi subsys/net/ip/icmpv4.c
-> See the patch/subsys/net/ip/icmpv4.c or icmpv4.c.patch file
enum net_verdict net_icmpv4_input(struct net_pkt *pkt,
                  struct net_ipv4_hdr *ip_hdr)
{
	...
	if (net_if_need_calc_rx_checksum(net_pkt_iface(pkt), NET_IF_CHECKSUM_IPV4_ICMP) ||
		net_pkt_is_ip_reassembled(pkt)) {
		if (net_calc_chksum_icmpv4(pkt) != 0U) {
			NET_DBG("DROP: Invalid checksum");
#if 0 /* ORIG_CODE - blocked for wireguard porting */
			goto drop;
#endif
		}
	}
	...
}

$ cd $YOUR_PATH/zephyr
$ vi samples/net/wireguard/prj.conf
-> Fix some configurations
CONFIG_NET_CONFIG_MY_IPV4_ADDR="192.168.8.50"
CONFIG_NET_CONFIG_MY_IPV4_NETMASK="255.255.255.0"
CONFIG_NET_CONFIG_PEER_IPV4_ADDR="192.168.8.1"
CONFIG_NET_CONFIG_VPN_IPV4_ADDR="10.1.1.50"
CONFIG_NET_CONFIG_VPN_IPV4_NETMASK="255.255.255.0"

$ vi samples/net/wireguard/src/wireguard_vpn.h
-> Fix some configurations
#define WG_LOCAL_ADDRESS        IPADDR4_INIT_BYTES(10, 1, 1, 50)   // my vpn ip address
#define WG_LOCAL_NETMASK        IPADDR4_INIT_BYTES(255, 255, 255, 0)
#define WG_LOCAL_NETWORK        IPADDR4_INIT_BYTES(10, 1, 1, 0)
#define WG_CLIENT_PRIVATE_KEY   "kL/HdaoIlqlDmrjtIkb/0PmF+3N7eApdkrjUQvsbK0c="
#define WG_CLIENT_PORT          51820
#define WG_PEER_PUBLIC_KEY      "isbaRdaRiSo5/WtqEdmpH+NrFeT1+QoLvnhVI1oFfhE="
#define WG_PEER_PORT            51820
#define WG_ENDPOINT_ADDRESS     IPADDR4_INIT_BYTES(192, 168, 8, 139)  //peer endpoint(real) ip address

$ west build -b nucleo_f207zg samples/net/sockets/wireguard
$ west flash

```
The code above was tested in a zephyr 3.7.0 and 4.0 environment with stm32 board which has an ethernet port.<br><br>
Good luck~ 😎
## My blog posting for this project
  For more information, please read the blog posting below.<br>
  https://slowbootkernelhacks.blogspot.com/2024/12/wireguard-for-zephyr-rtos.html <br><br>
  Caution: <br>
  <span style="color:red">You must first proceed with the patch by referring to the subsys/net/ip/icmpv4.c.patch file.</span> <br><br>
## Limitations
  <span style="color:blue">It is still in the early stages of development.</span><br>
  It only works in IPv4 and ethernet environments(Wi-Fi support is under consideration).<br>
## Reference codes
  https://github.com/smartalock/wireguard-lwip <br>
  The code is copyrighted under BSD 3 clause Copyright (c) 2021 Daniel Hope (www.floorsense.nz)<br><br>
  zephyr/samples/net/dhcpv4_client <br>
  zephyr/samples/net/sockets <br>
  zephyr/samples/net/virtual <br>
  <br>
  (***) WireGuard is a registered trademark of Jason A. Donenfeld.

