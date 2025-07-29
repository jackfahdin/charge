/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License,  Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,  software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,  either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include "common.h"
#include "cutils/properties.h"
#include "battery.h"
#include <sys/time.h>

#define RETRY_USLEEP_TIME	500000
#define INIT_RETRY_TIMES	50
#define MODE_CHECK_MAX 10
#define MAX_LEN	       32

int
main(int argc,  char **argv) {
	time_t start = time(NULL);
	int ret,  fd = 0;
	char charge_prop[PROPERTY_VALUE_MAX] = {0};
	int is_factory = 0;
	char factory_prop[10] = {0};
	struct stat s;
	int i = 0;

	LOGD("charge start\n");
	thread_ext_ctrl = THREAD_DEFAULT;
	property_get("ro.bootmode", charge_prop, "");
	LOGD("charge_prop: %s\n", charge_prop);
	if (!charge_prop[0] || strncmp(charge_prop, "charge", 6)) {
		LOGE("error: %s exit!\n", __func__);
		return EXIT_SUCCESS;
	}
	for (i = 0; i < INIT_RETRY_TIMES; i++) {
		ret = validate_rtc_time();
		if (!ret)
			break;
		usleep(RETRY_USLEEP_TIME);
	}
	for (i = 0; i < INIT_RETRY_TIMES; i++) {
		ret = ui_init();
		if (!ret)
			break;
		usleep(RETRY_USLEEP_TIME);
	}
	if (ret < 0) {
		LOGE("ui_init failed\n");
		return -1;
	}
	for (i = 0; i < INIT_RETRY_TIMES; i++) {
		ret = battery_status_init();
		if (!ret)
			break;
		usleep(RETRY_USLEEP_TIME);
	}
	if (ret < 0) {
		LOGE("battery_status_init failed\n");
	}
	ui_set_background();
	backlight_init();
	pthread_t t_1,  t_2,  t_3;

	ret = pthread_create(&t_1, NULL, charge_thread, NULL);
	if (ret) {
		LOGE("thread:charge_thread creat failed\n");
		return -1;
	}

	ret = pthread_create(&t_2, NULL, input_thread, NULL);
	if (ret) {
		LOGE("thread:input_thread creat failed\n");
		return -1;
	}

	ret = pthread_create(&t_3, NULL, power_thread, NULL);
	if (ret) {
		LOGE("thread: power_thread creat failed\n");
		return -1;
	}
	pthread_join(t_1,  NULL);
	pthread_join(t_2,  NULL);
	pthread_join(t_3,  NULL);
	LOGD("charge app exit\n");

	return EXIT_SUCCESS;
}
