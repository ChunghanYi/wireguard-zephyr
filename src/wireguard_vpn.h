#ifndef _WIREGUARD_VPN_H_
#define _WIREGUARD_VPN_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* 
 * Caution:
 * The prj.conf file also has network setup information, so you need to match it with the information below.
 */

#define WG_LOCAL_ADDRESS        IPADDR4_INIT_BYTES(10, 1, 1, 50)   // my vpn ip address
#define WG_LOCAL_NETMASK        IPADDR4_INIT_BYTES(255, 255, 255, 0)
#define WG_LOCAL_NETWORK        IPADDR4_INIT_BYTES(10, 1, 1, 0)

#define WG_CLIENT_PRIVATE_KEY   "kL/HdaoIlqlDmrjtIkb/0PmF+3N7eApdkrjUQvsbK0c="
#define WG_CLIENT_PORT          51820

#define WG_PEER_PUBLIC_KEY      "isbaRdaRiSo5/WtqEdmpH+NrFeT1+QoLvnhVI1oFfhE="
#define WG_PEER_PORT            51820
#define WG_ENDPOINT_ADDRESS     IPADDR4_INIT_BYTES(192, 168, 8, 139)  //peer endpoint(real) ip address

void wireguard_setup(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _WIREGUARD_VPN_H_
