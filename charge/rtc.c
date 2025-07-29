/********************************************
	unisoc rtc init in POF chg
*********************************************/
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <linux/rtc.h>
#include <linux/input.h>
#include <sys/time.h>
#include "common.h"

#define MODE_CHECK_MAX 10
#define MAX_LEN	       32
#define RTC_TIME_FILE		"/sys/class/rtc/rtc0/time"
#define RTC_DEV_FILE		"/dev/rtc0"

ssize_t validate_rtc_time(void)
{
	ssize_t time_fd, dev_fd,ret;
	char time_buf[MAX_LEN];
	/* default rtc time 2019.1.1 00:00:00 */
	struct rtc_time tm = {
		.tm_mday = 1,
		.tm_mon = 0,
		.tm_year = 119,
	};

	time_fd = open(RTC_TIME_FILE, O_RDONLY);
	if (time_fd < 0) {
		LOGE("open rtc time file failed: %zd, %s\n", time_fd, strerror(errno));
		return time_fd;
	}

	ret = read(time_fd, time_buf, sizeof(time_buf));
	if (ret > 0) {
		LOGD("read directly: time_buf = %s\n", time_buf);
		close(time_fd);
		return 0;
	}

	dev_fd = open(RTC_DEV_FILE, O_RDWR);
	if (dev_fd < 0) {
		LOGE("open /dev/rtc0 failed: %zd, %s\n", dev_fd, strerror(errno));
		close(time_fd);
		return dev_fd;
	}

	ret = ioctl(dev_fd, RTC_SET_TIME, &tm);
	if (ret < 0) {
		LOGE("ioctl /dev/rtc0 failed: %zd, %s\n", ret, strerror(errno));
		close(dev_fd);
		close(time_fd);
		return ret;
	}

	ret = read(time_fd, time_buf, sizeof(time_buf));
	if (ret > 0) {
		LOGD("read after set default time: time_buf = %s\n", time_buf);
		close(dev_fd);
		close(time_fd);
		return 0;
	}

	LOGD("read time_fd: ret = %zd\n", ret);
	close(dev_fd);
	close(time_fd);

	return -1;
}
