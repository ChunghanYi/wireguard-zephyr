# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sockets_wireguard)

target_sources(                     app PRIVATE src/wg_main.c)
target_sources(                     app PRIVATE src/dhcpc.c)
target_sources_ifdef(CONFIG_NET_UDP app PRIVATE src/wg_comm.c)
target_sources(                     app PRIVATE src/wg_tun.c)
target_sources(                     app PRIVATE src/wireguard_vpn.c)
target_sources(                     app PRIVATE src/wireguardif.c)
target_sources(                     app PRIVATE src/wireguard.c)
target_sources(                     app PRIVATE src/wireguard-platform.c)
target_sources(                     app PRIVATE src/wg_timer.c)
target_sources(                     app PRIVATE src/crypto.c)
target_sources(                     app PRIVATE src/crypto/blake2s.c)
target_sources(                     app PRIVATE src/crypto/chacha20.c)
target_sources(                     app PRIVATE src/crypto/chacha20poly1305.c)
target_sources(                     app PRIVATE src/crypto/poly1305-donna.c)
target_sources(                     app PRIVATE src/crypto/x25519.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(inc_file)
  generate_inc_file_for_target(
    app
    src/${inc_file}
    ${gen_dir}/${inc_file}.inc
    )
endforeach()
