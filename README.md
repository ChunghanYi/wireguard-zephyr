# wireguard-zephyr
<span style="color:#d3d3d3">WireGuard for Zephyr RTOS</span>
## Generate a wireguard static key(Curve25519 private/public keypair)
$ cd scripts <br>
$ __./genkey.sh__ <br>
-rw-rw-r-- 1 chyi chyi 45 11월 17 09:28 privatekey <br>
-rw-rw-r-- 1 chyi chyi 45 11월 17 09:28 publickey <br>
## How to build
Caution: <br>
 1) You must first copy this folder under samples/net/sockets. <br>
 2) You must edit the prj.conf and src/wireguard_vpn.h files for vpn settings.<br> 
$ __west build -b nucleo_f207zg samples/net/sockets/wireguard__
## How to run
$ __west flash__ <br>
## My blog posting for this project 
  https://slowbootkernelhacks.blogspot.com/2024/12/zephyr-rtos-programming.html <br>
## Limitations
  It only works in IPv4 environments.<br>
## Reference codes
  https://github.com/smartalock/wireguard-lwip <br>
  The code is copyrighted under BSD 3 clause Copyright (c) 2021 Daniel Hope (www.floorsense.nz)<br>
  <br>
  (***) WireGuard is a registered trademark of Jason A. Donenfeld.

