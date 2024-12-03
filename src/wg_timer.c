/*
 * Copyright (c) 2024 Chunghan Yi <chunghan.yi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(wg, LOG_LEVEL_DBG);

extern void wireguardif_tmr(struct k_timer *timer);

K_TIMER_DEFINE(wg_timer, wireguardif_tmr, NULL);

int start_wg_timer(uint32_t period)
{
	k_timer_start(&wg_timer, K_MSEC(period), K_MSEC(period));
	return 0;
}

int stop_wg_timer(void)
{
	k_timer_stop(&wg_timer);
	return 0;
}
