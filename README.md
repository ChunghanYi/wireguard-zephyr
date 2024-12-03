# wireguard-zephyr
<span style="color:#d3d3d3">WireGuard for Zephyr RTOS</span>
## Generate a wireguard static key(Curve25519 private/public keypair)
$ cd scripts <br>
$ __./genkey.sh__ <br>
-rw-rw-r-- 1 chyi chyi 45 11월 17 09:28 privatekey <br>
-rw-rw-r-- 1 chyi chyi 45 11월 17 09:28 publickey <br>
## How to build
Please copy this folder under the samples/net/sockets.
$ __west build -b nucleo_f207zg samples/net/sockets/wireguard__
## How to run
Caution: You must edit the prj.conf and src/wireguard_vpn.h files.<br> 
$ __west flash__ <br>
## Limitations
  It only works in IPv4 environments.<br>
## Reference codes
  https://github.com/smartalock/wireguard-lwip <br>
  The code is copyrighted under BSD 3 clause Copyright (c) 2021 Daniel Hope (www.floorsense.nz)
## My blogs for wireguard analysis
  https://slowbootkernelhacks.blogspot.com/2024/12/zephyr-rtos-programming.html <br><br>
  https://slowbootkernelhacks.blogspot.com/2020/09/wireguard-vpn.html <br>
  https://slowbootkernelhacks.blogspot.com/2023/02/nanopi-r4s-pq-wireguard-vpn-router.html <br>
  https://slowbootkernelhacks.blogspot.com/2024/06/layer-2-wireguard-vpn.html <br>
  https://slowbootkernelhacks.blogspot.com/2024/05/nanopi-wireguard-go-quantum-safe-vpn.html <br>
  https://slowbootkernelhacks.blogspot.com/2023/02/esp32-wireguard-nat-router-pqc.html <br>
  https://slowbootkernelhacks.blogspot.com/2023/01/orangepi-r1-plus-lts-pqc-wireguard-vpn.html <br>
  <br>
  (***) WireGuard is a registered trademark of Jason A. Donenfeld.

