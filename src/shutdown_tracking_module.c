/*
 * Copyright (c) 2019-2020 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/rtc/maxim_ds3231.h>
#include <zephyr/shell/shell.h>

#include "eeprom_memory.h"
#include "rtc.h"

#define GET_TIME_PERIOD_MS	K_MSEC(1000)

static const struct device *ds3231;
static const struct device *eeprom;

static int cmd_get_time(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Current time is %u: %s\n", get_current_time(ds3231), format_time(get_current_time(ds3231), -1));

	return 0;
}

static int cmd_set_time(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "Input string is: %s %s\n", argv[1], argv[2]);

	struct tm tm_time = { 0 };

	sscanf(argv[1], "%d-%d-%d",
        &tm_time.tm_year, &tm_time.tm_mon, &tm_time.tm_mday);

	sscanf(argv[2], "%d:%d:%d",
        &tm_time.tm_hour, &tm_time.tm_min, &tm_time.tm_sec);

	tm_time.tm_year -= 1900; // Year - 1900
    tm_time.tm_mon -= 1;     // Months starts from 0

	time_t utc_time = mktime(&tm_time);

	shell_print(sh, "Parsed UTC Time: %u, %s\n", (uint32_t)utc_time, format_time(utc_time, -1));

	set_current_time(ds3231, utc_time);

	return 0;
}

static int cmd_get_shutdown_time(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	
	shell_print(sh, "Previous shutdown time is %u: %s\n", get_previous_shutdown_time(), format_time(get_previous_shutdown_time(), -1));

	return 0;
}

int shutdown_tracking_module(void)
{
	ds3231 = DEVICE_DT_GET_ONE(maxim_ds3231);
	eeprom = get_eeprom_device();

	SHELL_STATIC_SUBCMD_SET_CREATE(rtc_time,
		SHELL_CMD(get_time, NULL, "Get current time.", cmd_get_time),
		SHELL_CMD(set_time, NULL, "Set current time. input string should be in format YYYY-MM-DD HH:MM:SS", cmd_set_time),
		SHELL_CMD(get_shutdown_time, NULL, "Get last shutdown_time.", cmd_get_shutdown_time),
		SHELL_SUBCMD_SET_END /* Array terminated. */
	);

	SHELL_CMD_REGISTER(rtc, &rtc_time, "RTC control commands", NULL);

	if (rtc_stat_update(ds3231) == 0)
	{
		/*In this place we exit if device not ready*/
		return 0;
	}
	
	compare_previous_time(eeprom, get_current_time(ds3231));

	for (;;)
	{		
		#if 0
		printk("Now %u: %s\n", now, format_time(now, -1));
		#endif
		set_previous_time(eeprom, get_current_time(ds3231));
		k_sleep(GET_TIME_PERIOD_MS);
	}
	//Never reached	
	k_sleep(K_FOREVER);
	return 0;
}
