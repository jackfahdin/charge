/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include "battery.h"

#define PROGRESSBAR_INDETERMINATE_STATES 7
#define PROGRESSBAR_INDETERMINATE_FPS 8

#define BACKLIGHT_ON_MS 6000
#define WAKEUP_ON_MS 2000
#define POWER_KEY_TIMEOUT_MS 1500
#define POLLING_MS 100

#ifdef __cplusplus
extern "C"
{
#endif
void *charge_thread(void *cookie);
void *power_thread(void *cookie);
void *input_thread(void *write_fd);
// Initialize the graphics system.
int ui_init(void);
// Initialize thr rtc
ssize_t validate_rtc_time(void);

// Use KEY_* codes from <linux/input.h> or KEY_DREAM_* from "minui/minui.h".
int ui_wait_key(void);            // waits for a key/button press, returns the code
int ui_key_pressed(int key);  // returns >0 if the code is currently pressed
int ui_text_visible(void);        // returns >0 if text log is currently visible
void ui_clear_key_queue(void);

// Write a message to the on-screen log shown with Alt-L (also to stderr).
// The screen is small, and users may need to report these messages to support,
// so keep the output short and not too cryptic.
void ui_print(const char *fmt, ...);

// Display some header text followed by a menu of items, which appears
// at the top of the screen (in place of any scrolling ui_print()
// output, if necessary).
void ui_start_menu(char** headers, char** items);
// Set the menu highlight to the given index, and return it (capped to
// the range [0..numitems).
int ui_menu_select(int sel);
// End menu mode, resetting the text overlay so that ui_print()
// statements will be displayed.
void ui_end_menu(void);
void ui_set_background(void);

// Show a progress bar and define the scope of the next operation:
//   portion - fraction of the progress bar the next operation will use
//   seconds - expected time interval (progress bar moves at this minimum rate)
void ui_show_progress(float portion, int seconds);
void ui_set_progress(float fraction);  // 0.0 - 1.0 within the defined scope

// Show a rotating "barberpole" for ongoing operations.  Updates automatically.
void ui_show_indeterminate_progress(void);
int alarm_flag_check(void);
int get_power_status(void);

// Hide and reset the progress bar.
void ui_reset_progress(void);
void backlight_init(void);
int backlight_on(void);
int backlight_off(void);
int led_off(void);
int led_on(int color);

int set_screen_state(int);

int autosuspend_enable(void);
int autosuspend_disable(void);
void log_write(int level, const char *fmt, ...)
            __attribute__((format(printf, 2, 3)));
#ifdef __cplusplus
}
#endif
// Set the icon (normally the only thing visible besides the progress bar).
enum {
  BACKGROUND_ICON_NONE,
  BACKGROUND_ICON_INSTALLING,
  BACKGROUND_ICON_ERROR,
  NUM_BACKGROUND_ICONS
};

enum thread_status{
	CHARGE_THREAD_EXIT_ERROR,
	CHARGE_THREAD_OK,
	POWER_THREAD_EXIT_PLUGOUT,
	POWER_THREAD_OK,
	INPUT_THREAD_POWERKEY_DOWN,
	INPUT_THREAD_POWERKEY_UP,
	INPUT_THREAD_POWERKEY_TIMEOUT,
	INPUT_THREAD_ALARM,
	INPUT_THREAD_TIMEOUT,
	INPUT_THREAD_OK,
}thread_st;

enum thread_ctrl{
	THREAD_DEFAULT,
	CHARGE_THREAD_CTRL,
	POWER_THREAD_CTRL,
	INPUT_THREAD_CTRL,
};

enum pixel{
        SIZE_360X640,
        SIZE_480X800,
        SIZE_720X1280,
        SIZE_1080X1920,
        SIZE_1440X2560,
};
int is_exit;
int status_index;
//enum thread_status thread_ext_ctrl;
int thread_ext_ctrl;
int thread_count;


// Default allocation of progress bar segments to operations
static const int VERIFICATION_PROGRESS_TIME = 60;
static const float VERIFICATION_PROGRESS_FRACTION = 0.25;
static const float DEFAULT_FILES_PROGRESS_FRACTION = 0.4;
static const float DEFAULT_IMAGE_PROGRESS_FRACTION = 0.1;
#define LOGE(x...)    log_write(3, "<3>charge: " x)
#define LOGW(x...)    log_write(5, "<5>charge: " x)
#define LOGI(x...)    log_write(6, "<6>charge: " x)
#define LOGD(x...)    log_write(6, "<6>charge: " x)
#define LOGV(x...)    log_write(6, "<6>charge: " x)


#define STRINGIFY(x) #x
#define EXPAND(x) STRINGIFY(x)

#endif  // COMMON_H_
