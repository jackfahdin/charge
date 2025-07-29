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

#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <math.h>

#include "common.h"
#include "minui/minui.h"
#include "battery.h"
#include <errno.h>

#define MAX_COLS 64
#define MAX_ROWS 32

#define CHAR_WIDTH 10
#define CHAR_HEIGHT 18

#define PICTURE_SHOW_PERCENT_SUPPORT

static pthread_mutex_t gUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gchargeMutex = PTHREAD_MUTEX_INITIALIZER;
static gr_surface gProgressBarIndeterminate[PROGRESSBAR_INDETERMINATE_STATES];
static gr_surface gProgressBarEmpty;
static gr_surface gProgressBarFill;
static gr_surface gNumber[10];
static gr_surface gPercent;
static gr_surface gColon;
static gr_surface gProgressBarError[3];

int alarm_flag_check(void);
extern int rotate;

const char *gm = "_360X640";
const char *gh = "_480X800";
const char *gxh = "_720X1280";
const char *gxxh = "_1080X1920";
const char *gxxxh = "_1440X2560";
const char *gIndeterminate = "indeterminate";
const char *gNo = "number";
const char *gError = "error";
const char *ro = "_rotate";

char gPer[33] = "number_percent";
char gCol[33] = "colon";
char gErr[3][33] = {0};
char gIndex[7][33] = {0};
char gNoIndex[10][33] = {0};

static int res_pixel_identify(void)
{
	int x = gr_fb_width();
	int y = gr_fb_height();
	int size = 0;

	if((x<= 360) && (y <= 640)){
		size = SIZE_360X640;
	}else if((x<=480) && (y<=800)){
		size = SIZE_480X800;
	}else if((x<=480) && (y<=854)) {
		size = SIZE_480X800;
		LOGD("480 * 854 Use res of 480 * 800\n");
	}else if((x<=720) && (y<=1280)){
		size = SIZE_720X1280;
	}else if((x<=1080) && (y<=1920)){
		size = SIZE_1080X1920;
	}else if((x<=1440) && (y<=2560)){
		size = SIZE_1440X2560;
	}else{
		LOGE("Picture size is not standard size\n");
	}

		return size;
}

static void res_init(void)
{
	int i = 0;
	int j = 0;
	int k = 0;
	char *temp;

	switch (res_pixel_identify()){
		case SIZE_360X640:
				temp =(char *) gm;
			break;
		case SIZE_480X800:
				temp =(char *) gh;
			break;
		case SIZE_720X1280:
				temp =(char *) gxh;
			break;
		case SIZE_1080X1920:
				temp =(char *) gxxh;
			break;
		case SIZE_1440X2560:
				temp =(char *) gxxxh;
			break;
		default:
				temp =(char *) gm;
			break;
	}

	for(i = 0; i<=6; i++){
		if(rotate)
			snprintf(&gIndex[i][0], sizeof(gIndex[i])-1, "%s%d%s%s", gIndeterminate,i,temp,ro);
		else
			snprintf(&gIndex[i][0], sizeof(gIndex[i])-1, "%s%d%s", gIndeterminate,i,temp);
		LOGD("picture is %s\n",gIndex[i]);
	}

	for(j = 0;j<10;j++){
		if(rotate)
			snprintf(&gNoIndex[j][0], sizeof(gNoIndex[j])-1, "%s_%d%s%s", gNo,j,temp,ro);
		else
			snprintf(&gNoIndex[j][0], sizeof(gNoIndex[j])-1,"%s_%d%s", gNo,j,temp);
		LOGD("number is %s\n",gNoIndex[j]);
	}

	for(k = 0;k<3;k++){
		if(rotate)
			snprintf(&gErr[k][0], sizeof(gErr[k])-1,"%s_%d%s%s", gError,k+1,temp,ro);
		else
			snprintf(&gErr[k][0], sizeof(gErr[k])-1,"%s_%d%s", gError,k+1,temp);
		LOGD("error is %s\n",gErr[k]);
	}

	if(rotate)
		snprintf(gPer + 14,sizeof(gPer) - 15,"%s%s",temp,ro);
	else
		snprintf(gPer + 14,sizeof(gPer) - 15,"%s",temp);

	snprintf(gCol + 5,sizeof(gCol) - 6,"%s",temp);

	return;
}

static int charge_health_check(void)
{
	int health_status = battery_health();
	int value = 0;
	switch (health_status){
		case BATTERY_HEALTH_OVERHEAT:
			value = 1;
			break;
		case BATTERY_HEALTH_COLD:
			value = 2;
			break;
		case BATTERY_HEALTH_OVER_VOLTAGE:
			value = 3;
			break;
		default:
			value = 0;
			break;
	}
	if(value > 0){
		backlight_on();
		set_screen_state(1);
	}
	return value;
}

static const struct { gr_surface* surface; char *name; } BITMAPS[] = {
        { &gProgressBarIndeterminate[0],	&gIndex[0][0] },
        { &gProgressBarIndeterminate[1],	&gIndex[1][0] },
        { &gProgressBarIndeterminate[2],	&gIndex[2][0] },
        { &gProgressBarIndeterminate[3],	&gIndex[3][0] },
        { &gProgressBarIndeterminate[4],	&gIndex[4][0] },
        { &gProgressBarIndeterminate[5],	&gIndex[5][0] },
        { &gProgressBarIndeterminate[6],	&gIndex[6][0] },
        { &gProgressBarEmpty,		&gIndex[0][0] },
        { &gProgressBarFill,		&gIndex[6][0] },
        { &gNumber[0],		&gNoIndex[0][0]},
        { &gNumber[1],		&gNoIndex[1][0]},
        { &gNumber[2],		&gNoIndex[2][0]},
        { &gNumber[3],		&gNoIndex[3][0]},
        { &gNumber[4],		&gNoIndex[4][0]},
        { &gNumber[5],		&gNoIndex[5][0]},
        { &gNumber[6],		&gNoIndex[6][0]},
        { &gNumber[7],		&gNoIndex[7][0]},
        { &gNumber[8],		&gNoIndex[8][0]},
        { &gNumber[9],		&gNoIndex[9][0]},
        { &gPercent,	gPer},
        { &gColon,		gCol},
        { &gProgressBarError[0], 	&gErr[0][0]},
        { &gProgressBarError[1], 	&gErr[1][0]},
        { &gProgressBarError[2], 	&gErr[2][0]},
        { NULL,		NULL },
};

static gr_surface gCurrentIcon = NULL;

static enum ProgressBarType {
    PROGRESSBAR_TYPE_NONE,
        PROGRESSBAR_TYPE_INDETERMINATE,
        PROGRESSBAR_TYPE_NORMAL,
} gProgressBarType = PROGRESSBAR_TYPE_NORMAL;


static int text_col = 0,  text_row = 0,  text_top = 0;

static void draw_background_locked(gr_surface icon) {
    gr_color(0,  0,  0,  255);
    gr_fill(0,  0,  gr_fb_width(),  gr_fb_height());
    if (icon) {
        int iconWidth = gr_get_width(icon);
        int iconHeight = gr_get_height(icon);
        int iconX = (gr_fb_width() - iconWidth) / 2;
        int iconY = (gr_fb_height() - iconHeight) / 2;
        gr_blit(icon,  0,  0,  iconWidth,  iconHeight,  iconX,  iconY);
    }
}


#ifndef PICTURE_SHOW_PERCENT_SUPPORT
static void draw_text_xy(int row,  int col,  const char* t) {
    if (t[0] != '\0') {
        gr_text(col,  row,  t, 0);
    }
}
#endif

static void draw_text_picture(int level)
{
	int hundred = 0, ten = 0, bit = 0;
	int width = gr_get_width(gNumber[0]);
	int height = gr_get_height(gNumber[0]);
	int capacity_w = gr_get_width(gPercent);
	int catpcity_h = gr_get_height(gPercent);
	int ProgressBar_h, ProgressBar_w;
	int dx, dy;

	hundred = level/100;
	ten = (level%100)/10;
	bit = level%10;
	if(rotate) {
		ProgressBar_w = gr_get_width(gProgressBarIndeterminate[0]);
		dx = gr_fb_width()/2 + ProgressBar_w/2 +  height/2;
		dy = (gr_fb_height() - height*4 - catpcity_h)/2;
		if(hundred == 1)
			gr_blit(gNumber[hundred],0,0,width,height,dx,dy);
		if(level >= 10)
			gr_blit(gNumber[ten],0,0,width,height,dx,dy + height);
		gr_blit(gNumber[bit],0,0,width,height,dx,dy + height*2);
		gr_blit(gPercent,0,0,capacity_w,catpcity_h,dx,dy + height*3);
	} else {
		ProgressBar_h = gr_get_height(gProgressBarIndeterminate[0]);
		dx = (gr_fb_width() - width*4 - capacity_w)/2;
		dy = gr_fb_height()/2 - ProgressBar_h/2 -  height *2;
		if(hundred == 1)
			gr_blit(gNumber[hundred],0,0,width,height,dx,dy);
		if(level >= 10)
			gr_blit(gNumber[ten],0,0,width,height,dx + width,dy);
		gr_blit(gNumber[bit],0,0,width,height,dx + width*2 ,dy);
		gr_blit(gPercent,0,0,capacity_w,catpcity_h,dx + width*3,dy);
	}
}

#ifdef SHOW_TIME_DATE_SUPPORT
static void draw_time_line(void)
{
	int hour,hour_unit,min,min_unit,sec,sec_unit,hour_type;
	char timer_buf[50];
	time_t t_Now;
	size_t result;
	struct tm* t_time;
	char time_zone[PROPERTY_VALUE_MAX] = {0};
	int i;

	int width = gr_get_width(gNumber[0]);
	int height = gr_get_height(gNumber[0]);
	int colon_w = gr_get_width(gColon);
	int colon_h = gr_get_height(gColon);

	int dx = (gr_fb_width() - width*4 - colon_w)/2;   // set fist number persion
	int dy= gr_fb_height()/2 + gr_get_height(gProgressBarIndeterminate[0])/2 +  height;

	property_get("persist.sys.timezone",  time_zone,  "");
	if(time_zone[0] == '\0'){
		LOGE("time_zone get failed\n");
		return;
	}else{
		LOGD("time_zone =  %s\n",time_zone);
	}

	t_Now = time(NULL);
	t_time = localtime(&t_Now);

	hour = t_time->tm_hour/10;
	hour_unit = t_time->tm_hour%10;
	min = t_time->tm_min/10;
	min_unit = t_time->tm_min%10;
    LOGE("t_time->tm_hour = %d t_time->tm_min =%d\n", t_time->tm_hour, t_time->tm_min);

	gr_blit(gNumber[hour], 0, 0, width, height, dx, dy);
	gr_blit(gNumber[hour_unit], 0, 0, width, height, dx+width, dy);

	gr_blit(gColon, 0, 0, colon_w, colon_h, dx+width*2, dy);

	gr_blit(gNumber[min], 0, 0, width, height, dx+width*2+colon_w, dy);
	gr_blit(gNumber[min_unit], 0, 0, width, height, dx+width*3+colon_w, dy);

	memset(timer_buf,0,sizeof(timer_buf));
	result = strftime(timer_buf , sizeof(timer_buf) , "%Y-%m-%d" , t_time);

	if(result !=0){
		gr_color(34,197,11,255);
#ifndef PICTURE_SHOW_PERCENT_SUPPORT
		draw_text_xy(dy + 90, dx + width, timer_buf);
#endif
	}
}
#endif

char bat[10]={0};
static void draw_progress_locked(int level) {
#if CIRCLE_CHARGE_UI_SUPPORT
    int is_fast_charging = 0; // TODO: battery.c获取快充状态
    gr_sync();
    gr_color(0, 0, 0, 255);
    gr_fill(0, 0, gr_fb_width(), gr_fb_height());
    draw_circle_charge_ui(level, is_fast_charging);
    gr_flip();
    return;
#else
    gr_sync();
    int width = gr_get_width(gProgressBarEmpty);
    int height = gr_get_height(gProgressBarEmpty);

    int dx = (gr_fb_width() - width)/2;
    int dy = (gr_fb_height() - height)/2;

    static int frame = 0;

    // Erase behind the progress bar (in case this was a progress-only update)
    gr_color(0,  0,  0,  255);
    gr_fill(0,  0,  gr_fb_width(),  gr_fb_height());

    gr_color(64,  96,  255,  255);

	if( status_index > 0){
#ifdef PICTURE_SHOW_PERCENT_SUPPORT
		draw_text_picture(level);
#else
		draw_text_xy((dy + height),  (gr_fb_width()/2 - 20),  bat);
#endif
		led_off();
		gr_blit(gProgressBarError[status_index-1],  0,  0,  width,  height,  dx,  dy);

#ifdef SHOW_TIME_DATE_SUPPORT
		draw_time_line();
#endif
		gr_flip();
		return;
	}

    if (level > 100)
        level = 100;
    else if (level < 0)
        level = 0;

    sprintf(bat,  "%d%%%c",  level,  '\0');
#ifdef SHOW_TIME_DATE_SUPPORT
	draw_time_line();
#endif
#ifdef PICTURE_SHOW_PERCENT_SUPPORT
	draw_text_picture(level);
#else
	draw_text_xy((dy + height),  (gr_fb_width()/2 - 20),  bat);
#endif
    if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL) {
        frame = level * (PROGRESSBAR_INDETERMINATE_STATES - 1) / 100;
        gr_blit(gProgressBarIndeterminate[frame],  0,  0,  width,  height,  dx,  dy);
		 gProgressBarType = PROGRESSBAR_TYPE_INDETERMINATE;
    }

    if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE) {
        gr_blit(gProgressBarIndeterminate[frame],  0,  0,  width,  height,  dx,  dy);
        frame = (frame + 1);
        if (frame >= PROGRESSBAR_INDETERMINATE_STATES) {
            frame = level * (PROGRESSBAR_INDETERMINATE_STATES - 1) / 100;
        }
    }
	gr_flip();
#endif
}

#define LED_GREEN         1
#define LED_RED           2
#define LED_BLUE          3
extern int screen_on_flag;

static void led_control(int level) {
      static int led_flag = 0;

      if (level > 100)
        level = 100;
      else if (level < 0)
        level = 0;
      if (level < 90) {
        if (led_flag != LED_RED) {
            led_on(LED_RED);
            led_flag = LED_RED;
        }
      } else {
         if (led_flag != LED_GREEN) {
             led_on(LED_GREEN);
             led_flag = LED_GREEN;
	}
     }
}

extern int screen_on_flag;

void *charge_thread(void *cookie) {
    int bat_level = 0;
    for (; !is_exit; ) {
	if(thread_ext_ctrl == CHARGE_THREAD_CTRL){
		thread_count++;
		if(thread_count > 5){
			is_exit = 1;
			thread_count = 0;
		}
	}
        usleep(1000000/ PROGRESSBAR_INDETERMINATE_FPS);
        bat_level = battery_capacity();
	if(bat_level < 0){
		thread_st = CHARGE_THREAD_EXIT_ERROR;
	}else{
		thread_st = CHARGE_THREAD_OK;
	}
	pthread_mutex_lock(&gchargeMutex);
	led_control(bat_level);
	status_index = charge_health_check();
	if (screen_on_flag == 1) {
	   draw_progress_locked(bat_level);
	}
	pthread_mutex_unlock(&gchargeMutex);
        usleep(500000);
    }

    usleep(200);
    return NULL;
}

void *power_thread(void *cookie) {
    for (; !is_exit; ) {
	 if(thread_ext_ctrl == POWER_THREAD_CTRL){
                thread_count++;
                if(thread_count > 5){
                        is_exit = 1;
                        thread_count = 0;
                }
        }
        if (battery_ac_online() == 0 && battery_usb_online() == 0) {
            LOGE("charger not present,  power off device\n");
	    thread_st = POWER_THREAD_EXIT_PLUGOUT;
            is_exit = 1;
	    if(thread_ext_ctrl != POWER_THREAD_CTRL)
		reboot(RB_POWER_OFF);
            usleep(200);
        }else{
		thread_st = POWER_THREAD_OK;
	}
	 usleep(500000);
    }
    return NULL;
}

inline long long timeval_diff(struct timeval big,  struct timeval small) {
    return (long long)(big.tv_sec-small.tv_sec)*1000000 + big.tv_usec - small.tv_usec;
}

void  *input_thread(void *write_fd) {
	int ret = 0;
	struct timeval start_time = {0,  0};
	struct timeval now_time = {0,  0};
	struct input_event ev = {0};

	int time_left;
	long long time_diff_temp;

	time_left = BACKLIGHT_ON_MS;
	for (; !is_exit; ) {
		if(thread_ext_ctrl == INPUT_THREAD_CTRL){
			thread_count++;
			if(thread_count > 10){
				is_exit = 1;
				thread_count = 0;
			}
		}
		ret = ev_get(&ev,  POLLING_MS);
		LOGD(" %s: %d,  ret:%d,  ev.type:%d,  ev.code:%d,  ev.value:%d  time_left = %d\n",  __func__,  \
						__LINE__,  ret,  ev.type,  ev.code,  ev.value ,time_left);
key_check:
		if(ret == 0){
			if(ev.code == KEY_POWER){
			    thread_st = INPUT_THREAD_POWERKEY_DOWN;
				pthread_mutex_lock(&gchargeMutex);
				set_screen_state(1);
				pthread_mutex_unlock(&gchargeMutex);
				time_left =  POWER_KEY_TIMEOUT_MS;
				do{
					while (gettimeofday(&start_time,  (struct timezone *)0) < 0) {;}
					ret = ev_get(&ev,  time_left);
					while (gettimeofday(&now_time,  (struct timezone *)0) < 0) {;}
					time_diff_temp = timeval_diff(now_time,  start_time);
					time_diff_temp = (time_diff_temp + 1000)/1000;
					time_left = (int)(time_left - time_diff_temp);

					LOGD(" %s: %d,  ret:%d,  ev.type:%d,  ev.code:%d,  ev.value:%d  time_left = %d\n",  __func__,  \
					__LINE__,  ret,  ev.type,  ev.code,  ev.value ,time_left);

					if((ret == 0) && (ev.code == KEY_POWER) && (ev.value == 0) ){
						thread_st = INPUT_THREAD_POWERKEY_UP;
						LOGD(" %s: %d %s\n",  __func__,  __LINE__,  "power key up found\n");
						usleep(500000);
						backlight_on();
						time_left = BACKLIGHT_ON_MS;
						break;
					}
					if(time_left <= 0){
						thread_st = INPUT_THREAD_POWERKEY_TIMEOUT;
						LOGD(" %s: %d\n",  __func__,  __LINE__);
						is_exit = 1;
						if(thread_ext_ctrl != INPUT_THREAD_CTRL)
							syscall(__NR_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2, "charger");
						usleep(500000);
						LOGD(" %s: %d,  reboot failed\n",  __func__,  __LINE__);
						break;
					}
				}while(time_left > 0);
			}
			if (ev.code == KEY_BRL_DOT8) { /* alarm event happen */
				thread_st = INPUT_THREAD_ALARM;
				if (alarm_flag_check()) {
					set_screen_state(1);
					is_exit = 1;
					LOGD(" %s: %d,  %s\n",  __func__,  __LINE__, "alarm happen 1,  exit");
					syscall(__NR_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2, "alarm");
					usleep(500000);
					LOGD(" %s: %d,  %s\n",  __func__,  __LINE__, "alarm reboot failed");
					break;
				} else {
					backlight_off();
					set_screen_state(0);
				}
            }
		}else{
			thread_st = INPUT_THREAD_OK;
//			while (gettimeofday(&start_time,  (struct timezone *)0) < 0) {;}
			do{
				while (gettimeofday(&start_time,  (struct timezone *)0) < 0) {;}
				ret = ev_get(&ev,  time_left);
				while (gettimeofday(&now_time,  (struct timezone *)0) < 0) {;}
				time_diff_temp = timeval_diff(now_time,  start_time);
				time_diff_temp = (time_diff_temp + 1000)/1000;
				time_left = (int)(time_left - time_diff_temp);

				LOGD(" %s: %d,  ret:%d,  ev.type:%d,  ev.code:%d,  ev.value:%d time_left = %d \n",  __func__,  \
				__LINE__,  ret,  ev.type,  ev.code,  ev.value ,time_left);
				if(ret == 0)
					goto key_check;
				if(time_left <= 0){
					thread_st = INPUT_THREAD_TIMEOUT;
					backlight_off();
					set_screen_state(0);
					time_left = WAKEUP_ON_MS;
					break;
				}
			}while(time_left > 0);
		}
	}
	return NULL;
}

int ui_init(void) {
	int i, result = 0;

	result = gr_init();
	if (result < 0) {
		LOGE("gr_init failed!\n");
		return result;
	}
	result = ev_init();
	if (result < 0) {
		LOGE("ev_init failed!\n");
		return result;
	}
	res_init();

	for (i = 0; BITMAPS[i].name != NULL; ++i) {
		result = res_create_display_surface(BITMAPS[i].name,  BITMAPS[i].surface);
		if (result < 0) {
			if (result == -2) {
				LOGI("Bitmap %s missing header\n",  BITMAPS[i].name);
			} else {
				LOGE("Missing bitmap %s\n(Code %d)\n",  BITMAPS[i].name,  result);
			}
			*BITMAPS[i].surface = NULL;
		}
	}
	return result;
}

void ui_set_background(void) {
    pthread_mutex_lock(&gUpdateMutex);
    gr_sync();
    draw_background_locked(gCurrentIcon);
    gr_flip();
    pthread_mutex_unlock(&gUpdateMutex);
}

int alarm_flag_check(void) {
   char *alarm_name = "/mnt/vendor/alarm_flag";
   char *poweron_name = "/mnt/vendor/poweron_timeinmillis";
   ssize_t ret;
   int alarm_flag_fd;
   int poweron_flag_fd;
   char read_buf[30] = {0};
   char read_buf1[30] = {0};
   char * buf_pos = NULL;
   long alarm_time_millis = 0;
   long alarm_time_millis1 = 0;
   struct timeval now_time = {0};
   extern int errno;

   alarm_flag_fd = open(alarm_name,  O_RDONLY);
   if (alarm_flag_fd >= 0) {
	   ret = read(alarm_flag_fd,  read_buf,  sizeof(read_buf));
	   read_buf[sizeof(read_buf)-2] = '\n';
	   read_buf[sizeof(read_buf)-1] = '\0';
	   if (ret >= 0 && read_buf[0]!= 0xff) {
		   LOGD("%s get: %s\n",  alarm_name,  read_buf);
		   buf_pos = strstr(read_buf,  "\n");
		   buf_pos += 1;
		   alarm_time_millis = strtoul(buf_pos,  (char **)NULL,  10);
		   LOGD("trans to %lu\n",  alarm_time_millis);
	   }
	   close(alarm_flag_fd);
   } else
	   LOGE("open %s failed errno=%d(%s)\n", alarm_name, errno, strerror(errno));

   poweron_flag_fd = open(poweron_name,  O_RDONLY);
   if (poweron_flag_fd >= 0) {
	   ret = read(poweron_flag_fd,  read_buf1,  sizeof(read_buf1));
	   read_buf1[sizeof(read_buf1)-2] = '\n';
	   read_buf1[sizeof(read_buf1)-1] = '\0';
	   if (ret >= 0 && read_buf1[0]!= 0xff) {
		   LOGD("%s get: %s\n",  poweron_name,  read_buf1);
		   buf_pos = strstr(read_buf1,  "\n");
		   buf_pos += 1;
		   alarm_time_millis1 = strtoul(buf_pos,  (char **)NULL,  10);
		   LOGD("trans to %lu\n",  alarm_time_millis1);
	   }
	   close(poweron_flag_fd);
   } else
	   LOGE("open %s failed errno=%d(%s)\n", poweron_name, errno, strerror(errno));

   while (gettimeofday(&now_time,  (struct timezone *)0) < 0) {;}
   LOGD("now time %lu\n",  now_time.tv_sec);
   alarm_time_millis = alarm_time_millis - now_time.tv_sec;
   alarm_time_millis1 = alarm_time_millis1 - now_time.tv_sec;
   if ((alarm_time_millis > -20 && alarm_time_millis < 180) || (alarm_time_millis1 > -20 && alarm_time_millis1 < 180))
	   return 1;
   return 0;
}

// 新增：圆环动画宏开关
#define CIRCLE_CHARGE_UI_SUPPORT 1
// 画圆环进度条
static void draw_circle_progress(int cx, int cy, int radius, float percent, uint32_t color, int thickness) {
    float start_angle = -90.0f; // 从正上方开始
    float end_angle = start_angle + 360.0f * percent;
    float step = 2.0f; // 每2度画一段
    for (float angle = start_angle; angle < end_angle; angle += step) {
        float rad1 = angle * M_PI / 180.0f;
        float rad2 = (angle + step) * M_PI / 180.0f;
        int x1 = cx + (int)(radius * cos(rad1));
        int y1 = cy + (int)(radius * sin(rad1));
        int x2 = cx + (int)(radius * cos(rad2));
        int y2 = cy + (int)(radius * sin(rad2));
        gr_color((color>>16)&0xFF, (color>>8)&0xFF, color&0xFF, 255);
        gr_thick_line(x1, y1, x2, y2, thickness);
    }
}

// 新圆环UI
static void draw_circle_charge_ui(int percent, int is_fast_charging) {
    int center_x = gr_fb_width() / 2;
    int center_y = gr_fb_height() / 2 - 40;
    int radius = 120;
    int thickness = 12;
    uint32_t green = 0x00FF00; // RGB绿色
    // 1. 灰色底环
    draw_circle_progress(center_x, center_y, radius, 1.0, 0x444444, thickness);
    // 2. 绿色进度环
    draw_circle_progress(center_x, center_y, radius, percent / 100.0f, green, thickness);
    // 3. 闪电图标
    const char* icon = is_fast_charging ? "charge/images/lightning_double.png" : "charge/images/lightning_single.png";
    gr_surface lightning = NULL;
    if (res_create_display_surface(icon, &lightning) == 0 && lightning) {
        int icon_w = gr_get_width(lightning);
        int icon_h = gr_get_height(lightning);
        gr_blit(lightning, 0, 0, icon_w, icon_h, center_x - icon_w/2, center_y - icon_h/2);
        res_free_surface(lightning);
    }
    // 4. 百分比和状态
    char text[32];
    if (is_fast_charging)
        snprintf(text, sizeof(text), "%d%% 快充中", percent);
    else
        snprintf(text, sizeof(text), "%d%% 充电中", percent);
    int text_w = gr_measure(text);
    int text_x = center_x - text_w / 2;
    int text_y = center_y + radius + 32;
    gr_color(255, 255, 255, 255);
    gr_text(text_x, text_y, text, 0);
}
