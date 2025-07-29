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
#include "cutils/properties.h"
#include <sys/time.h>

#define MISCDATA_BASE_ADDR		(10 * 1024 + 256)
#define MISCDATA_MAGIC_OFFSET		0
#define MISCDATA_MAGIC_SIZE		4
#define MISCDATA_RTC_TIME_OFFSET	4
#define MISCDATA_RTC_TIME_SIZE		8
#define MISCDATA_CHARGE_CYCLE_OFFSET	12
#define MISCDATA_CHARGE_CYCLE_SIZE	4
#define MISCDATA_BASP_OFFSET		16
#define MISCDATA_BASP_SIZE		4
#define MISCDATA_CAPACITY_OFFSET	20
#define MISCDATA_CAPACITY_SIZE		4
#define MISCDATA_CAPACITY_CHECK_OFFSET	24
#define MISCDATA_CAPACITY_CHECK_SIZE	4
#define MISCDATA_PATH		"/dev/block/by-name/miscdata"
#define TOTAL_MAH_PATH		"/sys/class/power_supply/battery/charge_full_design"
#define CHARGE_CYCLE_PATH	"/sys/class/power_supply/battery/cycle_count"
#define BASP_PATH		"/sys/class/power_supply/battery/voltage_max_design"

#define CHARGED_TRACK_CAP_KEY0  0x20160726
#define CHARGED_TRACK_CAP_KEY1	0x15211517

#define CHARGED_LOGE(x...)    charged_log_write(3, "<3>charged: " x)
#define CHARGED_LOGW(x...)    charged_log_write(5, "<5>charged: " x)
#define CHARGED_LOGI(x...)    charged_log_write(6, "<6>charged: " x)
#define CHARGED_LOGD(x...)    charged_log_write(6, "<6>charged: " x)
#define CHARGED_LOGV(x...)    charged_log_write(7, "<7>charged: " x)

#define CHARGED_LOG_BUF_MAX 512
#define CHARGED_LOG_LEVEL 7
#define CHARGED_READ_BUF_MAX 15
pthread_mutex_t glogMutex = PTHREAD_MUTEX_INITIALIZER;

static const char *name = "/dev/kmsg";

void charged_log_write(int level, const char *fmt, ...) {
	int log_fd = -1;
	char buf[CHARGED_LOG_BUF_MAX];
	va_list ap;

	if (level > CHARGED_LOG_LEVEL)
		return;

	memset(buf, '\0', sizeof(buf));

	pthread_mutex_lock(&glogMutex);
	log_fd = open(name, O_WRONLY);
	if (log_fd < 0) {
		pthread_mutex_unlock(&glogMutex);
		return;
	}

	va_start(ap, fmt);
	vsnprintf(buf, CHARGED_LOG_BUF_MAX, fmt, ap);
	buf[CHARGED_LOG_BUF_MAX - 1] = 0;
	va_end(ap);
	write(log_fd, buf, strlen(buf));
	close(log_fd);
	pthread_mutex_unlock(&glogMutex);
}

static void charge_set_shutdown_rtc_time(FILE *fp)
{
	time_t seconds;
	long long rtc_time;
	int ret;

	seconds = time(NULL);

	ret = fseek(fp, (MISCDATA_BASE_ADDR + MISCDATA_RTC_TIME_OFFSET), SEEK_SET);
	if (ret) {
		CHARGED_LOGE(" %s: fseek pos[%d] fail ret = %d \n",
			     __func__, MISCDATA_RTC_TIME_OFFSET, ret);
		return;
	}

	/* Note: In arm64 system, time_t is 64 bits.
	 * But in arm32 system, time_t is 32 bits*/
	rtc_time = (long long) seconds;

	ret = fwrite(&rtc_time, 1, MISCDATA_RTC_TIME_SIZE, fp);
	if (ret <= 0)
		CHARGED_LOGE(" %s: fwrite  pos[%d], seconds = %d fail, ret = %d \n",
			     __func__, MISCDATA_RTC_TIME_OFFSET, seconds, ret);
}

static void charge_set_charge_cycle(FILE *fp)
{
	int fd, ret;
	extern int errno;
	int charge_cycle;
	static int last_charge_cycle = -1;
	char read_buf[CHARGED_READ_BUF_MAX] = {'\0'};

	fd = open(CHARGE_CYCLE_PATH, O_RDONLY);
	if (fd < 0) {
		CHARGED_LOGE(" %s: open %s failed errno=%d(%s)\n",
			     __func__,  CHARGE_CYCLE_PATH, errno, strerror(errno));
		return;
	}

	ret = read(fd, read_buf, sizeof(read_buf));
	if (ret < 0) {
		CHARGED_LOGE(" %s: read %s failed errno=%d(%s), ret = %d\n",
			     __func__,  CHARGE_CYCLE_PATH, errno, strerror(errno), ret);
		goto close_charge_cycle_file;
	}

	charge_cycle = atoi(read_buf);
	if (charge_cycle < 0) {
		CHARGED_LOGE(" %s: convert charge cycle = %s failed \n", __func__,  read_buf);
		goto close_charge_cycle_file;
	}

	if (charge_cycle == last_charge_cycle)
		goto close_charge_cycle_file;

	ret = fseek(fp, (MISCDATA_BASE_ADDR + MISCDATA_CHARGE_CYCLE_OFFSET), SEEK_SET);
	if (ret) {
		CHARGED_LOGE(" %s: fseek pos[%d] fail ret = %d \n",
			     __func__, MISCDATA_CHARGE_CYCLE_OFFSET, ret);
		goto close_charge_cycle_file;
	}

	ret = fwrite(&charge_cycle, 1, MISCDATA_CHARGE_CYCLE_SIZE, fp);
	if (ret <= 0) {
		CHARGED_LOGE(" %s: fwrite pos[%d], charge_cycle = %d fail, ret = %d \n",
			     __func__, MISCDATA_CHARGE_CYCLE_OFFSET, charge_cycle, ret);
		goto close_charge_cycle_file;
	}

	if ((charge_cycle / 100) == (last_charge_cycle / 100 + 1))
		CHARGED_LOGI(" %s: store charge_cycle  = %d to miscdata pos[%d], read_buf = %s\n",
			     __func__, charge_cycle, MISCDATA_CHARGE_CYCLE_OFFSET, read_buf);

	last_charge_cycle = charge_cycle;

close_charge_cycle_file:
	if (fd != NULL)
		close(fd);
}

static void charge_set_basp(FILE *fp)
{
	int basp;
	int fd, ret;
	extern int errno;
	static int last_basp = -1;
	char read_buf[CHARGED_READ_BUF_MAX] = {'\0'};

	fd = open(BASP_PATH, O_RDONLY);
	if (fd < 0) {
		CHARGED_LOGE(" %s: open %s failed errno=%d(%s)\n",
			     __func__, BASP_PATH, errno, strerror(errno));
		return;
	}

	ret = read(fd, read_buf, sizeof(read_buf));
	if (ret < 0) {
		CHARGED_LOGE(" %s: read %s failed errno=%d(%s), ret = %d\n",
			     __func__, BASP_PATH, errno, strerror(errno), ret);
		goto close_basp_file;
	}

	basp = atoi(read_buf);
	if (basp < 0) {
		CHARGED_LOGE(" %s: convert basp = %s failed \n", __func__, read_buf);
		goto close_basp_file;
	}


	if (basp == last_basp)
		goto close_basp_file;

	ret = fseek(fp, (MISCDATA_BASE_ADDR + MISCDATA_BASP_OFFSET), SEEK_SET);
	if (ret) {
		CHARGED_LOGE(" %s: fseek pos[%d] fail ret = %d \n",  __func__,
			     MISCDATA_BASP_OFFSET, ret);
		goto close_basp_file;
	}

	ret = fwrite(&basp, 1, MISCDATA_BASP_SIZE, fp);
	if (ret <= 0) {
		CHARGED_LOGE(" %s: fwrite pos[%d], basp = %d fail, ret = %d \n",  __func__,
			     MISCDATA_BASP_OFFSET, basp, ret);
		goto close_basp_file;
	}

	CHARGED_LOGI(" %s: store basp = %d to miscdata pos[%d], read_buf = %s\n",
		     __func__, basp, MISCDATA_BASP_OFFSET, read_buf);

	last_basp = basp;

close_basp_file:
	if (fd != NULL)
		close(fd);
}

static void charge_set_total_mah(FILE *fp)
{
	int fd, ret;
	extern int errno;
	static int last_total_mah = -1;
	int total_mah, check_mah, mah;
	char read_buf[CHARGED_READ_BUF_MAX] = {'\0'};

	fd = open(TOTAL_MAH_PATH, O_RDONLY);
	if (fd < 0) {
		CHARGED_LOGE(" %s: open %s failed errno=%d(%s)\n",
			     __func__, TOTAL_MAH_PATH, errno, strerror(errno));
		return;
	}

	ret = read(fd, read_buf, sizeof(read_buf));
	if (ret < 0) {
		CHARGED_LOGE(" %s: read %s failed errno=%d(%s), ret = %d\n",
			     __func__, TOTAL_MAH_PATH, errno, strerror(errno), ret);
		goto close_total_mab_file;
	}

	mah = atoi(read_buf);
	if (mah <= 0) {
		CHARGED_LOGE(" %s: convert mah = %s failed \n", __func__, read_buf);
		goto close_total_mab_file;
	}

	if (mah == last_total_mah)
		goto close_total_mab_file;

	total_mah = mah ^ CHARGED_TRACK_CAP_KEY0;
	check_mah = mah ^ CHARGED_TRACK_CAP_KEY1;

	ret = fseek(fp, (MISCDATA_BASE_ADDR + MISCDATA_CAPACITY_OFFSET), SEEK_SET);
	if (ret) {
		CHARGED_LOGE(" %s: fseek pos[%d] fail ret = %d \n",
			     __func__,  MISCDATA_CAPACITY_OFFSET, ret);
		goto close_total_mab_file;
	}

	ret = fwrite(&total_mah, 1, MISCDATA_CAPACITY_SIZE, fp);
	if (ret <= 0) {
		CHARGED_LOGE(" %s: fwrite pos[%d], total_mah = %d fail, ret = %d \n",
			     __func__, MISCDATA_CAPACITY_OFFSET, total_mah, ret);
		goto close_total_mab_file;
	}

	ret = fseek(fp, (MISCDATA_BASE_ADDR + MISCDATA_CAPACITY_CHECK_OFFSET), SEEK_SET);
	if (ret) {
		CHARGED_LOGE(" %s: fseek pos[%d] fail ret = %d \n",
			     __func__,  MISCDATA_CAPACITY_CHECK_OFFSET, ret);
		goto close_total_mab_file;
	}

	ret = fwrite(&check_mah, 1, MISCDATA_CAPACITY_CHECK_SIZE, fp);
	if (ret <= 0) {
		CHARGED_LOGE(" %s: fwrite pos[%d], check_mah = %d fail, ret = %d \n",
			     __func__,  MISCDATA_CAPACITY_CHECK_OFFSET, check_mah, ret);
		goto close_total_mab_file;
	}

	CHARGED_LOGI(" %s: store total_mah  = %d to miscdata pos[%d], mah = %d, read_buf = %s\n",
		     __func__, total_mah, MISCDATA_CAPACITY_OFFSET, mah, read_buf);

	last_total_mah = mah;

close_total_mab_file:
	if (fd != NULL)
		close(fd);
}

void *charged_thread(void *cookie) {
	FILE * fp;
	int retry_cnt = 50;

	while (1) {
		fp = fopen(MISCDATA_PATH, "r+");
		if (fp == NULL) {
			CHARGED_LOGI(" %s: open miscdata path fail \n", __func__);
			 usleep(5000000);

			if ((retry_cnt--) == 0) {
				CHARGED_LOGI(" %s: open miscdata path fail \n", __func__);
				break;
			}
			continue;
		}

		retry_cnt = 50;
		charge_set_shutdown_rtc_time(fp);
		charge_set_charge_cycle(fp);
		charge_set_basp(fp);
		charge_set_total_mah(fp);

		if (fp != NULL)
			fclose(fp);

		usleep(15000000);
	}

    return NULL;
}

int main(int argc, char **argv) {
	int ret;

	CHARGED_LOGD("charged start\n");

	pthread_t charge_server;

	ret = pthread_create(&charge_server, NULL, charged_thread, NULL);
	if (ret) {
		CHARGED_LOGE("thread:charged_thread creat failed\n");
		return -1;
	}

	pthread_join(charge_server,  NULL);
	CHARGED_LOGD("charged init exit\n");

	return EXIT_SUCCESS;
}
