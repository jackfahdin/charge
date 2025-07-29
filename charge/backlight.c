/********************************************************************************
**  Copyright:  2016 Spreadtrum, Incorporated. All Rights Reserved.
*********************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"

#define LCD_BACKLIGHT_DEV            "/sys/class/backlight/sprd_backlight/brightness"
#define LCD_BACKLIGHT_MAX_DEV        "/sys/class/backlight/sprd_backlight/max_brightness"
/* TODO fix the following definition to control keyboard backlight
 * then add HAVE_KEYBOARD_BACKLIGHT in BoardConfig.mk to enable them
 * */
#define KEY_BACKLIGHT_DEV             "/sys/class/leds/keyboard-backlight/brightness"
#define KEY_BACKLIGHT_MAX_DEV         "/sys/class/leds/keyboard-backlight/max_brightness"

#define LED_GREEN_DEV                   "/sys/class/leds/sc27xx:green/brightness"
#define LED_GREEN_MAX_DEV               "/sys/class/leds/sc27xx:green/max_brightness"
#define LED_RED_DEV                     "/sys/class/leds/sc27xx:red/brightness"
#define LED_RED_MAX_DEV                 "/sys/class/leds/sc27xx:red/max_brightness"
#define LED_BLUE_DEV                    "/sys/class/leds/sc27xx:blue/brightness"
#define LED_BLUE_MAX_DEV                "/sys/class/leds/sc27xx:blue/max_brightness"

#define SPRD_DBG(...) LOGE(__VA_ARGS__)
static int max_lcd,  max_key, max_green_led, max_red_led;
/*
 *Set LCD backlight brightness level
 */
static int eng_lcdbacklight_test(int brightness) {
    int fd;
    ssize_t ret;
    char buffer[8];

    fd = open(LCD_BACKLIGHT_DEV,  O_RDWR);

    if (fd < 0) {
        SPRD_DBG("%s: open %s fail", __func__,  LCD_BACKLIGHT_DEV);
        return -1;
    }

    memset(buffer,  0,  sizeof(buffer));
    sprintf(buffer,  "%d",  brightness);
    ret = write(fd,  buffer,  strlen(buffer));

    close(fd);

    return 0;
}
static int eng_lcdbacklight_get(void) {
    int fd;
    ssize_t ret;
    char buffer[8];

    fd = open(LCD_BACKLIGHT_DEV,  O_RDWR);

    if (fd < 0) {
        SPRD_DBG("%s: open %s fail", __func__,  LCD_BACKLIGHT_DEV);
        return -1;
    }

    memset(buffer,  0,  sizeof(buffer));
    ret = read(fd,  buffer,  8);
    close(fd);
    buffer[7]='\0';
    LOGE("lcd brightness: %s\n",  buffer);

    return 0;
}

/*
 *Set LED backlight brightness level
 */
#ifdef K_BACKLIGHT
static int eng_keybacklight_test(int brightness) {
    int fd;
    ssize_t ret;
    char buffer[8];

    fd = open(KEY_BACKLIGHT_DEV,  O_RDWR);

    if (fd < 0) {
        SPRD_DBG("%s: open %s fail", __func__,  KEY_BACKLIGHT_DEV);
        return -1;
    }

    memset(buffer,  0,  sizeof(buffer));
    sprintf(buffer,  "%d",  brightness);
    ret = write(fd,  buffer,  strlen(buffer));

    close(fd);

    return 0;
}
static int eng_keybacklight_get(void) {
    int fd;
    ssize_t ret;
    char buffer[8];

    fd = open(KEY_BACKLIGHT_DEV,  O_RDWR);

    if (fd < 0) {
        SPRD_DBG("%s: open %s fail", __func__,  LCD_BACKLIGHT_DEV);
        return -1;
    }

    memset(buffer,  0,  sizeof(buffer));
    ret = read(fd,  buffer,  8);
    close(fd);
    buffer[7]='\0';
    LOGE("key brightness: %s \n",  buffer);

    return 0;
}
#endif

int backlight_on(void) {
	if(eng_lcdbacklight_test(max_lcd/2) < 0){
		LOGE("%s: %d fail", __func__, __LINE__);
		return -1;
	}
	if(eng_lcdbacklight_get() < 0){
		LOGE("%s: %d fail", __func__, __LINE__);
                return -1;
        }
#ifdef K_BACKLIGHT
	if(eng_keybacklight_test(max_key/2) < 0){
		LOGE("%s: %d fail", __func__, __LINE__);
                return -1;
        }
	if(eng_keybacklight_get() < 0){
		LOGE("%s: %d fail", __func__, __LINE__);
                return -1;
        }
#endif
	return 1;
}

int backlight_off(void) {
	int ret = 0;
	if(status_index > 0){
		return ret;
	}
	if(eng_lcdbacklight_test(0) < 0){
		LOGE("%s: %d fail", __func__, __LINE__);
                return -1;
        }
#ifdef K_BACKLIGHT
	if(eng_keybacklight_test(0) < 0){
		LOGE("%s: %d fail", __func__, __LINE__);
                return -1;
        }
#endif
	return 1;
}

static int eng_led_green_test(int brightness) {
        int fd;
        ssize_t ret;
        char buffer[8];

        fd = open(LED_GREEN_DEV, O_RDWR);
        if (fd < 0) {
                SPRD_DBG("%s: open %s fail", __func__, LED_GREEN_DEV);
		return -1;
	}

        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "%d", brightness);
        ret = write(fd, buffer, strlen(buffer));

        close(fd);

        return 0;
}
static int eng_led_red_test(int brightness) {
        int fd;
        ssize_t ret;
        char buffer[8];

        fd = open(LED_RED_DEV, O_RDWR);
        if (fd < 0) {
		SPRD_DBG("%s: open %s fail", __func__, LED_RED_DEV);
		return -1;
	}

        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "%d", brightness);
        ret = write(fd, buffer, strlen(buffer));

        close(fd);

        return 0;
}

static int eng_led_blue_test(int brightness) {
        int fd;
        ssize_t ret;
        char buffer[8];

        fd = open(LED_BLUE_DEV, O_RDWR);

        if (fd < 0) {
		SPRD_DBG("%s: open %s fail", __func__, LED_BLUE_DEV);
		return -1;
        }

        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "%d", brightness);
        ret = write(fd, buffer, strlen(buffer));

        close(fd);

        return 0;
}
int led_off(void) {
	if(eng_led_green_test(0) < 0){
		LOGE("%s: %d fail", __func__, __LINE__);
		return -1;
	}
	if(eng_led_green_test(0) < 0){
                LOGE("%s: %d fail", __func__, __LINE__);
                return -1;
        }
	if(eng_led_green_test(0) < 0){
                LOGE("%s: %d fail", __func__, __LINE__);
                return -1;
        }
	return 1;
}

int led_on(int color) {
    int ret = 0;
    if (color == 1) {
        if(eng_led_green_test(max_green_led/2) < 0){
	    LOGE("%s: %d fail", __func__, __LINE__);
	    return -1;
	}
	if(eng_led_red_test(0) < 0){
	    LOGE("%s: %d fail", __func__, __LINE__);
	    return -1;
	}
	if(eng_led_blue_test(0) < 0){
	    LOGE("%s: %d fail", __func__, __LINE__);
	    return -1;
	}
    } else if (color == 2) {
	if(eng_led_red_test(max_red_led/2) < 0){
	    LOGE("%s: %d fail", __func__, __LINE__);
	    return -1;
        }
	if(eng_led_green_test(0) < 0){
	    LOGE("%s: %d fail", __func__, __LINE__);
	    return -1;
	}
	if(eng_led_blue_test(0) < 0){
	    LOGE("%s: %d fail", __func__, __LINE__);
	    return -1;
	}
    } else if (color == 3) {
	if(eng_led_blue_test(0) < 0){
	    LOGE("%s: %d fail", __func__, __LINE__);
	    return -1;
	}
	if(eng_led_red_test(max_green_led/2) < 0){
	    LOGE("%s: %d fail", __func__, __LINE__);
	    return -1;
	}
	if(eng_led_green_test(max_red_led/2) < 0){
	    LOGE("%s: %d fail", __func__, __LINE__);
	    return -1;
	}
    } else {
	SPRD_DBG("%s: color is %d invalid\n", __func__, color);
	ret = 2;
    }
	return ret;
}


void backlight_init(void) {
    int fd;
    ssize_t ret;
    char buffer[8];

    max_lcd = 0;
    max_key = 0;
    max_green_led = 0;
    max_red_led = 0;

    fd = open(LCD_BACKLIGHT_MAX_DEV,  O_RDONLY);
    if (fd < 0) {
        SPRD_DBG("%s: open %s fail", __func__,  LCD_BACKLIGHT_MAX_DEV);
    } else {
        memset(buffer,  0,  sizeof(buffer));
        ret = read(fd,  buffer,  sizeof(buffer));
        max_lcd = atoi(buffer);
        if (ret < 0) {
            SPRD_DBG("%s: read %s fail", __func__,  LCD_BACKLIGHT_MAX_DEV);
        }
        close(fd);
    }
    fd = open(LED_GREEN_MAX_DEV, O_RDONLY);
    if (fd < 0) {
        SPRD_DBG("%s: open %s fail", __func__, LED_GREEN_MAX_DEV);
    } else {
        memset(buffer, 0, sizeof(buffer));
        ret = read(fd, buffer, sizeof(buffer));
        max_green_led = atoi(buffer);
        if (ret < 0) {
		SPRD_DBG("%s: read %s fail", __func__,LED_GREEN_MAX_DEV);
        }
        close(fd);
    }
    fd = open(LED_RED_MAX_DEV, O_RDONLY);
    if (fd < 0) {
        SPRD_DBG("%s: open %s fail", __func__, LED_RED_MAX_DEV);
    } else {
         memset(buffer, 0, sizeof(buffer));
         ret = read(fd, buffer, sizeof(buffer));
         max_red_led = atoi(buffer);
         if (ret < 0) {
             SPRD_DBG("%s: read %s or fail", __func__, LED_RED_MAX_DEV);
          }
          close(fd);
    }

#ifdef K_BACKLIGHT
    fd = open(KEY_BACKLIGHT_MAX_DEV,  O_RDONLY);
    if (fd < 0) {
        SPRD_DBG("%s: open %s fail", __func__,  KEY_BACKLIGHT_MAX_DEV);
    } else {
        memset(buffer,  0,  sizeof(buffer));
        ret = read(fd,  buffer,  sizeof(buffer));
        max_key = atoi(buffer);
        if (ret < 0) {
            SPRD_DBG("%s: read %s fail", __func__,  KEY_BACKLIGHT_MAX_DEV);
        }
        close(fd);
    }
#endif

    SPRD_DBG("%s max_lcd=%d; max_key=%d", __func__,  max_lcd,  max_key);
}

